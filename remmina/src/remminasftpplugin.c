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

#ifdef HAVE_LIBSSH

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remminapublic.h"
#include "remminasftpclient.h"
#include "remminapluginmanager.h"
#include "remminassh.h"
#include "remminaprotocolwidget.h"
#include "remminasftpplugin.h"

typedef struct _RemminaPluginSftpData
{
    GtkWidget *client;
    pthread_t thread;
} RemminaPluginSftpData;

static RemminaPluginService *remmina_plugin_service = NULL;

static gpointer
remmina_plugin_sftp_main_thread (gpointer data)
{
    RemminaProtocolWidget *gp = (RemminaProtocolWidget*) data;
    RemminaPluginSftpData *gpdata;
    RemminaFile *remminafile;
    RemminaSSH *ssh;
    RemminaSFTP *sftp = NULL;
    gboolean cont = FALSE;
    gint ret;

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    CANCEL_ASYNC

    gpdata = (RemminaPluginSftpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    ssh = g_object_get_data (G_OBJECT (gp), "user-data");
    if (ssh)
    {
        /* Create SFTP connection based on existing SSH session */
        sftp = remmina_sftp_new_from_ssh (ssh);
        if (remmina_ssh_init_session (REMMINA_SSH (sftp)) &&
            remmina_ssh_auth (REMMINA_SSH (sftp), NULL) > 0 &&
            remmina_sftp_open (sftp))
        {
            cont = TRUE;
        }
    }
    else
    {
        /* New SFTP connection */
        remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);
        g_free (remminafile->ssh_server);
        remminafile->ssh_server = g_strdup (remminafile->server);

        sftp = remmina_sftp_new_from_file (remminafile);
        while (1)
        {
            if (!remmina_ssh_init_session (REMMINA_SSH (sftp)))
            {
                remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (sftp)->error);
                break;
            }

            ret = remmina_ssh_auth_gui (REMMINA_SSH (sftp),
                REMMINA_INIT_DIALOG (remmina_protocol_widget_get_init_dialog (gp)), TRUE);
            if (ret == 0)
            {
                remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (sftp)->error);
            }
            if (ret <= 0) break;

            if (!remmina_sftp_open (sftp))
            {
                remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (sftp)->error);
                break;
            }

            cont = TRUE;
            break;
        }
    }
    if (!cont)
    {
        if (sftp) remmina_sftp_free (sftp);
        IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);
        return NULL;
    }

    remmina_sftp_client_open (REMMINA_SFTP_CLIENT (gpdata->client), sftp);

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");

    gpdata->thread = 0;
    return NULL;
}

static void
remmina_plugin_sftp_init (RemminaProtocolWidget *gp)
{
    RemminaPluginSftpData *gpdata;

    gpdata = g_new0 (RemminaPluginSftpData, 1);
    g_object_set_data_full (G_OBJECT (gp), "plugin-data", gpdata, g_free);

    gpdata->client = remmina_sftp_client_new ();
    gtk_widget_show (gpdata->client);
    gtk_container_add (GTK_CONTAINER (gp), gpdata->client);

    remmina_plugin_service->protocol_plugin_register_hostkey (gp, gpdata->client);
}

static gboolean
remmina_plugin_sftp_open_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginSftpData *gpdata;

    gpdata = (RemminaPluginSftpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    remmina_plugin_service->protocol_plugin_set_expand (gp, TRUE);
    remmina_plugin_service->protocol_plugin_set_width (gp, 640);
    remmina_plugin_service->protocol_plugin_set_height (gp, 480);

    if (pthread_create (&gpdata->thread, NULL, remmina_plugin_sftp_main_thread, gp))
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
    return TRUE;
}

static gboolean
remmina_plugin_sftp_close_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginSftpData *gpdata;

    gpdata = (RemminaPluginSftpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    if (gpdata->thread)
    {
        pthread_cancel (gpdata->thread);
        if (gpdata->thread) pthread_join (gpdata->thread, NULL);
    }

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
    return FALSE;
}

static gpointer
remmina_plugin_sftp_query_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature)
{
    return NULL;
}

static void
remmina_plugin_sftp_call_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature, const gpointer data)
{
}

static RemminaProtocolPlugin remmina_plugin_sftp =
{
    REMMINA_PLUGIN_TYPE_PROTOCOL,
    "SFTP",
    NULL,

    "remmina-sftp",
    "remmina-sftp",
    NULL,
    NULL,
    NULL,
    REMMINA_PROTOCOL_SSH_SETTING_SFTP,

    remmina_plugin_sftp_init,
    remmina_plugin_sftp_open_connection,
    remmina_plugin_sftp_close_connection,
    remmina_plugin_sftp_query_feature,
    remmina_plugin_sftp_call_feature
};

void
remmina_sftp_plugin_register (void)
{
    remmina_plugin_service = &remmina_plugin_manager_service;
    remmina_plugin_sftp.description = _("SFTP - Secure File Transfer");
    remmina_plugin_service->register_plugin ((RemminaPlugin *) &remmina_plugin_sftp);
}

#else
 
void
remmina_sftp_plugin_register (void)
{
}

#endif

