/* r9pgsql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 Sep 2019, 09:24:37 tquirk
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
 * This file contains the routines which store and retrieve data from
 * our user database files using PostgreSQL.
 *
 * Things to do
 *
 */

#define _XOPEN_SOURCE
#include <string.h>
#include <unistd.h>
#include <time.h>

/* The PRIu64 type macros are not defined unless specifically
 * requested by the following macro.
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sstream>
#include <stdexcept>

#include "r9pgsql.h"
#include "../game_obj.h"

PgSQL::PgSQL(const std::string& host, const std::string& user,
             const std::string& pass, const std::string& db)
    : DB(host, user, pass, db)
{
}

PgSQL::~PgSQL()
{
}

uint64_t PgSQL::check_authentication(const std::string& user,
                                     const uint8_t *pubkey,
                                     size_t key_size)
{
    PGconn *db_handle;
    PGresult *res;
    char str[256];
    uint64_t retval = 0;

    snprintf(str, sizeof(str),
             "SELECT a.playerid, b.public_key, LEN(b.public_key) "
             "FROM players AS a, player_keys AS b "
             "WHERE a.username='%.*s' "
             "AND a.playerid=b.playerid "
             "AND b.not_before <= current_timestamp "
             "AND (b.not_after IS NULL OR b.not_after >= current_timestamp) "
             "AND a.suspended=0 "
             "ORDER BY b.not_before DESC;",
             DB::MAX_USERNAME, user.c_str());
    db_handle = this->db_connect();

    res = PQexec(db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        if (key_size == atoi(PQgetvalue(res, 0, 2))
            && !memcmp(PQgetvalue(res, 0, 1), pubkey, key_size))
            retval = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

int PgSQL::check_authorization(uint64_t userid, uint64_t charid)
{
    PGconn *db_handle;
    PGresult *res;
    char str[350];
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
    db_handle = this->db_connect();

    res = PQexec(db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

int PgSQL::check_authorization(uint64_t userid, const std::string& charname)
{
    PGconn *db_handle;
    PGresult *res;
    char str[350];
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
    db_handle = this->db_connect();

    res = PQexec(db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

uint64_t PgSQL::get_characterid(uint64_t userid, const std::string& charname)
{
    PGconn *db_handle;
    PGresult *res;
    char str[256];
    uint64_t retval = 0;

    snprintf(str, sizeof(str),
             "SELECT b.characterid "
             "FROM players AS a, characters AS b "
             "WHERE a.playerid=%" PRIu64 " "
             "AND a.playerid=b.owner "
             "AND b.charactername='%.*s'",
             userid, DB::MAX_CHARNAME, charname.c_str());
    db_handle = this->db_connect();

    res = PQexec(db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

uint64_t PgSQL::get_character_objectid(uint64_t userid,
                                       const std::string& charname)
{
    PGconn *db_handle;
    PGresult *res;
    char str[350];
    uint64_t retval = 0;

    snprintf(str, sizeof(str),
             "SELECT d.objectid "
             "FROM players AS a, characters AS a, "
             "servers AS b, server_objects AS c "
             "WHERE a.playerid=%" PRIu64 " "
             "AND a.playerid=b.owner "
             "AND b.charactername='%.*s' "
             "AND b.characterid = d.characterid "
             "AND c.serverid=d.serverid "
             "AND c.ip='%s'",
             userid, DB::MAX_CHARNAME, charname.c_str(), this->host_ip);
    db_handle = this->db_connect();

    res = PQexec(db_handle, str);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

int PgSQL::get_server_skills(std::map<uint16_t, action_rec>& actions)
{
    PGconn *db_handle;
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
    db_handle = this->db_connect();

    res = PQexec(db_handle, str);
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
    this->db_close(db_handle);
    return count;
}

int PgSQL::get_server_objects(std::map<uint64_t, GameObject *> &gomap)
{
    PGconn *db_handle;
    PGresult *res;
    char str[256];
    int count = 0, num_tuples;

    snprintf(str, sizeof(str),
             "SELECT b.objectid, b.characterid, b.pos_x, b.pos_y, b.pos_z "
             "FROM servers AS a, server_objects AS b "
             "WHERE a.ip='%s' "
             "AND a.serverid=b.serverid",
             this->host_ip);
    db_handle = this->db_connect();

    res = PQexec(db_handle, str);
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
    this->db_close(db_handle);
    return count;
}

int PgSQL::get_player_server_skills(uint64_t userid,
                                    uint64_t charid,
                                    std::map<uint16_t, action_level>& actions)
{
    PGconn *db_handle;
    PGresult *res;
    char str[420];
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
    db_handle = this->db_connect();

    res = PQexec(db_handle, str);
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
    this->db_close(db_handle);
    return count;
}

PGconn *PgSQL::db_connect(void)
{
    PGconn *db_handle = PQsetdbLogin(this->dbhost.c_str(),
                                     NULL,
                                     NULL,
                                     NULL,
                                     this->dbname.c_str(),
                                     this->dbuser.c_str(),
                                     this->dbpass.c_str());
    if (PQstatus(db_handle) == CONNECTION_BAD)
    {
        std::ostringstream s;
        s << "couldn't connect to PGSQL server: "
          << PQerrorMessage(db_handle);
        db_handle = NULL;
        throw std::runtime_error(s.str());
    }
    return db_handle;
}

void PgSQL::db_close(PGconn *db_handle)
{
    PQfinish(db_handle);
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
