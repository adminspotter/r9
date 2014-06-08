/* pgsql.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 31 May 2014, 10:39:38 tquirk
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
 *
 * Things to do
 *
 */

#ifndef __INC_PGSQL_H__
#define __INC_PGSQL_H__

#include <postgresql/libpq-fe.h>

#include "db.h"

class PgSQL : public DB
{
  private:
    PGconn *db_handle;

  public:
    PgSQL(const char *, const char *, const char *, const char *);
    ~PgSQL();

  private:
    bool db_connect(void);
    void db_close(void);
};

#endif /* __INC_PGSQL_H__ */
