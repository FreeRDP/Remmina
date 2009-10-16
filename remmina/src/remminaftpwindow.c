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
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include "remminapublic.h"
#include "remminapref.h"
#include "remminamarshals.h"
#include "remminaftpwindow.h"

/* -------------------- RemminaCellRendererPixbuf ----------------------- */
/* A tiny cell renderer that extends the default pixbuf cell render to accept activation */

#define REMMINA_TYPE_CELL_RENDERER_PIXBUF \
    (remmina_cell_renderer_pixbuf_get_type ())
#define REMMINA_CELL_RENDERER_PIXBUF(obj) \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_CELL_RENDERER_PIXBUF, RemminaCellRendererPixbuf))
#define REMMINA_CELL_RENDERER_PIXBUF_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_CELL_RENDERER_PIXBUF, RemminaCellRendererPixbufClass))
#define REMMINA_IS_CELL_RENDERER_PIXBUF(obj) \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_CELL_RENDERER_PIXBUF))

typedef struct _RemminaCellRendererPixbuf
{
    GtkCellRendererPixbuf renderer;
} RemminaCellRendererPixbuf;

typedef struct _RemminaCellRendererPixbufClass
{
    GtkCellRendererPixbufClass parent_class;

    void (* activate) (RemminaCellRendererPixbuf *renderer);
} RemminaCellRendererPixbufClass;

GType remmina_cell_renderer_pixbuf_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (RemminaCellRendererPixbuf, remmina_cell_renderer_pixbuf, GTK_TYPE_CELL_RENDERER_PIXBUF)

static guint remmina_cell_renderer_pixbuf_signals[1] = { 0 };

gboolean
remmina_cell_renderer_pixbuf_activate (GtkCellRenderer *renderer,
    GdkEvent             *event,
    GtkWidget            *widget,
    const gchar          *path,
    GdkRectangle         *background_area,
    GdkRectangle         *cell_area,
    GtkCellRendererState  flags)
{
    g_signal_emit (G_OBJECT (renderer), remmina_cell_renderer_pixbuf_signals[0], 0, path);
    return TRUE;
}

static void
remmina_cell_renderer_pixbuf_class_init (RemminaCellRendererPixbufClass *klass)
{
    GtkCellRendererClass *renderer_class = GTK_CELL_RENDERER_CLASS (klass);

    renderer_class->activate = remmina_cell_renderer_pixbuf_activate;

    remmina_cell_renderer_pixbuf_signals[0] =
        g_signal_new ("activate",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaCellRendererPixbufClass, activate),
            NULL, NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE, 1,
            G_TYPE_STRING);
}

static void
remmina_cell_renderer_pixbuf_init (RemminaCellRendererPixbuf *renderer)
{
    g_object_set (G_OBJECT (renderer), "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
}

static GtkCellRenderer*
remmina_cell_renderer_pixbuf_new (void)
{
    GtkCellRenderer *renderer;

    renderer = GTK_CELL_RENDERER (g_object_new (REMMINA_TYPE_CELL_RENDERER_PIXBUF, NULL));
    return renderer;
}

/* --------------------- RemminaFTPWindow ----------------------------*/
G_DEFINE_TYPE (RemminaFTPWindow, remmina_ftp_window, GTK_TYPE_WINDOW)

#define BUSY_CURSOR \
    gdk_window_set_cursor (GTK_WIDGET (window)->window, gdk_cursor_new (GDK_WATCH));\
    gdk_flush ();

#define NORMAL_CURSOR \
    gdk_window_set_cursor (GTK_WIDGET (window)->window, NULL);

struct _RemminaFTPWindowPriv
{
    GtkUIManager *uimanager;

    GtkWidget *directory_combo;

    GtkTreeModel *file_list_model;
    GtkWidget *file_list_view;
    GtkActionGroup *file_action_group;

    GtkTreeModel *task_list_model;
    GtkWidget *task_list_view;

    gchar *current_directory;
    gchar *working_directory;
};

static gint remmina_ftp_window_taskid = 1;

enum {
    DISCONNECT_SIGNAL,
    OPEN_DIR_SIGNAL,
    NEW_TASK_SIGNAL,
    CANCEL_TASK_SIGNAL,
    DELETE_FILE_SIGNAL,
    LAST_SIGNAL
};

static guint remmina_ftp_window_signals[LAST_SIGNAL] = { 0 };

static void
remmina_ftp_window_class_init (RemminaFTPWindowClass *klass)
{
    remmina_ftp_window_signals[DISCONNECT_SIGNAL] =
        g_signal_new ("disconnect",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaFTPWindowClass, disconnect),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    remmina_ftp_window_signals[OPEN_DIR_SIGNAL] =
        g_signal_new ("open-dir",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaFTPWindowClass, open_dir),
            NULL, NULL,
            g_cclosure_marshal_VOID__STRING,
            G_TYPE_NONE, 1,
            G_TYPE_STRING);
    remmina_ftp_window_signals[NEW_TASK_SIGNAL] =
        g_signal_new ("new-task",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaFTPWindowClass, new_task),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
    remmina_ftp_window_signals[CANCEL_TASK_SIGNAL] =
        g_signal_new ("cancel-task",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaFTPWindowClass, cancel_task),
            NULL, NULL,
            remmina_marshal_BOOLEAN__INT,
            G_TYPE_BOOLEAN, 1,
            G_TYPE_INT);
    remmina_ftp_window_signals[DELETE_FILE_SIGNAL] =
        g_signal_new ("delete-file",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaFTPWindowClass, delete_file),
            NULL, NULL,
            remmina_marshal_BOOLEAN__INT_STRING,
            G_TYPE_BOOLEAN, 2,
            G_TYPE_INT, G_TYPE_STRING);
}

static void
remmina_ftp_window_destroy (RemminaFTPWindow *window, gpointer data)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    g_free (priv->current_directory);
    g_free (priv->working_directory);
    g_free (priv);
}

static void
remmina_ftp_window_cell_data_filetype_pixbuf (GtkTreeViewColumn *col,
    GtkCellRenderer   *renderer,
    GtkTreeModel      *model,
    GtkTreeIter       *iter,
    gpointer           user_data)
{
    gint type;

    /* Same as REMMINA_FTP_TASK_COLUMN_TYPE */
    gtk_tree_model_get (model, iter, REMMINA_FTP_FILE_COLUMN_TYPE, &type, -1);

    switch (type)
    {
    case REMMINA_FTP_FILE_TYPE_DIR:
        g_object_set (renderer, "stock-id", GTK_STOCK_DIRECTORY, NULL);
        break;
    default:
        g_object_set (renderer, "stock-id", GTK_STOCK_FILE, NULL);
        break;
    }
}

static void
remmina_ftp_window_cell_data_progress_pixbuf (GtkTreeViewColumn *col,
    GtkCellRenderer   *renderer,
    GtkTreeModel      *model,
    GtkTreeIter       *iter,
    gpointer           user_data)
{
    gint tasktype, status;

    gtk_tree_model_get (model, iter,
        REMMINA_FTP_TASK_COLUMN_TASKTYPE, &tasktype,
        REMMINA_FTP_TASK_COLUMN_STATUS, &status,
        -1);

    switch (status)
    {
    case REMMINA_FTP_TASK_STATUS_WAIT:
        g_object_set (renderer, "stock-id", GTK_STOCK_MEDIA_PAUSE, NULL);
        break;
    case REMMINA_FTP_TASK_STATUS_RUN:
        g_object_set (renderer, "stock-id",
            (tasktype == REMMINA_FTP_TASK_TYPE_UPLOAD ? GTK_STOCK_GO_UP : GTK_STOCK_GO_DOWN), NULL);
        break;
    case REMMINA_FTP_TASK_STATUS_FINISH:
        g_object_set (renderer, "stock-id", GTK_STOCK_YES, NULL);
        break;
    case REMMINA_FTP_TASK_STATUS_ERROR:
        g_object_set (renderer, "stock-id", GTK_STOCK_NO, NULL);
        break;
    }
}

static gchar*
remmina_ftp_window_size_to_str (gfloat size)
{
    gchar *str;

    if (size < 1024.0)
    {
        str = g_strdup_printf ("%i", (gint) size);
    }
    else if (size < 1024.0 * 1024.0)
    {
        str = g_strdup_printf ("%iK", (gint) (size / 1024.0));
    }
    else if (size < 1024.0 * 1024.0 * 1024.0)
    {
        str = g_strdup_printf ("%.1fM", size / 1024.0 / 1024.0);
    }
    else
    {
        str = g_strdup_printf ("%.1fG", size / 1024.0 / 1024.0 / 1024.0);
    }
    return str;
}

static void
remmina_ftp_window_cell_data_size (GtkTreeViewColumn *col,
    GtkCellRenderer   *renderer,
    GtkTreeModel      *model,
    GtkTreeIter       *iter,
    gpointer           user_data)
{
    gfloat size;
    gchar *str;

    gtk_tree_model_get (model, iter, REMMINA_FTP_FILE_COLUMN_SIZE, &size, -1);

    str = remmina_ftp_window_size_to_str (size);
    g_object_set (renderer, "text", str, NULL);
    g_object_set (renderer, "xalign", 1.0, NULL);
    g_free (str);
}

static void
remmina_ftp_window_cell_data_permission (GtkTreeViewColumn *col,
    GtkCellRenderer   *renderer,
    GtkTreeModel      *model,
    GtkTreeIter       *iter,
    gpointer           user_data)
{
    gint permission;
    gchar buf[11];

    gtk_tree_model_get (model, iter, REMMINA_FTP_FILE_COLUMN_PERMISSION, &permission, -1);

    buf[0] = (permission & 040000 ? 'd' : '-');
    buf[1] = (permission & 0400 ? 'r' : '-');
    buf[2] = (permission & 0200 ? 'w' : '-');
    buf[3] = (permission & 0100 ? 'x' : '-');
    buf[4] = (permission & 040 ? 'r' : '-');
    buf[5] = (permission & 020 ? 'w' : '-');
    buf[6] = (permission & 010 ? 'x' : '-');
    buf[7] = (permission & 04 ? 'r' : '-');
    buf[8] = (permission & 02 ? 'w' : '-');
    buf[9] = (permission & 01 ? 'x' : '-');
    buf[10] = '\0';

    g_object_set (renderer, "text", buf, NULL);
}

static void
remmina_ftp_window_cell_data_size_progress (GtkTreeViewColumn *col,
    GtkCellRenderer   *renderer,
    GtkTreeModel      *model,
    GtkTreeIter       *iter,
    gpointer           user_data)
{
    gint status;
    gfloat size, donesize;
    gchar *strsize, *strdonesize, *str;

    gtk_tree_model_get (model, iter,
        REMMINA_FTP_TASK_COLUMN_STATUS, &status,
        REMMINA_FTP_TASK_COLUMN_SIZE, &size,
        REMMINA_FTP_TASK_COLUMN_DONESIZE, &donesize,
        -1);

    if (status == REMMINA_FTP_TASK_STATUS_FINISH)
    {
        str = remmina_ftp_window_size_to_str (size);
    }
    else
    {
        strsize = remmina_ftp_window_size_to_str (size);
        strdonesize = remmina_ftp_window_size_to_str (donesize);
        str = g_strdup_printf ("%s / %s", strdonesize, strsize);
        g_free (strsize);
        g_free (strdonesize);
    }

    g_object_set (renderer, "text", str, NULL);
    g_object_set (renderer, "xalign", 1.0, NULL);
    g_free (str);
}

static void
remmina_ftp_window_cell_data_progress (GtkTreeViewColumn *col,
    GtkCellRenderer   *renderer,
    GtkTreeModel      *model,
    GtkTreeIter       *iter,
    gpointer           user_data)
{
    gint status;
    gfloat size, donesize;
    gint progress;

    gtk_tree_model_get (model, iter,
        REMMINA_FTP_TASK_COLUMN_STATUS, &status,
        REMMINA_FTP_TASK_COLUMN_SIZE, &size,
        REMMINA_FTP_TASK_COLUMN_DONESIZE, &donesize,
        -1);
    if (status == REMMINA_FTP_TASK_STATUS_FINISH)
    {
        progress = 100;
    }
    else
    {
        if (size <= 1)
        {
            progress = 0;
        }
        else
        {
            progress = (gint) (donesize / size * 100);
            if (progress > 99) progress = 99;
        }
    }
    g_object_set (renderer, "value", progress, NULL);
}

static void
remmina_ftp_window_open_dir (RemminaFTPWindow *window, const gchar *dir)
{
    BUSY_CURSOR
    g_signal_emit (G_OBJECT (window), remmina_ftp_window_signals[OPEN_DIR_SIGNAL], 0, dir);
    NORMAL_CURSOR
}

static void
remmina_ftp_window_dir_on_activate (GtkWidget *widget, RemminaFTPWindow *window)
{
    remmina_ftp_window_open_dir (window, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
remmina_ftp_window_dir_on_changed (GtkWidget *widget, RemminaFTPWindow *window)
{
    GtkWidget *entry = gtk_bin_get_child (GTK_BIN (widget));

    if (!gtk_widget_is_focus (entry))
    {
        gtk_widget_grab_focus (entry);
        /* If the text was changed but the entry is not the focus, it should be changed by the drop-down list.
           Not sure this will always work in the future, but it works right now :) */
        remmina_ftp_window_open_dir (window, gtk_entry_get_text (GTK_ENTRY (entry)));
    }
}

static void
remmina_ftp_window_file_selection_on_changed (GtkTreeSelection *selection, RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GList *list;

    list = gtk_tree_selection_get_selected_rows (selection, NULL);
    gtk_action_group_set_sensitive (priv->file_action_group, (list ? TRUE : FALSE));
    g_list_free (list);
}

static gchar*
remmina_ftp_window_get_download_dir (RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkWidget *dialog;
    gchar *localdir = NULL;

    dialog = gtk_file_chooser_dialog_new (_("Choose download location"), GTK_WINDOW (window),
        GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        NULL);
    if (priv->working_directory)
    {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), priv->working_directory);
    }
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        g_free (priv->working_directory);
        priv->working_directory = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
        localdir = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
    }
    gtk_widget_destroy (dialog);
    return localdir;
}

static void
remmina_ftp_window_download (RemminaFTPWindow *window, GtkTreeIter *piter, const gchar *localdir)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkListStore *store = GTK_LIST_STORE (priv->task_list_model);
    GtkTreeIter iter;
    gint type;
    gchar *name;
    gfloat size;

    gtk_tree_model_get (priv->file_list_model, piter,
        REMMINA_FTP_FILE_COLUMN_TYPE, &type,
        REMMINA_FTP_FILE_COLUMN_NAME, &name,
        REMMINA_FTP_FILE_COLUMN_SIZE, &size,
        -1);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
        REMMINA_FTP_TASK_COLUMN_TYPE, type,
        REMMINA_FTP_TASK_COLUMN_NAME, name,
        REMMINA_FTP_TASK_COLUMN_SIZE, size,
        REMMINA_FTP_TASK_COLUMN_TASKID, remmina_ftp_window_taskid++,
        REMMINA_FTP_TASK_COLUMN_TASKTYPE, REMMINA_FTP_TASK_TYPE_DOWNLOAD,
        REMMINA_FTP_TASK_COLUMN_REMOTEDIR, priv->current_directory,
        REMMINA_FTP_TASK_COLUMN_LOCALDIR, localdir,
        REMMINA_FTP_TASK_COLUMN_STATUS, REMMINA_FTP_TASK_STATUS_WAIT,
        REMMINA_FTP_TASK_COLUMN_DONESIZE, 0.0,
        REMMINA_FTP_TASK_COLUMN_TOOLTIP, NULL,
        -1);

    g_free (name);

    g_signal_emit (G_OBJECT (window), remmina_ftp_window_signals[NEW_TASK_SIGNAL], 0);
}

static gboolean
remmina_ftp_window_file_list_on_button_press (GtkWidget *widget, GdkEventButton *event, RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkWidget *popup;
    GList *list;
    GtkTreeIter iter;
    gint type;
    gchar *name;
    gchar *localdir;

    if (event->button == 3)
    {
        popup = gtk_ui_manager_get_widget (priv->uimanager, "/PopupMenu");
        if (popup)
        {
            gtk_menu_popup (GTK_MENU (popup), NULL, NULL, NULL, NULL, event->button, event->time);
        }
    }
    else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS)
    {
        list = gtk_tree_selection_get_selected_rows (
            gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_list_view)), NULL);
        if (list)
        {
            gtk_tree_model_get_iter (priv->file_list_model, &iter, (GtkTreePath*) list->data);
            gtk_tree_model_get (priv->file_list_model, &iter,
                REMMINA_FTP_FILE_COLUMN_TYPE, &type,
                REMMINA_FTP_FILE_COLUMN_NAME, &name,
                -1);
            switch (type)
            {
            case REMMINA_FTP_FILE_TYPE_DIR:
                remmina_ftp_window_open_dir (window, name);
                break;
            case REMMINA_FTP_FILE_TYPE_FILE:
            default:
                localdir = remmina_ftp_window_get_download_dir (window);
                if (localdir)
                {
                    remmina_ftp_window_download (window, &iter, localdir);
                    g_free (localdir);
                }
                break;
            }
            g_list_free (list);
            g_free (name);
        }
    }

    return FALSE;
}

static gboolean
remmina_ftp_window_task_list_on_query_tooltip (GtkWidget *widget,
    gint          x,
    gint          y,
    gboolean      keyboard_tip,
    GtkTooltip    *tooltip,
    RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkTreeIter iter;
    GtkTreePath *path = NULL;
    gchar *tmp;

    if (!gtk_tree_view_get_tooltip_context (GTK_TREE_VIEW (priv->task_list_view),
        &x, &y, keyboard_tip, NULL, &path, &iter))
    {
        return FALSE;
    }

    gtk_tree_model_get (priv->task_list_model, &iter, REMMINA_FTP_TASK_COLUMN_TOOLTIP, &tmp, -1);
    if (!tmp) return FALSE;

    gtk_tooltip_set_text (tooltip, tmp);

    gtk_tree_view_set_tooltip_row (GTK_TREE_VIEW (priv->task_list_view), tooltip, path);

    gtk_tree_path_free (path);
    g_free (tmp);

    return TRUE;
}

static void
remmina_ftp_window_action_parent (GtkAction *action, RemminaFTPWindow *window)
{
    remmina_ftp_window_open_dir (window, "..");
}

static void
remmina_ftp_window_action_home (GtkAction *action, RemminaFTPWindow *window)
{
    remmina_ftp_window_open_dir (window, NULL);
}

static void
remmina_ftp_window_action_refresh (GtkAction *action, RemminaFTPWindow *window)
{
    remmina_ftp_window_open_dir (window, ".");
}

static void
remmina_ftp_window_action_download (GtkAction *action, RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkTreeSelection *selection;
    gchar *localdir;
    GList *list, *list_iter;
    GtkTreeIter iter;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_list_view));
    if (!selection) return;
    list = gtk_tree_selection_get_selected_rows (selection, NULL);
    if (!list) return;

    localdir = remmina_ftp_window_get_download_dir (window);
    if (!localdir)
    {
        g_list_free (list);
        return;
    }

    list_iter = g_list_first (list);
    while (list_iter)
    {
        gtk_tree_model_get_iter (priv->file_list_model, &iter, (GtkTreePath*) list_iter->data);
        remmina_ftp_window_download (window, &iter, localdir);
        list_iter = g_list_next (list_iter);
    }
    g_list_free (list);
    g_free (localdir);
}

static void
remmina_ftp_window_action_delete (GtkAction *action, RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkWidget *dialog;
    GtkTreeSelection *selection;
    GList *list, *list_iter;
    GtkTreeIter iter;
    gint type;
    gchar *name;
    gchar *path;
    gint response;
    gboolean ret = TRUE;

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_list_view));
    if (!selection) return;
    list = gtk_tree_selection_get_selected_rows (selection, NULL);
    if (!list) return;

    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("Are you sure to delete the selected files on server?"));
    response = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (response != GTK_RESPONSE_YES) return;

    BUSY_CURSOR

    list_iter = g_list_first (list);
    while (list_iter)
    {
        gtk_tree_model_get_iter (priv->file_list_model, &iter, (GtkTreePath*) list_iter->data);

        gtk_tree_model_get (priv->file_list_model, &iter,
            REMMINA_FTP_FILE_COLUMN_TYPE, &type,
            REMMINA_FTP_FILE_COLUMN_NAME, &name,
            -1);

        path = remmina_public_combine_path (priv->current_directory, name);
        g_signal_emit (G_OBJECT (window), remmina_ftp_window_signals[DELETE_FILE_SIGNAL], 0, type, path, &ret);
        g_free (name);
        g_free (path);
        if (!ret) break;

        list_iter = g_list_next (list_iter);
    }
    g_list_free (list);

    NORMAL_CURSOR

    if (ret)
    {
        remmina_ftp_window_action_refresh (action, window);
    }
}

static void
remmina_ftp_window_action_upload (GtkAction *action, RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkListStore *store = GTK_LIST_STORE (priv->task_list_model);
    GtkTreeIter iter;
    GtkWidget *dialog;
    GSList *files = NULL;
    GSList *element;
    gchar *path;
    gchar *dir, *name;
    struct stat st;

    dialog = gtk_file_chooser_dialog_new (_("Choose a file to upload"), GTK_WINDOW (window),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (dialog), TRUE);
    if (priv->working_directory)
    {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (dialog), priv->working_directory);
    }
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        g_free (priv->working_directory);
        priv->working_directory = gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (dialog));
        files = gtk_file_chooser_get_filenames (GTK_FILE_CHOOSER (dialog));
    }
    gtk_widget_destroy (dialog);
    if (!files) return;


    for (element = files; element; element = element->next)
    {
        path = (gchar*) element->data;

        if (g_stat (path, &st) < 0) continue;

        name = g_strrstr (path, "/");
        if (name)
        {
            *name++ = '\0';
            dir = path;
        }
        else
        {
            name = path;
            dir = NULL;
        }

        gtk_list_store_append (store, &iter);
        gtk_list_store_set (store, &iter,
            REMMINA_FTP_TASK_COLUMN_TYPE, REMMINA_FTP_FILE_TYPE_FILE,
            REMMINA_FTP_TASK_COLUMN_NAME, name,
            REMMINA_FTP_TASK_COLUMN_SIZE, (gfloat) st.st_size,
            REMMINA_FTP_TASK_COLUMN_TASKID, remmina_ftp_window_taskid++,
            REMMINA_FTP_TASK_COLUMN_TASKTYPE, REMMINA_FTP_TASK_TYPE_UPLOAD,
            REMMINA_FTP_TASK_COLUMN_REMOTEDIR, priv->current_directory,
            REMMINA_FTP_TASK_COLUMN_LOCALDIR, dir,
            REMMINA_FTP_TASK_COLUMN_STATUS, REMMINA_FTP_TASK_STATUS_WAIT,
            REMMINA_FTP_TASK_COLUMN_DONESIZE, 0.0,
            REMMINA_FTP_TASK_COLUMN_TOOLTIP, NULL,
            -1);

        g_free (path);
    }

    g_slist_free (files);

    g_signal_emit (G_OBJECT (window), remmina_ftp_window_signals[NEW_TASK_SIGNAL], 0);
}

static void
remmina_ftp_window_task_list_cell_on_activate (GtkCellRenderer *renderer, gchar *path, RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkTreeIter iter;
    GtkTreePath *treepath;
    gint taskid;
    gboolean ret = FALSE;

    treepath = gtk_tree_path_new_from_string (path);
    gtk_tree_model_get_iter (priv->task_list_model, &iter, treepath);
    gtk_tree_path_free (treepath);

    gtk_tree_model_get (priv->task_list_model, &iter, REMMINA_FTP_TASK_COLUMN_TASKID, &taskid, -1);

    g_signal_emit (G_OBJECT (window), remmina_ftp_window_signals[CANCEL_TASK_SIGNAL], 0, taskid, &ret);

    if (ret)
    {
        gtk_list_store_remove (GTK_LIST_STORE (priv->task_list_model), &iter);
    }
}

static void
remmina_ftp_window_action_disconnect (GtkAction *action, RemminaFTPWindow *window)
{
    g_signal_emit (G_OBJECT (window), remmina_ftp_window_signals[DISCONNECT_SIGNAL], 0);
}

static const gchar *remmina_ftp_window_ui_xml = 
"<ui>"
"  <toolbar name='ToolBar'>"
"    <toolitem action='Home'/>"
"    <toolitem action='Parent'/>"
"    <toolitem action='Refresh'/>"
"    <separator/>"
"    <toolitem action='Download'/>"
"    <toolitem action='Upload'/>"
"    <toolitem action='Delete'/>"
"    <separator/>"
"    <toolitem action='Disconnect'/>"
"  </toolbar>"
"  <popup name='PopupMenu'>"
"    <menuitem action='Download'/>"
"    <menuitem action='Upload'/>"
"    <menuitem action='Delete'/>"
"  </popup>"
"</ui>";

static const GtkActionEntry remmina_ftp_window_ui_action_entries[] =
{
    { "Home", GTK_STOCK_HOME, NULL, "<control>H",
        N_("Go to home folder"),
        G_CALLBACK (remmina_ftp_window_action_home) },

    { "Parent", GTK_STOCK_GO_UP, NULL, "<control>U",
        N_("Go to parent folder"),
        G_CALLBACK (remmina_ftp_window_action_parent) },

    { "Refresh", GTK_STOCK_REFRESH, NULL, "<control>R",
        N_("Refresh current folder"),
        G_CALLBACK (remmina_ftp_window_action_refresh) },

    { "Upload", "document-send", N_("Upload"), "<control>P",
        N_("Upload to server"),
        G_CALLBACK (remmina_ftp_window_action_upload) },

    { "Disconnect", GTK_STOCK_DISCONNECT, NULL, "<control>X",
        NULL,
        G_CALLBACK (remmina_ftp_window_action_disconnect) }
};

static const GtkActionEntry remmina_ftp_window_ui_file_action_entries[] =
{
    { "Download", "document-save", N_("Download"), "<control>D",
        N_("Download from server"),
        G_CALLBACK (remmina_ftp_window_action_download) },

    { "Delete", GTK_STOCK_DELETE, NULL, "Delete",
        N_("Delete files on server"),
        G_CALLBACK (remmina_ftp_window_action_delete) }
};

static void
remmina_ftp_window_init (RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv;
    GtkWidget *vbox;
    GtkUIManager *uimanager;
    GtkActionGroup *action_group;
    GtkWidget *vpaned;
    GtkWidget *scrolledwindow;
    GtkWidget *widget;
    GtkCellRenderer *renderer;
    GtkTreeViewColumn *column;
    GError *error;

    priv = g_new (RemminaFTPWindowPriv, 1);
    window->priv = priv;
    priv->current_directory = NULL;
    priv->working_directory = NULL;

    gtk_window_set_default_size (GTK_WINDOW (window), 700, 500);

    /* Main container */
    vbox = gtk_vbox_new (FALSE, 4);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    /* Toolbar */
    uimanager = gtk_ui_manager_new ();
    priv->uimanager = uimanager;

    action_group = gtk_action_group_new ("RemminaFTPWindowActions");
    gtk_action_group_add_actions (action_group, remmina_ftp_window_ui_action_entries,
        G_N_ELEMENTS (remmina_ftp_window_ui_action_entries), window);
    gtk_ui_manager_insert_action_group (uimanager, action_group, 0);
    g_object_unref (action_group);

    action_group = gtk_action_group_new ("RemminaFTPWindowFileActions");
    gtk_action_group_add_actions (action_group, remmina_ftp_window_ui_file_action_entries,
        G_N_ELEMENTS (remmina_ftp_window_ui_file_action_entries), window);
    gtk_action_group_set_sensitive (action_group, FALSE);
    gtk_ui_manager_insert_action_group (uimanager, action_group, 0);
    g_object_unref (action_group);
    priv->file_action_group = action_group;

    error = NULL;
    gtk_ui_manager_add_ui_from_string (uimanager, remmina_ftp_window_ui_xml, -1, &error);
    if (error)
    {
        g_message ("building UI failed: %s", error->message);
        g_error_free (error);
    }

    widget = gtk_ui_manager_get_widget (uimanager, "/ToolBar");
    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);
    if (remmina_pref.small_toolbutton)
    {
        gtk_toolbar_set_icon_size (GTK_TOOLBAR (widget), GTK_ICON_SIZE_MENU);
    }

    gtk_window_add_accel_group (GTK_WINDOW (window), gtk_ui_manager_get_accel_group (uimanager));

    /* Remote Directory */
    widget = gtk_combo_box_entry_new_text ();
    gtk_widget_show (widget);
    gtk_combo_box_append_text (GTK_COMBO_BOX (widget), "/");
    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, TRUE, 0);

    priv->directory_combo = widget;

    /* The Paned to separate File List and Task List */
    vpaned = gtk_vpaned_new ();
    gtk_widget_show (vpaned);
    gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);
    gtk_paned_set_position (GTK_PANED (vpaned), 300);

    /* Remote File List */
    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_paned_pack1 (GTK_PANED (vpaned), scrolledwindow, FALSE, TRUE);

    widget = gtk_tree_view_new ();
    gtk_widget_show (widget);
    gtk_container_add (GTK_CONTAINER (scrolledwindow), widget);

    gtk_tree_selection_set_mode (
        gtk_tree_view_get_selection (GTK_TREE_VIEW (widget)),
        GTK_SELECTION_MULTIPLE);

    priv->file_list_view = widget;

    /* Remote File List - Columns */
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("File Name"));
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_FILE_COLUMN_NAME_SORT);
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, remmina_ftp_window_cell_data_filetype_pixbuf, NULL, NULL);
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_add_attribute (column, renderer, "text", REMMINA_FTP_FILE_COLUMN_NAME);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->file_list_view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Size"), renderer, NULL);
    gtk_tree_view_column_set_alignment (column, 1.0);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, remmina_ftp_window_cell_data_size, NULL, NULL);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_FILE_COLUMN_SIZE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->file_list_view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("User"),
        renderer, "text", REMMINA_FTP_FILE_COLUMN_USER, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_FILE_COLUMN_USER);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->file_list_view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Group"),
        renderer, "text", REMMINA_FTP_FILE_COLUMN_GROUP, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_FILE_COLUMN_GROUP);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->file_list_view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Permission"),
        renderer, "text", REMMINA_FTP_FILE_COLUMN_PERMISSION, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, remmina_ftp_window_cell_data_permission, NULL, NULL);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_FILE_COLUMN_PERMISSION);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->file_list_view), column);

    /* Remote File List - Model */
    priv->file_list_model = GTK_TREE_MODEL (gtk_list_store_new (REMMINA_FTP_FILE_N_COLUMNS,
        G_TYPE_INT, G_TYPE_STRING, G_TYPE_FLOAT,
        G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING));
    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->file_list_view), priv->file_list_model);
    gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (priv->file_list_model),
        REMMINA_FTP_FILE_COLUMN_NAME_SORT, GTK_SORT_ASCENDING);

    /* Task List */
    scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
    gtk_widget_show (scrolledwindow);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
        GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
    gtk_paned_pack2 (GTK_PANED (vpaned), scrolledwindow, FALSE, TRUE);

    widget = gtk_tree_view_new ();
    gtk_widget_show (widget);
    gtk_container_add (GTK_CONTAINER (scrolledwindow), widget);
    g_object_set (widget, "has-tooltip", TRUE, NULL);

    priv->task_list_view = widget;

    /* Task List - Columns */
    column = gtk_tree_view_column_new ();
    gtk_tree_view_column_set_title (column, _("File Name"));
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_expand (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_TASK_COLUMN_NAME);
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, remmina_ftp_window_cell_data_progress_pixbuf, NULL, NULL);
    renderer = gtk_cell_renderer_pixbuf_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, remmina_ftp_window_cell_data_filetype_pixbuf, NULL, NULL);
    renderer = gtk_cell_renderer_text_new ();
    gtk_tree_view_column_pack_start (column, renderer, FALSE);
    gtk_tree_view_column_add_attribute (column, renderer, "text", REMMINA_FTP_FILE_COLUMN_NAME);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->task_list_view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Remote"),
        renderer, "text", REMMINA_FTP_TASK_COLUMN_REMOTEDIR, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_TASK_COLUMN_REMOTEDIR);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->task_list_view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Local"),
        renderer, "text", REMMINA_FTP_TASK_COLUMN_LOCALDIR, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_TASK_COLUMN_LOCALDIR);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->task_list_view), column);

    renderer = gtk_cell_renderer_text_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Size"), renderer, NULL);
    gtk_tree_view_column_set_alignment (column, 1.0);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, remmina_ftp_window_cell_data_size_progress, NULL, NULL);
    gtk_tree_view_column_set_sort_column_id (column, REMMINA_FTP_TASK_COLUMN_SIZE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->task_list_view), column);

    renderer = gtk_cell_renderer_progress_new ();
    column = gtk_tree_view_column_new_with_attributes (_("Progress"), renderer, NULL);
    gtk_tree_view_column_set_resizable (column, TRUE);
    gtk_tree_view_column_set_cell_data_func (column, renderer, remmina_ftp_window_cell_data_progress, NULL, NULL);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->task_list_view), column);

    renderer = remmina_cell_renderer_pixbuf_new ();
    column = gtk_tree_view_column_new_with_attributes (NULL, renderer, NULL);
    g_object_set (G_OBJECT (renderer), "stock-id", GTK_STOCK_CANCEL, NULL);
    gtk_tree_view_column_set_resizable (column, FALSE);
    gtk_tree_view_append_column (GTK_TREE_VIEW (priv->task_list_view), column);

    g_signal_connect (G_OBJECT (renderer), "activate",
        G_CALLBACK (remmina_ftp_window_task_list_cell_on_activate), window);

    /* Task List - Model */
    priv->task_list_model = GTK_TREE_MODEL (gtk_list_store_new (REMMINA_FTP_TASK_N_COLUMNS,
        G_TYPE_INT, G_TYPE_STRING, G_TYPE_FLOAT, 
        G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_STRING));
    gtk_tree_view_set_model (GTK_TREE_VIEW (priv->task_list_view), priv->task_list_model);

    /* Setup the internal signals */
    g_signal_connect (G_OBJECT (window), "destroy",
        G_CALLBACK (remmina_ftp_window_destroy), NULL);
    g_signal_connect (G_OBJECT (gtk_bin_get_child (GTK_BIN (priv->directory_combo))), "activate",
        G_CALLBACK (remmina_ftp_window_dir_on_activate), window);
    g_signal_connect (G_OBJECT (priv->directory_combo), "changed",
        G_CALLBACK (remmina_ftp_window_dir_on_changed), window);
    g_signal_connect (G_OBJECT (priv->file_list_view), "button-press-event",
        G_CALLBACK (remmina_ftp_window_file_list_on_button_press), window);
    g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->file_list_view))), "changed",
        G_CALLBACK (remmina_ftp_window_file_selection_on_changed), window);
    g_signal_connect (G_OBJECT (priv->task_list_view), "query-tooltip",
        G_CALLBACK (remmina_ftp_window_task_list_on_query_tooltip), window);
}

GtkWidget*
remmina_ftp_window_new (GtkWindow *parent, const gchar *server_name)
{
    RemminaFTPWindow *window;
    gchar buf[200];

    window = REMMINA_FTP_WINDOW (g_object_new (REMMINA_TYPE_FTP_WINDOW, NULL));

    if (parent)
    {
        gtk_window_set_transient_for (GTK_WINDOW (window), parent);
    }

    /* Title */
    g_snprintf (buf, sizeof (buf), _("File Transfer: %s"), server_name);
    gtk_window_set_title (GTK_WINDOW (window), buf);

    return GTK_WIDGET (window);
}

void
remmina_ftp_window_clear_file_list (RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;

    gtk_list_store_clear (GTK_LIST_STORE (priv->file_list_model));
    gtk_action_group_set_sensitive (priv->file_action_group, FALSE);
}

void
remmina_ftp_window_add_file (RemminaFTPWindow *window, ...)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkListStore *store = GTK_LIST_STORE (priv->file_list_model);
    GtkTreeIter iter;
    va_list args;
    gint type;
    gchar *name;
    gchar *ptr;

    va_start (args, window);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set_valist (store, &iter, args);
    va_end (args);

    gtk_tree_model_get (GTK_TREE_MODEL (store), &iter,
        REMMINA_FTP_FILE_COLUMN_TYPE, &type,
        REMMINA_FTP_FILE_COLUMN_NAME, &name,
        -1);

    ptr = g_strdup_printf ("%i%s", type, name);
    gtk_list_store_set (store, &iter, REMMINA_FTP_FILE_COLUMN_NAME_SORT, ptr, -1);
    g_free (ptr);
    g_free (name);
}

void
remmina_ftp_window_set_dir (RemminaFTPWindow *window, const gchar *dir)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkTreeModel *model;
    GtkTreeIter iter;
    gboolean ret;
    gchar *t;

    if (priv->current_directory && g_strcmp0 (priv->current_directory, dir) == 0) return;

    model = gtk_combo_box_get_model (GTK_COMBO_BOX (priv->directory_combo));
    for (ret = gtk_tree_model_get_iter_first (model, &iter); ret; ret = gtk_tree_model_iter_next (model, &iter))
    {
        gtk_tree_model_get (model, &iter, 0, &t, -1);
        if (g_strcmp0 (t, dir) == 0)
        {
            gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
            g_free (t);
            break;
        }
        g_free (t);
    }

    gtk_combo_box_prepend_text (GTK_COMBO_BOX (priv->directory_combo), dir);
    gtk_combo_box_set_active (GTK_COMBO_BOX (priv->directory_combo), 0);

    g_free (priv->current_directory);
    priv->current_directory = g_strdup (dir);
}

gchar*
remmina_ftp_window_get_dir (RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;

    return g_strdup (priv->current_directory);
}

RemminaFTPTask*
remmina_ftp_window_get_waiting_task (RemminaFTPWindow *window)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkTreePath *path;
    GtkTreeIter iter;
    RemminaFTPTask task;

    if (!gtk_tree_model_get_iter_first (priv->task_list_model, &iter)) return NULL;

    while (1)
    {
        gtk_tree_model_get (priv->task_list_model, &iter,
            REMMINA_FTP_TASK_COLUMN_TYPE, &task.type,
            REMMINA_FTP_TASK_COLUMN_NAME, &task.name,
            REMMINA_FTP_TASK_COLUMN_SIZE, &task.size,
            REMMINA_FTP_TASK_COLUMN_TASKID, &task.taskid,
            REMMINA_FTP_TASK_COLUMN_TASKTYPE, &task.tasktype,
            REMMINA_FTP_TASK_COLUMN_REMOTEDIR, &task.remotedir,
            REMMINA_FTP_TASK_COLUMN_LOCALDIR, &task.localdir,
            REMMINA_FTP_TASK_COLUMN_STATUS, &task.status,
            REMMINA_FTP_TASK_COLUMN_DONESIZE, &task.donesize,
            REMMINA_FTP_TASK_COLUMN_TOOLTIP, &task.tooltip,
            -1);
        if (task.status == REMMINA_FTP_TASK_STATUS_WAIT)
        {
            path = gtk_tree_model_get_path (priv->task_list_model, &iter);
            task.rowref = gtk_tree_row_reference_new (priv->task_list_model, path);
            gtk_tree_path_free (path);
            return (RemminaFTPTask*) g_memdup (&task, sizeof (RemminaFTPTask));
        }
        if (!gtk_tree_model_iter_next (priv->task_list_model, &iter)) break;
    }
    
    return NULL;
}

void
remmina_ftp_window_update_task (RemminaFTPWindow *window, RemminaFTPTask* task)
{
    RemminaFTPWindowPriv *priv = (RemminaFTPWindowPriv*) window->priv;
    GtkListStore *store = GTK_LIST_STORE (priv->task_list_model);
    GtkTreePath *path;
    GtkTreeIter iter;

    path = gtk_tree_row_reference_get_path (task->rowref);
    gtk_tree_model_get_iter (priv->task_list_model, &iter, path);
    gtk_tree_path_free (path);
    gtk_list_store_set (store, &iter,
        REMMINA_FTP_TASK_COLUMN_SIZE, task->size,
        REMMINA_FTP_TASK_COLUMN_STATUS, task->status,
        REMMINA_FTP_TASK_COLUMN_DONESIZE, task->donesize,
        REMMINA_FTP_TASK_COLUMN_TOOLTIP, task->tooltip,
        -1);
}

void
remmina_ftp_task_free (RemminaFTPTask *task)
{
    if (task)
    {
        g_free (task->name);
        g_free (task->remotedir);
        g_free (task->localdir);
        g_free (task->tooltip);
        g_free (task);
    }
}

