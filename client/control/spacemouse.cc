/* spacemouse.cc
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
 * This file implements a class to use a SpaceMouse to control the client.
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

#include <linux/input.h>

#include <iostream>
#include <sstream>

#include <glm/vec3.hpp>
#include <glm/gtc/quaternion.hpp>

#include "spacemouse.h"
#include "../l10n.h"

void spacemouse::switch_led(bool state)
{
    struct input_event ev;

    ev.type = EV_LED;
    ev.code = LED_MISC;
    ev.value = (state == true ? 1 : 0);
    write(this->fd, &ev, sizeof(struct input_event));
}

void *spacemouse::spacemouse_worker(void *arg)
{
    spacemouse *sm = (spacemouse *)arg;
    struct input_event ev[64];
    int read_size;
    bool send_move = false, send_rotate = false;
    glm::vec3 move(0.0f, 0.0f, 0.0f), null_move(0.0f, 0.0f, 0.0f);
    glm::vec3 rot(0.0f, 0.0f, 0.0f), rot_x(0.0f, 0.0f, 0.0f);
    glm::vec3 rot_y(0.0f, 0.0f, 0.0f), rot_z(0.0f, 0.0f, 0.0f);
    float scale;

    for (;;)
    {
        read_size = read(sm->fd, ev, sizeof(struct input_event) * 64);
        read_size /= sizeof(struct input_event);
        for (int i = 0; i < read_size; ++i)
        {
            if (*(sm->comm) == NULL)
            {
                send_move = send_rotate = false;
                move = null_move;
                continue;
            }

            if (ev[i].type == EV_SYN && ev[i].code == SYN_REPORT)
            {
                if (send_move)
                {
                    (*(sm->comm))->send_action_request(
                        3,
                        move,
                        move.length() * 100
                    );
                    send_move = false;
                    move = null_move;
                }
                if (send_rotate)
                {
                    (*(sm->comm))->send_action_request(
                        4,
                        rot_x,
                        rot_x.length() * 100
                    );
                    (*(sm->comm))->send_action_request(
                        4,
                        rot_y,
                        rot_y.length() * 100
                    );
                    (*(sm->comm))->send_action_request(
                        4,
                        rot_z,
                        rot_z.length() * 100
                    );
                    send_rotate = false;
                }
            }
            else if (ev[i].type == EV_REL)
            {
                if (ev[i].value <= sm->deadzone
                    && ev[i].value >= -sm->deadzone)
                {
                    send_move = send_rotate = true;
                    continue;
                }

                /* Max values seem to be ±350 */
                scale = (ev[i].value < 0
                         ? ev[i].value + sm->deadzone
                         : ev[i].value - sm->deadzone)
                    / (350.0f - sm->deadzone);
                switch (ev[i].code)
                {
                  case REL_X:
                    move += glm::vec3(1.0, 0.0, 0.0) * scale;
                    send_move = true;
                    break;
                  case REL_Y:
                    move += glm::vec3(0.0, -1.0, 0.0) * scale;
                    send_move = true;
                    break;
                  case REL_Z:
                    move += glm::vec3(0.0, 0.0, -1.0) * scale;
                    send_move = true;
                    break;
                  case REL_RX:
                    rot_x = glm::vec3(1.0, 0.0, 0.0) * scale;
                    send_rotate = true;
                    break;
                  case REL_RY:
                    rot_y = glm::vec3(0.0, 1.0, 0.0) * scale;
                    send_rotate = true;
                    break;
                  case REL_RZ:
                    rot_z = glm::vec3(0.0, 0.0, 1.0) * scale;
                    send_rotate = true;
                    break;
                }
            }
            else if (ev[i].type == EV_KEY)
            {
                switch (ev[i].code)
                {
                  case BTN_0:
                    (*(sm->comm))->send_action_request(5, null_move, 100);
                    break;
                  case BTN_1:  break;
                }
            }
        }
    }
    return NULL;
}

void spacemouse::init(const char *device)
{
    int ret;

    if ((this->fd = open(device, O_RDWR)) == -1)
    {
        std::ostringstream s;
        char err[128];

        s << format(
            translate(
                "Error opening SpaceMouse device {1,name}: "
                "{2,errmsg} ({3,errno})"
            )
        ) % device % strerror_r(errno, err, sizeof(err)) % errno;
        throw std::runtime_error(s.str());
    }
    if ((ret = pthread_create(&this->worker,
                              NULL,
                              spacemouse::spacemouse_worker,
                              (void *)this)) != 0)
    {
        std::ostringstream s;
        char err[128];

        s << format(
            translate(
                "Error starting SpaceMouse worker thread: "
                "{1,errmsg} ({2,errno})"
            )
        ) % strerror_r(ret, err, sizeof(err)) % ret;
        close(this->fd);
        throw std::runtime_error(s.str());
    }
    this->switch_led(true);
    this->deadzone = 20;
    std::clog << format(
        translate("Initialized {1,devicename}")
    ) % device << std::endl;
}

spacemouse::spacemouse()
{
    glob_t devices;

    glob("/dev/input/by-id/*SpaceMouse*", 0, NULL, &devices);
    if (devices.gl_pathc == 0)
        throw std::runtime_error(
            translate("Could not find any SpaceMouse devices")
        );
    this->init(devices.gl_pathv[0]);
    globfree(&devices);
}

spacemouse::spacemouse(std::string& device)
{
    this->init(device.c_str());
}

spacemouse::~spacemouse()
{
    int retval;

    this->switch_led(false);
    if ((retval = pthread_cancel(this->worker)) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error cancelling SpaceMouse worker: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(retval, err, sizeof(err)) % retval << std::endl;
    }
    else if ((retval = pthread_join(this->worker, NULL)) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error joining SpaceMouse worker: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(retval, err, sizeof(err)) % retval << std::endl;
    }
    close(this->fd);
}

/* ARGSUSED */
void spacemouse::setup(ui::active *comp, Comm **comm)
{
    this->comm = comm;
}

/* ARGSUSED */
void spacemouse::cleanup(ui::active *comp, Comm **comm)
{
    if (this->comm == comm)
        this->comm = NULL;
}

extern "C" bool can_use(const char *device)
{
    return strstr(device, "SpaceMouse") != NULL;
}

extern "C" control *create(const char *device)
{
    std::string dev = device;
    return (control *)new spacemouse(dev);
}
