/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2014-2021 Antenore Gatta, Giovanni Panozzo, Mathias Winterhalter (ToolsDevler)
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
 */

/**
 * @file 	python_wrapper_common.h
 *
 * @brief	Contains functions and constants that are commonly used throughout the Python plugin implementation.
 *
 * @details	These functions should not be used outside of the Python plugin implementation, since everything is intended
 * 			to be used with the Python engine.
 */

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I N C L U D E S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "common/remmina_plugin.h"

#include <gtk/gtk.h>

#include <Python.h>
#include <glib.h>
#include <Python.h>
#include <structmember.h>

#include "remmina/plugin.h"
#include "config.h"


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// D E C L A R A T I O N S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

G_BEGIN_DECLS

// - Attribute names

extern const char* ATTR_NAME;
extern const char* ATTR_ICON_NAME;
extern const char* ATTR_DESCRIPTION;
extern const char* ATTR_VERSION;
extern const char* ATTR_ICON_NAME_SSH;
extern const char* ATTR_FEATURES;
extern const char* ATTR_BASIC_SETTINGS;
extern const char* ATTR_ADVANCED_SETTINGS;
extern const char* ATTR_SSH_SETTING;
extern const char* ATTR_EXPORT_HINTS;
extern const char* ATTR_PREF_LABEL;
extern const char* ATTR_INIT_ORDER;

// You can enable this for debuggin purposes or specify it in the build.
// #define WITH_PYTHON_TRACE_CALLS

/**
 * If WITH_PYTHON_TRACE_CALLS is defined, it logs the calls to the Python code and errors in case.
 */
#ifdef WITH_PYTHON_TRACE_CALLS
#define CallPythonMethod(instance, name, params, ...)                             \
		python_wrapper_last_result_set(PyObject_CallMethod(instance, name, params, ##__VA_ARGS__)); \
		python_wrapper_log_method_call(instance, name);                                          \
		python_wrapper_check_error()
#else
/**
 * If WITH_TRACE_CALL is not defined, it still logs errors but doesn't print the call anymore.
 */
#define CallPythonMethod(instance, name, params, ...)           \
    PyObject_CallMethod(instance, name, params, ##__VA_ARGS__); \
    python_wrapper_check_error()
#endif // WITH_PYTHON_TRACE_CALLS


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// T Y P E S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 	The Python abstraction of the protocol widget struct.
 *
 * @details This struct is responsible to provide the same accessibility to the protocol widget for Python as for
 * 			native plugins.
 */
typedef struct
{
	PyObject_HEAD
	RemminaProtocolWidget* gp;
} PyRemminaProtocolWidget;

/**
 * @brief 	Maps an instance of a Python plugin to a Remmina one.
 *
 * @details This is used to map a Python plugin instance to the Remmina plugin one. Also instance specific data as the
 * 			protocol widget are stored in this struct.
 */
typedef struct
{
	RemminaProtocolPlugin* protocol_plugin;
	RemminaFilePlugin* file_plugin;
	RemminaSecretPlugin* secret_plugin;
	RemminaToolPlugin* tool_plugin;
	RemminaEntryPlugin* entry_plugin;
	RemminaPrefPlugin* pref_plugin;
	RemminaPlugin* generic_plugin;
	PyRemminaProtocolWidget* gp;
	PyObject* instance;
} PyPlugin;

/**
 * A struct used to communicate data between Python and C without strict data type.
 */
typedef struct
{
	PyObject_HEAD;
	RemminaTypeHint type_hint;
	gpointer raw;
} PyGeneric;

/**
 * Checks if self is set and returns NULL if not.
 */
#define SELF_CHECK() if (!self) { \
        g_printerr("[%s:%d]: self is null!\n", __FILE__, __LINE__); \
        PyErr_SetString(PyExc_RuntimeError, "Method is not called from an instance (self is null)!"); \
        return NULL; \
    }

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * Creates a new instance of PyGeneric.
 */
PyGeneric* python_wrapper_generic_new(void);

/**
 * Registers the given plugin if no other plugin with the same name has been already registered.
 */
void python_wrapper_add_plugin(PyPlugin* plugin);

/**
 * Sets the pointer to the plugin service of Remmina.
 */
void python_wrapper_set_service(RemminaPluginService* service);

/**
 * Gets the pointer to the plugin service of Remmina.
 */
RemminaPluginService* python_wrapper_get_service(void);

/**
 * Extracts data from a PyObject instance to a generic pointer and returns a type hint if it could be determined.
 */
RemminaTypeHint python_wrapper_to_generic(PyObject* field, gpointer* target);

/**
 * Gets the result of the last python method call.
 */
PyObject* python_wrapper_last_result(void);

/**
 * @brief	Sets the result of the last python method call.
 *
 * @return 	Returns the passed result (it's done to be compatible with the CallPythonMethod macro).
 */
PyObject* python_wrapper_last_result_set(PyObject* result);

/**
 * @brief	Prints a log message to inform the user a python message has been called.
 *
 * @detail 	This method is called from the CALL_PYTHON macro if WITH_PYTHON_TRACE_CALLS is defined.
 *
 * @param 	instance The instance that contains the called method.
 * @param 	method The name of the method called.
 */
void python_wrapper_log_method_call(PyObject* instance, const char* method);

/**
 * @brief 	Checks if an error has occurred and prints it.
 *
 * @return 	Returns TRUE if an error has occurred.
 */
gboolean python_wrapper_check_error(void);

/**
 * @brief 	Gets the attribute as long value.
 *
 * @param 	instance The instance of the object to get the attribute.
 * @param 	constant_name The name of the attribute to get.
 * @param 	def The value to return if the attribute doesn't exist or is not set.
 *
 * @return 	The value attribute as long.
 */
long python_wrapper_get_attribute_long(PyObject* instance, const char* attr_name, long def);

/**
 * @brief 	Checks if a given attribute exists.
 *
 * @param 	instance The object to check for the attribute.
 * @param 	attr_name The name of the attribute to check.
 *
 * @return 	Returns TRUE if the attribute exists.
 */
gboolean python_wrapper_check_attribute(PyObject* instance, const char* attr_name);

/**
 * @brief 	Allocates memory and checks for errors before returning.
 *
 * @param 	bytes Amount of bytes to allocate.
 *
 * @return 	Address to the allocated memory.
 */
void* python_wrapper_malloc(int bytes);

/**
 * @biref 	Copies a string from a Python object to a new point in memory.
 *
 * @param 	string 	The python object, containing the string to copy.
 * @param 	len		The length of the string to copy.
 *
 * @return 	A char pointer to the new copy of the string.
 */
char* python_wrapper_copy_string_from_python(PyObject* string, Py_ssize_t len);

/**
 * @brief	Tries to find the Python plugin matching to the given instance of RemminaPlugin.
 *
 * @param 	plugin_map An array of PyPlugin pointers to search.
 * @param 	instance The RemminaPlugin instance to find the correct PyPlugin instance for.
 *
 * @return	A pointer to a PyPlugin instance if successful. Otherwise NULL is returned.
 */
PyPlugin* python_wrapper_get_plugin(const gchar* name);

/**
 * @brief	Tries to find the Python plugin matching to the given instance of RemminaPlugin.
 *
 * @param 	plugin_map An array of PyPlugin pointers to search.
 * @param 	instance The RemminaPlugin instance to find the correct PyPlugin instance for.
 *
 * @return	A pointer to a PyPlugin instance if successful. Otherwise NULL is returned.
 */
PyPlugin* python_wrapper_get_plugin_by_protocol_widget(RemminaProtocolWidget* gp);

/**
 * Creates a new GtkWidget
 * @param obj
 * @return
 */
GtkWidget* new_pywidget(GObject* obj);

/**
 * Extracts a GtkWidget from a PyObject instance.
 * @param obj
 * @return
 */
GtkWidget* get_pywidget(PyObject* obj);

/**
 * Initializes the pygobject library. This needs to be called before any Python plugin is being initialized.
 */
void init_pygobject(void);

G_END_DECLS
