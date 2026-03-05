/* addrinfo.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2026  Trinity Annabelle Quirk
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
 * This file contains a wrapper around the struct addrinfo, so that we
 * can use it in a more straightforward manner.  It allows us to use
 * an object hierarchy to segregate the network/unix dichotomy into a
 * single factory function.
 *
 * Things to do
 *
 */

#ifndef __INC_ADDRINFO_H__
#define __INC_ADDRINFO_H__

#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <errno.h>

#include <sstream>
#include <stdexcept>
#include <system_error>

typedef enum {
    UNIX = 0,
    STREAM = SOCK_STREAM,
    DGRAM = SOCK_DGRAM
} port_type;

class Addrinfo
{
  public:
    struct addrinfo *ai;

  protected:
    Addrinfo()
        {
            this->ai = NULL;
        }

  public:
    Addrinfo(int type, const std::string& addr, const std::string& port)
        {
            struct addrinfo hints;

            this->ai = NULL;
            memset(&hints, 0, sizeof(struct addrinfo));
            hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
            hints.ai_family = AF_UNSPEC;
            hints.ai_socktype = type;

            int ret = getaddrinfo(addr.c_str(), port.c_str(),
                                  &hints, &this->ai);
            if (ret != 0)
            {
                std::ostringstream s;

                s << "failed to get address info for "
                  << (type == SOCK_STREAM ? "stream" : "dgram")
                  << " port " << addr << ':' << port << ": "
                  << gai_strerror(ret) << " (" << ret << ")" << std::endl;
                throw std::runtime_error(s.str());
            }
        }
    virtual ~Addrinfo() { freeaddrinfo(this->ai); }
};

class Addrinfo_un : public Addrinfo
{
  public:
    Addrinfo_un(const std::string& path)
        : Addrinfo()
        {
            this->ai = new struct addrinfo;
            memset(this->ai, 0, sizeof(struct addrinfo));
            this->ai->ai_family = AF_UNIX;
            this->ai->ai_socktype = SOCK_STREAM;
            this->ai->ai_protocol = 0;
            this->ai->ai_addrlen = sizeof(struct sockaddr_un);
            struct sockaddr_un *sun = new struct sockaddr_un;
            memset(sun, 0, sizeof(struct sockaddr_un));
            sun->sun_family = AF_UNIX;
            strncpy(sun->sun_path, path.c_str(), path.size());
            this->ai->ai_addr = (struct sockaddr *)sun;
        }
    ~Addrinfo_un()
        {
            delete this->ai->ai_addr;
            this->ai->ai_addr = NULL;
        }
};

inline Addrinfo *build_addrinfo(port_type type,
                                const std::string& addr,
                                const std::string& port)
{
    if (type == UNIX)
        return new Addrinfo_un(addr);

    return new Addrinfo(type, addr, port);
}

inline Addrinfo *str_to_addrinfo(const std::string& str)
{
    /* Strings will be in the forms:
     * (dgram|stream):<addr>:<port>
     * unix:<path>
     *
     * addr is an optional IP address of some kind
     *   (1.2.3.4 or [::f00f:1234])
     * port is a port number
     * path is the pathname of the unix domain socket
     */
    std::string val = str, type_str, addr_str, port_str;
    std::string::size_type found;
    port_type type;

    if ((found = val.find_first_of(":")) == std::string::npos)
        return NULL;
    type_str = val.substr(0, found);
    val.replace(0, found + 1, "");

    if (type_str == "unix")
        return new Addrinfo_un(val);
    else if (type_str == "dgram")
        type = DGRAM;
    else if (type_str == "stream")
        type = STREAM;
    else
        return NULL;

    if ((found = val.find_last_of(":")) == std::string::npos)
        return NULL;
    addr_str = val.substr(0, found);
    port_str = val.substr(found + 1);

    /* See if we've got an IPv6 address */
    if ((found = addr_str.find_first_of(":")) != std::string::npos)
    {
        if ((found = addr_str.find_first_of("[")) == std::string::npos)
            return NULL;
        addr_str.erase(found, 1);
        if ((found = addr_str.find_last_of("]")) == std::string::npos)
            return NULL;
        addr_str.erase(found, 1);
    }
    return new Addrinfo((int)type, addr_str, port_str);
}

#endif /* __INC_ADDRINFO_H__ */
