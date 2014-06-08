/* signals.h
 *   by Trinity Quirk <trinity@io.com>
 *   last updated 15 May 2006, 11:15:32 trinity
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
 * This file contains global function prototypes and variable
 * declarations for the signal handling functions.
 *
 * Changes
 *   11 Apr 1998 TAQ - Created the file.
 *   10 May 1998 TAQ - Added CVS ID string.
 *   16 Apr 2000 TAQ - Reset the CVS ID string.
 *   15 May 2006 TAQ - Added the GPL notice.
 *
 * Things to do
 *
 * $Id: signals.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_SIGNALS_H__
#define __INC_SIGNALS_H__

void setup_signals(void);
void cleanup_signals(void);

#endif /*__INC_SIGNALS_H__*/
