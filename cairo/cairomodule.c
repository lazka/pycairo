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

#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif

/* C API.  Clients get at this via Pycairo_IMPORT or import_cairo(), defined in py3cairo.h.
 */
static Pycairo_CAPI_t CAPI = {
  &PycairoContext_Type,
  PycairoContext_FromContext,

  &PycairoFontFace_Type,
  &PycairoToyFontFace_Type,
  PycairoFontFace_FromFontFace,

  &PycairoFontOptions_Type,
  PycairoFontOptions_FromFontOptions,

  &PycairoMatrix_Type,
  PycairoMatrix_FromMatrix,

  &PycairoPath_Type,
  PycairoPath_FromPath,

  &PycairoPattern_Type,
  &PycairoSolidPattern_Type,
  &PycairoSurfacePattern_Type,
  &PycairoGradient_Type,
  &PycairoLinearGradient_Type,
  &PycairoRadialGradient_Type,
  PycairoPattern_FromPattern,

  &PycairoScaledFont_Type,
  PycairoScaledFont_FromScaledFont,

  &PycairoSurface_Type,
#ifdef CAIRO_HAS_IMAGE_SURFACE
  &PycairoImageSurface_Type,
#else
  0,
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  &PycairoPDFSurface_Type,
#else
  0,
#endif
#ifdef CAIRO_HAS_PS_SURFACE
  &PycairoPSSurface_Type,
#else
  0,
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  &PycairoSVGSurface_Type,
#else
  0,
#endif
#ifdef CAIRO_HAS_WIN32_SURFACE
  &PycairoWin32Surface_Type,
  &PycairoWin32PrintingSurface_Type,
#else
  0,
  0,
#endif
#ifdef CAIRO_HAS_XCB_SURFACE
  &PycairoXCBSurface_Type,
#else
  0,
#endif
#ifdef CAIRO_HAS_XLIB_SURFACE
  &PycairoXlibSurface_Type,
#else
  0,
#endif
  PycairoSurface_FromSurface,

  Pycairo_Check_Status,

  &PycairoRectangleInt_Type,
  PycairoRectangleInt_FromRectangleInt,

  &PycairoRegion_Type,
  PycairoRegion_FromRegion,
#ifdef CAIRO_HAS_RECORDING_SURFACE
  &PycairoRecordingSurface_Type,
#else
  0,
#endif
};

static PyObject *
pycairo_cairo_version (PyObject *self, PyObject *ignored) {
  return PyLong_FromLong (cairo_version());
}

static PyObject *
pycairo_cairo_version_string (PyObject *self, PyObject *ignored) {
  return PyUnicode_FromString (cairo_version_string());
}

static PyMethodDef cairo_functions[] = {
  {"cairo_version",    (PyCFunction)pycairo_cairo_version, METH_NOARGS},
  {"cairo_version_string", (PyCFunction)pycairo_cairo_version_string,
   METH_NOARGS},
  {NULL, NULL, 0, NULL},
};

static int exec_cairo(PyObject *m)
{
  PyObject *capi;

  if (PyType_Ready(&PycairoContext_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoFontFace_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoToyFontFace_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoFontOptions_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoMatrix_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoPath_Type) < 0)
    return -1;
  PycairoPathiter_Type.tp_iter=&PyObject_SelfIter;
  if (PyType_Ready(&PycairoPathiter_Type) < 0)
    return -1;

  if (PyType_Ready(&PycairoPattern_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoSolidPattern_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoSurfacePattern_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoGradient_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoLinearGradient_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoRadialGradient_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoMeshPattern_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoRasterSourcePattern_Type) < 0)
    return -1;

  if (PyType_Ready(&PycairoRectangleInt_Type) < 0)
    return -1;

  if (PyType_Ready(&PycairoDevice_Type) < 0)
    return -1;

  PycairoGlyph_Type.tp_base = &PyTuple_Type;
  if (PyType_Ready(&PycairoGlyph_Type) < 0)
    return -1;

  PycairoRectangle_Type.tp_base = &PyTuple_Type;
  if (PyType_Ready(&PycairoRectangle_Type) < 0)
    return -1;

  PycairoTextCluster_Type.tp_base = &PyTuple_Type;
  if (PyType_Ready(&PycairoTextCluster_Type) < 0)
    return -1;

  PycairoTextExtents_Type.tp_base = &PyTuple_Type;
  if (PyType_Ready(&PycairoTextExtents_Type) < 0)
    return -1;

  if (PyType_Ready(&PycairoSurface_Type) < 0)
    return -1;

#ifdef CAIRO_HAS_SCRIPT_SURFACE
  if (PyType_Ready(&PycairoScriptDevice_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoScriptSurface_Type) < 0)
    return -1;
#endif

  if (PyType_Ready(&PycairoRegion_Type) < 0)
    return -1;

  if (PyType_Ready(&PycairoScaledFont_Type) < 0)
    return -1;

#ifdef CAIRO_HAS_IMAGE_SURFACE
  if (PyType_Ready(&PycairoImageSurface_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoMappedImageSurface_Type) < 0)
    return -1;
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  if (PyType_Ready(&PycairoPDFSurface_Type) < 0)
    return -1;
#endif
#ifdef CAIRO_HAS_PS_SURFACE
  if (PyType_Ready(&PycairoPSSurface_Type) < 0)
    return -1;
#endif
#ifdef CAIRO_HAS_RECORDING_SURFACE
  if (PyType_Ready(&PycairoRecordingSurface_Type) < 0)
    return -1;
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  if (PyType_Ready(&PycairoSVGSurface_Type) < 0)
    return -1;
#endif
#ifdef CAIRO_HAS_WIN32_SURFACE
  if (PyType_Ready(&PycairoWin32Surface_Type) < 0)
    return -1;
  if (PyType_Ready(&PycairoWin32PrintingSurface_Type) < 0)
    return -1;
#endif
#ifdef CAIRO_HAS_XCB_SURFACE
  if (PyType_Ready(&PycairoXCBSurface_Type) < 0)
    return -1;
#endif
#ifdef CAIRO_HAS_XLIB_SURFACE
  if (PyType_Ready(&PycairoXlibSurface_Type) < 0)
    return -1;
#endif
#ifdef CAIRO_HAS_TEE_SURFACE
  if (PyType_Ready(&PycairoTeeSurface_Type) < 0)
    return -1;
#endif

  if(init_error(m) < 0)
    return -1;

  if(init_buffer_proxy() < 0)
    return -1;

  if(init_enums(m) < 0)
    return -1;

  PyModule_AddStringConstant(m, "version",
    PYCAIRO_STRINGIFY(PYCAIRO_VERSION_MAJOR) "."
    PYCAIRO_STRINGIFY(PYCAIRO_VERSION_MINOR) "."
    PYCAIRO_STRINGIFY(PYCAIRO_VERSION_MICRO));
  PyModule_AddObject(m, "version_info",
		     Py_BuildValue("(iii)",
				   PYCAIRO_VERSION_MAJOR,
				   PYCAIRO_VERSION_MINOR,
				   PYCAIRO_VERSION_MICRO
				   ));

  Py_INCREF(&PycairoContext_Type);
  PyModule_AddObject(m, "Context", (PyObject *)&PycairoContext_Type);
  Py_INCREF(&PycairoFontFace_Type);
  PyModule_AddObject(m, "FontFace",(PyObject *)&PycairoFontFace_Type);
  Py_INCREF(&PycairoToyFontFace_Type);
  PyModule_AddObject(m, "ToyFontFace",(PyObject *)&PycairoToyFontFace_Type);
  Py_INCREF(&PycairoFontOptions_Type);
  PyModule_AddObject(m, "FontOptions",(PyObject *)&PycairoFontOptions_Type);
  Py_INCREF(&PycairoMatrix_Type);
  PyModule_AddObject(m, "Matrix",  (PyObject *)&PycairoMatrix_Type);
  Py_INCREF(&PycairoPath_Type);
  /* Don't add Path object since it is not accessed directly as 'cairo.Path'
   * PyModule_AddObject(m, "Path", (PyObject *)&PycairoPath_Type);
   */
  Py_INCREF(&PycairoPattern_Type);
  PyModule_AddObject(m, "Pattern", (PyObject *)&PycairoPattern_Type);
  Py_INCREF(&PycairoSolidPattern_Type);
  PyModule_AddObject(m, "SolidPattern",
		     (PyObject *)&PycairoSolidPattern_Type);
  Py_INCREF(&PycairoSurfacePattern_Type);
  PyModule_AddObject(m, "SurfacePattern",
		     (PyObject *)&PycairoSurfacePattern_Type);
  Py_INCREF(&PycairoGradient_Type);
  PyModule_AddObject(m, "Gradient", (PyObject *)&PycairoGradient_Type);
  Py_INCREF(&PycairoLinearGradient_Type);
  PyModule_AddObject(m, "LinearGradient",
		     (PyObject *)&PycairoLinearGradient_Type);
  Py_INCREF(&PycairoRadialGradient_Type);
  PyModule_AddObject(m, "RadialGradient",
		     (PyObject *)&PycairoRadialGradient_Type);
  Py_INCREF(&PycairoRadialGradient_Type);
  PyModule_AddObject(m, "MeshPattern",
                     (PyObject *)&PycairoMeshPattern_Type);
  Py_INCREF(&PycairoRasterSourcePattern_Type);
  PyModule_AddObject(m, "RasterSourcePattern",
                     (PyObject *)&PycairoRasterSourcePattern_Type);

  Py_INCREF(&PycairoRectangleInt_Type);
  PyModule_AddObject(m, "RectangleInt",  (PyObject *)&PycairoRectangleInt_Type);

  Py_INCREF(&PycairoRegion_Type);
  PyModule_AddObject(m, "Region",  (PyObject *)&PycairoRegion_Type);

  Py_INCREF(&PycairoScaledFont_Type);
  PyModule_AddObject(m, "ScaledFont", (PyObject *)&PycairoScaledFont_Type);

  Py_INCREF(&PycairoSurface_Type);
  PyModule_AddObject(m, "Surface", (PyObject *)&PycairoSurface_Type);

  Py_INCREF(&PycairoDevice_Type);
  PyModule_AddObject(m, "Device", (PyObject *)&PycairoDevice_Type);

  Py_INCREF(&PycairoGlyph_Type);
  PyModule_AddObject(m, "Glyph", (PyObject *)&PycairoGlyph_Type);

  Py_INCREF(&PycairoRectangle_Type);
  PyModule_AddObject(m, "Rectangle", (PyObject *)&PycairoRectangle_Type);

  Py_INCREF(&PycairoTextCluster_Type);
  PyModule_AddObject(m, "TextCluster", (PyObject *)&PycairoTextCluster_Type);

  Py_INCREF(&PycairoTextExtents_Type);
  PyModule_AddObject(m, "TextExtents", (PyObject *)&PycairoTextExtents_Type);

  Py_INCREF(&PycairoPath_Type);
  PyModule_AddObject(m, "Path", (PyObject *)&PycairoPath_Type);

#ifdef CAIRO_HAS_SCRIPT_SURFACE
  Py_INCREF(&PycairoScriptDevice_Type);
  PyModule_AddObject(m, "ScriptDevice", (PyObject *)&PycairoScriptDevice_Type);
  Py_INCREF(&PycairoScriptSurface_Type);
  PyModule_AddObject(m, "ScriptSurface", (PyObject *)&PycairoScriptSurface_Type);
#endif

#ifdef CAIRO_HAS_IMAGE_SURFACE
  Py_INCREF(&PycairoImageSurface_Type);
  PyModule_AddObject(m, "ImageSurface",
		     (PyObject *)&PycairoImageSurface_Type);
#endif

#ifdef CAIRO_HAS_PDF_SURFACE
  Py_INCREF(&PycairoPDFSurface_Type);
  PyModule_AddObject(m, "PDFSurface", (PyObject *)&PycairoPDFSurface_Type);
  PyModule_AddIntConstant(m, "PDF_OUTLINE_ROOT", CAIRO_PDF_OUTLINE_ROOT);
#endif

#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 17, 8)
  PyModule_AddIntConstant(m, "COLOR_PALETTE_DEFAULT", CAIRO_COLOR_PALETTE_DEFAULT);
#endif

#ifdef CAIRO_HAS_PS_SURFACE
  Py_INCREF(&PycairoPSSurface_Type);
  PyModule_AddObject(m, "PSSurface", (PyObject *)&PycairoPSSurface_Type);
#endif

#ifdef CAIRO_HAS_RECORDING_SURFACE
  Py_INCREF(&PycairoRecordingSurface_Type);
  PyModule_AddObject(m, "RecordingSurface",
		     (PyObject *)&PycairoRecordingSurface_Type);
#endif

#ifdef CAIRO_HAS_SVG_SURFACE
  Py_INCREF(&PycairoSVGSurface_Type);
  PyModule_AddObject(m, "SVGSurface", (PyObject *)&PycairoSVGSurface_Type);
#endif

#ifdef CAIRO_HAS_WIN32_SURFACE
  Py_INCREF(&PycairoWin32Surface_Type);
  PyModule_AddObject(m, "Win32Surface",
		     (PyObject *)&PycairoWin32Surface_Type);
  Py_INCREF(&PycairoWin32PrintingSurface_Type);
  PyModule_AddObject(m, "Win32PrintingSurface",
		     (PyObject *)&PycairoWin32PrintingSurface_Type);
#endif

#ifdef CAIRO_HAS_XCB_SURFACE
  Py_INCREF(&PycairoXCBSurface_Type);
  PyModule_AddObject(m, "XCBSurface",
		     (PyObject *)&PycairoXCBSurface_Type);
#endif

#ifdef CAIRO_HAS_XLIB_SURFACE
  Py_INCREF(&PycairoXlibSurface_Type);
  PyModule_AddObject(m, "XlibSurface",
		     (PyObject *)&PycairoXlibSurface_Type);
#endif

#ifdef CAIRO_HAS_TEE_SURFACE
  Py_INCREF(&PycairoTeeSurface_Type);
  PyModule_AddObject(m, "TeeSurface",
		     (PyObject *)&PycairoTeeSurface_Type);
#endif

    /* constants */
#ifdef CAIRO_HAS_ATSUI_FONT
  PyModule_AddIntConstant(m, "HAS_ATSUI_FONT", 1);
#else
  PyModule_AddIntConstant(m, "HAS_ATSUI_FONT", 0);
#endif
#ifdef CAIRO_HAS_FT_FONT
  PyModule_AddIntConstant(m, "HAS_FT_FONT", 1);
#else
  PyModule_AddIntConstant(m, "HAS_FT_FONT", 0);
#endif
#ifdef CAIRO_HAS_GLITZ_SURFACE
  PyModule_AddIntConstant(m, "HAS_GLITZ_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_GLITZ_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_IMAGE_SURFACE
  PyModule_AddIntConstant(m, "HAS_IMAGE_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_IMAGE_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
  PyModule_AddIntConstant(m, "HAS_PDF_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_PDF_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_PNG_FUNCTIONS
  PyModule_AddIntConstant(m, "HAS_PNG_FUNCTIONS", 1);
#else
  PyModule_AddIntConstant(m, "HAS_PNG_FUNCTIONS", 0);
#endif
#ifdef CAIRO_HAS_PS_SURFACE
  PyModule_AddIntConstant(m, "HAS_PS_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_PS_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_RECORDING_SURFACE
  PyModule_AddIntConstant(m, "HAS_RECORDING_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_RECORDING_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_SVG_SURFACE
  PyModule_AddIntConstant(m, "HAS_SVG_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_SVG_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_USER_FONT
  PyModule_AddIntConstant(m, "HAS_USER_FONT", 1);
#else
  PyModule_AddIntConstant(m, "HAS_USER_FONT", 0);
#endif
#ifdef CAIRO_HAS_QUARTZ_SURFACE
  PyModule_AddIntConstant(m, "HAS_QUARTZ_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_QUARTZ_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_WIN32_FONT
  PyModule_AddIntConstant(m, "HAS_WIN32_FONT", 1);
#else
  PyModule_AddIntConstant(m, "HAS_WIN32_FONT", 0);
#endif
#ifdef CAIRO_HAS_WIN32_SURFACE
  PyModule_AddIntConstant(m, "HAS_WIN32_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_WIN32_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_XCB_SURFACE
  PyModule_AddIntConstant(m, "HAS_XCB_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_XCB_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_XLIB_SURFACE
  PyModule_AddIntConstant(m, "HAS_XLIB_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_XLIB_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_MIME_SURFACE
  PyModule_AddIntConstant(m, "HAS_MIME_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_MIME_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_SCRIPT_SURFACE
  PyModule_AddIntConstant(m, "HAS_SCRIPT_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_SCRIPT_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_TEE_SURFACE
  PyModule_AddIntConstant(m, "HAS_TEE_SURFACE", 1);
#else
  PyModule_AddIntConstant(m, "HAS_TEE_SURFACE", 0);
#endif
#ifdef CAIRO_HAS_DWRITE_FONT
  PyModule_AddIntConstant(m, "HAS_DWRITE_FONT", 1);
#else
  PyModule_AddIntConstant(m, "HAS_DWRITE_FONT", 0);
#endif

  PyModule_AddIntConstant(m, "CAIRO_VERSION", CAIRO_VERSION);
  PyModule_AddIntConstant(m, "CAIRO_VERSION_MAJOR", CAIRO_VERSION_MAJOR);
  PyModule_AddIntConstant(m, "CAIRO_VERSION_MICRO", CAIRO_VERSION_MICRO);
  PyModule_AddIntConstant(m, "CAIRO_VERSION_MINOR", CAIRO_VERSION_MINOR);
  PyModule_AddStringConstant(m, "CAIRO_VERSION_STRING", CAIRO_VERSION_STRING);

#define STRCONSTANT(x) PyModule_AddStringConstant(m, #x, CAIRO_##x)

  STRCONSTANT(MIME_TYPE_JP2);
  STRCONSTANT(MIME_TYPE_JPEG);
  STRCONSTANT(MIME_TYPE_PNG);
  STRCONSTANT(MIME_TYPE_URI);
  STRCONSTANT(MIME_TYPE_UNIQUE_ID);
  STRCONSTANT(MIME_TYPE_JBIG2);
  STRCONSTANT(MIME_TYPE_JBIG2_GLOBAL);
  STRCONSTANT(MIME_TYPE_JBIG2_GLOBAL_ID);
  STRCONSTANT(MIME_TYPE_CCITT_FAX);
  STRCONSTANT(MIME_TYPE_CCITT_FAX_PARAMS);
  STRCONSTANT(MIME_TYPE_EPS);
  STRCONSTANT(MIME_TYPE_EPS_PARAMS);
  STRCONSTANT(TAG_DEST);
  STRCONSTANT(TAG_LINK);
#if CAIRO_VERSION >= CAIRO_VERSION_ENCODE(1, 18, 0)
  STRCONSTANT(TAG_CONTENT);
  STRCONSTANT(TAG_CONTENT_REF);
#endif

#undef STRCONSTANT

  /* Create a Capsule containing the CAPI pointer */
  capi = PyCapsule_New((void *)(&CAPI), "cairo.CAPI", 0);

  if (capi != NULL) {
    PyModule_AddObject(m, "CAPI", capi);
  } else {
    return -1;
  }

  return 0;
}

static PyModuleDef_Slot cairo_slots[] = {
  {Py_mod_exec, (void*)exec_cairo},
  {0, NULL},
};

static struct PyModuleDef cairo_module = {
  PyModuleDef_HEAD_INIT,
  "cairo",
  NULL,
  0,
  cairo_functions,
  cairo_slots,
  NULL,
  NULL,
  NULL
};

PYCAIRO_MODINIT_FUNC PyInit__cairo(void)
{
  return PyModuleDef_Init(&cairo_module);
}
