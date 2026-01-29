/* control.h                                               -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game client
 * Copyright (C) 2019-2026  Trinity Annabelle Quirk
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
 * This file contains the declaration of the base control class for
 * the R9 client.  Subclasses will implement any setup and event
 * handling for their respective control devices.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_CONTROL_H__
#define __INC_R9CLIENT_CONTROL_H__

#include <config.h>

#ifdef HAVE_INOTIFY
#include <pthread.h>
#endif /* HAVE_INOTIFY */

#include <vector>
#include <string>
#include <tuple>
#include <map>
#include <functional>

#include <cuddly-gl/active.h>

#include "../comm.h"

class control
{
  public:
    control() {};
    virtual ~control() {};

    virtual void setup(ui::active *, Comm **) {};
    virtual void cleanup(ui::active *, Comm **) {};
};

#ifdef HAVE_INOTIFY
class inotify_watcher;
#endif /* HAVE_INOTIFY */

class control_factory
{
  private:
    typedef bool (*can_use_func_t)(const char *);
    typedef control *(*create_func_t)(const char *);
    typedef std::tuple<void *, can_use_func_t, create_func_t> lib_funcs_t;
    typedef std::map<std::string, lib_funcs_t> lib_map_t;
    typedef lib_map_t::iterator lib_iter_t;

    lib_map_t lib_map;
    std::map<std::string, control *> device_map;

    lib_iter_t find_lib_entry(const std::string&);

#ifdef HAVE_INOTIFY
    pthread_t device_thread;

    typedef std::tuple<control_factory *, ui::active *, Comm **> worker_args;

    void startup_inotify(ui::active *, Comm **);
    void shutdown_inotify(void);
    static void *device_worker(void *);

#endif /* HAVE_INOTIFY */

  public:
    control_factory();
    ~control_factory();

    std::vector<std::string> types(void);
    const std::vector<std::string> devices(void);

    void start(ui::active *, Comm **);
    void stop(ui::active *, Comm **);

    void new_device(const char *, ui::active *, Comm **);
    control *create(const std::string&, const char *);
};

#ifdef HAVE_INOTIFY
class inotify_watcher
{
  public:
    typedef std::map<
        uint32_t,
        std::function<void(const std::string&)>
    > watcher_map_t;

  private:
    std::string watch_path;
    uint32_t watch_mask;

    int notify_fd, notify_watch;

    void setup_watch(const std::string&, const watcher_map_t&);
    void cleanup_watch(void);

  public:
    inotify_watcher();
    ~inotify_watcher();

    void watch(const std::string&, watcher_map_t&);
};
#endif /* HAVE_INOTIFY */

#endif /* __INC_R9CLIENT_CONTROL_H__ */
