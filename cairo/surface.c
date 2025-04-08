/* -*- mode: C; c-basic-offset: 2 -*-
 *
 * Pycairo - Python bindings for cairo
 *
 * Copyright © 2003 James Henstridge
 * Copyright © 2004-2011 Steven Chaplin
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


/* Class Surface ---------------------------------------------------------- */

PyObject *
PycairoSurface_FromSurface (cairo_surface_t *surface, PyObject *base) {
  PyTypeObject *type = NULL;
  PyObject *o;

  assert (surface != NULL);

  if (Pycairo_Check_Status (cairo_surface_status (surface))) {
    cairo_surface_destroy (surface);
    return NULL;
  }

  switch (cairo_surface_get_type (surface)) {
#ifdef CAIRO_HAS_IMAGE_SURFACE
  case CAIRO_SURFACE_TYPE_IMAGE:
    type = &PycairoImageSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  case CAIRO_SURFACE_TYPE_PDF:
    type = &PycairoPDFSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_PS_SURFACE
  case CAIRO_SURFACE_TYPE_PS:
    type = &PycairoPSSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_RECORDING_SURFACE
  case CAIRO_SURFACE_TYPE_RECORDING:
    type = &PycairoRecordingSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  case CAIRO_SURFACE_TYPE_SVG:
    type = &PycairoSVGSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_WIN32_SURFACE
  case CAIRO_SURFACE_TYPE_WIN32:
    type = &PycairoWin32Surface_Type;
    break;
  case CAIRO_SURFACE_TYPE_WIN32_PRINTING:
    type = &PycairoWin32PrintingSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_XCB_SURFACE
  case CAIRO_SURFACE_TYPE_XCB:
    type = &PycairoXCBSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_XLIB_SURFACE
  case CAIRO_SURFACE_TYPE_XLIB:
    type = &PycairoXlibSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_SCRIPT_SURFACE
  case CAIRO_SURFACE_TYPE_SCRIPT:
    type = &PycairoScriptSurface_Type;
    break;
#endif
#ifdef CAIRO_HAS_TEE_SURFACE
  case CAIRO_SURFACE_TYPE_TEE:
    type = &PycairoTeeSurface_Type;
    break;
#endif
  default:
    type = &PycairoSurface_Type;
    break;
  }
  o = type->tp_alloc (type, 0);
  if (o == NULL) {
    cairo_surface_destroy (surface);
  } else {
    ((PycairoSurface *)o)->surface = surface;
    Py_XINCREF(base);
    ((PycairoSurface *)o)->base = base;
  }
  return o;
}

/* for use with
 * cairo_surface_write_to_png_stream()
 * cairo_pdf/ps/svg_surface_create_for_stream()
 */
static cairo_status_t
_write_func (void *closure, const unsigned char *data, unsigned int length) {
  PyGILState_STATE gstate = PyGILState_Ensure();
  PyObject *res = PyObject_CallMethod ((PyObject *)closure, "write", "(y#)",
				       data, (Py_ssize_t)length);
  if (res == NULL) {
    PyErr_Clear();
    /* an exception has occurred, it will be picked up later by
     * Pycairo_Check_Status()
     */
    PyGILState_Release(gstate);
    return CAIRO_STATUS_WRITE_ERROR;
  }
  Py_DECREF(res);
  PyGILState_Release(gstate);
  return CAIRO_STATUS_SUCCESS;
}

static const cairo_user_data_key_t surface_base_object_key;
static const cairo_user_data_key_t surface_is_mapped_image;
static const cairo_user_data_key_t surface_buffer_view_key;

static void
surface_dealloc (PycairoSurface *o) {
  if (o->surface) {
    if (cairo_surface_get_user_data (
        o->surface, &surface_is_mapped_image) == NULL) {
      cairo_surface_destroy(o->surface);
    }
    o->surface = NULL;
  }
  Py_CLEAR(o->base);

  Py_TYPE(o)->tp_free(o);
}

static void
_decref_destroy_func(void *user_data) {
  PyGILState_STATE gstate = PyGILState_Ensure();
  Py_DECREF(user_data);
  PyGILState_Release(gstate);
}

/* Like PycairoSurface_FromSurface, but keeps the base object alive as long
 * as cairo_surface_t exists and not as long as the wrapper exists.
 */
static PyObject *
_surface_create_with_object(cairo_surface_t *surface, PyObject *base) {
  PyObject *pysurface;
  cairo_status_t status;

  pysurface = PycairoSurface_FromSurface(surface, NULL);
  if (pysurface == NULL)
    return NULL;

  if (base != NULL) {
    status = cairo_surface_set_user_data(
      surface, &surface_base_object_key, base, _decref_destroy_func);
    if (status != CAIRO_STATUS_SUCCESS)
      Py_DECREF(pysurface);
    RETURN_NULL_IF_CAIRO_ERROR(status);
    Py_INCREF(base);
  }

  return pysurface;
}

static PyObject *
surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyErr_SetString(PyExc_TypeError,
		  "The Surface type cannot be instantiated");
  return NULL;
}

static PyObject *
surface_copy_page (PycairoSurface *o, PyObject *ignored) {
  Py_BEGIN_ALLOW_THREADS;
  cairo_surface_copy_page (o->surface);
  Py_END_ALLOW_THREADS;
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
surface_create_similar (PycairoSurface *o, PyObject *args) {
  cairo_content_t content;
  int width, height, content_arg;

  if (!PyArg_ParseTuple (args, "iii:Surface.create_similar",
                         &content_arg, &width, &height))
    return NULL;

  content = (cairo_content_t)content_arg;

  return PycairoSurface_FromSurface (
	     cairo_surface_create_similar (o->surface, content, width, height),
	     NULL);
}

static PyObject *
surface_finish (PycairoSurface *o, PyObject *ignored) {
  cairo_surface_finish (o->surface);
  Py_CLEAR(o->base);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
surface_flush (PycairoSurface *o, PyObject *ignored) {
  Py_BEGIN_ALLOW_THREADS;
  cairo_surface_flush (o->surface);
  Py_END_ALLOW_THREADS;
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
surface_get_content (PycairoSurface *o, PyObject *ignored) {
  RETURN_INT_ENUM (Content, cairo_surface_get_content (o->surface));
}

static PyObject *
surface_get_device_offset (PycairoSurface *o, PyObject *ignored) {
  double x_offset, y_offset;
  cairo_surface_get_device_offset (o->surface, &x_offset, &y_offset);
  return Py_BuildValue("(dd)", x_offset, y_offset);
}

static PyObject *
surface_get_device_scale (PycairoSurface *o, PyObject *ignored) {
  double x_scale, y_scale;
  cairo_surface_get_device_scale (o->surface, &x_scale, &y_scale);
  return Py_BuildValue("(dd)", x_scale, y_scale);
}

static PyObject *
surface_get_fallback_resolution (PycairoSurface *o, PyObject *ignored) {
  double x_ppi, y_ppi;
  cairo_surface_get_fallback_resolution (o->surface, &x_ppi, &y_ppi);
  return Py_BuildValue("(dd)", x_ppi, y_ppi);
}

static PyObject *
surface_get_font_options (PycairoSurface *o, PyObject *ignored) {
  cairo_font_options_t *options = cairo_font_options_create();

  cairo_surface_get_font_options (o->surface, options);
  /* there is no reference fn */
  return PycairoFontOptions_FromFontOptions (options);
}

static PyObject *
surface_get_device (PycairoSurface *o, PyObject *ignored) {
  cairo_device_t *device = cairo_surface_get_device (o->surface);
  if (device == NULL)
    Py_RETURN_NONE;
  return PycairoDevice_FromDevice (cairo_device_reference (device));
}

static PyObject *
surface_mark_dirty (PycairoSurface *o, PyObject *ignored) {
  cairo_surface_mark_dirty (o->surface);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
surface_mark_dirty_rectangle (PycairoSurface *o, PyObject *args) {
  int x, y, width, height;

  if (!PyArg_ParseTuple(args, "iiii:Surface.mark_dirty_rectangle",
			&x, &y, &width, &height))
    return NULL;

  cairo_surface_mark_dirty_rectangle (o->surface, x, y, width, height);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
surface_set_device_offset (PycairoSurface *o, PyObject *args) {
  double x_offset, y_offset;

  if (!PyArg_ParseTuple (args, "dd:Surface.set_device_offset",
			 &x_offset, &y_offset))
    return NULL;

  cairo_surface_set_device_offset (o->surface, x_offset, y_offset);
  Py_RETURN_NONE;
}

static PyObject *
surface_set_device_scale (PycairoSurface *o, PyObject *args) {
  double x_scale, y_scale;
  cairo_matrix_t transform;

  if (!PyArg_ParseTuple (args, "dd:Surface.set_device_scale",
			 &x_scale, &y_scale))
    return NULL;

  /* cairo asserts the following without reporting an error back.
   * Since we don't want things to crash in Python replicate the logic here.
   */
  cairo_matrix_init_scale (&transform, x_scale, y_scale);
  RETURN_NULL_IF_CAIRO_ERROR (cairo_matrix_invert (&transform));

  cairo_surface_set_device_scale (o->surface, x_scale, y_scale);
  Py_RETURN_NONE;

}

static PyObject *
surface_set_fallback_resolution (PycairoSurface *o, PyObject *args) {
  double x_ppi, y_ppi;

  if (!PyArg_ParseTuple(args, "dd:Surface.set_fallback_resolution",
			&x_ppi, &y_ppi))
    return NULL;
  cairo_surface_set_fallback_resolution (o->surface, x_ppi, y_ppi);
  Py_RETURN_NONE;
}

static PyObject *
surface_create_for_rectangle (PycairoSurface *o, PyObject *args) {
  double x, y, width, height;
  cairo_surface_t *new;

  if (!PyArg_ParseTuple(args, "dddd:Surface.create_for_rectangle",
                        &x, &y, &width, &height))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  new = cairo_surface_create_for_rectangle(o->surface, x, y, width, height);
  Py_END_ALLOW_THREADS;

  return PycairoSurface_FromSurface(new, NULL);
}

static PyObject *
surface_show_page (PycairoSurface *o, PyObject *ignored) {
  Py_BEGIN_ALLOW_THREADS;
  cairo_surface_show_page (o->surface);
  Py_END_ALLOW_THREADS;
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
surface_create_similar_image (PycairoSurface *o, PyObject *args) {
  cairo_format_t format;
  int width, height, format_arg;
  cairo_surface_t *new;

  if (!PyArg_ParseTuple (args, "iii:Surface.create_similar_image",
                         &format_arg, &width, &height))
    return NULL;

  format = (cairo_format_t)format_arg;

  Py_BEGIN_ALLOW_THREADS;
  new = cairo_surface_create_similar_image (o->surface, format, width, height);
  Py_END_ALLOW_THREADS;

  return PycairoSurface_FromSurface (new, NULL);
}

#ifdef CAIRO_HAS_PNG_FUNCTIONS
static PyObject *
surface_write_to_png (PycairoSurface *o, PyObject *args) {
  cairo_status_t status;
  char *name = NULL;
  PyObject *file;

  if (!PyArg_ParseTuple (args, "O:Surface.write_to_png", &file))
    return NULL;

  if (Pycairo_is_fspath (file)) {
    if (!PyArg_ParseTuple (args, "O&:Surface.write_to_png",
                           Pycairo_fspath_converter, &name))
      return NULL;
    Py_BEGIN_ALLOW_THREADS;
    status = cairo_surface_write_to_png (o->surface, name);
    Py_END_ALLOW_THREADS;
    PyMem_Free (name);
  } else {
    if (PyArg_ParseTuple (args, "O&:Surface.write_to_png",
                          Pycairo_writer_converter, &file)) {
      Py_BEGIN_ALLOW_THREADS;
      status = cairo_surface_write_to_png_stream (o->surface, _write_func,
                                                  file);
      Py_END_ALLOW_THREADS;
    } else {
      PyErr_Clear ();
      PyErr_SetString (PyExc_TypeError,
                       "Surface.write_to_png takes one argument which must be "
                       "a filename, file object, or a file-like object "
                       "which has a \"write\" method (like BytesIO) taking bytes");
      return NULL;
    }
  }

  RETURN_NULL_IF_CAIRO_ERROR (status);
  Py_RETURN_NONE;
}
#endif  /* CAIRO_HAS_PNG_FUNCTIONS */

static void
_destroy_mime_user_data_func (PyObject *user_data) {
  PyGILState_STATE gstate = PyGILState_Ensure();

  Py_DECREF(user_data);

  PyGILState_Release(gstate);
}

static void
_destroy_mime_data_func (PyObject *user_data) {
  cairo_surface_t *surface;
  Py_buffer *view;
  PyObject *mime_intern;

  PyGILState_STATE gstate = PyGILState_Ensure();

  /* Remove the user data holding the source object */
  surface = PyCapsule_GetPointer(PyTuple_GET_ITEM(user_data, 0), NULL);
  view = PyCapsule_GetPointer(PyTuple_GET_ITEM(user_data, 1), NULL);
  mime_intern = PyTuple_GET_ITEM(user_data, 3);
  cairo_surface_set_user_data(
    surface, (cairo_user_data_key_t *)mime_intern, NULL, NULL);

  /* Destroy the user data */
  PyBuffer_Release (view);
  PyMem_Free (view);
  Py_DECREF(user_data);

  PyGILState_Release(gstate);
}

static PyObject *
surface_set_mime_data (PycairoSurface *o, PyObject *args) {
  PyObject *obj, *user_data, *mime_intern, *surface_capsule, *view_capsule;
  const char *mime_type;
  int res;
  cairo_status_t status;

  if (!PyArg_ParseTuple(args, "sO:Surface.set_mime_data", &mime_type, &obj))
    return NULL;

  if (obj == Py_None) {
    status = cairo_surface_set_mime_data (
      o->surface, mime_type, NULL, 0, NULL, NULL);

    RETURN_NULL_IF_CAIRO_ERROR(status);
    Py_RETURN_NONE;
  }

  Py_buffer *view = PyMem_Malloc (sizeof (Py_buffer));
  if (view == NULL) {
    PyErr_NoMemory ();
    return NULL;
  }

  res = PyObject_GetBuffer (obj, view, PyBUF_SIMPLE);
  if (res == -1) {
      PyMem_Free (view);
      return NULL;
  }

  /* We use the interned mime type string as user data key and store the
   * passed in object with it. This allows us to return the same object in
   * surface_get_mime_data().
   */
  mime_intern = PyUnicode_InternFromString (mime_type);
  surface_capsule = PyCapsule_New(o->surface, NULL, NULL);
  view_capsule = PyCapsule_New(view, NULL, NULL);
  user_data = Py_BuildValue("(NNOO)", surface_capsule, view_capsule, obj, mime_intern);
  if (user_data == NULL) {
    PyBuffer_Release (view);
    PyMem_Free (view);
    return NULL;
  }

  status = cairo_surface_set_user_data(
    o->surface, (cairo_user_data_key_t *)mime_intern, user_data,
    (cairo_destroy_func_t)_destroy_mime_user_data_func);

  if (status != CAIRO_STATUS_SUCCESS) {
    PyBuffer_Release (view);
    PyMem_Free (view);
    Py_DECREF(user_data);
    Pycairo_Check_Status (status);
    return NULL;
  }

  Py_INCREF(user_data);
  status = cairo_surface_set_mime_data (
    o->surface, mime_type, view->buf, (unsigned long)view->len,
    (cairo_destroy_func_t)_destroy_mime_data_func, user_data);

  if (status != CAIRO_STATUS_SUCCESS) {
    cairo_surface_set_user_data(
      o->surface, (cairo_user_data_key_t *)mime_intern, NULL, NULL);
    PyBuffer_Release (view);
    PyMem_Free (view);
    Py_DECREF(user_data);
    Pycairo_Check_Status (status);
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject *
surface_get_mime_data (PycairoSurface *o, PyObject *args) {
  PyObject *user_data, *obj, *mime_intern;
  const char *mime_type;
  const unsigned char *buffer;
  unsigned long buffer_len;

  if (!PyArg_ParseTuple(args, "s:Surface.get_mime_data", &mime_type))
    return NULL;

  cairo_surface_get_mime_data (o->surface, mime_type, &buffer, &buffer_len);
  if (buffer == NULL) {
    Py_RETURN_NONE;
  }

  mime_intern = PyUnicode_InternFromString (mime_type);
  user_data = cairo_surface_get_user_data(
    o->surface, (cairo_user_data_key_t *)mime_intern);

  if (user_data == NULL) {
    /* In case the mime data wasn't set through the Python API just copy it */
    return Py_BuildValue("y#", buffer, buffer_len);
  } else {
    obj = PyTuple_GET_ITEM(user_data, 2);
    Py_INCREF(obj);
    return obj;
  }
}

static PyObject *
surface_supports_mime_type (PycairoSurface *self, PyObject *args) {
  const char *mime_type;

  if (!PyArg_ParseTuple(args, "s:Surface.supports_mime_type", &mime_type))
    return NULL;

  return PyBool_FromLong(
    cairo_surface_supports_mime_type(self->surface, mime_type));
}

static PyObject *
surface_has_show_text_glyphs (PycairoSurface *o, PyObject *ignored) {
  cairo_bool_t result;

  Py_BEGIN_ALLOW_THREADS;
  result = cairo_surface_has_show_text_glyphs (o->surface);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);
  return PyBool_FromLong (result);
}

#ifdef CAIRO_HAS_IMAGE_SURFACE

static PyObject *
surface_map_to_image (PycairoSurface *self, PyObject *args) {
  PyObject *pyextents, *pymapped;
  cairo_rectangle_int_t *extents;
  cairo_surface_t *mapped_surface;

  if (!PyArg_ParseTuple(args, "O:Surface.map_to_image", &pyextents))
    return NULL;

  if (PyObject_TypeCheck (pyextents, &PycairoRectangleInt_Type)) {
    extents = &(((PycairoRectangleInt *)pyextents)->rectangle_int);
  } else {
    if (pyextents == Py_None) {
      extents = NULL;
    } else {
      PyErr_SetString (PyExc_TypeError,
        "argument must be a RectangleInt or None.");
      return NULL;
    }
  }

  Py_BEGIN_ALLOW_THREADS;
  mapped_surface = cairo_surface_map_to_image (self->surface, extents);
  Py_END_ALLOW_THREADS;

  if (Pycairo_Check_Status (cairo_surface_status (mapped_surface))) {
    cairo_surface_destroy (mapped_surface);
    return NULL;
  }

  /* So we can skip the destroy() call in the base tp_dealloc */
  cairo_surface_set_user_data (
    mapped_surface, &surface_is_mapped_image, (void*)1, NULL);

  pymapped = PycairoMappedImageSurface_Type.tp_alloc (
    &PycairoMappedImageSurface_Type, 0);

  if (pymapped == NULL) {
    Py_BEGIN_ALLOW_THREADS;
    cairo_surface_unmap_image (self->surface, mapped_surface);
    Py_END_ALLOW_THREADS;
    return NULL;
  }

  ((PycairoSurface *)pymapped)->surface = mapped_surface;
  Py_XINCREF (self);
  ((PycairoSurface *)pymapped)->base = (PyObject *)self;

  return pymapped;
}

static PyObject *
surface_unmap_image (PycairoSurface *self, PyObject *args) {
  PycairoSurface *pymapped;
  cairo_surface_t *base_surface, *fake_surface;

  if (!PyArg_ParseTuple(args, "O!:Surface.unmap_image",
      &PycairoMappedImageSurface_Type, &pymapped))
    return NULL;

  if (cairo_surface_get_user_data (pymapped->surface,
      &surface_is_mapped_image) == NULL) {
    PyErr_SetString (PyExc_RuntimeError,
      "MappedImageSurface was already unmapped");
    return NULL;
  }

  base_surface = ((PycairoSurface *)(pymapped->base))->surface;
  if (base_surface != self->surface) {
    PyErr_SetString (PyExc_ValueError,
      "ImageSurface isn't mapped from this surface");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS;
  cairo_surface_unmap_image (self->surface, pymapped->surface);
  Py_END_ALLOW_THREADS;

  /* Replace the mapped image surface with a fake one and finish it so
   * that any operation on it fails.
   */
  fake_surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 0, 0);
  cairo_surface_finish (fake_surface);
  pymapped->surface = fake_surface;
  /* We no longer need the base surface */
  Py_CLEAR(pymapped->base);

  Py_RETURN_NONE;
}

#endif /* CAIRO_HAS_IMAGE_SURFACE */

static PyObject *
surface_ctx_enter (PyObject *obj, PyObject *ignored) {
  Py_INCREF (obj);
  return obj;
}

static PyObject *
surface_ctx_exit (PycairoSurface *obj, PyObject *args) {
  Py_BEGIN_ALLOW_THREADS;
  cairo_surface_finish (obj->surface);
  Py_END_ALLOW_THREADS;
  Py_RETURN_NONE;
}

static PyMethodDef surface_methods[] = {
  /* methods never exposed in a language binding:
   * cairo_surface_destroy()
   * cairo_surface_get_type()
   * cairo_surface_get_user_data()
   * cairo_surface_reference()
   * cairo_surface_set_user_data()
   */
  {"__enter__",      (PyCFunction)surface_ctx_enter,          METH_NOARGS},
  {"__exit__",       (PyCFunction)surface_ctx_exit,           METH_VARARGS},
  {"copy_page",      (PyCFunction)surface_copy_page,          METH_NOARGS},
  {"create_similar", (PyCFunction)surface_create_similar,     METH_VARARGS},
  {"create_similar_image", (PyCFunction)surface_create_similar_image,
   METH_VARARGS},
  {"finish",         (PyCFunction)surface_finish,             METH_NOARGS},
  {"flush",          (PyCFunction)surface_flush,              METH_NOARGS},
  {"get_content",    (PyCFunction)surface_get_content,        METH_NOARGS},
  {"get_device_offset",(PyCFunction)surface_get_device_offset,METH_NOARGS},
  {"get_device_scale", (PyCFunction)surface_get_device_scale, METH_NOARGS},
  {"get_fallback_resolution",(PyCFunction)surface_get_fallback_resolution,
   METH_NOARGS},
  {"get_font_options",(PyCFunction)surface_get_font_options,  METH_NOARGS},
  {"get_device",     (PyCFunction)surface_get_device,         METH_NOARGS},
  {"mark_dirty",     (PyCFunction)surface_mark_dirty,         METH_NOARGS},
  {"mark_dirty_rectangle", (PyCFunction)surface_mark_dirty_rectangle,
   METH_VARARGS},
  {"set_device_offset",(PyCFunction)surface_set_device_offset,METH_VARARGS},
  {"set_device_scale", (PyCFunction)surface_set_device_scale, METH_VARARGS},
  {"set_fallback_resolution",(PyCFunction)surface_set_fallback_resolution,
   METH_VARARGS},
  {"show_page",      (PyCFunction)surface_show_page,          METH_NOARGS},
#ifdef CAIRO_HAS_PNG_FUNCTIONS
  {"write_to_png",   (PyCFunction)surface_write_to_png,       METH_VARARGS},
#endif
  {"set_mime_data",  (PyCFunction)surface_set_mime_data,      METH_VARARGS},
  {"get_mime_data",  (PyCFunction)surface_get_mime_data,      METH_VARARGS},
  {"supports_mime_type", (PyCFunction)surface_supports_mime_type,
   METH_VARARGS},
  {"create_for_rectangle",(PyCFunction)surface_create_for_rectangle,
   METH_VARARGS},
  {"has_show_text_glyphs", (PyCFunction)surface_has_show_text_glyphs,
   METH_NOARGS},
#ifdef CAIRO_HAS_IMAGE_SURFACE
  {"map_to_image", (PyCFunction)surface_map_to_image, METH_VARARGS},
  {"unmap_image", (PyCFunction)surface_unmap_image, METH_VARARGS},
#endif
  {NULL, NULL, 0, NULL},
};

static PyObject*
surface_richcompare (PyObject *self, PyObject *other, int op)
{
  if (Py_TYPE(self) == Py_TYPE(other))
    return Pycairo_richcompare (
      ((PycairoSurface *)self)->surface,
      ((PycairoSurface *)other)->surface,
      op);
  else {
    Py_INCREF(Py_NotImplemented);
    return Py_NotImplemented;
  }
}

static Py_hash_t
surface_hash (PyObject *self)
{
  return PYCAIRO_Py_hash_t_FromVoidPtr (((PycairoSurface *)self)->surface);
}

PyTypeObject PycairoSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.Surface",                    /* tp_name */
  sizeof(PycairoSurface),             /* tp_basicsize */
  0,                                  /* tp_itemsize */
  (destructor)surface_dealloc,        /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  surface_hash,                       /* tp_hash */
  0,                                  /* tp_call */
  0,                                  /* tp_str */
  0,                                  /* tp_getattro */
  0,                                  /* tp_setattro */
  0,                                  /* tp_as_buffer */
  Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/* tp_flags */
  0,                                  /* tp_doc */
  0,                                  /* tp_traverse */
  0,                                  /* tp_clear */
  surface_richcompare,                /* tp_richcompare */
  0,                                  /* tp_weaklistoffset */
  0,                                  /* tp_iter */
  0,                                  /* tp_iternext */
  surface_methods,                    /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  0,                                  /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)surface_new,               /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


/* Class ImageSurface(Surface) -------------------------------------------- */
#ifdef CAIRO_HAS_IMAGE_SURFACE

static PyObject *
image_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  cairo_format_t format;
  int width, height, format_arg;

  if (!PyArg_ParseTuple (args, "iii:ImageSurface.__new__",
                         &format_arg, &width, &height))
    return NULL;

  format = (cairo_format_t)format_arg;

  return PycairoSurface_FromSurface (
	     cairo_image_surface_create (format, width, height),
	     NULL);
}

static void
_release_buffer_destroy_func (void *user_data) {
  Py_buffer *view = (Py_buffer *)user_data;
  PyGILState_STATE gstate = PyGILState_Ensure();
  PyBuffer_Release (view);
  PyMem_Free (view);
  PyGILState_Release(gstate);
}

/* METH_CLASS */
static PyObject *
image_surface_create_for_data (PyTypeObject *type, PyObject *args) {
  cairo_surface_t *surface;
  cairo_format_t format;
  cairo_status_t status;
  int width, height, stride = -1, res, format_arg;
  PyObject *obj;

  if (!PyArg_ParseTuple (args, "Oiii|i:ImageSurface.create_for_data",
                         &obj, &format_arg, &width, &height, &stride))
    return NULL;

  format = (cairo_format_t)format_arg;

  if (width <= 0) {
    PyErr_SetString(PyExc_ValueError, "width must be positive");
    return NULL;
  }
  if (height <= 0) {
    PyErr_SetString(PyExc_ValueError, "height must be positive");
    return NULL;
  }
  /* if stride is missing, calculate it from width */
  if (stride < 0) {
    stride = cairo_format_stride_for_width (format, width);
    if (stride == -1){
      PyErr_SetString(PyExc_ValueError,
		      "format is invalid or the width too large");
      return NULL;
    }
  }

  Py_buffer *view = PyMem_Malloc (sizeof (Py_buffer));
  if (view == NULL) {
    PyErr_NoMemory ();
    return NULL;
  }

  res = PyObject_GetBuffer (obj, view, PyBUF_WRITABLE);
  if (res == -1) {
    PyMem_Free (view);
    return NULL;
  }

  if (height * stride > view->len) {
    PyBuffer_Release (view);
    PyMem_Free (view);
    PyErr_SetString(PyExc_TypeError, "buffer is not long enough");
    return NULL;
  }

  Py_BEGIN_ALLOW_THREADS;
  surface = cairo_image_surface_create_for_data (view->buf, format, width,
                                                 height, stride);
  Py_END_ALLOW_THREADS;

  status = cairo_surface_set_user_data(
    surface, &surface_buffer_view_key, view,
    (cairo_destroy_func_t)_release_buffer_destroy_func);
  if (Pycairo_Check_Status (status)) {
    cairo_surface_destroy (surface);
    PyBuffer_Release (view);
    PyMem_Free (view);
    return NULL;
  }

  return PycairoSurface_FromSurface(surface, NULL);
}


#ifdef CAIRO_HAS_PNG_FUNCTIONS
static cairo_status_t
_read_func (void *closure, unsigned char *data, unsigned int length) {
  char *buffer;
  int ret;
  Py_ssize_t str_length;
  cairo_status_t status = CAIRO_STATUS_READ_ERROR;
  PyGILState_STATE gstate = PyGILState_Ensure();
  PyObject *pystr = PyObject_CallMethod ((PyObject *)closure, "read", "(i)",
					 length);
  if (pystr == NULL) {
    PyErr_Clear();
    /* an exception has occurred, it will be picked up later by
     * Pycairo_Check_Status()
     */
    goto end;
  }
  ret = PyBytes_AsStringAndSize (pystr, &buffer, &str_length);
  if (ret == -1 || str_length < (Py_ssize_t)length) {
    PyErr_Clear();
    goto end;
  }
  /* don't use strncpy() since png data may contain NUL bytes */
  memcpy (data, buffer, (size_t)str_length);
  status = CAIRO_STATUS_SUCCESS;
 end:
  Py_XDECREF(pystr);
  PyGILState_Release(gstate);
  return status;
}

/* METH_CLASS */
static PyObject *
image_surface_create_from_png (PyTypeObject *type, PyObject *args) {
  cairo_surface_t *image_surface;
  PyObject *file;
  char *name;

  if (!PyArg_ParseTuple (args, "O:ImageSurface.create_from_png", &file))
    return NULL;

  if (Pycairo_is_fspath (file)) {
    if (!PyArg_ParseTuple(args, "O&:ImageSurface.create_from_png",
                          Pycairo_fspath_converter, &name))
      return NULL;

    Py_BEGIN_ALLOW_THREADS;
    image_surface = cairo_image_surface_create_from_png (name);
    Py_END_ALLOW_THREADS;
    PyMem_Free(name);
    return PycairoSurface_FromSurface (image_surface, NULL);
  } else {
    if (PyArg_ParseTuple (args, "O&:ImageSurface.create_from_png",
                          Pycairo_reader_converter, &file)) {
      Py_BEGIN_ALLOW_THREADS;
      image_surface = cairo_image_surface_create_from_png_stream (
        _read_func, file);
      Py_END_ALLOW_THREADS;
      return PycairoSurface_FromSurface (image_surface, NULL);
    } else {
      PyErr_SetString(PyExc_TypeError,
                      "ImageSurface.create_from_png argument must be a "
                      "filename (str), file object, or an object that has a "
                      "\"read\" method (like BytesIO) returning bytes.");
      return NULL;
    }
  }
}
#endif /* CAIRO_HAS_PNG_FUNCTIONS */

/* METH_STATIC */
static PyObject *
image_surface_format_stride_for_width (PyObject *self, PyObject *args) {
  cairo_format_t format;
  int width, format_arg;

  if (!PyArg_ParseTuple (args, "ii:format_stride_for_width",
                         &format_arg, &width))
    return NULL;

  format = (cairo_format_t)format_arg;

  return PyLong_FromLong (cairo_format_stride_for_width (format, width));
}

static PyObject *
image_surface_get_data (PycairoImageSurface *o, PyObject *ignored) {
  cairo_surface_t *surface;
  int height, stride;
  unsigned char * buffer;

  surface = o->surface;

  // We need to figure out if the surface is finished or not to see
  // if the buffer is still valid.
  // https://gitlab.freedesktop.org/cairo/cairo/-/issues/406
  // https://github.com/gtk-rs/cairo/issues/323
  cairo_t *ctx = cairo_create (surface);
  cairo_status_t status = cairo_status (ctx);
  cairo_destroy (ctx);
  if (Pycairo_Check_Status (status)) {
    return NULL;
  }

  buffer = cairo_image_surface_get_data (surface);
  if (buffer == NULL) {
    Py_RETURN_NONE;
  }
  height = cairo_image_surface_get_height (surface);
  stride = cairo_image_surface_get_stride (surface);

  return buffer_proxy_create_view((PyObject *)o, buffer, height * stride, 0);
}

static PyObject *
image_surface_get_format (PycairoImageSurface *o, PyObject *ignored) {
  RETURN_INT_ENUM (Format, cairo_image_surface_get_format (o->surface));
}

static PyObject *
image_surface_get_height (PycairoImageSurface *o, PyObject *ignored) {
  return PyLong_FromLong (cairo_image_surface_get_height (o->surface));
}

static PyObject *
image_surface_get_stride (PycairoImageSurface *o, PyObject *ignored) {
  return PyLong_FromLong (cairo_image_surface_get_stride (o->surface));
}

static PyObject *
image_surface_get_width (PycairoImageSurface *o, PyObject *ignored) {
  return PyLong_FromLong (cairo_image_surface_get_width (o->surface));
}

static PyMethodDef image_surface_methods[] = {
  {"create_for_data",(PyCFunction)image_surface_create_for_data,
   METH_VARARGS | METH_CLASS},
#ifdef CAIRO_HAS_PNG_FUNCTIONS
  {"create_from_png", (PyCFunction)image_surface_create_from_png,
   METH_VARARGS | METH_CLASS},
#endif
  {"format_stride_for_width",
   (PyCFunction)image_surface_format_stride_for_width,
   METH_VARARGS | METH_STATIC},
  {"get_data",      (PyCFunction)image_surface_get_data,        METH_NOARGS},
  {"get_format",    (PyCFunction)image_surface_get_format,      METH_NOARGS},
  {"get_height",    (PyCFunction)image_surface_get_height,      METH_NOARGS},
  {"get_stride",    (PyCFunction)image_surface_get_stride,      METH_NOARGS},
  {"get_width",     (PyCFunction)image_surface_get_width,       METH_NOARGS},
  {NULL, NULL, 0, NULL},
};


PyTypeObject PycairoImageSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.ImageSurface",               /* tp_name */
  sizeof(PycairoImageSurface),        /* tp_basicsize */
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
  image_surface_methods,              /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)image_surface_new,         /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


/* Mapped Image Type*/

static PyObject *
mapped_image_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyErr_SetString(PyExc_TypeError,
                  "The MappedImage type cannot be instantiated");
  return NULL;
}

static void
mapped_image_surface_dealloc (PycairoImageSurface *self) {
  PycairoSurface *pybasesurface = (PycairoSurface *)(self->base);

  if (cairo_surface_get_user_data (
      self->surface, &surface_is_mapped_image) != NULL) {
    cairo_surface_unmap_image (pybasesurface->surface, self->surface);
    self->surface = NULL;
  } else {
    cairo_surface_destroy(self->surface);
    self->surface = NULL;
  }

  Py_CLEAR (self->base);
  Py_TYPE (self)->tp_free (self);
}

static PyObject *
mapped_image_surface_finish (PycairoSurface *self, PyObject *ignored) {
  PyErr_SetString(PyExc_RuntimeError,
    "The MappedImage type cannot be finished, "
    "use Surface.unmap_image instead");
  return NULL;
}

static PyObject *
mapped_image_surface_ctx_exit (PycairoImageSurface *self, PyObject *args) {
  PycairoSurface *pybasesurface = (PycairoSurface *)(self->base);
  PyObject *subargs, *result;

  subargs = Py_BuildValue("(O)", self);
  if (subargs == NULL)
    return NULL;
  result = surface_unmap_image (pybasesurface, subargs);
  Py_DECREF (subargs);
  return result;
}

static PyMethodDef mapped_image_surface_methods[] = {
  {"__exit__",     (PyCFunction)mapped_image_surface_ctx_exit,   METH_VARARGS},
  {"finish",       (PyCFunction)mapped_image_surface_finish,     METH_NOARGS},
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoMappedImageSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "__cairo__.MappedImageSurface",     /* tp_name */
  sizeof(PycairoImageSurface),        /* tp_basicsize */
  0,                                  /* tp_itemsize */
  (destructor)
    mapped_image_surface_dealloc,     /* tp_dealloc */
  0,                                  /* tp_print */
  0,                                  /* tp_getattr */
  0,                                  /* tp_setattr */
  0,                                  /* tp_compare */
  0,                                  /* tp_repr */
  0,                                  /* tp_as_number */
  0,                                  /* tp_as_sequence */
  0,                                  /* tp_as_mapping */
  PyObject_HashNotImplemented,        /* tp_hash */
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
  mapped_image_surface_methods,       /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoImageSurface_Type,          /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)mapped_image_surface_new,  /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


#endif /* CAIRO_HAS_IMAGE_SURFACE */


/* Class PDFSurface(Surface) ---------------------------------------------- */
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>

static PyObject *
pdf_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  double width_in_points, height_in_points;
  PyObject *file;
  cairo_surface_t *sfc;
  char *name;

  if (!PyArg_ParseTuple (args, "Odd:PDFSurface.__new__",
                         &file, &width_in_points, &height_in_points))
    return NULL;

  if (Pycairo_is_fspath (file) || file == Py_None) {
    if (!PyArg_ParseTuple (args, "O&dd:PDFSurface.__new__",
                           Pycairo_fspath_none_converter, &name,
                           &width_in_points, &height_in_points))
      return NULL;

    Py_BEGIN_ALLOW_THREADS;
    sfc = cairo_pdf_surface_create (name, width_in_points, height_in_points);
    Py_END_ALLOW_THREADS;
    PyMem_Free(name);
    return PycairoSurface_FromSurface (sfc, NULL);
  } else {
    if (PyArg_ParseTuple (args, "O&dd:PDFSurface.__new__",
                          Pycairo_writer_converter, &file,
                          &width_in_points, &height_in_points)) {
      Py_BEGIN_ALLOW_THREADS;
      sfc = cairo_pdf_surface_create_for_stream (
        _write_func, file, width_in_points, height_in_points);
      Py_END_ALLOW_THREADS;
      return _surface_create_with_object (sfc, file);
    } else {
      PyErr_Clear ();
      PyErr_SetString(PyExc_TypeError,
                      "PDFSurface argument 1 must be "
                      "None, or a filename (str), or "
                      "a file object, or an object that has a "
                      "\"write\" method (like BytesIO) taking bytes.");
      return NULL;
    }
  }
}

static PyObject *
pdf_surface_set_size (PycairoPDFSurface *o, PyObject *args) {
  double width_in_points, height_in_points;

  if (!PyArg_ParseTuple(args, "dd:PDFSurface.set_size", &width_in_points,
			&height_in_points))
    return NULL;
  cairo_pdf_surface_set_size (o->surface, width_in_points,
			      height_in_points);
  Py_RETURN_NONE;
}

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 6)
static PyObject *
pdf_surface_set_custom_metadata (PycairoPDFSurface *o, PyObject *args) {
  const char *name;
  const char *value;

  if (!PyArg_ParseTuple (args, "sz:PDFSurface.set_custom_metadata",
                         &name, &value))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_pdf_surface_set_custom_metadata (o->surface, name, value);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);
  Py_RETURN_NONE;
}
#endif

static PyObject *
pdf_get_versions (PyObject *self, PyObject *ignored) {
  PyObject *list, *num;
  const cairo_pdf_version_t *versions;
  int i, num_versions;

  Py_BEGIN_ALLOW_THREADS;
  cairo_pdf_get_versions (&versions, &num_versions);
  Py_END_ALLOW_THREADS;

  list = PyList_New (num_versions);
  if (list == NULL)
    return NULL;

  for (i=0; i < num_versions; i++) {
    num = CREATE_INT_ENUM (PDFVersion, versions[i]);
    if (num == NULL) {
      Py_DECREF (list);
      return NULL;
    }
    PyList_SET_ITEM (list, i, num);
  }

  return list;
}

static PyObject *
pdf_version_to_string (PyObject *self,  PyObject *args) {
  cairo_pdf_version_t version;
  int version_arg;
  const char *version_string;

  if (!PyArg_ParseTuple (args, "i:PDFSurface.version_to_string", &version_arg))
    return NULL;

  version = (cairo_pdf_version_t)version_arg;

  /* See https://gitlab.freedesktop.org/cairo/cairo/-/issues/590
   * if version < 0, cairo_pdf_version_to_string will return out-of-bound
   * memory address and could cause problems. We avoid those by checking
   * whether the version is greater than 0 and only then calling
   * that function.
  */
  if (version >= 0) {
    Py_BEGIN_ALLOW_THREADS;
    version_string = cairo_pdf_version_to_string (version);
    Py_END_ALLOW_THREADS;
  } else {
    version_string = NULL;
  }

  if (version_string == NULL) {
    PyErr_SetString (PyExc_ValueError, "invalid version");
    return NULL;
  }

  return PyUnicode_FromString (version_string);
}

static PyObject *
pdf_surface_restrict_to_version (PycairoPDFSurface *o, PyObject *args) {
  cairo_pdf_version_t version;
  int version_arg;

  if (!PyArg_ParseTuple (args, "i:PDFSurface.restrict_to_version",
                         &version_arg))
    return NULL;

  version = (cairo_pdf_version_t)version_arg;

  Py_BEGIN_ALLOW_THREADS;
  cairo_pdf_surface_restrict_to_version (o->surface, version);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);
  Py_RETURN_NONE;
}

static PyObject *
pdf_surface_set_page_label (PycairoPDFSurface *o, PyObject *args) {
  const char *utf8;

  if (!PyArg_ParseTuple (args, "es:PDFSurface.set_page_label",
                         "utf-8", &utf8))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_pdf_surface_set_page_label (o->surface, utf8);
  Py_END_ALLOW_THREADS;

  PyMem_Free((void *)utf8);

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);
  Py_RETURN_NONE;
}

static PyObject *
pdf_surface_set_metadata (PycairoPDFSurface *o, PyObject *args) {
  const char *utf8;
  cairo_pdf_metadata_t metadata;
  int metadata_arg;

  if (!PyArg_ParseTuple (args, "ies:PDFSurface.set_metadata",
                         &metadata_arg, "utf-8", &utf8))
    return NULL;

  metadata = (cairo_pdf_metadata_t)metadata_arg;

  Py_BEGIN_ALLOW_THREADS;
  cairo_pdf_surface_set_metadata (o->surface, metadata, utf8);
  Py_END_ALLOW_THREADS;

  PyMem_Free((void *)utf8);

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);
  Py_RETURN_NONE;
}

static PyObject *
pdf_surface_set_thumbnail_size (PycairoPDFSurface *o, PyObject *args) {
  int width, height;

  if (!PyArg_ParseTuple (args, "ii:PDFSurface.set_thumbnail_size",
                         &width, &height))
    return NULL;

  Py_BEGIN_ALLOW_THREADS;
  cairo_pdf_surface_set_thumbnail_size (o->surface, width, height);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);
  Py_RETURN_NONE;
}

static PyObject *
pdf_surface_add_outline (PycairoPDFSurface *o, PyObject *args) {

  int parent_id, added_id;
  const char *utf8;
  const char *link_attribs;
  cairo_pdf_outline_flags_t flags;
  int flags_arg;

  if (!PyArg_ParseTuple (args, "iesesi:PDFSurface.add_outline",
                         &parent_id, "utf-8", &utf8, "utf-8", &link_attribs, &flags_arg))
    return NULL;

  flags = (cairo_pdf_outline_flags_t)flags_arg;

  Py_BEGIN_ALLOW_THREADS;
  added_id = cairo_pdf_surface_add_outline (o->surface, parent_id, utf8, link_attribs, flags);
  Py_END_ALLOW_THREADS;

  PyMem_Free((void *)utf8);
  PyMem_Free((void *)link_attribs);

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);

  return PyLong_FromLong (added_id);
}

static PyMethodDef pdf_surface_methods[] = {
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 6)
  {"set_custom_metadata", (PyCFunction)pdf_surface_set_custom_metadata, METH_VARARGS},
#endif
  {"set_page_label", (PyCFunction)pdf_surface_set_page_label, METH_VARARGS},
  {"set_metadata", (PyCFunction)pdf_surface_set_metadata, METH_VARARGS},
  {"set_thumbnail_size", (PyCFunction)pdf_surface_set_thumbnail_size, METH_VARARGS},
  {"add_outline", (PyCFunction)pdf_surface_add_outline, METH_VARARGS},
  {"set_size", (PyCFunction)pdf_surface_set_size, METH_VARARGS},
  {"get_versions", (PyCFunction)pdf_get_versions, METH_NOARGS | METH_STATIC},
  {"version_to_string", (PyCFunction)pdf_version_to_string,
   METH_VARARGS | METH_STATIC},
  {"restrict_to_version", (PyCFunction)pdf_surface_restrict_to_version,
   METH_VARARGS},
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoPDFSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.PDFSurface",                 /* tp_name */
  sizeof(PycairoPDFSurface),          /* tp_basicsize */
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
  pdf_surface_methods,                /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)pdf_surface_new,           /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
#endif /* CAIRO_HAS_PDF_SURFACE */

#ifdef CAIRO_HAS_SCRIPT_SURFACE
#include <cairo-script.h>

typedef PycairoSurface PycairoScriptSurface;

static PyObject *
script_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  cairo_content_t content;
  int content_arg;
  double width, height;
  PyObject *pydevice;

  if (!PyArg_ParseTuple (args, "O!idd:ScriptSurface.__new__",
      &PycairoScriptDevice_Type, &pydevice, &content_arg, &width, &height))
    return NULL;

  content = (cairo_content_t)content_arg;

  return PycairoSurface_FromSurface (
    cairo_script_surface_create (
      ((PycairoDevice*)pydevice)->device, content, width, height),
    NULL);
}

static PyObject *
script_surface_create_for_target (PyTypeObject *type, PyObject *args) {
  PyObject *pydevice, *target;

  if (!PyArg_ParseTuple (args, "O!O!:ScriptSurface.create_for_target",
        &PycairoScriptDevice_Type, &pydevice, &PycairoSurface_Type, &target))
    return NULL;

  return PycairoSurface_FromSurface (
    cairo_script_surface_create_for_target (
      ((PycairoDevice*)pydevice)->device, ((PycairoSurface*)target)->surface),
    NULL);
}

static PyMethodDef script_surface_methods[] = {
  {"create_for_target",
    (PyCFunction)script_surface_create_for_target, METH_VARARGS | METH_CLASS},
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoScriptSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.ScriptSurface",              /* tp_name */
  sizeof(PycairoScriptSurface),      /* tp_basicsize */
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
  script_surface_methods,             /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)script_surface_new,        /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};

#endif /* CAIRO_HAS_SCRIPT_SURFACE */


/* Class PSSurface(Surface) ----------------------------------------------- */
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>

static PyObject *
ps_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  double width_in_points, height_in_points;
  PyObject *file;
  cairo_surface_t *sfc;
  char *name;

  if (!PyArg_ParseTuple (args, "Odd:PSSurface.__new__",
                         &file, &width_in_points, &height_in_points))
    return NULL;

  if (Pycairo_is_fspath (file) || file == Py_None) {
    if (!PyArg_ParseTuple (args, "O&dd:PSSurface.__new__",
                           Pycairo_fspath_none_converter, &name,
                           &width_in_points, &height_in_points))
      return NULL;

    Py_BEGIN_ALLOW_THREADS;
    sfc = cairo_ps_surface_create (name, width_in_points, height_in_points);
    Py_END_ALLOW_THREADS;
    PyMem_Free(name);
    return PycairoSurface_FromSurface (sfc, NULL);
  } else {
    if (PyArg_ParseTuple (args, "O&dd:PSSurface.__new__",
                          Pycairo_writer_converter, &file,
                          &width_in_points, &height_in_points)) {
      Py_BEGIN_ALLOW_THREADS;
      sfc = cairo_ps_surface_create_for_stream (
        _write_func, file, width_in_points, height_in_points);
      Py_END_ALLOW_THREADS;
      return _surface_create_with_object (sfc, file);
    } else {
      PyErr_Clear ();
      PyErr_SetString(PyExc_TypeError,
                      "PSSurface argument 1 must be "
                      "None, or a filename (str), or "
                      "a file object, or an object that has a "
                      "\"write\" method (like BytesIO) taking bytes.");
      return NULL;
    }
  }
}

static PyObject *
ps_surface_dsc_begin_page_setup (PycairoPSSurface *o, PyObject *ignored) {
  cairo_ps_surface_dsc_begin_page_setup (o->surface);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
ps_surface_dsc_begin_setup (PycairoPSSurface *o, PyObject *ignored) {
  cairo_ps_surface_dsc_begin_setup (o->surface);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
ps_surface_dsc_comment (PycairoPSSurface *o, PyObject *args) {
  const char *comment;
  if (!PyArg_ParseTuple(args, "s:PSSurface.dsc_comment", &comment))
    return NULL;
  cairo_ps_surface_dsc_comment (o->surface, comment);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
ps_surface_get_eps (PycairoPSSurface *o, PyObject *ignored) {
  PyObject *eps = cairo_ps_surface_get_eps (o->surface) ? Py_True : Py_False;
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_INCREF(eps);
  return eps;
}

/* METH_STATIC */
static PyObject *
ps_level_to_string (PyObject *self, PyObject *args) {
  cairo_ps_level_t level;
  int level_arg;
  const char *s;

  if (!PyArg_ParseTuple (args, "i:PSSurface.level_to_string", &level_arg))
    return NULL;

  level = (cairo_ps_level_t)level_arg;

  s = cairo_ps_level_to_string (level);
  if (s == NULL){
    PyErr_SetString(PyExc_ValueError, "level_to_string: "
		    "invalid level argument");
    return NULL;
  }
  return PyUnicode_FromString (s);
}

static PyObject *
ps_surface_restrict_to_level (PycairoPSSurface *o, PyObject *args) {
  cairo_ps_level_t level;
  int level_arg;

  if (!PyArg_ParseTuple (args, "i:PSSurface.restrict_to_level", &level_arg))
    return NULL;

  level = (cairo_ps_level_t)level_arg;

  cairo_ps_surface_restrict_to_level (o->surface, level);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
ps_surface_set_eps (PycairoPSSurface *o, PyObject *args) {
  PyObject *py_eps;
  if (!PyArg_ParseTuple(args, "O!:PSSurface.set_eps",
			&PyBool_Type, &py_eps))
    return NULL;
  cairo_ps_surface_set_eps (o->surface, (py_eps == Py_True));
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR(o->surface);
  Py_RETURN_NONE;
}

static PyObject *
ps_surface_set_size (PycairoPSSurface *o, PyObject *args) {
  double width_in_points, height_in_points;

  if (!PyArg_ParseTuple(args, "dd:PSSurface.set_size",
			&width_in_points, &height_in_points))
    return NULL;
  cairo_ps_surface_set_size (o->surface, width_in_points, height_in_points);
  Py_RETURN_NONE;
}

static PyObject *
ps_get_levels (PyObject *self, PyObject *ignored) {
  PyObject *list, *num;
  const cairo_ps_level_t *levels;
  int i, num_levels;

  Py_BEGIN_ALLOW_THREADS;
  cairo_ps_get_levels (&levels, &num_levels);
  Py_END_ALLOW_THREADS;

  list = PyList_New (num_levels);
  if (list == NULL)
    return NULL;

  for (i=0; i < num_levels; i++) {
    num = CREATE_INT_ENUM (PSLevel, levels[i]);
    if (num == NULL) {
      Py_DECREF (list);
      return NULL;
    }
    PyList_SET_ITEM (list, i, num);
  }

  return list;
}

static PyMethodDef ps_surface_methods[] = {
  {"dsc_begin_page_setup",
   (PyCFunction)ps_surface_dsc_begin_page_setup, METH_NOARGS },
  {"dsc_begin_setup", (PyCFunction)ps_surface_dsc_begin_setup, METH_NOARGS },
  {"dsc_comment", (PyCFunction)ps_surface_dsc_comment,        METH_VARARGS },
  {"get_eps", (PyCFunction)ps_surface_get_eps,                 METH_NOARGS },
  {"ps_level_to_string", (PyCFunction)ps_level_to_string,
   METH_VARARGS | METH_STATIC},
  {"level_to_string", (PyCFunction)ps_level_to_string,
   METH_VARARGS | METH_STATIC},
  {"restrict_to_level", (PyCFunction)ps_surface_restrict_to_level,
   METH_VARARGS },
  {"set_eps", (PyCFunction)ps_surface_set_eps,                METH_VARARGS },
  {"set_size", (PyCFunction)ps_surface_set_size,              METH_VARARGS },
  {"get_levels", (PyCFunction)ps_get_levels, METH_NOARGS | METH_STATIC},
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoPSSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.PSSurface",                  /* tp_name */
  sizeof(PycairoPSSurface),           /* tp_basicsize */
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
  ps_surface_methods,                 /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)ps_surface_new,            /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_PS_SURFACE */


/* Class RecordingSurface(Surface) ---------------------------------------- */
#ifdef CAIRO_HAS_RECORDING_SURFACE

static PyObject *
recording_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  cairo_content_t content;
  cairo_rectangle_t extents, *extents_ptr;
  cairo_surface_t *sfc;
  int content_arg;
  PyObject *extents_tuple;

  if (!PyArg_ParseTuple (args, "iO:RecordingSurface.__new__",
                         &content_arg, &extents_tuple))
    return NULL;

  content = (cairo_content_t)content_arg;

  if (extents_tuple == Py_None) {
    extents_ptr = NULL;
  } else {
    if (!PyArg_ParseTuple(extents_tuple, "dddd", &extents.x, &extents.y,
			  &extents.width, &extents.height)) {
      PyErr_SetString(PyExc_TypeError,
		      "RecordingSurface() argument 2 must be a "
		      "4-tuple of float");
      return NULL;
    }
    extents_ptr = &extents;
  }

  Py_BEGIN_ALLOW_THREADS;
  sfc = cairo_recording_surface_create (content, extents_ptr);
  Py_END_ALLOW_THREADS;
  return PycairoSurface_FromSurface (sfc, NULL);
}

static PyObject *
recording_surface_ink_extents (PycairoRecordingSurface *o, PyObject *ignored) {
  double x0, y0, width, height;

  cairo_recording_surface_ink_extents(o->surface, &x0, &y0, &width, &height);

  return Py_BuildValue("(dddd)", x0, y0, width, height);
}

static PyObject *
recording_surface_get_extents (PycairoRecordingSurface *o, PyObject *ignored) {
  cairo_rectangle_t extents;
  cairo_bool_t result;
  PyObject *rect, *args;

  Py_BEGIN_ALLOW_THREADS;
  result = cairo_recording_surface_get_extents (o->surface, &extents);
  Py_END_ALLOW_THREADS;

  if (!result) {
    Py_RETURN_NONE;
  }

  args = Py_BuildValue(
    "(dddd)", extents.x, extents.y, extents.width, extents.height);
  if (args == NULL)
    return NULL;
  rect = PyObject_Call((PyObject *)&PycairoRectangle_Type, args, NULL);
  Py_DECREF (args);
  return rect;
}

static PyMethodDef recording_surface_methods[] = {
  {"ink_extents", (PyCFunction)recording_surface_ink_extents, METH_NOARGS },
  {"get_extents", (PyCFunction)recording_surface_get_extents, METH_NOARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoRecordingSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.RecordingSurface",           /* tp_name */
  sizeof(PycairoRecordingSurface),    /* tp_basicsize */
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
  recording_surface_methods,          /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)recording_surface_new,     /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
#endif /* CAIRO_HAS_RECORDING_SURFACE */


/* Class SVGSurface(Surface) ----------------------------------------------- */
#ifdef CAIRO_HAS_SVG_SURFACE
#include <cairo-svg.h>

static PyObject *
svg_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  double width_in_points, height_in_points;
  PyObject *file;
  cairo_surface_t *sfc;
  char *name;

  if (!PyArg_ParseTuple (args, "Odd:SVGSurface.__new__",
                         &file, &width_in_points, &height_in_points))
    return NULL;

  if (Pycairo_is_fspath (file) || file == Py_None) {
    if (!PyArg_ParseTuple (args, "O&dd:SVGSurface.__new__",
                           Pycairo_fspath_none_converter, &name,
                           &width_in_points, &height_in_points))
      return NULL;

    Py_BEGIN_ALLOW_THREADS;
    sfc = cairo_svg_surface_create (name, width_in_points, height_in_points);
    Py_END_ALLOW_THREADS;
    PyMem_Free(name);
    return PycairoSurface_FromSurface (sfc, NULL);
  } else {
    if (PyArg_ParseTuple (args, "O&dd:SVGSurface.__new__",
                          Pycairo_writer_converter, &file,
                          &width_in_points, &height_in_points)) {
      Py_BEGIN_ALLOW_THREADS;
      sfc = cairo_svg_surface_create_for_stream (
        _write_func, file, width_in_points, height_in_points);
      Py_END_ALLOW_THREADS;
      return _surface_create_with_object (sfc, file);
    } else {
      PyErr_Clear ();
      PyErr_SetString(PyExc_TypeError,
                      "SVGSurface argument 1 must be "
                      "None, or a filename (str), or "
                      "a file object, or an object that has a "
                      "\"write\" method (like BytesIO) taking bytes.");
      return NULL;
    }
  }
}

static PyObject *
svg_get_versions (PyObject *self, PyObject *ignored) {
  PyObject *list, *num;
  const cairo_svg_version_t *versions;
  int i, num_versions;

  Py_BEGIN_ALLOW_THREADS;
  cairo_svg_get_versions (&versions, &num_versions);
  Py_END_ALLOW_THREADS;

  list = PyList_New (num_versions);
  if (list == NULL)
    return NULL;

  for (i=0; i < num_versions; i++) {
    num = CREATE_INT_ENUM (SVGVersion, versions[i]);
    if (num == NULL) {
      Py_DECREF (list);
      return NULL;
    }
    PyList_SET_ITEM (list, i, num);
  }

  return list;
}

static PyObject *
svg_version_to_string (PyObject *self,  PyObject *args) {
  cairo_svg_version_t version;
  int version_arg;
  const char *version_string;

  if (!PyArg_ParseTuple (args, "i:SVGSurface.version_to_string", &version_arg))
    return NULL;

  version = (cairo_svg_version_t)version_arg;

  Py_BEGIN_ALLOW_THREADS;
  version_string = cairo_svg_version_to_string (version);
  Py_END_ALLOW_THREADS;

  if (version_string == NULL) {
    PyErr_SetString (PyExc_ValueError, "invalid version");
    return NULL;
  }

  return PyUnicode_FromString (version_string);
}

static PyObject *
svg_surface_get_document_unit (PycairoSVGSurface *o, PyObject *ignored) {
  RETURN_INT_ENUM (SVGUnit, cairo_svg_surface_get_document_unit (o->surface));
}

static PyObject *
svg_surface_set_document_unit (PycairoSVGSurface *o, PyObject *args) {
  cairo_svg_unit_t unit;
  int unit_arg;

  if (!PyArg_ParseTuple (args, "i:SVGSurface.set_document_unit", &unit_arg))
    return NULL;

  unit = (cairo_svg_unit_t)unit_arg;

  Py_BEGIN_ALLOW_THREADS;
  cairo_svg_surface_set_document_unit (o->surface, unit);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);
  Py_RETURN_NONE;
}

static PyObject *
svg_surface_restrict_to_version (PycairoSVGSurface *o, PyObject *args) {
  cairo_svg_version_t version;
  int version_arg;

  if (!PyArg_ParseTuple (args, "i:SVGSurface.restrict_to_version",
                         &version_arg))
    return NULL;

  version = (cairo_svg_version_t)version_arg;

  Py_BEGIN_ALLOW_THREADS;
  cairo_svg_surface_restrict_to_version (o->surface, version);
  Py_END_ALLOW_THREADS;

  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (o->surface);
  Py_RETURN_NONE;
}

static PyMethodDef svg_surface_methods[] = {
  {"get_document_unit", (PyCFunction)svg_surface_get_document_unit,
   METH_NOARGS},
  {"set_document_unit", (PyCFunction)svg_surface_set_document_unit,
   METH_VARARGS},
  {"get_versions", (PyCFunction)svg_get_versions, METH_NOARGS | METH_STATIC},
  {"version_to_string", (PyCFunction)svg_version_to_string,
   METH_VARARGS | METH_STATIC},
  {"restrict_to_version", (PyCFunction)svg_surface_restrict_to_version,
   METH_VARARGS},
    {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoSVGSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.SVGSurface",                 /* tp_name */
  sizeof(PycairoSVGSurface),          /* tp_basicsize */
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
  svg_surface_methods,                /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)svg_surface_new,           /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_SVG_SURFACE */


#ifdef CAIRO_HAS_WIN32_SURFACE
#include <cairo-win32.h>

static int
convert_pyobject_to_hdc (PyObject *obj, HDC* addr) {
    HDC temp = PyLong_AsVoidPtr(obj);
    if (PyErr_Occurred() != NULL)
      return 0;
    *addr = temp;
    return 1;
}

/* Class Win32Surface(Surface) -------------------------------------------- */
static PyObject *
win32_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  HDC hdc;

  if (!PyArg_ParseTuple(args, "O&:Win32Surface.__new__",
                        convert_pyobject_to_hdc, &hdc))
    return NULL;
  return PycairoSurface_FromSurface (
		     cairo_win32_surface_create (hdc), NULL);
}

static PyMethodDef win32_surface_methods[] = {
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoWin32Surface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.Win32Surface",               /* tp_name */
  sizeof(PycairoWin32Surface),        /* tp_basicsize */
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
  win32_surface_methods,              /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)win32_surface_new,         /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};


/* Class Win32PrintingSurface(Surface) ------------------------------------ */
static PyObject *
win32_printing_surface_new (PyTypeObject *type, PyObject *args,
			    PyObject *kwds) {
  HDC hdc;

  if (!PyArg_ParseTuple(args, "O&:Win32PrintingSurface.__new__",
                        convert_pyobject_to_hdc, &hdc))
    return NULL;
  return PycairoSurface_FromSurface (
		     cairo_win32_printing_surface_create (hdc), NULL);
}

static PyMethodDef win32_printing_surface_methods[] = {
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoWin32PrintingSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.Win32PrintingSurface",       /* tp_name */
  sizeof(PycairoWin32PrintingSurface), /* tp_basicsize */
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
  win32_printing_surface_methods,     /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)win32_printing_surface_new,/* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_WIN32_SURFACE */


/* Class XCBSurface(Surface) --------------------------------------------- */
#ifdef CAIRO_HAS_XCB_SURFACE
#include <cairo-xcb.h>

static PyObject *
xcb_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyErr_SetString(PyExc_TypeError,
                  "Not supported");
  return NULL;
}

static PyObject *
xcb_surface_set_size (PycairoXCBSurface *o, PyObject *args) {
  int width, height;

  if (!PyArg_ParseTuple(args, "ii:XCBSurface.set_size", &width, &height))
    return NULL;
  cairo_xcb_surface_set_size (o->surface, width, height);
  Py_RETURN_NONE;
}

static PyMethodDef xcb_surface_methods[] = {
  {"set_size", (PyCFunction)xcb_surface_set_size,    METH_VARARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoXCBSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.XCBSurface",                 /* tp_name */
  sizeof(PycairoXCBSurface),          /* tp_basicsize */
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
  xcb_surface_methods,                /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)xcb_surface_new,           /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_XCB_SURFACE */


/* Class XlibSurface(Surface) --------------------------------------------- */
#ifdef CAIRO_HAS_XLIB_SURFACE
#include <cairo-xlib.h>

static PyObject *
xlib_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyErr_SetString(PyExc_TypeError,
		  "The XlibSurface type cannot be directly instantiated");
  return NULL;
}

static PyObject *
xlib_surface_get_depth (PycairoXlibSurface *o, PyObject *ignored) {
  return PyLong_FromLong (cairo_xlib_surface_get_depth (o->surface));
}

static PyObject *
xlib_surface_get_height (PycairoXlibSurface *o, PyObject *ignored) {
  return PyLong_FromLong (cairo_xlib_surface_get_height (o->surface));
}

static PyObject *
xlib_surface_get_width (PycairoXlibSurface *o, PyObject *ignored) {
  return PyLong_FromLong (cairo_xlib_surface_get_width (o->surface));
}

static PyMethodDef xlib_surface_methods[] = {
  {"get_depth", (PyCFunction)xlib_surface_get_depth,    METH_NOARGS },
  {"get_height",(PyCFunction)xlib_surface_get_height,   METH_NOARGS },
  {"get_width", (PyCFunction)xlib_surface_get_width,    METH_NOARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoXlibSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.XlibSurface",                /* tp_name */
  sizeof(PycairoXlibSurface),         /* tp_basicsize */
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
  xlib_surface_methods,               /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)xlib_surface_new,          /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_XLIB_SURFACE */

#ifdef CAIRO_HAS_TEE_SURFACE
#include <cairo-tee.h>

static PyObject *
tee_surface_new (PyTypeObject *type, PyObject *args, PyObject *kwds) {
  PyObject *pysurface;

  if (!PyArg_ParseTuple (args, "O!:TeeSurface.__new__",
      &PycairoSurface_Type, &pysurface))
    return NULL;

  return PycairoSurface_FromSurface (
    cairo_tee_surface_create(((PycairoSurface*)pysurface)->surface),
    NULL);
}

static PyObject *
tee_surface_add (PycairoTeeSurface *obj, PyObject *args) {
  PyObject *pysurface;

  if (!PyArg_ParseTuple(args, "O!:TeeSurface.add",
      &PycairoSurface_Type, &pysurface))
    return NULL;

  cairo_tee_surface_add (obj->surface, ((PycairoSurface*)pysurface)->surface);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (obj->surface);

  Py_RETURN_NONE;
}

static PyObject *
tee_surface_remove (PycairoTeeSurface *obj, PyObject *args) {
  PyObject *pysurface;

  if (!PyArg_ParseTuple(args, "O!:TeeSurface.remove",
      &PycairoSurface_Type, &pysurface))
    return NULL;

  cairo_tee_surface_remove (obj->surface, ((PycairoSurface*)pysurface)->surface);
  RETURN_NULL_IF_CAIRO_SURFACE_ERROR (obj->surface);

  Py_RETURN_NONE;
}

static PyObject *
tee_surface_index (PycairoTeeSurface *obj, PyObject *args) {
  unsigned int index;

  if (!PyArg_ParseTuple(args, "I:TeeSurface.index", &index))
    return NULL;

  return PycairoSurface_FromSurface (
    cairo_surface_reference (cairo_tee_surface_index (obj->surface, index)),
    NULL);
}

static PyMethodDef tee_surface_methods[] = {
  {"add",    (PyCFunction)tee_surface_add,    METH_VARARGS },
  {"remove", (PyCFunction)tee_surface_remove, METH_VARARGS },
  {"index",  (PyCFunction)tee_surface_index,  METH_VARARGS },
  {NULL, NULL, 0, NULL},
};

PyTypeObject PycairoTeeSurface_Type = {
  PyVarObject_HEAD_INIT(NULL, 0)
  "cairo.TeeSurface",                 /* tp_name */
  sizeof(PycairoTeeSurface),          /* tp_basicsize */
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
  tee_surface_methods,                /* tp_methods */
  0,                                  /* tp_members */
  0,                                  /* tp_getset */
  &PycairoSurface_Type,               /* tp_base */
  0,                                  /* tp_dict */
  0,                                  /* tp_descr_get */
  0,                                  /* tp_descr_set */
  0,                                  /* tp_dictoffset */
  0,                                  /* tp_init */
  0,                                  /* tp_alloc */
  (newfunc)tee_surface_new,           /* tp_new */
  0,                                  /* tp_free */
  0,                                  /* tp_is_gc */
  0,                                  /* tp_bases */
};
#endif  /* CAIRO_HAS_TEE_SURFACE */
