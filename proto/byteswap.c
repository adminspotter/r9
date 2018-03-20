/* byteswap.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 20 Mar 2018, 08:33:37 tquirk
 *
 * Revision IX game protocol
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
 * These are the routines which do the network byte order conversion
 * before sending and after receiving.
 *
 * Things to do
 *
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#if HAVE_ENDIAN_H
#include <endian.h>
#endif /* HAVE_ENDIAN_H */
#if HAVE_BYTESWAP_H
#include <byteswap.h>
#endif /* HAVE_BYTESWAP_H */
#if HAVE_ARCHITECTURE_BYTE_ORDER_H
#include <architecture/byte_order.h>
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htonll(x) __builtin_bswap64(x)
#define ntohll(x) __builtin_bswap64(x)
#endif /* __BYTE_ORDER__ */
#endif /* HAVE_ARCHITECTURE_BYTE_ORDER_H */

#if !(HAVE_HTONLL) && !defined(htonll)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define htonll(x) (x)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define htonll(x) __bswap_64(x)
#endif /* __BYTE_ORDER__ */
#endif /* HAVE_HTONLL */
#if !(HAVE_NTOHLL) && !defined(ntohll)
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define ntohll(x) (x)
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define ntohll(x) __bswap_64(x)
#endif /* __BYTE_ORDER__ */
#endif /* HAVE_NTOHLL */

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
    if (p->basic.type > TYPE_PNGPKT)
        return 0;
    return (packet_handlers[p->basic.type].hton)(p, s);
}

int ntoh_packet(packet *p, size_t s)
{
    if (p->basic.type > TYPE_PNGPKT)
        return 0;
    return (packet_handlers[p->basic.type].ntoh)(p, s);
}

size_t packet_size(packet *p)
{
    if (p->basic.type > TYPE_PNGPKT)
        return 0;
    return (packet_handlers[p->basic.type].packetsize);
}

/* ARGSUSED */
static int hton_basic_packet(packet *ap, size_t s)
{
    if (s < sizeof(basic_packet))
        return 0;
    ap->basic.sequence = htonll(ap->basic.sequence);
    return 1;
}

/* ARGSUSED */
static int ntoh_basic_packet(packet *ap, size_t s)
{
    if (s < sizeof(basic_packet))
        return 0;
    ap->basic.sequence = ntohll(ap->basic.sequence);
    return 1;
}

/* ARGSUSED */
static int hton_ack_packet(packet *ap, size_t s)
{
    if (s < sizeof(ack_packet))
        return 0;
    ap->ack.sequence = htonll(ap->ack.sequence);
    ap->ack.misc[0] = htonll(ap->ack.misc[0]);
    ap->ack.misc[1] = htonll(ap->ack.misc[1]);
    ap->ack.misc[2] = htonll(ap->ack.misc[2]);
    ap->ack.misc[3] = htonll(ap->ack.misc[3]);
    return 1;
}

/* ARGSUSED */
static int ntoh_ack_packet(packet *ap, size_t s)
{
    if (s < sizeof(ack_packet))
        return 0;
    ap->ack.sequence = ntohll(ap->ack.sequence);
    ap->ack.misc[0] = ntohll(ap->ack.misc[0]);
    ap->ack.misc[1] = ntohll(ap->ack.misc[1]);
    ap->ack.misc[2] = ntohll(ap->ack.misc[2]);
    ap->ack.misc[3] = ntohll(ap->ack.misc[3]);
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
    pu->pos.w_orient = htonl(pu->pos.z_orient);
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
    pu->pos.w_orient = ntohl(pu->pos.z_orient);
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
    sn->srv.port = htons(sn->srv.port);
    return 1;
}

/* ARGSUSED */
static int ntoh_server_notice(packet *sn, size_t s)
{
    if (s < sizeof(server_notice))
        return 0;
    sn->srv.port = ntohs(sn->srv.port);
    return 1;
}
