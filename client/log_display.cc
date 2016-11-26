/* log_display.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 25 Nov 2016, 18:13:15 tquirk
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
 * This file contains the UI widgets to display log entries.  We'll
 * have a mostly-stationary row-column, with individual multiline
 * labels for each entry.  We'll age each entry out after a
 * configurable time.
 *
 * We want the widget to remain at the bottom left corner of the
 * window, and grow and shrink upwards as needed.  We have the resize
 * callback list now, that we can use for this purpose.
 *
 * Things to do
 *
 */

#include <unistd.h>
#include <pthread.h>

#include <sstream>
#include <stdexcept>

#include "configdata.h"
#include "logbuf.h"
#include "l10n.h"

#include "ui/ui.h"
#include "ui/row_column.h"
#include "ui/multi_label.h"

#define DISTANCE_FROM_BOTTOM 10
#define ENTRY_LIFETIME 10 * 1000000000

typedef struct entry_tag
{
    logbuf::lb_entry *log_entry;
    ui::widget *label;

    const struct entry_tag& operator=(const struct entry_tag& et)
        {
            this->log_entry = et.log_entry;
            this->label = et.label;
            return *this;
        }
}
entry;

static void *clean_old_log_entries(void *);
static void context_resize_log_pos_callback(ui::active *, void *, void *);

static pthread_t log_cleanup_thread;
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t log_not_empty = PTHREAD_COND_INITIALIZER;
static bool exit_flag = false;
static std::list<entry> entries;
static ui::font *log_font;
static ui::row_column *log_window;
static const std::chrono::nanoseconds lifetime(ENTRY_LIFETIME);

void create_log_window(ui::context *ctx)
{
    glm::ivec2 grid(0, 1), spacing(15, 10), pos(10, 0);
    int packing = ui::order::column, ret;

    log_font = new ui::font(config.font_name, 13, config.font_paths);

    ctx->get(ui::element::size, ui::size::height, &pos.y);
    pos.y -= DISTANCE_FROM_BOTTOM;
    log_window = new ui::row_column(ctx, 10, 0);
    log_window->set_va(ui::element::order, 0, &packing,
                       ui::element::child_spacing, ui::size::all, &spacing,
                       ui::element::size, ui::size::grid, &grid,
                       ui::element::position, ui::position::all, &pos, 0);
    log_window->add_callback(ui::callback::resize,
                             context_resize_log_pos_callback, NULL);

    if ((ret = pthread_create(&log_cleanup_thread, NULL,
                              clean_old_log_entries, NULL)) != 0)
    {
        std::ostringstream s;
        s << _("Could not create log cleanup thread.") << std::endl;
        throw std::runtime_error(s.str());
    }
    std::clog.rdbuf(new logbuf());
}

void cleanup_log_window(void)
{
    int ret;

    exit_flag = true;
    pthread_cond_broadcast(&log_not_empty);
    if ((ret = pthread_join(log_cleanup_thread, NULL)) != 0)
    {
        std::ostringstream s;
        s << _("Could not join log cleanup thread.") << std::endl;
        throw std::runtime_error(s.str());
    }
    log_window->close();
}

void add_log_entry(logbuf::lb_entry *lbe)
{
    entry ent;
    int border = 1, orig_pos, orig_height, new_height;
    ui::multi_label *ml = new ui::multi_label(log_window, 150, 0);

    ent.log_entry = lbe;
    ent.label = ml;

    log_window->get_va(ui::element::position, ui::position::y, &orig_pos,
                       ui::element::size, ui::size::height, &orig_height, 0);
    ml->set_va(ui::element::font, ui::ownership::shared, log_font,
               ui::element::border, ui::side::all, &border,
               ui::element::string, 0, &lbe->entry, 0);
    log_window->get(ui::element::size, ui::size::height, &new_height);
    orig_pos -= new_height - orig_height;
    log_window->set(ui::element::position, ui::position::y, &orig_pos);

    pthread_mutex_lock(&log_lock);
    entries.push_back(std::move(ent));
    if (entries.size() != 0)
        pthread_cond_signal(&log_not_empty);
    pthread_mutex_unlock(&log_lock);
}

/* ARGSUSED */
void *clean_old_log_entries(void *arg)
{
    struct timespec ts;
    std::chrono::nanoseconds d;

    sleep(10);
    for (;;)
    {
        pthread_mutex_lock(&log_lock);
        while (entries.empty() && !exit_flag)
            pthread_cond_wait(&log_not_empty, &log_lock);

        if (exit_flag)
        {
            pthread_mutex_unlock(&log_lock);
            pthread_exit(NULL);
        }

        d = lifetime + entries.front().log_entry->timestamp
            - logbuf::lb_ts_time::now();
        ts.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(d).count();
        ts.tv_nsec = d.count() - (ts.tv_sec * 1000000000);
        std::cout << "sleeping " << ts.tv_sec << " seconds, " << ts.tv_nsec << " nsec" << std::endl;
        nanosleep(&ts, NULL);
        entries.front().label->close();
        entries.pop_front();
        pthread_mutex_unlock(&log_lock);
    }
    return NULL;
}

void context_resize_log_pos_callback(ui::active *a, void *call, void *client)
{
    ui::resize_call_data *call_data = (ui::resize_call_data *)call;
    ui::row_column *lw = dynamic_cast<ui::row_column *>(a);

    if (lw == log_window)
    {
        int log_height;

        lw->get(ui::element::size, ui::size::height, &log_height);
        log_height = call_data->new_size.y - log_height - DISTANCE_FROM_BOTTOM;
        lw->set(ui::element::position, ui::position::y, &log_height);
    }
}
