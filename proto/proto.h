/* proto.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Aug 2015, 08:51:26 tquirk
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
 * The basic structures which define the protocol for the game system.
 *
 * Current views are that servers can send all types of packets, but will
 * only respond to a certain subset of those packets.  Login/logout requests,
 * action requests, geometry updates, and possibly server notices (reporting
 * a new neighbor, for example).
 *
 * Types of packets:
 *   ACKPKT:  This packet is just for the server to respond to some type of
 *            request that the user has made.  Currently, we've only defined
 *            login responses.
 *   LOGREQ:  This packet is a user request to login.
 *   LGTREQ:  This packet is a user request to logout.
 *   ACTREQ:  This packet is a user request to perform some action.
 *   POSUPD:  This packet is a server position update response.
 *   GEOUPD:  This packet is a general-purpose update packet for geometry
 *            changes.  It could be from a client to a server to change
 *            some of the client's avater.  It could also be from a server
 *            to a client to send new model information, so that clients
 *            can change their avatars, and that we can have any kind of
 *            objects we want, without having to release new sets of objects
 *            or some such confusion.
 *   TEXUPD:  This packet is a general-purpose update packet for texture
 *            changes.  Similar to the GEOUPD, it can go in either direction.
 *   SRVNOT:  This packet is from a server (could be to a server, could be
 *            to a client).  The basic use of this packet is to notify a
 *            client that s/he is coming to the edge of the geometry which
 *            is managed by a given server, and s/he will need to open a
 *            second connection to the neighboring server, for position
 *            updates.  This will also be used to let servers know about
 *            new neighboring servers.
 *   GEOREQ:  This packet is a user request for a given geometry.
 *   TEXREQ:  This packet is a user request for a given texture.
 *
 * Things to do
 *   - We want to do some crypto on this protocol.
 *   - Rethink the removal of in-band geometry and texture fetching.
 *
 */

#ifndef __INC_PROTO_H__
#define __INC_PROTO_H__

#if HAVE_CONFIG_H
#include <config.h>
#endif /* HAVE_CONFIG_H */

#if HAVE_STDINT_H
#include <stdint.h>
#endif /* HAVE_STDINT_H */
#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */
#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

/* Some defines for packet types. */
#define TYPE_ACKPKT 0        /* Acknowledgement packet */
#define TYPE_LOGREQ 1        /* Login request */
#define TYPE_LGTREQ 2        /* Logout request */
#define TYPE_ACTREQ 3        /* Action request */
#define TYPE_POSUPD 4        /* Position update */
#define TYPE_SRVNOT 5        /* Server notice */
#define TYPE_PNGPKT 6        /* Ping packet */

/* Access types for the login ACK's misc field.  Are there more types? */
#define ACCESS_NONE 0        /* No access allowed */
#define ACCESS_VIEW 1        /* View-only access (can't enter the zone) */
#define ACCESS_MOVE 2        /* Move-around-in access (normal access) */
#define ACCESS_MDFY 3        /* Modify access */

/* For testing what type of packet it really is, plus a few of the basic
 * elements.
 */
typedef struct basic_packet_tag
{
    uint8_t type;
    uint8_t version;
    uint64_t sequence;
} __attribute__ ((__packed__))
basic_packet;

/* We can ACK any kind of packet with this, thus the orig_request and
 * timestamp/sequence elements.
 */
typedef struct ack_packet_tag
{
    uint8_t type;
    uint8_t version;        /* protocol version number */
    uint64_t sequence;      /* timestamp / sequence number */
    uint8_t request;        /* packet type of original request */
    uint8_t misc;           /* miscellaneous data */
} __attribute__ ((__packed__))
ack_packet;

typedef struct login_request_tag
{
    uint8_t type;
    uint8_t version;        /* protocol version number */
    uint64_t sequence;      /* timestamp / sequence number */
    char username[64];
    char password[64];
} __attribute__ ((__packed__))
login_request;

typedef struct logout_request_tag
{
    uint8_t type;
    uint8_t version;        /* protocol version number */
    uint64_t sequence;      /* timestamp / sequence number */
    /* Does this really need anything else? */
} __attribute__ ((__packed__))
logout_request;

typedef struct action_request_tag
{
    uint8_t type;
    uint8_t version;        /* protocol version number */
    uint64_t sequence;      /* timestamp / sequence number */
    uint64_t object_id;
    uint16_t action_id;
    uint8_t power_level;
    uint64_t x_pos_source, y_pos_source, z_pos_source;
    uint64_t dest_object_id;
    uint64_t x_pos_dest, y_pos_dest, z_pos_dest;
} __attribute__ ((__packed__))
action_request;

/* For each position, there are 2 elements: position and orientation
 * (angular rotation).
 *
 * We will send one position update packet *per object updated*.  This
 * could be the wrong way to go, but we'll see how it works for now.
 *
 * The position fields are to the nearest centimeter, and the orient and
 * look fields are to the nearest 0.01 degree.
 */
typedef struct position_update_tag
{
    uint8_t type;
    uint8_t version;        /* protocol version number */
    uint64_t sequence;      /* timestamp / sequence number */
    uint64_t object_id;
    uint16_t frame_number;
    /* We may consider adding the sector vector back in here */
    uint64_t x_pos, y_pos, z_pos;
    int32_t x_orient, y_orient, z_orient, w_orient;
    int32_t x_look, y_look, z_look;
} __attribute__ ((__packed__))
position_update;

/* When a server wants you to start taking updates from another server */
typedef struct server_notice_tag
{
    uint8_t type;
    uint8_t version;        /* protocol version number */
    uint64_t sequence;      /* timestamp / sequence number */
    uint8_t ipproto;        /* 4 or 6 */
    union srv_tag
    {
        struct in_addr v4;
        struct in6_addr v6;
    }
    srv;
    in_port_t port;
    uint8_t direction;      /* where the neighbor is */
} __attribute__ ((__packed__))
server_notice;

/* We can cast unknown packets to this and read basic.type to determine
 * the type, then use the corresponding element of the union to process it.
 * We don't need to convert byte order on a single byte, so this trick will
 * work simply.
 */
typedef union packet_tag
{
    basic_packet     basic;
    ack_packet       ack;
    login_request    log;
    logout_request   lgt;
    action_request   act;
    position_update  pos;
    server_notice    srv;
}
packet;

/* Some macros to figure out, from some buffer pointer, what type of packet
 * we're dealing with.
 */
#define is_ack_packet(x)       (((packet *)(x))->basic.type == TYPE_ACKPKT)
#define is_login_request(x)    (((packet *)(x))->basic.type == TYPE_LOGREQ)
#define is_logout_request(x)   (((packet *)(x))->basic.type == TYPE_LGTREQ)
#define is_action_request(x)   (((packet *)(x))->basic.type == TYPE_ACTREQ)
#define is_position_update(x)  (((packet *)(x))->basic.type == TYPE_POSUPD)
#define is_server_notice(x)    (((packet *)(x))->basic.type == TYPE_SRVNOT)
#define is_ping_packet(x)      (((packet *)(x))->basic.type == TYPE_PNGPKT)

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

int hton_packet(packet *, size_t);
int ntoh_packet(packet *, size_t);
size_t packet_size(packet *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_PROTO_H__ */
