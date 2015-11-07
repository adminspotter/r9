/* fdstreambuf.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 06 Nov 2015, 13:36:08 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2015  Trinity Annabelle Quirk
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
 * This file contains streambuf objects that take a file descriptor.
 * We'll use these for reading/writing in the consoles, since they're
 * all sockets.
 *
 * Things to do
 *
 */

#ifndef __INC_FDSTREAMBUF_H__
#define __INC_FDSTREAMBUF_H__

#include <unistd.h>

#include <cstring>

class fdobuf : public std::basic_streambuf<char, std::char_traits<char> >
{
  protected:
    int fd;

  public:
    fdobuf(int filedes)
        {
            this->fd = filedes;
        };

  protected:
    int overflow(int c)
        {
            if (c != EOF)
                if (write(this->fd, &c, 1) != 1)
                    return EOF;
            return c;
        };

    std::streamsize xsputn(const char* str, std::streamsize count)
        {
            return write(this->fd, str, count);
        };
};

class fdibuf : public std::basic_streambuf<char, std::char_traits<char> >
{
  protected:
    int fd;

  protected:
    static const int pb_len = 16;
    static const int len = 1024;
    char buf[pb_len + len], *start;

  public:
    fdibuf(int filedes)
        {
            this->fd = filedes;
            this->start = this->buf + this->pb_len;
            this->setg(this->start, this->start, this->start);
        };

  protected:
    int underflow(void)
        {
            int putbacklen, readlen;

            /* If we still have unread chars, just use them. */
            if (this->gptr() < this->egptr())
                return *this->gptr();

            /* If we have putback characters, copy them into place.
             * Clamp the number of put-back chars to the size of our
             * putback region.
             */
            if ((putbacklen = gptr() - eback()) > 0)
            {
                if (putbacklen > this->pb_len)
                    putbacklen = this->pb_len;
                memcpy(this->start - putbacklen,
                       this->gptr() - putbacklen,
                       putbacklen);
            }

            /* Grab new characters. */
            if ((readlen = read(this->fd, this->start, this->len)) <= 0)
                return EOF;

            this->setg(this->start - putbacklen,
                       this->start,
                       this->start + readlen);
            return *gptr();
        };
};

#endif /* __INC_FDSTREAMBUF_H__ */
