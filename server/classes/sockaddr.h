/* sockaddr.h                                           -*- C++ -*-
 *   by Trinity Quirk <trinity@ymb.net>
 *   last updated 17 Sep 2007, 18:10:28 trinity
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
 * This file contains a wrapper around the struct sockaddr_in, so that we
 * can use it in a more straightforward manner.
 *
 * Changes
 *   11 Aug 2006 TAQ - Created the file.
 *   22 Jun 2007 TAQ - Turns out I missed the sin_family member of the struct
 *                     and it was causing lots of problems when it was set
 *                     to random numbers from not being initialized.
 *   24 Jun 2007 TAQ - Changed it to be just sockaddr.h, and added some
 *                     conditional compilation for IPV4 and IPV6.
 *   30 Jun 2007 TAQ - Quick fix of the IPV6 address assignment routine.
 *                     Fixed IN_ADDR_ANY to be the correct INADDR_ANY.
 *
 * Things to do
 *
 * $Id: sockaddr.h 10 2013-01-25 22:13:00Z trinity $
 */

#ifndef __INC_SOCKADDR_H__
#define __INC_SOCKADDR_H__

#include <netinet/in.h>

#if defined(USE_IPV4) || !defined(USE_IPV6)

class Sockaddr
{
  public:
    struct sockaddr_in sin;

    Sockaddr()
	{
	    this->sin.sin_family = AF_INET;
	    this->sin.sin_port = htons(0);
	    this->sin.sin_addr.s_addr = htonl(INADDR_ANY);
	};
    Sockaddr(struct sockaddr_in &sock)
	{
	    this->sin.sin_family = sock.sin_family;
	    this->sin.sin_port = sock.sin_port;
	    this->sin.sin_addr.s_addr = sock.sin_addr.s_addr;
	};
    ~Sockaddr()
	{
	};

    inline const Sockaddr &operator=(const Sockaddr &sock)
	{
	    this->sin.sin_family = sock.sin.sin_family;
	    this->sin.sin_port = sock.sin.sin_port;
	    this->sin.sin_addr.s_addr = sock.sin.sin_addr.s_addr;
	    return *this;
	};
    inline const Sockaddr &operator=(const struct sockaddr_in &sock)
	{
	    this->sin.sin_family = sock.sin_family;
	    this->sin.sin_port = sock.sin_port;
	    this->sin.sin_addr.s_addr = sock.sin_addr.s_addr;
	    return *this;
	};
    inline int operator==(const Sockaddr &sock) const
	{
	    return (this->sin.sin_family == sock.sin.sin_family
		    && this->sin.sin_port == sock.sin.sin_port
		    && this->sin.sin_addr.s_addr == sock.sin.sin_addr.s_addr);
	};
    inline int operator<(const Sockaddr& sock) const
	{
	    return (this->sin.sin_addr.s_addr < sock.sin.sin_addr.s_addr
		    || this->sin.sin_port < sock.sin.sin_port);
	};
};

#elif defined(USE_IPV6)

/* A few functions to be able to handle IPV6 addresses easily:  assignment,
 * equality comparison, and less-than comparison.
 */
inline const struct in6_addr &operator=(struct in6_addr &a,
					const struct in6_addr &b)
{
    a.s6_addr32[0] = b.s6_addr32[0];
    a.s6_addr32[1] = b.s6_addr32[1];
    a.s6_addr32[2] = b.s6_addr32[2];
    a.s6_addr32[3] = b.s6_addr32[3];
    return a;
}

inline int operator==(const struct in6_addr &a, const struct in6_addr &b)
{
    return (a.s6_addr32[0] == b.s6_addr32[0]
	    && a.s6_addr32[1] == b.s6_addr32[1]
	    && a.s6_addr32[2] == b.s6_addr32[2]
	    && a.s6_addr32[3] == b.s6_addr32[3])
}

inline int operator<(const struct in6_addr &a, const struct in6_addr &b)
{
    /* We might need more granularity here, since network byte order and
     * host byte order may be different.
     */
    return (a.s6_addr[0] < b.s6_addr[0]
	    || a.s6_addr[1] < b.s6_addr[1]
	    || a.s6_addr[2] < b.s6_addr[2]
	    || a.s6_addr[3] < b.s6_addr[3]
	    || a.s6_addr[4] < b.s6_addr[4]
	    || a.s6_addr[5] < b.s6_addr[5]
	    || a.s6_addr[6] < b.s6_addr[6]
	    || a.s6_addr[7] < b.s6_addr[7]
	    || a.s6_addr[8] < b.s6_addr[8]
	    || a.s6_addr[9] < b.s6_addr[9]
	    || a.s6_addr[10] < b.s6_addr[10]
	    || a.s6_addr[11] < b.s6_addr[11]
	    || a.s6_addr[12] < b.s6_addr[12]
	    || a.s6_addr[13] < b.s6_addr[13]
	    || a.s6_addr[14] < b.s6_addr[14]
	    || a.s6_addr[15] < b.s6_addr[15]);
}

class Sockaddr
{
  public:
    struct sockaddr_in6 sin;

    Sockaddr()
	{
	    this->sin.sin6_family = AF_INET;
	    this->sin.sin6_port = htons(0);
	    this->sin.sin6_flowinfo = htonl(0L);
	    this->sin.sin6_addr.s6_addr = in6addr_any;
	    this->sin.sin6_scope_id = htonl(0L);
	};
    Sockaddr(struct sockaddr_in &sock)
	{
	    this->sin.sin6_family = sock.sin6_family;
	    this->sin.sin6_port = sock.sin6_port;
	    this->sin.sin6_flowinfo = sock.sin6_flowinfo;
	    this->sin.sin6_addr = sock.sin6_addr;
	    this->sin.sin6_scope_id = sock.sin6_scope_id;
	};
    ~Sockaddr()
	{
	};

    inline const Sockaddr &operator=(const Sockaddr &sock)
	{
	    this->sin.sin6_family = sock.sin.sin6_family;
	    this->sin.sin6_port = sock.sin.sin6_port;
	    this->sin.sin6_flowinfo = sock.sin.sin6_flowinfo;
	    this->sin.sin6_addr = sock.sin.sin6_addr;
	    this->sin.sin6_scope_id = sock.sin.sin6_scope_id;
	    return *this;
	};
    inline const Sockaddr &operator=(const struct sockaddr_in6 &sock)
	{
	    this->sin.sin6_family = sock.sin6_family;
	    this->sin.sin6_port = sock.sin6_port;
	    this->sin.sin6_flowinfo = sock.sin6_flowinfo;
	    this->sin.sin6_addr = sock.sin6_addr;
	    this->sin.sin6_scope_id = sock.sin6_scope_id;
	    return *this;
	};
    inline int operator==(const Sockaddr &sock) const
	{
	    return (this->sin.sin6_family == sock.sin.sin6_family
		    && this->sin.sin6_port == sock.sin.sin6_port
		    && this->sin.sin6_flowinfo == sock.sin.sin6_flowinfo
		    && this->sin.sin6_addr == sock.sin.sin6_addr
		    && this->sin.sin6_scope_id == sock.sin.sin6_scope_id);
	};
    inline int operator<(const Sockaddr& sock) const
	{
	    return (this->sin.sin6_addr < sock.sin.sin6_addr
		    || this->sin.sin6_port < sock.sin.sin6_port);
	};
};

#endif /* defined(USE_IPV4) || !defined(USE_IPV6) */
#endif /* __INC_SOCKADDR_H__ */
