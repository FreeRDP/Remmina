#include "remmina_plugin_python_remmina.h"
#include <Python.h>

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
static PyObject* remmina_plugin_python_register_plugin(PyObject* self, PyObject* plugin)
{
    return Py_None;
}

/**
 * @brief Retrieves the viewport of the current connection window if available.
 * 
 * @details This function is only called by the Python engine!
 * 
 * @param self    Is always NULL since it is a static function of the 'remmina' module
 * @param plugin  The handle to retrieve the RemminaConnectionWidget
 */
static PyObject* remmina_plugin_python_get_viewport(PyObject* self, PyObject* handle)
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
static PyObject* remmina_plugin_manager_register_plugin_wrapper(PyObject* self, PyObject* plugin)
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
