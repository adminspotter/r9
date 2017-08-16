/* stream.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 14 Aug 2017, 10:31:11 tquirk
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

stream_user::stream_user(uint64_t u, GameObject *g, listen_socket *l)
    : base_user(u, g, l)
{
    this->fd = 0;
}

const stream_user& stream_user::operator=(const stream_user& su)
{
    this->fd = su.fd;
    this->base_user::operator=(su);
    return *this;
}

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

void stream_socket::do_login(uint64_t userid,
                             Control *con,
                             access_list& al)
{
    stream_user *stu = new stream_user(userid, NULL, this);
    stu->fd = al.what.login.who.stream;
    this->users[userid] = stu;
    this->user_fds[userid] = al.what.login.who.stream;

    this->connect_user((base_user *)stu, al);
}

/* The do_logout method performs the only stream_socket-specific work
 * for removing a stream user from the object.  Everything else is
 * handled in listen_socket::reaper_worker.
 */
void stream_socket::do_logout(base_user *bu)
{
    stream_user *stu = dynamic_cast<stream_user *>(bu);

    if (stu != NULL)
    {
        close(stu->fd);
        if (stu->fd + 1 == this->max_fd)
            --this->max_fd;
        this->fds.erase(stu->fd);
        this->user_fds.erase(stu->userid);
    }
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
    struct linger ls;
    int fd, which;

    if (FD_ISSET(this->sock.sock, &this->readfs)
        && (fd = accept(this->sock.sock,
                        (struct sockaddr *)&sin, &slen)) > 0)
    {
        ls.l_onoff = (config.use_linger > 0);
        ls.l_linger = config.use_linger;
        setsockopt(fd, SOL_SOCKET, SO_LINGER,
                   &ls, sizeof(struct linger));
        setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                   &config.use_keepalive, sizeof(int));
        ioctl(fd, FIONBIO, &config.use_nonblock);
        this->fds[fd] = NULL;
        FD_SET(fd, &this->master_readfs);
        this->max_fd = std::max(this->max_fd, fd + 1);
    }
}

void stream_socket::handle_users(void)
{
    int len;
    unsigned char buf[1024];

    for (auto i = this->fds.begin(); i != this->fds.end(); ++i)
        if (FD_ISSET((*i).first, &this->readfs))
        {
            if ((len = read((*i).first,
                            buf,
                            sizeof(buf))) > 0)
            {
                /* We have recieved something. */
                std::clog << "got something" << std::endl;

                /* The first sizeof(int) bytes will be the socket
                 * descriptor, and the rest of it will be the actual
                 * client data that we got.
                 */
            }
            else
            {
                /* A 0 indicates that the other end closed the socket. */
                auto j = this->users.find((*i).second->userid);
                (*i).second = NULL;
                this->fds.erase(i--);
                this->users.erase(j);
            }
        }
}

void *stream_socket::stream_send_worker(void *arg)
{
    stream_socket *sts = (stream_socket *)arg;
    stream_user *stu;
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
            stu = dynamic_cast<stream_user *>(sts->users[req.who->userid]);
            if (stu == NULL)
                continue;

            fd = sts->user_fds[stu->userid];
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
