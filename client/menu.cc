/* menu.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game client
 * Copyright (C) 2026  Trinity Annabelle Quirk
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
 * This file contains the main menu for the R9 client program.
 *
 * Things to do
 *
 */

#include "menu.h"
#include "configdata.h"

#include <cuddly-gl/label.h>

#include <cuddly-gl/connect_glfw.h>

void create_login_dialog(ui::context *);

void login_callback(ui::active *, ui::btn_call_data&, void *);
void exit_callback(ui::active *, ui::btn_call_data&, void *);

static ui::font *menu_font = NULL;
static std::string menu_items[8] = {
    "Login", "Exit", "unused", "unused",
    "unused", "unused", "unused", "unused"
};

ui::pie_menu *setup_main_menu(ui::context *ctx)
{
    menu_font = new ui::font(config.font_name, 11, config.font_paths);

    ui::pie_menu *main_menu = new ui::pie_menu(
        ctx,
        ui::element::size, ui::size::width, 150,
        ui::element::size, ui::size::height, 100,
        ui::element::border, ui::side::inner, 0,
        ui::element::border, ui::side::outer, 0,
        ui::element::popup, ui::popup::button, ui::mouse::button0
    );

    for (int i = 0; i < 8; ++i)
    {
        ui::label *popup_label = new ui::label(
            main_menu,
            ui::element::font, ui::ownership::shared, menu_font,
            ui::element::string, ui::string::contents, menu_items[i]
        );
        if (i == 0)
            popup_label->add_callback(ui::callback::btn_up, login_callback, ctx);
        else if (i == 1)
            popup_label->add_callback(ui::callback::btn_up, exit_callback, ctx);
    }
    return main_menu;
}

void cleanup_main_menu(ui::pie_menu *mm)
{
    std::cout << "menu" << std::endl;
    if (mm != NULL)
    {
        std::cout << "closing menu" << std::endl;
        mm->close();
        std::cout << "deleting menu" << std::endl;
        delete mm;
        std::cout << "deleted" << std::endl;
    }
    if (menu_font != NULL)
    {
        std::cout << "deleting font" << std::endl;
        delete menu_font;
        std::cout << "deleted" << std::endl;
    }
}

void login_callback(ui::active *a, ui::btn_call_data& bcd, void *client)
{
    create_login_dialog((ui::context *)client);
}
