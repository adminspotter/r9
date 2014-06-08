/* db.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 07 Jun 2014, 14:41:08 tquirk
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
 * This file contains the base DB class.  Most of the interface is
 * just interface, so the real meat is within the derived,
 * DBMS-specific classes.
 *
 * Changes
 *   25 May 2014 TAQ - Created the file.
 *   31 May 2014 TAQ - No more conditional compiles!  The get_host_address
 *                     method now uses the new-style getaddrinfo, and
 *                     just saves the stringified IP.
 *   07 Jun 2014 TAQ - Some minor casting stuff in order to get this to
 *                     compile.  Also, get_host_address now throws exceptions
 *                     instead of trying to return things.
 *
 * Things to do
 *
 * $Id$
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#include "db.h"

void DB::get_host_address(void)
{
    struct addrinfo *info;
    char hostname[128];
    int ret;

    if ((ret = gethostname(hostname, sizeof(hostname))) != 0)
    {
	syslog(LOG_ERR, "couldn't get hostname: %s", strerror(errno));
	throw ret;
    }
    if ((ret = getaddrinfo(hostname, NULL, NULL, &info)) != 0)
    {
        syslog(LOG_ERR,
               "couldn't get address for %s: %s (%d)",
               hostname, gai_strerror(ret), ret);
        throw ret;
    }
    if (inet_ntop(info->ai_family,
                  info->ai_family == AF_INET
                  ? (void *)&(reinterpret_cast<struct sockaddr_in *>(info->ai_addr)->sin_addr)
                  : (void *)&(reinterpret_cast<struct sockaddr_in6 *>(info->ai_addr)->sin6_addr),
                  this->host_ip, INET6_ADDRSTRLEN) == NULL)
    {
        syslog(LOG_ERR,
               "couldn't convert IP for %s into a string: %s (%d)",
               hostname, strerror(errno), errno);
        throw errno;
    }
    freeaddrinfo(info);
}

DB::DB(const char *host, const char *user, const char *pass, const char *name)
    : dbhost(host), dbuser(user), dbpass(pass), dbname(name)
{
    try
    {
        this->get_host_address();
    }
    catch (int e)
    {
        syslog(LOG_INFO, "caught exception in get_host_address: %d", e);
    }
}

DB::~DB()
{
}
