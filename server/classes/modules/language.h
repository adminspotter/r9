/* language.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 15 Jul 2014, 11:40:34 trinityquirk
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
 * This file contains the base class for embedded language.  We'll derive
 * classes for each language.
 *
 * Changes
 *   15 Jul 2014 TAQ - Created the file.
 *
 * Things to do
 *
 */

#ifndef __INC_LANGUAGE_H__
#define __INC_LANGUAGE_H__

class Language
{
  public:
    virtual ~Language()
    {
    };
    virtual std::string execute(const std::string&) = 0;
};

typedef Language *(*create_lang_t)(void);
typedef void (*destroy_lang_t)(Language *);

#endif /* __INC_LANGUAGE_H__ */
