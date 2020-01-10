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

#pragma once

typedef gboolean (*RemminaStringListValidationFunc)(const gchar *new_str, gchar **error);

typedef struct _RemminaStringListPriv {
	RemminaStringListValidationFunc validation_func;
	const gchar *			fields_separator;
	gboolean			two_columns;
} RemminaStringListPriv;

typedef struct _RemminaStringList {
	GtkBuilder *		builder;
	GtkDialog *		dialog;

	GtkListStore *		liststore_items;
	GtkTreeView *		treeview_items;
	GtkTreeViewColumn *	treeviewcolumn_item;
	GtkTreeSelection *	treeview_selection;
	GtkCellRendererText *	cellrenderertext_item1;
	GtkCellRendererText *	cellrenderertext_item2;

	GtkButton *		button_add;
	GtkButton *		button_remove;
	GtkButton *		button_up;
	GtkButton *		button_down;

	GtkLabel *		label_title;
	GtkLabel *		label_status;

	RemminaStringListPriv * priv;
} RemminaStringList;

G_BEGIN_DECLS

/* RemminaStringList instance */
GtkDialog *remmina_string_list_new(gboolean two_columns, const gchar *fields_separator);
/* Load a string list by splitting a string value */
void remmina_string_list_set_text(const gchar *text, const gboolean clear_data);
/* Get a string value representing the string list */
gchar *remmina_string_list_get_text(void);
/* Set the dialog titles */
void remmina_string_list_set_titles(gchar *title1, gchar *title2);
/* Set a function that will be used to validate the new rows */
void remmina_string_list_set_validation_func(RemminaStringListValidationFunc func);

G_END_DECLS
