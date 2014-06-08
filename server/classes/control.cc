/* control.cc
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 10 May 2014, 18:07:48 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2014  Trinity Annabelle Quirk
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
 * NOTE: The way we're looking to send position updates, which is to
 * have each control object put together its own update packets and
 * send them, might not scale too well.  So keep an eye out on that;
 * it might become a pretty bad hot-spot, and might be more properly
 * done in the update threads, where a single update packet can be put
 * together and sent to a whole bunch of users.
 *
 * Changes
 *   02 May 2000 TAQ - Created the file.
 *   15 May 2000 TAQ - Added default_slave, since there could be a way
 *                     that people could trade brains or something.
 *   11 Jun 2000 TAQ - ParseCommand now takes a buffer and a length.
 *   21 Jun 2000 TAQ - Moved all implementation into this file.  We
 *                     will decide about inlining of functions later.
 *                     Got rid of the command source stuff, since it
 *                     doesn't quite get the job done with respect to
 *                     UDP sockets.
 *   21 Oct 2000 TAQ - Made it possible to initialize a control object
 *                     with no slave game object.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   27 Jun 2006 TAQ - We made the slave pointer public, so we can get
 *                     rid of the get/set routines.  I think now we'll just
 *                     check slave's value, and if it's NULL, we'll fall back
 *                     on default_slave.
 *   05 Jul 2006 TAQ - Added struct sockaddr_in member, so we init it in the
 *                     constructor.
 *   27 Jul 2006 TAQ - Changed the parametered constructor into a blank
 *                     constructor.
 *   02 Aug 2006 TAQ - Removed the which_inbound member.
 *   11 Aug 2006 TAQ - The remote member changed to a class.
 *   16 Aug 2006 TAQ - Added init for the userid member.  ParseCommand is now
 *                     called execute_action, and takes an action_request
 *                     packet instead of a char *.
 *   05 Sep 2007 TAQ - Removed references to remote member.
 *   06 Sep 2007 TAQ - Added the send method.
 *   08 Sep 2007 TAQ - Added the send_ack and send_update methods.  Added
 *                     sequence member.  Finished up the send method.  On
 *                     destruction, we unlink the slave objects from us.
 *                     Cleaned up the execute_action method.
 *   18 Sep 2007 TAQ - Debugging to find a crash.
 *   23 Sep 2007 TAQ - The ThreadPool interface changed, so updated the
 *                     sending funcs to no longer pass lengths.
 *   29 Sep 2007 TAQ - Added the why misc-value argument to send_ack.
 *   11 Oct 2007 TAQ - Reworked execute_action.
 *   13 Oct 2007 TAQ - Moved checking of the zone's action routine list
 *                     into the action pool worker.  Removed debugging output.
 *   22 Oct 2007 TAQ - Added send_ping method.
 *   10 May 2014 TAQ - The action level map now exists within the control
 *                     object.  Also, the motion-related stuff is moved out
 *                     of the game object and into a new motion object.
 *                     Added take_over method, for some checked slave setting.
 *                     Switched to the Eigen math library.  Moved the bulk of
 *                     execute_action into the same-named function within
 *                     the zone - we know what *we* can do, but zone needs
 *                     to do checking, and check its own tables.
 *
 * Things to do
 *
 * $Id$
 */

#include <Eigen/Core>
#include <Eigen/Geometry>

#include "control.h"
#include "motion.h"
#include "thread_pool.h"
#include "zone.h"
#include "zone_interface.h"

Control::Control(u_int64_t userid, Motion *slave)
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

bool Control::take_over(Motion *new_slave)
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
    u_int16_t skillid = buf.action_id;
    std::map<u_int16_t, action_level>::iterator i = this->actions.find(skillid);

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

/* Utility method to send a position update for some object to the user */
void Control::send_update(u_int64_t what)
{
    packet pkt;

    /* The what argument contains the objectid of the thing about
     * which we want to update this user.
     */
    game_object_list_element &elem = zone->game_objects[what];
    pkt.pos.type = TYPE_POSUPD;
    pkt.pos.object_id = what;
    /* pkt.frame_number = something */
    pkt.pos.x_pos = (u_int64_t)(elem.position.x() * 100);
    pkt.pos.y_pos = (u_int64_t)(elem.position.y() * 100);
    pkt.pos.z_pos = (u_int64_t)(elem.position.z() * 100);
    pkt.pos.x_orient = (u_int32_t)(elem.orientation.x() * 100);
    pkt.pos.y_orient = (u_int32_t)(elem.orientation.y() * 100);
    pkt.pos.z_orient = (u_int32_t)(elem.orientation.z() * 100);
    pkt.pos.w_orient = (u_int32_t)(elem.orientation.w() * 100);
    /* We've moved these out of the game object
    pkt.pos.x_look = (u_int32_t)(elem.obj->look.x() * 100);
    pkt.pos.y_look = (u_int32_t)(elem.obj->look.y() * 100);
    pkt.pos.z_look = (u_int32_t)(elem.obj->look.z() * 100);
    */

    this->send(&pkt);
}

void Control::send_ping(void)
{
    packet pkt;

    pkt.basic.type = TYPE_PNGPKT;
    this->send(&pkt);
}
