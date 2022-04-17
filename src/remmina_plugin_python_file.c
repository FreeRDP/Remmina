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

#include "remmina_plugin_python_common.h"
#include "remmina_plugin_python_file.h"
#include "remmina_plugin_python_remmina_file.h"
#include "remmina_file.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// D E C L A R A T I O N S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void remmina_plugin_python_file_init(void)
{
	TRACE_CALL(__func__);
}

gboolean remmina_plugin_python_file_import_test_func_wrapper(RemminaFilePlugin* instance, const gchar* from_file)
{
	TRACE_CALL(__func__);

	PyObject* result = NULL;

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);

	if (plugin)
	{
		result = CallPythonMethod(plugin->instance, "import_test_func", "s", from_file);
	}

	return result == Py_None || result != Py_False;
}

RemminaFile* remmina_plugin_python_file_import_func_wrapper(RemminaFilePlugin* instance, const gchar* from_file)
{
	TRACE_CALL(__func__);

	PyObject* result = NULL;

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);
	if (!plugin)
	{
		return NULL;
	}

	result = CallPythonMethod(plugin->instance, "import_func", "s", from_file);

	if (result == Py_None || result == Py_False)
	{
		return NULL;
	}

	return ((PyRemminaFile*)result)->file;
}

gboolean remmina_plugin_python_file_export_test_func_wrapper(RemminaFilePlugin* instance, RemminaFile* file)
{
	TRACE_CALL(__func__);

	PyObject* result = NULL;

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);
	if (plugin)
	{
		result = CallPythonMethod(plugin->instance,
			"export_test_func",
			"O",
			remmina_plugin_python_remmina_file_to_python(file));
	}

	return result == Py_None || result != Py_False;
}

gboolean
remmina_plugin_python_file_export_func_wrapper(RemminaFilePlugin* instance, RemminaFile* file, const gchar* to_file)
{
	TRACE_CALL(__func__);

	PyObject* result = NULL;

	PyPlugin* plugin = remmina_plugin_python_get_plugin(instance->name);
	if (plugin)
	{
		result = CallPythonMethod(plugin->instance, "export_func", "s", to_file);
	}

	return result == Py_None || result != Py_False;
}

RemminaPlugin* remmina_plugin_python_create_file_plugin(PyPlugin* plugin)
{
	TRACE_CALL(__func__);

	PyObject* instance = plugin->instance;
	Py_IncRef(instance);

	if (!remmina_plugin_python_check_attribute(instance, ATTR_NAME))
	{
		g_printerr("Unable to create file plugin. Aborting!\n");
		return NULL;
	}

	RemminaFilePlugin* remmina_plugin = (RemminaFilePlugin*)remmina_plugin_python_malloc(sizeof(RemminaFilePlugin));

	remmina_plugin->type = REMMINA_PLUGIN_TYPE_FILE;
	remmina_plugin->domain = GETTEXT_PACKAGE;
	remmina_plugin->name = PyUnicode_AsUTF8(PyObject_GetAttrString(instance, ATTR_NAME));
	remmina_plugin->version = PyUnicode_AsUTF8(PyObject_GetAttrString(instance, ATTR_VERSION));
	remmina_plugin->description = PyUnicode_AsUTF8(PyObject_GetAttrString(instance, ATTR_DESCRIPTION));
	remmina_plugin->export_hints = PyUnicode_AsUTF8(PyObject_GetAttrString(instance, ATTR_EXPORT_HINTS));

	remmina_plugin->import_test_func = remmina_plugin_python_file_import_test_func_wrapper;
	remmina_plugin->import_func = remmina_plugin_python_file_import_func_wrapper;
	remmina_plugin->export_test_func = remmina_plugin_python_file_export_test_func_wrapper;
	remmina_plugin->export_func = remmina_plugin_python_file_export_func_wrapper;

	plugin->file_plugin = remmina_plugin;
	plugin->generic_plugin = (RemminaPlugin*)remmina_plugin;

	remmina_plugin_python_add_plugin(plugin);

	return (RemminaPlugin*)remmina_plugin;
}
