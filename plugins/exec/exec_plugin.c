/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2017-2020 Antenore Gatta, Giovanni Panozzo
 *
 * Initially based on the plugin "Remmina Plugin EXEC", created and written by
 * Fabio Castelli (Muflone) <muflone@vbsimple.net>.
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

#include "exec_plugin_config.h"

#include "common/remmina_plugin.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define GET_PLUGIN_DATA(gp) (RemminaPluginExecData*)g_object_get_data(G_OBJECT(gp), "plugin-data")

typedef struct _RemminaPluginExecData {
	GtkWidget *log_view;
	GtkTextBuffer *log_buffer;
	GtkTextBuffer *err;
	GtkWidget *sw;
} RemminaPluginExecData;

static RemminaPluginService *remmina_plugin_service = NULL;

	static void
cb_child_watch( GPid pid, gint status)
{
	/* Close pid */
	g_spawn_close_pid( pid );
}

	static gboolean
cb_out_watch (GIOChannel *channel, GIOCondition cond, RemminaProtocolWidget *gp)
{
	gchar *string;
	gsize  size;

	RemminaPluginExecData *gpdata = GET_PLUGIN_DATA(gp);

	if( cond == G_IO_HUP )
	{
		g_io_channel_unref( channel );
		return FALSE;
	}

	g_io_channel_read_line( channel, &string, &size, NULL, NULL );
	gtk_text_buffer_insert_at_cursor( gpdata->log_buffer, string, -1 );
	g_free( string );

	return TRUE;
}

	static gboolean
cb_err_watch (GIOChannel *channel, GIOCondition cond, RemminaProtocolWidget *gp)
{
	gchar *string;
	gsize  size;

	RemminaPluginExecData *gpdata = GET_PLUGIN_DATA(gp);

	if( cond == G_IO_HUP )
	{
		g_io_channel_unref( channel );
		return FALSE;
	}

	g_io_channel_read_line( channel, &string, &size, NULL, NULL );
	gtk_text_buffer_insert_at_cursor( gpdata->err, string, -1 );
	g_free( string );

	return TRUE;
}

static void remmina_plugin_exec_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginExecData *gpdata;

	remmina_plugin_service->log_printf("[%s] Plugin init\n", PLUGIN_NAME);

	gpdata = g_new0(RemminaPluginExecData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	gpdata->log_view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(gpdata->log_view), GTK_WRAP_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(gpdata->log_view), FALSE);
	gtk_text_view_set_left_margin (GTK_TEXT_VIEW (gpdata->log_view), 20);
	gtk_text_view_set_right_margin (GTK_TEXT_VIEW (gpdata->log_view), 20);
	gpdata->log_buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (gpdata->log_view));
	gpdata->sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_set_size_request (gpdata->sw, 640, 480);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (gpdata->sw),
			GTK_POLICY_AUTOMATIC,
			GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->sw);
	gtk_container_add(GTK_CONTAINER(gpdata->sw), gpdata->log_view);
	gtk_text_buffer_set_text (gpdata->log_buffer, "Remmina Exec Plugin Logger", -1);

	gtk_widget_show_all(gpdata->sw);
}

static gboolean remmina_plugin_exec_run(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaFile* remminafile;
	const gchar *cmd;
	gchar *stdout_buffer;
	gchar *stderr_buffer;
	char **argv;
	GError *error = NULL;
	GPid child_pid;
	gint child_stdout, child_stderr;
	GtkDialog *dialog;
	GIOChannel *out_ch, *err_ch;

	remmina_plugin_service->log_printf("[%s] Plugin run\n", PLUGIN_NAME);
	RemminaPluginExecData *gpdata = GET_PLUGIN_DATA(gp);
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	cmd = remmina_plugin_service->file_get_string(remminafile, "execcommand");
	if (!cmd) {
		gtk_text_buffer_set_text (gpdata->log_buffer,
				_("You did not set any command to be executed"), -1);
		remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);
		return TRUE;
	}

	g_shell_parse_argv(cmd, NULL, &argv, &error);
	if (error) {
		gtk_text_buffer_set_text (gpdata->log_buffer, error->message, -1);
		remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);
		return TRUE;
		g_error_free(error);
	}

	if (remmina_plugin_service->file_get_int(remminafile, "runasync", FALSE)) {
		remmina_plugin_service->log_printf("[%s] Run Async\n", PLUGIN_NAME);
		g_spawn_async_with_pipes(   NULL,
				argv,
				NULL,
				G_SPAWN_DO_NOT_REAP_CHILD |
				G_SPAWN_SEARCH_PATH_FROM_ENVP |
				G_SPAWN_SEARCH_PATH,
				NULL,
				NULL,
				&child_pid,
				NULL,
				&child_stdout,
				&child_stderr,
				&error);
		if (error != NULL) {
			gtk_text_buffer_set_text (gpdata->log_buffer, error->message, -1);
			g_error_free(error);
			remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);
			return TRUE;
		}
		g_child_watch_add(child_pid, (GChildWatchFunc)cb_child_watch, gp );

		/* Create channels that will be used to read data from pipes. */
		out_ch = g_io_channel_unix_new(child_stdout);
		err_ch = g_io_channel_unix_new(child_stderr);
		/* Add watches to channels */
		g_io_add_watch(out_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_out_watch, gp );
		g_io_add_watch(err_ch, G_IO_IN | G_IO_HUP, (GIOFunc)cb_err_watch, gp );

	}else {
		dialog = GTK_DIALOG(gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
					GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			_("Warning: Running a command synchronously may cause Remmina not to respond.\nDo you really want to continue?")));
		gint result = gtk_dialog_run (GTK_DIALOG (dialog));

		switch (result)
		{
			case GTK_RESPONSE_YES:
				break;
			default:
				gtk_widget_destroy(GTK_WIDGET(dialog));
				return FALSE;
				break;
		}
		gtk_widget_destroy(GTK_WIDGET(dialog));
		remmina_plugin_service->log_printf("[%s] Run Sync\n", PLUGIN_NAME);
		g_spawn_sync (NULL,				    // CWD or NULL
				argv,
				NULL,				    // ENVP or NULL
				G_SPAWN_SEARCH_PATH |
				G_SPAWN_SEARCH_PATH_FROM_ENVP,
				NULL,
				NULL,
				&stdout_buffer, 		    // STDOUT
				&stderr_buffer,			    // STDERR
				NULL,				    // Exit status
				&error);
		if (!error) {
			remmina_plugin_service->log_printf("[%s] Command executed\n", PLUGIN_NAME);
			gtk_text_buffer_set_text (gpdata->log_buffer, stdout_buffer, -1);
		}else  {
			g_warning("Command %s exited with error: %s\n", cmd, error->message);
			gtk_text_buffer_set_text (gpdata->log_buffer, error->message, -1);
			g_error_free(error);
		}
	}

	g_strfreev(argv);

	remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);
	return TRUE;
}

static gboolean remmina_plugin_exec_close(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	remmina_plugin_service->log_printf("[%s] Plugin close\n", PLUGIN_NAME);
	remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
	return FALSE;
}

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_exec_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"execcommand",  N_("Command"),  FALSE,  NULL,   NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"runasync", N_("Asynchrounous execution"),  FALSE,  NULL,   NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	NULL,	    NULL,	    FALSE,  NULL,   NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin = {
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   // Type
	PLUGIN_NAME,                                    // Name
	PLUGIN_DESCRIPTION,                             // Description
	GETTEXT_PACKAGE,                                // Translation domain
	PLUGIN_VERSION,                                 // Version number
	PLUGIN_APPICON,                                 // Icon for normal connection
	PLUGIN_APPICON,                                 // Icon for SSH connection
	remmina_plugin_exec_basic_settings,             // Array for basic settings
	NULL,                                           // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_NONE,              // SSH settings type
	NULL,                                           // Array for available features
	remmina_plugin_exec_init,                       // Plugin initialization
	remmina_plugin_exec_run,                        // Plugin open connection
	remmina_plugin_exec_close,                      // Plugin close connection
	NULL,                                           // Query for available features
	NULL,                                           // Call a feature
	NULL,                                           // Send a keystroke
	NULL                                            // No screenshot support available
};

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin*)&remmina_plugin)) {
		return FALSE;
	}

	return TRUE;
}
