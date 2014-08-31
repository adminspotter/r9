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
 *   24 Aug 2014 TAQ - Removed a bunch of stuff that was made unnecessary
 *                     by the C++-ification of the geometry/texture caches.
 *   30 Aug 2014 TAQ - Removed things that no longer needed to exist.
 *   31 Aug 2014 TAQ - We need to split things into two halves, since the X
 *                     stuff will not apparently coexist in the same files
 *                     as stuff that uses Eigen.
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
