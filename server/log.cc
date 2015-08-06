/* log.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 15:12:52 tquirk
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
 * Things to do
 *
 */

#include <cstring>

#include "log.h"

Log::Log(std::string ident, int fac)
{
    this->facility = fac;
    this->priority = LOG_DEBUG;
    strncpy(this->ident, ident.c_str(), sizeof(this->ident));
    this->ident[sizeof(this->ident) - 1] = '\0';

    openlog(this->ident, LOG_PID, this->facility);
}

Log::~Log()
{
    this->close();
}

void Log::close(void)
{
    closelog();
}

int Log::sync(void)
{
    if (this->buf.length())
    {
        syslog(this->priority, "%s", this->buf.c_str());
        this->buf.erase();
        this->priority = LOG_DEBUG;
    }
    return 0;
}

int Log::overflow(int c)
{
    if (c != EOF)
        this->buf += static_cast<char>(c);
    else
        this->sync();
    return c;
}

std::ostream& operator<<(std::ostream& os, const LogPriority& prio)
{
    static_cast<Log *>(os.rdbuf())->priority = (int)prio;
    return os;
}
