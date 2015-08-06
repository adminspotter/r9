/* perl.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 16 Jul 2014, 10:11:11 trinityquirk
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
 * This file contains the class which handles an embedded perl
 * interpreter.  We can use this for quickly prototyping (or writing,
 * if performance is acceptable) control scripts, or possibly also for
 * doing fancy stuff on the server console.
 *
 * perlembed has improved dramatically since the first version of this
 * file.  There is, however, a bit of magic in the perl includes,
 * which expects the name of the interpreter to be my_perl.  There's a
 * define before the functions which sets it to our interpreter.
 * Undocumented magic, FTL.
 *
 * Things to do
 *   - Figure out what we really want to do with this.
 *
 */

#include <sstream>
#include <stdexcept>

#include "perl.h"

/* MAGIC!!! */
#define my_perl this->interp

PerlLanguage::PerlLanguage()
{
    int argc = 3;
    char *argv[] = { "", "-e", "0" };

    if ((this->interp = perl_alloc()) == NULL)
    {
        std::ostringstream s;
        s << "couldn't create perl interpreter";
        throw std::runtime_error(s.str());
    }
    perl_construct(this->interp);
    perl_parse(this->interp, NULL, argc, argv, NULL);
    PL_exit_flags |= PERL_EXIT_DESTRUCT_END;
    perl_run(this->interp);

    /* Install any routines into the interpreter that need installing */
}

PerlLanguage::~PerlLanguage(void)
{
    perl_destruct(this->interp);
    perl_free(this->interp);
    this->interp = NULL;
}

std::string PerlLanguage::execute(const std::string& cmd)
{
    SV *val = eval_pv(cmd.c_str(), TRUE);
    return std::string(SvPV_nolen(val));
}

Language *create_language(void)
{
    return new PerlLanguage();
}

void destroy_language(Language *lang)
{
    delete lang;
}
