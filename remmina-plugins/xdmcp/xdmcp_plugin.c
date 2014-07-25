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

#include "common/remmina_plugin.h"
#if GTK_VERSION == 3
#  include <gtk/gtkx.h>
#endif

INCLUDE_GET_AVAILABLE_XDISPLAY

typedef struct _RemminaPluginXdmcpData
{
	GtkWidget *socket;
	gint socket_id;
	GPid pid;
	gint output_fd;
	gint error_fd;
	gint display;
	gboolean ready;

#ifdef HAVE_PTHREAD
	pthread_t thread;
#else
	gint thread;
#endif
} RemminaPluginXdmcpData;

static RemminaPluginService *remmina_plugin_service = NULL;

static void remmina_plugin_xdmcp_on_plug_added(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	RemminaPluginXdmcpData *gpdata;

	gpdata = (RemminaPluginXdmcpData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");
	gpdata->ready = TRUE;
}

static void remmina_plugin_xdmcp_on_plug_removed(GtkSocket *socket, RemminaProtocolWidget *gp)
{
	remmina_plugin_service->protocol_plugin_close_connection(gp);
}

static gboolean remmina_plugin_xdmcp_start_xephyr(RemminaProtocolWidget *gp)
{
	RemminaPluginXdmcpData *gpdata;
	RemminaFile *remminafile;
	gchar *argv[50];
	gint argc;
	gchar *host;
	gint i;
	GError *error = NULL;
	gboolean ret;

	gpdata = (RemminaPluginXdmcpData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	gpdata->display = remmina_get_available_xdisplay();
	if (gpdata->display == 0)
	{
		remmina_plugin_service->protocol_plugin_set_error(gp, "Run out of available local X display number.");
		return FALSE;
	}

	argc = 0;
	argv[argc++] = g_strdup("Xephyr");

	argv[argc++] = g_strdup_printf(":%i", gpdata->display);

	argv[argc++] = g_strdup("-parent");
	argv[argc++] = g_strdup_printf("%i", gpdata->socket_id);

	/* All Xephyr version between 1.5.0 and 1.6.4 will break when -screen argument is specified with -parent.
	 * It's not possible to support color depth if you have those Xephyr version. Please see this bug
	 * http://bugs.freedesktop.org/show_bug.cgi?id=24144
	 * As a workaround, a "Default" color depth will not add the -screen argument.
	 */
	i = remmina_plugin_service->file_get_int(remminafile, "colordepth", 8);
	if (i >= 8)
	{
		argv[argc++] = g_strdup("-screen");
		argv[argc++] = g_strdup_printf("%ix%ix%i",
				remmina_plugin_service->file_get_int(remminafile, "resolution_width", 640),
				remmina_plugin_service->file_get_int(remminafile, "resolution_height", 480), i);
	}

	if (i == 2)
	{
		argv[argc++] = g_strdup("-grayscale");
	}

	if (remmina_plugin_service->file_get_int(remminafile, "showcursor", FALSE))
	{
		argv[argc++] = g_strdup("-host-cursor");
	}
	if (remmina_plugin_service->file_get_int(remminafile, "once", FALSE))
	{
		argv[argc++] = g_strdup("-once");
	}

	if (!remmina_plugin_service->file_get_int(remminafile, "ssh_enabled", FALSE))
	{
		remmina_plugin_service->get_server_port(remmina_plugin_service->file_get_string(remminafile, "server"), 0,
				&host, &i);

		argv[argc++] = g_strdup("-query");
		argv[argc++] = host;

		if (i)
		{
			argv[argc++] = g_strdup("-port");
			argv[argc++] = g_strdup_printf("%i", i);
		}
	}
	else
	{
		/* When the connection is through an SSH tunnel, it connects back to local unix socket,
		 * so for security we can disable tcp listening */
		argv[argc++] = g_strdup("-nolisten");
		argv[argc++] = g_strdup("tcp");

		/* FIXME: It's better to get the magic cookie back from xqproxy, then call xauth,
		 * instead of disable access control */
		argv[argc++] = g_strdup("-ac");
	}

	argv[argc++] = NULL;

	ret = g_spawn_async(NULL, argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL, &gpdata->pid, &error);
	for (i = 0; i < argc; i++)
		g_free(argv[i]);

	if (!ret)
	{
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s", error->message);
		return FALSE;
	}

	return TRUE;
}

static gboolean remmina_plugin_xdmcp_tunnel_init_callback(RemminaProtocolWidget *gp, gint remotedisplay, const gchar *server,
		gint port)
{
	RemminaPluginXdmcpData *gpdata;
	RemminaFile *remminafile;

	gpdata = (RemminaPluginXdmcpData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (!remmina_plugin_xdmcp_start_xephyr(gp))
		return FALSE;
	while (!gpdata->ready)
		sleep(1);

	remmina_plugin_service->protocol_plugin_set_display(gp, gpdata->display);

	if (remmina_plugin_service->file_get_string(remminafile, "exec"))
	{
		return remmina_plugin_service->protocol_plugin_ssh_exec(gp, FALSE, "DISPLAY=localhost:%i.0 %s", remotedisplay,
				remmina_plugin_service->file_get_string(remminafile, "exec"));
	}
	else
	{
		return remmina_plugin_service->protocol_plugin_ssh_exec(gp, TRUE,
				"xqproxy -display %i -host %s -port %i -query -manage", remotedisplay, server, port);
	}
}

static gboolean remmina_plugin_xdmcp_main(RemminaProtocolWidget *gp)
{
	RemminaPluginXdmcpData *gpdata;
	RemminaFile *remminafile;

	gpdata = (RemminaPluginXdmcpData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (remmina_plugin_service->file_get_int(remminafile, "ssh_enabled", FALSE))
	{
		if (!remmina_plugin_service->protocol_plugin_start_xport_tunnel(gp, remmina_plugin_xdmcp_tunnel_init_callback))
		{
			gpdata->thread = 0;
			return FALSE;
		}
	}
	else
	{
		if (!remmina_plugin_xdmcp_start_xephyr(gp))
		{
			gpdata->thread = 0;
			return FALSE;
		}
	}

	gpdata->thread = 0;
	return TRUE;
}

#ifdef HAVE_PTHREAD
static gpointer
remmina_plugin_xdmcp_main_thread (gpointer data)
{
	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

	CANCEL_ASYNC
	if (!remmina_plugin_xdmcp_main ((RemminaProtocolWidget*) data))
	{
		IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, data);
	}
	return NULL;
}
#endif

static void remmina_plugin_xdmcp_init(RemminaProtocolWidget *gp)
{
	RemminaPluginXdmcpData *gpdata;

	gpdata = g_new0(RemminaPluginXdmcpData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	gpdata->socket = gtk_socket_new();
	remmina_plugin_service->protocol_plugin_register_hostkey(gp, gpdata->socket);
	gtk_widget_show(gpdata->socket);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-added", G_CALLBACK(remmina_plugin_xdmcp_on_plug_added), gp);
	g_signal_connect(G_OBJECT(gpdata->socket), "plug-removed", G_CALLBACK(remmina_plugin_xdmcp_on_plug_removed), gp);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->socket);
}

static gboolean remmina_plugin_xdmcp_open_connection(RemminaProtocolWidget *gp)
{
	RemminaPluginXdmcpData *gpdata;
	RemminaFile *remminafile;
	gint width, height;

	gpdata = (RemminaPluginXdmcpData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	width = remmina_plugin_service->file_get_int(remminafile, "resolution_width", 640);
	height = remmina_plugin_service->file_get_int(remminafile, "resolution_height", 480);
	remmina_plugin_service->protocol_plugin_set_width(gp, width);
	remmina_plugin_service->protocol_plugin_set_height(gp, height);
	gtk_widget_set_size_request(GTK_WIDGET(gp), width, height);
	gpdata->socket_id = gtk_socket_get_id(GTK_SOCKET(gpdata->socket));

#ifdef HAVE_PTHREAD

	if (remmina_plugin_service->file_get_int (remminafile, "ssh_enabled", FALSE))
	{
		if (pthread_create (&gpdata->thread, NULL, remmina_plugin_xdmcp_main_thread, gp))
		{
			remmina_plugin_service->protocol_plugin_set_error (gp,
					"Failed to initialize pthread. Falling back to non-thread mode...");
			gpdata->thread = 0;
			return FALSE;
		}
		else
		{
			return TRUE;
		}
	}
	else
	{
		return remmina_plugin_xdmcp_main (gp);
	}

#else

	return remmina_plugin_xdmcp_main(gp);

#endif
}

static gboolean remmina_plugin_xdmcp_close_connection(RemminaProtocolWidget *gp)
{
	RemminaPluginXdmcpData *gpdata;

	gpdata = (RemminaPluginXdmcpData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

#ifdef HAVE_PTHREAD
	if (gpdata->thread)
	{
		pthread_cancel (gpdata->thread);
		if (gpdata->thread) pthread_join (gpdata->thread, NULL);
	}
#endif

	if (gpdata->pid)
	{
		kill(gpdata->pid, SIGTERM);
		g_spawn_close_pid(gpdata->pid);
		gpdata->pid = 0;
	}

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");

	return FALSE;
}

static gboolean remmina_plugin_xdmcp_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	return FALSE;
}

static void remmina_plugin_xdmcp_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
}

static gpointer colordepth_list[] =
{ "0", N_("Default"), "2", N_("Grayscale"), "8", N_("256 colors"), "16", N_("High color (16 bit)"), "24", N_(
		"True color (24 bit)"), NULL };

static const RemminaProtocolSetting remmina_plugin_xdmcp_basic_settings[] =
{
{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION, NULL, NULL, FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "colordepth", N_("Color depth"), FALSE, colordepth_list, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "exec", N_("Startup program"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "showcursor", N_("Use local cursor"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "once", N_("Disconnect after one session"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL } };

static RemminaProtocolPlugin remmina_plugin_xdmcp =
{ REMMINA_PLUGIN_TYPE_PROTOCOL, "XDMCP", N_("XDMCP - X Remote Session"), GETTEXT_PACKAGE, VERSION,

"remmina-xdmcp", "remmina-xdmcp-ssh", remmina_plugin_xdmcp_basic_settings, NULL, REMMINA_PROTOCOL_SSH_SETTING_TUNNEL, NULL,

remmina_plugin_xdmcp_init, remmina_plugin_xdmcp_open_connection, remmina_plugin_xdmcp_close_connection,
		remmina_plugin_xdmcp_query_feature, remmina_plugin_xdmcp_call_feature };

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_xdmcp))
	{
		return FALSE;
	}

	return TRUE;
}

