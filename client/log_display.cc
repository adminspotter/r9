/* log_display.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 26 Nov 2017, 08:03:45 tquirk
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

#include "cuddly-gl/ui_defs.h"
#include "log_display.h"
#include "cuddly-gl/multi_label.h"

#define DISTANCE_FROM_BOTTOM 10
#define LABEL_WIDTH 225
#define ENTRY_LIFETIME 10

static void resize_pos_callback(ui::active *, void *, void *);
static log_display::ld_iter operator+(log_display::ld_iter, int);

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
    if (this->entries.size() == 0
        || (this->created != this->entries.end()
            && this->created == --this->entries.end()))
        return;

    while (this->created != --this->entries.end())
    {
        if (this->created == this->entries.end())
            this->created = this->entries.begin();
        else
            ++this->created;

        int border = 1, orig_pos = this->pos.y, orig_height = this->dim.y;
        this->created->label = new ui::multi_label(this, LABEL_WIDTH, 0);
        this->created->label->set_va(
            ui::element::font, ui::ownership::shared, this->log_font,
            ui::element::string, 0, &this->created->log_entry,
            ui::element::border, ui::side::all, &border, 0);
        this->manage_children();
        orig_pos -= this->dim.y - orig_height;
        this->set_position(ui::position::y, &orig_pos);
    }
}

void *log_display::cleanup_entries(void *arg)
{
    log_display *ld = (log_display *)arg;
    log_display::ld_iter last_closed, next_closed;
    log_display::ld_ts_point now;
    std::chrono::duration<float> ftime;
    float ftime_val;
    struct timespec ts;

    last_closed = next_closed = ld->entries.end();

    for (;;)
    {
        if (ld->entries.empty()
            || (last_closed != ld->entries.end()
                && (next_closed = last_closed + 1) == ld->entries.end()))
            sleep(ENTRY_LIFETIME);
        else
        {
            if (last_closed == ld->entries.end())
                next_closed = ld->entries.begin();

            if (next_closed != ld->entries.end())
            {
                /* Do a nanosleep until the next entry expires. */
                ftime = next_closed->timestamp
                    + ld->entry_lifetime - log_display::ld_ts_time::now();
                ftime_val = ftime.count();
                if (ftime_val > 0.0f)
                {
                    ts.tv_sec = (int)truncf(ftime_val);
                    ftime_val -= (float)ts.tv_sec;
                    ts.tv_nsec = (int)truncf(ftime_val * 1000000000);
                    nanosleep(&ts, NULL);
                }
            }
        }

        /* See if anything needs closing */
        if (next_closed == ld->entries.end())
            continue;

        now = log_display::ld_ts_time::now();
        while (next_closed != ld->entries.end()
               && now >= (next_closed->timestamp + ld->entry_lifetime))
        {
            pthread_mutex_lock(&ld->queue_mutex);
            next_closed->label->close();
            next_closed->label = NULL;
            pthread_mutex_unlock(&ld->queue_mutex);
            last_closed = next_closed++;
        }
    }
}

log_display::log_display(ui::composite *p, GLuint w, GLuint h)
    : ui::row_column(p, w, h), ui::active::active(w, h), ui::rect(w, h),
      buf(), fname(), entries(), entry_lifetime(ENTRY_LIFETIME)
{
    int border = 1, ret;

    this->created = this->entries.end();
    this->grid_sz = glm::ivec2(1, 0);
    this->child_spacing = glm::ivec2(5, 5);
    this->pos = glm::ivec2(10, 0);

    this->pack_order = ui::order::column;

    this->log_font = new ui::font(config.font_name, 13, config.font_paths);

    this->composite::parent->get(ui::element::size,
                                 ui::size::height,
                                 &this->pos.y);
    this->pos.y -= DISTANCE_FROM_BOTTOM;

    this->background = glm::vec4(0.0, 0.0, 0.0, 0.0);

    this->add_callback(ui::callback::resize, resize_pos_callback, NULL);
    this->queue_mutex = PTHREAD_MUTEX_INITIALIZER;

    if ((ret = pthread_create(&(this->cleanup_thread), NULL,
                              log_display::cleanup_entries, (void *)this)) != 0)
    {
        std::ostringstream s;
        s << "Couldn't start log display cleanup thread: "
          << strerror(ret) << " (" << ret << ")";
        throw std::runtime_error(s.str());
    }

    this->orig_rdbuf = std::clog.rdbuf(this);
}

log_display::~log_display()
{
    int ret;

    /* Set std::clog's rdbuf back to a normal state, in case we need
     * to emit some errors to it.
     */
    std::clog.rdbuf(this->orig_rdbuf);

    if ((ret = pthread_cancel(this->cleanup_thread)) != 0)
        std::clog << "Couldn't cancel log display cleanup thread: "
                  << strerror(ret) << " (" << ret << ")" << std::endl;
    sleep(0);
    if ((ret = pthread_join(this->cleanup_thread, NULL)) != 0)
        std::clog << "Couldn't reap log display cleanup thread: "
                  << strerror(ret) << " (" << ret << ")" << std::endl;
    if ((ret = pthread_mutex_destroy(&this->queue_mutex)) != 0)
        std::clog << "Couldn't destroy log display mutex: "
                  << strerror(ret) << " (" << ret << ")" << std::endl;

    this->sync_to_file();
}

void log_display::draw(GLuint trans_uniform, const glm::mat4& parent_trans)
{
    /* We must lock the mutex here so that the deletion thread won't
     * try to delete something while we're trying to draw it.  This is
     * the reason we did the subclassing in the first place.
     */
    pthread_mutex_lock(&this->queue_mutex);
    this->create_log_labels();
    this->row_column::draw(trans_uniform, parent_trans);
    pthread_mutex_unlock(&this->queue_mutex);
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
        pthread_mutex_lock(&this->queue_mutex);
        this->entries.push_back(std::move(e));
        pthread_mutex_unlock(&this->queue_mutex);
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

/* ARGSUSED */
void resize_pos_callback(ui::active *a, void *call, void *client)
{
    ui::resize_call_data *call_data = (ui::resize_call_data *)call;
    log_display *ld = dynamic_cast<log_display *>(a);

    if (ld != NULL)
    {
        int log_height;

        ld->get(ui::element::size, ui::size::height, &log_height);
        log_height = call_data->new_size.y - log_height - DISTANCE_FROM_BOTTOM;
        ld->set(ui::element::position, ui::position::y, &log_height);
    }
}

/* We want to be able to say "hey, what's the next element in the
 * list?", but there's no general-purpose "add X to our iterator"
 * function.
 */
static log_display::ld_iter operator+(log_display::ld_iter iter, int interval)
{
    log_display::ld_iter tmp_iter = iter;

    for (int i = 0; i < interval; ++i)
        ++tmp_iter;

    return tmp_iter;
}
