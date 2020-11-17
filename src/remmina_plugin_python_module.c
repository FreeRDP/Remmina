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
    {"get_viewport", (PyCFunction)remmina_plugin_manager_register_plugin_wrapper},
	{"protocol_widget_get_width", (PyCFunction)remmina_protocol_widget_get_width_wrapper},
	{"protocol_widget_set_width", (PyCFunction)remmina_protocol_widget_set_width_wrapper},
	{"protocol_widget_get_height", (PyCFunction)remmina_protocol_widget_get_height_wrapper},
	{"protocol_widget_set_height", (PyCFunction)remmina_protocol_widget_set_height_wrapper},
	{"protocol_widget_get_current_scale_mode", (PyCFunction)remmina_protocol_widget_get_current_scale_mode_wrapper},
	{"protocol_widget_get_expand", (PyCFunction)remmina_protocol_widget_get_expand_wrapper},
	{"protocol_widget_set_expand", (PyCFunction)remmina_protocol_widget_set_expand_wrapper},
	{"protocol_widget_has_error", (PyCFunction)remmina_protocol_widget_has_error_wrapper},
	{"protocol_widget_set_error", (PyCFunction)remmina_protocol_widget_set_error_wrapper},
	{"protocol_widget_is_closed", (PyCFunction)remmina_protocol_widget_is_closed_wrapper},
	{"protocol_widget_get_file", (PyCFunction)remmina_protocol_widget_get_file_wrapper},
	{"protocol_widget_emit_signal", (PyCFunction)remmina_protocol_widget_emit_signal_wrapper},
	{"protocol_widget_register_hostkey", (PyCFunction)remmina_protocol_widget_register_hostkey_wrapper},
	{"protocol_widget_start_direct_tunnel", (PyCFunction)remmina_protocol_widget_start_direct_tunnel_wrapper},
	{"protocol_widget_start_reverse_tunnel", (PyCFunction)remmina_protocol_widget_start_reverse_tunnel_wrapper},
	{"protocol_widget_start_xport_tunnel", (PyCFunction)remmina_protocol_widget_start_xport_tunnel_wrapper},
	{"protocol_widget_set_display", (PyCFunction)remmina_protocol_widget_set_display_wrapper},
	{"protocol_widget_signal_connection_closed", (PyCFunction)remmina_protocol_widget_signal_connection_closed_wrapper},
    {"protocol_widget_signal_connection_opened", (PyCFunction)remmina_protocol_widget_signal_connection_opened_wrapper},
    {"protocol_widget_update_align", (PyCFunction)remmina_protocol_widget_update_align_wrapper},
    {"protocol_widget_unlock_dynres", (PyCFunction)remmina_protocol_widget_unlock_dynres_wrapper},
    {"protocol_widget_desktop_resize", (PyCFunction)remmina_protocol_widget_desktop_resize_wrapper},
	{"protocol_widget_panel_auth", (PyCFunction)remmina_protocol_widget_panel_auth_wrapper},
	{"protocol_widget_panel_new_certificate", (PyCFunction)remmina_protocol_widget_panel_new_certificate_wrapper},
	{"protocol_widget_panel_changed_certificate", (PyCFunction)remmina_protocol_widget_panel_changed_certificate_wrapper},
	{"protocol_widget_get_username", (PyCFunction)remmina_protocol_widget_get_username_wrapper},
	{"protocol_widget_get_password", (PyCFunction)remmina_protocol_widget_get_password_wrapper},
	{"protocol_widget_get_domain", (PyCFunction)remmina_protocol_widget_get_domain_wrapper},
	{"protocol_widget_get_savepassword", (PyCFunction)remmina_protocol_widget_get_savepassword_wrapper},
	{"protocol_widget_panel_authx509", (PyCFunction)remmina_protocol_widget_panel_authx509_wrapper},
	{"protocol_widget_get_cacert", (PyCFunction)remmina_protocol_widget_get_cacert_wrapper},
	{"protocol_widget_get_cacrl", (PyCFunction)remmina_protocol_widget_get_cacrl_wrapper},
	{"protocol_widget_get_clientcert", (PyCFunction)remmina_protocol_widget_get_clientcert_wrapper},
	{"protocol_widget_get_clientkey", (PyCFunction)remmina_protocol_widget_get_clientkey_wrapper},
	{"protocol_widget_save_cred", (PyCFunction)remmina_protocol_widget_save_cred_wrapper},
	{"protocol_widget_panel_show_listen", (PyCFunction)remmina_protocol_widget_panel_show_listen_wrapper},
	{"protocol_widget_panel_show_retry", (PyCFunction)remmina_protocol_widget_panel_show_retry_wrapper},
	{"protocol_widget_panel_show", (PyCFunction)remmina_protocol_widget_panel_show_wrapper},
	{"protocol_widget_panel_hide", (PyCFunction)remmina_protocol_widget_panel_hide_wrapper},
	{"protocol_widget_ssh_exec", (PyCFunction)remmina_protocol_widget_ssh_exec_wrapper},
	{"protocol_widget_chat_open", (PyCFunction)remmina_protocol_widget_chat_open_wrapper},
	{"protocol_widget_chat_close", (PyCFunction)remmina_protocol_widget_chat_close_wrapper},
	{"protocol_widget_chat_receive", (PyCFunction)remmina_protocol_widget_chat_receive_wrapper},
	{"protocol_widget_send_keys_signals", (PyCFunction)remmina_protocol_widget_send_keys_signals_wrapper},

	{"get_datadir", (PyCFunction)remmina_file_get_datadir_wrapper},

	{"file_new", (PyCFunction)remmina_file_new_wrapper},
	{"file_get_filename", (PyCFunction)remmina_file_get_filename_wrapper},
	{"file_set_string", (PyCFunction)remmina_file_set_string_wrapper},
	{"file_get_string", (PyCFunction)remmina_file_get_string_wrapper},
	{"file_get_secret", (PyCFunction)remmina_file_get_secret_wrapper},
	{"file_set_int", (PyCFunction)remmina_file_set_int_wrapper},
	{"file_get_int", (PyCFunction)remmina_file_get_int_wrapper},
	{"file_unsave_passwords", (PyCFunction)remmina_file_unsave_passwords_wrapper},

	{"pref_set_value", (PyCFunction)remmina_pref_set_value_wrapper},
	{"pref_get_value", (PyCFunction)remmina_pref_get_value_wrapper},
	{"pref_get_scale_quality", (PyCFunction)remmina_pref_get_scale_quality_wrapper},
	{"pref_get_sshtunnel_port", (PyCFunction)remmina_pref_get_sshtunnel_port_wrapper},
	{"pref_get_ssh_loglevel", (PyCFunction)remmina_pref_get_ssh_loglevel_wrapper},
	{"pref_get_ssh_parseconfig", (PyCFunction)remmina_pref_get_ssh_parseconfig_wrapper},
	{"pref_keymap_get_keyval", (PyCFunction)remmina_pref_keymap_get_keyval_wrapper},

	{"log_print", (PyCFunction)remmina_log_print_wrapper},
	{"log_printf", (PyCFunction)remmina_log_printf_wrapper},

	{"widget_pool_register", (PyCFunction)remmina_widget_pool_register_wrapper},

	{"rcw_open_from_file_full", (PyCFunction)rcw_open_from_file_full_wrapper },
	{"public_get_server_port", (PyCFunction)remmina_public_get_server_port_wrapper},
	{"masterthread_exec_is_main_thread", (PyCFunction)remmina_masterthread_exec_is_main_thread_wrapper},
	{"gtksocket_available", (PyCFunction)remmina_gtksocket_available_wrapper},
	{"protocol_widget_get_profile_remote_width", (PyCFunction)remmina_protocol_widget_get_profile_remote_width_wrapper},
	{"protocol_widget_get_profile_remote_heigh", (PyCFunction)remmina_protocol_widget_get_profile_remote_heigh_wrapper}t
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

static PyObject* remmina_protocol_query_feature_executor_result = NULL;
static void remmina_protocol_query_feature_executor(PyPlugin* plugin, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        remmina_protocol_query_feature_executor_result = PyObject_CallMethod(plugin->pythonInstance, "query_feature", "O", plugin);
    }
}
static gboolean remmina_protocol_query_feature_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_query_feature_executor, gp);
    return remmina_protocol_query_feature_executor_result == Py_True;
}

static void remmina_protocol_call_feature_executor(PyPlugin* plugin, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        PyObject_CallMethod(plugin->pythonInstance, "call_feature", "O", plugin);
    }
}
static void remmina_protocol_call_feature_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_call_feature_executor, gp);
}

static void remmina_protocol_send_keystrokes_executor(PyPlugin* plugin, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        PyObject_CallMethod(plugin->pythonInstance, "send_keystrokes", "O", plugin);
    }
}
static void remmina_protocol_send_keytrokes_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL(__func__);
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_send_keystrokes_executor, gp);
}


static PyObject* remmina_protocol_get_plugin_screenshot_executor_result = NULL;
static void remmina_protocol_get_plugin_screenshot_executor(PyPlugin* plugin, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    if (g_str_equal(plugin->protocolPlugin->name, gp->plugin->name)) {
        remmina_protocol_get_plugin_screenshot_executor_result = PyObject_CallMethod(plugin->pythonInstance, "get_plugin_screenshot", "O", plugin);
    }
}
static gboolean remmina_protocol_get_plugin_screenshot_wrapper(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd)
{
	TRACE_CALL(__func__);
    g_ptr_array_foreach(remmina_plugin_registry, (GFunc)remmina_protocol_get_plugin_screenshot_executor, gp);
    return remmina_protocol_get_plugin_screenshot_executor_result == Py_True;
}

static RemminaPlugin* remmina_plugin_python_create_protocol_plugin(PyObject* pluginInstance)
{
        RemminaProtocolPlugin* remmina_plugin = (RemminaProtocolPlugin*)malloc(sizeof(RemminaProtocolPlugin));
        
        if(!PyObject_HasAttrString(pluginInstance, "icon_name_ssh")) {
            g_printerr("Error creating Remmina plugin. Python plugin instance is missing member: icon_name_ssh\n");
            return NULL;
        }
        else if(!PyObject_HasAttrString(pluginInstance, "icon_name")) {
            g_printerr("Error creating Remmina plugin. Python plugin instance is missing member: icon_name\n");
            return NULL;
        }
       
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
        remmina_plugin->get_plugin_screenshot = remmina_protocol_get_plugin_screenshot_wrapper;                                // Screenshot support unavailable
        return remmina_plugin;
}

static RemminaPlugin* remmina_plugin_python_create_entry_plugin(pluginInstance)
{

}
static RemminaPlugin* remmina_plugin_python_create_file_plugin(pluginInstance)
{

}
static RemminaPlugin* remmina_plugin_python_create_tool_plugin(pluginInstance)
{

}
static RemminaPlugin* remmina_plugin_python_create_pref_plugin(pluginInstance)
{

}
static RemminaPlugin* remmina_plugin_python_create_secret_plugin(pluginInstance)
{

}

static gboolean remmina_plugin_python_check_mandatory_member(PyObject* instance, const gchar* member)
{
    if (PyObject_HasAttrString(instance, "name"))
        return TRUE;

    g_printerr("Missing mandatory member in Python plugin instance: %s\n", member);
    return FALSE;
}

static PyObject* remmina_plugin_python_register_plugin(PyObject* self, PyObject* pluginInstance)
{
	TRACE_CALL(__func__);
    if (pluginInstance) {

         if (!remmina_plugin_python_check_mandatory_member(pluginInstance, "name")
            || !remmina_plugin_python_check_mandatory_member(pluginInstance, "version")) {
            return NULL;
        }

        /* Protocol plugin definition and features */
        RemminaPlugin* remmina_plugin = NULL;
        
        const gchar* pluginType = PyUnicode_AsUTF8(PyObject_GetAttrString(pluginInstance, "type"));

        if (g_str_equal(pluginType, "protocol")) {
            remmina_plugin_python_create_protocol_plugin(pluginInstance);
        } else if (g_str_equal(pluginType, "entry")) {
            remmina_plugin_python_create_entry_plugin(pluginInstance);
        } else if (g_str_equal(pluginType, "file")) {
            remmina_plugin_python_create_file_plugin(pluginInstance);
        } else if (g_str_equal(pluginType, "tool")) {
            remmina_plugin_python_create_tool_plugin(pluginInstance);
        } else if (g_str_equal(pluginType, "pref")) {
            remmina_plugin_python_create_pref_plugin(pluginInstance);
        } else if (g_str_equal(pluginType, "secret")) {
            remmina_plugin_python_create_secret_plugin(pluginInstance);
        } else {
            g_printerr("Unknown plugin type: %s\n", pluginType);
        }

        if (remmina_plugin) {
            g_ptr_array_add(remmina_plugin_registry, pluginInstance);
            remmina_plugin_manager_service.register_plugin((RemminaPlugin *)remmina_plugin);

            PyPlugin* plugin = (PyPlugin*)malloc(sizeof(PyPlugin));
            plugin->protocolPlugin = remmina_plugin;
            plugin->pythonInstance = pluginInstance;
            plugin->gp = malloc(sizeof(PyRemminaProtocolWidget));
            plugin->gp->gp = NULL;
            g_ptr_array_add(remmina_plugin_registry, plugin);
        }
    }
        
    return Py_None;
}

static PyObject* remmina_plugin_manager_register_plugin_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_width_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_set_width_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_height_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_set_height_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_current_scale_mode_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_expand_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_set_expand_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_has_error_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_set_error_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_is_closed_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_file_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_emit_signal_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_register_hostkey_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_start_direct_tunnel_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_start_reverse_tunnel_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_start_xport_tunnel_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_set_display_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_signal_connection_closed_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_signal_connection_opened_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_update_align_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_unlock_dynres_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_desktop_resize_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_panel_auth_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_panel_new_certificate_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_panel_changed_certificate_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_username_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_password_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_domain_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_savepassword_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_panel_authx509_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_cacert_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_cacrl_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_clientcert_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_clientkey_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_save_cred_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_panel_show_listen_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_panel_show_retry_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_panel_show_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_panel_hide_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_ssh_exec_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_chat_open_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_chat_close_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_chat_receive_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_send_keys_signals_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_get_datadir_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_new_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_get_filename_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_set_string_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_get_string_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_get_secret_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_set_int_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_get_int_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_file_unsave_passwords_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_pref_set_value_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_pref_get_value_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_pref_get_scale_quality_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_pref_get_sshtunnel_port_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_pref_get_ssh_loglevel_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_pref_get_ssh_parseconfig_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_pref_keymap_get_keyval_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_log_print_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_log_printf_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_widget_pool_register_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* rcw_open_from_file_full_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_public_get_server_port_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_masterthread_exec_is_main_thread_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_gtksocket_available_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_profile_remote_heigh_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}
static PyObject* remmina_protocol_widget_get_profile_remote_width_wrapper(PyObject* self, PyObject* arg)
{
    return Py_None;
}