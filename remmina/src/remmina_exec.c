/*  See LICENSE and COPYING files for copyright and license details. */

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "remmina_main.h"
#include "remmina_widget_pool.h"
#include "remmina_pref_dialog.h"
#include "remmina_file.h"
#include "remmina_file_editor.h"
#include "remmina_connection_window.h"
#include "remmina_about.h"
#include "remmina_plugin_manager.h"
#include "remmina_exec.h"
#include "remmina/remmina_trace_calls.h"

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
			} else {
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

		default:
			break;
	}
}

