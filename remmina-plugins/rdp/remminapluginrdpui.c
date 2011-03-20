/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
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

/* rdp ui functions, run inside the rdp thread */

#include "remminapluginrdp.h"
#include "remminapluginrdpev.h"
#include "remminapluginrdpui.h"
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <gdk/gdkx.h>

/******* RDP to Xlib color conversion (ported from xfreerdp) *********/
#define SPLIT32BGR(_alpha, _red, _green, _blue, _pixel) \
  _red = _pixel & 0xff; \
  _green = (_pixel & 0xff00) >> 8; \
  _blue = (_pixel & 0xff0000) >> 16; \
  _alpha = (_pixel & 0xff000000) >> 24;

#define SPLIT24BGR(_red, _green, _blue, _pixel) \
  _red = _pixel & 0xff; \
  _green = (_pixel & 0xff00) >> 8; \
  _blue = (_pixel & 0xff0000) >> 16;

#define SPLIT24RGB(_red, _green, _blue, _pixel) \
  _blue  = _pixel & 0xff; \
  _green = (_pixel & 0xff00) >> 8; \
  _red   = (_pixel & 0xff0000) >> 16;

#define SPLIT16RGB(_red, _green, _blue, _pixel) \
  _red = ((_pixel >> 8) & 0xf8) | ((_pixel >> 13) & 0x7); \
  _green = ((_pixel >> 3) & 0xfc) | ((_pixel >> 9) & 0x3); \
  _blue = ((_pixel << 3) & 0xf8) | ((_pixel >> 2) & 0x7);

#define SPLIT15RGB(_red, _green, _blue, _pixel) \
  _red = ((_pixel >> 7) & 0xf8) | ((_pixel >> 12) & 0x7); \
  _green = ((_pixel >> 2) & 0xf8) | ((_pixel >> 8) & 0x7); \
  _blue = ((_pixel << 3) & 0xf8) | ((_pixel >> 2) & 0x7);

#define MAKE32RGB(_alpha, _red, _green, _blue) \
  (_alpha << 24) | (_red << 16) | (_green << 8) | _blue;

#define MAKE24RGB(_red, _green, _blue) \
  (_red << 16) | (_green << 8) | _blue;

#define MAKE15RGB(_red, _green, _blue) \
  (((_red & 0xff) >> 3) << 10) | \
  (((_green & 0xff) >> 3) <<  5) | \
  (((_blue & 0xff) >> 3) <<  0)

#define MAKE16RGB(_red, _green, _blue) \
  (((_red & 0xff) >> 3) << 11) | \
  (((_green & 0xff) >> 2) <<  5) | \
  (((_blue & 0xff) >> 3) <<  0)

static gint
remmina_plugin_rdpui_color_convert (RemminaPluginRdpData *gpdata, gint color)
{
    gint alpha;
    gint red;
    gint green;
    gint blue;
    gint rv;

    alpha = 0xff;
    red = 0;
    green = 0;
    blue = 0;
    rv = 0;
    switch (gpdata->settings->server_depth)
    {
        case 32:
            SPLIT32BGR(alpha, red, green, blue, color);
            break;
        case 24:
            SPLIT24BGR(red, green, blue, color);
            break;
        case 16:
            SPLIT16RGB(red, green, blue, color);
            break;
        case 15:
            SPLIT15RGB(red, green, blue, color);
            break;
        case 8:
            color &= 0xff;
            SPLIT24RGB(red, green, blue, gpdata->colormap[color]);
            break;
        case 1:
            if (color != 0)
            {
                red = 0xff;
                green = 0xff;
                blue = 0xff;
            }
            break;
        default:
            remmina_plugin_service->log_printf ("[RDP]unsupported server bpp %i\n", gpdata->settings->server_depth);
            break;
    }
    switch (gpdata->bpp)
    {
        case 32:
            rv = MAKE32RGB(alpha, red, green, blue);
            break;
        case 24:
            rv = MAKE24RGB(red, green, blue);
            break;
        case 16:
            rv = MAKE16RGB(red, green, blue);
            break;
        case 15:
            rv = MAKE15RGB(red, green, blue);
            break;
        case 1:
            if ((red != 0) || (green != 0) || (blue != 0))
            {
                rv = 1;
            }
            break;
        default:
            remmina_plugin_service->log_printf ("[RDP]unsupported client bpp %i\n", gpdata->bpp);
            break;
    }
    return rv;
}

static guchar *
remmina_plugin_rdpui_image_convert (RemminaPluginRdpData *gpdata, gint width, gint height, guchar *in_data)
{
    gint red;
    gint green;
    gint blue;
    gint index;
    gint pixel;
    guchar *out_data;
    guchar *src8;
    guchar *dst8;

    if ((gpdata->settings->server_depth == 24) && (gpdata->bpp == 32))
    {
        out_data = g_new (guchar, width * height * 4);
        src8 = in_data;
        dst8 = out_data;
        for (index = width * height; index > 0; index--)
        {
            blue = *(src8++);
            green = *(src8++);
            red = *(src8++);
            pixel = MAKE24RGB(red, green, blue);
            *dst8++ = pixel & 0xff;
            *dst8++ = (pixel >> 8) & 0xff;
            *dst8++ = (pixel >> 16) & 0xff;
            *dst8++ = (pixel >> 24) & 0xff;
        }
        return out_data;
    }
    else if ((gpdata->settings->server_depth == 16) && (gpdata->bpp == 32))
    {
        out_data = g_new (guchar, width * height * 4);
        src8 = in_data;
        dst8 = out_data;
        for (index = width * height; index > 0; index--)
        {
            pixel = *src8++;
            pixel |= (*src8++) << 8;
            SPLIT16RGB(red, green, blue, pixel);
            pixel = MAKE24RGB(red, green, blue);
            *dst8++ = pixel & 0xff;
            *dst8++ = (pixel >> 8) & 0xff;
            *dst8++ = (pixel >> 16) & 0xff;
            *dst8++ = (pixel >> 24) & 0xff;
        }
        return out_data;
    }
    else if ((gpdata->settings->server_depth == 15) && (gpdata->bpp == 32))
    {
        out_data = g_new (guchar, width * height * 4);
        src8 = in_data;
        dst8 = out_data;
        for (index = width * height; index > 0; index--)
        {
            pixel = *src8++;
            pixel |= (*src8++) << 8;
            SPLIT15RGB(red, green, blue, pixel);
            pixel = MAKE24RGB(red, green, blue);
            *dst8++ = pixel & 0xff;
            *dst8++ = (pixel >> 8) & 0xff;
            *dst8++ = (pixel >> 16) & 0xff;
            *dst8++ = (pixel >> 24) & 0xff;
        }
        return out_data;
    }
    else if ((gpdata->settings->server_depth == 8) && (gpdata->bpp == 32))
    {
        out_data = g_new (guchar, width * height * 4);
        src8 = in_data;
        dst8 = out_data;
        for (index = width * height; index > 0; index--)
        {
            pixel = *src8;
            src8++;
            pixel = gpdata->colormap[pixel];
            *dst8++ = pixel & 0xff;
            *dst8++ = (pixel >> 8) & 0xff;
            *dst8++ = (pixel >> 16) & 0xff;
            *dst8++ = (pixel >> 24) & 0xff;
        }
        return out_data;
    }
    else if ((gpdata->settings->server_depth == 15) && (gpdata->bpp == 16))
    {
        out_data = g_new (guchar, width * height * 2);
        src8 = in_data;
        dst8 = out_data;
        for (index = width * height; index > 0; index--)
        {
            pixel = *src8++;
            pixel |= (*src8++) << 8;
            SPLIT15RGB(red, green, blue, pixel);
            pixel = MAKE16RGB(red, green, blue);
            *dst8++ = pixel & 0xff;
            *dst8++ = (pixel >> 8) & 0xff;
        }
        return out_data;
    }
    else if ((gpdata->settings->server_depth == 8) && (gpdata->bpp == 16))
    {
        out_data = g_new (guchar, width * height * 2);
        src8 = in_data;
        dst8 = out_data;
        for (index = width * height; index > 0; index--)
        {
            pixel = *src8;
            src8++;
            pixel = gpdata->colormap[pixel];
            SPLIT24RGB(red, green, blue, pixel);
            pixel = MAKE16RGB(red, green, blue);
            *dst8++ = pixel & 0xff;
            *dst8++ = (pixel >> 8) & 0xff;
        }
        return out_data;
    }
    else if ((gpdata->settings->server_depth == 8) && (gpdata->bpp == 15))
    {
        out_data = g_new (guchar, width * height * 2);
        src8 = in_data;
        dst8 = out_data;
        for (index = width * height; index > 0; index--)
        {
            pixel = *src8;
            src8++;
            pixel = gpdata->colormap[pixel];
            SPLIT24RGB(red, green, blue, pixel);
            pixel = MAKE15RGB(red, green, blue);
            *dst8++ = pixel & 0xff;
            *dst8++ = (pixel >> 8) & 0xff;
        }
        return out_data;
    }
    else
    {
        return g_memdup (in_data, width * height * ((gpdata->settings->server_depth + 7) / 8));
    }
}

/*********************************************************************/
#define GLYPH_PIXEL(_rowdata,_x) (_rowdata[(_x) / 8] & (0x80 >> ((_x) % 8)))

static guchar hatch_patterns[] = {
    0x00, 0x00, 0x00, 0xff, 0x00, 0x00, 0x00, 0x00, /* 0 - bsHorizontal */
    0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, /* 1 - bsVertical */
    0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01, /* 2 - bsFDiagonal */
    0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, /* 3 - bsBDiagonal */
    0x08, 0x08, 0x08, 0xff, 0x08, 0x08, 0x08, 0x08, /* 4 - bsCross */
    0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81  /* 5 - bsDiagCross */
};

static void
remmina_plugin_rdpui_bitmap_flip (GdkPixbuf *pixbuf)
{
    guchar *data;
    guchar *buf;
    gint rowstride;
    gint width;
    gint height;
    gint iy;
    gint cy;

    data = gdk_pixbuf_get_pixels (pixbuf);
    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    buf = (guchar *) malloc (rowstride);
    cy = height / 2;
    for (iy = 0; iy < cy; iy++)
    {
        memcpy (buf, data + iy * rowstride, rowstride);
        memcpy (data + iy * rowstride, data + (height - iy - 1) * rowstride, rowstride);
        memcpy (data + (height - iy - 1) * rowstride, buf, rowstride);
    }
    free (buf);
}

static GdkPixbuf*
remmina_plugin_rdpui_bitmap_convert (RemminaPluginRdpData *gpdata,
    gint width, gint height, gint bpp, gboolean alpha, guchar *in_data)
{
    GdkPixbuf *pixbuf;
    guchar *out_data;
    gint ix;
    gint iy;
    gint rowstride;
    guchar *src;
    guchar *dst;

    if (bpp == 0)
    {
        bpp = gpdata->settings->server_depth;
    }
    pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, alpha, 8, width, height);
    out_data = gdk_pixbuf_get_pixels (pixbuf);
    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    switch (bpp)
    {
    case 32:
        src = in_data;
        for (iy = 0; iy < height; iy++)
        {
            dst = out_data + iy * rowstride;
            for (ix = 0; ix < width; ix++)
            {
                *dst++ = *(src + 2);
                *dst++ = *(src + 1);
                *dst++ = *src;
                if (alpha) *dst++ = 0xff;
                src += 4;
            }
        }
        break;

    case 24:
        src = in_data;
        for (iy = 0; iy < height; iy++)
        {
            dst = out_data + iy * rowstride;
            for (ix = 0; ix < width; ix++)
            {
                *dst++ = *(src + 2);
                *dst++ = *(src + 1);
                *dst++ = *src;
                if (alpha) *dst++ = 0xff;
                src += 3;
            }
        }
        break;

    case 16:
        src = in_data;
        for (iy = 0; iy < height; iy++)
        {
            dst = out_data + iy * rowstride;
            for (ix = 0; ix < width; ix++)
            {
                *dst++ = (*(src + 1) & 0xf8) | ((*(src + 1) & 0xe0) >> 5);
                *dst++ = ((*(src + 1) & 0x07) << 5) | ((*src & 0xe0) >> 3) | ((*(src + 1) & 0x06) >> 1);
                *dst++ = ((*src & 0x1f) << 3) | ((*src & 0x1c) >> 2);
                if (alpha) *dst++ = 0xff;
                src += 2;
            }
        }
        break;

    case 15:
        src = in_data;
        for (iy = 0; iy < height; iy++)
        {
            dst = out_data + iy * rowstride;
            for (ix = 0; ix < width; ix++)
            {
                *dst++ = ((*(src + 1) & 0x7c) << 1) | ((*(src + 1) & 0x70) >> 4);
                *dst++ = ((*(src + 1) & 0x03) << 6) | ((*src & 0xe0) >> 2) | (*(src + 1) & 0x03);
                *dst++ = ((*src & 0x1f) << 3) | ((*src & 0x1c) >> 2);
                if (alpha) *dst++ = 0xff;
                src += 2;
            }
        }
        break;

    case 8:
        src = in_data;
        for (iy = 0; iy < height; iy++)
        {
            dst = out_data + iy * rowstride;
            for (ix = 0; ix < width; ix++)
            {
                memcpy (dst, gpdata->colormap + (*src++) * 3, 3);
                dst += 3;
                if (alpha) *dst++ = 0xff;
            }
        }
        break;

    case 1:
        for (iy = 0; iy < height; iy++)
        {
            dst = out_data + iy * rowstride;
            src = in_data + (iy * ((width + 7) / 8));
            for (ix = 0; ix < width; ix++)
            {
                memset (dst, (GLYPH_PIXEL (src, ix) ? 0xff : 0), 3);
                dst += 3;
                if (alpha) *dst++ = 0xff;
            }
        }
        break;
    }
    return pixbuf;
}

static void
remmina_plugin_rdpui_bitmap_apply_mask (GdkPixbuf *pixbuf, guchar *mask)
{
    guchar *data;
    gint width;
    gint height;
    gint ix;
    gint iy;
    gint rowstride;
    guchar *src;
    guchar *dst;

    data = gdk_pixbuf_get_pixels (pixbuf);
    rowstride = gdk_pixbuf_get_rowstride (pixbuf);
    width = gdk_pixbuf_get_width (pixbuf);
    height = gdk_pixbuf_get_height (pixbuf);
    for (iy = 0; iy < height; iy++)
    {
        dst = data + iy * rowstride;
        src = mask + (iy * ((width + 7) / 8));
        for (ix = 0; ix < width; ix++)
        {
            if (GLYPH_PIXEL (src, ix))
            {
                *(dst + 3) = (*dst + *(dst + 1) + *(dst + 2)) / 3;
                *dst = ~(*dst);
                *(dst + 1) = ~(*(dst + 1));
                *(dst + 2) = ~(*(dst + 2));
            }
            else
            {
                *(dst + 3) = 0xff;
            }
            dst += 4;
        }
    }
}

static void
remmina_plugin_rdpui_queue_ui (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    g_async_queue_push (gpdata->ui_queue, ui);

    LOCK_BUFFER (TRUE)
    if (!gpdata->ui_handler)
    {
        gpdata->ui_handler = IDLE_ADD ((GSourceFunc) remmina_plugin_rdpev_queue_ui, gp);
    }
    UNLOCK_BUFFER (TRUE)
}

static void
remmina_plugin_rdpui_error (rdpInst *inst, const char *text)
{
    remmina_plugin_service->log_printf ("[RDP]%s", text);
}

static void
remmina_plugin_rdpui_begin_update (rdpInst *inst)
{
}

static void
remmina_plugin_rdpui_end_update (rdpInst *inst)
{
}

static void
remmina_plugin_rdpui_desktop_save (rdpInst *inst, int offset, int x, int y,
    int cx, int cy)
{
    printf("ui_desktop_save:\n");
}

static void
remmina_plugin_rdpui_desktop_restore (rdpInst *inst, int offset, int x, int y,
    int cx, int cy)
{
    printf("ui_desktop_restore:\n");
}

static RD_HGLYPH
remmina_plugin_rdpui_create_glyph (rdpInst *inst, int width, int height, uint8 * data)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;
    guint object_id;
    gint scanline;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    object_id = ++gpdata->object_id_seq;
    scanline = (width + 7) / 8;
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_CREATE_GLYPH;
    ui->object_id = object_id;
    ui->width = width;
    ui->height = height;
    ui->data = g_memdup (data, scanline * height);
    remmina_plugin_rdpui_queue_ui (gp, ui);
    return (RD_HGLYPH) object_id;
}

static void
remmina_plugin_rdpui_destroy_glyph (rdpInst *inst, RD_HGLYPH glyph)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_DESTROY_GLYPH;
    ui->object_id = (guint) glyph;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static RD_HBITMAP
remmina_plugin_rdpui_create_bitmap (rdpInst *inst, int width, int height, uint8 * data)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;
    guint object_id;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    object_id = ++gpdata->object_id_seq;
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_CREATE_BITMAP;
    ui->object_id = object_id;
    ui->width = width;
    ui->height = height;
    ui->data = remmina_plugin_rdpui_image_convert (gpdata, width, height, data);
    remmina_plugin_rdpui_queue_ui (gp, ui);
    return (RD_HBITMAP) object_id;
}

static void
remmina_plugin_rdpui_paint_bitmap (rdpInst *inst, int x, int y, int cx, int cy, int width,
    int height, uint8 * data)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_PAINT_BITMAP;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    ui->width = width;
    ui->height = height;
    ui->data = remmina_plugin_rdpui_image_convert (gpdata, width, height, data);
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_destroy_bitmap (rdpInst *inst, RD_HBITMAP bmp)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_DESTROY_BITMAP;
    ui->object_id = (guint) bmp;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_line (rdpInst *inst, uint8 opcode, int startx, int starty, int endx,
    int endy, RD_PEN * pen)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_LINE;
    ui->opcode = opcode;
    ui->srcx = startx;
    ui->srcy = starty;
    ui->x = endx;
    ui->y = endy;
    ui->fgcolor = remmina_plugin_rdpui_color_convert (gpdata, pen->color);
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_rect (rdpInst *inst, int x, int y, int cx, int cy, int color)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_RECT;
    ui->opcode = 0xcc;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    ui->fgcolor = remmina_plugin_rdpui_color_convert (gpdata, color);
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_polygon (rdpInst *inst, uint8 opcode, uint8 fillmode, RD_POINT * point,
    int npoints, RD_BRUSH * brush, int bgcolor, int fgcolor)
{
}

static void
remmina_plugin_rdpui_polyline (rdpInst *inst, uint8 opcode, RD_POINT * points, int npoints,
    RD_PEN * pen)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    if (npoints < 2) return;
    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_POLYLINE;
    ui->opcode = opcode;
    ui->fgcolor = remmina_plugin_rdpui_color_convert (gpdata, pen->color);
    ui->width = npoints;
    ui->data = g_memdup (points, sizeof (RD_POINT) * npoints);
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_ellipse (rdpInst *inst, uint8 opcode, uint8 fillmode, int x, int y,
    int cx, int cy, RD_BRUSH * brush, int bgcolor, int fgcolor)
{
    g_print ("ellipse:\n");
}

static void
remmina_plugin_rdpui_start_draw_glyphs (rdpInst *inst, int bgcolor, int fgcolor)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_START_DRAW_GLYPHS;
    ui->bgcolor = remmina_plugin_rdpui_color_convert (gpdata, bgcolor);
    ui->fgcolor = remmina_plugin_rdpui_color_convert (gpdata, fgcolor);
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_draw_glyph (rdpInst *inst, int x, int y, int cx, int cy,
    RD_HGLYPH glyph)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_DRAW_GLYPH;
    ui->object_id = (guint) glyph;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_end_draw_glyphs (rdpInst *inst, int x, int y, int cx, int cy)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_END_DRAW_GLYPHS;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static uint32
remmina_plugin_rdpui_get_toggle_keys_state (rdpInst *inst)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    uint32 state = 0;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    if (gpdata->capslock_initstate)
    {
        state |= KBD_SYNC_CAPS_LOCK;
    }
    if (gpdata->numlock_initstate)
    {
        state |= KBD_SYNC_NUM_LOCK;
    }
    
    return state;
}

static void
remmina_plugin_rdpui_bell (rdpInst *inst)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    THREADS_ENTER
    gdk_window_beep (gpdata->drawing_area->window);
    THREADS_LEAVE
}

static void
remmina_plugin_rdpui_destblt (rdpInst *inst, uint8 opcode, int x, int y, int cx, int cy)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_DESTBLT;
    ui->opcode = opcode;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_patblt (rdpInst *inst, uint8 opcode, int x, int y, int cx, int cy,
    RD_BRUSH * brush, int bgcolor, int fgcolor)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;
    gint i;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->opcode = opcode;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    ui->srcx = brush->xorigin;
    ui->srcy = brush->yorigin;
    ui->width = 8;
    ui->height = 8;
    switch (brush->style)
    {
        case 0: /* Solid */
            ui->type = REMMINA_PLUGIN_RDP_UI_RECT;
            ui->fgcolor = remmina_plugin_rdpui_color_convert (gpdata, fgcolor);
            break;
        case 2: /* Hatch */
            ui->type = REMMINA_PLUGIN_RDP_UI_PATBLT_GLYPH;
            ui->fgcolor = remmina_plugin_rdpui_color_convert (gpdata, fgcolor);
            ui->bgcolor = remmina_plugin_rdpui_color_convert (gpdata, bgcolor);
            ui->data = g_memdup (hatch_patterns + brush->pattern[0] * 8, 8);
            break;
        case 3: /* Pattern */
            if (brush->bd == 0) /* rdp4 brush */
            {
                ui->type = REMMINA_PLUGIN_RDP_UI_PATBLT_GLYPH;
                ui->fgcolor = remmina_plugin_rdpui_color_convert (gpdata, bgcolor);
                ui->bgcolor = remmina_plugin_rdpui_color_convert (gpdata, fgcolor);
                ui->data = g_new (guchar, 8);
                for (i = 0; i < 8; i++)
                {
                    ui->data[7 - i] = brush->pattern[i];
                }
            }
            else if (brush->bd->color_code > 1) /* > 1 bpp */
            {
                ui->type = REMMINA_PLUGIN_RDP_UI_PATBLT_BITMAP;
                i = (gpdata->settings->server_depth + 7) / 8;
                ui->data = g_memdup (brush->bd->data, 8 * 8 * i);
            }
            else
            {
                ui->type = REMMINA_PLUGIN_RDP_UI_PATBLT_GLYPH;
                ui->fgcolor = remmina_plugin_rdpui_color_convert (gpdata, bgcolor);
                ui->bgcolor = remmina_plugin_rdpui_color_convert (gpdata, fgcolor);
                ui->data = g_memdup (brush->bd->data, 8);
            }
            break;
        default:
            g_free (ui);
            remmina_plugin_service->log_printf ("[RDP]unsupported brush style %i\n", brush->style);
            return;
    }
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_screenblt (rdpInst *inst, uint8 opcode, int x, int y, int cx, int cy,
    int srcx, int srcy)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_SCREENBLT;
    ui->opcode = opcode;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    ui->srcx = srcx;
    ui->srcy = srcy;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_memblt (rdpInst *inst, uint8 opcode, int x, int y, int cx, int cy,
    RD_HBITMAP src, int srcx, int srcy)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_MEMBLT;
    ui->object_id = (guint) src;
    ui->opcode = opcode;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    ui->srcx = srcx;
    ui->srcy = srcy;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_triblt (rdpInst *inst, uint8 opcode, int x, int y, int cx, int cy,
    RD_HBITMAP src, int srcx, int srcy, RD_BRUSH * brush, int bgcolor, int fgcolor)
{
}

static int
remmina_plugin_rdpui_select (rdpInst *inst, int rdp_socket)
{
    return 1;
}

static void
remmina_plugin_rdpui_set_clip (rdpInst *inst, int x, int y, int cx, int cy)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_SET_CLIP;
    ui->x = x;
    ui->y = y;
    ui->cx = cx;
    ui->cy = cy;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_reset_clip (rdpInst *inst)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_RESET_CLIP;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_resize_window (rdpInst *inst)
{
}

static void
remmina_plugin_rdpui_set_cursor (rdpInst *inst, RD_HCURSOR cursor)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_SET_CURSOR;
    ui->pixbuf = gdk_pixbuf_copy (GDK_PIXBUF (cursor));
    ui->x = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cursor), "x"));
    ui->y = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cursor), "y"));
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_destroy_cursor (rdpInst *inst, RD_HCURSOR cursor)
{
    g_object_unref (GDK_PIXBUF (cursor));
}

static RD_HCURSOR
remmina_plugin_rdpui_create_cursor (rdpInst *inst, uint32 x, uint32 y,
    int width, int height, uint8 * andmask, uint8 * xormask, int bpp)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    GdkPixbuf *pixbuf;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);

    /*g_print ("create_cursor %i %i %i %i %i\n", x, y, width, height, bpp);*/
    pixbuf = remmina_plugin_rdpui_bitmap_convert (gpdata, width, height, bpp, TRUE, xormask);
    remmina_plugin_rdpui_bitmap_apply_mask (pixbuf, andmask);
    if (bpp > 1)
    {
        remmina_plugin_rdpui_bitmap_flip (pixbuf);
    }
    g_object_set_data (G_OBJECT (pixbuf), "x", GINT_TO_POINTER (x));
    g_object_set_data (G_OBJECT (pixbuf), "y", GINT_TO_POINTER (y));
    return (RD_HCURSOR) pixbuf;
}

static void
remmina_plugin_rdpui_set_null_cursor (rdpInst *inst)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_SET_CURSOR;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_set_default_cursor (rdpInst *inst)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_SET_DEFAULT_CURSOR;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static RD_HPALETTE
remmina_plugin_rdpui_create_colormap (rdpInst *inst, RD_PALETTE * colors)
{
    gint *colormap;
    gint index;
    gint red;
    gint green;
    gint blue;
    gint count;

    colormap = g_new0 (gint, 256);
    count = colors->ncolors;
    if (count > 256)
    {
        count = 256;
    }
    for (index = count - 1; index >= 0; index--)
    {
        red = colors->colors[index].red;
        green = colors->colors[index].green;
        blue = colors->colors[index].blue;
        colormap[index] = (red << 16) | (green << 8) | blue;
    }
    return (RD_HPALETTE) colormap;
}

static void
remmina_plugin_rdpui_move_pointer (rdpInst *inst, int x, int y)
{
}

static void
remmina_plugin_rdpui_set_colormap (rdpInst *inst, RD_HPALETTE map)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    if (gpdata->colormap)
    {
        g_free (gpdata->colormap);
    }
    gpdata->colormap = (gint *) map;
}

static RD_HBITMAP
remmina_plugin_rdpui_create_surface (rdpInst *inst, int width, int height, RD_HBITMAP old_surface)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;
    guint object_id;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);
    object_id = ++gpdata->object_id_seq;
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_CREATE_SURFACE;
    ui->object_id = object_id;
    ui->alt_object_id = (guint) old_surface;
    ui->width = width;
    ui->height = height;
    remmina_plugin_rdpui_queue_ui (gp, ui);
    return (RD_HBITMAP) object_id;
}

static void
remmina_plugin_rdpui_set_surface (rdpInst *inst, RD_HBITMAP surface)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_SET_SURFACE;
    ui->object_id = (guint) surface;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_destroy_surface (rdpInst *inst, RD_HBITMAP surface)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpUiObject *ui;

    gp = GET_WIDGET (inst);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_DESTROY_SURFACE;
    ui->object_id = (guint) surface;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

static void
remmina_plugin_rdpui_channel_data (rdpInst *inst, int chan_id, char * data, int data_size,
    int flags, int total_size)
{
    freerdp_chanman_data (inst, chan_id, data, data_size, flags, total_size);
}

static RD_BOOL
remmina_plugin_rdpui_authenticate (rdpInst *inst)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    gchar *s;
    gint ret;

    gp = GET_WIDGET (inst);
    gpdata = GET_DATA (gp);

    THREADS_ENTER
    ret = remmina_plugin_service->protocol_plugin_init_authuserpwd (gp, TRUE);
    THREADS_LEAVE

    if (ret == GTK_RESPONSE_OK)
    {
        s = remmina_plugin_service->protocol_plugin_init_get_username (gp);
        if (s)
        {
            strncpy (gpdata->settings->username, s, sizeof (gpdata->settings->username) - 1);
            g_free (s);
        }
        s = remmina_plugin_service->protocol_plugin_init_get_password (gp);
        if (s)
        {
            strncpy (gpdata->settings->password, s, sizeof (gpdata->settings->password) - 1);
            g_free (s);
        }
        s = remmina_plugin_service->protocol_plugin_init_get_domain (gp);
        if (s)
        {
            strncpy (gpdata->settings->domain, s, sizeof (gpdata->settings->domain) - 1);
            g_free (s);
        }
        return TRUE;
    }
    else
    {
        gpdata->user_cancelled = TRUE;
        return FALSE;
    }
}

static int
remmina_plugin_rdpui_decode(struct rdp_inst * inst, uint8 * data, int data_size)
{
	printf("l_ui_decode: size %d\n", data_size);
	return 0;
}

RD_BOOL
remmina_plugin_rdpui_check_certificate(rdpInst * inst, const char * fingerprint,
	const char * subject, const char * issuer, RD_BOOL verified)
{
	/* TODO: popup dialog, ask for confirmation and store known_hosts file */
	printf("certificate details:\n");
	printf("  Subject:\n    %s\n", subject);
	printf("  Issued by:\n    %s\n", issuer);
	printf("  Fingerprint:\n    %s\n",  fingerprint);

	if (!verified)
		printf("The server could not be authenticated. Connection security may be compromised!\n");

	return True;
}

/* Migrated from xfreerdp */
static gboolean
remmina_plugin_rdpui_get_key_state (KeyCode keycode, int state, XModifierKeymap *modmap)
{
    int modifierpos, key, keysymMask = 0;
    int offset;

    if (keycode == NoSymbol)
    {
        return FALSE;
    }
    for (modifierpos = 0; modifierpos < 8; modifierpos++)
    {
        offset = modmap->max_keypermod * modifierpos;
        for (key = 0; key < modmap->max_keypermod; key++)
        {
            if (modmap->modifiermap[offset + key] == keycode)
            {
                keysymMask |= 1 << modifierpos;
            }
        }
    }
    return (state & keysymMask) ? TRUE : FALSE;
}

void
remmina_plugin_rdpui_init (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    Window wdummy;
    int dummy;
    uint32 state;
    gint keycode;
    XModifierKeymap *modmap;

    gpdata = GET_DATA (gp);

    XQueryPointer(gpdata->display, GDK_ROOT_WINDOW (), &wdummy, &wdummy, &dummy, &dummy,
        &dummy, &dummy, &state);
    modmap = XGetModifierMapping (gpdata->display);

    keycode = XKeysymToKeycode (gpdata->display, XK_Caps_Lock);
    gpdata->capslock_initstate = remmina_plugin_rdpui_get_key_state (keycode, state, modmap);

    keycode = XKeysymToKeycode (gpdata->display, XK_Num_Lock);
    gpdata->numlock_initstate = remmina_plugin_rdpui_get_key_state (keycode, state, modmap);

    XFreeModifiermap(modmap);
}

void
remmina_plugin_rdpui_pre_connect (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    rdpInst *inst;

    gpdata = GET_DATA (gp);
    inst = gpdata->inst;
    inst->ui_error = remmina_plugin_rdpui_error;
    inst->ui_warning = remmina_plugin_rdpui_error;
    inst->ui_unimpl = remmina_plugin_rdpui_error;
    inst->ui_begin_update = remmina_plugin_rdpui_begin_update;
    inst->ui_end_update = remmina_plugin_rdpui_end_update;
    inst->ui_desktop_save = remmina_plugin_rdpui_desktop_save;
    inst->ui_desktop_restore = remmina_plugin_rdpui_desktop_restore;
    inst->ui_create_bitmap = remmina_plugin_rdpui_create_bitmap;
    inst->ui_paint_bitmap = remmina_plugin_rdpui_paint_bitmap;
    inst->ui_destroy_bitmap = remmina_plugin_rdpui_destroy_bitmap;
    inst->ui_line = remmina_plugin_rdpui_line;
    inst->ui_rect = remmina_plugin_rdpui_rect;
    inst->ui_polygon = remmina_plugin_rdpui_polygon;
    inst->ui_polyline = remmina_plugin_rdpui_polyline;
    inst->ui_ellipse = remmina_plugin_rdpui_ellipse;
    inst->ui_start_draw_glyphs = remmina_plugin_rdpui_start_draw_glyphs;
    inst->ui_draw_glyph = remmina_plugin_rdpui_draw_glyph;
    inst->ui_end_draw_glyphs = remmina_plugin_rdpui_end_draw_glyphs;
    inst->ui_get_toggle_keys_state = remmina_plugin_rdpui_get_toggle_keys_state;
    inst->ui_bell = remmina_plugin_rdpui_bell;
    inst->ui_destblt = remmina_plugin_rdpui_destblt;
    inst->ui_patblt = remmina_plugin_rdpui_patblt;
    inst->ui_screenblt = remmina_plugin_rdpui_screenblt;
    inst->ui_memblt = remmina_plugin_rdpui_memblt;
    inst->ui_triblt = remmina_plugin_rdpui_triblt;
    inst->ui_create_glyph = remmina_plugin_rdpui_create_glyph;
    inst->ui_destroy_glyph = remmina_plugin_rdpui_destroy_glyph;
    inst->ui_select = remmina_plugin_rdpui_select;
    inst->ui_set_clip = remmina_plugin_rdpui_set_clip;
    inst->ui_reset_clip = remmina_plugin_rdpui_reset_clip;
    inst->ui_resize_window = remmina_plugin_rdpui_resize_window;
    inst->ui_set_cursor = remmina_plugin_rdpui_set_cursor;
    inst->ui_destroy_cursor = remmina_plugin_rdpui_destroy_cursor;
    inst->ui_create_cursor = remmina_plugin_rdpui_create_cursor;
    inst->ui_set_null_cursor = remmina_plugin_rdpui_set_null_cursor;
    inst->ui_set_default_cursor = remmina_plugin_rdpui_set_default_cursor;
    inst->ui_create_colormap = remmina_plugin_rdpui_create_colormap;
    inst->ui_move_pointer = remmina_plugin_rdpui_move_pointer;
    inst->ui_set_colormap = remmina_plugin_rdpui_set_colormap;
    inst->ui_create_surface = remmina_plugin_rdpui_create_surface;
    inst->ui_set_surface = remmina_plugin_rdpui_set_surface;
    inst->ui_destroy_surface = remmina_plugin_rdpui_destroy_surface;
    inst->ui_channel_data = remmina_plugin_rdpui_channel_data;
    inst->ui_authenticate = remmina_plugin_rdpui_authenticate;
    inst->ui_decode = remmina_plugin_rdpui_decode;
    inst->ui_check_certificate = remmina_plugin_rdpui_check_certificate;
}

void
remmina_plugin_rdpui_post_connect (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gpdata = GET_DATA (gp);
    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_CONNECTED;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

void
remmina_plugin_rdpui_uninit (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    if (gpdata->colormap)
    {
        g_free (gpdata->colormap);
        gpdata->colormap = NULL;
    }
}

void
remmina_plugin_rdpui_get_fds (RemminaProtocolWidget *gp, void ** read_fds, int * read_count)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    if (gpdata->event_pipe[0] != -1)
    {
        read_fds[*read_count] = (void *) gpdata->event_pipe[0];
        (*read_count)++;
    }
}

int
remmina_plugin_rdpui_check_fds (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent *event;
    gchar buf[100];

    gpdata = GET_DATA (gp);
    if (gpdata->event_queue == NULL) return 0;

    while ((event = (RemminaPluginRdpEvent *) g_async_queue_try_pop (gpdata->event_queue)) != NULL)
    {
        gpdata->inst->rdp_send_input(gpdata->inst,
            event->type, event->flag, event->param1, event->param2);
        g_free (event);
    }

    (void) read (gpdata->event_pipe[0], buf, sizeof (buf));
    return 0;
}

void
remmina_plugin_rdpui_object_free (gpointer p)
{
    RemminaPluginRdpUiObject *obj = (RemminaPluginRdpUiObject *) p;
    g_free (obj->data);
    if (obj->pixbuf)
    {
        g_object_unref (obj->pixbuf);
    }
    g_free (obj);
}

