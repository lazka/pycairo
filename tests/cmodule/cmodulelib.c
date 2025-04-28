#include <Python.h>
#define PYCAIRO_NO_IMPORT
#include <py3cairo.h>
#include "cmodulelib.h"

PyObject *
create_image_surface (PyObject *self, PyObject *args)
{
    cairo_surface_t *surface;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);

    return PycairoSurface_FromSurface (surface, NULL);
}

PyObject *
create_context (PyObject *self, PyObject *args)
{
    cairo_surface_t *surface;
    cairo_t *ctx;

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
    ctx = cairo_create(surface);
    cairo_surface_destroy (surface);

    return PycairoContext_FromContext(ctx, &PycairoContext_Type, NULL);
}

PyObject *
check_status (PyObject *self, PyObject *args) {
    int status_arg;

    if (!PyArg_ParseTuple(args, "i", &status_arg))
        return NULL;

    if (Pycairo_Check_Status((cairo_status_t)status_arg))
        return NULL;

    Py_RETURN_NONE;
}

PyObject *
test_capi (PyObject *self, PyObject *args)
{
    cairo_surface_t *surface;
    cairo_t *ctx;
    cairo_font_face_t *font_face;

    // PycairoContext_Type
    if(PyType_Ready(&PycairoContext_Type) != 0) {
        return NULL;
    }

    // PycairoContext_FromContext
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
    PyObject *context = PycairoContext_FromContext(cairo_create(surface), &PycairoContext_Type, NULL);
    cairo_surface_destroy (surface);
    if (context == NULL) {
        return NULL;
    }
    Py_DECREF(context);

    // PycairoFontFace_Type
    if(PyType_Ready(&PycairoFontFace_Type) != 0) {
        return NULL;
    }

    // PycairoToyFontFace_Type
    if(PyType_Ready(&PycairoToyFontFace_Type) != 0) {
        return NULL;
    }

    // PycairoFontFace_FromFontFace
    font_face = cairo_toy_font_face_create("", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    PyObject *font_face_obj = PycairoFontFace_FromFontFace(font_face);
    if (font_face_obj == NULL) {
        return NULL;
    }
    Py_DECREF(font_face_obj);

    // PycairoFontOptions_Type
    if(PyType_Ready(&PycairoFontOptions_Type) != 0) {
        return NULL;
    }

    // PycairoFontOptions_FromFontOptions
    cairo_font_options_t *font_options = cairo_font_options_create();
    PyObject *font_options_obj = PycairoFontOptions_FromFontOptions(font_options);
    if (font_options_obj == NULL) {
        return NULL;
    }
    Py_DECREF(font_options_obj);

    // PycairoMatrix_Type
    if(PyType_Ready(&PycairoMatrix_Type) != 0) {
        return NULL;
    }

    // PycairoMatrix_FromMatrix
    cairo_matrix_t matrix;
    cairo_matrix_init_identity(&matrix);
    PyObject *matrix_obj = PycairoMatrix_FromMatrix(&matrix);
    if (matrix_obj == NULL) {
        return NULL;
    }
    Py_DECREF(matrix_obj);

    // PycairoPath_Type
    if(PyType_Ready(&PycairoPath_Type) != 0) {
        return NULL;
    }

    // PycairoPath_FromPath
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
    ctx = cairo_create(surface);
    cairo_path_t *path = cairo_copy_path(ctx);
    PyObject *path_obj = PycairoPath_FromPath(path);
    if (path_obj == NULL) {
        return NULL;
    }
    Py_DECREF(path_obj);
    cairo_destroy(ctx);
    cairo_surface_destroy(surface);

    // PycairoPattern_Type
    if(PyType_Ready(&PycairoPattern_Type) != 0) {
        return NULL;
    }

    // PycairoSolidPattern_Type
    if(PyType_Ready(&PycairoSolidPattern_Type) != 0) {
        return NULL;
    }

    // PycairoSurfacePattern_Type
    if(PyType_Ready(&PycairoSurfacePattern_Type) != 0) {
        return NULL;
    }

    // PycairoGradient_Type
    if(PyType_Ready(&PycairoGradient_Type) != 0) {
        return NULL;
    }

    // PycairoLinearGradient_Type
    if(PyType_Ready(&PycairoLinearGradient_Type) != 0) {
        return NULL;
    }

    // PycairoRadialGradient_Type
    if(PyType_Ready(&PycairoRadialGradient_Type) != 0) {
        return NULL;
    }

    // PycairoPattern_FromPattern
    cairo_pattern_t *pattern = cairo_pattern_create_rgb(0.5, 0.5, 0.5);
    PyObject *pattern_obj = PycairoPattern_FromPattern(pattern, NULL);
    if (pattern_obj == NULL) {
        return NULL;
    }
    Py_DECREF(pattern_obj);

    // PycairoScaledFont_Type
    if(PyType_Ready(&PycairoScaledFont_Type) != 0) {
        return NULL;
    }

    // PycairoScaledFont_FromScaledFont
    font_face = cairo_toy_font_face_create("", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
    cairo_matrix_t font_matrix, ctm;
    cairo_matrix_init_scale(&font_matrix, 12.0, 12.0);  // 12pt font
    cairo_matrix_init_identity(&ctm);
    cairo_font_options_t *options = cairo_font_options_create();
    cairo_scaled_font_t *scaled_font = cairo_scaled_font_create(
        font_face, &font_matrix,
        &ctm, options);
    cairo_font_options_destroy(options);
    cairo_font_face_destroy(font_face);
    PyObject *scaled_font_obj = PycairoScaledFont_FromScaledFont(scaled_font);
    if (scaled_font_obj == NULL) {
        return NULL;
    }
    Py_DECREF(scaled_font_obj);

    // PycairoSurface_Type
    if(PyType_Ready(&PycairoSurface_Type) != 0) {
        return NULL;
    }

    // PycairoImageSurface_Type
    #ifdef CAIRO_HAS_IMAGE_SURFACE
    if(PyType_Ready(&PycairoImageSurface_Type) != 0) {
        return NULL;
    }
    #endif

    // PycairoPDFSurface_Type
    #ifdef CAIRO_HAS_PDF_SURFACE
    if(PyType_Ready(&PycairoPDFSurface_Type) != 0) {
        return NULL;
    }
    #endif

    // PycairoPSSurface_Type
    #ifdef CAIRO_HAS_PS_SURFACE
    if(PyType_Ready(&PycairoPSSurface_Type) != 0) {
        return NULL;
    }
    #endif

    // PycairoSVGSurface_Type
    #ifdef CAIRO_HAS_SVG_SURFACE
    if(PyType_Ready(&PycairoSVGSurface_Type) != 0) {
        return NULL;
    }
    #endif

    // PycairoWin32Surface_Type / PycairoWin32PrintingSurface_Type
    #ifdef CAIRO_HAS_WIN32_SURFACE
    if(PyType_Ready(&PycairoWin32Surface_Type) != 0) {
        return NULL;
    }
    if(PyType_Ready(&PycairoWin32PrintingSurface_Type) != 0) {
        return NULL;
    }
    #endif

    // PycairoXCBSurface_Type
    #ifdef CAIRO_HAS_XCB_SURFACE
    if(PyType_Ready(&PycairoXCBSurface_Type) != 0) {
        return NULL;
    }
    #endif

    // PycairoXlibSurface_Type
    #ifdef CAIRO_HAS_XLIB_SURFACE
    if(PyType_Ready(&PycairoXlibSurface_Type) != 0) {
        return NULL;
    }
    #endif

    // PycairoSurface_FromSurface
    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 10, 10);
    PyObject *surface_obj = PycairoSurface_FromSurface(surface, NULL);
    if (surface_obj == NULL) {
        return NULL;
    }
    Py_DECREF(surface_obj);

    // Pycairo_Check_Status
    if (Pycairo_Check_Status(CAIRO_STATUS_SUCCESS) != 0) {
        return NULL;
    }
    Pycairo_Check_Status(CAIRO_STATUS_NO_MEMORY);
    PyObject *error = PyErr_Occurred();
    if (error == NULL) {
        PyErr_SetString(PyExc_RuntimeError, "Expected error not raised");
        return NULL;
    } else {
        PyErr_Clear();
    }

    // PycairoRectangleInt_Type
    if(PyType_Ready(&PycairoRectangleInt_Type) != 0) {
        return NULL;
    }

    // PycairoRectangleInt_FromRectangleInt
    cairo_rectangle_int_t rect = {0, 0, 10, 10};
    PyObject *rect_obj = PycairoRectangleInt_FromRectangleInt(&rect);
    if (rect_obj == NULL) {
        return NULL;
    }
    Py_DECREF(rect_obj);

    // PycairoRegion_Type
    if(PyType_Ready(&PycairoRegion_Type) != 0) {
        return NULL;
    }

    // PycairoRegion_FromRegion
    cairo_region_t *region = cairo_region_create();
    PyObject *region_obj = PycairoRegion_FromRegion(region);
    if (region_obj == NULL) {
        return NULL;
    }
    Py_DECREF(region_obj);

    // PycairoRecordingSurface_Type
    #ifdef CAIRO_HAS_RECORDING_SURFACE
    if(PyType_Ready(&PycairoRecordingSurface_Type) != 0) {
        return NULL;
    }
    #endif

    Py_RETURN_NONE;
}
