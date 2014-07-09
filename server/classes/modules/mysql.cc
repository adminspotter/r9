/* mysql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 09 Jul 2014, 15:14:55 trinityquirk
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
 * This file contains the MySQL routines to do the common database tasks.
 *
 * Changes
 *   20 Feb 1999 TAQ - Created the file.
 *   23 Feb 1999 TAQ - Wrote the db_connect routine.  The suspended
 *                     field is now an enum instead of an integer.
 *                     We might save a few bytes that way.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   11 Aug 2006 TAQ - Made the login checker a little cleaner.  The suspended
 *                     field is a tinyint, which should be like a byte.
 *   17 Aug 2006 TAQ - We now use mysql_init instead of allocating by hand,
 *                     since it just wasn't working the old way.  Also fixed
 *                     the table name in check_authentication.
 *   11 Jun 2007 TAQ - Removed save_last_login function.  Added some comments
 *                     on which database functions we might need.  Added
 *                     check_character_access and check_authorization.
 *                     Stubbed out get_server_skills and
 *                     get_player_server_skills.
 *   12 Jun 2007 TAQ - Fleshed out get_server_skills and
 *                     get_player_server_skills a bit.
 *   13 Jun 2007 TAQ - Removed check_character_access as redundant to
 *                     check_authorization.  Added get_host_address func
 *                     and call it whenever we need to verify the local
 *                     server.
 *   14 Jun 2007 TAQ - Added open_new_login, check_open_login, and
 *                     close_open_login routines.
 *   23 Aug 2007 TAQ - Commented references to server_port, since we've
 *                     changed the way our listening ports are allocated.
 *   09 Sep 2007 TAQ - Commented some stuff out for a clean compile.
 *   29 Sep 2007 TAQ - Tweaked prototype of check_authorization, since
 *                     an action request actually works on objectids,
 *                     rather than character names.  Minor other cleanups.
 *   11 Oct 2007 TAQ - Removed commented ports from all calls, since they
 *                     are not terribly meaningful anymore.  Fleshed out
 *                     get_server_skills.  Added some field-length limits
 *                     to the string arguments to snprintf.  This really
 *                     is a C++ file now, so we're changing the name.
 *   22 Nov 2009 TAQ - Changed the casting of in_addr_t to unsigned long,
 *                     despite the fact that it will not work properly
 *                     on a 64-bit machine.
 *   24 Nov 2009 TAQ - Fixed get_host_address's call to gethostbyname_r.
 *   10 May 2014 TAQ - Repaired some comments.
 *   25 May 2014 TAQ - This is now a subclass of the new DB object.  Moved
 *                     get_host_address into the DB class.
 *   31 May 2014 TAQ - The stringified version of our IP is now an instance
 *                     member, so we don't have to compute it when we need it.
 *   22 Jun 2014 TAQ - Constructor changed in the base, so we're changing too.
 *   24 Jun 2014 TAQ - Updated logging to use std::clog.  Small tweaks to
 *                     get things to compile properly.
 *   01 Jul 2014 TAQ - check_authentication now takes std::string&.
 *   09 Jul 2014 TAQ - We're now fully exception-happy.
 *
 * Things to do
 *   - Finish writing open_new_login and close_open_login.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>

#include "mysql.h"
#include "../game_obj.h"

MySQL::MySQL(const std::string& host, const std::string& user,
             const std::string& pass, const std::string& db)
    : DB(host, user, pass, db)
{
}

MySQL::~MySQL()
{
}

/* Check that the user really is who he says he is */
u_int64_t MySQL::check_authentication(const std::string& user,
                                      const std::string& pass)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[256];
    u_int64_t retval = 0;

    snprintf(str, sizeof(str),
	     "SELECT playerid "
	     "FROM players "
	     "WHERE username='%.*s' "
	     "AND password=SHA1('%.*s') "
	     "AND suspended=0",
	     DB::MAX_USERNAME, user.c_str(),
	     DB::MAX_PASSWORD, pass.c_str());
    this->db_connect();

    if (mysql_real_query(this->db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(this->db_handle)) != NULL
        && (row = mysql_fetch_row(res)) != NULL)
    {
        retval = atol(row[0]);
        mysql_free_result(res);
    }
    mysql_close(this->db_handle);

    /* Don't leave passwords lying around in memory */
    memset(str, 0, sizeof(str));
    return retval;
}

/* See what kind of access the user/character is allowed on this server */
int MySQL::check_authorization(u_int64_t userid, u_int64_t charid)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[256];
    int retval = 0;

    snprintf(str, sizeof(str),
	     "SELECT c.access_type "
	     "FROM players AS a, characters AS b, server_access AS c, "
	     "servers AS d "
	     "WHERE a.playerid=%lld "
	     "AND a.playerid=b.owner "
	     "AND b.characterid=%lld "
	     "AND b.characterid=c.characterid "
	     "AND c.serverid=d.serverid "
	     "AND d.ip='%s'",
	     userid, charid, this->host_ip);
    this->db_connect();

    if (mysql_real_query(this->db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(this->db_handle)) != NULL
        && (row = mysql_fetch_row(res)) != NULL)
    {
        retval = atoi(row[0]);
        mysql_free_result(res);
    }
    mysql_close(this->db_handle);
    return retval;
}

/* Get the list of skills that are used on this server */
int MySQL::get_server_skills(std::map<u_int16_t, action_rec>& actions)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[256];
    int count = 0;

    snprintf(str, sizeof(str),
	     "SELECT b.skillname, c.skillid, c.defaultid, c.lower, c.upper "
	     "FROM skills AS b, servers AS a, server_skills AS c "
	     "WHERE a.ip='%s' "
	     "AND a.serverid=c.serverid "
	     "AND b.skillid=c.skillid",
             this->host_ip);
    this->db_connect();

    if (mysql_real_query(this->db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(this->db_handle)) != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            u_int64_t id = strtoull(row[1], NULL, 10);

            if (actions[id].name != NULL)
                free(actions[id].name);
            actions[id].name = strdup(row[0]);
            actions[id].def = strtoull(row[2], NULL, 10);
            actions[id].lower = atoi(row[3]);
            actions[id].upper = atoi(row[4]);
            actions[id].valid = true;
            ++count;
        }
        mysql_free_result(res);
    }
    mysql_close(this->db_handle);
    return count;
}

int MySQL::get_server_objects(std::map<u_int64_t,
                              game_object_list_element> &gomap)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[256];
    int count = 0;

    snprintf(str, sizeof(str),
	     "SELECT b.objectid, b.pos_x, b.pos_y, b.pos_z "
	     "FROM servers AS a, server_characters AS b "
	     "WHERE a.ip='%s' "
             "AND a.serverid=b.serverid",
	     this->host_ip);
    this->db_connect();

    if (mysql_real_query(this->db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(this->db_handle)) != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            u_int64_t id = strtoull(row[0], NULL, 10);
            game_object_list_element gole;
            Geometry *geom = new Geometry();

            gole.obj = new GameObject(geom, id);
            gole.position[0] = atol(row[1]) / 100.0;
            gole.position[1] = atol(row[2]) / 100.0;
            gole.position[2] = atol(row[3]) / 100.0;
            /* All objects first rez invisible and non-interactive */
            gole.obj->natures["invisible"] = 1;
            gole.obj->natures["non-interactive"] = 1;
            gomap[id] = gole;
            ++count;
        }
        mysql_free_result(res);
    }
    mysql_close(this->db_handle);
    return count;
}

/* Get the list of a player's skills which are valid on this server */
int MySQL::get_player_server_skills(u_int64_t userid,
                                    u_int64_t charid,
                                    std::map<u_int16_t, action_level>& actions)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[256];
    int count = 0;

    snprintf(str, sizeof(str),
	     "SELECT e.skillid, e.level, e.improvement, "
	     "UNIX_TIMESTAMP(e.last_increase) "
	     "FROM players AS a, characters AS b, servers AS c, "
	     "server_skills AS d, character_skills AS e "
	     "WHERE a.playerid=%lld "
	     "AND a.playerid=b.owner "
	     "AND b.characterid=%lld "
	     "AND b.characterid=e.characterid "
	     "AND c.ip='%s' "
	     "AND c.serverid=d.serverid "
	     "AND d.skillid=e.skillid",
	     userid, charid, this->host_ip);
    this->db_connect();

    if (mysql_real_query(this->db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(this->db_handle)) != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            u_int64_t id = strtoull(row[0], NULL, 10);

            actions[id].level = atoi(row[1]);
            actions[id].improvement = atoi(row[2]);
            actions[id].last_level = (time_t)atol(row[3]);
            ++count;
        }
        mysql_free_result(res);
    }
    mysql_close(this->db_handle);
    return count;
}

int MySQL::open_new_login(u_int64_t userid, u_int64_t charid)
{
    /*MYSQL_RES *res;
    MYSQL_ROW row;
    char str[256];*/
    int retval = 0;

    /*snprintf(str, sizeof(str),
	     "INSERT INTO player_logins "
	     "(playerid, characterid, serverid, src_ip, src_port, login_time) "
	     "VALUES (%lld,%lld,%s,%d,%d)", userid, charid);
    this->db_connect();

    if ((retval = mysql_real_query(this->db_handle, str, strlen(str))) == 0)
        ;
    mysql_close(this->db_handle);*/
    return retval;
}

/* Returns count of open logins for the given player/char on this server */
int MySQL::check_open_login(u_int64_t userid, u_int64_t charid)
{
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[256];
    int retval = 0;

    snprintf(str, sizeof(str),
	     "SELECT COUNT(d.logout_time) "
	     "FROM players AS a, characters AS b, servers AS c, "
	     "player_logins AS d "
	     "WHERE a.playerid=%lld "
	     "AND a.playerid=d.playerid "
	     "AND a.playerid=b.owner "
	     "AND b.characterid=%lld "
	     "AND b.characterid=d.characterid "
	     "AND c.ip='%s' "
	     "AND c.serverid=d.serverid "
	     "AND d.logout_time IS NULL",
	     userid, charid, this->host_ip);
    this->db_connect();

    if (mysql_real_query(this->db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(this->db_handle)) != NULL
        && (row = mysql_fetch_row(res)) != NULL)
    {
        retval = atoi(row[0]);
        mysql_free_result(res);
    }
    mysql_close(this->db_handle);
    return retval;
}

int MySQL::close_open_login(u_int64_t userid, u_int64_t charid)
{
    return 0;
}

void MySQL::db_connect(void)
{
    if ((this->db_handle = mysql_init(NULL)) == NULL)
    {
        std::ostringstream s;
        s << "couldn't init MySQL handle: " << mysql_error(this->db_handle);
        throw std::runtime_error(s.str());
    }

    if (mysql_real_connect(this->db_handle, this->dbhost.c_str(),
                           this->dbuser.c_str(), this->dbpass.c_str(),
                           this->dbname.c_str(), 0, NULL, 0) == NULL)
    {
        std::ostringstream s;
        s << "couldn't connect to MySQL server: "
          << mysql_error(this->db_handle);
        throw std::runtime_error(s.str());
    }
}

extern "C" DB *create_db(const char *a, const char *b,
                         const char *c, const char *d)
{
    return new MySQL(a, b, c, d);
}

extern "C" void destroy_db(DB *db)
{
    delete db;
}
