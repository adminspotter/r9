/* zone_interface.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Jul 2014, 18:27:25 tquirk
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
 * This file contains the high-level creation/deletion interface to the
 * Zone class, used by the C server routines.  This stuff used to live
 * in zone.cc, but that was an inappropriate place for it.
 *
 * Changes
 *   11 Aug 2006 TAQ - Created the file.
 *   13 Aug 2006 TAQ - We removed the unused config options, and had to change
 *                     the zone creation to not refer to the now-missing opts.
 *   16 Aug 2006 TAQ - Set the new clean_on_pop member of the access pool so
 *                     that passwords will be zeroed once they're out of the
 *                     queue.  The send queue now takes packet_list objects,
 *                     instead of just packets.
 *   17 Aug 2006 TAQ - Added action library loading, registration,
 *                     unregistration, and unloading.  setup_zone now returns
 *                     an int.
 *   22 Jun 2007 TAQ - Added some debugging.
 *   24 Aug 2007 TAQ - Commented calls to create access and sending thread
 *                     pools, since they're going to be done elsewhere.
 *   05 Sep 2007 TAQ - Reworked things so the access pool is not part of
 *                     the zone.  Removed the sending pools, since they now
 *                     reside in the sockets.
 *   10 Sep 2007 TAQ - The access pool now is of type access_list.
 *   23 Sep 2007 TAQ - The thread_pool no longer takes the buffer count
 *                     in the constructor.
 *   30 Sep 2007 TAQ - Moved to using the database library as a loaded
 *                     object, rather than a linked-in thing.  Removed some
 *                     excessive debugging info.  Rearranged error handling
 *                     in setup_zone.  Added a little insanity protection
 *                     in cleanup_zone.
 *   07 Oct 2007 TAQ - Moved the action, motion, and update pool handling
 *                     into the zone, where it belongs.  Moved definitions
 *                     of database funcptrs into the access pool; they're now
 *                     just extern declarations in here.
 *   11 Oct 2007 TAQ - Updated database routine pointers to be of proper type.
 *   14 Oct 2007 TAQ - Added config.[xyz]_dim and config.[xyz]_steps into
 *                     the zone creation call.  Fixed a memory leak in
 *                     cleanup_zone.  Added error checking and fixed a
 *                     potential memory leak location in setup_zone.
 *   25 Oct 2007 TAQ - Some of the DB functions changed prototype.
 *   29 Nov 2009 TAQ - Added get_server_objects references.
 *   20 Jun 2014 TAQ - New library stuff, new database stuff.
 *   21 Jun 2014 TAQ - Converted syslog to use the logging stream.
 *   23 Jun 2014 TAQ - Fixed the database library name.
 *   01 Jul 2014 TAQ - The access pool moved into the listen_socket.  The
 *                     active_users set didn't seem to be used at all, so
 *                     it's gone too.
 *
 * Things to do
 *   - See if we can get rid of this file.  Maybe this ought to be main
 *     stuff?
 *
 */

#include "zone.h"
#include "zone_interface.h"

#include "../config.h"
#include "../log.h"

/* The single zone pointer. */
Zone *zone = NULL;
Library *db_lib = NULL;
DB *database = NULL;

int setup_zone(void)
{
    int ret = 0;
    create_db_t *create_db;

    std::clog << "in zone setup" << std::endl;
    try
    {
        zone = new Zone(config.size.dim[0], config.size.dim[1],
                        config.size.dim[2], config.size.steps[0],
                        config.size.steps[1], config.size.steps[2]);
    }
    catch (int errval)
    {
        ret = errval;
        goto BAILOUT1;
    }

    /* Load up the database lib before we start the access thread pool */
    try { db_lib = new Library("libr9_" + config.db_type + ".so"); }
    catch (std::string& s) { goto BAILOUT1; }

    try { create_db = (create_db_t *)db_lib->symbol("create_db"); }
    catch (std::string& s) { goto BAILOUT2; }

    database = (*create_db)(config.db_host, config.db_user,
                            config.db_pass, config.db_name);
    database->get_server_skills(zone->actions);
    database->get_server_objects(zone->game_objects);

    try
    {
        zone->start();
    }
    catch (int errval)
    {
        std::clog << syslogErr << "couldn't start zone thread pools: "
                  << strerror(errval) << " (" << errval << ")" << std::endl;
        ret = errval;
        goto BAILOUT2;
    }

    std::clog << "zone setup done" << std::endl;
    return ret;

  BAILOUT2:
    delete db_lib;
  BAILOUT1:
    delete zone;
    zone = NULL;
    return ret;
}

void cleanup_zone(void)
{
    std::clog << "in zone cleanup" << std::endl;
    if (zone != NULL)
    {
        std::clog << "deleting zone" << std::endl;
        delete zone;
        zone = NULL;
    }
    if (db_lib != NULL)
    {
        std::clog << "closing database library" << std::endl;
        destroy_db_t *destroy_db = (destroy_db_t *)db_lib->symbol("destroy_db");
        (*destroy_db)(database);
        delete db_lib;
        db_lib = NULL;
    }
    std::clog << "zone cleanup done" << std::endl;
}
