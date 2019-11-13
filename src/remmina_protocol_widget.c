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

#include "config.h"

#include <gtk/gtk.h>
#include <gtk/gtkx.h>
#include <glib/gi18n.h>
#include <stdlib.h>



#include "remmina_chat_window.h"
#include "remmina_masterthread_exec.h"
#include "remmina_ext_exec.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref.h"
#include "remmina_protocol_widget.h"
#include "remmina_public.h"
#include "remmina_ssh.h"
#include "remmina_log.h"
#include "remmina/remmina_trace_calls.h"

#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif

struct _RemminaProtocolWidgetPriv {
	RemminaFile *		remmina_file;
	RemminaProtocolPlugin * plugin;
	RemminaProtocolFeature *features;

	gint			width;
	gint			height;
	RemminaScaleMode	scalemode;
	gboolean		scaler_expand;

	gboolean		has_error;
	gchar *			error_message;
	RemminaSSHTunnel *	ssh_tunnel;
	RemminaTunnelInitFunc	init_func;

	GtkWidget *		chat_window;

	gboolean		closed;

	RemminaHostkeyFunc	hostkey_func;

	gint			profile_remote_width;
	gint			profile_remote_height;

	RemminaMessagePanel *	connect_message_panel;
	RemminaMessagePanel *	listen_message_panel;
	RemminaMessagePanel *	auth_message_panel;

	/* Data saved from the last message_panel when the user confirm */
	gchar *			username;
	gchar *			password;
	gchar *			domain;
	gboolean		save_password;

	gchar *			cacert;
	gchar *			cacrl;
	gchar *			clientcert;
	gchar *			clientkey;
};

enum panel_type {
	RPWDT_AUTH,
	RPWDT_QUESTIONYESNO,
	RPWDT_AUTHX509
};

G_DEFINE_TYPE(RemminaProtocolWidget, remmina_protocol_widget, GTK_TYPE_EVENT_BOX)

enum {
	CONNECT_SIGNAL,
	DISCONNECT_SIGNAL,
	DESKTOP_RESIZE_SIGNAL,
	UPDATE_ALIGN_SIGNAL,
	UNLOCK_DYNRES_SIGNAL,
	LAST_SIGNAL
};

typedef struct _RemminaProtocolWidgetSignalData {
	RemminaProtocolWidget * gp;
	const gchar *		signal_name;
} RemminaProtocolWidgetSignalData;

static guint remmina_protocol_widget_signals[LAST_SIGNAL] =
{ 0 };

static void remmina_protocol_widget_class_init(RemminaProtocolWidgetClass *klass)
{
	TRACE_CALL(__func__);
	remmina_protocol_widget_signals[CONNECT_SIGNAL] = g_signal_new("connect", G_TYPE_FROM_CLASS(klass),
								       G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, connect), NULL, NULL,
								       g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_protocol_widget_signals[DISCONNECT_SIGNAL] = g_signal_new("disconnect", G_TYPE_FROM_CLASS(klass),
									  G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, disconnect), NULL, NULL,
									  g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_protocol_widget_signals[DESKTOP_RESIZE_SIGNAL] = g_signal_new("desktop-resize", G_TYPE_FROM_CLASS(klass),
									      G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, desktop_resize), NULL, NULL,
									      g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_protocol_widget_signals[UPDATE_ALIGN_SIGNAL] = g_signal_new("update-align", G_TYPE_FROM_CLASS(klass),
									    G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, update_align), NULL, NULL,
									    g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_protocol_widget_signals[UNLOCK_DYNRES_SIGNAL] = g_signal_new("unlock-dynres", G_TYPE_FROM_CLASS(klass),
									     G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, unlock_dynres), NULL, NULL,
									     g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void remmina_protocol_widget_destroy(RemminaProtocolWidget *gp, gpointer data)
{
	TRACE_CALL(__func__);

	g_free(gp->priv->username);
	gp->priv->username = NULL;

	g_free(gp->priv->password);
	gp->priv->password = NULL;

	g_free(gp->priv->domain);
	gp->priv->domain = NULL;

	g_free(gp->priv->cacert);
	gp->priv->cacert = NULL;

	g_free(gp->priv->cacrl);
	gp->priv->cacrl = NULL;

	g_free(gp->priv->clientcert);
	gp->priv->clientcert = NULL;

	g_free(gp->priv->clientkey);
	gp->priv->clientkey = NULL;

	g_free(gp->priv->features);
	gp->priv->features = NULL;

	g_free(gp->priv->error_message);
	gp->priv->error_message = NULL;

	g_free(gp->priv->remmina_file);
	gp->priv->remmina_file = NULL;

	g_free(gp->priv);
	gp->priv = NULL;
}



void remmina_protocol_widget_grab_focus(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	GtkWidget *child;

	child = gtk_bin_get_child(GTK_BIN(gp));

	if (child) {
		gtk_widget_set_can_focus(child, TRUE);
		gtk_widget_grab_focus(child);
	}
}

static void remmina_protocol_widget_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidgetPriv *priv;

	priv = g_new0(RemminaProtocolWidgetPriv, 1);
	gp->priv = priv;
	gp->priv->closed = TRUE;

	g_signal_connect(G_OBJECT(gp), "destroy", G_CALLBACK(remmina_protocol_widget_destroy), NULL);
}

void remmina_protocol_widget_open_connection_real(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET(data);

	RemminaProtocolPlugin *plugin;
	RemminaProtocolFeature *feature;
	gint num_plugin;
	gint num_ssh;

	gp->priv->closed = FALSE;

	plugin = gp->priv->plugin;
	plugin->init(gp);

	for (num_plugin = 0, feature = (RemminaProtocolFeature *)plugin->features; feature && feature->type; num_plugin++, feature++) {
	}

	num_ssh = 0;
#ifdef HAVE_LIBSSH
	if (remmina_file_get_int(gp->priv->remmina_file, "ssh_enabled", FALSE))
		num_ssh += 2;

#endif
	if (num_plugin + num_ssh == 0) {
		gp->priv->features = NULL;
	} else {
		gp->priv->features = g_new0(RemminaProtocolFeature, num_plugin + num_ssh + 1);
		feature = gp->priv->features;
		if (plugin->features) {
			memcpy(feature, plugin->features, sizeof(RemminaProtocolFeature) * num_plugin);
			feature += num_plugin;
		}
#ifdef HAVE_LIBSSH
		if (num_ssh) {
			feature->type = REMMINA_PROTOCOL_FEATURE_TYPE_TOOL;
			feature->id = REMMINA_PROTOCOL_FEATURE_TOOL_SSH;
			feature->opt1 = _("Connect via SSH from a new terminal");
			feature->opt2 = "utilities-terminal";
			feature++;

			feature->type = REMMINA_PROTOCOL_FEATURE_TYPE_TOOL;
			feature->id = REMMINA_PROTOCOL_FEATURE_TOOL_SFTP;
			feature->opt1 = _("Open SFTP transfer");
			feature->opt2 = "folder-remote";
			feature++;
		}
		feature->type = REMMINA_PROTOCOL_FEATURE_TYPE_END;
#endif
	}

	if (!plugin->open_connection(gp))
		remmina_protocol_widget_close_connection(gp);
}

static void cancel_open_connection_cb(void *cbdata, int btn)
{
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)cbdata;

	remmina_protocol_widget_close_connection(gp);
}

void remmina_protocol_widget_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	gchar *s;
	const gchar *name;
	RemminaMessagePanel *mp;

	/* Exec precommand before everything else */
	mp = remmina_message_panel_new();
	remmina_message_panel_setup_progress(mp, _("Executing external commands…"), NULL, NULL);
	rco_show_message_panel(gp->cnnobj, mp);

	remmina_ext_exec_new(gp->priv->remmina_file, "precommand");
	rco_destroy_message_panel(gp->cnnobj, mp);

	name = remmina_file_get_string(gp->priv->remmina_file, "name");
	s = g_strdup_printf(_("Connecting to '%s'…"), (name ? name : "*"));

	mp = remmina_message_panel_new();
	remmina_message_panel_setup_progress(mp, s, cancel_open_connection_cb, gp);
	g_free(s);
	gp->priv->connect_message_panel = mp;
	rco_show_message_panel(gp->cnnobj, mp);

	remmina_protocol_widget_open_connection_real(gp);
}

static gboolean conn_closed(gpointer data)
{
	/* Close ssh tunnel */
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;

#ifdef HAVE_LIBSSH
	if (gp->priv->ssh_tunnel) {
		remmina_ssh_tunnel_free(gp->priv->ssh_tunnel);
		gp->priv->ssh_tunnel = NULL;
	}
#endif
	/* Exec postcommand */
	remmina_ext_exec_new(gp->priv->remmina_file, "postcommand");
	/* Notify listeners (usually rcw) that the connection is closed */
	g_signal_emit_by_name(G_OBJECT(gp), "disconnect");
	return G_SOURCE_REMOVE;
}

void remmina_protocol_widget_signal_connection_closed(RemminaProtocolWidget *gp)
{
	/* Plugin told us that it closed the connection,
	 * add async event to main thread to complete our close tasks */
	TRACE_CALL(__func__);
	gp->priv->closed = TRUE;
	g_idle_add(conn_closed, (gpointer)gp);
}

static gboolean conn_opened(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;

#ifdef HAVE_LIBSSH
	if (gp->priv->ssh_tunnel)
		remmina_ssh_tunnel_cancel_accept(gp->priv->ssh_tunnel);

#endif
	if (gp->priv->listen_message_panel) {
		rco_destroy_message_panel(gp->cnnobj, gp->priv->listen_message_panel);
		gp->priv->listen_message_panel = NULL;
	}
	if (gp->priv->connect_message_panel) {
		rco_destroy_message_panel(gp->cnnobj, gp->priv->connect_message_panel);
		gp->priv->connect_message_panel = NULL;
	}
	g_signal_emit_by_name(G_OBJECT(gp), "connect");
	return G_SOURCE_REMOVE;
}

void remmina_protocol_widget_signal_connection_opened(RemminaProtocolWidget *gp)
{
	/* Plugin told us that it closed the connection,
	 * add async event to main thread to complete our close tasks */
	TRACE_CALL(__func__);
	g_idle_add(conn_opened, (gpointer)gp);
}

static gboolean update_align(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;

	g_signal_emit_by_name(G_OBJECT(gp), "update-align");
	return G_SOURCE_REMOVE;
}

void remmina_protocol_widget_update_align(RemminaProtocolWidget *gp)
{
	/* Called by the plugin to do updates on rcw */
	TRACE_CALL(__func__);
	g_idle_add(update_align, (gpointer)gp);
}

static gboolean unlock_dynres(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;
	g_signal_emit_by_name(G_OBJECT(gp), "unlock-dynres");
	return G_SOURCE_REMOVE;
}

void remmina_protocol_widget_unlock_dynres(RemminaProtocolWidget *gp)
{
	/* Called by the plugin to do updates on rcw */
	TRACE_CALL(__func__);
	g_idle_add(unlock_dynres, (gpointer)gp);
}

static gboolean desktop_resize(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;
	g_signal_emit_by_name(G_OBJECT(gp), "desktop-resize");
	return G_SOURCE_REMOVE;
}

void remmina_protocol_widget_desktop_resize(RemminaProtocolWidget *gp)
{
	/* Called by the plugin to do updates on rcw */
	TRACE_CALL(__func__);
	g_idle_add(desktop_resize, (gpointer)gp);
}


void remmina_protocol_widget_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	/* kindly ask the protocol plugin to close the connection.
	 *      Nothing else is done here. */

	if (!GTK_IS_WIDGET(gp))
		return;

	if (gp->priv->chat_window) {
		gtk_widget_destroy(gp->priv->chat_window);
		gp->priv->chat_window = NULL;
	}

	if (gp->priv->closed) {
		/* Connection is already closed by the plugin, but
		 * rcw is asking to close again (usually after an error panel)
		 */
		g_signal_emit_by_name(G_OBJECT(gp), "disconnect");
		return;
	}

	/* Ask the plugin to close, async.
	 *      The plugin will emit a "disconnect" signal on gp to call our
	 *      remmina_protocol_widget_on_disconnected() when done */
	gp->priv->plugin->close_connection(gp);

	return;
}


/** Check if the plugin accepts keystrokes.
 */
gboolean remmina_protocol_widget_plugin_receives_keystrokes(RemminaProtocolWidget *gp)
{
	return gp->priv->plugin->send_keystrokes ? TRUE : FALSE;
}

/**
 * Send to the plugin some keystrokes.
 */
void remmina_protocol_widget_send_keystrokes(RemminaProtocolWidget *gp, GtkMenuItem *widget)
{
	TRACE_CALL(__func__);
	gchar *keystrokes = g_object_get_data(G_OBJECT(widget), "keystrokes");
	guint *keyvals;
	gint i;
	GdkKeymap *keymap = gdk_keymap_get_for_display(gdk_display_get_default());
	gchar *iter = keystrokes;
	gunichar character;
	guint keyval;
	GdkKeymapKey *keys;
	gint n_keys;
	/* Single keystroke replace */
	typedef struct _KeystrokeReplace {
		gchar * search;
		gchar * replace;
		guint	keyval;
	} KeystrokeReplace;
	/* Special characters to replace */
	KeystrokeReplace keystrokes_replaces[] =
	{
		{ "\\n",  "\n", GDK_KEY_Return	  },
		{ "\\t",  "\t", GDK_KEY_Tab	  },
		{ "\\b",  "\b", GDK_KEY_BackSpace },
		{ "\\e",  "\e", GDK_KEY_Escape	  },
		{ "\\\\", "\\", GDK_KEY_backslash },
		{ NULL,	  NULL, 0		  }
	};
	/* Keystrokes can only be sent to plugins that accepts them */
	if (remmina_protocol_widget_plugin_receives_keystrokes(gp)) {
		/* Replace special characters */
		for (i = 0; keystrokes_replaces[i].replace; i++) {
			remmina_public_str_replace_in_place(keystrokes,
							    keystrokes_replaces[i].search,
							    keystrokes_replaces[i].replace);
		}
		keyvals = (guint *)g_malloc(strlen(keystrokes));
		while (TRUE) {
			/* Process each character in the keystrokes */
			character = g_utf8_get_char_validated(iter, -1);
			if (character == 0)
				break;
			keyval = gdk_unicode_to_keyval(character);
			/* Replace all the special character with its keyval */
			for (i = 0; keystrokes_replaces[i].replace; i++) {
				if (character == keystrokes_replaces[i].replace[0]) {
					keys = g_new0(GdkKeymapKey, 1);
					keyval = keystrokes_replaces[i].keyval;
					/* A special character was generated, no keyval lookup needed */
					character = 0;
					break;
				}
			}
			/* Decode character if it’s not a special character */
			if (character) {
				/* get keyval without modifications */
				if (!gdk_keymap_get_entries_for_keyval(keymap, keyval, &keys, &n_keys)) {
					g_warning("keyval 0x%04x has no keycode!", keyval);
					iter = g_utf8_find_next_char(iter, NULL);
					continue;
				}
			}
			/* Add modifier keys */
			n_keys = 0;
			if (keys->level & 1)
				keyvals[n_keys++] = GDK_KEY_Shift_L;
			if (keys->level & 2)
				keyvals[n_keys++] = GDK_KEY_Alt_R;
			keyvals[n_keys++] = keyval;
			/* Send keystroke to the plugin */
			gp->priv->plugin->send_keystrokes(gp, keyvals, n_keys);
			g_free(keys);
			/* Process next character in the keystrokes */
			iter = g_utf8_find_next_char(iter, NULL);
		}
		g_free(keyvals);
	}
	g_free(keystrokes);
	return;
}

gboolean remmina_protocol_widget_plugin_screenshot(RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd)
{
	if (!gp->priv->plugin->get_plugin_screenshot) {
		remmina_log_printf("plugin screenshot function is not implemented\n");
		return FALSE;
	}

	return gp->priv->plugin->get_plugin_screenshot(gp, rpsd);
}

void remmina_protocol_widget_emit_signal(RemminaProtocolWidget *gp, const gchar *signal_name)
{
	TRACE_CALL(__func__);

	g_print("Emitting signals should be used from the object itself, not from another object");
	raise(SIGINT);

	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_PROTOCOLWIDGET_EMIT_SIGNAL;
		d->p.protocolwidget_emit_signal.signal_name = signal_name;
		d->p.protocolwidget_emit_signal.gp = gp;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}
	g_signal_emit_by_name(G_OBJECT(gp), signal_name);
}

const RemminaProtocolFeature *remmina_protocol_widget_get_features(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->features;
}

gboolean remmina_protocol_widget_query_feature_by_type(RemminaProtocolWidget *gp, RemminaProtocolFeatureType type)
{
	TRACE_CALL(__func__);
	const RemminaProtocolFeature *feature;

#ifdef HAVE_LIBSSH
	if (type == REMMINA_PROTOCOL_FEATURE_TYPE_TOOL &&
	    remmina_file_get_int(gp->priv->remmina_file, "ssh_enabled", FALSE))
		return TRUE;

#endif
	for (feature = gp->priv->plugin->features; feature && feature->type; feature++)
		if (feature->type == type)
			return TRUE;
	return FALSE;
}

gboolean remmina_protocol_widget_query_feature_by_ref(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	return gp->priv->plugin->query_feature(gp, feature);
}

void remmina_protocol_widget_call_feature_by_type(RemminaProtocolWidget *gp, RemminaProtocolFeatureType type, gint id)
{
	TRACE_CALL(__func__);
	const RemminaProtocolFeature *feature;

	for (feature = gp->priv->plugin->features; feature && feature->type; feature++) {
		if (feature->type == type && (id == 0 || feature->id == id)) {
			remmina_protocol_widget_call_feature_by_ref(gp, feature);
			break;
		}
	}
}

void remmina_protocol_widget_call_feature_by_ref(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	switch (feature->id) {
#ifdef HAVE_LIBSSH
	case REMMINA_PROTOCOL_FEATURE_TOOL_SSH:
		if (gp->priv->ssh_tunnel) {
			rcw_open_from_file_full(
				remmina_file_dup_temp_protocol(gp->priv->remmina_file, "SSH"), NULL, gp->priv->ssh_tunnel, NULL);
			return;
		}
		break;

	case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
		if (gp->priv->ssh_tunnel) {
			rcw_open_from_file_full(
				remmina_file_dup_temp_protocol(gp->priv->remmina_file, "SFTP"), NULL, gp->priv->ssh_tunnel, NULL);
			return;
		}
		break;
#endif
	default:
		break;
	}
	gp->priv->plugin->call_feature(gp, feature);
}

static gboolean remmina_protocol_widget_on_key_press(GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	if (gp->priv->hostkey_func)
		return gp->priv->hostkey_func(gp, event->keyval, FALSE);
	return FALSE;
}

static gboolean remmina_protocol_widget_on_key_release(GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	if (gp->priv->hostkey_func)
		return gp->priv->hostkey_func(gp, event->keyval, TRUE);

	return FALSE;
}

void remmina_protocol_widget_register_hostkey(RemminaProtocolWidget *gp, GtkWidget *widget)
{
	TRACE_CALL(__func__);
	g_signal_connect(G_OBJECT(widget), "key-press-event", G_CALLBACK(remmina_protocol_widget_on_key_press), gp);
	g_signal_connect(G_OBJECT(widget), "key-release-event", G_CALLBACK(remmina_protocol_widget_on_key_release), gp);
}

void remmina_protocol_widget_set_hostkey_func(RemminaProtocolWidget *gp, RemminaHostkeyFunc func)
{
	TRACE_CALL(__func__);
	gp->priv->hostkey_func = func;
}

RemminaMessagePanel *remmina_protocol_widget_mpprogress(RemminaConnectionObject *cnnobj, const gchar *msg, RemminaMessagePanelCallback response_callback, gpointer response_callback_data)
{
	RemminaMessagePanel *mp;

	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_PROTOCOLWIDGET_MPPROGRESS;
		d->p.protocolwidget_mpprogress.cnnobj = cnnobj;
		d->p.protocolwidget_mpprogress.message = msg;
		d->p.protocolwidget_mpprogress.response_callback = response_callback;
		d->p.protocolwidget_mpprogress.response_callback_data = response_callback_data;
		remmina_masterthread_exec_and_wait(d);
		mp = d->p.protocolwidget_mpprogress.ret_mp;
		g_free(d);
		return mp;
	}

	mp = remmina_message_panel_new();
	remmina_message_panel_setup_progress(mp, msg, response_callback, response_callback_data);
	rco_show_message_panel(cnnobj, mp);
	return mp;
}

void remmina_protocol_widget_mpdestroy(RemminaConnectionObject *cnnobj, RemminaMessagePanel *mp)
{
	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_PROTOCOLWIDGET_MPDESTROY;
		d->p.protocolwidget_mpdestroy.cnnobj = cnnobj;
		d->p.protocolwidget_mpdestroy.mp = mp;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}
	rco_destroy_message_panel(cnnobj, mp);
}

#ifdef HAVE_LIBSSH
static void cancel_init_tunnel_cb(void *cbdata, int btn)
{
	printf("Remmina: Cancelling an opening tunnel is not implemented\n");
}
static gboolean remmina_protocol_widget_init_tunnel(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel;
	gint ret;
	gchar *msg;
	RemminaMessagePanel *mp;

	/* Reuse existing SSH connection if it’s reconnecting to destination */
	if (gp->priv->ssh_tunnel == NULL) {
		tunnel = remmina_ssh_tunnel_new_from_file(gp->priv->remmina_file);

		msg = g_strdup_printf(_("Connecting to %s via SSH…"), REMMINA_SSH(tunnel)->server);

		mp = remmina_protocol_widget_mpprogress(gp->cnnobj, msg, cancel_init_tunnel_cb, NULL);
		g_free(msg);

		if (!remmina_ssh_init_session(REMMINA_SSH(tunnel))) {
			remmina_protocol_widget_set_error(gp, REMMINA_SSH(tunnel)->error);
			remmina_ssh_tunnel_free(tunnel);
			return FALSE;
		}

		ret = remmina_ssh_auth_gui(REMMINA_SSH(tunnel), gp, gp->priv->remmina_file);
		if (ret <= 0) {
			if (ret == 0)
				remmina_protocol_widget_set_error(gp, REMMINA_SSH(tunnel)->error);
			remmina_ssh_tunnel_free(tunnel);
			return FALSE;
		}

		remmina_protocol_widget_mpdestroy(gp->cnnobj, mp);

		gp->priv->ssh_tunnel = tunnel;
	}

	return TRUE;
}
#endif


#ifdef HAVE_LIBSSH
static void cancel_start_direct_tunnel_cb(void *cbdata, int btn)
{
	printf("Remmina: Cancelling start_direct_tunnel is not implemented\n");
}
#endif


/**
 * Start an SSH tunnel if possible and return the host:port string.
 *
 */
gchar *remmina_protocol_widget_start_direct_tunnel(RemminaProtocolWidget *gp, gint default_port, gboolean port_plus)
{
	TRACE_CALL(__func__);
	const gchar *server;
	gchar *host, *dest;
	gint port;
	gchar *msg;

	server = remmina_file_get_string(gp->priv->remmina_file, "server");

	if (!server)
		return g_strdup("");

	remmina_public_get_server_port(server, default_port, &host, &port);

	if (port_plus && port < 100)
		/* Protocols like VNC supports using instance number :0, :1, etc. as port number. */
		port += default_port;

#ifdef HAVE_LIBSSH
	RemminaMessagePanel *mp;

	if (!remmina_file_get_int(gp->priv->remmina_file, "ssh_enabled", FALSE)) {
		dest = g_strdup_printf("[%s]:%i", host, port);
		g_free(host);
		return dest;
	}

	/* If we have a previous SSH tunnel, destroy it */
	if (gp->priv->ssh_tunnel) {
		remmina_ssh_tunnel_free(gp->priv->ssh_tunnel);
		gp->priv->ssh_tunnel = NULL;
	}

	if (!remmina_protocol_widget_init_tunnel(gp)) {
		g_free(host);
		return NULL;
	}

	msg = g_strdup_printf(_("Connecting to %s via SSH…"), server);
	mp = remmina_protocol_widget_mpprogress(gp->cnnobj, msg, cancel_start_direct_tunnel_cb, NULL);
	g_free(msg);

	if (remmina_file_get_int(gp->priv->remmina_file, "ssh_loopback", FALSE)) {
		g_free(host);
		host = g_strdup("127.0.0.1");
	}

	if (!remmina_ssh_tunnel_open(gp->priv->ssh_tunnel, host, port, remmina_pref.sshtunnel_port)) {
		g_free(host);
		remmina_protocol_widget_set_error(gp, REMMINA_SSH(gp->priv->ssh_tunnel)->error);
		return NULL;
	}
	g_free(host);

	remmina_protocol_widget_mpdestroy(gp->cnnobj, mp);

	return g_strdup_printf("127.0.0.1:%i", remmina_pref.sshtunnel_port);

#else

	dest = g_strdup_printf("[%s]:%i", host, port);
	g_free(host);
	return dest;

#endif
}

#ifdef HAVE_LIBSSH
static void cancel_start_reverse_tunnel_cb(void *cbdata, int btn)
{
	printf("Remmina: Cancelling start_reverse_tunnel is not implemented\n");
}
#endif


gboolean remmina_protocol_widget_start_reverse_tunnel(RemminaProtocolWidget *gp, gint local_port)
{
	TRACE_CALL(__func__);
#ifdef HAVE_LIBSSH
	gchar *msg;
	RemminaMessagePanel *mp;

	if (!remmina_file_get_int(gp->priv->remmina_file, "ssh_enabled", FALSE))
		return TRUE;

	if (!remmina_protocol_widget_init_tunnel(gp))
		return FALSE;

	msg = g_strdup_printf(_("Awaiting incoming SSH connection at port %i…"), remmina_file_get_int(gp->priv->remmina_file, "listenport", 0));
	mp = remmina_protocol_widget_mpprogress(gp->cnnobj, msg, cancel_start_reverse_tunnel_cb, NULL);
	g_free(msg);

	if (!remmina_ssh_tunnel_reverse(gp->priv->ssh_tunnel, remmina_file_get_int(gp->priv->remmina_file, "listenport", 0), local_port)) {
		remmina_protocol_widget_set_error(gp, REMMINA_SSH(gp->priv->ssh_tunnel)->error);
		return FALSE;
	}
	remmina_protocol_widget_mpdestroy(gp->cnnobj, mp);
#endif

	return TRUE;
}

gboolean remmina_protocol_widget_ssh_exec(RemminaProtocolWidget *gp, gboolean wait, const gchar *fmt, ...)
{
	TRACE_CALL(__func__);
#ifdef HAVE_LIBSSH
	RemminaSSHTunnel *tunnel = gp->priv->ssh_tunnel;
	ssh_channel channel;
	gint status;
	gboolean ret = FALSE;
	gchar *cmd, *ptr;
	va_list args;

	if ((channel = ssh_channel_new(REMMINA_SSH(tunnel)->session)) == NULL)
		return FALSE;

	va_start(args, fmt);
	cmd = g_strdup_vprintf(fmt, args);
	va_end(args);

	if (ssh_channel_open_session(channel) == SSH_OK &&
	    ssh_channel_request_exec(channel, cmd) == SSH_OK) {
		if (wait) {
			ssh_channel_send_eof(channel);
			status = ssh_channel_get_exit_status(channel);
			ptr = strchr(cmd, ' ');
			if (ptr) *ptr = '\0';
			switch (status) {
			case 0:
				ret = TRUE;
				break;
			case 127:
				remmina_ssh_set_application_error(REMMINA_SSH(tunnel),
								  _("Could not find %s command on the SSH server"), cmd);
				break;
			default:
				remmina_ssh_set_application_error(REMMINA_SSH(tunnel),
								  _("Could not run %s command on the SSH server (status = %i)."), cmd, status);
				break;
			}
		} else {
			ret = TRUE;
		}
	} else {
		// TRANSLATORS: %s is a placeholder for an error message
		remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not run command: %s"));
	}
	g_free(cmd);
	if (wait)
		ssh_channel_close(channel);
	ssh_channel_free(channel);
	return ret;

#else

	return FALSE;

#endif
}

#ifdef HAVE_LIBSSH
static gboolean remmina_protocol_widget_tunnel_init_callback(RemminaSSHTunnel *tunnel, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET(data);
	gchar *server;
	gint port;
	gboolean ret;

	remmina_public_get_server_port(remmina_file_get_string(gp->priv->remmina_file, "server"), 177, &server, &port);
	ret = ((RemminaXPortTunnelInitFunc)gp->priv->init_func)(gp,
								tunnel->remotedisplay, (tunnel->bindlocalhost ? "localhost" : server), port);
	g_free(server);

	return ret;
}

static gboolean remmina_protocol_widget_tunnel_connect_callback(RemminaSSHTunnel *tunnel, gpointer data)
{
	TRACE_CALL(__func__);
	return TRUE;
}

static gboolean remmina_protocol_widget_tunnel_disconnect_callback(RemminaSSHTunnel *tunnel, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET(data);

	if (REMMINA_SSH(tunnel)->error)
		remmina_protocol_widget_set_error(gp, "%s", REMMINA_SSH(tunnel)->error);

	IDLE_ADD((GSourceFunc)remmina_protocol_widget_close_connection, gp);
	return TRUE;
}
#endif
#ifdef HAVE_LIBSSH
static void cancel_connect_xport_cb(void *cbdata, int btn)
{
	printf("Remmina: Cancelling an XPort connection is not implemented\n");
}
#endif
gboolean remmina_protocol_widget_start_xport_tunnel(RemminaProtocolWidget *gp, RemminaXPortTunnelInitFunc init_func)
{
	TRACE_CALL(__func__);
#ifdef HAVE_LIBSSH
	gboolean bindlocalhost;
	gchar *server;
	gchar *msg;
	RemminaMessagePanel *mp;

	if (!remmina_protocol_widget_init_tunnel(gp)) return FALSE;

	msg = g_strdup_printf(_("Connecting to %s via SSH…"), remmina_file_get_string(gp->priv->remmina_file, "server"));
	mp = remmina_protocol_widget_mpprogress(gp->cnnobj, msg, cancel_connect_xport_cb, NULL);
	g_free(msg);

	gp->priv->init_func = init_func;
	gp->priv->ssh_tunnel->init_func = remmina_protocol_widget_tunnel_init_callback;
	gp->priv->ssh_tunnel->connect_func = remmina_protocol_widget_tunnel_connect_callback;
	gp->priv->ssh_tunnel->disconnect_func = remmina_protocol_widget_tunnel_disconnect_callback;
	gp->priv->ssh_tunnel->callback_data = gp;

	remmina_public_get_server_port(remmina_file_get_string(gp->priv->remmina_file, "server"), 0, &server, NULL);
	bindlocalhost = (g_strcmp0(REMMINA_SSH(gp->priv->ssh_tunnel)->server, server) == 0);
	g_free(server);

	if (!remmina_ssh_tunnel_xport(gp->priv->ssh_tunnel, bindlocalhost)) {
		remmina_protocol_widget_set_error(gp, "Could not open channel, %s",
						  ssh_get_error(REMMINA_SSH(gp->priv->ssh_tunnel)->session));
		return FALSE;
	}

	remmina_protocol_widget_mpdestroy(gp->cnnobj, mp);

	return TRUE;

#else
	return FALSE;
#endif
}

void remmina_protocol_widget_set_display(RemminaProtocolWidget *gp, gint display)
{
	TRACE_CALL(__func__);
#ifdef HAVE_LIBSSH
	if (gp->priv->ssh_tunnel->localdisplay) g_free(gp->priv->ssh_tunnel->localdisplay);
	gp->priv->ssh_tunnel->localdisplay = g_strdup_printf("unix:%i", display);
#endif
}

gint remmina_protocol_widget_get_profile_remote_width(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	/* Returns the width of remote desktop as choosen by the user profile */
	return gp->priv->profile_remote_width;
}

gint remmina_protocol_widget_get_profile_remote_height(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	/* Returns the height of remote desktop as choosen by the user profile */
	return gp->priv->profile_remote_height;
}


gint remmina_protocol_widget_get_width(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->width;
}

void remmina_protocol_widget_set_width(RemminaProtocolWidget *gp, gint width)
{
	TRACE_CALL(__func__);
	gp->priv->width = width;
}

gint remmina_protocol_widget_get_height(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->height;
}

void remmina_protocol_widget_set_height(RemminaProtocolWidget *gp, gint height)
{
	TRACE_CALL(__func__);
	gp->priv->height = height;
}

RemminaScaleMode remmina_protocol_widget_get_current_scale_mode(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->scalemode;
}

void remmina_protocol_widget_set_current_scale_mode(RemminaProtocolWidget *gp, RemminaScaleMode scalemode)
{
	TRACE_CALL(__func__);
	gp->priv->scalemode = scalemode;
}

gboolean remmina_protocol_widget_get_expand(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->scaler_expand;
}

void remmina_protocol_widget_set_expand(RemminaProtocolWidget *gp, gboolean expand)
{
	TRACE_CALL(__func__);
	gp->priv->scaler_expand = expand;
	return;
}

gboolean remmina_protocol_widget_has_error(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->has_error;
}

const gchar *remmina_protocol_widget_get_error_message(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->error_message;
}

void remmina_protocol_widget_set_error(RemminaProtocolWidget *gp, const gchar *fmt, ...)
{
	TRACE_CALL(__func__);
	va_list args;

	if (gp->priv->error_message) g_free(gp->priv->error_message);

	if (fmt == NULL) {
		gp->priv->has_error = FALSE;
		gp->priv->error_message = NULL;
		return;
	}

	va_start(args, fmt);
	gp->priv->error_message = g_strdup_vprintf(fmt, args);
	va_end(args);

	gp->priv->has_error = TRUE;
}

gboolean remmina_protocol_widget_is_closed(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->closed;
}

RemminaFile *remmina_protocol_widget_get_file(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->remmina_file;
}

struct remmina_protocol_widget_dialog_mt_data_t {
	/* Input data */
	RemminaProtocolWidget *		gp;
	gchar *				title;
	gchar *				default_username;
	gchar *				default_password;
	gchar *				default_domain;
	gchar *				strpasswordlabel;
	enum panel_type			dtype;
	RemminaMessagePanelFlags	pflags;
	gboolean			called_from_subthread;
	/* Running status */
	pthread_mutex_t			pt_mutex;
	pthread_cond_t			pt_cond;
	/* Output/retval */
	int				rcbutton;
};

static void authpanel_mt_cb(void *user_data, int button)
{
	struct remmina_protocol_widget_dialog_mt_data_t *d = (struct remmina_protocol_widget_dialog_mt_data_t *)user_data;

	d->rcbutton = button;
	if (button == GTK_RESPONSE_OK) {
		if (d->dtype == RPWDT_AUTH) {
			d->gp->priv->password = remmina_message_panel_field_get_string(d->gp->priv->auth_message_panel, REMMINA_MESSAGE_PANEL_PASSWORD);
			d->gp->priv->username = remmina_message_panel_field_get_string(d->gp->priv->auth_message_panel, REMMINA_MESSAGE_PANEL_USERNAME);
			d->gp->priv->domain = remmina_message_panel_field_get_string(d->gp->priv->auth_message_panel, REMMINA_MESSAGE_PANEL_DOMAIN);
			d->gp->priv->save_password = remmina_message_panel_field_get_switch_state(d->gp->priv->auth_message_panel, REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD);
		} else if (d->dtype == RPWDT_AUTHX509) {
			d->gp->priv->cacert = remmina_message_panel_field_get_filename(d->gp->priv->auth_message_panel, REMMINA_MESSAGE_PANEL_CACERTFILE);
			d->gp->priv->cacrl = remmina_message_panel_field_get_filename(d->gp->priv->auth_message_panel, REMMINA_MESSAGE_PANEL_CACRLFILE);
			d->gp->priv->clientcert = remmina_message_panel_field_get_filename(d->gp->priv->auth_message_panel, REMMINA_MESSAGE_PANEL_CLIENTCERTFILE);
			d->gp->priv->clientkey = remmina_message_panel_field_get_filename(d->gp->priv->auth_message_panel, REMMINA_MESSAGE_PANEL_CLIENTKEYFILE);
		}
	}

	if (d->called_from_subthread) {
		/* Hide and destroy message panel, we can do it now because we are on the main thread */
		rco_destroy_message_panel(d->gp->cnnobj, d->gp->priv->auth_message_panel);

		/* Awake the locked subthread, when called from subthread */
		pthread_mutex_lock(&d->pt_mutex);
		pthread_cond_signal(&d->pt_cond);
		pthread_mutex_unlock(&d->pt_mutex);
	} else {
		/* Signal completion, when called from main thread. Message panel will be destroyed by the caller */
		remmina_message_panel_response(d->gp->priv->auth_message_panel, button);
	}
}

static gboolean remmina_protocol_widget_dialog_mt_setup(gpointer user_data)
{
	struct remmina_protocol_widget_dialog_mt_data_t *d = (struct remmina_protocol_widget_dialog_mt_data_t *)user_data;

	RemminaFile *remminafile = d->gp->priv->remmina_file;
	RemminaMessagePanel *mp;
	const gchar *s;

	mp = remmina_message_panel_new();

	if (d->dtype == RPWDT_AUTH) {
		remmina_message_panel_setup_auth(mp, authpanel_mt_cb, d, d->title, d->strpasswordlabel, d->pflags);
		remmina_message_panel_field_set_string(mp, REMMINA_MESSAGE_PANEL_USERNAME, d->default_username);
		if (d->pflags & REMMINA_MESSAGE_PANEL_FLAG_DOMAIN)
			remmina_message_panel_field_set_string(mp, REMMINA_MESSAGE_PANEL_DOMAIN, d->default_domain);
		remmina_message_panel_field_set_string(mp, REMMINA_MESSAGE_PANEL_PASSWORD, d->default_password);
		if (d->pflags & REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD)
			remmina_message_panel_field_set_switch(mp, REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD, (d->pflags & REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD) ? TRUE : FALSE);
	} else if (d->dtype == RPWDT_QUESTIONYESNO) {
		remmina_message_panel_setup_question(mp, d->title, authpanel_mt_cb, d);
	} else if (d->dtype == RPWDT_AUTHX509) {
		remmina_message_panel_setup_auth_x509(mp, authpanel_mt_cb, d);
		if ((s = remmina_file_get_string(remminafile, "cacert")) != NULL)
			remmina_message_panel_field_set_filename(mp, REMMINA_MESSAGE_PANEL_CACERTFILE, s);
		if ((s = remmina_file_get_string(remminafile, "cacrl")) != NULL)
			remmina_message_panel_field_set_filename(mp, REMMINA_MESSAGE_PANEL_CACRLFILE, s);
		if ((s = remmina_file_get_string(remminafile, "clientcert")) != NULL)
			remmina_message_panel_field_set_filename(mp, REMMINA_MESSAGE_PANEL_CLIENTCERTFILE, s);
		if ((s = remmina_file_get_string(remminafile, "clientkey")) != NULL)
			remmina_message_panel_field_set_filename(mp, REMMINA_MESSAGE_PANEL_CLIENTKEYFILE, s);
	}

	d->gp->priv->auth_message_panel = mp;
	rco_show_message_panel(d->gp->cnnobj, mp);

	return FALSE;
}

typedef struct {
	RemminaMessagePanel *	mp;
	GMainLoop *		loop;
	gint			response;
	gboolean		destroyed;
} MpRunInfo;

static void shutdown_loop(MpRunInfo *mpri)
{
	if (g_main_loop_is_running(mpri->loop))
		g_main_loop_quit(mpri->loop);
}

static void run_response_handler(RemminaMessagePanel *mp, gint response_id, gpointer data)
{
	MpRunInfo *mpri = (MpRunInfo *)data;

	mpri->response = response_id;
	shutdown_loop(mpri);
}

static void run_unmap_handler(RemminaMessagePanel *mp, gpointer data)
{
	MpRunInfo *mpri = (MpRunInfo *)data;

	mpri->response = GTK_RESPONSE_CANCEL;
	shutdown_loop(mpri);
}

static void run_destroy_handler(RemminaMessagePanel *mp, gpointer data)
{
	MpRunInfo *mpri = (MpRunInfo *)data;

	mpri->destroyed = TRUE;
	mpri->response = GTK_RESPONSE_CANCEL;
	shutdown_loop(mpri);
}

static int remmina_protocol_widget_dialog(enum panel_type dtype, RemminaProtocolWidget *gp, RemminaMessagePanelFlags pflags,
					  const gchar *title, const gchar *default_username, const gchar *default_password, const gchar *default_domain,
					  const gchar *strpasswordlabel)
{
	TRACE_CALL(__func__);

	struct remmina_protocol_widget_dialog_mt_data_t *d = (struct remmina_protocol_widget_dialog_mt_data_t *)g_malloc(sizeof(struct remmina_protocol_widget_dialog_mt_data_t));
	int rcbutton;

	d->gp = gp;
	d->pflags = pflags;
	d->dtype = dtype;
	d->title = g_strdup(title);
	d->strpasswordlabel = g_strdup(strpasswordlabel);
	d->default_username = g_strdup(default_username);
	d->default_password = g_strdup(default_password);
	d->default_domain = g_strdup(default_domain);
	d->called_from_subthread = FALSE;

	if (remmina_masterthread_exec_is_main_thread()) {
		/* Run the MessagePanel in main thread, in a very similar way of gtk_dialog_run() */
		MpRunInfo mpri = { NULL, NULL, GTK_RESPONSE_CANCEL, FALSE };

		gulong unmap_handler;
		gulong destroy_handler;
		gulong response_handler;

		remmina_protocol_widget_dialog_mt_setup(d);

		mpri.mp = d->gp->priv->auth_message_panel;

		if (!gtk_widget_get_visible(GTK_WIDGET(mpri.mp)))
			gtk_widget_show(GTK_WIDGET(mpri.mp));
		response_handler = g_signal_connect(mpri.mp, "response", G_CALLBACK(run_response_handler), &mpri);
		unmap_handler = g_signal_connect(mpri.mp, "unmap", G_CALLBACK(run_unmap_handler), &mpri);
		destroy_handler = g_signal_connect(mpri.mp, "destroy", G_CALLBACK(run_destroy_handler), &mpri);

		g_object_ref(mpri.mp);

		mpri.loop = g_main_loop_new(NULL, FALSE);
		g_main_loop_run(mpri.loop);
		g_main_loop_unref(mpri.loop);

		if (!mpri.destroyed) {
			g_signal_handler_disconnect(mpri.mp, response_handler);
			g_signal_handler_disconnect(mpri.mp, destroy_handler);
			g_signal_handler_disconnect(mpri.mp, unmap_handler);
		}
		g_object_unref(mpri.mp);

		rco_destroy_message_panel(d->gp->cnnobj, d->gp->priv->auth_message_panel);

		rcbutton = mpri.response;
	} else {
		d->called_from_subthread = TRUE;
		// pthread_cleanup_push(ptcleanup, (void*)d);
		pthread_cond_init(&d->pt_cond, NULL);
		pthread_mutex_init(&d->pt_mutex, NULL);
		g_idle_add(remmina_protocol_widget_dialog_mt_setup, d);
		pthread_mutex_lock(&d->pt_mutex);
		pthread_cond_wait(&d->pt_cond, &d->pt_mutex);
		// pthread_cleanup_pop(0);
		pthread_mutex_destroy(&d->pt_mutex);
		pthread_cond_destroy(&d->pt_cond);

		rcbutton = d->rcbutton;
	}

	g_free(d->title);
	g_free(d->strpasswordlabel);
	g_free(d->default_username);
	g_free(d->default_password);
	g_free(d->default_domain);
	g_free(d);
	return rcbutton;
}

gint remmina_protocol_widget_panel_question_yesno(RemminaProtocolWidget *gp, const char *msg)
{
	return remmina_protocol_widget_dialog(RPWDT_QUESTIONYESNO, gp, 0, msg, NULL, NULL, NULL, NULL);
}

gint remmina_protocol_widget_panel_auth(RemminaProtocolWidget *gp, RemminaMessagePanelFlags pflags,
					const gchar *title, const gchar *default_username, const gchar *default_password, const gchar *default_domain, const gchar *password_prompt)
{
	TRACE_CALL(__func__);
	return remmina_protocol_widget_dialog(RPWDT_AUTH, gp, pflags, title, default_username,
					      default_password, default_domain, password_prompt == NULL ? _("Password") : password_prompt);
}

gint remmina_protocol_widget_panel_authuserpwd_ssh_tunnel(RemminaProtocolWidget *gp, gboolean want_domain, gboolean allow_password_saving)
{
	TRACE_CALL(__func__);
	unsigned pflags;
	RemminaFile *remminafile = gp->priv->remmina_file;
	const gchar *username, *password;

	pflags = REMMINA_MESSAGE_PANEL_FLAG_USERNAME;
	if (remmina_file_get_filename(remminafile) != NULL &&
	    !remminafile->prevent_saving && allow_password_saving)
		pflags |= REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD;

	username = remmina_file_get_string(remminafile, "ssh_username");
	password = remmina_file_get_string(remminafile, "ssh_password");

	return remmina_protocol_widget_dialog(RPWDT_AUTH, gp, pflags, _("Type in username and password for SSH."), username,
					      password, NULL, _("Password"));
}

/*
 * gint remmina_protocol_widget_panel_authpwd(RemminaProtocolWidget* gp, RemminaAuthpwdType authpwd_type, gboolean allow_password_saving)
 * {
 *      TRACE_CALL(__func__);
 *      unsigned pflags;
 *      RemminaFile* remminafile = gp->priv->remmina_file;
 *      char *password_prompt;
 *      int rc;
 *
 *      pflags = 0;
 *      if (remmina_file_get_filename(remminafile) != NULL &&
 *               !remminafile->prevent_saving && allow_password_saving)
 *              pflags |= REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD;
 *
 *      switch (authpwd_type) {
 *              case REMMINA_AUTHPWD_TYPE_PROTOCOL:
 *                      password_prompt = g_strdup_printf(_("%s password"), remmina_file_get_string(remminafile, "protocol"));
 *                      break;
 *              case REMMINA_AUTHPWD_TYPE_SSH_PWD:
 *                      password_prompt = g_strdup(_("SSH password"));
 *                      break;
 *              case REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY:
 *                      password_prompt = g_strdup(_("SSH private key passphrase"));
 *                      break;
 *              default:
 *                      password_prompt = g_strdup(_("Password"));
 *                      break;
 *      }
 *
 *      rc = remmina_protocol_widget_dialog(RPWDT_AUTH, gp, pflags, password_prompt);
 *      g_free(password_prompt);
 *      return rc;
 *
 * }
 */
gint remmina_protocol_widget_panel_authx509(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	return remmina_protocol_widget_dialog(RPWDT_AUTHX509, gp, 0, NULL, NULL, NULL, NULL, NULL);
}


gint remmina_protocol_widget_panel_new_certificate(RemminaProtocolWidget *gp, const gchar *subject, const gchar *issuer, const gchar *fingerprint)
{
	TRACE_CALL(__func__);
	gchar *s;
	int rc;

	// For markup see https://developer.gnome.org/pygtk/stable/pango-markup-language.html
	s = g_strdup_printf(
		"<big>%s</big>\n\n%s %s\n%s %s\n%s %s\n\n<big>%s</big>",
		_("Certificate details:"),
		_("Subject:"), subject,
		_("Issuer:"), issuer,
		_("Fingerprint:"), fingerprint,
		_("Accept certificate?"));
	rc = remmina_protocol_widget_dialog(RPWDT_QUESTIONYESNO, gp, 0, s, NULL, NULL, NULL, NULL);
	g_free(s);

	/* For compatibility with plugin API: the plugin expects GTK_RESPONSE_OK when user confirms new cert */
	return rc == GTK_RESPONSE_YES ? GTK_RESPONSE_OK : rc;
}

gint remmina_protocol_widget_panel_changed_certificate(RemminaProtocolWidget *gp, const gchar *subject, const gchar *issuer, const gchar *new_fingerprint, const gchar *old_fingerprint)
{
	TRACE_CALL(__func__);
	gchar *s;
	int rc;

	// For markup see https://developer.gnome.org/pygtk/stable/pango-markup-language.html
	s = g_strdup_printf(
		"<big>%s</big>\n\n%s %s\n%s %s\n%s %s\n%s %s\n\n<big>%s</big>",
		_("The certificate changed! Details:"),
		_("Subject:"), subject,
		_("Issuer:"), issuer,
		_("Old fingerprint:"), old_fingerprint,
		_("New fingerprint:"), new_fingerprint,
		_("Accept changed certificate?"));
	rc = remmina_protocol_widget_dialog(RPWDT_QUESTIONYESNO, gp, 0, s, NULL, NULL, NULL, NULL);
	g_free(s);

	/* For compatibility with plugin API: The plugin expects GTK_RESPONSE_OK when user confirms new cert */
	return rc == GTK_RESPONSE_YES ? GTK_RESPONSE_OK : rc;
}

gchar *remmina_protocol_widget_get_username(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return g_strdup(gp->priv->username);
}

gchar *remmina_protocol_widget_get_password(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return g_strdup(gp->priv->password);
}

gchar *remmina_protocol_widget_get_domain(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return g_strdup(gp->priv->domain);
}

gboolean remmina_protocol_widget_get_savepassword(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp->priv->save_password;
}

gchar *remmina_protocol_widget_get_cacert(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	gchar *s;

	s = gp->priv->cacert;
	return s && s[0] ? g_strdup(s) : NULL;
}

gchar *remmina_protocol_widget_get_cacrl(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	gchar *s;

	s = gp->priv->cacrl;
	return s && s[0] ? g_strdup(s) : NULL;
}

gchar *remmina_protocol_widget_get_clientcert(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	gchar *s;

	s = gp->priv->clientcert;
	return s && s[0] ? g_strdup(s) : NULL;
}

gchar *remmina_protocol_widget_get_clientkey(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	gchar *s;

	s = gp->priv->clientkey;
	return s && s[0] ? g_strdup(s) : NULL;
}

void remmina_protocol_widget_save_cred(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	RemminaFile *remminafile = gp->priv->remmina_file;
	gchar *s;
	gboolean save = FALSE;

	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_INIT_SAVE_CRED;
		d->p.init_save_creds.gp = gp;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}

	/* Save username and certificates if any; save the password if it’s requested to do so */
	s = gp->priv->username;
	if (s && s[0]) {
		remmina_file_set_string(remminafile, "username", s);
		save = TRUE;
	}
	s = gp->priv->cacert;
	if (s && s[0]) {
		remmina_file_set_string(remminafile, "cacert", s);
		save = TRUE;
	}
	s = gp->priv->cacrl;
	if (s && s[0]) {
		remmina_file_set_string(remminafile, "cacrl", s);
		save = TRUE;
	}
	s = gp->priv->clientcert;
	if (s && s[0]) {
		remmina_file_set_string(remminafile, "clientcert", s);
		save = TRUE;
	}
	s = gp->priv->clientkey;
	if (s && s[0]) {
		remmina_file_set_string(remminafile, "clientkey", s);
		save = TRUE;
	}
	if (gp->priv->save_password) {
		remmina_file_set_string(remminafile, "password", gp->priv->password);
		save = TRUE;
	}
	if (save)
		remmina_file_save(remminafile);
}


void remmina_protocol_widget_panel_show_listen(RemminaProtocolWidget *gp, gint port)
{
	TRACE_CALL(__func__);
	RemminaMessagePanel *mp;
	gchar *s;

	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_PROTOCOLWIDGET_PANELSHOWLISTEN;
		d->p.protocolwidget_panelshowlisten.gp = gp;
		d->p.protocolwidget_panelshowlisten.port = port;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}

	mp = remmina_message_panel_new();
	s = g_strdup_printf(
		_("Listening on port %i for an incoming %s connection…"), port,
		remmina_file_get_string(gp->priv->remmina_file, "protocol"));
	remmina_message_panel_setup_progress(mp, s, NULL, NULL);
	g_free(s);
	gp->priv->listen_message_panel = mp;
	rco_show_message_panel(gp->cnnobj, mp);
}

void remmina_protocol_widget_panel_show_retry(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaMessagePanel *mp;

	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_PROTOCOLWIDGET_MPSHOWRETRY;
		d->p.protocolwidget_mpshowretry.gp = gp;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}

	mp = remmina_message_panel_new();
	remmina_message_panel_setup_progress(mp, _("Could not authenticate, attempting reconnection…"), NULL, NULL);
	rco_show_message_panel(gp->cnnobj, mp);
}

void remmina_protocol_widget_panel_show(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	printf("Remmina: The %s function is not implemented, and is left here only for plugin API compatibility.\n", __func__);
}

void remmina_protocol_widget_panel_hide(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	printf("Remmina: The %s function is not implemented, and is left here only for plugin API compatibility.\n", __func__);
}

static void remmina_protocol_widget_chat_on_destroy(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	gp->priv->chat_window = NULL;
}

void remmina_protocol_widget_chat_open(RemminaProtocolWidget *gp, const gchar *name,
				       void (*on_send)(RemminaProtocolWidget *gp, const gchar *text), void (*on_destroy)(RemminaProtocolWidget *gp))
{
	TRACE_CALL(__func__);
	if (gp->priv->chat_window) {
		gtk_window_present(GTK_WINDOW(gp->priv->chat_window));
	} else {
		gp->priv->chat_window = remmina_chat_window_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gp))), name);
		g_signal_connect_swapped(G_OBJECT(gp->priv->chat_window), "send", G_CALLBACK(on_send), gp);
		g_signal_connect_swapped(G_OBJECT(gp->priv->chat_window), "destroy",
					 G_CALLBACK(remmina_protocol_widget_chat_on_destroy), gp);
		g_signal_connect_swapped(G_OBJECT(gp->priv->chat_window), "destroy", G_CALLBACK(on_destroy), gp);
		gtk_widget_show(gp->priv->chat_window);
	}
}

void remmina_protocol_widget_chat_close(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	if (gp->priv->chat_window)
		gtk_widget_destroy(gp->priv->chat_window);
}

void remmina_protocol_widget_chat_receive(RemminaProtocolWidget *gp, const gchar *text)
{
	TRACE_CALL(__func__);
	/* This function can be called from a non main thread */

	if (gp->priv->chat_window) {
		if (!remmina_masterthread_exec_is_main_thread()) {
			/* Allow the execution of this function from a non main thread */
			RemminaMTExecData *d;
			d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
			d->func = FUNC_CHAT_RECEIVE;
			d->p.chat_receive.gp = gp;
			d->p.chat_receive.text = text;
			remmina_masterthread_exec_and_wait(d);
			g_free(d);
			return;
		}
		remmina_chat_window_receive(REMMINA_CHAT_WINDOW(gp->priv->chat_window), _("Server"), text);
		gtk_window_present(GTK_WINDOW(gp->priv->chat_window));
	}
}

void remmina_protocol_widget_setup(RemminaProtocolWidget *gp, RemminaFile *remminafile, RemminaConnectionObject *cnnobj)
{
	RemminaProtocolPlugin *plugin;

	gp->priv->remmina_file = remminafile;
	gp->cnnobj = cnnobj;

	/* Locate the protocol plugin */
	plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL,
									    remmina_file_get_string(remminafile, "protocol"));

	if (!plugin || !plugin->init || !plugin->open_connection) {
		remmina_protocol_widget_set_error(gp, _("Install the %s protocol plugin first."),
						  remmina_file_get_string(remminafile, "protocol"));
		gp->priv->plugin = NULL;
		return;
	}
	gp->priv->plugin = plugin;

	gp->priv->scalemode = remmina_file_get_int(gp->priv->remmina_file, "scale", FALSE);
	gp->priv->scaler_expand = remmina_file_get_int(gp->priv->remmina_file, "scaler_expand", FALSE);
}

GtkWidget *remmina_protocol_widget_new(void)
{
	return GTK_WIDGET(g_object_new(REMMINA_TYPE_PROTOCOL_WIDGET, NULL));
}

/* Send one or more keystrokes to a specific widget by firing key-press and
 * key-release events.
 * GdkEventType action can be GDK_KEY_PRESS or GDK_KEY_RELEASE or both to
 * press the keys and release them in reversed order. */
void remmina_protocol_widget_send_keys_signals(GtkWidget *widget, const guint *keyvals, int keyvals_length, GdkEventType action)
{
	TRACE_CALL(__func__);
	int i;
	GdkEventKey event;
	gboolean result;
	GdkKeymap *keymap = gdk_keymap_get_for_display(gdk_display_get_default());

	event.window = gtk_widget_get_window(widget);
	event.send_event = TRUE;
	event.time = GDK_CURRENT_TIME;
	event.state = 0;
	event.length = 0;
	event.string = "";
	event.group = 0;

	if (action & GDK_KEY_PRESS) {
		/* Press the requested buttons */
		event.type = GDK_KEY_PRESS;
		for (i = 0; i < keyvals_length; i++) {
			event.keyval = keyvals[i];
			event.hardware_keycode = remmina_public_get_keycode_for_keyval(keymap, event.keyval);
			event.is_modifier = (int)remmina_public_get_modifier_for_keycode(keymap, event.hardware_keycode);
			g_signal_emit_by_name(G_OBJECT(widget), "key-press-event", &event, &result);
		}
	}

	if (action & GDK_KEY_RELEASE) {
		/* Release the requested buttons in reverse order */
		event.type = GDK_KEY_RELEASE;
		for (i = (keyvals_length - 1); i >= 0; i--) {
			event.keyval = keyvals[i];
			event.hardware_keycode = remmina_public_get_keycode_for_keyval(keymap, event.keyval);
			event.is_modifier = (int)remmina_public_get_modifier_for_keycode(keymap, event.hardware_keycode);
			g_signal_emit_by_name(G_OBJECT(widget), "key-release-event", &event, &result);
		}
	}
}

void remmina_protocol_widget_update_remote_resolution(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	GdkRectangle rect;
	gint w, h;
	gint wfile, hfile;
	RemminaProtocolWidgetResolutionMode res_mode;
	RemminaScaleMode scalemode;

	rco_get_monitor_geometry(gp->cnnobj, &rect);

	/* Integrity check: check that we have a cnnwin visible and get t  */

	res_mode = remmina_file_get_int(gp->priv->remmina_file, "resolution_mode", RES_INVALID);
	scalemode = remmina_file_get_int(gp->priv->remmina_file, "scale", REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE);
	wfile = remmina_file_get_int(gp->priv->remmina_file, "resolution_width", -1);
	hfile = remmina_file_get_int(gp->priv->remmina_file, "resolution_height", -1);

	/* If resolution_mode is non-existent (-1), then we try to calculate it
	 * as we did before having resolution_mode */
	if (res_mode == RES_INVALID) {
		if (wfile <= 0 || hfile <= 0)
			res_mode = RES_USE_INITIAL_WINDOW_SIZE;
		else
			res_mode = RES_USE_CUSTOM;
	}

	if (res_mode == RES_USE_INITIAL_WINDOW_SIZE || scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES) {
		/* Use internal window size as remote desktop size */
		GtkAllocation al;
		gtk_widget_get_allocation(GTK_WIDGET(gp), &al);
		w = al.width;
		h = al.height;
		if (w < 10) {
			printf("Remmina warning: %s RemminaProtocolWidget w=%d h=%d are too small, adjusting to 640x480\n", __func__, w, h);
			w = 640;
			h = 480;
		}
		/* Due to approximations while GTK calculates scaling, (w x h) may exceed our monitor geometry
		 * Adjust to fit. */
		if (w > rect.width)
			w = rect.width;
		if (h > rect.height)
			h = rect.height;
	} else if (res_mode == RES_USE_CLIENT) {
		w = rect.width;
		h = rect.height;
	} else {
		w = wfile;
		h = hfile;
	}
	gp->priv->profile_remote_width = w;
	gp->priv->profile_remote_height = h;
}
