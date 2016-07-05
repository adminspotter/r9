/* pgsql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Jul 2016, 09:52:46 tquirk
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
 * This file contains the routines which store and retrieve data from
 * our user database files using PostgreSQL.  This is not likely to
 * be the final database system that we will use, but it's supported.
 *
 * Things to do
 *   - Implement the stubbed-out functions.
 *
 */

#define _XOPEN_SOURCE
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sstream>
#include <stdexcept>

#include "pgsql.h"

PgSQL::PgSQL(const std::string& host, const std::string& user,
             const std::string& pass, const std::string& db)
    : DB(host, user, pass, db)
{
}

uint64_t PgSQL::check_authentication(const std::string& user, const std::string& pass)
{
    PGresult *res;
    char str[256];
    uint64_t retval = 0;

    snprintf(str, sizeof(str),
             "SELECT playerid "
             "FROM players "
             "WHERE username='%s' "
             "AND password='%s' "
             "AND suspended=0;",
             user.c_str(), pass.c_str());
    this->db_connect();

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    this->db_close();

    /* Don't want to keep passwords around in core if we can help it */
    memset(str, 0, sizeof(str));
    return retval;
}

int PgSQL::check_authorization(uint64_t userid, uint64_t charid)
{
    return 0;
}

int PgSQL::check_authorization(uint64_t userid, const std::string& charname)
{
    return 0;
}

uint64_t PgSQL::get_character_objectid(const std::string& charname)
{
    return 0LL;
}

int PgSQL::get_server_skills(std::map<uint16_t, action_rec>& actions)
{
    return 0;
}

int PgSQL::get_server_objects(std::map<uint64_t, GameObject *> &gomap)
{
    return 0;
}

int PgSQL::get_player_server_skills(uint64_t userid,
                                    uint64_t charid,
                                    std::map<uint16_t, action_level>& actions)
{
    return 0;
}

int PgSQL::open_new_login(uint64_t userid, uint64_t charid, Sockaddr *sa)
{
    return 0;
}

int PgSQL::check_open_login(uint64_t userid, uint64_t charid)
{
    return 0;
}

int PgSQL::close_open_login(uint64_t userid, uint64_t charid, Sockaddr *sa)
{
    return 0;
}

void PgSQL::db_connect(void)
{
    this->db_handle = PQsetdbLogin(this->dbhost.c_str(),
                                   NULL,
                                   NULL,
                                   NULL,
                                   this->dbname.c_str(),
                                   this->dbuser.c_str(),
                                   this->dbpass.c_str());
    if (PQstatus(this->db_handle) == CONNECTION_BAD)
    {
        std::ostringstream s;
        s << "couldn't connect to PGSQL server: "
          << PQerrorMessage(this->db_handle);
        this->db_handle = NULL;
        throw std::runtime_error(s.str());
    }
}

void PgSQL::db_close(void)
{
    PQfinish(this->db_handle);
}

extern "C" DB *db_create(const std::string& a, const std::string& b,
                         const std::string& c, const std::string& d)
{
    return new PgSQL(a, b, c, d);
}

extern "C" void db_destroy(DB *db)
{
    delete db;
}
