/* command.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 26 Jul 2006, 09:44:35 trinity
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
 * This file contains the command area creation and management routines
 * for the Revision 9 client program.
 *
 * Changes
 *   18 Jul 2006 TAQ - Created the file.
 *
 * Things to do
 *
 * $Id: command.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>

#include "client.h"

static Widget comframe, comarea;

Widget create_command_area(Widget parent)
{
    comframe = XtVaCreateManagedWidget("comframe",
				       xmFrameWidgetClass,
				       parent,
				       XmNshadowType, XmSHADOW_OUT,
				       NULL);
    comarea = XtVaCreateManagedWidget("comarea",
				      xmLabelWidgetClass,
				      comframe,
				      NULL);
    return comframe;
}
