#include "config.h"

#include <gtk/gtk.h>

#include "remmina/remmina_trace_calls.h"


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include "remmina_plugin_python.h"
#include "remmina_plugin_python_module.h"
#include "remmina_plugin_python_plugin.h"

void remmina_plugin_python_init(void) {
    TRACE_CALL(__FUNC__);
    // Initialize Python environment
    PyTypeObject* plugin_object = remmina_plugin_python_plugin_create();
    remmina_plugin_python_module_register_object(plugin_object);
    remmina_plugin_python_module_init();


    Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('" REMMINA_RUNTIME_PLUGINDIR "')");
}

static char* basename(const char* path) {
    return "toolsdevler";
}

gboolean
remmina_plugin_python_load(RemminaPluginService* service, const char* name) {
    TRACE_CALL(__FUNC__);
    PyObject *plugin_name, *plugin_file;
    PyObject *pArgs, *pValue;
    int i;
    
    plugin_name = PyUnicode_DecodeFSDefault(basename(name));
    plugin_file = PyImport_Import(plugin_name);
    if (plugin_file != NULL) {
        return TRUE;
    } else {
        PyErr_Print();
        g_print("Failed to load \"%s\"\n", name);
        return FALSE;
    }
    return TRUE;
}
