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

#ifndef __REMMINAPLUGINRDP_H__
#define __REMMINAPLUGINRDP_H__

#include "common/remminaplugincommon.h"
#include <freerdp/freerdp.h>
#include <freerdp/chanman.h>
#include <gdk/gdkx.h>

#define LOCK_BUFFER(t)      if(t){CANCEL_DEFER}pthread_mutex_lock(&gpdata->mutex);
#define UNLOCK_BUFFER(t)    pthread_mutex_unlock(&gpdata->mutex);if(t){CANCEL_ASYNC}

#define SET_WIDGET(_inst,_data) (_inst)->param1 = (_data)
#define GET_WIDGET(_inst) ((RemminaProtocolWidget *) (_inst)->param1)
#define GET_DATA(_gp) (RemminaPluginRdpData*) g_object_get_data (G_OBJECT (_gp), "plugin-data")

#define DEFAULT_QUALITY_0 0x6f
#define DEFAULT_QUALITY_1 0x07
#define DEFAULT_QUALITY_2 0x01
#define DEFAULT_QUALITY_9 0x80

extern RemminaPluginService *remmina_plugin_service;

typedef struct _RemminaPluginRdpData
{
    /* main */
    rdpSet *settings;
    rdpInst *inst;
    rdpChanMan *chan_man;
    pthread_t thread;
    pthread_mutex_t mutex;
    gboolean scale;
    gboolean user_cancelled;

    RD_PLUGIN_DATA rdpdr_data[5];
    RD_PLUGIN_DATA drdynvc_data[5];

    GtkWidget *drawing_area;
    gint scale_width;
    gint scale_height;
    gdouble scale_x;
    gdouble scale_y;
    guint scale_handler;
    gboolean capslock_initstate;
    gboolean numlock_initstate;
    gboolean use_client_keymap;

    Display *display;
    Visual *visual;
    Drawable drw_surface;
    Pixmap rgb_surface;
    GdkPixmap *rgb_pixmap;
    GC gc;
    GC gc_default;
    Pixmap bitmap_mono;
    GC gc_mono;
    gint depth;
    gint bpp;
    gint bitmap_pad;
    gint *colormap;

    guint object_id_seq;
    GHashTable *object_table;

    GAsyncQueue *ui_queue;
    guint ui_handler;

    GArray *pressed_keys;
    GAsyncQueue *event_queue;
    gint event_pipe[2];
} RemminaPluginRdpData;

typedef struct _RemminaPluginRdpEvent
{
    gint type;
    gint flag;
    gint param1;
    gint param2;
} RemminaPluginRdpEvent;

typedef enum
{
    REMMINA_PLUGIN_RDP_UI_CONNECTED = 0,
    REMMINA_PLUGIN_RDP_UI_CREATE_GLYPH,
    REMMINA_PLUGIN_RDP_UI_DESTROY_GLYPH,
    REMMINA_PLUGIN_RDP_UI_CREATE_BITMAP,
    REMMINA_PLUGIN_RDP_UI_PAINT_BITMAP,
    REMMINA_PLUGIN_RDP_UI_DESTROY_BITMAP,
    REMMINA_PLUGIN_RDP_UI_LINE,
    REMMINA_PLUGIN_RDP_UI_RECT,
    REMMINA_PLUGIN_RDP_UI_POLYLINE,
    REMMINA_PLUGIN_RDP_UI_START_DRAW_GLYPHS,
    REMMINA_PLUGIN_RDP_UI_DRAW_GLYPH,
    REMMINA_PLUGIN_RDP_UI_END_DRAW_GLYPHS,
    REMMINA_PLUGIN_RDP_UI_DESTBLT,
    REMMINA_PLUGIN_RDP_UI_PATBLT_GLYPH,
    REMMINA_PLUGIN_RDP_UI_PATBLT_BITMAP,
    REMMINA_PLUGIN_RDP_UI_SCREENBLT,
    REMMINA_PLUGIN_RDP_UI_MEMBLT,
    REMMINA_PLUGIN_RDP_UI_SET_CLIP,
    REMMINA_PLUGIN_RDP_UI_RESET_CLIP,
    REMMINA_PLUGIN_RDP_UI_SET_CURSOR,
    REMMINA_PLUGIN_RDP_UI_SET_DEFAULT_CURSOR,
    REMMINA_PLUGIN_RDP_UI_CREATE_SURFACE,
    REMMINA_PLUGIN_RDP_UI_SET_SURFACE,
    REMMINA_PLUGIN_RDP_UI_DESTROY_SURFACE
} RemminaPluginRdpUiType;

typedef struct _RemminaPluginRdpUiObject
{
    RemminaPluginRdpUiType type;
    guint object_id;
    guint alt_object_id;
    gint x;
    gint y;
    gint cx;
    gint cy;
    gint srcx;
    gint srcy;
    gint width;
    gint height;
    gint bgcolor;
    gint fgcolor;
    gint opcode;
    guchar *data;
    GdkPixbuf *pixbuf;
} RemminaPluginRdpUiObject;

#endif

