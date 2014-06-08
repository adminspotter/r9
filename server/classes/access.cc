/* access.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 10 May 2014, 17:28:06 tquirk
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
 * This file contains the access thread pool worker routine, and all
 * other related support routines.
 *
 * Changes
 *   11 Aug 2006 TAQ - Created the file.
 *   16 Aug 2006 TAQ - We now zero out the password field in do_login once
 *                     we're done authenticating.  do_logout actually does
 *                     stuff.
 *   17 Aug 2006 TAQ - Added a check for NULL on logout.
 *   22 Jun 2007 TAQ - Added some debugging to check why our login responses
 *                     weren't being sent; turns out it was a problem with
 *                     the sockaddr_in wrapper not being complete.
 *   24 Jun 2007 TAQ - Changed name of sockaddr_in.h to sockaddr.h.
 *   05 Sep 2007 TAQ - Removed references to sockaddr stuff, and a lot of
 *                     zone items (which don't exist anymore).
 *   10 Sep 2007 TAQ - Added a user list with mutex.  Completed the login
 *                     and logout routines with our fancy new access_list
 *                     structure.
 *   12 Sep 2007 TAQ - Added some comments.
 *   17 Sep 2007 TAQ - Added handling of socks member of dgram_socket.
 *   18 Sep 2007 TAQ - Debugging to find a crash (it's in Control::send).
 *   23 Sep 2007 TAQ - Now we erase the datagram socket ID from the socks
 *                     member of the socket object.
 *   30 Sep 2007 TAQ - Changed to use funcptrs derived from the dynamically-
 *                     loaded lib, rather than linked-in funcs.  Put in a
 *                     commented call to open_new_login; once we figure out
 *                     how that needs to work (and write the func), we can
 *                     uncomment it.
 *   07 Oct 2007 TAQ - Moved database funcptr definitions in here.  Cleaned
 *                     up excessive debugging output.
 *   11 Oct 2007 TAQ - Updated database funcptrs to be of proper type.
 *   13 Oct 2007 TAQ - Cleaned up debugging output a bit.
 *   22 Oct 2007 TAQ - Set timestamp on login for dgram users.
 *   02 Dec 2007 TAQ - Removed an unused variable from do_login.  Reworked
 *                     logout sequence so that we can send an ack, and also
 *                     the "quick, logout so I don't get killed" loophole
 *                     is somewhat closed.
 *   15 Dec 2007 TAQ - Moved debugging so it wouldn't cause a segfault.  Jeez.
 *   19 Sep 2013 TAQ - Return NULL at the end of the worker routine to quiet
 *                     gcc.
 *   10 May 2014 TAQ - The control object now uses std::string instead of
 *                     a char pointer.
 *
 * Things to do
 *
 * $Id$
 */

#include <time.h>
#include <syslog.h>
#include <pthread.h>

#include "zone.h"
#include "zone_interface.h"
#include "dgram.h"
#include "stream.h"

static void do_login(access_list&);
static void do_logout(access_list&);

/* Funcptrs for database access */
u_int64_t (*check_authentication)(const char *, const char *);
int (*check_authorization)(const char *, u_int64_t);
int (*get_server_skills)(std::map<u_int16_t, action_rec>&);
int (*get_server_objects)(std::map<u_int64_t, game_object_list_element>&);
int (*get_player_server_skills)(const char *,
				u_int64_t,
				std::map<u_int16_t, action_level>&);
int (*open_new_login)(u_int64_t, u_int64_t);
int (*check_open_login)(u_int64_t, u_int64_t);
int (*close_open_login)(u_int64_t, u_int64_t);

/* ARGSUSED */
void *access_pool_worker(void *notused)
{
    access_list req;

    syslog(LOG_DEBUG, "started access pool worker");
    for (;;)
    {
        access_pool->pop(&req);

	ntoh_packet(&(req.buf), sizeof(packet));

	if (req.buf.basic.type == TYPE_LOGREQ)
	    do_login(req);
	else if (req.buf.basic.type == TYPE_LGTREQ)
	    do_logout(req);
	/* Otherwise, we don't recognize it, and will ignore it */
    }
    syslog(LOG_DEBUG, "access pool worker ending");
    return NULL;
}

static void do_login(access_list& p)
{
    u_int64_t userid = 0LL;
    dgram_socket *dgs = dynamic_cast<dgram_socket *>(p.parent);
    stream_socket *sts = dynamic_cast<stream_socket *>(p.parent);

    /* Need to check out the contents of username and password before
     * we start using them for stuff.
     */

    userid = (*check_authentication)(p.buf.log.username,
				     p.buf.log.password);
    /* Don't want to keep passwords around in core if we can help it */
    memset(p.buf.log.password, 0, sizeof(p.buf.log.password));
    syslog(LOG_DEBUG,
	   "login request from %s (%lld)",
	   p.buf.log.username, userid);
    if (userid != 0LL)
    {
	pthread_mutex_lock(&active_users_mutex);
	if (active_users->find(userid) == active_users->end())
	{
	    /* Not already logged in, so log them in */
	    active_users->insert(userid);
	    pthread_mutex_unlock(&active_users_mutex);

	    Control *newcontrol = new Control(userid, NULL);
	    newcontrol->username = p.buf.log.username;

	    /* Add this user to the socket's userlist */
	    if (dgs != NULL)
	    {
		dgram_user dgu;

		newcontrol->parent = (void *)(dgs->send);
		dgu.userid = userid;
		dgu.control = newcontrol;
		dgu.timestamp = time(NULL);
		dgu.sin = p.what.login.who.dgram;
		dgu.pending_logout = false;
		pthread_mutex_lock(&active_users_mutex);
		dgs->users[userid] = dgu;
		dgs->socks[dgu.sin] = &(dgs->users[userid]);
		pthread_mutex_unlock(&active_users_mutex);
		syslog(LOG_DEBUG,
		       "added new user %s (%lld) to dgram userlist %d",
		       newcontrol->username.c_str(), dgu.userid, dgs->port);
	    }
	    else if (sts != NULL)
	    {
		stream_user stu;

		newcontrol->parent = (void *)(sts->send);
		stu.userid = userid;
		stu.control = newcontrol;
		stu.subsrv = p.what.login.who.stream.sub;
		stu.fd = p.what.login.who.stream.sock;
		stu.pending_logout = false;
		pthread_mutex_lock(&active_users_mutex);
		sts->users[userid] = stu;
		pthread_mutex_unlock(&active_users_mutex);
		syslog(LOG_DEBUG,
		       "added new user %s (%lld) to stream userlist %d",
		       newcontrol->username.c_str(), stu.userid, sts->port);
	    }
	    /* Send an ack packet, to let the user know they're in */
	    newcontrol->send_ack(TYPE_LOGREQ);
	}
	else
	    /* User is already logged in, ignore */
	    /* Should we maybe send some sort of "you're already logged
	     * in, dickhead" message here?  Or just re-ack their login?
	     */
	    pthread_mutex_unlock(&active_users_mutex);
    }
    /* Otherwise, do nothing, and send nothing */
}

static void do_logout(access_list& p)
{
    dgram_socket *dgs = dynamic_cast<dgram_socket *>(p.parent);
    stream_socket *sts = dynamic_cast<stream_socket *>(p.parent);
    Control *control = NULL;

    /* Most of this function is now handled by the reaper threads */
    if (active_users->find(p.what.logout.who) != active_users->end())
    {
	if (dgs != NULL)
	{
	    control = dgs->users[p.what.logout.who].control;
	    dgs->users[p.what.logout.who].pending_logout = true;
	}
	else if (sts != NULL)
	{
	    control = sts->users[p.what.logout.who].control;
	    sts->users[p.what.logout.who].pending_logout = true;
	}
	syslog(LOG_DEBUG,
	       "logout request from %s (%lld)",
	       control->username.c_str(), control->userid);
	control->send_ack(TYPE_LGTREQ);
    }
}
