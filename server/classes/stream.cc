/* stream.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2007-2026  Trinity Annabelle Quirk
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 *
 * This file contains the implementation of the stream server socket.
 *
 * Things to do
 *
 */

#include <config.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "stream.h"
#include "game_obj.h"

#include "config_data.h"
#include "log.h"

static std::map<int, listen_socket::packet_handler> packet_handlers =
{
    { TYPE_LOGREQ, stream_socket::handle_login },
    { TYPE_ACKPKT, listen_socket::handle_ack },
    { TYPE_ACTREQ, listen_socket::handle_action },
    { TYPE_LGTREQ, listen_socket::handle_logout }
};

stream_socket::stream_socket()
    : listen_socket(), fds()
{
    this->init();
}

void stream_socket::init(void)
{
    FD_ZERO(&this->master_readfs);
    this->port_type = "stream";
}

stream_socket::stream_socket(Addrinfo *ai)
    : listen_socket(ai), fds()
{
    this->init();
}

stream_socket::~stream_socket()
{
    /* Thread pools, listen sockets, and users are handled by the
     * listen_socket destructor.
     */
}

void stream_socket::start(void)
{
    this->listen_socket::start();

    FD_SET(this->sock, &this->master_readfs);
    this->max_fd = this->sock + 1;

    this->send_pool->start(stream_socket::stream_send_worker, (void *)this);
    this->basesock::start(stream_socket::stream_listen_worker, (void *)this);
}

void stream_socket::stop(void)
{
    FD_ZERO(&this->readfs);
    this->basesock::stop();
    for (auto& fd : this->fds)
        close(fd.first);
    this->fds.clear();
    /* Users are deleted in the basesock stop method.  All user
     * pointers in this->user_fds are invalid at this point.
     */
    this->user_fds.clear();
}

void stream_socket::handle_packet(packet& p, int len, int fd)
{
    base_user *user = NULL;

    if (packet_handlers.find(p.basic.type) != packet_handlers.end())
    {
        std::shared_lock lock(this->user_mutex);
        if (this->fds.find(fd) != this->fds.end())
        {
            user = this->fds[fd];
            if (!user->decrypt_packet(p))
                return;
        }
        if (!ntoh_packet(&p, len))
            return;
        packet_handlers[p.basic.type](this, p, user, &fd);
    }
}

void stream_socket::handle_login(listen_socket *s, packet& p,
                                 base_user *u, void *fdp)
{
    access_list al;

    memcpy(&al.buf, &p, sizeof(login_request));
    al.what.login.who.stream = *((int *)fdp);
    s->access_pool->push(al);
}

void stream_socket::connect_user(base_user *bu, access_list& al)
{
    {
        std::unique_lock lock(this->user_mutex);
        this->fds[al.what.login.who.stream] = bu;
        this->user_fds[bu->userid] = al.what.login.who.stream;
    }

    this->listen_socket::connect_user(bu, al);
}

void stream_socket::disconnect_user(base_user *bu)
{
    {
        std::unique_lock lock(this->user_mutex);
        if (this->user_fds.find(bu->userid) != this->user_fds.end())
        {
            int fd = this->user_fds[bu->userid];
            close(fd);
            FD_CLR(fd, &this->master_readfs);
            if (fd + 1 == this->max_fd)
                --this->max_fd;
            this->fds.erase(fd);
            this->user_fds.erase(bu->userid);
        }
    }

    this->listen_socket::disconnect_user(bu);
}

void stream_socket::stream_listen_worker(void *arg)
{
    stream_socket *sts = (stream_socket *)arg;

    for (;;)
    {
        if (sts->exit_flag)
            break;

        if (sts->select_fd_set() < 1)
            continue;

        sts->accept_new_connection();
        sts->handle_users();
    }
    std::clog << "exiting connection loop for stream port "
              << sts->sa->port() << std::endl;
}

int stream_socket::select_fd_set(void)
{
    int retval;

    memcpy(&this->readfs, &this->master_readfs, sizeof(fd_set));

    if ((retval = select(this->max_fd, &this->readfs,
                         NULL, NULL, NULL)) < 1)
        /* We have no timeout, so a zero or negative return indicates
         * actual errors.  We'll short-circuit the rest of the connect
         * loop by clearing the descriptor set.
         */
        FD_ZERO(&this->readfs);

    if (retval == -1)
    {
        if (errno == EINTR)
        {
            std::clog << syslogNotice
                      << "select interrupted by signal in stream port "
                      << this->sa->port() << std::endl;
        }
        else
        {
            char err[128];

            std::clog << syslogErr
                      << "select error in stream port "
                      << this->sa->port() << ": "
                      << strerror_r(errno, err, sizeof(err))
                      << " (" << errno << ")"
                      << std::endl;
        }
    }
    return retval;
}

void stream_socket::accept_new_connection(void)
{
    struct sockaddr_in sin;
    socklen_t slen;
    int fd;

    if (this->exit_flag)
        return;

    if (FD_ISSET(this->sock, &this->readfs)
        && (fd = accept(this->sock, (struct sockaddr *)&sin, &slen)) > 0)
    {
        struct linger ls;
        int keepalive = (config.use_keepalive == true ? 1 : 0);
        int nonblock = (config.use_nonblock == true ? 1 : 0);

        ls.l_onoff = (config.use_linger > 0);
        ls.l_linger = config.use_linger;
        setsockopt(fd, SOL_SOCKET, SO_LINGER,
                   &ls, sizeof(struct linger));
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                   &keepalive, sizeof(int));
        ioctl(fd, FIONBIO, &nonblock);
        this->fds[fd] = NULL;
        FD_SET(fd, &this->master_readfs);
        this->max_fd = std::max(this->max_fd, fd + 1);
    }
}

void stream_socket::handle_users(void)
{
    int len;
    packet buf;

    for (auto& fd : this->fds)
    {
        if (this->exit_flag)
            return;

        if (FD_ISSET(fd.first, &this->readfs))
        {
            memset((char *)&buf, 0, sizeof(packet));
            if ((len = read(fd.first,
                            (void *)&buf,
                            sizeof(buf))) > 0)
                this->handle_packet(buf, len, fd.first);
            else
            {
                /* It's either that the other end closed the socket,
                 * or we have an error; either way, we should drop
                 * this socket.  Prevent the select from actually
                 * looking at it, and have the reaper take care of the
                 * rest of things.
                 */
                FD_CLR(fd.first, &this->master_readfs);
                fd.second->pending_logout = true;
            }
        }

        if (this->exit_flag)
            return;
    }
}

void stream_socket::stream_send_worker(void *arg)
{
    stream_socket *sts = (stream_socket *)arg;
    packet_list req;
    size_t realsize;

    std::clog << "started send pool worker for stream port "
              << sts->sa->port() << std::endl;
    for (;;)
    {
        if (!sts->send_pool->pop(&req))
            break;

        realsize = packet_size(&req.buf);

        std::shared_lock lock(sts->user_mutex);
        if (sts->user_fds.find(req.who->userid) != sts->user_fds.end()
            && hton_packet(&req.buf, realsize)
            && req.who->encrypt_packet(req.buf))
        {
            if (write(sts->user_fds[req.who->userid],
                      (void *)&req, realsize) == -1)
            {
                char err[128];

                std::clog << syslogErr
                          << "error sending packet out stream port "
                          << sts->sa->port() << ", user port "
                          << sts->user_fds[req.who->userid] << ": "
                          << strerror_r(errno, err, sizeof(err))
                          << " (" << errno << ")"
                          << std::endl;
            }
        }
    }
    std::clog << "exiting send pool worker for stream port "
              << sts->sa->port() << std::endl;
}
