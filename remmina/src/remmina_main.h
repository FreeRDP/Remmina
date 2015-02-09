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

#include "remmina_string_array.h"

#ifndef __REMMINAMAIN_H__
#define __REMMINAMAIN_H__

/* Defines the association between an Action and its callback for activation */
typedef struct _ActionsCallbackMap
{
	GtkAction *action;
	gpointer callback;
} ActionsCallbackMap;

G_BEGIN_DECLS

#define REMMINA_TYPE_MAIN               (remmina_main_get_type ())
#define REMMINA_MAIN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_MAIN, RemminaMain))
#define REMMINA_MAIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_MAIN, RemminaMainClass))
#define REMMINA_IS_MAIN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_MAIN))
#define REMMINA_IS_MAIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_MAIN))
#define REMMINA_MAIN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_MAIN, RemminaMainClass))

typedef struct _RemminaMainPriv RemminaMainPriv;

typedef struct _RemminaMain
{
	GtkWindow window;
	/* Menu widgets */
	GtkMenu *menu_popup;
	GtkMenu *menu_tools;
	/* Toolbar widgets */
	GtkToolbar *toolbar_main;
	GtkToolItem *toolbutton_separator_quick_search;
	GtkToolItem *toolbutton_quick_search;
	GtkEntry *entry_quick_search;
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
	GtkListStore *liststore_files_list;
	GtkTreeStore *treestore_files_list;
	/* Actions from the application ActionGroup */
	GtkAction *action_application_about;
	GtkAction *action_application_plugins;
	GtkAction *action_application_preferences;
	GtkAction *action_application_quit;
	/* Actions from the connection ActionGroup */
	GtkAction *action_connection_connect;
	GtkAction *action_connection_new;
	GtkAction *action_connection_edit;
	GtkAction *action_connection_copy;
	GtkAction *action_connection_delete;
	/* Actions from the view ActionGroup */
	GtkToggleAction *action_view_toolbar;
	GtkToggleAction *action_view_statusbar;
	GtkToggleAction *action_view_quick_search;
	GtkToggleAction *action_view_quick_connect;
	GtkToggleAction *action_view_small_toolbar_buttons;
	GtkToggleAction *action_view_mode_list;
	GtkToggleAction *action_view_mode_tree;
	/* Actions from the tools ActionGroup */
	GtkAction *action_tools_import;
	GtkAction *action_tools_export;
	GtkAction *action_tools_externaltools;
	/* Actions from the help ActionGroup */
	GtkAction *action_help_homepage;
	GtkAction *action_help_wiki;
	GtkAction *action_help_debug;

	RemminaMainPriv *priv;
} RemminaMain;

struct _RemminaMainPriv
{
	GtkTreeModel *file_model;
	GtkTreeModel *file_model_filter;
	GtkTreeModel *file_model_sort;

	gboolean initialized;

	gchar *selected_filename;
	gchar *selected_name;
	RemminaStringArray *expanded_group;
};

typedef struct _RemminaMainClass
{
	GtkWindowClass parent_class;
} RemminaMainClass;

GType remmina_main_get_type(void)
G_GNUC_CONST;

/* Create the main Remmina window */
GtkWidget* remmina_main_new(void);

G_END_DECLS

/* Callbacks for actions from the application ActionGroup */
static void remmina_main_on_action_application_quit();
static void remmina_main_on_action_application_preferences();
static void remmina_main_on_action_application_about();
static void remmina_main_on_action_application_plugins();
/* Callbacks for actions from the connection ActionGroup */
static void remmina_main_on_action_connection_connect();
static void remmina_main_on_action_connection_new();
static void remmina_main_on_action_connection_edit();
static void remmina_main_on_action_connection_copy();
static void remmina_main_on_action_connection_delete();
/* Callbacks for actions from the view ActionGroup */
static void remmina_main_on_action_view_toolbar();
static void remmina_main_on_action_view_statusbar();
static void remmina_main_on_action_view_quick_search();
static void remmina_main_on_action_view_quick_connect();
static void remmina_main_on_action_view_small_toolbar_buttons();
static void remmina_main_on_action_view_file_mode();
/* Callbacks for actions from the tools ActionGroup */
static void remmina_main_on_action_tools_import();
static void remmina_main_on_action_tools_export();
static void remmina_main_on_action_tools_externaltools();
/* Callbacks for actions from the help ActionGroup */
static void remmina_main_on_action_help_homepage();
static void remmina_main_on_action_help_wiki();
static void remmina_main_on_action_help_debug();

#endif  /* __REMMINAMAIN_H__  */
