/* spacemouse.h                                            -*- C++ -*-
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
 * This file declares a class to use a SpaceMouse to control the client.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_SPACEMOUSE_H__
#define __INC_R9CLIENT_SPACEMOUSE_H__

#include <pthread.h>

#include <string>

#include "control.h"

class spacemouse : public control
{
  private:
    int fd;
    Comm **comm;
    pthread_t worker;

  public:
    int deadzone;

  private:
    void switch_led(bool);

    static void *spacemouse_worker(void *);

    void init(const char *);

  public:
    spacemouse();
    explicit spacemouse(std::string&);
    virtual ~spacemouse();

    virtual void setup(ui::active *, Comm **) override;
    virtual void cleanup(ui::active *, Comm **) override;
};

#endif /* __INC_R9CLIENT_SPACEMOUSE_H__ */
