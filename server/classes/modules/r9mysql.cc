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

#include <proto/ec.h>

#include <sstream>
#include <stdexcept>

#include "r9mysql.h"
#include "../game_obj.h"
#include "../../../proto/proto.h"

static time_t MYSQL_TIME_to_time_t(MYSQL_TIME *mt)
{
    time_t result = 0L;
    struct tm timeinfo;

    timeinfo.tm_year = mt->year;
    timeinfo.tm_mon = mt->month;
    timeinfo.tm_mday = mt->day;
    timeinfo.tm_hour = mt->hour;
    timeinfo.tm_min = mt->minute;
    timeinfo.tm_sec = mt->second;
    return mktime(&timeinfo);
}

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
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[3];
    unsigned long length[2];
    my_bool is_null, error;
    char retval = ACCESS_NONE;

    db_handle = this->db_connect();
    stmt = mysql_stmt_init(db_handle);
    mysql_stmt_prepare(stmt,
                       DB::check_authorization_name_query,
                       strlen(DB::check_authorization_name_query));

    length[0] = charname.size();
    length[1] = strlen(this->host_ip);
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &userid;
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void *)charname.c_str();
    bind[1].buffer_length = DB::MAX_CHARNAME;
    bind[1].length = &length[0];
    bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[2].is_unsigned = true;
    bind[2].buffer = &this->host_id;

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_TINY;
    bind[0].is_unsigned = false;
    bind[0].buffer = &retval;
    bind[0].length = &length[0];
    bind[0].is_null = &is_null;
    bind[0].error = &error;

    mysql_stmt_bind_result(stmt, bind);
    mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);
    mysql_close(db_handle);
    return (int)retval;
}

uint64_t MySQL::get_characterid(uint64_t userid, const std::string& charname)
{
    MYSQL *db_handle;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[2];
    unsigned long length;
    my_bool is_null, error;
    uint64_t retval = 0;

    db_handle = this->db_connect();
    stmt = mysql_stmt_init(db_handle);
    mysql_stmt_prepare(stmt,
                       DB::get_characterid_query,
                       strlen(DB::get_characterid_query));

    length = charname.size();
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &userid;
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void *)charname.c_str();
    bind[1].buffer_length = DB::MAX_CHARNAME;
    bind[1].length = &length;

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &retval;
    bind[0].length = &length;
    bind[0].is_null = &is_null;
    bind[0].error = &error;

    mysql_stmt_bind_result(stmt, bind);
    mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);
    mysql_close(db_handle);
    return retval;
}

uint64_t MySQL::get_character_objectid(uint64_t userid,
                                       const std::string& charname)
{
    MYSQL *db_handle;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[3];
    unsigned long length;
    my_bool is_null, error;
    uint64_t retval = 0;

    db_handle = this->db_connect();
    stmt = mysql_stmt_init(db_handle);
    mysql_stmt_prepare(stmt,
                       DB::get_character_objectid_query,
                       strlen(DB::get_character_objectid_query));

    length = charname.size();
    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &userid;
    bind[1].buffer_type = MYSQL_TYPE_STRING;
    bind[1].buffer = (void *)charname.c_str();
    bind[1].buffer_length = DB::MAX_CHARNAME;
    bind[1].length = &length;
    bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[2].is_unsigned = true;
    bind[2].buffer = &this->host_id;

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &retval;
    bind[0].length = &length;
    bind[0].is_null = &is_null;
    bind[0].error = &error;

    mysql_stmt_bind_result(stmt, bind);
    mysql_stmt_fetch(stmt);
    mysql_stmt_close(stmt);
    mysql_close(db_handle);
    return retval;
}

/* Get the list of skills that are used on this server */
int MySQL::get_server_skills(std::map<uint16_t, action_rec>& actions)
{
    MYSQL *db_handle;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[5];
    uint64_t id, def;
    int lower, upper;
    char name[DB::MAX_SKILLNAME + 1];
    unsigned long length[5];
    my_bool is_null[5], error[5];
    int count = 0;

    db_handle = this->db_connect();
    stmt = mysql_stmt_init(db_handle);
    mysql_stmt_prepare(stmt,
                       DB::get_server_skills_query,
                       strlen(DB::get_server_skills_query));

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &this->host_id;

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_STRING;
    bind[0].buffer = name;
    bind[0].buffer_length = sizeof(name);
    bind[0].length = &length[0];
    bind[0].is_null = &is_null[0];
    bind[0].error = &error[0];
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].is_unsigned = true;
    bind[1].buffer = &id;
    bind[1].length = &length[1];
    bind[1].is_null = &is_null[1];
    bind[1].error = &error[1];
    bind[2].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[2].is_unsigned = true;
    bind[2].buffer = &def;
    bind[2].length = &length[2];
    bind[2].is_null = &is_null[2];
    bind[2].error = &error[2];
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &lower;
    bind[3].length = &length[3];
    bind[3].is_null = &is_null[3];
    bind[3].error = &error[3];
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = &upper;
    bind[4].length = &length[4];
    bind[4].is_null = &is_null[4];
    bind[4].error = &error[4];

    mysql_stmt_bind_result(stmt, bind);
    mysql_stmt_store_result(stmt);
    while (mysql_stmt_fetch(stmt) == 0)
    {
        actions[id].name = name;
        actions[id].def = def;
        actions[id].lower = lower;
        actions[id].upper = upper;
        actions[id].valid = true;
        ++count;
    }
    mysql_stmt_close(stmt);
    mysql_close(db_handle);
    return count;
}

int MySQL::get_server_objects(std::map<uint64_t, GameObject *> &gomap)
{
    MYSQL *db_handle;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[5];
    uint64_t objid, charid;
    long pos_x, pos_y, pos_z;
    unsigned long length[5];
    my_bool is_null[5], error[5];
    int count = 0;

    db_handle = this->db_connect();
    stmt = mysql_stmt_init(db_handle);
    mysql_stmt_prepare(stmt,
                       DB::get_server_objects_query,
                       strlen(DB::get_server_objects_query));

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &this->host_id;

    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);

    memset(bind, 0, sizeof(bind));
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &objid;
    bind[0].length = &length[0];
    bind[0].is_null = &is_null[0];
    bind[0].error = &error[0];
    bind[1].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[1].is_unsigned = true;
    bind[1].buffer = &charid;
    bind[1].length = &length[1];
    bind[1].is_null = &is_null[1];
    bind[1].error = &error[1];
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &pos_x;
    bind[2].length = &length[2];
    bind[2].is_null = &is_null[2];
    bind[2].error = &error[2];
    bind[3].buffer_type = MYSQL_TYPE_LONG;
    bind[3].buffer = &pos_y;
    bind[3].length = &length[3];
    bind[3].is_null = &is_null[3];
    bind[3].error = &error[3];
    bind[4].buffer_type = MYSQL_TYPE_LONG;
    bind[4].buffer = &pos_z;
    bind[4].length = &length[4];
    bind[4].is_null = &is_null[4];
    bind[4].error = &error[4];

    mysql_stmt_bind_result(stmt, bind);
    mysql_stmt_store_result(stmt);
    while (mysql_stmt_fetch(stmt) == 0)
    {
        //Geometry *geom = new Geometry();
        GameObject *go = new GameObject(NULL, NULL, objid);

        go->position[0] = pos_x / 100.0;
        go->position[1] = pos_y / 100.0;
        go->position[2] = pos_z / 100.0;
        if (charid != 0LL)
        {
            /* All characters first rez invisible and non-interactive */
            go->natures.insert("invisible");
            go->natures.insert("non-interactive");
        }
        gomap[objid] = go;
        ++count;
    }
    mysql_stmt_close(stmt);
    mysql_close(db_handle);
    return count;
}

/* Get the list of a player's skills which are valid on this server */
int MySQL::get_player_server_skills(uint64_t userid,
                                    uint64_t charid,
                                    std::map<uint16_t, action_level>& actions)
{
    MYSQL *db_handle;
    MYSQL_STMT *stmt;
    MYSQL_BIND bind[4];
    uint64_t skillid;
    int level, improvement;
    MYSQL_TIME last_increase;
    unsigned long length[4];
    my_bool is_null[4], error[4];
    int count = 0;

    db_handle = this->db_connect();
    stmt = mysql_stmt_init(db_handle);
    mysql_stmt_prepare(stmt,
                       DB::get_player_server_skills_query,
                       strlen(DB::get_player_server_skills_query));

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
    bind[0].buffer_type = MYSQL_TYPE_LONGLONG;
    bind[0].is_unsigned = true;
    bind[0].buffer = &skillid;
    bind[0].length = &length[0];
    bind[0].is_null = &is_null[0];
    bind[0].error = &error[0];
    bind[1].buffer_type = MYSQL_TYPE_LONG;
    bind[1].buffer = &level;
    bind[1].length = &length[1];
    bind[1].is_null = &is_null[1];
    bind[1].error = &error[1];
    bind[2].buffer_type = MYSQL_TYPE_LONG;
    bind[2].buffer = &improvement;
    bind[2].length = &length[2];
    bind[2].is_null = &is_null[2];
    bind[2].error = &error[2];
    bind[3].buffer_type = MYSQL_TYPE_TIMESTAMP;
    bind[3].buffer = &last_increase;
    bind[3].length = &length[3];
    bind[3].is_null = &is_null[3];
    bind[3].error = &error[3];

    mysql_stmt_bind_result(stmt, bind);
    mysql_stmt_store_result(stmt);
    while (mysql_stmt_fetch(stmt) == 0)
    {
        actions[skillid].level = level;
        actions[skillid].improvement = improvement;
        actions[skillid].last_level = MYSQL_TIME_to_time_t(&last_increase);
        ++count;
    }
    mysql_stmt_close(stmt);
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
