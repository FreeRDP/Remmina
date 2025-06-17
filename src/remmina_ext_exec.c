/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2014-2023 Antenore Gatta, Giovanni Panozzo
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

#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include "remmina_utils.h"
#include "remmina_ext_exec.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

#define SPAWN_TIMEOUT 10

#define GET_OBJECT(object_name) gtk_builder_get_object(builder, object_name)

static void wait_for_child(GPid pid, gint script_retval, gpointer data)
{
	PCon_Spinner *pcspinner = (PCon_Spinner*)data;

	gtk_spinner_stop(GTK_SPINNER(pcspinner->spinner));
	gtk_widget_destroy(GTK_WIDGET(pcspinner->dialog));
	g_spawn_close_pid(pid);
	/* TODO At the moment background processes will fail to start before the
	 * remmina connection.
	 * Adding a delay here could be a (not good) solution, or we should
	 * monitor each child opened, but it could be quit tricky and messy */
}

GtkDialog* remmina_ext_exec_new(RemminaFile* remminafile, const char *remmina_ext_exec_type)
{
	TRACE_CALL(__func__);
	GtkBuilder *builder;
	PCon_Spinner *pcspinner;
	GError *error = NULL;
	char **argv;
	gchar *cmd = NULL;
	gchar pre[11];
	gchar post[12];
	GPid child_pid;

	strcpy(pre, "precommand");
	strcpy(post, "postcommand");

	if (remmina_ext_exec_type != NULL && (
				strcmp(remmina_ext_exec_type, pre) |
				strcmp(remmina_ext_exec_type, post) )) {
		cmd = g_strdup(remmina_file_get_string(remminafile, remmina_ext_exec_type));
		g_debug("[%s] %s", remmina_ext_exec_type, cmd);
	} else
		return FALSE;

	cmd = remmina_file_format_properties(remminafile, cmd);
	cmd = remmina_utils_get_flatpak_command(cmd);
	g_debug("[%s] updated to: %s", remmina_ext_exec_type, cmd);
	if (cmd != NULL && *cmd != 0) {

		pcspinner = g_new(PCon_Spinner, 1);
		builder = remmina_public_gtk_builder_new_from_resource("/org/remmina/Remmina/src/../data/ui/remmina_spinner.glade");
		pcspinner->dialog = GTK_DIALOG(gtk_builder_get_object(builder, "DialogSpinner"));
		pcspinner->label_pleasewait = GTK_LABEL(GET_OBJECT("label_pleasewait"));
		pcspinner->spinner = GTK_WIDGET(GET_OBJECT("spinner"));
		pcspinner->button_cancel = GTK_BUTTON(GET_OBJECT("button_cancel"));
		/* Connect signals */
		gtk_builder_connect_signals(builder, NULL);

		/* Exec a predefined command */
		g_shell_parse_argv(cmd, NULL, &argv, &error);

		if (error) {
			g_warning("%s\n", error->message);
			g_error_free(error);
		}

		/* Consider using G_SPAWN_SEARCH_PATH_FROM_ENVP (from glib 2.38)*/
		g_spawn_async(  NULL,                           // cwd
			argv,                                   // argv
			NULL,                                   // envp
			G_SPAWN_SEARCH_PATH |
			G_SPAWN_SEARCH_PATH_FROM_ENVP |
			G_SPAWN_DO_NOT_REAP_CHILD,              // flags
			NULL,                                   // child_setup
			NULL,                                   // child_setup user data
			&child_pid,                             // pid location
			&error);                                // error
		if (!error) {
			gtk_spinner_start(GTK_SPINNER(pcspinner->spinner));
			g_child_watch_add(child_pid, wait_for_child, (gpointer)pcspinner);
			gtk_dialog_run(pcspinner->dialog);
		}else  {
			g_warning("Command %s exited with error: %s\n", cmd, error->message);
			g_error_free(error);
		}
		g_strfreev(argv);
		g_free(cmd);
		return (pcspinner->dialog);
	}
	g_free(cmd);
	return FALSE;
}
