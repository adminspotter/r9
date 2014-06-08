/* basesock.h                                              -*- C++ -*-
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 13 Sep 2007, 09:16:39 trinity
 *
 * Revision IX game server
 * Copyright (C) 2007  Trinity Annabelle Quirk
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
 * This file contains the base class so we can dynamic cast the
 * different types of sockets.
 *
 * Changes
 *   12 Sep 2007 TAQ - Created the file.
 *   13 Sep 2007 TAQ - Added create_socket static method.  Moved blank
 *                     constructor and destructor into basesock.cc.
 *
 * Things to do
 *
 * $Id: basesock.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_BASESOCK_H__
#define __INC_BASESOCK_H__

class basesock
{
  public:
    basesock();
    virtual ~basesock();

    static int create_socket(int, int);
};

#endif /* __INC_BASESOCK_H__ */
