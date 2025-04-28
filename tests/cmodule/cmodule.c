#include <Python.h>
#include <py3cairo.h>
#include "cmodulelib.h"

static PyMethodDef CModMethods[] = {
    {"create_image_surface", create_image_surface, METH_NOARGS, NULL},
    {"create_context", create_context, METH_NOARGS, NULL},
    {"check_status", check_status, METH_VARARGS, NULL},
    {"test_capi", test_capi, METH_NOARGS, NULL},
    {NULL, NULL, 0, NULL}
};

#ifdef __GNUC__
#define PYCAIRO_MODINIT_FUNC __attribute__((visibility("default"))) PyMODINIT_FUNC
#else
#define PYCAIRO_MODINIT_FUNC PyMODINIT_FUNC
#endif

static struct PyModuleDef cmod_module = {
    PyModuleDef_HEAD_INIT,
    "cmod",
    NULL,
    -1,
    CModMethods,
};

PYCAIRO_MODINIT_FUNC
PyInit_cmod (void)
{
    PyObject *m;

    if (import_cairo () < 0)
        return NULL;

    m = PyModule_Create (&cmod_module);
    if (m == NULL)
        return NULL;

    return m;
}
