/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

#include <gdk/gdkx.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <stdlib.h>
#include "config.h"
#include "remmina_public.h"
#include "remmina_main.h"
#include "remmina_file_manager.h"
#include "remmina_pref.h"
#include "remmina_widget_pool.h"
#include "remmina_plugin_manager.h"
#include "remmina_sftp_plugin.h"
#include "remmina_ssh_plugin.h"
#include "remmina_exec.h"
#include "remmina_icon.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <pthread.h>
#ifdef HAVE_LIBGCRYPT
#include <gcrypt.h>
# if GCRYPT_VERSION_NUMBER < 0x010600
GCRY_THREAD_OPTION_PTHREAD_IMPL;
# endif
#endif

static gboolean remmina_option_about;
static gchar *remmina_option_connect;
static gchar *remmina_option_edit;
static gboolean remmina_option_help;
static gboolean remmina_option_new;
static gchar *remmina_option_pref;
static gchar *remmina_option_plugin;
static gboolean remmina_option_quit;
static gboolean remmina_option_version;
static gchar *remmina_option_server;
static gchar *remmina_option_protocol;
static gchar *remmina_option_icon;


static GOptionEntry remmina_options[] =
{
	{ "about", 'a', 0, G_OPTION_ARG_NONE, &remmina_option_about, N_("Show about dialog"), NULL },
	{ "connect", 'c', 0, G_OPTION_ARG_FILENAME, &remmina_option_connect, N_("Connect to a .remmina file"), "FILE" },
	{ "edit", 'e', 0, G_OPTION_ARG_FILENAME, &remmina_option_edit, N_("Edit a .remmina file"), "FILE" },
	{ "help", '?', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &remmina_option_help, NULL, NULL },
	{ "new", 'n', 0, G_OPTION_ARG_NONE, &remmina_option_new, N_("Create a new connection profile"), NULL },
	{ "pref", 'p', 0, G_OPTION_ARG_STRING, &remmina_option_pref, N_("Show preferences dialog page"), "PAGENR" },
	{ "plugin", 'x', 0, G_OPTION_ARG_STRING, &remmina_option_plugin, N_("Execute the plugin"), "PLUGIN" },
	{ "quit", 'q', 0, G_OPTION_ARG_NONE, &remmina_option_quit, N_("Quit the application"), NULL },
	{ "server", 's', 0, G_OPTION_ARG_STRING, &remmina_option_server, N_("Use default server name"), "SERVER" },
	{ "protocol", 't', 0, G_OPTION_ARG_STRING, &remmina_option_protocol, N_("Use default protocol"), "PROTOCOL" },
	{ "icon", 'i', 0, G_OPTION_ARG_NONE, &remmina_option_icon, N_("Start as tray icon"), NULL },
	{ "version", 'v', 0, G_OPTION_ARG_NONE, &remmina_option_version, N_("Show the application's version"), NULL },
	{ NULL }
};

static gint remmina_on_command_line(GApplication *app, GApplicationCommandLine *cmdline)
{
	TRACE_CALL("remmina_on_command_line");
	gint status = 0;
	gint argc;
	gchar **argv;
	GError *error = NULL;
	GOptionContext *context;
	gboolean parsed;
	gchar *s;
	gboolean executed = FALSE;

	remmina_option_about = FALSE;
	remmina_option_connect = NULL;
	remmina_option_edit = NULL;
	remmina_option_help = FALSE;
	remmina_option_new = FALSE;
	remmina_option_pref = NULL;
	remmina_option_plugin = NULL;
	remmina_option_server = NULL;
	remmina_option_protocol = NULL;
	remmina_option_icon = FALSE;
	remmina_option_version = FALSE;

	argv = g_application_command_line_get_arguments(cmdline, &argc);

	context = g_option_context_new("- The GTK+ Remote Desktop Client");
	g_option_context_add_main_entries(context, remmina_options, GETTEXT_PACKAGE);
	g_option_context_add_group(context, gtk_get_option_group(FALSE));
	g_option_context_set_help_enabled(context, TRUE);

	parsed = g_option_context_parse(context, &argc, &argv, &error);
	g_strfreev(argv);

	if (!parsed)
	{
		g_print("option parsing failed: %s\n", error->message);
		status = 1;
	}

	if (remmina_option_version)
	{
		g_print ("%s - Version %s\n", g_get_application_name (), VERSION);
		executed = TRUE;
		status = 1;
	}

	if (remmina_option_quit)
	{
		remmina_exec_command(REMMINA_COMMAND_EXIT, NULL);
		executed = TRUE;
		status = 1;
	}

	if (remmina_option_about)
	{
		remmina_exec_command(REMMINA_COMMAND_ABOUT, NULL);
		executed = TRUE;
	}
	if (remmina_option_connect)
	{
		remmina_exec_command(REMMINA_COMMAND_CONNECT, remmina_option_connect);
		executed = TRUE;
	}
	if (remmina_option_edit)
	{
		remmina_exec_command(REMMINA_COMMAND_EDIT, remmina_option_edit);
		executed = TRUE;
	}
	if (remmina_option_help)
	{
		s = g_option_context_get_help(context, TRUE, NULL);
		g_print("%s", s);
		g_free(s);
		status = 1;
	}
	if (remmina_option_new)
	{
		if (remmina_option_server)
		{
			s = g_strdup_printf("%s,%s", remmina_option_protocol, remmina_option_server);
		}
		else
		{
			s = g_strdup(remmina_option_protocol);
		}
		remmina_exec_command(REMMINA_COMMAND_NEW, s);
		g_free(s);
		executed = TRUE;
	}
	if (remmina_option_pref)
	{
		remmina_exec_command(REMMINA_COMMAND_PREF, remmina_option_pref);
		executed = TRUE;
	}
	if (remmina_option_plugin)
	{
		remmina_exec_command(REMMINA_COMMAND_PLUGIN, remmina_option_plugin);
		executed = TRUE;
	}
	if (remmina_option_icon)
	{
		remmina_exec_command(REMMINA_COMMAND_NONE, remmina_option_icon);
		executed = TRUE;
	}
	if (!executed)
	{
		remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);
	}

	g_option_context_free(context);

	return status;
}

static void remmina_on_startup(GApplication *app)
{
	TRACE_CALL("remmina_on_startup");
	remmina_file_manager_init();
	remmina_pref_init();
	remmina_plugin_manager_init();
	remmina_widget_pool_init();
	remmina_sftp_plugin_register();
	remmina_ssh_plugin_register();
	remmina_icon_init();

	g_set_application_name("Remmina");
	gtk_window_set_default_icon_name("remmina");

	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
	                                   REMMINA_DATADIR G_DIR_SEPARATOR_S "icons");
}

static gboolean remmina_on_local_cmdline (GApplication *app, gchar ***arguments, gint *exit_status)
{
	TRACE_CALL("remmina_on_local_cmdline");
	/* Partially unimplemented local command line only used to append the
	 * command line arguments to the help text and to parse the --help/-h
	 * arguments locally instead of forwarding them to the remote instance */
	GOptionContext *context;
	gint i = 0;
	gchar **argv;

	context = g_option_context_new("- The GTK+ Remote Desktop Client");
	g_option_context_add_group(context, gtk_get_option_group(FALSE));
	g_option_context_set_help_enabled(context, TRUE);
	g_option_context_add_main_entries(context, remmina_options, GETTEXT_PACKAGE);
	g_option_context_set_translation_domain(context, GETTEXT_PACKAGE);
	*exit_status = 0;

	// Parse the command line arguments for --help/-h
	argv = *arguments;
	for (i = 0; argv[i] != NULL; i++)
	{
		if ((g_strcmp0(argv[i], "--help") == 0 || g_strcmp0(argv[i], "-h") == 0))
		{
			g_print("%s", g_option_context_get_help(context, TRUE, NULL));
			*exit_status = EXIT_FAILURE;
		}
	}
	g_option_context_free(context);
	// Exit from the local instance whenever the exit status is not zero
	if (*exit_status != 0)
		exit(*exit_status);
	return FALSE;
}

int main(int argc, char* argv[])
{
	TRACE_CALL("main");
	GApplication *app;
	GApplicationClass *app_class;
	int status;

	gdk_set_allowed_backends("x11,broadway,quartz");

	remmina_masterthread_exec_save_main_thread_id();

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

#ifdef HAVE_LIBGCRYPT
# if GCRYPT_VERSION_NUMBER < 0x010600
	gcry_control (GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread);
# endif
	gcry_check_version (NULL);
	gcry_control (GCRYCTL_DISABLE_SECMEM, 0);
	gcry_control (GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif

	gtk_init(&argc, &argv);

	app = g_application_new("org.Remmina", G_APPLICATION_HANDLES_COMMAND_LINE);
	app_class = G_APPLICATION_CLASS(G_OBJECT_GET_CLASS (app));
	app_class->local_command_line = remmina_on_local_cmdline;
	g_signal_connect(app, "startup", G_CALLBACK(remmina_on_startup), NULL);
	g_signal_connect(app, "command-line", G_CALLBACK(remmina_on_command_line), NULL);
	g_application_set_inactivity_timeout(app, 10000);

	status = g_application_run(app, argc, argv);

	if (status == 0 && !g_application_get_is_remote(app))
	{
		gtk_main();
	}

	g_object_unref(app);

	return status;
}
