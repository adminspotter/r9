/* client.h
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 12 Sep 2013, 14:22:58 trinity
 *
 * Revision IX game client
 * Copyright (C) 2004  Trinity Annabelle Quirk
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
 * This file contains the publically-available function prototypes
 * and structures for the client program.
 *
 * Changes
 *   26 Sep 1998 TAQ - Created the file.
 *   18 Jul 2006 TAQ - Changed it to client.h, and included all the routines
 *                     that need to be available everywhere.  Added GPL notice.
 *   19 Jul 2006 TAQ - Added socket creation prototype.  Added about creation
 *                     callback.
 *   30 Jul 2006 TAQ - Comm prototypes changed a bit.
 *   03 Aug 2006 TAQ - Reworked the geometry and texture management protos.
 *                     Added some default value defines.
 *   04 Aug 2006 TAQ - Added draw_texture prototype.  A couple other geo/tex
 *                     prototypes changed.
 *   09 Aug 2006 TAQ - The geometry and texture update routines have changed.
 *   10 Aug 2006 TAQ - Added draw_geometry prototype.
 *   12 Sep 2013 TAQ - Added linux/limits.h include for PATH_MAX.
 *
 * Things to do
 *
 * $Id: client.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_CLIENT_H__
#define __INC_CLIENT_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/limits.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <GL/gl.h>

#include "proto.h"

/* Some default values */
#ifndef GEOMETRY_PREFIX
#define GEOMETRY_PREFIX   "/usr/share/revision9/geometry"
#endif /* GEOMETRY_PREFIX */
#ifndef TEXTURE_PREFIX
#define TEXTURE_PREFIX    "/usr/share/revision9/texture"
#endif /* TEXTURE_PREFIX */
#ifndef TX_RING_ELEMENTS
#define TX_RING_ELEMENTS  16
#endif /* TX_RING_ELEMENTS */

extern struct config_struct_tag
{
    int modified;

    /* These network things are stored in network byte order */
    struct in_addr server_addr;
    u_int16_t server_port;

    char *username, *password;
}
config;

Widget create_menu_tree(Widget);
Widget create_main_view(Widget);
Widget create_command_area(Widget);
Widget create_message_area(Widget);
Widget create_settings_box(Widget);

void create_socket(struct in_addr *, u_int16_t);
void start_comm(void);
void cleanup_comm(void);
void send_login(char *, char *, u_int64_t, u_int64_t);
void send_action_request(u_int16_t, u_int8_t);
void send_logout(u_int64_t, u_int64_t);

void main_message_post_callback(Widget, XtPointer, XtPointer);
void main_post_message(char *);

void about_create_callback(Widget, XtPointer, XtPointer);
void settings_show_callback(Widget, XtPointer, XtPointer);

void load_settings(void);
void save_settings(void);
void read_config_file(const char *);
void write_config_file(const char *);

void setup_geometry(void);
void cleanup_geometry(void);
void load_geometry(u_int64_t, u_int16_t);
void update_geometry(u_int64_t, u_int16_t);
void draw_geometry(u_int64_t, u_int16_t);

void setup_texture(void);
void cleanup_texture(void);
void load_texture(u_int64_t);
void update_texture(u_int64_t);
void draw_texture(u_int64_t);

void move_object(u_int64_t, u_int16_t, double, double, double, double, double, double);

#endif /* __INC_CLIENT_H__ */
