/* db.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 09 Jul 2014, 11:33:11 trinityquirk
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
 *   09 Jul 2014 TAQ - Switched everything over to just throwing exceptions.
 *                     No syslogging here - let our callers handle that.
 *
 * Things to do
 *
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <arpa/inet.h>

#include <sstream>
#include <stdexcept>

#include "db.h"

void DB::get_host_address(void)
{
    struct addrinfo *info;
    char hostname[128];
    int ret;

    if ((ret = gethostname(hostname, sizeof(hostname))) != 0)
    {
        std::ostringstream s;
	s << "couldn't get hostname: "
          << strerror(errno) << " (" << errno << ")";
	throw std::runtime_error(s.str());
    }
    if ((ret = getaddrinfo(hostname, NULL, NULL, &info)) != 0)
    {
        std::ostringstream s;
        s << "couldn't get address for " << hostname << ": "
          << gai_strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }
    if (inet_ntop(info->ai_family,
                  info->ai_family == AF_INET
                  ? (void *)&(reinterpret_cast<struct sockaddr_in *>(info->ai_addr)->sin_addr)
                  : (void *)&(reinterpret_cast<struct sockaddr_in6 *>(info->ai_addr)->sin6_addr),
                  this->host_ip, INET6_ADDRSTRLEN) == NULL)
    {
        std::ostringstream s;
        s << "couldn't convert IP for " << hostname << " into a string: "
          << strerror(errno) << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }
    freeaddrinfo(info);
}

DB::DB(const std::string& host, const std::string& user,
       const std::string& pass, const std::string& name)
    : dbhost(host), dbuser(user), dbpass(pass), dbname(name)
{
    this->get_host_address();
}

DB::~DB()
{
}
