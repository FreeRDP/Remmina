/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2018 Denis Ollier
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

#include "spice_plugin.h"

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)

static void remmina_plugin_spice_file_transfer_cancel_cb(GtkButton *, SpiceFileTransferTask *);
static void remmina_plugin_spice_file_transfer_dialog_response_cb(GtkDialog *, gint, RemminaProtocolWidget *);
static void remmina_plugin_spice_file_transfer_finished_cb(SpiceFileTransferTask *, GError *, RemminaProtocolWidget *);
static void remmina_plugin_spice_file_transfer_progress_cb(GObject *, GParamSpec *, RemminaProtocolWidget *);

typedef struct _RemminaPluginSpiceXferWidgets {
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *progress;
	GtkWidget *label;
	GtkWidget *cancel;
} RemminaPluginSpiceXferWidgets;

static RemminaPluginSpiceXferWidgets * remmina_plugin_spice_xfer_widgets_new(SpiceFileTransferTask *);
static void remmina_plugin_spice_xfer_widgets_free(RemminaPluginSpiceXferWidgets *widgets);

void remmina_plugin_spice_file_transfer_new_cb(SpiceMainChannel *main_channel, SpiceFileTransferTask *task, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	GtkWidget *dialog_content;
	RemminaPluginSpiceXferWidgets *widgets;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	g_signal_connect(task,
		"finished",
		G_CALLBACK(remmina_plugin_spice_file_transfer_finished_cb),
		gp);

	if (!gpdata->file_transfers) {
		gpdata->file_transfers = g_hash_table_new_full(g_direct_hash,
			g_direct_equal,
			g_object_unref,
			(GDestroyNotify)remmina_plugin_spice_xfer_widgets_free);
	}

	if (!gpdata->file_transfer_dialog) {
		/*
		 * FIXME: Use the RemminaConnectionWindow as transient parent widget
		 * (and add the GTK_DIALOG_DESTROY_WITH_PARENT flag) if it becomes
		 * accessible from the Remmina plugin API.
		 */
		gpdata->file_transfer_dialog = gtk_dialog_new_with_buttons(_("File Transfers"),
			NULL, 0,
			_("_Cancel"),
			GTK_RESPONSE_CANCEL,
			NULL);
		dialog_content = gtk_dialog_get_content_area(GTK_DIALOG(gpdata->file_transfer_dialog));
		gtk_widget_set_size_request(dialog_content, 400, -1);
		gtk_window_set_resizable(GTK_WINDOW(gpdata->file_transfer_dialog), FALSE);
		g_signal_connect(gpdata->file_transfer_dialog,
			"response",
			G_CALLBACK(remmina_plugin_spice_file_transfer_dialog_response_cb),
			gp);
	}

	widgets = remmina_plugin_spice_xfer_widgets_new(task);
	g_hash_table_insert(gpdata->file_transfers, g_object_ref(task), widgets);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(gpdata->file_transfer_dialog))),
		widgets->vbox,
		TRUE, TRUE, 6);

	g_signal_connect(task,
		"notify::progress",
		G_CALLBACK(remmina_plugin_spice_file_transfer_progress_cb),
		gp);

	gtk_widget_show(gpdata->file_transfer_dialog);
}

static RemminaPluginSpiceXferWidgets * remmina_plugin_spice_xfer_widgets_new(SpiceFileTransferTask *task)
{
	TRACE_CALL(__func__);

	gchar *filename;
	RemminaPluginSpiceXferWidgets *widgets = g_new0(RemminaPluginSpiceXferWidgets, 1);

	widgets->vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	widgets->hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);

	filename = spice_file_transfer_task_get_filename(task);
	widgets->label = gtk_label_new(filename);
	gtk_widget_set_halign(widgets->label, GTK_ALIGN_START);
	gtk_widget_set_valign(widgets->label, GTK_ALIGN_BASELINE);

	widgets->progress = gtk_progress_bar_new();
	gtk_widget_set_hexpand(widgets->progress, TRUE);
	gtk_widget_set_valign(widgets->progress, GTK_ALIGN_CENTER);

	widgets->cancel = gtk_button_new_from_icon_name("gtk-cancel", GTK_ICON_SIZE_SMALL_TOOLBAR);
	g_signal_connect(widgets->cancel,
		"clicked",
		G_CALLBACK(remmina_plugin_spice_file_transfer_cancel_cb),
		task);
	gtk_widget_set_hexpand(widgets->cancel, FALSE);
	gtk_widget_set_valign(widgets->cancel, GTK_ALIGN_CENTER);

	gtk_box_pack_start(GTK_BOX(widgets->hbox), widgets->progress,
		TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(widgets->hbox), widgets->cancel,
		FALSE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(widgets->vbox), widgets->label,
		TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(widgets->vbox), widgets->hbox,
		TRUE, TRUE, 0);

	gtk_widget_show_all(widgets->vbox);

	g_free(filename);

	return widgets;
}

static void remmina_plugin_spice_xfer_widgets_free(RemminaPluginSpiceXferWidgets *widgets)
{
	TRACE_CALL(__func__);

	/* Child widgets will be destroyed automatically */
	gtk_widget_destroy(widgets->vbox);
	g_free(widgets);
}

static void remmina_plugin_spice_file_transfer_cancel_cb(GtkButton *button, SpiceFileTransferTask *task)
{
	TRACE_CALL(__func__);

	spice_file_transfer_task_cancel(task);
}


static void remmina_plugin_spice_file_transfer_dialog_response_cb(GtkDialog *dialog, gint response, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	GHashTableIter iter;
	gpointer key, value;
	SpiceFileTransferTask *task;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	if (response == GTK_RESPONSE_CANCEL) {
		g_hash_table_iter_init(&iter, gpdata->file_transfers);
		while (g_hash_table_iter_next(&iter, &key, &value)) {
			task = key;
			spice_file_transfer_task_cancel(task);
		}
	}
}

static void remmina_plugin_spice_file_transfer_progress_cb(GObject *task, GParamSpec *param_spec, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	RemminaPluginSpiceXferWidgets *widgets;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	widgets = g_hash_table_lookup(gpdata->file_transfers, task);
	if (widgets) {
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(widgets->progress),
			spice_file_transfer_task_get_progress(SPICE_FILE_TRANSFER_TASK(task)));
	}
}

static void remmina_plugin_spice_file_transfer_finished_cb(SpiceFileTransferTask *task, GError *error, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gchar *filename, *notification_message;
	GNotification *notification;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	/*
	 * Send a desktop notification to inform about the outcome of
	 * the file transfer.
	 */
	filename = spice_file_transfer_task_get_filename(task);

	if (error) {
		notification = g_notification_new(_("Transfer error"));
		notification_message = g_strdup_printf(_("%s: %s"),
			filename, error->message);
	}else  {
		notification = g_notification_new(_("Transfer completed"));
		notification_message = g_strdup_printf(_("The %s file has been transferred"),
			filename);
	}

	g_notification_set_body(notification, notification_message);
	g_application_send_notification(g_application_get_default(),
		"remmina-plugin-spice-file-transfer-finished",
		notification);

	g_hash_table_remove(gpdata->file_transfers, task);

	if (!g_hash_table_size(gpdata->file_transfers)) {
		gtk_widget_hide(gpdata->file_transfer_dialog);
	}

	g_free(filename);
	g_free(notification_message);
	g_object_unref(notification);
}
#  endif        /* SPICE_GTK_CHECK_VERSION(0, 31, 0) */
#endif          /* SPICE_GTK_CHECK_VERSION */
