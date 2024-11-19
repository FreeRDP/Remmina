/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
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
#include <gmodule.h>
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
	gboolean		user_disconnect;
	/* ssh_tunnels is an array of RemminaSSHTunnel*
	 * the 1st one is the "main" tunnel, other tunnels are used for example in sftp commands */
	GPtrArray *		ssh_tunnels;
	RemminaTunnelInitFunc	init_func;

	GtkWidget *		chat_window;

	gboolean		closed;

	RemminaHostkeyFunc	hostkey_func;

	gint			profile_remote_width;
	gint			profile_remote_height;
	gint			multimon;

	RemminaMessagePanel *	connect_message_panel;
	RemminaMessagePanel *	listen_message_panel;
	RemminaMessagePanel *	auth_message_panel;
	RemminaMessagePanel *	retry_message_panel;

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
	RPWDT_AUTHX509,
	RPWDT_ACCEPT
};

G_DEFINE_TYPE(RemminaProtocolWidget, remmina_protocol_widget, GTK_TYPE_EVENT_BOX)

enum {
	CONNECT_SIGNAL,
	DISCONNECT_SIGNAL,
	DESKTOP_RESIZE_SIGNAL,
	UPDATE_ALIGN_SIGNAL,
	LOCK_DYNRES_SIGNAL,
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
	remmina_protocol_widget_signals[LOCK_DYNRES_SIGNAL] = g_signal_new("lock-dynres", G_TYPE_FROM_CLASS(klass),
									   G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, lock_dynres), NULL, NULL,
									   g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_protocol_widget_signals[UNLOCK_DYNRES_SIGNAL] = g_signal_new("unlock-dynres", G_TYPE_FROM_CLASS(klass),
									     G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, unlock_dynres), NULL, NULL,
									     g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}


static void remmina_protocol_widget_close_all_tunnels(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	int i;

	if (gp->priv && gp->priv->ssh_tunnels) {
		for (i = 0; i < gp->priv->ssh_tunnels->len; i++) {
#ifdef HAVE_LIBSSH
			remmina_ssh_tunnel_free((RemminaSSHTunnel *)gp->priv->ssh_tunnels->pdata[i]);
#else
			REMMINA_DEBUG("LibSSH support turned off, no need to free SSH tunnel data");
#endif
		}
		g_ptr_array_set_size(gp->priv->ssh_tunnels, 0);
	}
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

	remmina_protocol_widget_close_all_tunnels(gp);

	if (gp->priv && gp->priv->ssh_tunnels) {
		g_ptr_array_free(gp->priv->ssh_tunnels, TRUE);
		gp->priv->ssh_tunnels = NULL;
	}
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
	gp->priv->user_disconnect = FALSE;
	gp->priv->closed = TRUE;
	gp->priv->ssh_tunnels = g_ptr_array_new();

	g_signal_connect(G_OBJECT(gp), "destroy", G_CALLBACK(remmina_protocol_widget_destroy), NULL);
}

void remmina_protocol_widget_open_connection_real(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET(data);

	REMMINA_DEBUG("Opening connection");

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
	if (remmina_file_get_int(gp->priv->remmina_file, "ssh_tunnel_enabled", FALSE))
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
		REMMINA_DEBUG("Have SSH");
		if (num_ssh) {
			feature->type = REMMINA_PROTOCOL_FEATURE_TYPE_TOOL;
			feature->id = REMMINA_PROTOCOL_FEATURE_TOOL_SSH;
			feature->opt1 = _("Connect via SSH from a new terminal");
			feature->opt1_type_hint = REMMINA_TYPEHINT_STRING;
			feature->opt2 = "utilities-terminal";
			feature->opt2_type_hint = REMMINA_TYPEHINT_STRING;
			feature->opt3 = NULL;
			feature->opt3_type_hint = REMMINA_TYPEHINT_UNDEFINED;
			feature++;

			feature->type = REMMINA_PROTOCOL_FEATURE_TYPE_TOOL;
			feature->id = REMMINA_PROTOCOL_FEATURE_TOOL_SFTP;
			feature->opt1 = _("Open SFTP transfer…");
			feature->opt1_type_hint = REMMINA_TYPEHINT_STRING;
			feature->opt2 = "folder-remote";
			feature->opt2_type_hint = REMMINA_TYPEHINT_STRING;
			feature->opt3 = NULL;
			feature->opt3_type_hint = REMMINA_TYPEHINT_UNDEFINED;
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
	// TRANSLATORS: “%s” is a placeholder for the connection profile name
	s = g_strdup_printf(_("Connecting to “%s”…"), (name ? name : "*"));

	mp = remmina_message_panel_new();
	remmina_message_panel_setup_progress(mp, s, cancel_open_connection_cb, gp);
	g_free(s);
	gp->priv->connect_message_panel = mp;
	rco_show_message_panel(gp->cnnobj, mp);

	remmina_protocol_widget_open_connection_real(gp);
}

static gboolean conn_closed_real(gpointer data, int button){
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;

	#ifdef HAVE_LIBSSH
	/* This will close all tunnels */
	remmina_protocol_widget_close_all_tunnels(gp);
#endif
	/* Exec postcommand */
	remmina_ext_exec_new(gp->priv->remmina_file, "postcommand");
	/* Notify listeners (usually rcw) that the connection is closed */
	g_signal_emit_by_name(G_OBJECT(gp), "disconnect");
	return G_SOURCE_REMOVE;

}

static gboolean conn_closed(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;
	int disconnect_prompt = remmina_file_get_int(gp->priv->remmina_file, "disconnect-prompt", FALSE);
	if (!gp->priv->user_disconnect && !gp->priv->has_error && disconnect_prompt){
		const char* msg = "Plugin Disconnected";
		if (gp->priv->has_error){
			msg = remmina_protocol_widget_get_error_message(gp);
			remmina_protocol_widget_set_error(gp, NULL);
		}
		gp->priv->user_disconnect = FALSE;
		RemminaMessagePanel* mp = remmina_message_panel_new();
		remmina_message_panel_setup_message(mp, msg, (RemminaMessagePanelCallback)conn_closed_real, gp);
		rco_show_message_panel(gp->cnnobj, mp);
		return G_SOURCE_REMOVE;
	}
	else{
		return conn_closed_real(gp, 0);
	}

}

void remmina_protocol_widget_signal_connection_closed(RemminaProtocolWidget *gp)
{
	/* User told us that they closed the connection,
	 * or the connection was closed with a known error,
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
	if (gp->priv->ssh_tunnels) {
		for (guint i = 0; i < gp->priv->ssh_tunnels->len; i++)
			remmina_ssh_tunnel_cancel_accept((RemminaSSHTunnel *)gp->priv->ssh_tunnels->pdata[i]);
	}
#endif
	if (gp->priv->listen_message_panel) {
		rco_destroy_message_panel(gp->cnnobj, gp->priv->listen_message_panel);
		gp->priv->listen_message_panel = NULL;
	}
	if (gp->priv->connect_message_panel) {
		rco_destroy_message_panel(gp->cnnobj, gp->priv->connect_message_panel);
		gp->priv->connect_message_panel = NULL;
	}
	if (gp->priv->retry_message_panel) {
		rco_destroy_message_panel(gp->cnnobj, gp->priv->retry_message_panel);
		gp->priv->retry_message_panel = NULL;
	}
	g_signal_emit_by_name(G_OBJECT(gp), "connect");
	return G_SOURCE_REMOVE;
}

void remmina_protocol_widget_signal_connection_opened(RemminaProtocolWidget *gp)
{
	/* Plugin told us that it opened the connection,
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

static gboolean lock_dynres(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;

	g_signal_emit_by_name(G_OBJECT(gp), "lock-dynres");
	return G_SOURCE_REMOVE;
}

static gboolean unlock_dynres(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;

	g_signal_emit_by_name(G_OBJECT(gp), "unlock-dynres");
	return G_SOURCE_REMOVE;
}

void remmina_protocol_widget_lock_dynres(RemminaProtocolWidget *gp)
{
	/* Called by the plugin to do updates on rcw */
	TRACE_CALL(__func__);
	g_idle_add(lock_dynres, (gpointer)gp);
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
		/* Clear the current error, or "disconnect" signal func will reshow a panel */
		remmina_protocol_widget_set_error(gp, NULL);
		g_signal_emit_by_name(G_OBJECT(gp), "disconnect");
		return;
	}
	gp->priv->user_disconnect = TRUE;

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
			REMMINA_DEBUG("Keystrokes before replacement is \'%s\'", keystrokes);
			keystrokes = g_strdup(remmina_public_str_replace_in_place(keystrokes,
										  keystrokes_replaces[i].search,
										  keystrokes_replaces[i].replace));
			REMMINA_DEBUG("Keystrokes after replacement is \'%s\'", keystrokes);
		}
		gchar *iter = g_strdup(keystrokes);
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

/**
 * Send to the plugin some keystrokes from the content of the clipboard
 * This is a copy of remmina_protocol_widget_send_keystrokes but it uses the clipboard content
 * get from remmina_protocol_widget_send_clipboard
 * Probably we don't need the replacement table.
 */
void remmina_protocol_widget_send_clip_strokes(GtkClipboard *clipboard, const gchar *clip_text, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET(data);
	if (clip_text == NULL){
		return;
	}
	gchar *text = g_utf8_normalize(clip_text, -1, G_NORMALIZE_DEFAULT_COMPOSE);
	guint *keyvals;
	gint i;
	GdkKeymap *keymap = gdk_keymap_get_for_display(gdk_display_get_default());
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
	KeystrokeReplace text_replaces[] =
	{
		{ "\\n",  "\n", GDK_KEY_Return	  },
		{ "\\t",  "\t", GDK_KEY_Tab	  },
		{ "\\b",  "\b", GDK_KEY_BackSpace },
		{ "\\e",  "\e", GDK_KEY_Escape	  },
		{ "\\\\", "\\", GDK_KEY_backslash },
		{ NULL,	  NULL, 0		  }
	};

	if (remmina_protocol_widget_plugin_receives_keystrokes(gp)) {
		if (text) {
			/* Replace special characters */
			for (i = 0; text_replaces[i].replace; i++) {
				REMMINA_DEBUG("Text clipboard before replacement is \'%s\'", text);
				text = g_strdup(remmina_public_str_replace_in_place(text,
										    text_replaces[i].search,
										    text_replaces[i].replace));
				REMMINA_DEBUG("Text clipboard after replacement is \'%s\'", text);
			}
			gchar *iter = g_strdup(text);
			REMMINA_DEBUG("Iter: %s", iter),
			keyvals = (guint *)g_malloc(strlen(text));
			while (TRUE) {
				/* Process each character in the keystrokes */
				character = g_utf8_get_char_validated(iter, -1);
				REMMINA_DEBUG("Char: U+%04" G_GINT32_FORMAT"X", character);
				if (character == 0)
					break;
				keyval = gdk_unicode_to_keyval(character);
				REMMINA_DEBUG("Keyval: %u", keyval);
				/* Replace all the special character with its keyval */
				for (i = 0; text_replaces[i].replace; i++) {
					if (character == text_replaces[i].replace[0]) {
						keys = g_new0(GdkKeymapKey, 1);
						keyval = text_replaces[i].keyval;
						REMMINA_DEBUG("Special Keyval: %u", keyval);
						/* A special character was generated, no keyval lookup needed */
						character = 0;
						break;
					}
				}
				/* Decode character if it’s not a special character */
				if (character) {
					/* get keyval without modifications */
					if (!gdk_keymap_get_entries_for_keyval(keymap, keyval, &keys, &n_keys)) {
						REMMINA_WARNING("keyval 0x%04x has no keycode!", keyval);
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
				/*
				 * @fixme heap buffer overflow
				 * In some cases, for example sending \t as the only sequence
				 * may lead to a buffer overflow
				 */
				keyvals[n_keys++] = keyval;
				/* Send keystroke to the plugin */
				gp->priv->plugin->send_keystrokes(gp, keyvals, n_keys);
				g_free(keys);
				/* Process next character in the keystrokes */
				iter = g_utf8_find_next_char(iter, NULL);
			}
			g_free(keyvals);
		}
		g_free(text);
	}
	return;
}

void remmina_protocol_widget_send_clipboard(RemminaProtocolWidget *gp, GObject*widget)
{
	TRACE_CALL(__func__);
	GtkClipboard *clipboard;

	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);

	/* Request the contents of the clipboard, contents_received will be
	 * called when we do get the contents.
	 */
	gtk_clipboard_request_text(clipboard,
				   remmina_protocol_widget_send_clip_strokes, gp);
}

gboolean remmina_protocol_widget_plugin_screenshot(RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd)
{
	TRACE_CALL(__func__);
	if (!gp->priv->plugin->get_plugin_screenshot) {
		REMMINA_DEBUG("plugin screenshot function is not implemented, using core Remmina functionality");
		return FALSE;
	}

	return gp->priv->plugin->get_plugin_screenshot(gp, rpsd);
}

gboolean remmina_protocol_widget_map_event(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	if (!gp->priv->plugin->map_event) {
		REMMINA_DEBUG("Map plugin function not implemented");
		return FALSE;
	}

	REMMINA_DEBUG("Calling plugin mapping function");
	return gp->priv->plugin->map_event(gp);
}

gboolean remmina_protocol_widget_unmap_event(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	if (!gp->priv->plugin->unmap_event) {
		REMMINA_DEBUG("Unmap plugin function not implemented");
		return FALSE;
	}

	REMMINA_DEBUG("Calling plugin unmapping function");
	return gp->priv->plugin->unmap_event(gp);
}

void remmina_protocol_widget_emit_signal(RemminaProtocolWidget *gp, const gchar *signal_name)
{
	TRACE_CALL(__func__);

	REMMINA_DEBUG("Emitting signals should be used from the object itself, not from another object");
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
	    remmina_file_get_int(gp->priv->remmina_file, "ssh_tunnel_enabled", FALSE))
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
		if (gp->priv->ssh_tunnels && gp->priv->ssh_tunnels->len > 0) {
			rcw_open_from_file_full(
				remmina_file_dup_temp_protocol(gp->priv->remmina_file, "SSH"), NULL,
				(RemminaSSHTunnel *)gp->priv->ssh_tunnels->pdata[0], NULL);
			return;
		}
		break;

	case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
		if (gp->priv->ssh_tunnels && gp->priv->ssh_tunnels->len > 0) {
			rcw_open_from_file_full(
				remmina_file_dup_temp_protocol(gp->priv->remmina_file, "SFTP"), NULL,
				gp->priv->ssh_tunnels->pdata[0], NULL);
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

static RemminaSSHTunnel *remmina_protocol_widget_init_tunnel(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel;
	gint ret;
	gchar *msg;
	RemminaMessagePanel *mp;
	gboolean partial = FALSE;
	gboolean cont = FALSE;

	tunnel = remmina_ssh_tunnel_new_from_file(gp->priv->remmina_file);

	REMMINA_DEBUG("Creating SSH tunnel to “%s” via SSH…", REMMINA_SSH(tunnel)->server);
	// TRANSLATORS: “%s” is a placeholder for an hostname or an IP address.
	msg = g_strdup_printf(_("Connecting to “%s” via SSH…"), REMMINA_SSH(tunnel)->server);

	mp = remmina_protocol_widget_mpprogress(gp->cnnobj, msg, cancel_init_tunnel_cb, NULL);
	g_free(msg);



	while (1) {
		if (!partial) {
			if (!remmina_ssh_init_session(REMMINA_SSH(tunnel))) {
				REMMINA_DEBUG("SSH Tunnel init session error: %s", REMMINA_SSH(tunnel)->error);
				remmina_protocol_widget_set_error(gp, REMMINA_SSH(tunnel)->error);
				// exit the loop here: OK
				break;
			}
		}

		ret = remmina_ssh_auth_gui(REMMINA_SSH(tunnel), gp, gp->priv->remmina_file);
		REMMINA_DEBUG("Tunnel auth returned %d", ret);
		switch (ret) {
		case REMMINA_SSH_AUTH_SUCCESS:
			REMMINA_DEBUG("Authentication success");
			break;
		case REMMINA_SSH_AUTH_PARTIAL:
			REMMINA_DEBUG("Continue with the next auth method");
			partial = TRUE;
			// Continue the loop: OK
			continue;
			break;
		case REMMINA_SSH_AUTH_RECONNECT:
			REMMINA_DEBUG("Reconnecting…");
			if (REMMINA_SSH(tunnel)->session) {
				ssh_disconnect(REMMINA_SSH(tunnel)->session);
				ssh_free(REMMINA_SSH(tunnel)->session);
				REMMINA_SSH(tunnel)->session = NULL;
			}
			g_free(REMMINA_SSH(tunnel)->callback);
			// Continue the loop: OK
			continue;
			break;
		case REMMINA_SSH_AUTH_USERCANCEL:
			REMMINA_DEBUG("Interrupted by the user");
			// exit the loop here: OK
			goto BREAK;
			break;
		default:
			REMMINA_DEBUG("Error during the authentication: %s", REMMINA_SSH(tunnel)->error);
			remmina_protocol_widget_set_error(gp, REMMINA_SSH(tunnel)->error);
			// exit the loop here: OK
			goto BREAK;
		}


		cont = TRUE;
		break;
	}

#if 0

	if (!remmina_ssh_init_session(REMMINA_SSH(tunnel))) {
		REMMINA_DEBUG("Cannot init SSH session with tunnel struct");
		remmina_protocol_widget_set_error(gp, REMMINA_SSH(tunnel)->error);
		remmina_ssh_tunnel_free(tunnel);
		return NULL;
	}

	ret = remmina_ssh_auth_gui(REMMINA_SSH(tunnel), gp, gp->priv->remmina_file);
	REMMINA_DEBUG("Tunnel auth returned %d", ret);
	if (ret != REMMINA_SSH_AUTH_SUCCESS) {
		if (ret != REMMINA_SSH_AUTH_USERCANCEL)
			remmina_protocol_widget_set_error(gp, REMMINA_SSH(tunnel)->error);
		remmina_ssh_tunnel_free(tunnel);
		return NULL;
	}

#endif

BREAK:
	if (!cont) {
		remmina_ssh_tunnel_free(tunnel);
		return NULL;
	}
	remmina_protocol_widget_mpdestroy(gp->cnnobj, mp);

	return tunnel;
}
#endif


#ifdef HAVE_LIBSSH
static void cancel_start_direct_tunnel_cb(void *cbdata, int btn)
{
	printf("Remmina: Cancelling start_direct_tunnel is not implemented\n");
}

static gboolean remmina_protocol_widget_tunnel_destroy(RemminaSSHTunnel *tunnel, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET(data);
	guint idx = 0;

#if GLIB_CHECK_VERSION(2, 54, 0)
	gboolean found = g_ptr_array_find(gp->priv->ssh_tunnels, tunnel, &idx);
#else
	int i;
	gboolean found = FALSE;
	for (i = 0; i < gp->priv->ssh_tunnels->len; i++) {
		if ((RemminaSSHTunnel *)gp->priv->ssh_tunnels->pdata[i] == tunnel) {
			found = TRUE;
			idx = i;
		}
	}
#endif

	printf("Tunnel %s found at idx = %d\n", found ? "yes": "not", idx);

	if (found) {
#ifdef HAVE_LIBSSH
		REMMINA_DEBUG("[Tunnel with idx %u has been disconnected", idx);
		remmina_ssh_tunnel_free(tunnel);
#endif
		g_ptr_array_remove(gp->priv->ssh_tunnels, tunnel);
	}
	return TRUE;
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
	const gchar *ssh_tunnel_server;
	gchar *ssh_tunnel_host, *srv_host, *dest;
	gint srv_port, ssh_tunnel_port = 0;

	REMMINA_DEBUG("SSH tunnel initialization…");

	server = remmina_file_get_string(gp->priv->remmina_file, "server");
	ssh_tunnel_server = remmina_file_get_string(gp->priv->remmina_file, "ssh_tunnel_server");

	if (!server)
		return g_strdup("");

	if (strstr(g_strdup(server), "unix:///") != NULL) {
		REMMINA_DEBUG("%s is a UNIX socket", server);
		return g_strdup(server);
	}

	REMMINA_DEBUG("Calling remmina_public_get_server_port");
	remmina_public_get_server_port(server, default_port, &srv_host, &srv_port);
	REMMINA_DEBUG("Calling remmina_public_get_server_port (tunnel)");
	remmina_public_get_server_port(ssh_tunnel_server, 22, &ssh_tunnel_host, &ssh_tunnel_port);
	REMMINA_DEBUG("server: %s, port: %d", srv_host, srv_port);

	if (port_plus && srv_port < 100)
		/* Protocols like VNC supports using instance number :0, :1, etc. as port number. */
		srv_port += default_port;

#ifdef HAVE_LIBSSH
	gchar *msg;
	RemminaMessagePanel *mp;
	RemminaSSHTunnel *tunnel;

	if (!remmina_file_get_int(gp->priv->remmina_file, "ssh_tunnel_enabled", FALSE)) {
		dest = g_strdup_printf("[%s]:%i", srv_host, srv_port);
		g_free(srv_host);
		g_free(ssh_tunnel_host);
		return dest;
	}

	tunnel = remmina_protocol_widget_init_tunnel(gp);
	if (!tunnel) {
		g_free(srv_host);
		g_free(ssh_tunnel_host);
		REMMINA_DEBUG("remmina_protocol_widget_init_tunnel failed with error is %s",
			      remmina_protocol_widget_get_error_message(gp));
		return NULL;
	}

	// TRANSLATORS: “%s” is a placeholder for an hostname or an IP address.
	msg = g_strdup_printf(_("Connecting to “%s” via SSH…"), server);
	mp = remmina_protocol_widget_mpprogress(gp->cnnobj, msg, cancel_start_direct_tunnel_cb, NULL);
	g_free(msg);

	if (remmina_file_get_int(gp->priv->remmina_file, "ssh_tunnel_loopback", FALSE)) {
		g_free(srv_host);
		g_free(ssh_tunnel_host);
		ssh_tunnel_host = NULL;
		srv_host = g_strdup("127.0.0.1");
	}

	REMMINA_DEBUG("Starting tunnel to: %s, port: %d", ssh_tunnel_host, ssh_tunnel_port);
	if (!remmina_ssh_tunnel_open(tunnel, srv_host, srv_port, remmina_pref.sshtunnel_port)) {
		g_free(srv_host);
		g_free(ssh_tunnel_host);
		remmina_protocol_widget_set_error(gp, REMMINA_SSH(tunnel)->error);
		remmina_ssh_tunnel_free(tunnel);
		return NULL;
	}
	g_free(srv_host);
	g_free(ssh_tunnel_host);

	remmina_protocol_widget_mpdestroy(gp->cnnobj, mp);

	tunnel->destroy_func = remmina_protocol_widget_tunnel_destroy;
	tunnel->destroy_func_callback_data = (gpointer)gp;

	g_ptr_array_add(gp->priv->ssh_tunnels, tunnel);


	//try startup command
	ssh_channel channel;
	int rc;
	const gchar* tunnel_command = remmina_file_get_string(gp->priv->remmina_file, "ssh_tunnel_command");
	if (tunnel_command != NULL){
		channel = ssh_channel_new(REMMINA_SSH(tunnel)->session);
		if (channel == NULL) return g_strdup_printf("127.0.0.1:%i", remmina_pref.sshtunnel_port);

		rc = ssh_channel_open_session(channel);
		if (rc != SSH_OK)
		{
			ssh_channel_free(channel);
			return g_strdup_printf("127.0.0.1:%i", remmina_pref.sshtunnel_port);
		}
		rc = ssh_channel_request_exec(channel, tunnel_command);
		if (rc != SSH_OK)
		{
			ssh_channel_close(channel);
			ssh_channel_free(channel);
			return g_strdup_printf("127.0.0.1:%i", remmina_pref.sshtunnel_port);
		}
		struct timeval timeout = {10, 0};
		ssh_channel channels[2];
		channels[0] = channel;
		channels[1] = NULL;
		rc = ssh_channel_select(channels, NULL, NULL, &timeout);
		if (rc == SSH_OK){
			char buffer[256];
			ssh_channel_read(channel, buffer, sizeof(buffer), 0);
		}
		
		REMMINA_DEBUG("Ran startup command");
		ssh_channel_close(channel);
		ssh_channel_free(channel);
	}


	return g_strdup_printf("127.0.0.1:%i", remmina_pref.sshtunnel_port);

#else

	dest = g_strdup_printf("[%s]:%i", srv_host, srv_port);
	g_free(srv_host);
	g_free(ssh_tunnel_host);
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
	RemminaSSHTunnel *tunnel;

	if (!remmina_file_get_int(gp->priv->remmina_file, "ssh_tunnel_enabled", FALSE))
		return TRUE;

	if (!(tunnel = remmina_protocol_widget_init_tunnel(gp)))
		return FALSE;

	// TRANSLATORS: “%i” is a placeholder for a TCP port number.
	msg = g_strdup_printf(_("Awaiting incoming SSH connection on port %i…"), remmina_file_get_int(gp->priv->remmina_file, "listenport", 0));
	mp = remmina_protocol_widget_mpprogress(gp->cnnobj, msg, cancel_start_reverse_tunnel_cb, NULL);
	g_free(msg);

	if (!remmina_ssh_tunnel_reverse(tunnel, remmina_file_get_int(gp->priv->remmina_file, "listenport", 0), local_port)) {
		remmina_ssh_tunnel_free(tunnel);
		remmina_protocol_widget_set_error(gp, REMMINA_SSH(tunnel)->error);
		return FALSE;
	}
	remmina_protocol_widget_mpdestroy(gp->cnnobj, mp);
	g_ptr_array_add(gp->priv->ssh_tunnels, tunnel);
#endif

	return TRUE;
}

gboolean remmina_protocol_widget_ssh_exec(RemminaProtocolWidget *gp, gboolean wait, const gchar *fmt, ...)
{
	TRACE_CALL(__func__);
#ifdef HAVE_LIBSSH
	RemminaSSHTunnel *tunnel;
	ssh_channel channel;
	gint status;
	gboolean ret = FALSE;
	gchar *cmd, *ptr;
	va_list args;

	if (gp->priv->ssh_tunnels->len < 1)
		return FALSE;

	tunnel = (RemminaSSHTunnel *)gp->priv->ssh_tunnels->pdata[0];

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
				// TRANSLATORS: “%s” is a place holder for a unix command path.
				remmina_ssh_set_application_error(REMMINA_SSH(tunnel),
								  _("The “%s” command is not available on the SSH server."), cmd);
				break;
			default:
				// TRANSLATORS: “%s” is a place holder for a unix command path. “%i” is a placeholder for an error code number.
				remmina_ssh_set_application_error(REMMINA_SSH(tunnel),
								  _("Could not run the “%s” command on the SSH server (status = %i)."), cmd, status);
				break;
			}
		} else {
			ret = TRUE;
		}
	} else {
		// TRANSLATORS: %s is a placeholder for an error message
		remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not run command. %s"));
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
static gboolean remmina_protocol_widget_xport_tunnel_init_callback(RemminaSSHTunnel *tunnel, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET(data);
	gchar *server;
	gint port;
	gboolean ret;

	REMMINA_DEBUG("Calling remmina_public_get_server_port");
	remmina_public_get_server_port(remmina_file_get_string(gp->priv->remmina_file, "server"), 177, &server, &port);
	ret = ((RemminaXPortTunnelInitFunc)gp->priv->init_func)(gp,
								tunnel->remotedisplay, (tunnel->bindlocalhost ? "localhost" : server), port);
	g_free(server);

	return ret;
}

static gboolean remmina_protocol_widget_xport_tunnel_connect_callback(RemminaSSHTunnel *tunnel, gpointer data)
{
	TRACE_CALL(__func__);
	return TRUE;
}

static gboolean remmina_protocol_widget_xport_tunnel_disconnect_callback(RemminaSSHTunnel *tunnel, gpointer data)
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
	RemminaSSHTunnel *tunnel;

	if (!(tunnel = remmina_protocol_widget_init_tunnel(gp))) return FALSE;

	// TRANSLATORS: “%s” is a placeholder for a hostname or IP address.
	msg = g_strdup_printf(_("Connecting to %s via SSH…"), remmina_file_get_string(gp->priv->remmina_file, "server"));
	mp = remmina_protocol_widget_mpprogress(gp->cnnobj, msg, cancel_connect_xport_cb, NULL);
	g_free(msg);

	gp->priv->init_func = init_func;
	tunnel->init_func = remmina_protocol_widget_xport_tunnel_init_callback;
	tunnel->connect_func = remmina_protocol_widget_xport_tunnel_connect_callback;
	tunnel->disconnect_func = remmina_protocol_widget_xport_tunnel_disconnect_callback;
	tunnel->callback_data = gp;

	REMMINA_DEBUG("Calling remmina_public_get_server_port");
	remmina_public_get_server_port(remmina_file_get_string(gp->priv->remmina_file, "server"), 0, &server, NULL);
	bindlocalhost = (g_strcmp0(REMMINA_SSH(tunnel)->server, server) == 0);
	g_free(server);

	if (!remmina_ssh_tunnel_xport(tunnel, bindlocalhost)) {
		remmina_protocol_widget_set_error(gp, "Could not open channel, %s",
						  ssh_get_error(REMMINA_SSH(tunnel)->session));
		remmina_ssh_tunnel_free(tunnel);
		return FALSE;
	}

	remmina_protocol_widget_mpdestroy(gp->cnnobj, mp);
	g_ptr_array_add(gp->priv->ssh_tunnels, tunnel);

	return TRUE;

#else
	return FALSE;
#endif
}

void remmina_protocol_widget_set_display(RemminaProtocolWidget *gp, gint display)
{
	TRACE_CALL(__func__);
#ifdef HAVE_LIBSSH
	RemminaSSHTunnel *tunnel;
	if (gp->priv->ssh_tunnels->len < 1)
		return;
	tunnel = (RemminaSSHTunnel *)gp->priv->ssh_tunnels->pdata[0];
	if (tunnel->localdisplay) g_free(tunnel->localdisplay);
	tunnel->localdisplay = g_strdup_printf("unix:%i", display);
#endif
}

gint remmina_protocol_widget_get_profile_remote_width(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	/* Returns the width of remote desktop as chosen by the user profile */
	return gp->priv->profile_remote_width;
}

gint remmina_protocol_widget_get_multimon(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	/* Returns ehenever multi monitor is enabled (1) */
	gp->priv->multimon = remmina_file_get_int(gp->priv->remmina_file, "multimon", -1);
	REMMINA_DEBUG("Multi monitor is set to %d", gp->priv->multimon);
	return gp->priv->multimon;
}

gint remmina_protocol_widget_get_profile_remote_height(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	/* Returns the height of remote desktop as chosen by the user profile */
	return gp->priv->profile_remote_height;
}

const gchar* remmina_protocol_widget_get_name(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	return gp ? gp->plugin ? gp->plugin->name : NULL : NULL;
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

	if (d->gp->cnnobj == NULL)
		return FALSE;

	mp = remmina_message_panel_new();

	if (d->dtype == RPWDT_AUTH) {
		if (d->pflags & REMMINA_MESSAGE_PANEL_FLAG_USERNAME) {
			remmina_message_panel_field_set_string(mp, REMMINA_MESSAGE_PANEL_USERNAME, d->default_username);
		}
		remmina_message_panel_setup_auth(mp, authpanel_mt_cb, d, d->title, d->strpasswordlabel, d->pflags);
		remmina_message_panel_field_set_string(mp, REMMINA_MESSAGE_PANEL_USERNAME, d->default_username);
		if (d->pflags & REMMINA_MESSAGE_PANEL_FLAG_DOMAIN)
			remmina_message_panel_field_set_string(mp, REMMINA_MESSAGE_PANEL_DOMAIN, d->default_domain);
		remmina_message_panel_field_set_string(mp, REMMINA_MESSAGE_PANEL_PASSWORD, d->default_password);
		if (d->pflags & REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD)
			remmina_message_panel_field_set_switch(mp, REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD, (d->default_password == NULL || d->default_password[0] == 0) ? FALSE: TRUE);
	} else if (d->dtype == RPWDT_QUESTIONYESNO) {
		remmina_message_panel_setup_question(mp, d->title, authpanel_mt_cb, d, FALSE);
	} else if (d->dtype == RPWDT_ACCEPT) {
		remmina_message_panel_setup_question(mp, d->title, authpanel_mt_cb, d, TRUE);
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
		pthread_cond_init(&d->pt_cond, NULL);
		pthread_mutex_init(&d->pt_mutex, NULL);
		g_idle_add(remmina_protocol_widget_dialog_mt_setup, d);
		pthread_mutex_lock(&d->pt_mutex);
		pthread_cond_wait(&d->pt_cond, &d->pt_mutex);
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

gint remmina_protocol_widget_panel_question_accept(RemminaProtocolWidget *gp, const char *msg)
{
	return remmina_protocol_widget_dialog(RPWDT_ACCEPT, gp, 0, msg, NULL, NULL, NULL, NULL);
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

	username = remmina_file_get_string(remminafile, "ssh_tunnel_username");
	password = remmina_file_get_string(remminafile, "ssh_tunnel_password");

	return remmina_protocol_widget_dialog(RPWDT_AUTH, gp, pflags, _("Type in SSH username and password."), username,
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

	if (remmina_pref_get_boolean("trust_all")) {
		/* For compatibility with plugin API: The plugin expects GTK_RESPONSE_OK when user confirms new cert */
		remmina_public_send_notification("remmina-security-trust-all-id", _("Fingerprint automatically accepted"), fingerprint);
		rc = GTK_RESPONSE_OK;
		return rc;
	}
	// For markup see https://developer.gnome.org/pygtk/stable/pango-markup-language.html
	s = g_strdup_printf(
		"<big>%s</big>\n\n%s %s\n%s %s\n%s %s\n\n<big>%s</big>",
		// TRANSLATORS: The user is asked to verify a new SSL certificate.
		_("Certificate details:"),
		// TRANSLATORS: An SSL certificate subject is usually the remote server the user connect to.
		_("Subject:"), subject,
		// TRANSLATORS: The name or email of the entity that have issued the SSL certificate
		_("Issuer:"), issuer,
		// TRANSLATORS: An SSL certificate fingerprint, is a hash of a certificate calculated on all certificate's data and its signature.
		_("Fingerprint:"), fingerprint,
		// TRANSLATORS: The user is asked to accept or refuse a new SSL certificate.
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

	if (remmina_pref_get_boolean("trust_all")) {
		/* For compatibility with plugin API: The plugin expects GTK_RESPONSE_OK when user confirms new cert */
		remmina_public_send_notification("remmina-security-trust-all-id", _("Fingerprint automatically accepted"), new_fingerprint);
		rc = GTK_RESPONSE_OK;
		return rc;
	}
	// For markup see https://developer.gnome.org/pygtk/stable/pango-markup-language.html
	s = g_strdup_printf(
		"<big>%s</big>\n\n%s %s\n%s %s\n%s %s\n%s %s\n\n<big>%s</big>",
		// TRANSLATORS: The user is asked to verify a new SSL certificate.
		_("The certificate changed! Details:"),
		// TRANSLATORS: An SSL certificate subject is usually the remote server the user connect to.
		_("Subject:"), subject,
		// TRANSLATORS: The name or email of the entity that have issued the SSL certificate
		_("Issuer:"), issuer,
		// TRANSLATORS: An SSL certificate fingerprint, is a hash of a certificate calculated on all certificate's data and its signature.
		_("Old fingerprint:"), old_fingerprint,
		// TRANSLATORS: An SSL certificate fingerprint, is a hash of a certificate calculated on all certificate's data and its signature.
		_("New fingerprint:"), new_fingerprint,
		// TRANSLATORS: The user is asked to accept or refuse a new SSL certificate.
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
		// TRANSLATORS: “%i” is a placeholder for a port number. “%s”  is a placeholder for a protocol name (VNC).
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
	gp->priv->retry_message_panel = mp;
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
		// TRANSLATORS: “%s” is a placeholder for a protocol name, like “RDP”.
		remmina_protocol_widget_set_error(gp, _("Install the %s protocol plugin first."),
						  remmina_file_get_string(remminafile, "protocol"));
		gp->priv->plugin = NULL;
		return;
	}
	gp->priv->plugin = plugin;
	gp->plugin = plugin;

	gp->priv->scalemode = remmina_file_get_int(gp->priv->remmina_file, "scale", FALSE);
	gp->priv->scaler_expand = remmina_file_get_int(gp->priv->remmina_file, "scaler_expand", FALSE);
}

GtkWindow *remmina_protocol_widget_get_gtkwindow(RemminaProtocolWidget *gp)
{
	return rcw_get_gtkwindow(gp->cnnobj);
}

GtkWidget *remmina_protocol_widget_gtkviewport(RemminaProtocolWidget *gp)
{
	return rcw_get_gtkviewport(gp->cnnobj);
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
			REMMINA_DEBUG("Sending keyval: %u, hardware_keycode: %u", event.keyval, event.hardware_keycode);
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
		/* use a multiple of four to mitigate scaling when remote host rounds up */
		w = al.width - al.width % 4;
		h = al.height - al.height % 4;
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
