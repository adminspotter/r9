/* log_display.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jan 2017, 08:33:51 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2017  Trinity Annabelle Quirk
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
 * This file contains the declaration for the log display object.  It
 * extends the row-column to add asynchronous addition and deletion of
 * children, and independent aging of its children.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_LOG_DISPLAY_H__
#define __INC_R9CLIENT_LOG_DISPLAY_H__

#include <pthread.h>

#include <iostream>
#include <chrono>
#include <list>
#include <string>

#include "ui/row_column.h"
#include "ui/font.h"

class log_display : public ui::row_column,
                    public std::basic_streambuf<char, std::char_traits<char> >
{
  protected:
    typedef std::chrono::steady_clock ld_ts_time;
    typedef std::chrono::time_point<ld_ts_time> ld_ts_point;
    typedef std::chrono::system_clock ld_wc_time;
    typedef std::chrono::time_point<ld_wc_time> ld_wc_point;
    typedef struct entry_tag
    {
        ld_ts_point timestamp;
        ld_wc_point display_time;
        std::string log_entry;
        ui::widget *label;

        const struct entry_tag& operator=(const struct entry_tag& et)
            {
                this->timestamp = et.timestamp;
                this->display_time = et.display_time;
                this->log_entry = et.log_entry;
                this->label = et.label;
                return *this;
            };
    }
    entry;

    std::list<entry> entries;
    std::list<entry>::iterator created;
    pthread_t cleanup_thread;
    pthread_mutex_t queue_mutex;
    std::chrono::seconds entry_lifetime;
    ui::font *log_font;
    std::streambuf *orig_rdbuf;
    std::string buf;

    static void *cleanup_entries(void *);

  public:
    log_display(ui::composite *, GLuint, GLuint);
    virtual ~log_display();

    virtual void draw(GLuint, const glm::mat4&) override;

  protected:
    int sync(void) override;
    int overflow(int) override;
};

#endif /* __INC_R9CLIENT_LOG_DISPLAY_H__ */
