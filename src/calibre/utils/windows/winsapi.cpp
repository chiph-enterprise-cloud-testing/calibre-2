/*
 * winsapi.cpp
 * Copyright (C) 2020 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPL3 license.
 */

#define _ATL_APARTMENT_THREADED
#include "common.h"

#include <atlbase.h>
extern CComModule _Module;
#include <atlcom.h>

#include <sapi.h>
#pragma warning( push )
#pragma warning( disable : 4996 )  // sphelper.h uses deprecated GetVersionEx
#include <sphelper.h>
#pragma warning( pop )

typedef struct {
    PyObject_HEAD
        ISpVoice *voice;
} Voice;


static PyTypeObject VoiceType = {
    PyVarObject_HEAD_INIT(NULL, 0)
};

static PyObject *
Voice_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    HRESULT hr = CoInitialize(NULL);
    if (hr != S_OK && hr != S_FALSE) {
        if (hr == RPC_E_CHANGED_MODE) {
            return error_from_hresult(hr, "COM initialization failed as it was already initialized in multi-threaded mode");
        }
        return PyErr_NoMemory();
    }
	Voice *self = (Voice *) type->tp_alloc(type, 0);
    if (self) {
        if (FAILED(hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&self->voice))) {
            Py_CLEAR(self);
            return error_from_hresult(hr, "Failed to create ISpVoice instance");
        }

    }
    return (PyObject*)self;
}

static void
Voice_dealloc(Voice *self) {
    if (self->voice) { self->voice->Release(); self->voice = NULL; }
    CoUninitialize();
}


static PyObject*
Voice_get_all_voices(Voice *self, PyObject *args) {
    HRESULT hr = S_OK;
    CComPtr<IEnumSpObjectTokens> iterator = NULL;
    if (FAILED(hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &iterator))) {
        return error_from_hresult(hr, "Failed to create voice category iterator");
        return NULL;
    }
    pyobject_raii ans(PyList_New(0));
    if (!ans) return NULL;
    while (true) {
        CComPtr<ISpObjectToken> token = NULL;
        if (FAILED(hr = iterator->Next(1, &token, NULL)) || hr == S_FALSE || !token) break;
        pyobject_raii dict(PyDict_New());
        if (!dict) return NULL;

        com_wchar_raii id, description;
        if (FAILED(hr = token->GetId(id.address()))) continue;
        pyobject_raii idpy(PyUnicode_FromWideChar(id.ptr(), -1));
        if (!idpy) return NULL;
        if (PyDict_SetItemString(dict.ptr(), "id", idpy.ptr()) != 0) return NULL;

        if (FAILED(hr = SpGetDescription(token, description.address(), NULL))) continue;
        pyobject_raii descriptionpy(PyUnicode_FromWideChar(description.ptr(), -1));
        if (!descriptionpy) return NULL;
        if (PyDict_SetItemString(dict.ptr(), "description", descriptionpy.ptr()) != 0) return NULL;
        CComPtr<ISpDataKey> attributes = NULL;
        if (FAILED(hr = token->OpenKey(L"Attributes", &attributes))) continue;
#define ATTR(name) {\
    com_wchar_raii val; \
    if (SUCCEEDED(attributes->GetStringValue(TEXT(#name), val.address()))) { \
        pyobject_raii pyval(PyUnicode_FromWideChar(val.ptr(), -1)); if (!pyval) return NULL; \
        if (PyDict_SetItemString(dict.ptr(), #name, pyval.ptr()) != 0) return NULL; \
    }\
}
        ATTR(gender); ATTR(name); ATTR(vendor); ATTR(age);
#undef ATTR
        com_wchar_raii val;
        if (SUCCEEDED(attributes->GetStringValue(L"language", val.address()))) {
            int lcid = wcstol(val.ptr(), NULL, 16);
            wchar_t buf[LOCALE_NAME_MAX_LENGTH];
            if (LCIDToLocaleName(lcid, buf, LOCALE_NAME_MAX_LENGTH, 0) > 0) {
                pyobject_raii pyval(PyUnicode_FromWideChar(buf, -1)); if (!pyval) return NULL;
                if (PyDict_SetItemString(dict.ptr(), "language", pyval.ptr()) != 0) return NULL;
            }
        }
        if (PyList_Append(ans.ptr(), dict.ptr()) != 0) return NULL;
    }
    return ans.detach();
}


#define M(name, args) { #name, (PyCFunction)Voice_##name, args, ""}
static PyMethodDef Voice_methods[] = {
    M(get_all_voices, METH_NOARGS),
    {NULL, NULL, 0, NULL}
};
#undef M

#define M(name, args) { #name, name, args, ""}
static PyMethodDef winsapi_methods[] = {
    {NULL, NULL, 0, NULL}
};
#undef M

static struct PyModuleDef winsapi_module = {
    /* m_base     */ PyModuleDef_HEAD_INIT,
    /* m_name     */ "winsapi",
    /* m_doc      */ "SAPI wrapper",
    /* m_size     */ -1,
    /* m_methods  */ winsapi_methods,
    /* m_slots    */ 0,
    /* m_traverse */ 0,
    /* m_clear    */ 0,
    /* m_free     */ 0,
};



extern "C" {

CALIBRE_MODINIT_FUNC PyInit_winsapi(void) {
    VoiceType.tp_name = "winsapi.Voice";
    VoiceType.tp_doc = "Wrapper for ISpVoice";
    VoiceType.tp_basicsize = sizeof(Voice);
    VoiceType.tp_itemsize = 0;
    VoiceType.tp_flags = Py_TPFLAGS_DEFAULT;
    VoiceType.tp_new = Voice_new;
    VoiceType.tp_methods = Voice_methods;
	VoiceType.tp_dealloc = (destructor)Voice_dealloc;
	if (PyType_Ready(&VoiceType) < 0) return NULL;

    PyObject *m = PyModule_Create(&winsapi_module);
    if (m == NULL) return NULL;

	Py_INCREF(&VoiceType);
    if (PyModule_AddObject(m, "Voice", (PyObject *) &VoiceType) < 0) {
        Py_DECREF(&VoiceType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

}
