/* sockaddr.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 17 Jun 2014, 17:31:45 tquirk
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
 * This file contains a wrapper around the struct sockaddr family, so that we
 * can use them in a more straightforward manner.
 *
 * Things to do
 *
 */

#ifndef __INC_SOCKADDR_H__
#define __INC_SOCKADDR_H__

#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

class Sockaddr
{
  public:
    struct sockaddr_storage ss;
  protected:
    char str[INET6_ADDRSTRLEN];

  public:
    Sockaddr()
        {
            this->ss.ss_family = AF_INET;
        };
    Sockaddr(const Sockaddr& s)
        {
            this->ss.ss_family = s.ss.ss_family;
        };
    Sockaddr(const struct sockaddr& s)
        {
            this->ss.ss_family = s.sa_family;
        };
    virtual ~Sockaddr()
        {
        };

    virtual bool operator==(const Sockaddr& s)
        {
            return !memcmp(&(this->ss),
                           &(s.ss),
                           sizeof(struct sockaddr_storage));
        };
    virtual bool operator==(const struct sockaddr *s)
        {
            return !memcmp(&(this->ss), s, sizeof(struct sockaddr_storage));
        };

    virtual bool operator<(const Sockaddr& s) const = 0;
    virtual bool operator<(const struct sockaddr& s) const = 0;

    virtual Sockaddr& operator=(const Sockaddr& s)
        {
            memcpy(&(this->ss), &(s.ss), sizeof(sockaddr_storage));
            return *this;
        };
    virtual Sockaddr& operator=(const struct sockaddr& s)
        {
            memcpy(&(this->ss), &s, sizeof(sockaddr_storage));
            return *this;
        }

    virtual char *ntop(void) = 0;
};

/* Since we can't use a Sockaddr directly in one of our map keys,
 * we'll need to use pointers to them, to leverage the polymorphism
 * here.  We need a compare functor to deref the pointers for the
 * less-than comparison that's required by the map.
 */
struct less_sockaddr : std::binary_function<const Sockaddr *,
                                            const Sockaddr *,
                                            bool>
{
    bool operator()(const Sockaddr *a, const Sockaddr *b) const
        {
            return (*a < *b);
        }
};

class Sockaddr_in : public Sockaddr
{
  public:
    struct sockaddr_in *sin;

    Sockaddr_in()
        {
            this->sin = (struct sockaddr_in *)&ss;
            this->sin->sin_family = AF_INET;
            this->sin->sin_port = htons(0);
            this->sin->sin_addr.s_addr = htonl(INADDR_ANY);
            memset(this->str, 0, INET6_ADDRSTRLEN);
        };
    Sockaddr_in(const Sockaddr& s)
        {
            const Sockaddr_in& sin = dynamic_cast<const Sockaddr_in&>(s);
            this->sin = (struct sockaddr_in *)&ss;
            this->sin->sin_family = sin.sin->sin_family;
            this->sin->sin_port = sin.sin->sin_port;
            this->sin->sin_addr.s_addr = sin.sin->sin_addr.s_addr;
            memcpy(this->str, sin.str, INET6_ADDRSTRLEN);
        };
    Sockaddr_in(const struct sockaddr& s)
        {
            const struct sockaddr_in& sin
                = reinterpret_cast<const struct sockaddr_in&>(s);
            this->sin = (struct sockaddr_in *)&ss;
            this->sin->sin_family = sin.sin_family;
            this->sin->sin_port = sin.sin_port;
            this->sin->sin_addr.s_addr = sin.sin_addr.s_addr;
            memset(this->str, 0, INET6_ADDRSTRLEN);
        };
    ~Sockaddr_in()
        {
        };

    bool operator==(const Sockaddr& s) const
        {
            const Sockaddr_in *si = dynamic_cast<const Sockaddr_in *>(&s);
            if (si == NULL)
                return false;
            return !memcmp(this->sin, si->sin, sizeof(struct sockaddr_in));
        };
    bool operator==(const struct sockaddr *s)
        {
            const struct sockaddr_in& sin
                = reinterpret_cast<const struct sockaddr_in&>(s);
            return !memcmp(&(this->ss), &sin, sizeof(struct sockaddr_in));
        };

    bool operator<(const Sockaddr& s) const
        {
            const Sockaddr_in *si = dynamic_cast<const Sockaddr_in *>(&s);
            if (si == NULL)
                return false;
            return (this->sin->sin_addr.s_addr < si->sin->sin_addr.s_addr
                    || this->sin->sin_port < si->sin->sin_port);
        };
    bool operator<(const struct sockaddr& s) const
        {
            const struct sockaddr_in& sin
                = reinterpret_cast<const struct sockaddr_in&>(s);
            return (this->sin->sin_addr.s_addr < sin.sin_addr.s_addr
                    || this->sin->sin_port < sin.sin_port);
        };

    char *ntop(void)
        {
            inet_ntop(this->sin->sin_family, &(this->sin->sin_addr),
                      this->str, INET6_ADDRSTRLEN);
            return this->str;
        };
};

/* A couple functions to be able to handle IPV6 addresses easily:
 * equality comparison and less-than comparison.  v6 addresses are
 * always stored in network byte order, so there's no need to convert
 * at all in these routines.
 */
inline bool operator==(const struct in6_addr& a, const struct in6_addr& b)
{
    return !memcmp(&a, &b, sizeof(struct in6_addr));
}

inline bool operator<(const struct in6_addr& a, const struct in6_addr& b)
{
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

class Sockaddr_in6 : public Sockaddr
{
  public:
    struct sockaddr_in6 *sin6;

    Sockaddr_in6()
        {
            this->sin6 = (struct sockaddr_in6 *)&ss;
            this->sin6->sin6_family = AF_INET;
            this->sin6->sin6_port = htons(0);
            this->sin6->sin6_flowinfo = htonl(0L);
            memcpy(&(this->sin6->sin6_addr.s6_addr),
                   &in6addr_any,
                    sizeof(struct in6_addr));
            this->sin6->sin6_scope_id = htonl(0L);
            memset(this->str, 0, INET6_ADDRSTRLEN);
        };
    Sockaddr_in6(const Sockaddr& s)
        {
            const Sockaddr_in6& sin6 = dynamic_cast<const Sockaddr_in6&>(s);
            this->sin6 = (struct sockaddr_in6 *)&ss;
            this->sin6->sin6_family = sin6.sin6->sin6_family;
            this->sin6->sin6_port = sin6.sin6->sin6_port;
            this->sin6->sin6_flowinfo = sin6.sin6->sin6_flowinfo;
            memcpy(&(this->sin6->sin6_addr),
                   &(sin6.sin6->sin6_addr),
                   sizeof(struct in6_addr));
            this->sin6->sin6_scope_id = sin6.sin6->sin6_scope_id;
            memcpy(this->str, sin6.str, INET6_ADDRSTRLEN);
        };
    Sockaddr_in6(const struct sockaddr& s)
        {
            struct sockaddr_in6 *si = (struct sockaddr_in6 *)(&s);
            this->sin6 = (struct sockaddr_in6 *)&ss;
            this->sin6->sin6_family = si->sin6_family;
            this->sin6->sin6_port = si->sin6_port;
            this->sin6->sin6_flowinfo = si->sin6_flowinfo;
            memcpy(&(this->sin6->sin6_addr),
                   &(si->sin6_addr),
                   sizeof(struct in6_addr));
            this->sin6->sin6_scope_id = si->sin6_scope_id;
            memset(this->str, 0, INET6_ADDRSTRLEN);
        };
    ~Sockaddr_in6()
        {
        };

    bool operator==(const Sockaddr &s) const
        {
            const Sockaddr_in6 *si = dynamic_cast<const Sockaddr_in6 *>(&s);
            if (si == NULL)
                return false;
            return !memcmp(this->sin6, si->sin6, sizeof(struct sockaddr_in6));
        };

    bool operator<(const Sockaddr& s) const
        {
            const Sockaddr_in6 *si = dynamic_cast<const Sockaddr_in6 *>(&s);
            if (si == NULL)
                return false;
            return (this->sin6->sin6_addr < si->sin6->sin6_addr
                    || this->sin6->sin6_port < si->sin6->sin6_port);
        };
    bool operator<(const struct sockaddr& s) const
        {
            const struct sockaddr_in6& sin6
                = reinterpret_cast<const struct sockaddr_in6&>(s);
            return (this->sin6->sin6_addr < sin6.sin6_addr
                    || this->sin6->sin6_port < sin6.sin6_port);
        };

    char *ntop(void)
        {
            inet_ntop(this->sin6->sin6_family, &(this->sin6->sin6_addr),
                      this->str, INET6_ADDRSTRLEN);
            return this->str;
        };
};

inline Sockaddr *build_sockaddr(struct sockaddr& s)
{
    switch (s.sa_family)
    {
      case AF_INET:
      {
          Sockaddr_in *sin = new Sockaddr_in(s);
          return dynamic_cast<Sockaddr *>(sin);
      }

      case AF_INET6:
      {
          Sockaddr_in6 *sin6 = new Sockaddr_in6(s);
          return dynamic_cast<Sockaddr *>(sin6);
      }

      default:
        return NULL;
    }
}

#endif /* __INC_SOCKADDR_H__ */
