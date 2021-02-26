/* action_pool.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Feb 2021, 14:11:00 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2020  Trinity Annabelle Quirk
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
 * the action libraries, and handles the dispatch entirely internally.
 *
 * Things to do
 *
 */

#include <config.h>

#include "action_pool.h"
#include "listensock.h"

#include "../../proto/proto.h"
#include "../server.h"

typedef void action_reg_t(actions_map&, MotionPool *);
typedef void action_unreg_t(actions_map&);

void ActionPool::load_actions(void)
{
    std::string path(ACTION_LIB_DIR);

    path += "/*" LT_MODULE_EXT;
    find_libraries(path, this->action_libs);
    if (this->action_libs.size())
    {
        std::clog << "loading action routines" << std::endl;
        for (auto i = this->action_libs.begin();
             i != this->action_libs.end();
             ++i)
        {
            action_reg_t *reg
                = (action_reg_t *)(*i)->symbol("actions_register");
            (*reg)(this->actions, motion_pool);
        }
        for (auto &i : this->actions)
            std::clog << "  loaded " << i.second.name
                      << " ("
                      << std::hex << (void *)i.second.action << std::dec << ')'
                      << std::endl;
    }
}

ActionPool::ActionPool(unsigned int pool_size,
                       GameObject::objects_map& game_obj,
                       DB *database)
    : ThreadPool<packet_list>("action", pool_size), actions(),
      action_libs(),
      game_objects(game_obj)
{
    database->get_server_skills(this->actions);
    this->load_actions();
}

ActionPool::~ActionPool()
{
    this->stop();

    std::clog << "cleaning up action routines" << std::endl;
    while (this->action_libs.size())
    {
        try
        {
            action_unreg_t *unreg = (action_unreg_t *)this->action_libs.back()
                ->symbol("actions_unregister");
            (*unreg)(this->actions);
        }
        catch (std::exception& e) { }
        delete this->action_libs.back();
        this->action_libs.pop_back();
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
    actions_iterator i = this->actions.find(req.action_id);
    Control::skills_iterator j = user->actions.find(req.action_id);
    GameObject::objects_iterator k =
        this->game_objects.find(req.dest_object_id);
    glm::dvec3 dest((double)req.x_pos_dest / (double)ACTREQ_POS_SCALE,
                    (double)req.y_pos_dest / (double)ACTREQ_POS_SCALE,
                    (double)req.z_pos_dest / (double)ACTREQ_POS_SCALE);
    GameObject *target = NULL;

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
        req.power_level = std::min<uint8_t>(req.power_level, j->second.level);

        user->send_ack(TYPE_ACTREQ,
                       (uint8_t)(*(i->second.action))(user->slave,
                                                      req.power_level,
                                                      target,
                                                      dest));
    }
}
