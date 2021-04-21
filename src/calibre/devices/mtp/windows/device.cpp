/*
 * device.cpp
 * Copyright (C) 2012 Kovid Goyal <kovid at kovidgoyal.net>
 *
 * Distributed under terms of the GPL3 license.
 */

#include "global.h"

using namespace wpd;
// Device.__init__() {{{
static void
dealloc(Device* self)
{
	self->pnp_id.release();
    if (self->bulk_properties) self->bulk_properties.Release();

    if (self->device) {
        Py_BEGIN_ALLOW_THREADS;
        self->device->Close();
		self->device.Release();
        Py_END_ALLOW_THREADS;
    }

	self->device_information.release();
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static int
init(Device *self, PyObject *args, PyObject *kwds)
{
    int ret = -1;
    if (!PyArg_ParseTuple(args, "O&", py_to_wchar_no_none, &self->pnp_id)) return -1;
    self->bulk_properties.Release();
    CComPtr<IPortableDeviceValues> client_information = get_client_information();
    if (client_information) {
        self->device = open_device(self->pnp_id.ptr(), client_information);
        if (self->device) {
            self->device_information.attach(get_device_information(self->device, self->bulk_properties));
            if (self->device_information) ret = 0;
        }
    }
    return ret;
}

// }}}

// update_device_data() {{{
static PyObject*
update_data(Device *self, PyObject *args) {
    PyObject *di = NULL;
    CComPtr<IPortableDevicePropertiesBulk> bulk_properties;
    di = get_device_information(self->device, bulk_properties);
    if (di == NULL) return NULL;
    self->device_information.attach(di);
    Py_RETURN_NONE;
} // }}}

// get_filesystem() {{{
static PyObject*
py_get_filesystem(Device *self, PyObject *args) {
    PyObject *storage_id, *ret, *callback;
    wchar_t *storage;

    if (!PyArg_ParseTuple(args, "OO", &storage_id, &callback)) return NULL;
    if (!PyCallable_Check(callback)) { PyErr_SetString(PyExc_TypeError, "callback is not a callable"); return NULL; }
    storage = unicode_to_wchar(storage_id);
    if (storage == NULL) return NULL;

    ret = wpd::get_filesystem(self->device, storage, self->bulk_properties, callback);
    free(storage);
    return ret;
} // }}}

// get_file() {{{
static PyObject*
py_get_file(Device *self, PyObject *args) {
    PyObject *object_id, *stream, *callback = NULL, *ret;
    wchar_t *object;

    if (!PyArg_ParseTuple(args, "OO|O", &object_id, &stream, &callback)) return NULL;
    object = unicode_to_wchar(object_id);
    if (object == NULL) return NULL;

    if (callback == NULL || !PyCallable_Check(callback)) callback = NULL;

    ret = wpd::get_file(self->device, object, stream, callback);
    free(object);
    return ret;
} // }}}

// create_folder() {{{
static PyObject*
py_create_folder(Device *self, PyObject *args) {
    PyObject *pparent_id, *pname, *ret;
    wchar_t *parent_id, *name;

    if (!PyArg_ParseTuple(args, "OO", &pparent_id, &pname)) return NULL;
    parent_id = unicode_to_wchar(pparent_id);
    name = unicode_to_wchar(pname);
    if (parent_id == NULL || name == NULL) return NULL;

    ret = wpd::create_folder(self->device, parent_id, name);
    free(parent_id); free(name);
    return ret;
} // }}}

// delete_object() {{{
static PyObject*
py_delete_object(Device *self, PyObject *args) {
    PyObject *pobject_id, *ret;
    wchar_t *object_id;

    if (!PyArg_ParseTuple(args, "O", &pobject_id)) return NULL;
    object_id = unicode_to_wchar(pobject_id);
    if (object_id == NULL) return NULL;

    ret =  wpd::delete_object(self->device, object_id);
    free(object_id);
    return ret;
} // }}}

// get_file() {{{
static PyObject*
py_put_file(Device *self, PyObject *args) {
    PyObject *pparent_id, *pname, *stream, *callback = NULL, *ret;
    wchar_t *parent_id, *name;
    unsigned long long size;

    if (!PyArg_ParseTuple(args, "OOOK|O", &pparent_id, &pname, &stream, &size, &callback)) return NULL;
    parent_id = unicode_to_wchar(pparent_id);
    name = unicode_to_wchar(pname);
    if (parent_id == NULL || name == NULL) return NULL;

    if (callback == NULL || !PyCallable_Check(callback)) callback = NULL;

    ret = wpd::put_file(self->device, parent_id, name, stream, size, callback);
    free(parent_id); free(name);
    return ret;
} // }}}

static PyMethodDef Device_methods[] = {
    {"update_data", (PyCFunction)update_data, METH_VARARGS,
     "update_data() -> Reread the basic device data from the device (total, space, free space, storage locations, etc.)"
    },

    {"get_filesystem", (PyCFunction)py_get_filesystem, METH_VARARGS,
     "get_filesystem(storage_id, callback) -> Get all files/folders on the storage identified by storage_id. Tries to use bulk operations when possible. callback must be a callable that is called as (object, level). It is called with every found object. If the callback returns False and the object is a folder, it is not recursed into."
    },

    {"get_file", (PyCFunction)py_get_file, METH_VARARGS,
     "get_file(object_id, stream, callback=None) -> Get the file identified by object_id from the device. The file is written to the stream object, which must be a file like object. If callback is not None, it must be a callable that accepts two arguments: (bytes_read, total_size). It will be called after each chunk is read from the device. Note that it can be called multiple times with the same values."
    },

    {"create_folder", (PyCFunction)py_create_folder, METH_VARARGS,
     "create_folder(parent_id, name) -> Create a folder. Returns the folder metadata."
    },

    {"delete_object", (PyCFunction)py_delete_object, METH_VARARGS,
     "delete_object(object_id) -> Delete the object identified by object_id. Note that trying to delete a non-empty folder will raise an error."
    },

    {"put_file", (PyCFunction)py_put_file, METH_VARARGS,
     "put_file(parent_id, name, stream, size_in_bytes, callback=None) -> Copy a file from the stream object, creating a new file on the device with parent identified by parent_id. Returns the file metadata of the newly created file. callback should be a callable that accepts two argument: (bytes_written, total_size). It will be called after each chunk is written to the device. Note that it can be called multiple times with the same arguments."
    },

    {NULL}
};

// Device.data {{{
static PyObject *
Device_data(Device *self, void *closure) {
	PyObject *ans = self->device_information.ptr();
	Py_INCREF(ans); return ans;
} // }}}


static PyGetSetDef Device_getsetters[] = {
    {(char *)"data",
     (getter)Device_data, NULL,
     (char *)"The basic device information.",
     NULL},

    {NULL}  /* Sentinel */
};


PyTypeObject wpd::DeviceType = { // {{{
    PyVarObject_HEAD_INIT(NULL, 0)
    /* tp_name           */ "wpd.Device",
    /* tp_basicsize      */ sizeof(Device),
    /* tp_itemsize       */ 0,
    /* tp_dealloc        */ (destructor)dealloc,
    /* tp_print          */ 0,
    /* tp_getattr        */ 0,
    /* tp_setattr        */ 0,
    /* tp_compare        */ 0,
    /* tp_repr           */ 0,
    /* tp_as_number      */ 0,
    /* tp_as_sequence    */ 0,
    /* tp_as_mapping     */ 0,
    /* tp_hash           */ 0,
    /* tp_call           */ 0,
    /* tp_str            */ 0,
    /* tp_getattro       */ 0,
    /* tp_setattro       */ 0,
    /* tp_as_buffer      */ 0,
    /* tp_flags          */ Py_TPFLAGS_DEFAULT|Py_TPFLAGS_BASETYPE,
    /* tp_doc            */ "Device",
    /* tp_traverse       */ 0,
    /* tp_clear          */ 0,
    /* tp_richcompare    */ 0,
    /* tp_weaklistoffset */ 0,
    /* tp_iter           */ 0,
    /* tp_iternext       */ 0,
    /* tp_methods        */ Device_methods,
    /* tp_members        */ 0,
    /* tp_getset         */ Device_getsetters,
    /* tp_base           */ 0,
    /* tp_dict           */ 0,
    /* tp_descr_get      */ 0,
    /* tp_descr_set      */ 0,
    /* tp_dictoffset     */ 0,
    /* tp_init           */ (initproc)init,
    /* tp_alloc          */ 0,
    /* tp_new            */ 0,
}; // }}}
