/* about.c
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 17 Aug 2015, 10:22:04 tquirk
 *
 * Revision IX game client
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
 * This file contains the "about" dialog box for the Revision 9 client.
 *
 * Things to do
 *
 */

#include <stdio.h>
#include <stdint.h>
#include <glob.h>
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
static XtIntervalId id = 0L;
static Pixmap shown_pixmap, shape_mask;
static int how_many, which_one;

/* ARGSUSED */
void about_create_callback(Widget w,
                           XtPointer client_data,
                           XtPointer call_data)
{
    Widget aboutselbox;
    Arg args[10];
    glob_t found_files;
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

    /* How many pixmaps are there */
    if (!glob(PIXMAP_PATH "about*.xpm", GLOB_NOSORT, NULL, &found_files))
    {
        how_many = found_files.gl_pathc;
        globfree(&found_files);

        if (how_many > 0)
        {
            which_one = 0;
            id = XtAppAddTimeOut(XtWidgetToApplicationContext(aboutdraw),
                                 (unsigned long)100L,
                                 about_timeout,
                                 &which_one);
        }
    }

    XtManageChild(aboutselbox);
}

/* ARGSUSED */
static void about_destroy_callback(Widget w,
                                   XtPointer client_data,
                                   XtPointer call_data)
{
    if (how_many > 0)
    {
        XtRemoveTimeOut(id);
        id = 0L;
    }
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
    int *which = (int *)client_data;
    char fname[256];

    *which = ((*which) + 1) % how_many;
    if (gc == 0L)
    {
        XGCValues vals;

        vals.function = GXcopy;
        gc = XCreateGC(XtDisplay(aboutdraw), XtWindow(aboutdraw),
                       GCFunction, &vals);
    }

    snprintf(fname, sizeof(fname), PIXMAP_PATH "about%02d.xpm", *which);
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
                         which);
}
