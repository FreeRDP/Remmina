/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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

#include <gtk/gtk.h>
#include <string.h>
#include "remmina_public.h"
#include "remmina_string_list.h"

G_DEFINE_TYPE( RemminaStringList, remmina_string_list, GTK_TYPE_TABLE)

#define ERROR_COLOR "red"

enum
{
	COLUMN_TEXT,
	COLUMN_COLOR,
	NUM_COLUMNS
};

static void remmina_string_list_status_error(RemminaStringList *gsl, const gchar *error)
{
	GdkColor color;

	gdk_color_parse(ERROR_COLOR, &color);
	gtk_widget_modify_fg(gsl->status_label, GTK_STATE_NORMAL, &color);
	gtk_label_set_text(GTK_LABEL(gsl->status_label), error);
}

static void remmina_string_list_status_hints(RemminaStringList *gsl)
{
	gtk_widget_modify_fg(gsl->status_label, GTK_STATE_NORMAL, NULL);
	gtk_label_set_text(GTK_LABEL(gsl->status_label), gsl->hints);
}

static void remmina_string_list_add(GtkWidget *widget, RemminaStringList *gsl)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreePath *path;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gsl->list));
	gtk_list_store_append(gsl->store, &iter);
	gtk_list_store_set(gsl->store, &iter, COLUMN_COLOR, ERROR_COLOR, -1);
	gtk_tree_selection_select_iter(selection, &iter);

	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gsl->store), &iter);
	gtk_tree_view_set_cursor(GTK_TREE_VIEW(gsl->list), path,
			gtk_tree_view_get_column(GTK_TREE_VIEW(gsl->list), COLUMN_TEXT), TRUE);
	gtk_tree_path_free(path);
}

static void remmina_string_list_remove(GtkWidget *widget, RemminaStringList *gsl)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gsl->list));
	if (gtk_tree_selection_get_selected(selection, NULL, &iter))
	{
		gtk_list_store_remove(gsl->store, &iter);
	}
	remmina_string_list_status_hints(gsl);
}

static void remmina_string_list_move(RemminaStringList *gsl, GtkTreeIter *from, GtkTreeIter *to)
{
	GtkTreePath *path;

	gtk_list_store_swap(gsl->store, from, to);
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(gsl->store), from);
	gtk_tree_view_scroll_to_cell(GTK_TREE_VIEW(gsl->list), path, NULL, 0, 0, 0);
	gtk_tree_path_free(path);
}

static void remmina_string_list_down(GtkWidget *widget, RemminaStringList *gsl)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeIter target_iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gsl->list));
	if (gtk_tree_selection_get_selected(selection, NULL, &iter))
	{
		gtk_tree_selection_get_selected(selection, NULL, &target_iter);
		if (gtk_tree_model_iter_next(GTK_TREE_MODEL(gsl->store), &target_iter))
		{
			remmina_string_list_move(gsl, &iter, &target_iter);
		}
	}
}

static void remmina_string_list_up(GtkWidget *widget, RemminaStringList *gsl)
{
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	GtkTreeIter target_iter;
	GtkTreePath *path;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(gsl->list));
	if (gtk_tree_selection_get_selected(selection, NULL, &iter))
	{
		gtk_tree_selection_get_selected(selection, NULL, &target_iter);
		path = gtk_tree_model_get_path(GTK_TREE_MODEL(gsl->store), &target_iter);
		if (gtk_tree_path_prev(path))
		{
			gtk_tree_model_get_iter(GTK_TREE_MODEL(gsl->store), &target_iter, path);
			gtk_tree_path_free(path);
			remmina_string_list_move(gsl, &iter, &target_iter);
		}
	}
}

static void remmina_string_list_cell_edited(GtkCellRendererText *cell, const gchar *path_string, const gchar *new_text,
		RemminaStringList *gsl)
{
	gchar *text, *ptr, *error;
	GtkTreePath *path = gtk_tree_path_new_from_string(path_string);
	GtkTreeIter iter;
	gboolean has_error = FALSE;

	gtk_tree_model_get_iter(GTK_TREE_MODEL(gsl->store), &iter, path);
	text = g_strdup(new_text);

	/* Eliminate delimitors... */
	for (ptr = text; *ptr; ptr++)
	{
		if (*ptr == STRING_DELIMITOR)
			*ptr = ' ';
	}

	if (gsl->validation_func)
	{
		if (!((*gsl->validation_func)(text, &error)))
		{
			remmina_string_list_status_error(gsl, error);
			g_free(error);
			has_error = TRUE;
		}
		else
		{
			remmina_string_list_status_hints(gsl);
		}
	}

	gtk_list_store_set(gsl->store, &iter, COLUMN_TEXT, text, COLUMN_COLOR, (has_error ? ERROR_COLOR : NULL),
	-1);

	gtk_tree_path_free
	(path);
}

static void remmina_string_list_class_init(RemminaStringListClass *klass)
{
}

static void remmina_string_list_init(RemminaStringList *gsl)
{
	GtkWidget *widget;
	GtkWidget *image;
	GtkWidget *scrolled_window;
	GtkWidget *vbox;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *frame;

	gtk_table_resize(GTK_TABLE(gsl), 3, 2);

	/* Create the frame and add a new scrolled window, followed by the group list */
	frame = gtk_frame_new(NULL);
	gtk_widget_show(frame);
	gtk_table_attach_defaults(GTK_TABLE(gsl), frame, 0, 1, 0, 1);

	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolled_window);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(frame), scrolled_window);

	gsl->store = gtk_list_store_new(NUM_COLUMNS, G_TYPE_STRING, G_TYPE_STRING);
	gsl->list = gtk_tree_view_new_with_model(GTK_TREE_MODEL(gsl->store));
	gtk_widget_show(gsl->list);
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(gsl->list), FALSE);
	gtk_container_add(GTK_CONTAINER(scrolled_window), gsl->list);

	renderer = gtk_cell_renderer_text_new();
	g_object_set(G_OBJECT(renderer), "editable", TRUE, NULL);
	g_signal_connect(renderer, "edited", G_CALLBACK(remmina_string_list_cell_edited), gsl);
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer, "text", COLUMN_TEXT, "foreground", COLUMN_COLOR,
			NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(gsl->list), column);

	/* buttons packed into a vbox */
	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_table_attach(GTK_TABLE(gsl), vbox, 1, 2, 0, 3, 0, GTK_EXPAND | GTK_FILL, 0, 0);

	image = gtk_image_new_from_stock(GTK_STOCK_ADD, GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	widget = gtk_button_new();
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(widget), image);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_string_list_add), gsl);

	image = gtk_image_new_from_stock(GTK_STOCK_REMOVE, GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	widget = gtk_button_new();
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(widget), image);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_string_list_remove), gsl);

	image = gtk_image_new_from_stock(GTK_STOCK_GO_UP, GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	widget = gtk_button_new();
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(widget), image);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_string_list_up), gsl);
	gsl->up_button = widget;

	image = gtk_image_new_from_stock(GTK_STOCK_GO_DOWN, GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	widget = gtk_button_new();
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(widget), image);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_string_list_down), gsl);
	gsl->down_button = widget;

	/* The last status label */
	gsl->status_label = gtk_label_new(NULL);
	gtk_widget_show(gsl->status_label);
	gtk_misc_set_alignment(GTK_MISC(gsl->status_label), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(gsl), gsl->status_label, 0, 2, 2, 3, GTK_EXPAND | GTK_FILL, 0, 0, 0);

	gsl->hints = NULL;
	gsl->validation_func = NULL;
}

GtkWidget*
remmina_string_list_new(void)
{
	return GTK_WIDGET(g_object_new(REMMINA_TYPE_STRING_LIST, NULL));
}

void remmina_string_list_set_auto_sort(RemminaStringList *gsl, gboolean auto_sort)
{
	if (auto_sort)
	{
		gtk_widget_hide(gsl->up_button);
		gtk_widget_hide(gsl->down_button);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gsl->store), COLUMN_TEXT, GTK_SORT_ASCENDING);
	}
	else
	{
		gtk_widget_show(gsl->up_button);
		gtk_widget_show(gsl->down_button);
		gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(gsl->store), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID,
				GTK_SORT_ASCENDING);
	}
}

void remmina_string_list_set_hints(RemminaStringList *gsl, const gchar *hints)
{
	gsl->hints = hints;
	remmina_string_list_status_hints(gsl);
}

void remmina_string_list_set_text(RemminaStringList *gsl, const gchar *text)
{
	GtkTreeIter iter;
	gchar *buf, *ptr1, *ptr2;

	gtk_list_store_clear(gsl->store);

	buf = g_strdup(text);
	ptr1 = buf;
	while (ptr1 && *ptr1 != '\0')
	{
		ptr2 = strchr(ptr1, STRING_DELIMITOR);
		if (ptr2)
			*ptr2++ = '\0';

		gtk_list_store_append(gsl->store, &iter);
		gtk_list_store_set(gsl->store, &iter, COLUMN_TEXT, ptr1, COLUMN_COLOR, NULL, -1);

		ptr1 = ptr2;
	}

	g_free(buf);
}

gchar*
remmina_string_list_get_text(RemminaStringList *gsl)
{
	GString *str;
	GtkTreeIter iter;
	gboolean first, ret;
	GValue value =
	{ 0 };
	const gchar *s;

	str = g_string_new(NULL);
	first = TRUE;

	ret = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(gsl->store), &iter);
	while (ret)
	{
		gtk_tree_model_get_value(GTK_TREE_MODEL(gsl->store), &iter, COLUMN_COLOR, &value);
		if (g_value_get_string(&value) == NULL)
		{
			g_value_unset(&value);
			gtk_tree_model_get_value(GTK_TREE_MODEL(gsl->store), &iter, COLUMN_TEXT, &value);
			s = g_value_get_string(&value);
			if (s && s[0] != '\0')
			{
				if (!first)
				{
					g_string_append_c(str, STRING_DELIMITOR);
				}
				else
				{
					first = FALSE;
				}
				g_string_append(str, s);
			}
		}
		g_value_unset(&value);

		ret = gtk_tree_model_iter_next(GTK_TREE_MODEL(gsl->store), &iter);
	}

	return g_string_free(str, FALSE);
}

void remmina_string_list_set_validation_func(RemminaStringList *gsl, RemminaStringListValidationFunc func)
{
	gsl->validation_func = func;
}

