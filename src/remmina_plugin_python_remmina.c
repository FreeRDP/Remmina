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

#include "remmina_plugin_python_remmina.h"

/**
 * @brief Holds pairs of Python and Remmina plugin instances (PyPlugin).
 */
GPtrArray *remmina_plugin_registry = NULL;

/**
 * @brief Wraps the log_printf function of RemminaPluginService.
 * 
 * @details This function is only called by the Python engine! There is only one argument
 * since Python offers an inline string formatting which renders any formatting capabilities
 * redundant.
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param msg     The message to print.         
 */
static PyObject* remmina_plugin_python_log_printf_wrapper(PyObject* self, PyObject* msg);

/**
 * @brief Accepts an instance of a Python class representing a Remmina plugin.
 * 
 * @details This function is only called by the Python engine!
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  An instance of a Python plugin class.
 */
static PyObject* remmina_register_plugin_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief Retrieves the viewport of the current connection window if available.
 * 
 * @details This function is only called by the Python engine!
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_plugin_python_get_viewport(PyObject* self, PyObject* handle);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_width_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_width_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_height_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_height_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_current_scale_mode_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_expand_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_expand_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_has_error_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_error_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_is_closed_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_file_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_emit_signal_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_register_hostkey_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_start_direct_tunnel_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_start_reverse_tunnel_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_start_xport_tunnel_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_display_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_signal_connection_closed_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_signal_connection_opened_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_update_align_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_unlock_dynres_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_desktop_resize_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_auth_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_new_certificate_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_changed_certificate_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_username_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_password_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_domain_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_savepassword_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_authx509_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_cacert_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_cacrl_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_clientcert_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_clientkey_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_save_cred_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_show_listen_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_show_retry_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_show_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_hide_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_ssh_exec_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_chat_open_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_chat_close_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_chat_receive_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_send_keys_signals_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_datadir_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_new_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_filename_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_set_string_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_string_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_secret_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_set_int_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_int_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_unsave_passwords_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_set_value_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_value_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_scale_quality_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_sshtunnel_port_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_ssh_loglevel_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_ssh_parseconfig_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_keymap_get_keyval_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_log_print_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_log_printf_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_widget_pool_register_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* rcw_open_from_file_full_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_public_get_server_port_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_masterthread_exec_is_main_thread_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_gtksocket_available_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_profile_remote_heigh_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_profile_remote_width_wrapper(PyObject* self, PyObject* plugin);

/**
 * @brief The functions of the remmina module.
 * 
 * @details If any function has to be added this is the place to do it. This list is referenced
 * by the PyModuleDef (describing the module) instance below.
 */
static PyMethodDef remmina_python_module_type_methods[] = {
	{"register_plugin", (PyCFunction)remmina_register_plugin_wrapper,  METH_O, NULL },
	{"log_print", (PyCFunction)remmina_plugin_python_log_printf_wrapper,  METH_VARARGS, NULL },
	{"get_datadir", (PyCFunction)remmina_file_get_datadir_wrapper},
	{"file_new", (PyCFunction)remmina_file_new_wrapper},
    
	{"pref_set_value", (PyCFunction)remmina_pref_set_value_wrapper},
	{"pref_get_value", (PyCFunction)remmina_pref_get_value_wrapper},
	{"pref_get_scale_quality", (PyCFunction)remmina_pref_get_scale_quality_wrapper},
	{"pref_get_sshtunnel_port", (PyCFunction)remmina_pref_get_sshtunnel_port_wrapper},
	{"pref_get_ssh_loglevel", (PyCFunction)remmina_pref_get_ssh_loglevel_wrapper},
	{"pref_get_ssh_parseconfig", (PyCFunction)remmina_pref_get_ssh_parseconfig_wrapper},
	{"pref_keymap_get_keyval", (PyCFunction)remmina_pref_keymap_get_keyval_wrapper},

	{"log_print", (PyCFunction)remmina_log_print_wrapper},
	{"log_printf", (PyCFunction)remmina_log_printf_wrapper},

	{"widget_pool_register", (PyCFunction)remmina_widget_pool_register_wrapper},

	{"rcw_open_from_file_full", (PyCFunction)rcw_open_from_file_full_wrapper },
	{"public_get_server_port", (PyCFunction)remmina_public_get_server_port_wrapper},
	{"masterthread_exec_is_main_thread", (PyCFunction)remmina_masterthread_exec_is_main_thread_wrapper},
	{"gtksocket_available", (PyCFunction)remmina_gtksocket_available_wrapper},
	{"protocol_widget_get_profile_remote_width", (PyCFunction)remmina_protocol_widget_get_profile_remote_width_wrapper},
	{"protocol_widget_get_profile_remote_heigh", (PyCFunction)remmina_protocol_widget_get_profile_remote_heigh_wrapper},
    {NULL}  /* Sentinel */
};

/**
 * @brief The definition of the Python module 'remmina'.
 */
static PyModuleDef remmina_python_module_type = {
    PyModuleDef_HEAD_INIT,
    .m_name = "remmina",
    .m_doc = "Remmina API.",
    .m_size = -1,
    .m_methods = remmina_python_module_type_methods
};

/**
 * @brief Is called from the Python engine when it initializes the 'remmina' module.
 * @details This function is only called by the Python engine!
 */
static PyMODINIT_FUNC remmina_plugin_python_module_initialize(void)
{
	TRACE_CALL(__func__);
    return PyModule_Create(&remmina_python_module_type);
}

/**
 * @brief Initializes all globals and registers the 'remmina' module in the Python engine.
 * @details This 
 */
void remmina_plugin_python_module_init(void)
{
	TRACE_CALL(__func__);
    remmina_plugin_registry = g_ptr_array_new();
    if (PyImport_AppendInittab("remmina", remmina_plugin_python_module_initialize)) {
        g_print("Error initializing remmina module for python!\n");
        PyErr_Print();
    }
}

/**
 * @brief Wraps the log_printf function of RemminaPluginService.
 * 
 * @details This function is only called by the Python engine! There is only one argument
 * since Python offers an inline string formatting which renders any formatting capabilities
 * redundant.
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param msg     The message to print.         
 */
static PyObject* remmina_plugin_python_log_printf_wrapper(PyObject* self, PyObject* msg)
{
	TRACE_CALL(__func__);
    char* fmt = NULL;

    if (PyArg_ParseTuple(msg, "s", &fmt)) {
        remmina_log_printf(fmt);
    } else {
        g_print("Failed to load.\n");
        PyErr_Print();
        return Py_None;
    }

    return Py_None;
}

/**
 * @brief Accepts an instance of a Python class representing a Remmina plugin.
 * 
 * @details This function is only called by the Python engine!
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  An instance of a Python plugin class.
 */
static PyObject* remmina_register_plugin_wrapper(PyObject* self, PyObject* plugin_instance)
{
	TRACE_CALL(__func__);
    if (plugin_instance) {
         if (!remmina_plugin_python_check_mandatory_member(plugin_instance, "name")
            || !remmina_plugin_python_check_mandatory_member(plugin_instance, "version")) {
            return NULL;
        }

        /* Protocol plugin definition and features */
        RemminaPlugin* remmina_plugin = NULL;
        
        const gchar* pluginType = PyUnicode_AsUTF8(PyObject_GetAttrString(plugin_instance, "type"));

        if (g_str_equal(pluginType, "protocol")) {
            remmina_plugin = remmina_plugin_python_create_protocol_plugin(plugin_instance);
        } else if (g_str_equal(pluginType, "entry")) {
            remmina_plugin = remmina_plugin_python_create_entry_plugin(plugin_instance);
        } else if (g_str_equal(pluginType, "file")) {
            remmina_plugin = remmina_plugin_python_create_file_plugin(plugin_instance);
        } else if (g_str_equal(pluginType, "tool")) {
            remmina_plugin = remmina_plugin_python_create_tool_plugin(plugin_instance);
        } else if (g_str_equal(pluginType, "pref")) {
            remmina_plugin = remmina_plugin_python_create_pref_plugin(plugin_instance);
        } else if (g_str_equal(pluginType, "secret")) {
            remmina_plugin = remmina_plugin_python_create_secret_plugin(plugin_instance);
        } else {
            g_printerr("Unknown plugin type: %s\n", pluginType);
        }

        if (remmina_plugin) {
            g_ptr_array_add(remmina_plugin_registry, plugin_instance);
            remmina_plugin_manager_service.register_plugin((RemminaPlugin *)remmina_plugin);

            PyPlugin* plugin = (PyPlugin*)malloc(sizeof(PyPlugin));
            plugin->protocol_plugin = remmina_plugin;
            plugin->generic_plugin = remmina_plugin;
            plugin->pythonInstance = plugin_instance;
            plugin->gp = malloc(sizeof(PyRemminaProtocolWidget));
            plugin->gp->gp = NULL;
            g_ptr_array_add(remmina_plugin_registry, plugin);
        }
    }
        
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_width_wrapper(PyObject* self, PyObject* plugin)
{

    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_width_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_height_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_height_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_current_scale_mode_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_expand_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_expand_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_has_error_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_error_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_is_closed_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_file_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_emit_signal_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_register_hostkey_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_start_direct_tunnel_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_start_reverse_tunnel_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_start_xport_tunnel_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_set_display_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_signal_connection_closed_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_signal_connection_opened_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_update_align_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_unlock_dynres_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_desktop_resize_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_auth_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_new_certificate_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_changed_certificate_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_username_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_password_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_domain_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_savepassword_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_authx509_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_cacert_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_cacrl_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_clientcert_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_clientkey_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_save_cred_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_show_listen_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_show_retry_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_show_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_panel_hide_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_ssh_exec_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_chat_open_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_chat_close_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_chat_receive_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_send_keys_signals_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_datadir_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_new_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_filename_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_set_string_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_string_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_secret_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_set_int_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_get_int_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_file_unsave_passwords_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_set_value_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_value_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_scale_quality_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_sshtunnel_port_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_ssh_loglevel_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_get_ssh_parseconfig_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_pref_keymap_get_keyval_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_log_print_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_log_printf_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_widget_pool_register_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* rcw_open_from_file_full_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_public_get_server_port_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_masterthread_exec_is_main_thread_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_gtksocket_available_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_profile_remote_heigh_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief 
 * 
 * @details 
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_protocol_widget_get_profile_remote_width_wrapper(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

static gboolean remmina_plugin_equal(gconstpointer lhs, gconstpointer rhs)
{
    return g_str_equal(((PyPlugin*)lhs)->generic_plugin->name, ((gchar*)rhs));
}

PyPlugin* remmina_plugin_python_module_get_plugin(RemminaProtocolWidget* gp)
{
    guint index = 0;
    if (g_ptr_array_find_with_equal_func(remmina_plugin_registry, gp->plugin->name, remmina_plugin_equal, &index)) {
        return (PyPlugin*) g_ptr_array_index(remmina_plugin_registry, index);
    } else {
        g_printerr("[%s:%s]: No plugin named %s!\n", __FILE__, __LINE__, gp->plugin->name);
        return NULL;
    }
}
