/* log.h                                                   -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 23 Jun 2014, 18:18:37 tquirk
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
 * This file contains a stream buffer to enable std::clog to work with
 * syslog, instead of a regular file.
 *
 * See also:  http://stackoverflow.com/questions/2638654/redirect-c-stdclog-to-syslog-on-unix
 *
 * Changes
 *   21 Jun 2014 TAQ - Created the file.
 *   23 Jun 2014 TAQ - Small tweaks to get things compiling correctly.
 *
 * Things to do
 *
 */

#ifndef __INC_LOG_H__
#define __INC_LOG_H__

#include <syslog.h>

#include <iostream>

enum LogPriority
{
    syslogEmerg  = LOG_EMERG,
    syslogAlert  = LOG_ALERT,
    syslogCrit   = LOG_CRIT,
    syslogErr    = LOG_ERR,
    syslogWarn   = LOG_WARNING,
    syslogNotice = LOG_NOTICE,
    syslogInfo   = LOG_INFO,
    syslogDebug  = LOG_DEBUG
};

std::ostream& operator<<(std::ostream&, const LogPriority&);

class Log : public std::basic_streambuf<char, std::char_traits<char> >
{
  private:
    std::string buf;
    int facility, priority;
    char ident[50];

  public:
    explicit Log(std::string, int);
    ~Log();

  protected:
    int sync();
    int overflow(int);

  private:
    friend std::ostream& operator<<(std::ostream&, const LogPriority&);
};

#endif /* __INC_LOG_H__ */
