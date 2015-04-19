/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Giovanni Panozzo
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <sys/wait.h>
#include "remmina_file.h"
#include "remmina_preexec.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

#define GET_OBJECT(object_name) gtk_builder_get_object(builder, object_name)

void child_watch (GPid pid, gint status, gpointer user_data)
{
	gboolean ok;

	/* Successful program termination is determined by the EXIT_SUCCESS code.
	 * Otherwise it is considered to have terminated abnormally.
	 */
	if (WIFEXITED (status) && WEXITSTATUS (status) == EXIT_SUCCESS)
		ok = TRUE;
	else
		ok = FALSE;

	g_debug ("client_os_command: child exited %s", ok ? "OK" : "FAIL");
}

GtkDialog* remmina_preexec_new(RemminaFile* remminafile)
{
	TRACE_CALL("remmina_preexec_new");
	GtkBuilder *builder;
	GtkDialog *dialog;
	GtkLabel *label_pleasewait;
	GtkWidget *spinner;
	GtkButton *button_cancel;
	GError *error = NULL;
	char **argv;
	gint argp;
	char const *cmd = NULL;
	gboolean ok, watch;
	GPid child_pid;

	cmd = remmina_file_get_string(remminafile, "precommand");
	ok = g_shell_parse_argv(cmd, &argp, &argv, NULL);

	g_assert(ok==TRUE);
	if (cmd)
	{
		builder = remmina_public_gtk_builder_new_from_file("remmina_spinner.glade");
		dialog = GTK_DIALOG(gtk_builder_get_object(builder, "DialogSpinner"));
		label_pleasewait = GTK_LABEL(GET_OBJECT("label_pleasewait"));
		spinner = GTK_WIDGET(GET_OBJECT("spinner"));
		button_cancel = GTK_BUTTON(GET_OBJECT("button_cancel"));
		/*  gtk_window_set_transient_for(GTK_WINDOW(dialog), parent_window); */
		/* Connect signals */
		gtk_builder_connect_signals(builder, NULL);

		/* Exec a predefined command */
		g_spawn_async(	NULL,                         // cwd
						argv,                         // argv
						NULL,                         // envp
						G_SPAWN_DO_NOT_REAP_CHILD,    // flags
						NULL,                         // child_setup
						NULL,                         // child_setup user data
						&child_pid,                   // exit status
						&error);                      // error
		if (error)
			g_warning ("%s", error->message);
		gtk_spinner_start (GTK_SPINNER (spinner));
		g_assert(watch==FALSE);
		gtk_spinner_stop(GTK_SPINNER (spinner));
		gtk_dialog_run(dialog);
	}
}
