/* signals.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 16 Sep 2019, 07:57:49 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2019  Trinity Annabelle Quirk
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
 * Things to do
 *   - Catch everything else that may need to be caught.
 *
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "signals.h"
#include "server.h"

#include "classes/config_data.h"
#include "classes/log.h"

/* Function prototypes */
static void sighup_handler(int);
static void sigusr1_handler(int);
static void sigusr2_handler(int);
static void sigterm_handler(int);
static void sigint_handler(int);
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
        char err[128];

        strerror_r(errno, err, sizeof(err));
        std::clog << syslogErr << "couldn't set SIGHUP handler: "
                  << err << " (" << errno << ")" << std::endl;
    }
    /* SIGUSR1 - reread the configuration files. */
    sa.sa_handler = sigusr1_handler;
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
    {
        char err[128];

        strerror_r(errno, err, sizeof(err));
        std::clog << syslogErr << "couldn't set SIGUSR1 handler: "
                  << err << " (" << errno << ")" << std::endl;
    }
    /* SIGUSR2 - recreate the zone. */
    sa.sa_handler = sigusr2_handler;
    if (sigaction(SIGUSR2, &sa, NULL) == -1)
    {
        char err[128];

        strerror_r(errno, err, sizeof(err));
        std::clog << syslogErr << "couldn't set SIGUSR2 handler: "
                  << err << " (" << errno << ")" << std::endl;
    }
    /* SIGTERM - terminate the process normally. */
    sa.sa_handler = sigterm_handler;
    sigaddset(&ss, SIGTERM);
    sa.sa_mask = ss;
    if (sigaction(SIGTERM, &sa, NULL) == -1)
    {
        char err[128];

        strerror_r(errno, err, sizeof(err));
        std::clog << syslogErr << "couldn't set SIGTERM handler: "
                  << err << " (" << errno << ")" << std::endl;
    }
    /* SIGINT - terminate the process normally. */
    sa.sa_handler = sigint_handler;
    sigaddset(&ss, SIGINT);
    sa.sa_mask = ss;
    if (sigaction(SIGINT, &sa, NULL) == -1)
    {
        char err[128];

        strerror_r(errno, err, sizeof(err));
        std::clog << syslogErr << "couldn't set SIGINT handler: "
                  << err << " (" << errno << ")" << std::endl;
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
    sigaction(SIGINT, &sa, NULL);
}

static void sighup_handler(int sig)
{
    std::clog << syslogInfo
              << "received SIGHUP, restarting application" << std::endl;
    complete_cleanup();
    complete_startup();
}

static void sigusr1_handler(int sig)
{
    std::clog << syslogInfo
              << "received SIGUSR1, reloading configuration" << std::endl;
    cleanup_configuration();
    setup_configuration(0, NULL);
}

static void sigusr2_handler(int sig)
{
    std::clog << syslogInfo
              << "received SIGUSR2, reloading zone (unimplemented)"
              << std::endl;
}

static void sigterm_handler(int sig)
{
    std::clog << syslogInfo << "received SIGTERM, terminating" << std::endl;
    set_exit_flag();
}

static void sigint_handler(int sig)
{
    std::clog << syslogInfo << "received SIGINT, terminating" << std::endl;
    set_exit_flag();
}
