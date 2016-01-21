/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_file.h"
#include "rdp_graphics.h"
#include "rdp_settings.h"
#include "rdp_cliprdr.h"
#include "rdp_channels.h"

#include <errno.h>
#include <pthread.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/error.h>
#include <winpr/memory.h>

#define REMMINA_RDP_FEATURE_TOOL_REFRESH         1
#define REMMINA_RDP_FEATURE_SCALE                2
#define REMMINA_RDP_FEATURE_UNFOCUS              3
#define REMMINA_RDP_FEATURE_TOOL_SENDCTRLALTDEL  4

/* Some string settings of freerdp are preallocated buffers of N bytes */
#define FREERDP_CLIENTHOSTNAME_LEN	32

RemminaPluginService* remmina_plugin_service = NULL;
static char remmina_rdp_plugin_default_drive_name[]="RemminaDisk";

static BOOL rf_process_event_queue(RemminaProtocolWidget* gp)
{
	TRACE_CALL("rf_process_event_queue");
	UINT16 flags;
	rdpInput* input;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent* event;

	if (rfi->event_queue == NULL)
		return True;

	input = rfi->instance->input;

	while ((event =(RemminaPluginRdpEvent*) g_async_queue_try_pop(rfi->event_queue)) != NULL)
	{
		switch (event->type)
		{
		case REMMINA_RDP_EVENT_TYPE_SCANCODE:
			flags = event->key_event.extended ? KBD_FLAGS_EXTENDED : 0;
			flags |= event->key_event.up ? KBD_FLAGS_RELEASE : KBD_FLAGS_DOWN;
			input->KeyboardEvent(input, flags, event->key_event.key_code);
			break;

		case REMMINA_RDP_EVENT_TYPE_MOUSE:
			input->MouseEvent(input, event->mouse_event.flags,
							  event->mouse_event.x, event->mouse_event.y);
			break;
		}

		g_free(event);
	}

	return True;
}

int rf_queue_ui(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("rf_queue_ui");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	gboolean ui_sync_save;
	int rc;

	ui_sync_save = ui->sync;

	if (ui_sync_save) {
		pthread_mutex_init(&ui->sync_wait_mutex,NULL);
		pthread_mutex_lock(&ui->sync_wait_mutex);
	}

	LOCK_BUFFER(TRUE)
			g_async_queue_push(rfi->ui_queue, ui);

	if (remmina_plugin_service->is_main_thread()) {
		/* in main thread we call directly */
		UNLOCK_BUFFER(TRUE)
				remmina_rdp_event_queue_ui(gp);
	} else {
		/* in a subthread, we schedule the call, if not already scheduled */
		if (!rfi->ui_handler)
			rfi->ui_handler = IDLE_ADD((GSourceFunc) remmina_rdp_event_queue_ui, gp);
		UNLOCK_BUFFER(TRUE)
	}

	if (ui_sync_save) {
		/* Wait for main thread function completion before returning */
		pthread_mutex_lock(&ui->sync_wait_mutex);
		pthread_mutex_unlock(&ui->sync_wait_mutex);
		rc = ui->retval;
		rf_object_free(gp, ui);
		return rc;
	}
	return 0;
}

void rf_object_free(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* obj)
{
	TRACE_CALL("rf_object_free");

	g_free(obj);
}

static BOOL rf_begin_paint(rdpContext* context)
{
	TRACE_CALL("rf_begin_paint");
	rdpGdi* gdi;
	HGDI_WND hwnd;

	if (!context)
		return FALSE;

	gdi = context->gdi;
	if (!gdi || !gdi->primary || !gdi->primary->hdc || !gdi->primary->hdc->hwnd)
		return FALSE;

	hwnd = gdi->primary->hdc->hwnd;
	if (!hwnd->ninvalid)
		return FALSE;

	hwnd->invalid->null = 1;
	hwnd->ninvalid = 0;
	return TRUE;
}

static BOOL rf_end_paint(rdpContext* context)
{
	TRACE_CALL("rf_end_paint");
	INT32 x1 = INT_MAX, y1 = INT_MAX, x2 = 0, y2 = 0, i, ninvalid;
	HGDI_RGN cinvalid;
	rdpGdi* gdi;
	rfContext* rfi = (rfContext*) context;
	RemminaPluginRdpUiObject* ui;

	if (!context || !rfi)
		return FALSE;

	gdi = context->gdi;
	if (!gdi || !gdi->primary || !gdi->primary->hdc || !gdi->primary->hdc->hwnd)
		return FALSE;

	if (gdi->primary->hdc->hwnd->invalid->null)
		return FALSE;

	ninvalid = gdi->primary->hdc->hwnd->ninvalid;
	cinvalid = gdi->primary->hdc->hwnd->cinvalid;

	for (i=0; i < ninvalid; i++)
	{
		x1 = MIN(x1, cinvalid[i].x);
		y1 = MIN(x1, cinvalid[i].y);
		x2 = MAX(x2, cinvalid[i].x + cinvalid[i].w);
		y2 = MAX(y2, cinvalid[i].y + cinvalid[i].h);
	}

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_UPDATE_REGION;
	ui->region.x = x1;
	ui->region.y = y1;
	ui->region.width = x2 - x1;
	ui->region.height = y2 - y1;

	rf_queue_ui(rfi->protocol_widget, ui);

	return TRUE;
}

static BOOL rf_desktop_resize(rdpContext* context)
{
	TRACE_CALL("rf_desktop_resize");
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	RemminaPluginRdpUiObject* ui;

	rfi = (rfContext*) context;

	if (!rfi)
		return FALSE;

	gp = rfi->protocol_widget;
	if (!gp)
		return FALSE;

	LOCK_BUFFER(TRUE);
	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->DesktopWidth);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->DesktopHeight);
	UNLOCK_BUFFER(TRUE);

	/* Call to remmina_rdp_event_update_scale(gp) on the main UI thread */
	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->sync = TRUE;	// Wait for completion too
	ui->type = REMMINA_RDP_UI_EVENT;
	ui->event.type = REMMINA_RDP_UI_EVENT_UPDATE_SCALE;
	rf_queue_ui(gp, ui);

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "desktop-resize");

	return TRUE;
}

static BOOL remmina_rdp_pre_connect(freerdp* instance)
{
	TRACE_CALL("remmina_rdp_pre_connect");
	ALIGN64 rdpSettings* settings;
	rdpChannels *channels;

	if (!instance || !instance->context)
		return FALSE;

	settings = instance->settings;
	channels = instance->context->channels;
	if (!settings || !channels)
		return FALSE;

	settings->OsMajorType = OSMAJORTYPE_UNIX;
	settings->OsMinorType = OSMINORTYPE_NATIVE_XSERVER;

	ZeroMemory(settings->OrderSupport, 32);
	settings->OrderSupport[NEG_DSTBLT_INDEX] = TRUE;
	settings->OrderSupport[NEG_PATBLT_INDEX] = TRUE;
	settings->OrderSupport[NEG_SCRBLT_INDEX] = TRUE;
	settings->OrderSupport[NEG_OPAQUE_RECT_INDEX] = TRUE;
	settings->OrderSupport[NEG_DRAWNINEGRID_INDEX] = FALSE;
	settings->OrderSupport[NEG_MULTIDSTBLT_INDEX] = FALSE;
	settings->OrderSupport[NEG_MULTIPATBLT_INDEX] = FALSE;
	settings->OrderSupport[NEG_MULTISCRBLT_INDEX] = FALSE;
	settings->OrderSupport[NEG_MULTIOPAQUERECT_INDEX] = TRUE;
	settings->OrderSupport[NEG_MULTI_DRAWNINEGRID_INDEX] = FALSE;
	settings->OrderSupport[NEG_LINETO_INDEX] = TRUE;
	settings->OrderSupport[NEG_POLYLINE_INDEX] = TRUE;
	settings->OrderSupport[NEG_MEMBLT_INDEX] = settings->BitmapCacheEnabled;
	settings->OrderSupport[NEG_MEM3BLT_INDEX] = (settings->SoftwareGdi) ? TRUE : FALSE;
	settings->OrderSupport[NEG_MEMBLT_V2_INDEX] = settings->BitmapCacheEnabled;
	settings->OrderSupport[NEG_MEM3BLT_V2_INDEX] = FALSE;
	settings->OrderSupport[NEG_SAVEBITMAP_INDEX] = FALSE;
	settings->OrderSupport[NEG_GLYPH_INDEX_INDEX] = TRUE;
	settings->OrderSupport[NEG_FAST_INDEX_INDEX] = TRUE;
	settings->OrderSupport[NEG_FAST_GLYPH_INDEX] = TRUE;
	settings->OrderSupport[NEG_POLYGON_SC_INDEX] = (settings->SoftwareGdi) ? FALSE : TRUE;
	settings->OrderSupport[NEG_POLYGON_CB_INDEX] = (settings->SoftwareGdi) ? FALSE : TRUE;
	settings->OrderSupport[NEG_ELLIPSE_SC_INDEX] = FALSE;
	settings->OrderSupport[NEG_ELLIPSE_CB_INDEX] = FALSE;

	PubSub_SubscribeChannelConnected(instance->context->pubSub,
									 (pChannelConnectedEventHandler)remmina_rdp_OnChannelConnectedEventHandler);
	PubSub_SubscribeChannelDisconnected(instance->context->pubSub,
										(pChannelDisconnectedEventHandler)remmina_rdp_OnChannelDisconnectedEventHandler);

	if (freerdp_client_load_addins(channels, instance->settings) < 0)
		return FALSE;

	if (freerdp_channels_pre_connect(channels, instance) < 0)
		return FALSE;

	if (!(instance->context->cache = cache_new(instance->settings)))
		return FALSE;

	return TRUE;
}


static BOOL remmina_rdp_post_connect(freerdp* instance)
{
	TRACE_CALL("remmina_rdp_post_connect");
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	RemminaPluginRdpUiObject* ui;
	UINT32 flags;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	rfi->width = rfi->settings->DesktopWidth;
	rfi->height = rfi->settings->DesktopHeight;

	rfi->clrconv = freerdp_clrconv_new(CLRCONV_ALPHA);
	rf_register_graphics(instance->context->graphics);

	flags = CLRCONV_ALPHA;

	if (rfi->settings->ColorDepth == 32)
	{
		flags |= CLRBUF_32BPP;
		rfi->cairo_format = CAIRO_FORMAT_ARGB32;
	}
	else
	{
		flags |= CLRBUF_16BPP;
		rfi->cairo_format = CAIRO_FORMAT_RGB16_565;
	}

	if (!rfi->settings->SoftwareGdi)
		return FALSE;

	if (!gdi_init(instance, flags, NULL))
		return FALSE;

	instance->update->BeginPaint = rf_begin_paint;
	instance->update->EndPaint = rf_end_paint;
	instance->update->DesktopResize = rf_desktop_resize;

	remmina_rdp_clipboard_init(rfi);
	if (freerdp_channels_post_connect(instance->context->channels, instance) < 0)
		return FALSE;

	rfi->connected = True;

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CONNECTED;
	rf_queue_ui(gp, ui);

	return TRUE;
}

static void remmina_rdp_post_disconnect(freerdp* instance)
{
	rfContext* rfi = (rfContext*) instance->context;

	freerdp_channels_disconnect(instance->context->channels, instance);

	if (rfi->hdc) {
		gdi_DeleteDC(rfi->hdc);
		rfi->hdc = NULL;
	}

	if (instance) {
		gdi_free(instance);

		if (instance->context->cache)
		{
			cache_free(instance->context->cache);
			instance->context->cache = NULL;
		}

		freerdp_clrconv_free(rfi->clrconv);
	}

	remmina_rdp_clipboard_free(rfi);
}

static BOOL remmina_rdp_authenticate(freerdp* instance, char** username, char** password, char** domain)
{
	TRACE_CALL("remmina_rdp_authenticate");
	gchar *s_username, *s_password, *s_domain;
	gint ret;
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	gboolean save;
	gboolean disablepasswordstoring;
	RemminaFile* remminafile;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);
	ret = remmina_plugin_service->protocol_plugin_init_authuserpwd(gp, TRUE, !disablepasswordstoring);

	if (ret == GTK_RESPONSE_OK)
	{
		s_username = remmina_plugin_service->protocol_plugin_init_get_username(gp);
		if (s_username) rfi->settings->Username = strdup(s_username);

		s_password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		if (s_password) rfi->settings->Password = strdup(s_password);

		s_domain = remmina_plugin_service->protocol_plugin_init_get_domain(gp);
		if (s_domain) rfi->settings->Domain = strdup(s_domain);

		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save)
		{
			// User has requested to save credentials. We put all the new cretentials
			// into remminafile->settings. They will be saved later, on successful connection, by
			// remmina_connection_window.c

			remmina_plugin_service->file_set_string( remminafile, "username", s_username );
			remmina_plugin_service->file_set_string( remminafile, "password", s_password );
			remmina_plugin_service->file_set_string( remminafile, "domain", s_domain );

		}

		if ( s_username ) g_free( s_username );
		if ( s_password ) g_free( s_password );
		if ( s_domain ) g_free( s_domain );

		return True;
	}
	else
	{
		rfi->user_cancelled = TRUE;
		return False;
	}

	return True;
}

static BOOL remmina_rdp_verify_certificate(freerdp* instance, char* subject, char* issuer, char* fingerprint)
{
	TRACE_CALL("remmina_rdp_verify_certificate");
	gint status;
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	status = remmina_plugin_service->protocol_plugin_init_certificate(gp, subject, issuer, fingerprint);

	if (status == GTK_RESPONSE_OK)
		return True;

	return False;
}
static BOOL remmina_rdp_verify_changed_certificate(freerdp* instance, char* subject, char* issuer,
												   char* new_fingerprint, char* old_subject, char* old_issuer, char* old_fingerprint)
{
	TRACE_CALL("remmina_rdp_verify_changed_certificate");
	gint status;
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	status = remmina_plugin_service->protocol_plugin_changed_certificate(gp, subject, issuer, new_fingerprint, old_fingerprint);

	if (status == GTK_RESPONSE_OK)
		return True;

	return False;
}

static int remmina_rdp_receive_channel_data(freerdp* instance, int channelId, UINT8* data, int size, int flags, int total_size)
{
	TRACE_CALL("remmina_rdp_receive_channel_data");
	return freerdp_channels_data(instance, channelId, data, size, flags, total_size);
}

static void remmina_rdp_main_loop(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_main_loop");
	DWORD nCount;
	DWORD status;
	HANDLE handles[64];
	gchar buf[100];
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	while (!freerdp_shall_disconnect(rfi->instance))
	{
		nCount = freerdp_get_event_handles(rfi->instance->context, &handles[0], 64);
		if (rfi->event_handle)
			handles[nCount++] = rfi->event_handle;

		if (nCount == 0)
		{
			fprintf(stderr, "freerdp_get_event_handles failed\n");
			break;
		}

		status = WaitForMultipleObjects(nCount, handles, FALSE, INFINITE);

		if (status == WAIT_FAILED)
		{
			fprintf(stderr, "WaitForMultipleObjects failed with %lu\n", (unsigned long)status);
			break;
		}

		if (rfi->event_handle && WaitForSingleObject(rfi->event_handle, 0) == WAIT_OBJECT_0) {
			if (!rf_process_event_queue(gp))
			{
				fprintf(stderr, "Failed to process local kb/mouse event queue\n");
				break;
			}
			if (read(rfi->event_pipe[0], buf, sizeof (buf)))
			{
			}
		}

		if (!freerdp_check_event_handles(rfi->instance->context))
		{
			fprintf(stderr, "Failed to check FreeRDP event handles\n");
			break;
		}
	}
}

/* Send CTRL+ALT+DEL keys keystrokes to the plugin drawing_area widget */
static void remmina_rdp_send_ctrlaltdel(RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_rdp_send_ctrlaltdel");
	guint keys[] = { GDK_KEY_Control_L, GDK_KEY_Alt_L, GDK_KEY_Delete };
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	remmina_plugin_service->protocol_plugin_send_keys_signals(rfi->drawing_area,
															  keys, G_N_ELEMENTS(keys), GDK_KEY_PRESS | GDK_KEY_RELEASE);
}

static void clear_argv(int argc, char* argv[])
{
	int i;

	for (i=0; i<argc; i++)
		free (argv[i]);
	free(argv);
}

static int add_argv(int argc, char** argv[], const char* prefix, const char* new_arg)
{
	int pos = argc;
	int next = argc + 1;
	char **tmp;
	
	if (argc < 0)
		return argc;

	if (!prefix)
	{
		if (argv)
			clear_argv(argc, *argv);
		return -1;
	}

	tmp = realloc(*argv, next * sizeof(char*));
	if (!tmp)
	{
		clear_argv(argc, *argv);
		*argv = NULL;
		return -1;
	}

	if (new_arg)
	{
		tmp[pos] =  g_strdup_printf("%s%s", prefix, new_arg);
	}
	else
	{
		tmp[pos] =  g_strdup_printf("%s", prefix);
	}

	*argv = tmp;

	return next;
}

static gboolean remmina_rdp_load_settings(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_main");
	gchar* s;
	gint port;
	gchar* host;
	gchar* value;
	gchar* hostport;
	gint rdpsnd_rate;
	gint rdpsnd_channel;
	char *rdpsnd_params[3];
	int rdpsnd_nparams;
	const gchar* cs;
	RemminaFile* remminafile;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	const gchar *cert_hostport;
	gchar *cert_host;
	gint cert_port;
	gchar *gateway_host;
	gint gateway_port;

	int argc = 0;
	char **argv = NULL;
	char *tmp = NULL;
	UINT32 ColorDepth;
	UINT32 DesktopHeight;
	UINT32 DesktopWidth;
	DWORD status;
	BOOL rc = TRUE;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	s = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, 3389, FALSE);

	if (s == NULL)
		return FALSE;

	remmina_plugin_service->get_server_port(s, 3389, &host, &port);
	argc = add_argv(argc, &argv, "remmina", ""); /* Dummy entry for command name */
	argc = add_argv(argc, &argv, "/codec-cache:", "nsc");

	argc = add_argv(argc, &argv, "/v:", host);
	tmp = g_strdup_printf("%d", port);
	argc = add_argv(argc, &argv, "/port:", tmp);
	g_free(tmp);

	cert_host = host;
	cert_port = port;

	if (remmina_plugin_service->file_get_int(remminafile, "ssh_enabled", FALSE)) {
		cert_hostport = remmina_plugin_service->file_get_string(remminafile, "server");
		if ( cert_hostport ) {
			remmina_plugin_service->get_server_port(cert_hostport, 3389, &cert_host, &cert_port);
		}
	}

	argc = add_argv(argc, &argv, "/gdi:", "sw");
	if (cert_port == 3389)
	{
		argc = add_argv(argc, &argv, "/cert-name:", cert_host);
	}
	else
	{
		hostport = g_strdup_printf("%s:%d", cert_host, cert_port);
		argc = add_argv(argc, &argv, "/cert-name:", hostport);
		g_free(hostport);
	}

	if (cert_host != host) g_free(cert_host);
	g_free(host);
	g_free(s);

	ColorDepth = remmina_plugin_service->file_get_int(remminafile, "colordepth", 0);

	switch (ColorDepth)
	{
	case 2:
		argc = add_argv(argc, &argv, "/gfx-h264", "");
	case 1:
		argc = add_argv(argc, &argv, "/gfx", "");
	case 0:
		argc = add_argv(argc, &argv, "/rfx", "");
		argc = add_argv(argc, &argv, "/codec-cache:", "rfx");
		ColorDepth = 32;
	default:
		tmp = g_strdup_printf("%d", ColorDepth);
		argc = add_argv(argc, &argv, "/bpp:", tmp);
		g_free(tmp);
		break;
	}

	DesktopHeight = remmina_plugin_service->file_get_int(remminafile, "resolution_height", 768);
	DesktopWidth = remmina_plugin_service->file_get_int(remminafile, "resolution_width", 768);
	tmp = g_strdup_printf("%d", DesktopWidth);
	argc = add_argv(argc, &argv, "/w:", tmp);
	g_free(tmp);

	tmp = g_strdup_printf("%d", DesktopHeight);
	argc = add_argv(argc, &argv, "/h:", tmp);
	g_free(tmp);

	remmina_plugin_service->protocol_plugin_set_width(gp, DesktopWidth);
	remmina_plugin_service->protocol_plugin_set_height(gp, DesktopHeight);

	if (remmina_plugin_service->file_get_string(remminafile, "username"))
		argc = add_argv(argc, &argv, "/u:", remmina_plugin_service->file_get_string(remminafile, "username"));

	if (remmina_plugin_service->file_get_string(remminafile, "domain"))
		argc = add_argv(argc, &argv, "/d:", remmina_plugin_service->file_get_string(remminafile, "domain"));

	s = remmina_plugin_service->file_get_secret(remminafile, "password");

	if (s)
	{
		argc = add_argv(argc, &argv, "/p:", s);
		g_free(s);
	}

	/* Remote Desktop Gateway server address */
	s = (gchar *) remmina_plugin_service->file_get_string(remminafile, "gateway_server");
	if (s)
	{
		remmina_plugin_service->get_server_port(s, 443, &gateway_host, &gateway_port);
		tmp = g_strdup_printf("%s:%d", gateway_host, gateway_port);
		argc = add_argv(argc, &argv, "/g:", tmp);
		g_free(tmp);
	}
	/* Remote Desktop Gateway domain */
	if (remmina_plugin_service->file_get_string(remminafile, "gateway_domain"))
	{
		argc = add_argv(argc, &argv, "/gd:", remmina_plugin_service->file_get_string(remminafile, "gateway_domain"));
	}
	/* Remote Desktop Gateway username */
	if (remmina_plugin_service->file_get_string(remminafile, "gateway_username"))
	{
		argc = add_argv(argc, &argv, "/gu:", remmina_plugin_service->file_get_string(remminafile, "gateway_username"));
	}
	/* Remote Desktop Gateway password */
	if (remmina_plugin_service->file_get_string(remminafile, "gateway_password"))
	{
		argc = add_argv(argc, &argv, "/gp:", remmina_plugin_service->file_get_string(remminafile, "gateway_password"));
	}
	/* Remote Desktop Gateway usage */
	switch(remmina_plugin_service->file_get_int(remminafile, "gateway_usage", FALSE))
	{
	case TSC_PROXY_MODE_DETECT:
		argc = add_argv(argc, &argv, "/gateway-usage-method:", "detect");
		break;
	case TSC_PROXY_MODE_DIRECT:
		argc = add_argv(argc, &argv, "/gateway-usage-method:", "direct");
		break;
	default:
		break;
	}
	/* Certificate ignore */
	if (remmina_plugin_service->file_get_int(remminafile, "cert_ignore", 0) != 0)
	{
		argc = add_argv(argc, &argv, "/cert-ignore", "");
	}

	/* ClientHostname is internally preallocated to 32 bytes by libfreerdp */
	if ((cs=remmina_plugin_service->file_get_string(remminafile, "clientname")))
	{
		argc = add_argv(argc, &argv, "/client-hostname:", cs);
	}
	else
	{
		argc = add_argv(argc, &argv, "/client-hostname:", g_get_host_name());
	}

	if (remmina_plugin_service->file_get_string(remminafile, "loadbalanceinfo"))
	{
		argc = add_argv(argc, &argv, "/load-balance-info:", remmina_plugin_service->file_get_string(remminafile, "loadbalanceinfo"));
	}

	if (remmina_plugin_service->file_get_string(remminafile, "exec"))
	{
		argc = add_argv(argc, &argv, "/shell:", remmina_plugin_service->file_get_string(remminafile, "exec"));
	}

	if (remmina_plugin_service->file_get_string(remminafile, "execpath"))
	{
		argc = add_argv(argc, &argv, "/shell-dir:", remmina_plugin_service->file_get_string(remminafile, "execpath"));
	}

	s = g_strdup_printf("rdp_quality_%i", remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0));
	value = remmina_plugin_service->pref_get_value(s);
	g_free(s);

	/* TODO: FreeRDP does currently not allow setting performance
	 * flags any more. */
	freerdp_performance_flags_make(rfi->settings);
	if (value && value[0])
	{
		rfi->settings->PerformanceFlags = strtoul(value, NULL, 16);
	}
	else
	{
		switch (remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0))
		{
		case 9:
			rfi->settings->PerformanceFlags = DEFAULT_QUALITY_9;
			break;

		case 2:
			rfi->settings->PerformanceFlags = DEFAULT_QUALITY_2;
			break;

		case 1:
			rfi->settings->PerformanceFlags = DEFAULT_QUALITY_1;
			break;

		case 0:
		default:
			rfi->settings->PerformanceFlags = DEFAULT_QUALITY_0;
			break;
		}
	}
	g_free(value);

	/* PerformanceFlags bitmask need also to be splitted into BOOL variables
	 * like rfi->settings->DisableWallpaper, rfi->settings->AllowFontSmoothing...
	 * or freerdp_get_param_bool() function will return the wrong value
	*/
	freerdp_performance_flags_split(rfi->settings);

	tmp = g_strdup_printf("0x%04X", remmina_rdp_settings_get_keyboard_layout());
	argc = add_argv(argc, &argv, "/kbd:", tmp);
	g_free(tmp);

	if (remmina_plugin_service->file_get_int(remminafile, "console", FALSE))
	{
		argc = add_argv(argc, &argv, "/admin", "");
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "security");

	if ((g_strcmp0(cs, "rdp") == 0) ||
			(g_strcmp0(cs, "tls") == 0) ||
			(g_strcmp0(cs, "nla") == 0))
	{
		argc = add_argv(argc, &argv, "/sec-", cs);
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "sound");

	if (g_strcmp0(cs, "remote") == 0)
	{
		/* AUDIO_MODE_PLAY_ON_SERVER */
		argc = add_argv(argc, &argv, "/audio-mode:", "1");
	}
	else if (g_str_has_prefix(cs, "local"))
	{

		rdpsnd_nparams = 0;
		rdpsnd_params[rdpsnd_nparams++] = "rdpsnd";

		tmp = NULL;
		cs = strchr(cs, ',');
		if (cs)
		{
			rdpsnd_rate = atoi(cs + 1);
			if (rdpsnd_rate > 1000 && rdpsnd_rate < 150000)
			{
				tmp = g_strdup_printf(":rate:%d", rdpsnd_rate);

				cs = strchr(cs +1, ',');
				if (cs)
				{
					rdpsnd_channel = atoi(cs + 1);
					if (rdpsnd_channel >= 1 && rdpsnd_channel <= 2)
					{
						gchar *tmp2 = g_strdup_printf("%s:channel:%d", tmp, rdpsnd_channel);
						g_free(tmp);
						tmp = tmp2;
					}
				}
			}
		}

		argc = add_argv(argc, &argv, "/sound", tmp);
	}

	if ( remmina_plugin_service->file_get_int(remminafile, "microphone", FALSE) ? TRUE : FALSE )
	{
		argc = add_argv(argc, &argv, "/microphone", "");
	}

	if ( remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE) == FALSE )
	{
		argc = add_argv(argc, &argv, "+clipboard", "");
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "sharefolder");

	if (cs && cs[0] == '/')
	{
		gsize sz;

		s = strrchr( cs, '/' );
		if ( s == NULL || s[1] == 0 )
			s = remmina_rdp_plugin_default_drive_name;
		else
			s++;
		s = g_convert_with_fallback(s, -1, "ascii", "utf-8", "_", NULL, &sz, NULL);

		tmp = g_strdup_printf("%s,%s", s, cs);
		argc = add_argv(argc, &argv, "/drive:", tmp);
		g_free(s);
		g_free(tmp);
	}

	if (remmina_plugin_service->file_get_int(remminafile, "shareprinter", FALSE))
	{
		argc = add_argv(argc, &argv, "/printer", "");
	}

	if (remmina_plugin_service->file_get_int(remminafile, "sharesmartcard", FALSE))
	{
		argc = add_argv(argc, &argv, "/smartcard", "");
	}

	if ((status = freerdp_client_settings_parse_command_line_arguments(rfi->settings, argc, argv, FALSE)) != 0)
	{

		rc = FALSE;
	}

	freerdp_client_settings_command_line_status_print(rfi->settings, status, argc, argv);
	clear_argv(argc, argv);

	return rc;
}

static gpointer remmina_rdp_main_thread(gpointer data)
{
	TRACE_CALL("remmina_rdp_main_thread");
	RemminaProtocolWidget* gp;
	rfContext* rfi;

	gp = (RemminaProtocolWidget*) data;
	rfi = GET_PLUGIN_DATA(gp);

	if (!gp || !rfi)
		return NULL;

	/* Parse command line. */
	if (!remmina_rdp_load_settings(gp))
		return NULL;

	/* Try to establish an RDP connection */
	if (!freerdp_connect(rfi->instance))
	{
		if (!rfi->user_cancelled)
		{
			UINT32 e;

			e = freerdp_get_last_error(rfi->instance->context);
			switch(e) {
			case FREERDP_ERROR_AUTHENTICATION_FAILED:
				remmina_plugin_service->protocol_plugin_set_error(
							gp, _("Authentication to RDP server %s failed.\nCheck username, password and domain."),
							rfi->settings->ServerHostname );
				// Invalidate the saved password, so the user will be re-asked at next logon
				remmina_plugin_service->file_unsave_password(
							remmina_plugin_service->protocol_plugin_get_file(gp));
				break;
			case FREERDP_ERROR_CONNECT_FAILED:
				remmina_plugin_service->protocol_plugin_set_error(
							gp, _("Connection to RDP server %s failed."),
							rfi->settings->ServerHostname );
				break;
			default:
				remmina_plugin_service->protocol_plugin_set_error(
							gp, _("Unable to connect to RDP server %s"),
							rfi->settings->ServerHostname);
				break;
			}

		}
		return NULL;
	}

	/* Run until quit. */
	remmina_rdp_main_loop(gp);

	/* Cleanup connection. */
	freerdp_disconnect(rfi->instance);

	IDLE_ADD((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);
	return NULL;
}

static BOOL remmina_rdp_context_new(freerdp* instance, rdpContext* context)
{
	if (!(context->channels = freerdp_channels_new()))
		return FALSE;
	return TRUE;
}

static void remmina_rdp_context_free(freerdp* instance, rdpContext* context)
{
	if (context && context->channels)
	{
		freerdp_channels_close(context->channels, instance);
		freerdp_channels_free(context->channels);
		context->channels = NULL;
	}
}

static void remmina_rdp_init(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_init");
	freerdp* instance;
	rfContext* rfi;

	instance = freerdp_new();
	instance->PreConnect = remmina_rdp_pre_connect;
	instance->PostConnect = remmina_rdp_post_connect;
	instance->PostDisconnect = remmina_rdp_post_disconnect;
	instance->Authenticate = remmina_rdp_authenticate;
	instance->VerifyCertificate = remmina_rdp_verify_certificate;
	instance->VerifyChangedCertificate = remmina_rdp_verify_changed_certificate;

	instance->ContextNew = remmina_rdp_context_new;
	instance->ContextFree = remmina_rdp_context_free;
	instance->ContextSize = sizeof(rfContext);
	
	if (!freerdp_context_new(instance))
		return;

	if (0 != freerdp_register_addin_provider(freerdp_channels_load_static_addin_entry, 0)) {
		return;
	}

	rfi = (rfContext*) instance->context;

	g_object_set_data_full(G_OBJECT(gp), "plugin-data", rfi, free);

	rfi->protocol_widget = gp;
	rfi->instance = instance;
	rfi->settings = instance->settings;
	rfi->connected = False;

	pthread_mutex_init(&rfi->mutex, NULL);

	remmina_rdp_event_init(gp);
}

static gboolean remmina_rdp_open_connection(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_open_connection");
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	rfi->scale = remmina_plugin_service->protocol_plugin_get_scale(gp);

	if (pthread_create(&rfi->thread, NULL, remmina_rdp_main_thread, gp))
	{
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s",
														  "Failed to initialize pthread. Falling back to non-thread mode...");

		rfi->thread = 0;

		return FALSE;
	}

	return TRUE;
}

static gboolean remmina_rdp_close_connection(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_close_connection");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	freerdp* instance;

	instance = rfi->instance;
	if (instance)
	{
		freerdp_abort_connect(instance);
		if ( rfi->connected ) {
			rfi->connected = False;
		}
	}
	
	if (rfi->thread)
		pthread_join(rfi->thread, NULL);

	pthread_mutex_destroy(&rfi->mutex);
	
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");
	/* Destroy event queue. Pending async events will be discarded. Should we flush it ? */
	remmina_rdp_event_uninit(gp);
	/* Remove instance->context from gp object data to avoid double free */
	g_object_steal_data(G_OBJECT(gp), "plugin-data");

	freerdp_context_free(instance); /* context is rfContext* rfi */
	freerdp_free(instance); /* This implicitly frees instance->context and rfi is no longer valid */

	return FALSE;
}

static gboolean remmina_rdp_query_feature(RemminaProtocolWidget* gp, const RemminaProtocolFeature* feature)
{
	TRACE_CALL("remmina_rdp_query_feature");
	return TRUE;
}

static void remmina_rdp_call_feature(RemminaProtocolWidget* gp, const RemminaProtocolFeature* feature)
{
	TRACE_CALL("remmina_rdp_call_feature");
	RemminaFile* remminafile;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	switch (feature->id)
	{
	case REMMINA_RDP_FEATURE_UNFOCUS:
		remmina_rdp_event_unfocus(gp);
		break;

	case REMMINA_RDP_FEATURE_SCALE:
		rfi->scale = remmina_plugin_service->file_get_int(remminafile, "scale", FALSE);
		remmina_rdp_event_update_scale(gp);
		break;

	case REMMINA_RDP_FEATURE_TOOL_REFRESH:
		gtk_widget_queue_draw_area(rfi->drawing_area, 0, 0,
								   remmina_plugin_service->protocol_plugin_get_width(gp),
								   remmina_plugin_service->protocol_plugin_get_height(gp));
		break;

	case REMMINA_RDP_FEATURE_TOOL_SENDCTRLALTDEL:
		remmina_rdp_send_ctrlaltdel(gp);
		break;

	default:
		break;
	}
}

/* Send a keystroke to the plugin window */
static void remmina_rdp_keystroke(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL("remmina_rdp_keystroke");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	remmina_plugin_service->protocol_plugin_send_keys_signals(rfi->drawing_area,
															  keystrokes, keylen, GDK_KEY_PRESS | GDK_KEY_RELEASE);
	return;
}

/* Array of key/value pairs for color depths */
static gpointer colordepth_list[] =
{
	"8", N_("256 colors (8 bpp)"),
	"15", N_("High color (15 bpp)"),
	"16", N_("High color (16 bpp)"),
	"24", N_("True color (24 bpp)"),
	"32", N_("True color (32 bpp)"),
	"0", N_("RemoteFX (32 bpp)"),
	"1", N_("RemoteFX [GFX] (32 bpp)"),
	"2", N_("H264 [GFX] (32 bpp)"),
	NULL
};

/* Array of key/value pairs for quality selection */
static gpointer quality_list[] =
{
	"0", N_("Poor (fastest)"),
	"1", N_("Medium"),
	"2", N_("Good"),
	"9", N_("Best (slowest)"),
	NULL
};

/* Array of key/value pairs for sound options */
static gpointer sound_list[] =
{
	"off", N_("Off"),
	"local", N_("Local"),
	"local,11025,1", N_("Local - low quality"),
	"local,22050,2", N_("Local - medium quality"),
	"local,44100,2", N_("Local - high quality"),
	"remote", N_("Remote"),
	NULL
};

/* Array of key/value pairs for security */
static gpointer security_list[] =
{
	"", N_("Negotiate"),
	"nla", "NLA",
	"tls", "TLS",
	"rdp", "RDP",
	NULL
};

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Unused pointer
 */
static const RemminaProtocolSetting remmina_rdp_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "domain", N_("Domain"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "gateway_server", N_("RD Gateway server"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "colordepth", N_("Color depth"), FALSE, colordepth_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_FOLDER, "sharefolder", N_("Share folder"), FALSE, NULL, NULL },
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
static const RemminaProtocolSetting remmina_rdp_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "quality", N_("Quality"), FALSE, quality_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "sound", N_("Sound"), FALSE, sound_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "security", N_("Security"), FALSE, security_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "clientname", N_("Client name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "exec", N_("Startup program"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "execpath", N_("Startup path"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "loadbalanceinfo", N_("Load Balance Info"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "microphone", N_("Redirect local microphone"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "shareprinter", N_("Share local printers"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "sharesmartcard", N_("Share smartcard"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard", N_("Disable clipboard sync"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "console", N_("Attach to console (Windows 2003 / 2003 R2)"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disablepasswordstoring", N_("Disable password storing"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "gateway_usage", N_("Use RD Gateway server for server detection"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "cert_ignore", N_("Ignore certificate"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_rdp_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_RDP_FEATURE_TOOL_REFRESH, N_("Refresh"), NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, REMMINA_RDP_FEATURE_SCALE, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_RDP_FEATURE_TOOL_SENDCTRLALTDEL, N_("Send Ctrl+Alt+Delete"), NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, REMMINA_RDP_FEATURE_UNFOCUS, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_rdp =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                 // Type
	"RDP",                                        // Name
	N_("RDP - Remote Desktop Protocol"),          // Description
	GETTEXT_PACKAGE,                              // Translation domain
	VERSION,                                      // Version number
	"remmina-rdp",                                // Icon for normal connection
	"remmina-rdp-ssh",                            // Icon for SSH connection
	remmina_rdp_basic_settings,                   // Array for basic settings
	remmina_rdp_advanced_settings,                // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,          // SSH settings type
	remmina_rdp_features,                         // Array for available features
	remmina_rdp_init,                             // Plugin initialization
	remmina_rdp_open_connection,                  // Plugin open connection
	remmina_rdp_close_connection,                 // Plugin close connection
	remmina_rdp_query_feature,                    // Query for available features
	remmina_rdp_call_feature,                     // Call a feature
	remmina_rdp_keystroke                         // Send a keystroke
};

/* File plugin definition and features */
static RemminaFilePlugin remmina_rdpf =
{
	REMMINA_PLUGIN_TYPE_FILE,                     // Type
	"RDPF",                                       // Name
	N_("RDP - RDP File Handler"),                 // Description
	GETTEXT_PACKAGE,                              // Translation domain
	VERSION,                                      // Version number
	remmina_rdp_file_import_test,                 // Test import function
	remmina_rdp_file_import,                      // Import function
	remmina_rdp_file_export_test,                 // Test export function
	remmina_rdp_file_export,                      // Export function
	NULL
};

/* Preferences plugin definition and features */
static RemminaPrefPlugin remmina_rdps =
{
	REMMINA_PLUGIN_TYPE_PREF,                     // Type
	"RDPS",                                       // Name
	N_("RDP - Preferences"),                      // Description
	GETTEXT_PACKAGE,                              // Translation domain
	VERSION,                                      // Version number
	"RDP",                                        // Label
	remmina_rdp_settings_new                      // Preferences body function
};

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService* service)
{
	int vermaj, vermin, verrev;

	TRACE_CALL("remmina_plugin_entry");
	remmina_plugin_service = service;

	/* Check that we are linked to the correct version of libfreerdp */

	freerdp_get_version(&vermaj, &vermin, &verrev);
	if (vermaj < FREERDP_REQUIRED_MAJOR ||
			(vermaj == FREERDP_REQUIRED_MAJOR && ( vermin < FREERDP_REQUIRED_MINOR ||
												   (vermin == FREERDP_REQUIRED_MINOR && verrev < FREERDP_REQUIRED_REVISION) ) ) ) {
		g_printf("Unable to load RDP plugin due to bad freerdp library version. Required "
				 "libfreerdp version is at least %d.%d.%d but we found libfreerdp version %d.%d.%d\n",
				 FREERDP_REQUIRED_MAJOR, FREERDP_REQUIRED_MINOR, FREERDP_REQUIRED_REVISION,
				 vermaj, vermin, verrev );
		return FALSE;
	}

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdp))
		return FALSE;

	remmina_rdpf.export_hints = _("Export connection in Windows .rdp file format");

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdpf))
		return FALSE;

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdps))
		return FALSE;

	remmina_rdp_settings_init();

	return TRUE;
}

