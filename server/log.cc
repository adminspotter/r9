/* log.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 21 Jun 2014, 16:09:17 tquirk
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
 * This file contains the implementation of a streambuf to allow use of
 * std::clog to log to syslog, instead of the regular stdout.
 *
 * Changes
 *   21 Jun 2014 TAQ - Created the file.
 *
 * Things to do
 *
 */

#include "log.h"

Log::Log(std::string ident, int fac)
{
    this->facility = fac;
    this->priority = LOG_DEBUG;
    strncpy(this->name, ident.c_str(), sizeof(this->name));
    this->ident[sizeof(this->name) - 1] = '\0';

    openlog(this->name, LOG_PID, this->facility);
}

Log::~Log()
{
    closelog();
}

int Log::sync(void)
{
    if (this->buf.length())
    {
        syslog(this->priority, this->buf.c_str());
        this->buf.erase();
        this->priority = LOG_DEBUG;
    }
    return 0;
}

int Log::overflow(int c)
{
    if (c != EOF)
        this->buffer += static_cast<char>(c);
    else
        this->sync();
    return c;
}

std::ostream& operator<<(std::ostream& os, const LogPriority& prio)
{
    static_cast<Log *>(os.rdbuf())->priority = (int)LogPriority;
    return os;
}
