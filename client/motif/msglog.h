/* msglog.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Oct 2015, 11:00:47 tquirk
 *
 * Revision IX game client
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
 * This file contains a stream buffer to enable std::clog to work with
 * the client message pane, instead of a regular file.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_MSGLOG_H__
#define __INC_R9CLIENT_MSGLOG_H__

#include <streambuf>

class MessageLog : public std::basic_streambuf<char, std::char_traits<char> >
{
  private:
    std::string buf;

  protected:
    int sync(void);
    int overflow(int);
};

#endif /* __INC_R9CLIENT_MSGLOG_H__ */
