/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee 
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
#include "remminassh.h"
#include "remminachatwindow.h"
#include "remminapluginmanager.h"
#include "remminaconnectionwindow.h"
#include "remminaprotocolwidget.h"

struct _RemminaProtocolWidgetPriv
{
    GtkWidget *init_dialog;

    RemminaFile *remmina_file;
    RemminaProtocolPlugin *plugin;

    gint width;
    gint height;
    gboolean scale;
    gboolean expand;

    gboolean has_error;
    gchar *error_message;
    RemminaSSHTunnel *ssh_tunnel;
    RemminaXPortTunnelInitFunc init_func;

    GtkWidget *chat_window;

    gboolean closed;

    GPtrArray *printers;
};

G_DEFINE_TYPE (RemminaProtocolWidget, remmina_protocol_widget, GTK_TYPE_EVENT_BOX)

enum {
    CONNECT_SIGNAL,
    DISCONNECT_SIGNAL,
    DESKTOP_RESIZE_SIGNAL,
    LAST_SIGNAL
};

typedef struct _RemminaProtocolWidgetSignalData
{
    RemminaProtocolWidget *gp;
    const gchar *signal_name;
} RemminaProtocolWidgetSignalData;

static guint remmina_protocol_widget_signals[LAST_SIGNAL] = { 0 };

static void
remmina_protocol_widget_class_init (RemminaProtocolWidgetClass *klass)
{
    remmina_protocol_widget_signals[CONNECT_SIGNAL] =
        g_signal_new ("connect",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaProtocolWidgetClass, connect),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
    remmina_protocol_widget_signals[DISCONNECT_SIGNAL] =
        g_signal_new ("disconnect",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaProtocolWidgetClass, disconnect),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
    remmina_protocol_widget_signals[DESKTOP_RESIZE_SIGNAL] =
        g_signal_new ("desktop-resize",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaProtocolWidgetClass, desktop_resize),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void
remmina_protocol_widget_init_cancel (RemminaInitDialog *dialog, gint response_id, RemminaProtocolWidget *gp)
{
    if ((response_id == GTK_RESPONSE_CANCEL || response_id == GTK_RESPONSE_DELETE_EVENT)
        && dialog->mode == REMMINA_INIT_MODE_CONNECTING)
    {
        remmina_protocol_widget_close_connection (gp);
    }
}

static void
remmina_protocol_widget_show_init_dialog (RemminaProtocolWidget *gp, const gchar *name)
{
    if (gp->priv->init_dialog)
    {
        gtk_widget_destroy (gp->priv->init_dialog);
    }
    gp->priv->init_dialog = remmina_init_dialog_new (_("Connecting to '%s'..."), (name ? name : "*"));
    g_signal_connect (G_OBJECT (gp->priv->init_dialog), "response", G_CALLBACK (remmina_protocol_widget_init_cancel), gp);
    gtk_widget_show (gp->priv->init_dialog);
}

static void
remmina_protocol_widget_hide_init_dialog (RemminaProtocolWidget *gp)
{
    if (gp->priv->init_dialog && GTK_IS_WIDGET (gp->priv->init_dialog))
    {
        gtk_widget_destroy (gp->priv->init_dialog);
    }
    gp->priv->init_dialog = NULL;
}

static void
remmina_protocol_widget_destroy (RemminaProtocolWidget *gp, gpointer data)
{
    remmina_protocol_widget_hide_init_dialog (gp);
    g_free (gp->priv->error_message);
    g_free (gp->priv);
}

static void
remmina_protocol_widget_connect (RemminaProtocolWidget *gp, gpointer data)
{
    remmina_protocol_widget_hide_init_dialog (gp);
}

static void
remmina_protocol_widget_disconnect (RemminaProtocolWidget *gp, gpointer data)
{
    remmina_protocol_widget_hide_init_dialog (gp);
}

void
remmina_protocol_widget_grab_focus (RemminaProtocolWidget *gp)
{
    GtkWidget *child;

    child = gtk_bin_get_child (GTK_BIN (gp));
    if (child)
    {
        GTK_WIDGET_SET_FLAGS (child, GTK_CAN_FOCUS);
        gtk_widget_grab_focus (child);
    }
}

static void
remmina_protocol_widget_init (RemminaProtocolWidget *gp)
{
    RemminaProtocolWidgetPriv *priv;

    priv = g_new0 (RemminaProtocolWidgetPriv, 1);
    gp->priv = priv;

    g_signal_connect (G_OBJECT (gp), "destroy", G_CALLBACK (remmina_protocol_widget_destroy), NULL);
    g_signal_connect (G_OBJECT (gp), "connect", G_CALLBACK (remmina_protocol_widget_connect), NULL);
    g_signal_connect (G_OBJECT (gp), "disconnect", G_CALLBACK (remmina_protocol_widget_disconnect), NULL);
}

void
remmina_protocol_widget_open_connection_real (GPtrArray *printers, gpointer data)
{
    RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET (data);
    RemminaProtocolPlugin *plugin;
    RemminaFile *remminafile = gp->priv->remmina_file;

    gp->priv->printers = printers;

    /* Locate the protocol plugin */
    plugin = remmina_plugin_manager_get_protocol_plugin (remminafile->protocol);
    if (!plugin || !plugin->init || !plugin->open_connection)
    {
        remmina_protocol_widget_set_error (gp, _("Protocol %s is not supported."), remminafile->protocol);
        remmina_protocol_widget_close_connection (gp);
        return;
    }

    plugin->init (gp);

    gp->priv->plugin = plugin;

    if (! plugin->open_connection (gp))
    {
        remmina_protocol_widget_close_connection (gp);
    }
}

void
remmina_protocol_widget_open_connection (RemminaProtocolWidget *gp, RemminaFile *remminafile)
{
    gp->priv->remmina_file = remminafile;

    /* Show "server" instead of "name" for quick connect */
    remmina_protocol_widget_show_init_dialog (gp, (remminafile->filename ? remminafile->name : remminafile->server));

    if (remminafile->shareprinter)
    {
        remmina_public_get_printers (remmina_protocol_widget_open_connection_real, gp);
    }
    else
    {
        remmina_protocol_widget_open_connection_real (NULL, gp);
    }
}

gboolean
remmina_protocol_widget_close_connection (RemminaProtocolWidget *gp)
{
    if (!GTK_IS_WIDGET (gp) || gp->priv->closed) return FALSE;
    gp->priv->closed = TRUE;

    if (gp->priv->printers)
    {
        g_ptr_array_free (gp->priv->printers, TRUE);
        gp->priv->printers = NULL;
    }

    if (gp->priv->chat_window)
    {
        gtk_widget_destroy (gp->priv->chat_window);
        gp->priv->chat_window = NULL;
    }

#ifdef HAVE_LIBSSH
    if (gp->priv->ssh_tunnel)
    {
        remmina_ssh_tunnel_free (gp->priv->ssh_tunnel);
        gp->priv->ssh_tunnel = NULL;
    }
#endif

    if (!gp->priv->plugin->close_connection)
    {
        remmina_protocol_widget_emit_signal (gp, "disconnect");
        return FALSE;
    }

    return gp->priv->plugin->close_connection (gp);
}

static gboolean
remmina_protocol_widget_emit_signal_timeout (gpointer user_data)
{
    RemminaProtocolWidgetSignalData *data = (RemminaProtocolWidgetSignalData*) user_data;
    g_signal_emit_by_name (G_OBJECT (data->gp), data->signal_name);
    g_free (data);
    return FALSE;
}

void
remmina_protocol_widget_emit_signal (RemminaProtocolWidget *gp, const gchar *signal_name)
{
    RemminaProtocolWidgetSignalData *data;

    data = g_new (RemminaProtocolWidgetSignalData, 1);
    data->gp = gp;
    data->signal_name = signal_name;
    TIMEOUT_ADD (0, remmina_protocol_widget_emit_signal_timeout, data);
}

gpointer
remmina_protocol_widget_query_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature)
{
    switch (feature)
    {
        case REMMINA_PROTOCOL_FEATURE_TOOL:
            return GINT_TO_POINTER (1);
#ifdef HAVE_LIBSSH
        case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
        case REMMINA_PROTOCOL_FEATURE_TOOL_SSHTERM:
            return (gp->priv->ssh_tunnel ? GINT_TO_POINTER (1) : NULL);
#endif
        default:
            return gp->priv->plugin->query_feature (gp, feature);
    }
}

void
remmina_protocol_widget_call_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature, const gpointer data)
{
    switch (feature)
    {
#ifdef HAVE_LIBSSH
        case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
            if (!gp->priv->ssh_tunnel) return;
            remmina_connection_window_open_from_file_with_data (
                remmina_file_dup_temp_protocol (gp->priv->remmina_file, "SFTP"), gp->priv->ssh_tunnel);
            break;

        case REMMINA_PROTOCOL_FEATURE_TOOL_SSHTERM:
            if (!gp->priv->ssh_tunnel) return;
            remmina_connection_window_open_from_file_with_data (
                remmina_file_dup_temp_protocol (gp->priv->remmina_file, "SSH"), gp->priv->ssh_tunnel);
            break;

#endif
        default:
            gp->priv->plugin->call_feature (gp, feature, data);
            break;
    }
}

#ifdef HAVE_LIBSSH
static gboolean
remmina_protocol_widget_init_tunnel (RemminaProtocolWidget *gp)
{
    RemminaSSHTunnel *tunnel;
    gint ret;

    /* Reuse existing SSH connection if it's reconnecting to destination */
    if (gp->priv->ssh_tunnel == NULL)
    {
        tunnel = remmina_ssh_tunnel_new_from_file (gp->priv->remmina_file);

        THREADS_ENTER
        remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
            _("Connecting to SSH server %s..."), REMMINA_SSH (tunnel)->server);
        THREADS_LEAVE

        if (!remmina_ssh_init_session (REMMINA_SSH (tunnel)))
        {
            remmina_protocol_widget_set_error (gp, REMMINA_SSH (tunnel)->error);
            remmina_ssh_tunnel_free (tunnel);
            return FALSE;
        }

        ret = remmina_ssh_auth_gui (REMMINA_SSH (tunnel), REMMINA_INIT_DIALOG (gp->priv->init_dialog), TRUE);
        if (ret <= 0)
        {
            if (ret == 0)
            {
                remmina_protocol_widget_set_error (gp, REMMINA_SSH (tunnel)->error);
            }
            remmina_ssh_tunnel_free (tunnel);
            return FALSE;
        }

        gp->priv->ssh_tunnel = tunnel;
    }

    THREADS_ENTER
    remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
        _("Connecting to %s through SSH tunnel..."), gp->priv->remmina_file->server);
    THREADS_LEAVE

    return TRUE;
}
#endif

gchar*
remmina_protocol_widget_start_direct_tunnel (RemminaProtocolWidget *gp, gint default_port)
{
    gchar *dest;

    if (!gp->priv->remmina_file->server || gp->priv->remmina_file->server[0] == '\0')
    {
        return g_strdup("");
    }

    if (strchr (gp->priv->remmina_file->server, ':') == NULL)
    {
        dest = g_strdup_printf ("%s:%i", gp->priv->remmina_file->server, default_port);
    }
    else
    {
        dest = g_strdup (gp->priv->remmina_file->server);
    }

#ifdef HAVE_LIBSSH
    if (!gp->priv->remmina_file->ssh_enabled)
    {
        return dest;
    }

    if (!remmina_protocol_widget_init_tunnel (gp))
    {
        g_free (dest);
        return NULL;
    }

    /* TODO: Provide an option to support connecting to loopback interface for easier configuration */
    /*if (gp->priv->remmina_file->ssh_server == NULL || gp->priv->remmina_file->ssh_server[0] == '\0')
    {
        ptr = strchr (dest, ':');
        if (ptr)
        {
            ptr = g_strdup_printf ("127.0.0.1:%s", ptr + 1);
            g_free (dest);
            dest = ptr;
        }
    }*/

    if (!remmina_ssh_tunnel_open (gp->priv->ssh_tunnel, dest, remmina_pref.sshtunnel_port))
    {
        g_free (dest);
        remmina_protocol_widget_set_error (gp, REMMINA_SSH (gp->priv->ssh_tunnel)->error);
        return NULL;
    }

    g_free (dest);
    return g_strdup_printf ("127.0.0.1:%i", remmina_pref.sshtunnel_port);

#else

    return dest;

#endif
}

gboolean
remmina_protocol_widget_ssh_exec (RemminaProtocolWidget *gp, const gchar *fmt, ...)
{
#ifdef HAVE_LIBSSH
    RemminaSSHTunnel *tunnel = gp->priv->ssh_tunnel;
    ssh_channel channel;
    gint status;
    gboolean ret = FALSE;
    gchar *cmd, *ptr;
    va_list args;

    if ((channel = channel_new (REMMINA_SSH (tunnel)->session)) == NULL)
    {
        return FALSE;
    }

    va_start (args, fmt);
    cmd = g_strdup_vprintf (fmt, args);
    va_end (args);

    if (channel_open_session (channel) == SSH_OK &&
        channel_request_exec (channel, cmd) == SSH_OK)
    {
        channel_send_eof (channel);
        status = channel_get_exit_status (channel);
        ptr = strchr (cmd, ' ');
        if (ptr) *ptr = '\0';
        switch (status)
        {
        case 0:
            ret = TRUE;
            break;
        case 127:
            remmina_ssh_set_application_error (REMMINA_SSH (tunnel),
                _("Command %s not found on SSH server"), cmd);
            break;
        default:
            remmina_ssh_set_application_error (REMMINA_SSH (tunnel),
                _("Command %s failed on SSH server (status = %i)."), cmd, status);
            break;
        }
    }
    g_free (cmd);
    channel_close (channel);
    channel_free (channel);
    return ret;

#else

    remmina_ssh_set_application_error (tunnel, "No SSH support");
    return FALSE;

#endif
}

#ifdef HAVE_LIBSSH
static gboolean
remmina_protocol_widget_tunnel_init_callback (RemminaSSHTunnel *tunnel, gpointer data)
{
    RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET (data);
    gchar *server;
    gint port;
    gboolean ret;

    remmina_public_get_server_port (gp->priv->remmina_file->server, 177, &server, &port);
    ret = gp->priv->init_func (gp, tunnel->remotedisplay, (tunnel->bindlocalhost ? "localhost" : server), port);
    g_free (server);

    return ret;
}

static gboolean
remmina_protocol_widget_tunnel_connect_callback (RemminaSSHTunnel *tunnel, gpointer data)
{
    return TRUE;
}

static gboolean
remmina_protocol_widget_tunnel_disconnect_callback (RemminaSSHTunnel *tunnel, gpointer data)
{
    RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET (data);

    if (REMMINA_SSH (tunnel)->error)
    {
        remmina_protocol_widget_set_error (gp, "%s", REMMINA_SSH (tunnel)->error);
    }
    IDLE_ADD ((GSourceFunc) remmina_protocol_widget_close_connection, gp);
    return TRUE;
}
#endif

gboolean
remmina_protocol_widget_start_xport_tunnel (RemminaProtocolWidget *gp, gint display,
    RemminaXPortTunnelInitFunc init_func)
{
#ifdef HAVE_LIBSSH
    gboolean bindlocalhost;
    gchar *server;

    if (!remmina_protocol_widget_init_tunnel (gp)) return FALSE;

    gp->priv->init_func = init_func;
    gp->priv->ssh_tunnel->init_func = remmina_protocol_widget_tunnel_init_callback;
    gp->priv->ssh_tunnel->connect_func = remmina_protocol_widget_tunnel_connect_callback;
    gp->priv->ssh_tunnel->disconnect_func = remmina_protocol_widget_tunnel_disconnect_callback;
    gp->priv->ssh_tunnel->callback_data = gp;

    remmina_public_get_server_port (gp->priv->remmina_file->server, 0, &server, NULL);
    bindlocalhost = (g_strcmp0 (REMMINA_SSH (gp->priv->ssh_tunnel)->server, server) == 0);
    g_free (server);

    if (!remmina_ssh_tunnel_xport (gp->priv->ssh_tunnel, display, bindlocalhost))
    {
        remmina_protocol_widget_set_error (gp, "Failed to open channel : %s",
            ssh_get_error (REMMINA_SSH (gp->priv->ssh_tunnel)->session));
        return FALSE;
    }

    return TRUE;

#else
    return FALSE;
#endif
}

GtkWidget*
remmina_protocol_widget_get_init_dialog (RemminaProtocolWidget *gp)
{
    return gp->priv->init_dialog;
}

gint
remmina_protocol_widget_get_width (RemminaProtocolWidget *gp)
{
    return gp->priv->width;
}

void
remmina_protocol_widget_set_width (RemminaProtocolWidget *gp, gint width)
{
    gp->priv->width = width;
}

gint
remmina_protocol_widget_get_height (RemminaProtocolWidget *gp)
{
    return gp->priv->height;
}

void
remmina_protocol_widget_set_height (RemminaProtocolWidget *gp, gint height)
{
    gp->priv->height = height;
}

gboolean
remmina_protocol_widget_get_scale (RemminaProtocolWidget *gp)
{
    return gp->priv->scale;
}

void
remmina_protocol_widget_set_scale (RemminaProtocolWidget *gp, gboolean scale)
{
    gp->priv->scale = scale;
}

gboolean
remmina_protocol_widget_get_expand (RemminaProtocolWidget *gp)
{
    return gp->priv->expand;
}

void
remmina_protocol_widget_set_expand (RemminaProtocolWidget *gp, gboolean expand)
{
    gp->priv->expand = expand;
}

gboolean
remmina_protocol_widget_has_error (RemminaProtocolWidget *gp)
{
    return gp->priv->has_error;
}

gchar*
remmina_protocol_widget_get_error_message (RemminaProtocolWidget *gp)
{
    return gp->priv->error_message;
}

void
remmina_protocol_widget_set_error (RemminaProtocolWidget *gp, const gchar *fmt, ...)
{
    va_list args;

    if (gp->priv->error_message) g_free (gp->priv->error_message);

    va_start (args, fmt);
    gp->priv->error_message = g_strdup_vprintf (fmt, args);
    va_end (args);

    gp->priv->has_error = TRUE;
}

gboolean
remmina_protocol_widget_is_closed (RemminaProtocolWidget *gp)
{
    return gp->priv->closed;
}

RemminaFile*
remmina_protocol_widget_get_file (RemminaProtocolWidget *gp)
{
    return gp->priv->remmina_file;
}

GPtrArray*
remmina_protocol_widget_get_printers (RemminaProtocolWidget *gp)
{
    return gp->priv->printers;
}

gint
remmina_protocol_widget_init_authpwd (RemminaProtocolWidget *gp, RemminaAuthpwdType authpwd_type)
{
    RemminaFile *remminafile = gp->priv->remmina_file;
    gchar *s;
    gint ret;

    switch (authpwd_type)
    {
    case REMMINA_AUTHPWD_TYPE_PROTOCOL:
        s = g_strdup_printf (_("%s Password"), remminafile->protocol);
        break;
    case REMMINA_AUTHPWD_TYPE_SSH_PWD:
        s = g_strdup (_("SSH Password"));
        break;
    case REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY:
        s = g_strdup (_("SSH Private Key Passphrase"));
        break;
    default:
        s = g_strdup (_("Password"));
        break;
    }
    ret = remmina_init_dialog_authpwd (REMMINA_INIT_DIALOG (gp->priv->init_dialog), s, 
        remminafile->filename != NULL &&
        authpwd_type != REMMINA_AUTHPWD_TYPE_SSH_PWD &&
        authpwd_type != REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY);
    g_free (s);

    return ret;
}

gint
remmina_protocol_widget_init_authuserpwd (RemminaProtocolWidget *gp)
{
    RemminaFile *remminafile = gp->priv->remmina_file;

    return remmina_init_dialog_authuserpwd (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
        remminafile->username, (remminafile->filename != NULL));
}

gchar*
remmina_protocol_widget_init_get_username (RemminaProtocolWidget *gp)
{
    return g_strdup (REMMINA_INIT_DIALOG (gp->priv->init_dialog)->username);
}

gchar*
remmina_protocol_widget_init_get_password (RemminaProtocolWidget *gp)
{
    return g_strdup (REMMINA_INIT_DIALOG (gp->priv->init_dialog)->password);
}

gint
remmina_protocol_widget_init_authx509 (RemminaProtocolWidget *gp)
{
    RemminaFile *remminafile = gp->priv->remmina_file;

    return remmina_init_dialog_authx509 (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
        remminafile->cacert, remminafile->cacrl, remminafile->clientcert, remminafile->clientkey);
}

gchar*
remmina_protocol_widget_init_get_cacert (RemminaProtocolWidget *gp)
{
    gchar *s;

    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->cacert;
    return (s && s[0] ? g_strdup (s) : NULL);
}

gchar*
remmina_protocol_widget_init_get_cacrl (RemminaProtocolWidget *gp)
{
    gchar *s;

    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->cacrl;
    return (s && s[0] ? g_strdup (s) : NULL);
}

gchar*
remmina_protocol_widget_init_get_clientcert (RemminaProtocolWidget *gp)
{
    gchar *s;

    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->clientcert;
    return (s && s[0] ? g_strdup (s) : NULL);
}

gchar*
remmina_protocol_widget_init_get_clientkey (RemminaProtocolWidget *gp)
{
    gchar *s;

    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->clientkey;
    return (s && s[0] ? g_strdup (s) : NULL);
}

void
remmina_protocol_widget_init_save_cred (RemminaProtocolWidget *gp)
{
    RemminaFile *remminafile = gp->priv->remmina_file;
    gchar *s;
    gboolean save = FALSE;

    /* Save user name and certificates if any; save the password if it's requested to do so */
    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->username;
    if (s && s[0])
    {
        if (remminafile->username) g_free (remminafile->username);
        remminafile->username = g_strdup (s);
        save = TRUE;
    }
    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->cacert;
    if (s && s[0])
    {
        if (remminafile->cacert) g_free (remminafile->cacert);
        remminafile->cacert = g_strdup (s);
        save = TRUE;
    }
    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->cacrl;
    if (s && s[0])
    {
        if (remminafile->cacrl) g_free (remminafile->cacrl);
        remminafile->cacrl = g_strdup (s);
        save = TRUE;
    }
    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->clientcert;
    if (s && s[0])
    {
        if (remminafile->clientcert) g_free (remminafile->clientcert);
        remminafile->clientcert = g_strdup (s);
        save = TRUE;
    }
    s = REMMINA_INIT_DIALOG (gp->priv->init_dialog)->clientkey;
    if (s && s[0])
    {
        if (remminafile->clientkey) g_free (remminafile->clientkey);
        remminafile->clientkey = g_strdup (s);
        save = TRUE;
    }
    if (REMMINA_INIT_DIALOG (gp->priv->init_dialog)->save_password)
    {
        if (remminafile->password) g_free (remminafile->password);
        remminafile->password =
            g_strdup (REMMINA_INIT_DIALOG (gp->priv->init_dialog)->password);
        save = TRUE;
    }
    if (save)
    {
        remmina_file_save (remminafile);
    }
}

void
remmina_protocol_widget_init_show_listen (RemminaProtocolWidget *gp, gint port)
{
    remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
        _("Listening on Port %i for an Incoming %s Connection..."),
        port, gp->priv->remmina_file->protocol);
}

void
remmina_protocol_widget_init_show_retry (RemminaProtocolWidget *gp)
{
    remmina_init_dialog_set_status_temp (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
        _("Authentication failed. Trying to reconnect..."));
}

static void
remmina_protocol_widget_chat_on_destroy (RemminaProtocolWidget *gp)
{
    gp->priv->chat_window = NULL;
}

void
remmina_protocol_widget_chat_open (RemminaProtocolWidget *gp, const gchar *name,
    void(*on_send)(RemminaProtocolWidget *gp, const gchar *text),
    void(*on_destroy)(RemminaProtocolWidget *gp))
{
    if (gp->priv->chat_window)
    {
        gtk_window_present (GTK_WINDOW (gp->priv->chat_window));
    }
    else
    {
        gp->priv->chat_window = remmina_chat_window_new (
            GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (gp))),
            name);
        g_signal_connect_swapped (G_OBJECT (gp->priv->chat_window), "send", G_CALLBACK (on_send), gp);
        g_signal_connect_swapped (G_OBJECT (gp->priv->chat_window), "destroy", G_CALLBACK (remmina_protocol_widget_chat_on_destroy), gp);
        g_signal_connect_swapped (G_OBJECT (gp->priv->chat_window), "destroy", G_CALLBACK (on_destroy), gp);
        gtk_widget_show (gp->priv->chat_window);
    }
}

void
remmina_protocol_widget_chat_close (RemminaProtocolWidget *gp)
{
    if (gp->priv->chat_window)
    {
        gtk_widget_destroy (gp->priv->chat_window);
    }
}

void
remmina_protocol_widget_chat_receive (RemminaProtocolWidget *gp, const gchar *text)
{
    if (gp->priv->chat_window)
    {
        remmina_chat_window_receive (REMMINA_CHAT_WINDOW (gp->priv->chat_window), _("Server"), text);
        gtk_window_present (GTK_WINDOW (gp->priv->chat_window));
    }
}

GtkWidget*
remmina_protocol_widget_new (void)
{
    return GTK_WIDGET (g_object_new (REMMINA_TYPE_PROTOCOL_WIDGET, NULL));
}

