/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
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

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "remmina_main.h"
#include "remmina_widget_pool.h"
#include "remmina_pref_dialog.h"
#include "remmina_file.h"
#include "remmina_pref.h"
#include "remmina_file_editor.h"
#include "remmina_connection_window.h"
#include "remmina_about.h"
#include "remmina_plugin_manager.h"
#include "remmina_exec.h"
#include "remmina_icon.h"
#include "remmina/remmina_trace_calls.h"


static gboolean cb_closewidget(GtkWidget *widget, gpointer data)
{
	TRACE_CALL("cb_closewidget");
	/* The correct way to close a remmina_connection_window is to send
	 * it a "delete-event" signal. Simply destroying it will not close
	 * all network connections */
	if (REMMINA_IS_CONNECTION_WINDOW(widget))
		return remmina_connection_window_delete(REMMINA_CONNECTION_WINDOW(widget));
	return TRUE;
}

void remmina_exec_exitremmina()
{
	TRACE_CALL("remmina_exec_exitremmina");
	GtkWidget* widget = NULL;

	widget = remmina_widget_pool_find(REMMINA_TYPE_CONNECTION_WINDOW, NULL);

	/* Don't quit Remmina if the user refuses to close the connection
	 * window when multiple tabs are opened */
	if (widget && !remmina_connection_window_delete(REMMINA_CONNECTION_WINDOW(widget)))
		return;

	/* Destroy all widgets, main window included */
	remmina_widget_pool_foreach(cb_closewidget, NULL);

	/* Remove systray menu */
	remmina_icon_destroy();

	/* Exit from Remmina */
	g_application_quit(g_application_get_default());
}

static gboolean disable_remmina_connection_window_delete_confirm_cb(GtkWidget *widget, gpointer data)
{
	TRACE_CALL("disable_remmina_connection_window_delete_confirm_cb");
	RemminaConnectionWindow *rcw;

	if (REMMINA_IS_CONNECTION_WINDOW(widget)) {
		rcw = (RemminaConnectionWindow*)widget;
		remmina_connection_window_set_delete_confirm_mode(rcw, REMMINA_CONNECTION_WINDOW_ONDELETE_NOCONFIRM);
	}
	return TRUE;
}

void remmina_application_condexit(RemminaCondExitType why)
{
	TRACE_CALL("remmina_application_check_exitremmina");

	/* Exit remmina only if there are no interesting windows left:
	 * no main window, no systray menu, no connection window.
	 * This function is usually called after a disconnection */

	switch(why) {
		case REMMINA_CONDEXIT_ONDISCONNECT:
			// A connection has disconnected, should we exit remmina ?
			if (remmina_widget_pool_count() < 1 && !remmina_main_get_window() && !remmina_icon_is_available())
				remmina_exec_exitremmina();
			break;
		case REMMINA_CONDEXIT_ONMAINWINDELETE:
			// Main window has been deleted
			if (remmina_widget_pool_count() < 1 && !remmina_icon_is_available())
				remmina_exec_exitremmina();
			break;
		case REMMINA_CONDEXIT_ONQUIT:
			// Quit command has been sent from main window or appindicator/systray menu
			// quit means QUIT.
			remmina_widget_pool_foreach(disable_remmina_connection_window_delete_confirm_cb, NULL);
			remmina_exec_exitremmina();
			break;
	}
}

void remmina_exec_command(RemminaCommandType command, const gchar* data)
{
	TRACE_CALL("remmina_exec_command");
	gchar* s1;
	gchar* s2;
	GtkWidget* widget;
	GtkWindow* mainwindow;
	GtkDialog* prefdialog;
	RemminaEntryPlugin* plugin;

	switch (command)
	{
	case REMMINA_COMMAND_MAIN:
		mainwindow = remmina_main_get_window();
		if (mainwindow)
		{
			gtk_window_present(mainwindow);
			gtk_window_deiconify(GTK_WINDOW(mainwindow));
		}
		else
		{
			widget = remmina_main_new();
			gtk_widget_show(widget);
		}
		break;

	case REMMINA_COMMAND_PREF:
		prefdialog = remmina_pref_dialog_get_dialog();
		if (prefdialog)
		{
			gtk_window_present(GTK_WINDOW(prefdialog));
			gtk_window_deiconify(GTK_WINDOW(prefdialog));
		}
		else
		{
			/* Create a new preference dialog */
			widget = GTK_WIDGET(remmina_pref_dialog_new(atoi(data), NULL));
			gtk_widget_show(widget);
		}
		break;

	case REMMINA_COMMAND_NEW:
		s1 = (data ? strchr(data, ',') : NULL);
		if (s1)
		{
			s1 = g_strdup(data);
			s2 = strchr(s1, ',');
			*s2++ = '\0';
			widget = remmina_file_editor_new_full(s2, s1);
			g_free(s1);
		}
		else
		{
			widget = remmina_file_editor_new_full(NULL, data);
		}
		gtk_widget_show(widget);
		break;

	case REMMINA_COMMAND_CONNECT:
		remmina_connection_window_open_from_filename(data);
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
		if (mainwindow)
		{
			remmina_about_open(NULL);
		}
		else
		{
			g_print ("%s - Version %s (git %s)\n", g_get_application_name (), VERSION, REMMINA_GIT_REVISION);
			/* As we do not use the "handle-local-options" signal, we have to exit Remmina */
			remmina_exec_command(REMMINA_COMMAND_EXIT, NULL);
		}

		break;

	case REMMINA_COMMAND_FULL_VERSION:
		mainwindow = remmina_main_get_window();
		if (mainwindow)
		{
			/* Show th widget with the list of plugins and versions */
			remmina_plugin_manager_show(mainwindow);
		}
		else
		{
			g_print("\n%s - Version %s (git %s)\n\n", g_get_application_name (), VERSION, REMMINA_GIT_REVISION);

			remmina_plugin_manager_show_stdout();
			remmina_exec_command(REMMINA_COMMAND_EXIT, NULL);
		}

		break;


	case REMMINA_COMMAND_PLUGIN:
		plugin = (RemminaEntryPlugin*) remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_ENTRY, data);
		if (plugin)
		{
			plugin->entry_func();
		}
		else
		{
			widget = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			                                _("Plugin %s is not registered."), data);
			g_signal_connect(G_OBJECT(widget), "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show(widget);
			remmina_widget_pool_register(widget);
		}
		break;

	case REMMINA_COMMAND_EXIT:
		remmina_widget_pool_foreach(disable_remmina_connection_window_delete_confirm_cb, NULL);
		remmina_exec_exitremmina();
		break;

	default:
		break;
	}
}

