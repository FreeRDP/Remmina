/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016 Denis Ollier
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

static void remmina_plugin_spice_usb_connect_failed_cb(GObject *, SpiceUsbDevice *, GError *, RemminaProtocolWidget *);

void remmina_plugin_spice_select_usb_devices(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	GtkWidget *dialog, *usb_device_widget;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	/*
	 * FIXME: Use the RemminaConnectionWindow as transient parent widget
	 * (and add the GTK_DIALOG_DESTROY_WITH_PARENT flag) if it becomes
	 * accessible from the Remmina plugin API.
	 */
	dialog = gtk_dialog_new_with_buttons(_("Select USB devices for redirection"),
	                                     NULL,
	                                     GTK_DIALOG_MODAL,
	                                     _("_Close"),
	                                     GTK_RESPONSE_ACCEPT,
	                                     NULL);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

	usb_device_widget = spice_usb_device_widget_new(gpdata->session, NULL);
	g_signal_connect(usb_device_widget,
	                 "connect-failed",
	                 G_CALLBACK(remmina_plugin_spice_usb_connect_failed_cb),
	                 gp);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
	                   usb_device_widget,
	                   TRUE,
	                   TRUE,
	                   0);
	gtk_widget_show_all(dialog);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void remmina_plugin_spice_usb_connect_failed_cb(GObject *object, SpiceUsbDevice *usb_device, GError *error, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	GtkWidget *dialog;

	if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_CANCELLED)
	{
		return;
	}

	/*
	 * FIXME: Use the RemminaConnectionWindow as transient parent widget
	 * (and add the GTK_DIALOG_DESTROY_WITH_PARENT flag) if it becomes
	 * accessible from the Remmina plugin API.
	 */
	dialog = gtk_message_dialog_new(NULL,
	                                GTK_DIALOG_MODAL,
	                                GTK_MESSAGE_ERROR,
	                                GTK_BUTTONS_CLOSE,
	                                _("USB redirection error"));
	gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog),
	                                         "%s",
	                                         error->message);
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
