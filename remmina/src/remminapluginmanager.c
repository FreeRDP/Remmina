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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include "config.h"
#include "remminafile.h"
#include "remminapref.h"
#include "remminaprotocolwidget.h"
#include "remminalog.h"
#include "remminapluginmanager.h"

static GTree* remmina_protocol_plugin_table = NULL;

static gboolean
remmina_plugin_manager_register_protocol_plugin (RemminaProtocolPlugin *plugin)
{
    g_tree_insert (remmina_protocol_plugin_table, plugin->protocol, plugin);
    return TRUE;
}

RemminaPluginService remmina_plugin_manager_service =
{
    remmina_plugin_manager_register_protocol_plugin,

    remmina_protocol_widget_get_width,
    remmina_protocol_widget_set_width,
    remmina_protocol_widget_get_height,
    remmina_protocol_widget_set_height,
    remmina_protocol_widget_get_scale,
    remmina_protocol_widget_set_scale,
    remmina_protocol_widget_get_expand,
    remmina_protocol_widget_set_expand,
    remmina_protocol_widget_has_error,
    remmina_protocol_widget_set_error,
    remmina_protocol_widget_is_closed,
    remmina_protocol_widget_get_file,
    remmina_protocol_widget_get_printers,
    remmina_protocol_widget_emit_signal,
    remmina_protocol_widget_start_direct_tunnel,
    remmina_protocol_widget_start_xport_tunnel,
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
    remmina_protocol_widget_ssh_exec,
    remmina_protocol_widget_chat_open,
    remmina_protocol_widget_chat_close,
    remmina_protocol_widget_chat_receive,

    remmina_pref_get_scale_quality,
    remmina_pref_get_sshtunnel_port,
    remmina_pref_keymap_get_keyval,

    remmina_log_print,
    remmina_log_printf
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

    remmina_protocol_plugin_table = g_tree_new ((GCompareFunc) g_strcmp0);

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

RemminaProtocolPlugin*
remmina_plugin_manager_get_protocol_plugin (const gchar *protocol)
{
    return g_tree_lookup (remmina_protocol_plugin_table, protocol);
}

void
remmina_plugin_manager_for_each_protocol (RemminaPluginFunc func, gpointer data)
{
    g_tree_foreach (remmina_protocol_plugin_table, (GTraverseFunc) func, data);
}

gchar*
remmina_plugin_manager_get_protocol_description (RemminaProtocolPlugin* plugin)
{
    return g_strdup_printf ("%s - %s", plugin->protocol, _(plugin->description));
}

/* Known plugin descriptions. For translation purpose only */
#ifdef __DO_NOT_COMPILE_ME__
N_("Windows Terminal Service")
N_("Virtual Network Computing")
N_("Incoming Connection")
N_("X Remote Session")
N_("NX Technology")
#endif

