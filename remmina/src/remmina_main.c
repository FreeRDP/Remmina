/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
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
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include "remmina_string_array.h"
#include "remmina_public.h"
#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_file_editor.h"
#include "remmina_connection_window.h"
#include "remmina_about.h"
#include "remmina_pref.h"
#include "remmina_pref_dialog.h"
#include "remmina_widget_pool.h"
#include "remmina_plugin_manager.h"
#include "remmina_log.h"
#include "remmina_icon.h"
#include "remmina_main.h"
#include "remmina_external_tools.h"
#include "remmina/remmina_trace_calls.h"

G_DEFINE_TYPE( RemminaMain, remmina_main, GTK_TYPE_WINDOW)



static void remmina_main_class_init(RemminaMainClass *klass)
{
	TRACE_CALL("remmina_main_class_init");
}

enum
{
	PROTOCOL_COLUMN, NAME_COLUMN, GROUP_COLUMN, SERVER_COLUMN, FILENAME_COLUMN, N_COLUMNS
};

static GtkTargetEntry remmina_drop_types[] =
{
{ "text/uri-list", 0, 1 } };

static void remmina_main_save_size(RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_save_size");
	if ((gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(remminamain))) & GDK_WINDOW_STATE_MAXIMIZED) == 0)
	{
		gtk_window_get_size(GTK_WINDOW(remminamain), &remmina_pref.main_width, &remmina_pref.main_height);
		remmina_pref.main_maximize = FALSE;
	}
	else
	{
		remmina_pref.main_maximize = TRUE;
	}
}

static void remmina_main_save_expanded_group_func(GtkTreeView *tree_view, GtkTreePath *path, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_save_expanded_group_func");
	GtkTreeIter iter;
	gchar *group;

	gtk_tree_model_get_iter(remminamain->priv->file_model_sort, &iter, path);
	gtk_tree_model_get(remminamain->priv->file_model_sort, &iter, GROUP_COLUMN, &group, -1);
	if (group)
	{
		remmina_string_array_add(remminamain->priv->expanded_group, group);
		g_free(group);
	}
}

static void remmina_main_save_expanded_group(RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_save_expanded_group");
	if (GTK_IS_TREE_STORE(remminamain->priv->file_model))
	{
		if (remminamain->priv->expanded_group)
		{
			remmina_string_array_free(remminamain->priv->expanded_group);
		}
		remminamain->priv->expanded_group = remmina_string_array_new();
		gtk_tree_view_map_expanded_rows(remminamain->priv->file_list,
				(GtkTreeViewMappingFunc) remmina_main_save_expanded_group_func, remminamain);
	}
}

static gboolean remmina_main_on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	TRACE_CALL("remmina_main_on_delete_event");
	RemminaMain *remminamain;

	remminamain = REMMINA_MAIN(widget);

	remmina_main_save_size(remminamain);

	remmina_main_save_expanded_group(remminamain);
	g_free(remmina_pref.expanded_group);
	remmina_pref.expanded_group = remmina_string_array_to_string(remminamain->priv->expanded_group);
	remmina_string_array_free(remminamain->priv->expanded_group);
	remminamain->priv->expanded_group = NULL;

	remmina_pref_save();
	return FALSE;
}

static void remmina_main_destroy(GtkWidget *widget, gpointer data)
{
	TRACE_CALL("remmina_main_destroy");
	g_object_unref(G_OBJECT(REMMINA_MAIN(widget)->priv->builder_models));
	g_object_unref(G_OBJECT(REMMINA_MAIN(widget)->priv->builder_actions));
	g_free(REMMINA_MAIN(widget)->priv->selected_filename);
	g_free(REMMINA_MAIN(widget)->priv->selected_name);
	g_free(REMMINA_MAIN(widget)->priv);
}

static void remmina_main_clear_selection_data(RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_clear_selection_data");
	g_free(remminamain->priv->selected_filename);
	g_free(remminamain->priv->selected_name);
	remminamain->priv->selected_filename = NULL;
	remminamain->priv->selected_name = NULL;
}

static gboolean remmina_main_selection_func(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path,
		gboolean path_currently_selected, gpointer user_data)
{
	TRACE_CALL("remmina_main_selection_func");
	RemminaMain *remminamain;
	guint context_id;
	GtkTreeIter iter;
	gchar buf[1000];

	remminamain = (RemminaMain*) user_data;
	if (path_currently_selected)
		return TRUE;

	if (!gtk_tree_model_get_iter(model, &iter, path))
		return TRUE;

	remmina_main_clear_selection_data(remminamain);

	gtk_tree_model_get(model, &iter, NAME_COLUMN, &remminamain->priv->selected_name, FILENAME_COLUMN,
			&remminamain->priv->selected_filename, -1);

	context_id = gtk_statusbar_get_context_id(remminamain->priv->statusbar, "status");
	gtk_statusbar_pop(remminamain->priv->statusbar, context_id);
	if (remminamain->priv->selected_filename)
	{
		g_snprintf(buf, sizeof(buf), "%s (%s)", remminamain->priv->selected_name, remminamain->priv->selected_filename);
		gtk_statusbar_push(remminamain->priv->statusbar, context_id, buf);
	}
	else
	{
		gtk_statusbar_push(remminamain->priv->statusbar, context_id, remminamain->priv->selected_name);
	}
	return TRUE;
}

static void remmina_main_load_file_list_callback(gpointer data, gpointer user_data)
{
	TRACE_CALL("remmina_main_load_file_list_callback");
	RemminaMain *remminamain;
	GtkTreeIter iter;
	GtkListStore *store;
	RemminaFile *remminafile;

	remminafile = (RemminaFile*) data;
	remminamain = (RemminaMain*) user_data;
	store = GTK_LIST_STORE(remminamain->priv->file_model);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, PROTOCOL_COLUMN, remmina_file_get_icon_name(remminafile), NAME_COLUMN,
			remmina_file_get_string(remminafile, "name"), GROUP_COLUMN,
			remmina_file_get_string(remminafile, "group"), SERVER_COLUMN,
			remmina_file_get_string(remminafile, "server"), FILENAME_COLUMN, remmina_file_get_filename(remminafile),
			-1);
}

static gboolean remmina_main_load_file_tree_traverse(GNode *node, GtkTreeStore *store, GtkTreeIter *parent)
{
	TRACE_CALL("remmina_main_load_file_tree_traverse");
	GtkTreeIter *iter;
	RemminaGroupData *data;
	GNode *child;

	iter = NULL;
	if (node->data)
	{
		data = (RemminaGroupData*) node->data;
		iter = g_new0(GtkTreeIter, 1);
		gtk_tree_store_append(store, iter, parent);
		gtk_tree_store_set(store, iter, PROTOCOL_COLUMN, "folder", NAME_COLUMN, data->name, GROUP_COLUMN,
				data->group, FILENAME_COLUMN, NULL, -1);
	}
	for (child = g_node_first_child(node); child; child = g_node_next_sibling(child))
	{
		remmina_main_load_file_tree_traverse(child, store, iter);
	}
	g_free(iter);
	return FALSE;
}

static void remmina_main_load_file_tree_group(GtkTreeStore *store)
{
	TRACE_CALL("remmina_main_load_file_tree_group");
	GNode *root;

	root = remmina_file_manager_get_group_tree();
	remmina_main_load_file_tree_traverse(root, store, NULL);
	remmina_file_manager_free_group_tree(root);
}

static void remmina_main_expand_group_traverse(RemminaMain *remminamain, GtkTreeIter *iter)
{
	TRACE_CALL("remmina_main_expand_group_traverse");
	GtkTreeModel *tree;
	gboolean ret;
	gchar *group, *filename;
	GtkTreeIter child;
	GtkTreePath *path;

	tree = remminamain->priv->file_model_sort;
	ret = TRUE;
	while (ret)
	{
		gtk_tree_model_get(tree, iter, GROUP_COLUMN, &group, FILENAME_COLUMN, &filename, -1);
		if (filename == NULL)
		{
			if (remmina_string_array_find(remminamain->priv->expanded_group, group) >= 0)
			{
				path = gtk_tree_model_get_path(tree, iter);
				gtk_tree_view_expand_row(remminamain->priv->file_list, path, FALSE);
				gtk_tree_path_free(path);
			}
			if (gtk_tree_model_iter_children(tree, &child, iter))
			{
				remmina_main_expand_group_traverse(remminamain, &child);
			}
		}
		g_free(group);
		g_free(filename);

		ret = gtk_tree_model_iter_next(tree, iter);
	}
}

static void remmina_main_expand_group(RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_expand_group");
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(remminamain->priv->file_model_sort, &iter))
	{
		remmina_main_expand_group_traverse(remminamain, &iter);
	}
}

static gboolean remmina_main_load_file_tree_find(GtkTreeModel *tree, GtkTreeIter *iter, const gchar *match_group)
{
	TRACE_CALL("remmina_main_load_file_tree_find");
	gboolean ret, match;
	gchar *group, *filename;
	GtkTreeIter child;

	match = FALSE;
	ret = TRUE;
	while (ret)
	{
		gtk_tree_model_get(tree, iter, GROUP_COLUMN, &group, FILENAME_COLUMN, &filename, -1);
		match = (filename == NULL && g_strcmp0(group, match_group) == 0);
		g_free(group);
		g_free(filename);
		if (match)
			break;
		if (gtk_tree_model_iter_children(tree, &child, iter))
		{
			match = remmina_main_load_file_tree_find(tree, &child, match_group);
			if (match)
			{
				memcpy(iter, &child, sizeof(GtkTreeIter));
				break;
			}
		}
		ret = gtk_tree_model_iter_next(tree, iter);
	}
	return match;
}

static void remmina_main_load_file_tree_callback(gpointer data, gpointer user_data)
{
	TRACE_CALL("remmina_main_load_file_tree_callback");
	RemminaMain *remminamain;
	GtkTreeIter iter, child;
	GtkTreeStore *store;
	RemminaFile *remminafile;
	gboolean found;

	remminafile = (RemminaFile*) data;
	remminamain = (RemminaMain*) user_data;
	store = GTK_TREE_STORE(remminamain->priv->file_model);

	found = FALSE;
	if (gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter))
	{
		found = remmina_main_load_file_tree_find(GTK_TREE_MODEL(store), &iter,
				remmina_file_get_string(remminafile, "group"));
	}

	gtk_tree_store_append(store, &child, (found ? &iter : NULL));
	gtk_tree_store_set(store, &child, PROTOCOL_COLUMN, remmina_file_get_icon_name(remminafile), NAME_COLUMN,
			remmina_file_get_string(remminafile, "name"), GROUP_COLUMN,
			remmina_file_get_string(remminafile, "group"), SERVER_COLUMN,
			remmina_file_get_string(remminafile, "server"), FILENAME_COLUMN, remmina_file_get_filename(remminafile),
			-1);
}

static void remmina_main_file_model_on_sort(GtkTreeSortable *sortable, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_file_model_on_sort");
	gint columnid;
	GtkSortType order;

	gtk_tree_sortable_get_sort_column_id(sortable, &columnid, &order);
	remmina_pref.main_sort_column_id = columnid;
	remmina_pref.main_sort_order = order;
	remmina_pref_save();
}

static gboolean remmina_main_filter_visible_func(GtkTreeModel *model, GtkTreeIter *iter, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_filter_visible_func");
	gchar *text;
	gchar *protocol, *name, *group, *server, *s;
	gboolean result = TRUE;

	if (!remmina_pref.show_quick_search)
		return TRUE;

	text = g_ascii_strdown(gtk_entry_get_text(remminamain->priv->quick_search_entry), -1);
	if (text && text[0])
	{
		gtk_tree_model_get(model, iter, PROTOCOL_COLUMN, &protocol, NAME_COLUMN, &name, GROUP_COLUMN, &group,
				SERVER_COLUMN, &server, -1);
		if (g_strcmp0(protocol, "folder") != 0)
		{
			s = g_ascii_strdown(name ? name : "", -1);
			g_free(name);
			name = s;
			s = g_ascii_strdown(group ? group : "", -1);
			g_free(group);
			group = s;
			s = g_ascii_strdown(server ? server : "", -1);
			g_free(server);
			server = s;
			result = (strstr(name, text) || strstr(server, text) || strstr(group, text));
		}
		g_free(protocol);
		g_free(name);
		g_free(group);
		g_free(server);
	}
	g_free(text);
	return result;
}

static void remmina_main_select_file(RemminaMain *remminamain, const gchar *filename)
{
	TRACE_CALL("remmina_main_select_file");
	GtkTreeIter iter;
	GtkTreePath *path;
	gchar *item_filename;
	gboolean cmp;

	if (!gtk_tree_model_get_iter_first(remminamain->priv->file_model_sort, &iter))
		return;

	while (1)
	{
		gtk_tree_model_get(remminamain->priv->file_model_sort, &iter, FILENAME_COLUMN, &item_filename, -1);
		cmp = g_strcmp0(item_filename, filename);
		g_free(item_filename);
		if (cmp == 0)
		{
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(remminamain->priv->file_list),
					&iter);
			path = gtk_tree_model_get_path(remminamain->priv->file_model_sort, &iter);
			gtk_tree_view_scroll_to_cell(remminamain->priv->file_list, path, NULL, TRUE, 0.5, 0.0);
			gtk_tree_path_free(path);
			return;
		}
		if (!gtk_tree_model_iter_next(remminamain->priv->file_model_sort, &iter))
			return;
	}
}

static void remmina_main_load_files(RemminaMain *remminamain, gboolean refresh)
{
	TRACE_CALL("remmina_main_load_files");
	gint items_count;
	gchar buf[200];
	guint context_id;

	if (refresh)
	{
		remmina_main_save_expanded_group(remminamain);
	}

	switch (remmina_pref.view_file_mode)
	{
		case REMMINA_VIEW_FILE_TREE:
			/* Hide the Group column in the tree view mode */
			gtk_tree_view_column_set_visible(GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(
				remminamain->priv->builder_models, "column_files_list_group")), FALSE);
			/* Use the TreeStore model to store data */
			remminamain->priv->file_model = GTK_TREE_MODEL(
				gtk_builder_get_object(remminamain->priv->builder_models, "treestore_files_list"));
			/* Clear any previous data in the model */
			gtk_tree_store_clear(GTK_TREE_STORE(remminamain->priv->file_model));
			/* Load groups first */
			remmina_main_load_file_tree_group(GTK_TREE_STORE(remminamain->priv->file_model));
			/* Load files list */
			items_count = remmina_file_manager_iterate(remmina_main_load_file_tree_callback, remminamain);
			break;

		case REMMINA_VIEW_FILE_LIST:
		default:
			/* Show the Group column in the list view mode */
			gtk_tree_view_column_set_visible(GTK_TREE_VIEW_COLUMN(gtk_builder_get_object(
				remminamain->priv->builder_models, "column_files_list_group")), TRUE);
			/* Use the ListStore model to store data */
			remminamain->priv->file_model = GTK_TREE_MODEL(
				gtk_builder_get_object(remminamain->priv->builder_models, "liststore_files_list"));
			/* Clear any previous data in the model */
			gtk_list_store_clear(GTK_LIST_STORE(remminamain->priv->file_model));
			/* Load files list */
			items_count = remmina_file_manager_iterate(remmina_main_load_file_list_callback, remminamain);
			break;
	}
	/* Apply sorted filtered model to the TreeView */
	remminamain->priv->file_model_filter = gtk_tree_model_filter_new(remminamain->priv->file_model, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(remminamain->priv->file_model_filter),
			(GtkTreeModelFilterVisibleFunc) remmina_main_filter_visible_func, remminamain, NULL);
	remminamain->priv->file_model_sort = gtk_tree_model_sort_new_with_model(remminamain->priv->file_model_filter);
	gtk_tree_view_set_model(remminamain->priv->file_list, remminamain->priv->file_model_sort);
	gtk_tree_sortable_set_sort_column_id(
		GTK_TREE_SORTABLE(remminamain->priv->file_model_sort),
		remmina_pref.main_sort_column_id,
		remmina_pref.main_sort_order);
	g_signal_connect(G_OBJECT(remminamain->priv->file_model_sort), "sort-column-changed",
		G_CALLBACK(remmina_main_file_model_on_sort), remminamain);
	remmina_main_expand_group(remminamain);
	/* Select the file previously selected */
	if (remminamain->priv->selected_filename)
	{
		remmina_main_select_file(remminamain, remminamain->priv->selected_filename);
	}
	/* Show in the status bar the total number of connections found */
	g_snprintf(buf, 200, ngettext("Total %i item.", "Total %i items.", items_count), items_count);
	context_id = gtk_statusbar_get_context_id(remminamain->priv->statusbar, "status");
	gtk_statusbar_pop(remminamain->priv->statusbar, context_id);
	gtk_statusbar_push(remminamain->priv->statusbar, context_id, buf);
}

static void remmina_main_action_connection_connect(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_connection_connect");
	if (!remminamain->priv->selected_filename)
		return;

	remmina_connection_window_open_from_filename(remminamain->priv->selected_filename);
}

static void remmina_main_action_connection_external_tools(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_connection_external_tools");
	if (!remminamain->priv->selected_filename)
		return;

	remmina_external_tools_from_filename(remminamain, remminamain->priv->selected_filename);
}

static void remmina_main_file_editor_destroy(GtkWidget *widget, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_file_editor_destroy");
	if (GTK_IS_WIDGET(remminamain))
	{
		remmina_main_load_files(remminamain, TRUE);
	}
}

static void remmina_main_action_connection_new(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_connection_new");
	GtkWidget *widget;

	widget = remmina_file_editor_new();
	g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(remmina_main_file_editor_destroy), remminamain);
	gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(remminamain));
	gtk_widget_show(widget);
}

static void remmina_main_action_connection_copy(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_connection_copy");
	GtkWidget *widget;

	if (!remminamain->priv->selected_filename)
		return;

	widget = remmina_file_editor_new_copy(remminamain->priv->selected_filename);
	if (widget)
	{
		g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(remmina_main_file_editor_destroy), remminamain);
		gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(remminamain));
		gtk_widget_show(widget);
	}
}

static void remmina_main_action_connection_edit(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_connection_edit");
	GtkWidget *widget;

	if (!remminamain->priv->selected_filename)
		return;

	widget = remmina_file_editor_new_from_filename(remminamain->priv->selected_filename);
	if (widget)
	{
		g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(remmina_main_file_editor_destroy), remminamain);
		gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(remminamain));
		gtk_widget_show(widget);
	}
}

static void remmina_main_action_connection_delete(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_connection_delete");
	GtkWidget *dialog;

	if (!remminamain->priv->selected_filename)
		return;

	dialog = gtk_message_dialog_new(GTK_WINDOW(remminamain), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
			_("Are you sure to delete '%s'"), remminamain->priv->selected_name);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
	{
		remmina_file_delete(remminamain->priv->selected_filename);
		remmina_icon_populate_menu();
		remmina_main_load_files(remminamain, TRUE);
	}
	gtk_widget_destroy(dialog);
	remmina_main_clear_selection_data(remminamain);
}

static void remmina_main_action_edit_preferences(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_edit_preferences");
	GtkWidget *widget;

	widget = remmina_pref_dialog_new(0);
	gtk_window_set_transient_for(GTK_WINDOW(widget), GTK_WINDOW(remminamain));
	gtk_widget_show(widget);
}

static void remmina_main_action_connection_close(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_connection_close");
	gtk_widget_destroy(GTK_WIDGET(remminamain));
}

static void remmina_main_action_view_toolbar(GtkToggleAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_view_toolbar");
	gboolean toggled;

	toggled = gtk_toggle_action_get_active(action);
	if (toggled)
	{
		gtk_widget_show(GTK_WIDGET(remminamain->priv->toolbar));
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(remminamain->priv->toolbar));
	}
	if (remminamain->priv->initialized)
	{
		remmina_pref.hide_toolbar = !toggled;
		remmina_pref_save();
	}
}

static void remmina_main_action_view_quick_search(GtkToggleAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_view_quick_search");
	gboolean toggled;

	toggled = gtk_toggle_action_get_active(action);
	if (toggled)
	{
		gtk_entry_set_text(remminamain->priv->quick_search_entry, "");
		gtk_widget_show(GTK_WIDGET(remminamain->priv->quick_search_separator));
		gtk_widget_show(GTK_WIDGET(remminamain->priv->quick_search_item));
		gtk_widget_grab_focus(GTK_WIDGET(remminamain->priv->quick_search_entry));
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(remminamain->priv->quick_search_separator));
		gtk_widget_hide(GTK_WIDGET(remminamain->priv->quick_search_item));
	}
	if (remminamain->priv->initialized)
	{
		remmina_pref.show_quick_search = toggled;
		remmina_pref_save();

		if (!toggled)
		{
			gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(remminamain->priv->file_model_filter));
			gtk_tree_view_expand_all(remminamain->priv->file_list);
		}
	}
}

static void remmina_main_action_view_statusbar(GtkToggleAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_view_statusbar");
	gboolean toggled;

	toggled = gtk_toggle_action_get_active(action);
	if (toggled)
	{
		gtk_widget_show(GTK_WIDGET(remminamain->priv->statusbar));
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(remminamain->priv->statusbar));
	}
	if (remminamain->priv->initialized)
	{
		remmina_pref.hide_statusbar = !toggled;
		remmina_pref_save();
	}
}

static void remmina_main_action_view_quick_connect(GtkToggleAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_view_quick_connect");
	gboolean toggled;

	toggled = gtk_toggle_action_get_active(action);
	if (toggled)
	{
		gtk_widget_show(GTK_WIDGET(remminamain->priv->quickconnect_box));
	}
	else
	{
		gtk_widget_hide(GTK_WIDGET(remminamain->priv->quickconnect_box));
	}
	if (remminamain->priv->initialized)
	{
		remmina_pref.hide_quick_connect = !toggled;
		remmina_pref_save();
	}
}

static void remmina_main_action_view_small_toolbutton(GtkToggleAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_view_small_toolbutton");
	gboolean toggled;

	toggled = gtk_toggle_action_get_active(action);
	if (toggled)
	{
		gtk_toolbar_set_icon_size(GTK_TOOLBAR(remminamain->priv->toolbar), GTK_ICON_SIZE_MENU);
	}
	else
	{
		gtk_toolbar_unset_icon_size(GTK_TOOLBAR(remminamain->priv->toolbar));
	}
	if (remminamain->priv->initialized)
	{
		remmina_pref.small_toolbutton = toggled;
		remmina_pref_save();
	}
}

static void remmina_main_action_view_file_mode(GtkRadioAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_view_file_mode");
	static GtkRadioAction *previous_action;
	if (!previous_action)
		previous_action = action;
	if (action != previous_action)
	{
		remmina_pref.view_file_mode = gtk_radio_action_get_current_value(action);
		remmina_pref_save();
		remmina_main_load_files(remminamain, TRUE);
	}
	previous_action = action;
}

static void remmina_main_import_file_list(RemminaMain *remminamain, GSList *files)
{
	TRACE_CALL("remmina_main_import_file_list");
	GtkWidget *dlg;
	GSList *element;
	gchar *path;
	RemminaFilePlugin *plugin;
	GString *err;
	RemminaFile *remminafile = NULL;
	gboolean imported;

	err = g_string_new(NULL);
	imported = FALSE;
	for (element = files; element; element = element->next)
	{
		path = (gchar*) element->data;
		plugin = remmina_plugin_manager_get_import_file_handler(path);
		if (plugin && (remminafile = plugin->import_func(path)) != NULL && remmina_file_get_string(remminafile, "name"))
		{
			remmina_file_generate_filename(remminafile);
			remmina_file_save_all(remminafile);
			imported = TRUE;
		}
		else
		{
			g_string_append(err, path);
			g_string_append_c(err, '\n');
		}
		if (remminafile)
		{
			remmina_file_free(remminafile);
			remminafile = NULL;
		}
		g_free(path);
	}
	g_slist_free(files);
	if (err->len > 0)
	{
		dlg = gtk_message_dialog_new(GTK_WINDOW(remminamain), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				_("Unable to import:\n%s"), err->str);
		g_signal_connect(G_OBJECT(dlg), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(dlg);
	}
	g_string_free(err, TRUE);
	if (imported)
	{
		remmina_main_load_files(remminamain, TRUE);
	}
}

static void remmina_main_action_tools_import_on_response(GtkDialog *dialog, gint response_id, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_tools_import_on_response");
	GSList *files;

	if (response_id == GTK_RESPONSE_ACCEPT)
	{
		files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
		remmina_main_import_file_list(remminamain, files);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void remmina_main_action_tools_import(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_tools_import");
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new(_("Import"), GTK_WINDOW(remminamain), GTK_FILE_CHOOSER_ACTION_OPEN, "document-open",
			GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(remmina_main_action_tools_import_on_response), remminamain);
	gtk_widget_show(dialog);
}

static void remmina_main_action_tools_export(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_tools_export");
	RemminaFilePlugin *plugin;
	RemminaFile *remminafile;
	GtkWidget *dialog;

	if (!remminamain->priv->selected_filename)
		return;

	remminafile = remmina_file_load(remminamain->priv->selected_filename);
	if (remminafile == NULL)
		return;
	plugin = remmina_plugin_manager_get_export_file_handler(remminafile);
	if (plugin)
	{
		dialog = gtk_file_chooser_dialog_new(plugin->export_hints, GTK_WINDOW(remminamain),
				GTK_FILE_CHOOSER_ACTION_SAVE, _("_Save"), GTK_RESPONSE_ACCEPT, NULL);
		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
		{
			plugin->export_func(remminafile, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
		}
		gtk_widget_destroy(dialog);
	}
	else
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(remminamain), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				_("This protocol does not support exporting."));
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(dialog);
	}
	remmina_file_free(remminafile);
}

static void remmina_main_action_tools_plugins(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_tools_plugins");
	remmina_plugin_manager_show(GTK_WINDOW(remminamain));
}

/* Activate a GtkAction from a RemminaToolPlugin */
static void remmina_main_action_tools_addition(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_tools_addition");
	RemminaToolPlugin *plugin = (RemminaToolPlugin*) remmina_plugin_manager_get_plugin(
		REMMINA_PLUGIN_TYPE_TOOL, gtk_action_get_name(action));
	if (plugin)
	{
		plugin->exec_func();
	}
}

static void remmina_main_action_help_homepage(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_help_homepage");
	g_app_info_launch_default_for_uri("http://remmina.sourceforge.net", NULL, NULL);
}

static void remmina_main_action_help_wiki(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_help_wiki");
	g_app_info_launch_default_for_uri("http://sourceforge.net/apps/mediawiki/remmina/", NULL, NULL);
}

static void remmina_main_action_help_debug(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_help_debug");
	remmina_log_start();
}

static void remmina_main_action_help_about(GtkAction *action, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_action_help_about");
	remmina_about_open(GTK_WINDOW(remminamain));
};

static gboolean remmina_main_quickconnect(RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_quickconnect");
	RemminaFile* remminafile;
	gchar* server;

	remminafile = remmina_file_new();
	server = strdup(gtk_entry_get_text(remminamain->priv->quickconnect_server));

	remmina_file_set_string(remminafile, "sound", "off");
	remmina_file_set_string(remminafile, "server", server);
	remmina_file_set_string(remminafile, "name", server);
	remmina_file_set_string(remminafile, "protocol", 
		gtk_combo_box_text_get_active_text(remminamain->priv->quickconnect_protocol));

	remmina_connection_window_open_from_file(remminafile);

	return FALSE;
}

static gboolean remmina_main_quickconnect_on_click(GtkWidget *widget, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_quickconnect_on_click");
	return remmina_main_quickconnect(remminamain);
}

/* Handle double click on a row in the connections list */
void remmina_main_file_list_on_row_activated(GtkTreeView *tree, GtkTreePath *path, GtkTreeViewColumn *column, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_file_list_on_row_activated");
	/* If a connection was selected then execute the default action */
	if (remminamain->priv->selected_filename)
	{
		switch (remmina_pref.default_action)
		{
			case REMMINA_ACTION_EDIT:
				remmina_main_action_connection_edit(NULL, remminamain);
				break;
			case REMMINA_ACTION_CONNECT:
			default:
				remmina_main_action_connection_connect(NULL, remminamain);
				break;
		}
	}
}

static gboolean remmina_main_file_list_on_button_press(GtkWidget *widget, GdkEventButton *event, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_file_list_on_button_press");
	GtkMenu *popup;

	if (event->button == MOUSE_BUTTON_RIGHT)
	{
		popup = GTK_MENU(gtk_builder_get_object(remminamain->priv->builder_actions, "menu_popup"));
		if (popup)
		{
			gtk_menu_popup(popup, NULL, NULL, NULL, NULL, event->button, event->time);
		}
	}
	return FALSE;
}

static void remmina_main_quick_search_on_icon_press(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event,
		RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_quick_search_on_icon_press");
	if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
	{
		gtk_entry_set_text(entry, "");
	}
}

static void remmina_main_quick_search_on_changed(GtkEditable *editable, RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_quick_search_on_changed");
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(remminamain->priv->file_model_filter));
	gtk_tree_view_expand_all(remminamain->priv->file_list);
}

static void remmina_main_on_drag_data_received(RemminaMain *remminamain, GdkDragContext *drag_context, gint x, gint y,
		GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
	TRACE_CALL("remmina_main_on_drag_data_received");
	gchar **uris;
	GSList *files = NULL;
	gint i;

	uris = g_uri_list_extract_uris((const gchar *) gtk_selection_data_get_data(data));
	for (i = 0; uris[i]; i++)
	{
		if (strncmp(uris[i], "file://", 7) != 0)
			continue;
		files = g_slist_append(files, g_strdup(uris[i] + 7));
	}
	g_strfreev(uris);
	remmina_main_import_file_list(remminamain, files);
}

/* Add a new menuitem to the Tools menu */
static gboolean remmina_main_add_tool_plugin(gchar *name, RemminaPlugin *plugin, gpointer data)
{
	TRACE_CALL("remmina_main_add_tool_plugin");
	RemminaMain *remminamain = REMMINA_MAIN(data);
	GtkWidget *menuitem = gtk_menu_item_new_with_label(plugin->name);
	GtkAction *action = gtk_action_new(name, plugin->description, NULL, NULL);

	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtk_builder_get_object(remminamain->priv->builder_actions, "menu_tools")), menuitem);
	gtk_activatable_set_related_action(GTK_ACTIVATABLE(menuitem), action);
	g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(remmina_main_action_tools_addition), remminamain);
	g_object_unref(action);

	return FALSE;
}


static gboolean remmina_main_on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer user_data)
{
	TRACE_CALL("remmina_main_on_window_state_event");
#ifdef ENABLE_MINIMIZE_TO_TRAY
	GdkScreen *screen;

	screen = gdk_screen_get_default();
	if (remmina_pref.minimize_to_tray && (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) != 0
			&& (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) != 0
			&& remmina_public_get_current_workspace(screen)
					== remmina_public_get_window_workspace(GTK_WINDOW(widget))
			&& gdk_screen_get_number(screen) == gdk_screen_get_number(gtk_widget_get_screen(widget)))
	{
		gtk_widget_hide(widget);
		return TRUE;
	}
#endif
	return FALSE;
}


static void remmina_main_init(RemminaMain *remminamain)
{
	TRACE_CALL("remmina_main_init");
	RemminaMainPriv *priv;
	GtkBox *remmina_main_container;
	GtkButton *button_quick_connect;
	GtkScrolledWindow *scrolledwindow;
	GtkBuilder *builder;
	gint i;

	priv = g_new0(RemminaMainPriv, 1);
	remminamain->priv = priv;

	remminamain->priv->expanded_group = remmina_string_array_new_from_string(remmina_pref.expanded_group);

	/* Create main window */
	g_signal_connect(G_OBJECT(remminamain), "delete-event", G_CALLBACK(remmina_main_on_delete_event), NULL);
	g_signal_connect(G_OBJECT(remminamain), "destroy", G_CALLBACK(remmina_main_destroy), NULL);
	g_signal_connect(G_OBJECT(remminamain), "window-state-event", G_CALLBACK(remmina_main_on_window_state_event), NULL);
	gtk_container_set_border_width(GTK_CONTAINER(remminamain), 0);
	gtk_window_set_title(GTK_WINDOW(remminamain), _("Remmina Remote Desktop Client"));
	gtk_window_set_default_size(GTK_WINDOW(remminamain), remmina_pref.main_width, remmina_pref.main_height);
	gtk_window_set_position(GTK_WINDOW(remminamain), GTK_WIN_POS_CENTER);
	if (remmina_pref.main_maximize)
	{
		gtk_window_maximize(GTK_WINDOW(remminamain));
	}

	/* Create the main container */
	remmina_main_container = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
	gtk_container_add(GTK_CONTAINER(remminamain), GTK_WIDGET(remmina_main_container));
	gtk_widget_show(GTK_WIDGET(remmina_main_container));

	priv->builder_actions = remmina_public_gtk_builder_new_from_file("remmina_main_actions.glade");

	/* Add the Menubar */
	remmina_public_gtk_widget_reparent(
		GTK_WIDGET(gtk_builder_get_object(priv->builder_actions, "menubar_main")),
		GTK_CONTAINER(remmina_main_container));

	/* Add a GtkMenuItem to the Tools menu for each plugin of type REMMINA_PLUGIN_TYPE_TOOL */
	remmina_plugin_manager_for_each_plugin(REMMINA_PLUGIN_TYPE_TOOL, remmina_main_add_tool_plugin, remminamain);

	/* Add the Toolbar */
	priv->toolbar = GTK_TOOLBAR(gtk_builder_get_object(priv->builder_actions, "toolbar_main"));
	remmina_public_gtk_widget_reparent(GTK_WIDGET(priv->toolbar), GTK_CONTAINER(remmina_main_container));
	
	/* Add the actions and connect signals */
	ActionsCallbackMap action_maps[] = {
		{ "action_connection_connect", remmina_main_action_connection_connect },
		{ "action_connection_new", remmina_main_action_connection_new },
		{ "action_connection_edit", remmina_main_action_connection_edit },
		{ "action_connection_copy", remmina_main_action_connection_copy },
		{ "action_connection_delete", remmina_main_action_connection_delete },
		{ "action_application_preferences", remmina_main_action_edit_preferences },
		{ "action_application_about", remmina_main_action_help_about },
		{ "action_application_plugins", remmina_main_action_tools_plugins },
		{ "action_application_quit", remmina_main_action_connection_close },
		{ "action_view_toolbar", remmina_main_action_view_toolbar },
		{ "action_view_statusbar", remmina_main_action_view_statusbar },
		{ "action_view_quick_search", remmina_main_action_view_quick_search },
		{ "action_view_quick_connect", remmina_main_action_view_quick_connect },
		{ "action_view_small_toolbar_buttons", remmina_main_action_view_small_toolbutton },
		{ "action_view_mode_list", remmina_main_action_view_file_mode },
		{ "action_view_mode_tree", remmina_main_action_view_file_mode },
		{ "action_tools_import", remmina_main_action_tools_import },
		{ "action_tools_export", remmina_main_action_tools_export },
		{ "action_tools_externaltools", remmina_main_action_connection_external_tools },
		{ "action_help_homepage", remmina_main_action_help_homepage },
		{ "action_help_wiki", remmina_main_action_help_wiki },
		{ "action_help_debug", remmina_main_action_help_debug },
		{ NULL, NULL }
	};
	for (i = 0; action_maps[i].action_name; i++)
	{
		g_signal_connect(
			G_OBJECT(gtk_builder_get_object(priv->builder_actions, action_maps[i].action_name)),
			"activate", G_CALLBACK(action_maps[i].callback), remminamain);
	};

	/* Connect the group accelerators to the GtkWindow */
	gtk_window_add_accel_group(GTK_WINDOW(remminamain),
		GTK_ACCEL_GROUP(gtk_builder_get_object(priv->builder_actions, "accelgroup_shortcuts")));

	/* Add the Quick Search box */
	builder = remmina_public_gtk_builder_new_from_file("remmina_main_quick_search.glade");
	remminamain->priv->quick_search_entry = GTK_ENTRY(
		gtk_builder_get_object(builder, "entry_quick_search"));
	remminamain->priv->quick_search_separator = GTK_TOOL_ITEM(
		gtk_builder_get_object(remminamain->priv->builder_actions, "toolbutton_separator_quick_search"));
	remminamain->priv->quick_search_item = GTK_TOOL_ITEM(
		gtk_builder_get_object(remminamain->priv->builder_actions, "toolbutton_quick_search"));
	remmina_public_gtk_widget_reparent(GTK_WIDGET(gtk_builder_get_object(builder, "box_quick_search")),
		GTK_CONTAINER(remminamain->priv->quick_search_item));
	g_signal_connect(G_OBJECT(remminamain->priv->quick_search_entry), "icon-press",
		G_CALLBACK(remmina_main_quick_search_on_icon_press), remminamain);
	g_signal_connect(G_OBJECT(remminamain->priv->quick_search_entry), "changed",
		G_CALLBACK(remmina_main_quick_search_on_changed), remminamain);

	/* Add a Quick Connection box */
	builder = remmina_public_gtk_builder_new_from_file("remmina_main_quick_connect.glade");
	priv->quickconnect_box = GTK_BOX(gtk_builder_get_object(builder, "box_quick_connect"));
	remmina_public_gtk_widget_reparent(GTK_WIDGET(priv->quickconnect_box), GTK_CONTAINER(remmina_main_container));
	priv->quickconnect_server = GTK_ENTRY(gtk_builder_get_object(builder, "entry_quick_connect_server"));
	priv->quickconnect_protocol = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "combo_quick_connect_protocol"));
	button_quick_connect = GTK_BUTTON(gtk_builder_get_object(builder, "button_quick_connect"));
	gtk_entry_set_activates_default(priv->quickconnect_server, TRUE);
	gtk_widget_grab_default(GTK_WIDGET(button_quick_connect));
	g_signal_connect(G_OBJECT(button_quick_connect), "clicked", G_CALLBACK(remmina_main_quickconnect_on_click), remminamain);
	g_object_unref(G_OBJECT(builder));

	/* Load the GtkBuilder for the TreeModels and leave it in memory as it's used
	 * by the TreeModels and it will be needed again during the view mode change */
	remminamain->priv->builder_models = remmina_public_gtk_builder_new_from_file("remmina_main_files_list.glade");

	/* Add the ScrolledWindow */
	scrolledwindow = GTK_SCROLLED_WINDOW(gtk_builder_get_object(remminamain->priv->builder_models, "scrolled_files_list"));
	remmina_public_gtk_widget_reparent(GTK_WIDGET(scrolledwindow), GTK_CONTAINER(remmina_main_container));
	gtk_box_set_child_packing(remmina_main_container, GTK_WIDGET(scrolledwindow), TRUE, TRUE, 0, GTK_PACK_START);

	/* Add the TreeView for the files list */
	priv->file_list = GTK_TREE_VIEW(gtk_builder_get_object(remminamain->priv->builder_models, "tree_files_list"));
	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(priv->file_list), remmina_main_selection_func,
			remminamain, NULL);
	/* Handle signal for mouse buttons in order to show the popup menu */
	g_signal_connect(G_OBJECT(priv->file_list), "button-press-event", G_CALLBACK(remmina_main_file_list_on_button_press), remminamain);
	/* Handle signal for double click on the row */
	g_signal_connect(G_OBJECT(priv->file_list), "row-activated",
		G_CALLBACK(remmina_main_file_list_on_row_activated), remminamain);

	/* Add a StatusBar for selected connection or items count */
	builder = remmina_public_gtk_builder_new_from_file("remmina_main_statusbar.glade");
	priv->statusbar = GTK_STATUSBAR(gtk_builder_get_object(builder, "statusbar_main"));
	remmina_public_gtk_widget_reparent(GTK_WIDGET(priv->statusbar), GTK_CONTAINER(remmina_main_container));
	g_object_unref(G_OBJECT(builder));

	/* Load the files list */
	remmina_main_load_files(remminamain, FALSE);

	/* Load the preferences */
	if (remmina_pref.hide_toolbar)
	{
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(
			gtk_builder_get_object(priv->builder_actions, "action_view_toolbar")),
			FALSE);
	}
	if (remmina_pref.hide_statusbar)
	{
		gtk_toggle_action_set_active(
			GTK_TOGGLE_ACTION(gtk_builder_get_object(priv->builder_actions, "action_view_statusbar")),
			FALSE);
	}
	if (remmina_pref.show_quick_search)
	{
		gtk_toggle_action_set_active(
			GTK_TOGGLE_ACTION(gtk_builder_get_object(priv->builder_actions, "action_view_quick_search")),
			TRUE);
	}
	if (remmina_pref.hide_quick_connect)
	{
		gtk_toggle_action_set_active(
			GTK_TOGGLE_ACTION(
			gtk_builder_get_object(priv->builder_actions, "action_view_quick_connect")),
			FALSE);
	}
	if (remmina_pref.small_toolbutton)
	{
		gtk_toggle_action_set_active(
			GTK_TOGGLE_ACTION(gtk_builder_get_object(priv->builder_actions, "action_view_small_toolbar_buttons")),
			TRUE);
	}
	if (remmina_pref.view_file_mode)
	{
		gtk_toggle_action_set_active(
			GTK_TOGGLE_ACTION(gtk_builder_get_object(priv->builder_actions, "action_view_mode_tree")),
			TRUE);
	}
	/* Drag-n-drop support */
	gtk_drag_dest_set(GTK_WIDGET(remminamain), GTK_DEST_DEFAULT_ALL, remmina_drop_types, 1, GDK_ACTION_COPY);
	g_signal_connect(G_OBJECT(remminamain), "drag-data-received", G_CALLBACK(remmina_main_on_drag_data_received), NULL);

	priv->initialized = TRUE;

	remmina_widget_pool_register(GTK_WIDGET(remminamain));
}

GtkWidget*
remmina_main_new(void)
{
	TRACE_CALL("remmina_main_new");
	RemminaMain *window;

	window = REMMINA_MAIN(g_object_new(REMMINA_TYPE_MAIN, NULL));
	return GTK_WIDGET(window);
}

