/* client.h
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 26 Nov 2017, 08:02:08 tquirk
 *
 * Revision IX game client
 * Copyright (C) 2016  Trinity Annabelle Quirk
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
 * This file contains the publically-available function prototypes
 * and structures for the main client program.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_H__
#define __INC_R9CLIENT_H__

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <string>

#include "cuddly-gl/ui.h"

void setup_comm(struct addrinfo *, const char *, const char *, const char *);
void cleanup_comm(void);

void create_login_dialog(ui::context *);

#endif /* __INC_R9CLIENT_H__ */
