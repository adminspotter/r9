/* tcl.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 12 Jun 2007, 23:05:13 trinity
 *
 * Revision IX game server
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
 * This files contains the code to load the tcl library, create an
 * interpreter, source in the game logic file, and execute arbitrary
 * commands in the interpreter.
 *
 * Changes
 *   25 Jul 1998 TAQ - Created the file.
 *   05 Sep 1998 TAQ - Minor text change.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   13 Aug 2006 TAQ - Small compiler error fixes.
 *   12 Jun 2007 TAQ - A few minor tweaks.
 *
 * Things to do
 *   - Consider whether we want to retain the pointer to the
 *   interpreter here and make this a totally opaque interface, or if
 *   we want to return the pointer and allow several different tcl
 *   interpreters to be active at any one time.  We may want to have a
 *   structure which holds the pointer to the interpreter and one to
 *   the eval routine, so we don't have to find it every time, if we go
 *   the latter route.
 *
 * $Id: tcl.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <dlfcn.h>
#include <tcl.h>
#include <syslog.h>
#include <errno.h>

#ifndef TCL_LIBNAME
#define TCL_LIBNAME      "libtcl8.4.so"
#endif

/* Some defines to make all the dlsym casting a little easier */
#define TCI_CAST         Tcl_Interp *(*)(void)
#define TEF_CAST         int (*)(Tcl_Interp *, char *)
#define TE_CAST          int (*)(Tcl_Interp *, char *)
#define TDI_CAST         void (*)(Tcl_Interp *)

void cleanup_tcl(void);

void *tcl_lib = NULL;
static Tcl_Interp *interp = NULL;
static int (*tcl_eval)(Tcl_Interp *, char *) = NULL;

void setup_tcl(const char *fname)
{
    void *tcl_sym = NULL;
    int retval;

    /* Make sure there isn't ANYTHING happening.  May go away. */
    cleanup_tcl();

    /* Let's GET IT ON! */
    if (tcl_lib == NULL && (tcl_lib = dlopen(TCL_LIBNAME, RTLD_LAZY)) == NULL)
    {
	syslog(LOG_ERR, "couldn't load tcl library: %s", dlerror());
	return;
    }
    if (interp == NULL)
    {
	if ((tcl_sym = dlsym(tcl_lib, "Tcl_CreateInterp")) == NULL)
	{
	    syslog(LOG_ERR, "couldn't find Tcl_CreateInterp: %s", dlerror());
	    cleanup_tcl();
	    return;
	}
	if ((interp = (*((TCI_CAST)tcl_sym))()) == NULL)
	{
	    syslog(LOG_ERR, "couldn't create tcl interpreter");
	    cleanup_tcl();
	    return;
	}
    }

    /* We have an interpreter, so source the logic file. */
    if ((tcl_sym = dlsym(tcl_lib, "Tcl_EvalFile")) == NULL)
    {
	syslog(LOG_ERR, "couldn't find Tcl_EvalFile: %s", dlerror());
	cleanup_tcl();
	return;
    }
    if ((retval = (*((TEF_CAST)tcl_sym))(interp, (char *)fname)) != TCL_OK)
	syslog(LOG_ERR, "tcl interpreter error %d: %s",
	       retval, interp->result);

    /* Now get the eval function pointer, just so we don't have to
     * *every* time.
     */
    if ((tcl_sym = dlsym(tcl_lib, "Tcl_Eval")) == NULL)
    {
	syslog(LOG_ERR, "couldn't find Tcl_Eval: %s", dlerror());
	cleanup_tcl();
	return;
    }
    tcl_eval = (TE_CAST)tcl_sym;
}

void execute_tcl(const char *cmd)
{
    int retval;

    if (interp == NULL || tcl_eval == NULL)
	return;

    if ((retval = (*tcl_eval)(interp, (char *)cmd)) != TCL_OK)
	syslog(LOG_ERR, "tcl interpreter error %d: %s",
	       retval, interp->result);
}

void cleanup_tcl(void)
{
    void *tcl_sym = NULL;

    if (interp != NULL)
    {
	if (tcl_lib != NULL
	    && (tcl_sym = dlsym(tcl_lib, "Tcl_DeleteInterp")) != NULL)
	    (*((TDI_CAST)tcl_sym))(interp);
    }
    interp = NULL;
    if (tcl_lib != NULL)
	dlclose(tcl_lib);
    tcl_lib = NULL;
    tcl_eval = NULL;
}
