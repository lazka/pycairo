/* -*- mode: C; c-basic-offset: 2 -*-
 *
 * Pycairo - Python bindings for cairo
 *
 * Copyright © 2003 James Henstridge, Steven Chaplin
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


/* class cairo.FontFace --------------------------------------------------- */

PyObject *
PycairoFontFace_FromFontFace (cairo_font_face_t *font_face) {
  PyTypeObject *type = NULL;
  PyObject *o;

  assert (font_face != NULL);

  if (Pycairo_Check_Status (cairo_font_face_status (font_face))) {
    cairo_font_face_destroy (font_face);
    return NULL;
  }

  switch (cairo_font_face_get_type (font_face)) {
  case CAIRO_FONT_TYPE_TOY:
    type = &PycairoToyFontFace_Type;
    break;
  default:
    type = &PycairoFontFace_Type;
    break;
  }
  o = type->tp_alloc (type, 0);
  if (o == NULL)
    cairo_font_face_destroy (font_face);
  else
    ((PycairoFontFace *)o)->font_face = font_face;
  return o;
}

static void
font_face_dealloc (PycairoFontFace *o) {
  if (o->font_face) {
    cairo_font_face_destroy (o->font_face);
    o->font_face = NULL;
  }
  Py_TYPE(o)->tp_free(o);
}

static PyObject *
font_face_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyErr_SetString (PyExc_TypeError, "The FontFace type cannot be "
		   "instantiated directly, use Context.get_font_face()");
  return NULL;
}

/*
static PyMethodDef font_face_methods[] = {
     * methods never exposed in a language binding:
     * cairo_font_face_destroy()
     * cairo_font_face_get_user_data()
     * cairo_font_face_get_type()
     * cairo_font_face_reference()
     * cairo_font_face_set_user_data(),
    {NULL, NULL, 0, NULL},
};
*/


static PyObject*
font_face_richcompare (PyObject *self, PyObject *other, int op)
{
  if (Py_TYPE(self) == Py_TYPE(other))
    return Pycairo_richcompare (
      ((PycairoFontFace *)self)->font_face,
      ((PycairoFontFace *)other)->font_face,
      op);
  else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

static Py_hash_t
font_face_hash (PyObject *self)
{
  return PYCAIRO_Py_hash_t_FromVoidPtr (((PycairoFontFace *)self)->font_face);
}

PyTypeObject PycairoFontFace_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.FontFace",                   /* tp_name */
  sizeof(PycairoFontFace),            /* tp_basicsize */
  0,                                  /* tp_itemsize */
  (destructor)font_face_dealloc,      /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  font_face_hash,                     /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  font_face_richcompare,              /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  0,                                  /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  0,                                  /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)font_face_new,             /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


/* class cairo.ToyFontFace ------------------------------------------------- */

static PyObject *
toy_font_face_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  const char *utf8;
  PyObject *o;
  cairo_font_slant_t slant;
  cairo_font_weight_t weight;
  int slant_arg = CAIRO_FONT_SLANT_NORMAL;
  int weight_arg = CAIRO_FONT_WEIGHT_NORMAL;

  if (!PyArg_ParseTuple (args, "es|ii:ToyFontFace.__new__",
                         "utf-8", &utf8, &slant_arg, &weight_arg))
    return NULL;

  slant = (cairo_font_slant_t)slant_arg;
  weight = (cairo_font_weight_t)weight_arg;

  o = PycairoFontFace_FromFontFace (
		     cairo_toy_font_face_create (utf8, slant, weight));
  PyMem_Free((void *)utf8);
  return o;
}

static PyObject *
toy_font_get_family (PycairoToyFontFace *o, PyObject *ignored) {
  return PyUnicode_FromString (cairo_toy_font_face_get_family (o->font_face));
}

static PyObject *
toy_font_get_slant (PycairoToyFontFace *o, PyObject *ignored) {
  RETURN_INT_ENUM (FontSlant, cairo_toy_font_face_get_slant (o->font_face));
}

static PyObject *
toy_font_get_weight (PycairoToyFontFace *o, PyObject *ignored) {
  RETURN_INT_ENUM (FontWeight, cairo_toy_font_face_get_weight (o->font_face));
}

static PyMethodDef toy_font_face_methods[] = {
  {"get_family",       (PyCFunction)toy_font_get_family, METH_NOARGS},
  {"get_slant",        (PyCFunction)toy_font_get_slant,  METH_NOARGS},
  {"get_weight",       (PyCFunction)toy_font_get_weight, METH_NOARGS},
  {NULL, NULL, 0, NULL},
};


PyTypeObject PycairoToyFontFace_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.ToyFontFace",                /* tp_name */
  sizeof(PycairoToyFontFace),         /* tp_basicsize */
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
  toy_font_face_methods,              /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoFontFace_Type,              /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)toy_font_face_new,         /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


/* class cairo.ScaledFont ------------------------------------------------- */

PyObject *
PycairoScaledFont_FromScaledFont (cairo_scaled_font_t *scaled_font) {
  PyObject *o;

  assert (scaled_font != NULL);

  if (Pycairo_Check_Status (cairo_scaled_font_status (scaled_font))) {
    cairo_scaled_font_destroy (scaled_font);
    return NULL;
  }

  o = PycairoScaledFont_Type.tp_alloc (&PycairoScaledFont_Type, 0);
  if (o == NULL)
    cairo_scaled_font_destroy (scaled_font);
  else
    ((PycairoScaledFont *)o)->scaled_font = scaled_font;
  return o;
}

static void
scaled_font_dealloc(PycairoScaledFont *o) {
  if (o->scaled_font) {
    cairo_scaled_font_destroy (o->scaled_font);
    o->scaled_font = NULL;
  }
  Py_TYPE(o)->tp_free(o);
}

static PyObject *
scaled_font_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PycairoFontFace *ff;
  PycairoFontOptions *fo;
  PycairoMatrix *mx1, *mx2;

  if (!PyArg_ParseTuple(args, "O!O!O!O!:ScaledFont.__new__",
			&PycairoFontFace_Type, &ff,
			&PycairoMatrix_Type, &mx1,
			&PycairoMatrix_Type, &mx2,
			&PycairoFontOptions_Type, &fo))
    return NULL;
  return PycairoScaledFont_FromScaledFont (
               cairo_scaled_font_create (ff->font_face, &mx1->matrix,
					 &mx2->matrix, fo->font_options));
}

static PyObject *
scaled_font_extents (PycairoScaledFont *o, PyObject *ignored) {
  cairo_font_extents_t e;

  cairo_scaled_font_extents (o->scaled_font, &e);
  RETURN_NULL_IF_CAIRO_SCALED_FONT_ERROR(o->scaled_font);
  return Py_BuildValue ("(ddddd)", e.ascent, e.descent, e.height,
			e.max_x_advance, e.max_y_advance);
}

static PyObject *
scaled_font_get_font_face (PycairoScaledFont *o, PyObject *ignored) {
  return PycairoFontFace_FromFontFace (
               cairo_font_face_reference (
		   cairo_scaled_font_get_font_face (o->scaled_font)));
}

static PyObject *
scaled_font_get_scale_matrix (PycairoScaledFont *o, PyObject *ignored) {
  cairo_matrix_t matrix;
  cairo_scaled_font_get_scale_matrix (o->scaled_font, &matrix);
  return PycairoMatrix_FromMatrix (&matrix);
}

static PyObject *
scaled_font_text_extents (PycairoScaledFont *o, PyObject *args) {
  cairo_text_extents_t extents;
  const char *utf8;
  PyObject *ext_args, *res;

  if (!PyArg_ParseTuple (args,
        "es:ScaledFont.text_extents", "utf-8", &utf8))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_scaled_font_text_extents (o->scaled_font, utf8, &extents);
  Py_END_ALLOW_THREADS;

  PyMem_Free ((void *)utf8);

  RETURN_NULL_IF_CAIRO_SCALED_FONT_ERROR (o->scaled_font);
  ext_args =  Py_BuildValue ("(dddddd)", extents.x_bearing, extents.y_bearing,
                             extents.width, extents.height, extents.x_advance,
                             extents.y_advance);
  res = PyObject_Call ((PyObject *)&PycairoTextExtents_Type, ext_args, NULL);
  Py_DECREF (ext_args);
  return res;
}

static PyObject *
scaled_font_get_ctm (PycairoScaledFont *o, PyObject *ignored) {
  cairo_matrix_t matrix;

  Py_BEGIN_ALLOW_THREADS;
  cairo_scaled_font_get_ctm (o->scaled_font, &matrix);
  Py_END_ALLOW_THREADS;

  return PycairoMatrix_FromMatrix (&matrix);
}

static PyObject *
scaled_font_get_font_matrix (PycairoScaledFont *o, PyObject *ignored) {
  cairo_matrix_t matrix;

  Py_BEGIN_ALLOW_THREADS;
  cairo_scaled_font_get_font_matrix (o->scaled_font, &matrix);
  Py_END_ALLOW_THREADS;

  return PycairoMatrix_FromMatrix (&matrix);
}

static PyObject *
scaled_font_get_font_options (PycairoScaledFont *o, PyObject *ignored) {
  cairo_font_options_t *options = cairo_font_options_create();

  Py_BEGIN_ALLOW_THREADS;
  cairo_scaled_font_get_font_options (o->scaled_font, options);
  Py_END_ALLOW_THREADS;

  return PycairoFontOptions_FromFontOptions (options);
}

static PyObject *
scaled_font_text_to_glyphs (PycairoScaledFont *o, PyObject *args) {
  const char *utf8;
  double x, y;
  int with_clusters = 1;

  cairo_status_t status;
  cairo_glyph_t *glyphs = NULL;
  int num_glyphs;
  cairo_text_cluster_t *clusters = NULL;
  int num_clusters;
  cairo_text_cluster_flags_t cluster_flags;

  int i;
  PyObject *glyph_list = NULL;
  PyObject *cluster_list = NULL;
  PyObject *flags = NULL;
  PyObject *pyglyph, *glyph_args;
  PyObject *pycluster, *cluster_args;

  if (!PyArg_ParseTuple (args,
      "ddes|i:ScaledFont.text_to_glyphs",
      &x, &y, "utf-8", &utf8, &with_clusters))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  status = cairo_scaled_font_text_to_glyphs (
    o->scaled_font,
    x, y,
    utf8, -1,
    &glyphs, &num_glyphs,
    (with_clusters) ? &clusters : NULL,
    (with_clusters) ? &num_clusters : NULL,
    (with_clusters) ? &cluster_flags : NULL);
  Py_END_ALLOW_THREADS;

  PyMem_Free ((void *)utf8);
  RETURN_NULL_IF_CAIRO_ERROR (status);

  glyph_list = PyList_New (num_glyphs);
  if (glyph_list == NULL)
    goto error;
  for(i=0; i < num_glyphs; i++) {
    cairo_glyph_t *glyph = &glyphs[i];
    glyph_args = Py_BuildValue(
      "(kdd)", glyph->index, glyph->x, glyph->y);
    if (glyph_args == NULL)
      goto error;
    pyglyph = PyObject_Call(
      (PyObject *)&PycairoGlyph_Type, glyph_args, NULL);
    Py_DECREF (glyph_args);
    if (pyglyph == NULL)
      goto error;
    PyList_SET_ITEM (glyph_list, i, pyglyph);
  }
  cairo_glyph_free (glyphs);
  glyphs = NULL;

  if (with_clusters) {
    cluster_list = PyList_New (num_clusters);
    if (cluster_list == NULL)
      goto error;
    for(i=0; i < num_clusters; i++) {
      cairo_text_cluster_t *cluster = &clusters[i];
      cluster_args = Py_BuildValue(
        "(ii)", cluster->num_bytes, cluster->num_glyphs);
      if (cluster_args == NULL)
        goto error;
      pycluster = PyObject_Call(
        (PyObject *)&PycairoTextCluster_Type, cluster_args, NULL);
      Py_DECREF (cluster_args);
      if (pycluster == NULL)
        goto error;
      PyList_SET_ITEM (cluster_list, i, pycluster);
    }
    cairo_text_cluster_free (clusters);
    clusters = NULL;

    flags = CREATE_INT_ENUM (TextClusterFlags, cluster_flags);
    if (flags == NULL)
      goto error;

    return Py_BuildValue ("(NNN)", glyph_list, cluster_list, flags);
  } else {
    return glyph_list;
  }
error:
  cairo_glyph_free (glyphs);
  cairo_text_cluster_free (clusters);
  Py_XDECREF (glyph_list);
  Py_XDECREF (cluster_list);
  Py_XDECREF (flags);
  return NULL;
}

static PyObject *
scaled_font_glyph_extents (PycairoScaledFont *o, PyObject *args) {
  int num_glyphs = -1;
  cairo_glyph_t *glyphs;
  cairo_text_extents_t extents;
  PyObject *py_object, *ext_args, *res;

  if (!PyArg_ParseTuple (args, "O|i:ScaledFont.glyph_extents",
      &py_object, &num_glyphs))
    return NULL;

  if (PyTuple_Size (args) > 1) {
    PyErr_WarnEx (PyExc_DeprecationWarning,
                  "The num_glyphs parameter to ScaledFont.glyph_extents is deprecated",
                  1);
  }

  glyphs = _PycairoGlyphs_AsGlyphs (py_object, &num_glyphs);
  if (glyphs == NULL)
    return NULL;

  Py_BEGIN_ALLOW_THREADS
  cairo_scaled_font_glyph_extents (
    o->scaled_font, glyphs, num_glyphs, &extents);
  Py_END_ALLOW_THREADS;

  PyMem_Free (glyphs);
  RETURN_NULL_IF_CAIRO_SCALED_FONT_ERROR (o->scaled_font);
  ext_args = Py_BuildValue ("(dddddd)", extents.x_bearing, extents.y_bearing,
                            extents.width, extents.height, extents.x_advance,
                            extents.y_advance);
  res = PyObject_Call ((PyObject *)&PycairoTextExtents_Type, ext_args, NULL);
  Py_DECREF (ext_args);
  return res;
}

static PyMethodDef scaled_font_methods[] = {
  /* methods never exposed in a language binding:
   * cairo_scaled_font_destroy()
   * cairo_scaled_font_get_type()
   * cairo_scaled_font_reference()
   */
  {"extents",       (PyCFunction)scaled_font_extents,        METH_NOARGS},
  {"get_font_face", (PyCFunction)scaled_font_get_font_face,  METH_NOARGS},
  {"get_ctm",       (PyCFunction)scaled_font_get_ctm,        METH_NOARGS},
  {"get_font_matrix",  (PyCFunction)scaled_font_get_font_matrix,  METH_NOARGS},
  {"get_font_options", (PyCFunction)scaled_font_get_font_options, METH_NOARGS},
  {"get_scale_matrix", (PyCFunction)scaled_font_get_scale_matrix, METH_VARARGS},
  {"text_extents",  (PyCFunction)scaled_font_text_extents,   METH_VARARGS},
  {"text_to_glyphs",  (PyCFunction)scaled_font_text_to_glyphs,    METH_VARARGS},
  {"glyph_extents", (PyCFunction)scaled_font_glyph_extents,  METH_VARARGS},
  {NULL, NULL, 0, NULL},
};

static PyObject*
scaled_font_richcompare (PyObject *self, PyObject *other, int op)
{
  if (Py_TYPE(self) == Py_TYPE(other))
    return Pycairo_richcompare (
      ((PycairoScaledFont *)self)->scaled_font,
      ((PycairoScaledFont *)other)->scaled_font,
      op);
  else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

static Py_hash_t
scaled_font_hash (PyObject *self)
{
  return PYCAIRO_Py_hash_t_FromVoidPtr (
    ((PycairoScaledFont *)self)->scaled_font);
}

PyTypeObject PycairoScaledFont_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.ScaledFont",                 /* tp_name */
  sizeof(PycairoScaledFont),          /* tp_basicsize */
  0,                                  /* tp_itemsize */
  (destructor)scaled_font_dealloc,    /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  scaled_font_hash,                   /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  scaled_font_richcompare,            /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  scaled_font_methods,                /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  0,                                  /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)scaled_font_new,           /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


/* class cairo.FontOptions ------------------------------------------------ */

PyObject *
PycairoFontOptions_FromFontOptions (cairo_font_options_t *font_options) {
  PyObject *o;

  assert (font_options != NULL);

  if (Pycairo_Check_Status (cairo_font_options_status (font_options))) {
    cairo_font_options_destroy (font_options);
    return NULL;
  }

  o = PycairoFontOptions_Type.tp_alloc (&PycairoFontOptions_Type, 0);
  if (o == NULL)
    cairo_font_options_destroy (font_options);
  else
    ((PycairoFontOptions *)o)->font_options = font_options;
  return o;
}

static void
font_options_dealloc(PycairoFontOptions *o) {
  if (o->font_options) {
    cairo_font_options_destroy (o->font_options);
    o->font_options = NULL;
  }
  Py_TYPE(o)->tp_free(o);
}

static PyObject *
font_options_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {

  if (!PyArg_ParseTuple (args, ":FontOptions.__new__"))
    return NULL;

  return PycairoFontOptions_FromFontOptions (cairo_font_options_create());
}

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 15, 12)
static PyObject *
font_options_set_variations (PycairoFontOptions *o, PyObject *args) {
    const char *variations;
    PyObject *maybe_none;

    if (!PyArg_ParseTuple (args, "O:FontOptions.set_variations", &maybe_none))
        return NULL;

    if (maybe_none != Py_None) {
        if (!PyArg_ParseTuple (args,
                "es:FontOptions.set_variations", "utf-8",
                &variations))
            return NULL;
    } else {
        variations = NULL;
    }

    Py_BEGIN_ALLOW_THREADS;
    cairo_font_options_set_variations (o->font_options, variations);
    Py_END_ALLOW_THREADS;

    if (variations != NULL)
        PyMem_Free((void *)variations);

    RETURN_NULL_IF_CAIRO_FONT_OPTIONS_ERROR(o->font_options);
    Py_RETURN_NONE;
}

static PyObject *
font_options_get_variations (PycairoFontOptions *o, PyObject *ignored) {
    const char *variations;

    Py_BEGIN_ALLOW_THREADS;
    variations = cairo_font_options_get_variations (o->font_options);
    Py_END_ALLOW_THREADS;

    if (variations == NULL)
        Py_RETURN_NONE;

    return PyUnicode_FromString (variations);
}
#endif

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 8)
static PyObject *
font_options_set_color_mode (PycairoFontOptions *o, PyObject *args) {
  cairo_color_mode_t color_mode;
  int color_mode_arg;

  if (!PyArg_ParseTuple (args, "i:FontOptions.set_color_mode", &color_mode_arg))
    return NULL;

  color_mode = (cairo_color_mode_t)color_mode_arg;

  cairo_font_options_set_color_mode (o->font_options, color_mode);
  RETURN_NULL_IF_CAIRO_FONT_OPTIONS_ERROR(o->font_options);
  Py_RETURN_NONE;
}

static PyObject *
font_options_get_color_mode (PycairoFontOptions *o, PyObject *ignored) {
  RETURN_INT_ENUM (ColorMode, cairo_font_options_get_color_mode (o->font_options));
}

static PyObject *
font_options_set_color_palette (PycairoFontOptions *o, PyObject *args) {
  unsigned int color_palette;

  if (!PyArg_ParseTuple (args, "I:FontOptions.set_color_palette", &color_palette))
    return NULL;

  cairo_font_options_set_color_palette (o->font_options, color_palette);
  RETURN_NULL_IF_CAIRO_FONT_OPTIONS_ERROR(o->font_options);
  Py_RETURN_NONE;
}

static PyObject *
font_options_get_color_palette (PycairoFontOptions *o, PyObject *ignored) {
  return PyLong_FromUnsignedLong (cairo_font_options_get_color_palette (o->font_options));
}

static PyObject *
font_options_set_custom_palette_color (PycairoFontOptions *o, PyObject *args) {
  double red, green, blue, alpha;
  unsigned int index;

  if (!PyArg_ParseTuple (args, "Idddd:FontOptions.set_custom_palette_color",
      &index, &red, &green, &blue, &alpha))
    return NULL;

  cairo_font_options_set_custom_palette_color (o->font_options, index, red, green, blue, alpha);
  RETURN_NULL_IF_CAIRO_FONT_OPTIONS_ERROR(o->font_options);
  Py_RETURN_NONE;
}

static PyObject *
font_options_get_custom_palette_color (PycairoFontOptions *o, PyObject *args) {
  double red, green, blue, alpha;
  unsigned int index;
  cairo_status_t status;

  if (!PyArg_ParseTuple (args, "I:FontOptions.get_custom_palette_color", &index))
    return NULL;

  status = cairo_font_options_get_custom_palette_color (o->font_options, index, &red, &green, &blue, &alpha);
  RETURN_NULL_IF_CAIRO_ERROR (status);
  return Py_BuildValue("(dddd)", red, green, blue, alpha);
}
#endif

static PyObject *
font_options_get_antialias (PycairoFontOptions *o, PyObject *ignored) {
  RETURN_INT_ENUM (Antialias, cairo_font_options_get_antialias (o->font_options));
}

static PyObject *
font_options_get_hint_metrics (PycairoFontOptions *o, PyObject *ignored) {
  RETURN_INT_ENUM (HintMetrics,
                   cairo_font_options_get_hint_metrics (o->font_options));
}

static PyObject *
font_options_get_hint_style (PycairoFontOptions *o, PyObject *ignored) {
  RETURN_INT_ENUM (HintStyle,
                   cairo_font_options_get_hint_style (o->font_options));
}

static PyObject *
font_options_get_subpixel_order (PycairoFontOptions *o, PyObject *ignored) {
  RETURN_INT_ENUM (SubpixelOrder,
                   cairo_font_options_get_subpixel_order (o->font_options));
}

static PyObject *
font_options_set_antialias (PycairoFontOptions *o, PyObject *args) {
  cairo_antialias_t antialias;
  int antialias_arg = CAIRO_ANTIALIAS_DEFAULT;

  if (!PyArg_ParseTuple (args, "|i:FontOptions.set_antialias", &antialias_arg))
    return NULL;

  antialias = (cairo_antialias_t)antialias_arg;

  cairo_font_options_set_antialias (o->font_options, antialias);
  RETURN_NULL_IF_CAIRO_FONT_OPTIONS_ERROR(o->font_options);
  Py_RETURN_NONE;
}

static PyObject *
font_options_set_hint_metrics (PycairoFontOptions *o, PyObject *args) {
  cairo_hint_metrics_t hint_metrics;
  int hint_metrics_arg = CAIRO_HINT_METRICS_DEFAULT;

  if (!PyArg_ParseTuple (args, "|i:FontOptions.set_hint_metrics",
                         &hint_metrics_arg))
    return NULL;

  hint_metrics = (cairo_hint_metrics_t)hint_metrics_arg;

  cairo_font_options_set_hint_metrics (o->font_options, hint_metrics);
  RETURN_NULL_IF_CAIRO_FONT_OPTIONS_ERROR(o->font_options);
  Py_RETURN_NONE;
}

static PyObject *
font_options_set_hint_style (PycairoFontOptions *o, PyObject *args) {
  cairo_hint_style_t hint_style;
  int hint_style_arg = CAIRO_HINT_STYLE_DEFAULT;

  if (!PyArg_ParseTuple (args, "|i:FontOptions.set_hint_style",
                         &hint_style_arg))
    return NULL;

  hint_style = (cairo_hint_style_t)hint_style_arg;

  cairo_font_options_set_hint_style (o->font_options, hint_style);
  RETURN_NULL_IF_CAIRO_FONT_OPTIONS_ERROR(o->font_options);
  Py_RETURN_NONE;
}

static PyObject *
font_options_set_subpixel_order (PycairoFontOptions *o, PyObject *args) {
  cairo_subpixel_order_t subpixel_order;
  int subpixel_order_arg = CAIRO_SUBPIXEL_ORDER_DEFAULT;

  if (!PyArg_ParseTuple (args, "|i:FontOptions.set_subpixel_order",
                         &subpixel_order_arg))
    return NULL;

  subpixel_order = (cairo_subpixel_order_t)subpixel_order_arg;

  cairo_font_options_set_subpixel_order (o->font_options, subpixel_order);
  RETURN_NULL_IF_CAIRO_FONT_OPTIONS_ERROR(o->font_options);
  Py_RETURN_NONE;
}

static PyObject *
font_options_copy (PycairoFontOptions *o, PyObject *ignored) {
  cairo_font_options_t *new;

  Py_BEGIN_ALLOW_THREADS;
  new = cairo_font_options_copy (o->font_options);
  Py_END_ALLOW_THREADS;

  return PycairoFontOptions_FromFontOptions (new);
}

static PyObject *
font_options_hash (PycairoFontOptions *o, PyObject *ignored) {
  unsigned long hash;

  Py_BEGIN_ALLOW_THREADS;
  hash = cairo_font_options_hash (o->font_options);
  Py_END_ALLOW_THREADS;

  return PyLong_FromUnsignedLong (hash);
}

static PyObject *
font_options_equal (PycairoFontOptions *o, PyObject *args) {
  PycairoFontOptions *other;
  cairo_bool_t result;

  if (!PyArg_ParseTuple(args, "O!:FontOptions.equal",
                        &PycairoFontOptions_Type, &other))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  result = cairo_font_options_equal (o->font_options, other->font_options);
  Py_END_ALLOW_THREADS;

  return PyBool_FromLong(result);
}

static PyObject *
font_options_merge (PycairoFontOptions *o, PyObject *args) {
  PycairoFontOptions *other;

  if (!PyArg_ParseTuple(args, "O!:FontOptions.merge",
                        &PycairoFontOptions_Type, &other))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_font_options_merge (o->font_options, other->font_options);
  Py_END_ALLOW_THREADS;

  Py_RETURN_NONE;
}

static PyMethodDef font_options_methods[] = {
  /* methods never exposed in a language binding:
   * cairo_font_options_destroy()
   * cairo_font_options_reference()
   */
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 15, 12)
  {"get_variations",    (PyCFunction)font_options_get_variations, METH_NOARGS},
  {"set_variations",    (PyCFunction)font_options_set_variations, METH_VARARGS},
#endif
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 8)
  {"get_color_mode",    (PyCFunction)font_options_get_color_mode, METH_NOARGS},
  {"set_color_mode",    (PyCFunction)font_options_set_color_mode, METH_VARARGS},
  {"get_color_palette", (PyCFunction)font_options_get_color_palette, METH_NOARGS},
  {"set_color_palette", (PyCFunction)font_options_set_color_palette, METH_VARARGS},
  {"get_custom_palette_color", (PyCFunction)font_options_get_custom_palette_color, METH_VARARGS},
  {"set_custom_palette_color", (PyCFunction)font_options_set_custom_palette_color, METH_VARARGS},
#endif
  {"get_antialias",     (PyCFunction)font_options_get_antialias,  METH_NOARGS},
  {"get_hint_metrics",  (PyCFunction)font_options_get_hint_metrics,
   METH_NOARGS},
  {"get_hint_style",    (PyCFunction)font_options_get_hint_style, METH_NOARGS},
  {"get_subpixel_order",(PyCFunction)font_options_get_subpixel_order,
   METH_NOARGS},
  {"set_antialias",     (PyCFunction)font_options_set_antialias,  METH_VARARGS},
  {"set_hint_metrics",  (PyCFunction)font_options_set_hint_metrics,
   METH_VARARGS},
  {"set_hint_style",    (PyCFunction)font_options_set_hint_style, METH_VARARGS},
  {"set_subpixel_order",(PyCFunction)font_options_set_subpixel_order,
   METH_VARARGS},
  {"copy",              (PyCFunction)font_options_copy,           METH_NOARGS},
  {"hash",              (PyCFunction)font_options_hash,           METH_NOARGS},
  {"equal",             (PyCFunction)font_options_equal,          METH_VARARGS},
  {"merge",             (PyCFunction)font_options_merge,          METH_VARARGS},
  {NULL, NULL, 0, NULL},
};

static PyObject *
font_options_tp_richcompare (PyObject *a, PyObject *b, int op) {
  PycairoFontOptions *other;
  cairo_bool_t is_equal;

  if (!PyObject_TypeCheck (b, &PycairoFontOptions_Type) ||
      (op != Py_EQ && op != Py_NE)) {
    Py_INCREF (Py_NotImplemented);
    return Py_NotImplemented;
  }

  other = (PycairoFontOptions *)b;

  Py_BEGIN_ALLOW_THREADS;
  is_equal = cairo_font_options_equal (((PycairoFontOptions *)a)->font_options,
                                       other->font_options);
  Py_END_ALLOW_THREADS;

  if (is_equal == (op == Py_EQ))
    Py_RETURN_TRUE;
  else
    Py_RETURN_FALSE;
}

PyTypeObject PycairoFontOptions_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.FontOptions",                /* tp_name */
  sizeof(PycairoFontOptions),         /* tp_basicsize */
  0,                                  /* tp_itemsize */
  (destructor)font_options_dealloc,   /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  (hashfunc)PyObject_HashNotImplemented,/* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT,                 /* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  font_options_tp_richcompare,        /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  font_options_methods,               /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  0,                                  /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)font_options_new,          /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
