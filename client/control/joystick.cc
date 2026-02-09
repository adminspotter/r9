/* joystick.cc
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
 * This file implements a class to use a joystick to control the client.
 *
 * Things to do
 *
 */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <glob.h>
#include <errno.h>

#include <linux/joystick.h>

#include <iostream>
#include <sstream>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "joystick.h"
#include "../l10n.h"

void *joystick::joystick_worker(void *arg)
{
    joystick *js = (joystick *)arg;
    struct js_event ev[64];
    int read_size;
    bool flip, send_move = false;
    glm::vec3 move(0.0f, 0.0f, 0.0f), rot(0.0, 0.0, 0.0);
    glm::vec3 rot_x(1.0f, 0.0f, 0.0f), rot_y(0.0f, 1.0f, 0.0f);
    glm::vec3 rot_z(0.0f, 0.0f, 1.0f);
    float scale;

    for (;;)
    {
        read_size = read(js->fd, ev, sizeof(struct js_event) * 64);
        read_size /= sizeof(struct js_event);
        for (int i = 0; i < read_size; ++i)
        {
            if (*(js->comm) == NULL)
            {
                send_move = false;
                continue;
            }

            if ((ev[i].type & ~JS_EVENT_INIT) == JS_EVENT_AXIS)
            {
                if (js->axes[ev[i].number].func == joystick::axis_func::NO_AXIS)
                    continue;
                if (ev[i].value <= js->deadzone && ev[i].value >= -js->deadzone)
                    scale = 0.0;
                else
                    scale = (ev[i].value < 0
                             ? ev[i].value + js->deadzone
                             : ev[i].value - js->deadzone)
                        / (32768.0f - js->deadzone);
                flip = js->axes[ev[i].number].flip;
                switch (js->axes[ev[i].number].func)
                {
                  case joystick::axis_func::HORIZONTAL:
                    move.x = (flip ? -scale : scale);
                    send_move = true;
                    break;
                  case joystick::axis_func::LATERAL:
                    move.y = (flip ? -scale : scale);
                    send_move = true;
                    break;
                  case joystick::axis_func::VERTICAL:
                    move.z = (flip ? -scale : scale);
                    send_move = true;
                    break;
                  case joystick::axis_func::PITCH:
                    (*(js->comm))->send_rotate(flip ? -rot_x : rot_x, scale);
                    break;
                  case joystick::axis_func::ROLL:
                    (*(js->comm))->send_rotate(flip ? -rot_y : rot_y, scale);
                    break;
                  case joystick::axis_func::YAW:
                    (*(js->comm))->send_rotate(flip ? -rot_z : rot_z, scale);
                    break;
                }
            }
            else if ((ev[i].type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON)
            {
                if (js->buttons[ev[i].number] == joystick::btn_func::NO_BTN)
                    continue;
                scale = ev[i].value;
                switch (js->buttons[ev[i].number])
                {
                  case joystick::btn_func::MOVE_LEFT:
                    move.x = -scale;  send_move = true;  break;
                  case joystick::btn_func::MOVE_RIGHT:
                    move.x = scale;  send_move = true;  break;
                  case joystick::btn_func::MOVE_FORWARD:
                    move.y = scale;  send_move = true;  break;
                  case joystick::btn_func::MOVE_BACK:
                    move.y = -scale;  send_move = true;  break;
                  case joystick::btn_func::MOVE_UP:
                    move.z = scale;  send_move = true;  break;
                  case joystick::btn_func::MOVE_DOWN:
                    move.z = -scale;  send_move = true;  break;
                  case joystick::btn_func::PITCH_UP:
                    (*(js->comm))->send_rotate(rot_x, scale);  break;
                  case joystick::btn_func::PITCH_DOWN:
                    (*(js->comm))->send_rotate(-rot_x, scale);  break;
                  case joystick::btn_func::ROLL_LEFT:
                    (*(js->comm))->send_rotate(-rot_y, scale);  break;
                  case joystick::btn_func::ROLL_RIGHT:
                    (*(js->comm))->send_rotate(rot_y, scale);  break;
                  case joystick::btn_func::YAW_LEFT:
                    (*(js->comm))->send_rotate(rot_z, scale);  break;
                  case joystick::btn_func::YAW_RIGHT:
                    (*(js->comm))->send_rotate(-rot_z, scale);  break;
                }
            }
        }
        if (send_move == true)
        {
            (*(js->comm))->send_move(move);
            send_move = false;
        }
    }
    return NULL;
}

void joystick::init(const char *device)
{
    int ret;

    this->axis_count = this->button_count = 0;
    this->axes = NULL;
    this->buttons = NULL;
    if ((this->fd = open(device, O_RDONLY)) < 0)
    {
        std::ostringstream s;
        char err[128];

        s << format(
            translate(
                "Error opening joystick device {1,name}: {2,errmsg} ({3,errno})"
            )
        ) % device % strerror_r(errno, err, sizeof(err)) % errno;
        throw std::runtime_error(s.str());
    }
    if (ioctl(this->fd, JSIOCGAXES, &this->axis_count) < 0)
    {
        std::ostringstream s;
        char err[128];

        s << format(
            translate(
                "Error fetching joystick axis count: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(errno, err, sizeof(err)) % errno;
        throw std::runtime_error(s.str());
    }
    this->axes = new joystick::axis_rec[this->axis_count];
    for (auto i = 0; i < this->axis_count; ++i)
        this->axes[i] = {joystick::axis_func::NO_AXIS, false};
    if (ioctl(this->fd, JSIOCGBUTTONS, &this->button_count) < 0)
    {
        std::ostringstream s;
        char err[128];

        s << format(
            translate(
                "Error fetching joystick button count: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(errno, err, sizeof(err)) % errno;
        throw std::runtime_error(s.str());
    }
    this->buttons = new joystick::btn_func[this->button_count];
    for (auto i = 0; i < this->button_count; ++i)
        this->buttons[i] = joystick::btn_func::NO_BTN;
    if ((ret = pthread_create(&this->worker,
                              NULL,
                              joystick::joystick_worker,
                              (void *)this)) != 0)
    {
        std::ostringstream s;
        char err[128];

        s << format(
            translate(
                "Error starting joystick worker thread: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(ret, err, sizeof(err)) % ret;
        close(this->fd);
        throw std::runtime_error(s.str());
    }
    this->deadzone = 20;
    std::clog << format(
        translate("Initialized {1,devicename}")
    ) % device << std::endl;
}

joystick::joystick(const std::string& device)
    : control(device)
{
    this->init(device.c_str());
    this->set_defaults();
}

joystick::~joystick()
{
    int ret;

    if ((ret = pthread_cancel(this->worker)) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error cancelling joystick worker: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(ret, err, sizeof(err)) % ret << std::endl;
    }
    else if ((ret = pthread_join(this->worker, NULL)) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error joining joystick worker: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(ret, err, sizeof(err)) % ret << std::endl;
    }
    close(this->fd);
}

void joystick::set_defaults(void)
{
    if (this->axis_count >= 2)
    {
        this->axes[0] = {joystick::axis_func::YAW, false};
        this->axes[1] = {joystick::axis_func::LATERAL, true};
    }
    if (this->axis_count >= 4)
    {
        this->axes[2] = {joystick::axis_func::HORIZONTAL, false};
        this->axes[3] = {joystick::axis_func::VERTICAL, true};
    }
    if (this->button_count >= 6)
    {
        this->buttons[4] = joystick::btn_func::ROLL_LEFT;
        this->buttons[5] = joystick::btn_func::ROLL_RIGHT;
    }
}

/* ARGSUSED */
void joystick::setup(ui::active *comp, Comm **comm)
{
    this->comm = comm;
}

/* ARGSUSED */
void joystick::cleanup(ui::active *comp, Comm **comm)
{
    if (this->comm == comm)
        this->comm = NULL;
}

extern "C" bool can_use(const char *device)
{
    return strstr(device, "joystick") != NULL
        && strstr(device, "event") == NULL;
}

extern "C" control *create(const char *device)
{
    std::string dev = device;
    return (control *)new joystick(dev);
}
