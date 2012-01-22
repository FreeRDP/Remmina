/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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

#ifndef __REMMINAPLUGINNX_H__
#define __REMMINAPLUGINNX_H__

G_BEGIN_DECLS

#include "nx_session.h"

typedef enum
{
	REMMINA_NX_EVENT_CANCEL,
	REMMINA_NX_EVENT_START,
	REMMINA_NX_EVENT_RESTORE,
	REMMINA_NX_EVENT_ATTACH,
	REMMINA_NX_EVENT_TERMINATE
} RemminaNXEventType;

typedef struct _RemminaPluginNxData
{
	GtkWidget *socket;
	gint socket_id;

	pthread_t thread;

	RemminaNXSession *nx;

	Display *display;
	Window window_id;
	int (*orig_handler)(Display *, XErrorEvent *);

	/* Session Manager data */
	gboolean manager_started;
	GtkWidget *manager_dialog;
	gboolean manager_selected;

	/* Communication between the NX thread and the session manager */
	gint event_pipe[2];
	guint session_manager_start_handler;
	gboolean attach_session;
	GtkTreeIter iter;
} RemminaPluginNxData;

extern RemminaPluginService *remmina_plugin_nx_service;

G_END_DECLS

#endif

