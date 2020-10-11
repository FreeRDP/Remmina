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
    remmina_plugin_python_module_register_object(remmina_plugin_python_plugin_create());
    remmina_plugin_python_module_init();

    Py_Initialize();
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('" REMMINA_RUNTIME_PLUGINDIR "')");
}

int basename_no_ext(const char* in, char** out) {
    // Find last segment of path...
    const char* end = in + strlen(in);
    const char* from = in;
    const char* to = end;
    for (const char* ptr = in; ptr != end; ++ptr) {
        if (*ptr == '/') {
            while (*ptr == '/' && ptr != end)
                ++ptr;
            if (ptr == end)
                return NULL;
            from = ptr;
        } else if (*ptr == '.') {
            to = ptr;
        }
    }

    int length = to - from;
    *out = (char*)malloc(sizeof(char*) * ((length) + 1));
    strncpy(*out, from, to-from);
    (*out)[length] = '\0';
    return length;
}

gboolean
remmina_plugin_python_load(RemminaPluginService* service, const char* name) {
    TRACE_CALL(__FUNC__);
    PyObject *plugin_name, *plugin_file;
    PyObject *pArgs, *pValue;
    int i;
    char* filename = NULL;
    basename_no_ext(name, &filename);
    plugin_name = PyUnicode_DecodeFSDefault(filename);
    plugin_file = PyImport_Import(plugin_name);
    if (plugin_file != NULL) {
        return TRUE;
    } else {
        g_print("Failed to load \"%s\"\n", name);
        PyErr_Print();
        return FALSE;
    }
    return TRUE;
}
