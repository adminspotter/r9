/* basesock.cc
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 20 May 2014, 18:23:04 tquirk
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
 * This file contains the socket creation routines, and the sending thread
 * routines.
 *
 * Changes
 *   13 Sep 2007 TAQ - Created the file from ../sockets.c.  Moved blank
 *                     constructor and destructor in here.
 *   13 Oct 2007 TAQ - Cleaned up some debugging info.
 *   22 Nov 2009 TAQ - Fixed const char warnings in create_socket.
 *
 * Things to do
 *
 * $Id: basesock.cc 10 2013-01-25 22:13:00Z trinity $
 */

#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <syslog.h>

#include "../config.h"
#include "basesock.h"

basesock::basesock()
{
    /* This should never be used */
}

basesock::~basesock()
{
    /* This should never be used */
}

int basesock::create_socket(int type, int port)
{
    int sock = -1;
    struct sockaddr_in sin;
    struct protoent *proto_ent = NULL;
    /* Save the euid and egid, since we don't save it anywhere else. */
    uid_t uid = geteuid();
    gid_t gid = getegid();
    const char *typestr = (type == SOCK_DGRAM ? "dgram" : "stream");

    if (type == SOCK_DGRAM)
    {
	if ((proto_ent = getprotobyname("udp")) == NULL)
	{
	    syslog(LOG_ERR, "couldn't find protocol 'udp'");
	    return -1;
	}
    }
    else if (type == SOCK_STREAM)
    {
	if ((proto_ent = getprotobyname("tcp")) == NULL)
	{
	    syslog(LOG_ERR, "couldn't find protocol 'tcp'");
	    return -1;
	}
    }

    /* Create the socket */
    sin.sin_family = AF_INET;
    sin.sin_port = htons((short)port);
    sin.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((sock = socket(AF_INET, type, proto_ent->p_proto)) < 0)
    {
	syslog(LOG_ERR,
	       "socket creation failed for %s port %d: %s",
	       typestr, port, strerror(errno));
	return sock;
    }

    /* Set the reuseaddr option, if required. */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR,
	       &config.use_reuse, sizeof(config.use_reuse));

    /* If root's gotta open the port, become root, if possible. */
    if (port < 1025)
    {
	if (getuid() != 0)
	{
	    syslog(LOG_ERR,
		   "can't open %s port %d as non-root user",
		   typestr, port);
	    close(sock);
	    return -1;
	}
	else
	{
	    /* Gotta be root, so get to it. */
	    seteuid(getuid());
	    setegid(getgid());
	}
    }
    if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
    {
	syslog(LOG_ERR,
	       "bind failed for %s port %d: %s",
	       typestr, port, strerror(errno));
	close(sock);
	return -1;
    }
    /* Restore the original euid and egid of the process, if necessary. */
    if (port < 1025)
    {
	seteuid(uid);
	setegid(gid);
    }

    if (type == SOCK_STREAM)
    {
	if (listen(sock, 5) < 0)
	{
	    syslog(LOG_ERR,
		   "listen failed for %s port %d: %s",
		   typestr, port, strerror(errno));
	    close(sock);
	    return -1;
	}
    }

    syslog(LOG_DEBUG,
	   "created %s (%d) socket %d on port %d",
	   typestr, proto_ent->p_proto, sock, port);
    return sock;
}
