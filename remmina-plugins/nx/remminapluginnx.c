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
#include "remminanxsession.h"

INCLUDE_GET_AVAILABLE_XDISPLAY

typedef struct _RemminaPluginNxData
{
    GtkWidget *socket;
    gint socket_id;
    GPid xephyr_pid;
    gint xephyr_display;
    gint output_fd;
    gint error_fd;

    pthread_t thread;

    RemminaNXSession *nx;
} RemminaPluginNxData;

static RemminaPluginService *remmina_plugin_service = NULL;

static void
remmina_plugin_nx_on_plug_added (GtkSocket *socket, RemminaProtocolWidget *gp)
{
}

static void
remmina_plugin_nx_on_plug_removed (GtkSocket *socket, RemminaProtocolWidget *gp)
{
    remmina_plugin_service->protocol_plugin_close_connection (gp);
}

static gboolean
remmina_plugin_nx_invoke_xephyr (RemminaProtocolWidget *gp)
{
    RemminaPluginNxData *gpdata;
    RemminaFile *remminafile;
    gchar *argv[50];
    gint argc;
    gint display;
    gint i;
    GError *error = NULL;
    gboolean ret;

    gpdata = (RemminaPluginNxData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    display = remmina_get_available_xdisplay ();
    if (display == 0)
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "Run out of available local X display number.");
        return FALSE;
    }

    argc = 0;
    argv[argc++] = g_strdup ("Xephyr");

    argv[argc++] = g_strdup_printf (":%i", display);

    argv[argc++] = g_strdup ("-parent");
    argv[argc++] = g_strdup_printf ("%i", gpdata->socket_id);

    if (remminafile->showcursor)
    {
        argv[argc++] = g_strdup ("-host-cursor");
    }
    argv[argc++] = NULL;

    ret = g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
        NULL, NULL, &gpdata->xephyr_pid, &error);
    for (i = 0; i < argc; i++) g_free (argv[i]);

    if (!ret)
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "%s", error->message);
        return FALSE;
    }

    gpdata->xephyr_display = display;

    return TRUE;
}

static gboolean
remmina_plugin_nx_start_session_real (RemminaProtocolWidget *gp, RemminaNXSession *nx)
{
    RemminaPluginNxData *gpdata;
    RemminaFile *remminafile;
    gchar *s1, *s2;
    gint port;
    gint ret;

    gpdata = (RemminaPluginNxData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    remmina_nx_session_set_encryption (nx, remminafile->disableencryption ? 0 : 1);

    s1 = g_strdup (remminafile->server);
    s2 = strrchr (s1, ':');
    if (s2)
    {
        *s2++ = '\0';
        port = atoi (s2);
    }
    else
    {
        port = 22;
    }

    ret = remmina_nx_session_open (nx, s1, port, NULL, "");
    g_free (s1);
    if (!ret) return FALSE;
g_print ("remmina_nx_session_open\n");

    THREADS_ENTER
    ret = remmina_plugin_service->protocol_plugin_init_authuserpwd (gp);
    THREADS_LEAVE

    if (ret != GTK_RESPONSE_OK) return FALSE;

    s1 = remmina_plugin_service->protocol_plugin_init_get_username (gp);
    s2 = remmina_plugin_service->protocol_plugin_init_get_password (gp);
    ret = remmina_nx_session_login (nx, s1, s2);
    g_free (s1);
    g_free (s2);
    if (!ret) return FALSE;
g_print ("remmina_nx_session_login\n");

    remmina_nx_session_add_parameter (nx, "session", "session");
    remmina_nx_session_add_parameter (nx, "type", "unix-gnome");
    remmina_nx_session_add_parameter (nx, "link", "adsl");
    remmina_nx_session_add_parameter (nx, "geometry", "%ix%i+188+118",
        remminafile->resolution_width, remminafile->resolution_height);
    remmina_nx_session_add_parameter (nx, "keyboard", "defkeymap");
    remmina_nx_session_add_parameter (nx, "kbtype", "pc102/defkeymap");
    remmina_nx_session_add_parameter (nx, "media", "0");
    remmina_nx_session_add_parameter (nx, "screeninfo", "%ix%ix%i+render",
        remminafile->resolution_width, remminafile->resolution_height,
        remminafile->colordepth ? remminafile->colordepth : 24);

    if (!remmina_nx_session_start (nx)) return FALSE;
g_print ("remmina_nx_session_start\n");

    if (!remmina_nx_session_tunnel_open (nx)) return FALSE;
g_print ("remmina_nx_session_tunnel_open\n");

    if (!remmina_nx_session_invoke_proxy (nx, gpdata->xephyr_display)) return FALSE;
g_print ("remmina_nx_session_invoke_proxy\n");

    return TRUE;
}

static gboolean
remmina_plugin_nx_start_session (RemminaProtocolWidget *gp)
{
    RemminaPluginNxData *gpdata;
    gboolean ret;
    const gchar *err;

    gpdata = (RemminaPluginNxData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    gpdata->nx = remmina_nx_session_new ();
    ret = remmina_plugin_nx_start_session_real (gp, gpdata->nx);
    if (!ret)
    {
        err = remmina_nx_session_get_error (gpdata->nx);
        if (err)
        {
            remmina_plugin_service->protocol_plugin_set_error (gp, "%s", err);
        }
    }
    
    return ret;
}

static gboolean
remmina_plugin_nx_main (RemminaProtocolWidget *gp)
{
    RemminaPluginNxData *gpdata;
    gboolean ret = TRUE;

    gpdata = (RemminaPluginNxData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    if (ret && !remmina_plugin_nx_invoke_xephyr (gp)) ret = FALSE;
    if (ret && !remmina_plugin_nx_start_session (gp)) ret = FALSE;

    if (ret) remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");

    gpdata->thread = 0;
    return ret;
}

static gpointer
remmina_plugin_nx_main_thread (gpointer data)
{
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

    CANCEL_ASYNC
    if (!remmina_plugin_nx_main ((RemminaProtocolWidget*) data))
    {
        IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, data);
    }
    return NULL;
}

static void
remmina_plugin_nx_init (RemminaProtocolWidget *gp)
{
    RemminaPluginNxData *gpdata;

    gpdata = g_new0 (RemminaPluginNxData, 1);
    g_object_set_data_full (G_OBJECT (gp), "plugin-data", gpdata, g_free);

    gpdata->socket = gtk_socket_new ();
    gtk_widget_show (gpdata->socket);
    g_signal_connect (G_OBJECT (gpdata->socket), "plug-added",
        G_CALLBACK (remmina_plugin_nx_on_plug_added), gp);
    g_signal_connect (G_OBJECT (gpdata->socket), "plug-removed",
        G_CALLBACK (remmina_plugin_nx_on_plug_removed), gp);
    gtk_container_add (GTK_CONTAINER (gp), gpdata->socket);
}

static gboolean
remmina_plugin_nx_open_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginNxData *gpdata;
    RemminaFile *remminafile;

    gpdata = (RemminaPluginNxData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    remmina_plugin_service->protocol_plugin_set_width (gp, remminafile->resolution_width);
    remmina_plugin_service->protocol_plugin_set_height (gp, remminafile->resolution_height);
    gtk_widget_set_size_request (GTK_WIDGET (gp), remminafile->resolution_width, remminafile->resolution_height);
    gpdata->socket_id = gtk_socket_get_id (GTK_SOCKET (gpdata->socket));

    if (pthread_create (&gpdata->thread, NULL, remmina_plugin_nx_main_thread, gp))
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

static gboolean
remmina_plugin_nx_close_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginNxData *gpdata;

    gpdata = (RemminaPluginNxData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    if (gpdata->thread)
    {
        pthread_cancel (gpdata->thread);
        if (gpdata->thread) pthread_join (gpdata->thread, NULL);
    }

    if (gpdata->xephyr_pid)
    {
        kill (gpdata->xephyr_pid, SIGTERM);
        g_spawn_close_pid (gpdata->xephyr_pid);
        gpdata->xephyr_pid = 0;
    }

    if (gpdata->nx)
    {
        remmina_nx_session_free (gpdata->nx);
        gpdata->nx = NULL;
    }

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");

    return FALSE;
}

static gpointer
remmina_plugin_nx_query_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature)
{
    return NULL;
}

static void
remmina_plugin_nx_call_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature, const gpointer data)
{
}

static const RemminaProtocolSetting remmina_plugin_nx_basic_settings[] =
{
    REMMINA_PROTOCOL_SETTING_SERVER,
    REMMINA_PROTOCOL_SETTING_USERNAME,
    REMMINA_PROTOCOL_SETTING_PASSWORD,
    REMMINA_PROTOCOL_SETTING_RESOLUTION,
    REMMINA_PROTOCOL_SETTING_COLORDEPTH,
    REMMINA_PROTOCOL_SETTING_CTL_CONCAT,
    REMMINA_PROTOCOL_SETTING_DISABLEENCRYPTION,
    REMMINA_PROTOCOL_SETTING_SHOWCURSOR,
    REMMINA_PROTOCOL_SETTING_CTL_CONCAT_END,
    REMMINA_PROTOCOL_SETTING_CTL_END
};

static RemminaProtocolPlugin remmina_plugin_nx =
{
    "NX",
    "NX Technology",
    "remmina-nx",
    "remmina-nx",
    NULL,
    (RemminaProtocolSetting*) remmina_plugin_nx_basic_settings,
    NULL,
    REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,

    remmina_plugin_nx_init,
    remmina_plugin_nx_open_connection,
    remmina_plugin_nx_close_connection,
    remmina_plugin_nx_query_feature,
    remmina_plugin_nx_call_feature
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry (RemminaPluginService *service)
{
    remmina_plugin_service = service;

    if (! service->register_protocol_plugin (&remmina_plugin_nx))
    {
        return FALSE;
    }

    return TRUE;
}

