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

const int keyboard::MOVE_UP = ui::key::e;
const int keyboard::MOVE_DOWN = ui::key::c;
const int keyboard::MOVE_LEFT = ui::key::a;
const int keyboard::MOVE_RIGHT = ui::key::d;
const int keyboard::MOVE_FORWARD = ui::key::w;
const int keyboard::MOVE_BACK = ui::key::s;
const int keyboard::YAW_LEFT = ui::key::l_arrow;
const int keyboard::YAW_RIGHT = ui::key::r_arrow;

void keyboard::keyboard_callback(ui::active *a, void *call, void *client)
{
    ui::key_call_data *call_data = (ui::key_call_data *)call;
    keyboard *kb = (keyboard *)client;

    if (call_data->key == kb->move_left
             || call_data->key == kb->move_right)
    {
        float val = (call_data->state == ui::key::up ? 0.0
                     : (call_data->key == kb->move_right ? 1.0 : -1.0));
        glm::vec3 move(val, 0.0, 0.0);
        kb->comm->send_action_request(3, move, 100);
    }
    else if (call_data->key == kb->move_forward
        || call_data->key == kb->move_back)
    {
        float val = (call_data->state == ui::key::up ? 0.0
                     : (call_data->key == kb->move_forward ? 1.0 : -1.0));
        glm::vec3 move(0.0, val, 0.0);
        kb->comm->send_action_request(3, move, 100);
    }
    else if (call_data->key == kb->move_up
             || call_data->key == kb->move_down)
    {
        float val = (call_data->state == ui::key::up ? 0.0
                     : (call_data->key == kb->move_up ? 1.0 : -1.0));
        glm::vec3 move(0.0, 0.0, val);
        kb->comm->send_action_request(3, move, 100);
    }
    else if (call_data->key == kb->yaw_left
             || call_data->key == kb->yaw_right)
    {
        glm::vec3 rot(0.0, 0.0, 0.0);

        if (call_data->state == ui::key::down)
        {
            float val = (call_data->key == kb->yaw_left ? 1.0 : -1.0);
            rot = glm::vec3(0.0, 0.0, val);
        }
        kb->comm->send_action_request(3, rot, 100);
    }
}

keyboard::keyboard()
{
    this->comm = NULL;
    this->set_defaults();
}

keyboard::~keyboard()
{
}

void keyboard::set_defaults(void)
{
    this->move_up = keyboard::MOVE_UP;
    this->move_down = keyboard::MOVE_DOWN;
    this->move_left = keyboard::MOVE_LEFT;
    this->move_right = keyboard::MOVE_RIGHT;
    this->move_forward = keyboard::MOVE_FORWARD;
    this->move_back = keyboard::MOVE_BACK;
    this->yaw_left = keyboard::YAW_LEFT;
    this->yaw_right = keyboard::YAW_RIGHT;
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
