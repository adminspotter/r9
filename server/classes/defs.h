/* defs.h                                                  -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 13:06:45 tquirk
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
 * Some basic definitions and typedefs for the server classes.
 *
 * Things to do
 *   - Flesh out the attribute and nature as needed.
 *
 */

#ifndef __INC_DEFS_H__
#define __INC_DEFS_H__

#include <cstdint>
#include <deque>
#include <map>
#include <Eigen/Core>
#include <Eigen/Geometry>

#include <stdlib.h>

#include "../../proto/proto.h"

/* Eliminate the multiple-include problems */
class GameObject;
class Motion;
class listen_socket;

typedef struct sequence_tag
{
    int frame_number, duration;
}
sequence_element;

typedef struct game_object_tag
{
    GameObject *obj;
    Motion *mot;
    Eigen::Vector3d position, look;
    Eigen::Quaterniond orientation;
}
game_object_list_element;

typedef struct packet_list_tag
{
    packet buf;
    uint64_t who;
}
packet_list;

typedef struct access_list_tag
{
    packet buf;
    listen_socket *parent;
    union
    {
        struct
        {
            union
            {
                struct sockaddr_storage dgram;
                struct
                {
                    int sub, sock;
                }
                stream;
            }
            who;
        }
        login;
        struct
        {
            uint64_t who;
        }
        logout;
    }
    what;
}
access_list;

class action_rec
{
  public:
    char *name;
    void (*action)(Motion *, int, Motion *, Eigen::Vector3d &);
    uint16_t def;                /* The "default" skill to use */
    int lower, upper;             /* The bounds for skill levels */
    bool valid;                   /* Is this action valid on this server? */

    inline action_rec()
        {
            this->name = NULL;
            this->action = NULL;
            this->def = 0;
            this->lower = 0;
            this->upper = 0;
            this->valid = false;
        };
    inline ~action_rec()
        {
            if (this->name != NULL)
                free(this->name);
        };
};
typedef int attribute;
typedef int nature;
typedef struct action_level_tag
{
    uint16_t index, level, improvement;
    time_t last_level;
}
action_level;

/* Typedefs we can use for casting things to come out of a dynamically
 * loaded library.
 */
typedef int zone_control_t(int); /* placeholder */
typedef void action_reg_t(std::map<uint16_t, action_rec>&);
typedef void action_unreg_t(std::map<uint16_t, action_rec>&);

#endif /* __INC_DEFS_H__ */
