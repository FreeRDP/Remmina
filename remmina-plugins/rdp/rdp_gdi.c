/*  See LICENSE and COPYING files for copyright and license details. */

/* rdp gdi functions, run inside the rdp thread */

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_gdi.h"
#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/cache/cache.h>

static void rf_desktop_resize(rdpContext* context)
{
	TRACE_CALL("rf_desktop_resize");
	RemminaProtocolWidget* gp;
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi;

	rfi = (rfContext*) context;
	gp = rfi->protocol_widget;

	LOCK_BUFFER(TRUE)

	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->DesktopWidth);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->DesktopHeight);

	UNLOCK_BUFFER(TRUE)

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->sync = TRUE;	// Wait for completion too
	ui->type = REMMINA_RDP_UI_EVENT;
	ui->event.type = REMMINA_RDP_UI_EVENT_UPDATE_SCALE;
	rf_queue_ui(gp, ui);

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "desktop-resize");
}

static void rf_gdi_palette(rdpContext* context, PALETTE_UPDATE* palette)
{
	TRACE_CALL("rf_gdi_palette");
	g_print("palette\n");
}

static void rf_gdi_set_bounds(rdpContext* context, rdpBounds* bounds)
{
	TRACE_CALL("rf_gdi_set_bounds");
}

static void rf_gdi_dstblt(rdpContext* context, DSTBLT_ORDER* dstblt)
{
	TRACE_CALL("rf_gdi_dstblt");
	g_print("dstblt\n");
}

static void rf_gdi_patblt(rdpContext* context, PATBLT_ORDER* patblt)
{
	TRACE_CALL("rf_gdi_patblt");
	g_print("patblt\n");
}

static void rf_gdi_scrblt(rdpContext* context, SCRBLT_ORDER* scrblt)
{
	TRACE_CALL("rf_gdi_scrblt");
	g_print("srcblt\n");
}

static void rf_gdi_opaque_rect(rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect)
{
	TRACE_CALL("rf_gdi_opaque_rect");
	g_print("opaque_rect\n");
}

static void rf_gdi_multi_opaque_rect(rdpContext* context, MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect)
{
	TRACE_CALL("rf_gdi_multi_opaque_rect");
	g_print("multi_opaque_rect\n");
}

static void rf_gdi_line_to(rdpContext* context, LINE_TO_ORDER* line_to)
{
	TRACE_CALL("rf_gdi_line_to");
	g_print("line_to\n");
}

static void rf_gdi_polyline(rdpContext* context, POLYLINE_ORDER* polyline)
{
	TRACE_CALL("rf_gdi_polyline");
	g_print("polyline\n");
}

static void rf_gdi_memblt(rdpContext* context, MEMBLT_ORDER* memblt)
{
	TRACE_CALL("rf_gdi_memblt");
	g_print("memblt\n");
}

static void rf_gdi_fast_index(rdpContext* context, FAST_INDEX_ORDER* fast_index)
{
	TRACE_CALL("rf_gdi_fast_index");
	g_print("fast_index\n");
}

static void rf_gdi_surface_bits(rdpContext* context, SURFACE_BITS_COMMAND* surface_bits_command)
{
	TRACE_CALL("rf_gdi_surface_bits");
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
	TRACE_CALL("rf_gdi_register_update_callbacks");
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
