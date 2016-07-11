/* pgsql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 11 Jul 2016, 18:40:46 tquirk
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
#include <inttypes.h>

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
    PGresult *res;
    char str[256];
    int retval = ACCESS_NONE;

    snprintf(str, sizeof(str),
             "SELECT c.access_type "
             "FROM players AS a, characters AS b, server_access AS c, "
             "servers AS d "
             "WHERE a.playerid=%" PRIu64 " "
             "AND a.playerid=b.owner "
             "AND b.characterid=%" PRIu64 " "
             "AND b.characterid=c.characterid "
             "AND c.serverid=d.serverid "
             "AND d.ip='%s'",
             userid, charid, this->host_ip);
    this->db_connect();

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    this->db_close();
    return retval;
}

int PgSQL::check_authorization(uint64_t userid, const std::string& charname)
{
    PGresult *res;
    char str[256];
    int retval = ACCESS_NONE;

    snprintf(str, sizeof(str),
             "SELECT c.access_type "
             "FROM players AS a, characters AS b, server_access AS c, "
             "servers AS d "
             "WHERE a.playerid=%" PRIu64 " "
             "AND a.playerid=b.owner "
             "AND b.charactername='%.*s' "
             "AND b.characterid=c.characterid "
             "AND c.serverid=d.serverid "
             "AND d.ip='%s'",
             userid, DB::MAX_CHARNAME, charname.c_str(), this->host_ip);
    this->db_connect();

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = atol(PQgetvalue(res, 0, 0), NULL, 10);
    PQclear(res);
    this->db_close();
    return retval;
}

uint64_t PgSQL::get_character_objectid(const std::string& charname)
{
    PGresult *res;
    char str[256];
    uint64_t retval = 0;

    snprintf(str, sizeof(str),
             "SELECT c.objectid "
             "FROM characters AS a, servers AS b, server_objects AS c "
             "WHERE a.charactername='%.*s' "
             "AND a.characterid = c.characterid "
             "AND b.serverid=c.serverid "
             "AND b.ip='%s'",
             DB::MAX_CHARNAME, charname.c_str(), this->host_ip);
    this->db_connect();

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
    PQclear(res);
    this->db_close();
    return retval;
}

int PgSQL::get_server_skills(std::map<uint16_t, action_rec>& actions)
{
    PGresult *res;
    char str[256];
    int count = 0, num_tuples;

    snprintf(str, sizeof(str),
             "SELECT b.skillname, c.skillid, c.defaultid, c.lower, c.upper "
             "FROM skills AS b, servers AS a, server_skills AS c "
             "WHERE a.ip='%s' "
             "AND a.serverid=c.serverid "
             "AND b.skillid=c.skillid",
             this->host_ip);
    this->db_connect();

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        num_tuples = PQntuples(res);
        for (count = 0; count < num_tuples; ++count)
        {
            uint64_t id = strtoull(PQgetvalue(res, count, 1), NULL, 10);

            actions[id].name = PQgetvalue(res, count, 0);
            actions[id].def = strtoull(PQgetvalue(res, count, 2), NULL, 10);
            actions[id].lower = atoi(PQgetvalue(res, count, 3));
            actions[id].upper = atoi(PQgetvalue(res, count, 4));
            actions[id].valid = true;
        }
    }
    PQclear(res);
    this->db_close();
    return count;
}

int PgSQL::get_server_objects(std::map<uint64_t, GameObject *> &gomap)
{
    PGresult *res;
    char str[256];
    int count = 0, num_tuples;

    snprintf(str, sizeof(str),
             "SELECT b.objectid, b.characterid, b.pos_x, b.pos_y, b.pos_z "
             "FROM servers AS a, server_objects AS b "
             "WHERE a.ip='%s' "
             "AND a.serverid=b.serverid",
             this->host_ip);
    this->db_connect();

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        num_tuples = PQntuples(res);
        for (count = 0; count < num_tuples; ++count)
        {
            uint64_t objid = strtoull(PQgetvalue(res, count, 0), NULL, 10);
            uint64_t charid = strtoull(PQgetvalue(res, count, 1), NULL, 10);

            GameObject *go = new GameObject(NULL, NULL, objid);

            go->position.x = atol(PQgetvalue(res, count, 2)) / 100.0;
            go->position.y = atol(PQgetvalue(res, count, 3)) / 100.0;
            go->position.z = atol(PQgetvalue(res, count, 4)) / 100.0;
            if (charid != 0LL)
            {
                go->natures.insert("invisible");
                go->natures.insert("non-interactive");
            }
            gomap[objid] = go;
        }
    }
    PQclear(res);
    this->db_close();
    return count;
}

int PgSQL::get_player_server_skills(uint64_t userid,
                                    uint64_t charid,
                                    std::map<uint16_t, action_level>& actions)
{
    PGresult *res;
    char str[256];
    int count = 0, num_tuples;

    snprintf(str, sizeof(str),
             "SELECT e.skillid, e.level, e.improvement, "
             "UNIX_TIMESTAMP(e.last_increase) "
             "FROM players AS a, characters AS b, servers AS c, "
             "server_skills AS d, character_skills AS e "
             "WHERE a.playerid=%" PRIu64 " "
             "AND a.playerid=b.owner "
             "AND b.characterid=%" PRIu64 " "
             "AND b.characterid=e.characterid "
             "AND c.ip='%s' "
             "AND c.serverid=d.serverid "
             "AND d.skillid=e.skillid",
             userid, charid, this->host_ip);
    this->db_connect();

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        num_tuples = PQntuples(res);
        for (count = 0; count < num_tuples; ++count)
        {
            uint64_t id = strtoull(PQgetvalue(res, count, 0), NULL, 10);

            actions[id].level = atoi(PQgetvalue(res, count, 1));
            actions[id].improvement = atoi(PQgetvalue(res, count, 2));
            actions[id].last_level = (time_t)atol(PQgetvalue(res, count, 3));
        }
    }
    PQclear(res);
    this->db_close();
    return count;
}

int PgSQL::open_new_login(uint64_t userid, uint64_t charid, Sockaddr *sa)
{
    PGresult *res;
    char str[256];
    int retval = 0;

    this->db_connect();
    snprintf(str, sizeof(str),
             "INSERT INTO player_logins "
             "(playerid, characterid, serverid, src_ip, src_port) "
             "VALUES (%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",'%s',%d)",
             userid, charid, this->host_id, sa->hostname(), sa->port());

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_COMMAND_OK)
        retval = 1;
    PQclear(res);
    this->db_close();
    return retval;
}

int PgSQL::check_open_login(uint64_t userid, uint64_t charid)
{
    return 0;
}

int PgSQL::close_open_login(uint64_t userid, uint64_t charid, Sockaddr *sa)
{
    PGresult *res;
    char str[256];
    int retval = 0;

    this->db_connect();
    snprintf(str, sizeof(str),
             "UPDATE player_logins SET logout_time=NOW() "
             "WHERE playerid=%" PRIu64 " "
             "AND characterid=%" PRIu64 " "
             "AND serverid=%" PRIu64 " "
             "AND src_ip='%s' "
             "AND src_port=%d "
             "AND logout_time IS NULL",
             userid, charid, this->host_id, sa->hostname(), sa->port());

    res = PQexec(this->db_handle, str);
    if (PQresultStatus(res) == PGRES_COMMAND_OK)
        retval = 1;
    PQclear(res);
    this->db_close();
    return retval;
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
