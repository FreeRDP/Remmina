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

typedef struct _RemminaPluginRdpData
{
    GtkWidget *socket;
    gint socket_id;
    GPid pid;
    gint output_fd;
    gint error_fd;

#ifdef HAVE_PTHREAD
    pthread_t thread;
#else
    gint thread;
#endif
} RemminaPluginRdpData;

static RemminaPluginService *remmina_plugin_service = NULL;

static void
remmina_plugin_rdp_on_plug_added (GtkSocket *socket, RemminaProtocolWidget *gp)
{
    remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");
}

static void
remmina_plugin_rdp_on_plug_removed (GtkSocket *socket, RemminaProtocolWidget *gp)
{
}

static void
remmina_plugin_rdp_set_error (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    gchar buffer[2000];
    gint len;

    gpdata = (RemminaPluginRdpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    lseek (gpdata->error_fd, 0, SEEK_SET);
    len = read (gpdata->error_fd, buffer, sizeof (buffer) - 1);
    buffer[len] = '\0';

    remmina_plugin_service->protocol_plugin_set_error (gp, "%s", buffer);
}

static void
remmina_plugin_rdp_on_child_exit (GPid pid, gint status, gpointer data)
{
    RemminaProtocolWidget *gp = (RemminaProtocolWidget*) data;
    RemminaPluginRdpData *gpdata;

    gpdata = (RemminaPluginRdpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    g_spawn_close_pid (pid);

    if (status != 0 && status != SIGTERM)
    {
        remmina_plugin_rdp_set_error (gp);
    }
    close (gpdata->output_fd);
    close (gpdata->error_fd);

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
}

static gboolean
remmina_plugin_rdp_main (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;
    gchar *argv[50];
    gchar *host;
    gint argc;
    gint i;
    GPtrArray *printers_array;
    GString *printers;
    gchar *printername;
    gint advargc = 0;
    gchar **advargv = NULL;
    GError *error = NULL;
    gboolean ret;

    gpdata = (RemminaPluginRdpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    if (remminafile->arguments && remminafile->arguments[0] != '\0')
    {
        if (!g_shell_parse_argv (remminafile->arguments, &advargc, &advargv, &error))
        {
            g_print ("%s\n", error->message);
            /* We simply ignore argument error. */
            advargv = NULL;
        }
    }

    host = remmina_plugin_service->protocol_plugin_start_direct_tunnel (gp, 3389);
    if (host == NULL)
    {
        gpdata->thread = 0;
        return FALSE;
    }

    argc = 0;
    argv[argc++] = g_strdup ("rdesktop");

    if (advargv)
    {
        for (i = 0; i < advargc; i++)
        {
            argv[argc++] = g_strdup (advargv[i]);
        }
        g_strfreev (advargv);
    }

    if (remminafile->username && remminafile->username[0] != '\0')
    {
        argv[argc++] = g_strdup ("-u");
        argv[argc++] = g_strdup (remminafile->username);
    }

    if (remminafile->domain && remminafile->domain[0] != '\0')
    {
        argv[argc++] = g_strdup ("-d");
        argv[argc++] = g_strdup (remminafile->domain);
    }

    if (remminafile->password && remminafile->password[0] != '\0')
    {
        argv[argc++] = g_strdup ("-p");
        argv[argc++] = g_strdup (remminafile->password);
    }

    if (remminafile->colordepth > 0)
    {
        argv[argc++] = g_strdup ("-a");
        argv[argc++] = g_strdup_printf ("%i", remminafile->colordepth);
    }

    if (remminafile->clientname && remminafile->clientname[0] != '\0')
    {
        argv[argc++] = g_strdup ("-n");
        argv[argc++] = g_strdup (remminafile->clientname);
    }

    if (remminafile->keymap && remminafile->keymap[0] != '\0')
    {
        argv[argc++] = g_strdup ("-k");
        argv[argc++] = g_strdup (remminafile->keymap);
    }

    /* rdesktop should not grab keyboard when embed into Remmina. It's up to Remmina users to
     * decide whether to grab keyboard for rdesktop. */
    argv[argc++] = g_strdup ("-K");

    if (remminafile->bitmapcaching)
    {
        argv[argc++] = g_strdup ("-P");
    }

    if (remminafile->compression)
    {
        argv[argc++] = g_strdup ("-z");
    }

    if (remminafile->console)
    {
        argv[argc++] = g_strdup ("-0");
    }

    if (remminafile->exec && remminafile->exec[0] != '\0')
    {
        argv[argc++] = g_strdup ("-s");
        argv[argc++] = g_strdup (remminafile->exec);
    }

    if (remminafile->execpath && remminafile->execpath[0] != '\0')
    {
        argv[argc++] = g_strdup ("-c");
        argv[argc++] = g_strdup (remminafile->execpath);
    }

    if (remminafile->sound && remminafile->sound[0] != '\0')
    {
        argv[argc++] = g_strdup ("-r");
        argv[argc++] = g_strdup_printf ("sound:%s", remminafile->sound);
    }

    printers_array = remmina_plugin_service->protocol_plugin_get_printers (gp);
    if (remminafile->shareprinter && printers_array && printers_array->len > 0)
    {
        printers = g_string_new ("");
        for (i = 0; i < printers_array->len; i++)
        {
            if (printers->len > 0)
            {
                g_string_append_c (printers, ',');
            }
            printername = (gchar*) g_ptr_array_index (printers_array, i);
            /* There's a bug in rdesktop causing it to crash if the printer name contains space.
             * This is the workaround and should be removed if it's fixed in rdesktop
             */
            if (strchr (printername, ' '))
            {
                g_string_append_c (printers, '"');
            }
            g_string_append (printers, printername);
            if (strchr (printername, ' '))
            {
                g_string_append_c (printers, '"');
            }
        }
        argv[argc++] = g_strdup ("-r");
        argv[argc++] = g_strdup_printf ("printer:%s", printers->str);
        g_string_free (printers, TRUE);
    }

    switch (remminafile->sharefolder)
    {
    case 1:
        argv[argc++] = g_strdup ("-r");
        argv[argc++] = g_strdup_printf ("disk:home=%s", g_get_home_dir ());
        break;
    case 2:
        argv[argc++] = g_strdup ("-r");
        argv[argc++] = g_strdup ("disk:root=/");
        break;
    }

    argv[argc++] = g_strdup ("-g");
    argv[argc++] = g_strdup_printf ("%ix%i", remminafile->resolution_width, remminafile->resolution_height);

    remmina_plugin_service->protocol_plugin_set_width (gp, remminafile->resolution_width);
    remmina_plugin_service->protocol_plugin_set_height (gp, remminafile->resolution_height);

    argv[argc++] = g_strdup ("-X");
    argv[argc++] = g_strdup_printf ("%i", gpdata->socket_id);

    argv[argc++] = host;
    argv[argc++] = NULL;

    ret = g_spawn_async_with_pipes (
        NULL, /* working_directory: take current */
        argv, /* argv[0] is the executable, parameters start from argv[1], end with NULL */
        NULL, /* envp: take the same as parent */
        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, /* flags */
        NULL, /* child_setup: function to be called before exec() */
        NULL, /* user_data: parameter for child_setup function */
        &gpdata->pid, /* child_pid */
        NULL, /* standard_input */
        &gpdata->output_fd, /* standard_output */
        &gpdata->error_fd, /* standard_error */
        &error);

    for (i = 0; i < argc; i++) g_free (argv[i]);

    if (ret)
    {
        g_child_watch_add (gpdata->pid, remmina_plugin_rdp_on_child_exit, gp);
        gpdata->thread = 0;
        return TRUE;
    }
    else
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "%s", error->message);
        gpdata->thread = 0;
        return FALSE;
    }
}

#ifdef HAVE_PTHREAD
static gpointer
remmina_plugin_rdp_main_thread (gpointer data)
{
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

    CANCEL_ASYNC
    if (!remmina_plugin_rdp_main ((RemminaProtocolWidget*) data))
    {
        IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, data);
    }
    return NULL;
}
#endif

static void
remmina_plugin_rdp_init (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = g_new0 (RemminaPluginRdpData, 1);
    g_object_set_data_full (G_OBJECT (gp), "plugin-data", gpdata, g_free);

    gpdata->socket = gtk_socket_new ();
    gtk_widget_show (gpdata->socket);
    g_signal_connect (G_OBJECT (gpdata->socket), "plug-added",
        G_CALLBACK (remmina_plugin_rdp_on_plug_added), gp);
    g_signal_connect (G_OBJECT (gpdata->socket), "plug-removed",
        G_CALLBACK (remmina_plugin_rdp_on_plug_removed), gp);
    gtk_container_add (GTK_CONTAINER (gp), gpdata->socket);
}

static gboolean
remmina_plugin_rdp_open_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;

    gpdata = (RemminaPluginRdpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    gtk_widget_set_size_request (GTK_WIDGET (gp), remminafile->resolution_width, remminafile->resolution_height);
    gpdata->socket_id = gtk_socket_get_id (GTK_SOCKET (gpdata->socket));

#ifdef HAVE_PTHREAD
    if (remminafile->ssh_enabled)
    {
        if (pthread_create (&gpdata->thread, NULL, remmina_plugin_rdp_main_thread, gp))
        {
            remmina_plugin_service->protocol_plugin_set_error (gp, "%s",
                "Failed to initialize pthread. Falling back to non-thread mode...");
            gpdata->thread = 0;
            return FALSE;
        }
        return TRUE;
    }
    else
    {
        return remmina_plugin_rdp_main (gp);
    }

#else

    return remmina_plugin_rdp_main (gp);

#endif
}

static gboolean
remmina_plugin_rdp_close_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = (RemminaPluginRdpData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

#ifdef HAVE_PTHREAD
    if (gpdata->thread)
    {
        pthread_cancel (gpdata->thread);
        if (gpdata->thread) pthread_join (gpdata->thread, NULL);
    }
#endif

    if (gpdata->pid)
    {
        /* If pid exists, "disconnect" signal will be emitted in the child_exit callback */
        kill (gpdata->pid, SIGTERM);
    }
    else
    {
        remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
    }
    return FALSE;
}

static gpointer
remmina_plugin_rdp_query_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature)
{
    return NULL;
}

static void
remmina_plugin_rdp_call_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature, const gpointer data)
{
}

static const RemminaProtocolSetting remmina_plugin_rdp_basic_settings[] =
{
    REMMINA_PROTOCOL_SETTING_SERVER,
    REMMINA_PROTOCOL_SETTING_USERNAME,
    REMMINA_PROTOCOL_SETTING_PASSWORD,
    REMMINA_PROTOCOL_SETTING_DOMAIN,
    REMMINA_PROTOCOL_SETTING_RESOLUTION,
    REMMINA_PROTOCOL_SETTING_COLORDEPTH,
    REMMINA_PROTOCOL_SETTING_SHAREFOLDER,
    REMMINA_PROTOCOL_SETTING_CTL_END
};

static const RemminaProtocolSetting remmina_plugin_rdp_advanced_settings[] =
{
    REMMINA_PROTOCOL_SETTING_SOUND,
    REMMINA_PROTOCOL_SETTING_KEYMAP,
    REMMINA_PROTOCOL_SETTING_CLIENTNAME,
    REMMINA_PROTOCOL_SETTING_EXEC,
    REMMINA_PROTOCOL_SETTING_EXECPATH,
    REMMINA_PROTOCOL_SETTING_SHAREPRINTER,
    REMMINA_PROTOCOL_SETTING_CONSOLE,
    REMMINA_PROTOCOL_SETTING_CTL_CONCAT,
    REMMINA_PROTOCOL_SETTING_BITMAPCACHING,
    REMMINA_PROTOCOL_SETTING_COMPRESSION,
    REMMINA_PROTOCOL_SETTING_CTL_CONCAT_END,
    REMMINA_PROTOCOL_SETTING_ARGUMENTS,
    REMMINA_PROTOCOL_SETTING_CTL_END
};

static RemminaProtocolPlugin remmina_plugin_rdp =
{
    "RDP",
    "Windows Terminal Service",
    "remmina-rdp",
    "remmina-rdp-ssh",
    NULL,
    remmina_plugin_rdp_basic_settings,
    remmina_plugin_rdp_advanced_settings,
    REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,

    remmina_plugin_rdp_init,
    remmina_plugin_rdp_open_connection,
    remmina_plugin_rdp_close_connection,
    remmina_plugin_rdp_query_feature,
    remmina_plugin_rdp_call_feature
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry (RemminaPluginService *service)
{
    remmina_plugin_service = service;

    if (! service->register_protocol_plugin (&remmina_plugin_rdp))
    {
        return FALSE;
    }

    return TRUE;
}

