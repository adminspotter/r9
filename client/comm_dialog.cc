/* comm_dialog.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Apr 2021, 08:08:49 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2021  Trinity Annabelle Quirk
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
#include "configdata.h"
#include "l10n.h"

#include <cuddly-gl/ui.h>
#include <cuddly-gl/row_column.h>
#include <cuddly-gl/label.h>
#include <cuddly-gl/text_field.h>
#include <cuddly-gl/password.h>
#include <cuddly-gl/button.h>

static ui::font *dialog_font;
static ui::text_field *user, *pass, *host;

void setup_comm_callback(ui::active *, void *, void *);
void close_dialog_callback(ui::active *, void *, void *);

void create_login_dialog(ui::context *ctx)
{
    ui::row_column *dialog;
    glm::ivec2 size, dlg_size;
    int border = 1, max_sz = 10;

    ui::button *b;
    ui::label *l;
    std::string str;

    dialog_font = new ui::font(config.font_name, 10, config.font_paths);

    dialog = new ui::row_column(ctx,
                                ui::element::border, ui::side::all, border,
                                ui::element::order, 0, ui::order::row,
                                ui::element::child_spacing, ui::size::width, 10,
                                ui::element::child_spacing,
                                ui::size::height,
                                10,
                                ui::element::size, ui::size::columns, 2);

    str = translate("Username");
    l = new ui::label(dialog,
                      ui::element::font, ui::ownership::shared, dialog_font,
                      ui::element::string, 0, str);
    str = config.username;
    user = new ui::text_field(dialog,
                              ui::element::font,
                              ui::ownership::shared,
                              dialog_font,
                              ui::element::size, ui::size::max_width, max_sz,
                              ui::element::border, ui::side::all, border,
                              ui::element::string, 0, str);

    str = translate("Password");
    l = new ui::label(dialog,
                      ui::element::font, ui::ownership::shared, dialog_font,
                      ui::element::string, 0, str);
    pass = new ui::password(dialog,
                            ui::element::font,
                            ui::ownership::shared,
                            dialog_font,
                            ui::element::size, ui::size::max_width, max_sz,
                            ui::element::border, ui::side::all, border);

    str = translate("Server");
    l = new ui::label(dialog,
                      ui::element::font, ui::ownership::shared, dialog_font,
                      ui::element::string, 0, str);
    str = config.server_addr;
    host = new ui::text_field(dialog,
                              ui::element::font,
                              ui::ownership::shared,
                              dialog_font,
                              ui::element::size, ui::size::max_width, max_sz,
                              ui::element::border, ui::side::all, border,
                              ui::element::string, 0, str);

    str = translate("OK");
    b = new ui::button(dialog,
                       ui::element::font, ui::ownership::shared, dialog_font,
                       ui::element::border, ui::side::all, border,
                       ui::element::string, 0, str);
    b->add_callback(ui::callback::btn_up, setup_comm_callback, NULL);
    b->add_callback(ui::callback::btn_up, close_dialog_callback, dialog);

    str = translate("Cancel");
    b = new ui::button(dialog,
                       ui::element::font, ui::ownership::shared, dialog_font,
                       ui::element::border, ui::side::all, border,
                       ui::element::string, 0, str);
    b->add_callback(ui::callback::btn_up, close_dialog_callback, dialog);

    dialog->manage_children();

    ctx->get(ui::element::size, ui::size::all, &size);
    dialog->get(ui::element::size, ui::size::all, &dlg_size);
    size /= 2;
    size -= dlg_size / 2;
    dialog->set(ui::element::position, ui::position::all, size);
}

void setup_comm_callback(ui::active *t, void *call, void *client)
{
    char portstr[16];
    struct addrinfo hints, *ai;
    std::string host_str, user_str, pass_str;
    int ret;

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
        std::cout << format(translate("Couldn't find host {1}: {2} ({3})"))
            % host_str % gai_strerror(ret) % ret
                  << std::endl;
        return;
    }
    config.read_crypto_key(pass_str);
    setup_comm(ai, user_str.c_str(), config.charname.c_str());
}

void close_dialog_callback(ui::active *t, void *call, void *client)
{
    ui::row_column *dialog = (ui::row_column *)client;

    dialog->close();
    delete dialog_font;
}
