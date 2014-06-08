/* config.h
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 19 Sep 2013, 20:13:52 trinity
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
 * This file contains function prototypes and global variable
 * declarations for the configuration subsystem.
 *
 * Changes
 *   11 Apr 1998 TAQ - Created the file.
 *   17 Apr 1998 TAQ - Added the prototype for cleanup_sockets into this
 *                     file.
 *   10 May 1998 TAQ - Added CVS ID string.
 *   21 Oct 1998 TAQ - Added limits.h include TOTAL_MAX define.
 *   14 Feb 1999 TAQ - Added config_data structure.
 *   23 Feb 1999 TAQ - Added database configuration fields.
 *   16 Apr 2000 TAQ - Reset CVS ID string.
 *   29 Jun 2000 TAQ - Added (min|max)_octree_depth elements.
 *   12 Jul 2000 TAQ - Added max_octree_polys element.  Added MIN_DEPTH,
 *                     MAX_DEPTH, and MAX_POLYS constants.
 *   13 Jul 2000 TAQ - Reformatted configuration structure.  Added
 *                     log_prefix option.
 *   26 Jul 2000 TAQ - Added lots of default defines.
 *   24 Aug 2000 TAQ - Added LOG_PREFIX define.
 *   29 Sep 2000 TAQ - Added [xyz]_steps, [xyz]_dim, and zone_fname elements,
 *                     and STEPS and DIMENSION default values.  Also added
 *                     (control|action)_(lib_dir|register_func) and REG_FUNC
 *                     and (AR|ZC)_LIBDIR.
 *   02 Oct 2000 TAQ - Added lib_suffix element and LIB_SUFFIX define.
 *   28 Oct 2000 TAQ - Added zone lib and un/registration stuff.
 *   04 Nov 2000 TAQ - Added console stuff.
 *   29 Jan 2002 TAQ - Added prototypes for console-interface routines.  Added
 *                     LogFacility.
 *   14 Feb 2002 TAQ - Straightened out the library confusion that was causing
 *                     seg faults.
 *   30 Jun 2002 TAQ - Added frame-sequence and polygon limits.
 *   15 May 2006 TAQ - Added the GPL notice.
 *   14 Jun 2006 TAQ - Added threads config options.
 *   06 Jul 2006 TAQ - Changed the dimensions to u_int64_t, and the default
 *                     dimension to 100 (was 100.0).  Reduced minimum thread
 *                     count to 1.
 *   30 Jul 2006 TAQ - Renamed some of the config elements.  Removed the
 *                     console-related stuff for now.
 *   13 Aug 1006 TAQ - Removed unused defaults and config elements.  Added
 *                     string db_type config element and default.
 *   17 Aug 2006 TAQ - Added action_lib to the config struct.
 *   21 Jul 2007 TAQ - Moved default values to defaults.h.  Removed OPEN_MAX
 *                     check, since it is settable per-process, and should
 *                     be grabbed by the process by getrlimit.
 *   21 Aug 2007 TAQ - Added num_users for count of hash buckets in user list.
 *   23 Aug 2007 TAQ - Added ports typedef, for doing open-ended numbers of
 *                     listening sockets.
 *   23 Sep 2007 TAQ - Removed num_users, num_buffers, and use_datagram.
 *   14 Oct 2007 TAQ - Added [xyz]_dim and [xyz]_steps elements.
 *   20 Oct 2007 TAQ - Moved dim and step elements into the location typedef.
 *                     Added a spawn point typedef.  Removed syslog.h include,
 *                     since it's not actually used in this file.
 *   19 Sep 2013 TAQ - Added console items back in.  It's console testing time.
 *
 * Things to do
 *
 * $Id: config.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_CONFIG_H__
#define __INC_CONFIG_H__

#include <limits.h>
#include <sys/types.h>

typedef struct ports_struct
{
    int num_ports, *port_nums;
}
ports;

typedef struct location_struct
{
    u_int64_t dim[3];
    u_int16_t steps[3];
}
location;

typedef struct config_struct
{
    int argc;
    char **argv;
    ports stream, dgram;
    int use_keepalive, use_linger, use_nonblock, use_reuse;
    int log_facility;
    char *server_root, *log_prefix, *pid_fname;
    float load_threshold;
    int min_subservers, max_subservers;
    int access_threads, action_threads, geom_threads, motion_threads;
    int send_threads, update_threads;
    location size, spawn;
    char *db_type, *db_host, *db_user, *db_pass, *db_name;
    char *action_lib;

    char *console_fname;
    int console_inet;
    char *console_hist;
    int history_len;
}
config_data;

extern config_data config;

void setup_configuration(int, char **);
void cleanup_configuration(void);

#endif /*__INC_CONFIG_H__*/
