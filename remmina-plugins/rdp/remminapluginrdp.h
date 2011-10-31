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
#include <freerdp/channels/channels.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/rfx.h>
#include <gdk/gdkx.h>

#define LOCK_BUFFER(t)      if(t){CANCEL_DEFER}pthread_mutex_lock(&gpdata->mutex);
#define UNLOCK_BUFFER(t)    pthread_mutex_unlock(&gpdata->mutex);if(t){CANCEL_ASYNC}

#define GET_DATA(_gp) (RemminaPluginRdpData*) g_object_get_data (G_OBJECT (_gp), "plugin-data")

#define DEFAULT_QUALITY_0 0x6f
#define DEFAULT_QUALITY_1 0x07
#define DEFAULT_QUALITY_2 0x01
#define DEFAULT_QUALITY_9 0x80

extern RemminaPluginService *remmina_plugin_service;

typedef struct _RemminaPluginRdpData
{
    rdpContext _p;

    RemminaProtocolWidget *protocol_widget;

    /* main */
    rdpSettings *settings;
    freerdp* instance;
    rdpChannels *channels;

    pthread_t thread;
    pthread_mutex_t mutex;
    gboolean scale;
    gboolean user_cancelled;

    RDP_PLUGIN_DATA rdpdr_data[5];
    RDP_PLUGIN_DATA drdynvc_data[5];
    RDP_PLUGIN_DATA rdpsnd_data[5];
    gchar rdpsnd_options[20];

    RFX_CONTEXT *rfx_context;

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
    cairo_surface_t *rgb_cairo_surface;
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

typedef enum
{
    REMMINA_PLUGIN_RDP_EVENT_TYPE_SCANCODE,
    REMMINA_PLUGIN_RDP_EVENT_TYPE_MOUSE
} RemminaPluginRdpEventType;

typedef struct _RemminaPluginRdpEvent
{
    RemminaPluginRdpEventType type;
    union
    {
        struct
        {
            boolean up;
            boolean extended;
            uint8 key_code;
        } key_event;
        struct
        {
            uint16 flags;
            uint16 x;
            uint16 y;
        } mouse_event;
    };
} RemminaPluginRdpEvent;

typedef enum
{
    REMMINA_PLUGIN_RDP_UI_CONNECTED = 0,
    REMMINA_PLUGIN_RDP_UI_RFX,
    REMMINA_PLUGIN_RDP_UI_NOCODEC
} RemminaPluginRdpUiType;

typedef struct _RemminaPluginRdpUiObject
{
    RemminaPluginRdpUiType type;
    union
    {
        struct
        {
            gint left;
            gint top;
            RFX_MESSAGE* message;
        } rfx;
        struct
        {
            uint8* bitmap;
        } nocodec;
    };
} RemminaPluginRdpUiObject;

#endif

