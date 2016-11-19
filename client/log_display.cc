/* log_display.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 19 Nov 2016, 10:44:34 tquirk
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

#include "configdata.h"

#include "ui/ui.h"
#include "ui/row_column.h"
#include "ui/multi_label.h"

#define DISTANCE_FROM_BOTTOM 10

static void context_resize_log_pos_callback(ui::active *, void *, void *);

static ui::font *log_font;
static ui::row_column *log_window;

void create_log_window(ui::context *ctx)
{
    glm::ivec2 grid(0, 1), spacing(15, 10), pos(10, 0);
    int packing = ui::order::column;

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
}

void add_log_entry(const std::string& str)
{
    int border = 1, orig_pos, orig_height, new_height;
    ui::multi_label *ml = new ui::multi_label(log_window, 150, 0);

    log_window->get_va(ui::element::position, ui::position::y, &orig_pos,
                       ui::element::size, ui::size::height, &orig_height, 0);
    ml->set_va(ui::element::font, ui::ownership::shared, log_font,
               ui::element::border, ui::side::all, &border,
               ui::element::string, 0, &str, 0);
    log_window->get(ui::element::size, ui::size::height, &new_height);
    orig_pos -= new_height - orig_height;
    log_window->set(ui::element::position, ui::position::y, &orig_pos);
}

void context_resize_log_pos_callback(ui::active *a, void *call, void *client)
{
    ui::resize_call_data *call_data = (ui::resize_call_data *)call;
    ui::row_column *lw = dynamic_cast<ui::row_column *>(a);

    if (lw != NULL && lw == log_window)
    {
        int log_height;

        lw->get(ui::element::size, ui::size::height, &log_height);
        log_height = call_data->new_size.y - log_height - DISTANCE_FROM_BOTTOM;
        lw->set(ui::element::position, ui::position::y, &log_height);
    }
}
