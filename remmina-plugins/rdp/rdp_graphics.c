/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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
#include "rdp_graphics.h"

#include <freerdp/codec/color.h>
#include <freerdp/codec/bitmap.h>
#include <winpr/memory.h>

//#define RF_BITMAP
//#define RF_GLYPH

/* Bitmap Class */

BOOL rf_Bitmap_New(rdpContext* context, rdpBitmap* bitmap)
{
	TRACE_CALL("rf_Bitmap_New");
#ifdef RF_BITMAP
	UINT8* data;
	Pixmap pixmap;
	XImage* image;
	rfContext* rfi = (rfContext*) context;

	XSetFunction(rfi->display, rfi->gc, GXcopy);
	pixmap = XCreatePixmap(rfi->display, rfi->drawable, bitmap->width, bitmap->height, rfi->depth);

	if (bitmap->data != NULL)
	{
		data = freerdp_image_convert(bitmap->data, NULL,
				bitmap->width, bitmap->height, rfi->srcBpp, rfi->bpp, rfi->clrconv);

		if (bitmap->ephemeral != TRUE)
		{
			image = XCreateImage(rfi->display, rfi->visual, rfi->depth,
				ZPixmap, 0, (char*) data, bitmap->width, bitmap->height, rfi->scanline_pad, 0);

			XPutImage(rfi->display, pixmap, rfi->gc, image, 0, 0, 0, 0, bitmap->width, bitmap->height);
			XFree(image);

			if (data != bitmap->data) && (data != NULL)
				free(data);
		}
		else
		{
			if (data != bitmap->data) && (data != NULL)
				free(bitmap->data);

			bitmap->data = data;
		}
	}

	((rfBitmap*) bitmap)->pixmap = pixmap;
#endif
	return TRUE;
}

void rf_Bitmap_Free(rdpContext* context, rdpBitmap* bitmap)
{
	TRACE_CALL("rf_Bitmap_Free");
#ifdef RF_BITMAP
	rfContext* rfi = (rfContext*) context;

	printf("rf_Bitmap_Free\n");

	if (((rfBitmap*) bitmap)->pixmap != 0)
		XFreePixmap(rfi->display, ((rfBitmap*) bitmap)->pixmap);
#endif
}

BOOL rf_Bitmap_Paint(rdpContext* context, rdpBitmap* bitmap)
{
	TRACE_CALL("rf_Bitmap_Paint");
#ifdef RF_BITMAP
	XImage* image;
	int width, height;
	rfContext* rfi = (rfContext*) context;

	printf("rf_Bitmap_Paint\n");

	width = bitmap->right - bitmap->left + 1;
	height = bitmap->bottom - bitmap->top + 1;

	XSetFunction(rfi->display, rfi->gc, GXcopy);

	image = XCreateImage(rfi->display, rfi->visual, rfi->depth,
			ZPixmap, 0, (char*) bitmap->data, bitmap->width, bitmap->height, rfi->scanline_pad, 0);

	XPutImage(rfi->display, rfi->primary, rfi->gc,
			image, 0, 0, bitmap->left, bitmap->top, width, height);

	XFree(image);

	//XCopyArea(rfi->display, rfi->primary, rfi->drawable, rfi->gc,
	//			bitmap->left, bitmap->top, width, height, bitmap->left, bitmap->top);

	//gdi_InvalidateRegion(rfi->hdc, bitmap->left, bitmap->top, width, height);
#endif
	return FALSE;
}

BOOL rf_Bitmap_Decompress(rdpContext* context, rdpBitmap* bitmap,
	const BYTE* data, UINT32 width, UINT32 height, UINT32 bpp, UINT32 length, BOOL compressed, UINT32 codec_id)
{
	TRACE_CALL("rf_Bitmap_Decompress");
#ifdef RF_BITMAP
	UINT16 size;

	printf("rf_Bitmap_Decompress\n");

	size = width * height * (bpp + 7) / 8;

	if (bitmap->data == NULL)
		bitmap->data = (UINT8*) xmalloc(size);
	else
		bitmap->data = (UINT8*) xrealloc(bitmap->data, size);

	if (compressed)
	{
		BOOL status;

		status = bitmap_decompress(data, bitmap->data, width, height, length, bpp, bpp);

		if (status != TRUE)
		{
			printf("Bitmap Decompression Failed\n");
		}
	}
	else
	{
		freerdp_image_flip(data, bitmap->data, width, height, bpp);
	}

	bitmap->compressed = FALSE;
	bitmap->length = size;
	bitmap->bpp = bpp;
#endif
	return TRUE;
}

BOOL rf_Bitmap_SetSurface(rdpContext* context, rdpBitmap* bitmap, BOOL primary)
{
	TRACE_CALL("rf_Bitmap_SetSurface");
#ifdef RF_BITMAP
	rfContext* rfi = (rfContext*) context;

	if (primary)
		rfi->drawing = rfi->primary;
	else
		rfi->drawing = ((rfBitmap*) bitmap)->pixmap;
#endif
	return TRUE;
}

/* Pointer Class */

BOOL rf_Pointer_New(rdpContext* context, rdpPointer* pointer)
{
	TRACE_CALL("rf_Pointer_New");
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*) context;

	if ((pointer->andMaskData != 0) && (pointer->xorMaskData != 0))
	{
		ui = g_new0(RemminaPluginRdpUiObject, 1);
		ui->type = REMMINA_RDP_UI_CURSOR;
		ui->sync = TRUE;	// Also wait for completion
		ui->cursor.context = context;
		ui->cursor.pointer = (rfPointer*) pointer;
		ui->cursor.type = REMMINA_RDP_POINTER_NEW;
		return remmina_rdp_event_queue_ui(rfi->protocol_widget, ui) ? TRUE : FALSE;
	}
	return FALSE;
}

void rf_Pointer_Free(rdpContext* context, rdpPointer* pointer)
{
	TRACE_CALL("rf_Pointer_Free");
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*) context;

#if GTK_VERSION == 2
	if (((rfPointer*) pointer)->cursor != NULL)
#else
	if (G_IS_OBJECT(((rfPointer*) pointer)->cursor))
#endif
	{
		ui = g_new0(RemminaPluginRdpUiObject, 1);
		ui->type = REMMINA_RDP_UI_CURSOR;
		ui->sync = TRUE;	// Also wait for completion
		ui->cursor.context = context;
		ui->cursor.pointer = (rfPointer*) pointer;
		ui->cursor.type = REMMINA_RDP_POINTER_FREE;

		remmina_rdp_event_queue_ui(rfi->protocol_widget, ui);
	}
}

BOOL rf_Pointer_Set(rdpContext* context, const rdpPointer* pointer)
{
	TRACE_CALL("rf_Pointer_Set");
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*) context;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->sync = TRUE;	// Also wait for completion
	ui->cursor.pointer = (rfPointer*) pointer;
	ui->cursor.type = REMMINA_RDP_POINTER_SET;

	return remmina_rdp_event_queue_ui(rfi->protocol_widget, ui) ? TRUE : FALSE;

}

BOOL rf_Pointer_SetNull(rdpContext* context)
{
	TRACE_CALL("rf_Pointer_SetNull");
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*) context;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->sync = TRUE;	// Also wait for completion
	ui->cursor.type = REMMINA_RDP_POINTER_NULL;

	return remmina_rdp_event_queue_ui(rfi->protocol_widget, ui) ? TRUE : FALSE;
}

BOOL rf_Pointer_SetDefault(rdpContext* context)
{
	TRACE_CALL("rf_Pointer_SetDefault");
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*) context;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->sync = TRUE;	// Also wait for completion
	ui->cursor.type = REMMINA_RDP_POINTER_DEFAULT;

	return remmina_rdp_event_queue_ui(rfi->protocol_widget, ui) ? TRUE : FALSE;
}

BOOL rf_Pointer_SetPosition(rdpContext* context, UINT32 x, UINT32 y)
{
	TRACE_CALL("rf_Pointer_SePosition");
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*) context;
	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->sync = TRUE;	// Also wait for completion
	ui->cursor.type = REMMINA_RDP_POINTER_SETPOS;
	ui->pos.x = x;
	ui->pos.y = y;

	return remmina_rdp_event_queue_ui(rfi->protocol_widget, ui) ? TRUE : FALSE;
}

/* Glyph Class */

BOOL rf_Glyph_New(rdpContext* context, const rdpGlyph* glyph)
{
	TRACE_CALL("rf_Glyph_New");
#ifdef RF_GLYPH
	int scanline;
	XImage* image;
	rfContext* rfi;
	rfGlyph* rf_glyph;

	rf_glyph = (rfGlyph*) glyph;
	rfi = (rfContext*) context;

	scanline = (glyph->cx + 7) / 8;

	rf_glyph->pixmap = XCreatePixmap(rfi->display, rfi->drawing, glyph->cx, glyph->cy, 1);

	image = XCreateImage(rfi->display, rfi->visual, 1,
			ZPixmap, 0, (char*) glyph->aj, glyph->cx, glyph->cy, 8, scanline);

	image->byte_order = MSBFirst;
	image->bitmap_bit_order = MSBFirst;

	XInitImage(image);
	XPutImage(rfi->display, rf_glyph->pixmap, rfi->gc_mono, image, 0, 0, 0, 0, glyph->cx, glyph->cy);
	XFree(image);
#endif
	return TRUE;
}

void rf_Glyph_Free(rdpContext* context, rdpGlyph* glyph)
{
	TRACE_CALL("rf_Glyph_Free");
#ifdef RF_GLYPH
	rfContext* rfi = (rfContext*) context;

	if (((rfGlyph*) glyph)->pixmap != 0)
		XFreePixmap(rfi->display, ((rfGlyph*) glyph)->pixmap);
#endif
}

static BOOL rf_Glyph_Draw(rdpContext* context, const rdpGlyph* glyph, UINT32 x,
		UINT32 y, UINT32 w, UINT32 h, UINT32 sx, UINT32 sy,
		BOOL fOpRedundant)
{
	TRACE_CALL("rf_Glyph_Draw");
#ifdef RF_GLYPH
	rfGlyph* rf_glyph;
	rfContext* rfi = (rfContext*) context;

	rf_glyph = (rfGlyph*) glyph;

	XSetStipple(rfi->display, rfi->gc, rf_glyph->pixmap);
	XSetTSOrigin(rfi->display, rfi->gc, x, y);
	XFillRectangle(rfi->display, rfi->drawing, rfi->gc, x, y, glyph->cx, glyph->cy);
	XSetStipple(rfi->display, rfi->gc, rfi->bitmap_mono);
#endif
	return TRUE;
}

static BOOL rf_Glyph_BeginDraw(rdpContext* context, UINT32 x, UINT32 y,
                               UINT32 width, UINT32 height, UINT32 bgcolor,
                               UINT32 fgcolor, BOOL fOpRedundant)
{
	TRACE_CALL("rf_Glyph_BeginDraw");
#ifdef RF_GLYPH
	rfContext* rfi = (rfContext*) context;

	bgcolor = (rfi->clrconv->invert)?
		freerdp_color_convert_var_bgr(bgcolor, rfi->srcBpp, 32, rfi->clrconv):
		freerdp_color_convert_var_rgb(bgcolor, rfi->srcBpp, 32, rfi->clrconv);

	fgcolor = (rfi->clrconv->invert)?
		freerdp_color_convert_var_bgr(fgcolor, rfi->srcBpp, 32, rfi->clrconv):
		freerdp_color_convert_var_rgb(fgcolor, rfi->srcBpp, 32, rfi->clrconv);

	XSetFunction(rfi->display, rfi->gc, GXcopy);
	XSetFillStyle(rfi->display, rfi->gc, FillSolid);
	XSetForeground(rfi->display, rfi->gc, fgcolor);
	XFillRectangle(rfi->display, rfi->drawing, rfi->gc, x, y, width, height);

	XSetForeground(rfi->display, rfi->gc, bgcolor);
	XSetBackground(rfi->display, rfi->gc, fgcolor);
	XSetFillStyle(rfi->display, rfi->gc, FillStippled);
#endif
	return TRUE;
}

static BOOL rf_Glyph_EndDraw(rdpContext* context, UINT32 x, UINT32 y,
                             UINT32 width, UINT32 height,
                             UINT32 bgcolor, UINT32 fgcolor)
{
	TRACE_CALL("rf_Glyph_EndDraw");
#ifdef RF_GLYPH
	rfContext* rfi = (rfContext*) context;

	if (rfi->drawing == rfi->primary)
	{
		//XCopyArea(rfi->display, rfi->primary, rfi->drawable, rfi->gc, x, y, width, height, x, y);
		//gdi_InvalidateRegion(rfi->hdc, x, y, width, height);
	}
#endif
	return TRUE;
}

/* Graphics Module */

void rf_register_graphics(rdpGraphics* graphics)
{
	TRACE_CALL("rf_register_graphics");
	rdpBitmap* bitmap;
	rdpPointer* pointer;
	rdpGlyph* glyph;

	bitmap = (rdpBitmap*) malloc(sizeof(rdpBitmap));
	ZeroMemory(bitmap, sizeof(rdpBitmap));
	bitmap->size = sizeof(rfBitmap);

	bitmap->New = rf_Bitmap_New;
	bitmap->Free = rf_Bitmap_Free;
	bitmap->Paint = rf_Bitmap_Paint;
	bitmap->Decompress = rf_Bitmap_Decompress;
	bitmap->SetSurface = rf_Bitmap_SetSurface;

	graphics_register_bitmap(graphics, bitmap);
	free(bitmap);

	pointer = (rdpPointer*) malloc(sizeof(rdpPointer));
	ZeroMemory(pointer, sizeof(rdpPointer));

	pointer->size = sizeof(rfPointer);

	pointer->New = rf_Pointer_New;
	pointer->Free = rf_Pointer_Free;
	pointer->Set = rf_Pointer_Set;
	pointer->SetNull = rf_Pointer_SetNull;
	pointer->SetDefault = rf_Pointer_SetDefault;
	pointer->SetPosition = rf_Pointer_SetPosition;

	graphics_register_pointer(graphics, pointer);

	free(pointer);

	glyph = (rdpGlyph*) malloc(sizeof(rdpGlyph));
	ZeroMemory(glyph, sizeof(rdpGlyph));

	glyph->size = sizeof(rfGlyph);

	glyph->New = rf_Glyph_New;
	glyph->Free = rf_Glyph_Free;
	glyph->Draw = rf_Glyph_Draw;
	glyph->BeginDraw = rf_Glyph_BeginDraw;
	glyph->EndDraw = rf_Glyph_EndDraw;

	graphics_register_glyph(graphics, glyph);

	free(glyph);
}
