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
// I N L U C E S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "remmina_plugin_python_common.h"
#include "remmina_plugin_python_secret.h"
#include "remmina_plugin_python_remmina_file.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void remmina_plugin_python_secret_init(void)
{
	TRACE_CALL(__func__);
}

gboolean remmina_plugin_python_secret_init_wrapper(RemminaSecretPlugin* instance)
{
	TRACE_CALL(__func__);

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);
	PyObject* result = CallPythonMethod(plugin->instance, "init", NULL);
	return result == Py_None || result != Py_False;
}

gboolean remmina_plugin_python_secret_is_service_available_wrapper(RemminaSecretPlugin* instance)
{
	TRACE_CALL(__func__);

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);
	PyObject* result = CallPythonMethod(plugin->instance, "is_service_available", NULL);
	return result == Py_None || result != Py_False;
}

void
remmina_plugin_python_secret_store_password_wrapper(RemminaSecretPlugin* instance, RemminaFile* file, const gchar* key, const gchar* password)
{
	TRACE_CALL(__func__);

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);
	CallPythonMethod(plugin
		->instance, "store_password", "Oss", (PyObject*)remmina_plugin_python_remmina_file_to_python(file), key, password);
}

gchar*
remmina_plugin_python_secret_get_password_wrapper(RemminaSecretPlugin* instance, RemminaFile* file, const gchar* key)
{
	TRACE_CALL(__func__);

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);
	PyObject* result = CallPythonMethod(plugin
		->instance, "get_password", "Os", (PyObject*)remmina_plugin_python_remmina_file_to_python(file), key);
	Py_ssize_t len = PyUnicode_GetLength(result);
	if (len == 0)
	{
		return NULL;
	}

	return remmina_plugin_python_copy_string_from_python(result, len);
}

void
remmina_plugin_python_secret_delete_password_wrapper(RemminaSecretPlugin* instance, RemminaFile* file, const gchar* key)
{
	TRACE_CALL(__func__);

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);
	CallPythonMethod(plugin
		->instance, "delete_password", "Os", (PyObject*)remmina_plugin_python_remmina_file_to_python(file), key);
}

RemminaPlugin* remmina_plugin_python_create_secret_plugin(PyPlugin* plugin)
{
	TRACE_CALL(__func__);

	PyObject* instance = plugin->instance;

	if (!remmina_plugin_python_check_attribute(instance, ATTR_NAME))
	{
		return NULL;
	}

	RemminaSecretPlugin* remmina_plugin = (RemminaSecretPlugin*)remmina_plugin_python_malloc(sizeof(RemminaSecretPlugin));

	remmina_plugin->type = REMMINA_PLUGIN_TYPE_SECRET;
	remmina_plugin->domain = GETTEXT_PACKAGE;
	remmina_plugin->name = PyUnicode_AsUTF8(PyObject_GetAttrString(instance, ATTR_NAME));
	remmina_plugin->version = PyUnicode_AsUTF8(PyObject_GetAttrString(instance, ATTR_VERSION));
	remmina_plugin->description = PyUnicode_AsUTF8(PyObject_GetAttrString(instance, ATTR_DESCRIPTION));
	remmina_plugin->init_order = PyLong_AsLong(PyObject_GetAttrString(instance, ATTR_INIT_ORDER));

	remmina_plugin->init = remmina_plugin_python_secret_init_wrapper;
	remmina_plugin->is_service_available = remmina_plugin_python_secret_is_service_available_wrapper;
	remmina_plugin->store_password = remmina_plugin_python_secret_store_password_wrapper;
	remmina_plugin->get_password = remmina_plugin_python_secret_get_password_wrapper;
	remmina_plugin->delete_password = remmina_plugin_python_secret_delete_password_wrapper;

	plugin->secret_plugin = remmina_plugin;
	plugin->generic_plugin = (RemminaPlugin*)remmina_plugin;

	remmina_plugin_python_add_plugin(plugin);

	return (RemminaPlugin*)remmina_plugin;
}