/*
 *     Project: Remmina Plugin X2Go
 * Description: Remmina protocol plugin to connect via X2Go using PyHoca
 *              Based on Fabio Castelli Team Viewer Plugin
 *      Author: Antenore Gatta <antenore@simbiosi.org>
 *   Copyright: 2015 Antenore Gatta
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

#include "plugin_config.h"
#include <common/remmina_plugin.h>
#if GTK_VERSION == 3
# include <gtk/gtkx.h>
#endif
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <X11/extensions/XKBrules.h>


#define GET_PLUGIN_DATA(gp) (RemminaPluginData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

typedef struct _RemminaPluginData
{
	GtkWidget *socket;
	gint socket_id;
	GPid pid_xephyr;
	GPid pid_x2go;
	gboolean ready;
} RemminaPluginData;

static RemminaPluginService *remmina_plugin_service = NULL;

static gboolean remmina_plugin_exec_xephyr(gint socket_id, gint pid, gint width, gint height)
{
	TRACE_CALL("remmina_plugin_exec_xephyr");
	GError *error = NULL;
	gboolean ret;
	gchar *argv[50];
	gint argc;
	gint i;
	argc = 0;
	argv[argc++] = g_strdup("Xephyr");
	argv[argc++] = g_strdup_printf(":%d", socket_id);	/* We use the window id as our display numeber */
	argv[argc++] = g_strdup("-parent");
	argv[argc++] = g_strdup_printf("%d", socket_id);
	argv[argc++] = g_strdup("-screen");
	argv[argc++] = g_strdup_printf ("%dx%d", width, height);
	argv[argc++] = g_strdup("-resizeable");
	//argv[argc++] = g_strdup("-reset");
	//argv[argc++] = g_strdup("-terminate");
	argv[argc++] = g_strdup("-ac");
	argv[argc++] = NULL;
	ret = g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &pid, &error);
	for (i = 0; i < argc; i++)
		g_free (argv[i]);
	if (!ret)
	{
		return FALSE;
	}
	return TRUE;
}

static gboolean remmina_plugin_exec_x2go(gint pid, gchar *host, gint sshport, gchar *username, gchar *password,
		gchar *command, gchar *kbdlayout, gchar *kbdtype, gchar *resolution, gint disp)
{
	TRACE_CALL("remmina_plugin_exec_x2go");
	GError *error = NULL;
	gboolean ret;

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
	argv[argc++] = g_strdup_printf ("%s", username);
	argv[argc++] = g_strdup("--password");
	argv[argc++] = g_strdup_printf ("%s", password);
	argv[argc++] = g_strdup("-c");
	argv[argc++] = g_strdup_printf ("%s", g_shell_quote(command));
	argv[argc++] = g_strdup("--kbd-layout");
	argv[argc++] = g_strdup_printf ("%s", kbdlayout);
	argv[argc++] = g_strdup("--kbd-type");
	argv[argc++] = g_strdup_printf ("%s", kbdtype);
	argv[argc++] = g_strdup("-g");
	argv[argc++] = g_strdup_printf ("%s", resolution);
	argv[argc++] = NULL;

	envp = g_environ_setenv (
			g_get_environ (),
			g_strdup ("DISPLAY"),
			g_strdup_printf (":%d", disp),
			TRUE
			);

	ret = g_spawn_async (NULL, argv, envp, G_SPAWN_SEARCH_PATH, NULL, NULL, &pid, &error);

	for (i = 0; i < argc; i++)
		g_free (argv[i]);
	if (!ret)
	{
		return FALSE;
	}
	return TRUE;

}

static void remmina_plugin_on_plug_added(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_on_plug_added");
	RemminaPluginData *gpdata;
	gpdata = (RemminaPluginData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remmina_plugin_service->log_printf("[%s] remmina_plugin_on_plug_added socket %d\n", PLUGIN_NAME, gpdata->socket_id);
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");
	gpdata->ready = TRUE;
	return;
}

static void remmina_plugin_on_plug_removed(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_on_plug_removed");
	remmina_plugin_service->log_printf("[%s] remmina_plugin_on_plug_removed\n", PLUGIN_NAME);
	remmina_plugin_service->protocol_plugin_close_connection(gp);
}

static void remmina_plugin_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_init");
	remmina_plugin_service->log_printf("[%s] remmina_plugin_init\n", PLUGIN_NAME);
	RemminaPluginData *gpdata;

	gpdata = g_new0(RemminaPluginData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	gpdata->socket = gtk_socket_new();
	remmina_plugin_service->protocol_plugin_register_hostkey(gp, gpdata->socket);
	gtk_widget_show(gpdata->socket);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-added", G_CALLBACK(remmina_plugin_on_plug_added), gp);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-removed", G_CALLBACK(remmina_plugin_on_plug_removed), gp);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->socket);
}

static gboolean remmina_plugin_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_open_connection");
	remmina_plugin_service->log_printf("[%s] remmina_plugin_open_connection\n", PLUGIN_NAME);
#define GET_PLUGIN_STRING(value) \
		g_strdup(remmina_plugin_service->file_get_string(remminafile, value))
#define GET_PLUGIN_PASSWORD(value) \
		g_strdup(remmina_plugin_service->file_get_secret(remminafile, value));
#define GET_PLUGIN_INT(value, default_value) \
		remmina_plugin_service->file_get_int(remminafile, value, default_value)
#define GET_PLUGIN_BOOLEAN(value) \
		remmina_plugin_service->file_get_int(remminafile, value, FALSE)

	RemminaPluginData *gpdata;
	RemminaFile *remminafile;
	GError *error = NULL;

	gchar *s, *host, *username, *password, *command, *kbdlayout, *kbdtype, *res;
	gint sshport;
	gchar **scrsize;
	gint width, height;

	gpdata = (RemminaPluginData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	s = GET_PLUGIN_STRING("server");
	remmina_plugin_service->get_server_port(s, 22, &host, &sshport);
	username = GET_PLUGIN_STRING("username");
	password = GET_PLUGIN_PASSWORD("password");
	command = GET_PLUGIN_STRING("command");
	kbdlayout = GET_PLUGIN_STRING("kbdlayout");
	kbdtype = GET_PLUGIN_STRING("kbdtype");
	res = GET_PLUGIN_STRING("resolution");
	if (!res || !strchr(res, 'x'))
	{
		width = 640;
		height = 480;
	}
	else
	{
		scrsize = g_strsplit (res, "x", -1 );
		width = g_ascii_strtoull(scrsize[0], NULL, 0);
		height = g_ascii_strtoull(scrsize[1], NULL, 0);
	}
	remmina_plugin_service->protocol_plugin_set_width(gp, width);
	remmina_plugin_service->protocol_plugin_set_height(gp, height);
	gtk_widget_set_size_request(GTK_WIDGET(gp), width, height);
	gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));

	remmina_plugin_service->log_printf("[%s] Before spawn socket id is %d\n", PLUGIN_NAME, gpdata->socket_id);

	if (!remmina_plugin_exec_xephyr(gpdata->socket_id, gpdata->pid_xephyr, width, height))
	{
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s", error->message);
		return FALSE;
	}

	if (!remmina_plugin_exec_x2go(
				gpdata->pid_x2go,
				host,
				sshport,
				username,
				password,
				command,
				kbdlayout,
				kbdtype,
				res,
				gpdata->socket_id	/* We use the window id as the display number */
				))
	{
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s", error->message);
		return FALSE;
	}

	gtk_container_add(GTK_CONTAINER(gp), gpdata->socket);

	remmina_plugin_service->log_printf("[%s] attached window to socket %d\n", PLUGIN_NAME, gpdata->socket_id);
	return TRUE;
}

static gboolean remmina_plugin_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_close_connection");
	remmina_plugin_service->log_printf("[%s] remmina_plugin_close_connection\n", PLUGIN_NAME);
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");
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
static const RemminaProtocolSetting remmina_plugin_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
	//{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "sshport", N_("remote SSH port (default: 22)"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "command", N_("Remote command"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "kbdlayout", N_("Keyboard Layout (us)"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "kbdtype", N_("Keyboard type (pc105/us)"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,              // Type
	PLUGIN_NAME,                               // Name
	PLUGIN_DESCRIPTION,                        // Description
	GETTEXT_PACKAGE,                           // Translation domain
	PLUGIN_VERSION,                            // Version number
	PLUGIN_APPICON,                            // Icon for normal connection
	PLUGIN_APPICON,                            // Icon for SSH connection
	remmina_plugin_basic_settings,             // Array for basic settings
	NULL,                                      // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,       // SSH settings type
	/* REMMINA_PROTOCOL_SSH_SETTING_NONE,      // SSH settings type */
	NULL,                                      // Array for available features
	remmina_plugin_init,                       // Plugin initialization
	remmina_plugin_open_connection,            // Plugin open connection
	remmina_plugin_close_connection,           // Plugin close connection
	NULL,                                      // Query for available features
	NULL,                                      // Call a feature
	/*remmina_plugin_keystroke                   // Send a keystroke    */
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL("remmina_plugin_entry");
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin))
	{
		return FALSE;
	}

	return TRUE;
}
