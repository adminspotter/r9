/* byteswap.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 22 Oct 2007, 23:44:31 trinity
 *
 * Revision IX game protocol
 * Copyright (C) 2007  Trinity Annabelle Quirk
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
 * These are the routines which do the network byte order conversion
 * before sending and after receiving.
 *
 * Changes
 *   14 Oct 2000 TAQ - Created the file.
 *   16 Oct 2000 TAQ - Added packet-size parameter and packet-size sanity
 *                     checking to all the functions which might need it.
 *                     All functions now return success/failure, indicating
 *                     whether or not an entire packet was found, or if the
 *                     data was corrupt or not.  We don't want seg-faults
 *                     or worse to occur just because we're reformatting
 *                     some data.
 *   17 Oct 2000 TAQ - Added logout request functions.  Fixed "open-ended"
 *                     arrays by just making them HUGE.  A stupid solution,
 *                     but it's a stupid problem.  Added up and forward
 *                     vectors to the position update structure.
 *   21 Oct 2000 TAQ - Added look vector to position updates.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   20 Jun 2006 TAQ - Added ARGSUSED tags to all, since the size param
 *                     is not always being used.  Position update
 *                     packet will only contain updates for one object,
 *                     so we can get rid of the count and size mumbo jumbo.
 *                     Fixed up the geometry update structure so that it
 *                     only has 100 triangles, and also will send a 32x32
 *                     RGBA texture.  Also added sizes to the jumptable, to
 *                     potentially speed up the send-out.
 *   06 Jul 2006 TAQ - Positions are now u_int64_t.
 *   26 Jul 2006 TAQ - Added normals to the geometry update routines.  Updated
 *                     the server notice packet processing based on the new
 *                     structure, with IPv4 and IPv6 addressing support.
 *                     Split the texture update out into its own packet type,
 *                     since things were getting pretty ugly within the
 *                     geometry update packet.
 *   09 Aug 2006 TAQ - Removed the geometry and texture request and update
 *                     packets, since we're going to do that OOB with a
 *                     webserver.
 *   16 Sep 2007 TAQ - Updated to new changes in proto.h.
 *   22 Oct 2007 TAQ - Added a entry for ping packets.  Added stubs for doing
 *                     a basic packet, which is what the ping uses.
 *
 * Things to do
 *   - Figure out if IPv6 addresses are ever converted to/from network byte
 *   ordering.  They may just be byte-for-byte, and not "ordered" at all.
 *
 * $Id: byteswap.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <netinet/in.h>

#include "proto.h"

static int hton_basic_packet(packet *, size_t);
static int ntoh_basic_packet(packet *, size_t);
static int hton_ack_packet(packet *, size_t);
static int ntoh_ack_packet(packet *, size_t);
static int hton_login_request(packet *, size_t);
static int ntoh_login_request(packet *, size_t);
static int hton_logout_request(packet *, size_t);
static int ntoh_logout_request(packet *, size_t);
static int hton_action_request(packet *, size_t);
static int ntoh_action_request(packet *, size_t);
static int hton_position_update(packet *, size_t);
static int ntoh_position_update(packet *, size_t);
static int hton_server_notice(packet *, size_t);
static int ntoh_server_notice(packet *, size_t);

/* Using a lookup table for these saves us what could be quite a large
 * amount of compares per invocation.
 */
const struct packet_handlers
{
    int (*hton)(packet *, size_t);
    int (*ntoh)(packet *, size_t);
    size_t packetsize;
}
packet_handlers[] =
{
    { hton_ack_packet,       ntoh_ack_packet,       sizeof(ack_packet)       },
    { hton_login_request,    ntoh_login_request,    sizeof(login_request)    },
    { hton_logout_request,   ntoh_logout_request,   sizeof(logout_request)   },
    { hton_action_request,   ntoh_action_request,   sizeof(action_request)   },
    { hton_position_update,  ntoh_position_update,  sizeof(position_update)  },
    { hton_server_notice,    ntoh_server_notice,    sizeof(server_notice)    },
    { hton_basic_packet,     ntoh_basic_packet,     sizeof(basic_packet)     }
};

int hton_packet(packet *p, size_t s)
{
    return (packet_handlers[p->basic.type].hton)(p, s);
}

int ntoh_packet(packet *p, size_t s)
{
    return (packet_handlers[p->basic.type].ntoh)(p, s);
}

size_t packet_size(packet *p)
{
    return (packet_handlers[p->basic.type].packetsize);
}

/* ARGSUSED */
static int hton_basic_packet(packet *ap, size_t s)
{
    /* Everything in the basic packet is a byte */
    return 1;
}

/* ARGSUSED */
static int ntoh_basic_packet(packet *ap, size_t s)
{
    /* Everything in the basic packet is a byte */
    return 1;
}

/* ARGSUSED */
static int hton_ack_packet(packet *ap, size_t s)
{
    if (s < sizeof(ack_packet))
        return 0;
    ap->ack.sequence = htonll(ap->ack.sequence);
    return 1;
}

/* ARGSUSED */
static int ntoh_ack_packet(packet *ap, size_t s)
{
    if (s < sizeof(ack_packet))
        return 0;
    ap->ack.sequence = ntohll(ap->ack.sequence);
    return 1;
}

/* ARGSUSED */
static int hton_login_request(packet *lr, size_t s)
{
    if (s < sizeof(login_request))
        return 0;
    lr->log.sequence = htonll(lr->log.sequence);
    return 1;
}

/* ARGSUSED */
static int ntoh_login_request(packet *lr, size_t s)
{
    if (s < sizeof(login_request))
        return 0;
    lr->log.sequence = ntohll(lr->log.sequence);
    return 1;
}

/* ARGSUSED */
static int hton_logout_request(packet *lr, size_t s)
{
    if (s < sizeof(logout_request))
        return 0;
    lr->lgt.sequence = htonll(lr->lgt.sequence);
     return 1;
}

/* ARGSUSED */
static int ntoh_logout_request(packet *lr, size_t s)
{
    if (s < sizeof(logout_request))
        return 0;
    lr->lgt.sequence = ntohll(lr->lgt.sequence);
    return 1;
}

/* ARGSUSED */
static int hton_action_request(packet *ar, size_t s)
{
    if (s < sizeof(action_request))
        return 0;
    ar->act.sequence = htonll(ar->act.sequence);
    ar->act.object_id = htonll(ar->act.object_id);
    ar->act.action_id = htons(ar->act.action_id);
    ar->act.x_pos_source = htonll(ar->act.x_pos_source);
    ar->act.y_pos_source = htonll(ar->act.y_pos_source);
    ar->act.z_pos_source = htonll(ar->act.z_pos_source);
    ar->act.dest_object_id = htonll(ar->act.dest_object_id);
    ar->act.x_pos_dest = htonl(ar->act.x_pos_dest);
    ar->act.y_pos_dest = htonl(ar->act.y_pos_dest);
    ar->act.z_pos_dest = htonl(ar->act.z_pos_dest);
    return 1;
}

/* ARGSUSED */
static int ntoh_action_request(packet *ar, size_t s)
{
    if (s < sizeof(action_request))
        return 0;
    ar->act.sequence = ntohll(ar->act.sequence);
    ar->act.object_id = ntohll(ar->act.object_id);
    ar->act.action_id = ntohs(ar->act.action_id);
    ar->act.x_pos_source = ntohll(ar->act.x_pos_source);
    ar->act.y_pos_source = ntohll(ar->act.y_pos_source);
    ar->act.z_pos_source = ntohll(ar->act.z_pos_source);
    ar->act.dest_object_id = ntohll(ar->act.dest_object_id);
    ar->act.x_pos_dest = ntohll(ar->act.x_pos_dest);
    ar->act.y_pos_dest = ntohll(ar->act.y_pos_dest);
    ar->act.z_pos_dest = ntohll(ar->act.z_pos_dest);
    return 1;
}

/* ARGSUSED */
static int hton_position_update(packet *pu, size_t s)
{
    if (s < sizeof(position_update))
        return 0;
    pu->pos.sequence = htonll(pu->pos.sequence);
    pu->pos.object_id = htonll(pu->pos.object_id);
    pu->pos.frame_number = htons(pu->pos.frame_number);
    pu->pos.x_pos = htonll(pu->pos.x_pos);
    pu->pos.y_pos = htonll(pu->pos.y_pos);
    pu->pos.z_pos = htonll(pu->pos.z_pos);
    pu->pos.x_orient = htonl(pu->pos.x_orient);
    pu->pos.y_orient = htonl(pu->pos.y_orient);
    pu->pos.z_orient = htonl(pu->pos.z_orient);
    pu->pos.x_look = htonl(pu->pos.x_look);
    pu->pos.y_look = htonl(pu->pos.y_look);
    pu->pos.z_look = htonl(pu->pos.z_look);
    return 1;
}

/* ARGSUSED */
static int ntoh_position_update(packet *pu, size_t s)
{
    if (s < sizeof(position_update))
        return 0;
    pu->pos.sequence = ntohll(pu->pos.sequence);
    pu->pos.object_id = ntohll(pu->pos.object_id);
    pu->pos.frame_number = ntohs(pu->pos.frame_number);
    pu->pos.x_pos = ntohll(pu->pos.x_pos);
    pu->pos.y_pos = ntohll(pu->pos.y_pos);
    pu->pos.z_pos = ntohll(pu->pos.z_pos);
    pu->pos.x_orient = ntohl(pu->pos.x_orient);
    pu->pos.y_orient = ntohl(pu->pos.y_orient);
    pu->pos.z_orient = ntohl(pu->pos.z_orient);
    pu->pos.x_look = ntohl(pu->pos.x_look);
    pu->pos.y_look = ntohl(pu->pos.y_look);
    pu->pos.z_look = ntohl(pu->pos.z_look);
    return 1;
}

/* ARGSUSED */
static int hton_server_notice(packet *sn, size_t s)
{
    if (s < sizeof(server_notice))
        return 0;
#if defined(USE_IPV4) || !defined(USE_IPV6)
    sn->srv.srv.s_addr = htonl(sn->srv.srv.s_addr);
    /* No need to convert ipv6 addresses - they're always in network order */
#endif
    sn->srv.port = htons(sn->srv.port);
    return 1;
}

/* ARGSUSED */
static int ntoh_server_notice(packet *sn, size_t s)
{
    if (s < sizeof(server_notice))
        return 0;
#if defined(USE_IPV4) || !defined(USE_IPV6)
    sn->srv.srv.s_addr = ntohl(sn->srv.srv.s_addr);
    /* No need to convert ipv6 addresses - they're always in network order */
#endif
    sn->srv.port = ntohs(sn->srv.port);
    return 1;
}
