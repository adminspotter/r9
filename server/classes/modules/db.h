/* db.h                                                     -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 13:14:34 tquirk
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
 * This file contains the base class for the database objects.  Most of the
 * methods will be pure virtual, since they don't really mean anything
 * without a specific database context.
 *
 * Things to do
 *
 */

#ifndef __INC_DB_H__
#define __INC_DB_H__

#include <string>
#include <map>

#include "../defs.h"

class DB
{
  protected:
    const std::string dbhost, dbuser, dbpass, dbname;

  public:
    /* A couple of maximum lengths */
    static const int MAX_USERNAME = 64;
    static const int MAX_PASSWORD = 64;

  protected:
    /* IPv6 addresses can be a lot longer than IPv4, so we'll just use
     * the IPv6 max length to ensure that everything will fit.
     */
    char host_ip[INET6_ADDRSTRLEN];

    void get_host_address(void);

  public:
    DB(const std::string&, const std::string&,
       const std::string&, const std::string&);
    virtual ~DB();

    /* Player functions */
    virtual uint64_t check_authentication(const std::string&,
                                           const std::string&) = 0;
    virtual int check_authorization(uint64_t, uint64_t) = 0;
    virtual int open_new_login(uint64_t, uint64_t) = 0;
    virtual int check_open_login(uint64_t, uint64_t) = 0;
    virtual int close_open_login(uint64_t, uint64_t) = 0;
    virtual int get_player_server_skills(uint64_t, uint64_t,
                                         std::map<uint16_t,
                                         action_level>&) = 0;

    /* Server functions */
    virtual int get_server_skills(std::map<uint16_t, action_rec>&) = 0;
    virtual int get_server_objects(std::map<uint64_t,
                                   game_object_list_element> &) = 0;
};

/* Our database types will be dynamically loaded, so these typedefs
 * will simplify loading the symbols from the library.
 */
typedef DB *create_db_t(const std::string&, const std::string&,
                        const std::string&, const std::string&);
typedef void destroy_db_t(DB *);


#endif /* __INC_DB_H__ */
