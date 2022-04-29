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

#include "python_wrapper_common.h"
#include "remmina/plugin.h"
#include "remmina/types.h"

#include "python_wrapper_remmina.h"
#include "python_wrapper_protocol_widget.h"

#include "python_wrapper_entry.h"
#include "python_wrapper_file.h"
#include "python_wrapper_protocol.h"
#include "python_wrapper_tool.h"
#include "python_wrapper_secret.h"
#include "python_wrapper_pref.h"

#include "python_wrapper_remmina_file.h"

#include <string.h>

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// D E C L A R A T I O N S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/**
 * Util function to check if a specific member is define in a Python object.
 */
gboolean python_wrapper_check_mandatory_member(PyObject* instance, const gchar* member);

static PyObject* python_wrapper_debug_wrapper(PyObject* self, PyObject* msg);
static PyObject* remmina_register_plugin_wrapper(PyObject* self, PyObject* plugin);
static PyObject* remmina_file_get_datadir_wrapper(PyObject* self, PyObject* plugin);
static PyObject* remmina_file_new_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* remmina_pref_set_value_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* remmina_pref_get_value_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* remmina_pref_get_scale_quality_wrapper(PyObject* self, PyObject* plugin);
static PyObject* remmina_pref_get_sshtunnel_port_wrapper(PyObject* self, PyObject* plugin);
static PyObject* remmina_pref_get_ssh_loglevel_wrapper(PyObject* self, PyObject* plugin);
static PyObject* remmina_pref_get_ssh_parseconfig_wrapper(PyObject* self, PyObject* plugin);
static PyObject* remmina_pref_keymap_get_keyval_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* python_wrapper_log_print_wrapper(PyObject* self, PyObject* arg);
static PyObject* remmina_widget_pool_register_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* rcw_open_from_file_full_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* remmina_public_get_server_port_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* remmina_masterthread_exec_is_main_thread_wrapper(PyObject* self, PyObject* plugin);
static PyObject* remmina_gtksocket_available_wrapper(PyObject* self, PyObject* plugin);
static PyObject*
remmina_protocol_widget_get_profile_remote_height_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject*
remmina_protocol_widget_get_profile_remote_width_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* python_wrapper_show_dialog_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);
static PyObject* python_wrapper_get_mainwindow_wrapper(PyObject* self, PyObject* args);
static PyObject* remmina_protocol_plugin_signal_connection_opened_wrapper(PyObject* self, PyObject* args);
static PyObject* remmina_protocol_plugin_signal_connection_closed_wrapper(PyObject* self, PyObject* args);
static PyObject* remmina_protocol_plugin_init_auth_wrapper(PyObject* self, PyObject* args, PyObject* kwargs);

/**
 * Declares functions for the Remmina module. These functions can be called from Python and are wired to one of the
 * functions here in this file.
 */
static PyMethodDef remmina_python_module_type_methods[] = {
	/**
	 * The first function that need to be called from the plugin code, since it registers the Python class acting as
	 * a Remmina plugin. Without this call the plugin will not be recognized.
	 */
	{ "register_plugin", remmina_register_plugin_wrapper, METH_O, NULL },

	/**
	 * Prints a string into the Remmina log infrastructure.
	 */
	{ "log_print", python_wrapper_log_print_wrapper, METH_VARARGS, NULL },

	/**
	 * Prints a debug message if enabled.
	 */
	{ "debug", python_wrapper_debug_wrapper, METH_VARARGS, NULL },

	/**
	 * Shows a GTK+ dialog.
	 */
	{ "show_dialog", (PyCFunction)python_wrapper_show_dialog_wrapper, METH_VARARGS | METH_KEYWORDS,
	  NULL },

	/**
	 * Returns the GTK+ object of the main window of Remmina.
	 */
	{ "get_main_window", python_wrapper_get_mainwindow_wrapper, METH_NOARGS, NULL },

	/**
	 * Calls remmina_file_get_datadir and returns its result.
	 */
	{ "get_datadir", remmina_file_get_datadir_wrapper, METH_VARARGS, NULL },

	/**
	 * Calls remmina_file_new and returns its result.
	 */
	{ "file_new", (PyCFunction)remmina_file_new_wrapper, METH_VARARGS | METH_KEYWORDS, NULL },

	/**
	 * Calls remmina_pref_set_value and returns its result.
	 */
	{ "pref_set_value", (PyCFunction)remmina_pref_set_value_wrapper, METH_VARARGS | METH_KEYWORDS, NULL },

	/**
	 * Calls remmina_pref_get_value and returns its result.
	 */
	{ "pref_get_value", (PyCFunction)remmina_pref_get_value_wrapper, METH_VARARGS | METH_KEYWORDS, NULL },

	/**
	 * Calls remmina_pref_get_scale_quality and returns its result.
	 */
	{ "pref_get_scale_quality", remmina_pref_get_scale_quality_wrapper, METH_VARARGS, NULL },

	/**
	 * Calls remmina_pref_get_sshtunnel_port and returns its result.
	 */
	{ "pref_get_sshtunnel_port", remmina_pref_get_sshtunnel_port_wrapper, METH_VARARGS, NULL },

	/**
	 * Calls remmina_pref_get_ssh_loglevel and returns its result.
	 */
	{ "pref_get_ssh_loglevel", remmina_pref_get_ssh_loglevel_wrapper, METH_VARARGS, NULL },

	/**
	 * Calls remmina_pref_get_ssh_parseconfig and returns its result.
	 */
	{ "pref_get_ssh_parseconfig", remmina_pref_get_ssh_parseconfig_wrapper, METH_VARARGS, NULL },

	/**
	 * Calls remmina_pref_keymap_get_keyval and returns its result.
	 */
	{ "pref_keymap_get_keyval", (PyCFunction)remmina_pref_keymap_get_keyval_wrapper, METH_VARARGS | METH_KEYWORDS,
	  NULL },

	/**
	 * Calls remmina_widget_pool_register and returns its result.
	 */
	{ "widget_pool_register", (PyCFunction)remmina_widget_pool_register_wrapper, METH_VARARGS | METH_KEYWORDS, NULL },

	/**
	 * Calls rcw_open_from_file_full and returns its result.
	 */
	{ "rcw_open_from_file_full", (PyCFunction)rcw_open_from_file_full_wrapper, METH_VARARGS | METH_KEYWORDS, NULL },

	/**
	 * Calls remmina_public_get_server_port and returns its result.
	 */
	{ "public_get_server_port", (PyCFunction)remmina_public_get_server_port_wrapper, METH_VARARGS | METH_KEYWORDS,
	  NULL },

	/**
	 * Calls remmina_masterthread_exec_is_main_thread and returns its result.
	 */
	{ "masterthread_exec_is_main_thread", remmina_masterthread_exec_is_main_thread_wrapper, METH_VARARGS, NULL },

	/**
	 * Calls remmina_gtksocket_available and returns its result.
	 */
	{ "gtksocket_available", remmina_gtksocket_available_wrapper, METH_VARARGS, NULL },

	/**
	 * Calls remmina_protocol_widget_get_profile_remote_width and returns its result.
	 */
	{ "protocol_widget_get_profile_remote_width",
	  (PyCFunction)remmina_protocol_widget_get_profile_remote_width_wrapper, METH_VARARGS | METH_KEYWORDS, NULL },

	/**
	 * Calls remmina_protocol_widget_get_profile_remote_height and returns its result.
	 */
	{ "protocol_widget_get_profile_remote_height",
	  (PyCFunction)remmina_protocol_widget_get_profile_remote_height_wrapper, METH_VARARGS | METH_KEYWORDS, NULL },
	{ "protocol_plugin_signal_connection_opened", (PyCFunction)remmina_protocol_plugin_signal_connection_opened_wrapper,
	  METH_VARARGS, NULL },

	{ "protocol_plugin_signal_connection_closed", (PyCFunction)remmina_protocol_plugin_signal_connection_closed_wrapper,
	  METH_VARARGS, NULL },

	{ "protocol_plugin_init_auth", (PyCFunction)remmina_protocol_plugin_init_auth_wrapper, METH_VARARGS | METH_KEYWORDS,
	  NULL },

	/* Sentinel */
	{ NULL }
};

/**
 * Adapter struct to handle Remmina protocol settings.
 */
typedef struct
{
	PyObject_HEAD
	RemminaProtocolSettingType settingType;
	gchar* name;
	gchar* label;
	gboolean compact;
	PyObject* opt1;
	PyObject* opt2;
} PyRemminaProtocolSetting;

/**
 * The definition of the Python module 'remmina'.
 */
static PyModuleDef remmina_python_module_type = {
	PyModuleDef_HEAD_INIT,
	.m_name = "remmina",
	.m_doc = "Remmina API.",
	.m_size = -1,
	.m_methods = remmina_python_module_type_methods
};

// -- Python Object -> Setting

/**
 * Initializes the memory and the fields of the remmina.Setting Python type.
 * @details This function is callback for the Python engine.
 */
static PyObject* python_protocol_setting_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	PyRemminaProtocolSetting* self = (PyRemminaProtocolSetting*)type->tp_alloc(type, 0);

	if (!self)
	{
		return NULL;
	}

	self->name = "";
	self->label = "";
	self->compact = FALSE;
	self->opt1 = NULL;
	self->opt2 = NULL;
	self->settingType = 0;

	return (PyObject*)self;
}

/**
 * Constructor of the remmina.Setting Python type.
 * @details This function is callback for the Python engine.
 */
static int python_protocol_setting_init(PyRemminaProtocolSetting* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static gchar* kwlist[] = { "type", "name", "label", "compact", "opt1", "opt2", NULL };
	PyObject* name;
	PyObject* label;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|lOOpOO", kwlist,
		&self->settingType, &name, &label, &self->compact, &self->opt1, &self->opt2))
	{
		return -1;
	}

	Py_ssize_t len = PyUnicode_GetLength(label);
	if (len == 0)
	{
		self->label = "";
	}
	else
	{
		self->label = python_wrapper_copy_string_from_python(label, len);
		if (!self->label)
		{
			g_printerr("Unable to extract label during initialization of Python settings module!\n");
			python_wrapper_check_error();
		}
	}

	len = PyUnicode_GetLength(name);
	if (len == 0)
	{
		self->name = "";
	}
	else
	{
		self->name = python_wrapper_copy_string_from_python(label, len);
		if (!self->name)
		{
			g_printerr("Unable to extract name during initialization of Python settings module!\n");
			python_wrapper_check_error();
		}
	}

	return 0;
}

static PyMemberDef python_protocol_setting_type_members[] = {
	{ "settingType", offsetof(PyRemminaProtocolSetting, settingType), T_INT, 0, NULL },
	{ "name", offsetof(PyRemminaProtocolSetting, name), T_STRING, 0, NULL },
	{ "label", offsetof(PyRemminaProtocolSetting, label), T_STRING, 0, NULL },
	{ "compact", offsetof(PyRemminaProtocolSetting, compact), T_BOOL, 0, NULL },
	{ "opt1", offsetof(PyRemminaProtocolSetting, opt1), T_OBJECT, 0, NULL },
	{ "opt2", offsetof(PyRemminaProtocolSetting, opt2), T_OBJECT, 0, NULL },
	{ NULL }
};

static PyTypeObject python_protocol_setting_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "remmina.Setting",
	.tp_doc = "Remmina Setting information",
	.tp_basicsize = sizeof(PyRemminaProtocolSetting),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = python_protocol_setting_new,
	.tp_init = (initproc)python_protocol_setting_init,
	.tp_members = python_protocol_setting_type_members
};

// -- Python Type -> Feature


static PyMemberDef python_protocol_feature_members[] = {
	{ "type", T_INT, offsetof(PyRemminaProtocolFeature, type), 0, NULL },
	{ "id", T_INT, offsetof(PyRemminaProtocolFeature, id), 0, NULL },
	{ "opt1", T_OBJECT, offsetof(PyRemminaProtocolFeature, opt1), 0, NULL },
	{ "opt2", T_OBJECT, offsetof(PyRemminaProtocolFeature, opt2), 0, NULL },
	{ "opt3", T_OBJECT, offsetof(PyRemminaProtocolFeature, opt3), 0, NULL },
	{ NULL }
};

PyObject* python_protocol_feature_new(PyTypeObject* type, PyObject* kws, PyObject* args)
{
	TRACE_CALL(__func__);

	PyRemminaProtocolFeature* self;
	self = (PyRemminaProtocolFeature*)type->tp_alloc(type, 0);
	if (!self)
		return NULL;

	self->id = 0;
	self->type = 0;
	self->opt1 = (PyGeneric*)Py_None;
	self->opt1->raw = NULL;
	self->opt1->type_hint = REMMINA_TYPEHINT_UNDEFINED;
	self->opt2 = (PyGeneric*)Py_None;
	self->opt2->raw = NULL;
	self->opt2->type_hint = REMMINA_TYPEHINT_UNDEFINED;
	self->opt3 = (PyGeneric*)Py_None;
	self->opt3->raw = NULL;
	self->opt3->type_hint = REMMINA_TYPEHINT_UNDEFINED;

	return (PyObject*)self;
}

static int python_protocol_feature_init(PyRemminaProtocolFeature* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "type", "id", "opt1", "opt2", "opt3", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|llOOO", kwlist, &self->type, &self->id, &self->opt1, &self
		->opt2, &self->opt3))
		return -1;

	return 0;
}

static PyTypeObject python_protocol_feature_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "remmina.ProtocolFeature",
	.tp_doc = "Remmina Setting information",
	.tp_basicsize = sizeof(PyRemminaProtocolFeature),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = python_protocol_feature_new,
	.tp_init = (initproc)python_protocol_feature_init,
	.tp_members = python_protocol_feature_members
};

PyRemminaProtocolFeature* python_wrapper_protocol_feature_new(void)
{
	PyRemminaProtocolFeature* feature = (PyRemminaProtocolFeature*)PyObject_New(PyRemminaProtocolFeature, &python_protocol_feature_type);
	feature->id = 0;
	feature->opt1 = python_wrapper_generic_new();
	feature->opt1->raw = NULL;
	feature->opt1->type_hint = REMMINA_TYPEHINT_UNDEFINED;
	feature->opt2 = python_wrapper_generic_new();
	feature->opt2->raw = NULL;
	feature->opt2->type_hint = REMMINA_TYPEHINT_UNDEFINED;
	feature->opt3 = python_wrapper_generic_new();
	feature->opt3->raw = NULL;
	feature->opt3->type_hint = REMMINA_TYPEHINT_UNDEFINED;
	feature->type = 0;
	Py_IncRef((PyObject*)feature);
	return feature;
}


// -- Python Type -> Screenshot Data

static PyMemberDef python_screenshot_data_members[] = {
	{ "buffer", T_OBJECT, offsetof(PyRemminaPluginScreenshotData, buffer), 0, NULL },
	{ "width", T_INT, offsetof(PyRemminaPluginScreenshotData, width), 0, NULL },
	{ "height", T_INT, offsetof(PyRemminaPluginScreenshotData, height), 0, NULL },
	{ "bitsPerPixel", T_INT, offsetof(PyRemminaPluginScreenshotData, bitsPerPixel), 0, NULL },
	{ "bytesPerPixel", T_INT, offsetof(PyRemminaPluginScreenshotData, bytesPerPixel), 0, NULL },
	{ NULL }
};

PyObject* python_screenshot_data_new(PyTypeObject* type, PyObject* kws, PyObject* args)
{
	TRACE_CALL(__func__);

	PyRemminaPluginScreenshotData* self;
	self = (PyRemminaPluginScreenshotData*)type->tp_alloc(type, 0);
	if (!self)
		return NULL;

	self->buffer = (PyByteArrayObject*)PyObject_New(PyByteArrayObject, &PyByteArray_Type);
	self->height = 0;
	self->width = 0;
	self->bitsPerPixel = 0;
	self->bytesPerPixel = 0;

	return (PyObject*)self;
}

static int python_screenshot_data_init(PyRemminaPluginScreenshotData* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	g_printerr("Not to be initialized within Python!");
	return -1;
}

static PyTypeObject python_screenshot_data_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "remmina.RemminaScreenshotData",
	.tp_doc = "Remmina Screenshot Data",
	.tp_basicsize = sizeof(PyRemminaPluginScreenshotData),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = python_screenshot_data_new,
	.tp_init = (initproc)python_screenshot_data_init,
	.tp_members = python_screenshot_data_members
};
PyRemminaPluginScreenshotData* python_wrapper_screenshot_data_new(void)
{
	PyRemminaPluginScreenshotData* data = (PyRemminaPluginScreenshotData*)PyObject_New(PyRemminaPluginScreenshotData, &python_screenshot_data_type);
	data->buffer = PyObject_New(PyByteArrayObject, &PyByteArray_Type);
	Py_IncRef((PyObject*)data->buffer);
	data->height = 0;
	data->width = 0;
	data->bitsPerPixel = 0;
	data->bytesPerPixel = 0;
	return data;
}

static PyObject* python_wrapper_generic_to_int(PyGeneric* self, PyObject* args);
static PyObject* python_wrapper_generic_to_bool(PyGeneric* self, PyObject* args);
static PyObject* python_wrapper_generic_to_string(PyGeneric* self, PyObject* args);

static void python_wrapper_generic_dealloc(PyObject* self)
{
	PyObject_Del(self);
}

static PyMethodDef python_wrapper_generic_methods[] = {
	{ "to_int", (PyCFunction)python_wrapper_generic_to_int, METH_NOARGS, "" },
	{ "to_bool", (PyCFunction)python_wrapper_generic_to_bool, METH_NOARGS, "" },
	{ "to_string", (PyCFunction)python_wrapper_generic_to_string, METH_NOARGS, "" },
	{ NULL }
};

static PyMemberDef python_wrapper_generic_members[] = {
	{ "raw", T_OBJECT, offsetof(PyGeneric, raw), 0, "" },
	{ NULL }
};

PyObject* python_wrapper_generic_type_new(PyTypeObject* type, PyObject* kws, PyObject* args)
{
	TRACE_CALL(__func__);

	PyGeneric* self;
	self = (PyGeneric*)type->tp_alloc(type, 0);
	if (!self)
		return NULL;

	self->raw = Py_None;

	return (PyObject*)self;
}

static int python_wrapper_generic_init(PyGeneric* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "raw", NULL };

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", kwlist, &self->raw))
		return -1;

	return 0;
}

static PyTypeObject python_generic_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "remmina.Generic",
	.tp_doc = "",
	.tp_basicsize = sizeof(PyGeneric),
	.tp_itemsize = 0,
	.tp_dealloc = python_wrapper_generic_dealloc,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = python_wrapper_generic_type_new,
	.tp_init = (initproc)python_wrapper_generic_init,
	.tp_members = python_wrapper_generic_members,
	.tp_methods = python_wrapper_generic_methods,
};

PyGeneric* python_wrapper_generic_new(void)
{
	PyGeneric* generic = (PyGeneric*)PyObject_New(PyGeneric, &python_generic_type);
	generic->raw = PyLong_FromLongLong(0LL);
	Py_IncRef((PyObject*)generic);
	return generic;
}

static PyObject* python_wrapper_generic_to_int(PyGeneric* self, PyObject* args)
{
	SELF_CHECK();

	if (self->raw == NULL)
	{
		return Py_None;
	}
	else if (self->type_hint == REMMINA_TYPEHINT_SIGNED)
	{
		return PyLong_FromLongLong((long long)self->raw);
	}
	else if (self->type_hint == REMMINA_TYPEHINT_UNSIGNED)
	{
		return PyLong_FromUnsignedLongLong((unsigned long long)self->raw);
	}

	return Py_None;
}
static PyObject* python_wrapper_generic_to_bool(PyGeneric* self, PyObject* args)
{
	SELF_CHECK();

	if (self->raw == NULL)
	{
		return Py_None;
	}
	else if (self->type_hint == REMMINA_TYPEHINT_BOOLEAN)
	{
		return PyBool_FromLong((long)self->raw);
	}

	return Py_None;
}
static PyObject* python_wrapper_generic_to_string(PyGeneric* self, PyObject* args)
{
	SELF_CHECK();

	if (self->raw == NULL)
	{
		return Py_None;
	}
	else if (self->type_hint == REMMINA_TYPEHINT_STRING)
	{
		return PyUnicode_FromString((const char*)self->raw);
	}

	return Py_None;
}

/**
 * Is called from the Python engine when it initializes the 'remmina' module.
 * @details This function is only called by the Python engine!
 */
PyMODINIT_FUNC python_wrapper_module_initialize(void)
{
	TRACE_CALL(__func__);

	if (PyType_Ready(&python_screenshot_data_type) < 0)
	{
		PyErr_Print();
		return NULL;
	}

	if (PyType_Ready(&python_generic_type) < 0)
	{
		PyErr_Print();
		return NULL;
	}

	if (PyType_Ready(&python_protocol_setting_type) < 0)
	{
		PyErr_Print();
		return NULL;
	}

	if (PyType_Ready(&python_protocol_feature_type) < 0)
	{
		PyErr_Print();
		return NULL;
	}

	python_wrapper_protocol_widget_type_ready();
	python_wrapper_remmina_init_types();

	PyObject* module = PyModule_Create(&remmina_python_module_type);
	if (!module)
	{
		PyErr_Print();
		return NULL;
	}

	PyModule_AddIntConstant(module, "BUTTONS_CLOSE", (long)GTK_BUTTONS_CLOSE);
	PyModule_AddIntConstant(module, "BUTTONS_NONE", (long)GTK_BUTTONS_NONE);
	PyModule_AddIntConstant(module, "BUTTONS_OK", (long)GTK_BUTTONS_OK);
	PyModule_AddIntConstant(module, "BUTTONS_CLOSE", (long)GTK_BUTTONS_CLOSE);
	PyModule_AddIntConstant(module, "BUTTONS_CANCEL", (long)GTK_BUTTONS_CANCEL);
	PyModule_AddIntConstant(module, "BUTTONS_YES_NO", (long)GTK_BUTTONS_YES_NO);
	PyModule_AddIntConstant(module, "BUTTONS_OK_CANCEL", (long)GTK_BUTTONS_OK_CANCEL);

	PyModule_AddIntConstant(module, "MESSAGE_INFO", (long)GTK_MESSAGE_INFO);
	PyModule_AddIntConstant(module, "MESSAGE_WARNING", (long)GTK_MESSAGE_WARNING);
	PyModule_AddIntConstant(module, "MESSAGE_QUESTION", (long)GTK_MESSAGE_QUESTION);
	PyModule_AddIntConstant(module, "MESSAGE_ERROR", (long)GTK_MESSAGE_ERROR);
	PyModule_AddIntConstant(module, "MESSAGE_OTHER", (long)GTK_MESSAGE_OTHER);

	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_SERVER", (long)REMMINA_PROTOCOL_SETTING_TYPE_SERVER);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_PASSWORD", (long)REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_RESOLUTION", (long)REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_KEYMAP", (long)REMMINA_PROTOCOL_SETTING_TYPE_KEYMAP);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_TEXT", (long)REMMINA_PROTOCOL_SETTING_TYPE_TEXT);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_SELECT", (long)REMMINA_PROTOCOL_SETTING_TYPE_SELECT);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_COMBO", (long)REMMINA_PROTOCOL_SETTING_TYPE_COMBO);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_CHECK", (long)REMMINA_PROTOCOL_SETTING_TYPE_CHECK);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_FILE", (long)REMMINA_PROTOCOL_SETTING_TYPE_FILE);
	PyModule_AddIntConstant(module, "PROTOCOL_SETTING_TYPE_FOLDER", (long)REMMINA_PROTOCOL_SETTING_TYPE_FOLDER);
	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_TYPE_MULTIMON", (long)REMMINA_PROTOCOL_FEATURE_TYPE_MULTIMON);

	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_TYPE_PREF", (long)REMMINA_PROTOCOL_FEATURE_TYPE_PREF);
	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_TYPE_TOOL", (long)REMMINA_PROTOCOL_FEATURE_TYPE_TOOL);
	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_TYPE_UNFOCUS", (long)REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS);
	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_TYPE_SCALE", (long)REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);
	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_TYPE_DYNRESUPDATE", (long)REMMINA_PROTOCOL_FEATURE_TYPE_DYNRESUPDATE);
	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_TYPE_GTKSOCKET", (long)REMMINA_PROTOCOL_FEATURE_TYPE_GTKSOCKET);

	PyModule_AddIntConstant(module, "PROTOCOL_SSH_SETTING_NONE", (long)REMMINA_PROTOCOL_SSH_SETTING_NONE);
	PyModule_AddIntConstant(module, "PROTOCOL_SSH_SETTING_TUNNEL", (long)REMMINA_PROTOCOL_SSH_SETTING_TUNNEL);
	PyModule_AddIntConstant(module, "PROTOCOL_SSH_SETTING_SSH", (long)REMMINA_PROTOCOL_SSH_SETTING_SSH);
	PyModule_AddIntConstant(module, "PROTOCOL_SSH_SETTING_REVERSE_TUNNEL", (long)REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL);
	PyModule_AddIntConstant(module, "PROTOCOL_SSH_SETTING_SFTP", (long)REMMINA_PROTOCOL_SSH_SETTING_SFTP);

	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_PREF_RADIO", (long)REMMINA_PROTOCOL_FEATURE_PREF_RADIO);
	PyModule_AddIntConstant(module, "PROTOCOL_FEATURE_PREF_CHECK", (long)REMMINA_PROTOCOL_FEATURE_PREF_CHECK);

	PyModule_AddIntConstant(module, "MESSAGE_PANEL_FLAG_USERNAME", REMMINA_MESSAGE_PANEL_FLAG_USERNAME);
	PyModule_AddIntConstant(module, "MESSAGE_PANEL_FLAG_USERNAME_READONLY", REMMINA_MESSAGE_PANEL_FLAG_USERNAME_READONLY);
	PyModule_AddIntConstant(module, "MESSAGE_PANEL_FLAG_DOMAIN", REMMINA_MESSAGE_PANEL_FLAG_DOMAIN);
	PyModule_AddIntConstant(module, "MESSAGE_PANEL_FLAG_SAVEPASSWORD", REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD);

	if (PyModule_AddObject(module, "Setting", (PyObject*)&python_protocol_setting_type) < 0)
	{
		Py_DECREF(&python_protocol_setting_type);
		Py_DECREF(module);
		PyErr_Print();
		return NULL;
	}

	Py_INCREF(&python_protocol_feature_type);
	if (PyModule_AddObject(module, "Feature", (PyObject*)&python_protocol_feature_type) < 0)
	{
		Py_DECREF(&python_protocol_setting_type);
		Py_DECREF(&python_protocol_feature_type);
		Py_DECREF(module);
		PyErr_Print();
		return NULL;
	}

	return module;
}

/**
 * Initializes all globals and registers the 'remmina' module in the Python engine.
 * @details This
 */
void python_wrapper_module_init(void)
{
	TRACE_CALL(__func__);

	if (PyImport_AppendInittab("remmina", python_wrapper_module_initialize))
	{
		PyErr_Print();
		exit(1);
	}

	python_wrapper_entry_init();
	python_wrapper_protocol_init();
	python_wrapper_tool_init();
	python_wrapper_pref_init();
	python_wrapper_secret_init();
	python_wrapper_file_init();
}

gboolean python_wrapper_check_mandatory_member(PyObject* instance, const gchar* member)
{
	TRACE_CALL(__func__);

	if (PyObject_HasAttrString(instance, member))
	{
		return TRUE;
	}

	g_printerr("Missing mandatory member '%s' in Python plugin instance!\n", member);
	return FALSE;
}

static PyObject* remmina_register_plugin_wrapper(PyObject* self, PyObject* plugin_instance)
{
	TRACE_CALL(__func__);

	if (plugin_instance)
	{
		if (!python_wrapper_check_mandatory_member(plugin_instance, "name")
			|| !python_wrapper_check_mandatory_member(plugin_instance, "version"))
		{
			return NULL;
		}

		/* Protocol plugin definition and features */
		const gchar* pluginType = PyUnicode_AsUTF8(PyObject_GetAttrString(plugin_instance, "type"));

		RemminaPlugin* remmina_plugin = NULL;

		PyPlugin* plugin = (PyPlugin*)python_wrapper_malloc(sizeof(PyPlugin));
		plugin->instance = plugin_instance;
		Py_INCREF(plugin_instance);
		plugin->protocol_plugin = NULL;
		plugin->entry_plugin = NULL;
		plugin->file_plugin = NULL;
		plugin->pref_plugin = NULL;
		plugin->secret_plugin = NULL;
		plugin->tool_plugin = NULL;
		g_print("New Python plugin registered: %ld\n", PyObject_Hash(plugin_instance));

		if (g_str_equal(pluginType, "protocol"))
		{
			remmina_plugin = python_wrapper_create_protocol_plugin(plugin);
		}
		else if (g_str_equal(pluginType, "entry"))
		{
			remmina_plugin = python_wrapper_create_entry_plugin(plugin);
		}
		else if (g_str_equal(pluginType, "file"))
		{
			remmina_plugin = python_wrapper_create_file_plugin(plugin);
		}
		else if (g_str_equal(pluginType, "tool"))
		{
			remmina_plugin = python_wrapper_create_tool_plugin(plugin);
		}
		else if (g_str_equal(pluginType, "pref"))
		{
			remmina_plugin = python_wrapper_create_pref_plugin(plugin);
		}
		else if (g_str_equal(pluginType, "secret"))
		{
			remmina_plugin = python_wrapper_create_secret_plugin(plugin);
		}
		else
		{
			g_printerr("Unknown plugin type: %s\n", pluginType);
		}

		if (remmina_plugin)
		{
			if (remmina_plugin->type == REMMINA_PLUGIN_TYPE_PROTOCOL)
			{
				plugin->gp = python_wrapper_protocol_widget_create();
			}

			if (python_wrapper_get_service()->register_plugin((RemminaPlugin*)remmina_plugin)) {
				g_print("%s: Successfully reigstered!\n", remmina_plugin->name);
			} else {
				g_print("%s: Failed to reigster!\n", remmina_plugin->name);
			}
		}
	}

	PyErr_Clear();
	return Py_None;
}

static PyObject* remmina_file_get_datadir_wrapper(PyObject* self, PyObject* plugin)
{
	TRACE_CALL(__func__);

	PyObject* result = Py_None;
	const gchar* datadir = python_wrapper_get_service()->file_get_user_datadir();

	if (datadir)
	{
		result = PyUnicode_FromFormat("%s", datadir);
	}

	python_wrapper_check_error();
	return result;
}

static PyObject* remmina_file_new_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	RemminaFile* file = python_wrapper_get_service()->file_new();
	if (file)
	{
		return (PyObject*)python_wrapper_remmina_file_to_python(file);
	}

	python_wrapper_check_error();
	return Py_None;
}

static PyObject* remmina_pref_set_value_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "key", "value", NULL };
	gchar* key, * value;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "ss", kwlist, &key, &value))
	{
		return Py_None;
	}

	if (key)
	{
		python_wrapper_get_service()->pref_set_value(key, value);
	}

	python_wrapper_check_error();
	return Py_None;
}

static PyObject* remmina_pref_get_value_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "key", NULL };
	gchar* key;
	PyObject* result = Py_None;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s", kwlist, &key))
	{
		return Py_None;
	}

	if (key)
	{
		const gchar* value = python_wrapper_get_service()->pref_get_value(key);
		if (value)
		{
			result = PyUnicode_FromFormat("%s", result);
		}
	}

	python_wrapper_check_error();
	return result;
}

static PyObject* remmina_pref_get_scale_quality_wrapper(PyObject* self, PyObject* plugin)
{
	TRACE_CALL(__func__);

	PyObject* result = PyLong_FromLong(python_wrapper_get_service()->pref_get_scale_quality());
	python_wrapper_check_error();
	return result;
}

static PyObject* remmina_pref_get_sshtunnel_port_wrapper(PyObject* self, PyObject* plugin)
{
	TRACE_CALL(__func__);

	PyObject* result = PyLong_FromLong(python_wrapper_get_service()->pref_get_sshtunnel_port());
	python_wrapper_check_error();
	return result;
}

static PyObject* remmina_pref_get_ssh_loglevel_wrapper(PyObject* self, PyObject* plugin)
{
	TRACE_CALL(__func__);

	PyObject* result = PyLong_FromLong(python_wrapper_get_service()->pref_get_ssh_loglevel());
	python_wrapper_check_error();
	return result;
}

static PyObject* remmina_pref_get_ssh_parseconfig_wrapper(PyObject* self, PyObject* plugin)
{
	TRACE_CALL(__func__);

	PyObject* result = PyLong_FromLong(python_wrapper_get_service()->pref_get_ssh_parseconfig());
	python_wrapper_check_error();
	return result;
}

static PyObject* remmina_pref_keymap_get_keyval_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "keymap", "keyval", NULL };
	gchar* keymap;
	guint keyval;
	PyObject* result = Py_None;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sl", kwlist, &keymap, &keyval))
	{
		return PyLong_FromLong(-1);
	}

	if (keymap)
	{
		const guint value = python_wrapper_get_service()->pref_keymap_get_keyval(keymap, keyval);
		result = PyLong_FromUnsignedLong(value);
	}

	python_wrapper_check_error();
	return result;
}

static PyObject* python_wrapper_log_print_wrapper(PyObject* self, PyObject* args)
{
	TRACE_CALL(__func__);

	gchar* text;
	if (!PyArg_ParseTuple(args, "s", &text) || !text)
	{
		return Py_None;
	}

	python_wrapper_get_service()->log_print(text);
	return Py_None;
}

static PyObject* python_wrapper_debug_wrapper(PyObject* self, PyObject* args)
{
	TRACE_CALL(__func__);

	gchar* text;
	if (!PyArg_ParseTuple(args, "s", &text) || !text)
	{
		return Py_None;
	}

	python_wrapper_get_service()->_remmina_debug("python", "%s", text);
	return Py_None;
}

static PyObject* remmina_widget_pool_register_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "widget", NULL };
	PyObject* widget;

	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &widget) && widget)
	{
		python_wrapper_get_service()->widget_pool_register(get_pywidget(widget));
	}

	return Py_None;
}

static PyObject* rcw_open_from_file_full_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "remminafile", "data", "handler", NULL };
	PyObject* pyremminafile;
	PyObject* data;

	if (PyArg_ParseTupleAndKeywords(args, kwargs, "OOO", kwlist, &pyremminafile, &data) && pyremminafile && data)
	{
		python_wrapper_get_service()->rcw_open_from_file_full((RemminaFile*)pyremminafile, NULL, (void*)data, NULL);
	}

	return Py_None;
}

static PyObject* remmina_public_get_server_port_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "server", "defaultport", "host", "port", NULL };
	gchar* server;
	gint defaultport;

	if (PyArg_ParseTupleAndKeywords(args, kwargs, "slsl", kwlist, &server, &defaultport) && server)
	{
		gchar* host;
		gint port;
		python_wrapper_get_service()->get_server_port(server, defaultport, &host, &port);

		PyObject* result = PyList_New(2);
		PyList_Append(result, PyUnicode_FromString(host));
		PyList_Append(result, PyLong_FromLong(port));
		return PyList_AsTuple(result);
	}

	return Py_None;
}

static PyObject* remmina_masterthread_exec_is_main_thread_wrapper(PyObject* self, PyObject* plugin)
{
	TRACE_CALL(__func__);

	return PyBool_FromLong(python_wrapper_get_service()->is_main_thread());
}

static PyObject* remmina_gtksocket_available_wrapper(PyObject* self, PyObject* plugin)
{
	TRACE_CALL(__func__);

	return PyBool_FromLong(python_wrapper_get_service()->gtksocket_available());
}

static PyObject*
remmina_protocol_widget_get_profile_remote_height_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "widget", NULL };
	PyPlugin* plugin;

	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &plugin) && plugin && plugin->gp)
	{
		python_wrapper_get_service()->get_profile_remote_height(plugin->gp->gp);
	}

	return Py_None;
}

static PyObject*
remmina_protocol_widget_get_profile_remote_width_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "widget", NULL };
	PyPlugin* plugin;

	if (PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &plugin) && plugin && plugin->gp)
	{
		python_wrapper_get_service()->get_profile_remote_width(plugin->gp->gp);
	}

	return Py_None;
}

void python_wrapper_to_protocol_setting(RemminaProtocolSetting* dest, PyObject* setting)
{
	TRACE_CALL(__func__);

	PyRemminaProtocolSetting* src = (PyRemminaProtocolSetting*)setting;
	Py_INCREF(setting);
	dest->name = src->name;
	dest->label = src->label;
	dest->compact = src->compact;
	dest->type = src->settingType;
	dest->validator = NULL;
	dest->validator_data = NULL;
	python_wrapper_to_generic(src->opt1, &dest->opt1);
	python_wrapper_to_generic(src->opt2, &dest->opt2);
}

void python_wrapper_to_protocol_feature(RemminaProtocolFeature* dest, PyObject* feature)
{
	TRACE_CALL(__func__);

	PyRemminaProtocolFeature* src = (PyRemminaProtocolFeature*)feature;
	Py_INCREF(feature);
	dest->id = src->id;
	dest->type = src->type;
	dest->opt1 = src->opt1->raw;
	dest->opt1_type_hint = src->opt1->type_hint;
	dest->opt2 = src->opt2->raw;
	dest->opt2_type_hint = src->opt2->type_hint;
	dest->opt3 = src->opt3->raw;
	dest->opt3_type_hint = src->opt3->type_hint;
}

PyObject* python_wrapper_show_dialog_wrapper(PyObject* self, PyObject* args, PyObject* kwargs)
{
	TRACE_CALL(__func__);

	static char* kwlist[] = { "type", "buttons", "message", NULL };
	GtkMessageType msgType;
	GtkButtonsType btnType;
	gchar* message;

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "lls", kwlist, &msgType, &btnType, &message))
	{
		return PyLong_FromLong(-1);
	}

	python_wrapper_get_service()->show_dialog(msgType, btnType, message);

	return Py_None;
}

PyObject* python_wrapper_get_mainwindow_wrapper(PyObject* self, PyObject* args)
{
	TRACE_CALL(__func__);

	GtkWindow* result = python_wrapper_get_service()->get_window();

	if (!result)
	{
		return Py_None;
	}

	return (PyObject*)new_pywidget((GObject*)result);
}

static PyObject* remmina_protocol_plugin_signal_connection_closed_wrapper(PyObject* self, PyObject* args)
{
	TRACE_CALL(__func__);

	PyObject* pygp = NULL;
	if (!PyArg_ParseTuple(args, "O", &pygp) || !pygp)
	{
		g_printerr("Please provide the Remmina protocol widget instance!");
		return Py_None;
	}

	python_wrapper_get_service()->protocol_plugin_signal_connection_closed(((PyRemminaProtocolWidget*)pygp)->gp);
	return Py_None;
}

static PyObject* remmina_protocol_plugin_init_auth_wrapper(PyObject* module, PyObject* args, PyObject* kwds)
{
	TRACE_CALL(__func__);

	static gchar* keyword_list[] = { "widget", "flags", "title", "default_username", "default_password",
									 "default_domain", "password_prompt" };

	PyRemminaProtocolWidget* self;
	gint pflags = 0;
	gchar* title, * default_username, * default_password, * default_domain, * password_prompt;

	if (PyArg_ParseTupleAndKeywords(args, kwds, "Oisssss", keyword_list, &self, &pflags, &title, &default_username,
		&default_password, &default_domain, &password_prompt))
	{
		if (pflags != 0 && !(pflags & REMMINA_MESSAGE_PANEL_FLAG_USERNAME)
			&& !(pflags & REMMINA_MESSAGE_PANEL_FLAG_USERNAME_READONLY)
			&& !(pflags & REMMINA_MESSAGE_PANEL_FLAG_DOMAIN)
			&& !(pflags & REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD))
		{
			g_printerr("panel_auth(pflags, title, default_username, default_password, default_domain, password_prompt): "
					   "%d is not a known value for RemminaMessagePanelFlags!\n", pflags);
		}
		else
		{
			return Py_BuildValue("i", python_wrapper_get_service()->protocol_widget_panel_auth(self
				->gp, pflags, title, default_username, default_password, default_domain, password_prompt));
		}
	}
	else
	{
		g_printerr("panel_auth(pflags, title, default_username, default_password, default_domain, password_prompt): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* remmina_protocol_plugin_signal_connection_opened_wrapper(PyObject* self, PyObject* args)
{
	PyObject* pygp = NULL;
	if (!PyArg_ParseTuple(args, "O", &pygp) || !pygp)
	{
		g_printerr("Please provide the Remmina protocol widget instance!");
		return Py_None;
	}

	python_wrapper_get_service()->protocol_plugin_signal_connection_opened(((PyRemminaProtocolWidget*)pygp)->gp);
	return Py_None;
}
