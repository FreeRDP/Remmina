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

/* rdp gdi functions, run inside the rdp thread */

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_gdi.h"
#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/cache/cache.h>

static void rf_desktop_resize(rdpContext* context)
{
	RemminaProtocolWidget* gp;
	rfContext* rfi;

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

static void rf_gdi_palette(rdpContext* context, PALETTE_UPDATE* palette)
{
	g_print("palette\n");
}

static void rf_gdi_set_bounds(rdpContext* context, rdpBounds* bounds)
{
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
	UINT8* bitmap;
	RFX_MESSAGE* message;
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*) context;

	if (surface_bits_command->codecID == RDP_CODEC_ID_REMOTEFX && rfi->rfx_context)
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
	else if (surface_bits_command->codecID == RDP_CODEC_ID_NONE)
	{
		bitmap = (UINT8*) calloc(1, surface_bits_command->width * surface_bits_command->height * 4);

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

void rf_gdi_register_update_callbacks(rdpUpdate* update)
{
	rdpPrimaryUpdate* primary;
	rdpSecondaryUpdate* secondary;

	primary = update->primary;
	secondary = update->secondary;

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
}
