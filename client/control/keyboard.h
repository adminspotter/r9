/* keyboard.h                                               -*- C++ -*-
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
 * This file declares a class to use the keyboard to control the client.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_KEYBOARD_H__
#define __INC_R9CLIENT_KEYBOARD_H__

#include "control.h"

class keyboard : public control
{
  public:
    /* Default constants */
    static const int MOVE_UP, MOVE_DOWN, MOVE_LEFT, MOVE_RIGHT;
    static const int MOVE_FORWARD, MOVE_BACK, PITCH_UP, PITCH_DOWN;
    static const int ROLL_LEFT, ROLL_RIGHT, YAW_LEFT, YAW_RIGHT;

    int move_up, move_down, move_left, move_right, move_forward, move_back;
    int pitch_up, pitch_down, roll_left, roll_right, yaw_left, yaw_right;

  private:
    Comm *comm;

    static void keyboard_callback(ui::active *, void *, void *);

  public:
    keyboard();
    ~keyboard();

    void set_defaults(void);

    virtual void setup(ui::active *, Comm *) override;
    virtual void cleanup(ui::active *, Comm *) override;
};

#endif /* __INC_R9CLIENT_KEYBOARD_H__ */
