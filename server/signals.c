/* signals.c
 *   by Trinity Quirk <trinity@io.com>
 *   last updated 23 Aug 2007, 20:20:15 trinity
 *
 * Revision IX game server
 * Copyright (C) 2004  Trinity Annabelle Quirk
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
 * This file contains the signal handlers and registration functions.
 *
 * Changes
 *   11 Apr 1998 TAQ - Created the file.
 *   12 Apr 1998 TAQ - Wrote some basic signal handlers.
 *   10 May 1998 TAQ - Added CVS ID string.
 *   24 Sep 1998 TAQ - Modified some comments.
 *   16 Apr 2000 TAQ - Reset the CVS string.
 *   14 Oct 2000 TAQ - Added USR1 and USR2 handlers.  Converted to use
 *                     sigaction, since there's much more control available.
 *                     Changed the meaning of HUP to "restart everything",
 *                     where USR1 rereads the config and USR2 regenerates
 *                     the zone.
 *   29 Oct 2000 TAQ - Removed zone.h include, and changed startup/cleanup
 *                     zone to un/load_zone.
 *   02 Aug 2003 TAQ - Improperly ordered arguments in print statements of
 *                     setup_signals.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   14 Jun 2006 TAQ - Added stdlib.h include.
 *   27 Jul 2006 TAQ - Tried to get SEGV handler to dump core.  No dice.
 *   13 Aug 2006 TAQ - Removed the load/unload_zone calls, since those
 *                     functions no longer exist.  The SIGUSR2 handler now
 *                     does nothing, but can be implemented later.
 *   04 Jul 2007 TAQ - Hopefully this thing will spit out a stack trace
 *                     when a SEGV happens.
 *
 * Things to do
 *   - See if we can make the sigsegv handler dump core.
 *   - Catch everything else that may need to be caught.
 *
 * $Id: signals.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <execinfo.h>

#include "signals.h"
#include "server.h"
#include "config.h"

/* Function prototypes */
static void sighup_handler(int);
static void sigusr1_handler(int);
static void sigusr2_handler(int);
static void sigterm_handler(int);
static void sigsegv_handler(int);

/* Set up our special signal handlers. */
void setup_signals(void)
{
    struct sigaction sa;
    sigset_t ss;

    /* SIGHUP - completely restart the program. */
    sa.sa_handler = sighup_handler;
    sa.sa_sigaction = NULL;
    sigemptyset(&ss);
    sigaddset(&ss, SIGHUP);
    sigaddset(&ss, SIGUSR1);
    sigaddset(&ss, SIGUSR2);
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    if (sigaction(SIGHUP, &sa, NULL) == -1)
    {
	syslog(LOG_ERR, "couldn't set SIGHUP handler: %s (%d)",
	       strerror(errno), errno);
    }
    /* SIGHUP - reread the configuration files. */
    sa.sa_handler = sigusr1_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
	syslog(LOG_ERR, "couldn't set SIGUSR1 handler: %s (%d)",
	       strerror(errno), errno);
    }
    /* SIGUSR2 - recreate the zone. */
    sa.sa_handler = sigusr2_handler;
    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
	syslog(LOG_ERR, "couldn't set SIGUSR2 handler: %s (%d)",
	       strerror(errno), errno);
    }
    /* SIGTERM - terminate the process normally. */
    sa.sa_handler = sigterm_handler;
    sigaddset(&ss, SIGTERM);
    sa.sa_mask = ss;
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
	syslog(LOG_ERR, "couldn't set SIGTERM handler: %s (%d)",
	       strerror(errno), errno);
    }
    /* SIGSEGV - clean up and terminate the process. */
    sa.sa_handler = sigsegv_handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1)
    {
	syslog(LOG_ERR, "couldn't set SIGSEGV handler: %s (%d)",
	       strerror(errno), errno);
    }
}

/* Reset the signal handlers to the default actions. */
void cleanup_signals(void)
{
    struct sigaction sa;
    sigset_t ss;

    sa.sa_handler = SIG_DFL;
    sa.sa_sigaction = NULL;
    sigemptyset(&ss);
    sa.sa_mask = ss;
    sa.sa_flags = 0;
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGUSR1, &sa, NULL);
    sigaction(SIGUSR2, &sa, NULL);
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}

static void sighup_handler(int sig)
{
    syslog(LOG_INFO, "received SIGHUP, restarting application");
    complete_cleanup();
    complete_startup();
}

static void sigusr1_handler(int sig)
{
    syslog(LOG_INFO, "received SIGUSR1, reloading configuration");
    cleanup_configuration();
    setup_configuration(0, NULL);
}

static void sigusr2_handler(int sig)
{
    syslog(LOG_INFO, "received SIGUSR2, reloading zone (unimplemented)");
}

static void sigterm_handler(int sig)
{
    syslog(LOG_INFO, "received SIGTERM, terminating");
    set_exit_flag();
}

static void sigsegv_handler(int sig)
{
    void *stack_trace[10];
    char **strings;
    size_t trace_size, i;

    syslog(LOG_INFO, "received SIGSEGV, detonating");

    /* Generate a stack dump.  This seems like it might be dangerous in
     * a signal handler (for SIGSEGV, even) because it appears that quite
     * a bit of memory will be allocated.  We'll go with it, and if it
     * works, ok, and if not, we'll have to move to backtrace_symbols_fd.
     */
    trace_size = backtrace(stack_trace, sizeof(stack_trace));
    strings = backtrace_symbols(stack_trace, trace_size);
    for (i = 0; i < trace_size; ++i)
	syslog(LOG_DEBUG, "%s", strings[i]);

    /* Let's try to actually get a corefile */
    abort();
}
