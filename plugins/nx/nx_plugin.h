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

#pragma once

#define GET_PLUGIN_DATA(gp) (RemminaPluginNxData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

G_BEGIN_DECLS

#include "nx_session.h"

typedef enum {
	REMMINA_NX_EVENT_CANCEL,
	REMMINA_NX_EVENT_START,
	REMMINA_NX_EVENT_RESTORE,
	REMMINA_NX_EVENT_ATTACH,
	REMMINA_NX_EVENT_TERMINATE
} RemminaNXEventType;

typedef struct _RemminaPluginNxData {
	GtkWidget *		socket;
	gint			socket_id;

	pthread_t		thread;

	RemminaNXSession *	nx;

	Display *		display;
	Window			window_id;
	int (*orig_handler)(Display *, XErrorEvent *);

	/* Session Manager data */
	gboolean		manager_started;
	GtkWidget *		manager_dialog;
	gboolean		manager_selected;

	/* Communication between the NX thread and the session manager */
	gint			event_pipe[2];
	guint			session_manager_start_handler;
	gboolean		attach_session;
	GtkTreeIter		iter;
	gint			default_response;
} RemminaPluginNxData;

extern RemminaPluginService *remmina_plugin_nx_service;

G_END_DECLS
