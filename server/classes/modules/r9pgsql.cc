/* r9pgsql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 28 Sep 2019, 10:11:08 tquirk
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

#include <sstream>
#include <stdexcept>

#include "r9pgsql.h"
#include "../game_obj.h"

const char PgSQL::check_authentication_query[] =
    "SELECT a.playerid "
    "FROM players AS a, player_keys AS b "
    "WHERE a.username=$1 "
    "AND a.playerid=b.playerid "
    "AND b.public_key=$2 "
    "AND b.not_before <= NOW() "
    "AND (b.not_after IS NULL OR b.not_after >= NOW()) "
    "AND a.suspended=0 "
    "ORDER BY b.not_before DESC;";
const char PgSQL::check_authorization_id_query[] =
    "SELECT c.access_type "
    "FROM players AS a, characters AS b, server_access AS c "
    "WHERE a.playerid=$1 "
    "AND a.playerid=b.owner "
    "AND b.characterid=$2 "
    "AND b.characterid=c.characterid "
    "AND c.serverid=$3;";
const char PgSQL::check_authorization_name_query[] =
    "SELECT c.access_type "
    "FROM players AS a, characters AS b, server_access AS c "
    "WHERE a.playerid=$1 "
    "AND a.playerid=b.owner "
    "AND b.charactername=$2 "
    "AND b.characterid=c.characterid "
    "AND c.serverid=$3;";
const char PgSQL::get_characterid_query[] =
    "SELECT b.characterid "
    "FROM players AS a, characters AS b "
    "WHERE a.playerid=$1 "
    "AND a.playerid=b.owner "
    "AND b.charactername=$2;";
const char PgSQL::get_character_objectid_query[] =
    "SELECT c.objectid "
    "FROM players AS a, characters AS b, server_objects AS c "
    "WHERE a.playerid=$1 "
    "AND a.playerid=b.owner "
    "AND b.charactername=$2 "
    "AND b.characterid=c.characterid "
    "AND c.serverid=$3;";
const char PgSQL::get_server_skills_query[] =
    "SELECT a.skillname, b.skillid, b.defaultid, b.lower, b.upper "
    "FROM skills AS a, server_skills AS b "
    "WHERE a.skillid=b.skillid "
    "AND b.serverid=$1;";
const char PgSQL::get_server_objects_query[] =
    "SELECT objectid, characterid, pos_x, pos_y, pos_z "
    "FROM server_objects "
    "WHERE serverid=$1;";
const char PgSQL::get_player_server_skills_query[] =
    "SELECT d.skillid, d.level, d.improvement, "
    "EXTRACT(EPOCH FROM d.last_increase) "
    "FROM players AS a, characters AS b, "
    "server_skills AS c, character_skills AS d "
    "WHERE a.playerid=$1 "
    "AND a.playerid=b.owner "
    "AND b.characterid=$2 "
    "AND b.characterid=d.characterid "
    "AND c.serverid=$3 "
    "AND c.skillid=d.skillid;";
const char PgSQL::get_serverid_query[] =
    "SELECT serverid FROM servers WHERE ip=$1;";

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
    PGconn *db_handle = this->db_connect();
    PGresult *stmt, *res;
    const char *vals[2] = {user.c_str(), (const char *)pubkey};
    const int lens[2] = {static_cast<int>(user.size()),
                         static_cast<int>(key_size)};
    const int binary[2] = {0, 1};
    uint64_t retval = 0;

    res = PQexecParams(db_handle,
                       PgSQL::check_authentication_query,
                       2, NULL,
                       vals, lens, binary, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

int PgSQL::check_authorization(uint64_t userid, uint64_t charid)
{
    PGconn *db_handle = this->db_connect();
    PGresult *res;
    std::string user_id = std::to_string(userid);
    std::string char_id = std::to_string(charid);
    std::string host_id = std::to_string(this->host_id);
    const char *vals[3] = {user_id.c_str(), char_id.c_str(), host_id.c_str()};
    int retval = ACCESS_NONE;

    res = PQexecParams(db_handle,
                       PgSQL::check_authorization_id_query,
                       3, NULL,
                       vals, NULL, NULL, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

int PgSQL::check_authorization(uint64_t userid, const std::string& charname)
{
    PGconn *db_handle = this->db_connect();
    PGresult *res;
    std::string user_id = std::to_string(userid);
    std::string host_id = std::to_string(this->host_id);
    const char *vals[3] = {user_id.c_str(), charname.c_str(), host_id.c_str()};
    int retval = ACCESS_NONE;

    res = PQexecParams(db_handle,
                       PgSQL::check_authorization_name_query,
                       3, NULL,
                       vals, NULL, NULL, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = atol(PQgetvalue(res, 0, 0));
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

uint64_t PgSQL::get_characterid(uint64_t userid, const std::string& charname)
{
    PGconn *db_handle = this->db_connect();
    PGresult *res;
    std::string user_id = std::to_string(userid);
    const char *vals[2] = {user_id.c_str(), charname.c_str()};
    uint64_t retval = 0;

    res = PQexecParams(db_handle,
                       PgSQL::get_characterid_query,
                       2, NULL,
                       vals, NULL, NULL, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

uint64_t PgSQL::get_character_objectid(uint64_t userid,
                                       const std::string& charname)
{
    PGconn *db_handle = this->db_connect();
    PGresult *res;
    std::string user_id = std::to_string(userid);
    std::string host_id = std::to_string(this->host_id);
    const char *vals[3] = {user_id.c_str(), charname.c_str(), host_id.c_str()};
    uint64_t retval = 0;

    res = PQexecParams(db_handle,
                       PgSQL::get_character_objectid_query,
                       3, NULL,
                       vals, NULL, NULL, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK && PQntuples(res) > 0)
        retval = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
    PQclear(res);
    this->db_close(db_handle);
    return retval;
}

int PgSQL::get_server_skills(std::map<uint16_t, action_rec>& actions)
{
    PGconn *db_handle = this->db_connect();
    PGresult *res;
    std::string host_id = std::to_string(this->host_id);
    const char *vals[1] = {host_id.c_str()};
    int count = 0;

    res = PQexecParams(db_handle,
                       PgSQL::get_server_skills_query,
                       1, NULL,
                       vals, NULL, NULL, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        int num_tuples = PQntuples(res);
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
    PGconn *db_handle = this->db_connect();
    PGresult *res;
    std::string host_id = std::to_string(this->host_id);
    const char *vals[1] = {host_id.c_str()};
    int count = 0;

    res = PQexecParams(db_handle,
                       PgSQL::get_server_objects_query,
                       1, NULL,
                       vals, NULL, NULL, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        int num_tuples = PQntuples(res);
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
    PGconn *db_handle = this->db_connect();
    PGresult *res;
    std::string user_id = std::to_string(userid);
    std::string char_id = std::to_string(charid);
    std::string host_id = std::to_string(this->host_id);
    const char *vals[3] = {user_id.c_str(), char_id.c_str(), host_id.c_str()};
    int count = 0;

    res = PQexecParams(db_handle,
                       PgSQL::get_player_server_skills_query,
                       3, NULL,
                       vals, NULL, NULL, 0);
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        int num_tuples = PQntuples(res);
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

    if (this->host_id == 0LL)
    {
        const char *vals[1] = {this->host_ip};
        PGresult *res = PQexecParams(db_handle,
                                     PgSQL::get_serverid_query,
                                     1, NULL,
                                     vals, NULL, NULL, 0);
        if (PQresultStatus(res) == PGRES_TUPLES_OK)
            this->host_id = strtoull(PQgetvalue(res, 0, 0), NULL, 10);
        PQclear(res);
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
