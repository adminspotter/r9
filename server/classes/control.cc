/* control.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Nov 2015, 08:24:17 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2015  Trinity Annabelle Quirk
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

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "control.h"
#include "thread_pool.h"
#include "zone.h"
#include "../server.h"

Control::Control(uint64_t userid, GameObject *slave)
{
    this->userid = userid;
    this->default_slave = this->slave = NULL;
    if (slave->connect(this))
        this->default_slave = this->slave = slave;
    /* Come up with some sort of random sequence number to start? */
    this->sequence = 0L;
}

Control::~Control()
{
    /* Disconnect the slave */
    if (this->slave != NULL)
        this->slave->disconnect(this);
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

void Control::execute_action(action_request& buf, size_t len)
{
    uint16_t skillid = buf.action_id;
    std::map<uint16_t, action_level>::iterator i = this->actions.find(skillid);

    if (i != this->actions.end())
    {
        /* Ceiling the power of the request by our skill level */
        buf.power_level = std::max<int>(buf.power_level, i->second.level);
        zone->execute_action(this, buf, len);
    }
}

/* Our parent member should be pointed to a sending queue */
void Control::send(packet *pkt)
{
    packet_list pl;

    /* If we don't have a parent sending queue, obviously we can't
     * send anything.
     */
    if (this->parent == NULL)
        return;

    memcpy(&(pl.buf), pkt, sizeof(packet));
    /* Set the basic params for all packets:  version and sequence number */
    pl.buf.basic.version = 1;
    pl.buf.basic.sequence = this->sequence++;
    pl.who = this->userid;
    ((ThreadPool<packet_list> *)this->parent)->push(pl);
}

/* Utility method to send a basic ack packet to the user */
void Control::send_ack(int what, int why)
{
    packet pkt;

    /* The what argument contains the type of request we're acknowledging */
    pkt.ack.type = TYPE_ACKPKT;
    pkt.ack.request = what;
    /* The why argument is the misc data */
    pkt.ack.misc = why;

    this->send(&pkt);
}

void Control::send_ping(void)
{
    packet pkt;

    pkt.basic.type = TYPE_PNGPKT;
    this->send(&pkt);
}
