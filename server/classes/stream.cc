/* stream.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 18 Jul 2017, 07:54:55 tquirk
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

stream_user::stream_user(uint64_t u, Control *c)
    : base_user(u, c)
{
    this->subsrv = 0;
    this->fd = 0;
}

const stream_user& stream_user::operator=(const stream_user& su)
{
    this->subsrv = su.subsrv;
    this->fd = su.fd;
    this->base_user::operator=(su);
    return *this;
}

const stream_socket::subserver& stream_socket::subserver::operator=(const stream_socket::subserver& ss)
{
    this->sock = ss.sock;
    this->pid = ss.pid;
    this->connections = ss.connections;
    return *this;
}

stream_socket::stream_socket(struct addrinfo *ai)
    : listen_socket(ai), subservers()
{
    FD_ZERO(&(this->master_readfs));
    FD_SET(this->sock.sock, &(this->master_readfs));
    this->max_fd = this->sock.sock + 1;
}

int stream_socket::create_subserver(void)
{
    int i, fd[2], pid, retval = -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == -1)
    {
        std::clog << syslogErr
                  << "couldn't create subserver sockets for stream port "
                  << this->sock.sa->port() << ": "
                  << strerror(errno) << " (" << errno << ")" << std::endl;
        return retval;
    }

    /* We want the port number available to the child, for error reporting */
    i = this->sock.sa->port();

    if ((pid = fork()) < 0)
    {
        std::clog << syslogErr
                  << "couldn't fork new subserver for stream port "
                  << this->sock.sa->port() << ": "
                  << strerror(errno) << " (" << errno << ")" << std::endl;
        close(fd[0]);
        close(fd[1]);
    }
    else if (pid == 0)
    {
        /* We are the child; close all sockets except for the one
         * which connects us to the main server.  We call closelog
         * explicitly because it doesn't seem to work otherwise.
         */
        struct rlimit rlim;

        if (getrlimit(RLIMIT_NOFILE, &rlim) != 0)
        {
            std::clog << syslogErr
                      << "can't get rlimit in child " << getpid()
                      << " for stream port " << i << ": "
                      << strerror(errno) << " (" << errno << ")" << std::endl;
            std::clog << syslogErr << "terminating child " << getpid()
                      << " for stream port " << i << std::endl;
            _exit(1);
        }

        /* Close all the open files except for our server
         * communication socket.
         */
        closelog();
        dup2(fd[1], STDIN_FILENO);
        for (i = 0; i < (int)rlim.rlim_cur; ++i)
            if (i != STDIN_FILENO)
                close(i);

        /* This call will never return */
        execl(SUBSERVER_PROG, "r9subserver", NULL);

        /* And in case it does... */
        close(STDIN_FILENO);
        _exit(0);
    }
    else
    {
        /* We are the parent */
        stream_socket::subserver new_sub;

        close(fd[1]);
        new_sub.sock = fd[0];
        new_sub.pid = pid;
        new_sub.connections = 0;
        this->max_fd = std::max(new_sub.sock + 1, this->max_fd);
        this->subservers.push_back(new_sub);
        retval = this->subservers.size() - 1;
        std::clog << "created subserver " << retval
                  << " for stream port " << this->sock.sa->port()
                  << ", sock " << new_sub.sock
                  << ", pid " << new_sub.pid << std::endl;
    }
    return retval;
}

/* The next function is the load-balancing functions which is used
 * to pick the subserver to which we'll hand off a child socket.
 *
 * The load balancing routine tries to keep a balance between all
 * subservers, and spawns a new subserver when the load on all the
 * current ones exceeds the threshold value.
 *
 * Logic dictates that smaller values of load_threshold would work
 * better with larger values of min_subservers.  Of course, in
 * practice it might be very different.
 *
 * It looks like we're going to have to deny connections at some
 * point, if the load gets way too great.  Our return value for that
 * will be -1.
 */
int stream_socket::choose_subserver(void)
{
    int i, lowest_load = config.max_subservers, best_index = -1;

    for (i = 0; i < (int)this->subservers.size(); ++i)
        if (this->subservers[i].connections < (int)(config.max_subservers
                                                    * config.load_threshold)
            && this->subservers[i].connections < lowest_load)
        {
            lowest_load = this->subservers[i].connections;
            best_index = i;
        }
    if (best_index == -1
        && (int)this->subservers.size() < config.max_subservers)
        best_index = this->create_subserver();
    return best_index;
}

/* The W. Richard Stevens function wasn't working in the unit test for
 * the subserver, so we had to do a little tweakery.  Working version
 * came mostly from http://stackoverflow.com/questions/28003921/sending-file-descriptor-by-linux-socket
 */
int stream_socket::pass_fd(int fd, int new_fd)
{
    struct cmsghdr *cmptr;
    struct iovec iov = { .iov_base = (void *)"", .iov_len = 1 };
    struct msghdr msg = { 0 };
    char buf[CMSG_SPACE(sizeof(new_fd))];

    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmptr = CMSG_FIRSTHDR(&msg);
    cmptr->cmsg_level = SOL_SOCKET;
    cmptr->cmsg_type = SCM_RIGHTS;
    cmptr->cmsg_len = CMSG_LEN(sizeof(new_fd));

    memmove(CMSG_DATA(cmptr), &new_fd, sizeof(new_fd));
    msg.msg_controllen = cmptr->cmsg_len;

    if (sendmsg(fd, &msg, 0) != 1)
        return -1;
    return 0;
}

stream_socket::~stream_socket()
{
    stream_socket::subserver_iterator i;

    if (this->sock.sock != 0)
        FD_CLR(this->sock.sock, &this->master_readfs);

    /* Kill each of the subservers, and empty the vector */
    for (i = this->subservers.begin(); i != this->subservers.end(); ++i)
    {
        if (kill((*i).pid, SIGTERM) == -1)
            std::clog << syslogErr
                      << "couldn't kill subserver " << (*i).pid
                      << " for stream port " << this->sock.sa->port() << ": "
                      << strerror(errno) << " (" << errno << ")" << std::endl;
        waitpid((*i).pid, NULL, 0);
        FD_CLR((*i).sock, &this->master_readfs);
        close((*i).sock);
    }
    this->subservers.erase(this->subservers.begin(), this->subservers.end());

    /* Thread pools are handled by the listen_socket destructor */
}

std::string stream_socket::port_type(void)
{
    return "stream";
}

void stream_socket::start(void)
{
    int i, retval;

    std::clog << "starting connection loop for stream port "
              << this->sock.sa->port() << std::endl;

    /* Before we go into the select loop, we should spawn off the
     * number of subservers in min_subservers.
     */
    for (i = 0; i < config.min_subservers; ++i)
        if ((this->create_subserver()) == -1)
        {
            std::ostringstream s;
            s << "couldn't create subserver for stream port "
              << this->sock.sa->port() << ": "
              << strerror(errno) << " (" << errno << ")";
            throw std::runtime_error(s.str());
        }

    /* Start up the sending thread pool */
    sleep(0);
    this->send_pool->startup_arg = (void *)this;
    this->send_pool->start(stream_socket::stream_send_worker);
    this->access_pool->startup_arg = (void *)this;
    this->access_pool->start(listen_socket::access_pool_worker);
    this->sock.listen_arg = (void *)this;
    this->sock.start(stream_socket::stream_listen_worker);

    /* Start up the reaping thread */
    if ((retval = pthread_create(&this->reaper, NULL,
                                 stream_reaper_worker, (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "couldn't create reaper thread for stream port "
          << this->sock.sa->port() << ": "
          << strerror(retval) << " (" << retval << ")";
        throw std::runtime_error(s.str());
    }
    this->reaper_running = true;
}

void stream_socket::do_login(uint64_t userid,
                             Control *con,
                             access_list& al)
{
    stream_user *stu = new stream_user(userid, con);
    stu->subsrv = al.what.login.who.stream.sub;
    stu->fd = al.what.login.who.stream.sock;
    users[userid] = stu;

    this->connect_user((base_user *)stu, al);
}

void *stream_socket::stream_listen_worker(void *arg)
{
    stream_socket *sts = (stream_socket *)arg;
    int fd, retval, len;
    unsigned char buf[1024];
    struct sockaddr_in sin;
    socklen_t slen;
    struct linger ls;
    stream_socket::subserver_iterator i, j;

    for (;;)
    {
        /* The readfs state is undefined after select; define it. */
        memcpy(&(sts->readfs), &(sts->master_readfs), sizeof(fd_set));

        pthread_testcancel();
        if ((retval = select(sts->max_fd, &(sts->readfs),
                             NULL, NULL, NULL)) == 0)
            continue;
        else if (retval == -1)
        {
            pthread_testcancel();
            if (errno == EINTR)
            {
                /* we got a signal; it could have been a HUP */
                std::clog << syslogNotice
                          << "select interrupted by signal in stream port "
                          << sts->sock.sa->port() << std::endl;
                continue;
            }
            else
            {
                std::clog << syslogErr
                          << "select error in stream port "
                          << sts->sock.sa->port() << ": "
                          << strerror(errno) << " (" << errno << ")"
                          << std::endl;
                /* Should we blow up or something here? */
            }
        }

        pthread_testcancel();
        /* We got some data from somebody. */
        /* Figure out if it's a listening socket. */
        if (FD_ISSET(sts->sock.sock, &(sts->readfs))
            && (fd = accept(sts->sock.sock,
                            (struct sockaddr *)&sin, &slen)) > 0)
        {
            /* Pass the new fd to a subserver immediately.
             * First, set some good socket options, based on
             * our config options.
             */
            ls.l_onoff = (config.use_linger > 0);
            ls.l_linger = config.use_linger;
            setsockopt(fd, SOL_SOCKET, SO_LINGER,
                       &ls, sizeof(struct linger));
            setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE,
                       &config.use_keepalive, sizeof(int));
            ioctl(fd, FIONBIO, &config.use_nonblock);
            if ((retval = sts->choose_subserver()) != -1)
            {
                /* We have connections available */
                sts->pass_fd(sts->subservers[retval].sock, fd);
                ++(sts->subservers[retval].connections);
            }
            else
                /* We should send some kind of "maximum number of
                 * connections exceeded" message here.
                 */
                close(fd);
        }

        for (i = sts->subservers.begin(); i != sts->subservers.end(); ++i)
            if (FD_ISSET((*i).sock, &(sts->readfs)))
            {
                /* It's a subserver socket. */
                if ((len = read((*i).sock,
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
                    /* This subserver has died. */
                    std::clog << "stream port " << sts->sock.sa->port()
                              << " subserver " << (*i).pid
                              << " died" << std::endl;
                    waitpid((*i).pid, NULL, 0);
                    close((*i).sock);
                    FD_CLR((*i).sock, &(sts->master_readfs));
                    sts->subservers.erase(i--);

                    /* Make sure max_fd is valid. */
                    sts->max_fd = sts->sock.sock + 1;
                    for (j = sts->subservers.begin();
                         j != sts->subservers.end();
                         ++j)
                        sts->max_fd = std::max((*j).sock + 1, sts->max_fd);
                }
            }
    }
    std::clog << "exiting connection loop for stream port "
              << sts->sock.sa->port() << std::endl;
    return NULL;
}

void *stream_socket::stream_reaper_worker(void *arg)
{
    stream_socket *sts = (stream_socket *)arg;
    listen_socket::users_iterator i;
    stream_user *stu;
    time_t now;

    std::clog << "started reaper thread for stream port "
              << sts->sock.sa->port() << std::endl;
    for (;;)
    {
        sleep(listen_socket::REAP_TIMEOUT);
        now = time(NULL);
        for (i = sts->users.begin(); i != sts->users.end(); ++i)
        {
            pthread_testcancel();
            if ((stu = dynamic_cast<stream_user *>((*i).second)) == NULL)
                continue;
            if (stu->timestamp < now - listen_socket::LINK_DEAD_TIMEOUT)
            {
                /* We'll consider the user link-dead */
                std::clog << "removing user "
                          << stu->control->username
                          << " (" << stu->userid << ") from stream port "
                          << sts->sock.sa->port() << std::endl;
                if (stu->control->slave != NULL)
                {
                    /* Clean up a user who has logged out */
                    stu->control->slave->natures.insert("invisible");
                    stu->control->slave->natures.insert("non-interactive");
                }
                delete stu->control;
                /* Tell subserver stu->subserv to close and erase user
                 * stu->fd
                 * To do this, we'll simply pass the descriptor again.
                 */
                sts->pass_fd(stu->subsrv, stu->fd);
                sts->users.erase((*(i--)).second->userid);
            }
            else if (stu->timestamp < now - listen_socket::PING_TIMEOUT
                     && stu->pending_logout == false)
                /* After 30 seconds, see if the user is still there */
                sts->send_ping(stu->control);
        }
        pthread_testcancel();
    }
    return NULL;
}

void *stream_socket::stream_send_worker(void *arg)
{
    stream_socket *sts = (stream_socket *)arg;
    stream_user *stu;
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
            /* TODO: Encryption */
            write(sts->subservers[stu->subsrv].sock,
                  (void *)&stu->fd, sizeof(int));
            write(sts->subservers[stu->subsrv].sock,
                  (void *)&req, realsize);
        }
    }
    std::clog << "exiting send pool worker for stream port "
              << sts->sock.sa->port() << std::endl;
    return NULL;
}
