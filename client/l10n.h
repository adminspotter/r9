/* l10n.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 21 Nov 2015, 10:11:13 tquirk
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
 * This file contains the localization-related includes and defines
 * for the client program.
 *
 * Things to do
 *
 */

#ifndef __INC_L10N_H__
#define __INC_L10N_H__

#include <config.h>

#if WANT_LOCALES && HAVE_LIBINTL_H
#include <libintl.h>
#define _(x)  maketext(x)
#else
#define _(x)  x
#endif /* WANT_LOCALES && HAVE_LIBINTL_H */

#endif /* __INC_L10N_H__ */
