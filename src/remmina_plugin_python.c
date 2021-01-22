/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2014-2021 Antenore Gatta, Giovanni Panozzo
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
 * @file remmina_plugin_python.c
 * @brief Remmina Python plugin loader.
 * @author Mathias Winterhalter
 * @date 14.10.2020
 *
 * When Remmina discovers Python scripts in the plugin root folder the plugin manager
 * passes the path to the Python plugin loader. There it gets exexuted and the plugin
 * classes get mapped to "real" Remmina plugin instances.
 *
 * For the communication between Remmina and Python the python module called 'remmina'
 * is initialized and loaded into the environment of the plugin script
 * (@see remmina_plugin_python_module.c).
 *
 * @see http://www.remmina.org/wp for more information.
 */

#include <gtk/gtk.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include "pygobject.h"

#include "config.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_plugin_python.h"
#include "remmina_plugin_python_module.h"

/**
 * @brief Extracts the filename without extention from a path.
 *
 * @param   in  The string to extract the filename from
 * @param   out The resulting filename without extention (must point to allocated memory).
 *
 * @return  The length of the filename extracted.
 */
static int basename_no_ext(const char* in, char** out);

/**
 *
 */
void remmina_plugin_python_init(void) {
    TRACE_CALL(__FUNC__);

    remmina_plugin_python_module_init();

    Py_Initialize();

    // Tell the Python engine where to look for Python scripts.
    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('" REMMINA_RUNTIME_PLUGINDIR "')");

    pygobject_init(-1, -1, -1);
}

gboolean remmina_plugin_python_load(RemminaPluginService* service, const gchar* name) {
    TRACE_CALL(__FUNC__);

    gchar* filename = NULL;
    if (basename_no_ext(name, &filename) == 0) {
        g_printerr("[%s:%s]: %s can not extract filename from name!\n", __FILE__, __LINE__, name);
        return FALSE;
    }

    PyObject *plugin_name = PyUnicode_DecodeFSDefault(filename);
    free(filename);

    if (!plugin_name) {
        g_printerr("[%s:%s]: Error converting plugin file name to PyUnicode!\n", __FILE__, __LINE__);
        return FALSE;
    }

    wchar_t* program_name = NULL;
    Py_ssize_t len = PyUnicode_AsWideChar(plugin_name, program_name, 0);
    program_name = malloc(sizeof(wchar_t)*len);
    if (!program_name) {
        g_printerr("[%s:%s]: Failed allocating %d bytes!\n", __FILE__, __LINE__, (sizeof(wchar_t)*len));
        return FALSE;
    }

    PyUnicode_AsWideChar(plugin_name, program_name, len);

    // This line works around the issue that in Python the sys.argv array is empty. This causes several problems since
    // many rely on the fact that the program name is set in sys.argv[0].
    PySys_SetArgv(1, &program_name);

    if (PyImport_Import(plugin_name)) {
        return TRUE;
    }

    g_print("Failed to load python plugin file: “%s”\n", name);
    PyErr_Print();
    return FALSE;
}


static int basename_no_ext(const char* in, char** out) {
    const char* base = strrchr(in, '/');
    if (base) {
        base++;
    }

    const char* base_end = strrchr(base, '.');
    if (!base_end) {
        base_end = base + strlen(base);
    }

    int length = base_end - base;
    int memsize = sizeof(char*) * ((length) + 1);
    *out = (char*)malloc(memsize);
    memset(*out, 0, memsize);
    strncpy(*out, base, length);
    (*out)[length] = '\0';

    return length;
}
