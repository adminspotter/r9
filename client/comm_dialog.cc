/* comm_dialog.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 09 Sep 2016, 07:45:33 tquirk
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

#include <stdio.h>
#include <string.h>

#include <iostream>

#include "client.h"

#include "ui/ui.h"
#include "ui/row_column.h"
#include "ui/text_field.h"
#include "ui/button.h"

static ui::font *dialog_font;
static ui::text_field *user, *pass, *host;

void create_login_dialog(ui::context *ctx)
{
    ui::row_column *dialog;
    glm::ivec2 grid(2, 0), spacing(10, 10);
    int packing = ui::order::row, border = 1, max_sz = 10;

    ui::button *b;
    std::string str;

    dialog_font = new ui::font(config.font_name, 20, config.font_paths);

    dialog = new ui::row_column(ctx, 0, 0);
    dialog->set_va(ui::element::border, ui::side::all, &border,
                   ui::element::order, 0, &packing,
                   ui::element::child_spacing, ui::size::all, &spacing,
                   ui::element::size, ui::size::grid, &grid, 0);

    b = new ui::button(dialog, 0, 0);
    str = _("OK");
    b->set_va(ui::element::font, ui::ownership::shared, dialog_font,
              ui::element::border, ui::side::all, &border,
              ui::element::string, 0, &str, 0);
    b->add_callback(setup_comm_callback, NULL);
    b->add_callback(close_dialog_callback, dialog);

    b = new ui::button(dialog, 0, 0);
    str = _("Cancel");
    b->set_va(ui::element::font, ui::ownership::shared, dialog_font,
              ui::element::border, ui::side::all, &border,
              ui::element::string, 0, &str, 0);
    b->add_callback(close_dialog_callback, dialog);
}

void setup_comm_callback(ui::event_target *t, void *call, void *client)
{
    struct addrinfo hints, *ai;
    std::string host_str, user_str, pass_str;

    if (host->get(ui::element::string, 0, &host_str) != 0
        || user->get(ui::element::string, 0, &user_str) != 0
        || pass->get(ui::element::string, 0, &pass_str) != 0)
        return;

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_NUMERICSERV;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    snprintf(portstr, sizeof(portstr), "%d", config.server_port);
    if ((ret = getaddrinfo(host_str.c_str(), portstr, &hints, &ai)) != 0)
    {
        std::cout << "Couldn't find host " << host_str
                  << ": " << gai_strerror(ret) << " (" << ret << ')'
                  << std::endl;
        return -1;
    }
    setup_comm(ai, user_str.c_str(), pass_str.c_str(), config.charname.c_str());
}

void close_dialog_callback(ui::event_target *t, void *call, void *client)
{
    ui::manager *dialog = (ui::manager *)client;

    delete dialog_font;
    dialog->close();
}
