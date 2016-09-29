/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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
#include <string.h>

#include <gdk/gdkx.h>

#include "remmina_public.h"
#include "remmina_file_manager.h"
#include "remmina_pref.h"
#include "remmina_protocol_widget.h"
#include "remmina_log.h"
#include "remmina_widget_pool.h"
#include "remmina_connection_window.h"
#include "remmina_plugin_manager.h"
#include "remmina_public.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"

static GPtrArray* remmina_plugin_table = NULL;

/* There can be only one secret plugin loaded */
static RemminaSecretPlugin *remmina_secret_plugin = NULL;

static const gchar *remmina_plugin_type_name[] =
{ N_("Protocol"), N_("Entry"), N_("File"), N_("Tool"), N_("Preference"), N_("Secret"), NULL };

static gint remmina_plugin_manager_compare_func(RemminaPlugin **a, RemminaPlugin **b)
{
	TRACE_CALL("remmina_plugin_manager_compare_func");
	return g_strcmp0((*a)->name, (*b)->name);
}

static gboolean remmina_plugin_manager_register_plugin(RemminaPlugin *plugin)
{
	TRACE_CALL("remmina_plugin_manager_register_plugin");
	if (plugin->type == REMMINA_PLUGIN_TYPE_SECRET)
	{
		if (remmina_secret_plugin)
		{
			g_print("Remmina plugin %s (type=%s) bypassed.\n", plugin->name,
			        _(remmina_plugin_type_name[plugin->type]));
			return FALSE;
		}
		remmina_secret_plugin = (RemminaSecretPlugin*) plugin;
	}
	g_ptr_array_add(remmina_plugin_table, plugin);
	g_ptr_array_sort(remmina_plugin_table, (GCompareFunc) remmina_plugin_manager_compare_func);
	/* g_print("Remmina plugin %s (type=%s) registered.\n", plugin->name, _(remmina_plugin_type_name[plugin->type])); */
	return TRUE;
}

static gboolean remmina_gtksocket_available()
{
	GdkDisplayManager* dm;
	GdkDisplay* d;
	gboolean available;

	dm = gdk_display_manager_get();
	d = gdk_display_manager_get_default_display(dm);
	available = FALSE;

#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(d))
	{
		/* GtkSocket support is available only under Xorg */
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
	remmina_protocol_widget_get_scale,
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
	remmina_protocol_widget_close_connection,
	remmina_protocol_widget_init_authpwd,
	remmina_protocol_widget_init_authuserpwd,
	remmina_protocol_widget_init_certificate,
	remmina_protocol_widget_changed_certificate,
	remmina_protocol_widget_init_get_username,
	remmina_protocol_widget_init_get_password,
	remmina_protocol_widget_init_get_domain,
	remmina_protocol_widget_init_get_savepassword,
	remmina_protocol_widget_init_authx509,
	remmina_protocol_widget_init_get_cacert,
	remmina_protocol_widget_init_get_cacrl,
	remmina_protocol_widget_init_get_clientcert,
	remmina_protocol_widget_init_get_clientkey,
	remmina_protocol_widget_init_save_cred,
	remmina_protocol_widget_init_show_listen,
	remmina_protocol_widget_init_show_retry,
	remmina_protocol_widget_init_show,
	remmina_protocol_widget_init_hide,
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
	remmina_file_unsave_password,

	remmina_pref_set_value,
	remmina_pref_get_value,
	remmina_pref_get_scale_quality,
	remmina_pref_get_sshtunnel_port,
	remmina_pref_get_ssh_loglevel,
	remmina_pref_get_ssh_parseconfig,
	remmina_pref_keymap_get_keyval,

	remmina_log_print,
	remmina_log_printf,

	remmina_widget_pool_register,

	remmina_connection_window_open_from_file_full,
	remmina_public_get_server_port,
	remmina_masterthread_exec_is_main_thread,
	remmina_gtksocket_available

};

static void remmina_plugin_manager_load_plugin(const gchar *name)
{
	TRACE_CALL("remmina_plugin_manager_load_plugin");
	GModule *module;
	RemminaPluginEntryFunc entry;

	module = g_module_open(name, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

	if (!module)
	{
		g_print("Failed to load plugin: %s.\n", name);
		g_print("Error: %s\n", g_module_error());
		return;
	}

	if (!g_module_symbol(module, "remmina_plugin_entry", (gpointer*) &entry))
	{
		g_print("Failed to locate plugin entry: %s.\n", name);
		return;
	}

	if (!entry(&remmina_plugin_manager_service))
	{
		g_print("Plugin entry returned false: %s.\n", name);
		return;
	}

	/* We don't close the module because we will need it throughout the process lifetime */
}

void remmina_plugin_manager_init(void)
{
	TRACE_CALL("remmina_plugin_manager_init");
	GDir *dir;
	const gchar *name, *ptr;
	gchar *fullpath;

	remmina_plugin_table = g_ptr_array_new();

	if (!g_module_supported())
	{
		g_print("Dynamic loading of plugins is not supported in this platform!\n");
		return;
	}

	dir = g_dir_open(REMMINA_PLUGINDIR, 0, NULL);
	if (dir == NULL)
		return;
	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if ((ptr = strrchr(name, '.')) == NULL)
			continue;
		ptr++;
		if (g_strcmp0(ptr, G_MODULE_SUFFIX) != 0)
			continue;
		fullpath = g_strdup_printf(REMMINA_PLUGINDIR "/%s", name);
		remmina_plugin_manager_load_plugin(fullpath);
		g_free(fullpath);
	}
	g_dir_close(dir);
}

RemminaPlugin* remmina_plugin_manager_get_plugin(RemminaPluginType type, const gchar *name)
{
	TRACE_CALL("remmina_plugin_manager_get_plugin");
	RemminaPlugin *plugin;
	gint i;

	for (i = 0; i < remmina_plugin_table->len; i++)
	{
		plugin = (RemminaPlugin *) g_ptr_array_index(remmina_plugin_table, i);
		if (plugin->type == type && g_strcmp0(plugin->name, name) == 0)
		{
			return plugin;
		}
	}
	return NULL;
}

void remmina_plugin_manager_for_each_plugin(RemminaPluginType type, RemminaPluginFunc func, gpointer data)
{
	TRACE_CALL("remmina_plugin_manager_for_each_plugin");
	RemminaPlugin *plugin;
	gint i;

	for (i = 0; i < remmina_plugin_table->len; i++)
	{
		plugin = (RemminaPlugin *) g_ptr_array_index(remmina_plugin_table, i);
		if (plugin->type == type)
		{
			func((gchar *) plugin->name, plugin, data);
		}
	}
}

static gboolean remmina_plugin_manager_show_for_each(RemminaPlugin *plugin, GtkListStore *store)
{
	TRACE_CALL("remmina_plugin_manager_show_for_each");
	GtkTreeIter iter;

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, plugin->name, 1, _(remmina_plugin_type_name[plugin->type]), 2,
	                   g_dgettext(plugin->domain, plugin->description), 3, plugin->version, -1);
	return FALSE;
}

void remmina_plugin_manager_show(GtkWindow *parent)
{
	TRACE_CALL("remmina_plugin_manager_show");
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
	g_ptr_array_foreach(remmina_plugin_table, (GFunc) remmina_plugin_manager_show_for_each, store);
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
	TRACE_CALL("remmina_plugin_manager_get_import_file_handler");
	RemminaFilePlugin *plugin;
	gint i;

	for (i = 0; i < remmina_plugin_table->len; i++)
	{
		plugin = (RemminaFilePlugin *) g_ptr_array_index(remmina_plugin_table, i);

		if (plugin->type != REMMINA_PLUGIN_TYPE_FILE)
			continue;

		if (plugin->import_test_func(file))
		{
			return plugin;
		}
	}
	return NULL;
}

RemminaFilePlugin* remmina_plugin_manager_get_export_file_handler(RemminaFile *remminafile)
{
	TRACE_CALL("remmina_plugin_manager_get_export_file_handler");
	RemminaFilePlugin *plugin;
	gint i;

	for (i = 0; i < remmina_plugin_table->len; i++)
	{
		plugin = (RemminaFilePlugin *) g_ptr_array_index(remmina_plugin_table, i);
		if (plugin->type != REMMINA_PLUGIN_TYPE_FILE)
			continue;
		if (plugin->export_test_func(remminafile))
		{
			return plugin;
		}
	}
	return NULL;
}

RemminaSecretPlugin* remmina_plugin_manager_get_secret_plugin(void)
{
	TRACE_CALL("remmina_plugin_manager_get_secret_plugin");
	return remmina_secret_plugin;
}

gboolean remmina_plugin_manager_query_feature_by_type(RemminaPluginType ptype, const gchar* name, RemminaProtocolFeatureType ftype)
{
	const RemminaProtocolFeature *feature;
	RemminaProtocolPlugin* plugin;

	plugin = (RemminaProtocolPlugin*) remmina_plugin_manager_get_plugin(ptype, name);

	if (plugin == NULL)
	{
		return FALSE;
	}

	for (feature = plugin->features; feature && feature->type; feature++)
	{
		if (feature->type == ftype)
			return TRUE;
	}
	return FALSE;
}

