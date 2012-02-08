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

static gboolean remmina_option_about;
static gchar *remmina_option_connect;
static gchar *remmina_option_edit;
static gboolean remmina_option_help;
static gboolean remmina_option_new;
static gchar *remmina_option_pref;
static gchar *remmina_option_plugin;
static gboolean remmina_option_quit;
static gchar *remmina_option_server;
static gchar *remmina_option_protocol;

static GOptionEntry remmina_options[] =
{
{ "about", 'a', 0, G_OPTION_ARG_NONE, &remmina_option_about, "Show about dialog", NULL },
{ "connect", 'c', 0, G_OPTION_ARG_FILENAME, &remmina_option_connect, "Connect to a .remmina file F", "F" },
{ "edit", 'e', 0, G_OPTION_ARG_FILENAME, &remmina_option_edit, "Edit a .remmina file F", "F" },
{ "help", '?', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE, &remmina_option_help, NULL, NULL },
{ "new", 'n', 0, G_OPTION_ARG_NONE, &remmina_option_new, "Create a new connection profile", NULL },
{ "pref", 'p', 0, G_OPTION_ARG_STRING, &remmina_option_pref, "Show preference dialog", NULL },
{ "plugin", 'x', 0, G_OPTION_ARG_STRING, &remmina_option_plugin, "Execute plugin P", "P" },
{ "quit", 'q', 0, G_OPTION_ARG_NONE, &remmina_option_quit, "Quit the application", NULL },
{ "server", 's', 0, G_OPTION_ARG_STRING, &remmina_option_server, "Use default server name S", "S" },
{ "protocol", 't', 0, G_OPTION_ARG_STRING, &remmina_option_protocol, "Use default protocol T", "T" },
{ NULL } };

static gint remmina_on_command_line(GApplication *app, GApplicationCommandLine *cmdline)
{
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

	argv = g_application_command_line_get_arguments(cmdline, &argc);

	context = g_option_context_new("- The GTK+ Remote Desktop Client");
	g_option_context_add_main_entries(context, remmina_options, GETTEXT_PACKAGE);
	g_option_context_set_help_enabled(context, FALSE);
	parsed = g_option_context_parse(context, &argc, &argv, &error);
	g_strfreev(argv);

	if (!parsed)
	{
		g_print("option parsing failed: %s\n", error->message);
		status = 1;
	}

	if (remmina_option_quit)
	{
		gtk_main_quit();
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
	if (!executed)
	{
		remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);
	}

	g_option_context_free(context);

	return status;
}

static void remmina_on_startup(GApplication *app)
{
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

int main(int argc, char* argv[])
{
	GApplication* app;
	int status;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

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

	gtk_init(&argc, &argv);

	app = g_application_new("org.Remmina", G_APPLICATION_HANDLES_COMMAND_LINE);
	g_signal_connect(app, "startup", G_CALLBACK(remmina_on_startup), NULL);
	g_signal_connect(app, "command-line", G_CALLBACK(remmina_on_command_line), NULL);
	g_application_set_inactivity_timeout(app, 10000);

	status = g_application_run(app, argc, argv);

	if (status == 0 && !g_application_get_is_remote(app))
	{
		THREADS_ENTER
		gtk_main();
		THREADS_LEAVE
	}

	g_object_unref(app);

	return status;
}

