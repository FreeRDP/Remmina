/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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

#include "remmina_string_array.h"

#pragma once

#include "remmina_file.h"

typedef struct _RemminaMainPriv RemminaMainPriv;

typedef struct _RemminaMain {
	GtkBuilder *		builder;
	GtkWindow *		window;
	/* Menu widgets */
	GtkMenu *		menu_popup;
	GtkMenuButton *		menu_header_button;
	GtkMenu *		menu_popup_full;
	GtkRadioMenuItem *	menuitem_view_mode_list;
	GtkRadioMenuItem *	menuitem_view_mode_tree;
	GtkMenuItem *		menuitem_connection_quit;
	/* Button new */
	GtkButton *		button_new;
	GtkButton *		button_make_default;
	/* Search bar objects */
	GtkToggleButton *	search_toggle;
	GtkToggleButton *	view_toggle_button;
	GtkToggleButton *	ustats_toggle;
	GtkSearchBar *		search_bar;
	/* Quick connect objects */
	GtkBox *		box_quick_connect;
	GtkComboBoxText *	combo_quick_connect_protocol;
	GtkEntry *		entry_quick_connect_server;
	GtkButton *		button_quick_connect;
	/* Other widgets */
	GtkTreeView *		tree_files_list;
	GtkTreeViewColumn *	column_files_list_group;
	GtkStatusbar *		statusbar_main;
	/* Non widget objects */
	GtkAccelGroup *		accelgroup_shortcuts;
	/* Actions from the view ActionGroup */
	GtkToggleAction *	action_view_quick_connect;
	GtkToggleAction *	action_view_mode_list;
	GtkToggleAction *	action_view_mode_tree;
	RemminaMainPriv *	priv;
} RemminaMain;

struct _RemminaMainPriv {
	GtkTreeModel *		file_model;
	GtkTreeModel *		file_model_filter;
	GtkTreeModel *		file_model_sort;

	gboolean		initialized;

	gchar *			selected_filename;
	gchar *			selected_name;
	gboolean		override_view_file_mode_to_list;
	RemminaStringArray *	expanded_group;
};

G_BEGIN_DECLS

/* Create the main Remmina window */
GtkWidget *remmina_main_new(void);
/* Get the current main window or NULL if not initialized */
GtkWindow *remmina_main_get_window(void);

void remmina_main_update_file_datetime(RemminaFile *file);

void remmina_main_destroy(void);
void remmina_main_save_before_destroy(void);

void remmina_main_show_warning_dialog(const gchar *message);
void remmina_main_on_action_application_about(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_application_default(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_application_mpchange(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_application_plugins(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_application_preferences(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_application_quit(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_connection_connect(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_connection_copy(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_connection_delete(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_connection_edit(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_connection_external_tools(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_connection_new(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_help_community(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_help_debug(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_help_donations(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_help_homepage(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_help_wiki(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_tools_export(GSimpleAction *action, GVariant *param, gpointer data);
void remmina_main_on_action_tools_import(GSimpleAction *action, GVariant *param, gpointer data);

G_END_DECLS
