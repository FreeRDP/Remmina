#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <string.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <gdk/gdkx.h>

#include "remmina_public.h"
#include "remmina_file_manager.h"
#include "remmina_pref.h"
#include "common/remmina_plugin_python.h"
#include "remmina_log.h"
#include "remmina_widget_pool.h"
#include "rcw.h"
#include "remmina_public.h"
#include "remmina_plugin_python.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"


PyMODINIT_FUNC PyInit_remmina(void) {
    // TODO: Initialize remmina Python module
}

static void
remmina_protocol_widget_dealloc(Noddy* self)
{
    // TODO: Free memory if necessary
}

static PyObject *
remmina_protocol_widget_init(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    // TODO: In case python creates a new widget, implement. Otherwise raise error.
}

static PyObject *
remmina_protocol_widget_new(PyTypeObject *self, PyObject *args, PyObject *kwds) {
    // TODO: In case python creates a new widget, implement. Otherwise raise error.
}

static PyObject* remmina_plugin_manager_python_register_plugin() {
    remmina_plugin_manager_python_register_plugin();
}

static PyObject* remmina_protocol_python_widget_get_width() {
    RemminaProtocolWidget *gp;
    remmina_protocol_widget_get_width();
}
static PyObject* remmina_protocol_python_widget_set_width() {
    remmina_protocol_widget_set_width();
}
static PyObject* remmina_protocol_python_widget_get_height() {
    remmina_protocol_widget_get_height();
}
static PyObject* remmina_protocol_python_widget_set_height() {
    remmina_protocol_widget_set_height();
}
static PyObject* remmina_protocol_python_widget_get_current_scale_mode() {
    remmina_protocol_widget_get_current_scale_mode();
}
static PyObject* remmina_protocol_python_widget_get_expand() {
    remmina_protocol_widget_get_expand();
}
static PyObject* remmina_protocol_python_widget_set_expand() {
    remmina_protocol_widget_set_expand();
}
static PyObject* remmina_protocol_python_widget_has_error() {
    remmina_protocol_widget_has_error();
}
static PyObject* remmina_protocol_python_widget_set_error() {
    remmina_protocol_widget_set_error();
}
static PyObject* remmina_protocol_python_widget_is_closed() {
    remmina_protocol_widget_is_closed();
}
static PyObject* remmina_protocol_python_widget_get_file() {
    remmina_protocol_widget_get_file();
}
static PyObject* remmina_protocol_python_widget_emit_signal() {
    remmina_protocol_widget_emit_signal();
}
static PyObject* remmina_protocol_python_widget_register_hostkey() {
    remmina_protocol_widget_register_hostkey();
}
static PyObject* remmina_protocol_python_widget_start_direct_tunnel() {
    remmina_protocol_widget_start_direct_tunnel();
}
static PyObject* remmina_protocol_python_widget_start_reverse_tunnel() {
    remmina_protocol_widget_start_reverse_tunnel();
}
static PyObject* remmina_protocol_python_widget_start_xport_tunnel() {
    remmina_protocol_widget_start_xport_tunnel();
}
static PyObject* remmina_protocol_python_widget_set_display() {
    remmina_protocol_widget_set_display();
}
static PyObject* remmina_protocol_python_widget_signal_connection_closed() {
    remmina_protocol_widget_signal_connection_closed();
}
static PyObject* remmina_protocol_python_widget_signal_connection_opened() {
    remmina_protocol_widget_signal_connection_opened();
}
static PyObject* remmina_protocol_python_widget_update_align() {
    remmina_protocol_widget_update_align();
}
static PyObject* remmina_protocol_python_widget_unlock_dynres() {
    remmina_protocol_widget_unlock_dynres();
}
static PyObject* remmina_protocol_python_widget_desktop_resize() {
    remmina_protocol_widget_desktop_resize();
}
static PyObject* remmina_protocol_python_widget_panel_auth() {
    remmina_protocol_widget_panel_auth();
}
static PyObject* remmina_protocol_python_widget_panel_new_certificate() {
    remmina_protocol_widget_panel_new_certificate();
}
static PyObject* remmina_protocol_python_widget_panel_changed_certificate() {
    remmina_protocol_widget_panel_changed_certificate();
}
static PyObject* remmina_protocol_python_widget_get_username() {
    remmina_protocol_widget_get_username();
}
static PyObject* remmina_protocol_python_widget_get_password() {
    remmina_protocol_widget_get_password();
}
static PyObject* remmina_protocol_python_widget_get_domain() {
    remmina_protocol_widget_get_domain();
}
static PyObject* remmina_protocol_python_widget_get_savepassword() {
    remmina_protocol_widget_get_savepassword();
}
static PyObject* remmina_protocol_python_widget_panel_authx509() {
    remmina_protocol_widget_panel_authx509();
}
static PyObject* remmina_protocol_python_widget_get_cacert() {
    remmina_protocol_widget_get_cacert();
}
static PyObject* remmina_protocol_python_widget_get_cacrl() {
    remmina_protocol_widget_get_cacrl();
}
static PyObject* remmina_protocol_python_widget_get_clientcert() {
    remmina_protocol_widget_get_clientcert();
}
static PyObject* remmina_protocol_python_widget_get_clientkey() {
    remmina_protocol_widget_get_clientkey();
}
static PyObject* remmina_protocol_python_widget_save_cred() {
    remmina_protocol_widget_save_cred();
}
static PyObject* remmina_protocol_python_widget_panel_show_listen() {
    remmina_protocol_widget_panel_show_listen();
}
static PyObject* remmina_protocol_python_widget_panel_show_retry() {
    remmina_protocol_widget_panel_show_retry();
}
static PyObject* remmina_protocol_python_widget_panel_show() {
    remmina_protocol_widget_panel_show();
}
static PyObject* remmina_protocol_python_widget_panel_hide() {
    remmina_protocol_widget_panel_hide();
}
static PyObject* remmina_protocol_python_widget_ssh_exec() {
    remmina_protocol_widget_ssh_exec();
}
static PyObject* remmina_protocol_python_widget_chat_open() {
    remmina_protocol_widget_chat_open();
}
static PyObject* remmina_protocol_python_widget_chat_close() {
    remmina_protocol_widget_chat_close();
}
static PyObject* remmina_protocol_python_widget_chat_receive() {
    remmina_protocol_widget_chat_receive();
}
static PyObject* remmina_protocol_python_widget_send_keys_signals() {
    remmina_protocol_widget_send_keys_signals();
}


	static PyObject* remmina_file_python_get_datadir() {
        remmina_file_get_datadir();
    }

	static PyObject* remmina_file_python_new() {
        remmina_file_new();
    }

	static PyObject* remmina_file_python_get_filename() {
        remmina_file_get_filename();
    }

	static PyObject* remmina_file_python_set_string() {
        remmina_file_set_string();
    }

	static PyObject* remmina_file_python_get_string() {
        remmina_file_get_string();
    }

	static PyObject* remmina_file_python_get_secret() {
        remmina_file_get_secret();
    }

	static PyObject* remmina_file_python_set_int() {
        remmina_file_set_int();
    }

	static PyObject* remmina_file_python_get_int() {
        remmina_file_get_int();
    }

	static PyObject* remmina_file_python_unsave_passwords() {
        remmina_file_unsave_passwords();
    }


	static PyObject* remmina_pref_pyton_set_value() {
        remmina_pref_set_value();
    }
	static PyObject* remmina_pref_pyton_get_value() {
        remmina_pref_get_value();
    }
	static PyObject* remmina_pref_pyton_get_scale_quality() {
        remmina_pref_get_scale_quality();
    }
	static PyObject* remmina_pref_pyton_get_sshtunnel_port() {
        remmina_pref_get_sshtunnel_port();
    }
	static PyObject* remmina_pref_pyton_get_ssh_loglevel() {
        remmina_pref_get_ssh_loglevel();
    }
	static PyObject* remmina_pref_pyton_get_ssh_parseconfig() {
        remmina_pref_get_ssh_parseconfig();
    }
	static PyObject* remmina_pref_pyton_keymap_get_keyval() {
        remmina_pref_keymap_get_keyval();
    }

	static PyObject* remmina_log_python_print() {
        remmina_log_print();
    }
	static PyObject* remmina_log_python_printf() {
        remmina_log_printf();
    }

	static PyObject* remmina_widget_python_pool_register() {
        remmina_widget_pool_register();
    }

	static PyObject* rcw_open_python_from_file_full() {
        rcw_open_from_file_full();
    }

	static PyObject* remmina_public_python_get_server_port() {
        remmina_public_get_server_port();
    }

	static PyObject* remmina_masterthread_exec_python_is_main_thread() {
        return remmina_masterthread_exec_is_main_thread();
    }

	static PyObject* remmina_gtksocket_available() {
        remmina_gtksocket_available();
    }

	static PyObject* remmina_protocol_python_widget_get_profile_remote_width() {
        remmina_protocol_widget_get_profile_remote_width();
    }

	static PyObject* remmina_protocol_python_widget_get_profile_remote_height() {
        remmina_protocol_widget_get_profile_remote_height();
    }
};

gboolean remmina_plugin_python_load(RemminaPluginService* service, const char* name) {
    PyObject *pName, *pModule, *pFunc;
    PyObject *pArgs, *pValue;
    int i;

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
