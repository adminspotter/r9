/* control.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Jul 2019, 08:14:37 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2019  Trinity Annabelle Quirk
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
 * This file contains the implementation for the standard control
 * class for the Revision IX system.
 *
 * Things to do
 *
 */

#include "control.h"
#include "thread_pool.h"

Control::Control(uint64_t userid, GameObject *slave)
{
    this->userid = userid;
    this->default_slave = this->slave = slave;
    if (this->slave != NULL)
        this->slave->connect(this);
}

Control::~Control()
{
    /* Disconnect the slaves */
    if (this->slave != NULL)
        this->slave->disconnect(this);
    if (this->default_slave != NULL && this->default_slave != this->slave)
        this->default_slave->disconnect(this);
    this->default_slave = this->slave = NULL;
}

bool Control::operator<(const Control& c) const
{
    return (this->userid < c.userid);
}

bool Control::operator==(const Control& c) const
{
    return (this->userid == c.userid);
}

const Control& Control::operator=(const Control& c)
{
    this->userid = c.userid;
    this->default_slave = c.default_slave;
    this->slave = c.slave;
    this->actions.insert(c.actions.begin(), c.actions.end());
    return *this;
}

bool Control::take_over(GameObject *new_slave)
{
    if (new_slave->connect(this))
    {
        this->slave = new_slave;
        return true;
    }
    return false;
}
