/* pgsql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 09 Jul 2014, 12:12:52 trinityquirk
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
 * This file contains the routines which store and retrieve data from
 * our user database files using PostgreSQL.  This is not likely to
 * be the final database system that we will use, but it's supported.
 *
 * Changes
 *   25 May 1998 TAQ - Created the file.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   29 Sep 2007 TAQ - Renamed the file.  Started fleshing some stuff out,
 *                     and using the current functions (the libpq funcs
 *                     have changed a bit since 1998).
 *   13 Sep 2013 TAQ - Just wanted this to compile, so updated the function
 *                     prototypes, and changed the filename to pgsql.cc.
 *   31 May 2014 TAQ - We're now a subclass of DB.
 *   22 Jun 2014 TAQ - Constructor changed in the base, so we need to also.
 *   01 Jul 2014 TAQ - check_authentication now takes std::string&.
 *   09 Jul 2014 TAQ - Exceptionified this class.  No more syslog.
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

u_int64_t PgSQL::check_authentication(const std::string& user, const std::string& pass)
{
    PGresult *res;
    char str[256];
    u_int64_t retval = 0;

    snprintf(str, sizeof(str),
             "SELECT playerid "
             "FROM players "
             "WHERE username='%s' "
             "AND password='%s' "
             "AND suspended=0;",
             user, pass);
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

int PgSQL::check_authorization(u_int64_t userid, u_int64_t charid)
{
    return 0;
}

int PgSQL::get_server_skills(std::map<u_int16_t, action_rec>& actions)
{
    return 0;
}

int PgSQL::get_server_objects(std::map<u_int64_t, game_object_list_element> &gomap)
{
    return 0;
}

int PgSQL::get_player_server_skills(u_int64_t userid,
                                    u_int64_t charid,
                                    std::map<u_int16_t, action_level>& actions)
{
    return 0;
}

int PgSQL::open_new_login(u_int64_t userid, u_int64_t charid)
{
    return 0;
}

int PgSQL::check_open_login(u_int64_t userid, u_int64_t charid)
{
    return 0;
}

int PgSQL::close_open_login(u_int64_t userid, u_int64_t charid)
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

extern "C" DB *create_db(const char *a, const char *b,
                         const char *c, const char *d)
{
    return new PgSQL(a, b, c, d);
}

extern "C" void destroy_db(DB *db)
{
    delete db;
}
