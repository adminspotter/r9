/* logbuf.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 Nov 2016, 11:05:21 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2016  Trinity Annabelle Quirk
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
 * This file contains a stream buffer to enable std::clog (or some
 * other stream object) to have some nice features we want:
 *   - Saving of elements
 *   - Saving to a file
 *   - Timestamps on individual log entries
 *   - Possible searching/sorting
 *
 * In the UI, we'll have one or more of these objects with a unified
 * log widget to display the entries, and age them out after a period
 * of time.
 */

#ifndef __INC_R9CLIENT_LOGBUF_H__
#define __INC_R9CLIENT_LOGBUF_H__

#include <iostream>

class logbuf : public std::basic_streambuf<char, std::char_traits<char> >
{
  public:
    explicit logbuf();
    explicit logbuf(std::string);
    ~logbuf();

    void close(void);

  protected:
    int sync(void);
    int overflow(int);
};

#endif /* __INC_R9CLIENT_LOGBUF_H__ */
