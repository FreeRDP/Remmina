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
#include "remminaplug_rdp.h"

G_DEFINE_TYPE (RemminaPlugRdp, remmina_plug_rdp, REMMINA_TYPE_PLUG)

static void
remmina_plug_rdp_plug_added (GtkSocket *socket, RemminaPlugRdp *gp_rdp)
{
    remmina_plug_emit_signal (REMMINA_PLUG (gp_rdp), "connect");
}

static void
remmina_plug_rdp_plug_removed (GtkSocket *socket, RemminaPlugRdp *gp_rdp)
{
}

static void
remmina_plug_rdp_set_error (RemminaPlugRdp *gp_rdp)
{
    gchar *buffer;
    gint len;

    buffer = REMMINA_PLUG (gp_rdp)->error_message;
    lseek (gp_rdp->error_fd, 0, SEEK_SET);
    len = read (gp_rdp->error_fd, buffer, MAX_ERROR_LENGTH);
    buffer[len] = '\0';

    REMMINA_PLUG (gp_rdp)->has_error = TRUE;
}

static void
remmina_plug_rdp_child_exit (GPid pid, gint status, gpointer data)
{
    RemminaPlugRdp *gp_rdp;

    gp_rdp = REMMINA_PLUG_RDP (data);
    g_spawn_close_pid (pid);

    if (status != 0 && status != SIGTERM)
    {
        remmina_plug_rdp_set_error (gp_rdp);
    }
    close (gp_rdp->output_fd);
    close (gp_rdp->error_fd);

    remmina_plug_emit_signal (REMMINA_PLUG (gp_rdp), "disconnect");
}

static gboolean
remmina_plug_rdp_main (RemminaPlugRdp *gp_rdp)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_rdp);
    RemminaFile *remminafile = gp->remmina_file;
    gchar *argv[50];
    gchar *host;
    gint argc;
    gint i;
    GString *printers;
    gchar *printername;
    gint advargc = 0;
    gchar **advargv = NULL;
    GError *error = NULL;
    gboolean ret;

    if (remminafile->arguments && remminafile->arguments[0] != '\0')
    {
        if (!g_shell_parse_argv (remminafile->arguments, &advargc, &advargv, &error))
        {
            g_print ("%s\n", error->message);
            /* We simply ignore argument error. */
            advargv = NULL;
        }
    }

    host = remmina_plug_start_direct_tunnel (gp, 3389);
    if (host == NULL)
    {
        gp_rdp->thread = 0;
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

    if (remminafile->shareprinter && gp->printers && gp->printers->len > 0)
    {
        printers = g_string_new ("");
        for (i = 0; i < gp->printers->len; i++)
        {
            if (printers->len > 0)
            {
                g_string_append_c (printers, ',');
            }
            printername = (gchar*) g_ptr_array_index (gp->printers, i);
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

    gp->width = remminafile->resolution_width;
    gp->height = remminafile->resolution_height;

    argv[argc++] = g_strdup ("-X");
    argv[argc++] = g_strdup_printf ("%i", gp_rdp->socket_id);

    argv[argc++] = host;
    argv[argc++] = NULL;

    ret = g_spawn_async_with_pipes (
        NULL, /* working_directory: take current */
        argv, /* argv[0] is the executable, parameters start from argv[1], end with NULL */
        NULL, /* envp: take the same as parent */
        G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, /* flags */
        NULL, /* child_setup: function to be called before exec() */
        NULL, /* user_data: parameter for child_setup function */
        &gp_rdp->pid, /* child_pid */
        NULL, /* standard_input */
        &gp_rdp->output_fd, /* standard_output */
        &gp_rdp->error_fd, /* standard_error */
        &error);

    for (i = 0; i < argc; i++) g_free (argv[i]);

    if (ret)
    {
        g_child_watch_add (gp_rdp->pid, remmina_plug_rdp_child_exit, gp_rdp);
        gp_rdp->thread = 0;
        return TRUE;
    }
    else
    {
        g_snprintf (gp->error_message, MAX_ERROR_LENGTH, "%s", error->message);
        gp->has_error = TRUE;
        gp_rdp->thread = 0;
        return FALSE;
    }
}

#ifdef HAVE_LIBSSH
static gpointer
remmina_plug_rdp_main_thread (gpointer data)
{
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

    CANCEL_ASYNC
    if (!remmina_plug_rdp_main (REMMINA_PLUG_RDP (data)))
    {
        IDLE_ADD ((GSourceFunc) remmina_plug_close_connection, data);
    }
    return NULL;
}
#endif

static gboolean
remmina_plug_rdp_open_connection (RemminaPlug *gp)
{
    RemminaPlugRdp *gp_rdp = REMMINA_PLUG_RDP (gp);
    RemminaFile *remminafile = gp->remmina_file;

    gtk_widget_set_size_request (GTK_WIDGET (gp), remminafile->resolution_width, remminafile->resolution_height);
    gp_rdp->socket_id = gtk_socket_get_id (GTK_SOCKET (gp_rdp->socket));

#ifdef HAVE_LIBSSH

    if (remminafile->ssh_enabled)
    {
        if (pthread_create (&gp_rdp->thread, NULL, remmina_plug_rdp_main_thread, gp))
        {
            g_snprintf (gp->error_message, MAX_ERROR_LENGTH, "%s",
                "Failed to initialize pthread. Falling back to non-thread mode...");
            gp->has_error = TRUE;
            gp_rdp->thread = 0;
            return FALSE;
        }
        return TRUE;
    }
    else
    {
        return remmina_plug_rdp_main (gp_rdp);
    }

#else

    return remmina_plug_rdp_main (gp_rdp);

#endif
}

static gboolean
remmina_plug_rdp_close_connection (RemminaPlug *gp)
{
    RemminaPlugRdp *gp_rdp = REMMINA_PLUG_RDP (gp);

#ifdef HAVE_PTHREAD
    if (gp_rdp->thread)
    {
        pthread_cancel (gp_rdp->thread);
        if (gp_rdp->thread) pthread_join (gp_rdp->thread, NULL);
    }
#endif

    if (gp_rdp->pid)
    {
        /* If pid exists, "disconnect" signal will be emitted in the child_exit callback */
        kill (gp_rdp->pid, SIGTERM);
    }
    else
    {
        remmina_plug_emit_signal (gp, "disconnect");
    }
    return FALSE;
}

static gpointer
remmina_plug_rdp_query_feature (RemminaPlug *gp, RemminaPlugFeature feature)
{
    return NULL;
}

static void
remmina_plug_rdp_call_feature (RemminaPlug *gp, RemminaPlugFeature feature, const gpointer data)
{
}

static void
remmina_plug_rdp_class_init (RemminaPlugRdpClass *klass)
{
    klass->parent_class.open_connection = remmina_plug_rdp_open_connection;
    klass->parent_class.close_connection = remmina_plug_rdp_close_connection;
    klass->parent_class.query_feature = remmina_plug_rdp_query_feature;
    klass->parent_class.call_feature = remmina_plug_rdp_call_feature;
}

static void
remmina_plug_rdp_destroy (GtkWidget *widget, gpointer data)
{
}

static void
remmina_plug_rdp_init (RemminaPlugRdp *gp_rdp)
{
    gp_rdp->socket = gtk_socket_new ();
    gtk_widget_show (gp_rdp->socket);
    g_signal_connect (G_OBJECT (gp_rdp->socket), "plug-added",
        G_CALLBACK (remmina_plug_rdp_plug_added), gp_rdp);
    g_signal_connect (G_OBJECT (gp_rdp->socket), "plug-removed",
        G_CALLBACK (remmina_plug_rdp_plug_removed), gp_rdp);
    gtk_container_add (GTK_CONTAINER (gp_rdp), gp_rdp->socket);

    g_signal_connect (G_OBJECT (gp_rdp), "destroy", G_CALLBACK (remmina_plug_rdp_destroy), NULL);

    gp_rdp->socket_id = 0;
    gp_rdp->pid = 0;
    gp_rdp->output_fd = 0;
    gp_rdp->error_fd = 0;
    gp_rdp->thread = 0;
}

GtkWidget*
remmina_plug_rdp_new (void)
{
    return GTK_WIDGET (g_object_new (REMMINA_TYPE_PLUG_RDP, NULL));
}

