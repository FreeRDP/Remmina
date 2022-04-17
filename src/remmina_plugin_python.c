/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2014-2022 Antenore Gatta, Giovanni Panozzo
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
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL. *  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so. *  If you
 * do not wish to do so, delete this exception statement from your
 * version. *  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I N C L U D E S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "config.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_plugin_python_common.h"
#include "remmina_plugin_python.h"
#include "remmina_plugin_python_remmina.h"
#include "remmina_plugin_python_protocol_widget.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// D E C L A R A T I O N S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * An null terminated array of commands that are executed after the initialization of the Python engine. Every entry
 * represents a line of Python code.
 */
static const char
	* python_init_commands[] = { "import sys", "sys.path.append('" REMMINA_RUNTIME_PLUGINDIR "')", NULL // Sentinel
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// U T I L S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief 	Extracts the filename without extension from a path.
 *
 * @param   in  The string to extract the filename from
 * @param   out The resulting filename without extension (must point to allocated memory).
 *
 * @return  The length of the filename extracted.
 */
static int basename_no_ext(const char* in, char** out)
{
	TRACE_CALL(__func__);

	assert(in);
	assert(out);

	const char* base = strrchr(in, '/');
	if (base)
	{
		base++;
	}

	const char* base_end = strrchr(base, '.');
	if (!base_end)
	{
		base_end = base + strlen(base);
	}

	const int length = base_end - base;
	const int memsize = sizeof(char*) * ((length) + 1);

	*out = (char*)remmina_plugin_python_malloc(memsize);

	memset(*out, 0, memsize);
	strncpy(*out, base, length);
	(*out)[length] = '\0';

	return length;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void remmina_plugin_python_init(void)
{
	TRACE_CALL(__func__);

	remmina_plugin_python_module_init();
	Py_Initialize();

	for (const char** ptr = python_init_commands; *ptr; ++ptr)
	{
		PyRun_SimpleString(*ptr);
	}

	remmina_plugin_python_protocol_widget_init();
}

gboolean remmina_plugin_python_load(RemminaPluginService* service, const char* name)
{
	TRACE_CALL(__func__);

	assert(service);
	assert(name);

	remmina_plugin_python_set_service(service);

	char* filename = NULL;
	if (basename_no_ext(name, &filename) == 0)
	{
		g_printerr("[%s:%d]: Can not extract filename from '%s'!\n", __FILE__, __LINE__, name);
		return FALSE;
	}

	PyObject* plugin_name = PyUnicode_DecodeFSDefault(filename);

	if (!plugin_name)
	{
		free(filename);
		g_printerr("[%s:%d]: Error converting plugin filename to PyUnicode!\n", __FILE__, __LINE__);
		return FALSE;
	}

	wchar_t* program_name = NULL;
	Py_ssize_t len = PyUnicode_AsWideChar(plugin_name, program_name, 0);
	if (len <= 0)
	{
		free(filename);
		g_printerr("[%s:%d]: Failed allocating %lu bytes!\n", __FILE__, __LINE__, (sizeof(wchar_t) * len));
		return FALSE;
	}

	program_name = (wchar_t*)remmina_plugin_python_malloc(sizeof(wchar_t) * len);
	if (!program_name)
	{
		free(filename);
		g_printerr("[%s:%d]: Failed allocating %lu bytes!\n", __FILE__, __LINE__, (sizeof(wchar_t) * len));
		return FALSE;
	}

	PyUnicode_AsWideChar(plugin_name, program_name, len);

	PySys_SetArgv(1, &program_name);

	if (PyImport_Import(plugin_name))
	{
		free(filename);
		return TRUE;
	}

	g_print("[%s:%d]: Failed to load python plugin file: '%s'\n", __FILE__, __LINE__, name);
	PyErr_Print();
	free(filename);

	return FALSE;
}
