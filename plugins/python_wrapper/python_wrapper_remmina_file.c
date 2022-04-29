/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2022 Antenore Gatta, Giovanni Panozzo
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


#include "remmina/remmina_trace_calls.h"
#include "python_wrapper_common.h"
#include "python_wrapper_remmina_file.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// D E C L A R A T I O N S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PyObject* file_get_path(PyRemminaFile* self, PyObject* args);
static PyObject* file_set_setting(PyRemminaFile* self, PyObject* args, PyObject* kwds);
static PyObject* file_get_setting(PyRemminaFile* self, PyObject* args, PyObject* kwds);
static PyObject* file_get_secret(PyRemminaFile* self, PyObject* setting);
static PyObject* file_unsave_passwords(PyRemminaFile* self, PyObject* args);

static void file_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

static PyMethodDef python_remmina_file_type_methods[] = {
	{ "get_path", (PyCFunction)file_get_path, METH_NOARGS, "" },
	{ "set_setting", (PyCFunction)file_set_setting, METH_VARARGS | METH_KEYWORDS, "Set file setting" },
	{ "get_setting", (PyCFunction)file_get_setting, METH_VARARGS | METH_KEYWORDS, "Get file setting" },
	{ "get_secret", (PyCFunction)file_get_secret, METH_VARARGS, "Get secret file setting" },
	{ "unsave_passwords", (PyCFunction)file_unsave_passwords, METH_NOARGS, "" },
	{ NULL }
};

/**
 * @brief The definition of the Python module 'remmina'.
 */
static PyTypeObject python_remmina_file_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "remmina.RemminaFile",
	.tp_doc = "",
	.tp_basicsize = sizeof(PyRemminaFile),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_methods = python_remmina_file_type_methods,
	.tp_dealloc = file_dealloc
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void python_wrapper_remmina_init_types(void)
{
	PyType_Ready(&python_remmina_file_type);
}

PyRemminaFile* python_wrapper_remmina_file_to_python(RemminaFile* file)
{
	TRACE_CALL(__func__);

	PyRemminaFile* result = PyObject_New(PyRemminaFile, &python_remmina_file_type);
	result->file = file;
	Py_INCREF(result);
	return result;
}

static PyObject* file_get_path(PyRemminaFile* self, PyObject* args)
{
	TRACE_CALL(__func__);

	return Py_BuildValue("s", python_wrapper_get_service()->file_get_path(self->file));
}

static PyObject* file_set_setting(PyRemminaFile* self, PyObject* args, PyObject* kwds)
{
	TRACE_CALL(__func__);

	static char* keyword_list[] = { "key", "value", NULL };
	gchar* key;
	PyObject* value;

	if (PyArg_ParseTupleAndKeywords(args, kwds, "s|O", keyword_list, &key, &value))
	{
		if (PyUnicode_Check(value))
		{
			python_wrapper_get_service()->file_set_string(self->file, key, PyUnicode_AsUTF8(value));
		}
		else if (PyLong_Check(value))
		{
			python_wrapper_get_service()->file_set_int(self->file, key, PyLong_AsLong(value));
		}
		else
		{
			g_printerr("%s: Not a string or int value\n", PyUnicode_AsUTF8(PyObject_Str(value)));
		}
		return Py_None;
	}
	else
	{
		g_printerr("file.set_setting(key, value): Error parsing arguments!\n");
		PyErr_Print();
		return NULL;
	}
}

static PyObject* file_get_setting(PyRemminaFile* self, PyObject* args, PyObject* kwds)
{
	TRACE_CALL(__func__);

	static gchar* keyword_list[] = { "key", "default" };
	gchar* key;
	PyObject* def;

	if (PyArg_ParseTupleAndKeywords(args, kwds, "sO", keyword_list, &key, &def))
	{
		if (PyUnicode_Check(def))
		{
			return Py_BuildValue("s", python_wrapper_get_service()->file_get_string(self->file, key));
		}
		else if (PyBool_Check(def))
		{
			return python_wrapper_get_service()->file_get_int(self->file, key, (gint)PyLong_AsLong(def)) ? Py_True : Py_False;
		}
		else if (PyLong_Check(def))
		{
			return Py_BuildValue("i", python_wrapper_get_service()->file_get_int(self->file, key, (gint)PyLong_AsLong(def)));
		}
		else
		{
			g_printerr("%s: Not a string or int value\n", PyUnicode_AsUTF8(PyObject_Str(def)));
		}
		return def;
	}
	else
	{
		g_printerr("file.get_setting(key, default): Error parsing arguments!\n");
		PyErr_Print();
		return Py_None;
	}
}

static PyObject* file_get_secret(PyRemminaFile* self, PyObject* key)
{
	TRACE_CALL(__func__);

	if (key && PyUnicode_Check(key))
	{
		return Py_BuildValue("s", python_wrapper_get_service()->file_get_secret(self->file, PyUnicode_AsUTF8(key)));
	}
	else
	{
		g_printerr("file.get_secret(key): Error parsing arguments!\n");
		PyErr_Print();
		return NULL;
	}
}

static PyObject* file_unsave_passwords(PyRemminaFile* self, PyObject* args)
{
	TRACE_CALL(__func__);

	if (self)
	{
		python_wrapper_get_service()->file_unsave_passwords(self->file);
		return Py_None;
	}
	else
	{
		g_printerr("unsave_passwords(): self is null!\n");
		return NULL;
	}
}
