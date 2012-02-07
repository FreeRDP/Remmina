/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
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

#include <errno.h>
#include <pthread.h>
#include "common/remmina_plugin.h"
#if GTK_VERSION == 3
#  include <gtk/gtkx.h>
#endif
#include <time.h>
#include <libssh/libssh.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>
#include "nx_plugin.h"
#include "nx_session_manager.h"

RemminaPluginService *remmina_plugin_nx_service = NULL;

static gchar *remmina_kbtype = "pc102/us";

/* When more than one NX sessions is connecting in progress, we need this mutex and array
 * to prevent them from stealing the same window id.
 */
static pthread_mutex_t remmina_nx_init_mutex;
static GArray *remmina_nx_window_id_array;

static gboolean remmina_plugin_nx_try_window_id(Window window_id)
{
	gint i;
	gboolean found = FALSE;

	pthread_mutex_lock(&remmina_nx_init_mutex);
	for (i = 0; i < remmina_nx_window_id_array->len; i++)
	{
		if (g_array_index(remmina_nx_window_id_array, Window, i) == window_id)
		{
			found = TRUE;
			break;
		}
	}
	if (!found)
	{
		g_array_append_val(remmina_nx_window_id_array, window_id);
	}
	pthread_mutex_unlock(&remmina_nx_init_mutex);

	return (!found);
}

static void remmina_plugin_nx_remove_window_id(Window window_id)
{
	gint i;
	gboolean found = FALSE;

	pthread_mutex_lock(&remmina_nx_init_mutex);
	for (i = 0; i < remmina_nx_window_id_array->len; i++)
	{
		if (g_array_index(remmina_nx_window_id_array, Window, i) == window_id)
		{
			found = TRUE;
			break;
		}
	}
	if (found)
	{
		g_array_remove_index_fast(remmina_nx_window_id_array, i);
	}
	pthread_mutex_unlock(&remmina_nx_init_mutex);
}

static void remmina_plugin_nx_on_plug_added(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	remmina_plugin_nx_service->protocol_plugin_emit_signal(gp, "connect");
}

static void remmina_plugin_nx_on_plug_removed(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	remmina_plugin_nx_service->protocol_plugin_close_connection(gp);
}

gboolean remmina_plugin_nx_ssh_auth_callback(gchar **passphrase, gpointer userdata)
{
	RemminaProtocolWidget *gp = (RemminaProtocolWidget*) userdata;
	gint ret;

	THREADS_ENTER ret = remmina_plugin_nx_service->protocol_plugin_init_authpwd(gp, REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY);
	THREADS_LEAVE

	if (ret != GTK_RESPONSE_OK)
		return FALSE;
	*passphrase = remmina_plugin_nx_service->protocol_plugin_init_get_password(gp);

	return TRUE;
}

static void remmina_plugin_nx_on_proxy_exit(GPid pid, gint status, gpointer data)
{
	RemminaProtocolWidget *gp = (RemminaProtocolWidget*) data;

	remmina_plugin_nx_service->protocol_plugin_close_connection(gp);
}

static int remmina_plugin_nx_dummy_handler(Display *dsp, XErrorEvent *err)
{
	return 0;
}

static gboolean remmina_plugin_nx_start_create_notify(RemminaProtocolWidget *gp)
{
	RemminaPluginNxData *gpdata;

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	gpdata->display = XOpenDisplay(gdk_display_get_name(gdk_display_get_default()));
	if (gpdata->display == NULL)
		return FALSE;

	gpdata->orig_handler = XSetErrorHandler(remmina_plugin_nx_dummy_handler);

	XSelectInput(gpdata->display, XDefaultRootWindow(gpdata->display), SubstructureNotifyMask);

	return TRUE;
}

static gboolean remmina_plugin_nx_monitor_create_notify(RemminaProtocolWidget *gp, const gchar *cmd)
{
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

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	atom = XInternAtom(gpdata->display, "WM_COMMAND", True);
	if (atom == None)
		return FALSE;

	ts.tv_sec = 0;
	ts.tv_nsec = 200000000;

	while (1)
	{
		pthread_testcancel();
		while (!XPending(gpdata->display))
		{
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
		if (data && strstr((char *) data, cmd) && remmina_plugin_nx_try_window_id(w))
		{
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
	fd_set set;
	guchar dummy = 0;

	FD_ZERO(&set);
	FD_SET(gpdata->event_pipe[0], &set);
	select(gpdata->event_pipe[0] + 1, &set, NULL, NULL, NULL);
	if (read(gpdata->event_pipe[0], &dummy, 1))
	{
	}
	return (gint) dummy;
}

static gboolean remmina_plugin_nx_start_session(RemminaProtocolWidget *gp)
{
	RemminaPluginNxData *gpdata;
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

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_nx_service->protocol_plugin_get_file(gp);
	nx = gpdata->nx;

	/* Connect */

	remmina_nx_session_set_encryption(nx,
			remmina_plugin_nx_service->file_get_int(remminafile, "disableencryption", FALSE) ? 0 : 1);
	remmina_nx_session_set_localport(nx, remmina_plugin_nx_service->pref_get_sshtunnel_port());
	remmina_nx_session_set_log_callback(nx, remmina_plugin_nx_service->log_printf);

	s2 = remmina_plugin_nx_service->protocol_plugin_start_direct_tunnel(gp, 22, FALSE);
	if (s2 == NULL)
	{
		return FALSE;
	}
	remmina_plugin_nx_service->get_server_port(s2, 22, &s1, &port);
	g_free(s2);

	if (!remmina_nx_session_open(nx, s1, port, remmina_plugin_nx_service->file_get_string(remminafile, "nx_privatekey"),
			remmina_plugin_nx_ssh_auth_callback, gp))
	{
		g_free(s1);
		return FALSE;
	}
	g_free(s1);

	/* Login */

	s1 = g_strdup(remmina_plugin_nx_service->file_get_string(remminafile, "username"));
	THREADS_ENTER s2 = remmina_plugin_nx_service->file_get_secret(remminafile, "password");
	THREADS_LEAVE
	if (s1 && s2)
	{
		ret = remmina_nx_session_login(nx, s1, s2);
	}
	else
	{
		g_free(s1);
		g_free(s2);

		THREADS_ENTER ret = remmina_plugin_nx_service->protocol_plugin_init_authuserpwd(gp, FALSE);
		THREADS_LEAVE

		if (ret != GTK_RESPONSE_OK)
			return FALSE;

		s1 = remmina_plugin_nx_service->protocol_plugin_init_get_username(gp);
		s2 = remmina_plugin_nx_service->protocol_plugin_init_get_password(gp);
		ret = remmina_nx_session_login(nx, s1, s2);
	}
	g_free(s1);
	g_free(s2);

	if (!ret)
		return FALSE;

	THREADS_ENTER
	remmina_plugin_nx_service->protocol_plugin_init_save_cred(gp);
	THREADS_LEAVE

	/* Prepare the session type and application */
	cs = remmina_plugin_nx_service->file_get_string(remminafile, "exec");
	if (!cs || g_strcmp0(cs, "GNOME") == 0)
	{
		type = "unix-gnome";
		app = NULL;
	}
	else
		if (g_strcmp0(cs, "KDE") == 0)
		{
			type = "unix-kde";
			app = NULL;
		}
		else
			if (g_strcmp0(cs, "Xfce") == 0)
			{
				/* NX does not know Xfce. So we simply launch the Xfce startup program. */
				type = "unix-application";
				app = "startxfce4";
			}
			else
				if (g_strcmp0(cs, "Shadow") == 0)
				{
					type = "shadow";
					app = NULL;
				}
				else
				{
					type = "unix-application";
					app = cs;
				}

	/* List sessions */

	gpdata->attach_session = (g_strcmp0(type, "shadow") == 0);
	while (1)
	{
		remmina_nx_session_add_parameter(nx, "type", type);
		if (!gpdata->attach_session)
		{
			remmina_nx_session_add_parameter(nx, "user",
					remmina_plugin_nx_service->file_get_string(remminafile, "username"));
			remmina_nx_session_add_parameter(nx, "status", "suspended,running");
		}

		if (!remmina_nx_session_list(nx))
		{
			return FALSE;
		}

		is_empty_list = !remmina_nx_session_iter_first(nx, &gpdata->iter);
		if (is_empty_list && !gpdata->manager_started && !gpdata->attach_session)
		{
			event_type = REMMINA_NX_EVENT_START;
		}
		else
		{
			remmina_nx_session_manager_start(gp);
			event_type = remmina_plugin_nx_wait_signal(gpdata);
			if (event_type == REMMINA_NX_EVENT_CANCEL)
			{
				return FALSE;
			}
			if (event_type == REMMINA_NX_EVENT_TERMINATE)
			{
				if (!is_empty_list)
				{
					s1 = remmina_nx_session_iter_get(nx, &gpdata->iter, REMMINA_NX_SESSION_COLUMN_ID);
					remmina_nx_session_add_parameter(nx, "sessionid", s1);
					g_free(s1);
					if (!remmina_nx_session_terminate(nx))
					{
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
			remmina_plugin_nx_service->file_get_int(remminafile, "resolution_width", 0),
			remmina_plugin_nx_service->file_get_int(remminafile, "resolution_height", 0));
	remmina_nx_session_add_parameter(nx, "keyboard", remmina_kbtype);
	remmina_nx_session_add_parameter(nx, "client", "linux");
	remmina_nx_session_add_parameter(nx, "media", "0");
	remmina_nx_session_add_parameter(nx, "clipboard",
			remmina_plugin_nx_service->file_get_int(remminafile, "disableclipboard", FALSE) ? "none" : "both");

	switch (event_type)
	{

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
	THREADS_ENTER
	gtk_socket_add_id(GTK_SOCKET(gpdata->socket), gpdata->window_id);
	THREADS_LEAVE

	return TRUE;
}

static gboolean remmina_plugin_nx_main(RemminaProtocolWidget *gp)
{
	RemminaPluginNxData *gpdata;
	gboolean ret;
	const gchar *err;

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	gpdata->nx = remmina_nx_session_new();
	ret = remmina_plugin_nx_start_session(gp);
	if (!ret)
	{
		err = remmina_nx_session_get_error(gpdata->nx);
		if (err)
		{
			remmina_plugin_nx_service->protocol_plugin_set_error(gp, "%s", err);
		}
	}

	gpdata->thread = 0;
	return ret;
}

static gpointer remmina_plugin_nx_main_thread(gpointer data)
{
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	CANCEL_ASYNC
	if (!remmina_plugin_nx_main((RemminaProtocolWidget*) data))
	{
		IDLE_ADD((GSourceFunc) remmina_plugin_nx_service->protocol_plugin_close_connection, data);
	}
	return NULL;
}

static void remmina_plugin_nx_init(RemminaProtocolWidget *gp)
{
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

	if (pipe(gpdata->event_pipe))
	{
		g_print("Error creating pipes.\n");
		gpdata->event_pipe[0] = -1;
		gpdata->event_pipe[1] = -1;
	}
	else
	{
		flags = fcntl(gpdata->event_pipe[0], F_GETFL, 0);
		fcntl(gpdata->event_pipe[0], F_SETFL, flags | O_NONBLOCK);
	}
}

static gboolean remmina_plugin_nx_open_connection(RemminaProtocolWidget *gp)
{
	RemminaPluginNxData *gpdata;
	RemminaFile *remminafile;
	const gchar *resolution;
	gint width, height;

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_nx_service->protocol_plugin_get_file(gp);

	resolution = remmina_plugin_nx_service->file_get_string(remminafile, "resolution");
	if (!resolution || !strchr(resolution, 'x'))
	{
		remmina_plugin_nx_service->protocol_plugin_set_expand(gp, TRUE);
		gtk_widget_set_size_request(GTK_WIDGET(gp), 640, 480);
	}
	else
	{
		width = remmina_plugin_nx_service->file_get_int(remminafile, "resolution_width", 640);
		height = remmina_plugin_nx_service->file_get_int(remminafile, "resolution_height", 480);
		remmina_plugin_nx_service->protocol_plugin_set_width(gp, width);
		remmina_plugin_nx_service->protocol_plugin_set_height(gp, height);
		gtk_widget_set_size_request(GTK_WIDGET(gp), width, height);
	}
	gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));

	if (pthread_create(&gpdata->thread, NULL, remmina_plugin_nx_main_thread, gp))
	{
		remmina_plugin_nx_service->protocol_plugin_set_error(gp,
				"Failed to initialize pthread. Falling back to non-thread mode...");
		gpdata->thread = 0;
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

static gboolean remmina_plugin_nx_close_connection(RemminaProtocolWidget *gp)
{
	RemminaPluginNxData *gpdata;

	gpdata = (RemminaPluginNxData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	if (gpdata->thread)
	{
		pthread_cancel(gpdata->thread);
		if (gpdata->thread)
			pthread_join(gpdata->thread, NULL);
	}
	if (gpdata->session_manager_start_handler)
	{
		g_source_remove(gpdata->session_manager_start_handler);
		gpdata->session_manager_start_handler = 0;
	}

	if (gpdata->window_id)
	{
		remmina_plugin_nx_remove_window_id(gpdata->window_id);
	}

	if (gpdata->nx)
	{
		remmina_nx_session_free(gpdata->nx);
		gpdata->nx = NULL;
	}

	if (gpdata->display)
	{
		XSetErrorHandler(gpdata->orig_handler);
		XCloseDisplay(gpdata->display);
		gpdata->display = NULL;
	}
	close(gpdata->event_pipe[0]);
	close(gpdata->event_pipe[1]);

	remmina_plugin_nx_service->protocol_plugin_emit_signal(gp, "disconnect");

	return FALSE;
}

static gboolean remmina_plugin_nx_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	return FALSE;
}

static void remmina_plugin_nx_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
}

static gpointer quality_list[] =
{ "0", N_("Poor (fastest)"), "1", N_("Medium"), "2", N_("Good"), "9", N_("Best (slowest)"), NULL };

static const RemminaProtocolSetting remmina_plugin_nx_basic_settings[] =
{
{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_FILE, "nx_privatekey", N_("Identity file"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION, NULL, NULL, FALSE, GINT_TO_POINTER(1), NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "quality", N_("Quality"), FALSE, quality_list, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_COMBO, "exec", N_("Startup program"), FALSE, "GNOME,KDE,Xfce,Shadow", NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL } };

static const RemminaProtocolSetting remmina_plugin_nx_advanced_settings[] =
{
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard", N_("Disable clipboard sync"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableencryption", N_("Disable encryption"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "showcursor", N_("Use local cursor"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL } };

static RemminaProtocolPlugin remmina_plugin_nx =
{ REMMINA_PLUGIN_TYPE_PROTOCOL, "NX", N_("NX - NX Technology"), GETTEXT_PACKAGE, VERSION,

"remmina-nx", "remmina-nx", remmina_plugin_nx_basic_settings, remmina_plugin_nx_advanced_settings,
		REMMINA_PROTOCOL_SSH_SETTING_TUNNEL, NULL,

		remmina_plugin_nx_init, remmina_plugin_nx_open_connection, remmina_plugin_nx_close_connection,
		remmina_plugin_nx_query_feature, remmina_plugin_nx_call_feature };

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	Display *dpy;
	XkbRF_VarDefsRec vd;
	gchar *s;

	remmina_plugin_nx_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if ((dpy = XkbOpenDisplay(NULL, NULL, NULL, NULL, NULL, NULL)) != NULL)
	{
		if (XkbRF_GetNamesProp(dpy, NULL, &vd))
		{
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
			g_print("NX: detected keyboard type %s\n", remmina_kbtype);
		}
		XCloseDisplay(dpy);
	}

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_nx))
	{
		return FALSE;
	}

	ssh_init();
	pthread_mutex_init(&remmina_nx_init_mutex, NULL);
	remmina_nx_window_id_array = g_array_new(FALSE, TRUE, sizeof(Window));

	return TRUE;
}

