/* client.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 31 Aug 2014, 11:06:33 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2014  Trinity Annabelle Quirk
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
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_CLIENT_H__
#define __INC_R9CLIENT_CLIENT_H__

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#include "../../proto/proto.h"

Widget create_menu_tree(Widget);
Widget create_main_view(Widget);
Widget create_command_area(Widget);
Widget create_message_area(Widget);
Widget create_settings_box(Widget);

void about_create_callback(Widget, XtPointer, XtPointer);
void settings_show_callback(Widget, XtPointer, XtPointer);

#endif /* __INC_R9CLIENT_CLIENT_H__ */
