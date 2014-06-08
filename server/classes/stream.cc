/* stream.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 May 2014, 21:24:37 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2014  Trinity Annabelle Quirk
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
 * Changes
 *   09 Sep 2007 TAQ - Created the file from the ashes of tcpserver.c.
 *   13 Sep 2007 TAQ - Removed server.h include.  Used basesock's static
 *                     create_socket instead of C version.
 *   23 Sep 2007 TAQ - The constructor of ThreadPool changed.
 *   16 Dec 2007 TAQ - Added timestamp and pending_logout members to
 *                     assignment operator for stream_user.  Added thread
 *                     routine to reap link-dead and logged-out users.
 *   22 Nov 2009 TAQ - Fixed typo (subserv != subsrv) in stream_reaper_worker.
 *                     Redeclared stream_reaper_worker as extern, so it
 *                     can be a friend to stream_socket.
 *   19 Sep 2013 TAQ - Return NULL at the end of the worker routine to quiet
 *                     gcc.
 *   11 May 2014 TAQ - We've moved the motion- and position-related parameters
 *                     out of the GameObject and into the Motion object, so
 *                     some pointers point at different things.
 *
 * Things to do
 *   - Move the contents of ../old/subserver.c in here.
 *
 * $Id$
 */

#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <errno.h>
#include <syslog.h>
#ifdef __APPLE__
#include <signal.h>
#endif

#include <algorithm>

#include "stream.h"
#include "zone_interface.h"
#include "../config.h"
#include "../subserver.h"

extern volatile int main_loop_exit_flag;

extern void *stream_send_pool_worker(void *);
extern void *stream_reaper_worker(void *);

bool stream_user::operator<(const stream_user& su) const
{
    return (this->userid < su.userid);
}

bool stream_user::operator==(const stream_user& su) const
{
    return (this->userid == su.userid);
}

const stream_user& stream_user::operator=(const stream_user& su)
{
    this->userid = su.userid;
    this->control = su.control;
    this->timestamp = su.timestamp;
    this->subsrv = su.subsrv;
    this->fd = su.fd;
    this->pending_logout = su.pending_logout;
    return *this;
}

const subserver& subserver::operator=(const subserver& ss)
{
    this->sock = ss.sock;
    this->pid = ss.pid;
    this->connections = ss.connections;
    return *this;
}

stream_socket::stream_socket(int p)
    : users(), subservers()
{
    try
    {
	this->send = new ThreadPool<packet_list>("send", config.send_threads);
    }
    catch (int error_val)
    {
	errno = error_val;
	return;
    }

    this->port = p;
    if ((this->sock = basesock::create_socket(SOCK_STREAM, this->port)) == -1)
    {
	return;
    }
    FD_ZERO(&(this->master_readfs));
    FD_SET(this->sock, &(this->master_readfs));
    this->max_fd = this->sock + 1;
}

stream_socket::~stream_socket()
{
    std::vector<subserver>::iterator i;
    int retval;

    /* Terminate the reaping thread */
    pthread_cancel(this->reaper);
    sleep(0);
    if ((retval = pthread_join(this->reaper, NULL)) != 0)
	syslog(LOG_ERR,
	       "error terminating reaper thread in stream port %d: %s",
	       this->port, strerror(retval));

    /* Close out the main socket */
    if (this->sock >= 0)
    {
	FD_CLR(this->sock, &this->master_readfs);
	close(this->sock);
    }

    /* Kill each of the subservers, and empty the vector */
    for (i = this->subservers.begin(); i != this->subservers.end(); ++i)
    {
	if (kill((*i).pid, SIGTERM) == -1)
	    syslog(LOG_ERR,
		   "couldn't kill subserver %d for stream port %d: %s",
		   (*i).pid, this->port, strerror(errno));
	waitpid((*i).pid, NULL, 0);
	FD_CLR((*i).sock, &this->master_readfs);
	close((*i).sock);
    }
    this->subservers.erase(this->subservers.begin(), this->subservers.end());

    /* Clear out the users map */
    this->users.erase(this->users.begin(), this->users.end());

    /* Stop the sending thread pool */
    if (send != NULL)
	delete send;
}

void stream_socket::listen(void)
{
    int i, fd, retval, len;
    unsigned char buf[1024];
    struct sockaddr_in sin;
    socklen_t slen;
    struct linger ls;

    syslog(LOG_DEBUG,
	   "starting connection loop for stream port %d",
	   this->port);

    /* Before we go into the select loop, we should spawn off the
     * number of subservers in min_subservers.
     */
    for (i = 0; i < config.min_subservers; ++i)
	if ((this->create_subserver()) == -1)
	{
	    syslog(LOG_ERR,
		   "couldn't create subserver for stream port %d: %s",
		   this->port, strerror(errno));
	    return;
	}

    /* Start up the sending thread pool */
    sleep(0);
    this->send->startup_arg = (void *)this;
    try
    {
	this->send->start(stream_send_pool_worker);
    }
    catch (int error_val)
    {
	syslog(LOG_ERR,
	       "couldn't start send pool for stream port %d: %s",
	       this->port, strerror(error_val));
	errno = error_val;
	return;
    }

    /* Start up the reaping thread */
    if ((retval = pthread_create(&this->reaper, NULL,
				 stream_reaper_worker, (void *)this)) != 0)
    {
	syslog(LOG_ERR,
	       "couldn't create reaper thread for stream port %d: %s",
	       this->port, strerror(retval));
	errno = retval;
	return;
    }

    /* Do the receiving part */
    for (;;)
    {
	if (main_loop_exit_flag == 1)
	    break;

	/* The readfs state is undefined after select; define it. */
	memcpy(&(this->readfs), &(this->master_readfs), sizeof(fd_set));

	if ((retval = select(this->max_fd, &(this->readfs),
			     NULL, NULL, NULL)) == 0)
	    continue;
	else if (retval == -1)
	{
	    if (errno == EINTR)
	    {
		/* we got a signal; it could have been a HUP */
		syslog(LOG_NOTICE,
		       "select interrupted by signal in stream port %d",
		       this->port);
		continue;
	    }
	    else
	    {
		syslog(LOG_ERR,
		       "select error in stream port %d: %s",
		       this->port, strerror(errno));
		/* Should we blow up or something here? */
	    }
	}

	/* We got some data from somebody. */
	/* Figure out if it's a listening socket. */
	if (FD_ISSET(this->sock, &(this->readfs))
	    && (fd = accept(this->sock,
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
	    if ((retval = this->choose_subserver()) != -1)
	    {
		/* We have connections available */
		this->pass_fd(this->subservers[retval].sock, fd);
		++(this->subservers[retval].connections);
	    }
	    else
		/* We should send some kind of "maximum number of
		 * connections exceeded" message here.
		 */
		close(fd);
	}

	for (std::vector<subserver>::iterator j = this->subservers.begin();
	     j != this->subservers.end();
	     ++j)
	    if (FD_ISSET((*j).sock, &(this->readfs)))
	    {
		/* It's a subserver socket. */
		if ((len = read((*j).sock,
				buf,
				sizeof(buf))) > 0)
		{
		    /* We have recieved something. */
		    syslog(LOG_DEBUG, "got something");

		    /* The first sizeof(int) bytes will be the socket
		     * descriptor, and the rest of it will be the actual
		     * client data that we got.
		     */
		}
		else
		{
		    /* This subserver has died. */
		    syslog(LOG_DEBUG,
			   "stream port %d subserver %d died",
			   this->port, (*j).pid);
		    waitpid((*j).pid, NULL, 0);
		    close((*j).sock);
		    FD_CLR((*j).sock, &(this->master_readfs));
		    this->subservers.erase(j--);

		    /* Make sure max_fd is valid. */
		    this->max_fd = this->sock + 1;
		    for (std::vector<subserver>::iterator k
			     = this->subservers.begin();
			 k != this->subservers.end();
			 ++k)
			this->max_fd = std::max((*k).sock + 1, this->max_fd);
		}
	    }
    }
    syslog(LOG_DEBUG,
	   "exiting connection loop for stream port %d",
	   this->port);
}

int stream_socket::create_subserver(void)
{
    int i, fd[2], pid, retval = -1;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == -1)
    {
	syslog(LOG_ERR,
	       "couldn't create subserver sockets for stream port %d: %s",
	       this->port, strerror(errno));
	return retval;
    }

    /* We want the port number available to the child, for error reporting */
    i = this->port;

    if ((pid = fork()) < 0)
    {
	syslog(LOG_ERR,
	       "couldn't fork new subserver for stream port %d: %s",
	       this->port, strerror(errno));
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
	    syslog(LOG_ERR,
		   "can't get rlimit in child %d for stream port %d: %s",
		   getpid(), i, strerror(errno));
	    syslog(LOG_ERR,
		   "terminating child %d for stream port %d",
		   getpid(), i);
	    _exit(1);
	}
	/* In case the open-file rlimit is set to infinity, we should
	 * impose *some* sort of cap.
	 */
	if (rlim.rlim_cur == RLIM_INFINITY)
	    rlim.rlim_cur = 1024;

	/* Close all the open files except for our server
	 * communication socket.
	 */
	closelog();
	dup2(fd[1], STDIN_FILENO);
	for (i = 0; i < (int)rlim.rlim_cur; ++i)
	    if (i != STDIN_FILENO)
		close(i);

	/* This call will never return */
	subserver_main_loop();
	/* And in case it does... */
	close(STDIN_FILENO);
	_exit(0);
    }
    else
    {
	/* We are the parent */
	subserver new_sub;

	close(fd[1]);
	new_sub.sock = fd[0];
	new_sub.pid = pid;
	new_sub.connections = 0;
	this->max_fd = std::max(new_sub.sock + 1, this->max_fd);
	this->subservers.push_back(new_sub);
	retval = this->subservers.size() - 1;
	syslog(LOG_DEBUG,
	       "created subserver %d for stream port %d, sock %d, pid %d",
	       retval, this->port, new_sub.sock, new_sub.pid);
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

/* This next function is ripped directly out of the W. Richard Stevens
 * "Advanced Programming in the UNIX Environment" book, pages 487-488.
 * Some names were changed to protect the innocent.
 */
int stream_socket::pass_fd(int fd, int new_fd)
{
    struct iovec iov[1];
    struct msghdr msg;
    char buf[2];

    /* We will use the BSD4.4 method, since Linux uses that method */
    iov[0].iov_base = buf;
    iov[0].iov_len = 2;
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    if (new_fd < 0)
    {
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	buf[1] = -fd;
	if (buf[1] == 0)
	    buf[1] = 1;
    }
    else
    {
	this->cmptr.cmsg_level = SOL_SOCKET;
	this->cmptr.cmsg_type = SCM_RIGHTS;
	this->cmptr.cmsg_len = sizeof(struct cmsghdr) + sizeof(int);
	msg.msg_control = (caddr_t)(&this->cmptr);
	msg.msg_controllen = sizeof(struct cmsghdr) + sizeof(int);
	*(int *)CMSG_DATA((&this->cmptr)) = new_fd;
	buf[1] = 0;
    }
    buf[0] = 0;
    if (sendmsg(fd, &msg, 0) != 2)
	return -1;
    return 0;
}

/* We need to have another thread active to reap logged-out and link-dead
 * users from the user list.
 */
void *stream_reaper_worker(void *arg)
{
    stream_socket *sock = (stream_socket *)arg;
    std::map<u_int64_t, stream_user>::iterator i;
    time_t now;

    syslog(LOG_DEBUG,
	   "started reaper thread for stream port %d",
	   sock->port);
    for (;;)
    {
	sleep(15);
	now = time(NULL);
	for (i = sock->users.begin(); i != sock->users.end(); ++i)
	{
	    pthread_testcancel();
	    if ((*i).second.timestamp < now - 75)
	    {
		/* After 75 seconds, we'll consider the user link-dead */
		syslog(LOG_DEBUG,
		       "removing user %s (%llu) from stream port %d",
		       (*i).second.control->username.c_str(),
                       (*i).second.userid,
		       sock->port);
		if ((*i).second.control->slave != NULL)
		{
		    /* Clean up a user who has logged out */
		    (*i).second.control->slave->object->natures["invisible"] = 1;
		    (*i).second.control->slave->object->natures["non-interactive"] = 1;
		}
		pthread_mutex_lock(&active_users_mutex);
		active_users->erase((*i).second.userid);
		pthread_mutex_unlock(&active_users_mutex);
		delete (*i).second.control;
		/* Tell subserver (*i).second.subserv to close and erase user
		 * (*i).second.fd
		 * To do this, we'll simply pass the descriptor again.
		 */
		sock->pass_fd((*i).second.subsrv, (*i).second.fd);
		sock->users.erase((*(i--)).second.userid);
	    }
	    else if ((*i).second.timestamp < now - 30
		     && (*i).second.pending_logout == false)
		/* After 30 seconds, see if the user is still there */
		(*i).second.control->send_ping();
	}
	pthread_testcancel();
    }
    return NULL;
}

void *start_stream_socket(void *arg)
{
    stream_socket *ss = new stream_socket(*(int *)arg);
    if (ss != NULL)
    {
	ss->listen();
	delete ss;
    }
    return NULL;
}
