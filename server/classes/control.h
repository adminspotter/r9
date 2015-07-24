/* control.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 13:28:13 tquirk
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
 * This file contains the declaration of the control class for the
 * Revision IX system.
 *
 * Changes
 *   02 May 2000 TAQ - Created the file.
 *   15 May 2000 TAQ - Added the default_slave, since there should be
 *                     a way for people to trade brains or something.
 *   11 Jun 2000 TAQ - ParseCommand now takes some args.
 *   21 Jun 2000 TAQ - Moved all implementation out of this file.  We
 *                     can decide about inlining later.  Removed all the
 *                     command source related stuff.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   27 Jun 2006 TAQ - Made the changeable slave public, to simplify things.
 *   05 Jul 2006 TAQ - Added struct sockaddr_in member.
 *   12 Jul 2006 TAQ - Added which_inbound member, which records which of
 *                     the main sockets this particular control is using.
 *   27 Jul 2006 TAQ - Made the constructor without arguments, since the
 *                     GameObject requires a command object to create.  Also
 *                     made the default slave public, since we'll have to set
 *                     it after creation.
 *   02 Aug 2006 TAQ - Removed which_inbound; it's possible that we'll deal
 *                     with connections that way in the future, but it's
 *                     unnecessary complication right now.
 *   11 Aug 2006 TAQ - Replaced the struct sockaddr_in with our new wrapper
 *                     class, Sockaddr_in.
 *   16 Aug 2006 TAQ - Added userid member.  ParseCommand is now called
 *                     execute_action, and takes an action_request packet
 *                     instead of a char *.
 *   22 Jun 2007 TAQ - Added username member.
 *   30 Jun 2007 TAQ - Changed Sockaddr to reflect name update.
 *   05 Sep 2007 TAQ - Removed sockaddr.h include and Sockaddr member.
 *   06 Sep 2007 TAQ - Added send method.
 *   08 Sep 2007 TAQ - Added private sequence number, used in sending packets.
 *   23 Sep 2007 TAQ - Removed the size arg from send(), since the ThreadPool
 *                     push() method no longer takes a size arg.
 *   29 Sep 2007 TAQ - Added a misc argument with default value to send_ack.
 *   22 Oct 2007 TAQ - Added send_ping method.
 *   10 May 2014 TAQ - The control object now references the new motion object,
 *                     rather than the game object itself.  We've also moved
 *                     the action levels here, since they should sit with
 *                     the controlling character, rather than the object that
 *                     is being controlled.  Added take_over method.  Updated
 *                     the username to be std::string instead of a char *.
 *   24 Jul 2015 TAQ - Converted to stdint types.  Comment cleanup.
 *
 * Things to do
 *   - The send* methods seem weird here.  Do they really belong?
 *
 */

#ifndef __INC_CONTROL_H__
#define __INC_CONTROL_H__

#include <sys/types.h>

#include <cstdint>
#include <map>
#include <string>

class Control;

#include "motion.h"

class Control
{
  public:
    uint64_t userid;
    Motion *default_slave, *slave;
    void *parent;  /* This will point at the sending thread queue */
    std::string username;
    std::map<uint16_t, action_level> actions;

  private:
    uint64_t sequence;

  public:
    Control(uint64_t, Motion *);
    ~Control();

    bool take_over(Motion *);

    void execute_action(action_request&, size_t);
    void send(packet *);
    void send_ack(int, int = 0);
    void send_update(uint64_t);
    void send_ping(void);
};

#endif /* __INC_CONTROL_H__ */
