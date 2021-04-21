/*
 * Copyright (C) 2020 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPL3 license.
 */

#pragma once
#define PY_SSIZE_T_CLEAN
#define UNICODE
#define _UNICODE
#include <Windows.h>
#include <Python.h>
#include <comdef.h>
#include "../cpp_binding.h"

static inline PyObject*
set_error_from_hresult(PyObject *exc_type, const char *file, const int line, const HRESULT hr, const char *prefix="", PyObject *name=NULL) {
    _com_error err(hr);
    PyObject *pmsg = PyUnicode_FromWideChar(err.ErrorMessage(), -1);
    if (name) PyErr_Format(exc_type, "%s:%d:%s:[%li] %V: %S", file, line, prefix, hr, pmsg, "Out of memory", name);
    else PyErr_Format(exc_type, "%s:%d:%s:[%li] %V", file, line, prefix, hr, pmsg, "Out of memory");
    Py_CLEAR(pmsg);
    return NULL;
}
#define error_from_hresult(hr, ...) set_error_from_hresult(PyExc_OSError, __FILE__, __LINE__, hr, __VA_ARGS__)

typedef generic_raii<wchar_t*, CoTaskMemFree, NULL> com_wchar_raii;
static inline void handle_destructor(HANDLE p) { CloseHandle(p); }
typedef generic_raii<HANDLE, handle_destructor, INVALID_HANDLE_VALUE> handle_raii;
