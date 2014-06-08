/* zone_interface.cc
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 29 Nov 2009, 16:22:07 trinity
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
 *
 * Things to do
 *
 * $Id: zone_interface.cc 10 2013-01-25 22:13:00Z trinity $
 */

#include <dlfcn.h>
#include "zone.h"
#include "zone_interface.h"

#include "../config.h"

/* The single zone pointer. */
Zone *zone = NULL;
void *db_lib = NULL, *action_lib = NULL;
ThreadPool<access_list> *access_pool = NULL;
pthread_mutex_t active_users_mutex = PTHREAD_MUTEX_INITIALIZER;
std::set<u_int64_t> *active_users = NULL;

/* The database library symbols */
extern u_int64_t (*check_authentication)(const char *, const char *);
extern int (*check_authorization)(u_int64_t, u_int64_t);
extern int (*get_server_skills)(std::map<u_int16_t, action_rec>&);
extern int (*get_server_objects)(std::map<u_int64_t,
				 game_object_list_element>&);
extern int (*get_player_server_skills)(u_int64_t,
				       u_int64_t,
				       std::map<u_int16_t, action_level>&);
extern int (*open_new_login)(u_int64_t, u_int64_t);
extern int (*check_open_login)(u_int64_t, u_int64_t);
extern int (*close_open_login)(u_int64_t, u_int64_t);

/* Static funcs */
static int load_db_lib(void);
static int load_action_lib(void);

int setup_zone(void)
{
    int ret = 0;

    syslog(LOG_DEBUG, "in zone setup");
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
    if (config.db_type != NULL && load_db_lib() != 0)
    {
	ret = 1;
	goto BAILOUT1;
    }
    syslog(LOG_DEBUG, "got db lib");
    (*get_server_skills)(zone->actions);
    (*get_server_objects)(zone->game_objects);

    /* Create the access thread pool */
    try
    {
	access_pool = new ThreadPool<access_list>("access",
						  config.access_threads);
    }
    catch (int errval)
    {
	syslog(LOG_ERR, "couldn't create thread pools: %s", strerror(errval));
	ret = errval;
	goto BAILOUT2;
    }
    access_pool->clean_on_pop = true;
    if ((active_users = new std::set<u_int64_t>) == NULL)
    {
	syslog(LOG_ERR, "couldn't create active users set");
	ret = ENOMEM;
	goto BAILOUT3;
    }

    try
    {
	/* Now start the actual thread pools up */
	access_pool->start(&access_pool_worker);
	zone->start(&action_pool_worker,
		    &motion_pool_worker,
		    &update_pool_worker);
    }
    catch (int errval)
    {
	syslog(LOG_ERR, "couldn't start thread pools: %s", strerror(errval));
	ret = errval;
	goto BAILOUT4;
    }

    /* Now load up the action library */
    if (config.action_lib != NULL && load_action_lib() != 0)
    {
	ret = 1;
	goto BAILOUT4;
    }
    syslog(LOG_DEBUG, "zone setup done");
    return ret;

  BAILOUT4:
    delete active_users;
    active_users = NULL;
  BAILOUT3:
    if (access_pool != NULL)
    {
	delete access_pool;
	access_pool = NULL;
    }
  BAILOUT2:
    if (dlclose(db_lib))
	syslog(LOG_ERR,
	       "couldn't close database library %s: %s",
	       config.db_type, dlerror());
  BAILOUT1:
    delete zone;
    zone = NULL;
    return ret;
}

static int load_db_lib(void)
{
    char str[128];
    void *sym;

    syslog(LOG_DEBUG, "loading database lib");
    snprintf(str, sizeof(str), "libr9_%s.so", config.db_type);
    if ((db_lib = dlopen(str, RTLD_LAZY | RTLD_GLOBAL)) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't open database library %s: %s",
	       str, dlerror());
	return 1;
    }

    if ((sym = dlsym(db_lib, "check_authentication")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find check_authentication in %s: %s",
	       str, dlerror());
	goto BAILOUT;
    }
    else
	check_authentication
	    = (u_int64_t (*)(const char *, const char *))sym;

    if ((sym = dlsym(db_lib, "check_authorization")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find check_authorization in %s: %s",
	       str, dlerror());
	goto BAILOUT;
    }
    else
	check_authorization = (int (*)(u_int64_t, u_int64_t))sym;

    if ((sym = dlsym(db_lib, "get_server_skills")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find get_server_skills in %s: %s",
	       str, dlerror());
	goto BAILOUT;
    }
    else
	get_server_skills = (int (*)(std::map<u_int16_t, action_rec>&))sym;

    if ((sym = dlsym(db_lib, "get_server_objects")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find get_server_objects in %s: %s",
	       str, dlerror());
	goto BAILOUT;
    }
    else
	get_server_objects = (int (*)(std::map<u_int64_t,
				      game_object_list_element>&))sym;

    if ((sym = dlsym(db_lib, "get_player_server_skills")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find get_player_server_skills in %s: %s",
	       str, dlerror());
	goto BAILOUT;
    }
    else
	get_player_server_skills = (int (*)(u_int64_t, u_int64_t, std::map<u_int16_t, action_level>&))sym;

    if ((sym = dlsym(db_lib, "open_new_login")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find open_new_login in %s: %s",
	       str, dlerror());
	goto BAILOUT;
    }
    else
	open_new_login = (int (*)(u_int64_t, u_int64_t))sym;

    if ((sym = dlsym(db_lib, "check_open_login")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find check_open_login in %s: %s",
	       str, dlerror());
	goto BAILOUT;
    }
    else
	check_open_login = (int (*)(u_int64_t, u_int64_t))sym;

    if ((sym = dlsym(db_lib, "close_open_login")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find close_open_login in %s: %s",
	       str, dlerror());
	goto BAILOUT;
    }
    else
	close_open_login = (int (*)(u_int64_t, u_int64_t))sym;

    return 0;

  BAILOUT:
    if (dlclose(db_lib))
	syslog(LOG_ERR,
	       "couldn't close database library %s: %s",
	       str, dlerror());
    return 1;
}

static int load_action_lib(void)
{
    void *sym;
    int ret = 1;

    syslog(LOG_DEBUG, "loading action lib");
    if ((action_lib = dlopen(config.action_lib, RTLD_LAZY)) == NULL)
	syslog(LOG_ERR,
	       "couldn't open action library %s: %s",
	       config.action_lib, dlerror());
    else if ((sym = dlsym(action_lib, "actions_register")) == NULL)
    {
	syslog(LOG_ERR,
	       "couldn't find actions_register in %s: %s",
	       config.action_lib, dlerror());
	if (dlclose(action_lib))
	    syslog(LOG_ERR,
		   "couldn't close action library %s: %s",
		   config.action_lib, dlerror());
    }
    else
    {
	(*(void (*)(std::map<u_int16_t, action_rec> &))sym)(zone->actions);
	ret = 0;
    }
    return ret;
}

void cleanup_zone(void)
{
    void *sym;

    syslog(LOG_DEBUG, "in zone cleanup");
    if (action_lib != NULL)
    {
	/* Deregister all the action routines and close the lib */
	syslog(LOG_DEBUG, "deregistering action routines");
	if ((sym = dlsym(action_lib, "actions_unregister")) == NULL)
	{
	    syslog(LOG_ERR, "couldn't find actions_unregister in %s: %s",
		   config.action_lib, dlerror());
	    /* Erase them all by hand */
	    zone->actions.erase(zone->actions.begin(), zone->actions.end());
	}
	else
	    (*(void (*)(std::map<u_int16_t, action_rec> &))sym)(zone->actions);
	syslog(LOG_DEBUG, "closing action library");
	if (dlclose(action_lib))
	    syslog(LOG_ERR,
		   "couldn't close action library %s: %s",
		   config.action_lib, dlerror());
	action_lib = NULL;
    }
    if (access_pool != NULL)
    {
	delete access_pool;
	access_pool = NULL;
    }
    if (active_users != NULL)
    {
	syslog(LOG_DEBUG, "deleting active users set");
	delete active_users;
	active_users = NULL;
    }
    if (zone != NULL)
    {
	syslog(LOG_DEBUG, "deleting zone");
	delete zone;
	zone = NULL;
    }
    if (db_lib != NULL)
    {
	syslog(LOG_DEBUG, "closing database library");
	check_authentication = NULL;
	check_authorization = NULL;
	get_server_skills = NULL;
	get_player_server_skills = NULL;
	open_new_login = NULL;
	check_open_login = NULL;
	close_open_login = NULL;
	if (dlclose(db_lib))
	    syslog(LOG_ERR,
		   "couldn't close database library: %s",
		   dlerror());
	db_lib = NULL;
    }
    syslog(LOG_DEBUG, "zone cleanup done");
}
