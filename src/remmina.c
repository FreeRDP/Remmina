/*
 * Remmina - The GTK Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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
#include "remmina_sodium.h"
#include "remmina.h"
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
#include "rmnews.h"
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

gboolean kioskmode;

static GOptionEntry remmina_options[] =
{
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "about",	      'a',  0,			  G_OPTION_ARG_NONE,	       NULL, N_("Show \'About\'"),								     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "connect",	      'c',  0,			  G_OPTION_ARG_FILENAME,       NULL, N_("Connect to desktop described in file (.remmina or type supported by plugin)"),	     "FILE"	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ G_OPTION_REMAINING, '\0', 0,			  G_OPTION_ARG_FILENAME_ARRAY, NULL, N_("Connect to desktop described in file (.remmina or type supported by plugin)"),	     "FILE"	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "edit",	      'e',  0,			  G_OPTION_ARG_FILENAME,       NULL, N_("Edit desktop connection described in file (.remmina or type supported by plugin)"), "FILE"	},
	{ "help",	      '?',  G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_NONE,	       NULL, NULL,										     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "kiosk",	      'k',  0,			  G_OPTION_ARG_NONE,	       NULL, N_("Start in kiosk mode"),							     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "new",	      'n',  0,			  G_OPTION_ARG_NONE,	       NULL, N_("Create new connection profile"),						     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "pref",	      'p',  0,			  G_OPTION_ARG_STRING,	       NULL, N_("Show preferences"),						     "PAGENR"	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "plugin",	      'x',  0,			  G_OPTION_ARG_STRING,	       NULL, N_("Run a plugin"),								     "PLUGIN"	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "quit",	      'q',  0,			  G_OPTION_ARG_NONE,	       NULL, N_("Quit"),							     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "server",	      's',  0,			  G_OPTION_ARG_STRING,	       NULL, N_("Use default server name (for --new)"),						     "SERVER"	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "protocol",	      't',  0,			  G_OPTION_ARG_STRING,	       NULL, N_("Use default protocol (for --new)"),						     "PROTOCOL" },
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "icon",	      'i',  0,			  G_OPTION_ARG_NONE,	       NULL, N_("Start in tray"),								     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "version",	      'v',  0,			  G_OPTION_ARG_NONE,	       NULL, N_("Show the application version"),						     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "full-version",     'V',  0,			  G_OPTION_ARG_NONE,	       NULL, N_("Show version of the application and its plugins"),		     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "update-profile",   0,    0,			  G_OPTION_ARG_FILENAME,       NULL, N_("Modify connection profile, (requires --set-option)"),			     NULL	},
	/// TRANSLATORS: Shown in terminal. Do not use charcters that may be not supported on a terminal
	{ "set-option",	      0,    0,			  G_OPTION_ARG_STRING_ARRAY,   NULL, N_("Set one or more profile settings, to be used with --update-profile"),		     NULL	},
	{ NULL }
};

#ifdef WITH_LIBGCRYPT
static int
_gpg_error_to_errno(gcry_error_t e)
{
	/* be lazy right now */
	if (e == GPG_ERR_NO_ERROR)
		return 0;
	else
		return EINVAL;
}
#endif /* !WITH_LIBGCRYPT */

static gint remmina_on_command_line(GApplication *app, GApplicationCommandLine *cmdline)
{
	TRACE_CALL(__func__);

	gint status = 0;
	gboolean executed = FALSE;
	GVariantDict *opts;
	gchar *str;
	const gchar **remaining_args;
	gchar *protocol;
	gchar *server;

#if SODIUM_VERSION_INT >= 90200
	remmina_sodium_init();
#endif
	remmina_pref_init();

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

	/** @todo This should be a G_OPTION_ARG_FILENAME_ARRAY (^aay) so that
	 * we can implement multi profile connection:
	 *    https://gitlab.com/Remmina/Remmina/issues/915
	 */
	if (g_variant_dict_lookup(opts, "connect", "^ay", &str)) {
		remmina_exec_command(REMMINA_COMMAND_CONNECT, g_strdup(str));
		g_free(str);
		executed = TRUE;
	}

	if (g_variant_dict_lookup(opts, G_OPTION_REMAINING, "^a&ay", &remaining_args)) {
		remmina_exec_command(REMMINA_COMMAND_CONNECT, remaining_args[0]);
		g_free(remaining_args);
		executed = TRUE;
	}

	if (g_variant_dict_lookup(opts, "edit", "^ay", &str)) {
		remmina_exec_command(REMMINA_COMMAND_EDIT, str);
		g_free(str);
		executed = TRUE;
	}

	if (g_variant_dict_lookup_value(opts, "kiosk", NULL)) {
		kioskmode = TRUE;
		remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);
		executed = TRUE;
	}

	if (g_variant_dict_lookup_value(opts, "new", NULL)) {
		if (!g_variant_dict_lookup(opts, "protocol", "&s", &protocol))
			protocol = NULL;

		if (g_variant_dict_lookup(opts, "server", "&s", &server))
			str = g_strdup_printf("%s,%s", protocol, server);
		else
			str = g_strdup(protocol);

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

	if (!executed)
		remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);

	return status;
}

static void remmina_on_startup(GApplication *app)
{
	TRACE_CALL(__func__);

	RemminaSecretPlugin *secret_plugin;

	remmina_widget_pool_init();
	remmina_sftp_plugin_register();
	remmina_ssh_plugin_register();
	remmina_icon_init();

	g_set_application_name("Remmina");
	gtk_window_set_default_icon_name(REMMINA_APP_ID);

	/* Setting the X11 program class (WM_CLASS) is necessary to group
	* windows with .desktop file which has the same StartupWMClass */
	gdk_set_program_class(REMMINA_APP_ID);

	gtk_icon_theme_append_search_path(gtk_icon_theme_get_default(),
					  REMMINA_RUNTIME_DATADIR G_DIR_SEPARATOR_S "icons");
	g_application_hold(app);

	remmina_stats_sender_schedule();
	rmnews_schedule();

	/* Check for secret plugin and service initialization and show console warnings if
	 * something is missing */
	secret_plugin = remmina_plugin_manager_get_secret_plugin();
	if (!secret_plugin) {
		g_print("WARNING: Remmina is running without a secret plugin. Passwords will be saved in a less secure way.\n");
	} else {
		if (!secret_plugin->is_service_available())
			g_print("WARNING: Remmina is running with a secret plugin, but it cannot connect to a secret service.\n");
	}
}

static gint remmina_on_local_cmdline(GApplication *app, GVariantDict *opts, gpointer user_data)
{
	TRACE_CALL(__func__);

	int status = -1;
	gchar *str;
	gchar **settings;

	/* Here you handle any command line options that you want to be executed
	 * in the local instance (the non-unique instance) */

	if (g_variant_dict_lookup_value(opts, "version", NULL)) {
		remmina_exec_command(REMMINA_COMMAND_VERSION, NULL);
		status = 0;
	}

	if (g_variant_dict_lookup_value(opts, "full-version", NULL)) {
		remmina_exec_command(REMMINA_COMMAND_FULL_VERSION, NULL);
		status = 0;
	}

	if (g_variant_dict_lookup(opts, "update-profile", "^&ay", &str)) { /* ^&ay no need to free */
		if (g_variant_dict_lookup(opts, "set-option", "^a&s", &settings)) {
			if (settings != NULL) {
				status = remmina_exec_set_setting(str, settings);
				g_free(settings);
			} else {
				status = 1;
			}
		} else {
			status = 1;
			g_print("Error: --update-profile requires --set-option\n");
		}
	}

	/* Returning a non negative value here makes the application exit */
	return status;
}

int main(int argc, char *argv[])
{
	TRACE_CALL(__func__);
	GtkApplication *app;
	const gchar *app_id;
	int status;

	/* Enable wayland backend only after GTK 3.22.27 or the clipboard
	 * will not work. See GTK bug 790031 */
	if (remmina_gtk_check_version(3, 22, 27))
		gdk_set_allowed_backends("wayland,x11,broadway,quartz,mir");
	else
		gdk_set_allowed_backends("x11,broadway,quartz,mir");

	remmina_masterthread_exec_save_main_thread_id();

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);

#ifdef HAVE_LIBGCRYPT
# if GCRYPT_VERSION_NUMBER < 0x010600
	gcry_error_t e;
	if (!gcrypt_thread_initialized) {
		if ((e = gcry_control(GCRYCTL_SET_THREAD_CBS, &gcry_threads_pthread)) != GPG_ERR_NO_ERROR)
			return -1;
		gcrypt_thread_initialized++;
	}
#endif  /* !GCRYPT_VERSION_NUMBER */
	gcry_check_version(NULL);
	gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
	gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
#endif  /* !HAVE_LIBGCRYPT */

	/* Initialize some Remmina parts needed also on a local instance for correct handle-local-options */
	remmina_pref_init();
	remmina_file_manager_init();
	remmina_plugin_manager_init();


	app_id = g_application_id_is_valid(REMMINA_APP_ID) ? REMMINA_APP_ID : NULL;
	app = gtk_application_new(app_id, G_APPLICATION_HANDLES_COMMAND_LINE);
#if !GTK_CHECK_VERSION(4, 0, 0) /* This is not needed anymore starting from GTK 4 */
	g_set_prgname(app_id);
#endif
	g_signal_connect(app, "startup", G_CALLBACK(remmina_on_startup), NULL);
	g_signal_connect(app, "command-line", G_CALLBACK(remmina_on_command_line), NULL);
	g_signal_connect(app, "handle-local-options", G_CALLBACK(remmina_on_local_cmdline), NULL);

	g_application_add_main_option_entries(G_APPLICATION(app), remmina_options);

	g_application_set_inactivity_timeout(G_APPLICATION(app), 10000);
	status = g_application_run(G_APPLICATION(app), argc, argv);
	g_object_unref(app);

	return status;
}
