/* -*- mode: C; c-basic-offset: 2 -*-
 *
 * Pycairo - Python bindings for cairo
 *
 * Copyright © 2004-2006 Steve Chaplin
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


/* Class Pattern ---------------------------------------------------------- */

PyObject *
PycairoPattern_FromPattern (cairo_pattern_t *pattern, PyObject *base) {
  PyTypeObject *type = NULL;
  PyObject *o;

  assert (pattern != NULL);

  if (Pycairo_Check_Status (cairo_pattern_status (pattern))) {
    cairo_pattern_destroy (pattern);
    return NULL;
  }

  switch (cairo_pattern_get_type (pattern)) {
  case CAIRO_PATTERN_TYPE_SOLID:
    type = &PycairoSolidPattern_Type;
    break;
  case CAIRO_PATTERN_TYPE_SURFACE:
    type = &PycairoSurfacePattern_Type;
    break;
  case CAIRO_PATTERN_TYPE_LINEAR:
    type = &PycairoLinearGradient_Type;
    break;
  case CAIRO_PATTERN_TYPE_RADIAL:
    type = &PycairoRadialGradient_Type;
    break;
  case CAIRO_PATTERN_TYPE_MESH:
    type = &PycairoMeshPattern_Type;
    break;
  case CAIRO_PATTERN_TYPE_RASTER_SOURCE:
    type = &PycairoRasterSourcePattern_Type;
    break;
  default:
    type = &PycairoPattern_Type;
    break;
  }

  o = type->tp_alloc(type, 0);
  if (o == NULL) {
    cairo_pattern_destroy (pattern);
  } else {
    ((PycairoPattern *)o)->pattern = pattern;
    Py_XINCREF(base);
    ((PycairoPattern *)o)->base = base;
  }
  return o;
}

static void
pattern_dealloc (PycairoPattern *o) {
  if (o->pattern) {
    cairo_pattern_destroy (o->pattern);
    o->pattern = NULL;
  }
  Py_CLEAR(o->base);

  Py_TYPE(o)->tp_free(o);
}

static PyObject *
pattern_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyErr_SetString(PyExc_TypeError,
		  "The Pattern type cannot be instantiated");
  return NULL;
}

static PyObject *
pattern_get_extend (PycairoPattern *o, PyObject *ignored) {
  RETURN_INT_ENUM (Extend, cairo_pattern_get_extend (o->pattern));
}

static PyObject *
pattern_get_matrix (PycairoPattern *o, PyObject *ignored) {
  cairo_matrix_t matrix;
  cairo_pattern_get_matrix (o->pattern, &matrix);
  return PycairoMatrix_FromMatrix (&matrix);
}

static PyObject *
pattern_set_extend (PycairoPattern *o, PyObject *args) {
  cairo_extend_t extend;
  int extend_arg;

  if (!PyArg_ParseTuple (args, "i:Pattern.set_extend", &extend_arg))
    return NULL;

  extend = (cairo_extend_t)extend_arg;

  cairo_pattern_set_extend (o->pattern, extend);
  Py_RETURN_NONE;
}

static PyObject *
pattern_set_matrix (PycairoPattern *o, PyObject *args) {
  PycairoMatrix *m;

  if (!PyArg_ParseTuple (args, "O!:Pattern.set_matrix",
			 &PycairoMatrix_Type, &m))
    return NULL;

  cairo_pattern_set_matrix (o->pattern, &m->matrix);
  Py_RETURN_NONE;
}

static PyObject *
pattern_get_filter (PycairoPattern *o, PyObject *ignored) {
  cairo_filter_t filter;

  Py_BEGIN_ALLOW_THREADS;
  filter = cairo_pattern_get_filter (o->pattern);
  Py_END_ALLOW_THREADS;

  RETURN_INT_ENUM (Filter, filter);
}

static PyObject *
pattern_set_filter (PycairoPattern *o, PyObject *args) {
  cairo_filter_t filter;
  int filter_arg;

  if (!PyArg_ParseTuple (args, "i:Pattern.set_filter", &filter_arg))
    return NULL;

  filter = (cairo_filter_t)filter_arg;

  Py_BEGIN_ALLOW_THREADS;
  cairo_pattern_set_filter (o->pattern, filter);
  Py_END_ALLOW_THREADS;

  Py_RETURN_NONE;
}

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
static PyObject *
pattern_get_dither (PycairoPattern *o, PyObject *ignored) {
  cairo_dither_t dither;

  Py_BEGIN_ALLOW_THREADS;
  dither = cairo_pattern_get_dither (o->pattern);
  Py_END_ALLOW_THREADS;

  RETURN_INT_ENUM (Dither, dither);
}

static PyObject *
pattern_set_dither (PycairoPattern *o, PyObject *args) {
  cairo_dither_t dither;
  int dither_arg;

  if (!PyArg_ParseTuple (args, "i:Pattern.set_dither", &dither_arg))
    return NULL;

  dither = (cairo_dither_t)dither_arg;

  Py_BEGIN_ALLOW_THREADS;
  cairo_pattern_set_dither (o->pattern, dither);
  Py_END_ALLOW_THREADS;

  Py_RETURN_NONE;
}
#endif

static PyMethodDef pattern_methods[] = {
  /* methods never exposed in a language binding:
   * cairo_pattern_destroy()
   * cairo_pattern_get_type()
   * cairo_pattern_reference()
   *
   * cairo_pattern_status()
   * - not needed since Pycairo handles status checking
   */
  {"get_extend", (PyCFunction)pattern_get_extend,          METH_NOARGS },
  {"get_matrix", (PyCFunction)pattern_get_matrix,          METH_NOARGS },
  {"set_extend", (PyCFunction)pattern_set_extend,          METH_VARARGS },
  {"set_matrix", (PyCFunction)pattern_set_matrix,          METH_VARARGS },
  {"get_filter", (PyCFunction)pattern_get_filter,          METH_NOARGS },
  {"set_filter", (PyCFunction)pattern_set_filter,          METH_VARARGS },
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
  {"get_dither", (PyCFunction)pattern_get_dither,          METH_NOARGS },
  {"set_dither", (PyCFunction)pattern_set_dither,          METH_VARARGS },
#endif
  {NULL, NULL, 0, NULL},
};

static PyObject*
pattern_richcompare (PyObject *self, PyObject *other, int op)
{
  if (Py_TYPE(self) == Py_TYPE(other))
    return Pycairo_richcompare (
      ((PycairoPattern *)self)->pattern,
      ((PycairoPattern *)other)->pattern,
      op);
  else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

static Py_hash_t
pattern_hash (PyObject *self)
{
  return PYCAIRO_Py_hash_t_FromVoidPtr (((PycairoPattern *)self)->pattern);
}

PyTypeObject PycairoPattern_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.Pattern",                    /* tp_name */
  sizeof(PycairoPattern),             /* tp_basicsize */
  0,                                  /* tp_itemsize */
  (destructor)pattern_dealloc,        /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  pattern_hash,                       /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  pattern_richcompare,                /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  pattern_methods,                    /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  0,                                  /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)pattern_new,               /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


/* Class SolidPattern ----------------------------------------------------- */

static PyObject *
solid_pattern_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  double r, g, b, a = 1.0;
  if (!PyArg_ParseTuple (args, "ddd|d:SolidPattern.__new__", &r, &g, &b, &a))
    return NULL;
  return PycairoPattern_FromPattern (cairo_pattern_create_rgba (r, g, b, a),
				     NULL);
}

static PyObject *
solid_pattern_get_rgba (PycairoSolidPattern *o, PyObject *ignored) {
  double red, green, blue, alpha;
  cairo_pattern_get_rgba (o->pattern, &red, &green, &blue, &alpha);
  return Py_BuildValue("(dddd)", red, green, blue, alpha);
}

static PyMethodDef solid_pattern_methods[] = {
  {"get_rgba",       (PyCFunction)solid_pattern_get_rgba,      METH_NOARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoSolidPattern_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.SolidPattern",               /* tp_name */
  sizeof(PycairoSolidPattern),        /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  solid_pattern_methods,              /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoPattern_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)solid_pattern_new,         /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};

/* Class SurfacePattern --------------------------------------------------- */

static PyObject *
surface_pattern_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PycairoSurface *s;
  if (!PyArg_ParseTuple (args, "O!:SurfacePattern.__new__",
			 &PycairoSurface_Type, &s))
    return NULL;
  return PycairoPattern_FromPattern (
	     cairo_pattern_create_for_surface (s->surface), NULL);
}

static PyObject *
surface_pattern_get_surface (PycairoSurfacePattern *o, PyObject *ignored) {
  cairo_surface_t *surface;
  RETURN_NULL_IF_CAIRO_ERROR(cairo_pattern_get_surface (o->pattern, &surface));
  return PycairoSurface_FromSurface(cairo_surface_reference (surface), NULL);
}

static PyMethodDef surface_pattern_methods[] = {
  {"get_surface",   (PyCFunction)surface_pattern_get_surface, METH_NOARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoSurfacePattern_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.SurfacePattern",             /* tp_name */
  sizeof(PycairoSurfacePattern),      /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  surface_pattern_methods,            /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoPattern_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)surface_pattern_new,       /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};

/* Class Gradient --------------------------------------------------------- */

static PyObject *
gradient_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyErr_SetString(PyExc_TypeError,
		  "The Gradient type cannot be instantiated");
  return NULL;
}

static PyObject *
gradient_add_color_stop_rgb (PycairoGradient *o, PyObject *args) {
  double offset, red, green, blue;
  if (!PyArg_ParseTuple(args, "dddd:Gradient.add_color_stop_rgb",
			&offset, &red, &green, &blue))
    return NULL;
  cairo_pattern_add_color_stop_rgb (o->pattern, offset, red, green, blue);
  RETURN_NULL_IF_CAIRO_PATTERN_ERROR(o->pattern);
  Py_RETURN_NONE;
}

static PyObject *
gradient_add_color_stop_rgba (PycairoGradient *o, PyObject *args) {
  double offset, red, green, blue, alpha;
  if (!PyArg_ParseTuple(args, "ddddd:Gradient.add_color_stop_rgba",
			&offset, &red, &green, &blue, &alpha))
    return NULL;
  cairo_pattern_add_color_stop_rgba (o->pattern, offset, red,
				     green, blue, alpha);
  RETURN_NULL_IF_CAIRO_PATTERN_ERROR(o->pattern);
  Py_RETURN_NONE;
}

static PyObject *
gradient_get_color_stops_rgba (PycairoGradient *obj, PyObject *ignored) {
  cairo_status_t status;
  double offset, red, green, blue, alpha;
  int count;
  PyObject *list, *tuple;
  int result, i;

  status = cairo_pattern_get_color_stop_count (obj->pattern, &count);
  RETURN_NULL_IF_CAIRO_ERROR (status);

  list = PyList_New (0);
  if (list == NULL)
    return NULL;

  for (i = 0; i < count; i++) {
    status = cairo_pattern_get_color_stop_rgba (
      obj->pattern, i, &offset, &red, &green, &blue, &alpha);
    if (status != CAIRO_STATUS_SUCCESS)
      goto error;

    tuple = Py_BuildValue("(ddddd)", offset, red, green, blue, alpha);
    if (tuple == NULL)
      goto error;

    result = PyList_Append (list, tuple);
    Py_DECREF (tuple);
    if (result == -1)
      goto error;
  }

  return list;
error:
  Py_DECREF (list);
  if (status != CAIRO_STATUS_SUCCESS)
    Pycairo_Check_Status (status);
  return NULL;
}

static PyMethodDef gradient_methods[] = {
  {"add_color_stop_rgb",(PyCFunction)gradient_add_color_stop_rgb,
   METH_VARARGS },
  {"add_color_stop_rgba",(PyCFunction)gradient_add_color_stop_rgba,
   METH_VARARGS },
  {"get_color_stops_rgba",(PyCFunction)gradient_get_color_stops_rgba,
   METH_NOARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoGradient_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.Gradient",                   /* tp_name */
  sizeof(PycairoGradient),            /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  gradient_methods,                   /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoPattern_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)gradient_new,              /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};

/* Class LinearGradient --------------------------------------------------- */

static PyObject *
linear_gradient_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  double x0, y0, x1, y1;
  if (!PyArg_ParseTuple(args, "dddd:LinearGradient.__new__",
			&x0, &y0, &x1, &y1))
    return NULL;
  return PycairoPattern_FromPattern (
	       cairo_pattern_create_linear (x0, y0, x1, y1), NULL);
}

static PyObject *
linear_gradient_get_linear_points (PycairoLinearGradient *o, PyObject *ignored) {
  double x0, y0, x1, y1;
  cairo_pattern_get_linear_points (o->pattern, &x0, &y0, &x1, &y1);
  return Py_BuildValue("(dddd)", x0, y0, x1, y1);
}

static PyMethodDef linear_gradient_methods[] = {
  {"get_linear_points", (PyCFunction)linear_gradient_get_linear_points,
   METH_NOARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoLinearGradient_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.LinearGradient",             /* tp_name */
  sizeof(PycairoLinearGradient),      /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  linear_gradient_methods,            /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoGradient_Type,              /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)linear_gradient_new,       /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};

/* Class RadialGradient --------------------------------------------------- */

static PyObject *
radial_gradient_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  double cx0, cy0, radius0, cx1, cy1, radius1;
  if (!PyArg_ParseTuple(args, "dddddd:RadialGradient.__new__",
			&cx0, &cy0, &radius0, &cx1, &cy1, &radius1))
    return NULL;
  return PycairoPattern_FromPattern (
	    cairo_pattern_create_radial (cx0, cy0, radius0, cx1, cy1, radius1),
	    NULL);
}

static PyObject *
radial_gradient_get_radial_circles (PycairoRadialGradient *o, PyObject *ignored) {
  double x0, y0, r0, x1, y1, r1;
  cairo_pattern_get_radial_circles (o->pattern, &x0, &y0, &r0,
				    &x1, &y1, &r1);
  return Py_BuildValue("(dddddd)", x0, y0, r0, x1, y1, r1);
}

static PyMethodDef radial_gradient_methods[] = {
  {"get_radial_circles", (PyCFunction)radial_gradient_get_radial_circles,
   METH_NOARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoRadialGradient_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.RadialGradient",             /* tp_name */
  sizeof(PycairoRadialGradient),      /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  radial_gradient_methods,            /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoGradient_Type,              /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)radial_gradient_new,       /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};

static PyObject *
mesh_pattern_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  if (!PyArg_ParseTuple (args, ":Mesh.__new__"))
    return NULL;

  return PycairoPattern_FromPattern (cairo_pattern_create_mesh (), NULL);
}

static PyObject *
mesh_pattern_begin_patch (PycairoMeshPattern *obj, PyObject *ignored) {
  Py_BEGIN_ALLOW_THREADS;
  cairo_mesh_pattern_begin_patch (obj->pattern);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_PATTERN_ERROR (obj->pattern);

  Py_RETURN_NONE;
}

static PyObject *
mesh_pattern_end_patch (PycairoMeshPattern *obj, PyObject *ignored) {
  Py_BEGIN_ALLOW_THREADS;
  cairo_mesh_pattern_end_patch (obj->pattern);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_PATTERN_ERROR (obj->pattern);

  Py_RETURN_NONE;
}

static PyObject *
mesh_pattern_curve_to (PycairoMeshPattern *obj, PyObject *args) {
  double x1, y1, x2, y2, x3, y3;

  if (!PyArg_ParseTuple(args, "dddddd:MeshPattern.curve_to",
      &x1, &y1, &x2, &y2, &x3, &y3))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_mesh_pattern_curve_to (obj->pattern, x1, y1, x2, y2, x3, y3);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_PATTERN_ERROR (obj->pattern);

  Py_RETURN_NONE;
}

static PyObject *
mesh_pattern_get_control_point (PycairoMeshPattern *obj, PyObject *args) {
  double x, y;
  unsigned int patch_num, point_num;
  cairo_status_t status;

  if (!PyArg_ParseTuple(args, "II:MeshPattern.get_control_point",
      &patch_num, &point_num))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  status = cairo_mesh_pattern_get_control_point (
    obj->pattern, patch_num, point_num, &x, &y);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_ERROR (status);

  return Py_BuildValue("(dd)", x, y);
}

static PyObject *
mesh_pattern_get_corner_color_rgba (PycairoMeshPattern *obj, PyObject *args) {
  double red, green, blue, alpha;
  unsigned int patch_num, corner_num;
  cairo_status_t status;

  if (!PyArg_ParseTuple(args, "II:MeshPattern.get_corner_color_rgba",
      &patch_num, &corner_num))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  status = cairo_mesh_pattern_get_corner_color_rgba (
    obj->pattern, patch_num, corner_num, &red, &green, &blue, &alpha);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_ERROR (status);

  return Py_BuildValue("(dddd)", red, green, blue, alpha);
}

static PyObject *
mesh_pattern_get_patch_count (PycairoMeshPattern *obj, PyObject *ignored) {
  unsigned int count;
  cairo_status_t status;

  Py_BEGIN_ALLOW_THREADS;
  status = cairo_mesh_pattern_get_patch_count (obj->pattern, &count);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_ERROR (status);

  return PyLong_FromLong ((long)count);
}

static PyObject *
mesh_pattern_get_path (PycairoMeshPattern *obj, PyObject *args) {
  unsigned int patch_num;
  cairo_path_t *path;

  if (!PyArg_ParseTuple(args, "I:MeshPattern.get_path", &patch_num))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  path = cairo_mesh_pattern_get_path (obj->pattern, patch_num);
  Py_END_ALLOW_THREADS;

  return PycairoPath_FromPath (path);
}

static PyObject *
mesh_pattern_line_to (PycairoMeshPattern *obj, PyObject *args) {
  double x, y;

  if (!PyArg_ParseTuple(args, "dd:MeshPattern.line_to", &x, &y))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_mesh_pattern_line_to (obj->pattern, x, y);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_PATTERN_ERROR (obj->pattern);

  Py_RETURN_NONE;
}

static PyObject *
mesh_pattern_move_to (PycairoMeshPattern *obj, PyObject *args) {
  double x, y;

  if (!PyArg_ParseTuple(args, "dd:MeshPattern.move_to", &x, &y))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_mesh_pattern_move_to (obj->pattern, x, y);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_PATTERN_ERROR (obj->pattern);

  Py_RETURN_NONE;
}

static PyObject *
mesh_pattern_set_control_point (PycairoMeshPattern *obj, PyObject *args) {
  double x, y;
  unsigned int point_num;

  if (!PyArg_ParseTuple(args, "Idd:MeshPattern.set_control_point",
      &point_num, &x, &y))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_mesh_pattern_set_control_point (obj->pattern, point_num, x, y);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_PATTERN_ERROR (obj->pattern);

  Py_RETURN_NONE;
}

static PyObject *
mesh_pattern_set_corner_color_rgb (PycairoMeshPattern *obj, PyObject *args) {
  double red, green, blue;
  unsigned int corner_num;

  if (!PyArg_ParseTuple(args, "Iddd:MeshPattern.set_corner_color_rgb",
      &corner_num, &red, &green, &blue))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_mesh_pattern_set_corner_color_rgb (
    obj->pattern, corner_num, red, green, blue);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_PATTERN_ERROR (obj->pattern);

  Py_RETURN_NONE;
}

static PyObject *
mesh_pattern_set_corner_color_rgba (PycairoMeshPattern *obj, PyObject *args) {
  double red, green, blue, alpha;
  unsigned int corner_num;

  if (!PyArg_ParseTuple(args, "Idddd:MeshPattern.set_corner_color_rgba",
      &corner_num, &red, &green, &blue, &alpha))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_mesh_pattern_set_corner_color_rgba (
    obj->pattern, corner_num, red, green, blue, alpha);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_PATTERN_ERROR (obj->pattern);

  Py_RETURN_NONE;
}

static PyMethodDef mesh_pattern_methods[] = {
  {"begin_patch",       (PyCFunction)mesh_pattern_begin_patch, METH_NOARGS},
  {"end_patch",         (PyCFunction)mesh_pattern_end_patch,   METH_NOARGS},
  {"curve_to",          (PyCFunction)mesh_pattern_curve_to,    METH_VARARGS},
  {"get_control_point",
   (PyCFunction)mesh_pattern_get_control_point, METH_VARARGS},
  {"get_corner_color_rgba",
   (PyCFunction)mesh_pattern_get_corner_color_rgba, METH_VARARGS},
  {"get_patch_count",
   (PyCFunction)mesh_pattern_get_patch_count, METH_NOARGS},
  {"get_path",          (PyCFunction)mesh_pattern_get_path,    METH_VARARGS},
  {"line_to",           (PyCFunction)mesh_pattern_line_to,     METH_VARARGS},
  {"move_to",           (PyCFunction)mesh_pattern_move_to,     METH_VARARGS},
  {"set_control_point",
   (PyCFunction)mesh_pattern_set_control_point, METH_VARARGS},
  {"set_corner_color_rgb",
   (PyCFunction)mesh_pattern_set_corner_color_rgb, METH_VARARGS},
  {"set_corner_color_rgba",
   (PyCFunction)mesh_pattern_set_corner_color_rgba, METH_VARARGS},
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoMeshPattern_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.MeshPattern",                /* tp_name */
  sizeof(PycairoMeshPattern),         /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  mesh_pattern_methods,               /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoPattern_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)mesh_pattern_new,          /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


static PyObject *
raster_source_pattern_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  cairo_content_t content;
  int width, height, content_arg;

  if (!PyArg_ParseTuple (args, "iii:RasterSourcePattern.__new__",
                         &content_arg, &width, &height))
    return NULL;

  content = (cairo_content_t)content_arg;

  return PycairoPattern_FromPattern (
    cairo_pattern_create_raster_source (NULL, content, width, height), NULL);
}

static const cairo_user_data_key_t raster_source_acquire_key;
static const cairo_user_data_key_t raster_source_release_key;

static void
_decref_destroy_func (void *user_data) {
  PyGILState_STATE gstate = PyGILState_Ensure ();
  Py_DECREF (user_data);
  PyGILState_Release (gstate);
}

static void
_raster_source_release_func (cairo_pattern_t *pattern,
                             void *callback_data,
                             cairo_surface_t *surface) {
  void *user_data;
  PyObject *pysurface, *result;
  PyGILState_STATE gstate;

  pattern = (cairo_pattern_t *)callback_data;

  user_data = cairo_pattern_get_user_data (
    pattern, &raster_source_release_key);
  /* in case there is a acquire callback but no release one, this gets called
   * anyway so we can free the surface
   */
  if (user_data == NULL) {
    cairo_surface_destroy (surface);
    return;
  }

  gstate = PyGILState_Ensure ();

  pysurface = PycairoSurface_FromSurface (
    cairo_surface_reference (surface), NULL);
  if (pysurface == NULL)
    goto error;

  result = PyObject_CallFunction ((PyObject *)user_data, "(O)", pysurface);
  if (result == NULL)
    goto error;

  if (result != Py_None) {
    Py_DECREF (result);
    PyErr_SetString (PyExc_TypeError,
      "Return value of release callback needs to be None");
    result = NULL;
    goto error;
  }
  Py_DECREF (result);

  Py_DECREF (pysurface);
  PyGILState_Release (gstate);
  cairo_surface_destroy (surface);
  return;
error:
  if (PyErr_Occurred ()) {
    PyErr_Print ();
    PyErr_Clear ();
  }
  Py_XDECREF (pysurface);
  PyGILState_Release (gstate);
  cairo_surface_destroy (surface);
  return;
}

static cairo_surface_t*
_raster_source_acquire_func (cairo_pattern_t *pattern, void *callback_data,
                             cairo_surface_t *target,
                             const cairo_rectangle_int_t *extents) {
  void *user_data;
  PyGILState_STATE gstate;
  PyObject *result;
  PyObject *pysurface = NULL, *pyrect = NULL;
  cairo_surface_t *result_surface;

  /* https://bugs.freedesktop.org/show_bug.cgi?id=101866
   * If something changes the callback data, we are screwed, but not much
   * we can do to detect that..
   */
  pattern = (cairo_pattern_t *)callback_data;

  gstate = PyGILState_Ensure ();

  user_data = cairo_pattern_get_user_data (
    pattern, &raster_source_acquire_key);
  if (user_data == NULL)
    goto error;

  pysurface = PycairoSurface_FromSurface (
    cairo_surface_reference (target), NULL);
  if (pysurface == NULL)
    goto error;

  pyrect = PycairoRectangleInt_FromRectangleInt (extents);
  if (pyrect == NULL)
    goto error;

  result = PyObject_CallFunction (
    (PyObject *)user_data, "(OO)", pysurface, pyrect);

  if (result != NULL) {
    if (!PyObject_TypeCheck (result, &PycairoSurface_Type)) {
      Py_DECREF (result);
      PyErr_SetString (PyExc_TypeError,
        "Return value of acquire callback needs to be of type Surface");
      result = NULL;
    }
  }

  if (result == NULL)
    goto error;

  Py_DECREF (pysurface);
  Py_DECREF (pyrect);
  result_surface = ((PycairoSurface *)result)->surface;
  cairo_surface_reference (result_surface);
  Py_DECREF (result);
  PyGILState_Release (gstate);
  return result_surface;

error:
  if (PyErr_Occurred ()) {
    PyErr_Print ();
    PyErr_Clear ();
  }
  Py_XDECREF (pysurface);
  Py_XDECREF (pyrect);
  PyGILState_Release (gstate);
  return NULL;
}

static PyObject *
raster_source_pattern_set_acquire (PycairoRasterSourcePattern *obj,
                                   PyObject *args) {
  PyObject *acquire_callable, *release_callable;
  cairo_status_t status;
  cairo_pattern_t *pattern;
  void *callback_data;
  cairo_raster_source_acquire_func_t acquire_func;
  cairo_raster_source_release_func_t release_func;
  void *acquire_user_data, *release_user_data;

  if (!PyArg_ParseTuple (args, "OO:RasterSourcePattern.set_acquire",
      &acquire_callable, &release_callable))
    return NULL;

  pattern = obj->pattern;

  callback_data = cairo_raster_source_pattern_get_callback_data (pattern);
  if (callback_data != NULL && callback_data != pattern) {
    PyErr_SetString (PyExc_RuntimeError,
      "Callback is set, but not through Pycairo. Replacing not supported.");
    return NULL;
  }

  if (!PyCallable_Check (acquire_callable) && acquire_callable != Py_None) {
    PyErr_SetString (
      PyExc_TypeError, "argument needs to be a callable or None");
    return NULL;
  }

  if (!PyCallable_Check (release_callable) && release_callable != Py_None) {
    PyErr_SetString (
      PyExc_TypeError, "argument needs to be a callable or None");
    return NULL;
  }

  if (acquire_callable == Py_None) {
    acquire_func = NULL;
    acquire_user_data = NULL;
  } else {
    acquire_func = _raster_source_acquire_func;
    acquire_user_data = acquire_callable;
  }

  if (release_callable == Py_None) {
    release_func = NULL;
    release_user_data = NULL;
  } else {
    release_func = _raster_source_release_func;
    release_user_data = release_callable;
  }

  /* in case acquire is set we have to clean up anyway */
  if (acquire_func != NULL && release_func == NULL) {
    release_func = _raster_source_release_func;
  }

  status = cairo_pattern_set_user_data (
    pattern, &raster_source_acquire_key, acquire_user_data,
    (acquire_user_data) ? _decref_destroy_func : NULL);
  RETURN_NULL_IF_CAIRO_ERROR (status);
  if (acquire_user_data != NULL)
    Py_INCREF (acquire_user_data);

  status = cairo_pattern_set_user_data (
    pattern, &raster_source_release_key, release_user_data,
    (release_user_data) ? _decref_destroy_func : NULL);
  if (status != CAIRO_STATUS_SUCCESS) {
    cairo_pattern_set_user_data (
      pattern, &raster_source_acquire_key, NULL, NULL);
    RETURN_NULL_IF_CAIRO_ERROR (status);
  }
  if (release_user_data != NULL)
    Py_INCREF (release_user_data);

  cairo_raster_source_pattern_set_callback_data (pattern, pattern);

  Py_BEGIN_ALLOW_THREADS;
  cairo_raster_source_pattern_set_acquire (
    pattern, acquire_func, release_func);
  Py_END_ALLOW_THREADS;

  Py_RETURN_NONE;
}

static PyObject *
raster_source_pattern_get_acquire (PycairoRasterSourcePattern *obj, PyObject *ignored) {
  cairo_pattern_t *pattern;
  void *user_data;
  PyObject *acquire_callable, *release_callable;

  pattern = obj->pattern;

  user_data = cairo_pattern_get_user_data (
    pattern, &raster_source_acquire_key);
  if (user_data == NULL) {
    acquire_callable = Py_None;
  } else {
    acquire_callable = user_data;
  }

  user_data = cairo_pattern_get_user_data (
    pattern, &raster_source_release_key);
  if (user_data == NULL) {
    release_callable = Py_None;
  } else {
    release_callable = user_data;
  }

  return Py_BuildValue ("(OO)", acquire_callable, release_callable);
}

static PyMethodDef raster_source_pattern_methods[] = {
  {"set_acquire",
   (PyCFunction)raster_source_pattern_set_acquire, METH_VARARGS},
  {"get_acquire",
   (PyCFunction)raster_source_pattern_get_acquire, METH_NOARGS},
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoRasterSourcePattern_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.RasterSourcePattern",        /* tp_name */
  sizeof(PycairoRasterSourcePattern), /* tp_basicsize */
  0,                                  /* tp_itemsize */
  0,                                  /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  0,                                  /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  0,                                  /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  raster_source_pattern_methods,      /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoPattern_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)raster_source_pattern_new, /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
