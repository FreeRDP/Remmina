#include <glib.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include "remmina_plugin_python_module.h"

/**
 * A list of objects initialized and provided by the Remmina Python module.
 */
static GPtrArray* remmina_python_module_object_table = NULL;
static GPtrArray* remmina_python_module_member_table = NULL;


static PyModuleDef remmina_python_module_type = {
    PyModuleDef_HEAD_INIT,
    "remmina",
    "Remmina API.",
    -1,
    NULL
};

/**
 * 
 */
PyMODINIT_FUNC remmina_plugin_python_module_initialize(void);

void remmina_plugin_python_module_init(void) {
    
    if (PyImport_AppendInittab("remmina", remmina_plugin_python_module_initialize))
        return FALSE;

}

void remmina_plugin_python_module_register_member(PyMemberDef* member) {
    if (!remmina_python_module_member_table)
        remmina_python_module_member_table = g_ptr_array_new();
    g_ptr_array_add(remmina_python_module_member_table, member);
}

void remmina_plugin_python_module_register_object(PyTypeObject* object) {
    if (!remmina_python_module_object_table)
        remmina_python_module_object_table = g_ptr_array_new();
    g_ptr_array_add(remmina_python_module_object_table, object);
}

PyMODINIT_FUNC remmina_plugin_python_module_initialize(void) {
    PyObject* module_instance = NULL;
    PyTypeObject* builtin;
    PyMethodDef* method;
	gint i;

    if (remmina_python_module_member_table) {
        remmina_python_module_type.m_methods = (PyMethodDef*)malloc((sizeof(PyMethodDef) * remmina_python_module_member_table->len) + 1);
        for (i = 0; i < remmina_python_module_member_table->len; i++) {
            method = (PyMethodDef*)g_ptr_array_index(remmina_python_module_member_table, i);
            remmina_python_module_type.m_methods[i] = *method;
        }
    }

    if (remmina_python_module_object_table) {
        for (i = 0; i < remmina_python_module_object_table->len; i++) {
            builtin = (PyTypeObject*)g_ptr_array_index(remmina_python_module_object_table, i);
            if (builtin) {
                PyType_Ready(builtin);
            }
        }
    }

    module_instance = PyModule_Create(&remmina_python_module_type);

    if (remmina_python_module_object_table) {
        for (i = 0; i < remmina_python_module_object_table->len; i++) {
            builtin = (PyTypeObject*)g_ptr_array_index(remmina_python_module_object_table, i);
            if (builtin) {
                Py_INCREF(builtin);
                if (PyModule_AddObject(module_instance, "ProtocolPlugin", (PyObject*)builtin)) {
                    printf("%s: Error initializing\n", builtin->tp_name);
                    Py_INCREF(builtin);
                }
            }
        }
    }

    // TODO: Initialize Remmina module for Python
    return module_instance;
}
