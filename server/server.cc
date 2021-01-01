/* server.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 13 Dec 2020, 21:38:31 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2020  Trinity Annabelle Quirk
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
 * This file contains a generic server skeleton.  The good stuff,
 * including forking of subservers and passing of file descriptors,
 * will be handled in the connection_loop routine, which is not in
 * this file.
 *
 * Everything that needs to be handled in here pretty well is already,
 * so we shouldn't need to muck about in here much more.
 *
 * Things to do
 *
 */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

#include "server.h"
#include "signals.h"

#include "classes/log.h"
#include "classes/config_data.h"
#include "classes/library.h"
#include "classes/basesock.h"
#include "classes/socket.h"
#include "classes/modules/console.h"

static void setup_daemon(void);
static void setup_log(void);
static void setup_sockets(void);
static struct addrinfo *get_addr_info(int,
                                      const std::string&,
                                      const std::string&);
static void free_addr_info(struct addrinfo *);
static void setup_zone(void);
static void setup_thread_pools(void);
static void setup_console(void);
static void cleanup_console(void);
static void cleanup_thread_pools(void);
static void cleanup_zone(void);
static void cleanup_sockets(void);
static void cleanup_log(void);
static void cleanup_daemon(void);

std::atomic<int> main_loop_exit_flag(0);
Zone *zone = NULL;
ActionPool *action_pool = NULL;   /* Takes action requests      */
MotionPool *motion_pool = NULL;   /* Processes motion/collision */
UpdatePool *update_pool = NULL;   /* Sends motion updates       */
DB *database = NULL;
static Library *db_lib = NULL, *console_lib = NULL;
std::vector<listen_socket *> sockets;
std::vector<Console *> consoles;
/* May need this mutex */
static pthread_mutex_t exit_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t exit_flag = PTHREAD_COND_INITIALIZER;

int main(int argc, char **argv)
{
    /* Set everything up. */
    setup_configuration(argc, argv);
    try { setup_daemon(); }
    catch (std::exception& e)
    {
        std::clog << e.what() << std::endl;
        return 1;
    }
    setup_log();
    setup_signals();
    try { setup_zone(); }
    catch (std::exception& e)
    {
        std::clog << syslogErr << e.what() << std::endl;
        goto BAILOUT1;
    }
    try { setup_sockets(); }
    catch (std::exception& e)
    {
        std::clog << syslogErr << e.what() << std::endl;
        goto BAILOUT2;
    }
    try { setup_console(); }
    catch (std::exception& e)
    {
        std::clog << syslogErr << e.what() << std::endl;
        goto BAILOUT3;
    }

    /* Since all our stuff is running in other threads, we'll just
     * wait until the exit flag gets waved.
     *
     * If we have errors with the exit flag here, we should just go
     * ahead and continue execution through to the exit.  Error
     * checking is kind of a moot point.
     */
    pthread_mutex_lock(&exit_mutex);
    pthread_cond_wait(&exit_flag, &exit_mutex);
    pthread_mutex_unlock(&exit_mutex);
    pthread_cond_destroy(&exit_flag);
    pthread_mutex_destroy(&exit_mutex);

    /* Clean everything up before we exit. */
    cleanup_console();
  BAILOUT3:
    cleanup_sockets();
  BAILOUT2:
    cleanup_zone();
  BAILOUT1:
    cleanup_log();
    cleanup_daemon();
    cleanup_configuration();
    cleanup_signals();
    return 0;
}

static void setup_daemon(void)
{
    int fd;
    pid_t pid;
    char str[16];

    if (config.daemonize)
    {
        /* Start up like a proper daemon */
        if ((pid = fork()) < 0)
        {
            std::ostringstream s;
            char err[128];

            strerror_r(errno, err, sizeof(err));
            s << "failed to fork: " << err << " (" << errno << ")";
            throw std::runtime_error(s.str());
        }
        else if (pid != 0)
            exit(0);
    }

    pid = getpid();
    setsid();
    chdir("/");
    umask(0);

    /* Now write the pid file if we can. */
    if ((fd = open(config.pid_fname.c_str(),
                   O_CREAT | O_WRONLY | O_EXCL,
                   S_IRUSR | S_IWUSR)) != -1)
    {
        snprintf(str, sizeof(str), "%d", pid);
        write(fd, str, strlen(str));
        close(fd);
    }
    else
    {
        /* Apparently another invocation is running, so we can't. */
        std::ostringstream s;
        char err[128];

        strerror_r(errno, err, sizeof(err));
        s << "couldn't create lock file: " << err << " (" << errno << ")";
        throw std::runtime_error(s.str());
    }
    if (config.daemonize)
    {
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);
    }
}

static void setup_log(void)
{
    /* Open the system log. */
    if (config.daemonize)
        std::clog.rdbuf(new Log(config.log_prefix, config.log_facility));

    std::clog << syslogNotice << "starting" << std::endl;
}

static void setup_sockets(void)
{
    struct addrinfo *ai;
    std::vector<port>::iterator i;
    std::vector<listen_socket *>::iterator j;
    int type_map[] = { 0, SOCK_DGRAM, SOCK_STREAM };
    int created = 0;

    /* Bailout now if there are no sockets to create */
    if (config.listen_ports.size() == 0)
    {
        std::string s("no sockets to create");
        throw std::runtime_error(s);
    }

    std::clog << "going to create "
              << config.listen_ports.size() << " listening port"
              << (config.listen_ports.size() == 1 ? "" : "s")
              << std::endl;

    for (i = config.listen_ports.begin(); i != config.listen_ports.end(); ++i)
    {
        /* First get an addrinfo struct for the socket */
        if ((ai = get_addr_info(type_map[i->type], i->addr, i->port)) == NULL)
            continue;
        try
        {
            sockets.push_back(socket_create(ai));
        }
        catch (std::exception& e)
        {
            while (sockets.size())
            {
                delete sockets.back();
                sockets.pop_back();
            }
            throw;
        }
        free_addr_info(ai);
        ++created;
    }
    if (created > 0)
        std::clog << "created " << created << " listening socket"
                  << (created == 1 ? "" : "s") << std::endl;

    /* Now start them all up */
    for (j = sockets.begin(); j != sockets.end(); ++j)
        (*j)->start();
}

static struct addrinfo *get_addr_info(int type,
                                      const std::string& addr,
                                      const std::string& port)
{
    struct addrinfo hints, *ai = NULL;
    int ret;

    if (type != SOCK_STREAM && type != SOCK_DGRAM)
    {
        struct sockaddr_un *sun = new struct sockaddr_un;

        /* Manufacture an addrinfo that has the unix socket structure
         * instead of a regular sockaddr_in/in6.  The console creator
         * understands what to do with it.  The listener creator will
         * probably blow up, but that would just be weird to do.
         */
        ai = new struct addrinfo;
        memset(ai, 0, sizeof(struct addrinfo));
        ai->ai_family = AF_UNIX;
        ai->ai_socktype = SOCK_STREAM;
        ai->ai_protocol = 0;
        ai->ai_addrlen = sizeof(struct sockaddr_un);
        ai->ai_addr = (struct sockaddr *)sun;

        memset(sun, 0, sizeof(struct sockaddr_un));
        sun->sun_family = AF_UNIX;
        strncpy(sun->sun_path, addr.c_str(), addr.size());
        return ai;
    }

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_flags = AI_PASSIVE | AI_ADDRCONFIG;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = type;
    if ((ret = getaddrinfo(addr.c_str(), port.c_str(), &hints, &ai)) != 0)
    {
        std::clog << syslogErr
                  << "failed to get address info for "
                  << (type == SOCK_STREAM ? "stream" : "dgram")
                  << " port " << addr << ':' << port << ": "
                  << gai_strerror(ret) << " (" << ret << ")" << std::endl;
        return NULL;
    }
    return ai;
}

void free_addr_info(struct addrinfo *ai)
{
    /* It wasn't clear whether freeaddrinfo was properly handling my
     * AF_UNIX-hacked addrinfo structs, so we'll go ahead and wrap
     * things to make sure they work correctly.
     */
    if (ai->ai_family == AF_UNIX)
    {
        delete ai->ai_addr;
        delete ai;
    }
    else
        freeaddrinfo(ai);
}

void set_exit_flag(void)
{
    pthread_mutex_lock(&exit_mutex);
    main_loop_exit_flag = 1;
    pthread_cond_broadcast(&exit_flag);
    pthread_mutex_unlock(&exit_mutex);
}

static void setup_zone(void)
{
    db_create_t *db_create;

    std::clog << "in zone setup" << std::endl;

    /* Load up the database lib before we start the access thread pool */
    db_lib = new Library("libr9_" + config.db_type + LT_MODULE_EXT);
    db_create = (db_create_t *)db_lib->symbol("db_create");

    database = db_create(config.db_host, config.db_user,
                         config.db_pass, config.db_name);

    zone = new Zone(config.size.dim[0], config.size.dim[1],
                    config.size.dim[2], config.size.steps[0],
                    config.size.steps[1], config.size.steps[2], database);

    setup_thread_pools();
    std::clog << "zone setup done" << std::endl;
}

static void setup_thread_pools(void)
{
    /* Motion pool needs to exist before we create the action pool;
     * the actions library keeps a pointer to it in its own address
     * space.
     */
    motion_pool = new MotionPool("motion", config.motion_threads);
    update_pool = new UpdatePool("update", config.update_threads);
    action_pool = new ActionPool(config.action_threads,
                                 zone->game_objects,
                                 database);

    action_pool->start();
    motion_pool->start();
    update_pool->start();
}

static void setup_console(void)
{
    console_create_t *console_create;
    console_destroy_t *console_destroy;
    std::vector<port>::iterator i;
    int type_map[] = { 0, SOCK_DGRAM, SOCK_STREAM };
    int created = 0;

    if (config.consoles.size() == 0)
    {
        std::clog << "no consoles to create" << std::endl;
        return;
    }

    /* Load the console module */
    console_lib = new Library("libr9_console" LT_MODULE_EXT);
    console_create =
        (console_create_t *)console_lib->symbol("console_create");
    console_destroy =
        (console_destroy_t *)console_lib->symbol("console_destroy");

    for (i = config.consoles.begin(); i != config.consoles.end(); ++i)
    {
        struct addrinfo *ai;

        /* First get an addrinfo struct for the socket */
        if ((ai = get_addr_info(type_map[i->type], i->addr, i->port)) == NULL)
            continue;
        try
        {
            consoles.push_back(console_create(ai));
        }
        catch (std::exception& e)
        {
            while (consoles.size())
            {
                console_destroy(consoles.back());
                consoles.pop_back();
            }
            throw;
        }
        free_addr_info(ai);
        ++created;
    }
    if (created > 0)
        std::clog << "created " << created << " console socket"
                  << (created == 1 ? "" : "s") << std::endl;
}

static void cleanup_console(void)
{
    struct stat st;

    /* If we didn't load the console lib, we didn't create any consoles */
    if (console_lib == NULL)
        return;

    try
    {
        console_destroy_t *console_destroy =
            (console_destroy_t *)console_lib->symbol("console_destroy");
        while (consoles.size())
        {
            console_destroy(consoles.back());
            consoles.pop_back();
        }
    }
    catch (std::exception& e) { /* Do nothing */ }

    /* Unload the library */
    std::clog << "closing console library" << std::endl;
    delete console_lib;
    console_lib = NULL;
}

static void cleanup_thread_pools(void)
{
    std::clog << "deleting thread pools" << std::endl;
    if (action_pool != NULL)
    {
        delete action_pool;
        action_pool = NULL;
    }
    if (motion_pool != NULL)
    {
        delete motion_pool;
        motion_pool = NULL;
    }
    if (update_pool != NULL)
    {
        delete update_pool;
        update_pool = NULL;
    }
}

static void cleanup_zone(void)
{
    cleanup_thread_pools();

    if (zone != NULL)
    {
        std::clog << "deleting zone" << std::endl;
        delete zone;
        zone = NULL;
    }
    if (db_lib != NULL)
    {
        std::clog << "closing database library" << std::endl;
        try
        {
            db_destroy_t *db_destroy =
                (db_destroy_t *)db_lib->symbol("db_destroy");
            db_destroy(database);
        }
        catch (std::exception& e) { /* Do nothing */ }
        delete db_lib;
        db_lib = NULL;
    }
    std::clog << "zone cleanup done" << std::endl;
}

static void cleanup_sockets(void)
{
    while (sockets.size())
    {
        delete sockets.back();
        sockets.pop_back();
    }
}

static void cleanup_log(void)
{
    std::clog << syslogNotice << "terminating" << std::endl;

    /* Close the system log gracefully. */
    if (config.daemonize)
    {
        Log *l = dynamic_cast<Log *>(std::clog.rdbuf());

        if (l != NULL)
            l->close();
    }
}

static void cleanup_daemon(void)
{
    /* Remove the pidfile so another invocation can run. */
    unlink(config.pid_fname.c_str());
}

/* For the signal handlers. */
void complete_startup(void)
{
    setup_configuration(0, NULL);
    setup_signals();
    try
    {
        setup_zone();
        setup_sockets();
    }
    catch (std::exception& e)
    {
        std::clog << syslogErr << e.what() << std::endl;
        exit(1);
    }
}

/* For the signal handlers. */
void complete_cleanup(void)
{
    cleanup_sockets();
    cleanup_zone();
    cleanup_configuration();
}
