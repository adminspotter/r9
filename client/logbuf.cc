/* logbuf.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Nov 2016, 06:56:18 tquirk
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
 * This file contains the implementation of a streambuf to allow use
 * of an iostream to keep timestamps on each element, save them, and
 * write them to a file.
 *
 * Things to do
 *
 */

#include <config.h>

#include "logbuf.h"

#include <fstream>
#include <ctime>
#include <iomanip>
#include <utility>

#include "client.h"

void logbuf::sync_to_file(void)
{
    if (this->fname.length())
    {
        std::ofstream fs(this->fname, std::ios::app | std::ios::ate);
        std::time_t tt;
#if !HAVE_STD_PUT_TIME
        char time_str[32];
#endif /* !HAVE_STD_PUT_TIME */

        if (fs.is_open())
        {
            for (auto i = this->entries.begin(); i != this->entries.end(); ++i)
            {
                tt = logbuf::lb_wc_time::to_time_t((*i).display_time);

#if HAVE_STD_PUT_TIME
                fs << std::put_time(std::localtime(&tt), "%Y-%m-%d %H:%M:%S ")
                   << (*i).entry << std::endl;
#else
                std::strftime(time_str, sizeof(time_str),
                              "%Y-%m-%d %H:%M:%S ", std::localtime(&tt));
                fs << time_str << (*i).entry << std::endl;
#endif /* HAVE_STD_PUT_TIME */
            }
            fs.close();
        }
    }
}

logbuf::logbuf()
    : buf(), fname(), entries()
{
}

logbuf::logbuf(std::string f)
    : buf(), fname(f), entries()
{
}

logbuf::~logbuf()
{
    this->close();
}

void logbuf::close(void)
{
    this->sync_to_file();
    this->entries.clear();
}

int logbuf::sync(void)
{
    if (this->buf.length())
    {
        lb_entry e;

        e.timestamp = logbuf::lb_ts_time::now();
        e.display_time = logbuf::lb_wc_time::now();
        e.entry = buf;

        /* Strip trailing whitespace */
        e.entry.erase(e.entry.find_last_not_of(" \n\r\t") + 1);

        e.ui_element = add_log_entry(e.entry);

        this->entries.push_back(std::move(e));
        this->buf.erase();
    }
    return 0;
}

int logbuf::overflow(int c)
{
    if (c != EOF)
        this->buf += static_cast<char>(c);
    else
        this->sync();
    return c;
}
