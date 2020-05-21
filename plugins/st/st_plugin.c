/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2017-2020 Antenore Gatta, Giovanni Panozzo
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

#include "st_plugin_config.h"

#include "common/remmina_plugin.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <glib.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

#define REMMINA_PLUGIN_ST_FEATURE_GTKSOCKET 1

typedef struct _RemminaPluginData
{
	GtkWidget *socket;
	gint socket_id;
	GPid pid;
} RemminaPluginData;

static RemminaPluginService *remmina_plugin_service = NULL;
#define REMMINA_PLUGIN_DEBUG(fmt, ...) remmina_plugin_service->_remmina_debug(__func__, fmt, ##__VA_ARGS__)

static gboolean remmina_st_query_feature(RemminaProtocolWidget* gp, const RemminaProtocolFeature* feature)
{
	TRACE_CALL(__func__);
	return TRUE;
}


static void remmina_plugin_st_on_plug_added(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginData *gpdata;
	gpdata = (RemminaPluginData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	REMMINA_PLUGIN_DEBUG("[%s] Plugin plug added on socket %d", PLUGIN_NAME, gpdata->socket_id);
	remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);
	return;
}

static void remmina_plugin_st_on_plug_removed(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("[%s] Plugin plug removed", PLUGIN_NAME);
	remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
}

static void remmina_plugin_st_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("[%s] Plugin init", PLUGIN_NAME);
	RemminaPluginData *gpdata;

	gpdata = g_new0(RemminaPluginData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	gpdata->socket = gtk_socket_new();
	remmina_plugin_service->protocol_plugin_register_hostkey(gp, gpdata->socket);
	gtk_widget_show(gpdata->socket);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-added", G_CALLBACK(remmina_plugin_st_on_plug_added), gp);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-removed", G_CALLBACK(remmina_plugin_st_on_plug_removed), gp);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->socket);
}

static gboolean remmina_plugin_st_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("[%s] Plugin open connection", PLUGIN_NAME);
#define ADD_ARGUMENT(name, value) \
	{ \
		argv[argc] = g_strdup(name); \
		argv_debug[argc] = g_strdup(name); \
		argc++; \
		if (value != NULL) \
		{ \
			argv[argc] = value; \
			argv_debug[argc++] = g_strdup(g_strcmp0(name, "-p") != 0 ? value : "XXXXX"); \
		} \
	}
	RemminaPluginData *gpdata;
	RemminaFile *remminafile;
	GError *error = NULL;
	gchar *argv[50];       // Contains all the arguments
	gchar *argv_debug[50]; // Contains all the arguments
	gchar *command_line;   // The whole command line obtained from argv_debug
	const gchar *term;     // Terminal emulator name from remimna profile.
	const gchar *wflag = NULL;
	const gchar *command;  // The command to be passed to the terminal (if any)
	gboolean isterm = FALSE;
	gint argc;
	gint i;

	gpdata = (RemminaPluginData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (!remmina_plugin_service->file_get_int(remminafile, "detached", FALSE)) {
		remmina_plugin_service->protocol_plugin_set_width(gp, 640);
		remmina_plugin_service->protocol_plugin_set_height(gp, 480);
		gtk_widget_set_size_request(GTK_WIDGET(gp), 640, 480);
		gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));
	}

	term = remmina_plugin_service->file_get_string(remminafile, "terminal");

	if (g_strcmp0(term, "st") == 0) {
		/* on Debian based distros st is packaged as stterm */
		if (!g_find_program_in_path(term))
			term = "stterm";
		wflag = "-w";
		isterm = TRUE;
	}else if (g_strcmp0(term, "urxvt") == 0) {
		wflag = "-embed";
		isterm = TRUE;
	}else if (g_strcmp0(term, "xterm") == 0) {
		wflag = "-xrm 'XTerm*allowSendEvents: true' -into";
		isterm = TRUE;
	}else if (g_strcmp0(term, "vim") == 0) {
		wflag = "-g --socketid";
		isterm = FALSE;
	}else if (g_strcmp0(term, "emacs") == 0) {
		wflag = "--parent-id";
		isterm = FALSE;
	}
	if (!g_find_program_in_path(term)) {
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s not found", term);
		return FALSE;
	}

	argc = 0;
	// Main executable name
	ADD_ARGUMENT(g_strdup_printf("%s", term), NULL);
	// Embed st-window in another window
	if (gpdata->socket_id != 0)
		ADD_ARGUMENT(g_strdup(wflag), g_strdup_printf("%i", gpdata->socket_id));
	// Add eventually any additional arguments set by the user
	command = remmina_plugin_service->file_get_string(remminafile, "cmd");
	if(command && isterm)
		ADD_ARGUMENT("-e", g_strdup_printf("%s", command));
	if(command && !isterm)
		ADD_ARGUMENT("", g_strdup_printf("%s", command));
	// End of the arguments list
	ADD_ARGUMENT(NULL, NULL);
	// Retrieve the whole command line
	command_line = g_strjoinv(g_strdup(" "), (gchar **)&argv_debug[0]);
	REMMINA_PLUGIN_DEBUG("[%s] starting %s", PLUGIN_NAME, command_line);
	// Execute the external process st
	g_spawn_command_line_async(command_line, &error);
	g_free(command_line);

	// Free the arguments list
	for (i = 0; i < argc; i++)
	{
		g_free(argv_debug[i]);
		g_free(argv[i]);
	}
	// Show error message
	if (error) {
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s", error->message);
		g_error_free(error);
		return FALSE;
	}
	// Show attached window socket ID
	if (!remmina_plugin_service->file_get_int(remminafile, "detached", FALSE)) {
		REMMINA_PLUGIN_DEBUG("[%s] attached window to socket %d",
				PLUGIN_NAME, gpdata->socket_id);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static gboolean remmina_plugin_st_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("[%s] Plugin close connection", PLUGIN_NAME);
	remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
	return FALSE;
}

static gpointer term_list[] =
{
	"st", "Suckless Simple Terminal",
	"urxvt", "rxvt-unicode",
	"xterm", "Xterm",
	"emacs", "GNU Emacs",
	"vim", "Vim Text Editor",
	NULL
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
static const RemminaProtocolSetting remmina_plugin_st_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "terminal", N_("Terminal Emulator"), FALSE, term_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "cmd", N_("Command to be executed"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Array of RemminaProtocolSetting for advanced settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_st_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "detached", N_("Detached window"), TRUE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_st_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_GTKSOCKET, REMMINA_PLUGIN_ST_FEATURE_GTKSOCKET,	    NULL,	   NULL,	NULL},
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END,   0,					    NULL,	   NULL,	NULL}
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                 // Type
	PLUGIN_NAME,                                  // Name
	PLUGIN_DESCRIPTION,                           // Description
	GETTEXT_PACKAGE,                              // Translation domain
	PLUGIN_VERSION,                               // Version number
	PLUGIN_APPICON,                               // Icon for normal connection
	PLUGIN_APPICON,                               // Icon for SSH connection
	remmina_plugin_st_basic_settings,             // Array for basic settings
	remmina_plugin_st_advanced_settings,          // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_NONE,            // SSH settings type
	remmina_st_features,                          // Array for available features
	remmina_plugin_st_init,                       // Plugin initialization
	remmina_plugin_st_open_connection,            // Plugin open connection
	remmina_plugin_st_close_connection,           // Plugin close connection
	remmina_st_query_feature,                     // Query for available features
	NULL,                                         // Call a feature
	NULL,                                         // Send a keystroke
	NULL                                          // Capture screenshot
};

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");


	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin))
	{
		return FALSE;
	}
	return TRUE;
}
