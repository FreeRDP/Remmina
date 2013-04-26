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

#include "rdp_plugin.h"
#include "rdp_gdi.h"
#include "rdp_event.h"
#include "rdp_graphics.h"
#include "rdp_file.h"
#include "rdp_settings.h"
#include "rdp_cliprdr.h"

#include <errno.h>
#include <pthread.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/channels.h>

#define REMMINA_RDP_FEATURE_TOOL_REFRESH		1
#define REMMINA_RDP_FEATURE_SCALE			2
#define REMMINA_RDP_FEATURE_UNFOCUS			3

RemminaPluginService* remmina_plugin_service = NULL;

void rf_get_fds(RemminaProtocolWidget* gp, void** rfds, int* rcount)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	if (rfi->event_pipe[0] != -1)
	{
		rfds[*rcount] = GINT_TO_POINTER(rfi->event_pipe[0]);
		(*rcount)++;
	}
}

BOOL rf_check_fds(RemminaProtocolWidget* gp)
{
	UINT16 flags;
	gchar buf[100];
	rdpInput* input;
	rfContext* rfi;
	RemminaPluginRdpEvent* event;

	rfi = GET_DATA(gp);

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

	if (read(rfi->event_pipe[0], buf, sizeof (buf)))
	{
	}

	return True;
}

void rf_queue_ui(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);
	g_async_queue_push(rfi->ui_queue, ui);

	LOCK_BUFFER(TRUE)

	if (!rfi->ui_handler)
		rfi->ui_handler = IDLE_ADD((GSourceFunc) remmina_rdp_event_queue_ui, gp);

	UNLOCK_BUFFER(TRUE)
}

void rf_object_free(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* obj)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	switch (obj->type)
	{
		case REMMINA_RDP_UI_RFX:
			rfx_message_free(rfi->rfx_context, obj->rfx.message);
			break;

		case REMMINA_RDP_UI_NOCODEC:
			free(obj->nocodec.bitmap);
			break;

		default:
			break;
	}

	g_free(obj);
}

void rf_begin_paint(rdpContext* context)
{
	rdpGdi* gdi = context->gdi;
	gdi->primary->hdc->hwnd->invalid->null = 1;
	gdi->primary->hdc->hwnd->ninvalid = 0;
}

void rf_end_paint(rdpContext* context)
{
	INT32 x, y;
	UINT32 w, h;
	rdpGdi* gdi;
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	RemminaPluginRdpUiObject* ui;

	gdi = context->gdi;
	rfi = (rfContext*) context;
	gp = rfi->protocol_widget;

	if (gdi->primary->hdc->hwnd->invalid->null)
		return;

	x = gdi->primary->hdc->hwnd->invalid->x;
	y = gdi->primary->hdc->hwnd->invalid->y;
	w = gdi->primary->hdc->hwnd->invalid->w;
	h = gdi->primary->hdc->hwnd->invalid->h;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_UPDATE_REGION;
	ui->region.x = x;
	ui->region.y = y;
	ui->region.width = w;
	ui->region.height = h;

	rf_queue_ui(rfi->protocol_widget, ui);
}

static void rf_desktop_resize(rdpContext* context)
{
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) context;
	gp = rfi->protocol_widget;

	LOCK_BUFFER(TRUE)

	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->DesktopWidth);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->DesktopHeight);

	UNLOCK_BUFFER(TRUE)

	THREADS_ENTER
	remmina_rdp_event_update_scale(gp);
	THREADS_LEAVE

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "desktop-resize");
}

static BOOL remmina_rdp_pre_connect(freerdp* instance)
{
	rfContext* rfi;
	rdpSettings* settings;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	settings = instance->settings;
	gp = rfi->protocol_widget;

	settings->BitmapCacheEnabled = True;
	settings->OffscreenSupportLevel = True;

	settings->OrderSupport[NEG_DSTBLT_INDEX] = True;
	settings->OrderSupport[NEG_PATBLT_INDEX] = True;
	settings->OrderSupport[NEG_SCRBLT_INDEX] = True;
	settings->OrderSupport[NEG_OPAQUE_RECT_INDEX] = True;
	settings->OrderSupport[NEG_DRAWNINEGRID_INDEX] = False;
	settings->OrderSupport[NEG_MULTIDSTBLT_INDEX] = False;
	settings->OrderSupport[NEG_MULTIPATBLT_INDEX] = False;
	settings->OrderSupport[NEG_MULTISCRBLT_INDEX] = False;
	settings->OrderSupport[NEG_MULTIOPAQUERECT_INDEX] = True;
	settings->OrderSupport[NEG_MULTI_DRAWNINEGRID_INDEX] = False;
	settings->OrderSupport[NEG_LINETO_INDEX] = True;
	settings->OrderSupport[NEG_POLYLINE_INDEX] = True;
	settings->OrderSupport[NEG_MEMBLT_INDEX] = True;
	settings->OrderSupport[NEG_MEM3BLT_INDEX] = False;
	settings->OrderSupport[NEG_MEMBLT_V2_INDEX] = True;
	settings->OrderSupport[NEG_MEM3BLT_V2_INDEX] = False;
	settings->OrderSupport[NEG_SAVEBITMAP_INDEX] = False;
	settings->OrderSupport[NEG_GLYPH_INDEX_INDEX] = True;
	settings->OrderSupport[NEG_FAST_INDEX_INDEX] = True;
	settings->OrderSupport[NEG_FAST_GLYPH_INDEX] = False;
	settings->OrderSupport[NEG_POLYGON_SC_INDEX] = False;
	settings->OrderSupport[NEG_POLYGON_CB_INDEX] = False;
	settings->OrderSupport[NEG_ELLIPSE_SC_INDEX] = False;
	settings->OrderSupport[NEG_ELLIPSE_CB_INDEX] = False;

	if (settings->RemoteFxCodec == True)
	{
		settings->FrameAcknowledge = False;
		settings->LargePointerFlag = True;
		settings->PerformanceFlags = PERF_FLAG_NONE;

		rfi->rfx_context = rfx_context_new();
	}

	freerdp_channels_pre_connect(rfi->channels, instance);

	rfi->clrconv = freerdp_clrconv_new(CLRCONV_ALPHA);

	instance->context->cache = cache_new(instance->settings);

	return True;
}


static BOOL remmina_rdp_post_connect(freerdp* instance)
{
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	RemminaPluginRdpUiObject* ui;
	rdpGdi* gdi;
	UINT32 flags;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	rfi->width = rfi->settings->DesktopWidth;
	rfi->height = rfi->settings->DesktopHeight;
	rfi->srcBpp = rfi->settings->ColorDepth;

	if (rfi->settings->RemoteFxCodec == FALSE)
		rfi->sw_gdi = TRUE;

	rf_register_graphics(instance->context->graphics);

	flags = CLRCONV_ALPHA;

	if (rfi->bpp == 32)
	{
		flags |= CLRBUF_32BPP;
		rfi->cairo_format = CAIRO_FORMAT_ARGB32;
	}
	else if (rfi->bpp == 24)
	{
		flags |= CLRBUF_24BPP;
		rfi->cairo_format = CAIRO_FORMAT_RGB24;
	}
	else
	{
		flags |= CLRBUF_16BPP;
		rfi->cairo_format = CAIRO_FORMAT_RGB16_565;
	}

	gdi_init(instance, flags, NULL);
	gdi = instance->context->gdi;
	rfi->primary_buffer = gdi->primary_buffer;

	rfi->hdc = gdi_GetDC();
	rfi->hdc->bitsPerPixel = rfi->bpp;
	rfi->hdc->bytesPerPixel = rfi->bpp / 8;

	rfi->hdc->hwnd = (HGDI_WND) malloc(sizeof(GDI_WND));
	rfi->hdc->hwnd->invalid = gdi_CreateRectRgn(0, 0, 0, 0);
	rfi->hdc->hwnd->invalid->null = 1;

	rfi->hdc->hwnd->count = 32;
	rfi->hdc->hwnd->cinvalid = (HGDI_RGN) malloc(sizeof(GDI_RGN) * rfi->hdc->hwnd->count);
	rfi->hdc->hwnd->ninvalid = 0;

	pointer_cache_register_callbacks(instance->update);

/*
	if (rfi->sw_gdi != true)
	{
		glyph_cache_register_callbacks(instance->update);
		brush_cache_register_callbacks(instance->update);
		bitmap_cache_register_callbacks(instance->update);
		offscreen_cache_register_callbacks(instance->update);
		palette_cache_register_callbacks(instance->update);
	}
*/

	instance->update->BeginPaint = rf_begin_paint;
	instance->update->EndPaint = rf_end_paint;
	instance->update->DesktopResize = rf_desktop_resize;

	freerdp_channels_post_connect(rfi->channels, instance);

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CONNECTED;
	rf_queue_ui(gp, ui);

	return True;
}

static BOOL remmina_rdp_authenticate(freerdp* instance, char** username, char** password, char** domain)
{
	gchar* s;
	gint ret;
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	THREADS_ENTER
	ret = remmina_plugin_service->protocol_plugin_init_authuserpwd(gp, TRUE);
	THREADS_LEAVE

	if (ret == GTK_RESPONSE_OK)
	{
		s = remmina_plugin_service->protocol_plugin_init_get_username(gp);

		if (s)
		{
			rfi->settings->Username = strdup(s);
			g_free(s);
		}

		s = remmina_plugin_service->protocol_plugin_init_get_password(gp);

		if (s)
		{
			rfi->settings->Password = strdup(s);
			g_free(s);
		}

		s = remmina_plugin_service->protocol_plugin_init_get_domain(gp);

		if (s)
		{
			rfi->settings->Domain = strdup(s);
			g_free(s);
		}

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
	gint status;
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	THREADS_ENTER
	status = remmina_plugin_service->protocol_plugin_init_certificate(gp, subject, issuer, fingerprint);
	THREADS_LEAVE

	if (status == GTK_RESPONSE_OK)
		return True;

	return False;
}
static BOOL remmina_rdp_verify_changed_certificate(freerdp* instance, char* subject, char* issuer, char* new_fingerprint, char* old_fingerprint)
{
	gint status;
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	THREADS_ENTER
	status = remmina_plugin_service->protocol_plugin_changed_certificate(gp, subject, issuer, new_fingerprint, old_fingerprint);
	THREADS_LEAVE

	if (status == GTK_RESPONSE_OK)
		return True;

	return False;
}

static int remmina_rdp_receive_channel_data(freerdp* instance, int channelId, UINT8* data, int size, int flags, int total_size)
{
	return freerdp_channels_data(instance, channelId, data, size, flags, total_size);
}

void remmina_rdp_channels_process_event(RemminaProtocolWidget* gp, wMessage* event)
{
	switch (GetMessageClass(event->id))
	{
		case CliprdrChannel_Class:
			remmina_rdp_channel_cliprdr_process(gp, event);
			break;
	}
}

static void remmina_rdp_main_loop(RemminaProtocolWidget* gp)
{
	int i;
	int fds;
	int rcount;
	int wcount;
	int max_fds;
	void *rfds[32];
	void *wfds[32];
	fd_set rfds_set;
	fd_set wfds_set;
	rfContext* rfi;
	wMessage* event;

	memset(rfds, 0, sizeof(rfds));
	memset(wfds, 0, sizeof(wfds));

	rfi = GET_DATA(gp);

	while (1)
	{
		rcount = 0;
		wcount = 0;

		if (!freerdp_get_fds(rfi->instance, rfds, &rcount, wfds, &wcount))
		{
			break;
		}
		if (!freerdp_channels_get_fds(rfi->channels, rfi->instance, rfds, &rcount, wfds, &wcount))
		{
			break;
		}
		rf_get_fds(gp, rfds, &rcount);

		max_fds = 0;
		FD_ZERO(&rfds_set);
		for (i = 0; i < rcount; i++)
		{
			fds = (int) (UINT64) (rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		FD_ZERO(&wfds_set);
		for (i = 0; i < wcount; i++)
		{
			fds = GPOINTER_TO_INT(wfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &wfds_set);
		}

		/* exit if nothing to do */
		if (max_fds == 0)
		{
			break;
		}

		/* do the wait */
		if (select(max_fds + 1, &rfds_set, &wfds_set, NULL, NULL) == -1)
		{
			/* these are not really errors */
			if (!((errno == EAGAIN) ||
				(errno == EWOULDBLOCK) ||
				(errno == EINPROGRESS) ||
				(errno == EINTR))) /* signal occurred */
			{
				break;
			}
		}

		/* check the libfreerdp fds */
		if (!freerdp_check_fds(rfi->instance))
		{
			break;
		}
		/* check channel fds */
		if (!freerdp_channels_check_fds(rfi->channels, rfi->instance))
		{
			break;
		}
		else
		{
			event = freerdp_channels_pop_event(rfi->channels);
			if (event)
				remmina_rdp_channels_process_event(gp, event);
		}
		/* check ui */
		if (!rf_check_fds(gp))
		{
			break;
		}
	}
}

gboolean remmina_rdp_load_plugin(rdpChannels* channels, rdpSettings* settings, const char* name, RDP_PLUGIN_DATA* plugin_data)
{
	void* entry = NULL;

	entry = freerdp_channels_client_find_static_entry("VirtualChannelEntry", name);

	if (entry)
	{
		if (freerdp_channels_client_load(channels, settings, entry, plugin_data) == 0)
		{
			g_printf("loading channel %s (static)\n", name);
			return TRUE;
		}
	}

	g_printf("loading channel %s (plugin)\n", name);
	freerdp_channels_load_plugin(channels, settings, name, plugin_data);

	return TRUE;
}

static gboolean remmina_rdp_main(RemminaProtocolWidget* gp)
{
	gchar* s;
	gint port;
	gchar* host;
	gchar* value;
	gint rdpdr_num;
	gint drdynvc_num;
	gint rdpsnd_num;
	const gchar* cs;
	RemminaFile* remminafile;
	rfContext* rfi;

	rfi = GET_DATA(gp);
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	s = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, 3389, FALSE);

	if (s == NULL)
		return FALSE;

	remmina_plugin_service->get_server_port(s, 3389, &host, &port);
	rfi->settings->ServerHostname = strdup(host);
	g_free(host);
	g_free(s);
	rfi->settings->ServerPort = port;

	rfi->settings->ColorDepth = remmina_plugin_service->file_get_int(remminafile, "colordepth", 0);

	if (rfi->settings->ColorDepth == 0)
	{
		rfi->settings->RemoteFxCodec = True;
		rfi->settings->ColorDepth = 32;
	}

	rfi->settings->DesktopWidth = remmina_plugin_service->file_get_int(remminafile, "resolution_width", 1024);
	rfi->settings->DesktopHeight = remmina_plugin_service->file_get_int(remminafile, "resolution_height", 768);
	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->DesktopWidth);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->DesktopHeight);

	if (remmina_plugin_service->file_get_string(remminafile, "username"))
		rfi->settings->Username = strdup(remmina_plugin_service->file_get_string(remminafile, "username"));

	if (remmina_plugin_service->file_get_string(remminafile, "domain"))
		rfi->settings->Domain = strdup(remmina_plugin_service->file_get_string(remminafile, "domain"));

	THREADS_ENTER
	s = remmina_plugin_service->file_get_secret(remminafile, "password");
	THREADS_LEAVE

	if (s)
	{
		rfi->settings->Password = strdup(s);
		rfi->settings->AutoLogonEnabled = 1;
		g_free(s);
	}

	if (remmina_plugin_service->file_get_string(remminafile, "clientname"))
	{
		strncpy(rfi->settings->ClientHostname, remmina_plugin_service->file_get_string(remminafile, "clientname"),
			sizeof(rfi->settings->ClientHostname) - 1);
	}
	else
	{
		strncpy(rfi->settings->ClientHostname, g_get_host_name(), sizeof(rfi->settings->ClientHostname) - 1);
	}

	if (remmina_plugin_service->file_get_string(remminafile, "exec"))
	{
		rfi->settings->AlternateShell = strdup(remmina_plugin_service->file_get_string(remminafile, "exec"));
	}

	if (remmina_plugin_service->file_get_string(remminafile, "execpath"))
	{
		rfi->settings->ShellWorkingDirectory = strdup(remmina_plugin_service->file_get_string(remminafile, "execpath"));
	}

	s = g_strdup_printf("rdp_quality_%i", remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0));
	value = remmina_plugin_service->pref_get_value(s);
	g_free(s);

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

	rfi->settings->KeyboardLayout = remmina_rdp_settings_get_keyboard_layout();

	if (remmina_plugin_service->file_get_int(remminafile, "console", FALSE))
	{
		rfi->settings->ConsoleSession = True;
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "security");

	if (g_strcmp0(cs, "rdp") == 0)
	{
		rfi->settings->RdpSecurity = True;
		rfi->settings->TlsSecurity = False;
		rfi->settings->NlaSecurity = False;
	}
	else if (g_strcmp0(cs, "tls") == 0)
	{
		rfi->settings->RdpSecurity = False;
		rfi->settings->TlsSecurity = True;
		rfi->settings->NlaSecurity = False;
	}
	else if (g_strcmp0(cs, "nla") == 0)
	{
		rfi->settings->RdpSecurity = False;
		rfi->settings->TlsSecurity = False;
		rfi->settings->NlaSecurity = True;
	}

	rfi->settings->CompressionEnabled = True;
	rfi->settings->FastPathInput = True;
	rfi->settings->FastPathOutput = True;

	drdynvc_num = 0;
	rdpsnd_num = 0;
	cs = remmina_plugin_service->file_get_string(remminafile, "sound");

	if (g_strcmp0(cs, "remote") == 0)
	{
		rfi->settings->RemoteConsoleAudio = 1;
	}
	else if (g_str_has_prefix(cs, "local"))
	{
		cs = strchr(cs, ',');

		if (cs)
		{
			snprintf(rfi->rdpsnd_options, sizeof(rfi->rdpsnd_options), "%s", cs + 1);
			s = strchr(rfi->rdpsnd_options, ',');

			if (s)
				*s++ = '\0';

			rfi->rdpsnd_data[rdpsnd_num].size = sizeof(RDP_PLUGIN_DATA);
			rfi->rdpsnd_data[rdpsnd_num].data[0] = "rate";
			rfi->rdpsnd_data[rdpsnd_num].data[1] = rfi->rdpsnd_options;
			rdpsnd_num++;

			if (s)
			{
				rfi->rdpsnd_data[rdpsnd_num].size = sizeof(RDP_PLUGIN_DATA);
				rfi->rdpsnd_data[rdpsnd_num].data[0] = "channel";
				rfi->rdpsnd_data[rdpsnd_num].data[1] = s;
				rdpsnd_num++;
			}
		}

		remmina_rdp_load_plugin(rfi->channels, rfi->settings, "rdpsnd", rfi->rdpsnd_data);

		rfi->drdynvc_data[drdynvc_num].size = sizeof(RDP_PLUGIN_DATA);
		rfi->drdynvc_data[drdynvc_num].data[0] = "audin";
		drdynvc_num++;
	}

	if (drdynvc_num)
	{
		remmina_rdp_load_plugin(rfi->channels, rfi->settings, "drdynvc", rfi->drdynvc_data);
	}

	if (!remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE))
	{
		remmina_rdp_load_plugin(rfi->channels, rfi->settings, "cliprdr", NULL);
	}

	rdpdr_num = 0;
	cs = remmina_plugin_service->file_get_string(remminafile, "sharefolder");

	if (cs && cs[0] == '/')
	{
		s = strrchr (cs, '/');
		s = (s && s[1] ? s + 1 : "root");
		rfi->rdpdr_data[rdpdr_num].size = sizeof(RDP_PLUGIN_DATA);
		rfi->rdpdr_data[rdpdr_num].data[0] = "disk";
		rfi->rdpdr_data[rdpdr_num].data[1] = s;
		rfi->rdpdr_data[rdpdr_num].data[2] = (gchar*) cs;
		rdpdr_num++;
	}

	if (remmina_plugin_service->file_get_int(remminafile, "shareprinter", FALSE))
	{
		rfi->rdpdr_data[rdpdr_num].size = sizeof(RDP_PLUGIN_DATA);
		rfi->rdpdr_data[rdpdr_num].data[0] = "printer";
		rdpdr_num++;
	}

	if (remmina_plugin_service->file_get_int(remminafile, "sharesmartcard", FALSE))
	{
		rfi->rdpdr_data[rdpdr_num].size = sizeof(RDP_PLUGIN_DATA);
		rfi->rdpdr_data[rdpdr_num].data[0] = "scard";
		rfi->rdpdr_data[rdpdr_num].data[1] = "scard";
		rdpdr_num++;
	}

	if (rdpdr_num)
	{
		remmina_rdp_load_plugin(rfi->channels, rfi->settings, "rdpdr", rfi->rdpdr_data);
	}

	if (!freerdp_connect(rfi->instance))
	{
		if (!rfi->user_cancelled)
		{
			remmina_plugin_service->protocol_plugin_set_error(gp, _("Unable to connect to RDP server %s"),
				rfi->settings->ServerHostname);
		}

		return FALSE;
	}

	remmina_rdp_main_loop(gp);

	return TRUE;
}

static gpointer remmina_rdp_main_thread(gpointer data)
{
	RemminaProtocolWidget* gp;
	rfContext* rfi;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	CANCEL_ASYNC
	gp = (RemminaProtocolWidget*) data;
	rfi = GET_DATA(gp);
	remmina_rdp_main(gp);
	rfi->thread = 0;
	IDLE_ADD((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);

	return NULL;
}

static void remmina_rdp_init(RemminaProtocolWidget* gp)
{
	freerdp* instance;
	rfContext* rfi;

	instance = freerdp_new();
	instance->PreConnect = remmina_rdp_pre_connect;
	instance->PostConnect = remmina_rdp_post_connect;
	instance->Authenticate = remmina_rdp_authenticate;
	instance->VerifyCertificate = remmina_rdp_verify_certificate;
	instance->VerifyChangedCertificate = remmina_rdp_verify_changed_certificate;
	instance->ReceiveChannelData = remmina_rdp_receive_channel_data;

	instance->context_size = sizeof(rfContext);
	freerdp_context_new(instance);
	rfi = (rfContext*) instance->context;

	g_object_set_data_full(G_OBJECT(gp), "plugin-data", rfi, free);

	rfi->protocol_widget = gp;
	rfi->instance = instance;
	rfi->settings = instance->settings;
	rfi->channels = freerdp_channels_new();

	pthread_mutex_init(&rfi->mutex, NULL);

	rfi->gmutex = g_mutex_new();
	rfi->gcond = g_cond_new();

	remmina_rdp_event_init(gp);
}

static gboolean remmina_rdp_open_connection(RemminaProtocolWidget* gp)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);
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
	rfContext* rfi;
	freerdp* instance;

	rfi = GET_DATA(gp);
	instance = rfi->instance;

	if (rfi->thread)
	{
		pthread_cancel(rfi->thread);

		if (rfi->thread)
			pthread_join(rfi->thread, NULL);
	}

	pthread_mutex_destroy(&rfi->mutex);

	g_mutex_free(rfi->gmutex);
	g_cond_free(rfi->gcond);

	remmina_rdp_event_uninit(gp);
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");

	if (instance)
	{
		if (rfi->channels)
		{
			//freerdp_channels_close(rfi->channels, instance);
			freerdp_channels_free(rfi->channels);
			rfi->channels = NULL;
		}
		if (freerdp_shall_disconnect(instance))
		{
			freerdp_disconnect(instance);
		}
	}

	if (rfi->rfx_context)
	{
		rfx_context_free(rfi->rfx_context);
		rfi->rfx_context = NULL;
	}

	if (instance)
	{
		//freerdp_context_free(instance); /* context is rfContext* rfi */
		freerdp_free(instance);
	}

	return FALSE;
}

static gboolean remmina_rdp_query_feature(RemminaProtocolWidget* gp, const RemminaProtocolFeature* feature)
{
	return TRUE;
}

static void remmina_rdp_call_feature(RemminaProtocolWidget* gp, const RemminaProtocolFeature* feature)
{
	RemminaFile* remminafile;
	rfContext* rfi;

	rfi = GET_DATA(gp);
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

		default:
			break;
	}
}

static gpointer colordepth_list[] =
{
	"8", N_("256 colors (8 bpp)"),
	"15", N_("High color (15 bpp)"),
	"16", N_("High color (16 bpp)"),
	"24", N_("True color (24 bpp)"),
	"32", N_("True color (32 bpp)"),
	"0", N_("RemoteFX (32 bpp)"),
	NULL
};

static gpointer quality_list[] =
{
	"0", N_("Poor (fastest)"),
	"1", N_("Medium"),
	"2", N_("Good"),
	"9", N_("Best (slowest)"),
	NULL
};

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

static gpointer security_list[] =
{
	"", N_("Negotiate"),
	"nla", "NLA",
	"tls", "TLS",
	"rdp", "RDP",
	NULL
};

static const RemminaProtocolSetting remmina_rdp_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "domain", N_("Domain"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "colordepth", N_("Color depth"), FALSE, colordepth_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_FOLDER, "sharefolder", N_("Share folder"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

static const RemminaProtocolSetting remmina_rdp_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "quality", N_("Quality"), FALSE, quality_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "sound", N_("Sound"), FALSE, sound_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "security", N_("Security"), FALSE, security_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "clientname", N_("Client name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "exec", N_("Startup program"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "execpath", N_("Startup path"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "shareprinter", N_("Share local printers"), TRUE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "sharesmartcard", N_("Share smartcard"), TRUE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard", N_("Disable clipboard sync"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "console", N_("Attach to console (Windows 2003 / 2003 R2)"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

static const RemminaProtocolFeature remmina_rdp_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_RDP_FEATURE_TOOL_REFRESH, N_("Refresh"), GTK_STOCK_REFRESH, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, REMMINA_RDP_FEATURE_SCALE, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, REMMINA_RDP_FEATURE_UNFOCUS, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

static RemminaProtocolPlugin remmina_rdp =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,
	"RDP",
	N_("RDP - Remote Desktop Protocol"),
	GETTEXT_PACKAGE,
	VERSION,

	"remmina-rdp",
	"remmina-rdp-ssh",
	remmina_rdp_basic_settings,
	remmina_rdp_advanced_settings,
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,
	remmina_rdp_features,

	remmina_rdp_init,
	remmina_rdp_open_connection,
	remmina_rdp_close_connection,
	remmina_rdp_query_feature,
	remmina_rdp_call_feature
};

static RemminaFilePlugin remmina_rdpf =
{
	REMMINA_PLUGIN_TYPE_FILE,
	"RDPF",
	N_("RDP - RDP File Handler"),
	GETTEXT_PACKAGE,
	VERSION,

	remmina_rdp_file_import_test,
	remmina_rdp_file_import,
	remmina_rdp_file_export_test,
	remmina_rdp_file_export,
	NULL
};

static RemminaPrefPlugin remmina_rdps =
{
	REMMINA_PLUGIN_TYPE_PREF,
	"RDPS",
	N_("RDP - Preferences"),
	GETTEXT_PACKAGE,
	VERSION,

	"RDP",
	remmina_rdp_settings_new
};

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService* service)
{
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdp))
		return FALSE;

	remmina_rdpf.export_hints = _("Export connection in Windows .rdp file format");

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdpf))
		return FALSE;

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdps))
		return FALSE;

	freerdp_channels_global_init();
	remmina_rdp_settings_init();

	return TRUE;
}

