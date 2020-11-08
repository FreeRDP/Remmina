/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

/**
 * @file remmina_plugin_python_module.c
 * @brief Implementation of the Python module 'remmina'.
 * @author Mathias Winterhalter
 * @date 14.10.2020
 *
 * The RemminaPluginService provides an API for plugins to interact with Remmina. The
 * module called 'remmina' forwards this interface to make it accessible for Python
 * scripts.
 *
 * This is an example of a minimal protocol plugin:
 *  
 * @code
 * import remmina
 * 
 * class MyProtocol:
 *  def __init__(self):
 *      self.name = "MyProtocol"
 *      self.description = "Example protocol plugin to explain how Python plugins work."
 *      self.version = "0.1"
 *      self.icon_name = ""
 *      self.icon_name_ssh = ""
 *
 *  def init(self, handle):
 *      print("This is getting logged to the standard output of Remmina.")
 *      remmina.log_print("For debugging purposes it would be better to log the output to the %s window %s!" % ("debug", ":)"))
 *      self.init_your_stuff(handle)
 * 
 *  def open_connection(self, handle):
 *      if not self.connect():
 *          remmina.log_print("Error! Can not connect...")
 *          return False
 *          
 *      remmina.remmina_signal_connected(handle)
 *      remmina.log_print("Connection established!")
 *      return True
 *      
 * 
 *  def close_connection(self, handle):
 *      self.disconnect()
 *      return True
 * 
 * plugin = MyProtocol()
 * remmina.register_plugin(plugin)
 * @endcode
 * 
 * 
 *
 * @see http://www.remmina.org/wp for more information.
 */

#include <glib.h>
#include <gtk/gtk.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include "config.h"
#include "pygobject.h"
#include "remmina_plugin_manager.h"
#include "remmina/plugin.h"
#include "remmina_protocol_widget.h"

/**
 * @brief Holds pairs of Python and Remmina plugin instances (PyPlugin).
 */
GPtrArray *remmina_plugin_registry = NULL;

typedef struct {
    PyObject_HEAD
    RemminaProtocolWidget* gp;
} PyRemminaProtocolWidget;

/**
 * @brief Maps an instance of a Python plugin to a Remmina one.
 */
typedef struct { 
    PyObject *pythonInstance;
    
    RemminaProtocolPlugin *protocolPlugin;
    //@TODO: Add more plugin types
    PyRemminaProtocolWidget *gp;
} PyPlugin;


/**
 * @brief Wraps the log_printf function of RemminaPluginService.
 * 
 * @details This function is only called by the Python engine! There is only one argument
 * since Python offers an inline string formatting which renders any formatting capabilities
 * redundant.
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param msg     The message to print.         
 */
static PyObject* remmina_plugin_python_log_printf_wrapper(PyObject* self, PyObject* msg);

/**
 * @brief Accepts an instance of a Python class representing a Remmina plugin.
 * 
 * @details This function is only called by the Python engine!
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  An instance of a Python plugin class.
 */
static PyObject* remmina_plugin_python_register_plugin(PyObject* self, PyObject* plugin);

/**
 * @brief Retrieves the viewport of the current connection window if available.
 * 
 * @details This function is only called by the Python engine!
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_plugin_python_get_viewport(PyObject* self, PyObject* handle);

/**
 * @brief Initializes the 'remmina' module
 * @details This function is only called by the Python engine!
 */
PyMODINIT_FUNC remmina_plugin_python_module_initialize(void);

/**
 * @brief The functions of the remmina module.
 * 
 * @details If any function has to be added this is the place to do it. This list is referenced
 * by the PyModuleDef (describing the module) instance below.
 */
static PyMethodDef remmina_python_module_type_methods[] = {
    // @TODO: Add all functions from RemminaPluginService
	{"register_plugin", (PyCFunction)remmina_plugin_python_register_plugin,  METH_O, NULL },
	{"log_print", (PyCFunction)remmina_plugin_python_log_printf_wrapper,  METH_VARARGS, NULL },
    {"get_viewport", (PyCFunction)remmina_plugin_python_get_viewport, METH_O, NULL },
    {NULL}  /* Sentinel */
};

/**
 * @brief The definition of the Python module 'remmina'.
 */
static PyModuleDef remmina_python_module_type = {
    PyModuleDef_HEAD_INIT,
    .m_name = "remmina",
    .m_doc = "Remmina API.",
    .m_size = -1,
    .m_methods = remmina_python_module_type_methods
};

static PyObject* remmina_plugin_python_log_printf_wrapper(PyObject* self, PyObject* n)
{
	TRACE_CALL(__func__);
    char* fmt = NULL;
    if (!PyArg_ParseTuple(n, "s", &fmt)) {
        g_print("Failed to load.\n");
        PyErr_Print();
        return NULL;
    }
    
    remmina_log_printf(fmt);

    return Py_None;
}

static PyObject* remmina_plugin_python_get_viewport(PyObject* self, PyObject* handle) {
    PyRemminaProtocolWidget *gp = (PyRemminaProtocolWidget*)handle;
    return pygobject_new(G_OBJECT(remmina_protocol_widget_gtkviewport(gp->gp)));
}

/**
 * @brief Initializes the remmina module and maps it to the Python engine.
 */
void remmina_plugin_python_module_init(void)
{
	TRACE_CALL(__func__);
    remmina_plugin_registry = g_ptr_array_new();
    if (PyImport_AppendInittab("remmina", remmina_plugin_python_module_initialize)) {
        g_print("Error initializing remmina module for python!\n");
        PyErr_Print();
        return;
    }
}

PyMODINIT_FUNC remmina_plugin_python_module_initialize(void)
{
	TRACE_CALL(__func__);
    return PyModule_Create(&remmina_python_module_type);
}

/**
 * 
 */
static void remmina_protocol_init_executor(PyPlugin* plugin, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        plugin->gp->gp = gp;
        PyObject_CallMethod(plugin->pythonInstance, "init", "O", plugin);
    }
}

static void remmina_protocol_init_wrapper(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_init_executor, gp);
}

static PyObject* remmina_protocol_open_connection_executor_result = NULL;
static void remmina_protocol_open_connection_executor(PyPlugin* plugin, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        remmina_protocol_open_connection_executor_result = PyObject_CallMethod(plugin->pythonInstance, "open_connection", "O", plugin);
    }
}
static gboolean remmina_protocol_open_connection_wrapper(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    remmina_plugin_manager_service.protocol_plugin_signal_connection_opened(gp);
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_open_connection_executor, gp);
    return remmina_protocol_open_connection_executor_result == Py_True;
}

static PyObject* remmina_protocol_close_connection_executor_result = NULL;
static void remmina_protocol_close_connection_executor(PyPlugin* plugin, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        remmina_protocol_close_connection_executor_result = PyObject_CallMethod(plugin->pythonInstance, "close_connection", "O", plugin);
    }
}
static gboolean remmina_protocol_close_connection_wrapper(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_close_connection_executor, gp);
    return remmina_protocol_close_connection_executor_result == Py_True;
}

static gboolean remmina_protocol_query_feature_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
    return TRUE;
}

static void remmina_protocol_call_feature_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
    return;
}

static void remmina_protocol_send_keytrokes_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL(__func__);
    return;
}

static gboolean remmina_protocol_get_plugin_screensho_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd)
{
	TRACE_CALL(__func__);
    return TRUE;
}
static PyObject* remmina_plugin_python_register_plugin(PyObject* self, PyObject* pluginInstance)
{
	TRACE_CALL(__func__);
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
        plugin->gp = malloc(sizeof(PyRemminaProtocolWidget));
        plugin->gp->gp = NULL;
        g_ptr_array_add(remmina_plugin_registry, plugin);
    }
        
    return Py_None;
}