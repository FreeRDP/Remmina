/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
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

#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include "common/remmina_plugin.h"
#include <gtk/gtkx.h>
#include <time.h>
#define LIBSSH_STATIC 1
#include <libssh/libssh.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>
#include "nx_plugin.h"
#include "nx_session_manager.h"

#define REMMINA_PLUGIN_NX_FEATURE_TOOL_SENDCTRLALTDEL 1
#define REMMINA_PLUGIN_NX_FEATURE_GTKSOCKET 1

/* Forward declaration */
static RemminaProtocolPlugin remmina_plugin_nx;

RemminaPluginService *remmina_plugin_nx_service = NULL;

static gchar *remmina_kbtype = "pc102/us";

/* When more than one NX sessions is connecting in progress, we need this mutex and array
 * to prevent them from stealing the same window ID.
 */
static pthread_mutex_t remmina_nx_init_mutex;
static GArray *remmina_nx_window_id_array;

/* --------- Support for execution on main thread of GTK functions -------------- */

struct onMainThread_cb_data {
	enum { FUNC_GTK_SOCKET_ADD_ID } func;

	GtkSocket* sk;
	Window w;

	/* Mutex for thread synchronization */
	pthread_mutex_t mu;
	/* Flag to catch cancellations */
	gboolean cancelled;
};

static gboolean onMainThread_cb(struct onMainThread_cb_data *d)
{
	TRACE_CALL(__func__);
	if ( !d->cancelled ) {
		switch ( d->func ) {
		case FUNC_GTK_SOCKET_ADD_ID:
			gtk_socket_add_id( d->sk, d->w );
			break;
		}
		pthread_mutex_unlock( &d->mu );
	} else {
		/* Thread has been cancelled, so we must free d memory here */
		g_free( d );
	}
	return G_SOURCE_REMOVE;
}


static void onMainThread_cleanup_handler(gpointer data)
{
	TRACE_CALL(__func__);
	struct onMainThread_cb_data *d = data;
	d->cancelled = TRUE;
}


static void onMainThread_schedule_callback_and_wait( struct onMainThread_cb_data *d )
{
	TRACE_CALL(__func__);
	d->cancelled = FALSE;
	pthread_cleanup_push( onMainThread_cleanup_handler, d );
	pthread_mutex_init( &d->mu, NULL );
	pthread_mutex_lock( &d->mu );
	gdk_threads_add_idle( (GSourceFunc)onMainThread_cb, (gpointer)d );

	pthread_mutex_lock( &d->mu );

	pthread_cleanup_pop(0);
	pthread_mutex_unlock( &d->mu );
	pthread_mutex_destroy( &d->mu );
}

static void onMainThread_gtk_socket_add_id( GtkSocket* sk, Window w)
{
	TRACE_CALL(__func__);

	struct onMainThread_cb_data *d;

	d = (struct onMainThread_cb_data *)g_malloc( sizeof(struct onMainThread_cb_data) );
	d->func = FUNC_GTK_SOCKET_ADD_ID;
	d->sk = sk;
	d->w = w;

	onMainThread_schedule_callback_and_wait( d );
	g_free(d);

}


/* --------------------------------------- */


static gboolean remmina_plugin_nx_try_window_id(Window window_id)
{
	TRACE_CALL(__func__);
	gint i;
	gboolean found = FALSE;

	pthread_mutex_lock(&remmina_nx_init_mutex);
	for (i = 0; i < remmina_nx_window_id_array->len; i++) {
		if (g_array_index(remmina_nx_window_id_array, Window, i) == window_id) {
			found = TRUE;
			break;
		}
	}
	if (!found) {
		g_array_append_val(remmina_nx_window_id_array, window_id);
	}
	pthread_mutex_unlock(&remmina_nx_init_mutex);

	return (!found);
}

static void remmina_plugin_nx_remove_window_id(Window window_id)
{
	TRACE_CALL(__func__);
	gint i;
	gboolean found = FALSE;

	pthread_mutex_lock(&remmina_nx_init_mutex);
	for (i = 0; i < remmina_nx_window_id_array->len; i++) {
		if (g_array_index(remmina_nx_window_id_array, Window, i) == window_id) {
			found = TRUE;
			break;
		}
	}
	if (found) {
		g_array_remove_index_fast(remmina_nx_window_id_array, i);
	}
	pthread_mutex_unlock(&remmina_nx_init_mutex);
}

static void remmina_plugin_nx_on_plug_added(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	remmina_plugin_nx_service->protocol_plugin_signal_connection_opened(gp);
}

static void remmina_plugin_nx_on_plug_removed(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	remmina_plugin_nx_service->protocol_plugin_signal_connection_closed(gp);
}

gboolean remmina_plugin_nx_ssh_auth_callback(gchar **passphrase, gpointer userdata)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget*)userdata;
	gint ret;

	/* SSH passwords must not be saved */
	ret = remmina_plugin_nx_service->protocol_plugin_init_auth(gp, 0,
		_("SSH credentials"), NULL,
		NULL,
		NULL,
		_("Password for private SSH key"));
	if (ret == GTK_RESPONSE_OK) {
		*passphrase = remmina_plugin_nx_service->protocol_plugin_init_get_password(gp);
		return TRUE;
	} else
		return FALSE;
}

static void remmina_plugin_nx_on_proxy_exit(GPid pid, gint status, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget*)data;

	remmina_plugin_nx_service->protocol_plugin_signal_connection_closed(gp);
}

static int remmina_plugin_nx_dummy_handler(Display *dsp, XErrorEvent *err)
{
	TRACE_CALL(__func__);
	return 0;
}

static gboolean remmina_plugin_nx_start_create_notify(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginNxData *gpdata = GET_PLUGIN_DATA(gp);

	gpdata->display = XOpenDisplay(gdk_display_get_name(gdk_display_get_default()));
	if (gpdata->display == NULL)
		return FALSE;

	gpdata->orig_handler = XSetErrorHandler(remmina_plugin_nx_dummy_handler);

	XSelectInput(gpdata->display, XDefaultRootWindow(gpdata->display), SubstructureNotifyMask);

	return TRUE;
}

static gboolean remmina_plugin_nx_monitor_create_notify(RemminaProtocolWidget *gp, const gchar *cmd)
{
	TRACE_CALL(__func__);
	RemminaPluginNxData *gpdata;
	Atom atom;
	XEvent xev;
	Window w;
	Atom type;
	int format;
	unsigned long nitems, rest;
	unsigned char *data = NULL;
	struct timespec ts;

	CANCEL_DEFER

		gpdata = GET_PLUGIN_DATA(gp);
	atom = XInternAtom(gpdata->display, "WM_COMMAND", True);
	if (atom == None)
		return FALSE;

	ts.tv_sec = 0;
	ts.tv_nsec = 200000000;

	while (1) {
		pthread_testcancel();
		while (!XPending(gpdata->display)) {
			nanosleep(&ts, NULL);
			continue;
		}
		XNextEvent(gpdata->display, &xev);
		if (xev.type != CreateNotify)
			continue;
		w = xev.xcreatewindow.window;
		if (XGetWindowProperty(gpdata->display, w, atom, 0, 255, False, AnyPropertyType, &type, &format, &nitems, &rest,
			    &data) != Success)
			continue;
		if (data && strstr((char*)data, cmd) && remmina_plugin_nx_try_window_id(w)) {
			gpdata->window_id = w;
			XFree(data);
			break;
		}
		if (data)
			XFree(data);
	}

	XSetErrorHandler(gpdata->orig_handler);
	XCloseDisplay(gpdata->display);
	gpdata->display = NULL;

	CANCEL_ASYNC
	return TRUE;
}

static gint remmina_plugin_nx_wait_signal(RemminaPluginNxData *gpdata)
{
	TRACE_CALL(__func__);
	fd_set set;
	guchar dummy = 0;

	FD_ZERO(&set);
	FD_SET(gpdata->event_pipe[0], &set);
	select(gpdata->event_pipe[0] + 1, &set, NULL, NULL, NULL);
	if (read(gpdata->event_pipe[0], &dummy, 1)) {
	}
	return (gint)dummy;
}

static void remmina_plugin_nx_log_callback(const gchar *fmt, ...)
{
	char buffer[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, args);
	REMMINA_PLUGIN_DEBUG(buffer);
	va_end(args);
}


static gboolean remmina_plugin_nx_start_session(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginNxData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	RemminaNXSession *nx;
	const gchar *type, *app;
	gchar *s1, *s2;
	gint port;
	gint ret;
	gboolean is_empty_list;
	gint event_type = 0;
	const gchar *cs;
	gint i;
	gboolean disablepasswordstoring;

	remminafile = remmina_plugin_nx_service->protocol_plugin_get_file(gp);
	nx = gpdata->nx;

	/* Connect */

	remmina_nx_session_set_encryption(nx,
		remmina_plugin_nx_service->file_get_int(remminafile, "disableencryption", FALSE) ? 0 : 1);
	remmina_nx_session_set_localport(nx, remmina_plugin_nx_service->pref_get_sshtunnel_port());
	remmina_nx_session_set_log_callback(nx, remmina_plugin_nx_log_callback);

	s2 = remmina_plugin_nx_service->protocol_plugin_start_direct_tunnel(gp, 22, FALSE);
	if (s2 == NULL) {
		return FALSE;
	}
	remmina_plugin_nx_service->get_server_port(s2, 22, &s1, &port);
	g_free(s2);

	if (!remmina_nx_session_open(nx, s1, port, remmina_plugin_nx_service->file_get_string(remminafile, "nx_privatekey"),
		    remmina_plugin_nx_ssh_auth_callback, gp)) {
		g_free(s1);
		return FALSE;
	}
	g_free(s1);

	/* Login */

	s1 = g_strdup(remmina_plugin_nx_service->file_get_string(remminafile, "username"));
	s2 = g_strdup(remmina_plugin_nx_service->file_get_string(remminafile, "password"));

	if (s1 && s2) {
		ret = remmina_nx_session_login(nx, s1, s2);
	} else {
		gchar *s_username, *s_password;

		disablepasswordstoring = remmina_plugin_nx_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);

		ret = remmina_plugin_nx_service->protocol_plugin_init_auth(gp,
			(disablepasswordstoring ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD) | REMMINA_MESSAGE_PANEL_FLAG_USERNAME,
			_("Enter NX authentication credentials"),
			remmina_plugin_nx_service->file_get_string(remminafile, "username"),
			remmina_plugin_nx_service->file_get_string(remminafile, "password"),
			NULL,
			NULL);
		if (ret == GTK_RESPONSE_OK) {
			gboolean save;
			s_username = remmina_plugin_nx_service->protocol_plugin_init_get_username(gp);
			s_password = remmina_plugin_nx_service->protocol_plugin_init_get_password(gp);
			save = remmina_plugin_nx_service->protocol_plugin_init_get_savepassword(gp);
			if (save) {
				remmina_plugin_nx_service->file_set_string(remminafile, "username", s_username);
				remmina_plugin_nx_service->file_set_string(remminafile, "password", s_password);
			} else
				remmina_plugin_nx_service->file_unsave_passwords(remminafile);
		} else {
			return False;
		}

		ret = remmina_nx_session_login(nx, s_username, s_password);
		g_free(s_username);
		g_free(s_password);
	}
	g_free(s1);
	g_free(s2);

	if (!ret)
		return FALSE;

	remmina_plugin_nx_service->protocol_plugin_init_save_cred(gp);

	/* Prepare the session type and application */
	cs = remmina_plugin_nx_service->file_get_string(remminafile, "exec");
	if (!cs || g_strcmp0(cs, "GNOME") == 0) {
		type = "unix-gnome";
		app = NULL;
	}else
	if (g_strcmp0(cs, "KDE") == 0) {
		type = "unix-kde";
		app = NULL;
	}else
	if (g_strcmp0(cs, "Xfce") == 0) {
		/* NX does not know Xfce. So we simply launch the Xfce startup program. */
		type = "unix-application";
		app = "startxfce4";
	}else
	if (g_strcmp0(cs, "Shadow") == 0) {
		type = "shadow";
		app = NULL;
	}else  {
		type = "unix-application";
		app = cs;
	}

	/* List sessions */

	gpdata->attach_session = (g_strcmp0(type, "shadow") == 0);
	while (1) {
		remmina_nx_session_add_parameter(nx, "type", type);
		if (!gpdata->attach_session) {
			remmina_nx_session_add_parameter(nx, "user",
				remmina_plugin_nx_service->file_get_string(remminafile, "username"));
			remmina_nx_session_add_parameter(nx, "status", "suspended,running");
		}

		if (!remmina_nx_session_list(nx)) {
			return FALSE;
		}

		is_empty_list = !remmina_nx_session_iter_first(nx, &gpdata->iter);
		if (is_empty_list && !gpdata->manager_started && !gpdata->attach_session) {
			event_type = REMMINA_NX_EVENT_START;
		}else  {
			remmina_nx_session_manager_start(gp);
			event_type = remmina_plugin_nx_wait_signal(gpdata);
			if (event_type == REMMINA_NX_EVENT_CANCEL) {
				return FALSE;
			}
			if (event_type == REMMINA_NX_EVENT_TERMINATE) {
				if (!is_empty_list) {
					s1 = remmina_nx_session_iter_get(nx, &gpdata->iter, REMMINA_NX_SESSION_COLUMN_ID);
					remmina_nx_session_add_parameter(nx, "sessionid", s1);
					g_free(s1);
					if (!remmina_nx_session_terminate(nx)) {
						remmina_nx_session_manager_start(gp);
						remmina_plugin_nx_wait_signal(gpdata);
					}
				}
				continue;
			}
		}

		break;
	}

	/* Start, Restore or Attach, based on the setting and existing session */
	remmina_nx_session_add_parameter(nx, "type", type);
	i = remmina_plugin_nx_service->file_get_int(remminafile, "quality", 0);
	remmina_nx_session_add_parameter(nx, "link", i > 2 ? "lan" : i == 2 ? "adsl" : i == 1 ? "isdn" : "modem");
	remmina_nx_session_add_parameter(nx, "geometry", "%ix%i",
		remmina_plugin_nx_service->get_profile_remote_width(gp),
		remmina_plugin_nx_service->get_profile_remote_height(gp));
	remmina_nx_session_add_parameter(nx, "keyboard", remmina_kbtype);
	remmina_nx_session_add_parameter(nx, "client", "linux");
	remmina_nx_session_add_parameter(nx, "media", "0");
	remmina_nx_session_add_parameter(nx, "clipboard",
		remmina_plugin_nx_service->file_get_int(remminafile, "disableclipboard", FALSE) ? "none" : "both");

	switch (event_type) {

	case REMMINA_NX_EVENT_START:
		if (app)
			remmina_nx_session_add_parameter(nx, "application", app);

		remmina_nx_session_add_parameter(nx, "session",
			remmina_plugin_nx_service->file_get_string(remminafile, "name"));
		remmina_nx_session_add_parameter(nx, "screeninfo", "%ix%ix24+render",
			remmina_plugin_nx_service->file_get_int(remminafile, "resolution_width", 0),
			remmina_plugin_nx_service->file_get_int(remminafile, "resolution_height", 0));

		if (!remmina_nx_session_start(nx))
			return FALSE;
		break;

	case REMMINA_NX_EVENT_ATTACH:
		s1 = remmina_nx_session_iter_get(nx, &gpdata->iter, REMMINA_NX_SESSION_COLUMN_ID);
		remmina_nx_session_add_parameter(nx, "id", s1);
		g_free(s1);

		s1 = remmina_nx_session_iter_get(nx, &gpdata->iter, REMMINA_NX_SESSION_COLUMN_DISPLAY);
		remmina_nx_session_add_parameter(nx, "display", s1);
		g_free(s1);

		if (!remmina_nx_session_attach(nx))
			return FALSE;
		break;

	case REMMINA_NX_EVENT_RESTORE:
		s1 = remmina_nx_session_iter_get(nx, &gpdata->iter, REMMINA_NX_SESSION_COLUMN_ID);
		remmina_nx_session_add_parameter(nx, "id", s1);
		g_free(s1);

		remmina_nx_session_add_parameter(nx, "session",
			remmina_plugin_nx_service->file_get_string(remminafile, "name"));

		if (!remmina_nx_session_restore(nx))
			return FALSE;
		break;

	default:
		return FALSE;
	}

	if (!remmina_nx_session_tunnel_open(nx))
		return FALSE;

	if (!remmina_plugin_nx_start_create_notify(gp))
		return FALSE;

	/* nxproxy */
	if (!remmina_nx_session_invoke_proxy(nx, -1, remmina_plugin_nx_on_proxy_exit, gp))
		return FALSE;

	/* get the window id of the remote nxagent */
	if (!remmina_plugin_nx_monitor_create_notify(gp, "nxagent"))
		return FALSE;

	/* embed it */
	onMainThread_gtk_socket_add_id(GTK_SOCKET(gpdata->socket), gpdata->window_id);


	return TRUE;
}

static gboolean remmina_plugin_nx_main(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginNxData *gpdata = GET_PLUGIN_DATA(gp);
	gboolean ret;
	const gchar *err;

	gpdata->nx = remmina_nx_session_new();
	ret = remmina_plugin_nx_start_session(gp);
	if (!ret) {
		err = remmina_nx_session_get_error(gpdata->nx);
		if (err) {
			remmina_plugin_nx_service->protocol_plugin_set_error(gp, "%s", err);
		}
	}

	gpdata->thread = 0;
	return ret;
}

static gpointer remmina_plugin_nx_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	CANCEL_ASYNC
	if (!remmina_plugin_nx_main(gp)) {
		remmina_plugin_nx_service->protocol_plugin_signal_connection_closed(gp);
	}
	return NULL;
}

static void remmina_plugin_nx_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginNxData *gpdata;
	gint flags;

	gpdata = g_new0(RemminaPluginNxData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	gpdata->socket = gtk_socket_new();
	remmina_plugin_nx_service->protocol_plugin_register_hostkey(gp, gpdata->socket);
	gtk_widget_show(gpdata->socket);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-added", G_CALLBACK(remmina_plugin_nx_on_plug_added), gp);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-removed", G_CALLBACK(remmina_plugin_nx_on_plug_removed), gp);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->socket);

	if (pipe(gpdata->event_pipe)) {
		g_print("Error creating pipes.\n");
		gpdata->event_pipe[0] = -1;
		gpdata->event_pipe[1] = -1;
	}else  {
		flags = fcntl(gpdata->event_pipe[0], F_GETFL, 0);
		fcntl(gpdata->event_pipe[0], F_SETFL, flags | O_NONBLOCK);
	}
}

static gboolean remmina_plugin_nx_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginNxData *gpdata = GET_PLUGIN_DATA(gp);
	gint width, height;

	if (!remmina_plugin_nx_service->gtksocket_available()) {
		remmina_plugin_nx_service->protocol_plugin_set_error(gp,
			_("The protocol \"%s\" is unavailable because GtkSocket only works under X.Org."),
			remmina_plugin_nx.name);
		return FALSE;
	}

	width = remmina_plugin_nx_service->get_profile_remote_width(gp);
	height = remmina_plugin_nx_service->get_profile_remote_height(gp);

	remmina_plugin_nx_service->protocol_plugin_set_width(gp, width);
	remmina_plugin_nx_service->protocol_plugin_set_height(gp, height);
	gtk_widget_set_size_request(GTK_WIDGET(gp), width, height);

	gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));

	if (pthread_create(&gpdata->thread, NULL, remmina_plugin_nx_main_thread, gp)) {
		remmina_plugin_nx_service->protocol_plugin_set_error(gp,
			"Failed to initialize pthread. Falling back to non-thread mode…");
		gpdata->thread = 0;
		return FALSE;
	}else  {
		return TRUE;
	}
}

static gboolean remmina_plugin_nx_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginNxData *gpdata = GET_PLUGIN_DATA(gp);

	if (gpdata->thread) {
		pthread_cancel(gpdata->thread);
		if (gpdata->thread)
			pthread_join(gpdata->thread, NULL);
	}
	if (gpdata->session_manager_start_handler) {
		g_source_remove(gpdata->session_manager_start_handler);
		gpdata->session_manager_start_handler = 0;
	}

	if (gpdata->window_id) {
		remmina_plugin_nx_remove_window_id(gpdata->window_id);
	}

	if (gpdata->nx) {
		remmina_nx_session_free(gpdata->nx);
		gpdata->nx = NULL;
	}

	if (gpdata->display) {
		XSetErrorHandler(gpdata->orig_handler);
		XCloseDisplay(gpdata->display);
		gpdata->display = NULL;
	}
	close(gpdata->event_pipe[0]);
	close(gpdata->event_pipe[1]);

	remmina_plugin_nx_service->protocol_plugin_signal_connection_closed(gp);

	return FALSE;
}

/* Send CTRL+ALT+DEL keys keystrokes to the plugin socket widget */
static void remmina_plugin_nx_send_ctrlaltdel(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	guint keys[] = { GDK_KEY_Control_L, GDK_KEY_Alt_L, GDK_KEY_Delete };
	RemminaPluginNxData *gpdata = GET_PLUGIN_DATA(gp);

	remmina_plugin_nx_service->protocol_plugin_send_keys_signals(gpdata->socket,
		keys, G_N_ELEMENTS(keys), GDK_KEY_PRESS | GDK_KEY_RELEASE);
}

static gboolean remmina_plugin_nx_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	return TRUE;
}

static void remmina_plugin_nx_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	switch (feature->id) {
	case REMMINA_PLUGIN_NX_FEATURE_TOOL_SENDCTRLALTDEL:
		remmina_plugin_nx_send_ctrlaltdel(gp);
		break;
	default:
		break;
	}
}

/* Array of key/value pairs for quality selection */
static gpointer quality_list[] =
{
	"0", N_("Poor (fastest)"),
	"1", N_("Medium"),
	"2", N_("Good"),
	"9", N_("Best (slowest)"),
	NULL
};

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting tooltip
 */
static const RemminaProtocolSetting remmina_plugin_nx_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER,	    "server",	     NULL,		    FALSE, NULL,		    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_FILE,	    "nx_privatekey", N_("Identity file"),   FALSE, NULL,		    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	    "username",	     N_("Username"),	    FALSE, NULL,		    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD,   "password",	     N_("User password"),   FALSE, NULL,		    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION, "resolution",    NULL,		    FALSE, GINT_TO_POINTER(1),	    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	    "quality",	     N_("Quality"),	    FALSE, quality_list,	    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_COMBO,	    "exec",	     N_("Startup program"), FALSE, "GNOME,KDE,Xfce,Shadow", NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	    NULL,	     NULL,		    FALSE, NULL,		    NULL }
};

/* Array of RemminaProtocolSetting for advanced settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting tooltip
 */
static const RemminaProtocolSetting remmina_plugin_nx_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard",	 N_("Disable clipboard sync"),	 FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableencryption",	 N_("Disable encryption"),	 FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "showcursor",		 N_("Use local cursor"),	 FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disablepasswordstoring", N_("Forget passwords after use"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,   NULL,			 NULL,				 FALSE, NULL, NULL }
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_plugin_nx_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_NX_FEATURE_TOOL_SENDCTRLALTDEL, N_("Send Ctrl+Alt+Del"), NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_GTKSOCKET, REMMINA_PLUGIN_NX_FEATURE_GTKSOCKET,	    NULL,	   NULL,	NULL},
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END,  0,					     NULL,			 NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin_nx =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   // Type
	"NX",                                           // Name
	N_("NX - NX Technology"),                       // Description
	GETTEXT_PACKAGE,                                // Translation domain
	VERSION,                                        // Version number
	"remmina-nx-symbolic",                          // Icon for normal connection
	"remmina-nx-symbolic",                          // Icon for SSH connection
	remmina_plugin_nx_basic_settings,               // Array for basic settings
	remmina_plugin_nx_advanced_settings,            // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,            // SSH settings type
	remmina_plugin_nx_features,                     // Array for available features
	remmina_plugin_nx_init,                         // Plugin initialization
	remmina_plugin_nx_open_connection,              // Plugin open connection
	remmina_plugin_nx_close_connection,             // Plugin close connection
	remmina_plugin_nx_query_feature,                // Query for available features
	remmina_plugin_nx_call_feature,                 // Call a feature
	NULL,                                           // Send a keystroke
	NULL                                            // Screenshot support unavailable
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);
	Display *dpy;
	XkbRF_VarDefsRec vd;
	gchar *s;

	remmina_plugin_nx_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if ((dpy = XkbOpenDisplay(NULL, NULL, NULL, NULL, NULL, NULL)) != NULL) {
		if (XkbRF_GetNamesProp(dpy, NULL, &vd)) {
			remmina_kbtype = g_strdup_printf("%s/%s", vd.model, vd.layout);
			if (vd.layout)
				XFree(vd.layout);
			if (vd.model)
				XFree(vd.model);
			if (vd.variant)
				XFree(vd.variant);
			if (vd.options)
				XFree(vd.options);
			s = strchr(remmina_kbtype, ',');
			if (s)
				*s = '\0';
			/* g_print("NX: Detected \"%s\" keyboard type\n", remmina_kbtype); */
		}
		XCloseDisplay(dpy);
	}

	if (!service->register_plugin((RemminaPlugin*)&remmina_plugin_nx)) {
		return FALSE;
	}

	ssh_init();
	pthread_mutex_init(&remmina_nx_init_mutex, NULL);
	remmina_nx_window_id_array = g_array_new(FALSE, TRUE, sizeof(Window));

	return TRUE;
}
