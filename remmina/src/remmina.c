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

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "remminapublic.h"
#include "remminamain.h"
#include "remminafilemanager.h"
#include "remminafileeditor.h"
#include "remminaconnectionwindow.h"
#include "remminapref.h"
#include "remminaprefdialog.h"
#include "remminawidgetpool.h"
#include "remminaabout.h"
#include "remminapluginmanager.h"
#include "remminasftpplugin.h"
#include "remminasshplugin.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifdef HAVE_LIBGCRYPT
#include <gcrypt.h>
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif

#ifdef HAVE_LIBUNIQUE
#include <unique/unique.h>
#endif

enum
{
    COMMAND_0,
    REMMINA_COMMAND_MAIN,
    REMMINA_COMMAND_PREF,
    REMMINA_COMMAND_QUICK,
    REMMINA_COMMAND_NEW,
    REMMINA_COMMAND_CONNECT,
    REMMINA_COMMAND_EDIT,
    REMMINA_COMMAND_ABOUT,
    REMMINA_COMMAND_PLUGIN
};

static gint
remmina_parse_command (int argc, char* argv[], gchar **data)
{
    gint command = 0;
    gint c;
    gint exec = 0;
    gchar *opt = NULL;
    gchar *server = NULL;
    gchar *protocol = NULL;

    while ((c = getopt (argc, argv, "ac:e:np:qs:t:x:")) != -1)
    {
        switch (c)
        {
        case 'a':
        case 'c':
        case 'e':
        case 'n':
        case 'p':
        case 'q':
        case 'x':
            if (exec != 0)
            {
                g_print ("%s: Invalid option %c\n", argv[0], c);
                break;
            }
            exec = c;
            if (c == 'c' || c == 'e' || c == 'p' || c == 'x')
            {
                opt = optarg;
            }
            break;
        case 's':
            server = optarg;
            break;
        case 't':
            protocol = optarg;
            break;
        }
    }

    switch (exec)
    {
    case 'p':
        command = REMMINA_COMMAND_PREF;
        *data = g_strdup (opt);
        break;
    case 'q':
        command = REMMINA_COMMAND_QUICK;
        if (server)
        {
            *data = g_strdup_printf ("%s,%s", protocol, server);
        }
        else
        {
            *data = g_strdup (protocol);
        }
        break;
    case 'n':
        command = REMMINA_COMMAND_NEW;
        if (server)
        {
            *data = g_strdup_printf ("%s,%s", protocol, server);
        }
        else
        {
            *data = g_strdup (protocol);
        }
        break;
    case 'c':
        command = REMMINA_COMMAND_CONNECT;
        *data = g_strdup (opt);
        break;
    case 'e':
        command = REMMINA_COMMAND_EDIT;
        *data = g_strdup (opt);
        break;
    case 'a':
        command = REMMINA_COMMAND_ABOUT;
        *data = NULL;
        break;
    case 'x':
        command = REMMINA_COMMAND_PLUGIN;
        *data = g_strdup (opt);
        break;
    default:
        command = REMMINA_COMMAND_MAIN;
        *data = NULL;
        break;
    }
    return command;
}

static void
remmina_exec_command (gint command, const gchar *data)
{
    GtkWidget *widget;
    gchar *s1, *s2;
    RemminaEntryPlugin *plugin;

    switch (command)
    {
    case REMMINA_COMMAND_MAIN:
        widget = remmina_widget_pool_find (REMMINA_TYPE_MAIN, NULL);
        if (widget)
        {
            gtk_window_present (GTK_WINDOW (widget));
        }
        else
        {
            widget = remmina_main_new ();
            gtk_widget_show (widget);
        }
        break;
    case REMMINA_COMMAND_PREF:
        widget = remmina_widget_pool_find (REMMINA_TYPE_PREF_DIALOG, NULL);
        if (widget)
        {
            gtk_window_present (GTK_WINDOW (widget));
        }
        else
        {
            widget = remmina_pref_dialog_new (atoi (data));
            gtk_widget_show (widget);
        }
        break;
    case REMMINA_COMMAND_QUICK:
        s1 = (data ? strchr (data, ',') : NULL);
        if (s1)
        {
            s1 = g_strdup (data);
            s2 = strchr (s1, ',');
            *s2++ = '\0';
            widget = remmina_file_editor_new_temp_full (s2, s1);
            g_free (s1);
        }
        else
        {
            widget = remmina_file_editor_new_temp_full (NULL, data);
        }
        gtk_widget_show (widget);
        break;
    case REMMINA_COMMAND_NEW:
        s1 = (data ? strchr (data, ',') : NULL);
        if (s1)
        {
            s1 = g_strdup (data);
            s2 = strchr (s1, ',');
            *s2++ = '\0';
            widget = remmina_file_editor_new_full (s2, s1);
            g_free (s1);
        }
        else
        {
            widget = remmina_file_editor_new_full (NULL, data);
        }
        gtk_widget_show (widget);
        break;
    case REMMINA_COMMAND_CONNECT:
        remmina_connection_window_open_from_filename (data);
        break;
    case REMMINA_COMMAND_EDIT:
        widget = remmina_file_editor_new_from_filename (data);
        if (widget) gtk_widget_show (widget);
        break;
    case REMMINA_COMMAND_ABOUT:
        remmina_about_open (NULL);
        break;
    case REMMINA_COMMAND_PLUGIN:
        plugin = (RemminaEntryPlugin *) remmina_plugin_manager_get_plugin (REMMINA_PLUGIN_TYPE_ENTRY, data);
        if (plugin)
        {
            plugin->entry_func ();
        }
        else
        {
            widget = gtk_message_dialog_new (NULL,
                GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
                _("Plugin %s is not registered."), data);
            g_signal_connect (G_OBJECT (widget), "response", G_CALLBACK (gtk_widget_destroy), NULL);
            gtk_widget_show (widget);
            remmina_widget_pool_register (widget);
        }
        break;
    }
}

#ifdef HAVE_LIBUNIQUE
static UniqueResponse
remmina_unique_message_received (
    UniqueApp         *app,
    UniqueCommand      command,
    UniqueMessageData *message,
    guint              time_,
    gpointer           user_data)
{
    gchar *data;

    data = (message ? unique_message_data_get_text (message) : NULL);
    remmina_exec_command (command, data);
    g_free (data);
    return UNIQUE_RESPONSE_OK;
}

static gboolean
remmina_unique_exec_command (gint command, const gchar *data)
{
    UniqueApp *app;
    UniqueMessageData *message;
    UniqueResponse resp;
    gboolean newapp = TRUE;

    app = unique_app_new_with_commands ("org.remmina", NULL,
        "main",     REMMINA_COMMAND_MAIN,
        "pref",     REMMINA_COMMAND_PREF,
        "quick",    REMMINA_COMMAND_QUICK,
        "newf",     REMMINA_COMMAND_NEW,
        "connect",  REMMINA_COMMAND_CONNECT,
        "edit",     REMMINA_COMMAND_EDIT,
        "about",    REMMINA_COMMAND_ABOUT,
        "plugin",   REMMINA_COMMAND_PLUGIN,
        NULL);
    if (unique_app_is_running (app))
    {
        message = unique_message_data_new ();
        unique_message_data_set_text (message, (data && data[0]) ? data : "none", -1);
        resp = unique_app_send_message (app, command, message);
        unique_message_data_free (message);

        if (resp == UNIQUE_RESPONSE_OK)
        {
            newapp = FALSE;
        }
        else
        {
            remmina_exec_command (command, data);
        }
        g_object_unref (app);
    }
    else
    {
        g_signal_connect (app, "message-received", G_CALLBACK (remmina_unique_message_received), NULL);
        remmina_exec_command (command, data);
    }
    return newapp;
}
#endif

int
main (int argc, char* argv[])
{
    gint command;
    gchar *data;
    gboolean newapp;

    bindtextdomain (GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
    textdomain (GETTEXT_PACKAGE);

#ifdef HAVE_PTHREAD
    g_thread_init (NULL);
    gdk_threads_init ();
#endif

#ifdef HAVE_LIBGCRYPT
    gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
    gcry_check_version (NULL);
    gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
    gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif

    gtk_init (&argc, &argv);

    remmina_file_manager_init ();
    remmina_pref_init ();
    remmina_plugin_manager_init ();
    remmina_widget_pool_init ();
    remmina_sftp_plugin_register ();
    remmina_ssh_plugin_register ();

    g_set_application_name ("Remmina");
    gtk_window_set_default_icon_name ("remmina");

    gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
        REMMINA_DATADIR G_DIR_SEPARATOR_S "icons");

    command = remmina_parse_command (argc, argv, &data);
#ifdef HAVE_LIBUNIQUE
    newapp = remmina_unique_exec_command (command, data);
#else
    remmina_exec_command (command, data);
    newapp = TRUE;
#endif
    g_free (data);

    if (newapp)
    {
        THREADS_ENTER
        gtk_main ();
        THREADS_LEAVE    
    }

    return 0;
}

