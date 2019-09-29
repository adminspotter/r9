/* r9mysql.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 Sep 2019, 09:07:34 tquirk
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
 * This file contains the derived MySQL database class.
 *
 * Things to do
 *
 */

#ifndef __INC_R9MYSQL_H__
#define __INC_R9MYSQL_H__

#include <mysql.h>

#include <cstdint>

#include "db.h"

class MySQL : public DB
{
  private:
    static const char check_authentication_query[239];
    static const char check_authorization_id_query[187];
    static const char check_authorization_name_query[189];
    static const char get_characterid_query[120];
    static const char get_character_objectid_query[187];
    static const char get_server_skills_query[141];
    static const char get_server_objects_query[87];
    static const char get_player_server_skills_query[271];
    static const char get_serverid_query[40];

  public:
    MySQL(const std::string&, const std::string&,
          const std::string&, const std::string&);
    ~MySQL();

    /* Player functions */
    uint64_t check_authentication(const std::string&, const uint8_t *, size_t);
    int check_authorization(uint64_t, uint64_t);
    int check_authorization(uint64_t, const std::string&);
    uint64_t get_characterid(uint64_t, const std::string&);
    uint64_t get_character_objectid(uint64_t, const std::string&);
    int get_player_server_skills(uint64_t, uint64_t,
                                 std::map<uint16_t,
                                 action_level>&);

    /* Server functions */
    int get_server_skills(std::map<uint16_t, action_rec>&);
    int get_server_objects(std::map<uint64_t, GameObject *> &);

  private:
    MYSQL *db_connect(void);
};

#endif /* __INC_R9MYSQL_H__ */
