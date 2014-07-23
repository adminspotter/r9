/* cache.h                                                 -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 23 Jul 2014, 17:54:53 tquirk
 *
 * Revision IX game client
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
 * This file contains a template class for doing cache hashes for
 * arbitrary types of objects.  It was created from the geometry and
 * texture caches.
 *
 * Changes
 *   20 Jul 2014 TAQ - Created the file.
 *   23 Jul 2014 TAQ - The parser is now being spawned correctly.  Also added
 *                     a typedef for the cleanup function objects.
 *
 * Things to do
 *
 */

#ifndef __INC_R9CLIENT_CACHE_H__
#define __INC_R9CLIENT_CACHE_H__

#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <typeinfo>
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
    typedef std::unordered_map<u_int64_t, typename _oct::object_struct> obj_map;

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
    std::string store, cache;
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
                    std::ostringstream s;
                    s << typeid(obj_type).name()
                      << " reaper thread couldn't get time of day: "
                      << strerror(errno) << " (" << errno << ")";
                    main_post_message(s.str());
                    continue;
                }
                limit.tv_sec -= _oct::PRUNE_LIFETIME;
                old_elems.clear();
                for (i = oc->objects.begin(); i != oc->objects.end(); ++i)
                    if (i->second.lastused.tv_sec >= limit.tv_sec)
                        old_elems.push_back(i);

                if (old_elems.size() > 0)
                {
                    typename _oct::clean_func_type cleanup_func = oc_cleanup();
                    for (j = old_elems.begin(); j != old_elems.end(); ++j)
                    {
                        cleanup_func((*j)->second);
                        oc->objects.erase(*j);
                    }
                    std::ostringstream s;
                    s << "Removed " << old_elems.size() << " entities from "
                      << typeid(obj_type).name() << " cache";
                    main_post_message(s.str());
                }
            }
        };
    bool parse_file(const std::string& fname, u_int64_t objid)
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
            catch (std::exception& e)
            {
                std::ostringstream s;
                s << "Couldn't load " << typeid(obj_type).name()
                  << " data file: " << e.what();
                main_post_message(s.str());
                retval = false;
            }
            if (parser != NULL)
                delete parser;
            if (fp != NULL)
                delete fp;
            return retval;
        };

  public:
    ObjectCache()
        : objects(), store(), cache()
        {
            int ret;

#ifndef STORE_PREFIX
#define STORE_PREFIX "/usr/share/r9/"
#endif
            this->store = STORE_PREFIX;
            this->store += typeid(obj_type).name();

            this->cache = getenv("HOME");
            this->cache += "/.revision9/";
            this->cache += typeid(obj_type).name();

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
                s << "Couldn't start " << typeid(obj_type).name()
                  << " reaper thread: " << strerror(ret) << " (" << ret << ")";
                throw std::runtime_error(s.str());
            }
        };
    ~ObjectCache()
        {
            int ret;
            typename _oct::obj_map::iterator i;

            if ((ret = pthread_cancel(this->prune_thread)) != 0)
            {
                std::ostringstream s;
                s << "Couldn't cancel " << typeid(obj_type).name()
                  << " reaper thread: " << strerror(ret) << " (" << ret << ")";
                main_post_message(s.str());
            }
            sleep(0);
            if ((ret = pthread_join(this->prune_thread, NULL)) != 0)
            {
                std::ostringstream s;
                s << "Couldn't reap " << typeid(obj_type).name()
                  << " reaper thread: " << strerror(ret) << " (" << ret << ")";
                main_post_message(s.str());
            }

            typename _oct::clean_func_type cleanup_func = oc_cleanup();

            for (i = this->objects.begin(); i != this->objects.end(); ++i)
                cleanup_func(i->second);
        };

    void load(u_int64_t objid)
        {
            /* Look in the user cache, since it could possibly be updated more
             * recently than the system store.
             */
            std::ostringstream user_fname;
            user_fname << this->cache << '/' << std::hex
                       << (objid & 0xFF) << '/' << objid << ".xml";
            if (this->parse_file(user_fname.str(), objid))
                return;

            /* Didn't find it in the user cache; now look in the system store */
            std::ostringstream store_fname;
            store_fname << this->store << '/' << std::hex
                        << (objid & 0xFF) << '/' << objid << ".xml";
            if (this->parse_file(store_fname.str(), objid))
                return;

            /* If we can't find the object and have to request it, we
             * should have some sort of fallback to use in the
             * meantime before we receive the new stuff.
             */
            /*send_object_request(objid);*/
        };
    obj_type& operator[](u_int64_t objid)
        {
            typename _oct::obj_map::iterator object = this->objects.find(objid);

            if (object == this->objects.end())
            {
                this->load(objid);
                object = this->objects.find(objid);
            }
            gettimeofday(&(object->second.lastused), NULL);
            return object->second.obj;
        };
    void erase(u_int64_t objid)
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
