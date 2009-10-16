/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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
 

#ifndef __REMMINAPLUG_VNC_H__
#define __REMMINAPLUG_VNC_H__

#include "config.h"
#include "remminaplug.h"

#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif

G_BEGIN_DECLS

#define REMMINA_TYPE_PLUG_VNC              (remmina_plug_vnc_get_type ())
#define REMMINA_PLUG_VNC(obj)              (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_PLUG_VNC, RemminaPlugVnc))
#define REMMINA_PLUG_VNC_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_PLUG_VNC, RemminaPlugVncClass))
#define REMMINA_IS_PLUG_VNC(obj)           (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_PLUG_VNC))
#define REMMINA_IS_PLUG_VNC_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_PLUG_VNC))
#define REMMINA_PLUG_VNC_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_PLUG_VNC, RemminaPlugVncClass))

typedef struct _RemminaPlugVnc
{
    RemminaPlug plug;

    /* Whether the user requests to connect/disconnect */
    gboolean connected;
    /* Whether the vnc process is running */
    gboolean running;
    /* Whether the initialzation calls the authentication process */
    gboolean auth_called;
    /* Whether it is the first attempt for authentication. Only first attempt will try to use cached credentials */
    gboolean auth_first;

    GtkWidget *drawing_area;
    guchar *vnc_buffer;
    GdkPixbuf *rgb_buffer;

    GdkPixbuf *scale_buffer;
    gint scale_width;
    gint scale_height;
    guint scale_handler;

    gint queuedraw_x, queuedraw_y, queuedraw_w, queuedraw_h;
    guint queuedraw_handler;

    gulong clipboard_handler;
    GTimeVal clipboard_timer;

    gpointer client;
    gint listen_sock;

    gint button_mask;

    GPtrArray *pressed_keys;

    GtkWidget *chat_window;

    GQueue *vnc_event_queue;
    gint vnc_event_pipe[2];

#ifdef HAVE_PTHREAD
    pthread_t thread;
    pthread_mutex_t buffer_mutex;
#else
    gint thread;
#endif
} RemminaPlugVnc;

typedef struct _RemminaPlugVncClass
{
    RemminaPlugClass parent_class;
} RemminaPlugVncClass;

GType remmina_plug_vnc_get_type (void) G_GNUC_CONST;

GtkWidget* remmina_plug_vnc_new (gboolean scale);

G_END_DECLS

#endif  /* __REMMINAPLUG_VNC_H__  */

