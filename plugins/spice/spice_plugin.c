/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2018 Denis Ollier
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

enum {
	REMMINA_PLUGIN_SPICE_FEATURE_PREF_VIEWONLY = 1,
	REMMINA_PLUGIN_SPICE_FEATURE_DYNRESUPDATE,
	REMMINA_PLUGIN_SPICE_FEATURE_PREF_DISABLECLIPBOARD,
	REMMINA_PLUGIN_SPICE_FEATURE_TOOL_SENDCTRLALTDEL,
	REMMINA_PLUGIN_SPICE_FEATURE_TOOL_USBREDIR,
	REMMINA_PLUGIN_SPICE_FEATURE_SCALE
};

RemminaPluginService *remmina_plugin_service = NULL;

static void remmina_plugin_spice_channel_new_cb(SpiceSession *, SpiceChannel *, RemminaProtocolWidget *);
static void remmina_plugin_spice_main_channel_event_cb(SpiceChannel *, SpiceChannelEvent, RemminaProtocolWidget *);
static void remmina_plugin_spice_agent_connected_event_cb(SpiceChannel *, RemminaProtocolWidget *);
static void remmina_plugin_spice_display_ready_cb(GObject *, GParamSpec *, RemminaProtocolWidget *);
static void remmina_plugin_spice_update_scale_mode(RemminaProtocolWidget *);
static gboolean remmina_plugin_spice_session_open_fd(RemminaProtocolWidget *gp, gint fd) __attribute__ ((unused));
static gboolean remmina_plugin_spice_channel_open_fd(SpiceChannel *, int with_tls, RemminaProtocolWidget *) __attribute__ ((unused));
//static gboolean remmina_plugin_spice_session_open_fd(RemminaProtocolWidget *gp, gint fd);
//static gboolean remmina_plugin_spice_channel_open_fd(SpiceChannel *, int with_tls, RemminaProtocolWidget *);

void remmina_plugin_spice_select_usb_devices(RemminaProtocolWidget *);
#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
void remmina_plugin_spice_file_transfer_new_cb(SpiceMainChannel *, SpiceFileTransferTask *, RemminaProtocolWidget *);
#  endif        /* SPICE_GTK_CHECK_VERSION(0, 31, 0) */
#endif          /* SPICE_GTK_CHECK_VERSION */

gchar* str_replace(const gchar *string, const gchar *search, const gchar *replacement)
{
	TRACE_CALL(__func__);
	gchar *str, **arr;

	g_return_val_if_fail(string != NULL, NULL);
	g_return_val_if_fail(search != NULL, NULL);

	if (replacement == NULL)
		replacement = "";

	arr = g_strsplit(string, search, -1);
	if (arr != NULL && arr[0] != NULL)
		str = g_strjoinv(replacement, arr);
	else
		str = g_strdup(string);

	g_strfreev(arr);
	return str;
}

static gboolean
remmina_plugin_spice_session_open_fd(RemminaProtocolWidget *gp, int fd)
{
	TRACE_CALL(__func__);
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	g_return_val_if_fail(gpdata != NULL, FALSE);

	REMMINA_PLUGIN_DEBUG("Opening spice session for FD: %d", fd);
	return spice_session_open_fd(gpdata->session, fd);
}

static gboolean
remmina_plugin_spice_channel_open_fd(SpiceChannel *channel, int with_tls G_GNUC_UNUSED, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	g_return_val_if_fail(gpdata != NULL, FALSE);

	gint id, type;

	g_object_get(channel, "channel-id", &id, "channel-type", &type, NULL);
	REMMINA_PLUGIN_DEBUG ("Opening channel %p %s %d with FD: %d", channel, g_type_name(G_OBJECT_TYPE(channel)), id, gpdata->fd);

	return spice_channel_open_fd(channel, gpdata->fd);
}

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
		"password", g_strdup(remmina_plugin_service->file_get_string(remminafile, "password")),
		"read-only", remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE),
		"enable-audio", remmina_plugin_service->file_get_int(remminafile, "enableaudio", FALSE),
		"enable-smartcard", remmina_plugin_service->file_get_int(remminafile, "sharesmartcard", FALSE),
		"shared-dir", remmina_plugin_service->file_get_string(remminafile, "sharefolder"),
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
	gchar *host, *tunnel;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	/* Setup SSH tunnel if needed */
	tunnel = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, XSPICE_DEFAULT_PORT, FALSE);

	if (!tunnel) {
		return FALSE;
	}

	/**-START- UNIX socket
	* if(strstr(g_strdup(tunnel), "unix:///") != NULL) {
	* 	REMMINA_PLUGIN_DEBUG("Tunnel contain unix:// -> %s", tunnel);
	* 	gchar *val = str_replace (tunnel, "unix://", "");
	* 	REMMINA_PLUGIN_DEBUG("tunnel after cleaning = %s", val);
	* 	//gchar *val = g_strdup(remmina_plugin_service->file_get_string(remminafile, "server"));
	* 	g_object_set(gpdata->session, "unix-path", val, NULL);
	* 	gint fd = remmina_plugin_service->open_unix_sock(val);
	* 	REMMINA_PLUGIN_DEBUG("Unix socket fd: %d", fd);
	* 	gpdata->fd = fd;
	* 	if (fd > 0)
	* 		remmina_plugin_spice_session_open_fd (gp, fd);
	* 	g_free(val);
	*
	* } else {
	*/


		remmina_plugin_service->get_server_port(tunnel,
				XSPICE_DEFAULT_PORT,
				&host,
				&port);

		g_object_set(gpdata->session, "host", host, NULL);
		g_free(host);
		g_free(tunnel);

		/* Unencrypted connection */
		if (!remmina_plugin_service->file_get_int(remminafile, "usetls", FALSE)) {
			g_object_set(gpdata->session, "port", g_strdup_printf("%i", port), NULL);
		}
		/* TLS encrypted connection */
		else{
			g_object_set(gpdata->session, "tls_port", g_strdup_printf("%i", port), NULL);

			/* Server CA certificate */
			cacert = remmina_plugin_service->file_get_string(remminafile, "cacert");
			if (cacert) {
				g_object_set(gpdata->session, "ca-file", cacert, NULL);
			}
		}

		spice_session_connect(gpdata->session);
	/** } -END- UNIX socket */

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

	if (gpdata->main_channel) {
		g_signal_handlers_disconnect_by_func(gpdata->main_channel,
			G_CALLBACK(remmina_plugin_spice_main_channel_event_cb),
			gp);
		g_signal_handlers_disconnect_by_func(gpdata->main_channel,
			G_CALLBACK(remmina_plugin_spice_agent_connected_event_cb),
			gp);
	}

	if (gpdata->session) {
		spice_session_disconnect(gpdata->session);
		g_object_unref(gpdata->session);
		gpdata->session = NULL;
		remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
	}

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
	if (gpdata->file_transfers) {
		g_hash_table_unref(gpdata->file_transfers);
	}
#  endif        /* SPICE_GTK_CHECK_VERSION(0, 31, 0) */
#endif          /* SPICE_GTK_CHECK_VERSION */

	return FALSE;
}

static gboolean remmina_plugin_spice_disable_gst_overlay(SpiceChannel *channel, void* pipeline_ptr, RemminaProtocolWidget *gp)
{
	g_signal_stop_emission_by_name(channel, "gst-video-overlay");
	return FALSE;
}

static void remmina_plugin_spice_channel_new_cb(SpiceSession *session, SpiceChannel *channel, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gint id, type;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	g_return_if_fail(gpdata != NULL);

	/**
	* g_signal_connect(channel,
	*	"open-fd",
	*	G_CALLBACK(remmina_plugin_spice_channel_open_fd),
	*	gp);
	*/

	g_object_get(channel, "channel-id", &id, "channel-type", &type, NULL);
	REMMINA_PLUGIN_DEBUG ("New spice channel %p %s %d", channel, g_type_name(G_OBJECT_TYPE(channel)), id);

	if (SPICE_IS_MAIN_CHANNEL(channel)) {
		gpdata->main_channel = SPICE_MAIN_CHANNEL(channel);
		g_signal_connect(channel,
			"channel-event",
			G_CALLBACK(remmina_plugin_spice_main_channel_event_cb),
			gp);
		g_signal_connect(channel,
			"main-agent-update",
			G_CALLBACK(remmina_plugin_spice_agent_connected_event_cb),
			gp);
#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
		g_signal_connect(channel,
			"new-file-transfer",
			G_CALLBACK(remmina_plugin_spice_file_transfer_new_cb),
			gp);
#  endif        /* SPICE_GTK_CHECK_VERSION(0, 31, 0) */
#endif          /* SPICE_GTK_CHECK_VERSION */
	}

	if (SPICE_IS_DISPLAY_CHANNEL(channel)) {
		gpdata->display_channel = SPICE_DISPLAY_CHANNEL(channel);
		gpdata->display = spice_display_new(gpdata->session, id);
		g_signal_connect(gpdata->display,
			"notify::ready",
			G_CALLBACK(remmina_plugin_spice_display_ready_cb),
			gp);
		remmina_plugin_spice_display_ready_cb(G_OBJECT(gpdata->display), NULL, gp);

		if (remmina_plugin_service->file_get_int(remminafile, "disablegstvideooverlay", FALSE)) {
			g_signal_connect(channel,
				"gst-video-overlay",
				G_CALLBACK(remmina_plugin_spice_disable_gst_overlay),
				gp);
		}

	}

	if (SPICE_IS_INPUTS_CHANNEL(channel)) {
		REMMINA_PLUGIN_DEBUG("New inputs channel");
	}
	if (SPICE_IS_PLAYBACK_CHANNEL(channel)) {
		REMMINA_PLUGIN_DEBUG("New audio channel");
		if (remmina_plugin_service->file_get_int(remminafile, "enableaudio", FALSE)) {
			gpdata->audio = spice_audio_get(gpdata->session, NULL);
		}
	}

	if (SPICE_IS_USBREDIR_CHANNEL(channel)) {
		REMMINA_PLUGIN_DEBUG("New usbredir channel");
	}

	if (SPICE_IS_WEBDAV_CHANNEL(channel)) {
		REMMINA_PLUGIN_DEBUG("New webdav channel");
		if (remmina_plugin_service->file_get_string(remminafile, "sharefolder")) {
			spice_channel_connect(channel);
		}
	}
}

static gboolean remmina_plugin_spice_ask_auth(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gint ret;
	gboolean disablepasswordstoring;
	gchar *s_password;
	gboolean save;

	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);

	ret = remmina_plugin_service->protocol_plugin_init_auth(gp,
		(disablepasswordstoring ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD),
		_("Enter SPICE password"),
		NULL,
		remmina_plugin_service->file_get_string(remminafile, "password"),
		NULL,
		NULL);
	if (ret == GTK_RESPONSE_OK) {
		s_password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save) {
			remmina_plugin_service->file_set_string(remminafile, "password", s_password);
		} else {
			remmina_plugin_service->file_set_string(remminafile, "password", NULL);
		}
	} else {
		return FALSE;
	}

	g_object_set(gpdata->session, "password", s_password, NULL);
	return TRUE;
}

static void remmina_plugin_spice_main_channel_event_cb(SpiceChannel *channel, SpiceChannelEvent event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gchar *server;
	gint port;
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	switch (event) {
	case SPICE_CHANNEL_CLOSED:
		remmina_plugin_service->get_server_port(remmina_plugin_service->file_get_string(remminafile, "server"),
			XSPICE_DEFAULT_PORT,
			&server,
			&port);
		remmina_plugin_service->protocol_plugin_set_error(gp, _("Disconnected from the SPICE server “%s”."), server);
		g_free(server);
		remmina_plugin_spice_close_connection(gp);
		break;
	case SPICE_CHANNEL_OPENED:
		break;
	case SPICE_CHANNEL_ERROR_AUTH:
		if (remmina_plugin_spice_ask_auth(gp)) {
			remmina_plugin_spice_open_connection(gp);
		}else{
			/* Connection is cancelled by the user by clicking cancel on auth panel, close it without showing errors */
			// remmina_plugin_service->protocol_plugin_set_error(gp, _("Invalid password."));
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
		remmina_plugin_service->protocol_plugin_set_error(gp, _("Connection to the SPICE server dropped."));
		remmina_plugin_spice_close_connection(gp);
		break;
	default:
		break;
	}
}

void remmina_plugin_spice_agent_connected_event_cb(SpiceChannel *channel, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	gboolean connected;

	g_object_get(channel,
		"agent-connected", &connected,
		NULL);

	if (connected) {
		remmina_plugin_service->protocol_plugin_unlock_dynres(gp);
	} else {
		remmina_plugin_service->protocol_plugin_lock_dynres(gp);
	}
}

static void remmina_plugin_spice_display_ready_cb(GObject *display, GParamSpec *param_spec, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gboolean ready;

	g_object_get(display, "ready", &ready, NULL);

	if (ready) {
		RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
		RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

		g_signal_handlers_disconnect_by_func(display,
			G_CALLBACK(remmina_plugin_spice_display_ready_cb),
			gp);

		RemminaScaleMode scaleMode = remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp);
		g_object_set(display,
			"scaling", (scaleMode  == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED),
			"resize-guest", (scaleMode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES),
			NULL);

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 34, 0)
		SpiceVideoCodecType videocodec = remmina_plugin_service->file_get_int(remminafile, "videocodec", 0);
		if (videocodec) {
#    if SPICE_GTK_CHECK_VERSION(0, 38, 0)
			GError *err = NULL;
			guint i;

			GArray *preferred_codecs = g_array_sized_new(FALSE, FALSE,
				sizeof(gint),
				(SPICE_VIDEO_CODEC_TYPE_ENUM_END - 1));

			g_array_append_val(preferred_codecs, videocodec);
			for (i = SPICE_VIDEO_CODEC_TYPE_MJPEG; i < SPICE_VIDEO_CODEC_TYPE_ENUM_END; ++i) {
				if (i != videocodec) {
					g_array_append_val(preferred_codecs, i);
				}
			}

			if (!spice_display_channel_change_preferred_video_codec_types(SPICE_CHANNEL(gpdata->display_channel),
					(gint *) preferred_codecs->data,
					preferred_codecs->len,
					&err)) {
				REMMINA_PLUGIN_DEBUG("Could not set video-codec preference. %s", err->message);
				g_error_free(err);
			}

			g_clear_pointer(&preferred_codecs, g_array_unref);

#    elif SPICE_GTK_CHECK_VERSION(0, 35, 0)
			spice_display_channel_change_preferred_video_codec_type(SPICE_CHANNEL(gpdata->display_channel),
					videocodec);
#    else
			spice_display_change_preferred_video_codec_type(SPICE_CHANNEL(gpdata->display_channel),
					videocodec);
#    endif
		}
#  endif
#endif

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
		SpiceImageCompression imagecompression = remmina_plugin_service->file_get_int(remminafile, "imagecompression", 0);
		if (imagecompression) {
#    if SPICE_GTK_CHECK_VERSION(0, 35, 0)
			spice_display_channel_change_preferred_compression(SPICE_CHANNEL(gpdata->display_channel),
				imagecompression);
#    else
			spice_display_change_preferred_compression(SPICE_CHANNEL(gpdata->display_channel),
				imagecompression);
#    endif
		}
#  endif
#endif

		gtk_container_add(GTK_CONTAINER(gp), GTK_WIDGET(display));
		gtk_widget_show(GTK_WIDGET(display));

		remmina_plugin_service->protocol_plugin_register_hostkey(gp, GTK_WIDGET(display));
		remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);
	}
}

/* Send a keystroke to the plugin window */
static void remmina_plugin_spice_keystroke(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL(__func__);
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);

	if (gpdata->display) {
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

static void remmina_plugin_spice_update_scale_mode(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gint width, height;
	RemminaPluginSpiceData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaScaleMode scaleMode = remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp);

	g_object_set(gpdata->display,
		"scaling", (scaleMode  == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED),
		"resize-guest", (scaleMode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES),
		NULL);

	if (scaleMode != REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE) {
		/* In scaled mode, the SpiceDisplay will get its dimensions from its parent */
		gtk_widget_set_size_request(GTK_WIDGET(gpdata->display), -1, -1 );
	}else {
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

	switch (feature->id) {
	case REMMINA_PLUGIN_SPICE_FEATURE_PREF_VIEWONLY:
		g_object_set(gpdata->session,
			"read-only",
			remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE),
			NULL);
		break;
	case REMMINA_PLUGIN_SPICE_FEATURE_PREF_DISABLECLIPBOARD:
		g_object_set(gpdata->gtk_session,
			"auto-clipboard",
			!remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE),
			NULL);
		break;
	case REMMINA_PLUGIN_SPICE_FEATURE_DYNRESUPDATE:
	case REMMINA_PLUGIN_SPICE_FEATURE_SCALE:
		remmina_plugin_spice_update_scale_mode(gp);
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

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 34, 0)
/* Array of key/value pairs for prefered video codec
 * Key - SpiceVideoCodecType (spice/enums.h)
 */
static gpointer videocodec_list[] =
{
	"0",	N_("Default"),
	"1",	"mjpeg",
	"2",	"vp8",
	"3",	"h264",
	"4",	"vp9",
	"5",	"h265",
	NULL
};
#  endif
#endif

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
/* Array of key/value pairs for prefered video codec
 * Key - SpiceImageCompression (spice/enums.h)
 */
static gpointer imagecompression_list[] =
{
	"0",	N_("Default"),
	"1",	N_("Off"),
	"2",	N_("Auto GLZ"),
	"3",	N_("Auto LZ"),
	"4",	"Quic",
	"5",	"GLZ",
	"6",	"LZ",
	"7",	"LZ4",
	NULL
};
#  endif
#endif

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 34, 0)
static gchar disablegstvideooverlay_tooltip[] =
	N_("Disable video overlay if videos are not displayed properly.\n");
#  endif
#endif

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_spice_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER,	  "server",	    NULL,					FALSE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "password",	    N_("User password"),			FALSE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "usetls",	    N_("Use TLS encryption"),			FALSE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_FILE,	  "cacert",	    N_("Server CA certificate"),		FALSE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_FOLDER,	  "sharefolder",    N_("Share folder"),				FALSE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,		    NULL,					FALSE,	NULL,	NULL}
};

/* Array of RemminaProtocolSetting for advanced settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_spice_advanced_settings[] =
{
#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 35, 0)
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	"videocodec",	    N_("Prefered video codec"),		FALSE, videocodec_list, NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"disablegstvideooverlay",	    N_("Turn off GStreamer overlay"),		FALSE,	NULL,	disablegstvideooverlay_tooltip},
#  endif
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	"imagecompression",	    N_("Prefered image compression"),		FALSE, imagecompression_list, NULL},
#  endif
#endif
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"disableclipboard",	    N_("No clipboard sync"),		TRUE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"disablepasswordstoring",   N_("Forget passwords after use"),		TRUE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"enableaudio",		    N_("Enable audio channel"),			TRUE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"sharesmartcard",	    N_("Share smart card"),			TRUE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"viewonly",		    N_("View only"),				TRUE,	NULL,	NULL},
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	NULL,			    NULL,					TRUE,	NULL,	NULL}
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_plugin_spice_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF,  REMMINA_PLUGIN_SPICE_FEATURE_PREF_VIEWONLY,	    GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK),	   "viewonly",	  N_("View only")},
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF,  REMMINA_PLUGIN_SPICE_FEATURE_PREF_DISABLECLIPBOARD,  GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK),	   "disableclipboard",	N_("No clipboard sync")},
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL,  REMMINA_PLUGIN_SPICE_FEATURE_TOOL_SENDCTRLALTDEL,    N_("Send Ctrl+Alt+Delete"),					   NULL,		NULL},
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL,  REMMINA_PLUGIN_SPICE_FEATURE_TOOL_USBREDIR,	    N_("Select USB devices for redirection"),			   NULL,		NULL},
	{ REMMINA_PROTOCOL_FEATURE_TYPE_DYNRESUPDATE,  REMMINA_PLUGIN_SPICE_FEATURE_DYNRESUPDATE,	    NULL,	   NULL,	NULL},
	{ REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, REMMINA_PLUGIN_SPICE_FEATURE_SCALE,		    NULL,							   NULL,		NULL},
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END,   0,						    NULL,							   NULL,		NULL}
};


static RemminaProtocolPlugin remmina_plugin_spice =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                                           // Type
	"SPICE",                                                                // Name
	N_("SPICE - Simple Protocol for Independent Computing Environments"),   // Description
	GETTEXT_PACKAGE,                                                        // Translation domain
	VERSION,                                                                // Version number
	"remmina-spice-symbolic",                                               // Icon for normal connection
	"remmina-spice-ssh-symbolic",                                               // Icon for SSH connection
	remmina_plugin_spice_basic_settings,                                    // Array for basic settings
	remmina_plugin_spice_advanced_settings,                                 // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,                                    // SSH settings type
	remmina_plugin_spice_features,                                          // Array for available features
	remmina_plugin_spice_init,                                              // Plugin initialization
	remmina_plugin_spice_open_connection,                                   // Plugin open connection
	remmina_plugin_spice_close_connection,                                  // Plugin close connection
	remmina_plugin_spice_query_feature,                                     // Query for available features
	remmina_plugin_spice_call_feature,                                      // Call a feature
	remmina_plugin_spice_keystroke,                                         // Send a keystroke
	NULL,                                                                   // No screenshot support available
	NULL,                                                                   // RCW map event
	NULL                                                                    // RCW unmap event
};

void remmina_plugin_spice_remove_list_option(gpointer *option_list, const gchar *option_to_remove) {
	gpointer *src, *dst;

	TRACE_CALL(__func__);

	dst = src = option_list;
	while (*src) {
		if (strcmp(*src, option_to_remove) != 0) {
			if (dst != src) {
				*dst = *src;
				*(dst + 1) = *(src + 1);
			}
			dst += 2;
		}
		src += 2;
	}
	*dst = NULL;
}

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
gboolean remmina_plugin_spice_is_lz4_supported() {
	gboolean result = FALSE;
	GOptionContext *context;
	GOptionGroup *spiceGroup;
	gchar *spiceHelp;

	TRACE_CALL(__func__);

	spiceGroup = spice_get_option_group();
	context = g_option_context_new("- SPICE client test application");
	g_option_context_add_group(context, spiceGroup);

	spiceHelp = g_option_context_get_help(context, FALSE, spiceGroup);
	if (g_strcmp0(spiceHelp, "") != 0) {
		gchar **spiceHelpLines, **line;
		spiceHelpLines = g_strsplit(spiceHelp, "\n", -1);

		for (line = spiceHelpLines; *line != NULL; ++line) {
			if (g_strstr_len(*line, -1, "spice-preferred-compression")) {
				if (g_strstr_len(*line, -1, ",lz4,")) {
					result = TRUE;
				}

				break;
			}
		}

		g_strfreev(spiceHelpLines);
	}
	g_option_context_free(context);
	g_free(spiceHelp);

	return result;
}
#  endif
#endif

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
	if (!remmina_plugin_spice_is_lz4_supported()) {
		char key_str[10];
		sprintf(key_str, "%d", SPICE_IMAGE_COMPRESSION_LZ4);
		remmina_plugin_spice_remove_list_option(imagecompression_list, key_str);
	}
#  endif
#endif

	if (!service->register_plugin((RemminaPlugin*)&remmina_plugin_spice)) {
		return FALSE;
	}

	return TRUE;
}

