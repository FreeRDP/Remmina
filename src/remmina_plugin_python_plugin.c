#include "config.h"

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

#include <glib.h>
#include <gtk/gtk.h>
#include "remmina_plugin_manager.h"
#include "remmina/plugin.h"
#include "remmina_plugin_python_plugin.h"
#include "remmina_protocol_widget.h"

GPtrArray* remmina_plugin_registry = NULL;

typedef struct {
    PyObject * name;
    PyObject * description;
    PyObject * version;
    PyObject * appicon;
    PyObject * icon_name;
    PyObject * icon_name_ssh;
    PyObject * basic_settings;
    PyObject * advanced_settings;
    PyObject * ssh_setting;
    PyObject * features;
    RemminaProtocolWidget* widget;
} RemminaPluginPythonPlugin;


static PyMemberDef remmina_plugin_python_protocol_plugin_members[] = {
    {"name", T_OBJECT_EX, offsetof(RemminaPluginPythonPlugin, name), 0, "plugin name"},
    {"description", T_OBJECT_EX, offsetof(RemminaPluginPythonPlugin, description), 0, "plugin description"},
    {"version", T_OBJECT_EX, offsetof(RemminaPluginPythonPlugin, version), 0, "plugin version"},
    {"appicon", T_OBJECT_EX, offsetof(RemminaPluginPythonPlugin, appicon), 0, "plugin app-icon name"},
	{"icon_name", T_OBJECT_EX, offsetof(RemminaPluginPythonPlugin, icon_name), 0, "icon_name"},
	{"icon_name_ssh", T_OBJECT_EX, offsetof(RemminaPluginPythonPlugin, icon_name_ssh), 0, "icon_name_ssh"},
	{"basic_settings", T_OBJECT_EX, offsetof(RemminaPluginPythonPlugin, basic_settings), 0, "basic_settings"},
	{"advanced_settings", T_INT, offsetof(RemminaPluginPythonPlugin, advanced_settings), 0, "advanced_settings"},
    {"ssh_setting", T_INT, offsetof(RemminaPluginPythonPlugin, ssh_setting), 0, "ssh_setting"},
	{"features", T_OBJECT_EX, offsetof(RemminaPluginPythonPlugin, features), 0, "features"},
    {NULL}  /* Sentinel */
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_protocol_python_plugin_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

static PyMethodDef remmina_plugin_python_protocol_plugin_methods[] = {
	{"init", METH_NOARGS, NULL },
	{"open_connection", METH_NOARGS, NULL },
	{"close_connection", METH_NOARGS, NULL },
	{"query_feature", METH_NOARGS, NULL },
	{"call_feature", METH_NOARGS, NULL },
	{"send_keystrokes", METH_NOARGS, NULL },
	{"get_plugin_screenshot", METH_NOARGS, NULL },
    {NULL}  /* Sentinel */
};

static PyMemberDef remmina_plugin_python_manager_service_members[] = {
    {NULL}  /* Sentinel */
};


/*
	void (*init)(RemminaProtocolWidget *gp);
	gboolean (*open_connection)(RemminaProtocolWidget *gp);
	gboolean (*close_connection)(RemminaProtocolWidget *gp);
	gboolean (*query_feature)(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
	void (*call_feature)(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
	void (*send_keystrokes)(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen);
	gboolean (*get_plugin_screenshot)(RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd);*/

static void remmina_protocol_plugin_wrapper_init(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_plugin_wrapper_open_connection(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_plugin_wrapper_close_connection(RemminaProtocolWidget *gp);
static gboolean remmina_protocol_plugin_wrapper_query_feature(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
static void remmina_protocol_plugin_wrapper_call_feature(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
static void remmina_protocol_plugin_wrapper_send_keystrokes(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen);
static gboolean remmina_protocol_plugin_wrapper_get_plugin_screenshot(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd);

static void
remmina_plugin_python_init_protocol_plugin(RemminaProtocolPlugin* plugin, RemminaPluginPythonPlugin* pythonObject);

static int
remmina_protocol_plugin_init(RemminaPluginPythonPlugin *self, PyObject *args, PyObject *kwds);

static PyObject *
remmina_protocol_plugin_new(PyTypeObject *type, PyObject *args, PyObject *kwds);

static PyTypeObject RemminaProtocolPluginPythonType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    .tp_name = "remmina.ProtocolPlugin",
    .tp_doc = "Custom objects",
    .tp_basicsize = sizeof(RemminaPluginPythonPlugin),
    .tp_itemsize = sizeof(RemminaPluginPythonPlugin),
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_members = remmina_plugin_python_protocol_plugin_members,
    .tp_methods = remmina_plugin_python_protocol_plugin_methods,
    .tp_new = remmina_protocol_plugin_new,
    .tp_init = remmina_protocol_plugin_init
};

static void
RemminaPluginPython_dealloc(RemminaPluginPythonPlugin *self){
    Py_XDECREF(self->name);
    Py_XDECREF(self->description);
    Py_XDECREF(self->version);
    Py_XDECREF(self->appicon);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

static void
remmina_plugin_python_init_protocol_plugin(RemminaProtocolPlugin* plugin, RemminaPluginPythonPlugin* pythonObject) {
    
}

typedef struct {
    PyObject_HEAD
    RemminaPluginPythonPlugin* internal;
    RemminaPlugin* plugin;
} PyPlugin;

PyTypeObject* remmina_plugin_python_plugin_create(void) { return &RemminaProtocolPluginPythonType; }

static PyObject *
remmina_protocol_plugin_new(PyTypeObject *type, PyObject *parent, PyObject *args)
{
    PyPlugin *state;
    PyObject *self;
    self = (PyObject*)type->tp_alloc(type, 0);
    if (self == NULL)
        return NULL;
    // Cast the object to the appropriate type
    state = (PyPlugin *) self;
    state->plugin = NULL;
    
    // Initialize your internal structure
    state->internal = malloc(sizeof(*state->internal));
    if (state->internal == NULL)
        return NULL;
    memset(state->internal, 0, sizeof(*state->internal));
    // This means no error occurred

    state->internal->name = PyUnicode_FromString("");
    Py_INCREF(state->internal->name);
    if (!state->internal->name) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->description = PyUnicode_FromString("");
    Py_INCREF(state->internal->description);
    if (!state->internal->description) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->version = PyUnicode_FromString("");
    Py_INCREF(state->internal->version);
    if (!state->internal->version) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->appicon = PyUnicode_FromString("");
    Py_INCREF(state->internal->appicon);
    if (!state->internal->appicon) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->icon_name = PyUnicode_FromString("");
    Py_INCREF(state->internal->icon_name);
    if (!state->internal->icon_name) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->icon_name_ssh = PyUnicode_FromString("");
    Py_INCREF(state->internal->icon_name_ssh);
    if (!state->internal->icon_name_ssh) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->basic_settings = PyUnicode_FromString("");
    Py_INCREF(state->internal->basic_settings);
    if (!state->internal->basic_settings) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->advanced_settings = PyUnicode_FromString("");
    Py_INCREF(state->internal->advanced_settings);
    if (!state->internal->advanced_settings) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->ssh_setting = PyUnicode_FromString("");
    Py_INCREF(state->internal->ssh_setting);
    if (!state->internal->ssh_setting) {
        Py_DECREF(self);
        return NULL;
    }

    state->internal->features = PyUnicode_FromString("");
    Py_INCREF(state->internal->features);
    if (!state->internal->features) {
        Py_DECREF(self);
        return NULL;
    }

    if (!remmina_plugin_registry)
        remmina_plugin_registry = g_ptr_array_new();

    g_ptr_array_add(remmina_plugin_registry, state);

    return self;
}


static int
remmina_protocol_plugin_init(RemminaPluginPythonPlugin *self, PyObject *args, PyObject *kwds)
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
    }
    if (description) {
        tmp = self->description;
        Py_INCREF(description);
        self->description = description;
    }
    if (version) {
        tmp = self->version;
        Py_INCREF(version);
        self->version = version;
    }
    if (appicon) {
        tmp = self->appicon;
        Py_INCREF(appicon);
        self->appicon = appicon;
    }

    const char *name_str;
    const char *description_str;
    const char *version_str;
    const char *appicon_str;

    if (PyArg_ParseTuple(args, "ssss", &name_str, &description_str, &version_str, &appicon_str))
        printf("Initialied plugin: %s\nVersion: %s\nApp-Icon: %s\nDescription: %s\n", name_str, version_str, appicon_str, description_str);

    /* Protocol plugin definition and features */
    RemminaProtocolPlugin* remmina_plugin = (RemminaProtocolPlugin*)malloc(sizeof(RemminaProtocolPlugin));
    remmina_plugin->type = REMMINA_PLUGIN_TYPE_PROTOCOL;
	remmina_plugin->name = name_str;                                               // Name
	remmina_plugin->description = description_str;             // Description
	remmina_plugin->domain = GETTEXT_PACKAGE;                                    // Translation domain
	remmina_plugin->version = version_str;                                           // Version number
	remmina_plugin->icon_name = appicon_str;                         // Icon for normal connection
	remmina_plugin->icon_name_ssh = appicon_str;                     // Icon for SSH connection
	remmina_plugin->basic_settings =  remmina_protocol_python_plugin_features;                // Array for basic settings
	remmina_plugin->advanced_settings = NULL;                                    // Array for advanced settings
	remmina_plugin->ssh_setting = REMMINA_PROTOCOL_SSH_SETTING_TUNNEL;        // SSH settings type
	remmina_plugin->features = remmina_protocol_python_plugin_features;                     // Array for available features
	remmina_plugin->init =  remmina_protocol_plugin_wrapper_init;                             // Plugin initialization
	remmina_plugin->open_connection =  remmina_protocol_plugin_wrapper_open_connection;       // Plugin open connection
	remmina_plugin->close_connection =  remmina_protocol_plugin_wrapper_close_connection;     // Plugin close connection
	remmina_plugin->query_feature =  remmina_protocol_plugin_wrapper_query_feature;           // Query for available features
	remmina_plugin->call_feature =  remmina_protocol_plugin_wrapper_call_feature;             // Call a feature
	remmina_plugin->send_keystrokes = NULL;                                      // Send a keystroke
	remmina_plugin->get_plugin_screenshot = NULL;                                // Screenshot support unavailable

	remmina_plugin_manager_service.register_plugin((RemminaPlugin *)remmina_plugin);

    return 0;
}



static PyObject* get_python_plugin_instance(RemminaProtocolWidget *gp) {
    PyPlugin* state = NULL;
    for (int i = 0; i < remmina_plugin_registry->len; i++) {
		state = (PyPlugin*)g_ptr_array_index(remmina_plugin_registry, i);
        if (!state->plugin)
            continue;
        if (g_strcmp0(state->plugin->name, gp->plugin->name)) {
            return (PyObject*)state;
        }
	}

    return NULL;
}

static void remmina_protocol_plugin_wrapper_init(RemminaProtocolWidget *gp) {
    PyObject* self = get_python_plugin_instance(gp);
    if (self) {
        PyObject* result = PyObject_CallMethod(self, "init", NULL);
    }
}

static gboolean remmina_protocol_plugin_wrapper_open_connection(RemminaProtocolWidget *gp) {
    PyObject* self = get_python_plugin_instance(gp);
    if (self) {
        PyObject* result = PyObject_CallMethod(self, "open_connection", NULL);
        return PyObject_IsTrue(result);
    }
    return FALSE;
}

static gboolean remmina_protocol_plugin_wrapper_close_connection(RemminaProtocolWidget *gp) {
    PyObject* self = get_python_plugin_instance(gp);
    if (self) {
        PyObject* result = PyObject_CallMethod(self, "close_connection", NULL);
        return PyObject_IsTrue(result);
    }
    return FALSE;
}

static gboolean remmina_protocol_plugin_wrapper_query_feature(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature) {
    return TRUE;
}

static void remmina_protocol_plugin_wrapper_call_feature(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature) {

}

static void remmina_protocol_plugin_wrapper_send_keystrokes(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen) {

}

static gboolean remmina_protocol_plugin_wrapper_get_plugin_screenshot(RemminaProtocolPlugin* plugin, RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd) {
    return TRUE;
}

