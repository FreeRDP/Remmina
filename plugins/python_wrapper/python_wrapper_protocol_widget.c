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
 * @file python_wrapper_protocol_widget.c
 * @brief Implementation of the Protocol Widget API.
 * @author Mathias Winterhalter
 * @date 19.11.2020
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I N L U C E S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "python_wrapper_common.h"
#include "remmina/plugin.h"
#include "remmina/types.h"
#include "python_wrapper_remmina_file.h"
#include "python_wrapper_protocol_widget.h"
#include "python_wrapper_protocol.h"

// -- Python Type -> RemminaWidget

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// D E C L A R A T I O N S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PyObject* protocol_widget_get_viewport(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_width(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_width(PyRemminaProtocolWidget* self, PyObject* var_width);
static PyObject* protocol_widget_get_height(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_height(PyRemminaProtocolWidget* self, PyObject* var_height);
static PyObject* protocol_widget_get_current_scale_mode(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_expand(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_expand(PyRemminaProtocolWidget* self, PyObject* var_expand);
static PyObject* protocol_widget_has_error(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_error(PyRemminaProtocolWidget* self, PyObject* var_msg);
static PyObject* protocol_widget_is_closed(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_file(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_emit_signal(PyRemminaProtocolWidget* self, PyObject* var_signal);
static PyObject* protocol_widget_register_hostkey(PyRemminaProtocolWidget* self, PyObject* var_widget);
static PyObject* protocol_widget_start_direct_tunnel(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_start_reverse_tunnel(PyRemminaProtocolWidget* self, PyObject* var_local_port);
static PyObject* protocol_widget_start_xport_tunnel(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_display(PyRemminaProtocolWidget* self, PyObject* var_display);
static PyObject* protocol_widget_signal_connection_closed(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_signal_connection_opened(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_update_align(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_unlock_dynres(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_desktop_resize(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_panel_new_certificate(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_panel_changed_certificate(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_username(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_password(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_domain(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_savepassword(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_panel_authx509(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_cacert(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_cacrl(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_clientcert(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_clientkey(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_save_cred(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_panel_show_listen(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_panel_show_retry(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_panel_show(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_panel_hide(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_ssh_exec(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_chat_open(PyRemminaProtocolWidget* self, PyObject* var_name);
static PyObject* protocol_widget_chat_close(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_chat_receive(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_send_keys_signals(PyRemminaProtocolWidget* self, PyObject* args);

static struct PyMethodDef python_protocol_widget_type_methods[] =
	{{ "get_viewport", (PyCFunction)protocol_widget_get_viewport, METH_NOARGS, "" },
	 { "get_width", (PyCFunction)protocol_widget_get_width, METH_NOARGS, "" },
	 { "set_width", (PyCFunction)protocol_widget_set_width, METH_VARARGS, "" },
	 { "get_height", (PyCFunction)protocol_widget_get_height, METH_VARARGS, "" },
	 { "set_height", (PyCFunction)protocol_widget_set_height, METH_VARARGS, "" },
	 { "get_current_scale_mode", (PyCFunction)protocol_widget_get_current_scale_mode, METH_VARARGS, "" },
	 { "get_expand", (PyCFunction)protocol_widget_get_expand, METH_VARARGS, "" },
	 { "set_expand", (PyCFunction)protocol_widget_set_expand, METH_VARARGS, "" },
	 { "has_error", (PyCFunction)protocol_widget_has_error, METH_VARARGS, "" },
	 { "set_error", (PyCFunction)protocol_widget_set_error, METH_VARARGS, "" },
	 { "is_closed", (PyCFunction)protocol_widget_is_closed, METH_VARARGS, "" },
	 { "get_file", (PyCFunction)protocol_widget_get_file, METH_NOARGS, "" },
	 { "emit_signal", (PyCFunction)protocol_widget_emit_signal, METH_VARARGS, "" },
	 { "register_hostkey", (PyCFunction)protocol_widget_register_hostkey, METH_VARARGS, "" },
	 { "start_direct_tunnel", (PyCFunction)protocol_widget_start_direct_tunnel, METH_VARARGS | METH_KEYWORDS, "" },
	 { "start_reverse_tunnel", (PyCFunction)protocol_widget_start_reverse_tunnel, METH_VARARGS, "" },
	 { "start_xport_tunnel", (PyCFunction)protocol_widget_start_xport_tunnel, METH_VARARGS, "" },
	 { "set_display", (PyCFunction)protocol_widget_set_display, METH_VARARGS, "" },
	 { "signal_connection_closed", (PyCFunction)protocol_widget_signal_connection_closed, METH_VARARGS, "" },
	 { "signal_connection_opened", (PyCFunction)protocol_widget_signal_connection_opened, METH_VARARGS, "" },
	 { "update_align", (PyCFunction)protocol_widget_update_align, METH_VARARGS, "" },
	 { "unlock_dynres", (PyCFunction)protocol_widget_unlock_dynres, METH_VARARGS, "" },
	 { "desktop_resize", (PyCFunction)protocol_widget_desktop_resize, METH_VARARGS, "" },
	 { "panel_new_certificate", (PyCFunction)protocol_widget_panel_new_certificate, METH_VARARGS | METH_KEYWORDS, "" },
	 { "panel_changed_certificate", (PyCFunction)protocol_widget_panel_changed_certificate,
	   METH_VARARGS | METH_KEYWORDS, "" },
	 { "get_username", (PyCFunction)protocol_widget_get_username, METH_VARARGS, "" },
	 { "get_password", (PyCFunction)protocol_widget_get_password, METH_VARARGS, "" },
	 { "get_domain", (PyCFunction)protocol_widget_get_domain, METH_VARARGS, "" },
	 { "get_savepassword", (PyCFunction)protocol_widget_get_savepassword, METH_VARARGS, "" },
	 { "panel_authx509", (PyCFunction)protocol_widget_panel_authx509, METH_VARARGS, "" },
	 { "get_cacert", (PyCFunction)protocol_widget_get_cacert, METH_VARARGS, "" },
	 { "get_cacrl", (PyCFunction)protocol_widget_get_cacrl, METH_VARARGS, "" },
	 { "get_clientcert", (PyCFunction)protocol_widget_get_clientcert, METH_VARARGS, "" },
	 { "get_clientkey", (PyCFunction)protocol_widget_get_clientkey, METH_VARARGS, "" },
	 { "save_cred", (PyCFunction)protocol_widget_save_cred, METH_VARARGS, "" },
	 { "panel_show_listen", (PyCFunction)protocol_widget_panel_show_listen, METH_VARARGS, "" },
	 { "panel_show_retry", (PyCFunction)protocol_widget_panel_show_retry, METH_VARARGS, "" },
	 { "panel_show", (PyCFunction)protocol_widget_panel_show, METH_VARARGS, "" },
	 { "panel_hide", (PyCFunction)protocol_widget_panel_hide, METH_VARARGS, "" },
	 { "ssh_exec", (PyCFunction)protocol_widget_ssh_exec, METH_VARARGS | METH_KEYWORDS, "" },
	 { "chat_open", (PyCFunction)protocol_widget_chat_open, METH_VARARGS, "" },
	 { "chat_close", (PyCFunction)protocol_widget_chat_close, METH_VARARGS, "" },
	 { "chat_receive", (PyCFunction)protocol_widget_chat_receive, METH_VARARGS, "" },
	 { "send_keys_signals", (PyCFunction)protocol_widget_send_keys_signals, METH_VARARGS | METH_KEYWORDS, "" },
	 { NULL }};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static PyObject* python_protocol_feature_new(PyTypeObject* type, PyObject* kws, PyObject* args)
{
	TRACE_CALL(__func__);
	PyRemminaProtocolWidget* self;
	self = (PyRemminaProtocolWidget*)type->tp_alloc(type, 0);
	if (!self)
	{
		return NULL;
	}

	return (PyObject*)self;
}

static int python_protocol_feature_init(PyObject* self, PyObject* args, PyObject* kwds)
{
	return 0;
}

static PyTypeObject python_protocol_widget_type = {
	PyVarObject_HEAD_INIT(NULL, 0)
	.tp_name = "remmina.RemminaProtocolWidget",
	.tp_doc = "RemminaProtocolWidget",
	.tp_basicsize = sizeof(PyRemminaProtocolWidget),
	.tp_itemsize = 0,
	.tp_flags = Py_TPFLAGS_DEFAULT,
	.tp_new = python_protocol_feature_new,
	.tp_init = python_protocol_feature_init,
	.tp_methods = python_protocol_widget_type_methods
};

PyRemminaProtocolWidget* python_wrapper_protocol_widget_create(void)
{
	TRACE_CALL(__func__);

	PyRemminaProtocolWidget* result = PyObject_NEW(PyRemminaProtocolWidget, &python_protocol_widget_type);
	assert(result);

	PyErr_Print();
	Py_INCREF(result);
	result->gp = NULL;
	return result;
}

void python_wrapper_protocol_widget_init(void)
{
	init_pygobject();
}

void python_wrapper_protocol_widget_type_ready(void)
{
	TRACE_CALL(__func__);
	if (PyType_Ready(&python_protocol_widget_type) < 0)
	{
		g_printerr("Error initializing remmina.RemminaWidget!\n");
		PyErr_Print();
	}
}

static PyObject* protocol_widget_get_viewport(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	return (PyObject*)new_pywidget(G_OBJECT(python_wrapper_get_service()->protocol_widget_gtkviewport(self->gp)));
}

static PyObject* protocol_widget_get_width(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("i", python_wrapper_get_service()->protocol_widget_get_width(self->gp));
}

static PyObject* protocol_widget_set_width(PyRemminaProtocolWidget* self, PyObject* var_width)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!var_width)
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	if (PyLong_Check(var_width))
	{
		g_printerr("[%s:%d@%s]: Argument is not of type Long!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	gint width = (gint)PyLong_AsLong(var_width);
	python_wrapper_get_service()->protocol_widget_set_height(self->gp, width);

	return Py_None;
}

static PyObject* protocol_widget_get_height(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("i", python_wrapper_get_service()->protocol_widget_get_height(self->gp));
}

static PyObject* protocol_widget_set_height(PyRemminaProtocolWidget* self, PyObject* var_height)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!var_height)
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	if (PyLong_Check(var_height))
	{
		g_printerr("[%s:%d@%s]: Argument is not of type Long!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	gint height = (gint)PyLong_AsLong(var_height);
	python_wrapper_get_service()->protocol_widget_set_height(self->gp, height);

	return Py_None;
}

static PyObject* protocol_widget_get_current_scale_mode(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("i", python_wrapper_get_service()->protocol_widget_get_current_scale_mode(self->gp));
}

static PyObject* protocol_widget_get_expand(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", python_wrapper_get_service()->protocol_widget_get_expand(self->gp));
}

static PyObject* protocol_widget_set_expand(PyRemminaProtocolWidget* self, PyObject* var_expand)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!var_expand)
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	if (PyBool_Check(var_expand))
	{
		g_printerr("[%s:%d@%s]: Argument is not of type Boolean!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	python_wrapper_get_service()->protocol_widget_set_expand(self->gp, PyObject_IsTrue(var_expand));

	return Py_None;
}

static PyObject* protocol_widget_has_error(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", python_wrapper_get_service()->protocol_widget_has_error(self->gp));
}

static PyObject* protocol_widget_set_error(PyRemminaProtocolWidget* self, PyObject* var_msg)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!var_msg)
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	if (PyUnicode_Check(var_msg))
	{
		g_printerr("[%s:%d@%s]: Argument is not of type String!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	const gchar* msg = PyUnicode_AsUTF8(var_msg);
	python_wrapper_get_service()->protocol_widget_set_error(self->gp, msg);

	return Py_None;
}

static PyObject* protocol_widget_is_closed(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", python_wrapper_get_service()->protocol_widget_is_closed(self->gp));
}

static PyObject* protocol_widget_get_file(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	RemminaFile* file = python_wrapper_get_service()->protocol_widget_get_file(self->gp);
	return (PyObject*)python_wrapper_remmina_file_to_python(file);
}

static PyObject* protocol_widget_emit_signal(PyRemminaProtocolWidget* self, PyObject* var_signal)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!var_signal)
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	if (PyUnicode_Check(var_signal))
	{
		g_printerr("[%s:%d@%s]: Argument is not of type String!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	python_wrapper_get_service()->protocol_widget_set_error(self->gp, PyUnicode_AsUTF8(var_signal));

	return Py_None;
}

static PyObject* protocol_widget_register_hostkey(PyRemminaProtocolWidget* self, PyObject* var_widget)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!var_widget)
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	python_wrapper_get_service()->protocol_widget_register_hostkey(self->gp, get_pywidget(var_widget));

	return Py_None;
}

static PyObject* protocol_widget_start_direct_tunnel(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	gint default_port;
	gboolean port_plus;

	if (!args)
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
	}

	if (PyArg_ParseTuple(args, "ii", &default_port, &port_plus))
	{
		return Py_BuildValue("s", python_wrapper_get_service()->protocol_widget_start_direct_tunnel(self->gp, default_port, port_plus));
	}
	else
	{
		PyErr_Print();
		return NULL;
	}
	return Py_None;
}

static PyObject* protocol_widget_start_reverse_tunnel(PyRemminaProtocolWidget* self, PyObject* var_local_port)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!PyLong_Check(var_local_port))
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	if (!PyLong_Check(var_local_port))
	{
		g_printerr("[%s:%d@%s]: Argument is not of type Long!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	return Py_BuildValue("p", python_wrapper_get_service()->protocol_widget_start_reverse_tunnel(self->gp, (gint)PyLong_AsLong(var_local_port)));
}

static gboolean xport_tunnel_init(RemminaProtocolWidget* gp, gint remotedisplay, const gchar* server, gint port)
{
	TRACE_CALL(__func__);
	PyPlugin* plugin = python_wrapper_get_plugin_by_protocol_widget(gp);
	PyObject* result = PyObject_CallMethod(plugin->instance, "xport_tunnel_init", "Oisi", gp, remotedisplay, server, port);
	return PyObject_IsTrue(result);
}

static PyObject* protocol_widget_start_xport_tunnel(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", python_wrapper_get_service()->protocol_widget_start_xport_tunnel(self->gp, xport_tunnel_init));
}

static PyObject* protocol_widget_set_display(PyRemminaProtocolWidget* self, PyObject* var_display)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!var_display)
	{
		g_printerr("[%s:%d@%s]: Argument is null!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	if (!PyLong_Check(var_display))
	{
		g_printerr("[%s:%d@%s]: Argument is not of type Long!\n", __FILE__, __LINE__, __func__);
		return NULL;
	}

	python_wrapper_get_service()->protocol_widget_set_display(self->gp, (gint)PyLong_AsLong(var_display));

	return Py_None;
}

static PyObject* protocol_widget_signal_connection_closed(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_signal_connection_closed(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_signal_connection_opened(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_signal_connection_opened(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_update_align(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_update_align(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_unlock_dynres(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_unlock_dynres(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_desktop_resize(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_desktop_resize(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_panel_new_certificate(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gchar* subject, * issuer, * fingerprint;

	if (PyArg_ParseTuple(args, "sss", &subject, &issuer, &fingerprint))
	{
		python_wrapper_get_service()->protocol_widget_panel_new_certificate(self->gp, subject, issuer, fingerprint);
	}
	else
	{
		PyErr_Print();
		return NULL;
	}
	return Py_None;
}

static PyObject* protocol_widget_panel_changed_certificate(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gchar* subject, * issuer, * new_fingerprint, * old_fingerprint;

	if (PyArg_ParseTuple(args, "sss", &subject, &issuer, &new_fingerprint, &old_fingerprint))
	{
		python_wrapper_get_service()->protocol_widget_panel_changed_certificate(self->gp, subject, issuer, new_fingerprint, old_fingerprint);
	}
	else
	{
		PyErr_Print();
		return NULL;
	}
	return Py_None;
}

static PyObject* protocol_widget_get_username(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", python_wrapper_get_service()->protocol_widget_get_username(self->gp));
}

static PyObject* protocol_widget_get_password(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", python_wrapper_get_service()->protocol_widget_get_password(self->gp));
}

static PyObject* protocol_widget_get_domain(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", python_wrapper_get_service()->protocol_widget_get_domain(self->gp));
}

static PyObject* protocol_widget_get_savepassword(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", python_wrapper_get_service()->protocol_widget_get_savepassword(self->gp));
}

static PyObject* protocol_widget_panel_authx509(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("i", python_wrapper_get_service()->protocol_widget_panel_authx509(self->gp));
}

static PyObject* protocol_widget_get_cacert(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", python_wrapper_get_service()->protocol_widget_get_cacert(self->gp));
}

static PyObject* protocol_widget_get_cacrl(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", python_wrapper_get_service()->protocol_widget_get_cacrl(self->gp));
}

static PyObject* protocol_widget_get_clientcert(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", python_wrapper_get_service()->protocol_widget_get_clientcert(self->gp));
}

static PyObject* protocol_widget_get_clientkey(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", python_wrapper_get_service()->protocol_widget_get_clientkey(self->gp));
}

static PyObject* protocol_widget_save_cred(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_save_cred(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_panel_show_listen(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gint port = 0;

	if (PyArg_ParseTuple(args, "i", &port))
	{
		python_wrapper_get_service()->protocol_widget_panel_show_listen(self->gp, port);
	}
	else
	{
		PyErr_Print();
		return NULL;
	}
	return Py_None;
}

static PyObject* protocol_widget_panel_show_retry(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_panel_show_retry(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_panel_show(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_panel_show(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_panel_hide(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_panel_hide(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_ssh_exec(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gboolean wait;
	gchar* cmd;

	if (PyArg_ParseTuple(args, "ps", &wait, &cmd))
	{
		python_wrapper_get_service()->protocol_widget_ssh_exec(self->gp, wait, cmd);
	}
	else
	{
		PyErr_Print();
		return NULL;
	}
	return Py_None;
}

static void _on_send_callback_wrapper(RemminaProtocolWidget* gp, const gchar* text)
{
	PyPlugin* plugin = python_wrapper_get_plugin_by_protocol_widget(gp);
	PyObject_CallMethod(plugin->instance, "on_send", "Os", gp, text);
}

static void _on_destroy_callback_wrapper(RemminaProtocolWidget* gp)
{
	PyPlugin* plugin = python_wrapper_get_plugin_by_protocol_widget(gp);
	PyObject_CallMethod(plugin->instance, "on_destroy", "O", gp);
}

static PyObject* protocol_widget_chat_open(PyRemminaProtocolWidget* self, PyObject* var_name)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (!PyUnicode_Check(var_name))
	{
		g_printerr("[%s:%d@%s]: Argument is not of type String!\n", __FILE__, __LINE__, __func__);
	}

	python_wrapper_get_service()->protocol_widget_chat_open(self->gp,
		PyUnicode_AsUTF8(var_name),
		_on_send_callback_wrapper,
		_on_destroy_callback_wrapper);

	return Py_None;
}

static PyObject* protocol_widget_chat_close(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	python_wrapper_get_service()->protocol_widget_panel_hide(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_chat_receive(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gchar* text;

	if (PyArg_ParseTuple(args, "s", &text))
	{
		python_wrapper_get_service()->protocol_widget_chat_receive(self->gp, text);
	}
	else
	{
		PyErr_Print();
		return NULL;
	}

	return Py_None;
}

static PyObject* protocol_widget_send_keys_signals(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	guint* keyvals;
	int length;
	GdkEventType event_type;
	PyObject* widget;

	if (PyArg_ParseTuple(args, "Osii", &widget, &keyvals, &length, &event_type) && widget && keyvals)
	{
		if (event_type < GDK_NOTHING || event_type >= GDK_EVENT_LAST)
		{
			g_printerr("[%s:%d@%s]: %d is not a known value for GdkEventType!\n", __FILE__, __LINE__, __func__, event_type);
			return NULL;
		}
		else
		{
			python_wrapper_get_service()->protocol_widget_send_keys_signals((GtkWidget*)widget, keyvals, length, event_type);
		}
	}
	else
	{
		PyErr_Print();
		return NULL;
	}

	return Py_None;
}
