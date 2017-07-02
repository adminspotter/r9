/* library.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 30 Jun 2017, 08:05:47 tquirk
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
 * This file contains the interface for the dynamic library class.
 *
 * Things to do
 *
 */

#ifndef __INC_LIBRARY_H__
#define __INC_LIBRARY_H__

#include <string>

class Library
{
  private:
    void *lib;
    std::string libname;

  public:
    Library(const std::string&);
    virtual ~Library();

    virtual void open(void);
    virtual void *symbol(const char *);
    virtual void close(void);
};

#endif /* __INC_LIBRARY_H__ */
