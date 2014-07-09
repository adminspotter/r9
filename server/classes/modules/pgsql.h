/* pgsql.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 09 Jul 2014, 12:41:04 trinityquirk
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
 * This file contains the derived PostgreSQL database class.
 *
 * Changes
 *   31 May 2014 TAQ - Created the file.
 *   22 Jun 2014 TAQ - Constructor changed in the base, so we will too.
 *   01 Jul 2014 TAQ - Added primary function prototypes.  They're pure
 *                     virtual, but we still have to declare them.
 *   09 Jul 2014 TAQ - db_connect now returns nothing, and throws exceptions.
 *
 * Things to do
 *
 */

#ifndef __INC_PGSQL_H__
#define __INC_PGSQL_H__

#include <postgresql/libpq-fe.h>

#include <string>

#include "db.h"

class PgSQL : public DB
{
  private:
    PGconn *db_handle;

  public:
    PgSQL(const std::string&, const std::string&,
          const std::string&, const std::string&);
    ~PgSQL();

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
    void db_connect(void);
    void db_close(void);
};

#endif /* __INC_PGSQL_H__ */
