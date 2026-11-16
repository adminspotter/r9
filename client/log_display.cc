/* log_display.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game client
 * Copyright (C) 2016-2026  Trinity Annabelle Quirk
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
 * This file contains the UI widgets to display log entries.  We'll
 * have a mostly-stationary row-column, with individual multiline
 * labels for each entry.  We'll age each entry out after a
 * configurable time, with individual widget timeouts.
 *
 * We want the widget to remain at the bottom left corner of the
 * window, and grow and shrink upwards as needed.  We have the resize
 * callback list now, that we can use for this purpose.
 *
 * Things to do
 *   - Make the lifetime, font size, and log filename config items.
 *
 */

#include <math.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

#include "configdata.h"

#include <cuddly-gl/ui_defs.h>
#include "log_display.h"
#include <cuddly-gl/label.h>

#include "l10n.h"

const int log_display::ENTRY_LIFETIME = 10;

void log_display::sync_to_file(void)
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
                tt = log_display::ld_wc_time::to_time_t((*i).display_time);

#if HAVE_STD_PUT_TIME
                fs << std::put_time(std::localtime(&tt), "%FT%T%z ")
                   << (*i).log_entry << std::endl;
#else
                std::strftime(time_str, sizeof(time_str),
                              "%FT%T%z ", std::localtime(&tt));
                fs << time_str << (*i).log_entry << std::endl;
#endif /* HAVE_STD_PUT_TIME */
            }
            fs.close();
        }
    }
}

void log_display::create_log_labels(void)
{
    while (this->created != this->entries.end() - 1)
    {
        ui::resize_call_data rcd;

        ++this->created;
        this->created->label = new ui::label(
            this,
            ui::element::font, ui::ownership::shared, this->log_font,
            ui::element::string, ui::string::contents, this->created->log_entry,
            ui::element::border, ui::side::all, 1,
            ui::element::color,
            ui::color::background,
            glm::vec4(0.0, 0.0, 0.0, 1.0)
        );
        GLuint x, y;
        this->created->label->get(ui::element::size, ui::size::width, &x,
                                  ui::element::size, ui::size::height, &y);
        this->manage_children();
        this->reposition(dynamic_cast<ui::active *>(this), rcd, NULL);
    }
}

void log_display::cleanup_entries(log_display *ld)
{
    log_display::ld_iter last_closed, next_closed;
    log_display::ld_ts_point now;
    std::chrono::seconds sleep_timeout(log_display::ENTRY_LIFETIME);
    std::chrono::duration<float> ftime;

    last_closed = ld->entries.begin();

    for (;;)
    {
        if (last_closed == ld->entries.end())
            next_closed = ld->entries.begin();
        else
            next_closed = last_closed + 1;

        {
            std::unique_lock lock(ld->exit_mutex);
            if (ld->exit_flag)
                break;
            if (next_closed == ld->entries.end())
            {
                if (ld->exit_cond.wait_for(lock, sleep_timeout)
                    != std::cv_status::timeout)
                    continue;
            }
            else
            {
                if (ld->exit_cond.wait_until(lock, next_closed->timestamp)
                    != std::cv_status::timeout)
                    continue;
            }
        }

        now = log_display::ld_ts_time::now();
        while (next_closed != ld->entries.end()
               && now >= (next_closed->timestamp + ld->entry_lifetime))
        {
            if (next_closed->label != NULL)
            {
                std::unique_lock lock(ld->queue_mutex);
                next_closed->label->close();
                next_closed->label = NULL;
            }
            last_closed = next_closed++;
        }
    }
}

void log_display::init(void)
{
    int border = 1, ret;
    entry e;

    e.timestamp = log_display::ld_ts_time::now();
    e.display_time = log_display::ld_wc_time::now();
    e.log_entry = "program start";
    e.label = NULL;

    this->entries.push_back(e);
    this->created = this->entries.begin();
    this->grid_sz = glm::ivec2(1, 0);
    this->child_spacing = glm::ivec2(5, 5);
    this->pack_order = ui::order::column;

    this->log_font = new ui::font(config.font_name, 13, config.font_paths);

    this->background = glm::vec4(0.0, 0.0, 0.0, 0.0);

    this->exit_flag = false;
    this->cleanup_thread = std::thread(log_display::cleanup_entries, this);

    this->orig_rdbuf = std::clog.rdbuf(this);
}

log_display::log_display(ui::composite *c)
    : ui::rect(0, 0), ui::active(0, 0), ui::row_column(c),
      buf(), fname(), entries(), entry_lifetime(log_display::ENTRY_LIFETIME),
      queue_mutex(), exit_cond()
{
    this->init();
}

log_display::~log_display()
{
    int ret;

    /* Set std::clog's rdbuf back to a normal state, in case we need
     * to emit some errors to it.
     */
    std::clog.rdbuf(this->orig_rdbuf);

    {
        std::unique_lock lock(this->exit_mutex);
        this->exit_flag = true;
        this->exit_cond.notify_all();
    }
    sleep(0);
    this->cleanup_thread.join();

    this->sync_to_file();
}

void log_display::draw(GLuint trans_uniform, const glm::mat4& parent_trans)
{
    /* We must lock the mutex here so that the deletion thread won't
     * try to delete something while we're trying to draw it.  This is
     * the reason we did the subclassing in the first place.
     */
    std::unique_lock lock(this->queue_mutex);
    this->create_log_labels();
    this->row_column::draw(trans_uniform, parent_trans);
}

int log_display::sync(void)
{
    if (this->buf.length())
    {
        entry e;

        e.timestamp = log_display::ld_ts_time::now();
        e.display_time = log_display::ld_wc_time::now();
        e.log_entry = buf;
        e.label = NULL;

        /* Strip trailing whitespace */
        e.log_entry.erase(e.log_entry.find_last_not_of(" \n\r\t") + 1);
        if (e.log_entry.length() == 0)
            return 0;
        {
            std::unique_lock lock(this->queue_mutex);
            this->entries.push_back(std::move(e));
        }
        this->buf.erase();
    }
    return 0;
}

int log_display::overflow(int c)
{
    if (c != EOF)
        this->buf += static_cast<char>(c);
    else
        this->sync();
    return c;
}
