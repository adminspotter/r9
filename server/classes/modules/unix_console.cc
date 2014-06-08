/* unix_console.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 08 Jun 2014, 14:45:21 tquirk
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
 * This file contains the unix file console.
 *
 * Changes
 *   31 May 2014 TAQ - Created the file.
 *
 * Things to do
 *
 */

#include <unistd.h>
#include <sys/un.h>
#include <syslog.h>
#include <errno.h>

#include "console.h"

UnixConsole::UnixConsole(char *fname)
    : console_fname(fname)
{
    this->open_socket();
    this->start(UnixConsole::listener);
}

UnixConsole::~UnixConsole()
{
}

void UnixConsole::open_socket(void)
{
    struct sockaddr_un sun;

    if ((this->console_sock = socket(PF_UNIX, SOCK_STREAM, 0)) < 0)
    {
	syslog(LOG_ERR, "socket creation failed for console %s: %s (%d)",
	       this->console_fname.c_str(), strerror(errno), errno);
        throw errno;
    }

    memset(&sun, 0, sizeof(struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    memcpy(&sun.sun_path,
           this->console_fname.c_str(),
           sizeof(sun.sun_path) - 1);

    if (bind(this->console_sock, (struct sockaddr *)&sun, sizeof(sun)) < 0)
    {
	syslog(LOG_ERR, "bind failed for console %s: %s (%d)",
	       this->console_fname.c_str(), strerror(errno), errno);
	close(this->console_sock);
        throw errno;
    }

    if (listen(this->console_sock, 5) < 0)
    {
	syslog(LOG_ERR, "listen failed for console %s: %s (%d)",
	       this->console_fname.c_str(), strerror(errno), errno);
	close(this->console_sock);
        throw errno;
    }

    syslog(LOG_DEBUG, "console %d created on %s",
           this->console_sock, this->console_fname.c_str());
}

void *UnixConsole::listener(void *arg)
{
    UnixConsole *con = (UnixConsole *)arg;
    int newsock;
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    ConsoleSession *sess = NULL;

    while ((newsock = accept(con->console_sock,
                             reinterpret_cast<struct sockaddr *>(&ss),
                             &ss_len)) != -1)
    {
        syslog(LOG_NOTICE,
               "began local console session on %s",
               con->console_fname.c_str());
        try
        {
            sess = new ConsoleSession(newsock);
        }
        catch (int e)
        {
            close(newsock);
            delete sess;
            continue;
        }
        con->sessions.push_back(sess);
    }
    pthread_exit(NULL);
    return NULL;
}
