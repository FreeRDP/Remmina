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
#include <ctype.h>

#define FEATURE_AVAILABLE(gpdata, feature) \
		gpdata->available_features ? (g_list_find_custom( \
						gpdata->available_features, \
						feature, \
						(GCompareFunc) g_strcmp0 \
					     ) ? TRUE : FALSE) : FALSE

#define FEATURE_NOT_AVAIL_STR(feature) \
	g_strdup_printf(_("The command-line feature '%s' is not available! Attempting " \
			  "to start PyHoca-CLI without using this feature…"), feature)

#define GET_PLUGIN_DATA(gp) \
		(RemminaPluginX2GoData*) g_object_get_data(G_OBJECT(gp), "plugin-data")

// --------- SESSIONS ------------
#define SET_RESUME_SESSION_DATA(gp, resume_data) \
		g_object_set_data_full(G_OBJECT(gp), "resume-session-data", \
				       resume_data, \
			               g_free)

#define GET_RESUME_SESSION_DATA(gp) \
		(struct _Session*) g_object_get_data(G_OBJECT(gp), "resume-session-data")

#define GET_CURRENT_SESSIONS(gp) \
		(GList*) g_object_get_data(G_OBJECT(gp), "current-sessions-list")

#define SET_CURRENT_SESSIONS(gp, current_sessions_list) \
		g_object_set_data_full(G_OBJECT(gp), "current-sessions-list", \
					current_sessions_list, \
					g_free)

// A session is selected if the returning value is something other than 0.
#define IS_SESSION_SELECTED(gp) \
		g_object_get_data(G_OBJECT(gp), "session-selected") ? TRUE : FALSE

// We don't use the function as a real pointer but rather as a boolean value.
#define SET_SESSION_SELECTED(gp, is_session_selected) \
		g_object_set_data_full(G_OBJECT(gp), "session-selected", \
				       is_session_selected, \
				       NULL)
// -------------------

#define SET_DIALOG_DATA(gp, ddata) \
		g_object_set_data_full(G_OBJECT(gp), "dialog-data", ddata, g_free);

#define GET_DIALOG_DATA(gp) \
		(struct _DialogData*) g_object_get_data(G_OBJECT(gp), "dialog-data");

#define REMMINA_PLUGIN_INFO(fmt, ...) \
		rm_plugin_service->_remmina_info("[%s] " fmt, \
						 PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_MESSAGE(fmt, ...) \
		rm_plugin_service->_remmina_message("[%s] " fmt, \
						    PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_DEBUG(fmt, ...) \
		rm_plugin_service->_remmina_debug(__func__, "[%s] " fmt, \
						  PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_WARNING(fmt, ...) \
		rm_plugin_service->_remmina_warning(__func__, "[%s] " fmt, \
						    PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_ERROR(fmt, ...) \
		rm_plugin_service->_remmina_error(__func__, "[%s] " fmt, \
						  PLUGIN_NAME, ##__VA_ARGS__)

#define REMMINA_PLUGIN_CRITICAL(fmt, ...) \
		rm_plugin_service->_remmina_critical(__func__, "[%s] " fmt, \
						     PLUGIN_NAME, ##__VA_ARGS__)

#define GET_PLUGIN_STRING(value) \
		g_strdup(rm_plugin_service->file_get_string(remminafile, value))

#define GET_PLUGIN_PASSWORD(value) \
		GET_PLUGIN_STRING(value)

#define GET_PLUGIN_INT(value, default_value) \
		rm_plugin_service->file_get_int(remminafile, value, default_value)

#define GET_PLUGIN_BOOLEAN(value) \
		rm_plugin_service->file_get_int(remminafile, value, FALSE)

static RemminaPluginService *rm_plugin_service = NULL;

/**
 * @brief TODO: FIXME: Complete me!
 */
struct _Session {
	gchar* session_id;
	gchar* cookie;
	gchar* agent_pid;
	gchar* display;
	gchar* status;
	gchar* graphic_port;
	gchar* snd_port;
	gchar* sshfs_port;
	gchar* username;
	gchar* hostname;
	gchar* create_date;
	gchar* suspended_since;
};

// Following str2int code was adapted from Stackoverflow:
// https://stackoverflow.com/questions/7021725/how-to-convert-a-string-to-integer-in-c
typedef enum {
	STR2INT_SUCCESS,
	STR2INT_OVERFLOW,
	STR2INT_UNDERFLOW,
	STR2INT_INCONVERTIBLE,
	STR2INT_INVALID_DATA
} str2int_errno;

/**
 * @brief Convert string s to int out.
 *
 * @param out The converted int. Cannot be NULL.
 *
 * @param s Input string to be converted. \n
 * 	    The format is the same as strtol,
 *	    except that the following are inconvertible: \n
 *		* empty string \n
 *		* leading whitespace \n
 *		* or any trailing characters that are not part of the number \n
 *	    Cannot be NULL.
 * @param base Base to interpret string in. Same range as strtol (2 to 36).
 *
 * @return Indicates if the operation succeeded, or why it failed with str2int_errno enum.
 */
str2int_errno str2int(gint *out, gchar *s, gint base)
{
	gchar *end;

	if (!s || !out || base <= 0) return STR2INT_INVALID_DATA;

	if (s[0] == '\0' || isspace(s[0])) return STR2INT_INCONVERTIBLE;

	errno = 0;
	glong l = strtol(s, &end, base);

	/* Both checks are needed because INT_MAX == LONG_MAX is possible. */
	if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX)) return STR2INT_OVERFLOW;
	if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN)) return STR2INT_UNDERFLOW;
	if (*end != '\0') return STR2INT_INCONVERTIBLE;

	*out = l;
	return STR2INT_SUCCESS;
}

/**
 * DialogData:
 * @param flags see GtkDialogFlags
 * @param type see GtkMessageType
 * @param buttons see GtkButtonsType
 * @param title Title of the Dialog
 * @param message Message of the Dialog
 * @param callbackfunc A GCallback function which will be executed on the dialogs
 *		       'response' signal. Allowed to be NULL. \n
 *		       The callback function is obliged to destroy the dialog widget. \n
 * @param dialog_factory A user-defined callback function that is called when it is time
 *			 to build the actual GtkDialog. \n
 *			 Can be used to build custom dialogs. Allowed to be NULL.
 *
 *
 * The `DialogData` structure contains all info needed to open a GTK dialog with
 * rmplugin_x2go_open_dialog()
 *
 * Quick example of a callback function: \n
 * static gboolean rmplugin_x2go_test_callback(RemminaProtocolWidget *gp, gint response_id, \n
 *					       GtkDialog *self) { \n
 *	REMMINA_PLUGIN_DEBUG("response: %i", response_id); \n
 *	if (response_id == GTK_RESPONSE_OK) { \n
 *		REMMINA_PLUGIN_DEBUG("OK!"); \n
 *	} \n
 *	gtk_widget_destroy(self); \n
 *	return G_SOURCE_REMOVE; \n
 * }
 *
 */
struct _DialogData
{
	GtkWindow 	*parent;
	GtkDialogFlags	flags;
	GtkMessageType	type;
	GtkButtonsType	buttons;
	gchar		*title;
	gchar		*message;
	GCallback 	callbackfunc;

	// If the dialog needs to be custom.
	GCallback 	dialog_factory_func;
	gpointer 	dialog_factory_data;
};

/**
 * @param gp getting DialogData via dialog-data saved in gp.
 *	     See define GET_DIALOG_DATA
 * @returns: FALSE. This source should be removed from main loop.
 * 	     #G_SOURCE_CONTINUE and #G_SOURCE_REMOVE are more memorable
 *	     names for the return value.
 */
static gboolean rmplugin_x2go_open_dialog(RemminaProtocolWidget *gp)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	struct _DialogData* ddata = GET_DIALOG_DATA(gp);

	if (ddata) {
		// Can't check type, flags or buttons
		// because they are enums and '0' is a valid value
		if (!ddata->title || !ddata->message) {
			REMMINA_PLUGIN_CRITICAL("%s", _("Broken `DialogData`! Aborting…"));
			return G_SOURCE_REMOVE;
		}
	} else {
		REMMINA_PLUGIN_CRITICAL("%s", _("Can't retrieve `DialogData`! Aborting…"));
		return G_SOURCE_REMOVE;
	}

	REMMINA_PLUGIN_DEBUG("`DialogData` checks passed. Now showing dialog…");

	GtkWidget* widget_gtk_dialog = NULL;

	if (ddata->dialog_factory_func != NULL) {
		REMMINA_PLUGIN_DEBUG("Calling *custom* dialog factory function…");
		GCallback dialog_factory_func = G_CALLBACK(ddata->dialog_factory_func);
		gpointer  dialog_factory_data = ddata->dialog_factory_data;

		// Calling dialog_factory_func(gp, dialog_factory_data);
		widget_gtk_dialog = ((GtkWidget* (*)(RemminaProtocolWidget*, gpointer))
			dialog_factory_func)(gp, dialog_factory_data);
	} else {
		widget_gtk_dialog = gtk_message_dialog_new(ddata->parent,
							   ddata->flags,
							   ddata->type,
							   ddata->buttons,
							   ddata->title);

		gtk_message_dialog_format_secondary_text(
			GTK_MESSAGE_DIALOG(widget_gtk_dialog), ddata->message);
	}

	if (!widget_gtk_dialog) {
		REMMINA_PLUGIN_CRITICAL("Error! Aborting.");
		return G_SOURCE_REMOVE;
	}

	if (ddata->callbackfunc) {
		g_signal_connect_swapped(G_OBJECT(widget_gtk_dialog), "response",
					 G_CALLBACK(ddata->callbackfunc),
					 gp);
	} else {
		g_signal_connect(G_OBJECT(widget_gtk_dialog), "response",
				 G_CALLBACK(gtk_widget_destroy),
				 NULL);
	}

	gtk_widget_show_all(widget_gtk_dialog);

	// Delete ddata object and reference 'dialog-data' in gp.
	g_object_set_data(G_OBJECT(gp), "dialog-data", NULL);

	return G_SOURCE_REMOVE;
}

static GtkWidget* rmplugin_x2go_choose_session_dialog_builder(RemminaProtocolWidget* gp,
							      gpointer factory_data)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	struct _DialogData* ddata = GET_DIALOG_DATA(gp);

	if (ddata && factory_data) {
		// Can't check type, flags or buttons
		// because they are enums and '0' is a valid value
		if (!ddata->title || !ddata->message)
		{
			REMMINA_PLUGIN_CRITICAL("%s", _("Broken `DialogData`! Aborting…"));
			return FALSE;
		}
	} else {
		REMMINA_PLUGIN_CRITICAL("%s", _("Can't retrieve `DialogData` or "
					       "`factory_data`! Aborting…"));
		return FALSE;
	}

	GtkWidget *widget_gtk_dialog = NULL;
	widget_gtk_dialog = gtk_message_dialog_new(ddata->parent,
						   ddata->flags,
						   ddata->type,
						   ddata->buttons,
						   ddata->title);

	GtkWidget *combo = gtk_combo_box_new();
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(
			       GTK_DIALOG(widget_gtk_dialog))),
			   GTK_WIDGET(combo), TRUE, TRUE, 5);

	GList *sessions_list = factory_data;
	GList *elem;

	// Make sessions_list available for callback function.
	SET_CURRENT_SESSIONS(gp, sessions_list);

	for (elem = sessions_list; elem; elem = elem->next) {
		struct _Session* session = elem->data;
		g_assert(session != NULL);

		REMMINA_PLUGIN_INFO("%s", g_strdup_printf(
			_("Found session: '%s'"),
			session->session_id
		));

		// TODO: FIXME: Build proper dialog now.
	}

	return widget_gtk_dialog;
}

static gboolean rmplugin_x2go_session_chooser_callback(RemminaProtocolWidget* gp,
						       gint response_id,
						       GtkDialog *self)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (response_id == GTK_RESPONSE_OK) {
		GList *sessions_list = GET_CURRENT_SESSIONS(gp);
		GList *elem;

		if (sessions_list == NULL) {
			REMMINA_PLUGIN_INFO("%s", g_strdup_printf(
				_("Internal error: %s"),
				_("Couldn't get 'sessions_list' via GET_CURRENT_SESSIONS.")
			));

			SET_RESUME_SESSION_DATA(gp, NULL);
		}

		for (elem = sessions_list; elem; elem = elem->next) {
			struct _Session* session = elem->data;
			g_assert(session != NULL);

			REMMINA_PLUGIN_INFO("%s", g_strdup_printf(
				_("Resuming session '%s'"),
				session->session_id
			));

			SET_RESUME_SESSION_DATA(gp, session);
			break;
		}
	} else {
		REMMINA_PLUGIN_DEBUG("User clicked dialog away. Creating a new "
				     "session then.");
		SET_RESUME_SESSION_DATA(gp, NULL);
	}

	// Unstucking main process. Telling it that a session has been selected.
	// We use a trick here. As long as there is something other
	// than 0 stored, a session is selected. So we use the gpointer as a gboolean.
	SET_SESSION_SELECTED(gp, (gpointer) TRUE);

	gtk_widget_destroy(GTK_WIDGET(self));

	return G_SOURCE_REMOVE;
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

#define RMPLUGIN_X2GO_FEATURE_GTKSOCKET 1

/* Forward declaration */
static RemminaProtocolPlugin rmplugin_x2go;

/* When more than one NX sessions is connecting in progress, we need this mutex and array
 * to prevent them from stealing the same window ID.
 */
static pthread_mutex_t remmina_x2go_init_mutex;
static GArray *remmina_x2go_window_id_array;

/* ------------- Support for execution on main thread of GTK functions ------------- */
struct onMainThread_cb_data
{
	enum { FUNC_GTK_SOCKET_ADD_ID } func;

	GtkSocket* sk;
	Window w;

	/* Mutex for thread synchronization */
	pthread_mutex_t mu;
	/* Flag to catch cancellations */
	gboolean cancelled;
};

static gboolean onMainThread_cb(struct onMainThread_cb_data *d)
{
	TRACE_CALL(__func__);
	if (!d->cancelled) {
		switch (d->func) {
		case FUNC_GTK_SOCKET_ADD_ID:
			gtk_socket_add_id(d->sk, d->w);
			break;
		}
		pthread_mutex_unlock(&d->mu);
	} else {
		/* thread has been cancelled, so we must free d memory here */
		g_free(d);
	}
	return G_SOURCE_REMOVE;
}


static void onMainThread_cleanup_handler(gpointer data)
{
	TRACE_CALL(__func__);
	struct onMainThread_cb_data *d = data;
	d->cancelled = TRUE;
}

static void onMainThread_schedule_callback_and_wait(struct onMainThread_cb_data *d)
{
	TRACE_CALL(__func__);
	d->cancelled = FALSE;
	pthread_cleanup_push(onMainThread_cleanup_handler, d);
	pthread_mutex_init(&d->mu, NULL);
	pthread_mutex_lock(&d->mu);
	gdk_threads_add_idle((GSourceFunc)onMainThread_cb, (gpointer) d);

	pthread_mutex_lock(&d->mu);

	pthread_cleanup_pop(0);
	pthread_mutex_unlock(&d->mu);
	pthread_mutex_destroy(&d->mu);
}

static void onMainThread_gtk_socket_add_id(GtkSocket* sk, Window w)
{
	TRACE_CALL(__func__);

	struct onMainThread_cb_data *d;

	d = g_new0(struct onMainThread_cb_data, 1);
	d->func = FUNC_GTK_SOCKET_ADD_ID;
	d->sk = sk;
	d->w = w;

	onMainThread_schedule_callback_and_wait(d);
	g_free(d);
}
/* /-/-/-/-/-/-/ Support for execution on main thread of GTK functions /-/-/-/-/-/-/ */

static void rmplugin_x2go_remove_window_id (Window window_id)
{
	gint i;
	gboolean already_seen = FALSE;

	pthread_mutex_lock(&remmina_x2go_init_mutex);
	for (i = 0; i < remmina_x2go_window_id_array->len; i++) {
		if (g_array_index(remmina_x2go_window_id_array, Window, i) == window_id) {
			already_seen = TRUE;
			REMMINA_PLUGIN_DEBUG("Window of X2Go Agent with ID [0x%lx] seen already.",
					     window_id);
			break;
		}
	}

	if (already_seen) {
		g_array_remove_index_fast(remmina_x2go_window_id_array, i);
		REMMINA_PLUGIN_DEBUG("Forgetting about window of X2Go Agent with ID [0x%lx]…",
				     window_id);
	}

	pthread_mutex_unlock(&remmina_x2go_init_mutex);
}

/**
 * @returns: FALSE. This source should be removed from main loop.
 * 	     #G_SOURCE_CONTINUE and #G_SOURCE_REMOVE are more memorable
 *	     names for the return value.
 */
static gboolean rmplugin_x2go_cleanup(RemminaProtocolWidget *gp)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	if (gpdata == NULL) {
		REMMINA_PLUGIN_DEBUG("Exiting since gpdata is already 'NULL'…");
		return G_SOURCE_REMOVE;
	}

	if (gpdata->thread) {
		pthread_cancel(gpdata->thread);
		if (gpdata->thread) pthread_join(gpdata->thread, NULL);
	}

	if (gpdata->window_id) {
		rmplugin_x2go_remove_window_id(gpdata->window_id);
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
	rm_plugin_service->protocol_plugin_signal_connection_closed(gp);

	return G_SOURCE_REMOVE;
}

static gboolean rmplugin_x2go_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);

	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (gpdata->disconnected) {
		REMMINA_PLUGIN_DEBUG("Doing nothing since the plugin is already disconnected.");
		return G_SOURCE_REMOVE;
	}

	rmplugin_x2go_cleanup(gp);

	// Try again.
	return G_SOURCE_CONTINUE;
}

static void rmplugin_x2go_pyhoca_cli_exited(GPid pid,
					    gint status,
					    struct _RemminaProtocolWidget *gp)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	if (!gpdata) {
		REMMINA_PLUGIN_DEBUG("Doing nothing as the disconnection "
				     "has already been handled.");
		return;
	}

	if (gpdata->pidx2go <= 0) {
		REMMINA_PLUGIN_DEBUG("Doing nothing since pyhoca-cli was expected to stop.");
		return;
	}

	REMMINA_PLUGIN_CRITICAL("%s", _("PyHoca-CLI exited unexpectedly. "
					"This connection will now be closed."));

	struct _DialogData *ddata = g_new0(struct _DialogData, 1);
	SET_DIALOG_DATA(gp, ddata);
	ddata->parent = NULL;
	ddata->flags = GTK_DIALOG_MODAL;
	ddata->type = GTK_MESSAGE_ERROR;
	ddata->buttons = GTK_BUTTONS_OK;
	ddata->title = _("An error occured.");
	ddata->message = _("The necessary child process 'pyhoca-cli' stopped unexpectedly.\n"
			   "Please check your profile settings and PyHoca-CLI's output for "
			   "possible errors. Also ensure the remote server is "
			   "reachable.");
	// We don't need the response.
	ddata->callbackfunc = NULL;
	IDLE_ADD((GSourceFunc) rmplugin_x2go_open_dialog, gp);

	// 1 Second. Give `Dialog` chance to open.
	usleep(1000 * 1000);

	rmplugin_x2go_close_connection(gp);
}

/**
 * @brief This function synchronously spawns a pyhoca-cli process with argv as arguments.
 * @param argc Number of arguments.
 * @param argv Arguments as string array. \n
 *	       Last elements has to be NULL. \n
 *	       Strings will get freed automatically.
 * @param error Will be filled with an error message on fail.
 *
 * @returns Returns either standard output string or NULL if it failed.
 */
static gchar* rmplugin_x2go_spawn_pyhoca_process(guint argc, gchar* argv[], GError **error)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (!argv) {
		gchar* errmsg = g_strdup_printf(
			_("Internal error: %s"),
			_("parameter 'argv' is 'NULL'.")
		);
		REMMINA_PLUGIN_CRITICAL("%s", errmsg);
		g_set_error(error, 1, 1, errmsg);
		return NULL;
	}

	if (!error) {
		// Can't report error message back since 'error' is NULL.
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("parameter 'error' is 'NULL'.")
		));
		return NULL;
	}

	gint exit_code = 0;
	gchar *standard_out;
	// Just supresses pyhoca-cli's help message when pyhoca-cli's version is too old.
	gchar *standard_err;

	gchar **envp = g_get_environ();
	gboolean success_ret = g_spawn_sync(NULL, argv, envp, G_SPAWN_SEARCH_PATH, NULL,
					    NULL, &standard_out, &standard_err,
					    &exit_code, error);

	REMMINA_PLUGIN_INFO("%s", _("Started PyHoca-CLI with the following arguments:"));
	// Print every argument except passwords. Free all arg strings.
	for (gint i = 0; i < argc - 1; i++) {
		if (g_strcmp0(argv[i], "--password") == 0) {
			g_printf("%s ", argv[i]);
			g_printf("XXXXXX ");
			g_free(argv[i]);
			g_free(argv[++i]);
			continue;
		} else {
			g_printf("%s ", argv[i]);
			g_free(argv[i]);
		}
	}
	g_printf("\n");

	if (!success_ret || (*error) || g_strcmp0(standard_out, "") == 0 || exit_code) {
		if (!(*error)) {
			REMMINA_PLUGIN_WARNING("%s", g_strdup_printf(
				      _("An unknown error occured while trying to start "
					"PyHoca-CLI. Exit code: %i"), exit_code));
		} else {
			REMMINA_PLUGIN_WARNING("%s", g_strdup_printf(
				      _("An unknown error occured while trying to start "
					"PyHoca-CLI. Exit code: %i. Error: '%s'"),
				      exit_code,
				      (*error)->message));
		}

		return NULL;
	}

	return standard_out;
}

/**
 * @brief Saves s_password and s_username if set.
 * @returns either TRUE or FALSE. If FALSE gets returned, `errmsg` is set.
 */
static gboolean rmplugin_x2go_save_credentials(RemminaFile* remminafile,
					       gchar* s_username, gchar* s_password,
					       gchar* errmsg)
{
	// User has requested to save credentials. We put all the new credentials
	// into remminafile->settings. They will be saved later, on successful
	// connection, by rcw.c
	if (s_password && s_username) {
		if (g_strcmp0(s_username, "") == 0) {
			g_strlcpy(errmsg, _("Can't save empty username!"), 512);
			//REMMINA_PLUGIN_CRITICAL("%s", errmsg); // No need.
			return FALSE;
		}

		// We allow the possibility to set an empty password because a X2Go
		// session can be still made using keyfiles or similar.
		rm_plugin_service->file_set_string(remminafile, "password",
						   s_password);
		rm_plugin_service->file_set_string(remminafile, "username",
						   s_username);
	} else {
		g_strlcpy(errmsg, g_strdup_printf(
			_("Internal error: %s"),
			_("Could not save new credentials.")
		), 512);
		
		REMMINA_PLUGIN_CRITICAL("%s", _("An error occured while trying to save "
						"new credentials: 's_password' or "
						"'s_username' strings were not set."));
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief Asks the user for a username and password.
 *
 * @param errmsg Error message if function failed.
 * @param username Default username. Gets set to new username on success.
 * @param password Default password. Gets set to new password on success.
 *
 * @returns FALSE if auth failed and TRUE on success.
 */
static gboolean rmplugin_x2go_get_auth(RemminaProtocolWidget *gp, gchar* errmsg,
				       gchar* username, gchar* password)
{
	gchar *s_username, *s_password;
	gint ret;
	gboolean save;
	gboolean disable_password_storing;
	RemminaFile *remminafile;

	remminafile = rm_plugin_service->protocol_plugin_get_file(gp);

	disable_password_storing = rm_plugin_service->file_get_int(remminafile,
								   "disablepasswordstoring",
								   FALSE);
	ret = rm_plugin_service->protocol_plugin_init_auth(
			gp, (disable_password_storing ? 0 :
			     REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD |
			     REMMINA_MESSAGE_PANEL_FLAG_USERNAME),
			_("Enter X2Go credentials"),
			username, // function arg 'username' is default username
			password, // function arg 'password' is default password
			NULL,
			NULL);


	if (ret == GTK_RESPONSE_OK) {
		s_username = rm_plugin_service->protocol_plugin_init_get_username(gp);
		s_password = rm_plugin_service->protocol_plugin_init_get_password(gp);
		if (rm_plugin_service->protocol_plugin_init_get_savepassword(gp))
			rm_plugin_service->file_set_string(remminafile, "password",
							   s_password);

		// Should be renamed to protocol_plugin_init_get_savecredentials()?!
		save = rm_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save) {
			if (!rmplugin_x2go_save_credentials(remminafile, s_username,
							    s_password, errmsg)) {
				return FALSE;
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
		g_strlcpy(errmsg, "Authentication cancelled. Aborting…", 512);
		REMMINA_PLUGIN_DEBUG("%s", errmsg);
		return FALSE;
	}

	return TRUE;
}

/**
 * @brief Stores all necessary information needed for retrieving sessions from
 *	  a X2Go server.
 */
struct _ConnectionData{
	gchar* host;
	gchar* username;
	gchar* password;
};

/**
 * @brief Executes 'pyhoca-cli --list-sessions' for username@host.
 *
 * @param gp RemminaProtocolWidget* is used to get the x2go-plugin data.
 * @param error	This is where a error message will be when NULL gets returned.
 * @param connect_data struct _ConnectionData* which stores all necessary information
 *		       needed for retrieving sessions from a X2Go server.
 *
 * @returns Standard output of pyhoca-cli command.
 *	    If NULL then errmsg is set to user-friendly error message.
 */
static gchar* rmplugin_x2go_get_pyhoca_sessions(RemminaProtocolWidget* gp, GError **error,
						struct _ConnectionData* connect_data)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");
	RemminaPluginX2GoData* gpdata = GET_PLUGIN_DATA(gp);

	gchar *host = NULL;
	gchar *username = NULL;
	gchar *password = NULL;

	if (!connect_data ||
	    !connect_data->host ||
	    !connect_data->username ||
	    !connect_data->password ||
	    strlen(connect_data->host) <= 0 ||
	    strlen(connect_data->username) <= 0)
	    // Allow empty passwords. Maybe the user wants to connect via public key?
	{
		g_set_error(error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("'Invalid connection data.'")
		));
		return NULL;
	} else {
		host = connect_data->host;
		username = connect_data->username;
		password = connect_data->password;
	}

	// We will now start pyhoca-cli with only the '--list-sessions' option.

	gchar *argv[50];
	gint argc = 0;

	argv[argc++] = g_strdup("pyhoca-cli");
	argv[argc++] = g_strdup("--list-sessions");

	argv[argc++] = g_strdup("--server"); // Not listed as feature.
	argv[argc++] = g_strdup_printf("%s", host);

	if (FEATURE_AVAILABLE(gpdata, "USERNAME")) {
		argv[argc++] = g_strdup("-u");
		if (username) {
			argv[argc++] = g_strdup_printf("%s", username);
		} else {
			argv[argc++] = g_strdup_printf("%s", g_get_user_name());
		}
	} else {
		g_set_error(error, 1, 1, FEATURE_NOT_AVAIL_STR("USERNAME"));
		REMMINA_PLUGIN_WARNING("%s", FEATURE_NOT_AVAIL_STR("USERNAME"));
		return NULL;
	}

	if (password && FEATURE_AVAILABLE(gpdata, "PASSWORD")) {
		if (FEATURE_AVAILABLE(gpdata, "AUTH_ATTEMPTS")) {
			argv[argc++] = g_strdup("--auth-attempts");
			argv[argc++] = g_strdup_printf ("%i", 0);
		} else {
			REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("AUTH_ATTEMPTS"));
		}
		argv[argc++] = g_strdup("--force-password");
		argv[argc++] = g_strdup("--password");
		argv[argc++] = g_strdup_printf("%s", password);
	} else if (!password) {
		g_set_error(error, 1, 1, FEATURE_NOT_AVAIL_STR("PASSWORD"));
		REMMINA_PLUGIN_WARNING("%s", FEATURE_NOT_AVAIL_STR("PASSWORD"));
		return NULL;
	}

	// No need to catch feature-not-available error.
	// `--quiet` is not that important.
	if (FEATURE_AVAILABLE(gpdata, "QUIET")) {
		argv[argc++] = g_strdup("--quiet");
	}

	argv[argc++] = NULL;

	gchar* std_out = rmplugin_x2go_spawn_pyhoca_process(argc, argv, error);

	if (!std_out || *error) {
		// If no error is set but std_out is NULL
		// then something is not right at all.
		// Most likely the developer forgot to add an error message. Crash.
		g_assert((*error) != NULL);
		return NULL;
	}

	return std_out;
}

/**
 * @brief This function is used to parse the output of
 * 	  rmplugin_x2go_get_pyhoca_sessions().
 *
 * @param gp RemminaProtocolWidget* is used to get the x2go-plugin data.
 * @param error This is where a error message will be when NULL gets returned.
 * @param connect_data	struct _ConnectionData* which stores all necessary information
 *			needed for retrieving sessions from a X2Go server.
 *
 * @returns Returns either a GList containing the IDs of every already existing session
 *	    found or if the function failes, NULL.
 *
 * TODO: If pyhoca-cli (python-x2go) implements `--json` or similar option -> Replace
 *	 entire function with JSON parsing.
 */
static GList* rmplugin_x2go_parse_pyhoca_sessions(RemminaProtocolWidget* gp,
						  GError **error,
						  struct _ConnectionData* connect_data)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	gchar *pyhoca_output = NULL;

	pyhoca_output = rmplugin_x2go_get_pyhoca_sessions(gp, error, connect_data);
	if (!pyhoca_output || *error) {
		// If no error is set but pyhoca_output is NULL
		// then something is not right at all.
		// Most likely the developer forgot to add an error message. Crash.
		g_assert((*error) != NULL);

		return NULL;
	}

	gchar **lines_list = g_strsplit(pyhoca_output, "\n", -1);
	// Assume at least two lines of output.
	if (lines_list == NULL || lines_list[0] == NULL || lines_list[1] == NULL) {
		g_set_error(error, 1, 1, _("Couldn't parse the output of PyHoca-CLI's "
				           "--list-sessions option. Creating a new "
				           "session now."));
		return NULL;
	}

	gboolean found_session = FALSE;
	GList* sessions = NULL;
	struct _Session *session = NULL;

	for (guint i = 0; lines_list[i] != NULL; i++) {
		gchar* current_line = lines_list[i];

		// TOO VERBOSE:
		//REMMINA_PLUGIN_DEBUG("pyhoca-cli: %s", current_line);

		// Hardcoded string "Session Name: " comes from python-x2go.
		if (!g_str_has_prefix(current_line, "Session Name: ") && !found_session) {
			// Doesn't begin with "Session Name: " and
			// the current line doesn't come after that either. Skipping.
			continue;
		}

		if (g_str_has_prefix(current_line, "Session Name: ")) {
			gchar* session_id = NULL;
			gchar** line_list = g_strsplit(current_line, ": ", 0);

			if (line_list == NULL ||
			    line_list[0] == NULL ||
			    line_list[1] == NULL ||
			    strlen(line_list[0]) <= 0 ||
			    strlen(line_list[1]) <= 0)
			{
				found_session = FALSE;
				continue;
			}


			session = g_new0(struct _Session, 1);
			sessions = g_list_append(sessions, session);

			session_id = line_list[1];
			session->session_id = session_id;

			REMMINA_PLUGIN_INFO("%s", g_strdup_printf(
				_("Found already existing X2Go session with ID: '%s'"),
				session_id)
			);

			found_session = TRUE;
			continue;
		}

		if (!found_session) {
			continue;
		}

		if (g_strcmp0(current_line, "-------------") == 0) {
			continue;
		}

		gchar* value = NULL;
		gchar** line_list = g_strsplit(current_line, ": ", 0);

		if (line_list == NULL ||
		    line_list[0] == NULL ||
		    line_list[1] == NULL ||
		    strlen(line_list[0]) <= 0 ||
		    strlen(line_list[1]) <= 0)
		{
			// Probably the empty line at the end of every session.
			found_session = FALSE;
			continue;
		}
		value = line_list[1];

		if (g_str_has_prefix(current_line, "cookie: ")) {
			REMMINA_PLUGIN_DEBUG("cookie:\t'%s'", value);
			session->cookie = value;
		} else if (g_str_has_prefix(current_line, "agent PID: ")) {
			REMMINA_PLUGIN_DEBUG("agent PID:\t'%s'", value);
			session->agent_pid = value;
		} else if (g_str_has_prefix(current_line, "display: ")) {
			REMMINA_PLUGIN_DEBUG("display:\t'%s'", value);
			session->display = value;
		} else if (g_str_has_prefix(current_line, "status: ")) {
			REMMINA_PLUGIN_DEBUG("status:\t'%s'", value);
			session->status = value;
		} else if (g_str_has_prefix(current_line, "graphic port: ")) {
			REMMINA_PLUGIN_DEBUG("graphic port:\t'%s'", value);
			session->graphic_port = value;
		} else if (g_str_has_prefix(current_line, "snd port: ")) {
			REMMINA_PLUGIN_DEBUG("snd port:\t'%s'", value);
			session->snd_port = value;
		} else if (g_str_has_prefix(current_line, "sshfs port: ")) {
			REMMINA_PLUGIN_DEBUG("sshfs port:\t'%s'", value);
			session->sshfs_port = value;
		} else if (g_str_has_prefix(current_line, "username: ")) {
			REMMINA_PLUGIN_DEBUG("username:\t'%s'", value);
			session->username = value;
		} else if (g_str_has_prefix(current_line, "hostname: ")) {
			REMMINA_PLUGIN_DEBUG("hostname:\t'%s'", value);
			session->hostname = value;
		} else if (g_str_has_prefix(current_line, "create date: ")) {
			REMMINA_PLUGIN_DEBUG("create date:\t'%s'", value);
			session->create_date = value;
		} else if (g_str_has_prefix(current_line, "suspended since: ")) {
			REMMINA_PLUGIN_DEBUG("suspended since:\t'%s'", value);
			session->suspended_since = value;
		} else {
			REMMINA_PLUGIN_DEBUG("Not supported:\t'%s'", value);
			found_session = FALSE;
		}
	}

	if (!sessions) {
		g_set_error(error, 1, 1,
			_("Could not find any sessions on remote machine. Creating a new "
			  "session now.")
		);

		// returning NULL with `error` set.
	}

	return sessions;
}

/**
 * @brief TODO: Complete me!
 */
static struct _Session* rmplugin_x2go_ask_session(RemminaProtocolWidget *gp, GError **error,
						  struct _ConnectionData* connect_data)
{
	GList *sessions_list = NULL;
	sessions_list = rmplugin_x2go_parse_pyhoca_sessions(gp, error, connect_data);

	if (!sessions_list || *error) {
		// If no error is set but sessions_list is NULL
		// then something is not right at all.
		// Most likely the developer forgot to add an error message. Crash.
		g_assert(*error != NULL);
		return NULL;
	}

	// Prep new DialogData struct.
	struct _DialogData *ddata = g_new0(struct _DialogData, 1);
	SET_DIALOG_DATA(gp, ddata);
	ddata->parent = NULL;
	ddata->flags = GTK_DIALOG_MODAL;
	ddata->type = GTK_MESSAGE_QUESTION;
	ddata->buttons = GTK_BUTTONS_OK;
	ddata->title = _("Choose a session to resume:");
	ddata->message = "Normal dailog factory function.";
	ddata->callbackfunc = G_CALLBACK(rmplugin_x2go_session_chooser_callback);
	ddata->dialog_factory_func = G_CALLBACK(rmplugin_x2go_choose_session_dialog_builder);
	ddata->dialog_factory_data = sessions_list;

	// Open dialog here. Dialog rmplugin_x2go_session_chooser_callback (callbackfunc)
	// should set SET_RESUME_SESSION_DATA.
	IDLE_ADD((GSourceFunc)rmplugin_x2go_open_dialog, gp);

	guint counter = 0;
	while (!IS_SESSION_SELECTED(gp)) {
		// 0.5 Seconds. Give dialog chance to open.
		usleep(500 * 1000);

		// Every 5 seconds
		if (counter % 10 == 0 || counter == 0) {
			REMMINA_PLUGIN_INFO("%s", _("Waiting for user to respond to dialog…"));
		}
		counter++;
	}

	struct _Session* chosen_resume_session = GET_RESUME_SESSION_DATA(gp);

	if (!chosen_resume_session) {
		g_set_error(error, 1, 1, _("No session was selected. Creating a new one."));
		return NULL;
	}

	// Safety first. If not set, crash.
	g_assert(chosen_resume_session->session_id != NULL);
	g_assert(strlen(chosen_resume_session->session_id) > 0);

	return chosen_resume_session;
}

static gboolean rmplugin_x2go_exec_x2go(gchar *host,
                                        gint   sshport,
                                        gchar *username,
                                        gchar *password,
                                        gchar *command,
                                        gchar *kbdlayout,
                                        gchar *kbdtype,
                                        gchar *audio,
                                        gchar *clipboard,
                                        gint   dpi,
                                        gchar *resolution,
                                        RemminaProtocolWidget *gp,
                                        gchar *errmsg)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);

	gchar *argv[50];
	gint argc = 0;

	// Sets `username` and `password`.
	if (!rmplugin_x2go_get_auth(gp, errmsg, username, password)) {
		return FALSE;
	}

	struct _ConnectionData* connect_data = g_new0(struct _ConnectionData, 1);
	connect_data->host = host;
	connect_data->username = username;
	connect_data->password = password;

	GError *session_error = NULL;
	struct _Session* resume_session = rmplugin_x2go_ask_session(gp, &session_error,
								    connect_data);

	if (!resume_session || session_error) {
		// If no error is set but session_id is NULL
		// then something is not right at all.
		// Most likely the developer forgot to add an error message. Crash.
		g_assert(session_error != NULL);

		REMMINA_PLUGIN_WARNING("%s", g_strdup_printf(
			_("A non-critical error happened: %s"),
			session_error->message
		));
	} else {
		REMMINA_PLUGIN_INFO("%s", g_strdup_printf(
			_("User chose to resume session with ID: '%s'"),
			resume_session->session_id
		));
	}

	argc = 0;
	argv[argc++] = g_strdup("pyhoca-cli");

	argv[argc++] = g_strdup("--server"); // Not listed as feature.
	argv[argc++] = g_strdup_printf ("%s", host);

	if (FEATURE_AVAILABLE(gpdata, "REMOTE_SSH_PORT")) {
		argv[argc++] = g_strdup("-p");
		argv[argc++] = g_strdup_printf ("%d", sshport);
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("REMOTE_SSH_PORT"));
	}

	if (resume_session) {
		REMMINA_PLUGIN_INFO(
		    "%s", g_strdup_printf(_("Resuming session '%s'…"),
		    resume_session->session_id)
		);

		if (FEATURE_AVAILABLE(gpdata, "RESUME")) {
			argv[argc++] = g_strdup("--resume");
			argv[argc++] = g_strdup_printf("%s", resume_session->session_id);
		} else {
			REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("RESUME"));
		}
	}

	if (FEATURE_AVAILABLE(gpdata, "USERNAME")) {
		argv[argc++] = g_strdup("-u");
		if (username){
			argv[argc++] = g_strdup_printf ("%s", username);
		} else {
			argv[argc++] = g_strdup_printf ("%s", g_get_user_name());
		}
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("USERNAME"));
	}

	if (password && FEATURE_AVAILABLE(gpdata, "PASSWORD")) {
		argv[argc++] = g_strdup("--force-password");
		argv[argc++] = g_strdup("--password");
		argv[argc++] = g_strdup_printf ("%s", password);
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("PASSWORD"));
	}

	if (FEATURE_AVAILABLE(gpdata, "AUTH_ATTEMPTS")) {
		argv[argc++] = g_strdup("--auth-attempts");
		argv[argc++] = g_strdup_printf ("%i", 0);
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("AUTH_ATTEMPTS"));
	}

	if (FEATURE_AVAILABLE(gpdata, "COMMAND")) {
		argv[argc++] = g_strdup("-c");
		// FIXME: pyhoca-cli is picky about multiple quotes around
		// 	  the command string...
		// argv[argc++] = g_strdup_printf ("%s", g_shell_quote(command));
		argv[argc++] = g_strdup(command);
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("COMMAND"));
	}

	if (FEATURE_AVAILABLE(gpdata, "KBD_LAYOUT")) {
		if (kbdlayout) {
			argv[argc++] = g_strdup("--kbd-layout");
			argv[argc++] = g_strdup_printf ("%s", kbdlayout);
		} else {
			argv[argc++] = g_strdup("--kbd-layout");
			argv[argc++] = g_strdup("auto");
		}
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("KBD_LAYOUT"));
	}

	if (FEATURE_AVAILABLE(gpdata, "KBD_TYPE")) {
		if (kbdtype) {
			argv[argc++] = g_strdup("--kbd-type");
			argv[argc++] = g_strdup_printf ("%s", kbdtype);
		} else {
			argv[argc++] = g_strdup("--kbd-type");
			argv[argc++] = g_strdup("auto");
		}
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("KBD_TYPE"));
	}

	if (FEATURE_AVAILABLE(gpdata, "GEOMETRY")) {
		if (!resolution)
			resolution = "800x600";
		argv[argc++] = g_strdup("-g");
		argv[argc++] = g_strdup_printf ("%s", resolution);
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("GEOMETRY"));
	}

	if (!resume_session) {
		if (FEATURE_AVAILABLE(gpdata, "TRY_RESUME")) {
			argv[argc++] = g_strdup("--try-resume");
		} else {
			REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("TRY_RESUME"));
		}
	}

	if (FEATURE_AVAILABLE(gpdata, "TERMINATE_ON_CTRL_C")) {
		argv[argc++] = g_strdup("--terminate-on-ctrl-c");
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("TERMINATE_ON_CTRL_C"));
	}

	if (FEATURE_AVAILABLE(gpdata, "SOUND")) {
		if (audio) {
			argv[argc++] = g_strdup("--sound");
			argv[argc++] = g_strdup_printf ("%s", audio);
		} else {
			argv[argc++] = g_strdup("--sound");
			argv[argc++] = g_strdup("none");
		}
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("SOUND"));
	}

	if (FEATURE_AVAILABLE(gpdata, "CLIPBOARD_MODE")) {
		if (clipboard) {
			argv[argc++] = g_strdup("--clipboard-mode");
			argv[argc++] = g_strdup_printf("%s", clipboard);
		}
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("CLIPBOARD_MODE"));
	}

	if (FEATURE_AVAILABLE(gpdata, "DPI")) {
		// Event though we validate the users input in Remmina Editor,
		// manipulating profile files is still very possible..
		// Values are extracted from pyhoca-cli.
		if (dpi < 20 || dpi > 400) {
			g_strlcpy(errmsg, _("DPI setting is out of bounds. Please adjust "
					    "it in profile settings."), 512);
			// No need, start_session() will handle output.
			//REMMINA_PLUGIN_CRITICAL("%s", errmsg);
			return FALSE;
		}
		argv[argc++] = g_strdup("--dpi");
		argv[argc++] = g_strdup_printf ("%i", dpi);
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("DPI"));
	}

	argv[argc++] = NULL;

	GError *error = NULL;
	gchar **envp = g_get_environ();
	gboolean success = g_spawn_async_with_pipes (NULL, argv, envp,
						     (G_SPAWN_DO_NOT_REAP_CHILD |
						      G_SPAWN_SEARCH_PATH), NULL,
						     NULL, &gpdata->pidx2go,
						     NULL, NULL, NULL, &error);

	REMMINA_PLUGIN_INFO("%s", _("Started pyhoca-cli with following arguments:"));
	// Print every argument except passwords. Free all arg strings.
	for (gint i = 0; i < argc - 1; i++) {
		if (g_strcmp0(argv[i], "--password") == 0) {
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
		// TRANSLATORS: Meta-error. Shouldn't be visible.
		if (!error) error = g_error_new(0, 0, _("Internal error."));

		gchar *error_title = _("An error occured while "
				       "starting an X2Go session…");

		struct _DialogData* ddata = g_new0(struct _DialogData, 1);
		SET_DIALOG_DATA(gp, ddata);
		ddata->parent = NULL;
		ddata->flags = GTK_DIALOG_MODAL;
		ddata->type = GTK_MESSAGE_ERROR;
		ddata->buttons = GTK_BUTTONS_OK;
		ddata->title = _("Could not start X2Go session.");
		ddata->message = g_strdup_printf(_("Could not start PyHoca-CLI (%i): '%s'"),
						 error->code,
						 error->message);
		// We don't need the response.
		ddata->callbackfunc = NULL;
		IDLE_ADD((GSourceFunc) rmplugin_x2go_open_dialog, gp);

		g_strlcpy(errmsg, error_title, 512);

		// No need to output here. rmplugin_x2go_start_session will do this.

		g_error_free(error);

		return FALSE;
	}

	// Prevent a race condition where pyhoca-cli is not
	// started yet (pidx2go == 0) but a watcher is added.
	while (gpdata->pidx2go == 0) {
		REMMINA_PLUGIN_DEBUG("Waiting for PyHoca-CLI to start…");
	};

	REMMINA_PLUGIN_DEBUG("Watching child 'pyhoca-cli' process now…");
	g_child_watch_add(gpdata->pidx2go,
			  (GChildWatchFunc) rmplugin_x2go_pyhoca_cli_exited,
			  gp);

	return TRUE;
}

/**
 * @returns a GList* with all features which pyhoca-cli had before the feature system.
 */
static GList* rmplugin_x2go_old_pyhoca_features()
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	#define AMOUNT_FEATURES 43
	gchar* features[AMOUNT_FEATURES] = {
		"ADD_TO_KNOWN_HOSTS", "AUTH_ATTEMPTS", "BROKER_PASSWORD", "BROKER_URL",
		"CLEAN_SESSIONS", "COMMAND", "DEBUG", "FORCE_PASSWORD",	"FORWARD_SSHAGENT",
		"GEOMETRY", "KBD_LAYOUT", "KBD_TYPE", "LIBDEBUG", "LIBDEBUG_SFTPXFER", "LINK",
		"LIST_CLIENT_FEATURES", "LIST_DESKTOPS", "LIST_SESSIONS", "NEW", "PACK",
		"PASSWORD", "PDFVIEW_CMD", "PRINTER", "PRINTING", "PRINT_ACTION", "PRINT_CMD",
		"QUIET", "REMOTE_SSH_PORT", "RESUME", "SAVE_TO_FOLDER", "SESSION_PROFILE",
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

/**
 *  @returns a GList* which includes all pyhoca-cli command line features we can use.
 */
static GList* rmplugin_x2go_populate_available_features_list()
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	GList* returning_glist = NULL;

	// We will now start pyhoca-cli with only the '--list-cmdline-features' option
	// and depending on the exit code and standard output we will determine if some
	// features are available or not.

	gchar* argv[50];
	gint argc = 0;

	argv[argc++] = g_strdup("pyhoca-cli");
	argv[argc++] = g_strdup("--list-cmdline-features");
	argv[argc++] = NULL;

	GError* error = NULL; // Won't be actually used.

	// Querying pyhoca-cli's command line features.
	gchar* features_string = rmplugin_x2go_spawn_pyhoca_process(argc, argv, &error);

	if (!features_string || error) {
		// We added the '--list-cmdline-features' on commit 17d1be1319ba6 of
		// pyhoca-cli. In order to protect setups which don't have the newest
		// version of pyhoca-cli available yet we artificially create a list
		// of an old limited set of features.

		REMMINA_PLUGIN_WARNING("%s",
			_("Couldn't get PyHoca-CLI's command-line features. This "
			  "indicates it is either too old, or not installed. "
			  "An old limited set of features will be used for now."));

		return rmplugin_x2go_old_pyhoca_features();
	} else {
		gchar **features_list = g_strsplit(features_string, "\n", 0);

		if (features_list == NULL) {
			gchar *error_msg = _("Could not parse PyHoca-CLI's command-line "
					     "features. Using a limited feature-set for now.");
			REMMINA_PLUGIN_WARNING("%s", error_msg);
			return rmplugin_x2go_old_pyhoca_features();
		}

		REMMINA_PLUGIN_INFO("%s", _("Retrieved the following PyHoca-CLI "
					    "command-line features:"));

		for(int k = 0; features_list[k] != NULL; k++) {
			// Filter out empty strings
			if (strlen(features_list[k]) <= 0) continue;

			REMMINA_PLUGIN_INFO("%s",
					 g_strdup_printf(_("Available feature[%i]: '%s'"),
							 k+1, features_list[k]));
			returning_glist = g_list_append(returning_glist, features_list[k]);
		}
		return returning_glist;
	}
}

static void rmplugin_x2go_on_plug_added(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	REMMINA_PLUGIN_DEBUG("Socket %d", gpdata->socket_id);
	rm_plugin_service->protocol_plugin_signal_connection_opened(gp);
	return;
}

static gboolean rmplugin_x2go_on_plug_removed(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("Function entry.");
	rmplugin_x2go_close_connection(gp);
	return G_SOURCE_CONTINUE;
}

static void rmplugin_x2go_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("Function entry.", PLUGIN_NAME);
	RemminaPluginX2GoData *gpdata;

	gpdata = g_new0(RemminaPluginX2GoData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	if (!rm_plugin_service->gtksocket_available()) {
		/* report this in open_connection, not reportable here... */
		return;
	}

	GList* available_features = rmplugin_x2go_populate_available_features_list();

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
	rm_plugin_service->protocol_plugin_register_hostkey(gp, gpdata->socket);
	gtk_widget_show(gpdata->socket);

	g_signal_connect(G_OBJECT(gpdata->socket), "plug-added",
			 G_CALLBACK(rmplugin_x2go_on_plug_added), gp);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-removed",
			 G_CALLBACK(rmplugin_x2go_on_plug_removed), gp);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->socket);
}

static gboolean rmplugin_x2go_try_window_id(Window window_id)
{
	TRACE_CALL(__func__);
	gint i;
	gboolean already_seen = FALSE;

	REMMINA_PLUGIN_DEBUG("Check if the window of X2Go Agent with ID [0x%lx] is already known or if "
			     "it needs registration", window_id);

	pthread_mutex_lock(&remmina_x2go_init_mutex);
	for (i = 0; i < remmina_x2go_window_id_array->len; i++) {
		if (g_array_index(remmina_x2go_window_id_array, Window, i) == window_id) {
			already_seen = TRUE;
			REMMINA_PLUGIN_DEBUG("Window of X2Go Agent with ID [0x%lx] "
					     "already seen.", window_id);
			break;
		}
	}
	if (!already_seen) {
		g_array_append_val(remmina_x2go_window_id_array, window_id);
		REMMINA_PLUGIN_DEBUG("Registered new window for X2Go Agent with "
				     "ID [0x%lx].", window_id);
	}
	pthread_mutex_unlock(&remmina_x2go_init_mutex);

	return (!already_seen);
}

static int rmplugin_x2go_dummy_handler(Display *dsp, XErrorEvent *err)
{
	TRACE_CALL(__func__);
	return 0;
}

static gboolean rmplugin_x2go_start_create_notify(RemminaProtocolWidget *gp, gchar *errmsg)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);

	gpdata->display = XOpenDisplay(gdk_display_get_name(gdk_display_get_default()));
	if (gpdata->display == NULL) {
		g_strlcpy(errmsg, _("Could not open X11 DISPLAY."), 512);
		return FALSE;
	}

	gpdata->orig_handler = XSetErrorHandler(rmplugin_x2go_dummy_handler);

	XSelectInput(gpdata->display,
		     XDefaultRootWindow(gpdata->display),
		     SubstructureNotifyMask);

	REMMINA_PLUGIN_DEBUG("X11 event-watcher created.");

	return TRUE;
}

static gboolean rmplugin_x2go_monitor_create_notify(RemminaProtocolWidget *gp,
						    const gchar *cmd,
						    gchar *errmsg)
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

	guint16 non_createnotify_count = 0;

	struct timespec ts;
	// wait_amount * ts.tv_nsec = 20s
	// 100 * 0.2s = 20s
	int wait_amount = 100;

	CANCEL_DEFER

	REMMINA_PLUGIN_DEBUG("%s", _("Waiting for window of X2Go Agent to appear…"));

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
			REMMINA_PLUGIN_DEBUG("Waiting for X2Go session to start…");
			continue;
		}

		while (!XPending(gpdata->display)) {
			nanosleep(&ts, NULL);
			wait_amount--;
			// Don't spam the console. Print every second though.
			if (wait_amount % 5 == 0) {
				REMMINA_PLUGIN_INFO("%s", _("Waiting for PyHoca-CLI to "
							    "show the session's window…"));
			}
			continue;
		}

		XNextEvent(gpdata->display, &xev);
		// Just ignore non CreatNotify events.
		if (xev.type != CreateNotify) {
			non_createnotify_count++;
			if (non_createnotify_count % 5 == 0) {
				REMMINA_PLUGIN_DEBUG("Saw '%i' X11 events, which weren't "
						     "CreateNotify.", non_createnotify_count);
			}
			continue;
		}

		w = xev.xcreatewindow.window;
		if (XGetWindowProperty(gpdata->display, w, atom, 0, 255, False,
				       AnyPropertyType, &type, &format, &nitems, &rest,
				       &data) != Success) {
			REMMINA_PLUGIN_DEBUG("Could not get WM_COMMAND property from X11 "
					     "window ID [0x%lx].", w);
			continue;
		}

		if (data) {
			REMMINA_PLUGIN_DEBUG("Saw '%i' X11 events, which weren't "
					     "CreateNotify.", non_createnotify_count);
			REMMINA_PLUGIN_DEBUG("Found X11 window with WM_COMMAND set "
					     "to '%s', the window ID is [0x%lx].",
					     (char*)data, w);
		}
		if (data && g_strrstr((gchar*)data, cmd) &&
		    rmplugin_x2go_try_window_id(w)) {
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
		g_strlcpy(errmsg, _("No X2Go session window appeared. "
				    "Something went wrong…"), 512);
		return FALSE;
	}

	return TRUE;
}

static gboolean rmplugin_x2go_start_session(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("Function entry.");

	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);;
	RemminaFile *remminafile;
	const gchar errmsg[512] = {0};
	gboolean ret = TRUE;

	gchar *servstr, *host, *username, *password, *command, *kbdlayout, *kbdtype,
	      *audio, *clipboard, *res;
	gint sshport, dpi;
	GdkDisplay *default_dsp;
	gint width, height;

	// We save the X Display name (:0) as we will need to synchronize the clipboards
	default_dsp = gdk_display_get_default();
	const gchar *default_dsp_name = gdk_display_get_name(default_dsp);
	REMMINA_PLUGIN_DEBUG("Default display is '%s'.", default_dsp_name);

	remminafile = rm_plugin_service->protocol_plugin_get_file(gp);

	servstr = GET_PLUGIN_STRING("server");
	if (servstr) {
		rm_plugin_service->get_server_port(servstr, 22, &host, &sshport);
	} else {
		return FALSE;
	}

	if (!sshport) sshport=22;

	username = GET_PLUGIN_STRING("username");
	password = GET_PLUGIN_PASSWORD("password");

	command = GET_PLUGIN_STRING("command");
	if (!command) command = "TERMINAL";

	kbdlayout = GET_PLUGIN_STRING("kbdlayout");
	kbdtype = GET_PLUGIN_STRING("kbdtype");

	audio = GET_PLUGIN_STRING("audio");

	clipboard = GET_PLUGIN_STRING("clipboard");

	dpi = GET_PLUGIN_INT("dpi", 0);

	width = rm_plugin_service->get_profile_remote_width(gp);
	height = rm_plugin_service->get_profile_remote_height(gp);
	/* multiple of 4 */
	width = (width + 3) & ~0x3;
	height = (height + 3) & ~0x3;
	if ((width > 0) && (height  > 0)) {
		res = g_strdup_printf ("%dx%d", width, height);
	} else {
		res = "800x600";
	}
	REMMINA_PLUGIN_DEBUG("Resolution set by user: '%s'.", res);

	REMMINA_PLUGIN_DEBUG("Attached window to socket '%d'.", gpdata->socket_id);

	/* register for notifications of window creation events */
	if (ret) ret = rmplugin_x2go_start_create_notify(gp, (gchar*)&errmsg);

	/* trigger the session start, session window should appear soon after this */
	if (ret) ret = rmplugin_x2go_exec_x2go(host, sshport, username, password, command,
					       kbdlayout, kbdtype, audio, clipboard, dpi,
					       res, gp, (gchar*)&errmsg);

	/* get the window ID of the remote x2goagent */
	if (ret) ret = rmplugin_x2go_monitor_create_notify(gp, "x2goagent",
						           (gchar*)&errmsg);

	if (!ret) {
		REMMINA_PLUGIN_CRITICAL("%s", _(errmsg));
		rm_plugin_service->protocol_plugin_set_error(gp, "%s", _(errmsg));
		return FALSE;
	}

	/* embed it */
	onMainThread_gtk_socket_add_id(GTK_SOCKET(gpdata->socket), gpdata->window_id);

	return TRUE;
}

static gboolean rmplugin_x2go_main(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	gboolean ret = FALSE;

	ret = rmplugin_x2go_start_session(gp);

	gpdata->thread = 0;
	return ret;
}

static gpointer rmplugin_x2go_main_thread(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	if (!gp) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("RemminaProtocolWidget* gp is 'NULL'!")
		));
		return NULL;
	}

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	CANCEL_ASYNC
	if (!rmplugin_x2go_main(gp)) {
		IDLE_ADD((GSourceFunc) rmplugin_x2go_cleanup, gp);
	}

	return NULL;
}

static gboolean rmplugin_x2go_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);

	if (!rm_plugin_service->gtksocket_available()) {
		rm_plugin_service->protocol_plugin_set_error(gp, _("The %s protocol is "
				  "unavailable because GtkSocket only works under X.org"),
				  PLUGIN_NAME);
		return FALSE;
	}

	gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));
	// casting to void* is allowed since return type 'gpointer' is actually void*.
	if (pthread_create(&gpdata->thread, NULL, (void*) rmplugin_x2go_main_thread, gp)) {
		rm_plugin_service->protocol_plugin_set_error(gp, _("Could not initialize "
					  "pthread. Falling back to non-threaded mode…"));
		gpdata->thread = 0;
		return FALSE;
	} else  {
		return TRUE;
	}
}

static gboolean rmplugin_x2go_query_feature(RemminaProtocolWidget* gp,
					    const RemminaProtocolFeature* feature)
{
	TRACE_CALL(__func__);
	return TRUE;
}

static const RemminaProtocolFeature rmplugin_x2go_features[] = {
	{REMMINA_PROTOCOL_FEATURE_TYPE_GTKSOCKET, RMPLUGIN_X2GO_FEATURE_GTKSOCKET, NULL, NULL, NULL},
	{REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL}
};


/**
 * @brief This function builds a string like: "'value1', 'value2' and 'value3'" \n
 * 	  To be used in a loop. \n
 *	  See rmplugin_x2go_string_setting_validator() for an example.
 *
 * @param max_elements Number of maximum elements.
 * @param element_to_add Next element to add to the string
 * @param current_element Which element is element_to_add?
 * @param string The string to which `element_to_add` will be added.
 */
static gchar* rmplugin_x2go_enumeration_prettifier(const guint max_elements,
						   const guint current_element,
						   gchar* element_to_add,
						   gchar* string)
{
	if (max_elements > 2) {
		if (current_element == max_elements - 1) {
			// TRANSLATORS: Presumably you just want to translate 'and' into
			// your language.
			// (Except your listing-grammar differs from english.)
			// 'value1', 'value2', 'valueN-1' and 'valueN'
			return g_strdup_printf(_("%sand '%s'"), string, element_to_add);
		} else if (current_element == max_elements - 2) {
			// TRANSLATORS: Presumably you just want to leave it english.
			// (Except your listing-grammar differs from english.)
			// 'value1', 'value2', 'valueN-1' and 'valueN'
			return g_strdup_printf(_("%s'%s' "), string, element_to_add);
		} else {
			// TRANSLATORS: Presumably you just want to leave it english.
			// (Except your listing-grammar differs from english.)
			// 'value1', 'value2', 'valueN-1' and 'valueN'
			return g_strdup_printf(_("%s'%s', "), string, element_to_add);
		}
	} else if (max_elements == 2) {
		if (current_element == max_elements - 1) {
			// TRANSLATORS: Presumably you just want to translate 'and' into
			// your language.
			// (Except your listing-grammar differs from english.)
			// 'value1' and 'value2'
			return g_strdup_printf(_("%sand '%s'"), string, element_to_add);
		} else {
			// TRANSLATORS: Presumably you just want to leave it english.
			// (Except your listing-grammar differs from english.)
			// 'value1' and 'value2'
			return g_strdup_printf(_("%s'%s' "), string, element_to_add);
		}
	} else {
		return g_strdup(element_to_add);
	}
}

/**
 * @brief Validator-functions are getting executed when the user wants to save profile
 *	  settings. It uses the given data (See RemminaProtocolSetting array) to determine
 *	  which strings are allowed and returns a end-user friendly error message.
 *
 * @param key Key is the setting's name.
 * @param value Value to validate.
 * @param data Data needed for validation process. See RemminaProtocolSetting array.
 *
 * @returns End-user friendly and translated error message, explaining why the given
 *	    value is invalid. If the given value is error-free then NULL gets returned.
 *
 */
static GError* rmplugin_x2go_string_setting_validator(gchar* key, gchar* value,
						      gchar* data)
{
	GError *error = NULL;

	if (!data) {
		gchar *error_msg = _("Invalid validation data in ProtocolSettings array!");
		REMMINA_PLUGIN_CRITICAL("%s", error_msg);
		g_set_error(&error, 1, 1, error_msg);
		return error;
	}

	gchar **elements_list = g_strsplit(data, ",", 0);

	guint elements_amount = 0;
	for (; elements_list[elements_amount] != NULL; elements_amount++) {
		// Just advance elements_amount...
	}

	if (elements_list == NULL ||
	    elements_list[0] == NULL ||
	    strlen(elements_list[0]) <= 0)
	{
		gchar *error_msg = _("Validation data in ProtocolSettings array is invalid!");
		REMMINA_PLUGIN_CRITICAL("%s", error_msg);
		g_set_error(&error, 1, 1, error_msg);
		return error;
	}

	gchar *data_str = "";

	if (!key || !value) {
		REMMINA_PLUGIN_CRITICAL("%s", _("Parameters 'key' or 'value' are 'NULL'!"));
		g_set_error(&error, 1, 1, _("Internal error."));
		return error;
	}

	for (guint i = 0; elements_list[i] != NULL; i++) {
		// Don't wanna crash if elements_list[i] is NULL.
		gchar* element = elements_list[i] ? elements_list[i] : "";
		if (g_strcmp0(value, element) == 0) {
			// We found value in elements_list. Value passed validation.
			return NULL;
		}

		data_str = rmplugin_x2go_enumeration_prettifier(elements_amount, i,
						     		element, data_str);
	}

	if (elements_amount > 1) {
		g_set_error(&error, 1, 1, _("Allowed values are %s."), data_str);
	} else {
		g_set_error(&error, 1, 1, _("The only allowed value is '%s'."), data_str);
	}

	g_free(data_str);
	g_strfreev(elements_list);

	return error;
}

/**
 * @brief Validator-functions are getting executed when the user wants to save profile
 *	  settings. It uses the given data (See RemminaProtocolSetting array) to determine
 *	  if the given value is a valid integer is in range and returns a end-user
 *	  friendly error message.
 *
 * @param key Key is the setting's name.
 * @param value Value to validate.
 * @param data Data needed for validation process. See RemminaProtocolSetting array.
 *
 * @returns End-user friendly and translated error message, explaining why the given
 *	    value is invalid. If the given value is error-free then NULL gets returned.
 *
 */
static GError* rmplugin_x2go_int_setting_validator(gchar* key, gpointer value,
						   gchar* data)
{
	GError *error = NULL;

	gchar **integer_list = g_strsplit(data, ";", 0);

	if (integer_list == NULL ||
	    integer_list[0] == NULL ||
	    integer_list[1] == NULL ||
	    strlen(integer_list[0]) <= 0 ||
	    strlen(integer_list[1]) <= 0)
	{
		gchar *error_msg = _("Validation data in ProtocolSettings array is invalid!");
		REMMINA_PLUGIN_CRITICAL("%s", error_msg);
		g_set_error(&error, 1, 1, error_msg);
		return error;
	}

	gint minimum;
	str2int_errno err = str2int(&minimum, integer_list[0], 10);
	if (err == STR2INT_INCONVERTIBLE) {
		g_set_error(&error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("The lower limit is not a valid integer!")
		));
	} else if (err == STR2INT_OVERFLOW) {
		g_set_error(&error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("The lower limit is too high!")
		));
	} else if (err == STR2INT_UNDERFLOW) {
		g_set_error(&error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("The lower limit is too low!")
		));
	} else if (err == STR2INT_INVALID_DATA) {
		g_set_error(&error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("Something unknown went wrong.")
		));
	}

	if (error) {
		REMMINA_PLUGIN_CRITICAL("%s", _("Please check the RemminaProtocolSetting "
						"array for possible errors."));
		return error;
	}

	gint maximum;
	err = str2int(&maximum, integer_list[1], 10);
	if (err == STR2INT_INCONVERTIBLE) {
		g_set_error(&error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("The upper limit is not a valid integer!")
		));
	} else if (err == STR2INT_OVERFLOW) {
		g_set_error(&error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("The upper limit is too high!")
		));
	} else if (err == STR2INT_UNDERFLOW) {
		g_set_error(&error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("The upper limit is too low!")
		));
	} else if (err == STR2INT_INVALID_DATA) {
		g_set_error(&error, 1, 1, g_strdup_printf(
			_("Internal error: %s"),
			_("Something unknown went wrong.")
		));
	}

	if (error) {
		REMMINA_PLUGIN_CRITICAL("%s", _("Please check the RemminaProtocolSetting "
						"array for possible errors."));
		return error;
	}

	gint int_value;
	err = str2int(&int_value, value, 10);
	if (err == STR2INT_INCONVERTIBLE) {
		// Can't happen in theory since non-numerical characters are can't
		// be entered but, let's be safe.
		g_set_error(&error, 1, 1, _("The input is not a valid integer!"));
	} else if (err == STR2INT_OVERFLOW || err == STR2INT_UNDERFLOW) {
		g_set_error(&error, 1, 1, _("Input must be a number between %i and %i."),
					minimum, maximum);
	} else if (err == STR2INT_INVALID_DATA) {
		g_set_error(&error, 1, 1, _("Something unknown went wrong."));
	}

	if (error) {
		REMMINA_PLUGIN_CRITICAL("%s", _("Please check the RemminaProtocolSetting "
						"array for possible errors."));
		return error;
	}

	/*REMMINA_PLUGIN_DEBUG("Key:  \t%s", (gchar*) key);
	REMMINA_PLUGIN_DEBUG("Value:\t%s", (gchar*) value);
	REMMINA_PLUGIN_DEBUG("Data: \t%s", data);
	REMMINA_PLUGIN_DEBUG("Min: %i, Max: %i", minimum, maximum);
	REMMINA_PLUGIN_DEBUG("Value converted:\t%i", int_value);*/

	if (err == STR2INT_SUCCESS && (minimum > int_value || int_value > maximum)) {
		g_set_error(&error, 1, 1, _("Input must be a number between %i and %i."),
			    minimum, maximum);
	}

	// Should be NULL.
	return error;
}

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting tooltip
 * g) Validation data pointer, will be passed to the validation callback method.
 * h) Validation callback method (Can be NULL. Every entry will be valid then.)
 *		use following prototype:
 *		gboolean mysetting_validator_method(gpointer key, gpointer value,
 *						    gpointer validator_data);
 *		gpointer key is a gchar* containing the setting's name,
 *		gpointer value contains the value which should be validated,
 *		gpointer validator_data contains your passed data.
 */
static const RemminaProtocolSetting rmplugin_x2go_basic_settings[] = {
    {REMMINA_PROTOCOL_SETTING_TYPE_SERVER,	"server",	NULL,				FALSE, NULL, NULL, NULL, NULL},
    {REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"username",	N_("Username"), 		FALSE, NULL, NULL, NULL, NULL},
    {REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD,	"password",	N_("Password"), 		FALSE, NULL, NULL, NULL, NULL},
    {REMMINA_PROTOCOL_SETTING_TYPE_COMBO,	"command",	N_("Startup program"),		FALSE,
     /* SELECT & COMBO Values */ "MATE,KDE,XFCE,LXDE,TERMINAL",
     /* Tooltip */ N_("Which command should be executed after creating the X2Go session?"), 			   NULL, NULL},
    {REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION,	"resolution",	NULL, 				FALSE, NULL, NULL, NULL, NULL},
    {REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"kbdlayout", 	N_("Keyboard Layout (auto)"), 	FALSE, NULL, NULL, NULL, NULL},
    {REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"kbdtype", 	N_("Keyboard type (auto)"), 	FALSE, NULL, NULL, NULL, NULL},
    {REMMINA_PROTOCOL_SETTING_TYPE_COMBO,	"audio", 	N_("Audio support"), 		FALSE,
     /* SELECT & COMBO Values */ "pulse,esd,none",
     /* Tooltip */ N_("The sound system of the X2Go server (default: 'pulse')."),
     /* Validation data */ "pulse,esd,none",
     /* Validation method */ G_CALLBACK(rmplugin_x2go_string_setting_validator)},
    {REMMINA_PROTOCOL_SETTING_TYPE_COMBO,	"clipboard", 	N_("Clipboard direction"), 	FALSE,
     /* SELECT & COMBO Values */ "none,server,client,both",
     /* Tooltip */ N_("Which direction should clipboard content be copied? "
		      "(default: 'both')."),
     /* Validation data */ "none,server,client,both",
     /* Validation method */ G_CALLBACK(rmplugin_x2go_string_setting_validator)},
    {REMMINA_PROTOCOL_SETTING_TYPE_INT,		"dpi", 		N_("DPI resolution"), 		FALSE,	NULL,
     /* Tooltip */ N_("Launch session with a specific resolution (in dots per inch). "
		      "Must be between 20 and 400."),
     /* Validation data */ "20;400", // "<min>;<max>;"
     /* Validation method */ G_CALLBACK(rmplugin_x2go_int_setting_validator)},
    {REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL, NULL, NULL}};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin rmplugin_x2go = {
	REMMINA_PLUGIN_TYPE_PROTOCOL,		// Type
	PLUGIN_NAME,				// Name
	PLUGIN_DESCRIPTION,			// Description
	GETTEXT_PACKAGE,			// Translation domain
	PLUGIN_VERSION,				// Version number
	PLUGIN_APPICON,				// Icon for normal connection
	PLUGIN_SSH_APPICON,			// Icon for SSH connection
	rmplugin_x2go_basic_settings,		// Array for basic settings
	NULL,					// Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,	// SSH settings type
	/* REMMINA_PROTOCOL_SSH_SETTING_NONE,	// SSH settings type */
	rmplugin_x2go_features,			// Array for available features
	rmplugin_x2go_init,			// Plugin initialization method
	rmplugin_x2go_open_connection,		// Plugin open connection method
	rmplugin_x2go_close_connection,		// Plugin connection-method closure
	rmplugin_x2go_query_feature,		// Query for available features
	NULL,					// Call a feature
	NULL,					// Send a keystroke
	NULL,					// Screenshot
};

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL("remmina_plugin_entry");
	rm_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &rmplugin_x2go)) {
		return FALSE;
	}

	pthread_mutex_init(&remmina_x2go_init_mutex, NULL);
	remmina_x2go_window_id_array = g_array_new(FALSE, TRUE, sizeof(Window));

	REMMINA_PLUGIN_MESSAGE("%s", _("X2Go plugin loaded."));

	return TRUE;
}
