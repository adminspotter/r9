/* server.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Jul 2014, 08:01:45 tquirk
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
 * This file contains a generic server skeleton.  The good stuff,
 * including forking of subservers and passing of file descriptors,
 * will be handled in the connection_loop routine, which is not in
 * this file.
 *
 * Everything that needs to be handled in here pretty well is already,
 * so we shouldn't need to muck about in here much more.
 *
 * Changes
 *   25 Mar 1998 TAQ - Created the file.
 *   28 Mar 1998 TAQ - Monkeyed with the socket-creation routine.  Moved
 *                     the config stuff into config.c.
 *   29 Mar 1998 TAQ - Added the options lines to setup_sockets.
 *   11 Apr 1998 TAQ - Moved the signal-handling to another file.  Moved
 *                     all external function prototypes into their
 *                     respective header files.
 *   12 Apr 1998 TAQ - Moved the socket variables into config.[ch].
 *   17 Apr 1998 TAQ - Made cleanup_sockets a global function.
 *   10 May 1998 TAQ - Minor changes, more includes, variable name changes
 *                     to make this file compilable.  Added CVS ID string.
 *   14 Feb 1999 TAQ - Moved calls around main since we actually want
 *                     to read the config first - we need to get the name
 *                     of the lock file we want to use before we try to
 *                     create the lock file.  It's not going to be a
 *                     compiled-in option any more.
 *   20 Feb 1999 TAQ - We now remove our pidfile when we terminate.
 *   06 Oct 1999 TAQ - Based on a new config option, we now support both
 *                     stream (tcp) and datagram (udp) socket creation.
 *   16 Apr 2000 TAQ - Got rid of the USE_LOCALHOST define, since it was
 *                     just a workaround for bad hosts.allow rules.  Also
 *                     made a couple other really minor changes.
 *   25 Apr 2000 TAQ - Fixed the one-interface-only problem - bind to IP
 *                     address 0.
 *   05 May 2000 TAQ - The UDP sockets weren't even being bound.  Duh.
 *                     Found the constant INADDR_ANY instead of 0 for
 *                     the bind call.  Stevens to the rescue again.
 *                     Removed all the gethost stuff, since we no longer
 *                     care.
 *   13 Jul 2000 TAQ - Permission tweak on creation of pidfile.  Made log
 *                     prefix a configuration option.
 *   27 Jul 2000 TAQ - Tiny string tweaks in setup_daemon.  Use default
 *                     log prefix and pid filename in case of null values
 *                     in the config struct.  Moved finding of the protocol
 *                     number out of the socket creation loop - we only need
 *                     to do it once.
 *   24 Aug 2000 TAQ - Removed checking of config values, since we're
 *                     now guaranteed that they'll be set - config routines
 *                     handle that for us.
 *   27 Aug 2000 TAQ - Terminate if we find that a normal user is trying
 *                     to bind to a privileged port.  Now cleanup the pid
 *                     file when we error out of setup_sockets.
 *   19 Sep 2000 TAQ - Since there was so much shared code between the
 *                     stream and datagram socket creation, it's now
 *                     *actually* shared.
 *   29 Sep 2000 TAQ - Added zone creation/deletion calls.
 *   14 Oct 2000 TAQ - Added complete_(startup|cleanup) for the signal
 *                     handlers.  setup_log now takes no arguments, to
 *                     make it just like the rest of the routines which
 *                     read straight out of the config structure.
 *   29 Oct 2000 TAQ - Added load_zone and unload_zone, since we're now
 *                     going to make the zone class and whatnot into a
 *                     dynamic lib for easy changes.  When setup_sockets
 *                     errors out, we now call cleanup_log and
 *                     cleanup_configuration in all cases.
 *   13 Nov 2000 TAQ - When we need root privs to bind ports < 1025, we
 *                     grab them at the last possible chance, and drop
 *                     them at the earliest possible chance.
 *   29 Jan 2002 TAQ - We now have the logging facility as a config option.
 *   14 Feb 2002 TAQ - The name of the element in the config structure for
 *                     the library directory changed, so we had to change it
 *                     in here.
 *   09 Dec 2005 TAQ - Moved some error handling around in setup_sockets and
 *                     unload_zone.  Also made main use complete_cleanup
 *                     instead of calling each by hand.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   08 Jun 2006 TAQ - Added thread setup/cleanup calls.
 *   14 Jun 2006 TAQ - Fixed up most of the thread handling.
 *   04 Jul 2006 TAQ - Thread pools moved into the zone.
 *   01 Aug 2006 TAQ - Added prototypes for a couple calls into zone.cc.
 *   02 Aug 2006 TAQ - Moved the zone prototypes into a header in the
 *                     classes dir.
 *   13 Aug 2006 TAQ - Removed load and unload zone - they're not used, and
 *                     they refer to a bunch of config options that have
 *                     been removed for the time being.
 *   17 Aug 2006 TAQ - setup_zone now returns an error code; a nonzero return
 *                     indicates that we've got no zone to use, so we bailout
 *                     in that case.
 *   23 Aug 2007 TAQ - Moved main_loop_exit_flag and set_exit_flag in here,
 *                     since connect.c is no longer worth keeping around.
 *                     Removed setup_sockets, since that's now handled in
 *                     the tcp and udp subserver files.
 *   30 Aug 2007 TAQ - Finished up the thread starting and stopping for the
 *                     socket listening stuff.  Renamed a couple funcs.
 *   13 Sep 2007 TAQ - Used proper name for start_*_socket.
 *   14 Oct 2007 TAQ - Used correct index for starting datagram sockets.
 *                     Cleaned up debugging output in setup_sockets.
 *   19 Sep 2013 TAQ - Added console setup/cleanup calls.
 *   21 Jun 2014 TAQ - We're abandoning the misguided idea that the base
 *                     server should be C, and starting to convert some
 *                     items to C++, the first of which is the syslog.
 *                     Renamed the file to reflect the actual language.
 *   22 Jun 2014 TAQ - C++-ification of the configuration is underway.
 *   02 Jul 2014 TAQ - We weren't catching some possible socket exceptions
 *                     on delete, and were also never starting up the sockets
 *                     once they were created.
 *
 * Things to do
 *   - Complete C++-ification.
 *     - zone creation/deletion (most of zone_interface.cc)
 *   - Figure out if we can use a pthread_cond_t without having to have a
 *     mutex around.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include "log.h"
#include "server.h"
#include "signals.h"
#include "config.h"
#include "classes/zone_interface.h"

#include "classes/basesock.h"
#include "classes/stream.h"
#include "classes/dgram.h"

/* Static function prototypes */
static int setup_daemon(void);
static void setup_log(void);
static void setup_sockets(void);
static void setup_console(void);
static void cleanup_console(void);
static void cleanup_sockets(void);
static void cleanup_log(void);
static void cleanup_daemon(void);

static struct addrinfo *get_addr_info(int, int);

/* File-global variables */
void *zone_lib = NULL;
int main_loop_exit_flag = 0;
std::vector<listen_socket *> sockets;
/* Really don't need this mutex, but whatever */
static pthread_mutex_t exit_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t exit_flag = PTHREAD_COND_INITIALIZER;

int main(int argc, char **argv)
{
    int retval;

    /* Set everything up. */
    setup_configuration(argc, argv);
    if ((retval = setup_daemon()) != 0)
	return retval;
    setup_log();
    setup_signals();
    /* The thread pools are created on zone setup */
    if ((retval = setup_zone()) != 0)
	goto BAILOUT1;
    try { setup_sockets(); }
    catch (...) { goto BAILOUT2; }
    setup_console();

    /* Since all our stuff is running in other threads, we'll just
     * wait until the exit flag gets waved.
     *
     * FIXME: do some error checking here
     */
    pthread_mutex_lock(&exit_mutex);
    pthread_cond_wait(&exit_flag, &exit_mutex);
    pthread_mutex_unlock(&exit_mutex);
    pthread_cond_destroy(&exit_flag);
    pthread_mutex_destroy(&exit_mutex);

    /* Clean everything up before we exit. */
    cleanup_console();
    cleanup_sockets();
  BAILOUT2:
    cleanup_zone();
  BAILOUT1:
    cleanup_log();
    cleanup_daemon();
    cleanup_configuration();
    cleanup_signals();
    return retval;
}

static int setup_daemon(void)
{
    int fd;
    pid_t pid;
    char str[16];

    /* Start up like a proper daemon */
    if ((pid = fork()) < 0)
	return -1;
    else if (pid != 0)
	exit(0);

    pid = getpid();
    setsid();
    chdir("/");
    umask(0);

    /* Now write the pid file if we can. */
    if ((fd = open(config.pid_fname.c_str(),
		   O_CREAT | O_WRONLY | O_EXCL,
		   S_IRUSR | S_IWUSR)) != -1)
    {
	snprintf(str, sizeof(str), "%d", pid);
	write(fd, str, strlen(str));
	close(fd);
    }
    else
    {
	/* Apparently another invocation is running, so we can't. */
        std::clog << "couldn't create lock file: " << strerror(errno)
                  << " (" << errno << "), terminating" << std::endl;
	return -1;
    }
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
    return 0;
}

static void setup_log(void)
{
    /* Open the system log. */
    std::clog.rdbuf(new Log(config.log_prefix, config.log_facility));
    std::clog << syslogNotice << "starting" << std::endl;
}

static void setup_sockets(void)
{
    struct addrinfo *ai;
    std::vector<int>::iterator i;
    std::vector<listen_socket *>::iterator j;
    int created = 0;

    /* Bailout now if there are no sockets to create */
    if (config.stream.size() == 0 && config.dgram.size() == 0)
    {
	std::clog << syslogErr << "no sockets to create" << std::endl;
        throw ENOENT;
    }

    std::clog << "going to create "
              << config.stream.size() << " stream port"
              << (config.stream.size() == 1 ? "" : "s")
              << " and " << config.dgram.size() << " dgram port"
              << (config.dgram.size() == 1 ? "" : "s") << std::endl;

    for (i = config.stream.begin(); i != config.stream.end(); ++i)
    {
        /* First get an addrinfo struct for the socket */
        if ((ai = get_addr_info(SOCK_DGRAM, *i)) == NULL)
            continue;
        try
        {
            stream_socket *sock = new stream_socket(ai, *i);
            sockets.push_back(sock);
        }
        catch (int e)
        {
            while (sockets.size())
            {
                delete sockets.back();
                sockets.pop_back();
            }
            throw;
	}
        freeaddrinfo(ai);
        ++created;
    }
    if (created > 0)
	std::clog << "created " << created << " stream socket"
                  << (created == 1 ? "" : "s") << std::endl;

    for (i = config.dgram.begin(); i != config.dgram.end(); ++i)
    {
        /* First get an addrinfo struct for the socket */
        if ((ai = get_addr_info(SOCK_STREAM, *i)) == NULL)
            continue;
        try
        {
            dgram_socket *sock = new dgram_socket(ai, *i);
            sockets.push_back(sock);
        }
        catch (int e)
        {
            while (sockets.size())
	    {
                delete sockets.back();
                sockets.pop_back();
	    }
            throw;
	}
        freeaddrinfo(ai);
    }
    created = sockets.size() - created;
    if (created > 0)
	std::clog << "created " << created << " dgram socket"
                  << (created == 1 ? "" : "s") << std::endl;

    /* Now start them all up */
    for (j = sockets.begin(); j != sockets.end(); ++j)
        (*j)->start();
}

struct addrinfo *get_addr_info(int type, int port)
{
    struct addrinfo hints, *ai = NULL;
    int ret;
    char port_str[16];

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = type;
    snprintf(port_str, sizeof(port_str), "%d", port);
    if ((ret = getaddrinfo(NULL, port_str, &hints, &ai)) != 0)
    {
        std::clog << syslogErr
                  << "failed to get address info for "
                  << (type == SOCK_STREAM ? "stream" : "dgram")
                  << " port " << port << ": "
                  << gai_strerror(ret) << " (" << ret << ")" << std::endl;
        return NULL;
    }
    return ai;
}

void set_exit_flag(void)
{
    pthread_mutex_lock(&exit_mutex);
    main_loop_exit_flag = 1;
    pthread_cond_broadcast(&exit_flag);
    pthread_mutex_unlock(&exit_mutex);
}

static void setup_console(void)
{
}

static void cleanup_console(void)
{
}

static void cleanup_sockets(void)
{
    while (sockets.size())
    {
        try { delete sockets.back(); }
        catch (...) { }
        sockets.pop_back();
    }
}

static void cleanup_log(void)
{
    /* Close the system log gracefully. */
    std::clog << syslogNotice << "terminating" << std::endl;
    /* Figure out how to set the stream buffer back to normal */
}

static void cleanup_daemon(void)
{
    /* Remove the pidfile so another invocation can run. */
    unlink(config.pid_fname.c_str());
}

/* For the signal handlers. */
void complete_startup(void)
{
    setup_configuration(0, NULL);
    if (setup_daemon() == -1)
	exit(1);
    setup_log();
    setup_signals();
    try { setup_sockets(); }
    catch (...) { exit(1); }
}

/* For the signal handlers. */
void complete_cleanup(void)
{
    cleanup_sockets();
    cleanup_zone();
    cleanup_log();
    cleanup_daemon();
    cleanup_configuration();
}
