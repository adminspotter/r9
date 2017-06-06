/* action_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Jun 2017, 18:49:05 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2017  Trinity Annabelle Quirk
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
 * The action pool takes action requests, decides if they're valid and
 * available on the server, and dispatches them to the appropriate
 * action routine.
 *
 * This object takes care of loading and unloading the actions from
 * the action library, and handles the dispatch entirely internally.
 *
 * Things to do
 *
 */

#include "action_pool.h"
#include "zone.h"
#include "listensock.h"

void ActionPool::load_actions(const std::string& libname)
{
    std::clog << "loading action routines" << std::endl;
    this->action_lib = new Library(libname);
    action_reg_t *reg
        = (action_reg_t *)this->action_lib->symbol("actions_register");
    (*reg)(this->actions);
}

ActionPool::ActionPool(const std::string& libname,
                       unsigned int pool_size,
                       DB *database)
    : ThreadPool<packet_list>("action", pool_size), actions(),
{
    database->get_server_skills(this->actions);
    this->load_actions(libname);
}

ActionPool::~ActionPool()
{
    /* Unregister all the action routines. */
    if (this->action_lib != NULL)
    {
        std::clog << "cleaning up action routines" << std::endl;
        try
        {
            action_unreg_t *unreg
                = (action_unreg_t *)this->action_lib->symbol("actions_unregister");
            (*unreg)(this->actions);
        }
        catch (std::exception& e) { /* Destructors don't throw exceptions */ }

        delete this->action_lib;
    }
    this->actions.clear();
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
