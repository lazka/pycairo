/* -*- mode: C; c-basic-offset: 2 -*-
 *
 * Pycairo - Python bindings for cairo
 *
 * Copyright © 2017 Christoph Reiter
 *
 * This library is free software; you can redistribute it and/or
 * modify it either under the terms of the GNU Lesser General Public
 * License version 2.1 as published by the Free Software Foundation
 * (the "LGPL") or, at your option, under the terms of the Mozilla
 * Public License Version 1.1 (the "MPL"). If you do not alter this
 * notice, a recipient may use your version of this file under either
 * the MPL or the LGPL.
 *
 * You should have received a copy of the LGPL along with this library
 * in the file COPYING-LGPL-2.1; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 * You should have received a copy of the MPL along with this library
 * in the file COPYING-MPL-1.1
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.1 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY
 * OF ANY KIND, either express or implied. See the LGPL or the MPL for
 * the specific language governing rights and limitations.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "private.h"

static PyTypeObject PycairoError_Type;
static PyObject *error_get_type_combined (
    PyObject *error, PyObject *other, char *name);

/* Like cairo_status_to_string(), but translates some C function names to
 * Python function names.
 */
static const char*
status_to_string(cairo_status_t status)
{
    if (status == CAIRO_STATUS_INVALID_RESTORE)
        return "Context.restore() without matching Context.save()";
    else if (status == CAIRO_STATUS_INVALID_POP_GROUP)
        return "Context.pop_group() without matching Context.push_group()";
    else
        return cairo_status_to_string(status);
}

/* Sets an exception based on a cairo_status_t */
static void
set_error (PyObject *error_type, cairo_status_t status)
{
    PyObject *args, *v, *int_enum;

    int_enum = CREATE_INT_ENUM(Status, status);
    if (int_enum == NULL)
        return;
    args = Py_BuildValue("(sO)", status_to_string(status), int_enum);
    Py_DECREF (int_enum);
    v = PyObject_Call(error_type, args, NULL);
    Py_DECREF(args);
    if (v != NULL) {
        PyErr_SetObject((PyObject *)Py_TYPE(v), v);
        Py_DECREF(v);
    }
}

int
Pycairo_Check_Status (cairo_status_t status) {
    PyObject *module, *suberror;

    if (PyErr_Occurred() != NULL)
        return 1;

    if (status == CAIRO_STATUS_SUCCESS)
        return 0;

    switch (status) {
        case CAIRO_STATUS_NO_MEMORY:
            suberror = error_get_type_combined (
                (PyObject *)&PycairoError_Type, PyExc_MemoryError, "cairo.MemoryError");
            set_error (suberror, status);
            Py_DECREF (suberror);
            break;
        case CAIRO_STATUS_READ_ERROR:
        case CAIRO_STATUS_WRITE_ERROR:
            suberror = error_get_type_combined (
                (PyObject *)&PycairoError_Type, PyExc_IOError, "cairo.IOError");
            set_error (suberror, status);
            Py_DECREF (suberror);
            break;
        default:
            set_error ((PyObject *)&PycairoError_Type, status);
    }

    return 1;
}

typedef struct {
    PyBaseExceptionObject base;
} PycairoErrorObject;

static PyObject *
error_get_args(PycairoErrorObject *self) {
    PyObject *args;

    args = PyObject_GetAttrString((PyObject *)self, "args");
    if (args == NULL)
        return NULL;

    if (!PyTuple_Check(args)) {
        PyErr_SetString(PyExc_TypeError, ".args not a tuple");
        Py_DECREF(args);
        return NULL;
    }

    return args;
}

static int
error_init(PycairoErrorObject *self, PyObject *args, PyObject *kwds)
{
    PyObject *status_obj, *error_args;

    if (PycairoError_Type.tp_base->tp_init((PyObject *)self, args, kwds) < 0)
        return -1;

    error_args = error_get_args(self);
    if (error_args == NULL)
        return -1;

    if(PyTuple_GET_SIZE(error_args) >= 2) {
        status_obj = PyTuple_GET_ITEM(error_args, 1);
    } else {
        status_obj = Py_None;
    }

    Py_DECREF(error_args);

    if (PyObject_SetAttrString ((PyObject *)self, "__status", status_obj) < 0)
        return -1;

    return 0;
}

static PyObject *
error_get_status(PycairoErrorObject *self, void *closure)
{
    return PyObject_GetAttrString ((PyObject *)self, "__status");
}

static int
error_set_status(PycairoErrorObject *self, PyObject *value, void *closure)
{
    if (value == NULL) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete attribute");
        return -1;
    }

    if (PyObject_SetAttrString ((PyObject *)self, "__status", value) < 0)
        return -1;

    return 0;
}

static PyGetSetDef error_getset[] = {
    {"status", (getter)error_get_status, (setter)error_set_status,},
    {NULL,},
};

static PyObject *
error_str(PycairoErrorObject *self)
{
    PyObject *result, *error_args;

    error_args = error_get_args(self);
    if (error_args == NULL)
        return NULL;

    /* Default to printing just the message */
    if (PyTuple_GET_SIZE(error_args) >= 1) {
        result = PyObject_Str(PyTuple_GET_ITEM(error_args, 0));
    } else {
        result = PycairoError_Type.tp_base->tp_str((PyObject*)self);
    }

    Py_DECREF(error_args);
    return result;
}

static PyObject *
error_check_status (PyTypeObject *type, PyObject *args) {
    cairo_status_t status;
    int status_arg;

    if (!PyArg_ParseTuple (args, "i:Error._check_status", &status_arg))
        return NULL;

    status = (cairo_status_t)status_arg;

    if (Pycairo_Check_Status (status))
        return NULL;

    Py_RETURN_NONE;
}

static PyMethodDef error_methods[] = {
  {"_check_status", (PyCFunction)error_check_status, METH_VARARGS | METH_CLASS},
  {NULL, NULL, 0, NULL},
};

static PyTypeObject PycairoError_Type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "cairo.Error",           /* tp_name */
    sizeof(PycairoErrorObject),/* tp_basicsize */
    0,                       /* tp_itemsize */
    0,                       /* tp_dealloc */
    0,                       /* tp_print */
    0,                       /* tp_getattr */
    0,                       /* tp_setattr */
    0,                       /* tp_reserved */
    0,                       /* tp_repr */
    0,                       /* tp_as_number */
    0,                       /* tp_as_sequence */
    0,                       /* tp_as_mapping */
    0,                       /* tp_hash */
    0,                       /* tp_call */
    (reprfunc)error_str,     /* tp_str */
    0,                       /* tp_getattro */
    0,                       /* tp_setattro */
    0,                       /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT |
        Py_TPFLAGS_BASETYPE, /* tp_flags */
    0,                       /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    error_methods,           /* tp_methods */
    0,                       /* tp_members */
    error_getset,            /* tp_getset */
    0,                       /* tp_base */
    0,                       /* tp_dict */
    0,                       /* tp_descr_get */
    0,                       /* tp_descr_set */
    0,                       /* tp_dictoffset */
    (initproc)error_init,    /* tp_init */
    0,                       /* tp_alloc */
    0,                       /* tp_new */
};

int
init_error (PyObject *module) {
    PyObject *error;

    PycairoError_Type.tp_base = (PyTypeObject*)PyExc_Exception;
    if (PyType_Ready(&PycairoError_Type) < 0)
        return -1;

    error = (PyObject*)&PycairoError_Type;

    Py_INCREF(error);
    if (PyModule_AddObject(module, "Error", error) < 0) {
        Py_DECREF (error);
        return -1;
    }

    Py_INCREF(error);
    if (PyModule_AddObject(module, "CairoError", error) < 0) {
        Py_DECREF (error);
        return -1;
    }

    return 0;
}

static PyObject *
error_get_type_combined (PyObject *error, PyObject *other, char* name) {
    PyObject *class_dict, *new_type_args;
    PyObject *new_type;

    class_dict = PyDict_New ();
    if (class_dict == NULL)
        return NULL;

    new_type_args = Py_BuildValue ("s(OO)O", name,
                                   error, other, class_dict);
    Py_DECREF (class_dict);
    if (new_type_args == NULL)
        return NULL;

    new_type = PyType_Type.tp_new (&PyType_Type, new_type_args, NULL);
    return new_type;
}
