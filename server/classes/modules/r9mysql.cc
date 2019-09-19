/* r9mysql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 Sep 2019, 22:47:07 tquirk
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
 * This file contains the MySQL routines to do the common database tasks.
 *
 * Things to do
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

/* The PRIu64 type macros are not defined unless specifically
 * requested by the following macro.
 */
#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <proto/ec.h>

#include <sstream>
#include <stdexcept>

#include "r9mysql.h"
#include "../game_obj.h"
#include "../../../proto/proto.h"

MySQL::MySQL(const std::string& host, const std::string& user,
             const std::string& pass, const std::string& db)
    : DB(host, user, pass, db)
{
}

MySQL::~MySQL()
{
}

/* Check that the user really is who they say they are */
uint64_t MySQL::check_authentication(const std::string& user,
                                     const uint8_t *pubkey,
                                     size_t key_size)
{
    MYSQL *db_handle;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    unsigned long length[2];
    my_bool is_null, error;
    uint64_t retval = 0;

    db_handle = this->db_connect();
    stmt = mysql_stmt_init(db_handle);
    mysql_stmt_prepare(stmt,
                       DB::check_authentication_query,
                       strlen(DB::check_authentication_query));

    length[0] = user.size();
    length[1] = key_size;
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = (void *)user.c_str();
    bind[0].buffer_length = DB::MAX_USERNAME;
    bind[0].length = &length[0];
    bind[1].buffer_type = MYSQL_TYPE_BLOB;
    bind[1].is_unsigned = true;
    bind[1].buffer = (void *)pubkey;
    bind[1].buffer_length = R9_PUBKEY_SZ;
    bind[1].length = &length[1];

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &retval;
    bind[0].length = &length[0];
    bind[0].is_null = &is_null;
    bind[0].error = &error;

    mysql_stmt_bind_result(stmt, bind);
    mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);
    mysql_close(db_handle);
    return retval;
}

/* See what kind of access the user/character is allowed on this server */
int MySQL::check_authorization(uint64_t userid, uint64_t charid)
{
    MYSQL *db_handle;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[3];
    unsigned long length;
    my_bool is_null, error;
    char retval = ACCESS_NONE;

    db_handle = this->db_connect();
    stmt = mysql_stmt_init(db_handle);
    mysql_stmt_prepare(stmt,
                       DB::check_authorization_id_query,
                       strlen(DB::check_authorization_id_query));

    length = strlen(this->host_ip);
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &userid;
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].is_unsigned = true;
    bind[1].buffer = &charid;
    bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[2].is_unsigned = true;
    bind[2].buffer = &this->host_id;

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_TINY;
    bind[0].is_unsigned = false;
    bind[0].buffer = &retval;
    bind[0].length = &length;
    bind[0].is_null = &is_null;
    bind[0].error = &error;

    mysql_stmt_bind_result(stmt, bind);
    mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);
    mysql_close(db_handle);
    return (int)retval;
}

int MySQL::check_authorization(uint64_t userid, const std::string& charname)
{
    MYSQL *db_handle;
    MYSQL_RES *res;
    MYSQL_ROW row;
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
    if (mysql_real_query(db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(db_handle)) != NULL
        && (row = mysql_fetch_row(res)) != NULL)
    {
        retval = atoi(row[0]);
        mysql_free_result(res);
    }
    mysql_close(db_handle);
    return retval;
}

uint64_t MySQL::get_characterid(uint64_t userid, const std::string& charname)
{
    MYSQL *db_handle;
    MYSQL_RES *res;
    MYSQL_ROW row;
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
    if (mysql_real_query(db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(db_handle)) != NULL
        && (row = mysql_fetch_row(res)) != NULL)
    {
        retval = strtoull(row[0], NULL, 10);
        mysql_free_result(res);
    }
    mysql_close(db_handle);
    return retval;
}

uint64_t MySQL::get_character_objectid(uint64_t userid,
                                       const std::string& charname)
{
    MYSQL *db_handle;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[350];
    uint64_t retval = 0;

    snprintf(str, sizeof(str),
             "SELECT d.objectid "
             "FROM players AS a, characters AS b, "
             "servers AS c, server_objects AS d "
             "WHERE a.playerid=%" PRIu64 " "
             "AND a.playerid=b.owner "
             "AND b.charactername='%.*s' "
             "AND b.characterid=d.characterid "
             "AND c.serverid=d.serverid "
             "AND c.ip='%s'",
             userid, DB::MAX_CHARNAME, charname.c_str(), this->host_ip);

    db_handle = this->db_connect();
    if (mysql_real_query(db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(db_handle)) != NULL
        && (row = mysql_fetch_row(res)) != NULL)
    {
        retval = strtoull(row[0], NULL, 10);
        mysql_free_result(res);
    }
    mysql_close(db_handle);
    return retval;
}

/* Get the list of skills that are used on this server */
int MySQL::get_server_skills(std::map<uint16_t, action_rec>& actions)
{
    MYSQL *db_handle;
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

    db_handle = this->db_connect();
    if (mysql_real_query(db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(db_handle)) != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            uint64_t id = strtoull(row[1], NULL, 10);

            actions[id].name = row[0];
            actions[id].def = strtoull(row[2], NULL, 10);
            actions[id].lower = atoi(row[3]);
            actions[id].upper = atoi(row[4]);
            actions[id].valid = true;
            ++count;
        }
        mysql_free_result(res);
    }
    mysql_close(db_handle);
    return count;
}

int MySQL::get_server_objects(std::map<uint64_t, GameObject *> &gomap)
{
    MYSQL *db_handle;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[256];
    int count = 0;

    snprintf(str, sizeof(str),
             "SELECT b.objectid, b.characterid, b.pos_x, b.pos_y, b.pos_z "
             "FROM servers AS a, server_objects AS b "
             "WHERE a.ip='%s' "
             "AND a.serverid=b.serverid",
             this->host_ip);

    db_handle = this->db_connect();
    if (mysql_real_query(db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(db_handle)) != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            uint64_t objid = strtoull(row[0], NULL, 10);
            uint64_t charid = strtoull(row[1], NULL, 10);

            //Geometry *geom = new Geometry();
            GameObject *go = new GameObject(NULL, NULL, objid);

            go->position[0] = atol(row[2]) / 100.0;
            go->position[1] = atol(row[3]) / 100.0;
            go->position[2] = atol(row[4]) / 100.0;
            if (charid != 0LL)
            {
                /* All characters first rez invisible and non-interactive */
                go->natures.insert("invisible");
                go->natures.insert("non-interactive");
            }
            gomap[objid] = go;
            ++count;
        }
        mysql_free_result(res);
    }
    mysql_close(db_handle);
    return count;
}

/* Get the list of a player's skills which are valid on this server */
int MySQL::get_player_server_skills(uint64_t userid,
                                    uint64_t charid,
                                    std::map<uint16_t, action_level>& actions)
{
    MYSQL *db_handle;
    MYSQL_RES *res;
    MYSQL_ROW row;
    char str[420];
    int count = 0;

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
    if (mysql_real_query(db_handle, str, strlen(str)) == 0
        && (res = mysql_use_result(db_handle)) != NULL)
    {
        while ((row = mysql_fetch_row(res)) != NULL)
        {
            uint64_t id = strtoull(row[0], NULL, 10);

            actions[id].level = atoi(row[1]);
            actions[id].improvement = atoi(row[2]);
            actions[id].last_level = (time_t)atol(row[3]);
            ++count;
        }
        mysql_free_result(res);
    }
    mysql_close(db_handle);
    return count;
}

MYSQL *MySQL::db_connect(void)
{
    MYSQL *db_handle;

    if ((db_handle = mysql_init(NULL)) == NULL
        || mysql_real_connect(db_handle, this->dbhost.c_str(),
                              this->dbuser.c_str(), this->dbpass.c_str(),
                              this->dbname.c_str(), 0, NULL, 0) == NULL)
    {
        std::ostringstream s;
        if (db_handle == NULL)
            s << "couldn't allocate MySQL handle";
        else
        {
            s << "couldn't connect to MySQL server: "
              << mysql_error(db_handle);
            mysql_close(db_handle);
        }
        throw std::runtime_error(s.str());
    }

    /* Retrieve our server id if we haven't already gotten it. */
    if (this->host_id == 0LL)
    {
        MYSQL_STMT *stmt;
        MYSQL_BIND bind[1];
        unsigned long length = strlen(this->host_ip);
        my_bool is_null, error;

        stmt = mysql_stmt_init(db_handle);
        mysql_stmt_prepare(stmt,
                           DB::get_serverid_query,
                           strlen(DB::get_serverid_query));

        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_STRING;
        bind[0].buffer = this->host_ip;
        bind[0].buffer_length = INET6_ADDRSTRLEN;
        bind[0].length = &length;

        mysql_stmt_bind_param(stmt, bind);
        mysql_stmt_execute(stmt);

        memset(bind, 0, sizeof(bind));
        bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
        bind[0].is_unsigned = true;
        bind[0].buffer = &this->host_id;
        bind[0].length = &length;
        bind[0].is_null = &is_null;
        bind[0].error = &error;

        mysql_stmt_bind_result(stmt, bind);
        mysql_stmt_fetch(stmt);
        mysql_stmt_close(stmt);
    }
    return db_handle;
}

extern "C" DB *db_create(const std::string& a, const std::string& b,
                         const std::string& c, const std::string& d)
{
    mysql_library_init(0, NULL, NULL);
    return new MySQL(a, b, c, d);
}

extern "C" void db_destroy(DB *db)
{
    delete db;
    mysql_library_end();
}
