/* menu.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 25 Oct 2015, 18:03:41 tquirk
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
 * This file contains routines to create the main menu tree and handle
 * callbacks to each of the menu items.
 *
 * Things to do
 *   - Almost all the callbacks are hooked up to emptyCallback.  Write
 *   some actual callback routines.
 *
 */

#include <config.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xm/CascadeB.h>
#include <Xm/RowColumn.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>

#include <iostream>

#include "client.h"

/* Static function prototypes */
static void create_file_menu(Widget);
static void create_edit_menu(Widget);
static void create_help_menu(Widget);
static void login_callback(Widget, XtPointer, XtPointer);
static void logout_callback(Widget, XtPointer, XtPointer);
static void exit_callback(Widget, XtPointer, XtPointer);
static void empty_callback(Widget, XtPointer, XtPointer);

Widget create_menu_tree(Widget parent)
{
    Widget menubar;

    menubar = XmCreateMenuBar(parent, "menubar", NULL, 0);
    create_file_menu(menubar);
    create_edit_menu(menubar);
    create_help_menu(menubar);
    XtManageChild(menubar);
    return menubar;
}

static void create_file_menu(Widget parent)
{
    Widget filemenu, filebutton;
    Widget connectbutton, disconnectbutton, exitbutton;

    filemenu = XmCreatePulldownMenu(parent, "filemenu", NULL, 0);
    filebutton = XtVaCreateManagedWidget("filebutton",
                                         xmCascadeButtonWidgetClass,
                                         parent,
                                         XmNsubMenuId, filemenu,
                                         NULL);
    connectbutton = XtVaCreateManagedWidget("fileconnect",
                                            xmPushButtonGadgetClass,
                                            filemenu,
                                            NULL);
    disconnectbutton = XtVaCreateManagedWidget("filedisconnect",
                                               xmPushButtonGadgetClass,
                                               filemenu,
                                               NULL);
    exitbutton = XtVaCreateManagedWidget("fileexit",
                                         xmPushButtonGadgetClass,
                                         filemenu,
                                         NULL);
    XtAddCallback(connectbutton,
                  XmNactivateCallback,
                  login_callback,
                  (XtPointer)filemenu);
    XtAddCallback(disconnectbutton,
                  XmNactivateCallback,
                  logout_callback,
                  (XtPointer)filemenu);
    XtAddCallback(exitbutton,
                  XmNactivateCallback,
                  exit_callback,
                  (XtPointer)filemenu);
}

static void create_edit_menu(Widget parent)
{
    Widget editmenu, editbutton;
    Widget cutbutton, copybutton, pastebutton, separator, setupbutton;

    editmenu = XmCreatePulldownMenu(parent, "editmenu", NULL, 0);
    editbutton = XtVaCreateManagedWidget("editbutton",
                                         xmCascadeButtonWidgetClass,
                                         parent,
                                         XmNsubMenuId, editmenu,
                                         NULL);
    cutbutton = XtVaCreateManagedWidget("editcut",
                                        xmPushButtonGadgetClass,
                                        editmenu,
                                        NULL);
    XtAddCallback(cutbutton,
                  XmNactivateCallback,
                  empty_callback,
                  (XtPointer)editmenu);
    copybutton = XtVaCreateManagedWidget("editcopy",
                                         xmPushButtonGadgetClass,
                                         editmenu,
                                         NULL);
    XtAddCallback(copybutton,
                  XmNactivateCallback,
                  empty_callback,
                  (XtPointer)editmenu);
    pastebutton = XtVaCreateManagedWidget("editpaste",
                                          xmPushButtonGadgetClass,
                                          editmenu,
                                          NULL);
    XtAddCallback(pastebutton,
                  XmNactivateCallback,
                  empty_callback,
                  (XtPointer)editmenu);
    separator = XtVaCreateManagedWidget("editsep1",
                                        xmSeparatorGadgetClass,
                                        editmenu,
                                        NULL);
    setupbutton = XtVaCreateManagedWidget("editsetup",
                                          xmPushButtonGadgetClass,
                                          editmenu,
                                          NULL);
    XtAddCallback(setupbutton,
                  XmNactivateCallback,
                  settings_show_callback,
                  (XtPointer)1);
}

static void create_help_menu(Widget parent)
{
    Widget helpmenu, helpbutton;
    Widget aboutbutton;

    helpmenu = XmCreatePulldownMenu(parent, "helpmenu", NULL, 0);
    helpbutton = XtVaCreateManagedWidget("helpbutton",
                                         xmCascadeButtonWidgetClass,
                                         parent,
                                         XmNsubMenuId, helpmenu,
                                         NULL);
    XtVaSetValues(parent, XmNmenuHelpWidget, helpbutton, NULL);
    aboutbutton = XtVaCreateManagedWidget("helpabout",
                                          xmPushButtonGadgetClass,
                                          helpmenu,
                                          NULL);
    XtAddCallback(aboutbutton,
                  XmNactivateCallback,
                  about_create_callback,
                  (XtPointer)helpmenu);
}

/* ARGSUSED */
static void login_callback(Widget w,
                           XtPointer client_data,
                           XtPointer call_data)
{
    /*send_login(config.username, config.password);*/
}

/* ARGSUSED */
static void logout_callback(Widget w,
                            XtPointer client_data,
                            XtPointer call_data)
{
    /*send_logout();*/
}

/* ARGSUSED */
static void exit_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtAppSetExitFlag(XtWidgetToApplicationContext(w));
}

/* ARGSUSED */
static void empty_callback(Widget w,
                           XtPointer client_data,
                           XtPointer call_data)
{
    std::clog << _("Sorry, this is not implemented yet.") << std::endl;
}
