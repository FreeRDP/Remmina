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

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include "remminafile.h"
#include "remminafilemanager.h"
#include "remminafileeditor.h"
#include "remminaconnectionwindow.h"
#include "remminaabout.h"
#include "remminapref.h"
#include "remminaprefdialog.h"
#include "remminawidgetpool.h"
#include "remminamain.h"

G_DEFINE_TYPE (RemminaMain, remmina_main, GTK_TYPE_WINDOW)

struct _RemminaMainPriv
{
    GtkWidget *file_list;
    GtkTreeModel *file_model;
    GtkUIManager *uimanager;
    GtkWidget *toolbar;
    GtkWidget *statusbar;

    GtkTreeViewColumn *group_column;

    GtkActionGroup *main_group;
    GtkActionGroup *file_sensitive_group;

    gboolean initialized;

    gchar *selected_filename;
    gchar *selected_name;
    GtkTreeIter *selected_iter;
};

static void
remmina_main_class_init (RemminaMainClass *klass)
{
}

enum
{
    PROTOCOL_COLUMN,
    NAME_COLUMN,
    GROUP_COLUMN,
    SERVER_COLUMN,
    FILENAME_COLUMN,
    N_COLUMNS
};

static void
remmina_main_save_size (RemminaMain *remminamain)
{
    if ((gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (remminamain))) & GDK_WINDOW_STATE_MAXIMIZED) == 0)
    {
        gtk_window_get_size (GTK_WINDOW (remminamain), &remmina_pref.main_width, &remmina_pref.main_height);
        remmina_pref.main_maximize = FALSE;
    }
    else
    {
        remmina_pref.main_maximize = TRUE;
    }
    remmina_pref_save ();
}

static gboolean
remmina_main_on_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    remmina_main_save_size (REMMINA_MAIN (widget));
    return FALSE;
}

static void
remmina_main_destroy (GtkWidget *widget, gpointer data)
{
    g_free (REMMINA_MAIN (widget)->priv);
}

static void
remmina_main_clear_selection_data (RemminaMain *remminamain)
{
    g_free (remminamain->priv->selected_filename);
    g_free (remminamain->priv->selected_name);
    remminamain->priv->selected_filename = NULL;
    remminamain->priv->selected_name = NULL;

    gtk_action_group_set_sensitive (remminamain->priv->file_sensitive_group, FALSE);
}

static gboolean
remmina_main_selection_func (
    GtkTreeSelection *selection,
    GtkTreeModel     *model,
    GtkTreePath      *path,
    gboolean          path_currently_selected,
    gpointer          user_data)
{
    RemminaMain *remminamain;
    guint context_id;
    GtkTreeIter iter;
    gchar buf[1000];

    remminamain = (RemminaMain*) user_data;
    if (path_currently_selected) return TRUE;

    if (!gtk_tree_model_get_iter (model, &iter, path)) return TRUE;

    remmina_main_clear_selection_data (remminamain);

    gtk_tree_model_get (model, &iter,
        NAME_COLUMN, &remminamain->priv->selected_name,
        FILENAME_COLUMN, &remminamain->priv->selected_filename,
        -1);

    context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (remminamain->priv->statusbar), "status");
    gtk_statusbar_pop (GTK_STATUSBAR (remminamain->priv->statusbar), context_id);
    if (remminamain->priv->selected_filename)
    {
        gtk_action_group_set_sensitive (remminamain->priv->file_sensitive_group, TRUE);
        g_snprintf (buf, sizeof (buf), "%s (%s)", remminamain->priv->selected_name, remminamain->priv->selected_filename);
        gtk_statusbar_push (GTK_STATUSBAR (remminamain->priv->statusbar), context_id, buf);
    }
    else
    {
        gtk_statusbar_push (GTK_STATUSBAR (remminamain->priv->statusbar), context_id, remminamain->priv->selected_name);
    }
    return TRUE;
}

static void
remmina_main_load_file_list_callback (gpointer data, gpointer user_data)
{
    RemminaMain *remminamain;
    GtkTreeIter iter;
    GtkListStore *store;
    RemminaFile *remminafile;

    remminafile = (RemminaFile*) data;
    remminamain = (RemminaMain*) user_data;
    store = GTK_LIST_STORE (remminamain->priv->file_model);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        PROTOCOL_COLUMN, remmina_file_get_icon_name (remminafile),
        NAME_COLUMN, remminafile->name,
        GROUP_COLUMN, remminafile->group,
        SERVER_COLUMN, remminafile->server,
        FILENAME_COLUMN, remminafile->filename,
        -1);

    if (remminamain->priv->selected_filename && g_strcmp0 (remminafile->filename, remminamain->priv->selected_filename) == 0)
    {
        remminamain->priv->selected_iter = gtk_tree_iter_copy (&iter);
    }
}

static void
remmina_main_load_file_tree_group (GtkTreeStore *store)
{
    GtkTreeIter iter;
    gchar *groups, *ptr1, *ptr2;

    groups = remmina_file_manager_get_groups ();
    if (groups == NULL || groups[0] == '\0') return;

    ptr1 = groups;
    while (ptr1)
    {
        ptr2 = strchr (ptr1, ',');
        if (ptr2) *ptr2++ = '\0';

        gtk_tree_store_append (store, &iter, NULL);
        gtk_tree_store_set (store, &iter,
            PROTOCOL_COLUMN, GTK_STOCK_DIRECTORY,
            NAME_COLUMN, ptr1,
            FILENAME_COLUMN, NULL,
            -1);

        ptr1 = ptr2;
    }

    g_free (groups);
}

static void
remmina_main_load_file_tree_callback (gpointer data, gpointer user_data)
{
    RemminaMain *remminamain;
    GtkTreeIter iter, child;
    GtkTreeStore *store;
    RemminaFile *remminafile;
    gboolean ret, match;
    gchar *name, *filename;

    remminafile = (RemminaFile*) data;
    remminamain = (RemminaMain*) user_data;
    store = GTK_TREE_STORE (remminamain->priv->file_model);

    ret = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);
    match = FALSE;
    while (ret)
    {
        gtk_tree_model_get (GTK_TREE_MODEL (store), &iter, NAME_COLUMN, &name, FILENAME_COLUMN, &filename, -1);
        match = (filename == NULL && g_strcmp0 (name, remminafile->group) == 0);
        g_free (name);
        g_free (filename);
        if (match) break;
        ret = gtk_tree_model_iter_next (GTK_TREE_MODEL (store), &iter);
    }

    gtk_tree_store_append (store, &child, (match ? &iter : NULL));
    gtk_tree_store_set (store, &child,
        PROTOCOL_COLUMN, remmina_file_get_icon_name (remminafile),
        NAME_COLUMN, remminafile->name,
        GROUP_COLUMN, remminafile->group,
        SERVER_COLUMN, remminafile->server,
        FILENAME_COLUMN, remminafile->filename,
        -1);

    if (remminamain->priv->selected_filename && g_strcmp0 (remminafile->filename, remminamain->priv->selected_filename) == 0)
    {
        remminamain->priv->selected_iter = gtk_tree_iter_copy (&child);
    }
}

static void
remmina_main_file_model_on_sort (GtkTreeSortable *sortable, RemminaMain *remminamain)
{
    gint columnid;
    GtkSortType order;

    gtk_tree_sortable_get_sort_column_id (sortable, &columnid, &order);
    remmina_pref.main_sort_column_id = columnid;
    remmina_pref.main_sort_order = order;
    remmina_pref_save ();
}

static void
remmina_main_load_files (RemminaMain *remminamain)
{
    gint n;
    gchar buf[200];
    guint context_id;
    GtkTreePath *path;

    switch (remmina_pref.view_file_mode)
    {

    case REMMINA_VIEW_FILE_TREE:
        gtk_tree_view_column_set_visible (remminamain->priv->group_column, FALSE);
        remminamain->priv->file_model = GTK_TREE_MODEL (gtk_tree_store_new (N_COLUMNS,
            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
        remmina_main_load_file_tree_group (GTK_TREE_STORE (remminamain->priv->file_model));
        n = remmina_file_manager_iterate (remmina_main_load_file_tree_callback, remminamain);
        break;

    case REMMINA_VIEW_FILE_LIST:
    default:
        gtk_tree_view_column_set_visible (remminamain->priv->group_column, TRUE);
        remminamain->priv->file_model = GTK_TREE_MODEL (gtk_list_store_new (N_COLUMNS,
            G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING));
        n = remmina_file_manager_iterate (remmina_main_load_file_list_callback, remminamain);
        break;
    }

    gtk_tree_view_set_model (GTK_TREE_VIEW (remminamain->priv->file_list), remminamain->priv->file_model);
    gtk_tree_view_expand_all (GTK_TREE_VIEW (remminamain->priv->file_list));
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (remminamain->priv->file_model),
        remmina_pref.main_sort_column_id, remmina_pref.main_sort_order);
    g_signal_connect (G_OBJECT (remminamain->priv->file_model), "sort-column-changed",
        G_CALLBACK (remmina_main_file_model_on_sort), remminamain);

    if (remminamain->priv->selected_iter)
    {
        gtk_tree_selection_select_iter (
            gtk_tree_view_get_selection (GTK_TREE_VIEW (remminamain->priv->file_list)),
            remminamain->priv->selected_iter);
        path = gtk_tree_model_get_path (remminamain->priv->file_model, remminamain->priv->selected_iter);
        gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (remminamain->priv->file_list),
            path, NULL, TRUE, 0.5, 0.0);
        gtk_tree_path_free (path);
        gtk_tree_iter_free (remminamain->priv->selected_iter);
        remminamain->priv->selected_iter = NULL;
    }

    g_snprintf (buf, 200, ngettext("Total %i item.", "Total %i items.", n), n);
    context_id = gtk_statusbar_get_context_id (GTK_STATUSBAR (remminamain->priv->statusbar), "status");
    gtk_statusbar_pop (GTK_STATUSBAR (remminamain->priv->statusbar), context_id);
    gtk_statusbar_push (GTK_STATUSBAR (remminamain->priv->statusbar), context_id, buf);
}

static void
remmina_main_action_action_quick_connect_full (RemminaMain *remminamain, const gchar *protocol)
{
    GtkWidget *widget;

    widget = remmina_file_editor_new_temp_full (NULL, protocol);
    gtk_widget_show (widget);
}

static void
remmina_main_action_action_quick_connect (GObject *object, RemminaMain *remminamain)
{
    remmina_main_action_action_quick_connect_full (remminamain, NULL);
}

static void
remmina_main_action_action_quick_connect_rdp (GObject *object, RemminaMain *remminamain)
{
    remmina_main_action_action_quick_connect_full (remminamain, "RDP");
}

static void
remmina_main_action_action_quick_connect_vnc (GObject *object, RemminaMain *remminamain)
{
    remmina_main_action_action_quick_connect_full (remminamain, "VNC");
}

static void
remmina_main_action_action_quick_connect_vnci (GObject *object, RemminaMain *remminamain)
{
    remmina_main_action_action_quick_connect_full (remminamain, "VNCI");
}

static void
remmina_main_action_action_quick_connect_xdmcp (GObject *object, RemminaMain *remminamain)
{
    remmina_main_action_action_quick_connect_full (remminamain, "XDMCP");
}

static void
remmina_main_action_action_quick_connect_sftp (GObject *object, RemminaMain *remminamain)
{
    remmina_main_action_action_quick_connect_full (remminamain, "SFTP");
}

static void
remmina_main_action_action_connect (GtkAction *action, RemminaMain *remminamain)
{
    remmina_connection_window_open_from_filename (remminamain->priv->selected_filename);
}

static void
remmina_main_file_editor_destroy (GtkWidget *widget, RemminaMain *remminamain)
{
    if (GTK_IS_WIDGET (remminamain))
    {
        remmina_main_load_files (remminamain);
    }
}

static void
remmina_main_action_edit_new (GtkAction *action, RemminaMain *remminamain)
{
    GtkWidget *widget;

    widget = remmina_file_editor_new ();
    g_signal_connect (G_OBJECT (widget), "destroy", G_CALLBACK (remmina_main_file_editor_destroy), remminamain);
    gtk_widget_show (widget);
}

static void
remmina_main_action_edit_copy (GtkAction *action, RemminaMain *remminamain)
{
    GtkWidget *widget;

    if (remminamain->priv->selected_filename == NULL) return;

    widget = remmina_file_editor_new_copy (remminamain->priv->selected_filename);
    if (widget)
    {
        g_signal_connect (G_OBJECT (widget), "destroy", G_CALLBACK (remmina_main_file_editor_destroy), remminamain);
        gtk_widget_show (widget);
    }
}

static void
remmina_main_action_edit_edit (GtkAction *action, RemminaMain *remminamain)
{
    GtkWidget *widget;

    if (remminamain->priv->selected_filename == NULL) return;

    widget = remmina_file_editor_new_from_filename (remminamain->priv->selected_filename);
    if (widget)
    {
        g_signal_connect (G_OBJECT (widget), "destroy", G_CALLBACK (remmina_main_file_editor_destroy), remminamain);
        gtk_widget_show (widget);
    }
}

static void
remmina_main_action_edit_delete (GtkAction *action, RemminaMain *remminamain)
{
    GtkWidget *dialog;

    if (remminamain->priv->selected_filename == NULL) return;

    dialog = gtk_message_dialog_new (GTK_WINDOW (remminamain),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("Are you sure to delete '%s'"), remminamain->priv->selected_name);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
    {
        g_unlink (remminamain->priv->selected_filename);
        remmina_main_load_files (remminamain);
    }
    gtk_widget_destroy (dialog);
}

static void
remmina_main_action_edit_preferences (GtkAction *action, RemminaMain *remminamain)
{
    GtkWidget *widget;

    widget = remmina_pref_dialog_new (0);
    gtk_widget_show (widget);
}

static void
remmina_main_action_action_quit (GtkAction *action, RemminaMain *remminamain)
{
    gtk_widget_destroy (GTK_WIDGET (remminamain));
}

static void
remmina_main_action_view_toolbar (GtkToggleAction *action, RemminaMain *remminamain)
{
	gboolean toggled;

	toggled = gtk_toggle_action_get_active (action);
    if (toggled)
    {
        gtk_widget_show (remminamain->priv->toolbar);
    }
    else
    {
        gtk_widget_hide (remminamain->priv->toolbar);
    }
    if (remminamain->priv->initialized)
    {
        remmina_pref.hide_toolbar = !toggled;
        remmina_pref_save ();
    }
}

static void
remmina_main_action_view_statusbar (GtkToggleAction *action, RemminaMain *remminamain)
{
	gboolean toggled;

	toggled = gtk_toggle_action_get_active (action);
    if (toggled)
    {
        gtk_widget_show (remminamain->priv->statusbar);
    }
    else
    {
        gtk_widget_hide (remminamain->priv->statusbar);
    }
    if (remminamain->priv->initialized)
    {
        remmina_pref.hide_statusbar = !toggled;
        remmina_pref_save ();
    }
}

static void
remmina_main_action_view_small_toolbutton (GtkToggleAction *action, RemminaMain *remminamain)
{
	gboolean toggled;

	toggled = gtk_toggle_action_get_active (action);
    if (toggled)
    {
        gtk_toolbar_set_icon_size (GTK_TOOLBAR (remminamain->priv->toolbar), GTK_ICON_SIZE_MENU);
    }
    else
    {
        gtk_toolbar_unset_icon_size (GTK_TOOLBAR (remminamain->priv->toolbar));
    }
    if (remminamain->priv->initialized)
    {
        remmina_pref.small_toolbutton = toggled;
        remmina_pref_save ();
    }
}

static void
remmina_main_action_view_file_mode (GtkRadioAction *action, GtkRadioAction *current, RemminaMain *remminamain)
{
    remmina_pref.view_file_mode = gtk_radio_action_get_current_value (action);
    remmina_pref_save ();
    remmina_main_load_files (remminamain);
}

static void
remmina_main_action_help_about (GtkAction *action, RemminaMain *remminamain)
{
    remmina_about_open (GTK_WINDOW (remminamain));
}

static const gchar *remmina_main_ui_xml = 
"<ui>"
"  <menubar name='MenuBar'>"
"    <menu name='ActionMenu' action='Action'>"
"      <menuitem name='ActionQuickConnectMenu' action='ActionQuickConnect'/>"
"      <menu name='ActionQuickConnectProtoMenu' action='ActionQuickConnectProto'>"
"        <menuitem name='ActionQuickConnectProtoRDPMenu' action='ActionQuickConnectProtoRDP'/>"
#ifdef HAVE_LIBVNCCLIENT
"        <menuitem name='ActionQuickConnectProtoVNCMenu' action='ActionQuickConnectProtoVNC'/>"
"        <menuitem name='ActionQuickConnectProtoVNCIMenu' action='ActionQuickConnectProtoVNCI'/>"
#endif
"        <menuitem name='ActionQuickConnectProtoXDMCPMenu' action='ActionQuickConnectProtoXDMCP'/>"
#ifdef HAVE_LIBSSH
"        <menuitem name='ActionQuickConnectProtoSFTPMenu' action='ActionQuickConnectProtoSFTP'/>"
#endif
"      </menu>"
"      <menuitem name='ActionConnectMenu' action='ActionConnect'/>"
"      <separator/>"
"      <menuitem name='ActionQuitMenu' action='ActionQuit'/>"
"    </menu>"
"    <menu name='EditMenu' action='Edit'>"
"      <menuitem name='EditNewMenu' action='EditNew'/>"
"      <menuitem name='EditCopyMenu' action='EditCopy'/>"
"      <menuitem name='EditEditMenu' action='EditEdit'/>"
"      <menuitem name='EditDeleteMenu' action='EditDelete'/>"
"      <separator/>"
"      <menuitem name='EditPreferencesMenu' action='EditPreferences'/>"
"    </menu>"
"    <menu name='ViewMenu' action='View'>"
"      <menuitem name='ViewToolbarMenu' action='ViewToolbar'/>"
"      <menuitem name='ViewStatusbarMenu' action='ViewStatusbar'/>"
"      <separator/>"
"      <menuitem name='ViewSmallToolbuttonMenu' action='ViewSmallToolbutton'/>"
"      <separator/>"
"      <menuitem name='ViewFileListMenu' action='ViewFileList'/>"
"      <menuitem name='ViewFileTreeMenu' action='ViewFileTree'/>"
"    </menu>"
"    <menu name='HelpMenu' action='Help'>"
"      <menuitem name='HelpAboutMenu' action='HelpAbout'/>"
"    </menu>"
"  </menubar>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='ActionConnect'/>"
"    <separator/>"
"    <toolitem action='EditNew'/>"
"    <toolitem action='EditCopy'/>"
"    <toolitem action='EditEdit'/>"
"    <toolitem action='EditDelete'/>"
"    <separator/>"
"    <toolitem action='EditPreferences'/>"
"  </toolbar>"
"  <popup name='PopupQuickConnectProtoMenu' action='ActionQuickConnectProto'>"
"    <menuitem name='PopupQuickConnectProtoRDPMenu' action='ActionQuickConnectProtoRDP'/>"
#ifdef HAVE_LIBVNCCLIENT
"    <menuitem name='PopupQuickConnectProtoVNCMenu' action='ActionQuickConnectProtoVNC'/>"
"    <menuitem name='PopupQuickConnectProtoVNCIMenu' action='ActionQuickConnectProtoVNCI'/>"
#endif
"    <menuitem name='PopupQuickConnectProtoXDMCPMenu' action='ActionQuickConnectProtoXDMCP'/>"
#ifdef HAVE_LIBSSH
"    <menuitem name='PopupQuickConnectProtoSFTPMenu' action='ActionQuickConnectProtoSFTP'/>"
#endif
"  </popup>"
"  <popup name='PopupMenu'>"
"    <menuitem action='ActionConnect'/>"
"    <separator/>"
"    <menuitem action='EditCopy'/>"
"    <menuitem action='EditEdit'/>"
"    <menuitem action='EditDelete'/>"
"  </popup>"
"</ui>";

static const GtkActionEntry remmina_main_ui_menu_entries[] =
{
    { "Action", NULL, N_("_Action") },
    { "Edit", NULL, N_("_Edit") },
    { "View", NULL, N_("_View") },
    { "Help", NULL, N_("_Help") },

    { "ActionQuickConnect", GTK_STOCK_JUMP_TO, N_("Quick Connect"), "<control>U",
        N_("Open a quick connection"),
        G_CALLBACK (remmina_main_action_action_quick_connect) },

    { "ActionQuickConnectProto", NULL, N_("Quick _Connect To") },

    { "ActionQuickConnectProtoRDP", "remmina-rdp", N_("RDP - Windows Terminal Service"), NULL, NULL,
        G_CALLBACK (remmina_main_action_action_quick_connect_rdp) },
    { "ActionQuickConnectProtoVNC", "remmina-vnc", N_("VNC - Virtual Network Computing"), NULL, NULL,
        G_CALLBACK (remmina_main_action_action_quick_connect_vnc) },
    { "ActionQuickConnectProtoVNCI", "remmina-vnc", N_("VNC - Incoming Connection"), NULL, NULL,
        G_CALLBACK (remmina_main_action_action_quick_connect_vnci) },
    { "ActionQuickConnectProtoXDMCP", "remmina-xdmcp", N_("XDMCP - X Remote Session"), NULL, NULL,
        G_CALLBACK (remmina_main_action_action_quick_connect_xdmcp) },
    { "ActionQuickConnectProtoSFTP", "remmina-sftp", N_("SFTP - Secure File Transfer"), NULL, NULL,
        G_CALLBACK (remmina_main_action_action_quick_connect_sftp) },

    { "EditNew", GTK_STOCK_NEW, NULL, "<control>N",
        N_("Create a new remote desktop file"),
        G_CALLBACK (remmina_main_action_edit_new) },

    { "EditPreferences", GTK_STOCK_PREFERENCES, NULL, "<control>P",
        N_("Open the preferences dialog"),
        G_CALLBACK (remmina_main_action_edit_preferences) },

    { "ActionQuit", GTK_STOCK_QUIT, NULL, "<control>Q",
        N_("Quit Remmina"),
        G_CALLBACK (remmina_main_action_action_quit) },

    { "HelpAbout", GTK_STOCK_ABOUT, NULL, NULL,
        NULL,
        G_CALLBACK (remmina_main_action_help_about) }
};

static const GtkActionEntry remmina_main_ui_file_sensitive_menu_entries[] =
{
    { "ActionConnect", GTK_STOCK_CONNECT, NULL, "<control>O",
        N_("Open the connection to the selected remote desktop file"),
        G_CALLBACK (remmina_main_action_action_connect) },

    { "EditCopy", GTK_STOCK_COPY, NULL, "<control>C",
        N_("Create a copy of the selected remote desktop file"),
        G_CALLBACK (remmina_main_action_edit_copy) },

    { "EditEdit", GTK_STOCK_EDIT, NULL, "<control>E",
        N_("Edit the selected remote desktop file"),
        G_CALLBACK (remmina_main_action_edit_edit) },

    { "EditDelete", GTK_STOCK_DELETE, NULL, "<control>D",
        N_("Delete the selected remote desktop file"),
        G_CALLBACK (remmina_main_action_edit_delete) }
};

static const GtkToggleActionEntry remmina_main_ui_toggle_menu_entries[] =
{
    { "ViewToolbar", NULL, N_("Toolbar"), NULL, NULL,
        G_CALLBACK (remmina_main_action_view_toolbar), TRUE },

    { "ViewStatusbar", NULL, N_("Statusbar"), NULL, NULL,
        G_CALLBACK (remmina_main_action_view_statusbar), TRUE },

    { "ViewSmallToolbutton", NULL, N_("Small Toolbar Buttons"), NULL, NULL,
        G_CALLBACK (remmina_main_action_view_small_toolbutton), FALSE }
};

static const GtkRadioActionEntry remmina_main_ui_view_file_mode_entries[] =
{
    { "ViewFileList", NULL, N_("List View"), NULL, NULL, REMMINA_VIEW_FILE_LIST },
    { "ViewFileTree", NULL, N_("Tree View"), NULL, NULL, REMMINA_VIEW_FILE_TREE }
};

static gboolean
remmina_main_file_list_on_button_press (GtkWidget *widget, GdkEventButton *event, RemminaMain *remminamain)
{
    GtkWidget *popup;

    if (event->button == 3)
    {
        popup = gtk_ui_manager_get_widget (remminamain->priv->uimanager, "/PopupMenu");
        if (popup)
        {
            gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL, event->button, event->time);
        }
    }
    else if (event->type == GDK_2BUTTON_PRESS && remminamain->priv->selected_filename)
    {
        switch (remmina_pref.default_action)
        {
        case REMMINA_ACTION_EDIT:
            remmina_main_action_edit_edit (NULL, remminamain);
            break;
        case REMMINA_ACTION_CONNECT:
        default:
            remmina_main_action_action_connect (NULL, remminamain);
            break;
        }
    }
    return FALSE;
}

static void
remmina_main_init (RemminaMain *remminamain)
{
    RemminaMainPriv *priv;
    GtkWidget *vbox;
    GtkWidget *menubar;
    GtkUIManager *uimanager;
    GtkActionGroup *action_group;
    GtkWidget *scrolledwindow;
    GtkWidget *tree;
    GtkToolItem *toolitem;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GError *error;

    priv = g_new (RemminaMainPriv, 1);
    remminamain->priv = priv;

    priv->initialized = FALSE;
    priv->selected_filename = NULL;
    priv->selected_name = NULL;
    priv->selected_iter = NULL;

    /* Create main window */
    g_signal_connect (G_OBJECT (remminamain), "delete-event", G_CALLBACK (remmina_main_on_delete_event), NULL);
    g_signal_connect (G_OBJECT (remminamain), "destroy", G_CALLBACK (remmina_main_destroy), NULL);
    gtk_container_set_border_width (GTK_CONTAINER (remminamain), 0);
    gtk_window_set_title (GTK_WINDOW (remminamain), _("Remote Desktop Client"));
    gtk_window_set_default_size (GTK_WINDOW (remminamain), remmina_pref.main_width, remmina_pref.main_height);
    gtk_window_set_position (GTK_WINDOW (remminamain), GTK_WIN_POS_CENTER);
    if (remmina_pref.main_maximize)
    {
        gtk_window_maximize (GTK_WINDOW (remminamain));
    }

    /* Create the main container */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_container_add (GTK_CONTAINER (remminamain), vbox);
    gtk_widget_show (vbox);

    /* Create the menubar and toolbar */
    uimanager = gtk_ui_manager_new ();
    priv->uimanager = uimanager;

    action_group = gtk_action_group_new ("RemminaMainActions");
    gtk_action_group_set_translation_domain (action_group, NULL);
    gtk_action_group_add_actions (action_group, remmina_main_ui_menu_entries,
        G_N_ELEMENTS (remmina_main_ui_menu_entries), remminamain);
    gtk_action_group_add_toggle_actions (action_group, remmina_main_ui_toggle_menu_entries,
        G_N_ELEMENTS (remmina_main_ui_toggle_menu_entries), remminamain);
    gtk_action_group_add_radio_actions (action_group, remmina_main_ui_view_file_mode_entries,
        G_N_ELEMENTS (remmina_main_ui_view_file_mode_entries),
        remmina_pref.view_file_mode, G_CALLBACK (remmina_main_action_view_file_mode), remminamain);

    gtk_ui_manager_insert_action_group (uimanager, action_group, 0);
    g_object_unref (action_group);
    priv->main_group = action_group;

    action_group = gtk_action_group_new ("RemminaMainFileSensitiveActions");
    gtk_action_group_set_translation_domain (action_group, NULL);
    gtk_action_group_add_actions (action_group, remmina_main_ui_file_sensitive_menu_entries,
        G_N_ELEMENTS (remmina_main_ui_file_sensitive_menu_entries), remminamain);

    gtk_ui_manager_insert_action_group (uimanager, action_group, 0);
    g_object_unref (action_group);
    priv->file_sensitive_group = action_group;

    error = NULL;
    gtk_ui_manager_add_ui_from_string (uimanager, remmina_main_ui_xml, -1, &error);
    if (error)
    {
        g_message ("building menus failed: %s", error->message);
        g_error_free (error);
    }

    menubar = gtk_ui_manager_get_widget (uimanager, "/MenuBar");
    gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);

    priv->toolbar = gtk_ui_manager_get_widget (uimanager, "/ToolBar");
    gtk_box_pack_start (GTK_BOX (vbox), priv->toolbar, FALSE, FALSE, 0);

    /* Customized menu items and toolbar items which can't be built through standard ui manager */
    toolitem = gtk_menu_tool_button_new_from_stock (GTK_STOCK_JUMP_TO);
    gtk_widget_show (GTK_WIDGET (toolitem));
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (toolitem), _("Quick Connect"));
    gtk_toolbar_insert (GTK_TOOLBAR (priv->toolbar), toolitem, 0);
    g_signal_connect (G_OBJECT (toolitem), "clicked",
        G_CALLBACK (remmina_main_action_action_quick_connect), remminamain);

    gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (toolitem),
        gtk_ui_manager_get_widget (uimanager, "/PopupQuickConnectProtoMenu"));

    gtk_window_add_accel_group (GTK_WINDOW (remminamain), gtk_ui_manager_get_accel_group (uimanager));

    gtk_action_group_set_sensitive (priv->file_sensitive_group, FALSE);

    /* Create the scrolled window for the file list */
    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

    /* Create the remmina file list */
    tree = gtk_tree_view_new ();

    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("Name"));
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, NAME_COLUMN);
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_add_attribute (column, renderer, "icon-name", PROTOCOL_COLUMN);
    g_object_set (G_OBJECT (renderer), "stock-size", GTK_ICON_SIZE_LARGE_TOOLBAR, NULL);
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_add_attribute (column, renderer, "text", NAME_COLUMN);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Group"),
        renderer, "text", GROUP_COLUMN, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, GROUP_COLUMN);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
    priv->group_column = column;

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Server"),
        renderer, "text", SERVER_COLUMN, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, SERVER_COLUMN);
    gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);

    gtk_container_add (GTK_CONTAINER (scrolledwindow), tree);
    gtk_widget_show (tree);

    gtk_tree_selection_set_select_function (
        gtk_tree_view_get_selection (GTK_TREE_VIEW (tree)),
        remmina_main_selection_func, remminamain, NULL);
    g_signal_connect (G_OBJECT (tree), "button-press-event",
        G_CALLBACK (remmina_main_file_list_on_button_press), remminamain);

    priv->file_list = tree;

    /* Create statusbar */
    priv->statusbar = gtk_statusbar_new ();
    gtk_box_pack_start (GTK_BOX (vbox), priv->statusbar, FALSE, FALSE, 0);
    gtk_widget_show (priv->statusbar);

    /* Prepare the data */
    remmina_main_load_files (remminamain);

    /* Load the preferences */
    if (remmina_pref.hide_toolbar)
    {
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (
            gtk_action_group_get_action (priv->main_group, "ViewToolbar")), FALSE);
    }
    if (remmina_pref.hide_statusbar)
    {
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (
            gtk_action_group_get_action (priv->main_group, "ViewStatusbar")), FALSE);
    }
    if (remmina_pref.small_toolbutton)
    {
        gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (
            gtk_action_group_get_action (priv->main_group, "ViewSmallToolbutton")), TRUE);
    }

    priv->initialized = TRUE;

    remmina_widget_pool_register (GTK_WIDGET (remminamain));
}

GtkWidget*
remmina_main_new (void)
{
    RemminaMain *window;

    window = REMMINA_MAIN (g_object_new (REMMINA_TYPE_MAIN, NULL));
    return GTK_WIDGET (window);
}

