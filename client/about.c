/* about.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 19 Jul 2006, 14:57:33 trinity
 *
 * Revision IX game client
 * Copyright (C) 2006  Trinity Annabelle Quirk
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
 * This file contains the "about" dialog box for the Revision 9 client.
 *
 * Changes
 *   19 Jul 2006 TAQ - Created the file.
 *
 * Things to do
 *
 * $Id: about.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/xpm.h>
#include <Xm/SelectioB.h>
#include <Xm/DrawingA.h>

static void about_destroy_callback(Widget, XtPointer, XtPointer);
static void about_timeout(XtPointer, XtIntervalId *);

static Widget aboutdraw;
static GC gc = 0L;
static XtIntervalId id;
static Pixmap shown_pixmap, shape_mask;

/* ARGSUSED */
void about_create_callback(Widget w,
                           XtPointer client_data,
                           XtPointer call_data)
{
    Widget aboutselbox;
    Arg args[10];
    int i = 0;

    XtSetArg(args[i], XmNwidth, 300);  ++i;
    XtSetArg(args[i], XmNheight, 200); ++i;
    aboutselbox = XmCreateSelectionDialog((Widget)client_data,
                                          "aboutselbox", args, i);
    XtUnmanageChild(XtNameToWidget(aboutselbox, "Apply"));
    XtUnmanageChild(XtNameToWidget(aboutselbox, "Cancel"));
    XtUnmanageChild(XtNameToWidget(aboutselbox, "Help"));
    XtUnmanageChild(XtNameToWidget(aboutselbox, "Items"));
    XtUnmanageChild(XtNameToWidget(aboutselbox, "ItemsListSW"));
    XtUnmanageChild(XtNameToWidget(aboutselbox, "Selection"));
    XtUnmanageChild(XtNameToWidget(aboutselbox, "Text"));
    aboutdraw = XtVaCreateManagedWidget("aboutdraw",
                                        xmDrawingAreaWidgetClass,
                                        aboutselbox,
                                        XmNwidth, 150,
                                        XmNheight, 100,
                                        NULL);
    XtAddCallback(XtNameToWidget(aboutselbox, "OK"),
                  XmNactivateCallback, about_destroy_callback,
                  (XtPointer)XtParent(aboutselbox));
    XtAppAddTimeOut(XtWidgetToApplicationContext(aboutdraw),
                    (unsigned long)100L,
                    about_timeout,
                    0L);
    XtManageChild(aboutselbox);
}

/* ARGSUSED */
static void about_destroy_callback(Widget w,
                                   XtPointer client_data,
                                   XtPointer call_data)
{
    XtRemoveTimeOut(id);
    id = 0L;
    XFreePixmap(XtDisplay(aboutdraw), shown_pixmap);
    if (shape_mask != 0L)
        XFreePixmap(XtDisplay(aboutdraw), shape_mask);
    XtDestroyWidget((Widget)client_data);
    XFreeGC(XtDisplay(aboutdraw), gc);
    gc = 0L;
}

static void about_timeout(XtPointer client_data,
                          XtIntervalId *xid)
{
    int which_one = ((int)client_data + 1) % 12;
    char fname[256];

    if (gc == 0L)
    {
        XGCValues vals;

        vals.function = GXcopy;
        gc = XCreateGC(XtDisplay(aboutdraw), XtWindow(aboutdraw),
                       GCFunction, &vals);
    }
#ifndef PIXMAP_PATH
#define PIXMAP_PATH "/home/trinity/src/revision9/client/pixmaps/"
#endif
    snprintf(fname, sizeof(fname), PIXMAP_PATH "about%02d.xpm", which_one);
    XpmReadFileToPixmap(XtDisplay(aboutdraw),
                        XtWindow(aboutdraw),
                        fname,
                        &shown_pixmap,
                        &shape_mask,
                        NULL);
    XCopyArea(XtDisplay(aboutdraw), shown_pixmap, XtWindow(aboutdraw),
              gc, 0, 0, 150, 100, 0, 0);
    id = XtAppAddTimeOut(XtWidgetToApplicationContext(aboutdraw),
                         (unsigned long)2500L,
                         about_timeout,
                         (XtPointer)which_one);
}
