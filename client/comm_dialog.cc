/* comm_dialog.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 08 Sep 2016, 23:18:14 tquirk
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
 * This file contains the dialog for logging into servers.  Username,
 * password, and hostname/IP fields, plus OK and Cancel buttons.
 *
 * Things to do
 *
 */

#include "ui/text_field.h"

static ui::font *dialog_font;
static ui::text_field *user, *pass, *host;

void create_login_dialog(ui::context *ctx)
{
}

void setup_comm_callback(ui::event_target *t, void *call, void *client)
{
}

void close_dialog_callback(ui::event_target *t, void *call, void *client)
{
}
