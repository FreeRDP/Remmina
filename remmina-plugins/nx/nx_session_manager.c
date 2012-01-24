/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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

#include "common/remmina_plugin.h"
#include <X11/Xlib.h>
#include "nx_plugin.h"
#include "nx_session_manager.h"

static void remmina_nx_session_manager_set_sensitive(RemminaProtocolWidget *gp, gboolean sensitive)
{
	RemminaPluginNxData *gpdata;

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (gpdata->attach_session)
	{
		gtk_dialog_set_response_sensitive(GTK_DIALOG(gpdata->manager_dialog), REMMINA_NX_EVENT_TERMINATE, sensitive);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(gpdata->manager_dialog), REMMINA_NX_EVENT_ATTACH, sensitive);
	}
	else
	{
		gtk_dialog_set_response_sensitive(GTK_DIALOG(gpdata->manager_dialog), REMMINA_NX_EVENT_TERMINATE, sensitive);
		gtk_dialog_set_response_sensitive(GTK_DIALOG(gpdata->manager_dialog), REMMINA_NX_EVENT_RESTORE, sensitive);
	}
}

static gboolean remmina_nx_session_manager_selection_func(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path,
		gboolean path_currently_selected, gpointer user_data)
{
	RemminaProtocolWidget *gp;
	RemminaPluginNxData *gpdata;

	gp = (RemminaProtocolWidget*) user_data;
	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	gpdata->manager_selected = FALSE;
	if (path_currently_selected)
	{
		remmina_nx_session_manager_set_sensitive(gp, FALSE);
		return TRUE;
	}

	if (!gtk_tree_model_get_iter(model, &gpdata->iter, path))
		return TRUE;
	gpdata->manager_selected = TRUE;
	remmina_nx_session_manager_set_sensitive(gp, TRUE);
	return TRUE;
}

static void remmina_nx_session_manager_send_signal(RemminaPluginNxData *gpdata, gint event_type)
{
	guchar dummy = (guchar) event_type;
	/* Signal the NX thread to resume execution */
	if (write(gpdata->event_pipe[1], &dummy, 1))
	{
	}
}

static void remmina_nx_session_manager_on_response(GtkWidget *dialog, gint response_id, RemminaProtocolWidget *gp)
{
	RemminaPluginNxData *gpdata;
	gint event_type;

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remmina_nx_session_manager_set_sensitive(gp, FALSE);
	if (response_id <= 0)
	{
		event_type = REMMINA_NX_EVENT_CANCEL;
	}
	else
	{
		event_type = response_id;
	}
	if (response_id == REMMINA_NX_EVENT_TERMINATE && gpdata->manager_selected)
	{
		remmina_nx_session_iter_set(gpdata->nx, &gpdata->iter, REMMINA_NX_SESSION_COLUMN_STATUS, _("Terminating"));
	}
	if (response_id != REMMINA_NX_EVENT_TERMINATE)
	{
		gtk_widget_destroy(dialog);
		gpdata->manager_dialog = NULL;
	}
	if (response_id != REMMINA_NX_EVENT_TERMINATE && response_id != REMMINA_NX_EVENT_CANCEL)
	{
		remmina_plugin_nx_service->protocol_plugin_init_show(gp);
	}
	remmina_nx_session_manager_send_signal(gpdata, event_type);
}

static gboolean remmina_nx_session_manager_main(RemminaProtocolWidget *gp)
{
	RemminaPluginNxData *gpdata;
	RemminaFile *remminafile;
	GtkWidget *dialog;
	GtkWidget *widget;
	gchar *s;
	GtkWidget *scrolledwindow;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_nx_service->protocol_plugin_get_file(gp);

	if (!gpdata->manager_started)
	{
		remmina_plugin_nx_service->protocol_plugin_init_hide(gp);

		dialog = gtk_dialog_new();
		s = g_strdup_printf(_("NX Sessions on %s"), remmina_plugin_nx_service->file_get_string(remminafile, "server"));
		gtk_window_set_title(GTK_WINDOW(dialog), s);
		g_free(s);
		if (gpdata->attach_session)
		{
			gtk_dialog_add_button(GTK_DIALOG(dialog), _("Attach"), REMMINA_NX_EVENT_ATTACH);
		}
		else
		{
			gtk_dialog_add_button(GTK_DIALOG(dialog), _("Restore"), REMMINA_NX_EVENT_RESTORE);
			gtk_dialog_add_button(GTK_DIALOG(dialog), _("Start"), REMMINA_NX_EVENT_START);
		}
		gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, REMMINA_NX_EVENT_CANCEL);

		widget = gtk_dialog_add_button(GTK_DIALOG(dialog), _("Terminate"), REMMINA_NX_EVENT_TERMINATE);
		gtk_button_box_set_child_secondary(GTK_BUTTON_BOX(gtk_dialog_get_action_area(GTK_DIALOG(dialog))), widget,
				TRUE);

		gtk_window_set_default_size(GTK_WINDOW(dialog), 640, 300);
		gpdata->manager_dialog = dialog;

		scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
		gtk_widget_show(scrolledwindow);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), scrolledwindow, TRUE, TRUE, 0);

		tree = gtk_tree_view_new();
		gtk_container_add(GTK_CONTAINER(scrolledwindow), tree);
		gtk_widget_show(tree);
		remmina_nx_session_set_tree_view(gpdata->nx, GTK_TREE_VIEW(tree));

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes("#", renderer, "text", REMMINA_NX_SESSION_COLUMN_ID, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, REMMINA_NX_SESSION_COLUMN_ID);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Type"), renderer, "text", REMMINA_NX_SESSION_COLUMN_TYPE,
				NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, REMMINA_NX_SESSION_COLUMN_TYPE);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Display"), renderer, "text",
				REMMINA_NX_SESSION_COLUMN_DISPLAY, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, REMMINA_NX_SESSION_COLUMN_DISPLAY);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Status"), renderer, "text",
				REMMINA_NX_SESSION_COLUMN_STATUS, NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, REMMINA_NX_SESSION_COLUMN_STATUS);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

		renderer = gtk_cell_renderer_text_new();
		column = gtk_tree_view_column_new_with_attributes(_("Name"), renderer, "text", REMMINA_NX_SESSION_COLUMN_NAME,
				NULL);
		gtk_tree_view_column_set_resizable(column, TRUE);
		gtk_tree_view_column_set_sort_column_id(column, REMMINA_NX_SESSION_COLUMN_NAME);
		gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

		gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)),
				remmina_nx_session_manager_selection_func, gp, NULL);

		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(remmina_nx_session_manager_on_response), gp);
		gpdata->manager_started = TRUE;
	}
	gpdata->manager_selected = FALSE;
	if (gpdata->manager_dialog)
	{
		remmina_nx_session_manager_set_sensitive(gp, FALSE);
		gtk_widget_show(gpdata->manager_dialog);
	}
	if (remmina_nx_session_has_error(gpdata->nx))
	{
		dialog = gtk_message_dialog_new((gpdata->manager_dialog ? GTK_WINDOW(gpdata->manager_dialog) : NULL),
		GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "%s",
		remmina_nx_session_get_error (gpdata->nx));
		remmina_nx_session_clear_error (gpdata->nx);
		gtk_dialog_run (GTK_DIALOG(dialog));
		gtk_widget_destroy (dialog);
		remmina_nx_session_manager_send_signal (gpdata, 0);
	}

	gpdata->session_manager_start_handler = 0;
	return FALSE;
}

void remmina_nx_session_manager_start(RemminaProtocolWidget *gp)
{
	RemminaPluginNxData *gpdata;

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (gpdata->session_manager_start_handler == 0)
	{
		gpdata->session_manager_start_handler = IDLE_ADD((GSourceFunc) remmina_nx_session_manager_main, gp);
	}
}

