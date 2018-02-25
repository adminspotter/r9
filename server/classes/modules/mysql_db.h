/* mysql_db.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 21 Jan 2018, 09:09:39 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2018  Trinity Annabelle Quirk
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
 * This file contains the derived MySQL database class.
 *
 * Things to do
 *
 */

#ifndef __INC_MYSQL_DB_H__
#define __INC_MYSQL_DB_H__

#include <mysql.h>

#include <cstdint>

#include "db.h"

class MySQL : public DB
{
  private:
    MYSQL db_handle;

  public:
    MySQL(const std::string&, const std::string&,
          const std::string&, const std::string&);
    ~MySQL();

    /* Player functions */
    uint64_t check_authentication(const std::string&, const std::string&);
    int check_authorization(uint64_t, uint64_t);
    int check_authorization(uint64_t, const std::string&);
    uint64_t get_characterid(uint64_t, const std::string&);
    uint64_t get_character_objectid(uint64_t, const std::string&);
    int open_new_login(uint64_t, uint64_t, Sockaddr *);
    int check_open_login(uint64_t, uint64_t);
    int close_open_login(uint64_t, uint64_t, Sockaddr *);
    int get_player_server_skills(uint64_t, uint64_t,
                                 std::map<uint16_t,
                                 action_level>&);

    /* Server functions */
    int get_server_skills(std::map<uint16_t, action_rec>&);
    int get_server_objects(std::map<uint64_t, GameObject *> &);

  private:
    void db_connect(void);
};

#endif /* __INC_MYSQL_DB_H__ */
