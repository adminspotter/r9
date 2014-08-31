/* msglog.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 30 Aug 2014, 16:34:31 tquirk
 *
 * Revision IX game client
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
 * This file contains the implementation to send things to the message
 * pane via a streambuf object within the std::clog stream.
 *
 * Changes
 *   24 Aug 2014 TAQ - Created the file.
 *
 * Things to do
 *
 */

#include "msglog.h"

void main_post_message(const std::string&);

int MessageLog::sync(void)
{
    if (this->buf.length())
    {
        main_post_message(this->buf);
        this->buf.erase();
    }
    return 0;
}

int MessageLog::overflow(int c)
{
    if (c != EOF)
        this->buf += static_cast<char>(c);
    else
        this->sync();
    return c;
}
