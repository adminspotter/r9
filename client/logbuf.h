/* logbuf.h                                                -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Nov 2016, 06:56:43 tquirk
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
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_LOGBUF_H__
#define __INC_R9CLIENT_LOGBUF_H__

#include <iostream>
#include <chrono>
#include <vector>
#include <string>

#include "ui/widget.h"

class logbuf : public std::basic_streambuf<char, std::char_traits<char> >
{
  public:
    typedef std::chrono::steady_clock lb_ts_time;
    typedef std::chrono::time_point<typename logbuf::lb_ts_time> lb_ts_point;
    typedef std::chrono::system_clock lb_wc_time;
    typedef std::chrono::time_point<typename logbuf::lb_wc_time> lb_wc_point;
    typedef struct log_entry_tag
    {
        lb_ts_point timestamp;
        lb_wc_point display_time;
        std::string entry;
    }
    lb_entry;

  private:
    typedef std::vector<typename logbuf::lb_entry> _lb_vector;

    typename logbuf::_lb_vector entries;
    std::string buf, fname;

    void sync_to_file(void);

  public:
    explicit logbuf();
    explicit logbuf(std::string);
    ~logbuf();

    void close(void);

  protected:
    int sync(void) override;
    int overflow(int) override;
};

#endif /* __INC_R9CLIENT_LOGBUF_H__ */
