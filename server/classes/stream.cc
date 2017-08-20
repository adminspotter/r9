/* stream.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 Aug 2017, 11:04:35 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2017  Trinity Annabelle Quirk
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
#if HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#if HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "stream.h"
#include "game_obj.h"

#include "../config_data.h"
#include "../log.h"

extern volatile int main_loop_exit_flag;

static std::map<int, listen_socket::packet_handler> packet_handlers =
{
    { TYPE_LOGREQ, stream_socket::handle_login },
    { TYPE_ACKPKT, listen_socket::handle_ack },
    { TYPE_ACTREQ, listen_socket::handle_action },
    { TYPE_LGTREQ, listen_socket::handle_logout }
};

stream_socket::stream_socket(struct addrinfo *ai)
    : listen_socket(ai), fds()
{
    FD_ZERO(&this->master_readfs);
    FD_SET(this->sock.sock, &this->master_readfs);
    this->max_fd = this->sock.sock + 1;
}

stream_socket::~stream_socket()
{
    for (auto i = this->fds.begin(); i != this->fds.end(); ++i)
        close((*i).first);
    this->fds.clear();

    /* Thread pools and users are handled by the listen_socket destructor */
}

std::string stream_socket::port_type(void)
{
    return "stream";
}

void stream_socket::start(void)
{
    this->listen_socket::start();

    /* Start up the sending thread pool */
    this->send_pool->startup_arg = (void *)this;
    this->send_pool->start(stream_socket::stream_send_worker);
    this->sock.listen_arg = (void *)this;
    this->sock.start(stream_socket::stream_listen_worker);
}

void stream_socket::handle_packet(packet& p, int fd)
{
    base_user *bu = NULL;
    auto handler = packet_handlers.find(p.basic.type);
    auto found = this->fds.find(fd);

    if (found != this->fds.end())
        bu = found->second;

    if (handler != packet_handlers.end())
        (handler->second)(this, p, bu, &fd);
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
    this->fds[al.what.login.who.stream] = bu;
    this->user_fds[bu->userid] = al.what.login.who.stream;

    this->listen_socket::connect_user(bu, al);
}

void stream_socket::disconnect_user(base_user *bu)
{
    int fd = this->user_fds[bu->userid];
    close(fd);
    FD_CLR(fd, &this->master_readfs);
    if (fd + 1 == this->max_fd)
        --this->max_fd;
    this->fds[fd] = NULL;
    this->fds.erase(fd);
    this->user_fds.erase(bu->userid);

    this->listen_socket::disconnect_user(bu);
}

void *stream_socket::stream_listen_worker(void *arg)
{
    stream_socket *sts = (stream_socket *)arg;

    for (;;)
    {
        if (main_loop_exit_flag == 1)
            break;

        pthread_testcancel();
        if (sts->select_fd_set() < 1)
            continue;
        pthread_testcancel();

        sts->accept_new_connection();
        sts->handle_users();
    }
    std::clog << "exiting connection loop for stream port "
              << sts->sock.sa->port() << std::endl;
    return NULL;
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
                      << this->sock.sa->port() << std::endl;
        }
        else
        {
            std::clog << syslogErr
                      << "select error in stream port "
                      << this->sock.sa->port() << ": "
                      << strerror(errno) << " (" << errno << ")"
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

    if (FD_ISSET(this->sock.sock, &this->readfs)
        && (fd = accept(this->sock.sock,
                        (struct sockaddr *)&sin, &slen)) > 0)
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

    for (auto i = this->fds.begin(); i != this->fds.end(); ++i)
        if (FD_ISSET((*i).first, &this->readfs))
        {
            memset((char *)&buf, 0, sizeof(packet));
            if ((len = read((*i).first,
                            (void *)&buf,
                            sizeof(buf))) > 0)
            {
                if (!ntoh_packet(&buf, len))
                    continue;

                this->handle_packet(buf, (*i).first);
            }
            else
            {
                /* It's either that the other end closed the socket,
                 * or we have an error; either way, we should drop
                 * this socket.  Prevent the select from actually
                 * looking at it, and have the reaper take care of the
                 * rest of things.
                 */
                FD_CLR((*i).first, &this->master_readfs);
                (*i).second->pending_logout = true;
            }
        }
}

void *stream_socket::stream_send_worker(void *arg)
{
    stream_socket *sts = (stream_socket *)arg;
    base_user *bu;
    int fd;
    packet_list req;
    size_t realsize;

    std::clog << "started send pool worker for stream port "
              << sts->sock.sa->port() << std::endl;
    for (;;)
    {
        sts->send_pool->pop(&req);

        realsize = packet_size(&req.buf);
        if (hton_packet(&req.buf, realsize))
        {
            fd = sts->user_fds[bu->userid];
            /* TODO: Encryption */
            if (write(fd, (void *)&req, realsize) == -1)
                std::clog << syslogErr
                          << "error sending packet out stream port "
                          << sts->sock.sa->port() << ", user port "
                          << fd << ": "
                          << strerror(errno) << " (" << errno << ")"
                          << std::endl;
        }
    }
    std::clog << "exiting send pool worker for stream port "
              << sts->sock.sa->port() << std::endl;
    return NULL;
}
