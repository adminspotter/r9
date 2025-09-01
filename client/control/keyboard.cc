/* keyboard.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game client
 * Copyright (C) 2019-2021  Trinity Annabelle Quirk
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
const int keyboard::PITCH_UP = ui::key::i;
const int keyboard::PITCH_DOWN = ui::key::k;
const int keyboard::ROLL_LEFT = ui::key::j;
const int keyboard::ROLL_RIGHT = ui::key::l;
const int keyboard::YAW_LEFT = ui::key::u;
const int keyboard::YAW_RIGHT = ui::key::o;

void keyboard::keyboard_callback(ui::active *a, void *call, void *client)
{
    ui::key_call_data *call_data = (ui::key_call_data *)call;
    keyboard *kb = (keyboard *)client;
    glm::vec3 move(0.0, 0.0, 0.0);
    bool key_down = call_data->state == ui::key::down;

    if (call_data->key == kb->move_left
             || call_data->key == kb->move_right)
    {
        move.x = (!key_down ? 0.0
                  : (call_data->key == kb->move_right ? 1.0 : -1.0));
        kb->comm->send_action_request(3, move, 100);
    }
    else if (call_data->key == kb->move_forward
        || call_data->key == kb->move_back)
    {
        move.y = (!key_down ? 0.0
                  : (call_data->key == kb->move_forward ? 1.0 : -1.0));
        kb->comm->send_action_request(3, move, 100);
    }
    else if (call_data->key == kb->move_up
             || call_data->key == kb->move_down)
    {
        move.z = (!key_down ? 0.0
                  : (call_data->key == kb->move_up ? 1.0 : -1.0));
        kb->comm->send_action_request(3, move, 100);
    }
    else if (call_data->key == kb->pitch_up
             || call_data->key == kb->pitch_down)
    {
        glm::vec3 rot((call_data->key == kb->pitch_up ? 1.0 : -1.0),
                      0.0,
                      0.0);
        kb->comm->send_action_request(4, rot, (key_down ? 100 : 0));
    }
    else if (call_data->key == kb->roll_left
             || call_data->key == kb->roll_right)
    {
        glm::vec3 rot(0.0,
                      (call_data->key == kb->roll_right ? 1.0 : -1.0),
                      0.0);
        kb->comm->send_action_request(4, rot, (key_down ? 100 : 0));
    }
    else if (call_data->key == kb->yaw_left
             || call_data->key == kb->yaw_right)
    {
        glm::vec3 rot(0.0,
                      0.0,
                      (call_data->key == kb->yaw_left ? 1.0 : -1.0));
        kb->comm->send_action_request(4, rot, (key_down ? 100 : 0));
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
    this->pitch_up = keyboard::PITCH_UP;
    this->pitch_down = keyboard::PITCH_DOWN;
    this->roll_left = keyboard::ROLL_LEFT;
    this->roll_right = keyboard::ROLL_RIGHT;
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
