/* action_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 17 Apr 2018, 05:41:21 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2018  Trinity Annabelle Quirk
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

#include <glm/vec3.hpp>

#include "action_pool.h"
#include "listensock.h"

#include "../../proto/proto.h"
#include "../server.h"

typedef void action_reg_t(std::map<uint16_t, action_rec>&, MotionPool *);
typedef void action_unreg_t(std::map<uint16_t, action_rec>&);

void ActionPool::load_actions(void)
{
    if (this->action_lib != NULL)
    {
        std::clog << "loading action routines" << std::endl;
        action_reg_t *reg
            = (action_reg_t *)this->action_lib->symbol("actions_register");
        (*reg)(this->actions, motion_pool);
        for (auto &i : this->actions)
            std::clog << "  loaded " << i.second.name
                      << " ("
                      << std::hex << (void *)i.second.action << std::dec << ')'
                      << std::endl;
    }
}

ActionPool::ActionPool(unsigned int pool_size,
                       std::map<uint64_t, GameObject *>& game_obj,
                       Library *lib,
                       DB *database)
    : ThreadPool<packet_list>("action", pool_size), actions(),
      game_objects(game_obj)
{
    database->get_server_skills(this->actions);
    this->action_lib = lib;
    this->load_actions();
}

ActionPool::~ActionPool()
{
    this->stop();

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

/* We will only use this thread pool in a very specific way, so making
 * the caller handle stuff that it doesn't need to know about is
 * inappropriate.  We'll set the required arg and start things up with
 * the expected function.
 */
void ActionPool::start(void)
{
    this->startup_arg = (void *)this;
    this->ThreadPool<packet_list>::start(ActionPool::action_pool_worker);
}

void *ActionPool::action_pool_worker(void *arg)
{
    ActionPool *act = (ActionPool *)arg;
    packet_list req;

    for (;;)
    {
        act->pop(&req);
        act->execute_action(req.who, req.buf.act);
    }
    return NULL;
}

void ActionPool::execute_action(base_user *user, action_request& req)
{
    ActionPool::actions_iterator i = this->actions.find(req.action_id);
    Control::actions_iterator j = user->actions.find(req.action_id);
    std::map<uint64_t, GameObject *>::iterator k =
        this->game_objects.find(req.dest_object_id);
    glm::dvec3 vec(req.x_pos_dest, req.y_pos_dest, req.z_pos_dest);
    GameObject *target = NULL;
    int retval;

    if (k != this->game_objects.end())
        target = k->second;

    /* TODO:  if the user doesn't have the skill, but it is valid, we
     * should add it at level 0 so they can start accumulating
     * improvement points.
     */

    if (i != this->actions.end()
        && j != user->actions.end()
        && user->slave->get_object_id() == req.object_id)
    {
        /* If it's not valid on this server, it should at least have
         * a default.
         */
        if (!i->second.valid)
        {
            req.action_id = i->second.def;
            i = this->actions.find(req.action_id);
        }

        req.power_level = std::max<uint8_t>(req.power_level, i->second.lower);
        req.power_level = std::min<uint8_t>(req.power_level, i->second.upper);
        req.power_level = std::max<uint8_t>(req.power_level, j->second.level);

        retval = (*(i->second.action))(user->slave,
                                       req.power_level,
                                       target,
                                       vec);
        user->send_ack(TYPE_ACTREQ, (uint8_t)retval);
    }
}
