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

#include "common/remminaplugincommon.h"

INCLUDE_GET_AVAILABLE_XDISPLAY

typedef struct _RemminaPluginXdmcpData
{
    GtkWidget *socket;
    gint socket_id;
    GPid pid;
    gint output_fd;
    gint error_fd;
    gint display;
    gboolean ready;

#ifdef HAVE_PTHREAD
    pthread_t thread;
#else
    gint thread;
#endif
} RemminaPluginXdmcpData;

static RemminaPluginService *remmina_plugin_service = NULL;

static void
remmina_plugin_xdmcp_on_plug_added (GtkSocket *socket, RemminaProtocolWidget *gp)
{
    RemminaPluginXdmcpData *gpdata;

    gpdata = (RemminaPluginXdmcpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");
    gpdata->ready = TRUE;
}

static void
remmina_plugin_xdmcp_on_plug_removed (GtkSocket *socket, RemminaProtocolWidget *gp)
{
    remmina_plugin_service->protocol_plugin_close_connection (gp);
}

static gboolean
remmina_plugin_xdmcp_start_xephyr (RemminaProtocolWidget *gp)
{
    RemminaPluginXdmcpData *gpdata;
    RemminaFile *remminafile;
    gchar *argv[50];
    gint argc;
    gchar *p1, *p2;
    gint i;
    GError *error = NULL;
    gboolean ret;

    gpdata = (RemminaPluginXdmcpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    gpdata->display = remmina_get_available_xdisplay ();
    if (gpdata->display == 0)
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "Run out of available local X display number.");
        return FALSE;
    }

    argc = 0;
    argv[argc++] = g_strdup ("Xephyr");

    argv[argc++] = g_strdup_printf (":%i", gpdata->display);

    argv[argc++] = g_strdup ("-parent");
    argv[argc++] = g_strdup_printf ("%i", gpdata->socket_id);

    /* All Xephyr version between 1.5.0 and 1.6.4 will break when -screen argument is specified with -parent.
     * It's not possible to support color depth if you have those Xephyr version. Please see this bug
     * http://bugs.freedesktop.org/show_bug.cgi?id=24144
     * As a workaround, a "Default" color depth will not add the -screen argument.    
     */
    if (remminafile->colordepth >= 8)
    {
        argv[argc++] = g_strdup ("-screen");
        argv[argc++] = g_strdup_printf ("%ix%ix%i",
            remminafile->resolution_width, remminafile->resolution_height, remminafile->colordepth);
    }

    if (remminafile->colordepth == 2)
    {
        argv[argc++] = g_strdup ("-grayscale");
    }

    if (remminafile->showcursor)
    {
        argv[argc++] = g_strdup ("-host-cursor");
    }
    if (remminafile->once)
    {
        argv[argc++] = g_strdup ("-once");
    }

    if (!remminafile->ssh_enabled)
    {
        argv[argc++] = g_strdup ("-query");
        p1 = g_strdup (remminafile->server);
        argv[argc++] = p1;
        p2 = g_strrstr (p1, ":");
        if (p2)
        {
            *p2++ = '\0';
            argv[argc++] = g_strdup ("-port");
            argv[argc++] = g_strdup (p2);
        }
    }
    else
    {
        /* When the connection is through an SSH tunnel, it connects back to local unix socket,
         * so for security we can disable tcp listening */
        argv[argc++] = g_strdup ("-nolisten");
        argv[argc++] = g_strdup ("tcp");

        /* FIXME: It's better to get the magic cookie back from xqproxy, then call xauth,
         * instead of disable access control */
        argv[argc++] = g_strdup ("-ac");
    }

    argv[argc++] = NULL;

    ret = g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
        NULL, NULL, &gpdata->pid, &error);
    for (i = 0; i < argc; i++) g_free (argv[i]);

    if (!ret)
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "%s", error->message);
        return FALSE;
    }

    return TRUE;
}

static gboolean
remmina_plugin_xdmcp_tunnel_init_callback (RemminaProtocolWidget *gp,
    gint remotedisplay, const gchar *server, gint port)
{
    RemminaPluginXdmcpData *gpdata;
    RemminaFile *remminafile;

    gpdata = (RemminaPluginXdmcpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    if (!remmina_plugin_xdmcp_start_xephyr (gp)) return FALSE;
    while (!gpdata->ready) sleep (1);

    remmina_plugin_service->protocol_plugin_set_display (gp, gpdata->display);

    if (remminafile->exec && remminafile->exec[0])
    {
        return remmina_plugin_service->protocol_plugin_ssh_exec (gp, FALSE,
            "DISPLAY=localhost:%i.0 %s", remotedisplay, remminafile->exec);
    }
    else
    {
        return remmina_plugin_service->protocol_plugin_ssh_exec (gp, TRUE,
            "xqproxy -display %i -host %s -port %i -query -manage",
            remotedisplay, server, port);
    }
}

static gboolean
remmina_plugin_xdmcp_main (RemminaProtocolWidget *gp)
{
    RemminaPluginXdmcpData *gpdata;
    RemminaFile *remminafile;

    gpdata = (RemminaPluginXdmcpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    if (remminafile->ssh_enabled)
    {
        if (!remmina_plugin_service->protocol_plugin_start_xport_tunnel (gp,
            remmina_plugin_xdmcp_tunnel_init_callback))
        {
            gpdata->thread = 0;
            return FALSE;
        }
    }
    else
    {
        if (!remmina_plugin_xdmcp_start_xephyr (gp))
        {
            gpdata->thread = 0;
            return FALSE;
        }
    }

    gpdata->thread = 0;
    return TRUE;
}

#ifdef HAVE_PTHREAD
static gpointer
remmina_plugin_xdmcp_main_thread (gpointer data)
{
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

    CANCEL_ASYNC
    if (!remmina_plugin_xdmcp_main ((RemminaProtocolWidget*) data))
    {
        IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, data);
    }
    return NULL;
}
#endif

static void
remmina_plugin_xdmcp_init (RemminaProtocolWidget *gp)
{
    RemminaPluginXdmcpData *gpdata;

    gpdata = g_new0 (RemminaPluginXdmcpData, 1);
    g_object_set_data_full (G_OBJECT (gp), "plugin-data", gpdata, g_free);

    gpdata->socket = gtk_socket_new ();
    remmina_plugin_service->protocol_plugin_register_hostkey (gp, gpdata->socket);
    gtk_widget_show (gpdata->socket);
    g_signal_connect (G_OBJECT (gpdata->socket), "plug-added",
        G_CALLBACK (remmina_plugin_xdmcp_on_plug_added), gp);
    g_signal_connect (G_OBJECT (gpdata->socket), "plug-removed",
        G_CALLBACK (remmina_plugin_xdmcp_on_plug_removed), gp);
    gtk_container_add (GTK_CONTAINER (gp), gpdata->socket);
}

static gboolean
remmina_plugin_xdmcp_open_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginXdmcpData *gpdata;
    RemminaFile *remminafile;

    gpdata = (RemminaPluginXdmcpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    remmina_plugin_service->protocol_plugin_set_width (gp, remminafile->resolution_width);
    remmina_plugin_service->protocol_plugin_set_height (gp, remminafile->resolution_height);
    gtk_widget_set_size_request (GTK_WIDGET (gp), remminafile->resolution_width, remminafile->resolution_height);
    gpdata->socket_id = gtk_socket_get_id (GTK_SOCKET (gpdata->socket));

#ifdef HAVE_PTHREAD

    if (remminafile->ssh_enabled)
    {
        if (pthread_create (&gpdata->thread, NULL, remmina_plugin_xdmcp_main_thread, gp))
        {
            remmina_plugin_service->protocol_plugin_set_error (gp,
                "Failed to initialize pthread. Falling back to non-thread mode...");
            gpdata->thread = 0;
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        return remmina_plugin_xdmcp_main (gp);
    }

#else

    return remmina_plugin_xdmcp_main (gp);

#endif
}

static gboolean
remmina_plugin_xdmcp_close_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginXdmcpData *gpdata;

    gpdata = (RemminaPluginXdmcpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

#ifdef HAVE_PTHREAD
    if (gpdata->thread)
    {
        pthread_cancel (gpdata->thread);
        if (gpdata->thread) pthread_join (gpdata->thread, NULL);
    }
#endif

    if (gpdata->pid)
    {
        kill (gpdata->pid, SIGTERM);
        g_spawn_close_pid (gpdata->pid);
        gpdata->pid = 0;
    }

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");

    return FALSE;
}

static gpointer
remmina_plugin_xdmcp_query_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature)
{
    return NULL;
}

static void
remmina_plugin_xdmcp_call_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature, const gpointer data)
{
}

static const RemminaProtocolSetting remmina_plugin_xdmcp_basic_settings[] =
{
    REMMINA_PROTOCOL_SETTING_SERVER,
    REMMINA_PROTOCOL_SETTING_RESOLUTION_FIXED,
    REMMINA_PROTOCOL_SETTING_COLORDEPTH2,
    REMMINA_PROTOCOL_SETTING_EXEC,
    REMMINA_PROTOCOL_SETTING_SHOWCURSOR_LOCAL,
    REMMINA_PROTOCOL_SETTING_ONCE,
    REMMINA_PROTOCOL_SETTING_CTL_END
};

static RemminaProtocolPlugin remmina_plugin_xdmcp =
{
    REMMINA_PLUGIN_TYPE_PROTOCOL,
    "XDMCP",
    "X Remote Session",

    "remmina-xdmcp",
    "remmina-xdmcp-ssh",
    NULL,
    remmina_plugin_xdmcp_basic_settings,
    NULL,
    REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,

    remmina_plugin_xdmcp_init,
    remmina_plugin_xdmcp_open_connection,
    remmina_plugin_xdmcp_close_connection,
    remmina_plugin_xdmcp_query_feature,
    remmina_plugin_xdmcp_call_feature
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry (RemminaPluginService *service)
{
    remmina_plugin_service = service;

    if (! service->register_plugin ((RemminaPlugin *) &remmina_plugin_xdmcp))
    {
        return FALSE;
    }

    return TRUE;
}

