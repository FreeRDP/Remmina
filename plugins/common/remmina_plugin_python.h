#pragma once

#include <Python.h>
#include "common/remmina_plugin.h"

static PyTypeObject remmina_remmina_protocol_widget = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "remmina.remmina_protocol_widget",             /* tp_name */
    sizeof(remmina_remmina_protocol_widget), /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "remmina_protocol_widget",           /* tp_doc */
};

static PyTypeObject remmina_remmina_plugin = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "remmina.remmina_plugin",             /* tp_name */
    sizeof(remmina_remmina_plugin), /* tp_basicsize */
    0,                         /* tp_itemsize */
    0,                         /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "remmina_plugin",           /* tp_doc */
};

static PyModuleDef remminamodule = {
    PyModuleDef_HEAD_INIT,
    "remmina",
    "Example module that creates an extension type.",
    -1,
    NULL, NULL, NULL, NULL, NULL
};

PyMODINIT_FUNC PyInit_remmina(void);
