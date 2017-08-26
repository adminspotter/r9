/* r9python.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 22 Mar 2017, 10:14:11 tquirk
 *
 * Revision IX game server
 * Copyright (C) 2017  Trinity Annabelle Quirk
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
 * This file contains the class implementation for embedding python.
 *
 * We'll have a single global interpreter, and each instance of our
 * class will be a thread under that instance.
 *
 * Python uses the standard file descriptors for I/O, but of course
 * those will be unavailable in our context.  We'll have to have a
 * mechanism to grab hold of the output, so we can return it in our
 * execute method.  We'll want to completely turn off standard input,
 * since there will be no available source to get any.
 *
 * We'll embed the interpreters in such a way that we will remap the
 * stdin/stdout file descriptors to something that we control, based
 * on the work in:
 * https://github.com/mloskot/workshop/blob/master/python/emb/emb.cpp
 *
 * Things to do
 *
 */

#include "r9python.h"

PythonLanguage::PythonLanguage()
{
}

PythonLanguage::~PythonLanguage()
{
}

std::string PythonLanguage::execute(const std::string &s)
{
}
