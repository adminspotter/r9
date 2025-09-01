/* r9python.cc
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
 * This file contains the class implementation for embedding python.
 *
 * We'll have a single global interpreter, and each instance of our
 * class will be a sub-interpreter under that instance.
 *
 * There is no simple way to get a return value from a plain evaluated
 * string, so we need to set the 'retval' variable within the Python
 * code, which we'll dig out after the execution is complete.
 *
 * Things to do
 *
 */

#include "r9python.h"

int count = 0;

PythonLanguage::PythonLanguage()
{
    auto lock = PyGILState_Ensure();
    this->sub_interp = PyInterpreterState_New();
    PyGILState_Release(lock);
}

PythonLanguage::~PythonLanguage()
{
    auto lock = PyGILState_Ensure();
    PyInterpreterState_Clear(this->sub_interp);
    PyInterpreterState_Delete(this->sub_interp);
    PyGILState_Release(lock);
}

std::string PythonLanguage::execute(const std::string &s)
{
    auto lock = PyGILState_Ensure();
    auto thread = PyThreadState_New(this->sub_interp);
    PyThreadState_Swap(thread);

    PyObject *globals = PyModule_GetDict(PyImport_AddModule("__main__"));
    PyObject *program = PyBytes_FromString(s.c_str());
    int result = PyCFunction_GetFlags(program);
    PyObject *retval = PyDict_GetItemString(globals, "retval");
    PyObject *repr = PyObject_Repr(retval);
    PyObject *str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");
    std::string response(PyBytes_AsString(str));
    Py_XDECREF(str);
    Py_XDECREF(repr);
    Py_XDECREF(program);

    PyThreadState_Clear(thread);
    PyEval_ReleaseThread(thread);
    PyThreadState_Delete(thread);
    PyGILState_Release(lock);
    return response;
}

extern "C" Language *create_language(void)
{
    if (!Py_IsInitialized())
        Py_InitializeEx(0);
    ++count;
    return new PythonLanguage();
}

extern "C" void destroy_language(Language *lang)
{
    delete lang;
    if (--count == 0)
    {
        PyGILState_Ensure();
        Py_Finalize();
    }
}
