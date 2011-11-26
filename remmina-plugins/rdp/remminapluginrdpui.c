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

/* rdp ui functions, run inside the rdp thread */

#include "remminapluginrdp.h"
#include "remminapluginrdpev.h"
#include "remminapluginrdpui.h"
#include <freerdp/constants.h>
#include <freerdp/utils/memory.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <gdk/gdkx.h>

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
remmina_plugin_rdpui_desktop_resize(rdpContext* context)
{
    RemminaPluginRdpData *gpdata;
    RemminaProtocolWidget *gp;

    gpdata = (RemminaPluginRdpData*) context;
    gp = gpdata->protocol_widget;

    LOCK_BUFFER (TRUE)

    remmina_plugin_service->protocol_plugin_set_width (gp, gpdata->settings->width);
    remmina_plugin_service->protocol_plugin_set_height (gp, gpdata->settings->height);

    UNLOCK_BUFFER (TRUE)

    THREADS_ENTER
    remmina_plugin_rdpev_update_scale (gp);
    THREADS_LEAVE

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "desktop-resize");
}

static void
remmina_plugin_rdpui_palette (rdpContext* context, PALETTE_UPDATE* palette)
{
    g_print ("palette\n");
}

static void
remmina_plugin_rdpui_set_bounds (rdpContext* context, rdpBounds* bounds)
{
    g_print ("set_bounds\n");
}

static void
remmina_plugin_rdpui_dstblt (rdpContext* context, DSTBLT_ORDER* dstblt)
{
    g_print ("dstblt\n");
}

static void
remmina_plugin_rdpui_patblt (rdpContext* context, PATBLT_ORDER* patblt)
{
    g_print ("patblt\n");
}

static void
remmina_plugin_rdpui_scrblt (rdpContext* context, SCRBLT_ORDER* scrblt)
{
    g_print ("srcblt\n");
}

static void
remmina_plugin_rdpui_opaque_rect (rdpContext* context, OPAQUE_RECT_ORDER* opaque_rect)
{
    g_print ("opaque_rect\n");
}

static void
remmina_plugin_rdpui_multi_opaque_rect (rdpContext* context, MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect)
{
    g_print ("multi_opaque_rect\n");
}

static void
remmina_plugin_rdpui_line_to (rdpContext* context, LINE_TO_ORDER* line_to)
{
    g_print ("line_to\n");
}

static void remmina_plugin_rdpui_polyline (rdpContext* context, POLYLINE_ORDER* polyline)
{
    g_print ("polyline\n");
}

static void remmina_plugin_rdpui_memblt (rdpContext* context, MEMBLT_ORDER* memblt)
{
    g_print ("memblt\n");
}

static void remmina_plugin_rdpui_fast_index (rdpContext* context, FAST_INDEX_ORDER* fast_index)
{
    g_print ("fast_index\n");
}

static void remmina_plugin_rdpui_cache_color_table (rdpContext* context, CACHE_COLOR_TABLE_ORDER* cache_color_table_order)
{
    g_print ("cache_color_table\n");
}

static void remmina_plugin_rdpui_cache_glyph (rdpContext* context, CACHE_GLYPH_ORDER* cache_glyph_order)
{
    g_print ("cache_glyph\n");
}

static void remmina_plugin_rdpui_cache_glyph_v2 (rdpContext* context, CACHE_GLYPH_V2_ORDER* cache_glyph_v2_order)
{
    g_print ("cache_glyph_v2\n");
}

static void remmina_plugin_rdpui_surface_bits (rdpContext* context, SURFACE_BITS_COMMAND* surface_bits_command)
{
    uint8* bitmap;
    RFX_MESSAGE* message;
    RemminaPluginRdpUiObject *ui;
    RemminaPluginRdpData *gpdata = (RemminaPluginRdpData*) context;

    if (surface_bits_command->codecID == CODEC_ID_REMOTEFX && gpdata->rfx_context)
    {
        message = rfx_process_message (gpdata->rfx_context, surface_bits_command->bitmapData, surface_bits_command->bitmapDataLength);

        ui = g_new0 (RemminaPluginRdpUiObject, 1);
        ui->type = REMMINA_PLUGIN_RDP_UI_RFX;
        ui->rfx.left = surface_bits_command->destLeft;
        ui->rfx.top = surface_bits_command->destTop;
        ui->rfx.message = message;
        remmina_plugin_rdpui_queue_ui (gpdata->protocol_widget, ui);
    }
    else if (surface_bits_command->codecID == CODEC_ID_NONE)
    {
        bitmap = (uint8*) xzalloc(surface_bits_command->width * surface_bits_command->height * 4);

        freerdp_image_flip(surface_bits_command->bitmapData, bitmap,
                surface_bits_command->width, surface_bits_command->height, 32);

        ui = g_new0 (RemminaPluginRdpUiObject, 1);
        ui->type = REMMINA_PLUGIN_RDP_UI_NOCODEC;
        ui->nocodec.left = surface_bits_command->destLeft;
        ui->nocodec.top = surface_bits_command->destTop;
        ui->nocodec.width = surface_bits_command->width;
        ui->nocodec.height = surface_bits_command->height;
        ui->nocodec.bitmap = bitmap;
        remmina_plugin_rdpui_queue_ui (gpdata->protocol_widget, ui);
    }
    else
    {
        printf("Unsupported codecID %d\n", surface_bits_command->codecID);
    }
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
    rdpSettings *settings;

    gpdata = GET_DATA (gp);
    settings = gpdata->settings;

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

        gpdata->rfx_context = rfx_context_new ();
        rfx_context_set_cpu_opt (gpdata->rfx_context, CPU_SSE2);
    }
}

void
remmina_plugin_rdpui_post_connect (RemminaProtocolWidget *gp)
{
    rdpUpdate* update;
    rdpPrimaryUpdate* primary;
    rdpSecondaryUpdate* secondary;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gpdata = GET_DATA (gp);
    update = gpdata->instance->update;
    primary = update->primary;
    secondary = update->secondary;

    update->DesktopResize = remmina_plugin_rdpui_desktop_resize;
    update->Palette = remmina_plugin_rdpui_palette;
    update->SetBounds = remmina_plugin_rdpui_set_bounds;

    primary->DstBlt = remmina_plugin_rdpui_dstblt;
    primary->PatBlt = remmina_plugin_rdpui_patblt;
    primary->ScrBlt = remmina_plugin_rdpui_scrblt;
    primary->OpaqueRect = remmina_plugin_rdpui_opaque_rect;
    primary->DrawNineGrid = NULL;
    primary->MultiDstBlt = NULL;
    primary->MultiPatBlt = NULL;
    primary->MultiScrBlt = NULL;
    primary->MultiOpaqueRect = remmina_plugin_rdpui_multi_opaque_rect;
    primary->MultiDrawNineGrid = NULL;
    primary->LineTo = remmina_plugin_rdpui_line_to;
    primary->Polyline = remmina_plugin_rdpui_polyline;
    primary->MemBlt = remmina_plugin_rdpui_memblt;
    primary->Mem3Blt = NULL;
    primary->SaveBitmap = NULL;
    primary->GlyphIndex = NULL;
    primary->FastIndex = remmina_plugin_rdpui_fast_index;
    primary->FastGlyph = NULL;
    primary->PolygonSC = NULL;
    primary->PolygonCB = NULL;
    primary->EllipseSC = NULL;
    primary->EllipseCB = NULL;

    secondary->CacheColorTable = remmina_plugin_rdpui_cache_color_table;
    secondary->CacheGlyph = remmina_plugin_rdpui_cache_glyph;
    secondary->CacheGlyphV2 = remmina_plugin_rdpui_cache_glyph_v2;

    update->SurfaceBits = remmina_plugin_rdpui_surface_bits;

    ui = g_new0 (RemminaPluginRdpUiObject, 1);
    ui->type = REMMINA_PLUGIN_RDP_UI_CONNECTED;
    remmina_plugin_rdpui_queue_ui (gp, ui);
}

void
remmina_plugin_rdpui_uninit (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);

    if (gpdata->rfx_context)
    {
        rfx_context_free (gpdata->rfx_context);
        gpdata->rfx_context = NULL;
    }
}

void
remmina_plugin_rdpui_get_fds (RemminaProtocolWidget *gp, void **rfds, int *rcount)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    if (gpdata->event_pipe[0] != -1)
    {
        rfds[*rcount] = GINT_TO_POINTER (gpdata->event_pipe[0]);
        (*rcount)++;
    }
}

boolean
remmina_plugin_rdpui_check_fds (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent *event;
    rdpInput* input;
    uint16 flags;
    gchar buf[100];

    gpdata = GET_DATA (gp);
    if (gpdata->event_queue == NULL) return True;

    input = gpdata->instance->input;

    while ((event = (RemminaPluginRdpEvent *) g_async_queue_try_pop (gpdata->event_queue)) != NULL)
    {
        switch (event->type)
        {
        case REMMINA_PLUGIN_RDP_EVENT_TYPE_SCANCODE:
            flags = event->key_event.extended ? KBD_FLAGS_EXTENDED : 0;
            flags |= event->key_event.up ? KBD_FLAGS_RELEASE : KBD_FLAGS_DOWN;
            input->KeyboardEvent(input, flags, event->key_event.key_code);
            break;
        case REMMINA_PLUGIN_RDP_EVENT_TYPE_MOUSE:
            input->MouseEvent(input,
                event->mouse_event.flags, event->mouse_event.x, event->mouse_event.y);
            break;
        }
        g_free (event);
    }

    if (read (gpdata->event_pipe[0], buf, sizeof (buf)))
    {
    }
    return True;
}

void
remmina_plugin_rdpui_object_free (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *obj)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    switch (obj->type)
    {
    case REMMINA_PLUGIN_RDP_UI_RFX:
        rfx_message_free (gpdata->rfx_context, obj->rfx.message);
        break;
    case REMMINA_PLUGIN_RDP_UI_NOCODEC:
        xfree (obj->nocodec.bitmap);
        break;
    default:
        break;
    }
    g_free (obj);
}

