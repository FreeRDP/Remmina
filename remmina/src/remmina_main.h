/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2017 Antenore Gatta, Giovanni Panozzo
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
	GtkBuilder *builder;
	GtkWindow *window;
	/* Menu widgets */
	GtkMenu *menu_popup;
	GtkMenu *menu_popup_full;
	GtkRadioMenuItem *menuitem_view_mode_list;
	GtkRadioMenuItem *menuitem_view_mode_tree;
	/* Quick connect objects */
	GtkBox *box_quick_connect;
	GtkComboBoxText *combo_quick_connect_protocol;
	GtkEntry *entry_quick_connect_server;
	GtkButton *button_quick_connect;
	/* Other widgets */
	GtkTreeView *tree_files_list;
	GtkTreeViewColumn *column_files_list_group;
	GtkStatusbar *statusbar_main;
	/* Non widget objects */
	GtkAccelGroup *accelgroup_shortcuts;
	GtkActionGroup *actiongroup_connection;
	/* Actions from the application ActionGroup */
	GtkAction *action_application_about;
	GtkAction *action_application_plugins;
	GtkAction *action_application_preferences;
	GtkAction *action_application_quit;
	/* Actions from the connections ActionGroup */
	GtkAction *action_connections_new;
	/* Actions from the connection ActionGroup */
	GtkAction *action_connection_connect;
	GtkAction *action_connection_edit;
	GtkAction *action_connection_copy;
	GtkAction *action_connection_delete;
	GtkAction *action_connection_external_tools;
	/* Actions from the view ActionGroup */
	GtkToggleAction *action_view_statusbar;
	GtkToggleAction *action_view_quick_connect;
	GtkToggleAction *action_view_mode_list;
	GtkToggleAction *action_view_mode_tree;
	/* Actions from the tools ActionGroup */
	GtkAction *action_tools_import;
	GtkAction *action_tools_export;
	/* Actions from the help ActionGroup */
	GtkAction *action_help_homepage;
	GtkAction *action_help_wiki;
	GtkAction *action_help_debug;
	RemminaMainPriv *priv;
} RemminaMain;

struct _RemminaMainPriv {
	GtkTreeModel *file_model;
	GtkTreeModel *file_model_filter;
	GtkTreeModel *file_model_sort;

	gboolean initialized;

	gchar *selected_filename;
	gchar *selected_name;
	gboolean override_view_file_mode_to_list;
	RemminaStringArray *expanded_group;
};

G_BEGIN_DECLS

/* Create the main Remmina window */
GtkWidget* remmina_main_new(void);
/* Get the current main window or NULL if not initialized */
GtkWindow* remmina_main_get_window(void);

void remmina_main_update_file_datetime(RemminaFile *file);

void remmina_main_destroy(void);
void remmina_main_save_before_destroy(void);

void remmina_main_show_warning_dialog(const gchar* message);

G_END_DECLS

