#include <glib.h>
#include <gtk/gtk.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include "remmina_plugin_python_remmina_file.h"

typedef struct {
    PyObject_HEAD
    PyObject* filename;
    PyDictObject* settings;
    PyDictObject* spsettings;
    PyObject* prevent_saving;
} PyRemminaFile;

static PyMemberDef python_remmina_file_type_members[] = {
    { "filename",       T_STRING, offsetof(PyRemminaFile, filename),        0, "File name"      },
    { "settings",       T_OBJECT, offsetof(PyRemminaFile, settings),        0, "Settings"       },
    { "spsettings",     T_OBJECT, offsetof(PyRemminaFile, spsettings),      0, "Secure Plugin Settings"    },
    { "prevent_saving", T_BOOL,   offsetof(PyRemminaFile, prevent_saving),  0, "Prevent Saving" },
    {NULL}  /* Sentinel */
};

/**
 * @brief The definition of the Python module 'remmina'.
 */
static PyTypeObject  python_remmina_file_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "remmina.RemminaFileType",
    .tp_doc = "",
    .tp_basicsize = sizeof(PyRemminaFile),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT
    .m_members = python_remmina_file_type_member
};

static void python_remmina_file_fill_dict(gpointer key, gpointer value, gpointer userdata)
{
    PyObject* dict = (PyObject*)userdata;
    PyDict_SetItem(dict, PyUnicode_FromString(gchar*)key, PyUnicode_FromString(gchar*)value);
}

PyObject* remmina_plugin_python_remmina_file_to_python(RemminaFile* file)
{
    PyRemminaFile* result;

    result = PyObject_New(python_remmina_file_type);
    PyObject_SetAttrString(result, "filename", PyUnicode_FromString(file->filename));
    PyObject_SetAttrString(result, "prevent_saving", Py_BuildValue("p", file->prevent_saving));
    PyDictObject* settings = PyDict_New();
    if (!settings) {
        g_printerr("[%s:%s] Error creating PyDictObject!\n", __FILE__, __LINE__);
        PyErr_Print();
    }
    g_hash_table_foreach(file->settings, python_remmina_file_fill_dict, settings);
    PyObject_SetAttrString(result, "settings", settings);

    PyDictObject* spsettings = PyDict_New();
    if (!spsettings) {
        g_printerr("[%s:%s] Error creating PyDictObject!\n", __FILE__, __LINE__);
        PyErr_Print();
    }
    g_hash_table_foreach(file->spsettings, python_remmina_file_fill_dict, spsettings);
    PyObject_SetAttrString(result, "spsettings", spsettings);

    return result;
}
