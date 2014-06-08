/* perl.c
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 13 Jun 2007, 17:58:30 trinity
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
 * This file contains the code to load the perl library, create an
 * interpreter, source in the game logic file, and execute arbitrary
 * commands in the interpreter.  This will likely be used for console
 * activity, and maybe NPC logic.
 *
 * Changes
 *   25 Jul 1998 TAQ - Created the file.
 *   20 Sep 1998 TAQ - Added a whole lot to this file.  It will likely be
 *                     a good long while before this actually does even
 *                     close to what it needs to do.  The documentation
 *                     is pretty obscure on what's really going on.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   13 Aug 2006 TAQ - Fixed the voluminous compiler errors.
 *   12 Jun 2007 TAQ - Figured out the last couple compiler errors, a few
 *                     other minor tweaks.
 *   13 Jun 2007 TAQ - Removed some unnecessary includes.
 *
 * Things to do
 *
 * $Id: perl.c 10 2013-01-25 22:13:00Z trinity $
 */

#include <dlfcn.h>
#include <EXTERN.h>
#include <perl.h>
#include <syslog.h>
#include <errno.h>

#ifndef PERL_LIBNAME
#define PERL_LIBNAME     "libperl.so.5"
#endif /* PERL_LIBNAME */

/* Some defines to make all the dlsym casting a little easier */
#define PA_CAST          PerlInterpreter *(*)(void)
#define PC_CAST          void (*)(PerlInterpreter *)
#define PP_CAST          void (*)(PerlInterpreter *, \
				  void (*)(void), \
				  int, \
				  char **, \
				  char **)
#define PE_CAST          void (*)(char *, I32)

void cleanup_perl(void);

static void *perl_lib = NULL;
static PerlInterpreter *interp = NULL;
static void (*perl_eval_pv)(char *, I32) = NULL;

void setup_perl(const char *fname)
{
    void *perl_sym = NULL;
    char *args[2];

    args[0] = "";
    args[1] = (char *)fname;

    /* Make sure there isn't ANYTHING happening.  May go away. */
    cleanup_perl();

    /* Let's GET IT ON! */
    if (perl_lib == NULL
	&& (perl_lib = dlopen(PERL_LIBNAME, RTLD_LAZY)) == NULL)
    {
	syslog(LOG_ERR, "couldn't load perl library: %s", dlerror());
	return;
    }
    if (interp == NULL)
    {
	if ((perl_sym = dlsym(perl_lib, "perl_alloc")) == NULL)
	{
	    syslog(LOG_ERR, "couldn't find perl_alloc: %s", dlerror());
	    cleanup_perl();
	    return;
	}
	if ((interp = (*((PA_CAST)perl_sym))()) == NULL)
	{
	    syslog(LOG_ERR, "couldn't create perl interpreter");
	    cleanup_perl();
	    return;
	}
	if ((perl_sym = dlsym(perl_lib, "perl_construct")) == NULL)
	{
	    syslog(LOG_ERR, "couldn't find perl_construct: %s", dlerror());
	    cleanup_perl();
	    return;
	}
	(*((PC_CAST)perl_sym))(interp);
    }

    /* We have an interpreter, so source the logic file. */
    if ((perl_sym = dlsym(perl_lib, "perl_parse")) == NULL)
    {
	syslog(LOG_ERR, "couldn't find perl_parse: %s", dlerror());
	cleanup_perl();
	return;
    }
    (*((PP_CAST)perl_sym))(interp, NULL, 2, args, NULL);
    /* Running perl_run actually completes all the initialization. */
    if ((perl_sym = dlsym(perl_lib, "perl_run")) == NULL)
    {
	syslog(LOG_ERR, "couldn't find perl_run: %s", dlerror());
	cleanup_perl();
	return;
    }
    (*((PC_CAST)perl_sym))(interp);

    if ((perl_sym = dlsym(perl_lib, "perl_eval_pv")) == NULL)
    {
	syslog(LOG_ERR, "couldn't find perl_eval_pv: %s", dlerror());
	cleanup_perl();
	return;
    }
    perl_eval_pv = (PE_CAST)perl_sym;

    /* Install any routines into the interpreter that need installing */
}

void execute_perl(const char *cmd)
{
    if (interp == NULL || perl_eval_pv == NULL)
	return;

    /* We want the interpreter to vomit if there's an error. */
    (*perl_eval_pv)((char *)cmd, TRUE);
}

void cleanup_perl(void)
{
    void *perl_sym = NULL;

    if (interp != NULL)
    {
	/* If any errors occur in here, there will be a memory leak,
	 * but I'm not sure how to do it otherwise.
	 */
	if ((perl_sym = dlsym(perl_lib, "perl_destruct")) == NULL)
	{
	    syslog(LOG_ERR, "couldn't find perl_destruct: %s", dlerror());
	    return;
	}
	(*((PC_CAST)perl_sym))(interp);
	if ((perl_sym = dlsym(perl_lib, "perl_free")) == NULL)
	{
	    syslog(LOG_ERR, "couldn't find perl_free: %s", dlerror());
	    return;
	}
	(*((PC_CAST)perl_sym))(interp);
    }
    interp = NULL;
    if (perl_lib != NULL)
	dlclose(perl_lib);
    perl_lib = NULL;
}
