/* r9mysql.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Oct 2021, 22:16:40 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2021  Trinity Annabelle Quirk
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

#include <config.h>

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

/* MySQL 8 dropped the my_bool type, so we'll reinstate it if it's missing */
#ifndef HAVE_MY_BOOL
typedef bool my_bool;
#endif

const char MySQL::check_authentication_query[] =
    "SELECT a.playerid "
    "FROM players AS a, player_keys AS b "
    "WHERE a.username=? "
    "AND a.playerid=b.playerid "
    "AND b.public_key=? "
    "AND b.not_before <= NOW() "
    "AND (b.not_after IS NULL OR b.not_after >= NOW()) "
    "AND a.suspended=0 "
    "ORDER BY b.not_before DESC";
const char MySQL::check_authorization_id_query[] =
    "SELECT c.access_type "
    "FROM players AS a, characters AS b, server_access AS c "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.characterid=? "
    "AND b.characterid=c.characterid "
    "AND c.serverid=?";
const char MySQL::check_authorization_name_query[] =
    "SELECT c.access_type "
    "FROM players AS a, characters AS b, server_access AS c "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.charactername=? "
    "AND b.characterid=c.characterid "
    "AND c.serverid=?";
const char MySQL::get_characterid_query[] =
    "SELECT b.characterid "
    "FROM players AS a, characters AS b "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.charactername=?";
const char MySQL::get_character_objectid_query[] =
    "SELECT c.objectid "
    "FROM players AS a, characters AS b, server_objects AS c "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.charactername=? "
    "AND b.characterid=c.characterid "
    "AND c.serverid=?";
const char MySQL::get_server_skills_query[] =
    "SELECT a.skillname, b.skillid, b.defaultid, b.lower, b.upper "
    "FROM skills AS a, server_skills AS b "
    "WHERE a.skillid=b.skillid "
    "AND b.serverid=?";
const char MySQL::get_server_objects_query[] =
    "SELECT objectid, characterid, pos_x, pos_y, pos_z "
    "FROM server_objects "
    "WHERE serverid=?";
const char MySQL::get_player_server_skills_query[] =
    "SELECT d.skillid, d.level, d.improvement, d.last_increase "
    "FROM players AS a, characters AS b, "
    "server_skills AS c, character_skills AS d "
    "WHERE a.playerid=? "
    "AND a.playerid=b.owner "
    "AND b.characterid=? "
    "AND b.characterid=d.characterid "
    "AND c.serverid=? "
    "AND c.skillid=d.skillid";
const char MySQL::get_serverid_query[] =
    "SELECT serverid FROM servers WHERE ip=?";

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
                       MySQL::check_authentication_query,
                       strlen(MySQL::check_authentication_query));

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
                       MySQL::check_authorization_id_query,
                       strlen(MySQL::check_authorization_id_query));

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
                       MySQL::check_authorization_name_query,
                       strlen(MySQL::check_authorization_name_query));

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
                       MySQL::get_characterid_query,
                       strlen(MySQL::get_characterid_query));

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
                       MySQL::get_character_objectid_query,
                       strlen(MySQL::get_character_objectid_query));

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
int MySQL::get_server_skills(actions_map& actions)
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
                       MySQL::get_server_skills_query,
                       strlen(MySQL::get_server_skills_query));

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

int MySQL::get_server_objects(GameObject::objects_map& gomap)
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
                       MySQL::get_server_objects_query,
                       strlen(MySQL::get_server_objects_query));

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

        go->set_position(glm::dvec3(pos_x / POSUPD_POS_SCALE,
                                    pos_y / POSUPD_POS_SCALE,
                                    pos_z / POSUPD_POS_SCALE));
        if (charid != 0LL)
            go->deactivate();
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
                                    Control::skills_map& actions)
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
                       MySQL::get_player_server_skills_query,
                       strlen(MySQL::get_player_server_skills_query));

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
                           MySQL::get_serverid_query,
                           strlen(MySQL::get_serverid_query));

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
