/*
 *     Project: Remmina Plugin X2Go
 * Description: Remmina protocol plugin to connect via X2Go using PyHocaCLI
 *      Author: Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
 *              Antenore Gatta <antenore@simbiosi.org>
 *   Copyright: 2010-2011 Vic Lee
 *              2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 *              2015 Antenore Gatta
 *              2016-2018 Antenore Gatta, Giovanni Panozzo
 *              2019 Mike Gabriel
 *              2021 Daniel Teichmann
 *     License: GPL-2+
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

#include "x2go_plugin.h"
#include "common/remmina_plugin.h"

#include <gtk/gtkx.h>
#include <gdk/gdkx.h>

#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>

#include <sys/types.h>
#include <signal.h>
#include <time.h>

#define FEATURE_AVAILABLE(gpdata, feature) \
	gpdata->available_features ? \
						(g_list_find_custom(gpdata->available_features, \
											feature, \
											remmina_plugin_x2go_safe_strcmp) \
											? TRUE : FALSE) : FALSE

#define GET_PLUGIN_DATA(gp) \
	(RemminaPluginX2GoData*) g_object_get_data(G_OBJECT(gp), "plugin-data")

#define SET_DIALOG_DATA(gp, ddata) \
	g_object_set_data_full(G_OBJECT(gp), "dialog-data", ddata, g_free);

#define GET_DIALOG_DATA(gp) \
	(DialogData*) g_object_get_data(G_OBJECT(gp), "dialog-data");

#define REMMINA_PLUGIN_INFO(fmt, ...)\
	remmina_plugin_service->_remmina_info("[%s] " fmt, PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_MESSAGE(fmt, ...)\
	remmina_plugin_service->_remmina_message("[%s] " fmt, PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_DEBUG(fmt, ...)\
	remmina_plugin_service->_remmina_debug(__func__, "[%s] " fmt, \
										   PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_WARNING(fmt, ...)\
	remmina_plugin_service->_remmina_warning(__func__, "[%s] " fmt, \
											 PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_ERROR(fmt, ...)\
	remmina_plugin_service->_remmina_error(__func__, "[%s] " fmt, \
										   PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_CRITICAL(fmt, ...)\
	remmina_plugin_service->_remmina_critical(__func__, "[%s] " fmt, \
											  PLUGIN_NAME, ##__VA_ARGS__)

#define GET_PLUGIN_STRING(value) \
		g_strdup(remmina_plugin_service->file_get_string(remminafile, value))
#define GET_PLUGIN_PASSWORD(value) \
		GET_PLUGIN_STRING(value)
#define GET_PLUGIN_INT(value, default_value) \
		remmina_plugin_service->file_get_int(remminafile, value, default_value)
#define GET_PLUGIN_BOOLEAN(value) \
		remmina_plugin_service->file_get_int(remminafile, value, FALSE)

static RemminaPluginService *remmina_plugin_service = NULL;

// Wrapper for strcmp which doesn't throw an exception when 'a' or 'b' are a nullpointer.
static gint remmina_plugin_x2go_safe_strcmp(gconstpointer a, gconstpointer b) {
	if (a && b) {
		return strcmp(a, b);
	}

	return -1;
}

/**
 * DialogData:
 * @flags: see GtkDialogFlags
 * @type: see GtkMessageType
 * @buttons: see GtkButtonsType
 * @title: Title of the Dialog
 * @message: Message of the Dialog
 * @callbackfunc: A GCallback function like
 * 				  callback(RemminaProtocolWidget *gp, GtkWidget *dialog)
 * 				  which will be executed on the dialogs 'response' signal.
 *				  The callback function is not obliged to destroy
 *				  the dialog widget.
 *
 * The `DialogData` structure contains all information needed
 * to open a gtk dialog with remmina_plugin_x2go_open_dialog()
 *
 * Quick example of a callback function.
 * static void remmina_plugin_x2go_test_callback(RemminaProtocolWidget *gp,
 *											     gint response_id,
 * 											     GtkDialog *self) {
 *		REMMINA_PLUGIN_DEBUG("response: %i", response_id);
 *		if (response_id == GTK_RESPONSE_OK) {
 *	 		REMMINA_PLUGIN_DEBUG("OK!");
 * 		}
 * 		gtk_widget_destroy(self);
 * }
 *
 *
 */

struct _DialogData {
  GtkWindow      *parent;
  GtkDialogFlags flags;
  GtkMessageType type;
  GtkButtonsType buttons;
  gchar          *title;
  gchar          *message;
  GCallback      callbackfunc;
};
typedef struct _DialogData DialogData;

static void remmina_plugin_x2go_open_dialog(RemminaProtocolWidget *gp) {
	REMMINA_PLUGIN_DEBUG("Function entry.");

	DialogData *ddata = GET_DIALOG_DATA(gp);

	if (ddata) {
		// Can't check type, flags or buttons
		// because they are enums and '0' is a valid value
		if (!ddata->title || !ddata->message) {
			REMMINA_PLUGIN_CRITICAL("%s", _("Broken DialogData! Aborting."));
			return;
		}
	} else {
		REMMINA_PLUGIN_CRITICAL("%s", _("Can't retrieve DialogData! Aborting."));
		return;
	}

	REMMINA_PLUGIN_DEBUG("DialogData checks passed. Dialog will now be shown.");

	GtkWidget *widget_gtk_dialog;
	widget_gtk_dialog = gtk_message_dialog_new(ddata->parent,
											   ddata->flags,
											   ddata->type,
											   ddata->buttons,
											   ddata->title);

	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG (widget_gtk_dialog),
											 ddata->message);

	if (ddata->callbackfunc) {
		g_signal_connect_swapped(G_OBJECT(widget_gtk_dialog),
					 "response",
					 G_CALLBACK(ddata->callbackfunc),
					 gp);
	} else {
		g_signal_connect(G_OBJECT(widget_gtk_dialog),
						"response",
						G_CALLBACK(gtk_widget_destroy),
						NULL);
	}

	gtk_widget_show_all(widget_gtk_dialog);
	g_free(ddata);
}

typedef struct _RemminaPluginX2GoData {
	GtkWidget *socket;
	gint socket_id;

	pthread_t thread;

	Display *display;
	Window window_id;
	int (*orig_handler)(Display *, XErrorEvent *);

	GPid pidx2go;

	gboolean disconnected;

	GList* available_features;
} RemminaPluginX2GoData;

#define REMMINA_PLUGIN_X2GO_FEATURE_GTKSOCKET 1

/* Forward declaration */
static RemminaProtocolPlugin remmina_plugin_x2go;

/* When more than one NX sessions is connecting in progress, we need this mutex and array
 * to prevent them from stealing the same window id.
 */
static pthread_mutex_t remmina_x2go_init_mutex;
static GArray *remmina_x2go_window_id_array;

/* --------- Support for execution on main thread of GTK functions -------------- */

struct onMainThread_cb_data {
	enum { FUNC_GTK_SOCKET_ADD_ID } func;

	GtkSocket* sk;
	Window w;

	/* Mutex for thread synchronization */
	pthread_mutex_t mu;
	/* Flag to catch cancellations */
	gboolean cancelled;
};

static gboolean onMainThread_cb(struct onMainThread_cb_data *d) {
	TRACE_CALL(__func__);
	if ( !d->cancelled ) {
		switch ( d->func ) {
		case FUNC_GTK_SOCKET_ADD_ID:
			gtk_socket_add_id( d->sk, d->w );
			break;
		}
		pthread_mutex_unlock( &d->mu );
	} else {
		/* thread has been cancelled, so we must free d memory here */
		g_free( d );
	}
	return G_SOURCE_REMOVE;
}


static void onMainThread_cleanup_handler(gpointer data) {
	TRACE_CALL(__func__);
	struct onMainThread_cb_data *d = data;
	d->cancelled = TRUE;
}

static void onMainThread_schedule_callback_and_wait( struct onMainThread_cb_data *d) {
	TRACE_CALL(__func__);
	d->cancelled = FALSE;
	pthread_cleanup_push( onMainThread_cleanup_handler, d );
	pthread_mutex_init( &d->mu, NULL );
	pthread_mutex_lock( &d->mu );
	gdk_threads_add_idle( (GSourceFunc)onMainThread_cb, (gpointer)d );

	pthread_mutex_lock( &d->mu );

	pthread_cleanup_pop(0);
	pthread_mutex_unlock( &d->mu );
	pthread_mutex_destroy( &d->mu );
}

static void onMainThread_gtk_socket_add_id( GtkSocket* sk, Window w) {
	TRACE_CALL(__func__);

	struct onMainThread_cb_data *d;

	d = g_new0(struct onMainThread_cb_data, 1);
	d->func = FUNC_GTK_SOCKET_ADD_ID;
	d->sk = sk;
	d->w = w;

	onMainThread_schedule_callback_and_wait( d );
	g_free(d);
}

/* --------------------------------------- */

static void remmina_plugin_x2go_remove_window_id (Window window_id) {
	gint i;
	gboolean already_seen = FALSE;

	pthread_mutex_lock (&remmina_x2go_init_mutex);
	for (i = 0; i < remmina_x2go_window_id_array->len; i++)
	{
		if (g_array_index (remmina_x2go_window_id_array, Window, i) == window_id)
		{
			already_seen = TRUE;
			REMMINA_PLUGIN_DEBUG("X2Go Agent window with ID [0x%lx] already seen.",
								 window_id);
			break;
		}
	}
	if (already_seen)
	{
		g_array_remove_index_fast (remmina_x2go_window_id_array, i);
		REMMINA_PLUGIN_DEBUG("forgetting about X2Go Agent window with ID [0x%lx]",
							 window_id);
	}
	pthread_mutex_unlock (&remmina_x2go_init_mutex);
}

static void remmina_plugin_x2go_cleanup(RemminaProtocolWidget *gp) {
	REMMINA_PLUGIN_DEBUG("Function entry.");

	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	if (gpdata == NULL) {
		REMMINA_PLUGIN_DEBUG("gpdata was already null. Exiting.");
		return;
	}

	if (gpdata->thread) {
		pthread_cancel(gpdata->thread);
		if (gpdata->thread)
			pthread_join(gpdata->thread, NULL);
	}

	if (gpdata->window_id) {
		remmina_plugin_x2go_remove_window_id(gpdata->window_id);
	}

	if (gpdata->pidx2go) {
		kill(gpdata->pidx2go, SIGTERM);
		g_spawn_close_pid(gpdata->pidx2go);
		gpdata->pidx2go = 0;
	}

	if (gpdata->display) {
		XSetErrorHandler(gpdata->orig_handler);
		XCloseDisplay(gpdata->display);
		gpdata->display = NULL;
	}

	g_object_steal_data(G_OBJECT(gp), "plugin-data");
	remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
}

static gboolean remmina_plugin_x2go_close_connection(RemminaProtocolWidget *gp) {
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);

	REMMINA_PLUGIN_DEBUG("function entry.");

	if (gpdata->disconnected) {
		REMMINA_PLUGIN_DEBUG("plugin disconnected already. No need for a second time.");
		return FALSE;
	}

	remmina_plugin_x2go_cleanup(gp);

	return TRUE;
}

static void remmina_plugin_x2go_pyhoca_cli_exited(GPid pid,
												  int status,
												  struct _RemminaProtocolWidget *gp) {
	REMMINA_PLUGIN_DEBUG("Function entry.");

	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	if (!gpdata) {
		REMMINA_PLUGIN_DEBUG("gpdata already null. Doing nothing then.");
		return;
	}
	
	if (gpdata->pidx2go <= 0) {
		REMMINA_PLUGIN_DEBUG("pidx2go <= 0! -> Doing nothing then.");
		return;
	}

	REMMINA_PLUGIN_CRITICAL("%s", _("pyhoca-cli exited unexpectedly. "
							        "This connection will now be closed."));

	DialogData *ddata = g_new0(DialogData, 1);
	SET_DIALOG_DATA(gp, ddata);
	ddata->parent = NULL;
	ddata->flags = GTK_DIALOG_MODAL;
	ddata->type = GTK_MESSAGE_ERROR;
	ddata->buttons = GTK_BUTTONS_OK;
	ddata->title = N_("An error occured.");
	ddata->message = N_("Necessary child process 'pyhoca-cli' stopped unexpectedly.\n"
						"Please check pyhoca-cli's output for errors and "
						"check your profile settings for possible errors.");
	ddata->callbackfunc = NULL; // Dialog frees itself, no need for an callbackfunc.
	IDLE_ADD((GSourceFunc) remmina_plugin_x2go_open_dialog, gp);

	// 1 Second. Give Dialog chance to open.
	usleep(1000 * 1000);

	remmina_plugin_x2go_close_connection(gp);
}

// Returns either all features separated with a '\n' or NULL if it failed.
static gchar* remmina_plugin_x2go_get_pyhoca_features() {
	REMMINA_PLUGIN_DEBUG("Function entry.");

	// We will now start pyhoca-cli with only the '--list-cmdline-features' option
	// and depending on the exit code and stdout output we will determine if some features
	// are available or not.
	gchar *argv[50];
	gint argc = 0;
	GError *error = NULL;
	gint exit_code = 0;
	gchar *standard_out;
	// just supresses pyhoca-cli help message. (When pyhoca-cli has old version)
	gchar *standard_err;

	argv[argc++] = g_strdup("pyhoca-cli");
	argv[argc++] = g_strdup("--list-cmdline-features");
	argv[argc++] = NULL;

	gchar **envp = g_get_environ();
	gboolean success_ret = g_spawn_sync (NULL, argv, envp, G_SPAWN_SEARCH_PATH,
									     NULL, NULL, &standard_out, &standard_err,
										 &exit_code, &error);

	if (!success_ret || error || strcmp(standard_out, "") == 0 || exit_code) {
		if (!error) {
			REMMINA_PLUGIN_WARNING("%s",
						g_strdup_printf(_("An unknown error happened while retrieving "
										  "pyhoca-cli's cmdline features! "
								          "Exit code: %i"), exit_code));
		} else {
			REMMINA_PLUGIN_WARNING("%s",
								   g_strdup_printf(_("Error: '%s'"), error->message));
			g_error_free(error);
		}

		return NULL;
	}

	return standard_out;
}

// Returns a string (gchar*) array (gchar**) of all available features
// if the parse was sucessfull. If not NULL gets returned and size == 0;
static gchar** remmina_plugin_x2go_parse_pyhoca_features(gchar* features_string,
												         uint32_t *size) {
    REMMINA_PLUGIN_DEBUG("Function entry.");
	char delim[] = "\n";

	// Counts the occurence of '\n', so the number of features passed.
	int i = 0;
	// work on a copy of the string, because strchr alters the string.
	char *pch = strchr(g_strdup(features_string), '\n');
	while (pch != NULL) {
		i++;
		pch = strchr(pch+1, '\n');
	}

	if (i <= 0) {
		// Something went wrong, there can't be 0 features.
		return NULL;
		*size = 0;
	}

	gchar **feature_list = NULL;
	// 50 characters per feature should do the trick, right?
	feature_list = malloc(sizeof(char) * i * 50);
	*size = i;

	// Split 'features_string' into array 'feature_list' using 'delim' as delimiter.
	char *ptr = strtok(g_strdup(features_string), delim);
	for(gint j = 0; (j < i && ptr != NULL); j++) {
		// Add feature to list
		feature_list[j] = g_strdup_printf(ptr);

		// Get next feature
		ptr = strtok(NULL, delim);
	}

	return feature_list;
}

static gboolean remmina_plugin_x2go_exec_x2go(gchar *host,
                                              gint sshport,
                                              gchar *username,
                                              gchar *password,
                                              gchar *command,
                                              gchar *kbdlayout,
                                              gchar *kbdtype,
                                              gchar *audio,
                                              gchar *clipboard,
                                              gint dpi,
                                              gchar *resolution,
                                              RemminaProtocolWidget *gp,
                                              gchar *errmsg) {
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	GError *error = NULL;
	gchar *s_username, *s_password;
	gint ret;
	gboolean save;
	gboolean password_storing_disabled;
	RemminaFile *remminafile;

	gchar **envp;

	gchar *argv[50];
	gint argc = 0;
	gint i = 0;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	password_storing_disabled = remmina_plugin_service->file_get_int(
																 remminafile,
																 "disablepasswordstoring",
																 FALSE);
	ret = remmina_plugin_service->protocol_plugin_init_auth(
			gp,
			(password_storing_disabled ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD |
											 REMMINA_MESSAGE_PANEL_FLAG_USERNAME),
			_("Enter X2Go credentials"),
			username, // function arg 'username' is default username
			password, // function arg 'password' is default password
			NULL,
			NULL);


	if (ret == GTK_RESPONSE_OK) {
		s_username = remmina_plugin_service->protocol_plugin_init_get_username(gp);
		s_password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		if (remmina_plugin_service->protocol_plugin_init_get_savepassword(gp))
			remmina_plugin_service->file_set_string(remminafile, "password", s_password);

		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save) {
			// User has requested to save credentials. We put all the new credentials
			// into remminafile->settings. They will be saved later, on successful
			// connection, by rcw.c

			if (s_password) {
				remmina_plugin_service->file_set_string(remminafile,
														"password",
														s_password);
			} else {
				REMMINA_PLUGIN_WARNING("%s", _("User has requested to save credentials "
											   "but 'password' is not set! Can't set "
											   "a new default password then."));
			}
		}
		if (s_username) {
			g_stpcpy(username, s_username);
			g_free(s_username);
		}
		if (s_password) {
			g_stpcpy(password, s_password);
			g_free(s_password);
		}
	} else  {
		g_strlcpy(errmsg, "Authentication cancelled. Aborting.", 512);
		REMMINA_PLUGIN_DEBUG("%s", errmsg);
		return FALSE;
	}

	argc = 0;
	argv[argc++] = g_strdup("pyhoca-cli");

	argv[argc++] = g_strdup("--server"); // Not listed as feature.
	argv[argc++] = g_strdup_printf ("%s", host);

	if (FEATURE_AVAILABLE(gpdata, "REMOTE_SSH_PORT")) {
		argv[argc++] = g_strdup("-p");
		argv[argc++] = g_strdup_printf ("%d", sshport);
	}

	if (FEATURE_AVAILABLE(gpdata, "USERNAME")) {
		argv[argc++] = g_strdup("-u");
		if (username){
			argv[argc++] = g_strdup_printf ("%s", username);
		} else {
			argv[argc++] = g_strdup_printf ("%s", g_get_user_name());
		}
	}

	if (password && FEATURE_AVAILABLE(gpdata, "PASSWORD")) {
		argv[argc++] = g_strdup("--force-password");
		argv[argc++] = g_strdup("--password");
		argv[argc++] = g_strdup_printf ("%s", password);
	}

	if (FEATURE_AVAILABLE(gpdata, "AUTH_ATTEMPTS")) {
		argv[argc++] = g_strdup("--auth-attempts");
		argv[argc++] = g_strdup_printf ("%i", 0);
	}

	if (FEATURE_AVAILABLE(gpdata, "COMMAND")) {
		argv[argc++] = g_strdup("-c");
	    //  FIXME: pyhoca-cli is picky about multiple quotes around the command string...
	    //	argv[argc++] = g_strdup_printf ("%s", g_shell_quote(command));
		argv[argc++] = g_strdup(command);
	}
	
	if (FEATURE_AVAILABLE(gpdata, "KBD_LAYOUT")) {
		if (kbdlayout) {
			argv[argc++] = g_strdup("--kbd-layout");
			argv[argc++] = g_strdup_printf ("%s", kbdlayout);
		}
	}

	if (FEATURE_AVAILABLE(gpdata, "KBD_TYPE")) {
		if (kbdtype) {
			argv[argc++] = g_strdup("--kbd-type");
			argv[argc++] = g_strdup_printf ("%s", kbdtype);
		} else {
			argv[argc++] = g_strdup("--kbd-type");
			argv[argc++] = g_strdup("auto");
		}
	}

	if (FEATURE_AVAILABLE(gpdata, "GEOMETRY")) {
		if (!resolution)
			resolution = "800x600";
		argv[argc++] = g_strdup("-g");
		argv[argc++] = g_strdup_printf ("%s", resolution);
	}

	if (FEATURE_AVAILABLE(gpdata, "TRY_RESUME")) {
		argv[argc++] = g_strdup("--try-resume");
	}

	if (FEATURE_AVAILABLE(gpdata, "TERMINATE_ON_CTRL_C")) {
		argv[argc++] = g_strdup("--terminate-on-ctrl-c");
	}

	if (FEATURE_AVAILABLE(gpdata, "SOUND")) {
		if (audio) {
			argv[argc++] = g_strdup("--sound");
			argv[argc++] = g_strdup_printf ("%s", audio);
		} else {
			argv[argc++] = g_strdup("--sound");
			argv[argc++] = g_strdup("none");
		}
	}

	if (clipboard && FEATURE_AVAILABLE(gpdata, "CLIPBOARD_MODE")) {
		argv[argc++] = g_strdup("--clipboard-mode");
		argv[argc++] = g_strdup_printf ("%s", clipboard);
	}

	if (FEATURE_AVAILABLE(gpdata, "DPI")) {
		// Values are extracted from pyhoca-cli.
		if (dpi < 20 || dpi > 400) {
			g_strlcpy(errmsg, N_("DPI setting is out of bounds. "
							     "Please adjust in profile settings!"), 512);
			// No need, start_session() will handle output.
			//REMMINA_PLUGIN_CRITICAL("%s", errmsg);
			return FALSE;
		}
		argv[argc++] = g_strdup("--dpi");
		argv[argc++] = g_strdup_printf ("%i", dpi);
	}

	argv[argc++] = NULL;

	envp = g_get_environ();
	gboolean success = g_spawn_async_with_pipes (NULL, argv, envp,
												 G_SPAWN_DO_NOT_REAP_CHILD |
												 G_SPAWN_SEARCH_PATH, NULL,
												 NULL, &gpdata->pidx2go,
												 NULL, NULL, NULL, &error);

	REMMINA_PLUGIN_DEBUG("Started pyhoca-cli with following arguments:");
	// Print every argument except passwords. Free all arg strings.
	for (i = 0; i < argc - 1; i++) {
		if (strcmp(argv[i], "--password") == 0) {
			g_printf("%s ", argv[i]);
			g_printf("XXXXXX ");
			g_free (argv[i]);
			g_free (argv[++i]);
			continue;
		} else {
			g_printf("%s ", argv[i]);
			g_free (argv[i]);
		}
	}
	g_printf("\n");

	if (!success || error) {
		if (!error)
			error = g_error_new(0, 0, "<error not available>");

		gchar *error_title = N_("An error occured while "
						        "starting a X2Go session.");

		DialogData *ddata = g_new0(DialogData, 1);
		SET_DIALOG_DATA(gp, ddata);
		ddata->parent = NULL;
		ddata->flags = GTK_DIALOG_MODAL;
		ddata->type = GTK_MESSAGE_ERROR;
		ddata->buttons = GTK_BUTTONS_OK;
		ddata->title = N_("An error occured while starting a X2Go session.");
		ddata->message = g_strdup_printf(N_("Failed to start pyhoca-cli (%i): '%s'"),
										 error->code,
										 error->message);
		ddata->callbackfunc = NULL; // Dialog frees itself, no need for an callbackfunc.
		IDLE_ADD((GSourceFunc) remmina_plugin_x2go_open_dialog, gp);

		g_strlcpy(errmsg, error_title, 512);

		// No need to output here. remmina_plugin_x2go_start_session will do this.

		g_error_free(error);

		return FALSE;
	}

	// Prevent a race condition where pyhoca-cli is not
	// started yet (pidx2go == 0) but a watcher is added.
	while (gpdata->pidx2go == 0) {
		REMMINA_PLUGIN_DEBUG("Waiting for pyhoca-cli to start...");
	};

	REMMINA_PLUGIN_DEBUG("Watching child pyhoca-cli process now.");
	g_child_watch_add(gpdata->pidx2go,
					  (GChildWatchFunc) remmina_plugin_x2go_pyhoca_cli_exited,
					  gp);

	return TRUE;
}

// Returns a GList* with all features which pyhoca-cli had before even
// implementing the feature system. (It can't change since its an old version)
static GList* remmina_plugin_x2go_old_pyhoca_features() {
	REMMINA_PLUGIN_DEBUG("Function entry.");

	#define AMOUNT_FEATURES 43
	gchar* features[AMOUNT_FEATURES] = {
		"ADD_TO_KNOWN_HOSTS", "AUTH_ATTEMPTS", "BROKER_PASSWORD", "BROKER_URL",
		"CLEAN_SESSIONS", "COMMAND", "DEBUG", "FORCE_PASSWORD",	"FORWARD_SSHAGENT",
		"GEOMETRY", "KBD_LAYOUT", "KBD_TYPE", "LIBDEBUG", "LIBDEBUG_SFTPXFER", "LINK",
		"LIST_CLIENT_FEATURES", "LIST_DESKTOPS", "LIST_SESSIONS", "NEW", "PACK",
		"PASSWORD", "PDFVIEW_CMD", "PRINTER", "PRINTING", "PRINT_ACTION", "PRINT_CMD",
		"QUIET", "REMOTE_SSH_PORT",	"RESUME", "SAVE_TO_FOLDER",	"SESSION_PROFILE",
		"SESSION_TYPE", "SHARE_DESKTOP", "SHARE_LOCAL_FOLDERS", "SHARE_MODE", "SOUND",
		"SSH_PRIVKEY", "SUSPEND", "TERMINATE", "TERMINATE_ON_CTRL_C", "TRY_RESUME",
		"USERNAME", "XINERAMA"
	};

	GList *features_list = NULL;
	for (int i = 0; i < AMOUNT_FEATURES; i++) {
		features_list = g_list_append(features_list, features[i]);
	}
	return features_list;
}

// Returns a GList* which includes all features we *can* use.
static GList* remmina_plugin_x2go_populate_available_features_list() {
	REMMINA_PLUGIN_DEBUG("Function entry.");

	GList* returning_glist = NULL;

	// Querying pyhoca-cli's command line features.
	gchar* features_string = remmina_plugin_x2go_get_pyhoca_features();

	if (!features_string) {
		// We added the '--list-cmdline-features' on commit 17d1be1319ba6 of
		// pyhoca-cli. In order to protect setups which don't have the newest
		// version of pyhoca-cli available yet we artificially create a list
		// of an old limited set of features.

		REMMINA_PLUGIN_WARNING("%s", _("Couldn't get pyhoca-cli's cmdline-features. This "
							   "indicates that either your pyhoca-cli version is too old "
							   "or pyhoca-cli is not installed! An old limited set of "
							   "features will be used now."));

		return remmina_plugin_x2go_old_pyhoca_features();
	} else {
		uint32_t features_size = 0;
		gchar **features = NULL;
		features = remmina_plugin_x2go_parse_pyhoca_features(features_string,
															 &features_size);
		if (features == NULL && features_size > 0) {
			gchar *error_msg = _("parsing pyhoca-cli features was not possible! "
							      "Using limited feature set now.");
			REMMINA_PLUGIN_CRITICAL("%s", error_msg);
			return remmina_plugin_x2go_old_pyhoca_features();
		}

		REMMINA_PLUGIN_INFO("%s", _("Successfully retrived "
									"following pyhoca-cli features:"));

		for(int k = 0; k < features_size; k++) {
			REMMINA_PLUGIN_INFO("%s", g_strdup_printf(_("Available feature[%i]: '%s'"),
														k+1, features[k]));
			returning_glist = g_list_append(returning_glist, features[k]);
		}
		return returning_glist;
	}
}

static void remmina_plugin_x2go_on_plug_added(GtkSocket *socket,
											  RemminaProtocolWidget *gp) {
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	REMMINA_PLUGIN_DEBUG("Socket %d", gpdata->socket_id);
	remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);
	return;
}

static gboolean remmina_plugin_x2go_on_plug_removed(GtkSocket *socket,
													RemminaProtocolWidget *gp) {
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("Function entry.");
	remmina_plugin_x2go_close_connection(gp);
	return TRUE;
}

static void remmina_plugin_x2go_init(RemminaProtocolWidget *gp) {
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("Function entry.", PLUGIN_NAME);
	RemminaPluginX2GoData *gpdata;

	gpdata = g_new0(RemminaPluginX2GoData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	if (!remmina_plugin_service->gtksocket_available()) {
		/* report this in open_connection, not reportable here... */
		return;
	}

	GList* available_features = remmina_plugin_x2go_populate_available_features_list();

	// available_features can't be NULL cause if it fails, it gets populated with an
	// old standard feature set.
	gpdata->available_features = available_features;

	gpdata->socket_id = 0;
	gpdata->thread = 0;

	gpdata->display = NULL;
	gpdata->window_id = 0;
	gpdata->pidx2go = 0;
	gpdata->orig_handler = NULL;

	gpdata->socket = gtk_socket_new();
	remmina_plugin_service->protocol_plugin_register_hostkey(gp, gpdata->socket);
	gtk_widget_show(gpdata->socket);

	g_signal_connect(G_OBJECT(gpdata->socket), "plug-added",
					 G_CALLBACK(remmina_plugin_x2go_on_plug_added), gp);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-removed",
					 G_CALLBACK(remmina_plugin_x2go_on_plug_removed), gp);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->socket);
}

static gboolean remmina_plugin_x2go_try_window_id(Window window_id) {
	TRACE_CALL(__func__);
	gint i;
	gboolean already_seen = FALSE;

	REMMINA_PLUGIN_DEBUG("Check if X2Go Agent window [0x%lx] is already known or if it "
						 "needs registration", window_id);

	pthread_mutex_lock(&remmina_x2go_init_mutex);
	for (i = 0; i < remmina_x2go_window_id_array->len; i++) {
		if (g_array_index(remmina_x2go_window_id_array, Window, i) == window_id) {
			already_seen = TRUE;
			REMMINA_PLUGIN_DEBUG("X2Go Agent window with ID [0x%lx] already seen.",
								 window_id);
			break;
		}
	}
	if (!already_seen) {
		g_array_append_val(remmina_x2go_window_id_array, window_id);
		REMMINA_PLUGIN_DEBUG("Registered new X2Go Agent window with ID [0x%lx].",
							 window_id);
	}
	pthread_mutex_unlock(&remmina_x2go_init_mutex);

	return (!already_seen);
}

static int remmina_plugin_x2go_dummy_handler(Display *dsp, XErrorEvent *err) {
	TRACE_CALL(__func__);
	return 0;
}

static gboolean remmina_plugin_x2go_start_create_notify(RemminaProtocolWidget *gp,
														gchar *errmsg) {
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);

	gpdata->display = XOpenDisplay(gdk_display_get_name(gdk_display_get_default()));
	if (gpdata->display == NULL) {
		g_strlcpy(errmsg, "Failed to open X11 DISPLAY.", 512);
		return FALSE;
	}

	gpdata->orig_handler = XSetErrorHandler(remmina_plugin_x2go_dummy_handler);

	XSelectInput(gpdata->display,
				 XDefaultRootWindow(gpdata->display),
				 SubstructureNotifyMask);

	REMMINA_PLUGIN_DEBUG("X11 event watcher created.");

	return TRUE;
}

static gboolean remmina_plugin_x2go_monitor_create_notify(RemminaProtocolWidget *gp, const gchar *cmd, gchar *errmsg)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata;

	gboolean agent_window_found = FALSE;
	Atom atom;
	XEvent xev;
	Window w;
	Atom type;
	int format;
	unsigned long nitems, rest;
	unsigned char *data = NULL;

	guint16 counter_x11_event_non_createnotify = 0;

	struct timespec ts;
	// wait_amount * ts.tv_nsec = 20s
	// 100 * 0.2s = 20s
	int wait_amount = 100;

	CANCEL_DEFER

	REMMINA_PLUGIN_DEBUG("%s", _("Waiting for X2Go Agent window to appear."));

	gpdata = GET_PLUGIN_DATA(gp);
	atom = XInternAtom(gpdata->display, "WM_COMMAND", True);
	if (atom == None) {
		CANCEL_ASYNC
		return FALSE;
	}

	ts.tv_sec = 0;
	// 0.2s = 200000000ns
	ts.tv_nsec = 200000000;

	while (wait_amount > 0) {
		pthread_testcancel();
		if (!(gpdata->pidx2go > 0)) {
			nanosleep(&ts, NULL);
			REMMINA_PLUGIN_DEBUG("Waiting for X2Go session to be launched.");
			continue;
		}

		while (!XPending(gpdata->display)) {
			nanosleep(&ts, NULL);
			wait_amount--;
			// Don't spam the console. Print every second though.
			if (wait_amount % 5 == 0) {
				REMMINA_PLUGIN_INFO("%s", _("Waiting for pyhoca-cli "
				                            "to show the session's window."));
			}
			continue;
		}

		XNextEvent(gpdata->display, &xev);
		// Just ignore non CreatNotify events.
		if (xev.type != CreateNotify) {
			counter_x11_event_non_createnotify++;
			if (counter_x11_event_non_createnotify % 5 == 0) {
				REMMINA_PLUGIN_DEBUG("Saw '%i' X11 events, which weren't CreateNotify.",
									 counter_x11_event_non_createnotify);
			}
			continue;
		}

		w = xev.xcreatewindow.window;
		if (XGetWindowProperty(gpdata->display, w, atom, 0, 255, False, AnyPropertyType, &type, &format, &nitems, &rest,
			&data) != Success) {
			REMMINA_PLUGIN_DEBUG("Failed to get WM_COMMAND property from X11 window ID [0x%lx].", w);
			continue;
		}

		if (data) {
			REMMINA_PLUGIN_DEBUG("Saw '%i' X11 events, which weren't CreateNotify.", counter_x11_event_non_createnotify);
			REMMINA_PLUGIN_DEBUG("Found X11 window with WM_COMMAND set to '%s', window ID is [0x%lx].", (char*)data, w);
		}
		if (data &&
			g_strrstr((gchar*)data, backend) &&
			remmina_plugin_x2go_try_window_id(w)) {

			gpdata->window_id = w;
			agent_window_found = TRUE;
			XFree(data);
			break;
		}
		if (data)
			XFree(data);
	}

	XSetErrorHandler(gpdata->orig_handler);
	XCloseDisplay(gpdata->display);
	gpdata->display = NULL;

	CANCEL_ASYNC

	if (!agent_window_found) {
		g_strlcpy(errmsg, "X2Go session window did not appear. Something went wrong...",
				  512);
		return FALSE;
	}

	return TRUE;
}

static gboolean remmina_plugin_x2go_start_session(RemminaProtocolWidget *gp) {
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("Function entry.");

	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);;
	RemminaFile *remminafile;
	const gchar errmsg[512] = {0};
	gboolean ret = TRUE;

	gchar *servstr, *host, *username, *password, *command, *kbdlayout, *kbdtype, *audio, *clipboard, *res;
	gint sshport, dpi;
	GdkDisplay *default_dsp;
	gint width, height;

	/* We save the X Display name (:0) as we will need to synchronize the clipboards */
	default_dsp = gdk_display_get_default();
	const gchar *default_dsp_name = gdk_display_get_name(default_dsp);
	REMMINA_PLUGIN_DEBUG("Default display is '%s'.", default_dsp_name);

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	servstr = GET_PLUGIN_STRING("server");
	if ( servstr ) {
		remmina_plugin_service->get_server_port(servstr, 22, &host, &sshport);
	} else {
		return FALSE;
	}
	if (!sshport)
		sshport=22;
	username = GET_PLUGIN_STRING("username");
	password = GET_PLUGIN_PASSWORD("password");
	command = GET_PLUGIN_STRING("command");
	if (!command)
		command = "TERMINAL";
	kbdlayout = GET_PLUGIN_STRING("kbdlayout");
	kbdtype = GET_PLUGIN_STRING("kbdtype");
	audio = GET_PLUGIN_STRING("audio");
	clipboard = GET_PLUGIN_STRING("clipboard");
	dpi = GET_PLUGIN_INT("dpi", 0);

	width = remmina_plugin_service->get_profile_remote_width(gp);
	height = remmina_plugin_service->get_profile_remote_height(gp);
	/* multiple of 4 */
	width = (width + 3) & ~0x3;
	height = (height + 3) & ~0x3;
	if( (width > 0) && (height  > 0))
		res = g_strdup_printf ("%dx%d", width, height);
	else
		res = "800x600";
	REMMINA_PLUGIN_DEBUG("Resolution set by user: '%s'.", res);

	REMMINA_PLUGIN_DEBUG("Attached window to socket '%d'.", gpdata->socket_id);

	/* register for notifications of window creation events */
	if (ret)
		ret = remmina_plugin_x2go_start_create_notify(gp, (gchar*)&errmsg);

	/* trigger the session start, session window should appear soon after this */
	if (ret)
		ret = remmina_plugin_x2go_exec_x2go(host, sshport, username, password, command,
											kbdlayout, kbdtype, audio, clipboard, dpi, res,
											gp, (gchar*)&errmsg);

	/* get the window id of the remote x2goagent */
	if (ret)
		ret = remmina_plugin_x2go_monitor_create_notify(gp, "x2goagent", (gchar*)&errmsg);

	if (!ret) {
		REMMINA_PLUGIN_CRITICAL("%s", errmsg);
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s", errmsg);
		return FALSE;
	}

	/* embed it */
	onMainThread_gtk_socket_add_id(GTK_SOCKET(gpdata->socket), gpdata->window_id);

	return TRUE;
}

static gboolean remmina_plugin_x2go_main(RemminaProtocolWidget *gp) {
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	gboolean ret = FALSE;

	ret = remmina_plugin_x2go_start_session(gp);

	gpdata->thread = 0;
	return ret;
}

static gpointer remmina_plugin_x2go_main_thread(gpointer data) {
	TRACE_CALL(__func__);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	CANCEL_ASYNC
	if (!remmina_plugin_x2go_main((RemminaProtocolWidget*)data)) {
		IDLE_ADD(
					(GSourceFunc) remmina_plugin_x2go_cleanup,
					(RemminaProtocolWidget*) data
				);
	}
	return NULL;
}

static gboolean remmina_plugin_x2go_open_connection(RemminaProtocolWidget *gp) {
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);

	if (!remmina_plugin_service->gtksocket_available()) {
		remmina_plugin_service->protocol_plugin_set_error(gp,
		    _("Protocol %s is unavailable because GtkSocket only works under X.org"),
		    PLUGIN_NAME);
		return FALSE;
	}

	gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));
	if (pthread_create(&gpdata->thread, NULL, remmina_plugin_x2go_main_thread, gp)) {
		remmina_plugin_service->protocol_plugin_set_error(gp,
					"Failed to initialize pthread. Falling back to non-thread mode...");
		gpdata->thread = 0;
		return FALSE;
	} else  {
		return TRUE;
	}
}

static gboolean remmina_plugin_x2go_query_feature(RemminaProtocolWidget* gp,
												  const RemminaProtocolFeature* feature) {
	TRACE_CALL(__func__);
	return TRUE;
}

static const RemminaProtocolFeature remmina_plugin_x2go_features[] = {
	{REMMINA_PROTOCOL_FEATURE_TYPE_GTKSOCKET,
		REMMINA_PLUGIN_X2GO_FEATURE_GTKSOCKET, NULL, NULL, NULL},
	{REMMINA_PROTOCOL_FEATURE_TYPE_END,
		0, NULL, NULL, NULL}
};

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */

static const RemminaProtocolSetting remmina_plugin_x2go_basic_settings[] = {
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER,
		"server", NULL, FALSE, NULL, NULL
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,
	    "username", N_("User name"), FALSE, NULL, NULL
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD,
		"password", N_("Password"), FALSE, NULL, NULL
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_COMBO,
		"command", N_("Startup program"), FALSE,
		/* Options to select (or custom user string) */ "MATE,KDE,XFCE,LXDE,TERMINAL",
		/* Tooltip */ N_("Which command should be "
						 "executed after creating the X2Go session?")
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION,
		"resolution", NULL, FALSE, NULL, NULL
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,
		"kbdlayout", N_("Keyboard Layout (auto)"), FALSE, NULL, NULL
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,
		"kbdtype", N_("Keyboard type (auto)"), FALSE, NULL, NULL
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_COMBO,
		"audio", N_("Audio support"), FALSE,
		/* Options to select (or custom user string) */ "pulse,esd,none",
		/* Tooltip */ N_("X2Go server's sound system (default: 'pulse').")
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_COMBO,
		"clipboard", N_("Clipboard direction"), FALSE,
		/* Options to select (or custom user string) */ "none,server,client,both",
		/* Tooltip */ N_("In which direction should clipboard content be copied? "
						 "(default: 'both').")
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,
		"dpi", N_("DPI resolution (int!)"), FALSE, NULL,
		/* Tooltip */ N_("Launch session with a specific resolution (in dots per inch). "
						 "Must be between 20 and 400")
	},
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin_x2go = {
	REMMINA_PLUGIN_TYPE_PROTOCOL,           // Type
	PLUGIN_NAME,                            // Name
	PLUGIN_DESCRIPTION,                     // Description
	GETTEXT_PACKAGE,                        // Translation domain
	PLUGIN_VERSION,                         // Version number
	PLUGIN_APPICON,                         // Icon for normal connection
	PLUGIN_APPICON,                         // Icon for SSH connection
	remmina_plugin_x2go_basic_settings,     // Array for basic settings
	NULL,                                   // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,    // SSH settings type
	/* REMMINA_PROTOCOL_SSH_SETTING_NONE,   // SSH settings type */
	remmina_plugin_x2go_features,           // Array for available features
	remmina_plugin_x2go_init,               // Plugin initialization method
	remmina_plugin_x2go_open_connection,    // Plugin open connection method
	remmina_plugin_x2go_close_connection,   // Plugin close connection method
	remmina_plugin_x2go_query_feature,      // Query for available features
	NULL,                                   // Call a feature
	NULL,                                   // Send a keystroke
	NULL,                                   // Screenshot
};

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService *service) {
	TRACE_CALL("remmina_plugin_entry");
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_x2go)) {
		return FALSE;
	}

	pthread_mutex_init(&remmina_x2go_init_mutex, NULL);
	remmina_x2go_window_id_array = g_array_new(FALSE, TRUE, sizeof(Window));

	REMMINA_PLUGIN_MESSAGE("%s", _("X2Go plugin loaded."));

	return TRUE;
}
