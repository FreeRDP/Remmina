/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
 * Copyright (C) 2023-2024 Hiroyuki Tanaka, Sunil Bhat
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

#include "config.h"


#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gmodule.h>
#include <json-glib/json-glib.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#elif defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdkwayland.h>
#endif
#include "remmina_public.h"
#include "remmina_main.h"
#include "remmina_file_manager.h"
#include "remmina_pref.h"
#include "remmina_info.h"
#include "remmina_protocol_widget.h"
#include "remmina_log.h"
#include "remmina_widget_pool.h"
#include "rcw.h"
#include "remmina_plugin_manager.h"
#include "remmina_plugin_native.h"
#include "remmina_public.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_curl_connector.h"
#include "remmina_utils.h"
#include "remmina_unlock.h"

static GPtrArray* remmina_plugin_table = NULL;

static GPtrArray* remmina_available_plugin_table = NULL;

static GtkDialog* remmina_plugin_window = NULL;

static SignalData* remmina_plugin_signal_data = NULL;

/* A GHashTable of GHashTables where to store the names of the encrypted settings */
static GHashTable *encrypted_settings_cache = NULL;

/* There can be only one secret plugin loaded */
static RemminaSecretPlugin *remmina_secret_plugin = NULL;



// TRANSLATORS: "Language Wrapper" is a wrapper for plugins written in other programmin languages (Python in this context)
static const gchar *remmina_plugin_type_name[] =
{ N_("Protocol"), N_("Entry"), N_("File"), N_("Tool"), N_("Preference"), N_("Secret"), N_("Language Wrapper"), NULL };

static gint remmina_plugin_manager_compare_func(RemminaPlugin **a, RemminaPlugin **b)
{
	TRACE_CALL(__func__);
	return g_strcmp0((*a)->name, (*b)->name);
}

static void htdestroy(gpointer ht)
{
	g_hash_table_unref(ht);
}

static void init_settings_cache(RemminaPlugin *plugin)
{
	TRACE_CALL(__func__);
	RemminaProtocolPlugin *protocol_plugin;
	const RemminaProtocolSetting* setting_iter;
	GHashTable *pht;

	/* This code make a encrypted setting cache only for protocol plugins */

	if (plugin->type != REMMINA_PLUGIN_TYPE_PROTOCOL) {
		return;
	}

	if (encrypted_settings_cache == NULL)
		encrypted_settings_cache = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, htdestroy);

	if (!(pht = g_hash_table_lookup(encrypted_settings_cache, plugin->name))) {
		pht = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);
		g_hash_table_insert(encrypted_settings_cache, g_strdup(plugin->name), pht);
	}

	/* Some settings are encrypted "by name" */
	/* g_hash_table_insert(pht, g_strdup("proxy_password"), (gpointer)TRUE); */

	g_hash_table_insert(pht, g_strdup("ssh_tunnel_password"), (gpointer)TRUE);
	g_hash_table_insert(pht, g_strdup("ssh_tunnel_passphrase"), (gpointer)TRUE);

	/* ssh_password is no longer used starting from remmina 1.4.
	 * But we MUST mark it as encrypted setting, or the migration procedure will fail */
	g_hash_table_insert(pht, g_strdup("ssh_password"), (gpointer)TRUE);

	/* Other settings are encrypted because of their type */
	protocol_plugin = (RemminaProtocolPlugin *)plugin;
	setting_iter = protocol_plugin->basic_settings;
	if (setting_iter) {
		while (setting_iter->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
			if (setting_iter->type == REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD)
				g_hash_table_insert(pht, g_strdup(remmina_plugin_manager_get_canonical_setting_name(setting_iter)), (gpointer)TRUE);
			setting_iter++;
		}
	}
	setting_iter = protocol_plugin->advanced_settings;
	if (setting_iter) {
		while (setting_iter->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
			if (setting_iter->type == REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD)
				g_hash_table_insert(pht, g_strdup(remmina_plugin_manager_get_canonical_setting_name(setting_iter)), (gpointer)TRUE);
			setting_iter++;
		}
	}

}

static gboolean remmina_plugin_manager_register_plugin(RemminaPlugin *plugin)
{
	TRACE_CALL(__func__);
	if (plugin->type == REMMINA_PLUGIN_TYPE_SECRET) {
		REMMINA_DEBUG("Remmina plugin %s (type=%s) has been registered, but is not yet initialized/activated. "
			"The initialization order is %d.", plugin->name,
			_(remmina_plugin_type_name[plugin->type]),
			((RemminaSecretPlugin*)plugin)->init_order);
	}
	init_settings_cache(plugin);

	g_ptr_array_add(remmina_plugin_table, plugin);
	g_ptr_array_sort(remmina_plugin_table, (GCompareFunc)remmina_plugin_manager_compare_func);
	return TRUE;
}

gboolean remmina_gtksocket_available()
{
	GdkDisplayManager* dm;
	GdkDisplay* d;
	gboolean available;

	dm = gdk_display_manager_get();
	d = gdk_display_manager_get_default_display(dm);
	available = FALSE;

#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(d)) {
		/* GtkSocket support is available only under X.Org */
		available = TRUE;
	}
#endif
	return available;
}

RemminaPluginService remmina_plugin_manager_service =
{
	remmina_plugin_manager_register_plugin,
	remmina_protocol_widget_get_width,
	remmina_protocol_widget_set_width,
	remmina_protocol_widget_get_height,
	remmina_protocol_widget_set_height,
	remmina_protocol_widget_get_current_scale_mode,
	remmina_protocol_widget_get_expand,
	remmina_protocol_widget_set_expand,
	remmina_protocol_widget_has_error,
	remmina_protocol_widget_set_error,
	remmina_protocol_widget_is_closed,
	remmina_protocol_widget_get_file,
	remmina_protocol_widget_emit_signal,
	remmina_protocol_widget_register_hostkey,
	remmina_protocol_widget_start_direct_tunnel,
	remmina_protocol_widget_start_reverse_tunnel,
	remmina_protocol_widget_start_xport_tunnel,
	remmina_protocol_widget_set_display,
	remmina_protocol_widget_signal_connection_closed,
	remmina_protocol_widget_signal_connection_opened,
	remmina_protocol_widget_update_align,
	remmina_protocol_widget_lock_dynres,
	remmina_protocol_widget_unlock_dynres,
	remmina_protocol_widget_desktop_resize,
	remmina_protocol_widget_panel_auth,
	remmina_protocol_widget_panel_new_certificate,
	remmina_protocol_widget_panel_changed_certificate,
	remmina_protocol_widget_get_username,
	remmina_protocol_widget_get_password,
	remmina_protocol_widget_get_domain,
	remmina_protocol_widget_get_savepassword,
	remmina_protocol_widget_panel_authx509,
	remmina_protocol_widget_get_cacert,
	remmina_protocol_widget_get_cacrl,
	remmina_protocol_widget_get_clientcert,
	remmina_protocol_widget_get_clientkey,
	remmina_protocol_widget_save_cred,
	remmina_protocol_widget_panel_show_listen,
	remmina_protocol_widget_panel_show_retry,
	remmina_protocol_widget_panel_show,
	remmina_protocol_widget_panel_hide,
	remmina_protocol_widget_ssh_exec,
	remmina_protocol_widget_chat_open,
	remmina_protocol_widget_chat_close,
	remmina_protocol_widget_chat_receive,
	remmina_protocol_widget_send_keys_signals,

	remmina_file_get_datadir,

	remmina_file_new,
	remmina_file_get_filename,
	remmina_file_set_string,
	remmina_file_get_string,
	remmina_file_get_secret,
	remmina_file_set_int,
	remmina_file_get_int,
	remmina_file_get_double,
	remmina_file_unsave_passwords,

	remmina_pref_set_value,
	remmina_pref_get_value,
	remmina_pref_get_scale_quality,
	remmina_pref_get_sshtunnel_port,
	remmina_pref_get_ssh_loglevel,
	remmina_pref_get_ssh_parseconfig,
	remmina_pref_keymap_get_table,
	remmina_pref_keymap_get_keyval,

	_remmina_info,
	_remmina_message,
	_remmina_debug,
	_remmina_warning,
	_remmina_audit,
	_remmina_error,
	_remmina_critical,
	remmina_log_print,
	remmina_log_printf,

	remmina_widget_pool_register,

	rcw_open_from_file_full,
	remmina_public_open_unix_sock,
	remmina_public_get_server_port,
	remmina_masterthread_exec_is_main_thread,
	remmina_gtksocket_available,
	remmina_protocol_widget_get_profile_remote_width,
	remmina_protocol_widget_get_profile_remote_height,
	remmina_protocol_widget_get_name,
	remmina_protocol_widget_get_width,
	remmina_protocol_widget_get_height,
	remmina_protocol_widget_set_width,
	remmina_protocol_widget_set_height,
	remmina_protocol_widget_get_current_scale_mode,
	remmina_protocol_widget_get_expand,
	remmina_protocol_widget_set_expand,
	remmina_protocol_widget_set_error,
	remmina_protocol_widget_has_error,
	remmina_protocol_widget_gtkviewport,
	remmina_protocol_widget_is_closed,
	remmina_protocol_widget_get_file,
	remmina_protocol_widget_panel_auth,
	remmina_protocol_widget_register_hostkey,
	remmina_protocol_widget_start_direct_tunnel,
	remmina_protocol_widget_start_reverse_tunnel,
	remmina_protocol_widget_send_keys_signals,
	remmina_protocol_widget_chat_receive,
	remmina_protocol_widget_panel_hide,
	remmina_protocol_widget_chat_open,
	remmina_protocol_widget_ssh_exec,
	remmina_protocol_widget_panel_show,
	remmina_protocol_widget_panel_show_retry,
	remmina_protocol_widget_start_xport_tunnel,
	remmina_protocol_widget_set_display,
	remmina_protocol_widget_signal_connection_closed,
	remmina_protocol_widget_signal_connection_opened,
	remmina_protocol_widget_update_align,
	remmina_protocol_widget_unlock_dynres,
	remmina_protocol_widget_desktop_resize,
	remmina_protocol_widget_panel_new_certificate,
	remmina_protocol_widget_panel_changed_certificate,
	remmina_protocol_widget_get_username,
	remmina_protocol_widget_get_password,
	remmina_protocol_widget_get_domain,
	remmina_protocol_widget_get_savepassword,
	remmina_protocol_widget_panel_authx509,
	remmina_protocol_widget_get_cacert,
	remmina_protocol_widget_get_cacrl,
	remmina_protocol_widget_get_clientcert,
	remmina_protocol_widget_get_clientkey,
	remmina_protocol_widget_save_cred,
	remmina_protocol_widget_panel_show_listen,
	remmina_widget_pool_register,
	rcw_open_from_file_full,
	remmina_main_show_dialog,
	remmina_main_get_window,
	remmina_unlock_new,
};

const char *get_filename_ext(const char *filename) {
	const char* last = strrchr(filename, '/');
    const char *dot = strrchr(last, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

static gint compare_secret_plugin_init_order(gconstpointer a, gconstpointer b)
{
	RemminaSecretPlugin *sa, *sb;

	sa = (RemminaSecretPlugin*)a;
	sb = (RemminaSecretPlugin*)b;

	if (sa->init_order > sb->init_order)
		return 1;
	else if (sa->init_order < sb->init_order)
		return -1;
	return 0;
}

gchar* remmina_plugin_manager_create_alt_plugin_dir()
{
	gchar *plugin_dir;
	plugin_dir = g_build_path("/", g_get_user_config_dir(), "remmina", "plugins", NULL);

	if (g_file_test(plugin_dir, G_FILE_TEST_IS_DIR)) {
		// Do nothing, directory already exists
	}
	else
	{
		gint result = g_mkdir_with_parents(plugin_dir, 0755);
		if (result != 0)
		{
			plugin_dir = NULL;
		}
	}
	return plugin_dir;
}


void remmina_plugin_manager_load_plugins(GPtrArray *plugin_dirs, int array_size)
{
	TRACE_CALL(__func__);
	GDir *dir;
	const gchar *name, *ptr;
	gchar *fullpath;
	RemminaPlugin *plugin;
	RemminaSecretPlugin *sp;
	guint i, j;
	GSList *secret_plugins;
	GSList *sple;
	GPtrArray *alternative_language_plugins;
	GPtrArray *loaded_plugins = g_ptr_array_new();
	alternative_language_plugins = g_ptr_array_new();
	char* plugin_dir = NULL;

	for (i = 0; i < array_size; i++){
		plugin_dir = g_ptr_array_remove_index(plugin_dirs, 0);
		if (plugin_dir){
			dir = g_dir_open(plugin_dir, 0, NULL);

			if (dir == NULL){
				continue;
			}
			while ((name = g_dir_read_name(dir)) != NULL) {
				if ((ptr = strrchr(name, '.')) == NULL)
					continue;
				ptr++;
				fullpath = g_strconcat(plugin_dir, "/", name, NULL);
				if (!remmina_plugin_manager_loader_supported(ptr)) {
					g_ptr_array_add(alternative_language_plugins, g_strconcat(plugin_dir, "/", name, NULL));
					g_free(fullpath);
					continue;
				}
				if (!g_ptr_array_find_with_equal_func(loaded_plugins, name, g_str_equal, NULL)){
					if (remmina_plugin_native_load(&remmina_plugin_manager_service, fullpath)){
						g_ptr_array_add(loaded_plugins, g_strdup(name));
					}
				}
				
				g_free(fullpath);
			}
			g_dir_close(dir);
		}
	}
	

	while (alternative_language_plugins->len > 0) {
		gboolean has_loaded = FALSE;
		gchar* name = (gchar*)g_ptr_array_remove_index(alternative_language_plugins, 0);
		const gchar* ext = get_filename_ext(name);
		char* filename = strrchr(name, '/');

		for (j = 0; j < remmina_plugin_table->len && !has_loaded; j++) {
			plugin = (RemminaPlugin*)g_ptr_array_index(remmina_plugin_table, j);
			if (plugin->type == REMMINA_PLUGIN_TYPE_LANGUAGE_WRAPPER) {
				RemminaLanguageWrapperPlugin* wrapper_plugin = (RemminaLanguageWrapperPlugin*)plugin;
				const gchar** supported_extention = wrapper_plugin->supported_extentions;
				while (*supported_extention) {
					if (g_str_equal(*supported_extention, ext)) {
						if (!g_ptr_array_find_with_equal_func(loaded_plugins, name, g_str_equal, NULL)){
							has_loaded = wrapper_plugin->load(wrapper_plugin, name);
							if (has_loaded) {
								if (filename){
									g_ptr_array_add(loaded_plugins, filename);
								}
								break;
							}
						}
					}
					supported_extention++;
				}
				if (has_loaded) break;
			}
		}

		if (!has_loaded) {
			g_print("%s: Skip unsupported file type '%s'\n", name, ext);
		}

		g_free(name);
	}

	/* Now all secret plugins needs to initialize, following their init_order.
	 * The 1st plugin which will initialize correctly will be
	 * the default remmina_secret_plugin */

	if (remmina_secret_plugin)
		g_print("Internal ERROR: remmina_secret_plugin must be null here\n");

	secret_plugins = NULL;
	for (i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaPlugin*)g_ptr_array_index(remmina_plugin_table, i);
		if (plugin->type == REMMINA_PLUGIN_TYPE_SECRET) {
			secret_plugins = g_slist_insert_sorted(secret_plugins, (gpointer)plugin, compare_secret_plugin_init_order);
		}
	}

	sple = secret_plugins;
	while(sple != NULL) {
		sp = (RemminaSecretPlugin*)sple->data;
		if (sp->init(sp)) {
			REMMINA_DEBUG("The %s secret plugin has been initialized and it will be your default secret plugin",
				sp->name);
			remmina_secret_plugin = sp;
			break;
		}
		sple = sple->next;
	}

	g_slist_free(secret_plugins);
	g_ptr_array_free(alternative_language_plugins, TRUE);
	g_ptr_array_free(loaded_plugins, TRUE);
}

void remmina_plugin_manager_init()
{
	TRACE_CALL(__func__);

	gchar* alternative_dir;

	remmina_plugin_table = g_ptr_array_new();
	remmina_available_plugin_table = g_ptr_array_new();
	GPtrArray *plugin_dirs = g_ptr_array_new();
	int array_size = 1;
	
	if (!g_module_supported()) {
		g_print("Dynamic loading of plugins is not supported on this platform!\n");
		return;
	}
	alternative_dir = remmina_plugin_manager_create_alt_plugin_dir();

	if(alternative_dir != NULL){
		g_ptr_array_add(plugin_dirs, alternative_dir);
		array_size += 1;
		
	}
	g_ptr_array_add(plugin_dirs, REMMINA_RUNTIME_PLUGINDIR);
	remmina_plugin_manager_load_plugins(plugin_dirs, array_size);


	if (alternative_dir){
		g_free(alternative_dir);
	}

	if (plugin_dirs != NULL) {
		g_ptr_array_free(plugin_dirs, TRUE);
	}
}

/*
 * Creates a JsonNode with all the installed plugin names and version
 *
 * @returns NULL if there is an error creating the json builder object. Otherwise, return the JsonNode object with the plugin data.
 *
 */
JsonNode *remmina_plugin_manager_get_installed_plugins()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	RemminaPlugin *plugin;
	guint i;

	b = json_builder_new();
	if(b != NULL)
	{
		json_builder_begin_object(b);

		for (i = 0; i < remmina_plugin_table->len; i++) {
			plugin = (RemminaPlugin*)g_ptr_array_index(remmina_plugin_table, i);
			json_builder_set_member_name(b, plugin->name);
			json_builder_add_string_value(b, plugin->version);
		}
		json_builder_end_object(b);
		r = json_builder_get_root(b);
		g_object_unref(b);

		return r;
	}
	else
	{
		return NULL;
	}
}

gboolean remmina_plugin_manager_is_python_wrapper_installed()
{
	gchar* name = "Python Wrapper";
	RemminaPlugin *plugin;

	for(int i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaPlugin*)g_ptr_array_index(remmina_plugin_table, i);
		if(g_strcmp0(plugin->name, name) == 0)
		{
			return TRUE;
		}
	}
	return FALSE;
}

/*
 * Creates a JsonNode with some environment data as well as the installed plugins
 *
 * @returns NULL if there is an error creating the json builder object. Otherwise, return the JsonNode object with the plugin data.
 *
 */
JsonNode *remmina_plugin_manager_plugin_stats_get_all()
{
	JsonNode *n;
	gchar *unenc_p, *enc_p, *uid;
	JsonGenerator *g;
	JsonBuilder *b_inner, *b_outer;

	b_outer = json_builder_new();
	if (b_outer == NULL) {
		g_object_unref(b_outer);
		return NULL;
	}
	json_builder_begin_object(b_outer);

	b_inner = json_builder_new();
	if(b_inner != NULL)
	{
		json_builder_begin_object(b_inner);

		//get architecture and python version to determine what plugins are compatible 
		n = remmina_info_stats_get_os_info();
		json_builder_set_member_name(b_inner, "OS_INFO");
		json_builder_add_value(b_inner, n);

		n = remmina_info_stats_get_python();
		json_builder_set_member_name(b_inner, "PYTHON");
		json_builder_add_value(b_inner, n);

		n = remmina_info_stats_get_uid();
		uid = g_strdup(json_node_get_string(n));
		json_builder_set_member_name(b_outer, "UID");
		json_builder_add_string_value(b_outer, uid);

		n = remmina_plugin_manager_get_installed_plugins();
		json_builder_set_member_name(b_inner, "PLUGINS");
		json_builder_add_value(b_inner, n);

		json_builder_end_object(b_inner);

		n = json_builder_get_root(b_inner);
		g_object_unref(b_inner);

		g = json_generator_new();
		json_generator_set_root(g, n);
		unenc_p = json_generator_to_data(g, NULL);


		enc_p = g_base64_encode((guchar *)unenc_p, strlen(unenc_p));
		if (enc_p == NULL) {
			g_object_unref(b_outer);
			g_object_unref(g);
			g_free(uid);
			g_free(unenc_p);
			return NULL;
		}


		json_builder_set_member_name(b_outer, "encdata");
		json_builder_add_string_value(b_outer, enc_p);

		if(remmina_plugin_manager_is_python_wrapper_installed() == TRUE) {
			json_builder_set_member_name(b_outer, "pywrapper_installed");
			json_builder_add_boolean_value(b_outer, TRUE);
		} else {
			json_builder_set_member_name(b_outer, "pywrapper_installed");
			json_builder_add_boolean_value(b_outer, FALSE);
		}

		json_builder_end_object(b_outer);
		n = json_builder_get_root(b_outer);
		g_object_unref(b_outer);

		g_free(enc_p);
		g_object_unref(g);
		g_free(unenc_p);
		g_free(uid);

		return n;
	}
	else
	{
		return NULL;
	}
}

gboolean remmina_plugin_manager_loader_supported(const char *filetype) {
	TRACE_CALL(__func__);
	return g_str_equal(G_MODULE_SUFFIX, filetype);
}

RemminaPlugin* remmina_plugin_manager_get_plugin(RemminaPluginType type, const gchar *name)
{
	TRACE_CALL(__func__);
	RemminaPlugin *plugin;
	guint i;

	for (i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaPlugin*)g_ptr_array_index(remmina_plugin_table, i);
		if (plugin->type == type && g_strcmp0(plugin->name, name) == 0) {
			return plugin;
		}
	}
	return NULL;
}

const gchar *remmina_plugin_manager_get_canonical_setting_name(const RemminaProtocolSetting* setting)
{
	if (setting->name == NULL) {
		if (setting->type == REMMINA_PROTOCOL_SETTING_TYPE_SERVER)
			return "server";
		if (setting->type == REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD)
			return "password";
		if (setting->type == REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION)
			return "resolution";
		if (setting->type == REMMINA_PROTOCOL_SETTING_TYPE_ASSISTANCE)
			return "assistance_mode";
		return "missing_setting_name_into_plugin_RemminaProtocolSetting";
	}
	return setting->name;
}

void remmina_plugin_manager_for_each_plugin(RemminaPluginType type, RemminaPluginFunc func, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaPlugin *plugin;
	guint i;

	for (i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaPlugin*)g_ptr_array_index(remmina_plugin_table, i);
		if (plugin->type == type) {
			func((gchar*)plugin->name, plugin, data);
		}
	}
}

/* A copy of remmina_plugin_manager_show and remmina_plugin_manager_show_for_each
 * This is because we want to print the list of plugins, and their versions, to the standard output
 * with the remmina command line option --full-version instead of using the plugins widget
 ** @todo Investigate to use only GListStore and than pass the elements to be shown to 2 separate
 *       functions
 * WARNING: GListStore is supported only from GLib 2.44 */
static gboolean remmina_plugin_manager_show_for_each_stdout(RemminaPlugin *plugin)
{
	TRACE_CALL(__func__);

	g_print("%-20s%-16s%-64s%-10s\n", plugin->name,
		_(remmina_plugin_type_name[plugin->type]),
		g_dgettext(plugin->domain, plugin->description),
		plugin->version);
	return FALSE;
}

void remmina_plugin_manager_show_stdout()
{
	TRACE_CALL(__func__);
	g_print("%-20s%-16s%-64s%-10s\n", "NAME", "TYPE", "DESCRIPTION", "PLUGIN AND LIBRARY VERSION");
	g_ptr_array_foreach(remmina_plugin_table, (GFunc)remmina_plugin_manager_show_for_each_stdout, NULL);
}

static gboolean remmina_plugin_manager_show_for_each(RemminaPlugin *plugin, GtkListStore *store)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, FALSE, 1, plugin->name, 2, _(remmina_plugin_type_name[plugin->type]), 3,
		g_dgettext(plugin->domain, plugin->description), 4, "Installed", 5, plugin->version, -1);
	
	return FALSE;
}

static gboolean remmina_plugin_manager_show_for_each_available(RemminaPlugin *plugin, GtkListStore *store)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;

	gtk_list_store_append(store, &iter);

	gtk_list_store_set(store, &iter, 0, FALSE, 1, plugin->name, 2, _(remmina_plugin_type_name[plugin->type]), 3,
		g_dgettext(plugin->domain, plugin->description), 4, "Available", 5, plugin->version, -1);
		
	return FALSE;
}

void result_dialog_cleanup(GtkDialog * dialog, gint response_id, gpointer user_data)
{
	TRACE_CALL(__func__);
	gtk_widget_destroy(GTK_WIDGET(dialog));

	gtk_widget_hide(remmina_plugin_signal_data->label);
	gtk_spinner_stop(GTK_SPINNER(remmina_plugin_signal_data->spinner));
	gtk_widget_set_visible(remmina_plugin_signal_data->spinner, FALSE);
}

void remmina_plugin_manager_download_result_dialog(GtkDialog * dialog, gchar * message)
{
	TRACE_CALL(__func__);
	GtkWidget *child_dialog, *content_area, *label;

	child_dialog = gtk_dialog_new_with_buttons(_("Plugin Download"), GTK_WINDOW(dialog), GTK_DIALOG_MODAL, _("_OK"), 1, NULL);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (child_dialog));
	label = gtk_label_new(message);
	remmina_plugin_signal_data->downloading = FALSE;

	gtk_window_set_default_size(GTK_WINDOW(child_dialog), 500, 50);
	gtk_container_add(GTK_CONTAINER (content_area), label);
	g_signal_connect(G_OBJECT(child_dialog), "response", G_CALLBACK(result_dialog_cleanup), NULL);
	gtk_widget_show_all(child_dialog);
}

void remmina_plugin_manager_toggle_checkbox(GtkCellRendererToggle * cell, gchar * path, GtkListStore * model)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;
	gboolean active;

	active = gtk_cell_renderer_toggle_get_active (cell);

	gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (model), &iter, path);
	if (active) {
		gtk_cell_renderer_set_alignment(GTK_CELL_RENDERER(cell), 0.5, 0.5);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, FALSE, -1);
	}
	else {
		gtk_cell_renderer_set_alignment(GTK_CELL_RENDERER(cell), 0.5, 0.5);
		gtk_list_store_set (GTK_LIST_STORE (model), &iter, 0, TRUE, -1);
	}

}

/*
 * Appends json objects that have been deserialized from the response string.
 *
 * @returns void but modifies @data_array by appending plugins to it. It is the caller's responsibility to free @data_array
 * and its underlying elements.
 */
void remmina_append_json_objects_from_response_str(JsonReader *reader, GArray* data_array)
{
	TRACE_CALL(__func__);

	int plugin_array_index = 0;

	while (json_reader_read_element(reader, plugin_array_index)) {
		if (plugin_array_index >= MAX_PLUGINS_ALLOWED_TO_DOWNLOAD) {
			break;
		}
		/* This is the current plugin we are building from the deserialized response string. */
		RemminaServerPluginResponse *current_plugin = g_malloc(sizeof(RemminaServerPluginResponse));
		if (current_plugin == NULL) {
			plugin_array_index += 1;
			continue;
		}

		json_reader_read_member(reader, "name");

		current_plugin->name = g_strdup(json_reader_get_string_value(reader));
		json_reader_end_member(reader);
		if (g_utf8_strlen(current_plugin->name, -1) > MAX_PLUGIN_NAME_SIZE) { // -1 indicates null terminated str.
			g_free(current_plugin);
			plugin_array_index += 1;
			continue; // continue attempting to deserialize the rest of the server response.
		}

		json_reader_read_member(reader, "version");
		current_plugin->version =  g_strdup(json_reader_get_string_value(reader));
		json_reader_end_member(reader);

		json_reader_read_member(reader, "filename");
		current_plugin->file_name = g_strdup(json_reader_get_string_value(reader));
		json_reader_end_member(reader);

		json_reader_read_member(reader, "data");
		const gchar* data = json_reader_get_string_value(reader);


		/* Because the data is in the form \'\bdata\' and we only want data, we must remove three characters
		and then add one more character at the end for NULL, which is (1 + total_length - 3). */
		current_plugin->data = g_malloc(1 + g_utf8_strlen(data, -1) - 3);
		if (current_plugin->data == NULL) {
			g_free(current_plugin);
			plugin_array_index += 1;
			continue;
		}

		/* remove \'\b and \' */
		g_strlcpy((char*)current_plugin->data, data + 2, g_utf8_strlen(data, -1) - 2);
		json_reader_end_member(reader);

		json_reader_read_member(reader, "signature");
		if (json_reader_get_string_value(reader) != NULL)
		{
			current_plugin->signature = (guchar*) g_strdup(json_reader_get_string_value(reader));
		}
		else
		{
			g_free(current_plugin);
			plugin_array_index += 1;
			continue; // continue attempting to deserialize the rest of the server response.
		}
		json_reader_end_member(reader);

		g_array_append_val(data_array, current_plugin);
		plugin_array_index += 1;

		json_reader_end_element(reader); // end inspection of this plugin element
	}

}

GFile* remmina_create_plugin_file(const gchar* plugin_name, const gchar* plugin_version)
{
	gchar file_name[MAX_PLUGIN_NAME_SIZE];
	gchar *plugin_dir;

	if (g_utf8_strlen(plugin_name, -1) > MAX_PLUGIN_NAME_SIZE) {
		return NULL;
	}


	if (plugin_version != NULL){
		plugin_dir = g_build_path("/", "/tmp", NULL);
		snprintf(file_name, MAX_PLUGIN_NAME_SIZE, "%s/%s_%s", plugin_dir, plugin_name, plugin_version);
	}
	else{
		plugin_dir = g_build_path("/", g_get_user_config_dir(), "remmina", "plugins", NULL);
		snprintf(file_name, MAX_PLUGIN_NAME_SIZE, "%s/%s", plugin_dir, plugin_name);
	}
	GFile* plugin_file = g_file_new_for_path(file_name);
	
	if (plugin_dir != NULL) {
		g_free(plugin_dir);
	}
	
	return plugin_file;
}

gchar* remmina_plugin_manager_get_selected_plugin_list_str(GArray *selected_plugins)
{
	gchar *plugin_list = NULL;

	for(guint i = 0; i < selected_plugins->len; i++)
	{
		gchar *plugin = g_array_index(selected_plugins, gchar*, i);

		if (i == 0)
		{
			plugin_list = g_strdup(plugin);
		}
		else
		{
			plugin_list = g_strconcat(plugin_list, ", ", plugin, NULL);
		}

	}
	return plugin_list;
}

gboolean remmina_attempt_to_write_plugin_data_to_disk(RemminaServerPluginResponse *plugin)
{
	gsize decoded_len = 0;
	guchar *decoded = g_base64_decode((gchar*) plugin->data, &decoded_len);

	if (decoded == NULL) {
		return FALSE;
	}

	gsize signature_len = 0;
	guchar * decoded_signature = g_base64_decode((gchar*) plugin->signature, &signature_len);
	if (decoded_signature == NULL) {
		g_free(decoded);
		return FALSE;
	}

	/* Create path write temporary plugin */
	GFile* plugin_file = remmina_create_plugin_file(plugin->file_name, plugin->version);
	if (plugin_file == NULL) {
		g_free(decoded);
		g_free(decoded_signature);
		return FALSE;
	}

	/* Convert the base64 into inflated memory */
	gsize bytes_converted = remmina_decompress_from_memory_to_file(decoded, (int) decoded_len, plugin_file);

	/* Don't attempt to write file if we can't inflate anything. */
	if (bytes_converted <= 0) {
		g_free(decoded);
		g_free(decoded_signature);
		g_file_delete(plugin_file, NULL, NULL);
		g_object_unref(plugin_file);
		return FALSE;
	}
	/* Verify the signature given to us by the server*/
	gboolean passed = remmina_verify_plugin_signature(decoded_signature, plugin_file, bytes_converted, signature_len); 

	if (!passed) {
		g_free(decoded);
		g_free(decoded_signature);
		g_file_delete(plugin_file, NULL, NULL);
		g_object_unref(plugin_file);
		return FALSE;
	}

	/* Create path to store plugin */
	GFile* plugin_file_final = remmina_create_plugin_file(plugin->file_name, NULL);
	if (plugin_file == NULL) {
		g_free(decoded);
		g_free(decoded_signature);
		g_file_delete(plugin_file, NULL, NULL);
		g_object_unref(plugin_file);
		return FALSE;
	}

	GError *error = NULL; // Error for testing
	GFileOutputStream *g_output_stream = g_file_replace(plugin_file_final, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error);
	if (error !=NULL) {
		g_free(decoded);
		g_free(decoded_signature);
		g_file_delete(plugin_file, NULL, NULL);
		g_object_unref(plugin_file);
		g_file_delete(plugin_file_final, NULL, NULL);
		g_object_unref(plugin_file_final);
		return FALSE;
	}

	GFileIOStream *tmp_file_stream = (GFileIOStream*)g_file_read(plugin_file, NULL, NULL);
	g_output_stream_splice((GOutputStream*)g_output_stream, (GInputStream*)tmp_file_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, NULL);
	GFileInfo* info = g_file_query_info(plugin_file_final, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
	gsize bytes_written = g_file_info_get_size(info);

	/* Delete temporary file */
	g_file_delete(plugin_file, NULL, NULL);

	if (bytes_written != bytes_converted) {
		g_object_unref(info);
		g_free(decoded);
		g_free(decoded_signature);
		g_file_delete(plugin_file_final, NULL, NULL);
		g_object_unref(plugin_file_final);
		return FALSE;
	}

	if (info != NULL) {
		g_object_unref(info);
	}
	g_free(decoded);
	g_free(decoded_signature);
	g_object_unref(g_output_stream);
	g_object_unref(plugin_file);
	return TRUE;
}


gboolean remmina_plugin_manager_parse_plugin_list(gpointer user_data)
{
	int index = 0;
	JsonReader* reader = (JsonReader*)user_data;
	if (!json_reader_read_element(reader, 0)) {
		return FALSE;
	}
	if (json_reader_read_member(reader, "LIST")){
		while (json_reader_read_element(reader, index)) {

			RemminaPlugin *available_plugin = g_malloc (sizeof(RemminaPlugin));

			json_reader_read_member(reader, "name");

			available_plugin->name = g_strdup(json_reader_get_string_value(reader));

			json_reader_end_member(reader);
			json_reader_read_member(reader, "description");

			available_plugin->description = g_strdup(json_reader_get_string_value(reader));

			json_reader_end_member(reader);
			json_reader_read_member(reader, "version");

			available_plugin->version = g_strdup(json_reader_get_string_value(reader));

			json_reader_end_member(reader);
			json_reader_read_member(reader, "type");

			const char *type = json_reader_get_string_value(reader);
			if (g_strcmp0(type, "protocol") == 0)
			{
				available_plugin->type=REMMINA_PLUGIN_TYPE_PROTOCOL;
			}
			else if(g_strcmp0(type, "entry") == 0)
			{
				available_plugin->type=REMMINA_PLUGIN_TYPE_ENTRY;
			}
			else if(g_strcmp0(type, "tool") == 0)
			{
				available_plugin->type=REMMINA_PLUGIN_TYPE_TOOL;
			}
			else if(g_strcmp0(type,"pref") == 0)
			{
				available_plugin->type=REMMINA_PLUGIN_TYPE_PREF;
			}
			else if(g_strcmp0(type, "secret") == 0)
			{
				available_plugin->type=REMMINA_PLUGIN_TYPE_SECRET;
			}
			else if(g_strcmp0(type, "language_wrapper") == 0)
			{
				available_plugin->type=REMMINA_PLUGIN_TYPE_LANGUAGE_WRAPPER;
			}
			else if(g_strcmp0(type, "file") == 0)
			{
				available_plugin->type=REMMINA_PLUGIN_TYPE_FILE;
			}
			else
			{
				available_plugin->type=REMMINA_PLUGIN_TYPE_PREF;
			}

			json_reader_end_member(reader);
			json_reader_read_member(reader, "domain");

			available_plugin->domain = g_strdup(json_reader_get_string_value(reader));

			json_reader_end_member(reader);

			if(remmina_plugin_manager_verify_duplicate_plugins(available_plugin) == FALSE)
			{
				g_ptr_array_add(remmina_available_plugin_table, available_plugin);
			}

			json_reader_end_element(reader);

			index = index + 1;

		}
	}
	json_reader_end_member(reader);
	json_reader_end_element(reader);
	g_object_unref(reader);
	return FALSE;
}



gboolean remmina_plugin_manager_download_plugins(gpointer user_data)
{
	int success_count = 0;
	guint i;
	JsonReader* reader = (JsonReader*)user_data;
	if (!json_reader_read_element(reader, 0)) {
		return FALSE;
	}

	GArray* data_array = g_array_new(FALSE, FALSE, sizeof(RemminaServerPluginResponse *));
	if (json_reader_read_member(reader, "PLUGINS")){
		remmina_append_json_objects_from_response_str(reader, data_array);

		for (i = 0; i < data_array->len; i+=1) {
			RemminaServerPluginResponse *plugin = g_array_index(data_array, RemminaServerPluginResponse *, i);
			if (remmina_attempt_to_write_plugin_data_to_disk(plugin)) {
				success_count += 1;
			}
		}

		if (remmina_plugin_window != NULL && remmina_plugin_signal_data != NULL && remmina_plugin_signal_data->downloading == TRUE){
			if(success_count == data_array->len)
			{
				remmina_plugin_manager_download_result_dialog(remmina_plugin_window, "Plugin download successful! Please reboot Remmina in order to apply changes.\n");
			}
			else if((success_count < data_array->len) && (success_count != 0))
			{
				remmina_plugin_manager_download_result_dialog(remmina_plugin_window, "Plugin download partially successful! Please reboot Remmina in order to apply changes.\n");
			}
			else
			{
				remmina_plugin_manager_download_result_dialog(remmina_plugin_window, "Plugin download failed.\n");
			}
			
		}

	}

	g_array_free(data_array, TRUE); //Passing in free_segment TRUE frees the underlying elements as well as the array. 
	json_reader_end_member(reader);
	json_reader_end_element(reader);
	g_object_unref(reader);

	return FALSE;
}


/*
 * Creates request to download selected plugins from the server
 *
 */
void remmina_plugin_manager_plugin_request(GArray *selected_plugins)
{
	TRACE_CALL(__func__);
	gchar *formdata, *encdata, *plugin_list, *uid;
	JsonGenerator *g;
	JsonObject *o;
	JsonBuilder *b;

	JsonNode *n = remmina_plugin_manager_plugin_stats_get_all();

	if (n == NULL || (o = json_node_get_object(n)) == NULL)
	{
		formdata = "{\"UID\":\"\"}";
	}
	else
	{
		b = json_builder_new();

		if(b != NULL)
		{
			json_builder_begin_object(b);

			uid = g_strdup(json_object_get_string_member(o, "UID"));

			json_builder_set_member_name(b, "UID");
			json_builder_add_string_value(b, uid);

			encdata = g_strdup(json_object_get_string_member(o, "encdata"));

			json_builder_set_member_name(b, "encdata");
			json_builder_add_string_value(b, encdata);

			plugin_list = remmina_plugin_manager_get_selected_plugin_list_str(selected_plugins);

			json_builder_set_member_name(b, "selected_plugins");
			json_builder_add_string_value(b, plugin_list);

			json_builder_set_member_name(b, "keyversion");
			json_builder_add_int_value(b, 1);

			json_builder_end_object(b);
			n = json_builder_get_root(b);
			g_object_unref(b);

			g = json_generator_new();
			json_generator_set_root(g, n);
			formdata = json_generator_to_data(g, NULL);

			g_object_unref(g);

			if(plugin_list != NULL)
			{
				g_free(plugin_list);
			}

			json_node_free(n);
			g_free(encdata);
			g_free(uid);
		}
		else
		{
			json_node_free(n);
			formdata = "{\"UID\":\"\"}";
		}
	}
	remmina_curl_compose_message(formdata, "POST", DOWNLOAD_URL, NULL);


}


gboolean remmina_plugin_manager_verify_duplicate_plugins(RemminaPlugin *available_plugin)
{
	RemminaPlugin *plugin;
	gsize i;
	for (i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaPlugin*)g_ptr_array_index(remmina_plugin_table, i);
		if ((g_strcmp0(plugin->name, available_plugin->name) == 0) && (g_strcmp0(plugin->version, available_plugin->version) == 0))
		{
			return TRUE;
		}
	}
	for (i = 0; i < remmina_available_plugin_table->len; i++) {
		plugin = (RemminaPlugin*)g_ptr_array_index(remmina_available_plugin_table, i);
		if ((g_strcmp0(plugin->name, available_plugin->name) == 0) && (g_strcmp0(plugin->version, available_plugin->version) == 0))
		{
			return TRUE;
		}
	}
	return FALSE;
}


void* remmina_plugin_manager_get_available_plugins()
{
	TRACE_CALL(__func__);

	gchar *formdata;
	JsonGenerator *g;

	JsonNode *n = remmina_plugin_manager_plugin_stats_get_all();

	if (n == NULL)
	{
		formdata = "{}";
	}
	else
	{
		g = json_generator_new();
		json_generator_set_root(g, n);
		formdata = json_generator_to_data(g, NULL);
		g_object_unref(g);
		json_node_free(n);
	}
	remmina_curl_compose_message(formdata, "POST", LIST_URL, NULL);
	return NULL;
}

void remmina_plugin_manager_on_response(GtkDialog * dialog, gint response_id, gpointer user_data)
{
	TRACE_CALL(__func__);
	SignalData *data = remmina_plugin_signal_data;

	GtkListStore *store = data->store;
	GtkWidget *label = data->label;
	GtkWidget *spinner = data->spinner;

	GtkTreeIter iter;
	gboolean found = TRUE;

	if (response_id == ON_DOWNLOAD){
		found = FALSE;
		data->downloading = TRUE;
		if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter)) {
			GArray* name_array = g_array_new(FALSE, FALSE, sizeof(gchar*));
			do {
				gboolean value;
				gchar *available, *name;
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &value, -1);
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 1, &name, -1);
				gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 4, &available, -1);

				if(value){
					if(g_strcmp0(available, "Installed") == 0){
						REMMINA_DEBUG("%s is already installed", name);
					}
					else{
						REMMINA_DEBUG("%s can be installed!", name);
						found = TRUE;
						gchar *c_name = g_strdup(name);

						g_array_append_val(name_array, c_name);

						gtk_widget_set_visible(spinner, TRUE);
						if (gtk_widget_get_visible(label)) {
							continue;
						} else {
							gtk_widget_show(label);
							gtk_spinner_start(GTK_SPINNER(spinner));
						}
					}
				}
				g_free(available);
				g_free(name);
			} while (gtk_tree_model_iter_next(GTK_TREE_MODEL(store), &iter));
			if (name_array->len > 0) {
				remmina_plugin_manager_plugin_request(name_array);
				g_array_free(name_array, TRUE); // free the array and element data

			}
		}
	}
	else{
		gtk_widget_destroy(GTK_WIDGET(dialog));
		remmina_plugin_window = NULL;
	}
	if (found == FALSE){
		remmina_plugin_manager_download_result_dialog(remmina_plugin_window, "No plugins selected for download\n");
	}
}

void remmina_plugin_manager_show(GtkWindow *parent)
{
	TRACE_CALL(__func__);
	GtkWidget *dialog;
	GtkWidget *scrolledwindow;
	GtkWidget *tree, *available_tree;
	GtkWidget *label = gtk_label_new("Downloading...");
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store, *available_store;
	SignalData* data = NULL;

	if (remmina_plugin_signal_data == NULL){
		data = malloc(sizeof(SignalData));
	}
	else{
		data = remmina_plugin_signal_data;

	}

	if (remmina_plugin_window == NULL){
		dialog = gtk_dialog_new_with_buttons(_("Plugins"), parent, GTK_DIALOG_DESTROY_WITH_PARENT, _("_Download"), 1, _("Close"), 2, NULL);
		GtkWidget *spinner = gtk_spinner_new();

		gtk_window_set_default_size(GTK_WINDOW(dialog), 800, 600);

		scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
		gtk_widget_show(scrolledwindow);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scrolledwindow, TRUE, TRUE, 0);

		tree = gtk_tree_view_new();
		available_tree = gtk_tree_view_new();
		GtkWidget* tree_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
		gtk_container_add(GTK_CONTAINER(scrolledwindow), tree_box);

		gtk_box_pack_end(GTK_BOX(tree_box), available_tree, TRUE, TRUE, 0);
		GtkLabel *label_available = (GtkLabel*)gtk_label_new("Available");
		GtkLabel *label_installed = (GtkLabel*)gtk_label_new("Installed");
		gtk_widget_set_halign(GTK_WIDGET(label_available), GTK_ALIGN_START);
		gtk_widget_set_halign(GTK_WIDGET(label_installed), GTK_ALIGN_START);
		gtk_box_pack_end(GTK_BOX(tree_box), GTK_WIDGET(label_available), TRUE, TRUE, 1);
		gtk_box_pack_end(GTK_BOX(tree_box), tree, TRUE, TRUE, 10);
		gtk_box_pack_end(GTK_BOX(tree_box), GTK_WIDGET(label_installed), TRUE, TRUE, 1);
		
		gtk_widget_show(tree);
		gtk_widget_show(available_tree);
		gtk_widget_show(tree_box);
		gtk_widget_show(GTK_WIDGET(label_available));
		gtk_widget_show(GTK_WIDGET(label_installed));

		store = gtk_list_store_new(6, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
		available_store = gtk_list_store_new(6, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

		g_ptr_array_foreach(remmina_plugin_table, (GFunc)remmina_plugin_manager_show_for_each, store);
		g_ptr_array_foreach(remmina_available_plugin_table, (GFunc)remmina_plugin_manager_show_for_each_available, available_store);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
		gtk_tree_view_set_model(GTK_TREE_VIEW(available_tree), GTK_TREE_MODEL(available_store));

		renderer = gtk_cell_renderer_toggle_new();
		column = gtk_tree_view_column_new_with_attributes(_("Install"), renderer, "active", 0, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(available_tree), column);
		g_signal_connect (renderer, "toggled", G_CALLBACK (remmina_plugin_manager_toggle_checkbox), available_store);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 1, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 1, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 0);
		gtk_tree_view_append_column(GTK_TREE_VIEW(available_tree), column);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, "text", 2, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 1);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, "text", 2, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 1);
		gtk_tree_view_append_column(GTK_TREE_VIEW(available_tree), column);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, "text", 3, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 2);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, "text", 3, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 2);
		gtk_tree_view_append_column(GTK_TREE_VIEW(available_tree), column);


		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Version"), renderer, "text", 5, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 4);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Version"), renderer, "text", 5, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, 4);
		gtk_tree_view_append_column(GTK_TREE_VIEW(available_tree), column);

		data->store = available_store;
		data->label = label;
		data->spinner = spinner;
		data->downloading = FALSE;

		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(remmina_plugin_manager_on_response), data);

		gtk_box_pack_end(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), spinner, FALSE, FALSE, 0);

	}
	else {
		return;
	}
	remmina_plugin_window = (GtkDialog*)dialog;
	remmina_plugin_signal_data = data;

	gtk_widget_show(GTK_WIDGET(dialog));
}

RemminaFilePlugin* remmina_plugin_manager_get_import_file_handler(const gchar *file)
{
	TRACE_CALL(__func__);
	RemminaFilePlugin *plugin;
	gsize i;

	for (i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaFilePlugin*)g_ptr_array_index(remmina_plugin_table, i);

		if (plugin->type != REMMINA_PLUGIN_TYPE_FILE)
			continue;

		if (plugin->import_test_func(plugin, file)) {
			return plugin;
		}
	}
	return NULL;
}

RemminaFilePlugin* remmina_plugin_manager_get_export_file_handler(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaFilePlugin *plugin;
	guint i;

	for (i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaFilePlugin*)g_ptr_array_index(remmina_plugin_table, i);
		if (plugin->type != REMMINA_PLUGIN_TYPE_FILE)
			continue;
		if (plugin->export_test_func(plugin, remminafile)) {
			return plugin;
		}
	}
	return NULL;
}

RemminaSecretPlugin* remmina_plugin_manager_get_secret_plugin(void)
{
	TRACE_CALL(__func__);
	return remmina_secret_plugin;
}

gboolean remmina_plugin_manager_query_feature_by_type(RemminaPluginType ptype, const gchar* name, RemminaProtocolFeatureType ftype)
{
	TRACE_CALL(__func__);
	const RemminaProtocolFeature *feature;
	RemminaProtocolPlugin* plugin;

	plugin = (RemminaProtocolPlugin*)remmina_plugin_manager_get_plugin(ptype, name);

	if (plugin == NULL) {
		return FALSE;
	}

	for (feature = plugin->features; feature && feature->type; feature++) {
		if (feature->type == ftype)
			return TRUE;
	}
	return FALSE;
}

gboolean remmina_plugin_manager_is_encrypted_setting(RemminaProtocolPlugin *pp, const char *setting)
{
	TRACE_CALL(__func__);
	GHashTable *pht;

	if (encrypted_settings_cache == NULL)
		return FALSE;

	if (!(pht = g_hash_table_lookup(encrypted_settings_cache, pp->name)))
		return FALSE;

	if (!g_hash_table_lookup(pht, setting))
		return FALSE;

	return TRUE;
}
