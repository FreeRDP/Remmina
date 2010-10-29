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
#include "remminaappletutil.h"

void
remmina_applet_util_launcher (RemminaLaunchType launch_type, const gchar *filename,
    const gchar *server, const gchar *protocol)
{
    gint argc;
    gchar *argv[50];
    gint i;
    GError *error = NULL;
    gboolean ret;
    GtkWidget *dialog;

    argc = 0;
    argv[argc++] = g_strdup ("remmina");

    switch (launch_type)
    {
    case REMMINA_LAUNCH_MAIN:
        break;

    case REMMINA_LAUNCH_PREF:
        argv[argc++] = g_strdup ("-p");
        argv[argc++] = g_strdup ("2");
        break;

    case REMMINA_LAUNCH_NEW:
        argv[argc++] = g_strdup ("-n");
        break;

    case REMMINA_LAUNCH_FILE:
        argv[argc++] = g_strdup ("-c");
        argv[argc++] = g_strdup (filename);
        break;

    case REMMINA_LAUNCH_EDIT:
        argv[argc++] = g_strdup ("-e");
        argv[argc++] = g_strdup (filename);
        break;

    case REMMINA_LAUNCH_ABOUT:
        argv[argc++] = g_strdup ("-a");
        break;

    }

    if (server)
    {
        argv[argc++] = g_strdup ("-s");
        argv[argc++] = g_strdup (server);
    }
    if (protocol)
    {
        argv[argc++] = g_strdup ("-t");
        argv[argc++] = g_strdup (protocol);
    }

    argv[argc++] = NULL;

    ret = g_spawn_async (
        NULL, /* working_directory: take current */
        argv, /* argv[0] is the executable, parameters start from argv[1], end with NULL */
        NULL, /* envp: take the same as parent */
        G_SPAWN_SEARCH_PATH, /* flags */
        NULL, /* child_setup: function to be called before exec() */
        NULL, /* user_data: parameter for child_setup function */
        NULL, /* child_pid */
        &error);

    for (i = 0; i < argc; i++) g_free (argv[i]);

    if (!ret)
    {
        dialog = gtk_message_dialog_new (NULL,
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            error->message, NULL);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
    }
}

gboolean
remmina_applet_util_get_pref_boolean (const gchar *key)
{
    gchar filename[MAX_PATH_LEN];
    GKeyFile *gkeyfile;
    gboolean value;

    g_snprintf (filename, sizeof (filename), "%s/.remmina/remmina.pref", g_get_home_dir ());
    gkeyfile = g_key_file_new ();
    if (g_key_file_load_from_file (gkeyfile, filename, G_KEY_FILE_NONE, NULL))
    {
        value = g_key_file_get_boolean (gkeyfile, "remmina_pref", key, NULL);
    }
    else
    {
        value = FALSE;
    }
    g_key_file_free (gkeyfile);

    return value;
}

void
remmina_applet_util_set_pref_boolean (const gchar *key, gboolean value)
{
    gchar filename[MAX_PATH_LEN];
    GKeyFile *gkeyfile;
    gchar *content;
    gsize length;

    g_snprintf (filename, sizeof (filename), "%s/.remmina/remmina.pref", g_get_home_dir ());
    gkeyfile = g_key_file_new ();
    if (g_key_file_load_from_file (gkeyfile, filename, G_KEY_FILE_NONE, NULL))
    {
        g_key_file_set_boolean (gkeyfile, "remmina_pref", key, value);
        content = g_key_file_to_data (gkeyfile, &length, NULL);
        g_file_set_contents (filename, content, length, NULL);
        g_free (content);
    }
    g_key_file_free (gkeyfile);
}


