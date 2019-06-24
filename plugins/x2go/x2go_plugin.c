/*
 *     Project: Remmina Plugin X2Go
 * Description: Remmina protocol plugin to connect via X2Go using PyHoca
 *      Author: Mike Gabriel <mike.gabriel@das-netzwerkteam.de>
 *              Antenore Gatta <antenore@simbiosi.org>
 *   Copyright: 2010-2011 Vic Lee
 *              2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 *              2015 Antenore Gatta
 *              2016-2018 Antenore Gatta, Giovanni Panozzo
 *              2019 Mike Gabriel
 *     License: GPL-2+
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

#include "x2go_plugin.h"
#include <common/remmina_plugin.h>
#include <gtk/gtkx.h>
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>


#define GET_PLUGIN_DATA(gp) (RemminaPluginX2GoData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

typedef struct _RemminaPluginX2GoData {
	GtkWidget *socket;
	gint socket_id;

	gint width;
	gint height;

	pthread_t thread;

	Display *display;
	Window window_id;
	int (*orig_handler)(Display *, XErrorEvent *);

	GPid pidx2go;
	gboolean ready;
} RemminaPluginX2GoData;

static RemminaPluginService *remmina_plugin_x2go_service = NULL;

/* Forward declaration */
static RemminaProtocolPlugin remmina_plugin_x2go;

/* When more than one NX sessions is connecting in progress, we need this mutex and array
 * to prevent them from stealing the same window id.
 */
static pthread_mutex_t remmina_x2go_init_mutex;
static GArray *remmina_x2go_window_id_array;

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
		/* thread has been cancelled, so we must free d memory here */
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


static gboolean remmina_plugin_x2go_exec_x2go(gchar *host, gint sshport, gchar *username, gchar *password,
		gchar *command, gchar *kbdlayout, gchar *kbdtype, gchar *resolution, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	GError *error = NULL;

	gchar **envp;

	gchar *argv[50];
	gint argc;
	gint i;

	argc = 0;

	argv[argc++] = g_strdup("pyhoca-cli");
	argv[argc++] = g_strdup("--server");
	argv[argc++] = g_strdup_printf ("%s", host);
	argv[argc++] = g_strdup("-p");
	argv[argc++] = g_strdup_printf ("%d", sshport);
	argv[argc++] = g_strdup("-u");
	if (!username){
		argv[argc++] = g_strdup_printf ("%s", g_get_user_name());
	}else{
		argv[argc++] = g_strdup_printf ("%s", username);
	}
	if (password) {
		argv[argc++] = g_strdup("--password");
		argv[argc++] = g_strdup_printf ("%s", password);
	}
	argv[argc++] = g_strdup("-c");
//FIXME: pyhoca-cli is picky about multiple quotes around the command string...
//	argv[argc++] = g_strdup_printf ("%s", g_shell_quote(command));
	argv[argc++] = g_strdup(command);
	if (kbdlayout) {
		argv[argc++] = g_strdup("--kbd-layout");
		argv[argc++] = g_strdup_printf ("%s", kbdlayout);
	}
	if (kbdtype) {
		argv[argc++] = g_strdup("--kbd-type");
		argv[argc++] = g_strdup_printf ("%s", kbdtype);
	}else{
		argv[argc++] = g_strdup("--kbd-type");
		argv[argc++] = g_strdup("auto");
	}
	if (!resolution)
		resolution = "800x600";
	argv[argc++] = g_strdup("-g");
	argv[argc++] = g_strdup_printf ("%s", resolution);
	argv[argc++] = g_strdup("--try-resume");
	argv[argc++] = g_strdup("--terminate-on-ctrl-c");
	argv[argc++] = NULL;

	envp = g_get_environ();

	g_spawn_async (NULL, argv, envp, G_SPAWN_SEARCH_PATH, NULL, NULL, &gpdata->pidx2go, &error);

	if (error) {
		g_printf ("failed to start PyHoca CLI: %s\n", error->message);
		return FALSE;
	}
	for (i = 0; i < argc; i++)
		g_free (argv[i]);

	return TRUE;
}

static void remmina_plugin_x2go_on_plug_added(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	printf("[%s] remmina_plugin_x2go_on_plug_added socket %d\n", PLUGIN_NAME, gpdata->socket_id);
	remmina_plugin_x2go_service->protocol_plugin_emit_signal(gp, "connect");
	gpdata->ready = TRUE;
	return;
}

static void remmina_plugin_x2go_on_plug_removed(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	printf("[%s] remmina_plugin_x2go_on_plug_removed\n", PLUGIN_NAME);
	remmina_plugin_x2go_service->protocol_plugin_close_connection(gp);
}

static void remmina_plugin_x2go_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	printf("[%s] remmina_plugin_x2go_init\n", PLUGIN_NAME);
	RemminaPluginX2GoData *gpdata;

	gpdata = g_new0(RemminaPluginX2GoData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	gpdata->socket = gtk_socket_new();
	remmina_plugin_x2go_service->protocol_plugin_register_hostkey(gp, gpdata->socket);
	gtk_widget_show(gpdata->socket);

	g_signal_connect(G_OBJECT(gpdata->socket), "plug-added", G_CALLBACK(remmina_plugin_x2go_on_plug_added), gp);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-removed", G_CALLBACK(remmina_plugin_x2go_on_plug_removed), gp);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->socket);
}

static gboolean remmina_plugin_x2go_try_window_id(Window window_id)
{
	TRACE_CALL(__func__);
	gint i;
	gboolean already_seen = FALSE;

	printf("[%s] remmina_plugin_x2go_start_create_notify: check if X2Go Agent window [0x%lx] is already known or if it needs registration\n", PLUGIN_NAME, window_id);

	CANCEL_DEFER
	pthread_mutex_lock(&remmina_x2go_init_mutex);
	for (i = 0; i < remmina_x2go_window_id_array->len; i++) {
		if (g_array_index(remmina_x2go_window_id_array, Window, i) == window_id) {
			already_seen = TRUE;
			printf("[%s] remmina_plugin_x2go_try_window_id: X2Go Agent window with ID [0x%lx] already seen\n", PLUGIN_NAME, window_id);
			break;
		}
	}
	if (TRUE != already_seen) {
		g_array_append_val(remmina_x2go_window_id_array, window_id);
		printf("[%s] remmina_plugin_x2go_try_window_id: registered new X2Go Agent window with ID [0x%lx]\n", PLUGIN_NAME, window_id);
	}
	pthread_mutex_unlock(&remmina_x2go_init_mutex);
	CANCEL_ASYNC

	return (!already_seen);
}

static void
remmina_plugin_x2go_remove_window_id (Window window_id)
{
	gint i;
	gboolean already_seen = FALSE;

	CANCEL_DEFER
	pthread_mutex_lock (&remmina_x2go_init_mutex);
	for (i = 0; i < remmina_x2go_window_id_array->len; i++)
	{
		if (g_array_index (remmina_x2go_window_id_array, Window, i) == window_id)
		{
			already_seen = TRUE;
			printf("[%s] remmina_plugin_x2go_remove_window_id: X2Go Agent window with ID [0x%lx] already seen\n", PLUGIN_NAME, window_id);
			break;
		}
	}
	if (TRUE == already_seen)
	{
		g_array_remove_index_fast (remmina_x2go_window_id_array, i);
		printf("[%s] remmina_plugin_x2go_remove_window_id: forgetting about X2Go Agent window with ID [0x%lx]\n", PLUGIN_NAME, window_id);
	}
	pthread_mutex_unlock (&remmina_x2go_init_mutex);
	CANCEL_ASYNC
}

static int remmina_plugin_x2go_dummy_handler(Display *dsp, XErrorEvent *err)
{
	TRACE_CALL(__func__);
	return 0;
}

static gboolean remmina_plugin_x2go_start_create_notify(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);

	gpdata->display = XOpenDisplay(gdk_display_get_name(gdk_display_get_default()));
	if (gpdata->display == NULL)
		return FALSE;

	gpdata->orig_handler = XSetErrorHandler(remmina_plugin_x2go_dummy_handler);

	XSelectInput(gpdata->display, XDefaultRootWindow(gpdata->display), SubstructureNotifyMask);

	printf("[%s] remmina_plugin_x2go_start_create_notify: X11 event watcher created\n", PLUGIN_NAME);

	return TRUE;
}

static gboolean remmina_plugin_x2go_monitor_create_notify(RemminaProtocolWidget *gp, const gchar *cmd)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata;
	Atom atom;
	XEvent xev;
	Window w;
	Atom type;
	int format;
	unsigned long nitems, rest;
	unsigned char *data = NULL;
	struct timespec ts;

	CANCEL_DEFER

	printf("[%s] remmina_plugin_x2go_monitor_create_notify: waiting for X2Go Agent window to appear\n", PLUGIN_NAME);

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
			printf("[%s] remmina_plugin_x2go_monitor_create_notify: still XPending for this DISPLAY\n", PLUGIN_NAME);
			continue;
		}
		XNextEvent(gpdata->display, &xev);
		if (xev.type != CreateNotify) {
			printf("[%s] remmina_plugin_x2go_monitor_create_notify: saw an X11 event, but it wasn't CreateNotify\n", PLUGIN_NAME);
			continue;
		}
		w = xev.xcreatewindow.window;
		if (XGetWindowProperty(gpdata->display, w, atom, 0, 255, False, AnyPropertyType, &type, &format, &nitems, &rest,
			&data) != Success) {
			printf("[%s] remmina_plugin_x2go_monitor_create_notify: failed to get WM_COMMAND property from X11 window ID [%lx]\n", PLUGIN_NAME, w);
			continue;
	}
		if (data)
			printf("[%s] remmina_plugin_x2go_monitor_create_notify: found X11 window with WM_COMMAND set to '%s'\n", PLUGIN_NAME, (char*)data);
		if (data && strstr((char*)data, cmd) && remmina_plugin_x2go_try_window_id(w)) {
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

static gboolean remmina_plugin_x2go_start_session(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	printf("[%s] remmina_plugin_x2go_open_connection\n", PLUGIN_NAME);
#define GET_PLUGIN_STRING(value) \
		g_strdup(remmina_plugin_x2go_service->file_get_string(remminafile, value))
#define GET_PLUGIN_PASSWORD(value) \
		GET_PLUGIN_STRING(value)
#define GET_PLUGIN_INT(value, default_value) \
		remmina_plugin_x2go_service->file_get_int(remminafile, value, default_value)
#define GET_PLUGIN_BOOLEAN(value) \
		remmina_plugin_x2go_service->file_get_int(remminafile, value, FALSE)

	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);;
	RemminaFile *remminafile;
	GError *error = NULL;

	gchar *servstr, *host, *username, *password, *command, *kbdlayout, *kbdtype;
	gint sshport;
	GdkDisplay *default_dsp;

	/* We save the X Display name (:0) as we will need to synchronize the clipboards */
	default_dsp = gdk_display_get_default();
	const gchar *default_dsp_name = gdk_display_get_name(default_dsp);
	printf("[%s] Default display is %s\n", PLUGIN_NAME, default_dsp_name);

	remminafile = remmina_plugin_x2go_service->protocol_plugin_get_file(gp);

	servstr = GET_PLUGIN_STRING("server");
	if ( servstr ) {
		remmina_plugin_x2go_service->get_server_port(servstr, 22, &host, &sshport);
	} else {
		return FALSE;
	}
	if (!sshport)
		sshport=22;
	username = GET_PLUGIN_STRING("username");
	password = GET_PLUGIN_PASSWORD("password");
	command = GET_PLUGIN_STRING("command");
	if (!command)
		command = "TERMINAL";
	kbdlayout = GET_PLUGIN_STRING("kbdlayout");
	kbdtype = GET_PLUGIN_STRING("kbdtype");

	remmina_plugin_x2go_service->log_printf("[%s] attached window to socket %d\n", PLUGIN_NAME, gpdata->socket_id);

	if (!remmina_plugin_x2go_start_create_notify(gp))
		return FALSE;

	if (!remmina_plugin_x2go_exec_x2go(host, sshport, username, password, command, kbdlayout, kbdtype, NULL, gp)) {
		remmina_plugin_x2go_service->protocol_plugin_set_error(gp, "%s", error->message);
		return FALSE;
	}

	/* get the window id of the remote nxagent */
	if (!remmina_plugin_x2go_monitor_create_notify(gp, "x2goagent"))
		return FALSE;

	/* embed it */
	onMainThread_gtk_socket_add_id(GTK_SOCKET(gpdata->socket), gpdata->window_id);

	return TRUE;
}

static gboolean remmina_plugin_x2go_main(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	gboolean ret;
	const gchar *err;

	ret = remmina_plugin_x2go_start_session(gp);
//	if (!ret) {
//			err = remmina_x2go_session_get_error(gpdata->nx);
//			if (err) {
//			remmina_plugin_x2go_service->protocol_plugin_set_error(gp, "%s", err);
//		}
//	}

	gpdata->thread = 0;
	return ret;
}

static gpointer remmina_plugin_x2go_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	CANCEL_ASYNC
	if (!remmina_plugin_x2go_main((RemminaProtocolWidget*)data)) {
		IDLE_ADD((GSourceFunc)remmina_plugin_x2go_service->protocol_plugin_close_connection, data);
	}
	return NULL;
}

static gboolean remmina_plugin_x2go_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	gint width, height;

	if (!remmina_plugin_x2go_service->gtksocket_available()) {
		remmina_plugin_x2go_service->protocol_plugin_set_error(gp,
			_("Protocol %s is unavailable because required GtkSocket only works under X.org"),
			remmina_plugin_x2go.name);
		return FALSE;
	}

	width = remmina_plugin_x2go_service->get_profile_remote_width(gp);
	height = remmina_plugin_x2go_service->get_profile_remote_height(gp);

	gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));

	remmina_plugin_x2go_service->protocol_plugin_set_width(gp, gpdata->width);
	remmina_plugin_x2go_service->protocol_plugin_set_height(gp, gpdata->height);
	gtk_widget_set_size_request(GTK_WIDGET(gp), gpdata->width, gpdata->height);

	gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));
	if (pthread_create(&gpdata->thread, NULL, remmina_plugin_x2go_main_thread, gp)) {
		remmina_plugin_x2go_service->protocol_plugin_set_error(gp,
		        "Failed to initialize pthread. Falling back to non-thread mode...");
		gpdata->thread = 0;
		return FALSE;
	}else  {
		return TRUE;
	}
}

static gboolean remmina_plugin_x2go_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginX2GoData *gpdata = GET_PLUGIN_DATA(gp);
	if (gpdata->pidx2go) {
		kill(gpdata->pidx2go, SIGTERM);
		g_spawn_close_pid(gpdata->pidx2go);
		gpdata->pidx2go = 0;
	}

	if (gpdata->window_id) {
		remmina_plugin_x2go_remove_window_id(gpdata->window_id);
	}

	printf("[%s] remmina_plugin_x2go_close_connection\n", PLUGIN_NAME);
	remmina_plugin_x2go_service->protocol_plugin_emit_signal(gp, "disconnect");
	return FALSE;
}

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Unused pointer  --->    TODO: used for sensitive
 */
static const RemminaProtocolSetting remmina_plugin_x2go_basic_settings[] = {
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "password", N_("Password"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_COMBO, "command", N_("Startup program"), FALSE,
		"MATE,KDE,XFCE,LXDE,TERMINAL", NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "kbdlayout", N_("Keyboard Layout (auto)"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "kbdtype", N_("Keyboard type (auto)"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "showcursor", N_("Use local cursor"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "once", N_("Disconnect after one session"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin_x2go = {
	REMMINA_PLUGIN_TYPE_PROTOCOL,           // Type
	PLUGIN_NAME,                            // Name
	PLUGIN_DESCRIPTION,                     // Description
	GETTEXT_PACKAGE,                        // Translation domain
	PLUGIN_VERSION,                         // Version number
	PLUGIN_APPICON,                         // Icon for normal connection
	PLUGIN_APPICON,                         // Icon for SSH connection
	remmina_plugin_x2go_basic_settings,     // Array for basic settings
	NULL,                                   // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,    // SSH settings type
	/* REMMINA_PROTOCOL_SSH_SETTING_NONE,   // SSH settings type */
	NULL,                                   // Array for available features
	remmina_plugin_x2go_init,               // Plugin initialization
	remmina_plugin_x2go_open_connection,    // Plugin open connection
	remmina_plugin_x2go_close_connection,   // Plugin close connection
	NULL,                                   // Query for available features
	NULL,                                   // Call a feature
	NULL,                                   // Send a keystroke
	NULL,                                   // Screenshot
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL("remmina_plugin_entry");
	remmina_plugin_x2go_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_x2go)) {
		return FALSE;
	}

	pthread_mutex_init(&remmina_x2go_init_mutex, NULL);
	remmina_x2go_window_id_array = g_array_new(FALSE, TRUE, sizeof(Window));

	return TRUE;
}
