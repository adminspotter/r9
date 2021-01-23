/* keyboard.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 16 Jan 2021, 13:34:13 tquirk
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
 * This file defines the keyboard interface to control the client.
 *
 * Things to do
 *
 */

#include "keyboard.h"

#include "../comm.h"
#include "../client_core.h"

void keyboard::keyboard_callback(ui::active *a, void *call, void *client)
{
    ui::key_call_data *call_data = (ui::key_call_data *)call;
    keyboard *kb = (keyboard *)client;

    if (call_data->key == ui::key::u_arrow
        || call_data->key == ui::key::d_arrow)
    {
        float val = (call_data->state == ui::key::up ? 0.0
                     : (call_data->key == ui::key::u_arrow ? 1.0 : -1.0));
        glm::vec3 move(0.0, val, 0.0);
        kb->comm->send_action_request(3, move, 100);
    }
    else if (call_data->key == ui::key::l_arrow
             || call_data->key == ui::key::r_arrow)
    {
        glm::vec3 rot(0.0, 0.0, 0.0);

        if (call_data->state == ui::key::down)
        {
            float val = (call_data->key == ui::key::l_arrow ? 1.0 : -1.0);
            rot = glm::vec3(0.0, 0.0, val);
        }
        kb->comm->send_action_request(3, rot, 100);
    }
}

keyboard::keyboard()
{
    this->comm = NULL;
}

keyboard::~keyboard()
{
}

void keyboard::setup(ui::active *comp, Comm *comm)
{
    this->comm = comm;
    comp->add_callback(ui::callback::key_down,
                       keyboard::keyboard_callback,
                       this);
    comp->add_callback(ui::callback::key_up,
                       keyboard::keyboard_callback,
                       this);
}

void keyboard::cleanup(ui::active *comp, Comm *comm)
{
    this->comm = NULL;
    comp->remove_callback(ui::callback::key_down,
                          keyboard::keyboard_callback,
                          this);
    comp->remove_callback(ui::callback::key_up,
                          keyboard::keyboard_callback,
                          this);
}

extern "C" control *create(void)
{
    return (control *)new keyboard();
}
