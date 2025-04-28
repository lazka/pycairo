#ifndef _PYCAIRO_CMODULELIB_H_
#define _PYCAIRO_CMODULELIB_H_

PyObject *create_image_surface (PyObject *self, PyObject *args);

PyObject *create_context (PyObject *self, PyObject *args);

PyObject *check_status (PyObject *self, PyObject *args);

PyObject *test_capi (PyObject *self, PyObject *args);

#endif
