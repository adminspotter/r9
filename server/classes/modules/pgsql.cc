/* pgsql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 31 May 2014, 10:39:13 tquirk
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
 *
 * Things to do
 *   - Implement the stubbed-out functions.
 *
 */

#define _XOPEN_SOURCE
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <time.h>

#include "server.h"
#include "pgsql.h"

PgSQL::PgSQL(const char *host, const char *user,
             const char *pass, const char *db)
    : DB(host, user, pass, db)
{
}

u_int64_t PgSQL::check_authentication(const char *user, const char *pass)
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
    if (db_connect())
    {
	res = PQexec(this->db_handle, str);
	if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
	    retval = atol(PQgetvalue(res, 0, 0));
	PQclear(res);
	db_close();
    }
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

bool PgSQL::db_connect(void)
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
	syslog(LOG_ERR,
	       "couldn't connect to PGSQL server: %s",
	       PQerrorMessage(this->db_handle));
	this->db_handle = NULL;
        return false;
    }
    return true;
}

void PgSQL::db_close(void)
{
    PQfinish(this->db_handle);
}
