/* defaults.h
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 19 Sep 2013, 19:18:27 trinity
 *
 * Revision IX game server
 * Copyright (C) 2007  Trinity Annabelle Quirk
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
 * This file contains the default configuration values for the Revision IX
 * game server.
 *
 * Changes
 *   21 Jul 2007 TAQ - Created the file.
 *   21 Aug 2007 TAQ - Added NUM_USERS.
 *   14 Oct 2007 TAQ - Added ZONE_STEPS and ZONE_DIM.
 *   19 Sep 2013 TAQ - Added HIST_FILE.
 *
 * Things to do
 *
 * $Id: defaults.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_DEFAULTS_H__
#define __INC_DEFAULTS_H__

#ifndef ACTION_LIB
#define ACTION_LIB      "libr9actions.so"
#endif
#ifndef DB_TYPE
#define DB_TYPE         "mysql"
#endif
#ifndef DB_HOST
#define DB_HOST         "localhost"
#endif
#ifndef DB_NAME
#define DB_NAME         "revision9"
#endif
#ifndef LINGER_LEN
#define LINGER_LEN      0
#endif
#ifndef LOAD_THRESH
#define LOAD_THRESH     75  /* Float initializers need to be * 100 */
#endif
#ifndef LOG_FACILITY
#define LOG_FACILITY    LOG_DAEMON
#endif
#ifndef LOG_PREFIX
#define LOG_PREFIX      "revision9"
#endif
#ifndef MIN_SUBSRV
#define MIN_SUBSRV      1
#endif
#ifndef MAX_SUBSRV
#define MAX_SUBSRV      250  /* We can't use OPEN_MAX anymore */
#endif
#ifndef NUM_BUFFERS
#define NUM_BUFFERS     64
#endif
#ifndef NUM_USERS
#define NUM_USERS       367
#endif
#ifndef NUM_THREADS
#define NUM_THREADS     8
#endif
#ifndef MIN_THREADS
#define MIN_THREADS     1
#endif
#ifndef PID_FNAME
#define PID_FNAME       "/var/run/revision9.pid"
#endif
#ifndef SERVER_ROOT
#define SERVER_ROOT     "/home/trinity/src/revision9/server"
#endif
#ifndef ZONE_STEPS
#define ZONE_STEPS      1
#endif
#ifndef ZONE_DIM
#define ZONE_DIM        1000
#endif
#ifndef HIST_FILE
#define HIST_FILE       SERVER_ROOT "/.console_history"
#endif

#endif /* __INC_DEFAULTS_H__ */
