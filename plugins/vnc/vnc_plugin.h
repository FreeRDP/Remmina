/*
 * Remmina - The GTK+ Remote Desktop Client
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

#pragma once

#ifndef __PLUGIN_CONFIG_H
#define __PLUGIN_CONFIG_H

#define VNC_PLUGIN_NAME        "VNC"
#define VNC_PLUGIN_DESCRIPTION N_("Remmina VNC Plugin")
#define VNC_PLUGIN_VERSION     VERSION
#define VNC_PLUGIN_APPICON     "remmina-vnc-symbolic"
#define VNC_PLUGIN_SSH_APPICON     "remmina-vnc-ssh-symbolic"
#define VNCI_PLUGIN_NAME        "VNCI"
#define VNCI_PLUGIN_DESCRIPTION N_("Remmina VNC listener Plugin")
#define VNCI_PLUGIN_VERSION     VERSION
#define VNCI_PLUGIN_APPICON     "remmina-vnc-symbolic"
#define VNCI_PLUGIN_SSH_APPICON     "remmina-vnc-ssh-symbolic"
#endif

typedef struct _RemminaPluginVncData {
	/* Whether the user requests to connect/disconnect */
	gboolean		connected;
	/* Whether the vnc process is running */
	gboolean		running;
	/* Whether the initialzation calls the authentication process */
	gboolean		auth_called;
	/* Whether it is the first attempt for authentication. Only first attempt will try to use cached credentials */
	gboolean		auth_first;

	GtkWidget *		drawing_area;
	guchar *		vnc_buffer;
	cairo_surface_t *	rgb_buffer;

	gint			queuedraw_x, queuedraw_y, queuedraw_w, queuedraw_h;
	guint			queuedraw_handler;

	gulong			clipboard_handler;
	GTimeVal		clipboard_timer;

	cairo_surface_t *	queuecursor_surface;
	gint			queuecursor_x, queuecursor_y;
	guint			queuecursor_handler;

	gpointer		client;
	gint			listen_sock;

	gint			button_mask;

	GPtrArray *		pressed_keys;

	pthread_mutex_t		vnc_event_queue_mutex;
	GQueue *		vnc_event_queue;
	gint			vnc_event_pipe[2];

	pthread_t		thread;
	pthread_mutex_t		buffer_mutex;
} RemminaPluginVncData;

enum {
	REMMINA_PLUGIN_VNC_EVENT_KEY,
	REMMINA_PLUGIN_VNC_EVENT_POINTER,
	REMMINA_PLUGIN_VNC_EVENT_CUTTEXT,
	REMMINA_PLUGIN_VNC_EVENT_CHAT_OPEN,
	REMMINA_PLUGIN_VNC_EVENT_CHAT_SEND,
	REMMINA_PLUGIN_VNC_EVENT_CHAT_CLOSE
};

typedef struct _RemminaPluginVncEvent {
	gint event_type;
	union {
		struct {
			guint		keyval;
			gboolean	pressed;
		} key;
		struct {
			gint	x;
			gint	y;
			gint	button_mask;
		} pointer;
		struct {
			gchar *text;
		} text;
	} event_data;
} RemminaPluginVncEvent;

typedef struct _RemminaPluginVncCoordinates {
	gint x, y;
} RemminaPluginVncCoordinates;

G_BEGIN_DECLS

/* --------- Support for execution on main thread of GUI functions -------------- */
static void remmina_plugin_vnc_update_scale(RemminaProtocolWidget *gp, gboolean scale);

G_END_DECLS
