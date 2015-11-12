/* zone.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 12 Nov 2015, 06:49:19 tquirk
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
 * This is the implementation of the zone object.
 *
 * Things to do
 *
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glob.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>

#include "zone.h"
#include "thread_pool.h"
#include "listensock.h"
#include "../config_data.h"
#include "../log.h"

extern std::vector<listen_socket *> sockets;

/* Private methods */
void Zone::init(void)
{
    int i;
    std::vector<Octree *> z_row;
    std::vector<std::vector<Octree *> > y_row;

    this->load_actions(config.action_lib);
    this->create_thread_pools();
    std::clog << syslogNotice << "creating " << this->x_steps << 'x'
              << this->y_steps << 'x' << this->z_steps << " elements"
              << std::endl;
    z_row.reserve(this->z_steps);
    for (i = 0; i < this->z_steps; ++i)
        z_row.push_back(NULL);
    y_row.reserve(this->y_steps);
    for (i = 0; i < this->y_steps; ++i)
        y_row.push_back(z_row);
    this->sectors.reserve(x_steps);
    for (i = 0; i < this->x_steps; ++i)
        this->sectors.push_back(y_row);
}

void Zone::load_actions(const std::string& libname)
{
    try
    {
        this->action_lib = new Library(libname);
        action_reg_t *reg
            = (action_reg_t *)this->action_lib->symbol("actions_register");
        (*reg)(this->actions);
    }
    catch (std::exception& e)
    {
        std::clog << syslogErr
                  << "error loading actions library: " << e.what() << std::endl;
    }
}

void Zone::create_thread_pools(void)
{
    this->action_pool
        = new ThreadPool<packet_list>("action", config.action_threads);
    this->motion_pool
        = new ThreadPool<Motion *>("motion", config.motion_threads);
    this->update_pool
        = new ThreadPool<GameObject *>("update", config.update_threads);
}

void *Zone::action_pool_worker(void *arg)
{
    Zone *zone = (Zone *)arg;
    packet_list req;
    uint16_t skillid;

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
            if (((Control *)req.who)->slave->object->get_object_id()
                == req.buf.act.object_id)
                ((Control *)req.who)->execute_action(req.buf.act,
                                                     sizeof(action_request));
        }
    }
    return NULL;
}

void *Zone::motion_pool_worker(void *arg)
{
    Zone *zone = (Zone *)arg;
    Motion *req;
    struct timeval current;
    double interval;
    Octree *sector;

    for (;;)
    {
        zone->motion_pool->pop(&req);

        gettimeofday(&current, NULL);
        interval = (current.tv_sec + (current.tv_usec * 1000000))
            - (req->last_updated.tv_sec
               + (req->last_updated.tv_usec * 1000000));
        memcpy(&req->last_updated, &current, sizeof(struct timeval));
        zone->sector_contains(req->position)->remove(req);
        req->position += req->movement * interval;
        /*req->orient += req->rotation * interval;*/
        sector = zone->sector_contains(req->position);
        if (sector == NULL)
        {
            Eigen::Vector3i sec = zone->which_sector(req->position);
            Eigen::Vector3d mn, mx;

            mn[0] = sec[0] * zone->x_dim;
            mn[1] = sec[1] * zone->y_dim;
            mn[2] = sec[2] * zone->z_dim;
            mx[0] = mn[0] + zone->x_dim;
            mx[1] = mn[1] + zone->y_dim;
            mx[2] = mn[2] + zone->z_dim;
            sector = new Octree(NULL, mn, mx, 0);
            zone->sectors[sec[0]][sec[1]][sec[2]] = sector;
        }
        sector->insert(req);
        /*zone->physics->collide(sector, req);*/
        zone->update_pool->push(req->object);

        if ((req->movement[0] != 0.0
             || req->movement[1] != 0.0
             || req->movement[2] != 0.0)
            || (req->rotation[0] != 0.0
                || req->rotation[1] != 0.0
                || req->rotation[2] != 0.0))
            zone->motion_pool->push(req);
    }
    return NULL;
}

void *Zone::update_pool_worker(void *arg)
{
    Zone *zone = (Zone *)arg;
    GameObject *req;
    uint64_t objid;
    std::vector<listen_socket *>::iterator sock;
    std::map<uint64_t, base_user *>::iterator user;

    for (;;)
    {
        zone->update_pool->pop(&req);

        objid = req->get_object_id();

        /* Figure out who to send it to */
        /* Send to EVERYONE (for now) */
        for (sock = sockets.begin(); sock != sockets.end(); ++sock)
            for (user = (*sock)->users.begin();
                 user != (*sock)->users.end();
                 ++user)
                user->second->control->send_update(objid);
    }
    return NULL;
}

/* Public methods */
Zone::Zone(uint64_t dim, uint16_t steps)
    : sectors(), actions(), game_objects()
{
    this->x_dim = this->y_dim = this->z_dim = dim;
    this->x_steps = this->y_steps = this->z_steps = steps;
    this->init();
}

Zone::Zone(uint64_t xd, uint64_t yd, uint64_t zd,
           uint16_t xs, uint16_t ys, uint16_t zs)
    : sectors(), actions(), game_objects()
{
    this->x_dim = xd;
    this->y_dim = yd;
    this->z_dim = zd;
    this->x_steps = xs;
    this->y_steps = ys;
    this->z_steps = zs;
    this->init();
}

Zone::~Zone()
{
    int i, j, k;

    /* Stop the thread pools first; deleting them should run through the
     * destructor, which will terminate all the threads and clean up
     * the queues and stuff.
     */
    std::clog << "deleting thread pools" << std::endl;
    if (this->action_pool != NULL)
    {
        delete this->action_pool;
        this->action_pool = NULL;
    }
    if (this->motion_pool != NULL)
    {
        delete this->motion_pool;
        this->motion_pool = NULL;
    }
    if (this->update_pool != NULL)
    {
        delete this->update_pool;
        this->update_pool = NULL;
    }

    /* Clear out the octrees */
    for (i = 0; i < this->x_steps; ++i)
        for (j = 0; j < this->y_steps; ++j)
            for (k = 0; k < this->z_steps; ++k)
                if (this->sectors[i][j][k] != NULL)
                    delete this->sectors[i][j][k];

    /* Delete all the game objects. */
    std::clog << "deleting game objects" << std::endl;
    if (this->game_objects.size())
    {
        std::map<uint64_t, game_object_list_element>::iterator i;

        for (i = this->game_objects.begin();
             i != this->game_objects.end();
             ++i)
            delete i->second.obj;
        /* Maybe save the game objects' locations before deleting them? */
        this->game_objects.erase(this->game_objects.begin(),
                                 this->game_objects.end());
    }

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
        catch (std::exception& e) { /* Do nothing */ }

        /* Close the actions library */
        delete this->action_lib;
    }
}

void Zone::start(void)
{
    /* Do we want to load up all the game objects here, before we start
     * up the thread pools?
     */

    this->action_pool->startup_arg = (void *)this;
    this->action_pool->start(Zone::action_pool_worker);

    this->motion_pool->startup_arg = (void *)this;
    this->motion_pool->start(Zone::motion_pool_worker);

    this->update_pool->startup_arg = (void *)this;
    this->update_pool->start(Zone::update_pool_worker);
}

void Zone::stop(void)
{
    this->action_pool->stop();
    this->motion_pool->stop();
    this->update_pool->stop();
}

void Zone::add_action_request(uint64_t from, packet *buf, size_t len)
{
    packet_list pl;

    if (this->action_pool != NULL)
    {
        memcpy(&(pl.buf), buf, len);
        pl.who = from;
        this->action_pool->push(pl);
    }
}

void Zone::execute_action(Control *con, action_request& req, size_t len)
{
    /* On entry, the control object has already determined if it has
     * the skill in question, and has scaled the power level of the
     * request to its skill level.
     */
    std::map<uint16_t, action_rec>::iterator i
        = this->actions.find(req.action_id);
    Eigen::Vector3d vec;

    vec << req.x_pos_dest, req.y_pos_dest, req.z_pos_dest;
    if (i != this->actions.end())
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

        (*(i->second.action))(con->slave,
                              req.power_level,
                              this->game_objects.find(req.dest_object_id)->second.mot,
                              vec);
    }
}
