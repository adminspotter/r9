/* console.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 08 Jun 2014, 14:48:38 tquirk
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
 * This file contains the base console class implementation, along
 * with the console session thread class.
 *
 * Changes
 *   25 May 2014 TAQ - Created the file.  Having the console(s) handled by
 *                     some C stuff, in weird ways, is just silly.  We can
 *                     handle things much more extensibly with some open-
 *                     ended C++ objects.
 *   31 May 2014 TAQ - Added some more utility functions, copied from
 *                     console.c.
 *   07 Jun 2014 TAQ - Removed a couple remnants of a previous bad idea.
 *                     Created the new ConsoleSession, which solves the
 *                     file descriptor leaking problem.
 *
 * Things to do
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <syslog.h>
#include <pthread.h>

#include "console.h"

pthread_mutex_t ConsoleSession::dispatch_lock = PTHREAD_MUTEX_INITIALIZER;

ConsoleSession::ConsoleSession(int sock)
{
    int ret;

    this->sock = sock;
    /* Let's simplify things with some file pointers */
    if ((this->in = fdopen(sock, "r")) == NULL)
    {
	syslog(LOG_ERR, "error in inbound fdopen: %s (%d)",
               strerror(errno), errno);
	throw errno;
    }
    if ((this->out = fdopen(sock, "w")) == NULL)
    {
	syslog(LOG_ERR, "error in outbound fdopen: %s (%d)",
               strerror(errno), errno);
        throw errno;
    }
    if ((ret = pthread_create(&(this->thread_id), NULL,
                              ConsoleSession::start,
                              (void *)this)) != 0)
    {
        syslog(LOG_ERR,
               "error creating console thread: %s (%d)",
               strerror(ret), ret);
        throw errno;
    }
}

ConsoleSession::~ConsoleSession()
{
    ConsoleSession::cleanup(this);
}

void ConsoleSession::get_lock(void)
{
    pthread_mutex_lock(&ConsoleSession::dispatch_lock);
}

void ConsoleSession::drop_lock(void)
{
    pthread_mutex_unlock(&ConsoleSession::dispatch_lock);
}

void *ConsoleSession::start(void *arg)
{
    ConsoleSession *sess = (ConsoleSession *)arg;
    int done = 0, old_cancel_type;

    pthread_cleanup_push(ConsoleSession::cleanup, (void *)sess);

    while (!done)
    {
        std::string &str = sess->get_line();

	/* Do we need to exit? */
	if (str == "exit")
	{
	    fprintf(sess->out, "exiting\n");
	    fflush(sess->out);
	    done = 1;
            break;
	}

        sess->get_lock();
        fprintf(sess->out, ConsoleSession::dispatch(str).c_str());
        sess->drop_lock();
        fflush(sess->out);
    }
    syslog(LOG_NOTICE, "closing console");

    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, &old_cancel_type);
    pthread_cleanup_pop(1);
    pthread_setcanceltype(old_cancel_type, NULL);
    pthread_exit(NULL);

    return NULL;
}

std::string ConsoleSession::dispatch(std::string &command)
{
    return std::string("not implemented");
}

/* So we don't leak file descriptors when we get cancelled */
void ConsoleSession::cleanup(void *arg)
{
    ConsoleSession *sess = (ConsoleSession *)arg;

    if (sess->in)
    {
        fclose(sess->in);
        sess->in = NULL;
    }
    if (sess->out)
    {
        fclose(sess->out);
        sess->out = NULL;
    }
    if (sess->sock)
    {
        close(sess->sock);
        sess->sock = 0;
    }
}

std::string &ConsoleSession::get_line(void)
{
    char line[1024];
    static std::string str = "";

    fprintf(this->out, "r9 ~> ");
    fflush(this->out);
    /* Make sure we're sane here.  Maybe also flush the in once we're done
     * to prevent any other stuff just hanging out?
     */
    if (fgets(line, sizeof(line), this->in) == NULL)
    {
        /* The other end went away */
        str = "exit";
        return str;
    }
    /* The \r and \n are left on the string */
    line[strlen(line) - 2] = '\0';
    str = line;
    return str;
}

Console::Console()
{
    this->console_sock = 0;
}

Console::~Console()
{
    this->stop();
    if (this->console_sock)
    {
        close(this->console_sock);
    }
}

void Console::start(void *(*func)(void *))
{
    int ret;

    if (this->console_sock == 0)
    {
        syslog(LOG_ERR, "no socket available to listen");
        throw ENOENT;
    }
    if ((ret = pthread_create(&(this->listen_thread),
                              NULL,
                              func,
                              (void *)this)) != 0)
    {
        syslog(LOG_ERR,
               "couldn't start listen thread: %s (%d)",
               strerror(ret), ret);
        throw ret;
    }
}

void Console::stop(void)
{
    int ret;

    if ((ret = pthread_cancel(this->listen_thread)) != 0)
    {
        syslog(LOG_ERR,
               "couldn't terminate console thread: %s (%d)",
               strerror(ret), ret);
        throw ret;
    }
    close(this->console_sock);
    this->console_sock = 0;
    while (sessions.size())
    {
        ConsoleSession *sess = sessions.back();
        pthread_cancel(sess->thread_id);
        sleep(0);
        pthread_join(sess->thread_id, NULL);
        delete sess;
        sessions.pop_back();
    }
}
