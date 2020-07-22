/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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

#include "config.h"
#include "buildflags.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "remmina.h"
#include "remmina_main.h"
#include "remmina_log.h"
#include "remmina_pref.h"
#include "remmina_widget_pool.h"
#include "remmina_unlock.h"
#include "remmina_pref_dialog.h"
#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_file_editor.h"
#include "rcw.h"
#include "remmina_about.h"
#include "remmina_plugin_manager.h"
#include "remmina_exec.h"
#include "remmina_icon.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_file_manager.h"
#include "remmina_crypt.h"

#ifdef SNAP_BUILD
#   define ISSNAP "- SNAP Build -"
#else
#   define ISSNAP "-"
#endif

static gboolean cb_closewidget(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	/* The correct way to close a rcw is to send
	 * it a "delete-event" signal. Simply destroying it will not close
	 * all network connections */
	if (REMMINA_IS_CONNECTION_WINDOW(widget))
		return rcw_delete(RCW(widget));
	return TRUE;
}

const gchar* remmina_exec_get_build_config(void)
{
	static const gchar build_config[] =
	    "Build configuration: " BUILD_CONFIG "\n"
	    "Build type:          " BUILD_TYPE "\n"
	    "CFLAGS:              " CFLAGS "\n"
	    "Compiler:            " COMPILER_ID ", " COMPILER_VERSION "\n"
	    "Target architecture: " TARGET_ARCH "\n";
	return build_config;
}

void remmina_exec_exitremmina()
{
	TRACE_CALL(__func__);

	/* Save main window state/position */
	remmina_main_save_before_destroy();

	/* Delete all widgets, main window not included */
	remmina_widget_pool_foreach(cb_closewidget, NULL);

	/* Remove systray menu */
	remmina_icon_destroy();

	/* close/destroy main window struct and window */
	remmina_main_destroy();

	/* Exit from Remmina */
	g_application_quit(g_application_get_default());
}

static gboolean disable_rcw_delete_confirm_cb(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *rcw;

	if (REMMINA_IS_CONNECTION_WINDOW(widget)) {
		rcw = (RemminaConnectionWindow*)widget;
		rcw_set_delete_confirm_mode(rcw, RCW_ONDELETE_NOCONFIRM);
	}
	return TRUE;
}

void remmina_application_condexit(RemminaCondExitType why)
{
	TRACE_CALL(__func__);

	/* Exit remmina only if there are no interesting windows left:
	 * no main window, no systray menu, no connection window.
	 * This function is usually called after a disconnection */

	switch (why) {
	case REMMINA_CONDEXIT_ONDISCONNECT:
		// A connection has disconnected, should we exit remmina ?
		if (remmina_widget_pool_count() < 1 && !remmina_main_get_window() && !remmina_icon_is_available())
			remmina_exec_exitremmina();
		break;
	case REMMINA_CONDEXIT_ONMAINWINDELETE:
		/* If we are in Kiosk mode, we just exit */
		if (kioskmode && kioskmode == TRUE)
			remmina_exec_exitremmina();
		// Main window has been deleted
		if (remmina_widget_pool_count() < 1 && !remmina_icon_is_available())
			remmina_exec_exitremmina();
		break;
	case REMMINA_CONDEXIT_ONQUIT:
		// Quit command has been sent from main window or appindicator/systray menu
		// quit means QUIT.
		remmina_widget_pool_foreach(disable_rcw_delete_confirm_cb, NULL);
		remmina_exec_exitremmina();
		break;
	}
}


static void newline_remove(char *s)
{
	char c;
	while((c = *s) != 0 && c != '\r' && c != '\n')
		s++;
	*s = 0;
}

/* used for commandline parameter --update-profile X --set-option Y --set-option Z
 * return a status code for exit()
 */
int remmina_exec_set_setting(gchar *profilefilename, gchar **settings)
{
	RemminaFile *remminafile;
	int i;
	gchar **tk, *value;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	gboolean abort = FALSE;

	remminafile = remmina_file_manager_load_file(profilefilename);

	if (!remminafile) {
		g_print("Unable to open profile file %s\n", profilefilename);
		return 2;
	}

	for(i = 0; settings[i] != NULL && !abort; i++) {
		if (strlen(settings[i]) > 0) {
			tk = g_strsplit(settings[i], "=", 2);
			if (tk[1] == NULL) {
				read = getline(&line, &len, stdin);
				if (read > 0) {
					newline_remove(line);
					value = line;
				} else {
					g_print("Error: an extra line of standard input is needed\n");
					abort = TRUE;
				}
			} else
				value = tk[1];
			remmina_file_set_string(remminafile, tk[0], value);
			g_strfreev(tk);
		}
	}

	if (line) free(line);

	if (!abort) remmina_file_save(remminafile);

	return 0;

}

static void remmina_exec_autostart_cb(RemminaFile *remminafile, gpointer user_data)
{
	TRACE_CALL(__func__);

	if (remmina_file_get_int(remminafile, "enable-autostart", FALSE)) {
		REMMINA_DEBUG ("Profile %s is set to autostart", remminafile->filename);
		rcw_open_from_filename(remminafile->filename);
	}

}

static void remmina_exec_connect(const gchar *data)
{
	TRACE_CALL(__func__);

	gchar *protocol;
	gchar **protocolserver;
	gchar *server;
	RemminaFile *remminafile;
	gchar **userat;
	gchar **userpass;
	gchar *user;
	gchar *password;
	gchar **domainuser;
	gchar **serverquery;
	gchar **querystring;
	gchar **querystringpart;
	gchar **querystringpartkv;
	gchar *value;
	gchar *temp;

	protocol = NULL;
	if (strncmp("rdp://", data, 6) == 0 || strncmp("RDP://", data, 6) == 0)
		protocol = "RDP";
	else if (strncmp("vnc://", data, 6) == 0 || strncmp("VNC://", data, 6) == 0)
		protocol = "VNC";
	else if (strncmp("ssh://", data, 6) == 0 || strncmp("SSH://", data, 6) == 0)
		protocol = "SSH";
	else if (strncmp("spice://", data, 8) == 0 || strncmp("SPICE://", data, 8) == 0)
		protocol = "SPICE";

	if (protocol == NULL) {
		rcw_open_from_filename(data);
		return;
	}

	protocolserver = g_strsplit(data, "://", 2);
	server = g_strdup(protocolserver[1]);

	// Support loading .remmina files using handler
	if ((temp = strrchr(server, '.')) != NULL && g_strcmp0(temp + 1, "remmina") == 0) {
		g_strfreev(protocolserver);
		temp = g_uri_unescape_string(server, NULL);
		g_free(server);
		server = temp;
		rcw_open_from_filename(server);
		return;
	}

	remminafile = remmina_file_new();

	// Check for username@server
	if ((strcmp(protocol, "RDP") == 0 || strcmp(protocol, "VNC") == 0 || strcmp(protocol, "SSH") == 0) && strstr(server, "@") != NULL) {
		userat = g_strsplit(server, "@", 2);

		// Check for username:password
		if (strstr(userat[0], ":") != NULL) {
			userpass = g_strsplit(userat[0], ":", 2);
			user = g_uri_unescape_string(userpass[0], NULL);
			password = g_uri_unescape_string(userpass[1], NULL);

			// Try to decrypt the password field if it contains =
			temp = password != NULL && strrchr(password, '=') != NULL ? remmina_crypt_decrypt(password) : NULL;
			if (temp != NULL) {
				g_free(password);
				password = temp;
			}
			remmina_file_set_string(remminafile, "password", password);
			g_free(password);
			g_strfreev(userpass);
		} else {
			user = g_uri_unescape_string(userat[0], NULL);
		}

		// Check for domain\user for RDP connections
		if (strcmp(protocol, "RDP") == 0 && strstr(user, "\\") != NULL) {
			domainuser = g_strsplit(user, "\\", 2);
			remmina_file_set_string(remminafile, "domain", domainuser[0]);
			g_free(user);
			user = g_strdup(domainuser[1]);
		}

		remmina_file_set_string(remminafile, "username", user);
		g_free(user);
		g_free(server);
		server = g_strdup(userat[1]);
		g_strfreev(userat);
	}

	if (strcmp(protocol, "VNC") == 0 && strstr(server, "?") != NULL) {
		// https://tools.ietf.org/html/rfc7869
		// VncUsername, VncPassword and ColorLevel supported for vnc-params

		// Check for query string parameters
		serverquery = g_strsplit(server, "?", 2);
		querystring = g_strsplit(serverquery[1], "&", -1);
		for (querystringpart = querystring; *querystringpart; querystringpart++) {
			if (strstr(*querystringpart, "=") == NULL)
				continue;
			querystringpartkv = g_strsplit(*querystringpart, "=", 2);
			value = g_uri_unescape_string(querystringpartkv[1], NULL);
			if (strcmp(querystringpartkv[0], "VncPassword") == 0) {
				// Try to decrypt password field if it contains =
				temp = value != NULL && strrchr(value, '=') != NULL ? remmina_crypt_decrypt(value) : NULL;
				if (temp != NULL) {
					g_free(value);
					value = temp;
				}
				remmina_file_set_string(remminafile, "password", value);
			} else if (strcmp(querystringpartkv[0], "VncUsername") == 0) {
				remmina_file_set_string(remminafile, "username", value);
			} else if (strcmp(querystringpartkv[0], "ColorLevel") == 0) {
				remmina_file_set_string(remminafile, "colordepth", value);
			}
			g_free(value);
			g_strfreev(querystringpartkv);
		}
		g_strfreev(querystring);
		g_free(server);
		server = g_strdup(serverquery[0]);
		g_strfreev(serverquery);
	}

	// Unescape server
	temp = g_uri_unescape_string(server, NULL);
	g_free(server);
	server = temp;

	remmina_file_set_string(remminafile, "server", server);
	remmina_file_set_string(remminafile, "name", server);
	remmina_file_set_string(remminafile, "sound", "off");
	remmina_file_set_string(remminafile, "protocol", protocol);
	g_free(server);
	g_strfreev(protocolserver);
	rcw_open_from_file(remminafile);
}

void remmina_exec_command(RemminaCommandType command, const gchar* data)
{
	TRACE_CALL(__func__);
	gchar *s1;
	gchar *s2;
	gchar *temp;
	GtkWidget *widget;
	GtkWindow *mainwindow;
	GtkDialog *prefdialog;
	RemminaEntryPlugin *plugin;
    int i;
    int ch;
	mainwindow = remmina_main_get_window();

	switch (command) {
	case REMMINA_COMMAND_AUTOSTART:
		remmina_file_manager_iterate((GFunc)remmina_exec_autostart_cb, NULL);
		break;

	case REMMINA_COMMAND_MAIN:
		if (mainwindow) {
			gtk_window_present(mainwindow);
			gtk_window_deiconify(GTK_WINDOW(mainwindow));
		}else  {
			widget = remmina_main_new();
			gtk_widget_show(widget);
		}
		break;

	case REMMINA_COMMAND_PREF:
		if (remmina_unlock_new(mainwindow) == 0)
			break;
		prefdialog = remmina_pref_dialog_get_dialog();
		if (prefdialog) {
			gtk_window_present(GTK_WINDOW(prefdialog));
			gtk_window_deiconify(GTK_WINDOW(prefdialog));
		}else  {
			/* Create a new preference dialog */
			widget = GTK_WIDGET(remmina_pref_dialog_new(atoi(data), NULL));
			gtk_widget_show(widget);
		}
		break;

	case REMMINA_COMMAND_NEW:
		s1 = (data ? strchr(data, ',') : NULL);
		if (s1) {
			s1 = g_strdup(data);
			s2 = strchr(s1, ',');
			*s2++ = '\0';
			widget = remmina_file_editor_new_full(s2, s1);
			g_free(s1);
		}else  {
			widget = remmina_file_editor_new_full(NULL, data);
		}
		gtk_widget_show(widget);
		break;

	case REMMINA_COMMAND_CONNECT:
		/** @todo This should be a G_OPTION_ARG_FILENAME_ARRAY (^aay) so that
		 * we can implement multi profile connection:
		 *    https://gitlab.com/Remmina/Remmina/issues/915
		 */
		remmina_exec_connect(data);
		break;

	case REMMINA_COMMAND_EDIT:
		widget = remmina_file_editor_new_from_filename(data);
		if (widget)
			gtk_widget_show(widget);
		break;

	case REMMINA_COMMAND_ABOUT:
		remmina_about_open(NULL);
		break;

	case REMMINA_COMMAND_VERSION:
		mainwindow = remmina_main_get_window();
		if (mainwindow) {
			remmina_about_open(NULL);
		}else  {
			g_print("%s %s %s (git %s)\n", g_get_application_name(), ISSNAP, VERSION, REMMINA_GIT_REVISION);
			/* As we do not use the "handle-local-options" signal, we have to exit Remmina */
			remmina_exec_command(REMMINA_COMMAND_EXIT, NULL);
		}

		break;

	case REMMINA_COMMAND_FULL_VERSION:
		mainwindow = remmina_main_get_window();
		if (mainwindow) {
			/* Show th widget with the list of plugins and versions */
			remmina_plugin_manager_show(mainwindow);
		}else  {
			g_print("\n%s %s %s (git %s)\n\n", g_get_application_name(), ISSNAP, VERSION, REMMINA_GIT_REVISION);

			remmina_plugin_manager_show_stdout();
			g_print("\n%s\n", remmina_exec_get_build_config());
			remmina_exec_command(REMMINA_COMMAND_EXIT, NULL);
		}

		break;


	case REMMINA_COMMAND_PLUGIN:
		plugin = (RemminaEntryPlugin*)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_ENTRY, data);
		if (plugin) {
			plugin->entry_func();
		}else  {
			widget = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				_("Plugin %s is not registered."), data);
			g_signal_connect(G_OBJECT(widget), "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show(widget);
			remmina_widget_pool_register(widget);
		}
		break;

	case REMMINA_COMMAND_ENCRYPT_PASSWORD:
		i = 0;
		g_print("Enter the password you want to encrypt: ");
		temp = (char *)g_malloc(255 * sizeof(char));
		while ((ch = getchar()) != EOF && ch != '\n') {
			if (i < 254) {
				temp[i] = ch;
				i++;
			}
		}
		temp[i] = '\0';
		s1 = remmina_crypt_encrypt(temp);
		s2 = g_uri_escape_string(s1, NULL, TRUE);
		g_print("\nEncrypted password: %s\n\n", s1);
		g_print("Usage:\n");
		g_print("rdp://username:%s@server\n", s1);
		g_print("vnc://username:%s@server\n", s1);
		g_print("vnc://server?VncUsername=user\\&VncPassword=%s\n", s2);
		g_free(s1);
		g_free(s2);
		g_free(temp);
		remmina_exec_exitremmina();
		break;

	case REMMINA_COMMAND_EXIT:
		remmina_widget_pool_foreach(disable_rcw_delete_confirm_cb, NULL);
		remmina_exec_exitremmina();
		break;

	default:
		break;
	}
}
