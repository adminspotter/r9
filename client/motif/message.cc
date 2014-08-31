/* message.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 30 Aug 2014, 17:19:32 tquirk
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
 * This file contains the message bar for the Revision 9 client.
 *
 * Changes
 *   18 Jul 2006 TAQ - Created the file.
 *   26 Jul 2006 TAQ - Renamed a bunch of stuff.  Setting the blank message
 *                     to a space, rather than an empty string, solved the
 *                     vertical resize problem.
 *   24 Aug 2014 TAQ - We're now going to have the message box function as
 *                     a sink for std::clog messages, via the MessageLog
 *                     streambuf.
 *
 * Things to do
 *   - Have the message post duration configurable?
 *
 */

#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>

#include <iostream>

#include "msglog.h"

static void main_unpost_message_timeout(XtPointer, XtIntervalId *);

static Widget msglabel;
static XtIntervalId timer = 0L;

Widget create_message_area(Widget parent)
{
    Widget msgframe;

    msgframe = XtVaCreateManagedWidget("msgframe",
                                       xmFrameWidgetClass,
                                       parent,
                                       XmNshadowType, XmSHADOW_OUT,
                                       NULL);
    msglabel = XtVaCreateManagedWidget("msglabel",
                                       xmLabelWidgetClass,
                                       msgframe,
                                       NULL);
    std::clog.rdbuf(new MessageLog());
    return msgframe;
}

void main_post_message(const std::string& msg)
{
    XmString str = XmStringCreateLocalized(const_cast<char *>(msg.c_str()));

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
