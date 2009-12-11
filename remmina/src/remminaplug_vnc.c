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

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include "config.h"
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "remminapublic.h"
#include "remminapref.h"
#include "remminafile.h"
#include "remminainitdialog.h"
#include "remminachatwindow.h"
#include "remminaplug.h"
#include "remminaplug_vnc.h"

G_DEFINE_TYPE (RemminaPlugVnc, remmina_plug_vnc, REMMINA_TYPE_PLUG)

#define dot_cursor_width 5
#define dot_cursor_height 5
#define dot_cursor_x_hot 2
#define dot_cursor_y_hot 2
static const gchar dot_cursor_bits[] = {
    0x00, 0x0e, 0x0e, 0x0e, 0x00 };
static const gchar dot_cursor_mask_bits[] = {
    0x0e, 0x1f, 0x1f, 0x1f, 0x0e };

#ifdef HAVE_PTHREAD
#define LOCK_BUFFER(t)      if(t){CANCEL_DEFER}pthread_mutex_lock(&gp_vnc->buffer_mutex);
#define UNLOCK_BUFFER(t)    pthread_mutex_unlock(&gp_vnc->buffer_mutex);if(t){CANCEL_ASYNC}
#else
#define LOCK_BUFFER(t)
#define UNLOCK_BUFFER(t)
#endif

enum
{
    REMMINA_PLUG_VNC_EVENT_KEY,
    REMMINA_PLUG_VNC_EVENT_POINTER,
    REMMINA_PLUG_VNC_EVENT_CUTTEXT,
    REMMINA_PLUG_VNC_EVENT_CHAT_OPEN,
    REMMINA_PLUG_VNC_EVENT_CHAT_SEND,
    REMMINA_PLUG_VNC_EVENT_CHAT_CLOSE
};
typedef struct _RemminaPlugVncEvent
{
    gint event_type;
    union
    {
        struct
        {
            guint keyval;
            gboolean pressed;
        } key;
        struct
        {
            gint x;
            gint y;
            gint button_mask;
        } pointer;
        struct
        {
            gchar *text;
        } text;
    } event_data;
} RemminaPlugVncEvent;

static void
remmina_plug_vnc_event_push (RemminaPlugVnc *gp_vnc, gint event_type, gpointer p1, gpointer p2, gpointer p3)
{
    RemminaPlugVncEvent *event;

    event = g_new (RemminaPlugVncEvent, 1);
    event->event_type = event_type;
    switch (event_type)
    {
    case REMMINA_PLUG_VNC_EVENT_KEY:
        event->event_data.key.keyval = GPOINTER_TO_UINT (p1);
        event->event_data.key.pressed = GPOINTER_TO_INT (p2);
        break;
    case REMMINA_PLUG_VNC_EVENT_POINTER:
        event->event_data.pointer.x = GPOINTER_TO_INT (p1);
        event->event_data.pointer.y = GPOINTER_TO_INT (p2);
        event->event_data.pointer.button_mask = GPOINTER_TO_INT (p3);
        break;
    case REMMINA_PLUG_VNC_EVENT_CUTTEXT:
    case REMMINA_PLUG_VNC_EVENT_CHAT_SEND:
        event->event_data.text.text = g_strdup ((char*) p1);
        break;
    default:
        break;
    }
    g_queue_push_tail (gp_vnc->vnc_event_queue, event);
    if (write (gp_vnc->vnc_event_pipe[1], "\0", 1))
    {
        /* Ignore */
    }
}

static void
remmina_plug_vnc_event_free (RemminaPlugVncEvent *event)
{
    switch (event->event_type)
    {
    case REMMINA_PLUG_VNC_EVENT_CUTTEXT:
    case REMMINA_PLUG_VNC_EVENT_CHAT_SEND:
        g_free (event->event_data.text.text);
        break;
    default:
        break;
    }
    g_free (event);
}

static void
remmina_plug_vnc_event_free_all (RemminaPlugVnc *gp_vnc)
{
    RemminaPlugVncEvent *event;

    while ((event = g_queue_pop_head (gp_vnc->vnc_event_queue)) != NULL)
    {
        remmina_plug_vnc_event_free (event);
    }
}

static void
remmina_plug_vnc_scale_area (RemminaPlugVnc *gp_vnc, gint *x, gint *y, gint *w, gint *h)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    gint sx, sy, sw, sh;

    if (gp_vnc->rgb_buffer == NULL || gp_vnc->scale_buffer == NULL) return;

    /* We have to extend the scaled region one scaled pixel, to avoid gaps */
    sx = MIN (MAX (0, (*x) * gp_vnc->scale_width / gp->width
        - gp_vnc->scale_width / gp->width - 1), gp_vnc->scale_width - 1);
    sy = MIN (MAX (0, (*y) * gp_vnc->scale_height / gp->height
        - gp_vnc->scale_height / gp->height - 1), gp_vnc->scale_height - 1);
    sw = MIN (gp_vnc->scale_width - sx, (*w) * gp_vnc->scale_width / gp->width
        + gp_vnc->scale_width / gp->width + 1);
    sh = MIN (gp_vnc->scale_height - sy, (*h) * gp_vnc->scale_height / gp->height
        + gp_vnc->scale_height / gp->height + 1);

    gdk_pixbuf_scale (gp_vnc->rgb_buffer, gp_vnc->scale_buffer,
        sx, sy,
        sw, sh,
        0, 0,
        (double) gp_vnc->scale_width / (double) gp->width,
        (double) gp_vnc->scale_height / (double) gp->height,
        remmina_pref.scale_quality);

    *x = sx; *y = sy; *w = sw; *h = sh;
}

static gboolean
remmina_plug_vnc_update_scale_buffer (RemminaPlugVnc *gp_vnc, gboolean in_thread)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    gint width, height;
    gint x, y, w, h;
    GdkPixbuf *pixbuf;

    if (gp_vnc->running)
    {
        width = GTK_WIDGET (gp_vnc)->allocation.width;
        height = GTK_WIDGET (gp_vnc)->allocation.height;
        if (gp->scale)
        {
            if (width > 1 && height > 1)
            {
                LOCK_BUFFER (in_thread)

                if (gp_vnc->scale_buffer)
                {
                    g_object_unref (gp_vnc->scale_buffer);
                }
                gp_vnc->scale_width = (gp->remmina_file->hscale > 0 ?
                    MAX (1, gp->width * gp->remmina_file->hscale / 100) : width);
                gp_vnc->scale_height = (gp->remmina_file->vscale > 0 ?
                    MAX (1, gp->height * gp->remmina_file->vscale / 100) : height);

                pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8,
                    gp_vnc->scale_width, gp_vnc->scale_height);
                gp_vnc->scale_buffer = pixbuf;

                x = 0; y = 0; w = gp->width; h = gp->height;
                remmina_plug_vnc_scale_area (gp_vnc, &x, &y, &w, &h);

                UNLOCK_BUFFER (in_thread)
            }
        }
        else
        {
            LOCK_BUFFER (in_thread)

            if (gp_vnc->scale_buffer)
            {
                g_object_unref (gp_vnc->scale_buffer);
                gp_vnc->scale_buffer = NULL;
            }
            gp_vnc->scale_width = 0;
            gp_vnc->scale_height = 0;

            UNLOCK_BUFFER (in_thread)
        }
        if (width > 1 && height > 1)
        {
            if (in_thread)
            {
                THREADS_ENTER
                gtk_widget_queue_draw_area (GTK_WIDGET (gp_vnc), 0, 0, width, height);
                THREADS_LEAVE
            }
            else
            {
                gtk_widget_queue_draw_area (GTK_WIDGET (gp_vnc), 0, 0, width, height);
            }
        }
    }
    gp_vnc->scale_handler = 0;
    return FALSE;
}

static gboolean
remmina_plug_vnc_update_scale_buffer_main (RemminaPlugVnc *gp_vnc)
{
    return remmina_plug_vnc_update_scale_buffer (gp_vnc, FALSE);
}

static void
remmina_plug_vnc_update_scale (RemminaPlugVnc *gp_vnc, gboolean scale)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    gp->scale = scale;
    if (scale)
    {
        gtk_widget_set_size_request (GTK_WIDGET (gp_vnc->drawing_area),
            (gp->remmina_file->hscale > 0 ? gp->width * gp->remmina_file->hscale / 100 : -1),
            (gp->remmina_file->vscale > 0 ? gp->height * gp->remmina_file->vscale / 100 : -1));
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (gp_vnc->drawing_area), gp->width, gp->height);
    }
}

#ifdef HAVE_LIBVNCCLIENT

typedef struct _RemminaKeyVal
{
    guint keyval;
    guint16 keycode;
} RemminaKeyVal;

/***************************** LibVNCClient related codes *********************************/
#include <rfb/rfbclient.h>

static void
remmina_plug_vnc_process_vnc_event (RemminaPlugVnc *gp_vnc)
{
    rfbClient *cl = (rfbClient*) gp_vnc->client;
    RemminaPlugVncEvent *event;
    gchar buf[100];

    while ((event = g_queue_pop_head (gp_vnc->vnc_event_queue)) != NULL)
    {
        if (cl)
        {
            switch (event->event_type)
            {
            case REMMINA_PLUG_VNC_EVENT_KEY:
                SendKeyEvent (cl, event->event_data.key.keyval, event->event_data.key.pressed);
                break;
            case REMMINA_PLUG_VNC_EVENT_POINTER:
                SendPointerEvent (cl, event->event_data.pointer.x, event->event_data.pointer.y,
                    event->event_data.pointer.button_mask);
                break;
            case REMMINA_PLUG_VNC_EVENT_CUTTEXT:
                SendClientCutText (cl, event->event_data.text.text, strlen (event->event_data.text.text));
                break;
            case REMMINA_PLUG_VNC_EVENT_CHAT_OPEN:
                TextChatOpen (cl);
                break;
            case REMMINA_PLUG_VNC_EVENT_CHAT_SEND:
                TextChatSend (cl, event->event_data.text.text);
                break;
            case REMMINA_PLUG_VNC_EVENT_CHAT_CLOSE:
                TextChatClose (cl);
                TextChatFinish (cl);
                break;
            }
        }
        remmina_plug_vnc_event_free (event);
    }
    if (read (gp_vnc->vnc_event_pipe[0], buf, sizeof (buf)))
    {
        /* Ignore */
    }
}

typedef struct _RemminaPlugVncCuttextParam
{
    RemminaPlugVnc *gp_vnc;
    gchar *text;
    gint textlen;
} RemminaPlugVncCuttextParam;

static void
remmina_plug_vnc_update_quality (rfbClient *cl, gint quality)
{
    switch (quality)
    {
    case 9:
        cl->appData.useBGR233 = 0;
        cl->appData.encodingsString = "copyrect hextile raw";
        cl->appData.compressLevel = 0;
        cl->appData.qualityLevel = 9;
        break;
    case 2:
        cl->appData.useBGR233 = 0;
        cl->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
        cl->appData.compressLevel = 3;
        cl->appData.qualityLevel = 7;
        break;
    case 1:
        cl->appData.useBGR233 = 0;
        cl->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
        cl->appData.compressLevel = 5;
        cl->appData.qualityLevel = 5;
        break;
    case 0:
    default:
        cl->appData.useBGR233 = 1;
        cl->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
        cl->appData.compressLevel = 9;
        cl->appData.qualityLevel = 0;
        break;
    }
}

static void
remmina_plug_vnc_update_colordepth (rfbClient *cl, gint colordepth)
{
    cl->format.depth = colordepth;
    cl->format.bigEndian = 0;
    cl->appData.requestedDepth = colordepth;

    switch (colordepth)
    {
    case 24:
        cl->format.bitsPerPixel = 32;
        cl->format.redMax = 0xff;
        cl->format.greenMax = 0xff;
        cl->format.blueMax = 0xff;
        cl->format.redShift = 16;
        cl->format.greenShift = 8;
        cl->format.blueShift = 0;
        break;
    case 16:
        cl->format.bitsPerPixel = 16;
        cl->format.redMax = 0x1f;
        cl->format.greenMax = 0x3f;
        cl->format.blueMax = 0x1f;
        cl->format.redShift = 11;
        cl->format.greenShift = 5;
        cl->format.blueShift = 0;
        break;
    case 15:
        cl->format.bitsPerPixel = 16;
        cl->format.redMax = 0x1f;
        cl->format.greenMax = 0x1f;
        cl->format.blueMax = 0x1f;
        cl->format.redShift = 10;
        cl->format.greenShift = 5;
        cl->format.blueShift = 0;
        break;
    case 8:
    default:
        cl->format.bitsPerPixel = 8;
        cl->format.redMax = 7;
        cl->format.greenMax = 7;
        cl->format.blueMax = 3;
        cl->format.redShift = 0;
        cl->format.greenShift = 3;
        cl->format.blueShift = 6;
        break;
    }
}

static rfbBool
remmina_plug_vnc_rfb_allocfb (rfbClient *cl)
{
    RemminaPlugVnc *gp_vnc;
    RemminaPlug *gp;
    gint width, height, depth, size;
    GdkPixbuf *new_pixbuf, *old_pixbuf;

    gp_vnc = REMMINA_PLUG_VNC (rfbClientGetClientData (cl, NULL));
    gp = REMMINA_PLUG (gp_vnc);

    width = cl->width;
    height = cl->height;
    depth = cl->format.bitsPerPixel;
    size = width * height * (depth / 8);

    /* Putting gdk_pixbuf_new inside a gdk_thread_enter/leave pair could cause dead-lock! */
    new_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, FALSE, 8, width, height);
    if (new_pixbuf == NULL) return FALSE;
    gdk_pixbuf_fill (new_pixbuf, 0);
    old_pixbuf = gp_vnc->rgb_buffer;

    LOCK_BUFFER (TRUE)

    gp->width = cl->width;
    gp->height = cl->height;

    gp_vnc->rgb_buffer = new_pixbuf;

    if (gp_vnc->vnc_buffer) g_free (gp_vnc->vnc_buffer);
    gp_vnc->vnc_buffer = (guchar*) g_malloc (size);
    cl->frameBuffer = gp_vnc->vnc_buffer;

    UNLOCK_BUFFER (TRUE)

    if (old_pixbuf) g_object_unref (old_pixbuf);
    
    THREADS_ENTER
    remmina_plug_vnc_update_scale (gp_vnc, gp->scale);
    THREADS_LEAVE

    if (gp_vnc->scale_handler == 0) remmina_plug_vnc_update_scale_buffer (gp_vnc, TRUE);

    /* Notify window of change so that scroll border can be hidden or shown if needed */
    remmina_plug_emit_signal (gp, "desktop-resize");

    /* Refresh the client's updateRect - bug in xvncclient */
    cl->updateRect.w = width;
    cl->updateRect.h = height;

    return TRUE;
}

static gint
remmina_plug_vnc_bits (gint n)
{
    gint b = 0;
    while (n) { b++; n >>= 1; }
    return b;
}

static gboolean
remmina_plug_vnc_queue_draw_area_real (RemminaPlugVnc *gp_vnc)
{
    gint x, y, w, h;

    if (GTK_IS_WIDGET (gp_vnc) && gp_vnc->connected)
    {
        LOCK_BUFFER (FALSE)
        x = gp_vnc->queuedraw_x;
        y = gp_vnc->queuedraw_y;
        w = gp_vnc->queuedraw_w;
        h = gp_vnc->queuedraw_h;
        gp_vnc->queuedraw_handler = 0;
        UNLOCK_BUFFER (FALSE)

        gtk_widget_queue_draw_area (GTK_WIDGET (gp_vnc), x, y, w, h);
    }
    return FALSE;
}

static void
remmina_plug_vnc_queue_draw_area (RemminaPlugVnc *gp_vnc, gint x, gint y, gint w, gint h)
{
    gint nx2, ny2, ox2, oy2;

    LOCK_BUFFER (TRUE)
    if (gp_vnc->queuedraw_handler)
    {
        nx2 = x + w;
        ny2 = y + h;
        ox2 = gp_vnc->queuedraw_x + gp_vnc->queuedraw_w;
        oy2 = gp_vnc->queuedraw_y + gp_vnc->queuedraw_h;
        gp_vnc->queuedraw_x = MIN (gp_vnc->queuedraw_x, x);
        gp_vnc->queuedraw_y = MIN (gp_vnc->queuedraw_y, y);
        gp_vnc->queuedraw_w = MAX (ox2, nx2) - gp_vnc->queuedraw_x;
        gp_vnc->queuedraw_h = MAX (oy2, ny2) - gp_vnc->queuedraw_y;
    }
    else
    {
        gp_vnc->queuedraw_x = x;
        gp_vnc->queuedraw_y = y;
        gp_vnc->queuedraw_w = w;
        gp_vnc->queuedraw_h = h;
        gp_vnc->queuedraw_handler = IDLE_ADD ((GSourceFunc) remmina_plug_vnc_queue_draw_area_real, gp_vnc);
    }
    UNLOCK_BUFFER (TRUE)
}

static void
remmina_plug_vnc_rfb_updatefb (rfbClient* cl, int x, int y, int w, int h)
{
    RemminaPlugVnc *gp_vnc;
    RemminaPlug *gp;
    gint ix, iy, x2, y2;
    guchar *destptr, *srcptr;
    gint bytesPerPixel;
    gint rowstride;
    gint i;
    guint32 pixel;
    gint rs, gs, bs, rm, gm, bm, rb, gb, bb;

    gp_vnc = REMMINA_PLUG_VNC (rfbClientGetClientData (cl, NULL));
    gp = REMMINA_PLUG (gp_vnc);

    LOCK_BUFFER (TRUE)

    if (w >= 1 || h >= 1)
    {
        x2 = x + w;
        y2 = y + h;
        bytesPerPixel = cl->format.bitsPerPixel / 8;
        rowstride = gdk_pixbuf_get_rowstride (gp_vnc->rgb_buffer);
        switch (cl->format.bitsPerPixel)
        {
        case 32:
            /* The following codes swap red/green value */
            for (iy = y; iy < y2; iy++)
            {
                destptr = gdk_pixbuf_get_pixels (gp_vnc->rgb_buffer) + iy * rowstride + x * 3;
                srcptr = gp_vnc->vnc_buffer + ((iy * gp->width + x) * bytesPerPixel);
                for (ix = x; ix < x2; ix++)
                {
                    *destptr++ = *(srcptr + 2);
                    *destptr++ = *(srcptr + 1);
                    *destptr++ = *srcptr;
                    srcptr += 4;
                }
            }
            break;
        default:
            rm = cl->format.redMax;
            gm = cl->format.greenMax;
            bm = cl->format.blueMax;
            rb = 8 - remmina_plug_vnc_bits (rm);
            gb = 8 - remmina_plug_vnc_bits (gm);
            bb = 8 - remmina_plug_vnc_bits (bm);
            rs = cl->format.redShift;
            gs = cl->format.greenShift;
            bs = cl->format.blueShift;
            for (iy = y; iy < y2; iy++)
            {
                destptr = gdk_pixbuf_get_pixels (gp_vnc->rgb_buffer) + iy * rowstride + x * 3;
                srcptr = gp_vnc->vnc_buffer + ((iy * gp->width + x) * bytesPerPixel);
                for (ix = x; ix < x2; ix++)
                {
                    pixel = 0;
                    for (i = 0; i < bytesPerPixel; i++) pixel += (*srcptr++) << (8 * i);
                    *destptr++ = ((pixel >> rs) & rm) << rb;
                    *destptr++ = ((pixel >> gs) & gm) << gb;
                    *destptr++ = ((pixel >> bs) & bm) << bb;
                }
            }
            break;
        }
    }

    if (gp->scale)
    {
        remmina_plug_vnc_scale_area (gp_vnc, &x, &y, &w, &h);
    }

    UNLOCK_BUFFER (TRUE)

    remmina_plug_vnc_queue_draw_area (gp_vnc, x, y, w, h);
}

static gboolean
remmina_plug_vnc_queue_cuttext (RemminaPlugVncCuttextParam *param)
{
    RemminaPlugVnc *gp_vnc;
    GTimeVal t;
    glong diff;

    gp_vnc = param->gp_vnc;
    if (GTK_IS_WIDGET (gp_vnc) && gp_vnc->connected)
    {
        g_get_current_time (&t);
        diff = (t.tv_sec - gp_vnc->clipboard_timer.tv_sec) * 10 + (t.tv_usec - gp_vnc->clipboard_timer.tv_usec) / 100000;
        if (diff >= 10)
        {
            gp_vnc->clipboard_timer = t;
            gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD), param->text, param->textlen);
        }
    }
    g_free (param->text);
    g_free (param);
    return FALSE;
}

static void
remmina_plug_vnc_rfb_cuttext (rfbClient* cl, const char *text, int textlen)
{
    RemminaPlugVncCuttextParam *param;

    param = g_new (RemminaPlugVncCuttextParam, 1);
    param->gp_vnc = REMMINA_PLUG_VNC (rfbClientGetClientData (cl, NULL));
    param->text = g_malloc (textlen);
    memcpy (param->text, text, textlen);
    param->textlen = textlen;
    IDLE_ADD ((GSourceFunc) remmina_plug_vnc_queue_cuttext, param);
}

static char*
remmina_plug_vnc_rfb_password (rfbClient *cl)
{
    RemminaPlugVnc *gp_vnc;
    RemminaPlug *gp;
    GtkWidget *dialog;
    gint ret;
    gchar *pwd = NULL;

    gp_vnc = REMMINA_PLUG_VNC (rfbClientGetClientData (cl, NULL));
    gp_vnc->auth_called = TRUE;
    gp = REMMINA_PLUG (gp_vnc);

    dialog = gp->init_dialog;

    if (gp_vnc->auth_first && gp->remmina_file->password && gp->remmina_file->password[0] != '\0')
    {
        pwd = g_strdup (gp->remmina_file->password);
    }
    else
    {
        THREADS_ENTER
        ret = remmina_init_dialog_authpwd (REMMINA_INIT_DIALOG (dialog), _("VNC Password"), 
            (gp->remmina_file->filename != NULL));
        THREADS_LEAVE

        if (ret == GTK_RESPONSE_OK)
        {
            pwd = g_strdup (REMMINA_INIT_DIALOG (dialog)->password);
        }
        else
        {
            gp_vnc->connected = FALSE;
        }
    }
    return pwd;
}

#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
static rfbCredential*
remmina_plug_vnc_rfb_credential (rfbClient *cl, int credentialType)
{
    rfbCredential *cred;
    RemminaPlugVnc *gp_vnc;
    RemminaPlug *gp;
    GtkWidget *dialog;
    gchar *s;
    gint ret;

    gp_vnc = REMMINA_PLUG_VNC (rfbClientGetClientData (cl, NULL));
    gp_vnc->auth_called = TRUE;
    gp = REMMINA_PLUG (gp_vnc);

    dialog = gp->init_dialog;

    cred = g_new0 (rfbCredential, 1);

    switch (credentialType)
    {

    case rfbCredentialTypeUser:

        if (gp_vnc->auth_first && 
            gp->remmina_file->username && gp->remmina_file->username[0] != '\0' &&
            gp->remmina_file->password && gp->remmina_file->password[0] != '\0')
        {
            cred->userCredential.username = g_strdup (gp->remmina_file->username);
            cred->userCredential.password = g_strdup (gp->remmina_file->password);
        }
        else
        {
            THREADS_ENTER
            ret = remmina_init_dialog_authuserpwd (REMMINA_INIT_DIALOG (dialog), gp->remmina_file->username,
                (gp->remmina_file->filename != NULL));
            THREADS_LEAVE

            if (ret == GTK_RESPONSE_OK)
            {
                cred->userCredential.username = g_strdup (REMMINA_INIT_DIALOG (dialog)->username);
                cred->userCredential.password = g_strdup (REMMINA_INIT_DIALOG (dialog)->password);
            }
            else
            {
                g_free (cred);
                cred = NULL;
                gp_vnc->connected = FALSE;
            }
        }
        break;

    case rfbCredentialTypeX509:
        if (gp_vnc->auth_first &&
            gp->remmina_file->cacert && gp->remmina_file->cacert[0] != '\0')
        {
            s = gp->remmina_file->cacert;
            cred->x509Credential.x509CACertFile = (s && s[0] ? g_strdup (s) : NULL);
            s = gp->remmina_file->cacrl;
            cred->x509Credential.x509CACrlFile = (s && s[0] ? g_strdup (s) : NULL);
            s = gp->remmina_file->clientcert;
            cred->x509Credential.x509ClientCertFile = (s && s[0] ? g_strdup (s) : NULL);
            s = gp->remmina_file->clientkey;
            cred->x509Credential.x509ClientKeyFile = (s && s[0] ? g_strdup (s) : NULL);
        }
        else
        {
            THREADS_ENTER
            ret = remmina_init_dialog_authx509 (REMMINA_INIT_DIALOG (dialog), gp->remmina_file->cacert,
                gp->remmina_file->cacrl, gp->remmina_file->clientcert, gp->remmina_file->clientkey);
            THREADS_LEAVE

            if (ret == GTK_RESPONSE_OK)
            {
                s = REMMINA_INIT_DIALOG (dialog)->cacert;
                cred->x509Credential.x509CACertFile = (s && s[0] ? g_strdup (s) : NULL);
                s = REMMINA_INIT_DIALOG (dialog)->cacrl;
                cred->x509Credential.x509CACrlFile = (s && s[0] ? g_strdup (s) : NULL);
                s = REMMINA_INIT_DIALOG (dialog)->clientcert;
                cred->x509Credential.x509ClientCertFile = (s && s[0] ? g_strdup (s) : NULL);
                s = REMMINA_INIT_DIALOG (dialog)->clientkey;
                cred->x509Credential.x509ClientKeyFile = (s && s[0] ? g_strdup (s) : NULL);
            }
            else
            {
                g_free (cred);
                cred = NULL;
                gp_vnc->connected = FALSE;
            }
        }
        break;

    default:
        g_free (cred);
        cred = NULL;
        break;
    }
    return cred;
}
#endif

static void 
remmina_plug_vnc_rfb_cursor_shape (rfbClient *cl, int xhot, int yhot, int width, int height, int bytesPerPixel)
{
    RemminaPlugVnc *gp_vnc;
    guchar *pixbuf_data;
    gint iy;
    guchar *destptr, *srcptr, *srcmaskptr;
    gint i;
    gint rs, gs, bs, rm, gm, bm, rb, gb, bb;
    guint32 pixel;
    GdkDisplay *display;
    GdkPixbuf *pixbuf;
    GdkCursor *cursor;

    gp_vnc = REMMINA_PLUG_VNC (rfbClientGetClientData (cl, NULL));
    if (!GTK_WIDGET (gp_vnc)->window) return;

    if (width && height)
    {
        pixbuf_data = g_malloc (width * height * 4);
        
        switch (cl->format.bitsPerPixel)
        {
        case 32:
            /* The following codes fill in the Alpha channel of the cursor and swap red/blue value */
            destptr = pixbuf_data;
            srcptr = cl->rcSource;
            srcmaskptr = cl->rcMask;
            for (iy = 0; iy < width * height; iy++)
            {
                *destptr++ = *(srcptr + 2);
                *destptr++ = *(srcptr + 1);
                *destptr++ = *srcptr;
                *destptr++ = (*srcmaskptr++) ? 0xff : 0x00;
                srcptr += 4;
            }
            break;
        default:
            rm = cl->format.redMax;
            gm = cl->format.greenMax;
            bm = cl->format.blueMax;
            rb = 8 - remmina_plug_vnc_bits (rm);
            gb = 8 - remmina_plug_vnc_bits (gm);
            bb = 8 - remmina_plug_vnc_bits (bm);
            rs = cl->format.redShift;
            gs = cl->format.greenShift;
            bs = cl->format.blueShift;
            destptr = pixbuf_data;
            srcptr = cl->rcSource;
            srcmaskptr = cl->rcMask;
            for (iy = 0; iy < width * height; iy++)
            {
                pixel = 0;
                for (i = 0; i < bytesPerPixel; i++) pixel += (*srcptr++) << (8 * i);
                *destptr++ = ((pixel >> rs) & rm) << rb;
                *destptr++ = ((pixel >> gs) & gm) << gb;
                *destptr++ = ((pixel >> bs) & bm) << bb;
                *destptr++ = (*srcmaskptr++) ? 0xff : 0x00;
            }
            break;
        }

        pixbuf = gdk_pixbuf_new_from_data (pixbuf_data, GDK_COLORSPACE_RGB,
                                 TRUE, 8, width, height,
                                 width * 4, NULL, NULL);
        THREADS_ENTER

        display = gdk_drawable_get_display (GDK_DRAWABLE (GTK_WIDGET (gp_vnc)->window));

        cursor = gdk_cursor_new_from_pixbuf (display,
                                 pixbuf,
                                 xhot, yhot);

        gdk_window_set_cursor (GTK_WIDGET (gp_vnc)->window, cursor);
        gdk_cursor_unref (cursor);

        THREADS_LEAVE

        gdk_pixbuf_unref (pixbuf);

        g_free (pixbuf_data);
    }
}

static void
remmina_plug_vnc_rfb_bell (rfbClient *cl)
{
    RemminaPlugVnc *gp_vnc;
    GdkWindow *window;
    
    gp_vnc = REMMINA_PLUG_VNC (rfbClientGetClientData (cl, NULL));
    window = GTK_WIDGET (gp_vnc)->window;

    gdk_window_beep (window);
}

/* Translate known VNC messages. It's for intltool only, not for gcc */
#ifdef __DO_NOT_COMPILE_ME__
N_("Unable to connect to VNC server")
N_("Couldn't convert '%s' to host address")
N_("VNC connection failed: %s")
N_("Your connection has been rejected.")
#endif
/* TODO: We only store the last message at this moment. */
static gchar vnc_error[MAX_ERROR_LENGTH + 1];
static void
remmina_plug_vnc_rfb_output(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    gchar *f, *p;

    /* eliminate the last \n */
    f = g_strdup (format);
    if (f[strlen (f) - 1] == '\n') f[strlen (f) - 1] = '\0';

/*g_printf("%s,len=%i\n", f, strlen(f));*/
    if (g_strcmp0 (f, "VNC connection failed: %s") == 0)
    {
        p = va_arg (args, gchar*);
/*g_printf("(param)%s,len=%i\n", p, strlen(p));*/
        g_snprintf (vnc_error, MAX_ERROR_LENGTH, _(f), _(p));
    }
    else
    {
        g_vsnprintf (vnc_error, MAX_ERROR_LENGTH, _(f), args);
    }
/*g_print ("%s\n", vnc_error);*/
    g_free (f);

    va_end(args);
}

static void
remmina_plug_vnc_chat_send (GtkWidget *window, gchar *text, RemminaPlugVnc *gp_vnc)
{
    gchar *ptr;

    /* Need to add a line-feed for UltraVNC */
    ptr = g_strdup_printf ("%s\n", text);
    remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_CHAT_SEND, ptr, NULL, NULL);
    g_free (ptr);
}

static void
remmina_plug_vnc_chat_window_destroy (GtkWidget *widget, RemminaPlugVnc *gp_vnc)
{
    gp_vnc->chat_window = NULL;
    remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_CHAT_CLOSE, NULL, NULL, NULL);
}

static gboolean
remmina_plug_vnc_close_chat (RemminaPlugVnc *gp_vnc)
{
    if (gp_vnc->chat_window)
    {
        gtk_widget_destroy (gp_vnc->chat_window);
    }
    return FALSE;
}

static gboolean
remmina_plug_vnc_open_chat (RemminaPlugVnc *gp_vnc)
{
    rfbClient *cl = (rfbClient*) gp_vnc->client;

    if (gp_vnc->chat_window)
    {
        gtk_window_present (GTK_WINDOW (gp_vnc->chat_window));
    }
    else
    {
        gp_vnc->chat_window = remmina_chat_window_new (
            GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (gp_vnc))),
            cl->desktopName);
        g_signal_connect (G_OBJECT (gp_vnc->chat_window), "destroy",
            G_CALLBACK (remmina_plug_vnc_chat_window_destroy), gp_vnc);
        g_signal_connect (G_OBJECT (gp_vnc->chat_window), "send",
            G_CALLBACK (remmina_plug_vnc_chat_send), gp_vnc);
        gtk_widget_show (gp_vnc->chat_window);
    }
    remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_CHAT_OPEN, NULL, NULL, NULL);
    return FALSE;
}

static void
remmina_plug_vnc_rfb_chat (rfbClient* cl, int value, char *text)
{
    RemminaPlugVnc *gp_vnc;

    gp_vnc = REMMINA_PLUG_VNC (rfbClientGetClientData (cl, NULL));
    switch (value)
    {
    case rfbTextChatOpen:
        IDLE_ADD ((GSourceFunc) remmina_plug_vnc_open_chat, gp_vnc);
        break;
    case rfbTextChatClose:
        /* Do nothing... but wait for the next rfbTextChatFinished signal */
        break;
    case rfbTextChatFinished:
        IDLE_ADD ((GSourceFunc) remmina_plug_vnc_close_chat, gp_vnc);
        break;
    default:
        /* value is the text length */
        THREADS_ENTER
        if (gp_vnc->chat_window)
        {
            remmina_chat_window_receive (REMMINA_CHAT_WINDOW (gp_vnc->chat_window), _("Server"), text);
            gtk_window_present (GTK_WINDOW (gp_vnc->chat_window));
        }
        THREADS_LEAVE
        break;
    }
}

static gboolean
remmina_plug_vnc_incoming_connection (RemminaPlugVnc *gp_vnc, rfbClient *cl)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    fd_set fds;

    gp_vnc->listen_sock = ListenAtTcpPort (cl->listenPort);
    if (gp_vnc->listen_sock < 0) return FALSE;

    remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->init_dialog),
        _("Listening on Port %i for an Incoming VNC Connection..."), cl->listenPort);

    FD_ZERO (&fds); 
    FD_SET (gp_vnc->listen_sock, &fds);
    select (FD_SETSIZE, &fds, NULL, NULL, NULL);

    if (!FD_ISSET (gp_vnc->listen_sock, &fds))
    {
        close (gp_vnc->listen_sock);
        gp_vnc->listen_sock = -1;
        return FALSE;
    }

    cl->sock = AcceptTcpConnection (gp_vnc->listen_sock);
    close (gp_vnc->listen_sock);
    gp_vnc->listen_sock = -1;
    if (cl->sock < 0 || !SetNonBlocking (cl->sock))
    {
        return FALSE;
    }

    return TRUE;
}

static gboolean
remmina_plug_vnc_main_loop (RemminaPlugVnc *gp_vnc)
{
    gint ret;
    rfbClient *cl;
    fd_set fds;
    struct timeval timeout;

    if (!gp_vnc->connected)
    {
        gp_vnc->running = FALSE;
        return FALSE;
    }

    cl = (rfbClient*) gp_vnc->client;

    timeout.tv_sec = 10;
    timeout.tv_usec = 0;
    FD_ZERO (&fds);
    FD_SET (cl->sock, &fds);
    FD_SET (gp_vnc->vnc_event_pipe[0], &fds);
    ret = select (FD_SETSIZE, &fds, NULL, NULL, &timeout);

    /* Sometimes it returns <0 when opening a modal dialog in other window. Absolutely weird */
    /* So we continue looping anyway */
    if (ret <= 0) return TRUE;

    if (FD_ISSET (gp_vnc->vnc_event_pipe[0], &fds))
    {
        remmina_plug_vnc_process_vnc_event (gp_vnc);
    }
    if (FD_ISSET (cl->sock, &fds))
    {
        ret = HandleRFBServerMessage (cl);
        if (!ret)
        {
            gp_vnc->running = FALSE;
            if (gp_vnc->connected && !REMMINA_PLUG (gp_vnc)->closed)
            {
                IDLE_ADD ((GSourceFunc) remmina_plug_close_connection, gp_vnc);
            }
            return FALSE;
        }
    }

    return TRUE;
}

static gboolean
remmina_plug_vnc_main (RemminaPlugVnc *gp_vnc)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    rfbClient *cl = NULL;
    gchar *host, *pos;
    gchar *s;
    gboolean save = FALSE;

    gp_vnc->running = TRUE;

    rfbClientLog = remmina_plug_vnc_rfb_output;
    rfbClientErr = remmina_plug_vnc_rfb_output;

    while (gp_vnc->connected)
    {
        gp_vnc->auth_called = FALSE;

        host = remmina_plug_start_direct_tunnel (gp, 5900);

        if (host == NULL)
        {
            gp_vnc->connected = FALSE;
            break;
        }

        cl = rfbGetClient(8, 3, 4);
        cl->MallocFrameBuffer = remmina_plug_vnc_rfb_allocfb;
        cl->canHandleNewFBSize = TRUE;
        cl->GetPassword = remmina_plug_vnc_rfb_password;
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
        cl->GetCredential = remmina_plug_vnc_rfb_credential;
#endif
        cl->GotFrameBufferUpdate = remmina_plug_vnc_rfb_updatefb;
        cl->GotXCutText = remmina_plug_vnc_rfb_cuttext;
        cl->GotCursorShape = remmina_plug_vnc_rfb_cursor_shape;
        cl->Bell = remmina_plug_vnc_rfb_bell;
        cl->HandleTextChat = remmina_plug_vnc_rfb_chat;
        rfbClientSetClientData (cl, NULL, gp_vnc);

        if (host[0] == '\0')
        {
            cl->serverHost = host;
            cl->listenSpecified = TRUE;
            cl->listenPort = gp->remmina_file->listenport;

            remmina_plug_vnc_incoming_connection (gp_vnc, cl);
        }
        else
        {
            pos = g_strrstr (host, ":");
            *pos++ = '\0';
            cl->serverPort = MAX (0, atoi (pos));
            cl->serverHost = host;

            /* Support short-form (:0, :1) */
            if (cl->serverPort < 100) cl->serverPort += 5900;
        }

        cl->appData.useRemoteCursor = (gp->remmina_file->showcursor ? FALSE : TRUE);

        remmina_plug_vnc_update_quality (cl, gp->remmina_file->quality);
        remmina_plug_vnc_update_colordepth (cl, gp->remmina_file->colordepth);
        SetFormatAndEncodings (cl);

        if (rfbInitClient (cl, NULL, NULL)) break;

        /* If the authentication is not called, it has to be a fatel error and must quit */
        if (!gp_vnc->auth_called)
        {
            gp_vnc->connected = FALSE;
            break;
        }

        /* Otherwise, it's a password error. Try to clear saved password if any */
        if (gp->remmina_file->password) gp->remmina_file->password[0] = '\0';

        if (!gp_vnc->connected) break;

        THREADS_ENTER
        remmina_init_dialog_set_status_temp (REMMINA_INIT_DIALOG (gp->init_dialog),
            _("VNC authentication failed. Trying to reconnect..."));
        THREADS_LEAVE
        /* It's safer to sleep a while before reconnect */
        sleep (2);

        gp_vnc->auth_first = FALSE;
    }

    if (!gp_vnc->connected)
    {
        if (cl && !gp_vnc->auth_called && !(gp->has_error))
        {
            g_snprintf (gp->error_message, MAX_ERROR_LENGTH, "%s", vnc_error);
            gp->has_error = TRUE;
        }
        gp_vnc->running = FALSE;

        IDLE_ADD ((GSourceFunc) remmina_plug_close_connection, gp_vnc);

        return FALSE;
    }

    /* Save user name and certificates if any; save the password if it's requested to do so */
    s = REMMINA_INIT_DIALOG (gp->init_dialog)->username;
    if (s && s[0])
    {
        if (gp->remmina_file->username) g_free (gp->remmina_file->username);
        gp->remmina_file->username = g_strdup (s);
        save = TRUE;
    }
    s = REMMINA_INIT_DIALOG (gp->init_dialog)->cacert;
    if (s && s[0])
    {
        if (gp->remmina_file->cacert) g_free (gp->remmina_file->cacert);
        gp->remmina_file->cacert = g_strdup (s);
        save = TRUE;
    }
    s = REMMINA_INIT_DIALOG (gp->init_dialog)->cacrl;
    if (s && s[0])
    {
        if (gp->remmina_file->cacrl) g_free (gp->remmina_file->cacrl);
        gp->remmina_file->cacrl = g_strdup (s);
        save = TRUE;
    }
    s = REMMINA_INIT_DIALOG (gp->init_dialog)->clientcert;
    if (s && s[0])
    {
        if (gp->remmina_file->clientcert) g_free (gp->remmina_file->clientcert);
        gp->remmina_file->clientcert = g_strdup (s);
        save = TRUE;
    }
    s = REMMINA_INIT_DIALOG (gp->init_dialog)->clientkey;
    if (s && s[0])
    {
        if (gp->remmina_file->clientkey) g_free (gp->remmina_file->clientkey);
        gp->remmina_file->clientkey = g_strdup (s);
        save = TRUE;
    }
    if (REMMINA_INIT_DIALOG (gp->init_dialog)->save_password)
    {
        if (gp->remmina_file->password) g_free (gp->remmina_file->password);
        gp->remmina_file->password =
            g_strdup (REMMINA_INIT_DIALOG (gp->init_dialog)->password);
        save = TRUE;
    }
    if (save)
    {
        remmina_file_save (gp->remmina_file);
    }

    gp_vnc->client = cl;

    remmina_plug_emit_signal (gp, "connect");

    if (gp->remmina_file->disableserverinput)
    {
        PermitServerInput(cl, 1);
    }

    if (gp_vnc->thread)
    {
        while (remmina_plug_vnc_main_loop (gp_vnc)) { }
        gp_vnc->running = FALSE;
    }
    else
    {
        IDLE_ADD ((GSourceFunc) remmina_plug_vnc_main_loop, gp_vnc);
    }

    return FALSE;
}

#ifdef HAVE_PTHREAD
static gpointer
remmina_plug_vnc_main_thread (gpointer data)
{
    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

    CANCEL_ASYNC
    remmina_plug_vnc_main (REMMINA_PLUG_VNC (data));
    return NULL;
}
#endif

static gboolean
remmina_plug_vnc_on_motion (GtkWidget *widget, GdkEventMotion *event, RemminaPlugVnc *gp_vnc)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    gint x, y;

    if (!gp_vnc->connected || !gp_vnc->client) return FALSE;
    if (gp->remmina_file->viewonly) return FALSE;

    if (gp->scale)
    {
        x = event->x * gp->width / gp_vnc->scale_width;
        y = event->y * gp->height / gp_vnc->scale_height;
    }
    else
    {
        x = event->x;
        y = event->y;
    }
    remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_POINTER,
        GINT_TO_POINTER (x), GINT_TO_POINTER (y), GINT_TO_POINTER (gp_vnc->button_mask));
    return TRUE;
}

static gboolean
remmina_plug_vnc_on_button (GtkWidget *widget, GdkEventButton *event, RemminaPlugVnc *gp_vnc)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    gint x, y;
    gint mask;

    if (!gp_vnc->connected || !gp_vnc->client) return FALSE;
    if (gp->remmina_file->viewonly) return FALSE;

    /* We only accept 3 buttons */
    if (event->button < 1 || event->button > 3) return FALSE;
    /* We bypass 2button-press and 3button-press events */
    if (event->type != GDK_BUTTON_PRESS && event->type != GDK_BUTTON_RELEASE) return TRUE;

    mask = (1 << (event->button - 1));
    gp_vnc->button_mask = (event->type == GDK_BUTTON_PRESS ?
        (gp_vnc->button_mask | mask) :
        (gp_vnc->button_mask & (0xff - mask)));
    if (gp->scale)
    {
        x = event->x * gp->width / gp_vnc->scale_width;
        y = event->y * gp->height / gp_vnc->scale_height;
    }
    else
    {
        x = event->x;
        y = event->y;
    }
    remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_POINTER,
        GINT_TO_POINTER (x), GINT_TO_POINTER (y), GINT_TO_POINTER (gp_vnc->button_mask));
    return TRUE;
}

static gboolean
remmina_plug_vnc_on_scroll (GtkWidget *widget, GdkEventScroll *event, RemminaPlugVnc *gp_vnc)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    gint x, y;
    gint mask;

    if (!gp_vnc->connected || !gp_vnc->client) return FALSE;
    if (gp->remmina_file->viewonly) return FALSE;

    switch (event->direction)
    {
    case GDK_SCROLL_UP:
        mask = (1 << 3);
        break;
    case GDK_SCROLL_DOWN:
        mask = (1 << 4);
        break;
    case GDK_SCROLL_LEFT:
        mask = (1 << 5);
        break;
    case GDK_SCROLL_RIGHT:
        mask = (1 << 6);
        break;
    default:
        return FALSE;
    }

    if (gp->scale)
    {
        x = event->x * gp->width / gp_vnc->scale_width;
        y = event->y * gp->height / gp_vnc->scale_height;
    }
    else
    {
        x = event->x;
        y = event->y;
    }
    remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_POINTER,
        GINT_TO_POINTER (x), GINT_TO_POINTER (y), GINT_TO_POINTER (mask | gp_vnc->button_mask));
    remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_POINTER,
        GINT_TO_POINTER (x), GINT_TO_POINTER (y), GINT_TO_POINTER (gp_vnc->button_mask));

    return TRUE;
}

static void
remmina_plug_vnc_release_key (RemminaPlugVnc *gp_vnc, guint16 keycode)
{
    RemminaKeyVal *k;
    gint i;

    if (keycode == 0)
    {
        /* Send all release key events for previously pressed keys */
        for (i = 0; i < gp_vnc->pressed_keys->len; i++)
        {
            k = g_ptr_array_index (gp_vnc->pressed_keys, i);
            remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_KEY, GUINT_TO_POINTER (k->keyval), GINT_TO_POINTER (FALSE), NULL);
            g_free (k);
        }
        g_ptr_array_set_size (gp_vnc->pressed_keys, 0);
    }
    else
    {
        /* Unregister the keycode only */
        for (i = 0; i < gp_vnc->pressed_keys->len; i++)
        {
            k = g_ptr_array_index (gp_vnc->pressed_keys, i);
            if (k->keycode == keycode)
            {
                g_free (k);
                g_ptr_array_remove_index_fast (gp_vnc->pressed_keys, i);
                break;
            }
        }
    }
}

static gboolean
remmina_plug_vnc_on_key (GtkWidget *widget, GdkEventKey *event, RemminaPlugVnc *gp_vnc)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    RemminaKeyVal *k;
    guint keyval;

    if (!gp_vnc->connected || !gp_vnc->client) return FALSE;
    if (gp->remmina_file->viewonly) return FALSE;

    keyval = remmina_pref_keymap_keyval (gp->remmina_file->gkeymap, event->keyval);

    remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_KEY, GUINT_TO_POINTER (keyval),
        GINT_TO_POINTER (event->type == GDK_KEY_PRESS ? TRUE : FALSE), NULL);

    /* Register/unregister the pressed key */
    if (event->type == GDK_KEY_PRESS)
    {
        k = g_new (RemminaKeyVal, 1);
        k->keyval = keyval;
        k->keycode = event->hardware_keycode;
        g_ptr_array_add (gp_vnc->pressed_keys, k);
    }
    else
    {
        remmina_plug_vnc_release_key (gp_vnc, event->hardware_keycode);
    }
    return TRUE;
}

static void
remmina_plug_vnc_on_cuttext_request (GtkClipboard *clipboard, const gchar *text, RemminaPlugVnc *gp_vnc)
{
    GTimeVal t;
    glong diff;

    if (text)
    {
        /* A timer (1 second) to avoid clipboard "loopback": text cut out from VNC won't paste back into VNC */
        g_get_current_time (&t);
        diff = (t.tv_sec - gp_vnc->clipboard_timer.tv_sec) * 10 + (t.tv_usec - gp_vnc->clipboard_timer.tv_usec) / 100000;
        if (diff < 10) return;

        gp_vnc->clipboard_timer = t;
        remmina_plug_vnc_event_push (gp_vnc, REMMINA_PLUG_VNC_EVENT_CUTTEXT, (gpointer) text, NULL, NULL);
    }
}

static void
remmina_plug_vnc_on_cuttext (GtkClipboard *clipboard, GdkEvent *event, RemminaPlugVnc *gp_vnc)
{
    if (!gp_vnc->connected || !gp_vnc->client) return;
    if (REMMINA_PLUG (gp_vnc)->remmina_file->viewonly) return;

    gtk_clipboard_request_text (clipboard, (GtkClipboardTextReceivedFunc) remmina_plug_vnc_on_cuttext_request, gp_vnc);
}

static void
remmina_plug_vnc_on_realize (RemminaPlugVnc *gp_vnc, gpointer data)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    GdkCursor *cursor;
    GdkPixmap *source, *mask;
    GdkColor fg, bg;

    if (gp->remmina_file->showcursor)
    {
        /* Hide local cursor (show a small dot instead) */
        gdk_color_parse ("black", &fg);
        gdk_color_parse ("white", &bg);
        source = gdk_bitmap_create_from_data (NULL, dot_cursor_bits, dot_cursor_width, dot_cursor_height);
        mask = gdk_bitmap_create_from_data (NULL, dot_cursor_mask_bits, dot_cursor_width, dot_cursor_height);
        cursor = gdk_cursor_new_from_pixmap (source, mask, &fg, &bg, dot_cursor_x_hot, dot_cursor_y_hot);
        gdk_pixmap_unref (source);
        gdk_pixmap_unref (mask);
        gdk_window_set_cursor (GTK_WIDGET (gp)->window, cursor);
        gdk_cursor_unref (cursor);
    }
}

/******************************************************************************************/

static gboolean
remmina_plug_vnc_open_connection (RemminaPlug *gp)
{
    RemminaPlugVnc *gp_vnc = REMMINA_PLUG_VNC (gp);

    gp_vnc->connected = TRUE;

    g_signal_connect (G_OBJECT (gp_vnc), "realize",
        G_CALLBACK (remmina_plug_vnc_on_realize), NULL);
    g_signal_connect (G_OBJECT (gp_vnc->drawing_area), "motion-notify-event",
        G_CALLBACK (remmina_plug_vnc_on_motion), gp_vnc);
    g_signal_connect (G_OBJECT (gp_vnc->drawing_area), "button-press-event",
        G_CALLBACK (remmina_plug_vnc_on_button), gp_vnc);
    g_signal_connect (G_OBJECT (gp_vnc->drawing_area), "button-release-event",
        G_CALLBACK (remmina_plug_vnc_on_button), gp_vnc);
    g_signal_connect (G_OBJECT (gp_vnc->drawing_area), "scroll-event",
        G_CALLBACK (remmina_plug_vnc_on_scroll), gp_vnc);
    g_signal_connect (G_OBJECT (gp_vnc->drawing_area), "key-press-event",
        G_CALLBACK (remmina_plug_vnc_on_key), gp_vnc);
    g_signal_connect (G_OBJECT (gp_vnc->drawing_area), "key-release-event",
        G_CALLBACK (remmina_plug_vnc_on_key), gp_vnc);

    gp_vnc->clipboard_handler = g_signal_connect (G_OBJECT (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD)),
        "owner-change", G_CALLBACK (remmina_plug_vnc_on_cuttext), gp_vnc);

#ifdef HAVE_PTHREAD
    if (pthread_create (&gp_vnc->thread, NULL, remmina_plug_vnc_main_thread, gp_vnc))
    {
        /* I don't think this will ever happen... */
        g_print ("Failed to initialize pthread. Falling back to non-thread mode...\n");
        g_timeout_add (0, (GSourceFunc) remmina_plug_vnc_main, gp);
        gp_vnc->thread = 0;
    }
#else
    g_timeout_add (0, (GSourceFunc) remmina_plug_vnc_main, gp);
#endif

    return TRUE;
}

static gboolean
remmina_plug_vnc_close_connection_timeout (RemminaPlugVnc *gp_vnc)
{
    /* wait until the running attribute is set to false by the VNC thread */
    if (gp_vnc->running) return TRUE;

    /* unregister the clipboard monitor */
    if (gp_vnc->clipboard_handler)
    {
        g_signal_handler_disconnect (G_OBJECT (gtk_clipboard_get (GDK_SELECTION_CLIPBOARD)),
            gp_vnc->clipboard_handler);
        gp_vnc->clipboard_handler = 0;
    }

    if (gp_vnc->queuedraw_handler)
    {
        g_source_remove (gp_vnc->queuedraw_handler);
        gp_vnc->queuedraw_handler = 0;
    }
    if (gp_vnc->scale_handler)
    {
        g_source_remove (gp_vnc->scale_handler);
        gp_vnc->scale_handler = 0;
    }
    if (gp_vnc->chat_window)
    {
        gtk_widget_destroy (gp_vnc->chat_window);
        gp_vnc->chat_window = NULL;
    }
    if (gp_vnc->listen_sock >= 0)
    {
        close (gp_vnc->listen_sock);
    }
    if (gp_vnc->client)
    {
        rfbClientCleanup((rfbClient*) gp_vnc->client);
        gp_vnc->client = NULL;
    }
    if (gp_vnc->rgb_buffer)
    {
        g_object_unref (gp_vnc->rgb_buffer);
        gp_vnc->rgb_buffer = NULL;
    }
    if (gp_vnc->vnc_buffer)
    {
        g_free (gp_vnc->vnc_buffer);
        gp_vnc->vnc_buffer = NULL;
    }
    if (gp_vnc->scale_buffer)
    {
        g_object_unref (gp_vnc->scale_buffer);
        gp_vnc->scale_buffer = NULL;
    }
    g_ptr_array_free (gp_vnc->pressed_keys, TRUE);
    remmina_plug_vnc_event_free_all (gp_vnc);
    g_queue_free (gp_vnc->vnc_event_queue);
    close (gp_vnc->vnc_event_pipe[0]);
    close (gp_vnc->vnc_event_pipe[1]);
    
#ifdef HAVE_PTHREAD
    pthread_mutex_destroy (&gp_vnc->buffer_mutex);
#endif

    remmina_plug_emit_signal (REMMINA_PLUG (gp_vnc), "disconnect");

    return FALSE;
}

static gboolean
remmina_plug_vnc_close_connection (RemminaPlug *gp)
{
    RemminaPlugVnc *gp_vnc = REMMINA_PLUG_VNC (gp);

    gp_vnc->connected = FALSE;

#ifdef HAVE_PTHREAD
    if (gp_vnc->thread)
    {
        pthread_cancel (gp_vnc->thread);
        if (gp_vnc->thread) pthread_join (gp_vnc->thread, NULL);
        gp_vnc->running = FALSE;
        remmina_plug_vnc_close_connection_timeout (gp_vnc);
    }
    else
    {
        g_timeout_add (200, (GSourceFunc) remmina_plug_vnc_close_connection_timeout, gp);
    }
#else
    g_timeout_add (200, (GSourceFunc) remmina_plug_vnc_close_connection_timeout, gp);
#endif

    return FALSE;
}

static gpointer
remmina_plug_vnc_query_feature (RemminaPlug *gp, RemminaPlugFeature feature)
{
    switch (feature)
    {
        case REMMINA_PLUG_FEATURE_PREF:
        case REMMINA_PLUG_FEATURE_PREF_QUALITY:
        case REMMINA_PLUG_FEATURE_PREF_VIEWONLY:
        case REMMINA_PLUG_FEATURE_UNFOCUS:
        case REMMINA_PLUG_FEATURE_SCALE:
            return GINT_TO_POINTER (1);
        case REMMINA_PLUG_FEATURE_PREF_DISABLESERVERINPUT:
            return (SupportsClient2Server ((rfbClient*) (REMMINA_PLUG_VNC (gp)->client), rfbSetServerInput) ?
                GINT_TO_POINTER (1) : NULL);
        case REMMINA_PLUG_FEATURE_TOOL_CHAT:
            return (SupportsClient2Server ((rfbClient*) (REMMINA_PLUG_VNC (gp)->client), rfbTextChat) ?
                GINT_TO_POINTER (1) : NULL);
        default:
            return NULL;
    }
}

static void
remmina_plug_vnc_call_feature (RemminaPlug *gp, RemminaPlugFeature feature, const gpointer data)
{
    RemminaPlugVnc *gp_vnc = REMMINA_PLUG_VNC (gp);
    switch (feature)
    {
        case REMMINA_PLUG_FEATURE_PREF_QUALITY:
            remmina_plug_vnc_update_quality ((rfbClient*) (gp_vnc->client), GPOINTER_TO_INT (data));
            SetFormatAndEncodings ((rfbClient*) (gp_vnc->client));
            break;
        case REMMINA_PLUG_FEATURE_PREF_VIEWONLY:
            gp->remmina_file->viewonly = (data != NULL);
            break;
        case REMMINA_PLUG_FEATURE_PREF_DISABLESERVERINPUT:
            PermitServerInput ((rfbClient*) (gp_vnc->client), (data ? 1 : 0));
            gp->remmina_file->disableserverinput = (data ? TRUE : FALSE);
            break;
        case REMMINA_PLUG_FEATURE_UNFOCUS:
            remmina_plug_vnc_release_key (gp_vnc, 0);
            break;
        case REMMINA_PLUG_FEATURE_SCALE:
            remmina_plug_vnc_update_scale (gp_vnc, (data != NULL));
            break;
        case REMMINA_PLUG_FEATURE_TOOL_CHAT:
            remmina_plug_vnc_open_chat (gp_vnc);
            break;
        default:
            break;
    }
}

#else /* HAVE_LIBVNCCLIENT */

static gboolean
remmina_plug_vnc_open_connection (RemminaPlug *gp)
{
    GtkWidget *dialog;
    /* This should never happen because if no VNC support users are not able to select VNC in preference dialog */
    dialog = gtk_message_dialog_new (NULL,
        GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
        "VNC not supported");
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    return FALSE;
}

static gboolean
remmina_plug_vnc_close_connection (RemminaPlug *gp)
{
    return FALSE;
}

static gpointer
remmina_plug_vnc_query_feature (RemminaPlug *gp, RemminaPlugFeature feature)
{
    return NULL;
}

static void
remmina_plug_vnc_call_feature (RemminaPlug *gp, RemminaPlugFeature feature, const gpointer data)
{
}

#endif /* HAVE_LIBVNCCLIENT */

static gboolean
remmina_plug_vnc_on_expose (GtkWidget *widget, GdkEventExpose *event, RemminaPlugVnc *gp_vnc)
{
    RemminaPlug *gp = REMMINA_PLUG (gp_vnc);
    GdkPixbuf *buffer;
    gint width, height, x, y, rowstride;

    LOCK_BUFFER (FALSE)

    /* widget == gp_vnc->drawing_area */
    buffer = (gp->scale ? gp_vnc->scale_buffer : gp_vnc->rgb_buffer);
    if (!buffer)
    {
        UNLOCK_BUFFER (FALSE)
        return FALSE;
    }

    width = (gp->scale ? gp_vnc->scale_width : gp->width);
    height = (gp->scale ? gp_vnc->scale_height : gp->height);
    if (event->area.x >= width || event->area.y >= height)
    {
        UNLOCK_BUFFER (FALSE)
        return FALSE;
    }
    x = event->area.x;
    y = event->area.y;
    rowstride = gdk_pixbuf_get_rowstride (buffer);

    /* this is a little tricky. It "moves" the rgb_buffer pointer to (x,y) as top-left corner,
       and keeps the same rowstride. This is an effective way to "clip" the rgb_buffer for gdk. */
    gdk_draw_rgb_image (widget->window, widget->style->white_gc,
        x, y,
        MIN (width - x, event->area.width), MIN (height - y, event->area.height),
        GDK_RGB_DITHER_MAX,
        gdk_pixbuf_get_pixels (buffer) + y * rowstride + x * 3,
        rowstride);

    UNLOCK_BUFFER (FALSE)
    return TRUE;
}

static gboolean
remmina_plug_vnc_on_configure (GtkWidget *widget, GdkEventConfigure *event, RemminaPlugVnc *gp_vnc)
{
    /* We do a delayed reallocating to improve performance */
    if (gp_vnc->scale_handler) g_source_remove (gp_vnc->scale_handler);
    gp_vnc->scale_handler = g_timeout_add (1000, (GSourceFunc) remmina_plug_vnc_update_scale_buffer_main, gp_vnc);
    return FALSE;
}

static void
remmina_plug_vnc_class_init (RemminaPlugVncClass *klass)
{
    klass->parent_class.open_connection = remmina_plug_vnc_open_connection;
    klass->parent_class.close_connection = remmina_plug_vnc_close_connection;
    klass->parent_class.query_feature = remmina_plug_vnc_query_feature;
    klass->parent_class.call_feature = remmina_plug_vnc_call_feature;
}

static void
remmina_plug_vnc_destroy (GtkWidget *widget, gpointer data)
{
}

static void
remmina_plug_vnc_init (RemminaPlugVnc *gp_vnc)
{
    gint flags;

    gp_vnc->drawing_area = gtk_drawing_area_new ();
    gtk_widget_show (gp_vnc->drawing_area);
    gtk_container_add (GTK_CONTAINER (gp_vnc), gp_vnc->drawing_area);

    gtk_widget_add_events (gp_vnc->drawing_area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK
        | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    GTK_WIDGET_SET_FLAGS (gp_vnc->drawing_area, GTK_CAN_FOCUS);

    g_signal_connect (G_OBJECT (gp_vnc->drawing_area), "expose_event",
        G_CALLBACK (remmina_plug_vnc_on_expose), gp_vnc);
    g_signal_connect (G_OBJECT (gp_vnc->drawing_area), "configure_event",
        G_CALLBACK (remmina_plug_vnc_on_configure), gp_vnc);
    g_signal_connect (G_OBJECT (gp_vnc), "destroy", G_CALLBACK (remmina_plug_vnc_destroy), NULL);

    gp_vnc->connected = FALSE;
    gp_vnc->running = FALSE;
    gp_vnc->auth_called = FALSE;
    gp_vnc->auth_first = TRUE;
    gp_vnc->rgb_buffer = NULL;
    gp_vnc->vnc_buffer = NULL;
    gp_vnc->scale_buffer = NULL;
    gp_vnc->scale_width = 0;
    gp_vnc->scale_height = 0;
    gp_vnc->scale_handler = 0;
    gp_vnc->queuedraw_x = 0;
    gp_vnc->queuedraw_y = 0;
    gp_vnc->queuedraw_w = 0;
    gp_vnc->queuedraw_h = 0;
    gp_vnc->queuedraw_handler = 0;
    gp_vnc->clipboard_handler = 0;
    g_get_current_time (&gp_vnc->clipboard_timer);
    gp_vnc->client = NULL;
    gp_vnc->listen_sock = -1;
    gp_vnc->button_mask = 0;
    gp_vnc->pressed_keys = g_ptr_array_new ();
    gp_vnc->chat_window = NULL;
    gp_vnc->thread = 0;
    gp_vnc->vnc_event_queue = g_queue_new ();
    if (pipe (gp_vnc->vnc_event_pipe))
    {
        g_print ("Error creating pipes.\n");
        gp_vnc->vnc_event_pipe[0] = 0;
        gp_vnc->vnc_event_pipe[1] = 0;
    }
    flags = fcntl (gp_vnc->vnc_event_pipe[0], F_GETFL, 0);
    fcntl (gp_vnc->vnc_event_pipe[0], F_SETFL, flags | O_NONBLOCK);
#ifdef HAVE_PTHREAD
    pthread_mutex_init (&gp_vnc->buffer_mutex, NULL);
#endif
}

GtkWidget*
remmina_plug_vnc_new (gboolean scale)
{
    RemminaPlugVnc *gp_vnc;

    gp_vnc = REMMINA_PLUG_VNC (g_object_new (REMMINA_TYPE_PLUG_VNC, NULL));
    REMMINA_PLUG (gp_vnc)->scale = scale;

    return GTK_WIDGET (gp_vnc);
}

