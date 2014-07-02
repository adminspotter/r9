/* perl.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 02 Jul 2014, 08:51:20 tquirk
 *
 * Revision IX game server
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
 *   02 Jul 2014 TAQ - Substituted typedefs for symbol defines.  We're now
 *                     using the C++ library and logging objects, which make
 *                     things much more straightforward.
 *
 * Things to do
 *   - Figure out what we really want to do with this.
 *
 */

#include <EXTERN.h>
#include <perl.h>

#include "../library.h"

#ifndef PERL_LIBNAME
#define PERL_LIBNAME     "libperl.so.5"
#endif /* PERL_LIBNAME */

/* Some typedefs to make symbol-grabbing a little easier */
typedef PerlInterpreter *perl_alloc_t(void);
typedef void perl_construct_t(PerlInterpreter *);
typedef void perl_parse_t(PerlInterpreter *, void (*)(void), int,
                          char **, char **);
typedef void perl_eval_t(char *, I32)

void cleanup_perl(void);

static Library *perl_lib = NULL;
static PerlInterpreter *interp = NULL;
static perl_eval_t *perl_eval_pv = NULL;

void setup_perl(const char *fname)
{
    perl_alloc_t *alloc;
    perl_construct_t *construct, *run;
    perl_parse_t *parse;
    char *args[2];

    args[0] = "";
    args[1] = (char *)fname;

    try
    {
        perl_lib = new Library(PERL_LIBNAME);
        alloc = (perl_alloc_t *)perl_lib->symbol("perl_alloc");
        construct = (perl_construct_t *)perl_lib->symbol("perl_construct");
        parse = (perl_parse_t *)perl_lib->symbol("perl_parse");
        run = (perl_construct_t *)perl_lib->symbol("perl_run");
        perl_eval_pv = (perl_eval_t *)perl_lib->symbol("perl_eval_pv");
    }
    catch (std::string& e)
    {
        /* Didn't get either the lib or symbols, so we'll keep failing. */
        throw;
    }

    if ((interp = (*alloc)()) == NULL)
    {
        std::clog << syslogErr
                  << "couldn't create perl interpreter" << std::endl;
        cleanup_perl();
        return;
    }
    (*construct)(interp);

    /* We have an interpreter, so source the logic file. */
    (*parse)(interp, NULL, 2, args, NULL);
    /* Running perl_run actually completes all the initialization. */
    (*run)(interp);

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
    perl_construct_t *destruct, *free;

    if (perl_lib != NULL)
    {
        if (interp != NULL)
        {
            /* If any errors occur in here, there will be a memory leak,
             * but I'm not sure how to do it otherwise.
             */
            destruct = (perl_construct_t *)perl_lib->symbol("perl_destruct");
            free = (perl_construct_t *)perl_lib->symbol("perl_free");

            (*destruct)(interp);
            (*free)(interp);
        }
        interp = NULL;
        delete perl_lib;
        perl_lib = NULL;
    }
}
