/* setup.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 12:24:40 tquirk
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
 * This file contains the configuration box for the Revision9 game client.
 *
 * Changes
 *   19 Jul 2006 TAQ - Created the file.
 *   20 Jul 2006 TAQ - Hooked up the couple config settings to a structure.
 *                     Started working on the config file.
 *   26 Jul 2006 TAQ - Renamed a bunch of routines.  Instead of printing
 *                     error messages to stderr, we now pass them to
 *                     main_post_message.  Debugging is still going to
 *                     stderr though.  We're now getting the correct IP
 *                     address out of gethostbyname, and after a bit of work,
 *                     it's being transferred to the config structure
 *                     correctly as well.
 *   31 Jul 2006 TAQ - Moved all the file-handling stuff into config.c.  Also
 *                     moved the config structure there.
 *   01 Aug 2006 TAQ - Removed debugging.  We're now keeping track of the new
 *                     modified element in the config structure, and setting
 *                     it when appropriate.
 *   24 Jul 2015 TAQ - Converted to stdint types.
 *
 * Things to do
 *   - When we type in an invalid hostname/IP, set keyboard focus to the
 *   networkhost box.  It's not currently working properly.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <netdb.h>
#include <errno.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/SelectioB.h>
#include <Xm/Notebook.h>
#include <Xm/PushB.h>

#include <Xm/Label.h>
#include <Xm/ToggleB.h>
#include <Xm/PushBG.h>
#include <Xm/RowColumn.h>
#include <Xm/TextF.h>
#include <Xm/FileSB.h>

#include <iostream>
#include <string>

#include "client.h"
#include "../config.h"

static void create_video_settings_form(Widget, int);
static void create_sound_settings_form(Widget, int);
static void create_network_settings_form(Widget, int);
static void create_game_settings_form(Widget, int);

static void settings_apply_callback(Widget, XtPointer, XtPointer);
static void settings_cancel_callback(Widget, XtPointer, XtPointer);

static Widget settingbox, settingnb;
static Widget networkhost, networkport, networkuser, networkpass;

Widget create_settings_box(Widget parent)
{
    settingbox = XmCreateSelectionDialog(parent, "settingbox", NULL, 0);
    XtManageChild(XtNameToWidget(settingbox, "Apply"));
    XtUnmanageChild(XtNameToWidget(settingbox, "Items"));
    XtUnmanageChild(XtNameToWidget(settingbox, "ItemsListSW"));
    XtUnmanageChild(XtNameToWidget(settingbox, "Selection"));
    XtUnmanageChild(XtNameToWidget(settingbox, "Text"));
    settingnb = XtVaCreateManagedWidget("settingnb",
                                        xmNotebookWidgetClass,
                                        settingbox,
                                        XmNbackPagePlacement, XmBOTTOM_RIGHT,
                                        XmNorientation, XmHORIZONTAL,
                                        XmNbindingType, XmNONE,
                                        XmNmajorTabSpacing, 1,
                                        NULL);
    XtVaSetValues(XtNameToWidget(settingnb, "PageScroller"),
                  XmNarrowLayout, XmARROWS_SPLIT,
                  XmNarrowOrientation, XmARROWS_HORIZONTAL,
                  NULL);
    create_video_settings_form(settingnb, 1);
    create_sound_settings_form(settingnb, 2);
    create_network_settings_form(settingnb, 3);
    create_game_settings_form(settingnb, 4);
    XtVaSetValues(settingnb,
                  XmNfirstPageNumber, 1,
                  XmNlastPageNumber, 4,
                  XmNcurrentPageNumber, 1,
                  NULL);
    XtAddCallback(XtNameToWidget(settingbox, "OK"),
                  XmNactivateCallback, settings_apply_callback, NULL);
    XtAddCallback(XtNameToWidget(settingbox, "Apply"),
                  XmNactivateCallback, settings_apply_callback, NULL);
    XtAddCallback(XtNameToWidget(settingbox, "Cancel"),
                  XmNactivateCallback, settings_cancel_callback, NULL);

    /* we will call the cancel routine to set the initial state */
    settings_cancel_callback(settingbox, NULL, NULL);
    return settingbox;
}

static void create_video_settings_form(Widget parent, int pgnum)
{
    Widget settingvideo, videobutton;

    settingvideo = XtVaCreateManagedWidget("settingvideo",
                                           xmRowColumnWidgetClass,
                                           parent,
                                           XmNnotebookChildType, XmPAGE,
                                           XmNpageNumber, pgnum,
                                           XmNnumColumns, 2,
                                           XmNorientation, XmHORIZONTAL,
                                           XmNpacking, XmPACK_COLUMN,
                                           XmNentryVerticalAlignment,
                                           XmALIGNMENT_CENTER,
                                           XmNadjustLast, False,
                                           NULL);
    videobutton = XtVaCreateManagedWidget("videobutton",
                                          xmPushButtonWidgetClass,
                                          parent,
                                          XmNnotebookChildType, XmMAJOR_TAB,
                                          XmNpageNumber, pgnum,
                                          NULL);
}

static void create_sound_settings_form(Widget parent, int pgnum)
{
    Widget settingsound, soundbutton;

    settingsound = XtVaCreateManagedWidget("settingsound",
                                           xmRowColumnWidgetClass,
                                           parent,
                                           XmNnotebookChildType, XmPAGE,
                                           XmNpageNumber, pgnum,
                                           XmNnumColumns, 2,
                                           XmNorientation, XmHORIZONTAL,
                                           XmNpacking, XmPACK_COLUMN,
                                           XmNentryVerticalAlignment,
                                           XmALIGNMENT_CENTER,
                                           XmNadjustLast, False,
                                           NULL);
    soundbutton = XtVaCreateManagedWidget("soundbutton",
                                          xmPushButtonWidgetClass,
                                          parent,
                                          XmNnotebookChildType, XmMAJOR_TAB,
                                          XmNpageNumber, pgnum,
                                          NULL);
}

static void create_network_settings_form(Widget parent, int pgnum)
{
    Widget settingnetwork, networkbutton, networklabel;

    settingnetwork = XtVaCreateManagedWidget("settingnetwork",
                                             xmRowColumnWidgetClass,
                                             parent,
                                             XmNnotebookChildType, XmPAGE,
                                             XmNpageNumber, pgnum,
                                             XmNnumColumns, 4,
                                             XmNorientation, XmHORIZONTAL,
                                             XmNpacking, XmPACK_COLUMN,
                                             XmNentryVerticalAlignment,
                                             XmALIGNMENT_CENTER,
                                             XmNadjustLast, False,
                                             NULL);
    networkbutton = XtVaCreateManagedWidget("networkbutton",
                                            xmPushButtonWidgetClass,
                                            parent,
                                            XmNnotebookChildType, XmMAJOR_TAB,
                                            XmNpageNumber, pgnum,
                                            NULL);
    networklabel = XtVaCreateManagedWidget("networkhostlabel",
                                           xmLabelWidgetClass,
                                           settingnetwork,
                                           NULL);
    networkhost = XtVaCreateManagedWidget("networkhost",
                                          xmTextFieldWidgetClass,
                                          settingnetwork,
                                          NULL);
    networklabel = XtVaCreateManagedWidget("networkportlabel",
                                           xmLabelWidgetClass,
                                           settingnetwork,
                                           NULL);
    networkport = XtVaCreateManagedWidget("networkport",
                                          xmTextFieldWidgetClass,
                                          settingnetwork,
                                          NULL);
    networklabel = XtVaCreateManagedWidget("networkuserlabel",
                                           xmLabelWidgetClass,
                                           settingnetwork,
                                           NULL);
    networkuser = XtVaCreateManagedWidget("networkuser",
                                          xmTextFieldWidgetClass,
                                          settingnetwork,
                                          NULL);
    networklabel = XtVaCreateManagedWidget("networkpasslabel",
                                           xmLabelWidgetClass,
                                           settingnetwork,
                                           NULL);
    networkpass = XtVaCreateManagedWidget("networkpass",
                                          xmTextFieldWidgetClass,
                                          settingnetwork,
                                          NULL);
}

static void create_game_settings_form(Widget parent, int pgnum)
{
    Widget settinggame, gamebutton;

    settinggame = XtVaCreateManagedWidget("settinggame",
                                          xmRowColumnWidgetClass,
                                          parent,
                                          XmNnotebookChildType, XmPAGE,
                                          XmNpageNumber, pgnum,
                                          XmNnumColumns, 2,
                                          XmNorientation, XmHORIZONTAL,
                                          XmNpacking, XmPACK_COLUMN,
                                          XmNentryVerticalAlignment,
                                          XmALIGNMENT_CENTER,
                                          XmNadjustLast, False,
                                          NULL);
    gamebutton = XtVaCreateManagedWidget("gamebutton",
                                         xmPushButtonWidgetClass,
                                         parent,
                                         XmNnotebookChildType, XmMAJOR_TAB,
                                         XmNpageNumber, pgnum,
                                         NULL);
}


/* ARGSUSED */
void settings_show_callback(Widget w,
                            XtPointer client_data,
                            XtPointer call_data)
{
    uintptr_t page_num = (uintptr_t)client_data;

    if (!XtIsManaged(settingbox))
        XtManageChild(settingbox);
    XtVaSetValues(settingnb, XmNcurrentPageNumber, page_num, NULL);
}

/* ARGSUSED */
static void settings_apply_callback(Widget w,
                                    XtPointer client_data,
                                    XtPointer call_data)
{
    int ret;
    char *c_ptr;
    uint16_t newport;
    struct addrinfo hints, *ai;

    XtVaGetValues(networkhost, XmNvalue, &c_ptr, NULL);
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if ((ret = getaddrinfo(c_ptr, NULL, &hints, &ai)) != 0)
    {
        /* It's a valid IP address or hostname */
        config.server_addr = c_ptr;
    }
    else
    {
        /* It's invalid as a hostname or IP address; turn to the
         * network settings page and put the keyboard focus into the
         * networkhost box.  If we're trying to dismiss the box as
         * well, we should not do it.
         */
        std::cout << "Invalid hostname/IP address: " << c_ptr << ": "
                  << gai_strerror(ret) << std::endl;
        XtVaSetValues(settingnb, XmNcurrentPageNumber, 3, NULL);
        /* Set keyboard focus to networkhost - this doesn't work */
        XtSetKeyboardFocus(settingbox, XtParent(networkhost));
    }
    freeaddrinfo(ai);
    XtFree(c_ptr);

    XtVaGetValues(networkport, XmNvalue, &c_ptr, NULL);
    newport = htons((uint16_t)atoi(c_ptr));
    if (newport != config.server_port)
        config.server_port = newport;
    XtFree(c_ptr);

    XtVaGetValues(networkuser, XmNvalue, &c_ptr, NULL);
    if (config.username != c_ptr)
        config.username = c_ptr;
    XtFree(c_ptr);

    XtVaGetValues(networkpass, XmNvalue, &c_ptr, NULL);
    if (config.password != c_ptr)
        config.password = c_ptr;
    XtFree(c_ptr);
}

/* ARGSUSED */
static void settings_cancel_callback(Widget w,
                                     XtPointer client_data,
                                     XtPointer call_data)
{
    char c_str[64];

    XtVaSetValues(networkhost, XmNvalue, config.server_addr.c_str(), NULL);
    snprintf(c_str, sizeof(c_str), "%d", config.server_port);
    XtVaSetValues(networkport, XmNvalue, c_str, NULL);
    XtVaSetValues(networkuser, XmNvalue, config.username.c_str(), NULL);
    XtVaSetValues(networkpass, XmNvalue, config.password.c_str(), NULL);
}
