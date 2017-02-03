/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016 Denis Ollier
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

#include "spice_plugin.h"

#define XSPICE_DEFAULT_PORT 5900

enum
{
	REMMINA_PLUGIN_SPICE_FEATURE_PREF_VIEWONLY = 1,
	REMMINA_PLUGIN_SPICE_FEATURE_PREF_RESIZEGUEST,
	REMMINA_PLUGIN_SPICE_FEATURE_PREF_DISABLECLIPBOARD,
	REMMINA_PLUGIN_SPICE_FEATURE_TOOL_SENDCTRLALTDEL,
	REMMINA_PLUGIN_SPICE_FEATURE_TOOL_USBREDIR,
	REMMINA_PLUGIN_SPICE_FEATURE_SCALE
};

static RemminaPluginService *remmina_plugin_service = NULL;

static void remmina_plugin_spice_channel_new_cb(SpiceSession *, SpiceChannel *, RemminaProtocolWidget *);
static void remmina_plugin_spice_main_channel_event_cb(SpiceChannel *, SpiceChannelEvent, RemminaProtocolWidget *);
static void remmina_plugin_spice_display_ready_cb(GObject *, GParamSpec *, RemminaProtocolWidget *);
static void remmina_plugin_spice_update_scale(RemminaProtocolWidget *);

void remmina_plugin_spice_select_usb_devices(RemminaProtocolWidget *);
#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
void remmina_plugin_spice_file_transfer_new_cb(SpiceMainChannel *, SpiceFileTransferTask *, RemminaProtocolWidget *);
#  endif /* SPICE_GTK_CHECK_VERSION(0, 31, 0) */
#endif /* SPICE_GTK_CHECK_VERSION */

static void remmina_plugin_spice_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	RemminaPluginSpiceData *gpdata;
	RemminaFile *remminafile;

	gpdata = g_new0(RemminaPluginSpiceData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	gpdata->session = spice_session_new();
	g_signal_connect(gpdata->session,
	                 "channel-new",
	                 G_CALLBACK(remmina_plugin_spice_channel_new_cb),
	                 gp);

	g_object_set(gpdata->session,
	             "password", remmina_plugin_service->file_get_secret(remminafile, "password"),
	             "read-only", remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE),
	             "enable-audio", remmina_plugin_service->file_get_int(remminafile, "enableaudio", FALSE),
	             "enable-smartcard", remmina_plugin_service->file_get_int(remminafile, "sharesmartcard", FALSE),
	             NULL);

	gpdata->gtk_session = spice_gtk_session_get(gpdata->session);
	g_object_set(gpdata->gtk_session,
	             "auto-clipboard",
	             !remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE),
	             NULL);
}

static gboolean remmina_plugin_spice_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gint port;
	const gchar *cacert;
	gchar *host;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	remmina_plugin_service->get_server_port(remmina_plugin_service->file_get_string(remminafile, "server"),
	                                        XSPICE_DEFAULT_PORT,
	                                        &host,
	                                        &port);

	g_object_set(gpdata->session, "host", host, NULL);
	g_free(host);

	/* Unencrypted connection */
	if (!remmina_plugin_service->file_get_int(remminafile, "usetls", FALSE))
	{
		g_object_set(gpdata->session, "port", g_strdup_printf("%i", port), NULL);
	}
	/* TLS encrypted connection */
	else
	{
		g_object_set(gpdata->session, "tls_port", g_strdup_printf("%i", port), NULL);

		/* Server CA certificate */
		cacert = remmina_plugin_service->file_get_string(remminafile, "cacert");
		if (cacert)
		{
			g_object_set(gpdata->session, "ca-file", cacert, NULL);
		}
	}

	spice_session_connect(gpdata->session);

	/*
	 * FIXME: Add a waiting loop until the g_signal "channel-event" occurs.
	 * If the event is SPICE_CHANNEL_OPENED, TRUE should be returned,
	 * otherwise FALSE should be returned.
	 */
	return TRUE;
}

static gboolean remmina_plugin_spice_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	if (gpdata->main_channel)
	{
		g_signal_handlers_disconnect_by_func(gpdata->main_channel,
		                                     G_CALLBACK(remmina_plugin_spice_main_channel_event_cb),
		                                     gp);
	}

	if (gpdata->session)
	{
		spice_session_disconnect(gpdata->session);
		g_object_unref(gpdata->session);
		gpdata->session = NULL;
		remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");
	}

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
	if (gpdata->file_transfers)
	{
		g_hash_table_unref(gpdata->file_transfers);
	}
#  endif /* SPICE_GTK_CHECK_VERSION(0, 31, 0) */
#endif /* SPICE_GTK_CHECK_VERSION */

	return FALSE;
}

static void remmina_plugin_spice_channel_new_cb(SpiceSession *session, SpiceChannel *channel, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gint id;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	g_object_get(channel, "channel-id", &id, NULL);

	if (SPICE_IS_MAIN_CHANNEL(channel))
	{
		gpdata->main_channel = SPICE_MAIN_CHANNEL(channel);
		g_signal_connect(channel,
		                 "channel-event",
		                 G_CALLBACK(remmina_plugin_spice_main_channel_event_cb),
		                 gp);
#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
		g_signal_connect(channel,
		                 "new-file-transfer",
		                 G_CALLBACK(remmina_plugin_spice_file_transfer_new_cb),
		                 gp);
#  endif /* SPICE_GTK_CHECK_VERSION(0, 31, 0) */
#endif /* SPICE_GTK_CHECK_VERSION */
	}

	if (SPICE_IS_DISPLAY_CHANNEL(channel))
	{
		gpdata->display_channel = SPICE_DISPLAY_CHANNEL(channel);
		gpdata->display = spice_display_new(gpdata->session, id);
		g_signal_connect(gpdata->display,
		                 "notify::ready",
		                 G_CALLBACK(remmina_plugin_spice_display_ready_cb),
		                 gp);
		remmina_plugin_spice_display_ready_cb(G_OBJECT(gpdata->display), NULL, gp);
	}

	if (SPICE_IS_PLAYBACK_CHANNEL(channel))
	{
		if (remmina_plugin_service->file_get_int(remminafile, "enableaudio", FALSE))
		{
			gpdata->audio = spice_audio_get(gpdata->session, NULL);
		}
	}
}

static gboolean remmina_plugin_spice_ask_auth(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gint ret;
	gboolean disablepasswordstoring;

	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);
	ret = remmina_plugin_service->protocol_plugin_init_authpwd(gp, REMMINA_AUTHPWD_TYPE_PROTOCOL, !disablepasswordstoring);

	if (ret == GTK_RESPONSE_OK)
	{
		g_object_set(gpdata->session,
		             "password",
		             remmina_plugin_service->protocol_plugin_init_get_password(gp),
		             NULL);

		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

static void remmina_plugin_spice_main_channel_event_cb(SpiceChannel *channel, SpiceChannelEvent event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gchar *server;
	gint port;
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	switch (event)
	{
		case SPICE_CHANNEL_CLOSED:
			remmina_plugin_service->get_server_port(remmina_plugin_service->file_get_string(remminafile, "server"),
			                                        XSPICE_DEFAULT_PORT,
			                                        &server,
			                                        &port);
			remmina_plugin_service->protocol_plugin_set_error(gp, _("Disconnected from SPICE server %s."), server);
			g_free(server);
			remmina_plugin_spice_close_connection(gp);
			break;
		case SPICE_CHANNEL_OPENED:
			break;
		case SPICE_CHANNEL_ERROR_AUTH:
			if (remmina_plugin_spice_ask_auth(gp))
			{
				remmina_plugin_spice_open_connection(gp);
			}
			else
			{
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Invalid password."));
				remmina_plugin_spice_close_connection(gp);
			}
			break;
		case SPICE_CHANNEL_ERROR_TLS:
			remmina_plugin_service->protocol_plugin_set_error(gp, _("TLS connection error."));
			remmina_plugin_spice_close_connection(gp);
			break;
		case SPICE_CHANNEL_ERROR_IO:
		case SPICE_CHANNEL_ERROR_LINK:
		case SPICE_CHANNEL_ERROR_CONNECT:
			remmina_plugin_service->protocol_plugin_set_error(gp, _("Connection to SPICE server failed."));
			remmina_plugin_spice_close_connection(gp);
			break;
		default:
			break;
	}
}

static void remmina_plugin_spice_display_ready_cb(GObject *display, GParamSpec *param_spec, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gboolean ready;
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	g_object_get(display, "ready", &ready, NULL);

	if (ready)
	{
		g_signal_handlers_disconnect_by_func(display,
		                                     G_CALLBACK(remmina_plugin_spice_display_ready_cb),
		                                     gp);

		g_object_set(display,
		             "scaling", remmina_plugin_service->protocol_plugin_get_scale(gp),
		             "resize-guest", remmina_plugin_service->file_get_int(remminafile, "resizeguest", FALSE),
		             NULL);
		gtk_container_add(GTK_CONTAINER(gp), GTK_WIDGET(display));
		gtk_widget_show(GTK_WIDGET(display));

		remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");
	}
}

/* Send a keystroke to the plugin window */
static void remmina_plugin_spice_keystroke(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL(__func__);
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	if (gpdata->display)
	{
		spice_display_send_keys(gpdata->display,
		                        keystrokes,
		                        keylen,
		                        SPICE_DISPLAY_KEY_EVENT_CLICK);
	}
}

/* Send CTRL+ALT+DEL keys keystrokes to the plugin socket widget */
static void remmina_plugin_spice_send_ctrlaltdel(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	guint keys[] = { GDK_KEY_Control_L, GDK_KEY_Alt_L, GDK_KEY_Delete };

	remmina_plugin_spice_keystroke(gp, keys, G_N_ELEMENTS(keys));
}

static void remmina_plugin_spice_update_scale(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gint scale, width, height;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	scale = remmina_plugin_service->file_get_int(remminafile, "scale", FALSE);
	g_object_set(gpdata->display, "scaling", scale, NULL);

	if (scale)
	{
		/* In scaled mode, the SpiceDisplay will get its dimensions from its parent */
		gtk_widget_set_size_request(GTK_WIDGET(gpdata->display), -1, -1 );
	}
	else
	{
		/* In non scaled mode, the plugins forces dimensions of the SpiceDisplay */
		g_object_get(gpdata->display_channel,
		             "width", &width,
		             "height", &height,
		             NULL);
		gtk_widget_set_size_request(GTK_WIDGET(gpdata->display), width, height);
	}
}

static gboolean remmina_plugin_spice_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);

	return TRUE;
}

static void remmina_plugin_spice_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);

	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	switch (feature->id)
	{
		case REMMINA_PLUGIN_SPICE_FEATURE_PREF_VIEWONLY:
			g_object_set(gpdata->session,
			             "read-only",
			             remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE),
			             NULL);
			break;
		case REMMINA_PLUGIN_SPICE_FEATURE_PREF_RESIZEGUEST:
			g_object_set(gpdata->display,
			             "resize-guest",
			             remmina_plugin_service->file_get_int(remminafile, "resizeguest", TRUE),
			             NULL);
			break;
		case REMMINA_PLUGIN_SPICE_FEATURE_PREF_DISABLECLIPBOARD:
			g_object_set(gpdata->gtk_session,
			             "auto-clipboard",
			             !remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE),
			             NULL);
			break;
		case REMMINA_PLUGIN_SPICE_FEATURE_SCALE:
			remmina_plugin_spice_update_scale(gp);
			break;
		case REMMINA_PLUGIN_SPICE_FEATURE_TOOL_SENDCTRLALTDEL:
			remmina_plugin_spice_send_ctrlaltdel(gp);
			break;
		case REMMINA_PLUGIN_SPICE_FEATURE_TOOL_USBREDIR:
			remmina_plugin_spice_select_usb_devices(gp);
			break;
		default:
			break;
	}
}

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Unused pointer
 */
static const RemminaProtocolSetting remmina_plugin_spice_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "resizeguest", N_("Resize guest to match window size"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "usetls", N_("Use TLS encryption"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_FILE,  "cacert", N_("Server CA certificate"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Array of RemminaProtocolSetting for advanced settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Unused pointer
 */
static const RemminaProtocolSetting remmina_plugin_spice_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "multimonitor", N_("Multi monitor support"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "viewonly", N_("View only"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard", N_("Disable clipboard sync"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disablepasswordstoring", N_("Disable password storing"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enableaudio", N_("Enable audio channel"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "sharesmartcard", N_("Share smartcard"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_plugin_spice_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF, REMMINA_PLUGIN_SPICE_FEATURE_PREF_VIEWONLY, GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "viewonly", N_("View only") },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF, REMMINA_PLUGIN_SPICE_FEATURE_PREF_RESIZEGUEST, GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "resizeguest", N_("Resize guest to match window size") },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF, REMMINA_PLUGIN_SPICE_FEATURE_PREF_DISABLECLIPBOARD, GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "disableclipboard", N_("Disable clipboard sync") },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SPICE_FEATURE_TOOL_SENDCTRLALTDEL, N_("Send Ctrl+Alt+Delete"), NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SPICE_FEATURE_TOOL_USBREDIR, N_("Select USB devices for redirection"), NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, REMMINA_PLUGIN_SPICE_FEATURE_SCALE, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

static RemminaProtocolPlugin remmina_plugin_spice =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                                         // Type
	"SPICE",                                                              // Name
	N_("SPICE - Simple Protocol for Independent Computing Environments"), // Description
	GETTEXT_PACKAGE,                                                      // Translation domain
	VERSION,                                                              // Version number
	"remmina-spice",                                                      // Icon for normal connection
	"remmina-spice",                                                      // Icon for SSH connection
	remmina_plugin_spice_basic_settings,                                  // Array for basic settings
	remmina_plugin_spice_advanced_settings,                               // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_NONE,                                    // SSH settings type
	remmina_plugin_spice_features,                                        // Array for available features
	remmina_plugin_spice_init,                                            // Plugin initialization
	remmina_plugin_spice_open_connection,                                 // Plugin open connection
	remmina_plugin_spice_close_connection,                                // Plugin close connection
	remmina_plugin_spice_query_feature,                                   // Query for available features
	remmina_plugin_spice_call_feature,                                    // Call a feature
	remmina_plugin_spice_keystroke,                                       // Send a keystroke
	NULL                                                                  // Screenshot
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_spice))
	{
		return FALSE;
	}

	return TRUE;
}
