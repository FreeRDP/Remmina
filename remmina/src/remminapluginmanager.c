/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include "remminapublic.h"
#include "remminafile.h"
#include "remminapref.h"
#include "remminaprotocolwidget.h"
#include "remminalog.h"
#include "remminawidgetpool.h"
#include "remminaconnectionwindow.h"
#include "remminapluginmanager.h"

static GPtrArray* remmina_plugin_table = NULL;

static const gchar *remmina_plugin_type_name[] =
{
    N_("Protocol"),
    N_("Entry"),
    N_("File"),
    N_("Tool"),
    N_("Preference"),
    NULL
};

static gint
remmina_plugin_manager_compare_func (RemminaPlugin **a, RemminaPlugin **b)
{
    return g_strcmp0 ((*a)->name, (*b)->name);
}

static gboolean
remmina_plugin_manager_register_plugin (RemminaPlugin *plugin)
{
    g_ptr_array_add (remmina_plugin_table, plugin);
    g_ptr_array_sort (remmina_plugin_table, (GCompareFunc) remmina_plugin_manager_compare_func);
    g_print ("Remmina plugin %s (type=%s) registered.\n", plugin->name, _(remmina_plugin_type_name[plugin->type]));
    return TRUE;
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
    remmina_protocol_widget_init_get_username,
    remmina_protocol_widget_init_get_password,
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

    remmina_pref_set_value,
    remmina_pref_get_value,
    remmina_pref_get_scale_quality,
    remmina_pref_get_sshtunnel_port,
    remmina_pref_keymap_get_keyval,

    remmina_log_print,
    remmina_log_printf,

    remmina_widget_pool_register,

    remmina_connection_window_open_from_file_full,
    remmina_public_get_server_port
};

static void
remmina_plugin_manager_load_plugin (const gchar *name)
{
    GModule *module;
    RemminaPluginEntryFunc entry;

    module = g_module_open (name, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);
    if (!module)
    {
        g_print ("Failed to load plugin: %s.\n", name);
        return;
    }

    if (!g_module_symbol (module, "remmina_plugin_entry", (gpointer*) &entry))
    {
        g_print ("Failed to locate plugin entry: %s.\n", name);
        return;
    }

    if (!entry (&remmina_plugin_manager_service))
    {
        g_print ("Plugin entry returned false: %s.\n", name);
        return;
    }

    /* We don't close the module because we will need it throughout the process lifetime */
}

void
remmina_plugin_manager_init (void)
{
    GDir *dir;
    const gchar *name, *ptr;
    gchar *fullpath;

    remmina_plugin_table = g_ptr_array_new ();

    if (!g_module_supported ())
    {
        g_print ("Dynamic loading of plugins is not supported in this platform!\n");
        return;
    }

    dir = g_dir_open (REMMINA_PLUGINDIR, 0, NULL);
    if (dir == NULL) return;
    while ((name = g_dir_read_name (dir)) != NULL)
    {
        if ((ptr = strrchr (name, '.')) == NULL) continue;
        ptr++;
        if (g_strcmp0 (ptr, G_MODULE_SUFFIX) != 0) continue;
        fullpath = g_strdup_printf (REMMINA_PLUGINDIR "/%s", name);
        remmina_plugin_manager_load_plugin (fullpath);
        g_free (fullpath);
    }
    g_dir_close (dir);
}

RemminaPlugin*
remmina_plugin_manager_get_plugin (RemminaPluginType type, const gchar *name)
{
    RemminaPlugin *plugin;
    gint i;

    for (i = 0; i < remmina_plugin_table->len; i++)
    {
        plugin = (RemminaPlugin *) g_ptr_array_index (remmina_plugin_table, i);
        if (plugin->type == type && g_strcmp0 (plugin->name, name) == 0)
        {
            return plugin;
        }
    }
    g_print ("Plugin not found (type=%i, name=%s)\n", type, name);
    return NULL;
}

void
remmina_plugin_manager_for_each_plugin (RemminaPluginType type, RemminaPluginFunc func, gpointer data)
{
    RemminaPlugin *plugin;
    gint i;

    for (i = 0; i < remmina_plugin_table->len; i++)
    {
        plugin = (RemminaPlugin *) g_ptr_array_index (remmina_plugin_table, i);
        if (plugin->type == type)
        {
            func ((gchar *) plugin->name, plugin, data);
        }
    }
}

static gboolean
remmina_plugin_manager_show_for_each (RemminaPlugin *plugin, GtkListStore *store)
{
    GtkTreeIter iter;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        0, plugin->name,
        1, _(remmina_plugin_type_name[plugin->type]),
        2, plugin->description,
        -1);
    return FALSE;
}

void
remmina_plugin_manager_show (GtkWindow *parent)
{
    GtkWidget *dialog;
    GtkWidget *scrolledwindow;
    GtkWidget *tree;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GtkListStore *store;

    dialog = gtk_dialog_new_with_buttons (_("Plugins"), parent, GTK_DIALOG_MODAL,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
    g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (gtk_widget_destroy), dialog);
    gtk_window_set_default_size (GTK_WINDOW (dialog), 400, 300);

    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), scrolledwindow, TRUE, TRUE, 0);
    
    tree = gtk_tree_view_new ();
    gtk_container_add (GTK_CONTAINER (scrolledwindow), tree);
    gtk_widget_show (tree);

    store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    g_ptr_array_foreach (remmina_plugin_table, (GFunc) remmina_plugin_manager_show_for_each, store);
    gtk_tree_view_set_model (GTK_TREE_VIEW (tree), GTK_TREE_MODEL (store));

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Name"),
        renderer, "text", 0, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, 0);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Type"),
        renderer, "text", 1, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, 1);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Description"),
        renderer, "text", 2, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, 2);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    gtk_widget_show (dialog);
}

RemminaFilePlugin*
remmina_plugin_manager_get_import_file_handler (const gchar *file)
{
    RemminaFilePlugin *plugin;
    gint i;

    for (i = 0; i < remmina_plugin_table->len; i++)
    {
        plugin = (RemminaFilePlugin *) g_ptr_array_index (remmina_plugin_table, i);
        if (plugin->type != REMMINA_PLUGIN_TYPE_FILE) continue;
        if (plugin->import_test_func (file))
        {
            return plugin;
        }
    }
    return NULL;
}

RemminaFilePlugin*
remmina_plugin_manager_get_export_file_handler (RemminaFile *remminafile)
{
    RemminaFilePlugin *plugin;
    gint i;

    for (i = 0; i < remmina_plugin_table->len; i++)
    {
        plugin = (RemminaFilePlugin *) g_ptr_array_index (remmina_plugin_table, i);
        if (plugin->type != REMMINA_PLUGIN_TYPE_FILE) continue;
        if (plugin->export_test_func (remminafile))
        {
            return plugin;
        }
    }
    return NULL;
}

