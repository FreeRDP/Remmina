/*  See LICENSE and COPYING files for copyright and license details. */

#include "remmina_string_array.h"

#ifndef __REMMINAMAIN_H__
#define __REMMINAMAIN_H__

typedef struct _RemminaMainPriv RemminaMainPriv;

typedef struct _RemminaMain
{
	GtkBuilder *builder;
	GtkWindow *window;
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
	/* The file_mode previously selected before the quick search */
	gint previous_file_mode;
	RemminaStringArray *expanded_group;
};

G_BEGIN_DECLS

/* Create the main Remmina window */
GtkWidget* remmina_main_new(void);
/* Get the current main window or NULL if not initialized */
GtkWindow* remmina_main_get_window(void);

G_END_DECLS

#endif  /* __REMMINAMAIN_H__  */
