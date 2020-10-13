#include "config.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "pygobject.h"
#include "structmember.h"

#include <glib.h>
#include <gtk/gtk.h>
#include "remmina_plugin_manager.h"
#include "remmina/plugin.h"
#include "remmina_protocol_widget.h"

GPtrArray* remmina_plugin_registry = NULL;

typedef struct {
    PyObject* pythonInstance;
    RemminaProtocolPlugin* protocolPlugin;
} PyPlugin;

/**
 * A list of objects initialized and provided by the Remmina Python module.
 */
static GPtrArray* remmina_python_module_object_table = NULL;
static GPtrArray* remmina_python_module_member_table = NULL;

static void remmina_protocol_plugin_wrapper_init(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_plugin_wrapper_open_connection(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_plugin_wrapper_close_connection(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_plugin_wrapper_query_feature(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
static void remmina_protocol_plugin_wrapper_call_feature(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
static void remmina_protocol_plugin_wrapper_send_keystrokes(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen);
static gboolean remmina_protocol_plugin_wrapper_get_plugin_screenshot(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd);


static void remmina_protocol_plugin_wrapper_init(RemminaProtocolWidget *gp) {
    
}

static gboolean remmina_protocol_plugin_wrapper_open_connection(RemminaProtocolWidget *gp) {
    return FALSE;
}

static gboolean remmina_protocol_plugin_wrapper_close_connection(RemminaProtocolWidget *gp) {
    return FALSE;
}

static gboolean remmina_protocol_plugin_wrapper_query_feature(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature) {
    return TRUE;
}

static void remmina_protocol_plugin_wrapper_call_feature(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature) {

}

static void remmina_protocol_plugin_wrapper_send_keystrokes(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen) {

}

static gboolean remmina_protocol_plugin_wrapper_get_plugin_screenshot(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd) {
    return TRUE;
}



static PyObject* remmina_plugin_python_log_printf_wrapper(PyObject* self, PyObject* n) {
    char* fmt = NULL;
    if (!PyArg_ParseTuple(n, "s", &fmt)) {
        g_print("Failed to load.\n");
        PyErr_Print();
        return NULL;
    }
    
    remmina_log_printf(fmt);

    return Py_None;
}

static void remmina_protocol_init_wrapper(RemminaProtocolWidget *gp);
static void remmina_protocol_basic_settings_wrapper(RemminaProtocolWidget *gp);
static void remmina_protocol_features_wrapper(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_open_connection_wrapper(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_close_connection_wrapper(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_query_feature_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
static void remmina_protocol_call_feature_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
static void remmina_protocol_send_keytrokes_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen);
static gboolean remmina_protocol_get_plugin_screensho_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd);

static PyObject* remmina_plugin_python_register_plugin(PyObject* self, PyObject* pluginInstance) {
    if (pluginInstance) {
        g_ptr_array_add(remmina_plugin_registry, pluginInstance);

        /* Protocol plugin definition and features */
        RemminaProtocolPlugin* remmina_plugin = (RemminaProtocolPlugin*)malloc(sizeof(RemminaProtocolPlugin));
        remmina_plugin->type = REMMINA_PLUGIN_TYPE_PROTOCOL;
        remmina_plugin->name = PyUnicode_AsUTF8(PyObject_GetAttrString(pluginInstance, "name"));                                               // Name
        remmina_plugin->description = PyUnicode_AsUTF8(PyObject_GetAttrString(pluginInstance, "description"));             // Description
        remmina_plugin->domain = GETTEXT_PACKAGE;                                    // Translation domain
        remmina_plugin->version = PyUnicode_AsUTF8(PyObject_GetAttrString(pluginInstance, "version"));                                           // Version number
        remmina_plugin->icon_name = PyUnicode_AsUTF8(PyObject_GetAttrString(pluginInstance, "icon_name"));                         // Icon for normal connection
        remmina_plugin->icon_name_ssh = PyUnicode_AsUTF8(PyObject_GetAttrString(pluginInstance, "icon_name_ssh"));                     // Icon for SSH connection
        remmina_plugin->basic_settings = NULL;                // Array for basic settings
        remmina_plugin->advanced_settings = NULL;                                    // Array for advanced settings
        remmina_plugin->ssh_setting = REMMINA_PROTOCOL_SSH_SETTING_TUNNEL;        // SSH settings type
        remmina_plugin->features = NULL;                     // Array for available features
        remmina_plugin->init = remmina_protocol_init_wrapper ;                             // Plugin initialization
        remmina_plugin->open_connection = remmina_protocol_open_connection_wrapper ;       // Plugin open connection
        remmina_plugin->close_connection = remmina_protocol_close_connection_wrapper ;     // Plugin close connection
        remmina_plugin->query_feature = remmina_protocol_query_feature_wrapper ;           // Query for available features
        remmina_plugin->call_feature = remmina_protocol_call_feature_wrapper ;             // Call a feature
        remmina_plugin->send_keystrokes = remmina_protocol_send_keytrokes_wrapper;                                      // Send a keystroke
        remmina_plugin->get_plugin_screenshot = remmina_protocol_get_plugin_screensho_wrapper;                                // Screenshot support unavailable

        remmina_plugin_manager_service.register_plugin((RemminaPlugin *)remmina_plugin);

        PyPlugin* plugin = (PyPlugin*)malloc(sizeof(PyPlugin));
        plugin->protocolPlugin = remmina_plugin;
        plugin->pythonInstance = pluginInstance;
        g_ptr_array_add(remmina_plugin_registry, plugin);
    }
        
    return Py_None;
}

static PyMethodDef remmina_python_module_type_methods[] = {
	{"register_plugin", (PyCFunction)remmina_plugin_python_register_plugin,  METH_O, NULL },
	{"log_print", (PyCFunction)remmina_plugin_python_log_printf_wrapper,  METH_VARARGS, NULL },
    {NULL}  /* Sentinel */
};

static PyModuleDef remmina_python_module_type = {
    PyModuleDef_HEAD_INIT,
    .m_name = "remmina",
    .m_doc = "Remmina API.",
    .m_size = -1,
    .m_methods = remmina_python_module_type_methods
};

/**
 * 
 */
PyMODINIT_FUNC remmina_plugin_python_module_initialize(void);

void remmina_plugin_python_module_init(void) {
    remmina_plugin_registry = g_ptr_array_new();
    if (PyImport_AppendInittab("remmina", remmina_plugin_python_module_initialize))
        return FALSE;

}

PyMODINIT_FUNC remmina_plugin_python_module_initialize(void) {
    PyObject* module_instance = NULL;
    PyTypeObject* builtin;
    PyMethodDef* method;
	gint i;

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

static void remmina_protocol_init_executor(PyPlugin* plugin, RemminaProtocolWidget *gp) {
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        PyObject_CallMethod(plugin->pythonInstance, "init", NULL);
    }
}

static void remmina_protocol_init_wrapper(RemminaProtocolWidget *gp) {
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_init_executor, gp);
}

static PyObject* remmina_protocol_open_connection_executor_result = NULL;
static void remmina_protocol_open_connection_executor(PyPlugin* plugin, RemminaProtocolWidget *gp) {
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        remmina_protocol_open_connection_executor_result = PyObject_CallMethod(plugin->pythonInstance, "open_connection", "O", pygobject_new(G_OBJECT(remmina_protocol_widget_gtkviewport(gp))));
    }
}
static gboolean remmina_protocol_open_connection_wrapper(RemminaProtocolWidget *gp) {
    remmina_plugin_manager_service.protocol_plugin_signal_connection_opened(gp);
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_open_connection_executor, gp);
    return remmina_protocol_open_connection_executor_result == Py_True;
}

static PyObject* remmina_protocol_close_connection_executor_result = NULL;
static void remmina_protocol_close_connection_executor(PyPlugin* plugin, RemminaProtocolWidget *gp) {
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        remmina_protocol_close_connection_executor_result = PyObject_CallMethod(plugin->pythonInstance, "close_connection","O", pygobject_new(G_OBJECT(remmina_protocol_widget_get_gtkwindow(gp))));
    }
}
static gboolean remmina_protocol_close_connection_wrapper(RemminaProtocolWidget *gp) {
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_close_connection_executor, gp);
    return remmina_protocol_close_connection_executor_result == Py_True;
}

static gboolean remmina_protocol_query_feature_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature) {
    return TRUE;
}

static void remmina_protocol_call_feature_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature) {
    return;
}

static void remmina_protocol_send_keytrokes_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen) {
    return;
}

static gboolean remmina_protocol_get_plugin_screensho_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd) {
    return TRUE;
}
