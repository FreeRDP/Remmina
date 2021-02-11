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
 * @file remmina_plugin_python_protocol_widget.c
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

#include <glib.h>
#include <gtk/gtk.h>
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>

#include "config.h"
#include "pygobject.h"
#include "remmina_plugin_manager.h"
#include "remmina/plugin.h"
#include "remmina_protocol_widget.h"
#include "remmina_file.h"
#include "remmina_plugin_python_remmina.h"
#include "remmina_plugin_python_remmina_file.h"

#include "remmina_plugin_python_protocol_widget.h"

// -- Python Type -> RemminaWidget

static PyObject* protocol_widget_get_viewport(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_width(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_width(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_height(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_height(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_current_scale_mode(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_expand(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_expand(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_has_error(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_error(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_is_closed(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_get_file(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_emit_signal(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_register_hostkey(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_start_direct_tunnel(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_start_reverse_tunnel(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_start_xport_tunnel(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_set_display(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_signal_connection_closed(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_signal_connection_opened(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_update_align(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_unlock_dynres(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_desktop_resize(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_panel_auth(PyRemminaProtocolWidget* self, PyObject* args);
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
static PyObject* protocol_widget_chat_open(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_chat_close(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_chat_receive(PyRemminaProtocolWidget* self, PyObject* args);
static PyObject* protocol_widget_send_keys_signals(PyRemminaProtocolWidget* self, PyObject* args);

static PyMethodDef python_protocol_widget_type_methods[] = {
	{"get_viewport", (PyCFunction)protocol_widget_get_viewport, METH_NOARGS, "" },
	{"get_width", (PyCFunction)protocol_widget_get_width, METH_NOARGS, "" },
	{"set_width", (PyCFunction)protocol_widget_set_width, METH_VARARGS, "" },
	{"get_height", (PyCFunction)protocol_widget_get_height, METH_VARARGS, "" },
	{"set_height", (PyCFunction)protocol_widget_set_height, METH_VARARGS, "" },
	{"get_current_scale_mode", (PyCFunction)protocol_widget_get_current_scale_mode, METH_VARARGS, "" },
	{"get_expand", (PyCFunction)protocol_widget_get_expand, METH_VARARGS, "" },
	{"set_expand", (PyCFunction)protocol_widget_set_expand, METH_VARARGS, "" },
	{"has_error", (PyCFunction)protocol_widget_has_error, METH_VARARGS, "" },
	{"set_error", (PyCFunction)protocol_widget_set_error, METH_VARARGS, "" },
	{"is_closed", (PyCFunction)protocol_widget_is_closed, METH_VARARGS, "" },
	{"get_file", (PyCFunction)protocol_widget_get_file, METH_VARARGS, "" },
	{"emit_signal", (PyCFunction)protocol_widget_emit_signal, METH_VARARGS, "" },
	{"register_hostkey", (PyCFunction)protocol_widget_register_hostkey, METH_VARARGS, "" },
	{"start_direct_tunnel", (PyCFunctionWithKeywords)protocol_widget_start_direct_tunnel, METH_VARARGS | METH_KEYWORDS, "" },
	{"start_reverse_tunnel", (PyCFunction)protocol_widget_start_reverse_tunnel, METH_VARARGS, "" },
	{"start_xport_tunnel", (PyCFunction)protocol_widget_start_xport_tunnel, METH_VARARGS, "" },
	{"set_display", (PyCFunction)protocol_widget_set_display, METH_VARARGS, "" },
	{"signal_connection_closed", (PyCFunction)protocol_widget_signal_connection_closed, METH_VARARGS, "" },
    {"signal_connection_opened", (PyCFunction)protocol_widget_signal_connection_opened, METH_VARARGS, "" },
    {"update_align", (PyCFunction)protocol_widget_update_align, METH_VARARGS, "" },
    {"unlock_dynres", (PyCFunction)protocol_widget_unlock_dynres, METH_VARARGS, "" },
    {"desktop_resize", (PyCFunction)protocol_widget_desktop_resize, METH_VARARGS, "" },
	{"panel_auth", (PyCFunction)protocol_widget_panel_auth, METH_VARARGS | METH_KEYWORDS, "" },
	{"panel_new_certificate", (PyCFunction)protocol_widget_panel_new_certificate, METH_VARARGS | METH_KEYWORDS, "" },
	{"panel_changed_certificate", (PyCFunction)protocol_widget_panel_changed_certificate, METH_VARARGS | METH_KEYWORDS, "" },
	{"get_username", (PyCFunction)protocol_widget_get_username, METH_VARARGS, "" },
	{"get_password", (PyCFunction)protocol_widget_get_password, METH_VARARGS, "" },
	{"get_domain", (PyCFunction)protocol_widget_get_domain, METH_VARARGS, "" },
	{"get_savepassword", (PyCFunction)protocol_widget_get_savepassword, METH_VARARGS, "" },
	{"panel_authx509", (PyCFunction)protocol_widget_panel_authx509, METH_VARARGS, "" },
	{"get_cacert", (PyCFunction)protocol_widget_get_cacert, METH_VARARGS, "" },
	{"get_cacrl", (PyCFunction)protocol_widget_get_cacrl, METH_VARARGS, "" },
	{"get_clientcert", (PyCFunction)protocol_widget_get_clientcert, METH_VARARGS, "" },
	{"get_clientkey", (PyCFunction)protocol_widget_get_clientkey, METH_VARARGS, "" },
	{"save_cred", (PyCFunction)protocol_widget_save_cred, METH_VARARGS, "" },
	{"panel_show_listen", (PyCFunction)protocol_widget_panel_show_listen, METH_VARARGS, "" },
	{"panel_show_retry", (PyCFunction)protocol_widget_panel_show_retry, METH_VARARGS, "" },
	{"panel_show", (PyCFunction)protocol_widget_panel_show, METH_VARARGS, "" },
	{"panel_hide", (PyCFunction)protocol_widget_panel_hide, METH_VARARGS, "" },
	{"ssh_exec", (PyCFunction)protocol_widget_ssh_exec, METH_VARARGS | METH_KEYWORDS, "" },
	{"chat_open", (PyCFunction)protocol_widget_chat_open, METH_VARARGS, "" },
	{"chat_close", (PyCFunction)protocol_widget_chat_close, METH_VARARGS, "" },
	{"chat_receive", (PyCFunction)protocol_widget_chat_receive, METH_VARARGS, "" },
	{"send_keys_signals", (PyCFunction)protocol_widget_send_keys_signals, METH_VARARGS | METH_KEYWORDS, "" }
};

static PyTypeObject python_protocol_widget_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "remmina.RemminaWidget",
    .tp_doc = "Remmina protocol widget",
    .tp_basicsize = sizeof(PyRemminaProtocolWidget),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = python_protocol_widget_type_methods
};

typedef struct {
	PyObject_HEAD
	PyDictObject* settings;
	PyDictObject* spsettings;
} PyRemminaFile;

#define SELF_CHECK() if (!self) { \
		g_printerr("[%s:%d]: self is null!\n", __FILE__, __LINE__); \
		PyErr_SetString(PyExc_RuntimeError, "Method is not called from an instance (self is null)!"); \
		return NULL; \
	}

void remmina_plugin_python_protocol_widget_init(void) {
  pygobject_init(-1, -1, -1);
}

static PyObject* protocol_widget_get_viewport(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	return pygobject_new(G_OBJECT(remmina_protocol_widget_gtkviewport(self->gp)));
}

static PyObject* protocol_widget_get_width(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("i", remmina_protocol_widget_get_width(self->gp));
}

static PyObject* protocol_widget_set_width(PyRemminaProtocolWidget* self, PyObject* var_width)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_width && PyLong_Check(var_width)) {
		gint width = (gint)PyLong_AsLong(var_width);
		remmina_protocol_widget_set_height(self->gp, width);
	} else {
		g_printerr("set_width(val): Error parsing arguments!\n");
		PyErr_Print();
	}

	return Py_None;
}

static PyObject* protocol_widget_get_height(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("i", remmina_protocol_widget_get_height(self->gp));
}

static PyObject* protocol_widget_set_height(PyRemminaProtocolWidget* self, PyObject* var_height)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_height && PyLong_Check(var_height)) {
		gint height = (gint)PyLong_AsLong(var_height);
		remmina_protocol_widget_set_height(self->gp, height);
	} else {
		g_printerr("set_height(val): Error parsing arguments!\n");
		PyErr_Print();
	}

	return Py_None;
}

static PyObject* protocol_widget_get_current_scale_mode(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("i", remmina_protocol_widget_get_current_scale_mode(self->gp));
}

static PyObject* protocol_widget_get_expand(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", remmina_protocol_widget_get_expand(self->gp));
}

static PyObject* protocol_widget_set_expand(PyRemminaProtocolWidget* self, PyObject* var_expand)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_expand && PyBool_Check(var_expand)) {
		remmina_protocol_widget_set_expand(self->gp, PyObject_IsTrue(var_expand));
	} else {
		g_printerr("set_expand(val): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_has_error(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", remmina_protocol_widget_has_error(self->gp));
}

static PyObject* protocol_widget_set_error(PyRemminaProtocolWidget* self, PyObject* var_msg)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_msg && PyUnicode_Check(var_msg)) {
		gchar* msg = PyUnicode_AsUTF8(var_msg);
		remmina_protocol_widget_set_error(self->gp, msg);
	} else {
		g_printerr("set_error(msg): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_is_closed(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", remmina_protocol_widget_is_closed(self->gp));
}

static PyObject* protocol_widget_get_file(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	RemminaFile* file = remmina_protocol_widget_get_file(self->gp);
	return remmina_plugin_python_remmina_file_to_python(file);
}

static PyObject* protocol_widget_emit_signal(PyRemminaProtocolWidget* self, PyObject* var_signal)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_signal && PyUnicode_Check(var_signal)) {
		remmina_protocol_widget_set_error(self->gp, PyUnicode_AsUTF8(var_signal));
	} else {
		g_printerr("emit_signal(signal): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_register_hostkey(PyRemminaProtocolWidget* self, PyObject* var_widget)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_widget) {
		remmina_protocol_widget_register_hostkey(self->gp, pygobject_get(var_widget));
	} else {
		g_printerr("register_hostkey(widget): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_start_direct_tunnel(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	gint default_port;
	gboolean port_plus;

	if (args && PyArg_ParseTuple(args, "ii", &default_port, &port_plus)) {
		return Py_BuildValue("s", remmina_protocol_widget_start_direct_tunnel(self->gp, default_port, port_plus));
	} else {
		g_printerr("start_direct_tunnel(default_port, port_plus): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_start_reverse_tunnel(PyRemminaProtocolWidget* self, PyObject* var_local_port)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_local_port && PyLong_Check(var_local_port)) {
		return Py_BuildValue("p", remmina_protocol_widget_start_reverse_tunnel(self->gp, (gint)PyLong_AsLong(var_local_port)));
	} else {
		g_printerr("start_direct_tunnel(local_port): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static gboolean xport_tunnel_init(RemminaProtocolWidget *gp, gint remotedisplay, const gchar *server, gint port)
{
	PyPlugin* plugin = remmina_plugin_python_module_get_plugin(gp);
	PyObject* result = PyObject_CallMethod(plugin, "xport_tunnel_init", "Oisi", gp, remotedisplay, server, port);
	return PyObject_IsTrue(result);
}

static PyObject* protocol_widget_start_xport_tunnel(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", remmina_protocol_widget_start_xport_tunnel(self->gp, xport_tunnel_init));
}

static PyObject* protocol_widget_set_display(PyRemminaProtocolWidget* self, PyObject* var_display)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_display && PyLong_Check(var_display)) {
		remmina_protocol_widget_set_display(self->gp, (gint)PyLong_AsLong(var_display));
	} else {
		g_printerr("set_display(display): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_signal_connection_closed(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_signal_connection_closed(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_signal_connection_opened(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_signal_connection_opened(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_update_align(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_update_align(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_unlock_dynres(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_unlock_dynres(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_desktop_resize(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_desktop_resize(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_panel_auth(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	gint pflags = 0;
	gchar* title, default_username, default_password, default_domain, password_prompt;

	if (PyArg_ParseTuple(args, "isssss", &pflags, &title, &default_username, &default_password, &default_domain, &password_prompt)) {
		if (pflags != REMMINA_MESSAGE_PANEL_FLAG_USERNAME
			&& pflags != REMMINA_MESSAGE_PANEL_FLAG_USERNAME_READONLY
			&& pflags != REMMINA_MESSAGE_PANEL_FLAG_DOMAIN
			&& pflags != REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD) {
				g_printerr("panel_auth(pflags, title, default_username, default_password, default_domain, password_prompt): "
						   "%d is not a known value for RemminaMessagePanelFlags!\n", pflags);
		} else {
			remmina_protocol_widget_panel_auth(self->gp, pflags, title, default_username, default_password, default_domain, password_prompt);
		}
	} else {
		g_printerr("panel_auth(pflags, title, default_username, default_password, default_domain, password_prompt): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_panel_new_certificate(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gchar* subject, issuer, fingerprint;

	if (PyArg_ParseTuple(args, "sss", &subject, &issuer, &fingerprint)) {
		remmina_protocol_widget_panel_new_certificate(self->gp, subject, issuer, fingerprint);
	} else {
		g_printerr("panel_new_certificate(subject, issuer, fingerprint): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_panel_changed_certificate(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gchar* subject, issuer, new_fingerprint, old_fingerprint;

	if (PyArg_ParseTuple(args, "sss", &subject, &issuer, &new_fingerprint, &old_fingerprint)) {
		remmina_protocol_widget_panel_changed_certificate(self->gp, subject, issuer, new_fingerprint, old_fingerprint);
	} else {
		g_printerr("panel_changed_certificate(subject, issuer, new_fingerprint, old_fingerprint): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_get_username(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", remmina_protocol_widget_get_username(self->gp));
}

static PyObject* protocol_widget_get_password(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", remmina_protocol_widget_get_password(self->gp));
}

static PyObject* protocol_widget_get_domain(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", remmina_protocol_widget_get_domain(self->gp));
}

static PyObject* protocol_widget_get_savepassword(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("p", remmina_protocol_widget_get_savepassword(self->gp));
}

static PyObject* protocol_widget_panel_authx509(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("i", remmina_protocol_widget_panel_authx509(self->gp));
}

static PyObject* protocol_widget_get_cacert(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", remmina_protocol_widget_get_cacert(self->gp));
}

static PyObject* protocol_widget_get_cacrl(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", remmina_protocol_widget_get_cacrl(self->gp));
}

static PyObject* protocol_widget_get_clientcert(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", remmina_protocol_widget_get_clientcert(self->gp));
}

static PyObject* protocol_widget_get_clientkey(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	return Py_BuildValue("s", remmina_protocol_widget_get_clientkey(self->gp));
}

static PyObject* protocol_widget_save_cred(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_save_cred(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_panel_show_listen(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gint port = 0;

	if (PyArg_ParseTuple(args, "i", &port)) {
		remmina_protocol_widget_panel_show_listen(self->gp, port);
	} else {
		g_printerr("panel_show_listen(port): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyObject* protocol_widget_panel_show_retry(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_panel_show_retry(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_panel_show(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_panel_show(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_panel_hide(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_panel_hide(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_ssh_exec(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gboolean wait;
	gchar* cmd;

	if (PyArg_ParseTuple(args, "ps", &wait, &cmd)) {
		remmina_protocol_widget_ssh_exec(self->gp, wait, cmd);
	} else {
		g_printerr("ssh_exec(wait, cmd): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static gboolean _on_send_callback_wrapper(RemminaProtocolWidget *gp, const gchar *text)
{
	PyPlugin* plugin = remmina_plugin_python_module_get_plugin(gp);
	PyObject* result = PyObject_CallMethod(plugin, "on_send", "Os", gp, text);
	return PyObject_IsTrue(result);
}

static gboolean _on_destroy_callback_wrapper(RemminaProtocolWidget *gp)
{
	PyPlugin* plugin = remmina_plugin_python_module_get_plugin(gp);
	PyObject* result = PyObject_CallMethod(plugin, "on_destroy", "O", gp);
	return PyObject_IsTrue(result);
}

static PyObject* protocol_widget_chat_open(PyRemminaProtocolWidget* self, PyObject* var_name)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	if (var_name, PyUnicode_Check(var_name)) {
		remmina_protocol_widget_chat_open(self->gp, PyUnicode_AsUTF8(var_name), _on_send_callback_wrapper, _on_destroy_callback_wrapper);
	} else {
		g_printerr("chat_open(name): Error parsing arguments!\n");
		PyErr_Print();
	}

	return Py_None;
}

static PyObject* protocol_widget_chat_close(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();

	remmina_protocol_widget_panel_hide(self->gp);
	return Py_None;
}

static PyObject* protocol_widget_chat_receive(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gchar* text;

	if (PyArg_ParseTuple(args, "s", &text)) {
		remmina_protocol_widget_chat_receive(self->gp, text);
	} else {
		g_printerr("chat_receive(text): Error parsing arguments!\n");
		PyErr_Print();
	}

	return Py_None;
}

static PyObject* protocol_widget_send_keys_signals(PyRemminaProtocolWidget* self, PyObject* args)
{
	TRACE_CALL(__func__);
	SELF_CHECK();
	gchar* keyvals;
	int length;
	GdkEventType event_type;

	if (PyArg_ParseTuple(args, "sii", &keyvals, &length, &event_type)) {
		if (event_type < GDK_NOTHING || event_type >= GDK_EVENT_LAST) {
			g_printerr("send_keys_signals(keyvals, length, event_type): "
						   "%d is not a known value for GdkEventType!\n", event_type);
		} else {
			remmina_protocol_widget_send_keys_signals(self->gp, keyvals, length, event_type);
		}
	} else {
		g_printerr("send_keys_signals(keyvals): Error parsing arguments!\n");
		PyErr_Print();
	}

	return Py_None;
}

