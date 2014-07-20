/* message.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 12 Sep 2013, 14:20:44 trinity
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
 * This file contains the message bar for the Revision 9 client.
 *
 * Changes
 *   18 Jul 2006 TAQ - Created the file.
 *   26 Jul 2006 TAQ - Renamed a bunch of stuff.  Setting the blank message
 *                     to a space, rather than an empty string, solved the
 *                     vertical resize problem.
 *
 * Things to do
 *   - Have the message post duration configurable?
 *
 * $Id: message.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>

#include "client.h"

static void main_unpost_message_timeout(XtPointer, XtIntervalId *);

static Widget msgframe, msglabel;
static XtIntervalId timer = 0L;

Widget create_message_area(Widget parent)
{
    msgframe = XtVaCreateManagedWidget("msgframe",
				       xmFrameWidgetClass,
				       parent,
				       XmNshadowType, XmSHADOW_OUT,
				       NULL);
    msglabel = XtVaCreateManagedWidget("msglabel",
				       xmLabelWidgetClass,
				       msgframe,
				       NULL);
    return msgframe;
}

/* ARGSUSED */
void main_message_post_callback(Widget w,
				XtPointer client_data,
				XtPointer call_data)
{
    main_post_message((char *)client_data);
}

void main_post_message(char *msg)
{
    XmString str = XmStringCreateLocalized(msg);

    if (timer != 0L)
    {
	XtRemoveTimeOut(timer);
	timer = 0L;
    }
    XtVaSetValues(msglabel, XmNlabelString, str, NULL);
    XmStringFree(str);
    timer = XtAppAddTimeOut(XtWidgetToApplicationContext(msglabel),
			    (unsigned long)10000L,
			    main_unpost_message_timeout,
			    (XtPointer)NULL);
}

/* ARGSUSED */
static void main_unpost_message_timeout(XtPointer client_data,
					XtIntervalId *id)
{
    XmString str = XmStringCreateLocalized(" ");

    XtVaSetValues(msglabel, XmNlabelString, str, NULL);
    XmStringFree(str);
    timer = 0L;
}
