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

#define _FILE_OFFSET_BITS 64

#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include "config.h"
#include "remmina_public.h"
#include "remmina_pref.h"
#include "remmina_marshals.h"
#include "remmina_file.h"
#include "remmina_ftp_client.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"

/* -------------------- RemminaCellRendererPixbuf ----------------------- */
/* A tiny cell renderer that extends the default pixbuf cell render to accept activation */

#define REMMINA_TYPE_CELL_RENDERER_PIXBUF \
	(remmina_cell_renderer_pixbuf_get_type())
#define REMMINA_CELL_RENDERER_PIXBUF(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST((obj), REMMINA_TYPE_CELL_RENDERER_PIXBUF, RemminaCellRendererPixbuf))
#define REMMINA_CELL_RENDERER_PIXBUF_CLASS(klass) \
	(G_TYPE_CHECK_CLASS_CAST((klass), REMMINA_TYPE_CELL_RENDERER_PIXBUF, RemminaCellRendererPixbufClass))
#define REMMINA_IS_CELL_RENDERER_PIXBUF(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE((obj), REMMINA_TYPE_CELL_RENDERER_PIXBUF))

typedef struct _RemminaCellRendererPixbuf {
	GtkCellRendererPixbuf renderer;
} RemminaCellRendererPixbuf;

typedef struct _RemminaCellRendererPixbufClass {
	GtkCellRendererPixbufClass parent_class;

	void (*activate)(RemminaCellRendererPixbuf * renderer);
} RemminaCellRendererPixbufClass;

GType remmina_cell_renderer_pixbuf_get_type(void)
G_GNUC_CONST;

G_DEFINE_TYPE(RemminaCellRendererPixbuf, remmina_cell_renderer_pixbuf, GTK_TYPE_CELL_RENDERER_PIXBUF)

static guint remmina_cell_renderer_pixbuf_signals[1] =
{ 0 };

static gboolean remmina_cell_renderer_pixbuf_activate(GtkCellRenderer *renderer, GdkEvent *event, GtkWidget *widget,
						      const gchar *path, const GdkRectangle *background_area, const GdkRectangle *cell_area,
						      GtkCellRendererState flags)
{
	TRACE_CALL(__func__);
	g_signal_emit(G_OBJECT(renderer), remmina_cell_renderer_pixbuf_signals[0], 0, path);
	return TRUE;
}

static void remmina_cell_renderer_pixbuf_class_init(RemminaCellRendererPixbufClass *klass)
{
	TRACE_CALL(__func__);
	GtkCellRendererClass *renderer_class = GTK_CELL_RENDERER_CLASS(klass);

	renderer_class->activate = remmina_cell_renderer_pixbuf_activate;

	remmina_cell_renderer_pixbuf_signals[0] = g_signal_new("activate", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaCellRendererPixbufClass, activate), NULL,
		NULL, g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void remmina_cell_renderer_pixbuf_init(RemminaCellRendererPixbuf *renderer)
{
	TRACE_CALL(__func__);
	g_object_set(G_OBJECT(renderer), "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
}

static GtkCellRenderer*
remmina_cell_renderer_pixbuf_new(void)
{
	TRACE_CALL(__func__);
	GtkCellRenderer *renderer;

	renderer = GTK_CELL_RENDERER(g_object_new(REMMINA_TYPE_CELL_RENDERER_PIXBUF, NULL));
	return renderer;
}

/* --------------------- RemminaFTPClient ----------------------------*/
G_DEFINE_TYPE( RemminaFTPClient, remmina_ftp_client, GTK_TYPE_GRID)

#define BUSY_CURSOR \
	if (GDK_IS_WINDOW(gtk_widget_get_window(GTK_WIDGET(client)))) \
	{ \
		gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(client)), gdk_cursor_new_for_display(gdk_display_get_default(), GDK_WATCH)); \
		gdk_display_flush(gdk_display_get_default()); \
	}

#define NORMAL_CURSOR \
	if (GDK_IS_WINDOW(gtk_widget_get_window(GTK_WIDGET(client)))) \
	{ \
		gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(client)), NULL); \
	}

struct _RemminaFTPClientPriv {
	GtkWidget *directory_combo;
	GtkWidget *vpaned;

	GtkTreeModel *file_list_model;
	GtkTreeModel *file_list_filter;
	GtkTreeModel *file_list_sort;
	GtkWidget *file_list_view;
	gboolean file_list_show_hidden;

	GtkTreeModel *task_list_model;
	GtkWidget *task_list_view;

	gchar *current_directory;
	gchar *working_directory;

	GtkWidget *file_action_widgets[10];
	gboolean sensitive;
	gboolean overwrite_all;
};

static gint remmina_ftp_client_taskid = 1;

enum {
	OPEN_DIR_SIGNAL, NEW_TASK_SIGNAL, CANCEL_TASK_SIGNAL, DELETE_FILE_SIGNAL, LAST_SIGNAL
};

static guint remmina_ftp_client_signals[LAST_SIGNAL] =
{ 0 };

static void remmina_ftp_client_class_init(RemminaFTPClientClass *klass)
{
	TRACE_CALL(__func__);
	remmina_ftp_client_signals[OPEN_DIR_SIGNAL] = g_signal_new("open-dir", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaFTPClientClass, open_dir), NULL, NULL,
		g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
	remmina_ftp_client_signals[NEW_TASK_SIGNAL] = g_signal_new("new-task", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaFTPClientClass, new_task), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_ftp_client_signals[CANCEL_TASK_SIGNAL] = g_signal_new("cancel-task", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaFTPClientClass, cancel_task), NULL, NULL,
		remmina_marshal_BOOLEAN__INT, G_TYPE_BOOLEAN, 1, G_TYPE_INT);
	remmina_ftp_client_signals[DELETE_FILE_SIGNAL] = g_signal_new("delete-file", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaFTPClientClass, delete_file), NULL, NULL,
		remmina_marshal_BOOLEAN__INT_STRING, G_TYPE_BOOLEAN, 2, G_TYPE_INT, G_TYPE_STRING);
}

static void remmina_ftp_client_destroy(RemminaFTPClient *client, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	g_free(priv->current_directory);
	g_free(priv->working_directory);
	g_free(priv);
}

static void remmina_ftp_client_cell_data_filetype_pixbuf(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
							 GtkTreeIter *iter, gpointer user_data)
{
	TRACE_CALL(__func__);
	gint type;

	/* Same as REMMINA_FTP_TASK_COLUMN_TYPE */
	gtk_tree_model_get(model, iter, REMMINA_FTP_FILE_COLUMN_TYPE, &type, -1);

	switch (type) {
	case REMMINA_FTP_FILE_TYPE_DIR:
		g_object_set(renderer, "stock-id", "folder", NULL);
		break;
	default:
		g_object_set(renderer, "stock-id", "text-x-generic", NULL);
		break;
	}
}

static void remmina_ftp_client_cell_data_progress_pixbuf(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
							 GtkTreeIter *iter, gpointer user_data)
{
	TRACE_CALL(__func__);
	gint tasktype, status;

	gtk_tree_model_get(model, iter, REMMINA_FTP_TASK_COLUMN_TASKTYPE, &tasktype, REMMINA_FTP_TASK_COLUMN_STATUS, &status,
		-1);

	switch (status) {
	case REMMINA_FTP_TASK_STATUS_WAIT:
		g_object_set(renderer, "stock-id", "P_ause", NULL);
		break;
	case REMMINA_FTP_TASK_STATUS_RUN:
		g_object_set(renderer, "stock-id",
			(tasktype == REMMINA_FTP_TASK_TYPE_UPLOAD ? "go-up" : "go-down"), NULL);
		break;
	case REMMINA_FTP_TASK_STATUS_FINISH:
		g_object_set(renderer, "stock-id", "_Yes", NULL);
		break;
	case REMMINA_FTP_TASK_STATUS_ERROR:
		g_object_set(renderer, "stock-id", "_No", NULL);
		break;
	}
}

static gchar*
remmina_ftp_client_size_to_str(gfloat size)
{
	TRACE_CALL(__func__);

	return g_format_size_full((guint64)size, G_FORMAT_SIZE_IEC_UNITS);
}

static void remmina_ftp_client_cell_data_size(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
					      GtkTreeIter *iter, gpointer user_data)
{
	TRACE_CALL(__func__);
	gfloat size;
	gchar *str;

	gtk_tree_model_get(model, iter, REMMINA_FTP_FILE_COLUMN_SIZE, &size, -1);

	str = remmina_ftp_client_size_to_str(size);
	g_object_set(renderer, "text", str, NULL);
	g_object_set(renderer, "xalign", 1.0, NULL);
	g_free(str);
}

static void remmina_ftp_client_cell_data_permission(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
						    GtkTreeIter *iter, gpointer user_data)
{
	TRACE_CALL(__func__);
	gint permission;
	gchar buf[11];

	gtk_tree_model_get(model, iter, REMMINA_FTP_FILE_COLUMN_PERMISSION, &permission, -1);

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

	g_object_set(renderer, "text", buf, NULL);
}

static void remmina_ftp_client_cell_data_size_progress(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
						       GtkTreeIter *iter, gpointer user_data)
{
	TRACE_CALL(__func__);
	gint status;
	gfloat size, donesize;
	gchar *strsize, *strdonesize, *str;

	gtk_tree_model_get(model, iter, REMMINA_FTP_TASK_COLUMN_STATUS, &status, REMMINA_FTP_TASK_COLUMN_SIZE, &size,
		REMMINA_FTP_TASK_COLUMN_DONESIZE, &donesize, -1);

	if (status == REMMINA_FTP_TASK_STATUS_FINISH) {
		str = remmina_ftp_client_size_to_str(size);
	}else  {
		strsize = remmina_ftp_client_size_to_str(size);
		strdonesize = remmina_ftp_client_size_to_str(donesize);
		str = g_strdup_printf("%s / %s", strdonesize, strsize);
		g_free(strsize);
		g_free(strdonesize);
	}

	g_object_set(renderer, "text", str, NULL);
	g_object_set(renderer, "xalign", 1.0, NULL);
	g_free(str);
}

static void remmina_ftp_client_cell_data_progress(GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model,
						  GtkTreeIter *iter, gpointer user_data)
{
	TRACE_CALL(__func__);
	gint status;
	gfloat size, donesize;
	gint progress;

	gtk_tree_model_get(model, iter, REMMINA_FTP_TASK_COLUMN_STATUS, &status, REMMINA_FTP_TASK_COLUMN_SIZE, &size,
		REMMINA_FTP_TASK_COLUMN_DONESIZE, &donesize, -1);
	if (status == REMMINA_FTP_TASK_STATUS_FINISH) {
		progress = 100;
	}else  {
		if (size <= 1) {
			progress = 0;
		}else  {
			progress = (gint)(donesize / size * 100);
			if (progress > 99)
				progress = 99;
		}
	}
	g_object_set(renderer, "value", progress, NULL);
}

static void remmina_ftp_client_open_dir(RemminaFTPClient *client, const gchar *dir)
{
	TRACE_CALL(__func__);
	BUSY_CURSOR
	g_signal_emit(G_OBJECT(client), remmina_ftp_client_signals[OPEN_DIR_SIGNAL], 0, dir);
	NORMAL_CURSOR
}

static void remmina_ftp_client_dir_on_activate(GtkWidget *widget, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	remmina_ftp_client_open_dir(client, gtk_entry_get_text(GTK_ENTRY(widget)));
}

static void remmina_ftp_client_dir_on_changed(GtkWidget *widget, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	GtkWidget *entry = gtk_bin_get_child(GTK_BIN(widget));

	if (!gtk_widget_is_focus(entry)) {
		gtk_widget_grab_focus(entry);
		/* If the text was changed but the entry is not the focus, it should be changed by the drop-down list.
		   Not sure this will always work in the future, but it works right now :) */
		remmina_ftp_client_open_dir(client, gtk_entry_get_text(GTK_ENTRY(entry)));
	}
}

static void remmina_ftp_client_set_file_action_sensitive(RemminaFTPClient *client, gboolean sensitive)
{
	TRACE_CALL(__func__);
	gint i;
	for (i = 0; client->priv->file_action_widgets[i]; i++) {
		gtk_widget_set_sensitive(client->priv->file_action_widgets[i], sensitive);
	}
	client->priv->sensitive = sensitive;
}

static void remmina_ftp_client_file_selection_on_changed(GtkTreeSelection *selection, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	GList *list;

	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	remmina_ftp_client_set_file_action_sensitive(client, (list ? TRUE : FALSE));
	g_list_free(list);
}

static gchar*
remmina_ftp_client_get_download_dir(RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkWidget *dialog;
	gchar *localdir = NULL;

	dialog = gtk_file_chooser_dialog_new(_("Choose download location"),
		GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(client))), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
		"_Cancel", GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_ACCEPT, NULL);
	if (priv->working_directory) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), priv->working_directory);
	}
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		g_free(priv->working_directory);
		priv->working_directory = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		localdir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	}
	gtk_widget_destroy(dialog);
	return localdir;
}

static void remmina_ftp_client_download(RemminaFTPClient *client, GtkTreeIter *piter, const gchar *localdir)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkListStore *store = GTK_LIST_STORE(priv->task_list_model);
	GtkTreeIter iter;
	gint type;
	gchar *name;
	gfloat size;

	gtk_tree_model_get(priv->file_list_sort, piter, REMMINA_FTP_FILE_COLUMN_TYPE, &type, REMMINA_FTP_FILE_COLUMN_NAME,
		&name, REMMINA_FTP_FILE_COLUMN_SIZE, &size, -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, REMMINA_FTP_TASK_COLUMN_TYPE, type, REMMINA_FTP_TASK_COLUMN_NAME, name,
		REMMINA_FTP_TASK_COLUMN_SIZE, size, REMMINA_FTP_TASK_COLUMN_TASKID, remmina_ftp_client_taskid++,
		REMMINA_FTP_TASK_COLUMN_TASKTYPE, REMMINA_FTP_TASK_TYPE_DOWNLOAD, REMMINA_FTP_TASK_COLUMN_REMOTEDIR,
		priv->current_directory, REMMINA_FTP_TASK_COLUMN_LOCALDIR, localdir, REMMINA_FTP_TASK_COLUMN_STATUS,
		REMMINA_FTP_TASK_STATUS_WAIT, REMMINA_FTP_TASK_COLUMN_DONESIZE, 0.0, REMMINA_FTP_TASK_COLUMN_TOOLTIP,
		NULL, -1);

	g_free(name);

	g_signal_emit(G_OBJECT(client), remmina_ftp_client_signals[NEW_TASK_SIGNAL], 0);
}

static gboolean remmina_ftp_client_task_list_on_query_tooltip(GtkWidget *widget, gint x, gint y, gboolean keyboard_tip,
							      GtkTooltip *tooltip, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkTreeIter iter;
	GtkTreePath *path = NULL;
	gchar *tmp;

	if (!gtk_tree_view_get_tooltip_context(GTK_TREE_VIEW(priv->task_list_view), &x, &y, keyboard_tip, NULL, &path, &iter)) {
		return FALSE;
	}

	gtk_tree_model_get(priv->task_list_model, &iter, REMMINA_FTP_TASK_COLUMN_TOOLTIP, &tmp, -1);
	if (!tmp)
		return FALSE;

	gtk_tooltip_set_text(tooltip, tmp);

	gtk_tree_view_set_tooltip_row(GTK_TREE_VIEW(priv->task_list_view), tooltip, path);

	gtk_tree_path_free(path);
	g_free(tmp);

	return TRUE;
}

static void remmina_ftp_client_action_parent(GObject *object, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	remmina_ftp_client_open_dir(client, "..");
}

static void remmina_ftp_client_action_home(GObject *object, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	remmina_ftp_client_open_dir(client, NULL);
}

static void remmina_ftp_client_action_refresh(GObject *object, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	remmina_ftp_client_open_dir(client, ".");
}

static void remmina_ftp_client_action_download(GObject *object, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkTreeSelection *selection;
	gchar *localdir;
	GList *list, *list_iter;
	GtkTreeIter iter;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_list_view));
	if (!selection)
		return;
	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	if (!list)
		return;

	localdir = remmina_ftp_client_get_download_dir(client);
	if (!localdir) {
		g_list_free(list);
		return;
	}

	list_iter = g_list_first(list);
	while (list_iter) {
		gtk_tree_model_get_iter(priv->file_list_sort, &iter, (GtkTreePath*)list_iter->data);
		remmina_ftp_client_download(client, &iter, localdir);
		list_iter = g_list_next(list_iter);
	}
	g_list_free(list);
	g_free(localdir);
}

static void remmina_ftp_client_action_delete(GObject *object, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkWidget *dialog;
	GtkTreeSelection *selection;
	GList *list, *list_iter;
	GtkTreeIter iter;
	gint type;
	gchar *name;
	gchar *path;
	gint response;
	gboolean ret = TRUE;

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_list_view));
	if (!selection)
		return;
	list = gtk_tree_selection_get_selected_rows(selection, NULL);
	if (!list)
		return;

	dialog = gtk_message_dialog_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(client))), GTK_DIALOG_MODAL,
		GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO, _("Are you sure to delete the selected files on server?"));
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
	if (response != GTK_RESPONSE_YES)
		return;

	BUSY_CURSOR

		list_iter = g_list_first(list);
	while (list_iter) {
		gtk_tree_model_get_iter(priv->file_list_sort, &iter, (GtkTreePath*)list_iter->data);

		gtk_tree_model_get(priv->file_list_sort, &iter, REMMINA_FTP_FILE_COLUMN_TYPE, &type,
			REMMINA_FTP_FILE_COLUMN_NAME, &name, -1);

		path = remmina_public_combine_path(priv->current_directory, name);
		g_signal_emit(G_OBJECT(client), remmina_ftp_client_signals[DELETE_FILE_SIGNAL], 0, type, path, &ret);
		g_free(name);
		g_free(path);
		if (!ret)
			break;

		list_iter = g_list_next(list_iter);
	}
	g_list_free(list);

	NORMAL_CURSOR

	if (ret) {
		remmina_ftp_client_action_refresh(object, client);
	}
}

static void remmina_ftp_client_upload_folder_on_toggled(GtkToggleButton *togglebutton, GtkWidget *widget)
{
	TRACE_CALL(__func__);
	gtk_file_chooser_set_action(
		GTK_FILE_CHOOSER(widget),
		gtk_toggle_button_get_active(togglebutton) ?
		GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER : GTK_FILE_CHOOSER_ACTION_OPEN);
}

static void remmina_ftp_client_action_upload(GObject *object, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkListStore *store = GTK_LIST_STORE(priv->task_list_model);
	GtkTreeIter iter;
	GtkWidget *dialog;
	GtkWidget *upload_folder_check;
	gint type;
	GSList *files = NULL;
	GSList *element;
	gchar *path;
	gchar *dir, *name;
	struct stat st;

	dialog = gtk_file_chooser_dialog_new(_("Choose a file to upload"),
		GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(client))), GTK_FILE_CHOOSER_ACTION_OPEN, "_Cancel",
		GTK_RESPONSE_CANCEL, "_OK", GTK_RESPONSE_ACCEPT, NULL);
	gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dialog), TRUE);
	if (priv->working_directory) {
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), priv->working_directory);
	}
	upload_folder_check = gtk_check_button_new_with_label(_("Upload folder"));
	gtk_widget_show(upload_folder_check);
	g_signal_connect(G_OBJECT(upload_folder_check), "toggled", G_CALLBACK(remmina_ftp_client_upload_folder_on_toggled),
		dialog);
	gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), upload_folder_check);
	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		g_free(priv->working_directory);
		priv->working_directory = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(dialog));
		files = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dialog));
	}
	type = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(upload_folder_check)) ?
	       REMMINA_FTP_FILE_TYPE_DIR : REMMINA_FTP_FILE_TYPE_FILE;
	gtk_widget_destroy(dialog);
	if (!files)
		return;

	for (element = files; element; element = element->next) {
		path = (gchar*)element->data;

		if (g_stat(path, &st) < 0)
			continue;

		name = g_strrstr(path, "/");
		if (name) {
			*name++ = '\0';
			dir = path;
		}else  {
			name = path;
			dir = NULL;
		}

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, REMMINA_FTP_TASK_COLUMN_TYPE, type, REMMINA_FTP_TASK_COLUMN_NAME, name,
			REMMINA_FTP_TASK_COLUMN_SIZE, (gfloat)st.st_size, REMMINA_FTP_TASK_COLUMN_TASKID,
			remmina_ftp_client_taskid++, REMMINA_FTP_TASK_COLUMN_TASKTYPE, REMMINA_FTP_TASK_TYPE_UPLOAD,
			REMMINA_FTP_TASK_COLUMN_REMOTEDIR, priv->current_directory, REMMINA_FTP_TASK_COLUMN_LOCALDIR,
			dir, REMMINA_FTP_TASK_COLUMN_STATUS, REMMINA_FTP_TASK_STATUS_WAIT,
			REMMINA_FTP_TASK_COLUMN_DONESIZE, 0.0, REMMINA_FTP_TASK_COLUMN_TOOLTIP, NULL, -1);

		g_free(path);
	}

	g_slist_free(files);

	g_signal_emit(G_OBJECT(client), remmina_ftp_client_signals[NEW_TASK_SIGNAL], 0);
}

static void remmina_ftp_client_popup_menu(RemminaFTPClient *client, GdkEventButton *event)
{
	TRACE_CALL(__func__);
	GtkWidget *menu;
	GtkWidget *menuitem;
	GtkWidget *image;

	menu = gtk_menu_new();

	menuitem = gtk_menu_item_new_with_label(_("Download"));
	gtk_widget_show(menuitem);
	image = gtk_image_new_from_icon_name("remmina-document-save-symbolic", GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_ftp_client_action_download), client);

	menuitem = gtk_menu_item_new_with_label(_("Upload"));
	gtk_widget_show(menuitem);
	image = gtk_image_new_from_icon_name("remmina-document-send-symbolic", GTK_ICON_SIZE_MENU);
	gtk_widget_show(image);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_ftp_client_action_upload), client);

	menuitem = gtk_menu_item_new_with_mnemonic(_("_Delete"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_ftp_client_action_delete), client);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_pointer(GTK_MENU(menu), (GdkEvent*)event);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event->button, event->time);
#endif
}

static gboolean remmina_ftp_client_file_list_on_button_press(GtkWidget *widget, GdkEventButton *event, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GList *list;
	GtkTreeIter iter;
	gint type;
	gchar *name;
	gchar *localdir;

	if (event->button == 3) {
		remmina_ftp_client_popup_menu(client, event);
	}else if (event->button == 1 && event->type == GDK_2BUTTON_PRESS) {
		list = gtk_tree_selection_get_selected_rows(
			gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_list_view)), NULL);
		if (list) {
			gtk_tree_model_get_iter(priv->file_list_sort, &iter, (GtkTreePath*)list->data);
			gtk_tree_model_get(priv->file_list_sort, &iter, REMMINA_FTP_FILE_COLUMN_TYPE, &type,
				REMMINA_FTP_FILE_COLUMN_NAME, &name, -1);
			switch (type) {
			case REMMINA_FTP_FILE_TYPE_DIR:
				remmina_ftp_client_open_dir(client, name);
				break;
			case REMMINA_FTP_FILE_TYPE_FILE:
			default:
				localdir = remmina_ftp_client_get_download_dir(client);
				if (localdir) {
					remmina_ftp_client_download(client, &iter, localdir);
					g_free(localdir);
				}
				break;
			}
			g_list_free(list);
			g_free(name);
		}
	}

	return FALSE;
}

static void remmina_ftp_client_task_list_cell_on_activate(GtkCellRenderer *renderer, gchar *path, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkTreeIter iter;
	GtkTreePath *treepath;
	gint taskid;
	gboolean ret = FALSE;

	treepath = gtk_tree_path_new_from_string(path);
	gtk_tree_model_get_iter(priv->task_list_model, &iter, treepath);
	gtk_tree_path_free(treepath);

	gtk_tree_model_get(priv->task_list_model, &iter, REMMINA_FTP_TASK_COLUMN_TASKID, &taskid, -1);

	g_signal_emit(G_OBJECT(client), remmina_ftp_client_signals[CANCEL_TASK_SIGNAL], 0, taskid, &ret);

	if (ret) {
		gtk_list_store_remove(GTK_LIST_STORE(priv->task_list_model), &iter);
	}
}

static GtkWidget* remmina_ftp_client_create_toolbar(RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	GtkWidget *box;
	GtkWidget *button;
	GtkWidget *image;
	gint i = 0;

	box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_widget_show(box);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(box), GTK_BUTTONBOX_START);
	gtk_grid_attach(GTK_GRID(client), box, 0, 0, 1, 1);

	button = gtk_button_new_with_label(_("Home"));
	gtk_widget_show(button);
	gtk_widget_set_tooltip_text(button, _("Go to home folder"));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remmina_ftp_client_action_home), client);

	button = gtk_button_new_with_label(_("Up"));
	gtk_widget_show(button);
	gtk_widget_set_tooltip_text(button, _("Go to parent folder"));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remmina_ftp_client_action_parent), client);

	button = gtk_button_new_with_label(_("Refresh"));
	gtk_widget_show(button);
	gtk_widget_set_tooltip_text(button, _("Refresh current folder"));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remmina_ftp_client_action_refresh), client);

	button = gtk_button_new_with_label(_("Download"));
	gtk_widget_show(button);
	gtk_widget_set_tooltip_text(button, _("Download from server"));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	image = gtk_image_new_from_icon_name("remmina-document-save-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remmina_ftp_client_action_download), client);

	client->priv->file_action_widgets[i++] = button;

	button = gtk_button_new_with_label(_("Upload"));
	gtk_widget_show(button);
	gtk_widget_set_tooltip_text(button, _("Upload to server"));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	image = gtk_image_new_from_icon_name("remmina-document-send-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_button_set_image(GTK_BUTTON(button), image);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remmina_ftp_client_action_upload), client);

	button = gtk_button_new_with_label(_("Delete"));
	gtk_widget_show(button);
	gtk_widget_set_tooltip_text(button, _("Delete files on server"));
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remmina_ftp_client_action_delete), client);

	client->priv->file_action_widgets[i++] = button;

	return box;
}

void remmina_ftp_client_set_show_hidden(RemminaFTPClient *client, gboolean show_hidden)
{
	TRACE_CALL(__func__);
	client->priv->file_list_show_hidden = show_hidden;
	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(client->priv->file_list_filter));
}

static gboolean remmina_ftp_client_filter_visible_func(GtkTreeModel *model, GtkTreeIter *iter, RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	gchar *name;
	gboolean result = TRUE;

	if (client->priv->file_list_show_hidden)
		return TRUE;

	gtk_tree_model_get(model, iter, REMMINA_FTP_FILE_COLUMN_NAME, &name, -1);
	if (name && name[0] == '.') {
		result = FALSE;
	}
	g_free(name);
	return result;
}

/* Set the overwrite_all status */
void remmina_ftp_client_set_overwrite_status(RemminaFTPClient *client, gboolean status)
{
	TRACE_CALL(__func__);
	client->priv->overwrite_all = status;
}

/* Get the overwrite_all status */
gboolean remmina_ftp_client_get_overwrite_status(RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	return client->priv->overwrite_all;
}

static void remmina_ftp_client_init(RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv;
	GtkWidget *vpaned;
	GtkWidget *toolbar;
	GtkWidget *scrolledwindow;
	GtkWidget *widget;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkWidget *vbox;

	priv = g_new0(RemminaFTPClientPriv, 1);
	client->priv = priv;

	/* Initialize overwrite status to FALSE */
	client->priv->overwrite_all = FALSE;

	/* Main container */
	gtk_widget_set_vexpand(GTK_WIDGET(client), TRUE);
	gtk_widget_set_hexpand(GTK_WIDGET(client), TRUE);

	/* Toolbar */
	toolbar = remmina_ftp_client_create_toolbar(client);

	/* The Paned to separate File List and Task List */
	vpaned = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
	gtk_widget_set_vexpand(GTK_WIDGET(vpaned), TRUE);
	gtk_widget_set_hexpand(GTK_WIDGET(vpaned), TRUE);
	gtk_widget_show(vpaned);
	gtk_grid_attach_next_to(GTK_GRID(client), vpaned, toolbar, GTK_POS_BOTTOM, 1, 1);

	priv->vpaned = vpaned;

	/* Remote */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox);
	gtk_paned_pack1(GTK_PANED(vpaned), vbox, TRUE, FALSE);

	/* Remote Directory */
	widget = gtk_combo_box_text_new_with_entry();
	gtk_widget_show(widget);
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(widget), "/");
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);

	priv->directory_combo = widget;

	/* Remote File List */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_box_pack_start(GTK_BOX(vbox), scrolledwindow, TRUE, TRUE, 0);

	widget = gtk_tree_view_new();
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), widget);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(widget)), GTK_SELECTION_MULTIPLE);

	priv->file_list_view = widget;

	/* Remote File List - Columns */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Filename"));
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_FILE_COLUMN_NAME_SORT);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, remmina_ftp_client_cell_data_filetype_pixbuf, NULL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", REMMINA_FTP_FILE_COLUMN_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->file_list_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Size"), renderer, NULL);
	gtk_tree_view_column_set_alignment(column, 1.0);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, remmina_ftp_client_cell_data_size, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_FILE_COLUMN_SIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->file_list_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("User"), renderer, "text", REMMINA_FTP_FILE_COLUMN_USER, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_FILE_COLUMN_USER);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->file_list_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Group"), renderer, "text", REMMINA_FTP_FILE_COLUMN_GROUP, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_FILE_COLUMN_GROUP);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->file_list_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Permission"), renderer, "text", REMMINA_FTP_FILE_COLUMN_PERMISSION,
		NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, remmina_ftp_client_cell_data_permission, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_FILE_COLUMN_PERMISSION);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->file_list_view), column);

	/* Remote File List - Model */
	priv->file_list_model = GTK_TREE_MODEL(
		gtk_list_store_new(REMMINA_FTP_FILE_N_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING));

	priv->file_list_filter = gtk_tree_model_filter_new(priv->file_list_model, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(priv->file_list_filter),
		(GtkTreeModelFilterVisibleFunc)remmina_ftp_client_filter_visible_func, client, NULL);

	priv->file_list_sort = gtk_tree_model_sort_new_with_model(priv->file_list_filter);
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(priv->file_list_sort), REMMINA_FTP_FILE_COLUMN_NAME_SORT,
		GTK_SORT_ASCENDING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->file_list_view), priv->file_list_sort);

	/* Task List */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_paned_pack2(GTK_PANED(vpaned), scrolledwindow, FALSE, TRUE);

	widget = gtk_tree_view_new();
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), widget);
	g_object_set(widget, "has-tooltip", TRUE, NULL);

	priv->task_list_view = widget;

	/* Task List - Columns */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(column, _("Filename"));
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_expand(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_TASK_COLUMN_NAME);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, remmina_ftp_client_cell_data_progress_pixbuf, NULL, NULL);
	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, remmina_ftp_client_cell_data_filetype_pixbuf, NULL, NULL);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", REMMINA_FTP_FILE_COLUMN_NAME);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->task_list_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Remote"), renderer, "text", REMMINA_FTP_TASK_COLUMN_REMOTEDIR,
		NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_TASK_COLUMN_REMOTEDIR);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->task_list_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Local"), renderer, "text", REMMINA_FTP_TASK_COLUMN_LOCALDIR, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_TASK_COLUMN_LOCALDIR);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->task_list_view), column);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes(_("Size"), renderer, NULL);
	gtk_tree_view_column_set_alignment(column, 1.0);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, remmina_ftp_client_cell_data_size_progress, NULL, NULL);
	gtk_tree_view_column_set_sort_column_id(column, REMMINA_FTP_TASK_COLUMN_SIZE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->task_list_view), column);

	renderer = gtk_cell_renderer_progress_new();
	column = gtk_tree_view_column_new_with_attributes(_("Progress"), renderer, NULL);
	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, remmina_ftp_client_cell_data_progress, NULL, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->task_list_view), column);

	renderer = remmina_cell_renderer_pixbuf_new();
	column = gtk_tree_view_column_new_with_attributes(NULL, renderer, NULL);
	g_object_set(G_OBJECT(renderer), "stock-id", "_Cancel", NULL);
	gtk_tree_view_column_set_resizable(column, FALSE);
	gtk_tree_view_append_column(GTK_TREE_VIEW(priv->task_list_view), column);

	g_signal_connect(G_OBJECT(renderer), "activate", G_CALLBACK(remmina_ftp_client_task_list_cell_on_activate), client);

	/* Task List - Model */
	priv->task_list_model = GTK_TREE_MODEL(
		gtk_list_store_new(REMMINA_FTP_TASK_N_COLUMNS, G_TYPE_INT, G_TYPE_STRING, G_TYPE_FLOAT, G_TYPE_INT,
			G_TYPE_INT, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INT, G_TYPE_FLOAT, G_TYPE_STRING));
	gtk_tree_view_set_model(GTK_TREE_VIEW(priv->task_list_view), priv->task_list_model);

	/* Setup the internal signals */
	g_signal_connect(G_OBJECT(client), "destroy", G_CALLBACK(remmina_ftp_client_destroy), NULL);
	g_signal_connect(G_OBJECT(gtk_bin_get_child(GTK_BIN(priv->directory_combo))), "activate",
		G_CALLBACK(remmina_ftp_client_dir_on_activate), client);
	g_signal_connect(G_OBJECT(priv->directory_combo), "changed", G_CALLBACK(remmina_ftp_client_dir_on_changed), client);
	g_signal_connect(G_OBJECT(priv->file_list_view), "button-press-event",
		G_CALLBACK(remmina_ftp_client_file_list_on_button_press), client);
	g_signal_connect(G_OBJECT(gtk_tree_view_get_selection(GTK_TREE_VIEW(priv->file_list_view))), "changed",
		G_CALLBACK(remmina_ftp_client_file_selection_on_changed), client);
	g_signal_connect(G_OBJECT(priv->task_list_view), "query-tooltip",
		G_CALLBACK(remmina_ftp_client_task_list_on_query_tooltip), client);
}

GtkWidget*
remmina_ftp_client_new(void)
{
	TRACE_CALL(__func__);
	RemminaFTPClient *client;

	client = REMMINA_FTP_CLIENT(g_object_new(REMMINA_TYPE_FTP_CLIENT, NULL));

	return GTK_WIDGET(client);
}

void remmina_ftp_client_save_state(RemminaFTPClient *client, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	gint pos;

	pos = gtk_paned_get_position(GTK_PANED(client->priv->vpaned));
	remmina_file_set_int(remminafile, "ftp_vpanedpos", pos);
}

void remmina_ftp_client_load_state(RemminaFTPClient *client, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	gint pos;
	GtkAllocation a;

	pos = remmina_file_get_int(remminafile, "ftp_vpanedpos", 0);
	if (pos) {
		gtk_widget_get_allocation(client->priv->vpaned, &a);
		if (a.height > 0 && pos > a.height - 60) {
			pos = a.height - 60;
		}
		gtk_paned_set_position(GTK_PANED(client->priv->vpaned), pos);
	}
}

void remmina_ftp_client_clear_file_list(RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;

	gtk_list_store_clear(GTK_LIST_STORE(priv->file_list_model));
	remmina_ftp_client_set_file_action_sensitive(client, FALSE);
}

void remmina_ftp_client_add_file(RemminaFTPClient *client, ...)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkListStore *store = GTK_LIST_STORE(priv->file_list_model);
	GtkTreeIter iter;
	va_list args;
	gint type;
	gchar *name;
	gchar *ptr;

	va_start(args, client);
	gtk_list_store_append(store, &iter);
	gtk_list_store_set_valist(store, &iter, args);
	va_end(args);

	gtk_tree_model_get(GTK_TREE_MODEL(store), &iter,
		REMMINA_FTP_FILE_COLUMN_TYPE, &type,
		REMMINA_FTP_FILE_COLUMN_NAME, &name,
		-1);

	ptr = g_strdup_printf("%i%s", type, name);
	gtk_list_store_set(store, &iter, REMMINA_FTP_FILE_COLUMN_NAME_SORT, ptr, -1);
	g_free(ptr);
	g_free(name);
}

void remmina_ftp_client_set_dir(RemminaFTPClient *client, const gchar *dir)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gboolean ret;
	gchar *t;

	if (priv->current_directory && g_strcmp0(priv->current_directory, dir) == 0)
		return;

	model = gtk_combo_box_get_model(GTK_COMBO_BOX(priv->directory_combo));
	for (ret = gtk_tree_model_get_iter_first(model, &iter); ret; ret = gtk_tree_model_iter_next(model, &iter)) {
		gtk_tree_model_get(model, &iter, 0, &t, -1);
		if (g_strcmp0(t, dir) == 0) {
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			g_free(t);
			break;
		}
		g_free(t);
	}

	gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(priv->directory_combo), dir);
	gtk_combo_box_set_active(GTK_COMBO_BOX(priv->directory_combo), 0);

	g_free(priv->current_directory);
	priv->current_directory = g_strdup(dir);
}

gchar*
remmina_ftp_client_get_dir(RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;

	return g_strdup(priv->current_directory);
}

RemminaFTPTask*
remmina_ftp_client_get_waiting_task(RemminaFTPClient *client)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkTreePath *path;
	GtkTreeIter iter;
	RemminaFTPTask task;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		RemminaFTPTask* retval;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_FTP_CLIENT_GET_WAITING_TASK;
		d->p.ftp_client_get_waiting_task.client = client;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.ftp_client_get_waiting_task.retval;
		g_free(d);
		return retval;
	}

	if (!gtk_tree_model_get_iter_first(priv->task_list_model, &iter))
		return NULL;

	while (1) {
		gtk_tree_model_get(priv->task_list_model, &iter, REMMINA_FTP_TASK_COLUMN_TYPE, &task.type,
			REMMINA_FTP_TASK_COLUMN_NAME, &task.name, REMMINA_FTP_TASK_COLUMN_SIZE, &task.size,
			REMMINA_FTP_TASK_COLUMN_TASKID, &task.taskid, REMMINA_FTP_TASK_COLUMN_TASKTYPE, &task.tasktype,
			REMMINA_FTP_TASK_COLUMN_REMOTEDIR, &task.remotedir, REMMINA_FTP_TASK_COLUMN_LOCALDIR,
			&task.localdir, REMMINA_FTP_TASK_COLUMN_STATUS, &task.status, REMMINA_FTP_TASK_COLUMN_DONESIZE,
			&task.donesize, REMMINA_FTP_TASK_COLUMN_TOOLTIP, &task.tooltip, -1);
		if (task.status == REMMINA_FTP_TASK_STATUS_WAIT) {
			path = gtk_tree_model_get_path(priv->task_list_model, &iter);
			task.rowref = gtk_tree_row_reference_new(priv->task_list_model, path);
			gtk_tree_path_free(path);
			return (RemminaFTPTask*)g_memdup(&task, sizeof(RemminaFTPTask));
		}
		if (!gtk_tree_model_iter_next(priv->task_list_model, &iter))
			break;
	}

	return NULL;
}

void remmina_ftp_client_update_task(RemminaFTPClient *client, RemminaFTPTask* task)
{
	TRACE_CALL(__func__);
	RemminaFTPClientPriv *priv = (RemminaFTPClientPriv*)client->priv;
	GtkListStore *store = GTK_LIST_STORE(priv->task_list_model);
	GtkTreePath *path;
	GtkTreeIter iter;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_FTP_CLIENT_UPDATE_TASK;
		d->p.ftp_client_update_task.client = client;
		d->p.ftp_client_update_task.task = task;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}



	path = gtk_tree_row_reference_get_path(task->rowref);
	if (path == NULL)
		return;
	gtk_tree_model_get_iter(priv->task_list_model, &iter, path);
	gtk_tree_path_free(path);
	gtk_list_store_set(store, &iter, REMMINA_FTP_TASK_COLUMN_SIZE, task->size, REMMINA_FTP_TASK_COLUMN_STATUS, task->status,
		REMMINA_FTP_TASK_COLUMN_DONESIZE, task->donesize, REMMINA_FTP_TASK_COLUMN_TOOLTIP, task->tooltip, -1);
}

void remmina_ftp_task_free(RemminaFTPTask *task)
{
	TRACE_CALL(__func__);
	if (task) {
		g_free(task->name);
		g_free(task->remotedir);
		g_free(task->localdir);
		g_free(task->tooltip);
		g_free(task);
	}
}

