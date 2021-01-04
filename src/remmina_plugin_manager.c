/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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
#include <gio/gio.h>
#include <string.h>

#include <gdk/gdkx.h>

#include "remmina_public.h"
#include "remmina_file_manager.h"
#include "remmina_pref.h"
#include "remmina_protocol_widget.h"
#include "remmina_log.h"
#include "remmina_widget_pool.h"
#include "rcw.h"
#include "remmina_plugin_manager.h"
#include "remmina_plugin_native.h"
#include "remmina_plugin_python.h"
#include "remmina_public.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"

static GPtrArray* remmina_plugin_table = NULL;

/* A GHashTable of GHashTables where to store the names of the encrypted settings */
static GHashTable *encrypted_settings_cache = NULL;

/* There can be only one secret plugin loaded */
static RemminaSecretPlugin *remmina_secret_plugin = NULL;

static const gchar *remmina_plugin_type_name[] =
{ N_("Protocol"), N_("Entry"), N_("File"), N_("Tool"), N_("Preference"), N_("Secret"), NULL };

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
		g_print("Remmina plugin %s (type=%s) has been registered, but is not yet initialized/activated. "
			"The initialization order is %d.\n", plugin->name,
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
	remmina_file_unsave_passwords,

	remmina_pref_set_value,
	remmina_pref_get_value,
	remmina_pref_get_scale_quality,
	remmina_pref_get_sshtunnel_port,
	remmina_pref_get_ssh_loglevel,
	remmina_pref_get_ssh_parseconfig,
	remmina_pref_keymap_get_keyval,

	_remmina_debug,
	remmina_log_print,
	remmina_log_printf,

	remmina_widget_pool_register,

	rcw_open_from_file_full,
	remmina_public_get_server_port,
	remmina_masterthread_exec_is_main_thread,
	remmina_gtksocket_available,
	remmina_protocol_widget_get_profile_remote_width,
	remmina_protocol_widget_get_profile_remote_height
};

const char *get_filename_ext(const char *filename) {
	const char* last = strrchr(filename, '/');
    const char *dot = strrchr(last, '.');
    if(!dot || dot == filename) return "";
    return dot + 1;
}

static void remmina_plugin_manager_load_plugin(const gchar *name)
{
	const char* ext = get_filename_ext(name);

	if (g_str_equal(G_MODULE_SUFFIX, ext)) {
		remmina_plugin_native_load(&remmina_plugin_manager_service, name);
	} else if (g_str_equal("py", ext)) {
		remmina_plugin_python_load(&remmina_plugin_manager_service, name);
	} else {
		g_print("%s: Skip unsupported file type '%s'\n", name, ext);
	}
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

void remmina_plugin_manager_init()
{
	TRACE_CALL(__func__);
	GDir *dir;
	const gchar *name, *ptr;
	gchar *fullpath;
	RemminaPlugin *plugin;
	RemminaSecretPlugin *sp;
	int i;
	GSList *secret_plugins;
	GSList *sple;

	remmina_plugin_table = g_ptr_array_new();
	remmina_plugin_python_init();
	
	if (!g_module_supported()) {
		g_print("Dynamic loading of plugins is not supported on this platform!\n");
		return;
	}

	g_print("Load modules from %s\n", REMMINA_RUNTIME_PLUGINDIR);
	dir = g_dir_open(REMMINA_RUNTIME_PLUGINDIR, 0, NULL);

	if (dir == NULL)
		return;
	while ((name = g_dir_read_name(dir)) != NULL) {
		if ((ptr = strrchr(name, '.')) == NULL)
			continue;
		ptr++;
		if (!remmina_plugin_manager_loader_supported(ptr))
			continue;
		fullpath = g_strdup_printf(REMMINA_RUNTIME_PLUGINDIR "/%s", name);
		remmina_plugin_manager_load_plugin(fullpath);
		g_free(fullpath);
	}
	g_dir_close(dir);

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
		if (sp->init()) {
			g_print("The %s secret plugin  has been initialized and it will be your default secret plugin\n",
				sp->name);
			remmina_secret_plugin = sp;
			break;
		}
		sple = sple->next;
	}

	g_slist_free(secret_plugins);
}

gboolean remmina_plugin_manager_loader_supported(const char *filetype) {
	TRACE_CALL(__func__);
	return g_str_equal("py", filetype) || g_str_equal(G_MODULE_SUFFIX, filetype);
}

RemminaPlugin* remmina_plugin_manager_get_plugin(RemminaPluginType type, const gchar *name)
{
	TRACE_CALL(__func__);
	RemminaPlugin *plugin;
	gint i;

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
		return "missing_setting_name_into_plugin_RemminaProtocolSetting";
	}
	return setting->name;
}

void remmina_plugin_manager_for_each_plugin(RemminaPluginType type, RemminaPluginFunc func, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaPlugin *plugin;
	gint i;

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
	gtk_list_store_set(store, &iter, 0, plugin->name, 1, _(remmina_plugin_type_name[plugin->type]), 2,
		g_dgettext(plugin->domain, plugin->description), 3, plugin->version, -1);
	return FALSE;
}

void remmina_plugin_manager_show(GtkWindow *parent)
{
	TRACE_CALL(__func__);
	GtkWidget *dialog;
	GtkWidget *scrolledwindow;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;

	dialog = gtk_dialog_new_with_buttons(_("Plugins"), parent, GTK_DIALOG_MODAL, _("_OK"), GTK_RESPONSE_ACCEPT, NULL);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), dialog);
	gtk_window_set_default_size(GTK_WINDOW(dialog), 500, 350);

	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scrolledwindow, TRUE, TRUE, 0);

	tree = gtk_tree_view_new();
	gtk_container_add(GTK_CONTAINER(scrolledwindow), tree);
	gtk_widget_show(tree);

	store = gtk_list_store_new(4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	g_ptr_array_foreach(remmina_plugin_table, (GFunc)remmina_plugin_manager_show_for_each, store);
	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", 0, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, "text", 1, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 1);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Description"), renderer, "text", 2, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 2);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Version"), renderer, "text", 3, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, 3);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	gtk_widget_show(dialog);
}

RemminaFilePlugin* remmina_plugin_manager_get_import_file_handler(const gchar *file)
{
	TRACE_CALL(__func__);
	RemminaFilePlugin *plugin;
	gint i;

	for (i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaFilePlugin*)g_ptr_array_index(remmina_plugin_table, i);

		if (plugin->type != REMMINA_PLUGIN_TYPE_FILE)
			continue;

		if (plugin->import_test_func(file)) {
			return plugin;
		}
	}
	return NULL;
}

RemminaFilePlugin* remmina_plugin_manager_get_export_file_handler(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaFilePlugin *plugin;
	gint i;

	for (i = 0; i < remmina_plugin_table->len; i++) {
		plugin = (RemminaFilePlugin*)g_ptr_array_index(remmina_plugin_table, i);
		if (plugin->type != REMMINA_PLUGIN_TYPE_FILE)
			continue;
		if (plugin->export_test_func(remminafile)) {
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
