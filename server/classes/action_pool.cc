/* action_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 28 Nov 2015, 10:01:07 tquirk
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
 * The minimal implementation of the Action pool.
 */

#include "action_pool.h"
#include "zone.h"
#include "listensock.h"

ActionPool::ActionPool(const char *pool_name, unsigned int pool_size)
    : ThreadPool<packet_list>(pool_name, pool_size)
{
}

ActionPool::~ActionPool()
{
}

void ActionPool::start(void *(*func)(void *))
{
    ThreadPool<packet_list>::start(func);
}

/* Unfortunately since we depend on some other stuff in the zone, we
 * have to take it, rather than ourselves, as an argument.  For
 * testing purposes, we should be able to mock the zone out enough to
 * verify that we're operating correctly.
 */
void *ActionPool::action_pool_worker(void *arg)
{
    Zone *zone = (Zone *)arg;
    packet_list req;
    uint16_t skillid;
    int ret;

    for (;;)
    {
        /* Grab the next packet off the queue */
        zone->action_pool->pop(&req);

        /* Process the packet */
        ntoh_packet(&req.buf, sizeof(packet));

        /* Make sure the action exists and is valid on this server,
         * before trying to do anything
         */
        skillid = req.buf.act.action_id;
        if (zone->actions.find(skillid) != zone->actions.end()
            && zone->actions[skillid].valid == true)
        {
            /* Make the action call.
             *
             * The action routine will handle checking the environment
             * and relevant skills of the target, and spawning of any
             * new needed subobjects.
             */
            if (req.who->slave->get_object_id() == req.buf.act.object_id)
                if ((ret = zone->execute_action(req.who,
                                                req.buf.act,
                                                sizeof(action_request))) > 0)
                {
                    packet_list pkt;

                    pkt.buf.ack.type = TYPE_ACKPKT;
                    pkt.buf.ack.version = 1;
                    /*pkt.buf.ack.sequence = ;*/
                    pkt.buf.ack.request = skillid;
                    pkt.buf.ack.misc = (uint8_t)ret;
                    pkt.who = req.who;

                    req.parent->send_pool->push(pkt);
                }
        }
    }
    return NULL;
}
