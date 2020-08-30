#include "config.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include <glib.h>
#include <gtk/gtk.h>
#include "remmina/plugin.h"
#include "remmina_plugin_python_plugin.h"

static const gchar* STR_REMMINA_PLUGIN_TYPE_PROTOCOL = "remmina.plugin_type.protocol";
static const gchar* STR_REMMINA_PLUGIN_TYPE_ENTRY = "remmina.plugin_type.entry";
static const gchar* STR_REMMINA_PLUGIN_TYPE_FILE = "remmina.plugin_type.file";
static const gchar* STR_REMMINA_PLUGIN_TYPE_TOOL = "remmina.plugin_type.tool";
static const gchar* STR_REMMINA_PLUGIN_TYPE_PREF = "remmina.plugin_type.pref";
static const gchar* STR_REMMINA_PLUGIN_TYPE_SECRET = "remmina.plugin_type.secret";


static int
remmina_remmina_plugin_init(RemminaPluginPython *self, PyObject *args, PyObject *kwds);

static PyObject *
remmina_remmina_plugin_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyMemberDef RemminaPluginPython_members[] = {
    {"name", T_OBJECT_EX, offsetof(RemminaPluginPython, name), 0,
     "plugin name"},
    {"description", T_OBJECT_EX, offsetof(RemminaPluginPython, description), 0,
     "plugin description"},
    {"version", T_OBJECT_EX, offsetof(RemminaPluginPython, version), 0,
     "plugin version"},
    {"appicon", T_OBJECT_EX, offsetof(RemminaPluginPython, appicon), 0,
     "plugin app-icon name"},
    {NULL}  /* Sentinel */
};


typedef struct {
    PyObject_HEAD
	PyObject *name;
	PyObject *description;
	PyObject *version;
	PyObject *appicon;
    RemminaPlugin* remmina_plugin;
} RemminaPluginPython;

typedef struct {
	PyObject *  type;
	PyObject *  name;
	PyObject *  description;
	PyObject *  domain;
	PyObject *  version;

	PyObject *  icon_name;
	PyObject *  icon_name_ssh;
	PyObject *  basic_settings;
	PyObject *  advanced_settings;
	PyObject *  ssh_setting;
	PyObject *	features;

    PyMethodDef m_mehods[] = {
	{"init", METH_VARARGS, NULL },
	{"open_connection", METH_VARARGS, NULL },
	{"close_connection", METH_VARARGS, NULL },
	{"query_feature", METH_VARARGS, NULL },
	{"call_feature", METH_VARARGS, NULL },
	{"send_keystrokes", METH_VARARGS, NULL },
	{"get_plugin_screenshot", METH_VARARGS, NULL }};
} RemminaProtocolPluginPython;

static PyTypeObject RemminaPluginPythonType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "remmina.Plugin",
    .tp_doc = "Custom objects",
    .tp_basicsize = sizeof(RemminaPluginPython),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT,
    .tp_members = RemminaPluginPython_members,
    .tp_new = remmina_remmina_plugin_new,
    .tp_init = remmina_remmina_plugin_init
};

static void
RemminaPluginPython_dealloc(RemminaPluginPython *self){
    Py_XDECREF(self->name);
    Py_XDECREF(self->description);
    Py_XDECREF(self->version);
    Py_XDECREF(self->appicon);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

PyTypeObject* remmina_plugin_python_plugin_create(void) { return &RemminaPluginPythonType; }

static PyObject *
remmina_remmina_plugin_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    RemminaPluginPython *self;

    self = (RemminaPluginPython *)type->tp_alloc(type, 0);
    if (!self)
        return NULL;
    
    self->name = PyUnicode_FromString("");
    if (!self->name) {
        Py_DECREF(self);
        return NULL;
    }
    
    self->description = PyUnicode_FromString("");
    if (!self->description) {
        Py_DECREF(self);
        return NULL;
    }
    
    self->version = PyUnicode_FromString("");
    if (!self->version) {
        Py_DECREF(self);
        return NULL;
    }
    
    self->appicon = PyUnicode_FromString("");
    if (!self->appicon) {
        Py_DECREF(self);
        return NULL;
    }

    return (PyObject *)self;
}

static int
remmina_remmina_plugin_init(RemminaPluginPython *self, PyObject *args, PyObject *kwds)
{
    static char* kwlist[] = { "name", "description", "version", "appicon", NULL };
    PyObject *name = NULL;
    PyObject *description = NULL;
    PyObject *version = NULL;
    PyObject *appicon = NULL;
    PyObject *tmp;

    if (!PyArg_ParseTupleAndKeywords(args, kwds, "|OOOO", kwlist, &name, &description, &version, &appicon))
        return -1;
    
    if (name) {
        tmp = self->name;
        Py_INCREF(name);
        self->name = name;
        Py_XDECREF(tmp);
    }
    if (description) {
        tmp = self->description;
        Py_INCREF(description);
        self->description = description;
        Py_XDECREF(tmp);
    }
    if (version) {
        tmp = self->version;
        Py_INCREF(version);
        self->version = version;
        Py_XDECREF(tmp);
    }
    if (appicon) {
        tmp = self->appicon;
        Py_INCREF(appicon);
        self->appicon = appicon;
        Py_XDECREF(tmp);
    }

    const char *name_str;
    const char *description_str;
    const char *version_str;
    const char *appicon_str;

    if (PyArg_ParseTuple(args, "ssss", &name_str, &version_str, &appicon_str, &description_str))
        printf("Initialied plugin: %s\nVersion: %s\nApp-Icon: %s\nDescription: %s\n", name_str, version_str, appicon_str, description_str);
    
    self->remmina_plugin = (RemminaPlugin*)malloc(sizeof(*self->remmina_plugin));
    self->remmina_plugin->name = name_str;
    self->remmina_plugin->type = REMMINA_PLUGIN_TYPE_PROTOCOL;

    return 0;
}