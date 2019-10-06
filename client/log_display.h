/* log_display.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 06 Oct 2019, 08:24:13 tquirk
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
#include <deque>
#include <string>

#include "cuddly-gl/label.h"
#include "cuddly-gl/row_column.h"
#include "cuddly-gl/font.h"

class log_display : public ui::row_column,
                    public std::basic_streambuf<char, std::char_traits<char> >
{
  public:
    typedef std::chrono::steady_clock ld_ts_time;
    typedef std::chrono::time_point<ld_ts_time> ld_ts_point;
    typedef std::chrono::system_clock ld_wc_time;
    typedef std::chrono::time_point<ld_wc_time> ld_wc_point;
    typedef struct entry_tag
    {
        ld_ts_point timestamp;
        ld_wc_point display_time;
        std::string log_entry;
        ui::label *label;

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
    typedef std::deque<entry>::iterator ld_iter;

    static const int ENTRY_LIFETIME;

  protected:
    std::deque<entry> entries;
    std::deque<entry>::iterator created;
    pthread_t cleanup_thread;
    pthread_mutex_t queue_mutex;
    std::chrono::seconds entry_lifetime;
    ui::font *log_font;
    std::streambuf *orig_rdbuf;
    std::string buf, fname;

    void sync_to_file(void);

    void create_log_labels(void);
    static void *cleanup_entries(void *);

    void init(void);

  public:
    explicit log_display(ui::composite *);
    template<typename... Args>
    log_display(ui::composite *c, Args... args)
        : ui::rect(0, 0), ui::active(0, 0), ui::row_column(c),
          buf(), fname(), entries(), entry_lifetime(ENTRY_LIFETIME)
        {
            this->init();
            this->set(args...);
        };
    virtual ~log_display();

    virtual void draw(GLuint, const glm::mat4&) override;

  protected:
    int sync(void) override;
    int overflow(int) override;
};

#endif /* __INC_R9CLIENT_LOG_DISPLAY_H__ */
