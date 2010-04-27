/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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

#define LOCK_BUFFER(t)      if(t){CANCEL_DEFER}pthread_mutex_lock(&gpdata->mutex);
#define UNLOCK_BUFFER(t)    pthread_mutex_unlock(&gpdata->mutex);if(t){CANCEL_ASYNC}

#define SET_WIDGET(_inst,_data) (_inst)->param1 = (_data)
#define GET_WIDGET(_inst) ((RemminaProtocolWidget *) (_inst)->param1)
#define GET_DATA(_gp) (RemminaPluginRdpData*) g_object_get_data (G_OBJECT (_gp), "plugin-data")

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

    RD_PLUGIN_DATA rdpdr_data[2];

    GtkWidget *drawing_area;
    GdkPixbuf *drw_buffer;
    GdkPixbuf *rgb_buffer;
    GdkPixbuf *scale_buffer;
    gint scale_width;
    gint scale_height;
    guint scale_handler;
    guchar *colourmap;
    gint clip_x, clip_y, clip_w, clip_h;
    gboolean clip;
    guchar fgcolor[3];
    guchar bgcolor[3];
    guchar pattern[64 * 3];
    gint pattern_w, pattern_h;

    gint queuedraw_x, queuedraw_y, queuedraw_w, queuedraw_h;
    guint queuedraw_handler;

    GArray *pressed_keys;
    GQueue *event_queue;
    gint event_pipe[2];
} RemminaPluginRdpData;

typedef struct _RemminaPluginRdpEvent
{
    gint type;
    gint flag;
    gint param1;
    gint param2;
} RemminaPluginRdpEvent;

#endif

