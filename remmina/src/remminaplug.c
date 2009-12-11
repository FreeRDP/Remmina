/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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
#include "config.h"
#include "remminapublic.h"
#include "remminapref.h"
#include "remminasftpwindow.h"
#include "remminaplug.h"

G_DEFINE_TYPE (RemminaPlug, remmina_plug, GTK_TYPE_EVENT_BOX)

enum {
    CONNECT_SIGNAL,
    DISCONNECT_SIGNAL,
    DESKTOP_RESIZE_SIGNAL,
    LAST_SIGNAL
};

typedef struct _RemminaPlugSignalData
{
    RemminaPlug *gp;
    const gchar *signal_name;
} RemminaPlugSignalData;

static guint remmina_plug_signals[LAST_SIGNAL] = { 0 };

static void
remmina_plug_class_init (RemminaPlugClass *klass)
{
    remmina_plug_signals[CONNECT_SIGNAL] =
        g_signal_new ("connect",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaPlugClass, connect),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
    remmina_plug_signals[DISCONNECT_SIGNAL] =
        g_signal_new ("disconnect",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaPlugClass, disconnect),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
    remmina_plug_signals[DESKTOP_RESIZE_SIGNAL] =
        g_signal_new ("desktop-resize",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaPlugClass, desktop_resize),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
remmina_plug_init_cancel (RemminaInitDialog *dialog, gint response_id, RemminaPlug *gp)
{
    if ((response_id == GTK_RESPONSE_CANCEL || response_id == GTK_RESPONSE_DELETE_EVENT)
        && dialog->mode == REMMINA_INIT_MODE_CONNECTING)
    {
        remmina_plug_close_connection (gp);
    }
}

static void
remmina_plug_show_init_dialog (RemminaPlug *gp, const gchar *name)
{
    if (gp->init_dialog)
    {
        gtk_widget_destroy (gp->init_dialog);
    }
    gp->init_dialog = remmina_init_dialog_new (_("Connecting to '%s'..."), (name ? name : "*"));
    g_signal_connect (G_OBJECT (gp->init_dialog), "response", G_CALLBACK (remmina_plug_init_cancel), gp);
    gtk_widget_show (gp->init_dialog);
}

static void
remmina_plug_hide_init_dialog (RemminaPlug *gp)
{
    if (gp->init_dialog && GTK_IS_WIDGET (gp->init_dialog))
    {
        gtk_widget_destroy (gp->init_dialog);
    }
    gp->init_dialog = NULL;
}

static void
remmina_plug_destroy (RemminaPlug *gp, gpointer data)
{
    remmina_plug_hide_init_dialog (gp);
}

static void
remmina_plug_connect (RemminaPlug *gp, gpointer data)
{
    remmina_plug_hide_init_dialog (gp);
}

static void
remmina_plug_disconnect (RemminaPlug *gp, gpointer data)
{
    remmina_plug_hide_init_dialog (gp);
}

void
remmina_plug_grab_focus (RemminaPlug *gp)
{
    GtkWidget *child;

    child = gtk_bin_get_child (GTK_BIN (gp));
    GTK_WIDGET_SET_FLAGS (child, GTK_CAN_FOCUS);
    gtk_widget_grab_focus (child);
}

static void
remmina_plug_init (RemminaPlug *gp)
{
    g_signal_connect (G_OBJECT (gp), "destroy", G_CALLBACK (remmina_plug_destroy), NULL);
    g_signal_connect (G_OBJECT (gp), "connect", G_CALLBACK (remmina_plug_connect), NULL);
    g_signal_connect (G_OBJECT (gp), "disconnect", G_CALLBACK (remmina_plug_disconnect), NULL);

    gp->init_dialog = NULL;
    gp->remmina_file = NULL;
    gp->has_error = FALSE;
    gp->width = 0;
    gp->height = 0;
    gp->scale = FALSE;
    gp->error_message[0] = '\0';
    gp->ssh_tunnel = NULL;
    gp->sftp_window = NULL;
    gp->closed = FALSE;
    gp->printers = NULL;
}

void
remmina_plug_open_connection_real (GPtrArray *printers, gpointer data)
{
    RemminaPlug *gp = REMMINA_PLUG (data);

    gp->printers = printers;

    if (! (* (REMMINA_PLUG_GET_CLASS (gp)->open_connection)) (gp))
    {
        remmina_plug_close_connection (gp);
    }
}

void
remmina_plug_open_connection (RemminaPlug *gp, RemminaFile *remminafile)
{
    gp->remmina_file = remminafile;

    /* Show "server" instead of "name" for quick connect */
    remmina_plug_show_init_dialog (gp, (remminafile->filename ? remminafile->name : remminafile->server));

    if (remminafile->shareprinter)
    {
        remmina_public_get_printers (remmina_plug_open_connection_real, gp);
    }
    else
    {
        remmina_plug_open_connection_real (NULL, gp);
    }
}

gboolean
remmina_plug_close_connection (RemminaPlug *gp)
{
    if (!GTK_IS_WIDGET (gp) || gp->closed) return FALSE;
    gp->closed = TRUE;

    if (gp->printers)
    {
        g_ptr_array_free (gp->printers, TRUE);
        gp->printers = NULL;
    }

#ifdef HAVE_LIBSSH
    if (gp->ssh_tunnel)
    {
        remmina_ssh_tunnel_free (gp->ssh_tunnel);
        gp->ssh_tunnel = NULL;
    }
#endif
    return (* (REMMINA_PLUG_GET_CLASS (gp)->close_connection)) (gp);
}

#ifdef HAVE_LIBSSH
static void
remmina_plug_sftp_window_destroy (GtkWidget *widget, RemminaPlug *gp)
{
    gp->sftp_window = NULL;
}
#endif

static gboolean
remmina_plug_emit_signal_timeout (gpointer user_data)
{
    RemminaPlugSignalData *data = (RemminaPlugSignalData*) user_data;
    g_signal_emit_by_name (G_OBJECT (data->gp), data->signal_name);
    g_free (data);
    return FALSE;
}

void
remmina_plug_emit_signal (RemminaPlug *gp, const gchar *signal_name)
{
    RemminaPlugSignalData *data;

    data = g_new (RemminaPlugSignalData, 1);
    data->gp = gp;
    data->signal_name = signal_name;
    TIMEOUT_ADD (0, remmina_plug_emit_signal_timeout, data);
}

gpointer
remmina_plug_query_feature (RemminaPlug *gp, RemminaPlugFeature feature)
{
    switch (feature)
    {
        case REMMINA_PLUG_FEATURE_TOOL:
            return GINT_TO_POINTER (1);
#ifdef HAVE_LIBSSH
        case REMMINA_PLUG_FEATURE_TOOL_SFTP:
        case REMMINA_PLUG_FEATURE_TOOL_SSHTERM:
            return (gp->remmina_file->ssh_enabled ? GINT_TO_POINTER (1) : NULL);
#endif
        default:
            return (* (REMMINA_PLUG_GET_CLASS (gp)->query_feature)) (gp, feature);
    }
}

void
remmina_plug_call_feature (RemminaPlug *gp, RemminaPlugFeature feature, const gpointer data)
{
    switch (feature)
    {
#ifdef HAVE_LIBSSH
        case REMMINA_PLUG_FEATURE_TOOL_SFTP:
            if (!gp->sftp_window)
            {
                RemminaSFTP *sftp;

                if (!gp->ssh_tunnel) return;

                sftp = remmina_sftp_new_from_ssh (REMMINA_SSH (gp->ssh_tunnel));
                gp->sftp_window = remmina_sftp_window_new_init (sftp);
                if (!gp->sftp_window) return;

                g_signal_connect (G_OBJECT (gp->sftp_window), "destroy",
                    G_CALLBACK (remmina_plug_sftp_window_destroy), gp);
            }
            gtk_window_present (GTK_WINDOW (gp->sftp_window));
            break;

        case REMMINA_PLUG_FEATURE_TOOL_SSHTERM:
            if (gp->ssh_tunnel)
            {
                RemminaSSHTerminal *term;
                GtkWidget *dialog;

                term = remmina_ssh_terminal_new_from_ssh (REMMINA_SSH (gp->ssh_tunnel));
                if (!remmina_ssh_terminal_open (term))
                {
                    dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (gp))),
                        GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                        (REMMINA_SSH (term))->error, NULL);
                    gtk_dialog_run (GTK_DIALOG (dialog));
                    gtk_widget_destroy (dialog);
                    remmina_ssh_free (REMMINA_SSH (term));
                    return;
                }
            }
            break;

#endif
        default:
            (* (REMMINA_PLUG_GET_CLASS (gp)->call_feature)) (gp, feature, data);
            break;
    }
}

#ifdef HAVE_LIBSSH
static gboolean
remmina_plug_init_tunnel (RemminaPlug *gp)
{
    RemminaSSHTunnel *tunnel;
    gint ret;

    /* Reuse existing SSH connection if it's reconnecting to destination */
    if (gp->ssh_tunnel == NULL)
    {
        tunnel = remmina_ssh_tunnel_new_from_file (gp->remmina_file);

        THREADS_ENTER
        remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->init_dialog),
            _("Connecting to SSH server %s..."), REMMINA_SSH (tunnel)->server);
        THREADS_LEAVE

        if (!remmina_ssh_init_session (REMMINA_SSH (tunnel)))
        {
            g_strlcpy (gp->error_message, REMMINA_SSH (tunnel)->error, MAX_ERROR_LENGTH);
            remmina_ssh_tunnel_free (tunnel);
            gp->has_error = TRUE;
            return FALSE;
        }

        ret = remmina_ssh_auth_gui (REMMINA_SSH (tunnel), REMMINA_INIT_DIALOG (gp->init_dialog), TRUE);
        if (ret <= 0)
        {
            if (ret == 0)
            {
                g_strlcpy (gp->error_message, REMMINA_SSH (tunnel)->error, MAX_ERROR_LENGTH);
                gp->has_error = TRUE;
            }
            remmina_ssh_tunnel_free (tunnel);
            return FALSE;
        }

        gp->ssh_tunnel = tunnel;
    }

    THREADS_ENTER
    remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->init_dialog),
        _("Connecting to %s through SSH tunnel..."), gp->remmina_file->server);
    THREADS_LEAVE

    return TRUE;
}
#endif

gchar*
remmina_plug_start_direct_tunnel (RemminaPlug *gp, gint default_port)
{
    gchar *dest;

    if (remmina_file_is_incoming (gp->remmina_file))
    {
        dest = g_strdup("");
    }
    else
    {
        if (strchr (gp->remmina_file->server, ':') == NULL)
        {
            dest = g_strdup_printf ("%s:%i", gp->remmina_file->server, default_port);
        }
        else
        {
            dest = g_strdup (gp->remmina_file->server);
        }
    }

#ifdef HAVE_LIBSSH
    if (!gp->remmina_file->ssh_enabled || remmina_file_is_incoming (gp->remmina_file))
    {
        return dest;
    }

    if (!remmina_plug_init_tunnel (gp))
    {
        g_free (dest);
        return NULL;
    }

    /* TODO: Provide an option to support connecting to loopback interface for easier configuration */
    /*if (gp->remmina_file->ssh_server == NULL || gp->remmina_file->ssh_server[0] == '\0')
    {
        ptr = strchr (dest, ':');
        if (ptr)
        {
            ptr = g_strdup_printf ("127.0.0.1:%s", ptr + 1);
            g_free (dest);
            dest = ptr;
        }
    }*/

    if (!remmina_ssh_tunnel_open (gp->ssh_tunnel, dest, remmina_pref.sshtunnel_port))
    {
        g_free (dest);
        g_strlcpy (gp->error_message, REMMINA_SSH (gp->ssh_tunnel)->error, MAX_ERROR_LENGTH);
        gp->has_error = TRUE;
        return NULL;
    }

    g_free (dest);
    return g_strdup_printf ("127.0.0.1:%i", remmina_pref.sshtunnel_port);

#else

    return dest;

#endif
}

gboolean
remmina_plug_start_xport_tunnel (RemminaPlug *gp, gint display,
    RemminaSSHTunnelCallback init_func,
    RemminaSSHTunnelCallback connect_func,
    RemminaSSHTunnelCallback disconnect_func,
    gpointer callback_data)
{
#ifdef HAVE_LIBSSH
    gboolean bindlocalhost;
    gchar *server;

    if (!remmina_plug_init_tunnel (gp)) return FALSE;

    gp->ssh_tunnel->init_func = init_func;
    gp->ssh_tunnel->connect_func = connect_func;
    gp->ssh_tunnel->disconnect_func = disconnect_func;
    gp->ssh_tunnel->callback_data = callback_data;

    remmina_public_get_server_port (gp->remmina_file->server, 0, &server, NULL);
    bindlocalhost = (g_strcmp0 (REMMINA_SSH (gp->ssh_tunnel)->server, server) == 0);
    g_free (server);

    if (!remmina_ssh_tunnel_xport (gp->ssh_tunnel, display, bindlocalhost))
    {
        g_snprintf (gp->error_message, MAX_ERROR_LENGTH,
            "Failed to open channel : %s", ssh_get_error (REMMINA_SSH (gp->ssh_tunnel)->session));
        gp->has_error = TRUE;
        return FALSE;
    }

    return TRUE;

#else
    return FALSE;
#endif
}

