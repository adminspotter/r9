/* db.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 27 Feb 2018, 07:51:03 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2018  Trinity Annabelle Quirk
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

const char DB::check_authentication_query[] =
    "SELECT a.playerid "
    "FROM players AS a, player_keys AS b "
    "WHERE a.username=? "
    "AND a.playerid=b.playerid "
    "AND b.public_key=? "
    "AND b.not_before <= NOW() "
    "AND (b.not_after IS NULL OR b.not_after >= NOW()) "
    "AND a.suspended=0 "
    "ORDER BY b.not_before DESC";
const char DB::check_authorization_id_query[] =
    "SELECT c.access_type "
    "FROM players AS a, characters AS b, server_access AS c "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.characterid=? "
    "AND b.characterid=c.characterid "
    "AND c.serverid=?";
const char DB::check_authorization_name_query[] =
    "SELECT c.access_type "
    "FROM players AS a, characters AS b, server_access AS c "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.charactername=? "
    "AND b.characterid=c.characterid "
    "AND c.serverid=?";
const char DB::get_characterid_query[] =
    "SELECT b.characterid "
    "FROM players AS a, characters AS b "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.charactername=?";
const char DB::get_character_objectid_query[] =
    "SELECT c.objectid "
    "FROM players AS a, characters AS b, server_objects AS c "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.charactername=? "
    "AND b.characterid=c.characterid "
    "AND c.serverid=?";
const char DB::get_server_skills_query[] =
    "SELECT a.skillname, b.skillid, b.defaultid, b.lower, b.upper "
    "FROM skills AS a, server_skills AS b "
    "WHERE a.skillid=b.skillid "
    "AND b.serverid=?";
const char DB::get_server_objects_query[] =
    "SELECT objectid, characterid, pos_x, pos_y, pos_z "
    "FROM server_objects "
    "WHERE serverid=?";
const char DB::get_serverid_query[] =
    "SELECT serverid FROM servers WHERE ip=?";

void DB::get_host_address(void)
{
    struct addrinfo *info;
    char hostname[128];
    int ret;

    if ((ret = gethostname(hostname, sizeof(hostname))) != 0)
    {
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << "couldn't get hostname: " << err << " (" << errno << ")";
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
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << "couldn't convert IP for " << hostname << " into a string: "
          << err << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }
    freeaddrinfo(info);
}

DB::DB(const std::string& host, const std::string& user,
       const std::string& pass, const std::string& name)
    : dbhost(host), dbuser(user), dbpass(pass), dbname(name)
{
    this->host_id = 0LL;
    this->get_host_address();
}

DB::~DB()
{
}

