#include "config.h"

#include <gtk/gtk.h>

#include "remmina_plugin_python.h"
#include "remmina/remmina_trace_calls.h"


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include "remmina_plugin_python_module.h"

gboolean
remmina_plugin_python_load(RemminaPluginService* service, const char* name) {
    TRACE_CALL(__FUNC__);
    
    // Initialize Python environment
    remmina_plugin_python_module_init();

    Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(" REMMINA_RUNTIME_PLUGINDIR ")");

    return FALSE;
}
