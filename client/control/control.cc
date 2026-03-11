/* control.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game client
 * Copyright (C) 2021-2026  Trinity Annabelle Quirk
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
 * This file contains the control factory for the R9 client.
 *
 * Things to do
 *
 */

#include <config.h>

#include <glob.h>
#include <dlfcn.h>

#ifdef HAVE_INOTIFY
#include <unistd.h>
#include <fcntl.h>
#include <sys/inotify.h>
#include <pthread.h>
#endif /* HAVE_INOTIFY */

#include <iostream>
#include <sstream>
#include <stdexcept>

#include "control.h"
#include "l10n.h"

#define DEVICE_PATH  "/dev/input/by-id"

static std::string glob_error_to_string(int err)
{
    switch (err)
    {
      case 1:   return translate("Error message", "Out of memory");
      case 2:   return translate("Error message", "Read error");
      case 3:   return translate("Error message", "No match found");
      case 4:   return translate("Error message", "Function not implemented");
    }
    return translate("Error message", "Unknown error");
}

control_factory::lib_iter_t control_factory::find_lib_entry(
    const std::string& name
)
{
    void *lib;
    create_func_t create_sym;
    can_use_func_t can_use_sym;
    char *err;
    lib_iter_t entry = this->lib_map.find(name);

    if (entry == lib_map.end())
    {
        lib = dlopen(name.c_str(), RTLD_LAZY | RTLD_GLOBAL);
        if ((err = dlerror()) != NULL)
        {
            std::ostringstream s;
            s << format(
                translate("Error opening library {1,name}: {2,errmsg}")
            ) % name % err;
            throw std::runtime_error(s.str());
        }
        dlerror();
        can_use_sym = (can_use_func_t)dlsym(lib, "can_use");
        if ((err = dlerror()) != NULL)
        {
            std::ostringstream s;
            s << format(
                translate("Error finding can_use symbol: {1,errmsg}")
            ) % err;
            throw std::runtime_error(s.str());
        }
        dlerror();
        create_sym = (create_func_t)dlsym(lib, "create");
        if ((err = dlerror()) != NULL)
        {
            std::ostringstream s;
            s << format(
                translate("Error finding create symbol: {1,errmsg}")
            ) % err;
            throw std::runtime_error(s.str());
        }
        dlerror();
        this->lib_map[name] = lib_funcs_t(lib, can_use_sym, create_sym);
        entry = this->lib_map.find(name);
    }
    return entry;
}

#ifdef HAVE_INOTIFY

void control_factory::startup_inotify(ui::active *active, Comm **comm)
{
    int ret;

    worker_args args = std::make_tuple(this, active, comm);
    if ((ret = pthread_create(
             &this->device_thread, NULL, this->device_worker, &args)
        ) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error starting device notify thread: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(ret, err, sizeof(err)) % ret << std::endl;

        /* Try not to leak.  Instead of throwing, we'll just log and
         * ignore, and keep the static data we've already got.
         */
        this->shutdown_inotify();
    }
}

void control_factory::shutdown_inotify(void)
{
    int ret;

    if ((ret = pthread_cancel(this->device_thread)) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error cancelling device notify thread: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(ret, err, sizeof(err)) % ret << std::endl;
    }
    else if ((ret = pthread_join(this->device_thread, NULL)) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error joining device notify thread: {1,errmsg} ({2,errno})"
            )
        ) % strerror_r(ret, err, sizeof(err)) % ret << std::endl;
    }
}

void *control_factory::device_worker(void *args)
{
    control_factory *factory = std::get<0>(*(worker_args *)args);
    ui::active *active = std::get<1>(*(worker_args *)args);
    Comm **comm = std::get<2>(*(worker_args *)args);
    std::map<std::string, control *>& dev_map = factory->device_map;
    std::map<std::string, control *>::iterator iter;
    inotify_watcher notifier;
    inotify_watcher::watcher_map_t watch_map;

    /* Make sure we can be cancelled as we expect. */
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

    watch_map[IN_MOVED_TO] = [&](const std::string& str)
        {
            factory->new_device(str.c_str(), active, comm);
        };
    watch_map[IN_DELETE] = [&](const std::string& str)
        {
            if ((iter = dev_map.find(str)) != dev_map.end())
            {
                iter->second->cleanup(active, comm);
                delete iter->second;
                dev_map.erase(iter);
                std::clog << format(
                    translate("Removed device {1,name}")
                ) % str << std::endl;
            }
        };

    notifier.watch(DEVICE_PATH, watch_map);

    std::clog << translate("notify watcher terminated") << std::endl;
    return NULL;
}

#endif /* HAVE_INOTIFY */

control_factory::control_factory()
{
}

control_factory::~control_factory()
{
    char *errstr;

    for (auto& dev_ref : this->device_map)
        delete dev_ref.second;
    this->device_map.clear();

    for (auto& lib_ref : this->lib_map)
    {
        dlerror();
        dlclose(std::get<0>(lib_ref.second));
        if ((errstr = dlerror()) != NULL)
        {
            std::clog << format(
                translate("Error closing library {1,name}: {2,errmsg}")
            ) % lib_ref.first % errstr << std::endl;
        }
    }
}

std::vector<std::string> control_factory::types(void)
{
    int errno;
    glob_t mods;
    std::vector<std::string> mod_types;

    if ((errno = glob(CLIENT_LIB_DIR "/*" LT_MODULE_EXT, 0, NULL, &mods)) != 0)
    {
        std::ostringstream s;

        s << format(
            translate("Error finding control modules: {1,errmsg} ({2,errno})")
        ) % glob_error_to_string(errno) % errno;
        throw std::runtime_error(s.str());
    }
    for (int i = 0; i < mods.gl_pathc; ++i)
    {
        std::string mod(mods.gl_pathv[i]);
        std::string::size_type found;

        if ((found = mod.find_last_of("/")) != std::string::npos)
            mod = mod.substr(found + 1);
        if ((found = mod.find_last_of(".")) != std::string::npos)
            mod = mod.substr(0, found);
        mod_types.push_back(mod);
    }
    globfree(&mods);
    return mod_types;
}

const std::vector<std::string> control_factory::devices(void)
{
    std::vector<std::string> device_list;

    for (auto& i : this->device_map)
        device_list.push_back(i.first);

    return device_list;
}

void control_factory::start(ui::active *a, Comm **c)
{
    control *control_obj;
    int ret;
    glob_t device_glob;
    char *dev;

    if ((ret = glob(DEVICE_PATH "/*", 0, NULL, &device_glob)) != 0)
    {
        std::ostringstream s;

        s << format(
            translate("Error finding devices: {1,errmsg} ({2,errno})")
        ) % glob_error_to_string(ret) % ret;
        throw std::runtime_error(s.str());
    }

    for (int i = 0; i < device_glob.gl_pathc; ++i)
        this->new_device(device_glob.gl_pathv[i], a, c);
    globfree(&device_glob);

#ifdef HAVE_INOTIFY
    std::clog << translate("Starting inotify") << std::endl;
    this->startup_inotify(a, c);
#endif /* HAVE_INOTIFY */
}

void control_factory::stop(ui::active *a, Comm **c)
{
#ifdef HAVE_INOTIFY
    std::clog << translate("Stopping inotify") << std::endl;
    this->shutdown_inotify();
#endif /* HAVE_INOTIFY */

    for (auto& dev : this->device_map)
    {
        dev.second->cleanup(a, c);
        delete dev.second;
    }
    this->device_map.clear();
}

void control_factory::new_device(const char *dev, ui::active *a, Comm **c)
{
    if (this->device_map.find(dev) != this->device_map.end())
        return;

    for (auto& type : this->types())
    {
        control *control_obj = this->create(type, dev);
        if (control_obj != NULL)
        {
            control_obj->setup(a, c);
            this->device_map[dev] = control_obj;
            break;
        }
    }
}

control *control_factory::create(const std::string& control_type,
                                 const char *dev)
{
    std::string fname(CLIENT_LIB_DIR);

    fname += "/" + control_type + LT_MODULE_EXT;

    lib_iter_t entry = this->find_lib_entry(fname);
    if ((std::get<1>(entry->second))(dev))
        return (std::get<2>(entry->second))(dev);
    return NULL;
}

#ifdef HAVE_INOTIFY

void inotify_watcher::setup_watch(const std::string& path,
                                  const watcher_map_t& watchers)
{
    uint32_t watch_mask = 0;

    if (this->notify_watch != 0)
        this->cleanup_watch();

    for (auto& w : watchers)
        watch_mask |= w.first;

    if ((this->notify_watch = inotify_add_watch(
             this->notify_fd, path.c_str(), watch_mask
         )) < 0)
    {
        std::ostringstream s;
        char err[128];

        s << format(
            translate(
                "Error creating notify watch descriptor: "
                "{1,errmsg} ({2,errno})"
            )
        ) % strerror_r(errno, err, sizeof(err)) % errno << std::endl;
        throw std::runtime_error(s.str());
    }
}

void inotify_watcher::cleanup_watch(void)
{
    if (this->notify_watch == 0)
        return;

    if (inotify_rm_watch(this->notify_fd, this->notify_watch) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error removing notify watch descriptor: "
                "{1,errmsg} ({2,errno})"
            )
        ) % strerror_r(errno, err, sizeof(err)) % errno << std::endl;
    }
    this->notify_watch = 0;
}

inotify_watcher::inotify_watcher()
{
    this->notify_watch = 0;
    if ((this->notify_fd = inotify_init()) < 0)
    {
        std::ostringstream s;
        char err[128];

        s << format(
            translate(
                "Error creating notify file descriptor: "
                "{1,errmsg} ({2,errno})"
            )
        ) % strerror_r(errno, err, sizeof(err)) % errno << std::endl;
        throw std::runtime_error(s.str());
    }
}

inotify_watcher::~inotify_watcher()
{
    if (close(this->notify_fd) != 0)
    {
        char err[128];

        std::clog << format(
            translate(
                "Error closing notify file descriptor: "
                "{1,errmsg} ({2,errno})"
            )
        ) % strerror_r(errno, err, sizeof(err)) % errno << std::endl;
    }
}

void inotify_watcher::watch(const std::string& watch_path,
                            watcher_map_t& watches)
{
    unsigned char read_buf[sizeof(struct inotify_event) + NAME_MAX + 1];
    struct inotify_event *event = (struct inotify_event *)read_buf;
    int len;
    uint32_t watch_mask;

    this->setup_watch(watch_path, watches);
    for (;;)
    {
        memset(read_buf, 0, sizeof(read_buf));
        if ((len = read(this->notify_fd, &read_buf, sizeof(read_buf))) < 0)
        {
            char err[128];

            std::clog << format(
                translate(
                    "Error reading notification: {1,errmsg} ({2,errno})"
                )
            ) % strerror_r(errno, err, sizeof(err)) % errno << std::endl;

            continue;
        }
        else if (len == 0)
        {
            std::clog << translate("notify connection closed") << std::endl;
            break;
        }

        while (len >= sizeof(struct inotify_event) + event->len)
        {
            if (event->len > 0)
            {
                std::string path = watch_path + "/" + event->name;
                for (auto& watch : watches)
                    if (event->mask & watch.first)
                        watch.second(path);
            }
            len -= sizeof(struct inotify_event) + event->len;
        }
    }
    this->cleanup_watch();
}

#endif /* HAVE_INOTIFY */
