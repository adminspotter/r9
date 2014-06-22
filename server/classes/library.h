/* library.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 22 Jun 2014, 15:48:23 tquirk
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
 * Changes
 *   18 Jun 2014 TAQ - Created the file.
 *   20 Jun 2014 TAQ - We'll use chars instead of strings.  Just simpler.
 *   22 Jun 2014 TAQ - Config object changed, so we've got a bunch of
 *                     strings hanging around.  It'll be easier to use
 *                     strings to construct.
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
    ~Library();

    void open(void);
    void *symbol(const char *);
    void close(void);
};

#endif /* __INC_LIBRARY_H__ */
