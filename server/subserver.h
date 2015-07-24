/* subserver.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 24 Jul 2015, 13:22:06 tquirk
 *
 * Revision IX game server
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
 * This file contains the exported functions and type definitions for
 * the games server's subservers (the actual game programs).
 *
 * Changes
 *   10 May 1998 TAQ - Created the file.
 *   11 May 1998 TAQ - Added the player info structure and some comments.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   13 Sep 2007 TAQ - Added extern "C" stuff for availability in C++ code.
 *   24 Jul 2015 TAQ - Comment cleanup.
 *
 * Things to do
 *
 */

#ifndef __INC_SUBSERVER_H__
#define __INC_SUBSERVER_H__

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

void subserver_main_loop(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __INC_SUBSERVER_H__ */
