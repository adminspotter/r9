/* cache.h                                                 -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 05 Aug 2015, 15:08:18 tquirk
 *
 * Revision IX game client
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
 * This file contains a template class for doing cache hashes for
 * arbitrary types of objects.  It was created from the geometry and
 * texture caches.
 *
 * Things to do
 *   - See if we can nab the config directory out of the config object when
 *     we try to create our cache directory string.
 *   - We can do the resource request in here, I think.  Another
 *     static method, which is spawned as a new thread, which grabs
 *     the resource from whatever server, and saves it into the user's
 *     cache directory.  Or maybe also loads it into the memory cache.
 *
 */

#ifndef __INC_R9CLIENT_CACHE_H__
#define __INC_R9CLIENT_CACHE_H__

#include <config.h>

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <time.h>
#if HAVE_SYS/TIME_H
#include <sys/time.h>
#endif /* HAVE_SYS/TIME_H */
#if HAVE_SYS/STAT_H
#include <sys/stat.h>
#endif /* HAVE_SYS/STAT_H */
#include <pthread.h>

#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/SAXException.hpp>

#include <cstdint>
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <typeinfo>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include "r9client.h"

#define XNS XERCES_CPP_NAMESPACE

template <typename T>
struct noop_cleanup
{
    void operator()(T& o) { return; }
};

template <typename obj_type,
          typename parser_type,
          typename cleanup = noop_cleanup<obj_type> >
class ObjectCache
{
  private:
    typedef ObjectCache<obj_type, parser_type, cleanup> _oct;
    typedef struct object_timeval_tag
    {
        struct timeval lastused;
        obj_type obj;
    }
    object_struct;
    typedef std::unordered_map<uint64_t, typename _oct::object_struct> obj_map;

    struct oc_cleanup
    {
        void operator()(typename _oct::object_struct& val)
            {
                std::function<void(obj_type&)> cleanup_func = cleanup();
                cleanup_func(val.obj);
            }
    };
    typedef std::function<void(typename _oct::object_struct&)> clean_func_type;

    const static int PRUNE_INTERVAL = 600;
    const static int PRUNE_LIFETIME = 1200;

    _oct::obj_map objects;
    std::string type, store, cache;
    pthread_t prune_thread;

    static void *prune_worker(void *arg)
        {
            _oct *oc = (_oct *)arg;
            struct timeval limit;
            typename _oct::obj_map::iterator i;
            std::vector<typename _oct::obj_map::iterator> old_elems;
            typename std::vector<typename _oct::obj_map::iterator>::iterator j;

            for (;;)
            {
                sleep(_oct::PRUNE_INTERVAL);
                if (!gettimeofday(&limit, NULL))
                {
                    std::clog << oc->type
                              << " reaper thread couldn't get time of day: "
                              << strerror(errno) << " (" << errno << ")"
                              << std::endl;
                    continue;
                }
                limit.tv_sec -= _oct::PRUNE_LIFETIME;
                old_elems.clear();
                for (i = oc->objects.begin(); i != oc->objects.end(); ++i)
                    if (i->first != 0LL
                        && i->second.lastused.tv_sec >= limit.tv_sec)
                        old_elems.push_back(i);

                if (old_elems.size() > 0)
                {
                    typename _oct::clean_func_type cleanup_func = oc_cleanup();
                    for (j = old_elems.begin(); j != old_elems.end(); ++j)
                    {
                        cleanup_func((*j)->second);
                        oc->objects.erase(*j);
                    }
                    std::clog << "Removed " << old_elems.size()
                              << " entities from " << oc->type
                              << " cache" << std::endl;
                }
            }
        };
    bool parse_file(const std::string& fname, uint64_t objid)
        {
            bool retval = true;
            XNS::SAXParser *parser = NULL;
            parser_type *fp = NULL;

            try
            {
                XNS::XMLPlatformUtils::Initialize();
                parser = new XNS::SAXParser();
                parser->setValidationScheme(XNS::SAXParser::Val_Auto);
                parser->setValidationSchemaFullChecking(true);
                parser->setDoNamespaces(true);
                fp = new parser_type(&(this->objects[objid].obj));
                parser->setDocumentHandler((XNS::DocumentHandler *)fp);
                parser->setErrorHandler((XNS::ErrorHandler *)fp);
                parser->parse(fname.c_str());
            }
            catch (XNS::SAXException& s)
            {
                std::clog << "Couldn't load " << this->type
                          << " data file: " << s.getMessage() << std::endl;
            }
            catch (std::exception& e)
            {
                std::clog << "Couldn't load " << this->type
                          << " data file: " << e.what() << std::endl;
                retval = false;
            }
            if (parser != NULL)
                delete parser;
            if (fp != NULL)
                delete fp;
            return retval;
        };

  public:
    ObjectCache(const std::string type_name)
        : objects(), type(type_name), store(), cache()
        {
            int ret;

            this->store = STORE_PREFIX;
            this->store += '/';
            this->store += this->type;

            this->cache = getenv("HOME");
            this->cache += "/.r9/";
            this->cache += this->type;

            /* We might consider doing a simple file mtime comparison
             * between everything in the cache and everything in the
             * store here, and deleting stuff from the cache which is
             * newer in the store.
             */

            if ((ret = pthread_create(&(this->prune_thread),
                                      NULL,
                                      this->prune_worker,
                                      (void *)this)) != 0)
            {
                std::ostringstream s;
                s << "Couldn't start " << this->type
                  << " reaper thread: " << strerror(ret) << " (" << ret << ")";
                throw std::runtime_error(s.str());
            }

            this->load(0LL);
        };
    ~ObjectCache()
        {
            int ret;
            typename _oct::obj_map::iterator i;

            if ((ret = pthread_cancel(this->prune_thread)) != 0)
                std::clog << "Couldn't cancel " << this->type
                          << " reaper thread: "
                          << strerror(ret) << " (" << ret << ")" << std::endl;
            sleep(0);
            if ((ret = pthread_join(this->prune_thread, NULL)) != 0)
                std::clog << "Couldn't reap " << this->type
                          << " reaper thread: "
                          << strerror(ret) << " (" << ret << ")" << std::endl;

            typename _oct::clean_func_type cleanup_func = oc_cleanup();

            for (i = this->objects.begin(); i != this->objects.end(); ++i)
                cleanup_func(i->second);
        };

    void load(uint64_t objid)
        {
            struct stat st;

            /* Look in the user cache, since it could possibly be updated more
             * recently than the system store.
             */
            std::ostringstream user_fname;
            user_fname << this->cache << '/';
            user_fname.fill('0');
            user_fname.width(2);
            user_fname << std::hex << std::right
                       << (objid & 0xFF) << '/';
            user_fname.width(16);
            user_fname << objid << ".xml";
            if (stat(user_fname.str().c_str(), &st) != -1)
                if (this->parse_file(user_fname.str(), objid))
                    return;

            /* Didn't find it in the user cache; now look in the system store */
            std::ostringstream store_fname;
            store_fname << this->store << '/';
            store_fname.fill('0');
            store_fname.width(2);
            store_fname << std::hex << std::right
                        << (objid & 0xFF) << '/';
            store_fname.width(16);
            store_fname << objid << ".xml";
            if (stat(store_fname.str().c_str(), &st) != -1)
                if (this->parse_file(store_fname.str(), objid))
                    return;

            /* If we can't find the object and have to request it, we
             * should have some sort of fallback to use in the
             * meantime before we receive the new stuff.
             */
            /*send_object_request(objid);*/
        };
    obj_type& operator[](uint64_t objid)
        {
            typename _oct::obj_map::iterator object = this->objects.find(objid);

            if (object == this->objects.end())
            {
                /* Maybe spawn another thread to do the load? */
                this->load(objid);
                object = this->objects.find(0LL);
            }
            gettimeofday(&(object->second.lastused), NULL);
            return object->second.obj;
        };
    void erase(uint64_t objid)
        {
            typename _oct::obj_map::iterator object = this->objects.find(objid);

            if (object != this->objects.end())
            {
                typename _oct::clean_func_type cleanup_func = oc_cleanup();

                cleanup_func(object->second);
                this->objects.erase(object);
            }
        };
};

#endif /* __INC_R9CLIENT_CACHE_H__ */
