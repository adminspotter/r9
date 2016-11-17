/* log_display.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 16 Nov 2016, 18:33:09 tquirk
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
 * Things to do
 *
 */

#include "ui/ui.h"
#include "ui/row_column.h"
#include "ui/multi_label.h"

static ui::row_column *log_window;

void create_log_window(ui::context *ctx)
{
}
