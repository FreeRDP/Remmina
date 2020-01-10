/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee
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

#include <gtk/gtk.h>
#include <string.h>
#include "config.h"
#include "remmina_public.h"
#include "remmina_string_list.h"
#include "remmina/remmina_trace_calls.h"

static RemminaStringList *string_list;

#define COLUMN_DESCRIPTION 0
#define COLUMN_VALUE 1
#define GET_OBJECT(object_name) gtk_builder_get_object(string_list->builder, object_name)

/* Update the buttons state on the items in the TreeModel */
void remmina_string_list_update_buttons_state(void)
{
	gint items_count = gtk_tree_model_iter_n_children(
		GTK_TREE_MODEL(string_list->liststore_items), NULL);

	gtk_widget_set_sensitive(GTK_WIDGET(string_list->button_remove), items_count > 0);
	gtk_widget_set_sensitive(GTK_WIDGET(string_list->button_up), items_count > 1);
	gtk_widget_set_sensitive(GTK_WIDGET(string_list->button_down), items_count > 1);
}

/* Check the text inserted in the list */
void remmina_string_list_on_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text)
{
	TRACE_CALL(__func__);
	gchar *text;
	gchar *error;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_string);
	GtkTreeIter iter;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(string_list->liststore_items), &iter, path);
	/* Remove delimitors from the string */
	text = remmina_public_str_replace(new_text, STRING_DELIMITOR, " ");
	if (cell == string_list->cellrenderertext_item1) {
		gtk_list_store_set(string_list->liststore_items, &iter, COLUMN_DESCRIPTION, text, -1);
	}else  {
		/* Check for validation only in second field */
		if (string_list->priv->validation_func) {
			if (!((*string_list->priv->validation_func)(text, &error))) {
				gtk_label_set_text(string_list->label_status, error);
				gtk_widget_show(GTK_WIDGET(string_list->label_status));
				g_free(error);
			}else  {
				gtk_widget_hide(GTK_WIDGET(string_list->label_status));
			}
		}
		gtk_list_store_set(string_list->liststore_items, &iter, COLUMN_VALUE, text, -1);
	}
	gtk_tree_path_free(path);
	g_free(text);
}

/* Move a TreeIter position */
static void remmina_string_list_move_iter(GtkTreeIter *from, GtkTreeIter *to)
{
	TRACE_CALL(__func__);
	GtkTreePath *path;

	gtk_list_store_swap(string_list->liststore_items, from, to);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(string_list->liststore_items), from);
	gtk_tree_view_scroll_to_cell(string_list->treeview_items, path, NULL, 0, 0, 0);
	gtk_tree_path_free(path);
}

/* Move down the selected TreeRow */
void remmina_string_list_on_action_down(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;
	GtkTreeIter target_iter;

	if (gtk_tree_selection_get_selected(string_list->treeview_selection, NULL, &iter)) {
		gtk_tree_selection_get_selected(string_list->treeview_selection, NULL, &target_iter);
		if (gtk_tree_model_iter_next(GTK_TREE_MODEL(string_list->liststore_items), &target_iter)) {
			remmina_string_list_move_iter(&iter, &target_iter);
		}
	}
}

/* Move up the selected TreeRow */
void remmina_string_list_on_action_up(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;
	GtkTreeIter target_iter;
	GtkTreePath *path;

	if (gtk_tree_selection_get_selected(string_list->treeview_selection, NULL, &iter)) {
		gtk_tree_selection_get_selected(string_list->treeview_selection, NULL, &target_iter);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(string_list->liststore_items), &target_iter);
		/* Before moving the TreeRow check if there’s a previous item */
		if (gtk_tree_path_prev(path)) {
			gtk_tree_model_get_iter(GTK_TREE_MODEL(string_list->liststore_items), &target_iter, path);
			gtk_tree_path_free(path);
			remmina_string_list_move_iter(&iter, &target_iter);
		}
	}
}

/* Add a new TreeRow to the list */
void remmina_string_list_on_action_add(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;
	GtkTreePath *path;

	gtk_list_store_append(string_list->liststore_items, &iter);
	gtk_tree_selection_select_iter(string_list->treeview_selection, &iter);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(string_list->liststore_items), &iter);
	gtk_tree_view_set_cursor_on_cell(string_list->treeview_items, path,
		string_list->treeviewcolumn_item,
		GTK_CELL_RENDERER(string_list->priv->two_columns ? string_list->cellrenderertext_item1 : string_list->cellrenderertext_item2),
		TRUE);
	gtk_tree_path_free(path);
	remmina_string_list_update_buttons_state();
}

/* Remove the selected TreeRow from the list */
void remmina_string_list_on_action_remove(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;

	if (gtk_tree_selection_get_selected(string_list->treeview_selection, NULL, &iter)) {
		gtk_list_store_remove(string_list->liststore_items, &iter);
	}
	gtk_widget_hide(GTK_WIDGET(string_list->label_status));
	remmina_string_list_update_buttons_state();
}

/* Load a string list by splitting a string value */
void remmina_string_list_set_text(const gchar *text, const gboolean clear_data)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;
	gchar **items;
	gchar **values;
	gint i;
	/* Clear the data before to load new items */
	if (clear_data)
		gtk_list_store_clear(string_list->liststore_items);
	/* Split the string and insert each snippet in the string list */
	items = g_strsplit(text, STRING_DELIMITOR, -1);
	for (i = 0; i < g_strv_length(items); i++) {
		values = g_strsplit(items[i], string_list->priv->fields_separator, -1);
		gtk_list_store_append(string_list->liststore_items, &iter);
		if (g_strv_length(values) > 1) {
			/* Two columns data */
			gtk_list_store_set(string_list->liststore_items, &iter,
				COLUMN_DESCRIPTION, values[0],
				COLUMN_VALUE, values[1],
				-1);
		}else  {
			/* Single column data */
			gtk_list_store_set(string_list->liststore_items, &iter,
				COLUMN_VALUE, values[0],
				-1);
		}
		g_strfreev(values);
	}
	g_strfreev(items);
	remmina_string_list_update_buttons_state();
}

/* Get a string value representing the string list */
gchar* remmina_string_list_get_text(void)
{
	TRACE_CALL(__func__);
	GString *str;
	GtkTreeIter iter;
	gboolean first;
	gboolean ret;
	const gchar *item_description;
	const gchar *item_value;

	str = g_string_new(NULL);
	first = TRUE;
	/* Cycle each GtkTreeIter in the ListStore */
	ret = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(string_list->liststore_items), &iter);
	while (ret) {
		gtk_tree_model_get(GTK_TREE_MODEL(string_list->liststore_items), &iter,
			COLUMN_DESCRIPTION, &item_description,
			COLUMN_VALUE, &item_value,
			-1);
		if (!item_description)
			item_description = "";
		if (item_value && strlen(item_value) > 0) {
			/* Add a delimitor after the first element */
			if (!first) {
				g_string_append(str, STRING_DELIMITOR);
			}else  {
				first = FALSE;
			}
			/* Add the description for two columns list */
			if (string_list->priv->two_columns) {
				g_string_append(str, item_description);
				g_string_append(str, string_list->priv->fields_separator);
			}
			/* Add the element to the string */
			g_string_append(str, item_value);
		}
		ret = gtk_tree_model_iter_next(GTK_TREE_MODEL(string_list->liststore_items), &iter);
	}
	return g_string_free(str, FALSE);
}

/* Set a function that will be used to validate the new rows */
void remmina_string_list_set_validation_func(RemminaStringListValidationFunc func)
{
	TRACE_CALL(__func__);
	string_list->priv->validation_func = func;
}

/* Set the dialog titles */
void remmina_string_list_set_titles(gchar *title1, gchar *title2)
{
	/* Set dialog titlebar */
	gtk_window_set_title(GTK_WINDOW(string_list->dialog),
		(title1 && strlen(title1) > 0) ? title1 : "");
	/* Set title label */
	if (title2 && strlen(title2) > 0) {
		gtk_label_set_text(string_list->label_title, title2);
		gtk_widget_show(GTK_WIDGET(string_list->label_title));
	}else  {
		gtk_widget_hide(GTK_WIDGET(string_list->label_title));
	}
}

/* RemminaStringList initialization */
static void remmina_string_list_init(void)
{
	TRACE_CALL(__func__);
	string_list->priv->validation_func = NULL;
	/* When two columns are requested, show also the first column */
	if (string_list->priv->two_columns)
		gtk_cell_renderer_set_visible(GTK_CELL_RENDERER(string_list->cellrenderertext_item1), TRUE);
	remmina_string_list_update_buttons_state();
}

/* RemminaStringList instance */
GtkDialog* remmina_string_list_new(gboolean two_columns, const gchar *fields_separator)
{
	TRACE_CALL(__func__);
	string_list = g_new0(RemminaStringList, 1);
	string_list->priv = g_new0(RemminaStringListPriv, 1);

	string_list->builder = remmina_public_gtk_builder_new_from_file("remmina_string_list.glade");
	string_list->dialog = GTK_DIALOG(gtk_builder_get_object(string_list->builder, "DialogStringList"));

	string_list->liststore_items = GTK_LIST_STORE(GET_OBJECT("liststore_items"));
	string_list->treeview_items = GTK_TREE_VIEW(GET_OBJECT("treeview_items"));
	string_list->treeviewcolumn_item = GTK_TREE_VIEW_COLUMN(GET_OBJECT("treeviewcolumn_item"));
	string_list->treeview_selection = GTK_TREE_SELECTION(GET_OBJECT("treeview_selection"));
	string_list->cellrenderertext_item1 = GTK_CELL_RENDERER_TEXT(GET_OBJECT("cellrenderertext_item1"));
	string_list->cellrenderertext_item2 = GTK_CELL_RENDERER_TEXT(GET_OBJECT("cellrenderertext_item2"));
	string_list->button_add = GTK_BUTTON(GET_OBJECT("button_add"));
	string_list->button_remove = GTK_BUTTON(GET_OBJECT("button_remove"));
	string_list->button_up = GTK_BUTTON(GET_OBJECT("button_up"));
	string_list->button_down = GTK_BUTTON(GET_OBJECT("button_down"));
	string_list->label_title = GTK_LABEL(GET_OBJECT("label_title"));
	string_list->label_status = GTK_LABEL(GET_OBJECT("label_status"));

	/* Connect signals */
	gtk_builder_connect_signals(string_list->builder, NULL);
	/* Initialize the window and load the values */
	if (!fields_separator)
		fields_separator = STRING_DELIMITOR2;
	string_list->priv->fields_separator = fields_separator;
	string_list->priv->two_columns = two_columns;
	remmina_string_list_init();

	return string_list->dialog;
}
