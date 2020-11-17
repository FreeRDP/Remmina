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
#include "remmina_plugin_python_remmina.h"
#include "remmina/plugin.h"
#include "remmina_protocol_widget.h"

/**
 * 
 */
static void remmina_protocol_init_wrapper(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
    PyPlugin* plugin = remmina_plugin_python_module_get_plugin(gp);
    plugin->gp->gp = gp;
    plugin->gp->viewport = pygobject_new(G_OBJECT(remmina_protocol_widget_gtkviewport(gp->gp)));
    PyObject_CallMethod(plugin->pythonInstance, "init", "O", plugin);
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
