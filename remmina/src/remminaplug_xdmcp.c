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
#include <unistd.h>
#include <signal.h>
#include "config.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#include "remminapublic.h"
#include "remminafile.h"
#include "remminaplug.h"
#include "remminassh.h"
#include "remminaplug_xdmcp.h"

G_DEFINE_TYPE (RemminaPlugXdmcp, remmina_plug_xdmcp, REMMINA_TYPE_PLUG)

static void
remmina_plug_xdmcp_plug_added (GtkSocket *socket, RemminaPlugXdmcp *gp_xdmcp)
{
}

static void
remmina_plug_xdmcp_plug_removed (GtkSocket *socket, RemminaPlugXdmcp *gp_xdmcp)
{
    remmina_plug_close_connection (REMMINA_PLUG (gp_xdmcp));
}

#ifdef HAVE_LIBSSH
static gboolean
remmina_plug_xdmcp_tunnel_init_callback (RemminaSSHTunnel *tunnel, gpointer data)
{
    RemminaPlug *gp = REMMINA_PLUG (data);
    ssh_channel channel;
    gchar *server;
    gint port;
    gchar *cmd;
    gint status;
    gboolean ret = FALSE;

    if ((channel = channel_new (REMMINA_SSH (tunnel)->session)) == NULL)
    {
        return FALSE;
    }

    /* If the SSH authentication happens too fast, for example an auto-pubkey authentication passes very quick,
     * the xqproxy might be executed before Xephyr finish initialize the X server unix socket. Then all
     * connections from the display manager will be forwarded into a black hole.
     * Sleep one second is a workaround for this race condition. but it is not guarantee... Probably the best
     * way is to check the unix socket.
     */
    sleep (1);

    remmina_public_get_server_port (gp->remmina_file->server, 177, &server, &port);
    cmd = g_strdup_printf ("xqproxy -display %i -host %s -port %i -query -manage",
        tunnel->remotedisplay, (tunnel->bindlocalhost ? "localhost" : server), port);
    g_free (server);
    if (channel_open_session (channel) == SSH_OK &&
        channel_request_exec (channel, cmd) == SSH_OK)
    {
        channel_send_eof (channel);
        status = channel_get_exit_status (channel);
        switch (status)
        {
        case 0:
            ret = TRUE;
            break;
        case 127:
            remmina_ssh_set_application_error (tunnel,
                _("Please install xqproxy on SSH server in order to run XDMCP over SSH"));
            break;
        default:
            ((RemminaSSH*)tunnel)->error = g_strdup_printf ("Error executing xqproxy on SSH server (status = %i).", status);
            break;
        }
    }
    g_free (cmd);
    channel_close (channel);
    channel_free (channel);
    return ret;
}

static gboolean
remmina_plug_xdmcp_tunnel_connect_callback (RemminaSSHTunnel *tunnel, gpointer data)
{
    remmina_plug_emit_signal (REMMINA_PLUG (data), "connect");
    return TRUE;
}

static gboolean
remmina_plug_xdmcp_tunnel_disconnect_callback (RemminaSSHTunnel *tunnel, gpointer data)
{
    RemminaPlug *gp = REMMINA_PLUG (data);

    if (REMMINA_SSH (tunnel)->error)
    {
        g_snprintf (gp->error_message, MAX_ERROR_LENGTH, "%s", REMMINA_SSH (tunnel)->error);
        gp->has_error = TRUE;
    }
    IDLE_ADD ((GSourceFunc) remmina_plug_close_connection, gp);
    return TRUE;
}
#endif

static gboolean
remmina_plug_xdmcp_main (RemminaPlugXdmcp *gp_xdmcp)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_xdmcp);
    RemminaFile *remminafile = gp->remmina_file;
    gchar *argv[50];
    gint argc;
    gchar *p1, *p2;
    gint display;
    gint i;
    GError *error = NULL;
    gboolean ret;

    display = remmina_public_get_available_xdisplay ();
    if (display == 0)
    {
        g_snprintf (gp->error_message, MAX_ERROR_LENGTH, "%s", "Run out of available local X display number.");
        gp->has_error = TRUE;
        gp_xdmcp->thread = 0;
        return FALSE;
    }

    argc = 0;
    argv[argc++] = g_strdup ("Xephyr");

    argv[argc++] = g_strdup_printf (":%i", display);

    argv[argc++] = g_strdup ("-parent");
    argv[argc++] = g_strdup_printf ("%i", gp_xdmcp->socket_id);

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
        NULL, NULL, &gp_xdmcp->pid, &error);
    for (i = 0; i < argc; i++) g_free (argv[i]);

    if (!ret)
    {
        g_snprintf (gp->error_message, MAX_ERROR_LENGTH, "%s", error->message);
        gp->has_error = TRUE;
        gp_xdmcp->thread = 0;
        return FALSE;
    }

#ifdef HAVE_LIBSSH
    if (remminafile->ssh_enabled)
    {
        if (!remmina_plug_start_xport_tunnel (gp, display,
            remmina_plug_xdmcp_tunnel_init_callback,
            remmina_plug_xdmcp_tunnel_connect_callback,
            remmina_plug_xdmcp_tunnel_disconnect_callback,
            gp_xdmcp))
        {
            gp_xdmcp->thread = 0;
            return FALSE;
        }
    }
    else
    {
        remmina_plug_emit_signal (REMMINA_PLUG (gp_xdmcp), "connect");
    }
#else
    remmina_plug_emit_signal (REMMINA_PLUG (gp_xdmcp), "connect");
#endif

    gp_xdmcp->thread = 0;
    return TRUE;
}

#ifdef HAVE_LIBSSH
static gpointer
remmina_plug_xdmcp_main_thread (gpointer data)
{
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

    CANCEL_ASYNC
    if (!remmina_plug_xdmcp_main (REMMINA_PLUG_XDMCP (data)))
    {
        IDLE_ADD ((GSourceFunc) remmina_plug_close_connection, data);
    }
    return NULL;
}
#endif

static gboolean
remmina_plug_xdmcp_open_connection (RemminaPlug *gp)
{
    RemminaPlugXdmcp *gp_xdmcp = REMMINA_PLUG_XDMCP (gp);
    RemminaFile *remminafile = gp->remmina_file;

    gp->width = remminafile->resolution_width;
    gp->height = remminafile->resolution_height;
    gtk_widget_set_size_request (GTK_WIDGET (gp), remminafile->resolution_width, remminafile->resolution_height);
    gp_xdmcp->socket_id = gtk_socket_get_id (GTK_SOCKET (gp_xdmcp->socket));

#ifdef HAVE_LIBSSH

    if (remminafile->ssh_enabled)
    {
        if (pthread_create (&gp_xdmcp->thread, NULL, remmina_plug_xdmcp_main_thread, gp))
        {
            g_snprintf (gp->error_message, MAX_ERROR_LENGTH, "%s",
                "Failed to initialize pthread. Falling back to non-thread mode...");
            gp->has_error = TRUE;
            gp_xdmcp->thread = 0;
            return FALSE;
        }
        else
        {
            return TRUE;
        }
    }
    else
    {
        return remmina_plug_xdmcp_main (gp_xdmcp);
    }

#else

    return remmina_plug_xdmcp_main (gp_xdmcp);

#endif
}

static gboolean
remmina_plug_xdmcp_close_connection (RemminaPlug *gp)
{
    RemminaPlugXdmcp *gp_xdmcp = REMMINA_PLUG_XDMCP (gp);

#ifdef HAVE_PTHREAD
    if (gp_xdmcp->thread)
    {
        pthread_cancel (gp_xdmcp->thread);
        if (gp_xdmcp->thread) pthread_join (gp_xdmcp->thread, NULL);
    }
#endif

    if (gp_xdmcp->pid)
    {
        kill (gp_xdmcp->pid, SIGTERM);
        g_spawn_close_pid (gp_xdmcp->pid);
        gp_xdmcp->pid = 0;
    }

    remmina_plug_emit_signal (gp, "disconnect");

    return FALSE;
}

static gpointer
remmina_plug_xdmcp_query_feature (RemminaPlug *gp, RemminaPlugFeature feature)
{
    return NULL;
}

static void
remmina_plug_xdmcp_call_feature (RemminaPlug *gp, RemminaPlugFeature feature, const gpointer data)
{
}

static void
remmina_plug_xdmcp_class_init (RemminaPlugXdmcpClass *klass)
{
    klass->parent_class.open_connection = remmina_plug_xdmcp_open_connection;
    klass->parent_class.close_connection = remmina_plug_xdmcp_close_connection;
    klass->parent_class.query_feature = remmina_plug_xdmcp_query_feature;
    klass->parent_class.call_feature = remmina_plug_xdmcp_call_feature;
}

static void
remmina_plug_xdmcp_destroy (GtkWidget *widget, gpointer data)
{
}

static void
remmina_plug_xdmcp_init (RemminaPlugXdmcp *gp_xdmcp)
{
    gp_xdmcp->socket = gtk_socket_new ();
    gtk_widget_show (gp_xdmcp->socket);
    g_signal_connect (G_OBJECT (gp_xdmcp->socket), "plug-added",
        G_CALLBACK (remmina_plug_xdmcp_plug_added), gp_xdmcp);
    g_signal_connect (G_OBJECT (gp_xdmcp->socket), "plug-removed",
        G_CALLBACK (remmina_plug_xdmcp_plug_removed), gp_xdmcp);
    gtk_container_add (GTK_CONTAINER (gp_xdmcp), gp_xdmcp->socket);

    g_signal_connect (G_OBJECT (gp_xdmcp), "destroy", G_CALLBACK (remmina_plug_xdmcp_destroy), NULL);

    gp_xdmcp->socket_id = 0;
    gp_xdmcp->pid = 0;
    gp_xdmcp->output_fd = 0;
    gp_xdmcp->error_fd = 0;
    gp_xdmcp->thread = 0;
}

GtkWidget*
remmina_plug_xdmcp_new (void)
{
    return GTK_WIDGET (g_object_new (REMMINA_TYPE_PLUG_XDMCP, NULL));
}

