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

#include <errno.h>
#include <pthread.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/utils/memory.h>

#define REMMINA_RDP_FEATURE_TOOL_REFRESH		1
#define REMMINA_RDP_FEATURE_SCALE			2
#define REMMINA_RDP_FEATURE_UNFOCUS			3

RemminaPluginService* remmina_plugin_service = NULL;

/* Migrated from xfreerdp */
static gboolean rf_get_key_state(KeyCode keycode, int state, XModifierKeymap* modmap)
{
	int offset;
	int modifierpos, key, keysymMask = 0;

	if (keycode == NoSymbol)
		return FALSE;

	for (modifierpos = 0; modifierpos < 8; modifierpos++)
	{
		offset = modmap->max_keypermod * modifierpos;

		for (key = 0; key < modmap->max_keypermod; key++)
		{
			if (modmap->modifiermap[offset + key] == keycode)
				keysymMask |= 1 << modifierpos;
		}
	}

	return (state & keysymMask) ? TRUE : FALSE;
}

void rf_init(RemminaProtocolWidget* gp)
{
	int dummy;
	uint32 state;
	gint keycode;
	Window wdummy;
	XModifierKeymap* modmap;
	rfContext* rfi;

	rfi = GET_DATA(gp);

	XQueryPointer(rfi->display, GDK_ROOT_WINDOW(), &wdummy, &wdummy, &dummy, &dummy,
		&dummy, &dummy, &state);

	modmap = XGetModifierMapping(rfi->display);

	keycode = XKeysymToKeycode(rfi->display, XK_Caps_Lock);
	rfi->capslock_initstate = rf_get_key_state(keycode, state, modmap);

	keycode = XKeysymToKeycode(rfi->display, XK_Num_Lock);
	rfi->numlock_initstate = rf_get_key_state(keycode, state, modmap);

	XFreeModifiermap(modmap);
}

void rf_uninit(RemminaProtocolWidget* gp)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	if (rfi->rfx_context)
	{
		rfx_context_free(rfi->rfx_context);
		rfi->rfx_context = NULL;
	}

#if 0
	if (rfi->channels)
	{
		freerdp_channels_free(rfi->channels);
		rfi->channels = NULL;
	}

	if (rfi->instance)
	{
		freerdp_free(rfi->instance);
		rfi->instance = NULL;
	}
#endif
}

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

boolean rf_check_fds(RemminaProtocolWidget* gp)
{
	uint16 flags;
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
			xfree(obj->nocodec.bitmap);
			break;

		default:
			break;
	}

	g_free(obj);
}

void rf_sw_begin_paint(rdpContext* context)
{
	rdpGdi* gdi = context->gdi;
	gdi->primary->hdc->hwnd->invalid->null = 1;
	gdi->primary->hdc->hwnd->ninvalid = 0;
}

void rf_sw_end_paint(rdpContext* context)
{
	sint32 x, y;
	uint32 w, h;
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

static void rf_sw_desktop_resize(rdpContext* context)
{
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) context;
	gp = rfi->protocol_widget;

	LOCK_BUFFER(TRUE)

	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->width);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->height);

	UNLOCK_BUFFER(TRUE)

	THREADS_ENTER
	remmina_rdp_event_update_scale(gp);
	THREADS_LEAVE

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "desktop-resize");
}

void rf_hw_begin_paint(rdpContext* context)
{

}

void rf_hw_end_paint(rdpContext* context)
{

}

static void rf_hw_desktop_resize(rdpContext* context)
{
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) context;
	gp = rfi->protocol_widget;

	LOCK_BUFFER(TRUE)

	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->width);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->height);

	UNLOCK_BUFFER(TRUE)

	THREADS_ENTER
	remmina_rdp_event_update_scale(gp);
	THREADS_LEAVE

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "desktop-resize");
}

static boolean remmina_rdp_pre_connect(freerdp* instance)
{
	rfContext* rfi;
	rdpSettings* settings;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	settings = instance->settings;
	gp = rfi->protocol_widget;

	settings->bitmap_cache = True;
	settings->offscreen_bitmap_cache = True;

	settings->order_support[NEG_DSTBLT_INDEX] = True;
	settings->order_support[NEG_PATBLT_INDEX] = True;
	settings->order_support[NEG_SCRBLT_INDEX] = True;
	settings->order_support[NEG_OPAQUE_RECT_INDEX] = True;
	settings->order_support[NEG_DRAWNINEGRID_INDEX] = False;
	settings->order_support[NEG_MULTIDSTBLT_INDEX] = False;
	settings->order_support[NEG_MULTIPATBLT_INDEX] = False;
	settings->order_support[NEG_MULTISCRBLT_INDEX] = False;
	settings->order_support[NEG_MULTIOPAQUERECT_INDEX] = True;
	settings->order_support[NEG_MULTI_DRAWNINEGRID_INDEX] = False;
	settings->order_support[NEG_LINETO_INDEX] = True;
	settings->order_support[NEG_POLYLINE_INDEX] = True;
	settings->order_support[NEG_MEMBLT_INDEX] = True;
	settings->order_support[NEG_MEM3BLT_INDEX] = False;
	settings->order_support[NEG_MEMBLT_V2_INDEX] = True;
	settings->order_support[NEG_MEM3BLT_V2_INDEX] = False;
	settings->order_support[NEG_SAVEBITMAP_INDEX] = False;
	settings->order_support[NEG_GLYPH_INDEX_INDEX] = True;
	settings->order_support[NEG_FAST_INDEX_INDEX] = True;
	settings->order_support[NEG_FAST_GLYPH_INDEX] = False;
	settings->order_support[NEG_POLYGON_SC_INDEX] = False;
	settings->order_support[NEG_POLYGON_CB_INDEX] = False;
	settings->order_support[NEG_ELLIPSE_SC_INDEX] = False;
	settings->order_support[NEG_ELLIPSE_CB_INDEX] = False;

	if (settings->rfx_codec == True)
	{
		settings->frame_acknowledge = False;
		settings->large_pointer = True;
		settings->performance_flags = PERF_FLAG_NONE;

		rfi->rfx_context = rfx_context_new();
		rfx_context_set_cpu_opt(rfi->rfx_context, CPU_SSE2);
	}

	freerdp_channels_pre_connect(rfi->channels, instance);

	rfi->clrconv = xnew(CLRCONV);
	rfi->clrconv->alpha = true;
	rfi->clrconv->invert = false;
	rfi->clrconv->rgb555 = false;
	rfi->clrconv->palette = xnew(rdpPalette);

	instance->context->cache = cache_new(instance->settings);

	return True;
}

static boolean remmina_rdp_post_connect(freerdp* instance)
{
	rfContext* rfi;
	XGCValues gcv = { 0 };
	RemminaProtocolWidget* gp;
	RemminaPluginRdpUiObject* ui;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	rfi->width = rfi->settings->width;
	rfi->height = rfi->settings->height;
	rfi->srcBpp = rfi->settings->color_depth;

	rfi->drawable = DefaultRootWindow(rfi->display);
	rfi->primary = XCreatePixmap(rfi->display, rfi->drawable, rfi->width, rfi->height, rfi->depth);
	rfi->drawing = rfi->primary;

	rfi->drawable = rfi->primary;
	rfi->gc = XCreateGC(rfi->display, rfi->drawable, GCGraphicsExposures, &gcv);
	rfi->gc_default = XCreateGC(rfi->display, rfi->drawable, GCGraphicsExposures, &gcv);
	rfi->bitmap_mono = XCreatePixmap(rfi->display, rfi->drawable, 8, 8, 1);
	rfi->gc_mono = XCreateGC(rfi->display, rfi->bitmap_mono, GCGraphicsExposures, &gcv);

	if (rfi->settings->rfx_codec == false)
		rfi->sw_gdi = true;

	rf_register_graphics(instance->context->graphics);

	if (rfi->sw_gdi)
	{
		rdpGdi* gdi;
		uint32 flags;

		flags = CLRCONV_ALPHA;

		if (rfi->bpp > 16)
			flags |= CLRBUF_32BPP;
		else
			flags |= CLRBUF_16BPP;

		gdi_init(instance, flags, NULL);
		gdi = instance->context->gdi;
		rfi->primary_buffer = gdi->primary_buffer;

		rfi->image = XCreateImage(rfi->display, rfi->visual, rfi->depth, ZPixmap, 0,
				(char*) rfi->primary_buffer, rfi->width, rfi->height, rfi->scanline_pad, 0);
	}
	else
	{
		rf_gdi_register_update_callbacks(instance->update);
	}

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

	if (rfi->sw_gdi != true)
	{
		glyph_cache_register_callbacks(instance->update);
		brush_cache_register_callbacks(instance->update);
		bitmap_cache_register_callbacks(instance->update);
		offscreen_cache_register_callbacks(instance->update);
		palette_cache_register_callbacks(instance->update);
	}

	if (rfi->sw_gdi)
	{
		instance->update->BeginPaint = rf_sw_begin_paint;
		instance->update->EndPaint = rf_sw_end_paint;
		instance->update->DesktopResize = rf_sw_desktop_resize;
	}
	else
	{
		instance->update->BeginPaint = rf_hw_begin_paint;
		instance->update->EndPaint = rf_hw_end_paint;
		instance->update->DesktopResize = rf_hw_desktop_resize;
	}

	freerdp_channels_post_connect(rfi->channels, instance);

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CONNECTED;
	rf_queue_ui(gp, ui);

	return True;
}

static boolean remmina_rdp_authenticate(freerdp* instance, char** username, char** password, char** domain)
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
			rfi->settings->username = xstrdup(s);
			g_free(s);
		}

		s = remmina_plugin_service->protocol_plugin_init_get_password(gp);

		if (s)
		{
			rfi->settings->password = xstrdup(s);
			g_free(s);
		}

		s = remmina_plugin_service->protocol_plugin_init_get_domain(gp);

		if (s)
		{
			rfi->settings->domain = xstrdup(s);
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

static boolean remmina_rdp_verify_certificate(freerdp* instance, char* subject, char* issuer, char* fingerprint)
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

static int remmina_rdp_receive_channel_data(freerdp* instance, int channelId, uint8* data, int size, int flags, int total_size)
{
	return freerdp_channels_data(instance, channelId, data, size, flags, total_size);
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
			fds = (int) (uint64) (rfds[i]);

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
		/* check ui */
		if (!rf_check_fds(gp))
		{
			break;
		}
	}
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
	rfi->settings->hostname = xstrdup(host);
	g_free(host);
	g_free(s);
	rfi->settings->port = port;

	rfi->settings->color_depth = remmina_plugin_service->file_get_int(remminafile, "colordepth", 0);

	if (rfi->settings->color_depth == 0)
	{
		rfi->settings->rfx_codec = True;
		rfi->settings->color_depth = 32;
	}

	rfi->settings->width = remmina_plugin_service->file_get_int(remminafile, "resolution_width", 1024);
	rfi->settings->height = remmina_plugin_service->file_get_int(remminafile, "resolution_height", 768);
	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->width);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->height);

	if (remmina_plugin_service->file_get_string(remminafile, "username"))
		rfi->settings->username = xstrdup(remmina_plugin_service->file_get_string(remminafile, "username"));

	if (remmina_plugin_service->file_get_string(remminafile, "domain"))
		rfi->settings->domain = xstrdup(remmina_plugin_service->file_get_string(remminafile, "domain"));

	THREADS_ENTER
	s = remmina_plugin_service->file_get_secret(remminafile, "password");
	THREADS_LEAVE

	if (s)
	{
		rfi->settings->password = xstrdup(s);
		rfi->settings->autologon = 1;
		g_free(s);
	}

	if (remmina_plugin_service->file_get_string(remminafile, "clientname"))
	{
		strncpy(rfi->settings->client_hostname, remmina_plugin_service->file_get_string(remminafile, "clientname"),
			sizeof(rfi->settings->client_hostname) - 1);
	}
	else
	{
		strncpy(rfi->settings->client_hostname, g_get_host_name(), sizeof(rfi->settings->client_hostname) - 1);
	}

	if (remmina_plugin_service->file_get_string(remminafile, "exec"))
	{
		rfi->settings->shell = xstrdup(remmina_plugin_service->file_get_string(remminafile, "exec"));
	}

	if (remmina_plugin_service->file_get_string(remminafile, "execpath"))
	{
		rfi->settings->directory = xstrdup(remmina_plugin_service->file_get_string(remminafile, "execpath"));
	}

	s = g_strdup_printf("rdp_quality_%i", remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0));
	value = remmina_plugin_service->pref_get_value(s);
	g_free(s);

	if (value && value[0])
	{
		rfi->settings->performance_flags = strtoul(value, NULL, 16);
	}
	else
	{
		switch (remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0))
		{
			case 9:
				rfi->settings->performance_flags = DEFAULT_QUALITY_9;
				break;

			case 2:
				rfi->settings->performance_flags = DEFAULT_QUALITY_2;
				break;

			case 1:
				rfi->settings->performance_flags = DEFAULT_QUALITY_1;
				break;

			case 0:
			default:
				rfi->settings->performance_flags = DEFAULT_QUALITY_0;
				break;
		}
	}
	g_free(value);

	rfi->settings->kbd_layout = remmina_rdp_settings_get_keyboard_layout();

	if (remmina_plugin_service->file_get_int(remminafile, "console", FALSE))
	{
		rfi->settings->console_session = True;
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "security");

	if (g_strcmp0(cs, "rdp") == 0)
	{
		rfi->settings->rdp_security = True;
		rfi->settings->tls_security = False;
		rfi->settings->nla_security = False;
	}
	else if (g_strcmp0(cs, "tls") == 0)
	{
		rfi->settings->rdp_security = False;
		rfi->settings->tls_security = True;
		rfi->settings->nla_security = False;
	}
	else if (g_strcmp0(cs, "nla") == 0)
	{
		rfi->settings->rdp_security = False;
		rfi->settings->tls_security = False;
		rfi->settings->nla_security = True;
	}

	rfi->settings->compression = True;
	rfi->settings->fastpath_input = True;
	rfi->settings->fastpath_output = True;

	drdynvc_num = 0;
	rdpsnd_num = 0;
	cs = remmina_plugin_service->file_get_string(remminafile, "sound");

	if (g_strcmp0(cs, "remote") == 0)
	{
		rfi->settings->console_audio = 1;
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

		freerdp_channels_load_plugin(rfi->channels, rfi->settings, "rdpsnd", rfi->rdpsnd_data);

		rfi->drdynvc_data[drdynvc_num].size = sizeof(RDP_PLUGIN_DATA);
		rfi->drdynvc_data[drdynvc_num].data[0] = "audin";
		drdynvc_num++;
	}

	if (drdynvc_num)
	{
		freerdp_channels_load_plugin(rfi->channels, rfi->settings, "drdynvc", rfi->drdynvc_data);
	}

	if (!remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE))
	{
		freerdp_channels_load_plugin(rfi->channels, rfi->settings, "cliprdr", NULL);
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

	if (rdpdr_num)
	{
		freerdp_channels_load_plugin(rfi->channels, rfi->settings, "rdpdr", rfi->rdpdr_data);
	}

	if (!freerdp_connect(rfi->instance))
	{
		if (!rfi->user_cancelled)
		{
			remmina_plugin_service->protocol_plugin_set_error(gp, _("Unable to connect to RDP server %s"),
				rfi->settings->hostname);
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
	instance->ReceiveChannelData = remmina_rdp_receive_channel_data;

	instance->context_size = sizeof(rfContext);
	freerdp_context_new(instance);
	rfi = (rfContext*) instance->context;

	g_object_set_data_full(G_OBJECT(gp), "plugin-data", rfi, xfree);

	rfi->protocol_widget = gp;
	rfi->instance = instance;
	rfi->settings = instance->settings;
	rfi->channels = freerdp_channels_new();

	pthread_mutex_init(&rfi->mutex, NULL);

	remmina_rdp_event_init(gp);
	rf_init(gp);
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

	rfi = GET_DATA(gp);

	if (rfi->thread)
	{
		pthread_cancel(rfi->thread);

		if (rfi->thread)
			pthread_join(rfi->thread, NULL);
	}

	if (rfi->instance)
	{
		if (rfi->channels)
		{
			//freerdp_channels_close(rfi->channels, rfi->instance);
		}

		freerdp_disconnect(rfi->instance);
	}

	pthread_mutex_destroy(&rfi->mutex);

	remmina_rdp_event_uninit(gp);
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");

	rf_uninit(gp);

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

