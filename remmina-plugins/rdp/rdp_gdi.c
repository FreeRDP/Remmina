/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

/* rdp gdi functions, run inside the rdp thread */

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_gdi.h"
#include <freerdp/constants.h>
#include <freerdp/cache/cache.h>
#include <freerdp/utils/memory.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <gdk/gdkx.h>

static void rf_queue_ui(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);
	g_async_queue_push(rfi->ui_queue, ui);

	LOCK_BUFFER(TRUE)

	if (!rfi->ui_handler)
		rfi->ui_handler = IDLE_ADD((GSourceFunc) remmina_rdp_event_queue_ui, gp);

	UNLOCK_BUFFER(TRUE)
}

static void rf_desktop_resize(rdpContext* context)
{
	RemminaProtocolWidget* gp;
	rfContext* rfi;

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

static void rf_gdi_palette(rdpContext* context, PALETTE_UPDATE* palette)
{
	g_print("palette\n");
}

static void rf_gdi_set_bounds(rdpContext* context, rdpBounds* bounds)
{
	/*
	XRectangle clip;
	rfContext* rfi = (rfContext*) context;

	if (bounds != NULL)
	{
		clip.x = bounds->left;
		clip.y = bounds->top;
		clip.width = bounds->right - bounds->left + 1;
		clip.height = bounds->bottom - bounds->top + 1;
		XSetClipRectangles(rfi->display, rfi->gc, 0, 0, &clip, 1, YXBanded);
	}
	else
	{
		XSetClipMask(rfi->display, rfi->gc, None);
	}
	*/
}

static void rf_gdi_dstblt(rdpContext* context, DSTBLT_ORDER* dstblt)
{
	g_print("dstblt\n");
}

static void rf_gdi_patblt(rdpContext* context, PATBLT_ORDER* patblt)
{
	g_print("patblt\n");
}

static void rf_gdi_scrblt(rdpContext* context, SCRBLT_ORDER* scrblt)
{
	g_print("srcblt\n");
}

static void rf_gdi_opaque_rect(rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect)
{
	g_print("opaque_rect\n");
}

static void rf_gdi_multi_opaque_rect(rdpContext* context, MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect)
{
	g_print("multi_opaque_rect\n");
}

static void rf_gdi_line_to(rdpContext* context, LINE_TO_ORDER* line_to)
{
	g_print("line_to\n");
}

static void rf_gdi_polyline(rdpContext* context, POLYLINE_ORDER* polyline)
{
	g_print("polyline\n");
}

static void rf_gdi_memblt(rdpContext* context, MEMBLT_ORDER* memblt)
{
	g_print("memblt\n");
}

static void rf_gdi_fast_index(rdpContext* context, FAST_INDEX_ORDER* fast_index)
{
	g_print("fast_index\n");
}

static void rf_gdi_surface_bits(rdpContext* context, SURFACE_BITS_COMMAND* surface_bits_command)
{
	uint8* bitmap;
	RFX_MESSAGE* message;
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*) context;

	if (surface_bits_command->codecID == CODEC_ID_REMOTEFX && rfi->rfx_context)
	{
		message = rfx_process_message(rfi->rfx_context, surface_bits_command->bitmapData,
				surface_bits_command->bitmapDataLength);

		ui = g_new0(RemminaPluginRdpUiObject, 1);
		ui->type = REMMINA_RDP_UI_RFX;
		ui->rfx.left = surface_bits_command->destLeft;
		ui->rfx.top = surface_bits_command->destTop;
		ui->rfx.message = message;

		rf_queue_ui(rfi->protocol_widget, ui);
	}
	else if (surface_bits_command->codecID == CODEC_ID_NONE)
	{
		bitmap = (uint8*) xzalloc(surface_bits_command->width * surface_bits_command->height * 4);

		freerdp_image_flip(surface_bits_command->bitmapData, bitmap,
				surface_bits_command->width, surface_bits_command->height, 32);

		ui = g_new0(RemminaPluginRdpUiObject, 1);
		ui->type = REMMINA_RDP_UI_NOCODEC;
		ui->nocodec.left = surface_bits_command->destLeft;
		ui->nocodec.top = surface_bits_command->destTop;
		ui->nocodec.width = surface_bits_command->width;
		ui->nocodec.height = surface_bits_command->height;
		ui->nocodec.bitmap = bitmap;

		rf_queue_ui(rfi->protocol_widget, ui);
	}
	else
	{
		printf("Unsupported codecID %d\n", surface_bits_command->codecID);
	}
}

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

void rf_pre_connect(RemminaProtocolWidget* gp)
{
	rdpSettings* settings;
	rfContext* rfi;

	rfi = GET_DATA(gp);
	settings = rfi->settings;

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

	if (settings->color_depth == 32)
	{
		settings->rfx_codec = True;
		settings->frame_acknowledge = False;
		settings->large_pointer = True;
		settings->performance_flags = PERF_FLAG_NONE;

		rfi->rfx_context = rfx_context_new();
		rfx_context_set_cpu_opt(rfi->rfx_context, CPU_SSE2);
	}
}

void rf_post_connect(RemminaProtocolWidget* gp)
{
	rdpUpdate* update;
	rdpSettings* settings;
	rdpPrimaryUpdate* primary;
	rdpSecondaryUpdate* secondary;
	rfContext* rfi;
	RemminaPluginRdpUiObject* ui;

	rfi = GET_DATA(gp);
	update = rfi->instance->update;
	primary = update->primary;
	secondary = update->secondary;
	settings = rfi->instance->settings;

	update->DesktopResize = rf_desktop_resize;
	update->Palette = rf_gdi_palette;
	update->SetBounds = rf_gdi_set_bounds;

	primary->DstBlt = rf_gdi_dstblt;
	primary->PatBlt = rf_gdi_patblt;
	primary->ScrBlt = rf_gdi_scrblt;
	primary->OpaqueRect = rf_gdi_opaque_rect;
	primary->DrawNineGrid = NULL;
	primary->MultiDstBlt = NULL;
	primary->MultiPatBlt = NULL;
	primary->MultiScrBlt = NULL;
	primary->MultiOpaqueRect = rf_gdi_multi_opaque_rect;
	primary->MultiDrawNineGrid = NULL;
	primary->LineTo = rf_gdi_line_to;
	primary->Polyline = rf_gdi_polyline;
	primary->MemBlt = rf_gdi_memblt;
	primary->Mem3Blt = NULL;
	primary->SaveBitmap = NULL;
	primary->GlyphIndex = NULL;
	primary->FastIndex = rf_gdi_fast_index;
	primary->FastGlyph = NULL;
	primary->PolygonSC = NULL;
	primary->PolygonCB = NULL;
	primary->EllipseSC = NULL;
	primary->EllipseCB = NULL;

	update->SurfaceBits = rf_gdi_surface_bits;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CONNECTED;

	rfi->srcBpp = settings->color_depth;

	rf_queue_ui(gp, ui);
}

void rf_uninit(RemminaProtocolWidget* gp)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	if (rfi->rfx_context)
	{
		rfx_context_free (rfi->rfx_context);
		rfi->rfx_context = NULL;
	}
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
