/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2017 Antenore Gatta, Giovanni Panozzo
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
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "config.h"
#include "remmina_exec.h"
#include "remmina_file_manager.h"
#include "remmina_icon.h"
#include "remmina_main.h"
#include "remmina_masterthread_exec.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref.h"
#include "remmina_public.h"
#include "remmina_sftp_plugin.h"
#include "remmina_ssh_plugin.h"
#include "remmina_widget_pool.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_stats_sender.h"


#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#include <pthread.h>
#ifdef HAVE_LIBGCRYPT
#include <gcrypt.h>
# if GCRYPT_VERSION_NUMBER < 0x010600
GCRY_THREAD_OPTION_PTHREAD_IMPL;
#endif  /* !GCRYPT_VERSION_NUMBER */
#endif  /* HAVE_LIBGCRYPT */

#ifdef HAVE_LIBGCRYPT
# if GCRYPT_VERSION_NUMBER < 0x010600
static int gcrypt_thread_initialized = 0;
#endif  /* !GCRYPT_VERSION_NUMBER */
#endif  /* HAVE_LIBGCRYPT */

static GOptionEntry remmina_options[] =
{
	{ "about",	  'a', 0,		     G_OPTION_ARG_NONE,	    NULL, N_("Show about dialog"),					       NULL	  },
	{ "connect",	  'c', 0,		     G_OPTION_ARG_FILENAME, NULL, N_("Connect to a .remmina file"),				       "FILE"	  },
	{ "edit",	  'e', 0,		     G_OPTION_ARG_FILENAME, NULL, N_("Edit a .remmina file"),					       "FILE"	  },
	{ "help",	  '?', G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,	    NULL, NULL,								       NULL	  },
	{ "new",	  'n', 0,		     G_OPTION_ARG_NONE,	    NULL, N_("Create a new connection profile"),			       NULL	  },
	{ "pref",	  'p', 0,		     G_OPTION_ARG_STRING,   NULL, N_("Show preferences dialog page"),				       "PAGENR"	  },
	{ "plugin",	  'x', 0,		     G_OPTION_ARG_STRING,   NULL, N_("Execute the plugin"),					       "PLUGIN"	  },
	{ "quit",	  'q', 0,		     G_OPTION_ARG_NONE,	    NULL, N_("Quit the application"),					       NULL	  },
	{ "server",	  's', 0,		     G_OPTION_ARG_STRING,   NULL, N_("Use default server name (for --new)"),			       "SERVER"	  },
	{ "protocol",	  't', 0,		     G_OPTION_ARG_STRING,   NULL, N_("Use default protocol (for --new)"),			       "PROTOCOL" },
	{ "icon",	  'i', 0,		     G_OPTION_ARG_NONE,	    NULL, N_("Start as tray icon"),					       NULL	  },
	{ "version",	  'v', 0,		     G_OPTION_ARG_NONE,	    NULL, N_("Show the application's version"),				       NULL	  },
	{ "full-version", 'V', 0,		     G_OPTION_ARG_NONE,	    NULL, N_("Show the application's version, including the pulgin versions"), NULL	  },
	{ NULL }
};

#ifdef WITH_LIBGCRYPT
static int
_gpg_error_to_errno(gcry_error_t e)
{
	/* be lazy right now */
	if (e == GPG_ERR_NO_ERROR)
		return (0);
	else
		return (EINVAL);
}
#endif /* !WITH_LIBGCRYPT */

static gint remmina_on_command_line(GApplication *app, GApplicationCommandLine *cmdline)
{
	TRACE_CALL(__func__);

	gint status = 0;
	gboolean executed = FALSE;
	GVariantDict *opts;
	gchar *str;
	gchar *protocol;
	gchar *server;

	opts = g_application_command_line_get_options_dict(cmdline);

	if (g_variant_dict_lookup_value(opts, "quit", NULL)) {
		remmina_exec_command(REMMINA_COMMAND_EXIT, NULL);
		executed = TRUE;
		status = 1;
	}

	if (g_variant_dict_lookup_value(opts, "about", NULL)) {
		remmina_exec_command(REMMINA_COMMAND_ABOUT, NULL);
		executed = TRUE;
	}

	if (g_variant_dict_lookup(opts, "connect", "^ay", &str)) {
		remmina_exec_command(REMMINA_COMMAND_CONNECT, str);
		g_free(str);
		executed = TRUE;
	}

	if (g_variant_dict_lookup(opts, "edit", "^ay", &str)) {
		remmina_exec_command(REMMINA_COMMAND_EDIT, str);
		g_free(str);
		executed = TRUE;
	}

	if (g_variant_dict_lookup_value(opts, "new", NULL)) {
		if (!g_variant_dict_lookup(opts, "protocol", "&s", &protocol))
			protocol = NULL;

		if (g_variant_dict_lookup(opts, "server", "&s", &server)) {
			str = g_strdup_printf("%s,%s", protocol, server);
		}else  {
			str = g_strdup(protocol);
		}

		remmina_exec_command(REMMINA_COMMAND_NEW, str);
		g_free(str);
		executed = TRUE;
	}

	if (g_variant_dict_lookup(opts, "pref", "&s", &str)) {
		remmina_exec_command(REMMINA_COMMAND_PREF, str);
		executed = TRUE;
	}

	if (g_variant_dict_lookup(opts, "plugin", "&s", &str)) {
		remmina_exec_command(REMMINA_COMMAND_PLUGIN, str);
		executed = TRUE;
	}

	if (g_variant_dict_lookup_value(opts, "icon", NULL)) {
		remmina_exec_command(REMMINA_COMMAND_NONE, NULL);
		executed = TRUE;
	}

	if (g_variant_dict_lookup_value(opts, "version", NULL)) {
		remmina_exec_command(REMMINA_COMMAND_VERSION, NULL);
		executed = TRUE;
	}

	if (g_variant_dict_lookup_value(opts, "full-version", NULL)) {
		remmina_exec_command(REMMINA_COMMAND_FULL_VERSION, NULL);
		executed = TRUE;
	}

	if (!executed) {
		remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);
	}

	return status;
}

static void remmina_on_startup(GApplication *app)
{
	TRACE_CALL(__func__);
	remmina_file_manager_init();
	remmina_pref_init();
	remmina_plugin_manager_init();
	remmina_widget_pool_init();
	remmina_sftp_plugin_register();
	remmina_ssh_plugin_register();
	remmina_icon_init();

	g_set_application_name("Remmina");
	gtk_window_set_default_icon_name("remmina");

	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
		REMMINA_RUNTIME_DATADIR G_DIR_SEPARATOR_S "icons");
	g_application_hold(app);

	remmina_stats_sender_schedule();
}

static gint remmina_on_local_cmdline(GApplication *app, GVariantDict *options, gpointer user_data)
{
	TRACE_CALL(__func__);

	int status = -1;

	/* Here you handle any command line options that you want to be executed
	 * from command line, one time, and than exit */

	return status;
}

int main(int argc, char* argv[])
{
	TRACE_CALL(__func__);
	GtkApplication *app;
	const gchar *app_id;
	int status;

	gdk_set_allowed_backends("wayland,x11,broadway,quartz,mir");

	remmina_masterthread_exec_save_main_thread_id();

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

#ifdef HAVE_LIBGCRYPT
# if GCRYPT_VERSION_NUMBER < 0x010600
	gcry_error_t e;
	if (!gcrypt_thread_initialized) {
		if ((e = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread)) != GPG_ERR_NO_ERROR) {
			return (-1);
		}
		gcrypt_thread_initialized++;
	}
#endif  /* !GCRYPT_VERSION_NUMBER */
	gcry_check_version(NULL);
	gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif  /* !HAVE_LIBGCRYPT */

	app_id = g_application_id_is_valid(UNIQUE_APPNAME) ? UNIQUE_APPNAME : NULL;
	app = gtk_application_new(app_id, G_APPLICATION_HANDLES_COMMAND_LINE);
	g_signal_connect(app, "startup", G_CALLBACK(remmina_on_startup), NULL);
	g_signal_connect(app, "command-line", G_CALLBACK(remmina_on_command_line), NULL);
	g_signal_connect(app, "handle-local-options", G_CALLBACK(remmina_on_local_cmdline), NULL);

	g_application_add_main_option_entries(G_APPLICATION(app), remmina_options);

	g_application_set_inactivity_timeout(G_APPLICATION(app), 10000);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
