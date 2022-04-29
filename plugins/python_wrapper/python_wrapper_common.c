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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I N C L U D E S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "python_wrapper_common.h"

#include <assert.h>
#include <stdio.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include "pygobject.h"
#pragma GCC diagnostic pop

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// D E C L A R A T I O N S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * A cache to store the last result that has been returned by the Python code using CallPythonMethod
 * (@see python_wrapper_common.h)
 */
static PyObject* __last_result;
static GPtrArray* plugin_map = NULL;

static RemminaPluginService* remmina_plugin_service;

const char* ATTR_NAME = "name";
const char* ATTR_ICON_NAME = "icon_name";
const char* ATTR_DESCRIPTION = "description";
const char* ATTR_VERSION = "version";
const char* ATTR_ICON_NAME_SSH = "icon_name_ssh";
const char* ATTR_FEATURES = "features";
const char* ATTR_BASIC_SETTINGS = "basic_settings";
const char* ATTR_ADVANCED_SETTINGS = "advanced_settings";
const char* ATTR_SSH_SETTING = "ssh_setting";
const char* ATTR_EXPORT_HINTS = "export_hints";
const char* ATTR_PREF_LABEL = "pref_label";
const char* ATTR_INIT_ORDER = "init_order";

/**
 * To prevent some memory related attacks or accidental allocation of an excessive amount of byes, this limit should
 * always be used to check for a sane amount of bytes to allocate.
 */
static const int REASONABLE_LIMIT_FOR_MALLOC = 1024 * 1024;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PyObject* python_wrapper_last_result(void)
{
	TRACE_CALL(__func__);

	return __last_result;
}

PyObject* python_wrapper_last_result_set(PyObject* last_result)
{
	TRACE_CALL(__func__);

	return __last_result = last_result;
}

gboolean python_wrapper_check_error(void)
{
	TRACE_CALL(__func__);

	if (PyErr_Occurred())
	{
		PyErr_Print();
		return TRUE;
	}

	return FALSE;
}

void python_wrapper_log_method_call(PyObject* instance, const char* method)
{
	TRACE_CALL(__func__);

	assert(instance);
	assert(method);
	g_print("Python@%ld: %s.%s(...) -> %s\n",
		PyObject_Hash(instance),
		instance->ob_type->tp_name,
		method,
		PyUnicode_AsUTF8(PyObject_Str(python_wrapper_last_result())));
}

long python_wrapper_get_attribute_long(PyObject* instance, const char* attr_name, long def)
{
	TRACE_CALL(__func__);

	assert(instance);
	assert(attr_name);
	PyObject* attr = PyObject_GetAttrString(instance, attr_name);
	if (attr && PyLong_Check(attr))
	{
		return PyLong_AsLong(attr);
	}

	return def;
}

gboolean python_wrapper_check_attribute(PyObject* instance, const char* attr_name)
{
	TRACE_CALL(__func__);

	assert(instance);
	assert(attr_name);
	if (PyObject_HasAttrString(instance, attr_name))
	{
		return TRUE;
	}

	g_printerr("Python plugin instance is missing member: %s\n", attr_name);
	return FALSE;
}

void* python_wrapper_malloc(int bytes)
{
	TRACE_CALL(__func__);

	assert(bytes > 0);
	assert(bytes <= REASONABLE_LIMIT_FOR_MALLOC);

	void* result = malloc(bytes);

	if (!result)
	{
		g_printerr("Unable to allocate %d bytes in memory!\n", bytes);
		perror("malloc");
	}

	return result;
}

char* python_wrapper_copy_string_from_python(PyObject* string, Py_ssize_t len)
{
	TRACE_CALL(__func__);

	char* result = NULL;
	if (len <= 0 || string == NULL)
	{
		return NULL;
	}

	const char* py_str = PyUnicode_AsUTF8(string);
	if (py_str)
	{
		const int label_size = sizeof(char) * (len + 1);
		result = (char*)python_wrapper_malloc(label_size);
		result[len] = '\0';
		memcpy(result, py_str, len);
	}

	return result;
}

void python_wrapper_set_service(RemminaPluginService* service)
{
	remmina_plugin_service = service;
}

RemminaPluginService* python_wrapper_get_service(void)
{
	return remmina_plugin_service;
}

void python_wrapper_add_plugin(PyPlugin* plugin)
{
	TRACE_CALL(__func__);

	if (!plugin_map)
	{
		plugin_map = g_ptr_array_new();
	}

	PyPlugin* test = python_wrapper_get_plugin(plugin->generic_plugin->name);
	if (test)
	{
		g_printerr("A plugin named '%s' has already been registered! Skipping...", plugin->generic_plugin->name);
	}
	else
	{
		g_ptr_array_add(plugin_map, plugin);
	}
}

RemminaTypeHint python_wrapper_to_generic(PyObject* field, gpointer* target)
{
	TRACE_CALL(__func__);

	if (PyUnicode_Check(field))
	{
		Py_ssize_t len = PyUnicode_GetLength(field);

		if (len > 0)
		{
			*target = python_wrapper_copy_string_from_python(field, len);
		}
		else
		{
			*target = "";
		}

		return REMMINA_TYPEHINT_STRING;
	}
	else if (PyBool_Check(field))
	{
		*target = python_wrapper_malloc(sizeof(long));
		long* long_target = (long*)target;
		*long_target = PyLong_AsLong(field);
		return REMMINA_TYPEHINT_BOOLEAN;
	}
	else if (PyLong_Check(field))
	{
		*target = python_wrapper_malloc(sizeof(long));
		long* long_target = (long*)target;
		*long_target = PyLong_AsLong(field);
		return REMMINA_TYPEHINT_SIGNED;
	}
	else if (PyTuple_Check(field))
	{
		Py_ssize_t len = PyTuple_Size(field);
		if (len)
		{
			gpointer* dest = (gpointer*)python_wrapper_malloc(sizeof(gpointer) * (len + 1));
			memset(dest, 0, sizeof(gpointer) * (len + 1));

			for (Py_ssize_t i = 0; i < len; ++i)
			{
				PyObject* item = PyTuple_GetItem(field, i);
				python_wrapper_to_generic(item, dest + i);
			}

			*target = dest;
		}
		return REMMINA_TYPEHINT_TUPLE;
	}

	*target = NULL;
	return REMMINA_TYPEHINT_UNDEFINED;
}

PyPlugin* python_wrapper_get_plugin(const gchar* name)
{
	TRACE_CALL(__func__);

	assert(plugin_map);
	assert(name);

	for (gint i = 0; i < plugin_map->len; ++i)
	{
		PyPlugin* plugin = (PyPlugin*)g_ptr_array_index(plugin_map, i);
		if (plugin->generic_plugin && plugin->generic_plugin->name && g_str_equal(name, plugin->generic_plugin->name))
		{
			return plugin;
		}
	}

	return NULL;
}

PyPlugin* python_wrapper_get_plugin_by_protocol_widget(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);

	assert(plugin_map);
	assert(gp);

	const gchar* name = python_wrapper_get_service()->protocol_widget_get_name(gp);
	if (!name) {
		return NULL;
	}

	return python_wrapper_get_plugin(name);
}

void init_pygobject()
{
	pygobject_init(-1, -1, -1);
}

GtkWidget* new_pywidget(GObject* obj)
{
	return (GtkWidget*)pygobject_new(obj);
}

GtkWidget* get_pywidget(PyObject* obj)
{
	return (GtkWidget*)pygobject_get(obj);
}
