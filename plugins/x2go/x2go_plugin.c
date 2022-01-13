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
#define SET_RESUME_SESSION(gp, resume_data) \
		g_object_set_data_full(G_OBJECT(gp), "resume-session-data", \
				       resume_data, \
			               g_free)

#define GET_RESUME_SESSION(gp) \
		(gchar*) g_object_get_data(G_OBJECT(gp), "resume-session-data")

// A session is selected if the returning value is something other than 0.
#define IS_SESSION_SELECTED(gp) \
		g_object_get_data(G_OBJECT(gp), "session-selected") ? TRUE : FALSE

// We don't use the function as a real pointer but rather as a boolean value.
#define SET_SESSION_SELECTED(gp, is_session_selected) \
		g_object_set_data_full(G_OBJECT(gp), "session-selected", \
				       is_session_selected, \
				       NULL)
// -------------------

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

#define REMMINA_PLUGIN_AUDIT(fmt, ...) \
		rm_plugin_service->_remmina_audit(__func__, fmt, ##__VA_ARGS__)

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

/**
 * @brief Can be used to pass custom user data between functions and threads.
 *	  *AND* pass the useful RemminaProtocolWidget with it along.
 */
typedef struct _X2GoCustomUserData {
	RemminaProtocolWidget* gp;
	gpointer dialog_data;
	gpointer connect_data;
	gpointer opt1;
	gpointer opt2;
} X2GoCustomUserData;

/**
 * @brief Used for the session chooser dialog (GtkListStore)
 *	  See the example at: https://docs.gtk.org/gtk3/class.ListStore.html
 *	  The order is the exact same as the user sees in the dialog.
 *	  SESSION_NUM_PROPERTIES is used to keep count of the properties
 *	  and it must be the last object.
 */
enum SESSION_PROPERTIES {
	SESSION_DISPLAY = 0,
	SESSION_STATUS,
	SESSION_SESSION_ID,
	SESSION_SUSPENDED_SINCE,
	SESSION_CREATE_DATE,
	SESSION_AGENT_PID,
	SESSION_USERNAME,
	SESSION_HOSTNAME,
	SESSION_COOKIE,
	SESSION_GRAPHIC_PORT,
	SESSION_SND_PORT,
	SESSION_SSHFS_PORT,
	SESSION_DIALOG_IS_VISIBLE,
	SESSION_NUM_PROPERTIES // Must be last. Counts all enum elements.
};

// Following str2int code was adapted from Stackoverflow:
// https://stackoverflow.com/questions/7021725/how-to-convert-a-string-to-integer-in-c
typedef enum _str2int_errno {
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
 * @param custom_data X2GoCustomUserData structure with the following: \n
 *				gp -> gp (RemminaProtocolWidget*) \n
 *				dialog_data -> dialog data (struct _DialogData*)
 * @returns: FALSE. This source should be removed from main loop.
 * 	     #G_SOURCE_CONTINUE and #G_SOURCE_REMOVE are more memorable
 *	     names for the return value.
 */
static gboolean rmplugin_x2go_open_dialog(X2GoCustomUserData *custom_data)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (!custom_data || !custom_data->gp || !custom_data->dialog_data) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Parameter 'custom_data' is not initialized!")
		));

		return G_SOURCE_REMOVE;
	}

	RemminaProtocolWidget *gp = (RemminaProtocolWidget*) custom_data->gp;
	struct _DialogData *ddata = (struct _DialogData*) custom_data->dialog_data;

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

		// Calling dialog_factory_func(custom_data, dialog_factory_data);
		widget_gtk_dialog = ((GtkWidget* (*)(X2GoCustomUserData*, gpointer))
			dialog_factory_func)(custom_data, dialog_factory_data);
	} else {
		widget_gtk_dialog = gtk_message_dialog_new(ddata->parent,
							   ddata->flags,
							   ddata->type,
							   ddata->buttons,
							   "%s", ddata->title);

		gtk_message_dialog_format_secondary_text(
			GTK_MESSAGE_DIALOG(widget_gtk_dialog), "%s", ddata->message);
	}

	if (!widget_gtk_dialog) {
		REMMINA_PLUGIN_CRITICAL("Error! Aborting.");
		return G_SOURCE_REMOVE;
	}

	if (ddata->callbackfunc) {
		g_signal_connect_swapped(G_OBJECT(widget_gtk_dialog), "response",
					 G_CALLBACK(ddata->callbackfunc),
					 custom_data);
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

/**
 * @brief These define the responses of session-chooser-dialog's buttons.
 */
enum SESSION_CHOOSER_RESPONSE_TYPE {
  SESSION_CHOOSER_RESPONSE_NEW = 0,
  SESSION_CHOOSER_RESPONSE_CHOOSE,
  SESSION_CHOOSER_RESPONSE_TERMINATE,
};

/**
 * @brief Finds a child GtkWidget of a parent GtkWidget.
 * 	  Copied from https://stackoverflow.com/a/23497087 ;)
 *
 * @param parent Parent GtkWidget*
 * @param name Name string of child. (Must be set before, er else it will be a
 *	       default string)
 * @return GtkWidget*
 */
static GtkWidget* rmplugin_x2go_find_child(GtkWidget* parent, const gchar* name)
{
	const gchar* parent_name = gtk_widget_get_name((GtkWidget*) parent);
	if (g_ascii_strcasecmp(parent_name, (gchar*) name) == 0) {
		return parent;
	}

	if (GTK_IS_BIN(parent)) {
		GtkWidget *child = gtk_bin_get_child(GTK_BIN(parent));
		return rmplugin_x2go_find_child(child, name);
	}

	if (GTK_IS_CONTAINER(parent)) {
		GList *children = gtk_container_get_children(GTK_CONTAINER(parent));
		while (children != NULL) {
			GtkWidget *widget = rmplugin_x2go_find_child(children->data, name);
			if (widget != NULL) {
				return widget;
			}

			children = g_list_next(children);
		}
	}

	return NULL;
}

/**
 * @brief Gets executed on "row-activated" signal. It is emitted when the method when
 *	  the user double clicks a treeview row. It is also emitted when a non-editable
 *	  row is selected and one of the keys: Space, Shift+Space, Return or Enter is
 *	  pressed.
 *
 * @param custom_data X2GoCustomUserData structure with the following: \n
 *				gp -> gp (RemminaProtocolWidget*) \n
 *				opt1 -> dialog widget (GtkWidget*)
 */
static gboolean rmplugin_x2go_session_chooser_row_activated(GtkTreeView *treeview,
							    GtkTreePath *path,
							    GtkTreeViewColumn *column,
							    X2GoCustomUserData *custom_data)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (!custom_data || !custom_data->gp || !custom_data->opt1) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Parameter 'custom_data' is not initialized!")
		));

		return G_SOURCE_REMOVE;
	}

	RemminaProtocolWidget* gp = (RemminaProtocolWidget*) custom_data->gp;
	// dialog_data (unused)
	// connect_data (unused)
	GtkWidget* dialog = GTK_WIDGET(custom_data->opt1);

	gchar *session_id;
	GtkTreeIter iter;
	GtkTreeModel *model = gtk_tree_view_get_model(treeview);

	if (gtk_tree_model_get_iter(model, &iter, path)) {
		gtk_tree_model_get(GTK_TREE_MODEL(model), &iter,
				   SESSION_SESSION_ID, &session_id, -1);

		// Silent bail out.
		if (!session_id || strlen(session_id) <= 0) return G_SOURCE_REMOVE;

		SET_RESUME_SESSION(gp, session_id);

		// Unstucking main process. Telling it that a session has been selected.
		// We use a trick here. As long as there is something other than 0
		// stored, a session is selected. So we use the gpointer as a gboolean.
		SET_SESSION_SELECTED(gp, (gpointer) TRUE);
		gtk_widget_hide(GTK_WIDGET(dialog));
		gtk_widget_destroy(GTK_WIDGET(dialog));
	}

	return G_SOURCE_REMOVE;
}

/**
 * @brief Translates a session property (described by SESSION_PROPERTIES enum) to a
 *	  string containing it's display name.
 *
 * @param session_property A session property. (as described by SESSION_PROPERTIES enum)
 * @return gchar* Translated display name. (Can be NULL, if session_property is invalid!)
 */
static gchar *rmplugin_x2go_session_property_to_string(guint session_property) {
	gchar* return_char = NULL;

	switch (session_property) {
		// I think we can close one eye here regarding max line-length.
		case SESSION_DISPLAY:		return_char = g_strdup(_("X Display"));		break;
		case SESSION_STATUS:		return_char = g_strdup(_("Status"));		break;
		case SESSION_SESSION_ID:	return_char = g_strdup(_("Session ID"));	break;
		case SESSION_CREATE_DATE:	return_char = g_strdup(_("Create date"));	break;
		case SESSION_SUSPENDED_SINCE:	return_char = g_strdup(_("Suspended since"));	break;
		case SESSION_AGENT_PID:		return_char = g_strdup(_("Agent PID"));		break;
		case SESSION_USERNAME:		return_char = g_strdup(_("Username"));		break;
		case SESSION_HOSTNAME:		return_char = g_strdup(_("Hostname"));		break;
		case SESSION_COOKIE:		return_char = g_strdup(_("Cookie"));		break;
		case SESSION_GRAPHIC_PORT:	return_char = g_strdup(_("Graphic port"));	break;
		case SESSION_SND_PORT:		return_char = g_strdup(_("SND port"));		break;
		case SESSION_SSHFS_PORT:	return_char = g_strdup(_("SSHFS port"));	break;
		case SESSION_DIALOG_IS_VISIBLE:	return_char = g_strdup(_("Visible"));		break;
	}

	return return_char;
}

/**
 * @brief Builds a dialog which contains all found X2Go-Sessions of the remote server.
 *	  The dialog gives the user the option to choose between resuming or terminating
 *	  an existing session or to create a new one.
 *
 * @param custom_data X2GoCustomUserData structure with the following: \n
 *				gp -> gp (RemminaProtocolWidget*) \n
 *				dialog_data -> dialog data (struct _DialogData*) \n
 *				connect_data -> connection data (struct _ConnectionData*)
 * @param sessions_list The GList* Should contain all found X2Go-Sessions.
 *			Sessions are string arrays of properties.
 *			The type of the GList is gchar**.
 *
 * @returns GtkWidget* custom dialog.
 */
static GtkWidget* rmplugin_x2go_choose_session_dialog_factory(X2GoCustomUserData *custom_data,
							      GList *sessions_list)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (!custom_data || !custom_data->gp ||
	    !custom_data->dialog_data || !custom_data->connect_data) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Parameter 'custom_data' is not initialized!")
		));

		return NULL;
	}

	struct _DialogData* ddata = (struct _DialogData*) custom_data->dialog_data;

	if (!ddata || !sessions_list || !ddata->title) {
		REMMINA_PLUGIN_CRITICAL("%s", _("Couldn't retrieve valid `DialogData` or "
					        "`sessions_list`! Aborting…"));
		return NULL;
	}

	GtkWidget *widget_gtk_dialog = NULL;
	widget_gtk_dialog = gtk_dialog_new_with_buttons(ddata->title, ddata->parent,
							ddata->flags,
	// TRANSLATORS: Stick to x2goclient's translation for terminate.
							_("_Terminate"),
							SESSION_CHOOSER_RESPONSE_TERMINATE,
	// TRANSLATORS: Stick to x2goclient's translation for resume.
							_("_Resume"),
							SESSION_CHOOSER_RESPONSE_CHOOSE,
							_("_New"),
							SESSION_CHOOSER_RESPONSE_NEW,
							NULL);

	GtkWidget *button = gtk_dialog_get_widget_for_response(
						GTK_DIALOG(widget_gtk_dialog),
						SESSION_CHOOSER_RESPONSE_TERMINATE);
	// TRANSLATORS: Tooltip for terminating button inside Session-Chooser-Dialog.
	// TRANSLATORS: Please stick to X2GoClient's way of translating.
	gtk_widget_set_tooltip_text(button, _("Terminating X2Go sessions can take a moment."));

	#define DEFAULT_DIALOG_WIDTH 720
	#define DEFAULT_DIALOG_HEIGHT (DEFAULT_DIALOG_WIDTH * 9) / 16

	gtk_widget_set_size_request(GTK_WIDGET(widget_gtk_dialog),
				    DEFAULT_DIALOG_WIDTH, DEFAULT_DIALOG_HEIGHT);
	gtk_window_set_default_size(GTK_WINDOW(widget_gtk_dialog),
				    DEFAULT_DIALOG_WIDTH, DEFAULT_DIALOG_HEIGHT);

	gtk_window_set_resizable(GTK_WINDOW(widget_gtk_dialog), TRUE);

	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	//gtk_widget_show(scrolled_window);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(
				GTK_DIALOG(widget_gtk_dialog))
			   ), GTK_WIDGET(scrolled_window), TRUE, TRUE, 5);

	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled_window),
				       GTK_POLICY_AUTOMATIC,
				       GTK_POLICY_AUTOMATIC);


	GType types[SESSION_NUM_PROPERTIES];

	// First to last in SESSION_PROPERTIES.
	for (gint i = 0; i < SESSION_NUM_PROPERTIES; ++i) {
		// Everything is a String. (Except IS_VISIBLE flag)
		// If that changes one day, you could extent the if statement here.
		// But you would propably need a *lot* of refactoring.
		// Especially in the session parser.
		if (i == SESSION_DIALOG_IS_VISIBLE) {
			types[i] = G_TYPE_BOOLEAN;
		} else {
			types[i] = G_TYPE_STRING;
		}
	}

	// create tree view
	GtkListStore *store = gtk_list_store_newv(SESSION_NUM_PROPERTIES, types);

	GtkTreeModelFilter *filter = GTK_TREE_MODEL_FILTER(
					gtk_tree_model_filter_new(GTK_TREE_MODEL(store),
								  NULL)
				     );
	gtk_tree_model_filter_set_visible_column(filter, SESSION_DIALOG_IS_VISIBLE);

	GtkWidget *tree_view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(filter));
	g_object_unref (G_OBJECT (store));  // tree now holds reference
	gtk_widget_set_size_request(tree_view, -1, 300);

	// Gets name to be findable by rmplugin_x2go_find_child()
	gtk_widget_set_name(GTK_WIDGET(tree_view), "session_chooser_treeview");

	// create list view columns
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree_view), TRUE);
	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW(tree_view), FALSE);
	gtk_tree_view_set_enable_search(GTK_TREE_VIEW(tree_view), TRUE);
	gtk_widget_show (tree_view);
	gtk_container_add (GTK_CONTAINER(scrolled_window), tree_view);

	GtkTreeViewColumn *tree_view_col = NULL;
	GtkCellRenderer *cell_renderer = NULL;
	gchar *header_title = NULL;

	// First to last in SESSION_PROPERTIES.
	for (guint i = 0; i < SESSION_NUM_PROPERTIES; ++i) {
		// Do not display SESSION_DIALOG_IS_VISIBLE.
		if (i == SESSION_DIALOG_IS_VISIBLE) continue;

		header_title = rmplugin_x2go_session_property_to_string(i);
		if (!header_title) {
			REMMINA_PLUGIN_WARNING("%s", g_strdup_printf(
				_("Internal error: %s"), g_strdup_printf(
				_("Unknown property '%i'"), i
			)));
			header_title = g_strdup_printf(_("Unknown property '%i'"), i);
		}

		tree_view_col = gtk_tree_view_column_new();
		gtk_tree_view_column_set_title(tree_view_col, header_title);
		gtk_tree_view_column_set_clickable(tree_view_col, FALSE);
		gtk_tree_view_column_set_sizing (tree_view_col, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
		gtk_tree_view_column_set_resizable(tree_view_col, TRUE);

		cell_renderer = gtk_cell_renderer_text_new();
		gtk_tree_view_column_pack_start(tree_view_col, cell_renderer, TRUE);
		gtk_tree_view_column_add_attribute(tree_view_col, cell_renderer, "text", i);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree_view), tree_view_col);
	}

	GList *elem = NULL;
	GtkTreeIter iter;

	for (elem = sessions_list; elem; elem = elem->next) {
		gchar** session = (gchar**) elem->data;
		g_assert(session != NULL);

		gtk_list_store_append(store, &iter);

		for (gint i = 0; i < SESSION_NUM_PROPERTIES; i++) {
			gchar* property = session[i];
			GValue a = G_VALUE_INIT;

			// Everything here is a string (except SESSION_DIALOG_IS_VISIBLE)

			if (i == SESSION_DIALOG_IS_VISIBLE) {
				g_value_init(&a, G_TYPE_BOOLEAN);
				g_assert(G_VALUE_HOLDS_BOOLEAN(&a) && "GValue does not "
								      "hold a boolean!");
				// Default is to show every new session.
				g_value_set_boolean(&a, TRUE);
			} else {
				g_value_init(&a, G_TYPE_STRING);
				g_assert(G_VALUE_HOLDS_STRING(&a) && "GValue does not "
								      "hold a string!");
				g_value_set_static_string (&a, property);
			}

			gtk_list_store_set_value(store, &iter, i, &a);
		}
	}

	/* Prepare X2GoCustomUserData *custom_data
	 *	gp -> gp (RemminaProtocolWidget*)
	 *	dialog_data -> dialog data (struct _DialogData*)
	 *	connect_data -> connection data (struct _ConnectionData*)
	 *	opt1 -> dialog widget (GtkWidget*)
	 */
	// everything else is already initialized.
	custom_data->opt1 = widget_gtk_dialog;

	g_signal_connect(tree_view, "row-activated",
			 G_CALLBACK(rmplugin_x2go_session_chooser_row_activated),
			 custom_data);

	return widget_gtk_dialog;
}

/**
 * @brief Uses either 'dialog' or 'treeview' to return the GtkTreeModel of the
 *	  Session-Chooser-Dialog. Directly giving 'treeview' as a parameter is faster.
 *	  Only *one* parameter has to be given. The other one can be NULL.
 *	  Error messages are all handled already.
 *
 * @param dialog The Session-Chooser-Dialog itself. (Slower) Can be NULL.
 * @param treeview The GtkTreeView of the Session-Chooser-Dialog. (faster) Can be NULL.
 * @return GtkTreeModelFilter* (Does not contains all rows, only the visible ones!!!) \n
 *	   But you can get all rows with: \n
 *	   	gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(return_model));
 */
static GtkTreeModelFilter* rmplugin_x2go_session_chooser_get_filter_model(GtkWidget *dialog,
							                  GtkTreeView* treeview)
{
	//REMMINA_PLUGIN_DEBUG("Function entry.");
	GtkTreeModel *return_model = NULL;

	if (!treeview && dialog) {
		GtkWidget *treeview_new = rmplugin_x2go_find_child(GTK_WIDGET(dialog),
						              "session_chooser_treeview");

		if (!treeview_new) {
			REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
				_("Internal error: %s"),
				_("Couldn't find child GtkTreeView of "
				  "session chooser dialog.")
			));
			return NULL;
		}

		return_model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview_new));
	} else if (treeview) {
		return_model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	} else {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Neither the 'dialog' nor 'treeview' parameters are initialized! "
			  "At least one of them must be given.")
		));
		return NULL;
	}

	if (!return_model || !GTK_TREE_MODEL_FILTER(return_model)) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Could not obtain \"GtkTreeModelFilter*\" of the session chooser dialog, "
			  "for unknown reason.")
		));
	}

	return GTK_TREE_MODEL_FILTER(return_model);
}

/**
 * @brief Gets the selected row of the Session-Chooser-Dialog.
 *	  The path gets converted with gtk_tree_model_filter_convert_child_path_to_path()
 *	  before it gets returned. So path describes a row of 'filter' and *not* its
 *	  child GtkTreeModel.
 *
 * @param dialog The Session-Chooser-Dialog.
 * @return GtkTreePath* describing the path to the row.
 */
static GtkTreePath* rmplugin_x2go_session_chooser_get_selected_row(GtkWidget *dialog)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	GtkWidget *treeview = rmplugin_x2go_find_child(GTK_WIDGET(dialog),
						       "session_chooser_treeview");
	if (!treeview) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Couldn't find child GtkTreeView of session chooser dialog.")
		));
		return NULL;
	}

	GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	if (!selection) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Couldn't get currently selected row (session)!")
		));
		return NULL;
	}

	GtkTreeModelFilter *filter = rmplugin_x2go_session_chooser_get_filter_model(
							   NULL, GTK_TREE_VIEW(treeview));
	GtkTreeModel *model = gtk_tree_model_filter_get_model(filter);
	if (!model) return NULL; // error message was already handled.

	GtkTreeModel *filter_model = GTK_TREE_MODEL(filter);
	g_assert(filter_model && "Could not cast 'filter' to a GtkTreeModel!");
	GList *selected_rows = gtk_tree_selection_get_selected_rows(selection, &filter_model);

	// We only support single selection.
	gint selected_rows_num = gtk_tree_selection_count_selected_rows(selection);
	if (selected_rows_num != 1) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"), g_strdup_printf(
			_("Exactly one session should be selectable but '%i' rows "
			  "(sessions) are selected."),
			selected_rows_num
		)));
		return NULL;
	}

	// This would be very dangerous (we didn't check for NULL) if we hadn't just
	// checked that only one row is selected.
	GtkTreePath *path = selected_rows->data;

	// Convert to be path of GtkTreeModelFilter and *not* its child GtkTreeModel.
	path = gtk_tree_model_filter_convert_child_path_to_path(filter, path);

	return path;
}

/**
 * @brief Finds the GtkTreeView inside of the session chooser dialog,
 *	  determines the selected row and extracts a property.
 *
 * @param dialog GtkWidget* the dialog itself.
 * @param property_index Index of property.
 * @param row A specific row to get the property of. (Can be NULL)
 *
 * @return GValue The value of property. (Can be non-initialized!)
 */
static GValue rmplugin_x2go_session_chooser_get_property(GtkWidget *dialog,
							 gint property_index,
							 GtkTreePath *row)
{
	//REMMINA_PLUGIN_DEBUG("Function entry.");

	GValue ret_value = G_VALUE_INIT;

	if (!row) {
		GtkTreePath *selected_row = rmplugin_x2go_session_chooser_get_selected_row(dialog);
		if (!selected_row) return ret_value; // error message was already handled.
		row = selected_row;
	}

	GtkTreeModelFilter *filter = rmplugin_x2go_session_chooser_get_filter_model(dialog, NULL);
	GtkTreeModel *model = gtk_tree_model_filter_get_model(filter);
	if (!model) return ret_value; // error message was already handled.

	GtkTreeIter iter;
	gboolean success = gtk_tree_model_get_iter(model, &iter, row);
	if (!success) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Failed to fill 'GtkTreeIter'.")
		));

		return ret_value;
	}

	GValue property = G_VALUE_INIT;
	gtk_tree_model_get_value(model, &iter, property_index, &property);

	return property;
}

/**
 * @brief This function dumps all properties of a session to the console.
 *	  It can/should be used with: \n
 *	      gtk_tree_model_foreach(GTK_TREE_MODEL(model), (GtkTreeModelForeachFunc) \n
 *				     rmplugin_x2go_dump_session_properties, \n
 *				     dialog);
 */
/*static void rmplugin_x2go_dump_session_properties(GtkTreeModel *model, GtkTreePath *path,
						  GtkTreeIter *iter, GtkWidget *dialog)
{
	//REMMINA_PLUGIN_DEBUG("Function entry.");

	g_debug(_("Properties for session with path '%s':"), gtk_tree_path_to_string(path));
	for (guint i = 0; i < SESSION_NUM_PROPERTIES; i++) {
		GValue property = G_VALUE_INIT;
		property = rmplugin_x2go_session_chooser_get_property(dialog, i, path);

		gchar* display_name = rmplugin_x2go_session_property_to_string(i);
		g_assert(display_name && "Couldn't get display name for a property!");

		if (i == SESSION_DIALOG_IS_VISIBLE) {
			g_assert(G_VALUE_HOLDS_BOOLEAN(&property) && "GValue does not "
								      "hold a boolean!");
			g_debug("\t%s: '%s'", display_name,
				g_value_get_boolean(&property) ? "TRUE" : "FALSE");
		} else {
			g_assert(G_VALUE_HOLDS_STRING(&property) && "GValue does not "
								      "hold a string!");
			g_debug("\t%s: '%s'", display_name, g_value_get_string(&property));
		}
	}
}*/

/**
 * @brief This function synchronously spawns a pyhoca-cli process with argv as arguments.
 * @param argc Number of arguments.
 * @param argv Arguments as string array. \n
 *	       Last elements has to be NULL. \n
 *	       Strings will get freed automatically.
 * @param error Will be filled with an error message on fail.
 * @param env String array of enviroment variables. \n
 *	      The list is NULL terminated and each item in
 *	      the list is of the form `NAME=VALUE`.
 *
 * @returns Returns either standard output string or NULL if it failed.
 */
static gchar* rmplugin_x2go_spawn_pyhoca_process(guint argc, gchar* argv[],
						 GError** error, gchar** env)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (!argv) {
		gchar* errmsg = g_strdup_printf(
			_("Internal error: %s"),
			_("parameter 'argv' is 'NULL'.")
		);
		REMMINA_PLUGIN_CRITICAL("%s", errmsg);
		g_set_error(error, 1, 1, "%s", errmsg);
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

	if (!env || !env[0]) {
		gchar* errmsg = g_strdup_printf(
			_("Internal error: %s"),
			_("parameter 'env' is either invalid or uninitialized.")
		);
		REMMINA_PLUGIN_CRITICAL("%s", errmsg);
		g_set_error(error, 1, 1, "%s", errmsg);
		return NULL;
	}

	gint exit_code = 0;
	gchar *standard_out;
	// Just supresses pyhoca-cli's help message when pyhoca-cli's version is too old.
	gchar *standard_err;

	gboolean success_ret = g_spawn_sync(NULL, argv, env, G_SPAWN_SEARCH_PATH, NULL,
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

	/* TOO VERBOSE: */
	/*
	REMMINA_PLUGIN_DEBUG("%s", _("Started PyHoca-CLI with the "
				    "following environment variables:"));
	REMMINA_PLUGIN_DEBUG("%s", g_strjoinv("\n", env));
	*/

	if (standard_err && strlen(standard_err) > 0) {
		if (g_str_has_prefix(standard_err, "pyhoca-cli: error: a socket error "
				     "occured while establishing the connection:")) {
			// Log error into GUI.
			gchar* errmsg = g_strdup_printf(
				_("The necessary PyHoca-CLI process has encountered a "
				  "internet connection problem.")
			);

			// Log error into debug window and stdout
			REMMINA_PLUGIN_CRITICAL("%s:\n%s", errmsg, standard_err);
			g_set_error(error, 1, 1, "%s", errmsg);
			return NULL;
		} else {
			gchar* errmsg = g_strdup_printf(
				_("Could not start "
				  "PyHoca-CLI:\n%s"),
				standard_err
			);
			REMMINA_PLUGIN_CRITICAL("%s", errmsg);
			g_set_error(error, 1, 1, "%s", errmsg);
			return NULL;
		}
	} else if (!success_ret || (*error) || strlen(standard_out) <= 0 || exit_code) {
		if (!(*error)) {
			gchar* errmsg = g_strdup_printf(
				_("An unknown error occured while trying "
				  "to start PyHoca-CLI. Exit code: %i"),
				exit_code);
			REMMINA_PLUGIN_WARNING("%s", errmsg);
			g_set_error(error, 1, 1, "%s", errmsg);
		} else {
			gchar* errmsg = g_strdup_printf(
				_("An unknown error occured while trying to start "
				  "PyHoca-CLI. Exit code: %i. Error: '%s'"),
				exit_code, (*error)->message);
			REMMINA_PLUGIN_WARNING("%s", errmsg);
		}

		return NULL;
	}

	return standard_out;
}

/**
 * @brief Stores all necessary information needed for retrieving sessions from
 *	  a X2Go server.
 */
struct _ConnectionData {
	gchar* host;
	gchar* username;
	gchar* password;
};

/**
 * @brief Either sets a specific row visible or invisible.
 *	  Also handles 'terminate' and 'resume' buttons of session-chooser-dialog.
 *	  If there are no sessions available anymore, disable all buttons which are not 'new'
 *	  and if a session is available again, enable them.
 *
 * @param path Describes which row. (GtkTreePath*) \n
 *	       Should be from GtkTreeModelFilter's perspective!
 * @param value TRUE = row is visible & FALSE = row is invisible (gboolean)
 * @param dialog Session-Chooser-Dialog (GtkDialog*)
 * @return Returns TRUE if successful. (gboolean)
 */
static gboolean rmplugin_x2go_session_chooser_set_row_visible(GtkTreePath *path,
							      gboolean value,
							      GtkDialog *dialog) {
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (!path || !dialog) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Neither the 'path' nor 'dialog' parameters are initialized.")
		));
		return FALSE;
	}

	GtkTreeModelFilter *filter = rmplugin_x2go_session_chooser_get_filter_model(
								GTK_WIDGET(dialog), NULL);
	GtkTreeModel *model = gtk_tree_model_filter_get_model(filter);

	// error message was already handled.
	if (!model) return FALSE;

	GtkTreeIter iter;
	if (!gtk_tree_model_get_iter(GTK_TREE_MODEL(model), &iter, path)) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("GtkTreePath 'path' describes a non-existing row!")
		));
		return FALSE;
	}


	// Make session either visible or invisible.
	gtk_list_store_set(GTK_LIST_STORE(model), &iter,
			   SESSION_DIALOG_IS_VISIBLE, value, -1);

	// Update row.
	gtk_tree_model_row_changed(GTK_TREE_MODEL(model), path, &iter);

	/* Get IS_VISIBLE flag of a session. */
	// GValue ret_value = G_VALUE_INIT;
	// ret_value = rmplugin_x2go_session_chooser_get_property(GTK_WIDGET(dialog),
	// 						       SESSION_DIALOG_IS_VISIBLE,
	// 						       path);
	// g_debug("Is visible: %s", g_value_get_boolean(&ret_value) ? "TRUE" : "FALSE");


	GtkWidget *term_button = gtk_dialog_get_widget_for_response(
					GTK_DIALOG(dialog),
					SESSION_CHOOSER_RESPONSE_TERMINATE);
	GtkWidget *resume_button = gtk_dialog_get_widget_for_response(
					GTK_DIALOG(dialog),
					SESSION_CHOOSER_RESPONSE_CHOOSE);

	// If no (visible) row is left to terminate disable terminate and resume buttons.
	gint rows_amount = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(filter), NULL);
	if (rows_amount <= 0) {
		gtk_widget_set_sensitive(term_button, FALSE);
		gtk_widget_set_sensitive(resume_button, FALSE);
	} else {
		gtk_widget_set_sensitive(term_button, TRUE);
		gtk_widget_set_sensitive(resume_button, TRUE);
	}

	// Success, yay!
	return TRUE;
}

/**
 * @brief Terminates a specific X2Go session using pyhoca-cli.
 *
 * @param custom_data X2GoCustomUserData structure with the following: \n
 *				gp -> gp (RemminaProtocolWidget*) \n
 *				dialog_data -> dialog data (struct _DialogData*) \n
 *				connect_data -> connection data (struct _ConnectionData*) \n
 *				opt1 -> selected row (GtkTreePath*) \n
 *				opt2 -> session-selection-dialog (GtkDialog*)
 *
 * @return G_SOURCE_REMOVE (FALSE), #G_SOURCE_CONTINUE and #G_SOURCE_REMOVE are more
 *	   memorable names for the return value. See GLib docs. \n
 *	   https://docs.gtk.org/glib/const.SOURCE_REMOVE.html
 */
static gboolean rmplugin_x2go_pyhoca_terminate_session(X2GoCustomUserData *custom_data)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (!custom_data || !custom_data->gp ||
	    !custom_data->dialog_data || !custom_data->connect_data ||
	    !custom_data->opt1 || !custom_data->opt2) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Parameter 'custom_data' is not fully initialized!")
		));

		return G_SOURCE_REMOVE;
	}

	// Extract data passed by X2GoCustomUserData *custom_data.
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(custom_data->gp);
	//struct _DialogData *ddata = (struct _DialogData*) custom_data->dialog_data;
	struct _ConnectionData *connect_data = (struct _ConnectionData*) custom_data->connect_data;
	GtkTreePath* selected_row = (GtkTreePath*) custom_data->opt1;
	GtkDialog *dialog = GTK_DIALOG(custom_data->opt2);

	/* Check connect_data. */
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
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("'Invalid connection data.'")
		));

		return G_SOURCE_REMOVE;
	} else {
		host = connect_data->host;
		username = connect_data->username;
		password = connect_data->password;
	}

	GValue value = rmplugin_x2go_session_chooser_get_property(GTK_WIDGET(dialog),
								SESSION_SESSION_ID,
								selected_row);
	// error message was handled already.
	if (!G_VALUE_HOLDS_STRING(&value)) return G_SOURCE_REMOVE;
	const gchar *session_id = g_value_get_string(&value);

	// We will now start pyhoca-cli with only the '--terminate $SESSION_ID' option.
	// (and of course auth related stuff)
	gchar *argv[50];
	gint argc = 0;

	argv[argc++] = g_strdup("pyhoca-cli");

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
		REMMINA_PLUGIN_CRITICAL("%s", FEATURE_NOT_AVAIL_STR("USERNAME"));
		return G_SOURCE_REMOVE;
	}

	if (password && FEATURE_AVAILABLE(gpdata, "PASSWORD")) {
		if (FEATURE_AVAILABLE(gpdata, "AUTH_ATTEMPTS")) {
			argv[argc++] = g_strdup("--auth-attempts");
			argv[argc++] = g_strdup_printf ("%i", 0);
		} else {
			REMMINA_PLUGIN_WARNING("%s", FEATURE_NOT_AVAIL_STR("AUTH_ATTEMPTS"));
		}
		argv[argc++] = g_strdup("--force-password");
		argv[argc++] = g_strdup("--password");
		argv[argc++] = g_strdup_printf("%s", password);
	} else if (!password) {
		REMMINA_PLUGIN_CRITICAL("%s", FEATURE_NOT_AVAIL_STR("PASSWORD"));
		return G_SOURCE_REMOVE;
	}

	if (FEATURE_AVAILABLE(gpdata, "TERMINATE")) {
		argv[argc++] = g_strdup("--terminate");
		argv[argc++] = g_strdup_printf("%s", session_id);
	} else {
		REMMINA_PLUGIN_CRITICAL("%s", FEATURE_NOT_AVAIL_STR("TERMINATE"));
		return G_SOURCE_REMOVE;
	}

	if (FEATURE_AVAILABLE(gpdata, "NON_INTERACTIVE")) {
		argv[argc++] = g_strdup("--non-interactive");
	} else {
		REMMINA_PLUGIN_WARNING("%s", FEATURE_NOT_AVAIL_STR("NON_INTERACTIVE"));
	}

	argv[argc++] = NULL;

	GError* error = NULL;
	gchar** envp = g_get_environ();
	rmplugin_x2go_spawn_pyhoca_process(argc, argv, &error, envp);
	g_strfreev(envp);

	if (error) {
		gchar *err_msg = g_strdup_printf(
			_("An error occured while trying to terminate X2Go session '%s':\n%s"),
			session_id,
			error->message
		);

		REMMINA_PLUGIN_CRITICAL("%s", err_msg);

		struct _DialogData *err_ddata = g_new0(struct _DialogData, 1);
		err_ddata->parent = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(dialog)));
		err_ddata->flags = GTK_DIALOG_MODAL;
		err_ddata->type = GTK_MESSAGE_ERROR;
		err_ddata->buttons = GTK_BUTTONS_OK;
		err_ddata->title = _("An error occured.");
		err_ddata->message = err_msg;
		// We don't need the response.
		err_ddata->callbackfunc = NULL;
		// We don't need a custom dialog either.
		err_ddata->dialog_factory_func = NULL;
		err_ddata->dialog_factory_data = NULL;

		/* Prepare X2GoCustomUserData *custom_data
		*	gp -> gp (RemminaProtocolWidget*)
		*	dialog_data -> dialog data (struct _DialogData*)
		*/
		custom_data->gp = custom_data->gp;
		custom_data->dialog_data = err_ddata;
		custom_data->connect_data = NULL;
		custom_data->opt1 = NULL;
		custom_data->opt2 = NULL;

		IDLE_ADD((GSourceFunc) rmplugin_x2go_open_dialog, custom_data);

		// Too verbose:
		// GtkTreeModel *model = gtk_tree_model_filter_get_model(
		// 					GTK_TREE_MODEL_FILTER(filter));
		// gtk_tree_model_foreach(GTK_TREE_MODEL(model), (GtkTreeModelForeachFunc)
		//			  rmplugin_x2go_dump_session_properties, dialog);

		// Set row visible again since we couldn't terminate the session.
		if (!rmplugin_x2go_session_chooser_set_row_visible(selected_row, TRUE,
								   dialog)) {
			// error message was already handled.
			return G_SOURCE_REMOVE;
		}
	}

	return G_SOURCE_REMOVE;
}

/**
 * @brief Gets executed on dialog's 'response' signal
 *
 * @param custom_data X2GoCustomUserData*: \n
 *				gp -> gp (RemminaProtocolWidget*) \n
 *				dialog_data -> dialog data (struct _DialogData*) \n
 *				connect_data -> connection data (struct _ConnectionData*)
 * @param response_id See GTK 'response' signal.
 * @param self The dialog itself.
 *
 * @return gboolean, #G_SOURCE_CONTINUE and #G_SOURCE_REMOVE are more memorable
 *	   names for the return value. See GLib docs. \n
 *	   https://docs.gtk.org/glib/const.SOURCE_REMOVE.html
 */
static gboolean rmplugin_x2go_session_chooser_callback(X2GoCustomUserData* custom_data,
						       gint response_id,
						       GtkDialog *self)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	if (!custom_data || !custom_data->gp || !custom_data->dialog_data) {
		REMMINA_PLUGIN_CRITICAL("%s", g_strdup_printf(
			_("Internal error: %s"),
			_("Parameter 'custom_data' is not initialized!")
		));

		return G_SOURCE_REMOVE;
	}
	RemminaProtocolWidget *gp = (RemminaProtocolWidget*) custom_data->gp;

	// Don't need to run other stuff, if the user just wants a new session.
	// Also it can happen, that no session is there anymore which can be selected!
	if (response_id == SESSION_CHOOSER_RESPONSE_NEW) {
		REMMINA_PLUGIN_DEBUG("The user explicitly requested a new session. "
			             "Creating a new session…");
		SET_RESUME_SESSION(gp, NULL);

		// Unstucking main process. Telling it that a session has been selected.
		// We use a trick here. As long as there is something other
		// than 0 stored, a session is selected. So we use the gpointer as a gboolean.
		SET_SESSION_SELECTED(gp, (gpointer) TRUE);

		gtk_widget_destroy(GTK_WIDGET(self));

		return G_SOURCE_REMOVE;
	}

	// This assumes that there are sessions which can be selected!
	GValue value = rmplugin_x2go_session_chooser_get_property(
				GTK_WIDGET(self),
				SESSION_SESSION_ID,
				NULL // Let the function search for the selected row.
		       );

	// error message was handled already.
	if (!G_VALUE_HOLDS_STRING(&value)) return G_SOURCE_REMOVE;

	gchar *session_id = (gchar*) g_value_get_string(&value);

	if (response_id == SESSION_CHOOSER_RESPONSE_CHOOSE) {
		if (!session_id || strlen(session_id) <= 0) {
			REMMINA_PLUGIN_DEBUG(
				"%s",
				_("Couldn't get session ID from session chooser dialog.")
			);
			SET_RESUME_SESSION(gp, NULL);
		} else {
			SET_RESUME_SESSION(gp, session_id);

			REMMINA_PLUGIN_MESSAGE("%s", g_strdup_printf(
				_("Resuming session: '%s'"),
				session_id
			));
		}
	} else if (response_id == SESSION_CHOOSER_RESPONSE_TERMINATE) {
		if (!session_id || strlen(session_id) <= 0) {
			REMMINA_PLUGIN_DEBUG(
				"%s",
				_("Couldn't get session ID from session chooser dialog.")
			);
			SET_RESUME_SESSION(gp, NULL);
		} else {
			SET_RESUME_SESSION(gp, session_id);

			REMMINA_PLUGIN_MESSAGE("%s", g_strdup_printf(
				_("Terminating session: '%s'"),
				session_id
			));
		}

		GtkTreePath *path = rmplugin_x2go_session_chooser_get_selected_row(
									GTK_WIDGET(self));
		// error message was already handled.
		if (!path) return G_SOURCE_REMOVE;

		// Actually set row invisible.
		if (!rmplugin_x2go_session_chooser_set_row_visible(path, FALSE, self)) {
			// error message was already handled.
			return G_SOURCE_REMOVE;
		}

		/* Prepare X2GoCustomUserData *custom_data
		 *	gp -> gp (RemminaProtocolWidget*)
		 *	dialog_data -> dialog data (struct _DialogData*)
		 *	connect_data -> connection data (struct _ConnectionData*)
		 *	opt1 -> selected row (GtkTreePath*)
		 *	opt2 -> session selection dialog (GtkDialog*)
		 */
		// everything else is already initialized.
		custom_data->opt1 = path;
		custom_data->opt2 = self;

		// Actually start pyhoca-cli process with --terminate $session_id.
		g_thread_new("terminate-session-thread",
			     (GThreadFunc) rmplugin_x2go_pyhoca_terminate_session,
			     custom_data);

		// Dialog should stay open.
		return G_SOURCE_CONTINUE;
	} else {
		REMMINA_PLUGIN_DEBUG("User clicked dialog away. "
				     "Creating a new session then.");
		SET_RESUME_SESSION(gp, NULL);
	}

	// Unstucking main process. Telling it that a session has been selected.
	// We use a trick here. As long as there is something other
	// than 0 stored, a session is selected. So we use the gpointer as a gboolean.
	SET_SESSION_SELECTED(gp, (gpointer) TRUE);

	gtk_widget_destroy(GTK_WIDGET(self));

	return G_SOURCE_REMOVE;
}

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

	gchar *server;
	gint port;

	RemminaFile *remminafile = rm_plugin_service->protocol_plugin_get_file(gp);
	rm_plugin_service->get_server_port(rm_plugin_service->file_get_string(remminafile, "server"),
			22,
			&server,
			&port);

	REMMINA_PLUGIN_AUDIT(_("Disconnected from %s:%d via X2Go"), server, port);
	g_free(server), server = NULL;

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
					    RemminaProtocolWidget *gp)
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
	ddata->parent = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gp)));
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
	// We don't need a custom dialog either.
	ddata->dialog_factory_func = NULL;
	ddata->dialog_factory_data = NULL;

	/* Prepare X2GoCustomUserData *custom_data
	 *	gp -> gp (RemminaProtocolWidget*)
	 *	dialog_data -> dialog data (struct _DialogData*)
	 */
	X2GoCustomUserData *custom_data = g_new0(X2GoCustomUserData, 1);
	g_assert(custom_data && "custom_data could not be initialized.");

	custom_data->gp = gp;
	custom_data->dialog_data = ddata;
	custom_data->connect_data = NULL;
	custom_data->opt1 = NULL;

	IDLE_ADD((GSourceFunc) rmplugin_x2go_open_dialog, custom_data);

	// 1 Second. Give `Dialog` chance to open.
	usleep(1000 * 1000);

	rmplugin_x2go_close_connection(gp);
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
 * @param errmsg Pointer to error message string (set if function failed).
 * @param username Pointer to default username. Gets set to new username on success.
 * @param password Pointer to default password. Gets set to new password on success.
 *
 * @returns FALSE if auth failed and TRUE on success.
 */
static gboolean rmplugin_x2go_get_auth(RemminaProtocolWidget *gp, gchar** errmsg,
				       gchar** default_username, gchar** default_password)
{
	REMMINA_PLUGIN_DEBUG("Function entry.");

	g_assert(errmsg != NULL);
	g_assert(gp != NULL);
	g_assert(default_username != NULL);
	g_assert(default_password != NULL);

	if (!(*default_username)) {
		(*errmsg) = g_strdup_printf(
			_("Internal error: %s"),
			_("Parameter 'default_username' is uninitialized.")
		);
		REMMINA_PLUGIN_CRITICAL("%s", errmsg);
		return FALSE;
	}

	// We can handle ((*default_password) == NULL).
	// Password is probably NULL because something did go wrong at the secret-plugin.
	// For example: The user didn't input a password for keyring.
	if ((*default_password) == NULL) {
		(*default_password) = g_strdup("");
	}

	gchar *s_username, *s_password;
	gint ret;
	gboolean save;
	gboolean disable_password_storing;
	RemminaFile *remminafile;

	remminafile = rm_plugin_service->protocol_plugin_get_file(gp);

	disable_password_storing = rm_plugin_service->file_get_int(
		remminafile, "disablepasswordstoring", FALSE
	);

	ret = rm_plugin_service->protocol_plugin_init_auth(
			gp, (disable_password_storing ? 0 :
			     REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD |
			     REMMINA_MESSAGE_PANEL_FLAG_USERNAME),
			_("Enter X2Go credentials"),
			(*default_username), (*default_password), NULL, NULL
	);

	if (ret == GTK_RESPONSE_OK) {
		s_username = rm_plugin_service->protocol_plugin_init_get_username(gp);
		s_password = rm_plugin_service->protocol_plugin_init_get_password(gp);
		if (rm_plugin_service->protocol_plugin_init_get_savepassword(gp))
			rm_plugin_service->file_set_string(
				remminafile, "password", s_password
			);

		// Should be renamed to protocol_plugin_init_get_savecredentials()?!
		save = rm_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save) {
			if (!rmplugin_x2go_save_credentials(remminafile, s_username,
							    s_password, (*errmsg))) {
				return FALSE;
			}
		}
		if (s_username) {
			(*default_username) = g_strdup(s_username);
			g_free(s_username);
		}
		if (s_password) {
			(*default_password) = g_strdup(s_password);
			g_free(s_password);
		}
	} else  {
		(*errmsg) = g_strdup("Authentication cancelled. Aborting…");
		return FALSE;
	}

	return TRUE;
}

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
		g_set_error(error, 1, 1, "%s", g_strdup_printf(
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
		g_set_error(error, 1, 1, "%s", FEATURE_NOT_AVAIL_STR("USERNAME"));
		REMMINA_PLUGIN_CRITICAL("%s", FEATURE_NOT_AVAIL_STR("USERNAME"));
		return NULL;
	}

	if (FEATURE_AVAILABLE(gpdata, "NON_INTERACTIVE")) {
		argv[argc++] = g_strdup("--non-interactive");
	} else {
		REMMINA_PLUGIN_WARNING("%s", FEATURE_NOT_AVAIL_STR("NON_INTERACTIVE"));
	}

	if (password && FEATURE_AVAILABLE(gpdata, "PASSWORD")) {
		if (FEATURE_AVAILABLE(gpdata, "AUTH_ATTEMPTS")) {
			argv[argc++] = g_strdup("--auth-attempts");
			argv[argc++] = g_strdup_printf ("%i", 0);
		} else {
			REMMINA_PLUGIN_WARNING("%s", FEATURE_NOT_AVAIL_STR("AUTH_ATTEMPTS"));
		}
		if (strlen(password) > 0) {
			argv[argc++] = g_strdup("--force-password");
			argv[argc++] = g_strdup("--password");
			argv[argc++] = g_strdup_printf("%s", password);
		}
	} else if (!password) {
		g_set_error(error, 1, 1, "%s", FEATURE_NOT_AVAIL_STR("PASSWORD"));
		REMMINA_PLUGIN_CRITICAL("%s", FEATURE_NOT_AVAIL_STR("PASSWORD"));
		return NULL;
	}

	// No need to catch feature-not-available error.
	// `--quiet` is not that important.
	if (FEATURE_AVAILABLE(gpdata, "QUIET")) {
		argv[argc++] = g_strdup("--quiet");
	}

	argv[argc++] = NULL;

	//#ifndef GLIB_AVAILABLE_IN_2_68
		gchar** envp = g_get_environ();
		gchar* envp_splitted = g_strjoinv(";", envp);
		envp_splitted = g_strconcat(envp_splitted, ";LANG=C", (void*) NULL);
		envp = g_strsplit(envp_splitted, ";", 0);
	/*
	* #else
	*	// Only available after glib version 2.68.
	*	// TODO: FIXME: NOT TESTED!
	*	GStrvBuilder* builder = g_strv_builder_new();
	*	g_strv_builder_add(builder, "LANG=C");
	*	GStrv envp = g_strv_builder_end(builder);
	* #endif
	*/

	gchar* std_out = rmplugin_x2go_spawn_pyhoca_process(argc, argv, error, envp);
	g_strfreev(envp);

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
		g_set_error(error, 1, 1, "%s", _("Couldn't parse the output of PyHoca-CLI's "
				           "--list-sessions option. Creating a new "
				           "session now."));
		return NULL;
	}

	gboolean found_session = FALSE;
	GList* sessions = NULL;
	gchar** session = NULL;

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

			session = malloc(sizeof(gchar*) * (SESSION_NUM_PROPERTIES+1));
			if (!session) {
				REMMINA_PLUGIN_CRITICAL("%s", _("Couldn't allocate "
								"enough memory!"));
			}
			session[SESSION_NUM_PROPERTIES] = NULL;
			sessions = g_list_append(sessions, session);

			session_id = line_list[1];
			session[SESSION_SESSION_ID] = session_id;

			REMMINA_PLUGIN_INFO("%s", g_strdup_printf(
				_("Found already existing X2Go session with ID: '%s'"),
				session[SESSION_SESSION_ID])
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
			session[SESSION_COOKIE] = value;
		} else if (g_str_has_prefix(current_line, "agent PID: ")) {
			REMMINA_PLUGIN_DEBUG("agent PID:\t'%s'", value);
			session[SESSION_AGENT_PID] = value;
		} else if (g_str_has_prefix(current_line, "display: ")) {
			REMMINA_PLUGIN_DEBUG("display:\t'%s'", value);
			session[SESSION_DISPLAY] = value;
		} else if (g_str_has_prefix(current_line, "status: ")) {
			if (g_strcmp0(value, "S") == 0) {
				// TRANSLATORS: Please stick to X2GoClient's translation.
				value = _("Suspended");
			} else if (g_strcmp0(value, "R") == 0) {
				// TRANSLATORS: Please stick to X2GoClient's translation.
				value = _("Running");
			} else if (g_strcmp0(value, "T") == 0) {
				// TRANSLATORS: Please stick to X2GoClient's translation.
				value = _("Terminated");
			}
			REMMINA_PLUGIN_DEBUG("status:\t'%s'", value);
			session[SESSION_STATUS] = value;
		} else if (g_str_has_prefix(current_line, "graphic port: ")) {
			REMMINA_PLUGIN_DEBUG("graphic port:\t'%s'", value);
			session[SESSION_GRAPHIC_PORT] = value;
		} else if (g_str_has_prefix(current_line, "snd port: ")) {
			REMMINA_PLUGIN_DEBUG("snd port:\t'%s'", value);
			session[SESSION_SND_PORT] = value;
		} else if (g_str_has_prefix(current_line, "sshfs port: ")) {
			REMMINA_PLUGIN_DEBUG("sshfs port:\t'%s'", value);
			session[SESSION_SSHFS_PORT] = value;
		} else if (g_str_has_prefix(current_line, "username: ")) {
			REMMINA_PLUGIN_DEBUG("username:\t'%s'", value);
			session[SESSION_USERNAME] = value;
		} else if (g_str_has_prefix(current_line, "hostname: ")) {
			REMMINA_PLUGIN_DEBUG("hostname:\t'%s'", value);
			session[SESSION_HOSTNAME] = value;
		} else if (g_str_has_prefix(current_line, "create date: ")) {
			REMMINA_PLUGIN_DEBUG("create date:\t'%s'", value);
			session[SESSION_CREATE_DATE] = value;
		} else if (g_str_has_prefix(current_line, "suspended since: ")) {
			REMMINA_PLUGIN_DEBUG("suspended since:\t'%s'", value);
			session[SESSION_SUSPENDED_SINCE] = value;
		} else {
			REMMINA_PLUGIN_DEBUG("Not supported:\t'%s'", value);
			found_session = FALSE;
		}
	}

	if (!sessions) {
		g_set_error(error, 1, 1,
			"%s", _("Could not find any sessions on remote machine. Creating a new "
			  "session now.")
		);

		// returning NULL with `error` set.
	}

	return sessions;
}

/**
 * @brief Asks the user, with the help of a dialog, to continue an already existing
 *	  session, terminate or create a new one.
 *
 * @param error Is set if there is something to tell the user. \n
 *		Not necessarily an *error* message.
 * @param connect_data Stores all necessary information needed for
 *		       etrieving sessions from a X2Go server.
 * @return gchar* ID of session. Can be 'NULL' but then 'error' is set.
 */
static gchar* rmplugin_x2go_ask_session(RemminaProtocolWidget *gp, GError **error,
					struct _ConnectionData* connect_data)
{
	if (!connect_data ||
	    !connect_data->host ||
	    !connect_data->username ||
	    !connect_data->password ||
	    strlen(connect_data->host) <= 0 ||
	    strlen(connect_data->username) <= 0)
	    // Allow empty passwords. Maybe the user wants to connect via public key?
	{
		g_set_error(error, 1, 1, "%s", g_strdup_printf(
			_("Internal error: %s"),
			_("'Invalid connection data.'")
		));
		return NULL;
	}

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
	ddata->parent = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gp)));
	ddata->flags = GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT;
	//ddata->type = GTK_MESSAGE_QUESTION;
	//ddata->buttons = GTK_BUTTONS_OK; // Doesn't get used in our custom factory.
	ddata->title = _("Choose a session to resume:");
	ddata->message = "";

	// gboolean factory(X2GoCustomUserData*, gpointer)
	// 	X2GoCustomUserData*:
	//		gp -> gp (RemminaProtocolWidget*)
	//		dialog_data -> dialog data (struct _DialogData*)
	//		connect_data -> connection data (struct _ConnectionData*)
	//	gpointer: dialog_factory_data
	ddata->callbackfunc = G_CALLBACK(rmplugin_x2go_session_chooser_callback);

	// gboolean factory(X2GoCustomUserData*, gpointer)
	// 	X2GoCustomUserData*:
	//		gp -> gp (RemminaProtocolWidget*)
	//		dialog_data -> dialog data (struct _DialogData*)
	//		connect_data -> connection data (struct _ConnectionData*)
	//	gpointer: dialog_factory_data
	ddata->dialog_factory_data = sessions_list;
	ddata->dialog_factory_func = G_CALLBACK(rmplugin_x2go_choose_session_dialog_factory);

	/* Prepare X2GoCustomUserData *custom_data
	 *	gp -> gp (RemminaProtocolWidget*)
	 *	dialog_data -> dialog data (struct _DialogData*)
	 */
	X2GoCustomUserData *custom_data = g_new0(X2GoCustomUserData, 1);
	g_assert(custom_data && "custom_data could not be initialized.");

	custom_data->gp = gp;
	custom_data->dialog_data = ddata;
	custom_data->connect_data = connect_data;
	custom_data->opt1 = NULL;

	// Open dialog here. Dialog rmplugin_x2go_session_chooser_callback (callbackfunc)
	// should set SET_RESUME_SESSION.
	IDLE_ADD((GSourceFunc)rmplugin_x2go_open_dialog, custom_data);

	guint counter = 0;
	while (!IS_SESSION_SELECTED(gp)) {
		// 0.5 Seconds. Give dialog chance to open.
		usleep(500 * 1000);

		// Every 5 seconds
		if (counter % 10 == 0 || counter == 0) {
			REMMINA_PLUGIN_INFO("%s", _("Waiting for user to select a session…"));
		}
		counter++;
	}

	gchar* chosen_resume_session = GET_RESUME_SESSION(gp);

	if (!chosen_resume_session || strlen(chosen_resume_session) <= 0) {
		g_set_error(error, 1, 1, "%s", _("No session was selected. Creating a new one."));
		return NULL;
	}

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
	if (!rmplugin_x2go_get_auth(gp, &errmsg, &username, &password)) {
		return FALSE;
	}

	struct _ConnectionData* connect_data = g_new0(struct _ConnectionData, 1);
	connect_data->host = host;
	connect_data->username = username;
	connect_data->password = password;

	GError *session_error = NULL;
	gchar* resume_session_id = rmplugin_x2go_ask_session(gp, &session_error,
							     connect_data);

	if (!resume_session_id || session_error || strlen(resume_session_id) <= 0) {
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
			resume_session_id
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

	if (resume_session_id && strlen(resume_session_id) > 0) {
		REMMINA_PLUGIN_INFO("%s", g_strdup_printf(
			// TRANSLATORS: Please stick to X2GoClient's way of translating.
			_("Resuming session '%s'…"),
			resume_session_id
		));

		if (FEATURE_AVAILABLE(gpdata, "RESUME")) {
			argv[argc++] = g_strdup("--resume");
			argv[argc++] = g_strdup_printf("%s", resume_session_id);
		} else {
			REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("RESUME"));
		}
	}

	// Deprecated. The user either wants to continue a
	// session or just not. No inbetween.
	// if (!resume_session_id) {
	// 	if (FEATURE_AVAILABLE(gpdata, "TRY_RESUME")) {
	// 		argv[argc++] = g_strdup("--try-resume");
	// 	} else {
	// 		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("TRY_RESUME"));
	// 	}
	// }

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
		if (strlen(password) > 0) {
			argv[argc++] = g_strdup("--force-password");
			argv[argc++] = g_strdup("--password");
			argv[argc++] = g_strdup_printf ("%s", password);
		}
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("PASSWORD"));
	}

	if (FEATURE_AVAILABLE(gpdata, "AUTH_ATTEMPTS")) {
		argv[argc++] = g_strdup("--auth-attempts");
		argv[argc++] = g_strdup_printf ("%i", 0);
	} else {
		REMMINA_PLUGIN_DEBUG("%s", FEATURE_NOT_AVAIL_STR("AUTH_ATTEMPTS"));
	}

	if (FEATURE_AVAILABLE(gpdata, "NON_INTERACTIVE")) {
		argv[argc++] = g_strdup("--non-interactive");
	} else {
		REMMINA_PLUGIN_WARNING("%s", FEATURE_NOT_AVAIL_STR("NON_INTERACTIVE"));
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

	REMMINA_PLUGIN_INFO("%s", _("Started PyHoca-CLI with the following arguments:"));
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
		ddata->parent = GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gp)));
		ddata->flags = GTK_DIALOG_MODAL;
		ddata->type = GTK_MESSAGE_ERROR;
		ddata->buttons = GTK_BUTTONS_OK;
		ddata->title = _("Could not start X2Go session.");
		ddata->message = g_strdup_printf(_("Could not start PyHoca-CLI (%i): '%s'"),
						 error->code,
						 error->message);
		// We don't need the response.
		ddata->callbackfunc = NULL;
		// We don't need a custom dialog either.
		ddata->dialog_factory_func = NULL;
		ddata->dialog_factory_data = NULL;

		/* Prepare X2GoCustomUserData *custom_data
		*	gp -> gp (RemminaProtocolWidget*)
		*	dialog_data -> dialog data (struct _DialogData*)
		*/
		X2GoCustomUserData *custom_data = g_new0(X2GoCustomUserData, 1);
		g_assert(custom_data && "custom_data could not be initialized.");

		custom_data->gp = gp;
		custom_data->dialog_data = ddata;
		custom_data->connect_data = NULL;
		custom_data->opt1 = NULL;

		IDLE_ADD((GSourceFunc) rmplugin_x2go_open_dialog, custom_data);

		g_strlcpy(errmsg, error_title, 512);

		// No need to output here. rmplugin_x2go_start_session will do this.

		g_error_free(error);

		return FALSE;
	}

	// Prevent a race condition where pyhoca-cli is not
	// started yet (pidx2go == 0) but a watcher is added.

	struct timespec ts;
	// 0.001 seconds.
	ts.tv_nsec = 1 * 1000 * 1000;
	ts.tv_sec = 0;
	while (gpdata->pidx2go == 0) {
		nanosleep(&ts, NULL);
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
	gchar** envp = g_get_environ();
	gchar* features_string = rmplugin_x2go_spawn_pyhoca_process(argc, argv,
								    &error, envp);
	g_strfreev(envp);

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

	gchar *server;
	gint port;

	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	REMMINA_PLUGIN_DEBUG("Socket %d", gpdata->socket_id);
	rm_plugin_service->protocol_plugin_signal_connection_opened(gp);

	RemminaFile *remminafile = rm_plugin_service->protocol_plugin_get_file(gp);
	rm_plugin_service->get_server_port(rm_plugin_service->file_get_string(remminafile, "server"),
			22,
			&server,
			&port);

	REMMINA_PLUGIN_AUDIT(_("Connected to %s:%d via X2Go"), server, port);
	g_free(server), server = NULL;

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
		g_set_error(&error, 1, 1, "%s", error_msg);
		return error;
	}

	gchar **elements_list = g_strsplit(data, ",", 0);

	guint elements_amount = 0;
	elements_amount = g_strv_length(elements_list);

	if (elements_list == NULL ||
	    elements_list[0] == NULL ||
	    strlen(elements_list[0]) <= 0)
	{
		gchar *error_msg = _("Validation data in ProtocolSettings array is invalid!");
		REMMINA_PLUGIN_CRITICAL("%s", error_msg);
		g_set_error(&error, 1, 1, "%s", error_msg);
		return error;
	}

	gchar *data_str = "";

	if (!key || !value) {
		REMMINA_PLUGIN_CRITICAL("%s", _("Parameters 'key' or 'value' are 'NULL'!"));
		g_set_error(&error, 1, 1, "%s", _("Internal error."));
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
		g_set_error(&error, 1, 1, "%s", error_msg);
		return error;
	}

	gint minimum;
	str2int_errno err = str2int(&minimum, integer_list[0], 10);
	if (err == STR2INT_INCONVERTIBLE) {
		g_set_error(&error, 1, 1, "%s", g_strdup_printf(
			_("Internal error: %s"),
			_("The lower limit is not a valid integer!")
		));
	} else if (err == STR2INT_OVERFLOW) {
		g_set_error(&error, 1, 1, "%s", g_strdup_printf(
			_("Internal error: %s"),
			_("The lower limit is too high!")
		));
	} else if (err == STR2INT_UNDERFLOW) {
		g_set_error(&error, 1, 1, "%s", g_strdup_printf(
			_("Internal error: %s"),
			_("The lower limit is too low!")
		));
	} else if (err == STR2INT_INVALID_DATA) {
		g_set_error(&error, 1, 1, "%s", g_strdup_printf(
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
		g_set_error(&error, 1, 1, "%s", g_strdup_printf(
			_("Internal error: %s"),
			_("The upper limit is not a valid integer!")
		));
	} else if (err == STR2INT_OVERFLOW) {
		g_set_error(&error, 1, 1, "%s", g_strdup_printf(
			_("Internal error: %s"),
			_("The upper limit is too high!")
		));
	} else if (err == STR2INT_UNDERFLOW) {
		g_set_error(&error, 1, 1, "%s", g_strdup_printf(
			_("Internal error: %s"),
			_("The upper limit is too low!")
		));
	} else if (err == STR2INT_INVALID_DATA) {
		g_set_error(&error, 1, 1, "%s", g_strdup_printf(
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
		g_set_error(&error, 1, 1, "%s", _("The input is not a valid integer!"));
	} else if (err == STR2INT_OVERFLOW || err == STR2INT_UNDERFLOW) {
		g_set_error(&error, 1, 1, _("Input must be a number between %i and %i."),
					minimum, maximum);
	} else if (err == STR2INT_INVALID_DATA) {
		g_set_error(&error, 1, 1, "%s", _("Something unknown went wrong."));
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
