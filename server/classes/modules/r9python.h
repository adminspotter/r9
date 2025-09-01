/* r9python.h                                              -*- C++ -*-
 *   by Trinity Quirk <tquirk@ymb.net>
 *
 * Revision IX game server
 * Copyright (C) 2017-2025  Trinity Annabelle Quirk
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
 * This file contains the class for embedding python.
 *
 * Things to do
 *
 */

#ifndef __INC_R9PYTHON_H__
#define __INC_R9PYTHON_H__

#include <Python.h>

#include "language.h"

class PythonLanguage : public Language
{
  private:
    PyThreadState *sub_interp;

  public:
    PythonLanguage();
    ~PythonLanguage();

    std::string execute(const std::string &);
};

#endif /* __INC_R9PYTHON_H__ */
