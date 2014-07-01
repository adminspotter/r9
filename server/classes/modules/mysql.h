/* mysql.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Jul 2014, 17:45:16 tquirk
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
 * This file contains the derived MySQL database class.
 *
 * Changes
 *   25 May 2014 TAQ - Created the file.
 *   31 May 2014 TAQ - Removed all IP-related stuff to the DB class, and all
 *                     we need now is a string.  Also added the db_handle
 *                     member and db_connect method.
 *   22 Jun 2014 TAQ - Constructor changed in the base, so we're changing too.
 *   24 Jun 2014 TAQ - Changed include.  Added prototypes for virtuals we
 *                     inherit from DB.
 *   01 Jul 2014 TAQ - check_authentication now takes std::string&.
 *
 * Things to do
 *
 */

#ifndef __INC_MYSQL_H__
#define __INC_MYSQL_H__

#include <mysql.h>

#include "db.h"

class MySQL : public DB
{
  private:
    MYSQL *db_handle;

  public:
    MySQL(const std::string&, const std::string&,
          const std::string&, const std::string&);
    ~MySQL();

    /* Player functions */
    u_int64_t check_authentication(const std::string&, const std::string&);
    int check_authorization(u_int64_t, u_int64_t);
    int open_new_login(u_int64_t, u_int64_t);
    int check_open_login(u_int64_t, u_int64_t);
    int close_open_login(u_int64_t, u_int64_t);
    int get_player_server_skills(u_int64_t, u_int64_t,
                                 std::map<u_int16_t,
                                 action_level>&);

    /* Server functions */
    int get_server_skills(std::map<u_int16_t, action_rec>&);
    int get_server_objects(std::map<u_int64_t, game_object_list_element> &);

  private:
    bool db_connect(void);
};

#endif /* __INC_MYSQL_H__ */
