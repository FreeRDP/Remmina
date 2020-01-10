/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
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

#include "common/remmina_plugin.h"
#include <telepathy-glib/account.h>
#include <telepathy-glib/channel.h>
#include <telepathy-glib/contact.h>
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/defs.h>
#include <telepathy-glib/handle.h>
#include <telepathy-glib/interfaces.h>
#include <telepathy-glib/svc-client.h>
#include <telepathy-glib/util.h>
#include "telepathy_channel_handler.h"

extern RemminaPluginService *remmina_plugin_telepathy_service;

typedef struct _RemminaTpChannelHandler {
	gchar *connection_path;
	gchar *channel_path;
	GHashTable *channel_properties;
	DBusGMethodInvocation *context;

	GtkWidget *proto_widget;
	guint disconnect_handler;

	TpDBusDaemon *bus;
	TpAccount *account;
	TpConnection *connection;
	TpChannel *channel;

	gchar *alias;
	gchar *host;
	guint port;
	gchar *protocol;
} RemminaTpChannelHandler;

static void remmina_tp_channel_handler_free(RemminaTpChannelHandler *chandler)
{
	TRACE_CALL(__func__);
	if (chandler->disconnect_handler) {
		g_signal_handler_disconnect(chandler->proto_widget, chandler->disconnect_handler);
		chandler->disconnect_handler = 0;
	}
	g_free(chandler->connection_path);
	g_free(chandler->channel_path);
	g_hash_table_destroy(chandler->channel_properties);
	if (chandler->bus) {
		g_object_unref(chandler->bus);
	}
	if (chandler->account) {
		g_object_unref(chandler->account);
	}
	if (chandler->connection) {
		g_object_unref(chandler->connection);
	}
	if (chandler->channel) {
		g_object_unref(chandler->channel);
	}
	if (chandler->alias) {
		g_free(chandler->alias);
	}
	if (chandler->host) {
		g_free(chandler->host);
	}
	if (chandler->protocol) {
		g_free(chandler->protocol);
	}
	g_free(chandler);
}

static void remmina_tp_channel_handler_channel_closed(TpChannel *channel, gpointer user_data, GObject *self)
{
	TRACE_CALL(__func__);
	RemminaTpChannelHandler *chandler = (RemminaTpChannelHandler*)user_data;

	g_print("%s: %s\n", __func__, chandler->channel_path);
	remmina_tp_channel_handler_free(chandler);
}

static void remmina_tp_channel_handler_on_disconnect(GtkWidget *widget, RemminaTpChannelHandler *chandler)
{
	TRACE_CALL(__func__);
	g_print("%s: %s\n", __func__, chandler->channel_path);
	g_signal_handler_disconnect(widget, chandler->disconnect_handler);
	chandler->disconnect_handler = 0;
	tp_cli_channel_call_close(chandler->channel, -1, NULL, NULL, NULL, NULL);
}

static void remmina_tp_channel_handler_connect(RemminaTpChannelHandler *chandler)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;
	gchar *s;

	remminafile = remmina_plugin_telepathy_service->file_new();
	remmina_plugin_telepathy_service->file_set_string(remminafile, "name", chandler->alias);
	remmina_plugin_telepathy_service->file_set_string(remminafile, "protocol", chandler->protocol);
	s = g_strdup_printf("[%s]:%i", chandler->host, chandler->port);
	remmina_plugin_telepathy_service->file_set_string(remminafile, "server", s);
	g_free(s);
	remmina_plugin_telepathy_service->file_set_int(remminafile, "colordepth", 8);

	g_free(chandler->alias);
	chandler->alias = NULL;
	g_free(chandler->protocol);
	chandler->protocol = NULL;

	chandler->proto_widget = remmina_plugin_telepathy_service->open_connection(remminafile,
		G_CALLBACK(remmina_tp_channel_handler_on_disconnect), chandler, &chandler->disconnect_handler);
}

static void remmina_tp_channel_handler_get_service(TpProxy *channel, const GValue *service, const GError *error,
						   gpointer user_data, GObject *weak_object)
{
	TRACE_CALL(__func__);
	RemminaTpChannelHandler *chandler = (RemminaTpChannelHandler*)user_data;
	const gchar *svc;

	if (error != NULL) {
		g_print("%s: %s", __func__, error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}
	svc = g_value_get_string(service);
	g_print("%s: %s %s:%u\n", __func__, svc, chandler->host, chandler->port);

	if (g_strcmp0(svc, "rfb") == 0) {
		chandler->protocol = g_strdup("VNC");
	}else  {
		chandler->protocol = g_ascii_strup(svc, -1);
	}
	remmina_tp_channel_handler_connect(chandler);
}

static void remmina_tp_channel_handler_accept(TpChannel *channel, const GValue *address, const GError *error,
					      gpointer user_data, GObject *weak_object)
{
	TRACE_CALL(__func__);
	RemminaTpChannelHandler *chandler = (RemminaTpChannelHandler*)user_data;

	if (error != NULL) {
		g_print("%s: %s", __func__, error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}

	dbus_g_type_struct_get(address, 0, &chandler->host, 1, &chandler->port, G_MAXUINT);

	tp_cli_dbus_properties_call_get(channel, -1, TP_IFACE_CHANNEL_TYPE_STREAM_TUBE, "Service",
		remmina_tp_channel_handler_get_service, chandler, NULL, NULL);
}

static void remmina_tp_channel_handler_on_response(GtkDialog *dialog, gint response_id, RemminaTpChannelHandler *chandler)
{
	TRACE_CALL(__func__);
	GValue noop =
	{ 0 };
	GError *error;

	if (response_id == GTK_RESPONSE_YES) {
		g_value_init(&noop, G_TYPE_INT);
		tp_cli_channel_type_stream_tube_call_accept(chandler->channel, -1, TP_SOCKET_ADDRESS_TYPE_IPV4,
			TP_SOCKET_ACCESS_CONTROL_LOCALHOST, &noop, remmina_tp_channel_handler_accept, chandler, NULL,
			NULL);
		g_value_unset(&noop);
		tp_svc_client_handler_return_from_handle_channels(chandler->context);
	}else  {
		error = g_error_new(TP_ERROR, TP_ERROR_NOT_AVAILABLE, "Channel rejected by user.");
		dbus_g_method_return_error(chandler->context, error);
		g_error_free(error);
		remmina_tp_channel_handler_free(chandler);
	}
}

static void remmina_tp_channel_handler_get_contacts(TpConnection *connection, guint n_contacts, TpContact * const *contacts,
						    guint n_failed, const TpHandle *failed, const GError *error, gpointer user_data, GObject *weak_object)
{
	TRACE_CALL(__func__);
	RemminaTpChannelHandler *chandler = (RemminaTpChannelHandler*)user_data;
	TpContact *contact;
	gchar *token;
	const gchar *cm;
	const gchar *protocol;
	gchar *filename;
	GdkPixbuf *pixbuf;
	GtkWidget *image;
	GtkWidget *dialog;

	if (error != NULL) {
		g_print("%s: %s", __func__, error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}
	if (n_contacts <= 0) {
		g_print("%s: no contacts\n", __func__);
		remmina_tp_channel_handler_free(chandler);
		return;
	}
	contact = contacts[0];
	chandler->alias = g_strdup(tp_contact_get_alias(contact));

	dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
		_("%s wants to share their desktop.\nDo you accept?"), chandler->alias);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(remmina_tp_channel_handler_on_response), chandler);
	g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_window_set_title(GTK_WINDOW(dialog), _("Desktop sharing invitation"));
	remmina_plugin_telepathy_service->ui_register(dialog);
	gtk_widget_show(dialog);

	token = (gchar*)tp_contact_get_avatar_token(contact);
	if (token == NULL) {
		return;
	}
	protocol = tp_connection_get_protocol_name(chandler->connection);
	cm = tp_connection_get_cm_name(chandler->connection);
	if (!protocol || !cm) {
		return;
	}
	token = tp_escape_as_identifier(token);
	filename = g_build_filename(g_get_user_cache_dir(), "telepathy", "avatars", cm, protocol, token, NULL);
	g_free(token);
	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
		pixbuf = gdk_pixbuf_new_from_file(filename, NULL);
		if (pixbuf) {
			image = gtk_image_new_from_pixbuf(pixbuf);
			gtk_widget_show(image);
			g_object_unref(pixbuf);
			gtk_message_dialog_set_image(GTK_MESSAGE_DIALOG(dialog), image);
		}
	}
	g_free(filename);
}

static void remmina_tp_channel_handler_channel_ready(TpChannel *channel, const GError *channel_error, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaTpChannelHandler *chandler = (RemminaTpChannelHandler*)user_data;
	TpHandle handle;
	GError *error = NULL;
	TpContactFeature features[] =
	{ TP_CONTACT_FEATURE_ALIAS, TP_CONTACT_FEATURE_AVATAR_TOKEN };

	if (channel_error != NULL) {
		g_print("%s: %s\n", __func__, channel_error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}

	if (tp_cli_channel_connect_to_closed(channel, remmina_tp_channel_handler_channel_closed, chandler, NULL, NULL, &error)
	    == NULL) {
		g_print("tp_cli_channel_connect_to_closed: %s\n", channel_error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}
	g_print("%s: %s\n", __func__, chandler->channel_path);

	handle = tp_channel_get_handle(channel, NULL);
	tp_connection_get_contacts_by_handle(chandler->connection, 1, &handle, G_N_ELEMENTS(features), features,
		remmina_tp_channel_handler_get_contacts, chandler, NULL, NULL);
}

static void remmina_tp_channel_handler_connection_ready(TpConnection *connection, const GError *connection_error,
							gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaTpChannelHandler *chandler = (RemminaTpChannelHandler*)user_data;
	GError *error = NULL;

	if (connection_error != NULL) {
		g_print("%s: %s\n", __func__, connection_error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}

	chandler->channel = tp_channel_new_from_properties(connection, chandler->channel_path, chandler->channel_properties,
		&error);
	if (chandler->channel == NULL) {
		g_print("tp_channel_new_from_properties: %s\n", error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}
	tp_channel_call_when_ready(chandler->channel, remmina_tp_channel_handler_channel_ready, chandler);
}

static void remmina_tp_channel_handler_account_ready(GObject *account, GAsyncResult *res, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaTpChannelHandler *chandler = (RemminaTpChannelHandler*)user_data;
	GError *error = NULL;

	if (!tp_account_prepare_finish(TP_ACCOUNT(account), res, &error)) {
		g_print("tp_account_prepare_finish: %s\n", error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}

	chandler->connection = tp_connection_new(chandler->bus, NULL, chandler->connection_path, &error);
	if (chandler->connection == NULL) {
		g_print("tp_connection_new: %s\n", error->message);
		remmina_tp_channel_handler_free(chandler);
		return;
	}
	tp_connection_call_when_ready(chandler->connection, remmina_tp_channel_handler_connection_ready, chandler);
}

void remmina_tp_channel_handler_new(const gchar *account_path, const gchar *connection_path, const gchar *channel_path,
				    GHashTable *channel_properties, DBusGMethodInvocation *context)
{
	TRACE_CALL(__func__);
	TpDBusDaemon *bus;
	TpAccount *account;
	GError *error = NULL;
	RemminaTpChannelHandler *chandler;

	bus = tp_dbus_daemon_dup(&error);
	if (bus == NULL) {
		g_print("tp_dbus_daemon_dup: %s", error->message);
		return;
	}
	account = tp_account_new(bus, account_path, &error);
	if (account == NULL) {
		g_object_unref(bus);
		g_print("tp_account_new: %s", error->message);
		return;
	}

	chandler = g_new0(RemminaTpChannelHandler, 1);
	chandler->bus = bus;
	chandler->account = account;
	chandler->connection_path = g_strdup(connection_path);
	chandler->channel_path = g_strdup(channel_path);
	chandler->channel_properties = tp_asv_new(NULL, NULL);
	tp_g_hash_table_update(chandler->channel_properties, channel_properties, (GBoxedCopyFunc)g_strdup,
		(GBoxedCopyFunc)tp_g_value_slice_dup);
	chandler->context = context;

	tp_account_prepare_async(account, NULL, remmina_tp_channel_handler_account_ready, chandler);
}

