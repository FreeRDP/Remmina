/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

G_DEFINE_TYPE( RemminaMain, remmina_main, GTK_TYPE_WINDOW)



static void remmina_main_class_init(RemminaMainClass *klass)
{
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
	if (GTK_IS_TREE_STORE(remminamain->priv->file_model))
	{
		if (remminamain->priv->expanded_group)
		{
			remmina_string_array_free(remminamain->priv->expanded_group);
		}
		remminamain->priv->expanded_group = remmina_string_array_new();
		gtk_tree_view_map_expanded_rows(GTK_TREE_VIEW(remminamain->priv->file_list),
				(GtkTreeViewMappingFunc) remmina_main_save_expanded_group_func, remminamain);
	}
}

static gboolean remmina_main_on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
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
	g_free(REMMINA_MAIN(widget)->priv->selected_filename);
	g_free(REMMINA_MAIN(widget)->priv->selected_name);
	g_free(REMMINA_MAIN(widget)->priv);
}

static void remmina_main_clear_selection_data(RemminaMain *remminamain)
{
	g_free(remminamain->priv->selected_filename);
	g_free(remminamain->priv->selected_name);
	remminamain->priv->selected_filename = NULL;
	remminamain->priv->selected_name = NULL;

	gtk_action_group_set_sensitive(remminamain->priv->file_sensitive_group, FALSE);
}

static gboolean remmina_main_selection_func(GtkTreeSelection *selection, GtkTreeModel *model, GtkTreePath *path,
		gboolean path_currently_selected, gpointer user_data)
{
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

	context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(remminamain->priv->statusbar), "status");
	gtk_statusbar_pop(GTK_STATUSBAR(remminamain->priv->statusbar), context_id);
	if (remminamain->priv->selected_filename)
	{
		gtk_action_group_set_sensitive(remminamain->priv->file_sensitive_group, TRUE);
		g_snprintf(buf, sizeof(buf), "%s (%s)", remminamain->priv->selected_name, remminamain->priv->selected_filename);
		gtk_statusbar_push(GTK_STATUSBAR(remminamain->priv->statusbar), context_id, buf);
	}
	else
	{
		gtk_statusbar_push(GTK_STATUSBAR(remminamain->priv->statusbar), context_id, remminamain->priv->selected_name);
	}
	return TRUE;
}

static void remmina_main_load_file_list_callback(gpointer data, gpointer user_data)
{
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
	GtkTreeIter *iter;
	RemminaGroupData *data;
	GNode *child;

	iter = NULL;
	if (node->data)
	{
		data = (RemminaGroupData*) node->data;
		iter = g_new0(GtkTreeIter, 1);
		gtk_tree_store_append(store, iter, parent);
		gtk_tree_store_set(store, iter, PROTOCOL_COLUMN, GTK_STOCK_DIRECTORY, NAME_COLUMN, data->name, GROUP_COLUMN,
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
	GNode *root;

	root = remmina_file_manager_get_group_tree();
	remmina_main_load_file_tree_traverse(root, store, NULL);
	remmina_file_manager_free_group_tree(root);
}

static void remmina_main_expand_group_traverse(RemminaMain *remminamain, GtkTreeIter *iter)
{
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
				gtk_tree_view_expand_row(GTK_TREE_VIEW(remminamain->priv->file_list), path, FALSE);
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
	GtkTreeIter iter;

	if (gtk_tree_model_get_iter_first(remminamain->priv->file_model_sort, &iter))
	{
		remmina_main_expand_group_traverse(remminamain, &iter);
	}
}

static gboolean remmina_main_load_file_tree_find(GtkTreeModel *tree, GtkTreeIter *iter, const gchar *match_group)
{
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
	gint columnid;
	GtkSortType order;

	gtk_tree_sortable_get_sort_column_id(sortable, &columnid, &order);
	remmina_pref.main_sort_column_id = columnid;
	remmina_pref.main_sort_order = order;
	remmina_pref_save();
}

static gboolean remmina_main_filter_visible_func(GtkTreeModel *model, GtkTreeIter *iter, RemminaMain *remminamain)
{
	gchar *text;
	gchar *protocol, *name, *group, *server, *s;
	gboolean result = TRUE;

	if (!remmina_pref.show_quick_search)
		return TRUE;

	text = g_ascii_strdown(gtk_entry_get_text(GTK_ENTRY(remminamain->priv->quick_search_entry)), -1);
	if (text && text[0])
	{
		gtk_tree_model_get(model, iter, PROTOCOL_COLUMN, &protocol, NAME_COLUMN, &name, GROUP_COLUMN, &group,
				SERVER_COLUMN, &server, -1);
		if (g_strcmp0(protocol, GTK_STOCK_DIRECTORY) != 0)
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
			gtk_tree_selection_select_iter(gtk_tree_view_get_selection(GTK_TREE_VIEW(remminamain->priv->file_list)),
					&iter);
			path = gtk_tree_model_get_path(remminamain->priv->file_model_sort, &iter);
			gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(remminamain->priv->file_list), path, NULL, TRUE, 0.5, 0.0);
			gtk_tree_path_free(path);
			return;
		}
		if (!gtk_tree_model_iter_next(remminamain->priv->file_model_sort, &iter))
			return;
	}
}

static void remmina_main_load_files(RemminaMain *remminamain, gboolean refresh)
{
	GtkTreeModel *filter;
	GtkTreeModel *sort;
	gint n;
	gchar buf[200];
	guint context_id;

	if (refresh)
	{
		remmina_main_save_expanded_group(remminamain);
	}

	switch (remmina_pref.view_file_mode)
	{

		case REMMINA_VIEW_FILE_TREE:
			gtk_tree_view_column_set_visible(remminamain->priv->group_column, FALSE);
			remminamain->priv->file_model = GTK_TREE_MODEL(
					gtk_tree_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
							G_TYPE_STRING, G_TYPE_STRING));
			remmina_main_load_file_tree_group(GTK_TREE_STORE(remminamain->priv->file_model));
			n = remmina_file_manager_iterate(remmina_main_load_file_tree_callback, remminamain);
			break;

		case REMMINA_VIEW_FILE_LIST:
		default:
			gtk_tree_view_column_set_visible(remminamain->priv->group_column, TRUE);
			remminamain->priv->file_model = GTK_TREE_MODEL(
					gtk_list_store_new(N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
							G_TYPE_STRING, G_TYPE_STRING));
			n = remmina_file_manager_iterate(remmina_main_load_file_list_callback, remminamain);
			break;
	}

	filter = gtk_tree_model_filter_new(remminamain->priv->file_model, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter),
			(GtkTreeModelFilterVisibleFunc) remmina_main_filter_visible_func, remminamain, NULL);
	remminamain->priv->file_model_filter = filter;

	sort = gtk_tree_model_sort_new_with_model(filter);
	remminamain->priv->file_model_sort = sort;

	gtk_tree_view_set_model(GTK_TREE_VIEW(remminamain->priv->file_list), sort);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(sort), remmina_pref.main_sort_column_id,
			remmina_pref.main_sort_order);
	g_signal_connect(G_OBJECT(sort), "sort-column-changed", G_CALLBACK(remmina_main_file_model_on_sort), remminamain);
	remmina_main_expand_group(remminamain);

	if (remminamain->priv->selected_filename)
	{
		remmina_main_select_file(remminamain, remminamain->priv->selected_filename);
	}

	g_snprintf(buf, 200, ngettext("Total %i item.", "Total %i items.", n), n);
	context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(remminamain->priv->statusbar), "status");
	gtk_statusbar_pop(GTK_STATUSBAR(remminamain->priv->statusbar), context_id);
	gtk_statusbar_push(GTK_STATUSBAR(remminamain->priv->statusbar), context_id, buf);
}

static void remmina_main_action_connection_connect(GtkAction *action, RemminaMain *remminamain)
{
	remmina_connection_window_open_from_filename(remminamain->priv->selected_filename);
}

static void remmina_main_action_connection_external_tools(GtkAction *action, RemminaMain *remminamain)
{
	remmina_external_tools_from_filename(remminamain, remminamain->priv->selected_filename);
}

static void remmina_main_file_editor_destroy(GtkWidget *widget, RemminaMain *remminamain)
{
	if (GTK_IS_WIDGET(remminamain))
	{
		remmina_main_load_files(remminamain, TRUE);
	}
}

static void remmina_main_action_connection_new(GtkAction *action, RemminaMain *remminamain)
{
	GtkWidget *widget;

	widget = remmina_file_editor_new();
	g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(remmina_main_file_editor_destroy), remminamain);
	gtk_widget_show(widget);
}

static void remmina_main_action_connection_copy(GtkAction *action, RemminaMain *remminamain)
{
	GtkWidget *widget;

	if (remminamain->priv->selected_filename == NULL)
		return;

	widget = remmina_file_editor_new_copy(remminamain->priv->selected_filename);
	if (widget)
	{
		g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(remmina_main_file_editor_destroy), remminamain);
		gtk_widget_show(widget);
	}
}

static void remmina_main_action_connection_edit(GtkAction *action, RemminaMain *remminamain)
{
	GtkWidget *widget;

	if (remminamain->priv->selected_filename == NULL)
		return;

	widget = remmina_file_editor_new_from_filename(remminamain->priv->selected_filename);
	if (widget)
	{
		g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(remmina_main_file_editor_destroy), remminamain);
		gtk_widget_show(widget);
	}
}

static void remmina_main_action_connection_delete(GtkAction *action, RemminaMain *remminamain)
{
	GtkWidget *dialog;

	if (remminamain->priv->selected_filename == NULL)
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
	GtkWidget *widget;

	widget = remmina_pref_dialog_new(0);
	gtk_widget_show(widget);
}

static void remmina_main_action_connection_close(GtkAction *action, RemminaMain *remminamain)
{
	gtk_widget_destroy(GTK_WIDGET(remminamain));
}

static void remmina_main_action_view_toolbar(GtkToggleAction *action, RemminaMain *remminamain)
{
	gboolean toggled;

	toggled = gtk_toggle_action_get_active(action);
	if (toggled)
	{
		gtk_widget_show(remminamain->priv->toolbar);
	}
	else
	{
		gtk_widget_hide(remminamain->priv->toolbar);
	}
	if (remminamain->priv->initialized)
	{
		remmina_pref.hide_toolbar = !toggled;
		remmina_pref_save();
	}
}

static void remmina_main_action_view_quick_search(GtkToggleAction *action, RemminaMain *remminamain)
{
	gboolean toggled;

	toggled = gtk_toggle_action_get_active(action);
	if (toggled)
	{
		gtk_entry_set_text(GTK_ENTRY(remminamain->priv->quick_search_entry), "");
		gtk_widget_show(GTK_WIDGET(remminamain->priv->quick_search_separator));
		gtk_widget_show(GTK_WIDGET(remminamain->priv->quick_search_item));
		gtk_widget_grab_focus(remminamain->priv->quick_search_entry);
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
			gtk_tree_view_expand_all(GTK_TREE_VIEW(remminamain->priv->file_list));
		}
	}
}

static void remmina_main_action_view_statusbar(GtkToggleAction *action, RemminaMain *remminamain)
{
	gboolean toggled;

	toggled = gtk_toggle_action_get_active(action);
	if (toggled)
	{
		gtk_widget_show(remminamain->priv->statusbar);
	}
	else
	{
		gtk_widget_hide(remminamain->priv->statusbar);
	}
	if (remminamain->priv->initialized)
	{
		remmina_pref.hide_statusbar = !toggled;
		remmina_pref_save();
	}
}

static void remmina_main_action_view_small_toolbutton(GtkToggleAction *action, RemminaMain *remminamain)
{
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

static void remmina_main_action_view_file_mode(GtkRadioAction *action, GtkRadioAction *current, RemminaMain *remminamain)
{
	remmina_pref.view_file_mode = gtk_radio_action_get_current_value(action);
	remmina_pref_save();
	remmina_main_load_files(remminamain, TRUE);
}

static void remmina_main_import_file_list(RemminaMain *remminamain, GSList *files)
{
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
	GtkWidget *dialog;

	dialog = gtk_file_chooser_dialog_new(_("Import"), GTK_WINDOW(remminamain), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN,
			GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(remmina_main_action_tools_import_on_response), remminamain);
	gtk_widget_show(dialog);
}

static void remmina_main_action_tools_export(GtkAction *action, RemminaMain *remminamain)
{
	RemminaFilePlugin *plugin;
	RemminaFile *remminafile;
	GtkWidget *dialog;

	if (remminamain->priv->selected_filename == NULL)
		return;

	remminafile = remmina_file_load(remminamain->priv->selected_filename);
	if (remminafile == NULL)
		return;
	plugin = remmina_plugin_manager_get_export_file_handler(remminafile);
	if (plugin)
	{
		dialog = gtk_file_chooser_dialog_new(plugin->export_hints, GTK_WINDOW(remminamain),
				GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
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
	remmina_plugin_manager_show(GTK_WINDOW(remminamain));
}

static void remmina_main_action_tools_addition(GtkAction *action, RemminaMain *remminamain)
{
	RemminaToolPlugin *plugin;

	plugin = (RemminaToolPlugin *) remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_TOOL, gtk_action_get_name(action));
	if (plugin)
	{
		plugin->exec_func();
	}
}

static void remmina_main_action_help_homepage(GtkAction *action, RemminaMain *remminamain)
{
	g_app_info_launch_default_for_uri("http://remmina.sourceforge.net", NULL, NULL);
}

static void remmina_main_action_help_wiki(GtkAction *action, RemminaMain *remminamain)
{
	g_app_info_launch_default_for_uri("http://sourceforge.net/apps/mediawiki/remmina/", NULL, NULL);
}

static void remmina_main_action_help_debug(GtkAction *action, RemminaMain *remminamain)
{
	remmina_log_start();
}

static void remmina_main_action_help_about(GtkAction *action, RemminaMain *remminamain)
{
	remmina_about_open(GTK_WINDOW(remminamain));
}

static const gchar *remmina_main_ui_xml = "<ui>"
		"  <menubar name='MenuBar'>"
		"    <menu name='ConnectionMenu' action='Connection'>"
		"      <menuitem name='ConnectionConnectMenu' action='ConnectionConnect'/>"
		"      <separator/>"
		"      <menuitem name='ConnectionNewMenu' action='ConnectionNew'/>"
		"      <menuitem name='ConnectionCopyMenu' action='ConnectionCopy'/>"
		"      <menuitem name='ConnectionEditMenu' action='ConnectionEdit'/>"
		"      <menuitem name='ConnectionDeleteMenu' action='ConnectionDelete'/>"
		"      <separator/>"
		"      <menuitem name='ConnectionCloseMenu' action='ConnectionClose'/>"
		"    </menu>"
		"    <menu name='EditMenu' action='Edit'>"
		"      <menuitem name='EditPreferencesMenu' action='EditPreferences'/>"
		"    </menu>"
		"    <menu name='ViewMenu' action='View'>"
		"      <menuitem name='ViewToolbarMenu' action='ViewToolbar'/>"
		"      <menuitem name='ViewStatusbarMenu' action='ViewStatusbar'/>"
		"      <menuitem name='ViewQuickSearchMenu' action='ViewQuickSearch'/>"
		"      <separator/>"
		"      <menuitem name='ViewSmallToolbuttonMenu' action='ViewSmallToolbutton'/>"
		"      <separator/>"
		"      <menuitem name='ViewFileListMenu' action='ViewFileList'/>"
		"      <menuitem name='ViewFileTreeMenu' action='ViewFileTree'/>"
		"    </menu>"
		"    <menu name='ToolsMenu' action='Tools'>"
		"      <menuitem name='ToolsImportMenu' action='ToolsImport'/>"
		"      <menuitem name='ToolsExportMenu' action='ToolsExport'/>"
		"      <placeholder name='ToolsAdditions'/>"
		"      <separator/>"
		"      <menuitem name='ToolsPluginsMenu' action='ToolsPlugins'/>"
		"    </menu>"
		"    <menu name='HelpMenu' action='Help'>"
		"      <menuitem name='HelpHomepageMenu' action='HelpHomepage'/>"
		"      <menuitem name='HelpWikiMenu' action='HelpWiki'/>"
		"      <menuitem name='HelpDebugMenu' action='HelpDebug'/>"
		"      <separator/>"
		"      <menuitem name='HelpAboutMenu' action='HelpAbout'/>"
		"    </menu>"
		"  </menubar>"
		"  <toolbar name='ToolBar'>"
		"    <toolitem action='ConnectionConnect'/>"
		"    <separator/>"
		"    <toolitem action='ConnectionNew'/>"
		"    <toolitem action='ConnectionCopy'/>"
		"    <toolitem action='ConnectionEdit'/>"
		"    <toolitem action='ConnectionDelete'/>"
		"    <separator/>"
		"    <toolitem action='EditPreferences'/>"
		"  </toolbar>"
		"  <popup name='PopupMenu'>"
		"    <menuitem action='ConnectionConnect'/>"
		"    <separator/>"
		"    <menuitem action='ConnectionCopy'/>"
		"    <menuitem action='ConnectionEdit'/>"
		"    <menuitem action='ConnectionDelete'/>"
		"    <menuitem action='ConnectionExternalTools'/>"
		"  </popup>"
		"</ui>";

static const GtkActionEntry remmina_main_ui_menu_entries[] =
{
{ "Connection", NULL, N_("_Connection") },
{ "Edit", NULL, N_("_Edit") },
{ "View", NULL, N_("_View") },
{ "Tools", NULL, N_("_Tools") },
{ "Help", NULL, N_("_Help") },

{ "ConnectionNew", GTK_STOCK_NEW, NULL, "<control>N", N_("Create a new remote desktop file"), G_CALLBACK(
		remmina_main_action_connection_new) },

{ "EditPreferences", GTK_STOCK_PREFERENCES, NULL, "<control>P", N_("Open the preferences dialog"), G_CALLBACK(
		remmina_main_action_edit_preferences) },

{ "ConnectionClose", GTK_STOCK_CLOSE, NULL, "<control>X", NULL, G_CALLBACK(remmina_main_action_connection_close) },

{ "ToolsImport", NULL, N_("Import"), NULL, NULL, G_CALLBACK(remmina_main_action_tools_import) },

{ "ToolsPlugins", NULL, N_("Plugins"), NULL, NULL, G_CALLBACK(remmina_main_action_tools_plugins) },

{ "HelpHomepage", NULL, N_("Homepage"), NULL, NULL, G_CALLBACK(remmina_main_action_help_homepage) },

{ "HelpWiki", NULL, N_("Online Wiki"), NULL, NULL, G_CALLBACK(remmina_main_action_help_wiki) },

{ "HelpDebug", NULL, N_("Debug Window"), NULL, NULL, G_CALLBACK(remmina_main_action_help_debug) },

{ "HelpAbout", GTK_STOCK_ABOUT, NULL, NULL, NULL, G_CALLBACK(remmina_main_action_help_about) } };

static const GtkActionEntry remmina_main_ui_file_sensitive_menu_entries[] =
{
{ "ConnectionConnect", GTK_STOCK_CONNECT, NULL, "<control>O", N_("Open the connection to the selected remote desktop file"),
		G_CALLBACK(remmina_main_action_connection_connect) },

{ "ConnectionCopy", GTK_STOCK_COPY, NULL, "<control>C", N_("Create a copy of the selected remote desktop file"), G_CALLBACK(
		remmina_main_action_connection_copy) },

{ "ConnectionEdit", GTK_STOCK_EDIT, NULL, "<control>E", N_("Edit the selected remote desktop file"), G_CALLBACK(
		remmina_main_action_connection_edit) },

{ "ConnectionDelete", GTK_STOCK_DELETE, NULL, "<control>D", N_("Delete the selected remote desktop file"), G_CALLBACK(
		remmina_main_action_connection_delete) },

{ "ToolsExport", NULL, N_("Export"), NULL, NULL, G_CALLBACK(remmina_main_action_tools_export) },

{ "ConnectionExternalTools", NULL, N_("External Tools"), "<control>T", NULL,
		G_CALLBACK(remmina_main_action_connection_external_tools) }

};

static const GtkToggleActionEntry remmina_main_ui_toggle_menu_entries[] =
{
{ "ViewToolbar", NULL, N_("Toolbar"), NULL, NULL, G_CALLBACK(remmina_main_action_view_toolbar), TRUE },

{ "ViewStatusbar", NULL, N_("Statusbar"), NULL, NULL, G_CALLBACK(remmina_main_action_view_statusbar), TRUE },

{ "ViewQuickSearch", NULL, N_("Quick Search"), NULL, NULL, G_CALLBACK(remmina_main_action_view_quick_search), FALSE },

{ "ViewSmallToolbutton", NULL, N_("Small Toolbar Buttons"), NULL, NULL, G_CALLBACK(remmina_main_action_view_small_toolbutton),
		FALSE } };

static const GtkRadioActionEntry remmina_main_ui_view_file_mode_entries[] =
{
{ "ViewFileList", NULL, N_("List View"), NULL, NULL, REMMINA_VIEW_FILE_LIST },
{ "ViewFileTree", NULL, N_("Tree View"), NULL, NULL, REMMINA_VIEW_FILE_TREE } };

static gboolean remmina_main_quickconnect(RemminaMain *remminamain)
{
	RemminaFile* remminafile;
	gint index;
	gchar* server;
	gchar* protocol;

	remminafile = remmina_file_new();
	server = strdup(gtk_entry_get_text(GTK_ENTRY(remminamain->priv->quickconnect_server)));
	index = gtk_combo_box_get_active(GTK_COMBO_BOX(remminamain->priv->quickconnect_protocol));

	switch (index)
	{
		case 0:
			protocol = "RDP";
			break;
		case 1:
			protocol = "VNC";
			break;
		case 2:
			protocol = "NX";
			break;
		case 3:
			protocol = "SSH";
			break;
		default:
			protocol = "RDP";
			break;
	}

	remmina_file_set_string(remminafile, "sound", "off");
	remmina_file_set_string(remminafile, "server", server);
	remmina_file_set_string(remminafile, "name", server);
	remmina_file_set_string(remminafile, "protocol", protocol);

	remmina_connection_window_open_from_file(remminafile);

	return FALSE;
}
static gboolean remmina_main_quickconnect_on_click(GtkWidget *widget, RemminaMain *remminamain)
{
	return remmina_main_quickconnect(remminamain);
}
static gboolean remmina_main_quickconnect_on_key_press(GtkWidget *widget, GdkEventKey *event, RemminaMain *remminamain)
{
#if GTK_VERSION == 3
    if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter))
#else
    if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter))
#endif
    {
		return remmina_main_quickconnect(remminamain);
    }
    return FALSE;
}

static gboolean remmina_main_file_list_on_button_press(GtkWidget *widget, GdkEventButton *event, RemminaMain *remminamain)
{
	GtkWidget *popup;

	if (event->button == 3)
	{
		popup = gtk_ui_manager_get_widget(remminamain->priv->uimanager, "/PopupMenu");
		if (popup)
		{
			gtk_menu_popup(GTK_MENU(popup), NULL, NULL, NULL, NULL, event->button, event->time);
		}
	}
	else
		if (event->type == GDK_2BUTTON_PRESS && remminamain->priv->selected_filename)
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
	return FALSE;
}
static gboolean remmina_main_file_list_on_key_press(GtkWidget *widget, GdkEventKey *event, RemminaMain *remminamain)
{
#if GTK_VERSION == 3
	if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_KEY_Return || event->keyval == GDK_KEY_KP_Enter) && remminamain->priv->selected_filename)
#else
	if (event->type == GDK_KEY_PRESS && (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter) && remminamain->priv->selected_filename)
#endif
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
	return FALSE;
}

static void remmina_main_quick_search_on_icon_press(GtkEntry *entry, GtkEntryIconPosition icon_pos, GdkEvent *event,
		RemminaMain *remminamain)
{
	if (icon_pos == GTK_ENTRY_ICON_SECONDARY)
	{
		gtk_entry_set_text(entry, "");
	}
}

static void remmina_main_quick_search_on_changed(GtkEditable *editable, RemminaMain *remminamain)
{
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(remminamain->priv->file_model_filter));
	gtk_tree_view_expand_all(GTK_TREE_VIEW(remminamain->priv->file_list));
}

static void remmina_main_create_quick_search(RemminaMain *remminamain)
{
	GtkWidget *widget;
	GValue val =
	{ 0 };

	remminamain->priv->quick_search_separator = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(remminamain->priv->toolbar), remminamain->priv->quick_search_separator, -1);

	remminamain->priv->quick_search_item = gtk_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(remminamain->priv->toolbar), remminamain->priv->quick_search_item, -1);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(widget), GTK_ENTRY_ICON_PRIMARY, GTK_STOCK_FIND);
	gtk_entry_set_icon_from_stock(GTK_ENTRY(widget), GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_CLEAR);
	gtk_entry_set_width_chars(GTK_ENTRY(widget), 25);
	gtk_container_add(GTK_CONTAINER(remminamain->priv->quick_search_item), widget);

	g_value_init(&val, G_TYPE_BOOLEAN);
	g_value_set_boolean(&val, FALSE);
	g_object_set_property(G_OBJECT(widget), "primary-icon-activatable", &val);
	g_value_unset(&val);

	g_signal_connect(G_OBJECT(widget), "icon-press", G_CALLBACK(remmina_main_quick_search_on_icon_press), remminamain);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(remmina_main_quick_search_on_changed), remminamain);

	remminamain->priv->quick_search_entry = widget;
}

static void remmina_main_on_drag_data_received(RemminaMain *remminamain, GdkDragContext *drag_context, gint x, gint y,
		GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
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

static gboolean remmina_main_add_tool_plugin(gchar *name, RemminaPlugin *plugin, gpointer data)
{
	RemminaMain *remminamain = REMMINA_MAIN(data);
	guint merge_id;
	GtkAction *action;

	merge_id = gtk_ui_manager_new_merge_id(remminamain->priv->uimanager);
	action = gtk_action_new(name, plugin->description, NULL, NULL);
	gtk_action_group_add_action(remminamain->priv->main_group, action);
	g_signal_connect(G_OBJECT(action), "activate", G_CALLBACK(remmina_main_action_tools_addition), remminamain);
	g_object_unref(action);

	gtk_ui_manager_add_ui(remminamain->priv->uimanager, merge_id, "/MenuBar/ToolsMenu/ToolsAdditions", name, name,
			GTK_UI_MANAGER_MENUITEM, FALSE);

	return FALSE;
}

static gboolean remmina_main_on_window_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer user_data)
{
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
	return FALSE;
}

static void remmina_main_init(RemminaMain *remminamain)
{
	RemminaMainPriv *priv;
	GtkWidget *vbox;
	GtkWidget *menubar;
	GtkWidget *hbox;
	GtkWidget *quickconnect;
	GtkWidget *tool_item;
	GtkUIManager *uimanager;
	GtkActionGroup *action_group;
	GtkWidget *scrolledwindow;
	GtkWidget *tree;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GError *error;

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
#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_container_add(GTK_CONTAINER(remminamain), vbox);
	gtk_widget_show(vbox);

	/* Create the menubar and toolbar */
	uimanager = gtk_ui_manager_new();
	priv->uimanager = uimanager;

	action_group = gtk_action_group_new("RemminaMainActions");
	gtk_action_group_set_translation_domain(action_group, NULL);
	gtk_action_group_add_actions(action_group, remmina_main_ui_menu_entries, G_N_ELEMENTS(remmina_main_ui_menu_entries),
			remminamain);
	gtk_action_group_add_toggle_actions(action_group, remmina_main_ui_toggle_menu_entries,
			G_N_ELEMENTS(remmina_main_ui_toggle_menu_entries), remminamain);
	gtk_action_group_add_radio_actions(action_group, remmina_main_ui_view_file_mode_entries,
			G_N_ELEMENTS(remmina_main_ui_view_file_mode_entries), remmina_pref.view_file_mode,
			G_CALLBACK(remmina_main_action_view_file_mode), remminamain);

	gtk_ui_manager_insert_action_group(uimanager, action_group, 0);
	g_object_unref(action_group);
	priv->main_group = action_group;

	action_group = gtk_action_group_new("RemminaMainFileSensitiveActions");
	gtk_action_group_set_translation_domain(action_group, NULL);
	gtk_action_group_add_actions(action_group, remmina_main_ui_file_sensitive_menu_entries,
			G_N_ELEMENTS(remmina_main_ui_file_sensitive_menu_entries), remminamain);

	gtk_ui_manager_insert_action_group(uimanager, action_group, 0);
	g_object_unref(action_group);
	priv->file_sensitive_group = action_group;

	error = NULL;
	gtk_ui_manager_add_ui_from_string(uimanager, remmina_main_ui_xml, -1, &error);
	if (error)
	{
		g_message("building menus failed: %s", error->message);
		g_error_free(error);
	}

	remmina_plugin_manager_for_each_plugin(REMMINA_PLUGIN_TYPE_TOOL, remmina_main_add_tool_plugin, remminamain);

	menubar = gtk_ui_manager_get_widget(uimanager, "/MenuBar");
	gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);

	priv->toolbar = gtk_ui_manager_get_widget(uimanager, "/ToolBar");
#if GTK_VERSION == 3
	gtk_style_context_add_class(gtk_widget_get_style_context(priv->toolbar), GTK_STYLE_CLASS_PRIMARY_TOOLBAR);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), priv->toolbar, FALSE, FALSE, 0);

	tool_item = gtk_ui_manager_get_widget(uimanager, "/ToolBar/ConnectionConnect");
	gtk_tool_item_set_is_important (GTK_TOOL_ITEM(tool_item), TRUE);

	tool_item = gtk_ui_manager_get_widget(uimanager, "/ToolBar/ConnectionNew");
	gtk_tool_item_set_is_important (GTK_TOOL_ITEM(tool_item), TRUE);

	remmina_main_create_quick_search(remminamain);

	gtk_window_add_accel_group(GTK_WINDOW(remminamain), gtk_ui_manager_get_accel_group(uimanager));

	gtk_action_group_set_sensitive(priv->file_sensitive_group, FALSE);

	/* Add a Fast Connection box */
#if GTK_VERSION == 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#elif GTK_VERSION == 2
	hbox = gtk_hbox_new(FALSE, 0);
#endif

	
#if GTK_VERSION == 3
	priv->quickconnect_protocol = gtk_combo_box_text_new();
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(priv->quickconnect_protocol), "RDP", "RDP");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(priv->quickconnect_protocol), "VNC", "VNC");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(priv->quickconnect_protocol), "NX", "NX");
	gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(priv->quickconnect_protocol), "SSH", "SSH");
#elif GTK_VERSION == 2
	priv->quickconnect_protocol = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(priv->quickconnect_protocol), "RDP");
	gtk_combo_box_append_text(GTK_COMBO_BOX(priv->quickconnect_protocol), "VNC");
	gtk_combo_box_append_text(GTK_COMBO_BOX(priv->quickconnect_protocol), "NX");
	gtk_combo_box_append_text(GTK_COMBO_BOX(priv->quickconnect_protocol), "SSH");
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(priv->quickconnect_protocol), 0);
	gtk_widget_show(priv->quickconnect_protocol);
	gtk_box_pack_start(GTK_BOX(hbox), priv->quickconnect_protocol, FALSE, FALSE, 0);

	priv->quickconnect_server = gtk_entry_new();
	gtk_entry_set_width_chars(GTK_ENTRY(priv->quickconnect_server), 25);
	gtk_widget_show(priv->quickconnect_server);
	gtk_box_pack_start(GTK_BOX(hbox), priv->quickconnect_server, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(priv->quickconnect_server), "key-press-event", G_CALLBACK(remmina_main_quickconnect_on_key_press), remminamain);

	quickconnect = gtk_button_new_with_label("Connect !");
	gtk_widget_show(quickconnect);
	gtk_box_pack_start(GTK_BOX(hbox), quickconnect, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(quickconnect), "clicked", G_CALLBACK(remmina_main_quickconnect_on_click), remminamain);

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	gtk_widget_show(hbox);

	/* Create the scrolled window for the file list */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(vbox), scrolledwindow, TRUE, TRUE, 0);

	/* Create the remmina file list */
	tree = gtk_tree_view_new();

	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Name"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, NAME_COLUMN);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "icon-name", PROTOCOL_COLUMN);
	g_object_set(G_OBJECT(renderer), "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", NAME_COLUMN);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Group"), renderer, "text", GROUP_COLUMN, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, GROUP_COLUMN);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);
	priv->group_column = column;

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Server"), renderer, "text", SERVER_COLUMN, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, SERVER_COLUMN);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	gtk_container_add(GTK_CONTAINER(scrolledwindow), tree);
	gtk_widget_show(tree);

	gtk_tree_selection_set_select_function(gtk_tree_view_get_selection(GTK_TREE_VIEW(tree)), remmina_main_selection_func,
			remminamain, NULL);
	g_signal_connect(G_OBJECT(tree), "button-press-event", G_CALLBACK(remmina_main_file_list_on_button_press), remminamain);
	g_signal_connect(G_OBJECT(tree), "key-press-event", G_CALLBACK(remmina_main_file_list_on_key_press), remminamain);

	priv->file_list = tree;

	/* Create statusbar */
	priv->statusbar = gtk_statusbar_new();
	gtk_box_pack_start(GTK_BOX(vbox), priv->statusbar, FALSE, FALSE, 0);
	gtk_widget_show(priv->statusbar);

	/* Prepare the data */
	remmina_main_load_files(remminamain, FALSE);

	/* Load the preferences */
	if (remmina_pref.hide_toolbar)
	{
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(priv->main_group, "ViewToolbar")),
				FALSE);
	}
	if (remmina_pref.hide_statusbar)
	{
		gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(gtk_action_group_get_action(priv->main_group, "ViewStatusbar")),
				FALSE);
	}
	if (remmina_pref.show_quick_search)
	{
		gtk_toggle_action_set_active(
				GTK_TOGGLE_ACTION(gtk_action_group_get_action(priv->main_group, "ViewQuickSearch")), TRUE);
	}
	if (remmina_pref.small_toolbutton)
	{
		gtk_toggle_action_set_active(
				GTK_TOGGLE_ACTION(gtk_action_group_get_action(priv->main_group, "ViewSmallToolbutton")), TRUE);
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
	RemminaMain *window;

	window = REMMINA_MAIN(g_object_new(REMMINA_TYPE_MAIN, NULL));
	return GTK_WIDGET(window);
}

