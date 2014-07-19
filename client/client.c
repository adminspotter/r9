/* client.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 12 Sep 2013, 14:13:54 trinity
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
 * This file contains the main window and session management for a Revision9
 * client.
 *
 * Changes
 *   26 Sep 1998 TAQ - Created the file.
 *   30 Jan 1999 TAQ - Added some session management.  This stuff is crazy.
 *   22 May 1999 TAQ - Modified some comments.
 *   18 Jul 2006 TAQ - Added GPL notice.  Made this actually do something.
 *   26 Jul 2006 TAQ - Renamed a bunch of routines.  Removed a bunch of stuff
 *                     we were saving for no real reason.
 *   29 Jul 2006 TAQ - Made the application context non-static for the comm
 *                     stuff.
 *   30 Jul 2006 TAQ - Made the application context static again; the receiving
 *                     stuff just isn't going to work like we thought.
 *   01 Aug 2006 TAQ - Removed the debug.h include, since we're getting rid
 *                     of it.
 *
 * Things to do
 *
 * $Id: client.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <stdlib.h>
#include <limits.h>
#include <unistd.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xm/MainW.h>
#include <X11/Xmu/Editres.h>

#include "client.h"

/* File-static routines */
static void save_callback(Widget, XtPointer, XtPointer);
static void save_complete_callback(Widget, XtPointer, XtPointer);
static void die_callback(Widget, XtPointer, XtPointer);
static void interact_callback(Widget, XtPointer, XtPointer);
static Boolean save_state(void);

/* File-global variables */
static XtAppContext context;
static Widget toplevel, mainwin;
static String restart_command[6], discard_command[4];

extern int thread_exit_flag;

int main(int argc, char **argv)
{
    toplevel = XtVaOpenApplication(&context, "Revision9",
                                   NULL, 0,
                                   &argc, argv,
                                   NULL, sessionShellWidgetClass,
                                   XtNwidth, 800,
                                   XtNheight, 600,
                                   NULL);
    /* Set up some stuff for session management */
    restart_command[0] = argv[0];
    XtAddCallback(toplevel, XtNsaveCallback, save_callback, NULL);
    XtAddCallback(toplevel, XtNcancelCallback, save_complete_callback, NULL);
    XtAddCallback(toplevel,
                  XtNsaveCompleteCallback, save_complete_callback, NULL);
    XtAddCallback(toplevel, XtNdieCallback, die_callback, NULL);

    /* Load up the config file, or create a new one if there isn't one */
    load_settings();
    setup_geometry();
    setup_texture();

    /* Create the actual window and subparts thereof */
    mainwin = XmCreateMainWindow(toplevel, "mainwin", NULL, 0);
    XtManageChild(mainwin);
    XtVaSetValues(mainwin,
                  XmNcommandWindow, create_command_area(mainwin),
                  XmNcommandWindowLocation, XmCOMMAND_ABOVE_WORKSPACE,
                  XmNmenuBar, create_menu_tree(mainwin),
                  XmNmessageWindow, create_message_area(mainwin),
                  XmNshowSeparator, True,
                  XmNworkWindow, create_main_view(mainwin),
                  NULL);
    create_settings_box(mainwin);

    /* We want editres to work with this application */
    XtAddEventHandler(toplevel,
                      (EventMask)0,
                      True,
                      _XEditResCheckMessages,
                      NULL);
    /* Show the main window and enter the event loop */
    XtRealizeWidget(toplevel);
    main_post_message("Welcome to Revision 9");
    XtAppMainLoop(context);

    cleanup_texture();
    cleanup_geometry();
    save_settings();

    return 0;
}

/* The callback for when we get a message to save ourselves. */
/* ARGSUSED */
static void save_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtCheckpointToken token = (XtCheckpointToken)call_data;

    if (token->shutdown || token->interact_style != SmInteractStyleNone)
        XtSetSensitive(toplevel, False);
    if (token->interact_style == SmInteractStyleNone)
        token->save_success = save_state();
    else
    {
        if (token->interact_style == SmInteractStyleAny)
            token->interact_dialog_type = SmDialogNormal;
        else /* token->interact_style == SmInteractErrors */
            token->interact_dialog_type = SmDialogError;
        XtAddCallback(toplevel, XtNinteractCallback, interact_callback, NULL);
    }
}

/* The callback for when a save is complete (or cancelled). */
/* ARGSUSED */
static void save_complete_callback(Widget w,
                                   XtPointer client_data,
                                   XtPointer call_data)
{
    XtSetSensitive(toplevel, True);
}

/* The callback when we get a message to die. */
/* ARGSUSED */
static void die_callback(Widget w, XtPointer client_data, XtPointer call_data)
{
    XtAppSetExitFlag(XtWidgetToApplicationContext(w));
}

/* The callback when we are allowed to interact with the user during a save. */
/* ARGSUSED */
static void interact_callback(Widget w,
                              XtPointer client_data,
                              XtPointer call_data)
{
}

/* We are not going to include a resign or shutdown command, since
 * we don't have anything major to do for them.  The default is probably
 * to do nothing, which is what we want.
 */
static Boolean save_state(void)
{
    String cwd;
    char session_fname[PATH_MAX];

    /* Get the session ID, save it, and create a filename from it. */
    XtVaGetValues(toplevel, XtNsessionID, &restart_command[2], NULL);
    snprintf(session_fname, sizeof(session_fname),
             "%s/.revision9/session.%s", getenv("HOME"), restart_command[2]);

    /* The command to restart our program with a given state. */
    restart_command[1] = "-xtsessionID";
    restart_command[3] = "-sessionFile";
    restart_command[4] = session_fname;
    restart_command[5] = NULL;

    /* The command to remove the state file. */
    discard_command[0] = "rm";
    discard_command[1] = "-f";
    discard_command[2] = session_fname;
    discard_command[3] = NULL;

    /* Now write some data to the state file. */

    /* Find out what the working directory is. */
    cwd = getcwd(NULL, PATH_MAX);

    /* Set the properties in the SessionShell widget. */
    XtVaSetValues(toplevel,
                  XtNcurrentDirectory, cwd,
                  XtNrestartCommand, restart_command,
                  XtNdiscardCommand, discard_command,
                  NULL);
    free(cwd);
    return True;
}
