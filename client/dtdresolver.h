/* dtdresolver.h                                           -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 01 Dec 2015, 08:23:18 tquirk
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
 * This file contains the entity resolver, which should load our DTD
 * files from their locations in the filesystem.
 *
 * Things to do
 *
 */

#ifndef __INC_DTDRESOLVER_H__
#define __INC_DTDRESOLVER_H__

#include <xercesc/sax/EntityResolver.hpp>
#include <xercesc/framework/LocalFileInputSource.hpp>
#include <xercesc/util/XMLString.hpp>
#ifndef XNS
#define XNS  XERCES_CPP_NAMESPACE
#endif /* XNS */

class R9Resolver : public XNS::EntityResolver
{
  public:
    const char *dtd_path, *dtd_name;

    XNS::InputSource *resolveEntity(const XMLCh *const pub_id,
                                    const XMLCh *const sys_id)
        {
            char *sys_string = XNS::XMLString::transcode(sys_id);

            if (strcmp(this->dtd_name, sys_string) == 0)
            {
                XMLCh *path = XNS::XMLString::transcode(this->dtd_path);
                XMLCh *name = XNS::XMLString::transcode(this->dtd_name);
                return new XNS::LocalFileInputSource(path, name);
            }
            return NULL;
        };
};

#endif /* __INC_DTDRESOLVER_H__ */
