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
remmina_plugin_rdpui_palette (rdpUpdate* update, PALETTE_UPDATE* palette)
{
    g_print ("palette\n");
}

static void
remmina_plugin_rdpui_set_bounds (rdpUpdate* update, BOUNDS* bounds)
{
    g_print ("set_bounds\n");
}

static void
remmina_plugin_rdpui_dstblt (rdpUpdate* update, DSTBLT_ORDER* dstblt)
{
    g_print ("dstblt\n");
}

static void
remmina_plugin_rdpui_patblt (rdpUpdate* update, PATBLT_ORDER* patblt)
{
    g_print ("patblt\n");
}

static void
remmina_plugin_rdpui_scrblt (rdpUpdate* update, SCRBLT_ORDER* scrblt)
{
    g_print ("srcblt\n");
}

static void
remmina_plugin_rdpui_opaque_rect (rdpUpdate* update, OPAQUE_RECT_ORDER* opaque_rect)
{
    g_print ("opaque_rect\n");
}

static void
remmina_plugin_rdpui_multi_opaque_rect (rdpUpdate* update, MULTI_OPAQUE_RECT_ORDER* multi_opaque_rect)
{
    g_print ("multi_opaque_rect\n");
}

static void
remmina_plugin_rdpui_line_to (rdpUpdate* update, LINE_TO_ORDER* line_to)
{
    g_print ("line_to\n");
}

static void remmina_plugin_rdpui_polyline (rdpUpdate* update, POLYLINE_ORDER* polyline)
{
    g_print ("polyline\n");
}

static void remmina_plugin_rdpui_memblt (rdpUpdate* update, MEMBLT_ORDER* memblt)
{
    g_print ("memblt\n");
}

static void remmina_plugin_rdpui_fast_index (rdpUpdate* update, FAST_INDEX_ORDER* fast_index)
{
    g_print ("fast_index\n");
}

static void remmina_plugin_rdpui_cache_color_table (rdpUpdate* update, CACHE_COLOR_TABLE_ORDER* cache_color_table_order)
{
    g_print ("cache_color_table\n");
}

static void remmina_plugin_rdpui_cache_glyph (rdpUpdate* update, CACHE_GLYPH_ORDER* cache_glyph_order)
{
    g_print ("cache_glyph\n");
}

static void remmina_plugin_rdpui_cache_glyph_v2 (rdpUpdate* update, CACHE_GLYPH_V2_ORDER* cache_glyph_v2_order)
{
    g_print ("cache_glyph_v2\n");
}

static void remmina_plugin_rdpui_surface_bits (rdpUpdate* update, SURFACE_BITS_COMMAND* surface_bits_command)
{
    g_print ("surface_bits\n");
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
}

void
remmina_plugin_rdpui_post_connect (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;
    rdpUpdate *update;

    gpdata = GET_DATA (gp);
    update = gpdata->instance->update;

    update->Palette = remmina_plugin_rdpui_palette;
    update->SetBounds = remmina_plugin_rdpui_set_bounds;
    update->DstBlt = remmina_plugin_rdpui_dstblt;
    update->PatBlt = remmina_plugin_rdpui_patblt;
    update->ScrBlt = remmina_plugin_rdpui_scrblt;
    update->OpaqueRect = remmina_plugin_rdpui_opaque_rect;
    update->DrawNineGrid = NULL;
    update->MultiDstBlt = NULL;
    update->MultiPatBlt = NULL;
    update->MultiScrBlt = NULL;
    update->MultiOpaqueRect = remmina_plugin_rdpui_multi_opaque_rect;
    update->MultiDrawNineGrid = NULL;
    update->LineTo = remmina_plugin_rdpui_line_to;
    update->Polyline = remmina_plugin_rdpui_polyline;
    update->MemBlt = remmina_plugin_rdpui_memblt;
    update->Mem3Blt = NULL;
    update->SaveBitmap = NULL;
    update->GlyphIndex = NULL;
    update->FastIndex = remmina_plugin_rdpui_fast_index;
    update->FastGlyph = NULL;
    update->PolygonSC = NULL;
    update->PolygonCB = NULL;
    update->EllipseSC = NULL;
    update->EllipseCB = NULL;

    update->CacheColorTable = remmina_plugin_rdpui_cache_color_table;
    update->CacheGlyph = remmina_plugin_rdpui_cache_glyph;
    update->CacheGlyphV2 = remmina_plugin_rdpui_cache_glyph_v2;

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
remmina_plugin_rdpui_object_free (gpointer p)
{
    RemminaPluginRdpUiObject *obj = (RemminaPluginRdpUiObject *) p;
    g_free (obj);
}

