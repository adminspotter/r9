/* subserver.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Aug 2015, 15:38:25 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2015  Trinity Annabelle Quirk
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
 * This file contains the skeleton for the subserver.  We get spawned
 * from the main server, start accepting passed-in file descriptors
 * from stdin, load up the interepreter library (depending on what
 * config file we need to read in), and get to work.  All I/O will be
 * either on stdin (to/from the main server) or on each of the client
 * sockets.
 *
 * Things that forked children do NOT inherit:
 *     mallocked memory blocks (pointers are still pointed correctly, but
 *                              the memory block is owned by the parent).
 *     signal handlers (obviously)
 * Things that forked children DO inherit:
 *     pipes which two processes are already using both ends of.
 *     open files (including sockets bound to a port).
 *     variables on the stack (which aren't pointers to mallocked memory).
 *
 * It might almost be more worthwhile to have the subservers as
 * separate binaries, for purposes of making sure that we don't access
 * anything that really doesn't belong to us, or have a bunch of open
 * descriptors that we don't use (or have any right to use) clogging
 * up our file table.
 *
 * -- 17 Apr 1998
 * Upon further reflection (after a conversation with Kevin), it might
 * be more appropriate to have the server-subserver connection as some
 * sort of socket, AF_UNIX or otherwise.  It requires less file
 * descriptors, and that means that we can have twice as many users on
 * at any one time.
 *
 * -- 10 May 1998
 * When we load up the interpreter library, right now we just look for
 * a symbol called "initialize" and call it as a void function.  It
 * will return a pointer to a newly-allocated union of all the
 * different kinds of state structures.  This will be passed into the
 * looping routine, and will also be passed into the cleanup routine,
 * which will deallocate the memory.  The first two elements of every
 * struct in the union MUST be ints called type and exit_flag.  It is
 * modelled from the X XEvent type.
 *
 * -- 25 May 1998
 * After some reading of Stevens, it turns out that children inherit
 * ALL open fd's of the parent, including listening sockets.  Before
 * this routine gets called, right after the fork call, we now close
 * ALL descriptors except for the server-subserver socket.
 *
 * -- 05 Sep 2007
 * Now that we're making some real progress, it appears that the best
 * way to make the communications as simple as possible is to have the
 * record keeping as to what user is connected to what socket in what
 * subserver handled in the main listening socket thread.  We'll
 * simply pass the socket number along with the data to be passed, in
 * both directions, for user identification.
 *
 * Changes
 *   04 Apr 1998 TAQ - Created the file.
 *   11 Apr 1998 TAQ - Modified some comments.  This file is going to
 *                     be very difficult to write.  Each different
 *                     interpreter type (planned interpreters include
 *                     tcl, perl, lisp, C, C++, and maybe even python)
 *                     will probably end up in its own file.
 *   12 Apr 1998 TAQ - Contemplated the design of this a bit more.
 *   17 Apr 1998 TAQ - More contemplation.  We're going to work to get
 *                     this bad boy to compile.
 *   10 May 1998 TAQ - Added CVS ID string.  It turns out that we can
 *                     probably get away with just one file, and have
 *                     each different interpreter have its own startup
 *                     and cleanup routine.  Fleshed this file out a
 *                     whole lot.  Most of the dynamic-loading stuff
 *                     is now here.
 *   11 May 1998 TAQ - The connection loop now uses socketpair to create
 *                     our communication channel.  Our I/O with the
 *                     main server is through STDIN_FILENO.
 *   25 May 1998 TAQ - Monkeyed with the funcptrs.  Changed some
 *                     comments in light of new information.
 *   24 Sep 1998 TAQ - Got rid of the extra crap in recv_fd.  The
 *                     entirety of subserver_main_loop is gone.
 *   20 Oct 1998 TAQ - Tweaked recv_fd to eliminate a compile warning.
 *   24 Oct 1998 TAQ - The subserver loop now actually does something,
 *                     though it won't accept any new connections yet.
 *                     Also attempted to open a syslog connection.
 *   01 Nov 1998 TAQ - We now keep track of each connection as it comes
 *                     in.  Unfortunately the connections aren't making
 *                     it this far (something with inetd and hosts.allow
 *                     I'm guessing).
 *   16 Jan 1999 TAQ - Changed some comments.
 *   16 Apr 2000 TAQ - Reset the CVS ID string.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   16 Aug 2006 TAQ - The exit flag is now volatile.  Misc tiny tweaks.
 *   05 Sep 2007 TAQ - Figured out how we want to pass data between the
 *                     master listening thread and us.
 *   21 Jun 2014 TAQ - The C++-ification has begun, starting with syslog.
 *   04 Jul 2014 TAQ - We're now a completely separate binary.
 *   05 Aug 2015 TAQ - subserver.h is completely irrelevant anymore.
 *
 * Things to do
 *   - Finish up the sending and recieving code.
 *   - We can send things to the master, but we'll never receive anything
 *     back, other than a file descriptor.  Not quite what we're after.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stddef.h>
#include <signal.h>
#include <errno.h>

#include <algorithm>
#include <set>

#include "log.h"

#define CONTROLLEN   (sizeof(struct cmsghdr) + sizeof(int))

/* Function prototypes */
static int handle_client(int);
static void subserver_set_exit_flag(void);
static int recv_fd(void);
static void cleanup_subserver(void);
static void sigterm_handler(int);

/* File-global variables */
static struct cmsghdr *cmptr = NULL;
static fd_set readfs, master_readfs;
static volatile int subserver_loop_exit_flag = 0;
static std::set<int> connections;
static int max_fd = 0;

/* This next function is the main loop; it does nothing now, and dies
 * when it exits.
 */
int main(int argc, char **argv)
{
    std::set<int>::iterator i;
    int select_stat, val;

    /* Install our sigterm handler so we exit in a timely fashion. */
    signal(SIGTERM, sigterm_handler);

    /* Open up a connection to the syslog. */
    std::clog.rdbuf(new Log("subserver", LOG_DAEMON));
    std::clog << syslogNotice << "subserver starting";

    /* Set up the select descriptor set. */
    FD_ZERO(&master_readfs);
    FD_SET(STDIN_FILENO, &master_readfs);
    max_fd = STDIN_FILENO + 1;

    for (;;)
    {
        if (subserver_loop_exit_flag == 1)
            break;

        /* The state of readfs is undefined after select returns,
         * or on entry to this loop; define it.
         */
        memcpy(&readfs, &master_readfs, sizeof(fd_set));

        /* Here we go... */
        if ((select_stat = select(max_fd, &readfs, NULL, NULL, NULL)) == 0)
            continue;
        else if (select_stat == -1)
        {
            /* There was some kind of error */
            switch (errno)
            {
              case EINTR:
                /* we got a signal */
                continue;

              case ENOMEM:
                std::clog << syslogErr << "select memory error: "
                          << strerror(errno) << " (" << errno << ")"
                          << std::endl;
                break;

              default:
                std::clog << syslogErr << "select error: "
                          << strerror(errno) << " (" << errno << ")"
                          << std::endl;
                break;
            }
        }

        if (subserver_loop_exit_flag == 1)
            break;

        /* At this point, we should know that we had a valid reason
         * for select to exit, so look at our descriptors.
         */
        if (FD_ISSET(STDIN_FILENO, &readfs) && (val = recv_fd()) >= 0)
        {
            /* It's a new connection from the parent. */
            connections.insert(val);
            FD_SET(val, &master_readfs);
            max_fd = std::max(val + 1, max_fd);
            std::clog << syslogNotice << "Got connection " << val << std::endl;
        }

        if (subserver_loop_exit_flag == 1)
            break;

        for (i = connections.begin(); i != connections.end(); ++i)
            if (FD_ISSET(*i, &readfs) && !handle_client(*i))
            {
                /* Somebody dropped their connection, or there was
                 * some other problem.
                 */
                std::clog << syslogNotice
                          << "Lost connection " << *i << std::endl;
                close(*i);
                FD_CLR(*i, &master_readfs);
                FD_CLR(*i, &readfs);
                connections.erase(*(i--));

                /* Make sure max_fd is valid. */
                if (connections.rbegin() != connections.rend())
                    max_fd = *(connections.rbegin()) + 1;
                else
                    max_fd = STDIN_FILENO + 1;
            }
    }

    /* Clean up the connections and exit. */
    cleanup_subserver();
    exit(0);
}

/* Handle a given client connection.
 *
 * We return an int: nonzero when successful, and zero when there was a
 * problem.  The zero-return will cause the connection to be closed.
 */
static int handle_client(int fd)
{
    int retval;
    /* FIXME:  make sure this buf is long enough for all our protocol */
    unsigned char buf[1024];

    /* We index into the buffer by sizeof(int) because we only want to
     * do a single write when sending to the master.
     */
    if ((retval = read(fd, buf + sizeof(int), sizeof(buf) - sizeof(int))) > 0)
    {
        /* Replace the first sizeof(int) bytes with the fd number */
        *((int *)buf) = fd;
        write(STDIN_FILENO, buf, retval + sizeof(int));
    }
    return retval;
}

/* This next function just sets a flag that causes the subserver to exit
 * the next time it makes a trip around the loop.
 */
static void subserver_set_exit_flag(void)
{
    subserver_loop_exit_flag = 1;
}

/* The next function is closely modelled after the W. Richard Stevens
 * "Advanced Programming in the UNIX Environment" book, pages 488-489.
 *
 * We should only call this function if we get a positive return from
 * select, so blocking should not ever be a problem.
 */
static int recv_fd(void)
{
    int newfd, nread, status = -1;
    char *ptr, buf[2];
    struct iovec iov[1];
    struct msghdr msg;

    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof(buf);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_name = NULL;
    msg.msg_namelen = 0;
    if (cmptr == NULL
        && (cmptr = (struct cmsghdr *)malloc(CONTROLLEN)) == NULL)
        return -1;
    msg.msg_control = (caddr_t)cmptr;
    msg.msg_controllen = CONTROLLEN;

    /* Our read pipe is *always* going to be on STDIN_FILENO */
    if ((nread = recvmsg(STDIN_FILENO, &msg, 0)) < 0)
    {
        std::clog << syslogErr
                  << "recvmsg error: " << strerror(errno)
                  << " (" << errno << ")" << std::endl;
        return -1;
    }
    else if (nread == 0)
    {
        /* This happens when the main server closes its socket to us;
         * this condition *should* never happen, but it might.  We'll
         * exit nicely if it does.
         */
        std::clog << syslogErr << "connection closed by server" << std::endl;
        subserver_set_exit_flag();
    }

    for (ptr = buf; ptr < &buf[nread]; )
    {
        if (*ptr++ == 0)
        {
            if (ptr != &buf[nread - 1])
            {
                std::clog << syslogErr << "message format error" << std::endl;
                return -1;
            }
            status = *ptr & 255;
            if (status == 0)
            {
                if (msg.msg_controllen != CONTROLLEN)
                {
                    std::clog << syslogErr
                              << "status = 0 but no fd" << std::endl;
                    return -1;
                }
                newfd = *(int *)CMSG_DATA(cmptr);
            }
            else
                newfd = -status;
            nread -= 2;
        }
    }
    if (status >= 0)
        return newfd;
    return status;
}

/* Clean up our mess.  We don't have much cleanup to do.
 */
static void cleanup_subserver(void)
{
    std::clog << syslogInfo << "subserver terminating" << std::endl;
    closelog();
}

/* Instead of having some protocol thing between main server and subserver,
 * the main is just going to send a TERM signal to the sub.  Easy enough.
 */
static void sigterm_handler(int sig)
{
    signal(SIGTERM, SIG_IGN);
    std::clog << syslogInfo << "Recieved SIGTERM, terminating" << std::endl;
    subserver_set_exit_flag();
    signal(SIGTERM, sigterm_handler);
}
