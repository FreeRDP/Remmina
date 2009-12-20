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

#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <vte/vte.h>
#include "remminapublic.h"
#include "remminapluginmanager.h"
#include "remminassh.h"
#include "remminaprotocolwidget.h"
#include "remminasshplugin.h"

typedef struct _RemminaPluginSshData
{
    RemminaSSHShell *shell;
    GtkWidget *vte;
    pthread_t thread;
} RemminaPluginSshData;

static RemminaPluginService *remmina_plugin_service = NULL;

static gpointer
remmina_plugin_ssh_main_thread (gpointer data)
{
    RemminaProtocolWidget *gp = (RemminaProtocolWidget*) data;
    RemminaPluginSshData *gpdata;
    RemminaFile *remminafile;
    RemminaSSH *ssh;
    RemminaSSHShell *shell = NULL;
    gboolean cont = FALSE;
    gint ret;
    gchar *charset;

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    CANCEL_ASYNC

    gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    ssh = g_object_get_data (G_OBJECT (gp), "user-data");
    if (ssh)
    {
        /* Create SSH Shell connection based on existing SSH session */
        shell = remmina_ssh_shell_new_from_ssh (ssh);
        if (remmina_ssh_init_session (REMMINA_SSH (shell)) &&
            remmina_ssh_auth (REMMINA_SSH (shell), NULL) > 0 &&
            remmina_ssh_shell_open (shell, (RemminaSSHExitFunc) remmina_plugin_service->protocol_plugin_close_connection, gp))
        {
            cont = TRUE;
        }
    }
    else
    {
        /* New SSH Shell connection */
        remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);
        g_free (remminafile->ssh_server);
        remminafile->ssh_server = g_strdup (remminafile->server);

        shell = remmina_ssh_shell_new_from_file (remminafile);
        while (1)
        {
            if (!remmina_ssh_init_session (REMMINA_SSH (shell)))
            {
                remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
                break;
            }

            ret = remmina_ssh_auth_gui (REMMINA_SSH (shell),
                REMMINA_INIT_DIALOG (remmina_protocol_widget_get_init_dialog (gp)), TRUE);
            if (ret == 0)
            {
                remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
            }
            if (ret <= 0) break;

            if (!remmina_ssh_shell_open (shell, (RemminaSSHExitFunc) remmina_plugin_service->protocol_plugin_close_connection, gp))
            {
                remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
                break;
            }

            cont = TRUE;
            break;
        }
    }
    if (!cont)
    {
        if (shell) remmina_ssh_shell_free (shell);
        IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);
        return NULL;
    }

    gpdata->shell = shell;

    charset = REMMINA_SSH (shell)->charset;

    THREADS_ENTER
    if (charset && charset[0] != '\0')
    {
        vte_terminal_set_encoding (VTE_TERMINAL (gpdata->vte), charset);
    }
    vte_terminal_set_pty (VTE_TERMINAL (gpdata->vte), shell->slave);
    THREADS_LEAVE

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");

    gpdata->thread = 0;
    return NULL;
}

static gboolean
remmina_plugin_ssh_on_focus_in (GtkWidget *widget, GdkEventFocus *event, RemminaProtocolWidget *gp)
{
    RemminaPluginSshData *gpdata;

    gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    gtk_widget_grab_focus (gpdata->vte);
    return TRUE;
}

static void
remmina_plugin_ssh_on_resize_window (VteTerminal *vte, guint x, guint y, RemminaProtocolWidget *gp)
{
g_print ("%i x %i\n", x, y);
}

static void
remmina_plugin_ssh_init (RemminaProtocolWidget *gp)
{
    RemminaPluginSshData *gpdata;
    GtkWidget *hbox;
    GtkWidget *vscrollbar;
    GtkWidget *vte;

    gpdata = g_new0 (RemminaPluginSshData, 1);
    g_object_set_data_full (G_OBJECT (gp), "plugin-data", gpdata, g_free);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_container_add (GTK_CONTAINER (gp), hbox);
    g_signal_connect (G_OBJECT (hbox), "focus-in-event", G_CALLBACK (remmina_plugin_ssh_on_focus_in), gp);

    vte = vte_terminal_new ();
    gtk_widget_show (vte);
    vte_terminal_set_size (VTE_TERMINAL (vte), 80, 25);
    vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (vte), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), vte, TRUE, TRUE, 0);
    g_signal_connect (G_OBJECT (vte), "resize-window", G_CALLBACK (remmina_plugin_ssh_on_resize_window), gp);
    gpdata->vte = vte;

    vscrollbar = gtk_vscrollbar_new (vte_terminal_get_adjustment (VTE_TERMINAL (vte)));
    gtk_widget_show (vscrollbar);
    gtk_box_pack_start (GTK_BOX (hbox), vscrollbar, FALSE, TRUE, 0);
}

static gboolean
remmina_plugin_ssh_open_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginSshData *gpdata;

    gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT (gp), "plugin-data");

    remmina_plugin_service->protocol_plugin_set_expand (gp, TRUE);
    remmina_plugin_service->protocol_plugin_set_width (gp, 640);
    remmina_plugin_service->protocol_plugin_set_height (gp, 480);

    if (pthread_create (&gpdata->thread, NULL, remmina_plugin_ssh_main_thread, gp))
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
remmina_plugin_ssh_close_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginSshData *gpdata;

    gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT (gp), "plugin-data");
    if (gpdata->thread)
    {
        pthread_cancel (gpdata->thread);
        if (gpdata->thread) pthread_join (gpdata->thread, NULL);
    }
    if (gpdata->shell)
    {
        remmina_ssh_shell_free (gpdata->shell);
        gpdata->shell = NULL;
    }

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
    return FALSE;
}

static gpointer
remmina_plugin_ssh_query_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature)
{
    return NULL;
}

static void
remmina_plugin_ssh_call_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature, const gpointer data)
{
}

static RemminaProtocolPlugin remmina_plugin_ssh =
{
    "SSH",
    N_("Secure Shell"),
    "utilities-terminal",
    "utilities-terminal",
    NULL,
    NULL,
    NULL,
    REMMINA_PROTOCOL_SSH_SETTING_SSH,

    remmina_plugin_ssh_init,
    remmina_plugin_ssh_open_connection,
    remmina_plugin_ssh_close_connection,
    remmina_plugin_ssh_query_feature,
    remmina_plugin_ssh_call_feature
};

void
remmina_ssh_plugin_register (void)
{
    remmina_plugin_service = &remmina_plugin_manager_service;
    remmina_plugin_service->register_protocol_plugin (&remmina_plugin_ssh);
}

#else
 
void
remmina_ssh_plugin_register (void)
{
}

#endif

