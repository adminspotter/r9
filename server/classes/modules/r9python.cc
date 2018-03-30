/* r9python.cc
 *   by Trinity Quirk <tquirk@ymb.net>
 *   last updated 29 Mar 2018, 23:48:12 tquirk
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
    PyGILState_STATE lock = PyGILState_Ensure();
    PyThreadState *tmp = PyThreadState_Get();
    this->sub_interp = Py_NewInterpreter();
    PyThreadState_Swap(tmp);
    PyGILState_Release(lock);
}

PythonLanguage::~PythonLanguage()
{
    PyGILState_STATE lock = PyGILState_Ensure();
    PyThreadState *tmp = PyThreadState_Swap(this->sub_interp);
    Py_EndInterpreter(this->sub_interp);
    PyThreadState_Swap(tmp);
    PyGILState_Release(lock);
}

std::string PythonLanguage::execute(const std::string &s)
{
    PyThreadState *thread = PyThreadState_New(this->sub_interp->interp);
    if (!PyGILState_Check())
        PyGILState_Ensure();
    PyThreadState_Swap(thread);

    PyObject *globals = PyModule_GetDict(PyImport_AddModule("__main__"));
    PyObject *result = PyRun_StringFlags(s.c_str(),
                                         Py_file_input,
                                         globals, globals, NULL);
    PyObject *retval = PyDict_GetItemString(globals, "retval");
    PyObject *repr = PyObject_Repr(retval);
    PyObject *str = PyUnicode_AsEncodedString(repr, "utf-8", "~E~");
    std::string response(PyBytes_AS_STRING(str));
    Py_XDECREF(str);
    Py_XDECREF(repr);
    Py_XDECREF(result);

    PyThreadState_Clear(thread);
    PyEval_ReleaseThread(thread);
    PyThreadState_Delete(thread);
    return response;
}

extern "C" Language *create_language(void)
{
    if (!Py_IsInitialized())
    {
        Py_InitializeEx(0);
        PyEval_InitThreads();

        /* The GIL is held at this point.  Not sure how to release it
         * without having previously gotten a PyGILState_STATE
         * object.
         */
    }
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
