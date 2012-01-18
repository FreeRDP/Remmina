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

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_graphics.h"

#include <freerdp/utils/memory.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/bitmap.h>

//#define RF_BITMAP
//#define RF_GLYPH

/* Bitmap Class */

void rf_Bitmap_New(rdpContext* context, rdpBitmap* bitmap)
{
#ifdef RF_BITMAP
	uint8* data;
	Pixmap pixmap;
	XImage* image;
	rfContext* rfi = (rfContext*) context;

	XSetFunction(rfi->display, rfi->gc, GXcopy);
	pixmap = XCreatePixmap(rfi->display, rfi->drawable, bitmap->width, bitmap->height, rfi->depth);

	if (bitmap->data != NULL)
	{
		data = freerdp_image_convert(bitmap->data, NULL,
				bitmap->width, bitmap->height, rfi->srcBpp, rfi->bpp, rfi->clrconv);

		if (bitmap->ephemeral != true)
		{
			image = XCreateImage(rfi->display, rfi->visual, rfi->depth,
				ZPixmap, 0, (char*) data, bitmap->width, bitmap->height, rfi->scanline_pad, 0);

			XPutImage(rfi->display, pixmap, rfi->gc, image, 0, 0, 0, 0, bitmap->width, bitmap->height);
			XFree(image);

			if (data != bitmap->data)
				xfree(data);
		}
		else
		{
			if (data != bitmap->data)
				xfree(bitmap->data);

			bitmap->data = data;
		}
	}

	((rfBitmap*) bitmap)->pixmap = pixmap;
#endif
}

void rf_Bitmap_Free(rdpContext* context, rdpBitmap* bitmap)
{
#ifdef RF_BITMAP
	rfContext* rfi = (rfContext*) context;

	printf("rf_Bitmap_Free\n");

	if (((rfBitmap*) bitmap)->pixmap != 0)
		XFreePixmap(rfi->display, ((rfBitmap*) bitmap)->pixmap);
#endif
}

void rf_Bitmap_Paint(rdpContext* context, rdpBitmap* bitmap)
{
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
}

void rf_Bitmap_Decompress(rdpContext* context, rdpBitmap* bitmap,
	uint8* data, int width, int height, int bpp, int length, boolean compressed)
{
#ifdef RF_BITMAP
	uint16 size;

	printf("rf_Bitmap_Decompress\n");

	size = width * height * (bpp + 7) / 8;

	if (bitmap->data == NULL)
		bitmap->data = (uint8*) xmalloc(size);
	else
		bitmap->data = (uint8*) xrealloc(bitmap->data, size);

	if (compressed)
	{
		boolean status;

		status = bitmap_decompress(data, bitmap->data, width, height, length, bpp, bpp);

		if (status != true)
		{
			printf("Bitmap Decompression Failed\n");
		}
	}
	else
	{
		freerdp_image_flip(data, bitmap->data, width, height, bpp);
	}

	bitmap->compressed = false;
	bitmap->length = size;
	bitmap->bpp = bpp;
#endif
}

void rf_Bitmap_SetSurface(rdpContext* context, rdpBitmap* bitmap, boolean primary)
{
#ifdef RF_BITMAP
	rfContext* rfi = (rfContext*) context;

	if (primary)
		rfi->drawing = rfi->primary;
	else
		rfi->drawing = ((rfBitmap*) bitmap)->pixmap;
#endif
}

/* Pointer Class */

void rf_Pointer_New(rdpContext* context, rdpPointer* pointer)
{

}

void rf_Pointer_Free(rdpContext* context, rdpPointer* pointer)
{

}

void rf_Pointer_Set(rdpContext* context, rdpPointer* pointer)
{

}

/* Glyph Class */

void rf_Glyph_New(rdpContext* context, rdpGlyph* glyph)
{
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
}

void rf_Glyph_Free(rdpContext* context, rdpGlyph* glyph)
{
#ifdef RF_GLYPH
	rfContext* rfi = (rfContext*) context;

	if (((rfGlyph*) glyph)->pixmap != 0)
		XFreePixmap(rfi->display, ((rfGlyph*) glyph)->pixmap);
#endif
}

void rf_Glyph_Draw(rdpContext* context, rdpGlyph* glyph, int x, int y)
{
#ifdef RF_GLYPH
	rfGlyph* rf_glyph;
	rfContext* rfi = (rfContext*) context;

	rf_glyph = (rfGlyph*) glyph;

	XSetStipple(rfi->display, rfi->gc, rf_glyph->pixmap);
	XSetTSOrigin(rfi->display, rfi->gc, x, y);
	XFillRectangle(rfi->display, rfi->drawing, rfi->gc, x, y, glyph->cx, glyph->cy);
	XSetStipple(rfi->display, rfi->gc, rfi->bitmap_mono);
#endif
}

void rf_Glyph_BeginDraw(rdpContext* context, int x, int y, int width, int height, uint32 bgcolor, uint32 fgcolor)
{
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
}

void rf_Glyph_EndDraw(rdpContext* context, int x, int y, int width, int height, uint32 bgcolor, uint32 fgcolor)
{
#ifdef RF_GLYPH
	rfContext* rfi = (rfContext*) context;

	if (rfi->drawing == rfi->primary)
	{
		//XCopyArea(rfi->display, rfi->primary, rfi->drawable, rfi->gc, x, y, width, height, x, y);
		//gdi_InvalidateRegion(rfi->hdc, x, y, width, height);
	}
#endif
}

/* Graphics Module */

void rf_register_graphics(rdpGraphics* graphics)
{
	rdpBitmap* bitmap;
	rdpPointer* pointer;
	rdpGlyph* glyph;

	bitmap = xnew(rdpBitmap);
	bitmap->size = sizeof(rfBitmap);

	bitmap->New = rf_Bitmap_New;
	bitmap->Free = rf_Bitmap_Free;
	bitmap->Paint = rf_Bitmap_Paint;
	bitmap->Decompress = rf_Bitmap_Decompress;
	bitmap->SetSurface = rf_Bitmap_SetSurface;

	graphics_register_bitmap(graphics, bitmap);
	xfree(bitmap);

	pointer = xnew(rdpPointer);
	pointer->size = sizeof(rfPointer);

	pointer->New = rf_Pointer_New;
	pointer->Free = rf_Pointer_Free;
	pointer->Set = rf_Pointer_Set;

	graphics_register_pointer(graphics, pointer);
	xfree(pointer);

	glyph = xnew(rdpGlyph);
	glyph->size = sizeof(rfGlyph);

	glyph->New = rf_Glyph_New;
	glyph->Free = rf_Glyph_Free;
	glyph->Draw = rf_Glyph_Draw;
	glyph->BeginDraw = rf_Glyph_BeginDraw;
	glyph->EndDraw = rf_Glyph_EndDraw;

	graphics_register_glyph(graphics, glyph);
	xfree(glyph);
}
