#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <string.h>

#include <gdk/gdkx.h>

#include "remmina_public.h"
#include "remmina_file_manager.h"
#include "remmina_pref.h"
#include "remmina_log.h"
#include "remmina_widget_pool.h"
#include "rcw.h"
#include "remmina_public.h"
#include "remmina_plugin_python.h"
#include "remmina_plugin_manager.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"


#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

// Level 0 - Remmina as a python module

static PyObject *
remmina_plugin_python_print(PyObject *self, PyObject *args) {
    const char* format;
    if (!PyArg_ParseTuple(args, "s", &format))
        return NULL;

    printf("%s\n", format);
    return self;
}

static PyMethodDef remminaMethods[] = {
    {"printf", remmina_plugin_python_print, METH_VARARGS, "Print to shell"},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef remminamodule = {
    PyModuleDef_HEAD_INIT,
    "remmina",
    "Remmina API.",
    -1,
    remminaMethods
};

// Level 1 - Remmina Plugin



typedef struct {
    PyObject_HEAD
	PyObject *name;
	PyObject *description;
	PyObject *version;
	PyObject *appicon;
} RemminaPluginPython;

static void
RemminaPluginPython_dealloc(RemminaPluginPython *self)
{
    Py_XDECREF(self->name);
    Py_XDECREF(self->description);
    Py_XDECREF(self->version);
    Py_XDECREF(self->appicon);
    Py_TYPE(self)->tp_free((PyObject *) self);
}

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


static RemminaPluginService *remmina_plugin_service = NULL;

static void remmina_plugin_hello_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	remmina_plugin_service->log_printf("[Hello] Plugin init\n");
}

static gboolean remmina_plugin_hello_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	remmina_plugin_service->log_printf("[Hello] Plugin open connection\n");

	GtkDialog *dialog;
	dialog = GTK_DIALOG(gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
			GTK_MESSAGE_INFO, GTK_BUTTONS_OK, ""));
	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
	return FALSE;
}

static gboolean remmina_plugin_hello_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	remmina_plugin_service->log_printf("[Hello] Plugin close connection\n");
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");
	return FALSE;
}

#define PLUGIN_NAME        "SET BY PYTHON"
#define PLUGIN_DESCRIPTION N_("SET BY PYTHON")
#define PLUGIN_VERSION     "SET BY PYTHON"
#define PLUGIN_APPICON     "SET BY PYTHON"

/* Array of RemminaProtocolSetting for basic settings.
* Each item is composed by:
* a) RemminaProtocolSettingType for setting type
* b) Setting name
* c) Setting description
* d) Compact disposition
* e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
* f) Setting Tooltip
*/
static const RemminaProtocolSetting remmina_plugin_hello_basic_settings[] =
{
    { REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

RemminaProtocolPlugin remmina_hello_plugin = {
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   // Type
	PLUGIN_NAME,                                    // Name
	PLUGIN_DESCRIPTION,                             // Description
	GETTEXT_PACKAGE,                                // Translation domain
	PLUGIN_VERSION,                                 // Version number
	PLUGIN_APPICON,                                 // Icon for normal connection
	PLUGIN_APPICON,                                 // Icon for SSH connection
	remmina_plugin_hello_basic_settings,             // Array for basic settings
	NULL,                                           // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_NONE,              // SSH settings type
	NULL,                                           // Array for available features
	remmina_plugin_hello_init,                       // Plugin initialization
	remmina_plugin_hello_open_connection,            // Plugin open connection
	remmina_plugin_hello_close_connection,           // Plugin close connection
	NULL,                                           // Query for available features
	NULL,                                           // Call a feature
	NULL,                                           // Send a keystroke
	NULL                                            // No screenshot support available
};

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
    

    remmina_hello_plugin.name = name_str;
    remmina_hello_plugin.description = description_str;
    remmina_hello_plugin.version = version_str;
    remmina_hello_plugin.icon_name = appicon_str;
    remmina_hello_plugin.icon_name_ssh = appicon_str;

    return 0;
}

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


PyMODINIT_FUNC PyInit_remmina(void) {
    // TODO: Initialize remmina Python module
    printf("Initializing remmina Python module");
    PyObject* module;

    if (PyType_Ready(&RemminaPluginPythonType) < 0)
        return NULL;

    module = PyModule_Create(&remminamodule);

    if (module)
    {
        Py_INCREF(&RemminaPluginPythonType);
        if (PyModule_AddObject(module, "Plugin", (PyObject*) &RemminaPluginPythonType) < 0)
        {
            Py_DECREF(&RemminaPluginPythonType);
            Py_DECREF(module);
            return NULL;
        }
    }
    

    return module;
}


gboolean remmina_plugin_python_load(RemminaPluginService* service, const char* name) {
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;
    
    if (PyImport_AppendInittab("remmina", PyInit_remmina) == -1) {
        fprintf(stderr, "Error: could not extend in-built modules table\n");
        return FALSE;
    }

    Py_Initialize();

    PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append(\"/usr/local/lib/remmina/plugins\")");

    pName = PyUnicode_DecodeFSDefault("toolsdevler");
    /* Error checking of pName left out */

    pModule = PyImport_Import(pName);
    if (pModule != NULL) {
        return TRUE;
    } else {
        PyErr_Print();
        g_print("Failed to load \"%s\"\n", name);
        return FALSE;
    }
    return TRUE;
}
