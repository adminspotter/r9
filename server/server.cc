/* server.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 1998-2026  Trinity Annabelle Quirk
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>

#include <fstream>
#include <stdexcept>
#include <system_error>

#include "server.h"
#include "signals.h"

#include "classes/log.h"
#include "classes/config_data.h"
#include "classes/library.h"
#include "classes/socket.h"
#include "classes/modules/console.h"

static void setup_daemon(void);
static void setup_log(void);
static void setup_sockets(void);
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
ActionPool *action_pool = NULL;
MotionPool *motion_pool = NULL;
UpdatePool *update_pool = NULL;
DB *database = NULL;
static Library *db_lib = NULL, *console_lib = NULL;
std::vector<listen_socket *> sockets;
std::vector<Console *> consoles;
static pthread_mutex_t exit_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t exit_flag = PTHREAD_COND_INITIALIZER;

int main(int argc, char **argv)
{
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
        if ((pid = fork()) < 0)
        {
            std::string s("failed to fork");

            throw std::system_error(errno, std::generic_category(), s);
        }
        else if (pid != 0)
            exit(0);
    }

    pid = getpid();
    setsid();
    chdir("/");
    umask(0);

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
        std::string s("couldn't create lock file");

        throw std::system_error(errno, std::generic_category(), s);
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
    if (config.daemonize)
        std::clog.rdbuf(new Log(config.log_prefix, config.log_facility));

    std::clog << syslogNotice << "starting" << std::endl;
}

static void setup_sockets(void)
{
    int created = 0;

    if (config.listen_ports.size() == 0)
    {
        std::string s("no sockets to create");
        throw std::runtime_error(s);
    }

    std::clog << "going to create "
              << config.listen_ports.size() << " listening port"
              << (config.listen_ports.size() == 1 ? "" : "s")
              << std::endl;

    for (auto& i : config.listen_ports)
    {
        try {
            sockets.push_back(socket_create(i));
            ++created;
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
    }
    if (created > 0)
        std::clog << "created " << created << " listening socket"
                  << (created == 1 ? "" : "s") << std::endl;

    for (auto& i : sockets)
        i->start();
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

    db_lib = new Library("libr9_" + config.db_type + LT_MODULE_EXT);
    db_create = (db_create_t *)db_lib->symbol("db_create");

    database = db_create(config.db_host, config.db_port, config.db_user,
                         config.db_pass, config.db_name);
    std::clog << "created database" << std::endl;

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
    int created = 0;

    if (config.consoles.size() == 0)
    {
        std::clog << "no consoles to create" << std::endl;
        return;
    }

    console_lib = new Library("libr9_console" LT_MODULE_EXT);
    console_create =
        (console_create_t *)console_lib->symbol("console_create");
    console_destroy =
        (console_destroy_t *)console_lib->symbol("console_destroy");

    for (auto& i : config.consoles)
    {
        try
        {
            consoles.push_back(console_create(i));
            ++created;
        }
        catch (std::exception& e)
        {
            while (consoles.size())
            {
                console_destroy(consoles.back());
                consoles.pop_back();
            }
            delete console_lib;
            console_lib = NULL;
            throw;
        }
    }
    if (created > 0)
        std::clog << "created " << created << " console socket"
                  << (created == 1 ? "" : "s") << std::endl;
}

static void cleanup_console(void)
{
    struct stat st;

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
    catch (std::exception& e) {}

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
        catch (std::exception& e) {}
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

    if (config.daemonize)
    {
        Log *l = dynamic_cast<Log *>(std::clog.rdbuf());

        if (l != NULL)
            l->close();
    }
}

static void cleanup_daemon(void)
{
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
