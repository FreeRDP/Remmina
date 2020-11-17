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
#include "remmina_plugin_python_remmina_file.h"

#include "remmina_plugin_python_protocol_widget.h"

static PyOject* protocol_widget_get_viewport(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_width(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_set_width(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_height(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_set_height(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_current_scale_mode(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_expand(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_set_expand(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_has_error(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_set_error(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_is_closed(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_file(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_emit_signal(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_register_hostkey(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_start_direct_tunnel(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_start_reverse_tunnel(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_start_xport_tunnel(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_set_display(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_signal_connection_closed(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_signal_connection_opened(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_update_align(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_unlock_dynres(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_desktop_resize(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_panel_auth(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_panel_new_certificate(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_panel_changed_certificate(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_username(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_password(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_domain(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_savepassword(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_panel_authx509(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_cacert(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_cacrl(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_clientcert(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_get_clientkey(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_save_cred(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_panel_show_listen(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_panel_show_retry(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_panel_show(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_panel_hide(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_ssh_exec(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_chat_open(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_chat_close(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_chat_receive(PyObject* self, PyRemminaProtocolWidget* gp);
static PyOject* protocol_widget_send_keys_signals(PyObject* self, PyRemminaProtocolWidget* gp);

static PyMethodDef python_protocol_widget_type_methods[] = {
	{"get_viewport", (PyCFunction)protocol_widget_get_viewport},
	{"get_width", (PyCFunction)protocol_widget_get_width},
	{"set_width", (PyCFunction)protocol_widget_set_width},
	{"get_height", (PyCFunction)protocol_widget_get_height},
	{"set_height", (PyCFunction)protocol_widget_set_height},
	{"get_current_scale_mode", (PyCFunction)protocol_widget_get_current_scale_mode},
	{"get_expand", (PyCFunction)protocol_widget_get_expand},
	{"set_expand", (PyCFunction)protocol_widget_set_expand},
	{"has_error", (PyCFunction)protocol_widget_has_error},
	{"set_error", (PyCFunction)protocol_widget_set_error},
	{"is_closed", (PyCFunction)protocol_widget_is_closed},
	{"get_file", (PyCFunction)protocol_widget_get_file},
	{"emit_signal", (PyCFunction)protocol_widget_emit_signal},
	{"register_hostkey", (PyCFunction)protocol_widget_register_hostkey},
	{"start_direct_tunnel", (PyCFunction)protocol_widget_start_direct_tunnel},
	{"start_reverse_tunnel", (PyCFunction)protocol_widget_start_reverse_tunnel},
	{"start_xport_tunnel", (PyCFunction)protocol_widget_start_xport_tunnel},
	{"set_display", (PyCFunction)protocol_widget_set_display},
	{"signal_connection_closed", (PyCFunction)protocol_widget_signal_connection_closed},
    {"signal_connection_opened", (PyCFunction)protocol_widget_signal_connection_opened},
    {"update_align", (PyCFunction)protocol_widget_update_align},
    {"unlock_dynres", (PyCFunction)protocol_widget_unlock_dynres},
    {"desktop_resize", (PyCFunction)protocol_widget_desktop_resize},
	{"panel_auth", (PyCFunction)protocol_widget_panel_auth},
	{"panel_new_certificate", (PyCFunction)protocol_widget_panel_new_certificate},
	{"panel_changed_certificate", (PyCFunction)protocol_widget_panel_changed_certificate},
	{"get_username", (PyCFunction)protocol_widget_get_username},
	{"get_password", (PyCFunction)protocol_widget_get_password},
	{"get_domain", (PyCFunction)protocol_widget_get_domain},
	{"get_savepassword", (PyCFunction)protocol_widget_get_savepassword},
	{"panel_authx509", (PyCFunction)protocol_widget_panel_authx509},
	{"get_cacert", (PyCFunction)protocol_widget_get_cacert},
	{"get_cacrl", (PyCFunction)protocol_widget_get_cacrl},
	{"get_clientcert", (PyCFunction)protocol_widget_get_clientcert},
	{"get_clientkey", (PyCFunction)protocol_widget_get_clientkey},
	{"save_cred", (PyCFunction)protocol_widget_save_cred},
	{"panel_show_listen", (PyCFunction)protocol_widget_panel_show_listen},
	{"panel_show_retry", (PyCFunction)protocol_widget_panel_show_retry},
	{"panel_show", (PyCFunction)protocol_widget_panel_show},
	{"panel_hide", (PyCFunction)protocol_widget_panel_hide},
	{"ssh_exec", (PyCFunction)protocol_widget_ssh_exec},
	{"chat_open", (PyCFunction)protocol_widget_chat_open},
	{"chat_close", (PyCFunction)protocol_widget_chat_close},
	{"chat_receive", (PyCFunction)protocol_widget_chat_receive},
	{"send_keys_signals", (PyCFunction)protocol_widget_send_keys_signals}
};

static PyTypeObject python_protocol_widget_type = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "remmina.RemminaWidget",
    .tp_doc = "Remmina protocol widget",
    .tp_basicsize = sizeof(PyRemminaProtocolWidget),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_new = PyType_GenericNew,
    .tp_methods = python_protocol_widget_type_methods,
};

static PyMemberDef python_remmina_file_type_member = {

};

typedef struct {
	PyObject_HEAD
	
	PyDictObject* settings;
	PyDictObject* spsettings;
	
} PyRemminaFile;

static PyTypeObject 

static PyOject* protocol_widget_get_viewport(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return pygobject_new(G_OBJECT(remmina_protocol_widget_gtkviewport(gp->gp)));
}

static PyOject* protocol_widget_get_width(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_BuildValue("i", remmina_protocol_widget_get_width(gp->gp));
}

static PyOject* protocol_widget_set_width(PyObject* self, PyObject* args)
{
	PyRemminaProtocolWidget* gp;
	gint val;
	if (PyArgParseTuple(args, "Oi", &gp, &val)) {
		remmina_protocol_widget_set_width(gp->gp, val);
	} else {
		g_printerr("set_width(gp, val): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_get_height(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_BuildValue("i", remmina_protocol_widget_get_height(gp->gp));
}

static PyOject* protocol_widget_set_height(PyObject* self, PyObject* args)
{
	PyRemminaProtocolWidget* gp;
	gint val;
	if (PyArgParseTuple(args, "Oi", &gp, &val)) {
		remmina_protocol_widget_set_height(gp->gp, val);
	} else {
		g_printerr("set_height(gp, val): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_get_current_scale_mode(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_BuildValue("i", remmina_protocol_widget_get_current_scale_mode(gp->gp));
}

static PyOject* protocol_widget_get_expand(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_BuildValue("p", remmina_protocol_widget_get_expand(gp->gp));
}

static PyOject* protocol_widget_set_expand(PyObject* self, PyObject* args)
{
	PyRemminaProtocolWidget* gp;
	gboolean val;
	if (PyArgParseTuple(args, "Op", &gp, &val)) {
		remmina_protocol_widget_set_expand(gp->gp, val);
	} else {
		g_printerr("set_expand(gp, val): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_has_error(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_BuildValue("p", remmina_protocol_widget_has_error(gp->gp));
}

static PyOject* protocol_widget_set_error(PyObject* self, PyObject* args)
{
	PyRemminaProtocolWidget* gp;
	gchar* msg;
	if (PyArgParseTuple(args, "Os", &gp, &msg)) {
		remmina_protocol_widget_set_error(gp->gp, msg);
	} else {
		g_printerr("set_error(gp, msg): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_is_closed(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_BuildValue("p", remmina_protocol_widget_is_closed(gp->gp));
}

static PyOject* protocol_widget_get_file(PyObject* self, PyRemminaProtocolWidget* gp)
{
	RemminaFile* file = remmina_protocol_widget_get_file(gp->gp);
	return remmina_plugin_python_remmina_file(file);
}

static PyOject* protocol_widget_emit_signal(PyObject* self, PyObject* args)
{
	PyRemminaProtocolWidget* gp;
	gchar* signal;
	if (PyArgParseTuple(args, "Os", &gp, &signal)) {
		remmina_protocol_widget_set_error(gp->gp, signal);
	} else {
		g_printerr("emit_signal(gp, signal): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_register_hostkey(PyObject* self, PyObject* args)
{
	PyRemminaProtocolWidget* gp;
	PyGObject* widget;
	
	if (PyArgParseTuple(args, "OO", &gp, &widget)) {
		remmina_protocol_widget_register_hostkey(gp->gp, pygobject_get(widget));
	} else {
		g_printerr("register_hostkey(gp, widget): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_start_direct_tunnel(PyObject* self, PyRemminaProtocolWidget* gp)
{
	PyRemminaProtocolWidget* gp;
	gint default_port;
	gboolean port_plus;
	
	if (PyArgParseTuple(args, "Oip", &gp, &default_port, &port_plus)) {
		return Py_BuildValue("s", remmina_protocol_widget_start_direct_tunnel(gp->gp, default_port, port_plus));
	} else {
		g_printerr("start_direct_tunnel(gp, default_port, port_plus): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_start_reverse_tunnel(PyObject* self, PyRemminaProtocolWidget* gp)
{
	PyRemminaProtocolWidget* gp;
	gint local_port;
	
	if (PyArgParseTuple(args, "Oip", &gp, &local_port)) {
		return Py_BuildValue("s", remmina_protocol_widget_start_reverse_tunnel(gp->gp, local_port));
	} else {
		g_printerr("start_direct_tunnel(gp, local_port): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static gboolean xport_tunnel_init(RemminaProtocolWidget *gp, gint remotedisplay, const gchar *server, gint port)
{
	PyPlugin* plugin = remmina_plugin_python_module_get_plugin(gp->gp);
	PyObject* result = PyObject_CallMethod(plugin, "xport_tunnel_init", "Oisi", gp, remotedisplay, server, port));
	return PyObject_IsTrue(result);
}

static PyOject* protocol_widget_start_xport_tunnel(PyObject* self, PyRemminaProtocolWidget* gp)
{
	PyRemminaProtocolWidget* gp;
	gint local_port;
	
	if (PyArgParseTuple(args, "O", &gp, &local_port)) {
		return Py_BuildValue("s", remmina_protocol_widget_start_xport_tunnel(gp->gp, xport_tunnel_init));
	} else {
		g_printerr("start_direct_tunnel(gp, local_port): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_set_display(PyObject* self, PyObject* args)
{
	PyRemminaProtocolWidget* gp;
	gint val;
	if (PyArgParseTuple(args, "Oi", &gp, &val)) {
		remmina_protocol_widget_set_display(gp->gp, val);
	} else {
		g_printerr("get_width(gp, val): Error parsing arguments!\n");
		PyErr_Print();
	}
	return Py_None;
}

static PyOject* protocol_widget_signal_connection_closed(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_signal_connection_opened(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_update_align(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_unlock_dynres(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_desktop_resize(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_panel_auth(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_panel_new_certificate(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_panel_changed_certificate(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_get_username(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_get_password(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_get_domain(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_get_savepassword(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_panel_authx509(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_get_cacert(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_get_cacrl(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_get_clientcert(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_get_clientkey(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_save_cred(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_panel_show_listen(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_panel_show_retry(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_panel_show(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_panel_hide(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_ssh_exec(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_chat_open(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_chat_close(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_chat_receive(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

static PyOject* protocol_widget_send_keys_signals(PyObject* self, PyRemminaProtocolWidget* gp)
{
	return Py_None;
}

