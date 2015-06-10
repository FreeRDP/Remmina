/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAPLUGINNX_H__
#define __REMMINAPLUGINNX_H__

#define GET_PLUGIN_DATA(gp) (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

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
	gint default_response;
} RemminaPluginNxData;

extern RemminaPluginService *remmina_plugin_nx_service;

G_END_DECLS

#endif

