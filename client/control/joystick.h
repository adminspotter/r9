/* joystick.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game client
 * Copyright (C) 2021-2026  Trinity Annabelle Quirk
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
 * This file declares a class to use a joystick to control the client.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_JOYSTICK_H__
#define __INC_R9CLIENT_JOYSTICK_H__

#include <pthread.h>

#include <string>

#include "control.h"

class joystick : public control
{
  public:
    typedef enum
    {
        NO_AXIS = 0,
        HORIZONTAL, LATERAL, VERTICAL, PITCH, ROLL, YAW
    }
    axis_func;
    typedef struct axis_rec_tag
    {
        axis_func func;
        bool flip;
    }
    axis_rec;
    typedef enum
    {
        NO_BTN = 0,
        MOVE_LEFT, MOVE_RIGHT, MOVE_FORWARD, MOVE_BACK, MOVE_UP, MOVE_DOWN,
        PITCH_UP, PITCH_DOWN, ROLL_LEFT, ROLL_RIGHT, YAW_LEFT, YAW_RIGHT
    }
    btn_func;

  private:
    int fd;
    Comm **comm;
    pthread_t worker;
    char axis_count, button_count;
    axis_rec *axes;
    btn_func *buttons;

  public:
    int deadzone;

  private:
    static void *joystick_worker(void *);

    void init(const char *);

  public:
    explicit joystick(const std::string&);
    virtual ~joystick();

    void set_defaults(void);

    virtual void setup(ui::active *, Comm **) override;
    virtual void cleanup(ui::active *, Comm **) override;
};

#endif /* __INC_R9CLIENT_JOYSTICK_H__ */
