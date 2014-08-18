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

#include "common/remmina_plugin.h"

#define REMMINA_PLUGIN_VNC_FEATURE_PREF_QUALITY            1
#define REMMINA_PLUGIN_VNC_FEATURE_PREF_VIEWONLY           2
#define REMMINA_PLUGIN_VNC_FEATURE_PREF_DISABLESERVERINPUT 3
#define REMMINA_PLUGIN_VNC_FEATURE_TOOL_REFRESH            4
#define REMMINA_PLUGIN_VNC_FEATURE_TOOL_CHAT               5
#define REMMINA_PLUGIN_VNC_FEATURE_SCALE                   6
#define REMMINA_PLUGIN_VNC_FEATURE_UNFOCUS                 7

typedef struct _RemminaPluginVncData
{
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

	GdkPixbuf *queuecursor_pixbuf;
	gint queuecursor_x, queuecursor_y;
	guint queuecursor_handler;

	gpointer client;
	gint listen_sock;

	gint button_mask;

	GPtrArray *pressed_keys;

	GQueue *vnc_event_queue;
	gint vnc_event_pipe[2];

#ifdef HAVE_PTHREAD
	pthread_t thread;
	pthread_mutex_t buffer_mutex;
#else
	gint thread;
#endif
} RemminaPluginVncData;

static RemminaPluginService *remmina_plugin_service = NULL;

static int dot_cursor_x_hot = 2;
static int dot_cursor_y_hot = 2;
static const gchar * dot_cursor_xpm[] =
{ "5 5 3 1", " 	c None", ".	c #000000", "+	c #FFFFFF", " ... ", ".+++.", ".+ +.", ".+++.", " ... " };

#ifdef HAVE_PTHREAD
#define LOCK_BUFFER(t)      if(t){CANCEL_DEFER}pthread_mutex_lock(&gpdata->buffer_mutex);
#define UNLOCK_BUFFER(t)    pthread_mutex_unlock(&gpdata->buffer_mutex);if(t){CANCEL_ASYNC}
#else
#define LOCK_BUFFER(t)
#define UNLOCK_BUFFER(t)
#endif

enum
{
	REMMINA_PLUGIN_VNC_EVENT_KEY,
	REMMINA_PLUGIN_VNC_EVENT_POINTER,
	REMMINA_PLUGIN_VNC_EVENT_CUTTEXT,
	REMMINA_PLUGIN_VNC_EVENT_CHAT_OPEN,
	REMMINA_PLUGIN_VNC_EVENT_CHAT_SEND,
	REMMINA_PLUGIN_VNC_EVENT_CHAT_CLOSE
};

typedef struct _RemminaPluginVncEvent
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
} RemminaPluginVncEvent;

static void remmina_plugin_vnc_event_push(RemminaProtocolWidget *gp, gint event_type, gpointer p1, gpointer p2, gpointer p3)
{
	RemminaPluginVncData *gpdata;
	RemminaPluginVncEvent *event;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	event = g_new(RemminaPluginVncEvent, 1);
	event->event_type = event_type;
	switch (event_type)
	{
		case REMMINA_PLUGIN_VNC_EVENT_KEY:
			event->event_data.key.keyval = GPOINTER_TO_UINT(p1);
			event->event_data.key.pressed = GPOINTER_TO_INT(p2);
			break;
		case REMMINA_PLUGIN_VNC_EVENT_POINTER:
			event->event_data.pointer.x = GPOINTER_TO_INT(p1);
			event->event_data.pointer.y = GPOINTER_TO_INT(p2);
			event->event_data.pointer.button_mask = GPOINTER_TO_INT(p3);
			break;
		case REMMINA_PLUGIN_VNC_EVENT_CUTTEXT:
		case REMMINA_PLUGIN_VNC_EVENT_CHAT_SEND:
			event->event_data.text.text = g_strdup((char*) p1);
			break;
		default:
			break;
	}
	g_queue_push_tail(gpdata->vnc_event_queue, event);
	if (write(gpdata->vnc_event_pipe[1], "\0", 1))
	{
		/* Ignore */
	}
}

static void remmina_plugin_vnc_event_free(RemminaPluginVncEvent *event)
{
	switch (event->event_type)
	{
		case REMMINA_PLUGIN_VNC_EVENT_CUTTEXT:
		case REMMINA_PLUGIN_VNC_EVENT_CHAT_SEND:
			g_free(event->event_data.text.text);
			break;
		default:
			break;
	}
	g_free(event);
}

static void remmina_plugin_vnc_event_free_all(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	RemminaPluginVncEvent *event;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	while ((event = g_queue_pop_head(gpdata->vnc_event_queue)) != NULL)
	{
		remmina_plugin_vnc_event_free(event);
	}
}

static void remmina_plugin_vnc_scale_area(RemminaProtocolWidget *gp, gint *x, gint *y, gint *w, gint *h)
{
	RemminaPluginVncData *gpdata;
	gint sx, sy, sw, sh;
	gint width, height;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (gpdata->rgb_buffer == NULL || gpdata->scale_buffer == NULL)
		return;

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);

	if (gpdata->scale_width == width && gpdata->scale_height == height)
	{
		/* Same size, just copy the pixels */
		gdk_pixbuf_copy_area(gpdata->rgb_buffer, *x, *y, *w, *h, gpdata->scale_buffer, *x, *y);
		return;
	}

	/* We have to extend the scaled region 2 scaled pixels, to avoid gaps */
	sx = MIN(MAX(0, (*x) * gpdata->scale_width / width - gpdata->scale_width / width - 2), gpdata->scale_width - 1);
	sy = MIN(MAX(0, (*y) * gpdata->scale_height / height - gpdata->scale_height / height - 2), gpdata->scale_height - 1);
	sw = MIN(gpdata->scale_width - sx, (*w) * gpdata->scale_width / width + gpdata->scale_width / width + 4);
	sh = MIN(gpdata->scale_height - sy, (*h) * gpdata->scale_height / height + gpdata->scale_height / height + 4);

	gdk_pixbuf_scale(gpdata->rgb_buffer, gpdata->scale_buffer, sx, sy, sw, sh, 0, 0,
			(double) gpdata->scale_width / (double) width, (double) gpdata->scale_height / (double) height,
			remmina_plugin_service->pref_get_scale_quality());

	*x = sx;
	*y = sy;
	*w = sw;
	*h = sh;
}

static gboolean remmina_plugin_vnc_update_scale_buffer(RemminaProtocolWidget *gp, gboolean in_thread)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	gint width, height;
	gint gpwidth, gpheight;
	gint hscale, vscale;
	gboolean scale;
	gint x, y, w, h;
	GdkPixbuf *pixbuf;
	GtkAllocation a;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (gpdata->running)
	{
		gtk_widget_get_allocation(GTK_WIDGET(gp), &a);
		width = a.width;
		height = a.height;
		scale = remmina_plugin_service->protocol_plugin_get_scale(gp);
		if (scale)
		{
			if (width > 1 && height > 1)
			{
				LOCK_BUFFER (in_thread)

				if (gpdata->scale_buffer)
				{
					g_object_unref(gpdata->scale_buffer);
				}
				gpwidth = remmina_plugin_service->protocol_plugin_get_width(gp);
				gpheight = remmina_plugin_service->protocol_plugin_get_height(gp);
				hscale = remmina_plugin_service->file_get_int(remminafile, "hscale", 0);
				vscale = remmina_plugin_service->file_get_int(remminafile, "vscale", 0);
				gpdata->scale_width = (hscale > 0 ? MAX(1, gpwidth * hscale / 100) : width);
				gpdata->scale_height = (vscale > 0 ? MAX(1, gpheight * vscale / 100) : height);

				pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, gpdata->scale_width,
						gpdata->scale_height);
				gpdata->scale_buffer = pixbuf;

				x = 0;
				y = 0;
				w = gpwidth;
				h = gpheight;
				remmina_plugin_vnc_scale_area(gp, &x, &y, &w, &h);

UNLOCK_BUFFER			(in_thread)
		}
	}
	else
	{
		LOCK_BUFFER (in_thread)

		if (gpdata->scale_buffer)
		{
			g_object_unref (gpdata->scale_buffer);
			gpdata->scale_buffer = NULL;
		}
		gpdata->scale_width = 0;
		gpdata->scale_height = 0;

		UNLOCK_BUFFER (in_thread)
	}
		if (width > 1 && height > 1)
		{
			if (in_thread)
			{
				THREADS_ENTER
				gtk_widget_queue_draw_area(GTK_WIDGET(gp), 0, 0, width, height);
				THREADS_LEAVE
			}
			else
			{
				gtk_widget_queue_draw_area(GTK_WIDGET(gp), 0, 0, width, height);
			}
		}
	}
	gpdata->scale_handler = 0;
	return FALSE;
}

static gboolean remmina_plugin_vnc_update_scale_buffer_main(RemminaProtocolWidget *gp)
{
	return remmina_plugin_vnc_update_scale_buffer(gp, FALSE);
}

static void remmina_plugin_vnc_update_scale(RemminaProtocolWidget *gp, gboolean scale)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	gint width, height;
	gint hscale, vscale;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);
	if (scale)
	{
		hscale = remmina_plugin_service->file_get_int(remminafile, "hscale", 0);
		vscale = remmina_plugin_service->file_get_int(remminafile, "vscale", 0);
		gtk_widget_set_size_request(GTK_WIDGET(gpdata->drawing_area), (hscale > 0 ? width * hscale / 100 : -1),
		(vscale > 0 ? height * vscale / 100 : -1));
	}
	else
	{
		gtk_widget_set_size_request (GTK_WIDGET (gpdata->drawing_area), width, height);
	}

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "update-align");
}

gboolean remmina_plugin_vnc_setcursor(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	GdkCursor *cur;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	LOCK_BUFFER (FALSE)
	gpdata->queuecursor_handler = 0;

	if (gpdata->queuecursor_pixbuf)
	{
		cur = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), gpdata->queuecursor_pixbuf, gpdata->queuecursor_x,
				gpdata->queuecursor_y);
		gdk_window_set_cursor(gtk_widget_get_window(gpdata->drawing_area), cur);
#if GTK_VERSION == 3
		g_object_unref(cur);
#else
		gdk_cursor_unref(cur);
#endif
		g_object_unref(gpdata->queuecursor_pixbuf);
		gpdata->queuecursor_pixbuf = NULL;
	}
	else
	{
		gdk_window_set_cursor(gtk_widget_get_window(gpdata->drawing_area), NULL);
	}
	UNLOCK_BUFFER (FALSE)

	return FALSE;
}

static void remmina_plugin_vnc_queuecursor(RemminaProtocolWidget *gp, GdkPixbuf *pixbuf, gint x, gint y)
{
	RemminaPluginVncData *gpdata;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	if (gpdata->queuecursor_pixbuf)
	{
		g_object_unref(gpdata->queuecursor_pixbuf);
	}
	gpdata->queuecursor_pixbuf = pixbuf;
	gpdata->queuecursor_x = x;
	gpdata->queuecursor_y = y;
	if (!gpdata->queuecursor_handler)
	{
		gpdata->queuecursor_handler = IDLE_ADD((GSourceFunc) remmina_plugin_vnc_setcursor, gp);
	}
}

typedef struct _RemminaKeyVal
{
	guint keyval;
	guint16 keycode;
} RemminaKeyVal;

/***************************** LibVNCClient related codes *********************************/
#include <rfb/rfbclient.h>

#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
static const uint32_t remmina_plugin_vnc_no_encrypt_auth_types[] =
{	rfbNoAuth, rfbVncAuth, rfbMSLogon, 0};
#endif

static void remmina_plugin_vnc_process_vnc_event(RemminaProtocolWidget *gp)
{
	RemminaPluginVncEvent *event;
	RemminaPluginVncData *gpdata;
	rfbClient *cl;
	gchar buf[100];

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	cl = (rfbClient*) gpdata->client;
	while ((event = g_queue_pop_head(gpdata->vnc_event_queue)) != NULL)
	{
		if (cl)
		{
			switch (event->event_type)
			{
				case REMMINA_PLUGIN_VNC_EVENT_KEY:
					SendKeyEvent(cl, event->event_data.key.keyval, event->event_data.key.pressed);
					break;
				case REMMINA_PLUGIN_VNC_EVENT_POINTER:
					SendPointerEvent(cl, event->event_data.pointer.x, event->event_data.pointer.y,
							event->event_data.pointer.button_mask);
					break;
				case REMMINA_PLUGIN_VNC_EVENT_CUTTEXT:
					SendClientCutText(cl, event->event_data.text.text, strlen(event->event_data.text.text));
					break;
				case REMMINA_PLUGIN_VNC_EVENT_CHAT_OPEN:
					TextChatOpen(cl);
					break;
				case REMMINA_PLUGIN_VNC_EVENT_CHAT_SEND:
					TextChatSend(cl, event->event_data.text.text);
					break;
				case REMMINA_PLUGIN_VNC_EVENT_CHAT_CLOSE:
					TextChatClose(cl);
					TextChatFinish(cl);
					break;
			}
		}
		remmina_plugin_vnc_event_free(event);
	}
	if (read(gpdata->vnc_event_pipe[0], buf, sizeof(buf)))
	{
		/* Ignore */
	}
}

typedef struct _RemminaPluginVncCuttextParam
{
	RemminaProtocolWidget *gp;
	gchar *text;
	gint textlen;
} RemminaPluginVncCuttextParam;

static void remmina_plugin_vnc_update_quality(rfbClient *cl, gint quality)
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

static void remmina_plugin_vnc_update_colordepth(rfbClient *cl, gint colordepth)
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

static rfbBool remmina_plugin_vnc_rfb_allocfb(rfbClient *cl)
{
	RemminaProtocolWidget *gp;
	RemminaPluginVncData *gpdata;
	gint width, height, depth, size;
	gboolean scale;
	GdkPixbuf *new_pixbuf, *old_pixbuf;

	gp = (RemminaProtocolWidget*) (rfbClientGetClientData(cl, NULL));
	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	width = cl->width;
	height = cl->height;
	depth = cl->format.bitsPerPixel;
	size = width * height * (depth / 8);

	/* Putting gdk_pixbuf_new inside a gdk_thread_enter/leave pair could cause dead-lock! */
	new_pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE, 8, width, height);
	if (new_pixbuf == NULL)
		return FALSE;
	gdk_pixbuf_fill(new_pixbuf, 0);
	old_pixbuf = gpdata->rgb_buffer;

	LOCK_BUFFER (TRUE)

	remmina_plugin_service->protocol_plugin_set_width(gp, cl->width);
	remmina_plugin_service->protocol_plugin_set_height(gp, cl->height);

	gpdata->rgb_buffer = new_pixbuf;

	if (gpdata->vnc_buffer)
		g_free(gpdata->vnc_buffer);
	gpdata->vnc_buffer = (guchar*) g_malloc(size);
	cl->frameBuffer = gpdata->vnc_buffer;

	UNLOCK_BUFFER (TRUE)

	if (old_pixbuf)
		g_object_unref(old_pixbuf);

	scale = remmina_plugin_service->protocol_plugin_get_scale(gp);
	THREADS_ENTER
	remmina_plugin_vnc_update_scale( gp, scale);
	THREADS_LEAVE

	if (gpdata->scale_handler == 0)
		remmina_plugin_vnc_update_scale_buffer(gp, TRUE);

	/* Notify window of change so that scroll border can be hidden or shown if needed */
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "desktop-resize");

	/* Refresh the client's updateRect - bug in xvncclient */
	cl->updateRect.w = width;
	cl->updateRect.h = height;

	return TRUE;
}

static gint remmina_plugin_vnc_bits(gint n)
{
	gint b = 0;
	while (n)
	{
		b++;
		n >>= 1;
	}
	return b ? b : 1;
}

static gboolean remmina_plugin_vnc_queue_draw_area_real(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	gint x, y, w, h;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (GTK_IS_WIDGET(gp) && gpdata->connected)
	{
		LOCK_BUFFER (FALSE)
		x = gpdata->queuedraw_x;
		y = gpdata->queuedraw_y;
		w = gpdata->queuedraw_w;
		h = gpdata->queuedraw_h;
		gpdata->queuedraw_handler = 0;
		UNLOCK_BUFFER (FALSE)

		gtk_widget_queue_draw_area(GTK_WIDGET(gp), x, y, w, h);
	}
	return FALSE;
}

static void remmina_plugin_vnc_queue_draw_area(RemminaProtocolWidget *gp, gint x, gint y, gint w, gint h)
{
	RemminaPluginVncData *gpdata;
	gint nx2, ny2, ox2, oy2;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	LOCK_BUFFER (TRUE)
	if (gpdata->queuedraw_handler)
	{
		nx2 = x + w;
		ny2 = y + h;
		ox2 = gpdata->queuedraw_x + gpdata->queuedraw_w;
		oy2 = gpdata->queuedraw_y + gpdata->queuedraw_h;
		gpdata->queuedraw_x = MIN(gpdata->queuedraw_x, x);
		gpdata->queuedraw_y = MIN(gpdata->queuedraw_y, y);
		gpdata->queuedraw_w = MAX(ox2, nx2) - gpdata->queuedraw_x;
		gpdata->queuedraw_h = MAX(oy2, ny2) - gpdata->queuedraw_y;
	}
	else
	{
		gpdata->queuedraw_x = x;
		gpdata->queuedraw_y = y;
		gpdata->queuedraw_w = w;
		gpdata->queuedraw_h = h;
		gpdata->queuedraw_handler = IDLE_ADD((GSourceFunc) remmina_plugin_vnc_queue_draw_area_real, gp);
	}
UNLOCK_BUFFER (TRUE)
}

static void remmina_plugin_vnc_rfb_fill_buffer(rfbClient* cl, guchar *dest, gint dest_rowstride, guchar *src,
		gint src_rowstride, guchar *mask, gint w, gint h)
{
	guchar *destptr, *srcptr;
	gint bytesPerPixel;
	guint32 pixel;
	gint ix, iy;
	gint i;
	guchar c;
	gint rs, gs, bs, rm, gm, bm, rl, gl, bl, rr, gr, br;
	gint r;

	bytesPerPixel = cl->format.bitsPerPixel / 8;
	switch (cl->format.bitsPerPixel)
	{
		case 32:
			/* The following codes fill in the Alpha channel swap red/green value */
			for (iy = 0; iy < h; iy++)
			{
				destptr = dest + iy * dest_rowstride;
				srcptr = src + iy * src_rowstride;
				for (ix = 0; ix < w; ix++)
				{
					*destptr++ = *(srcptr + 2);
					*destptr++ = *(srcptr + 1);
					*destptr++ = *srcptr;
					if (mask)
						*destptr++ = (*mask++) ? 0xff : 0x00;
					srcptr += 4;
				}
			}
			break;
		default:
			rm = cl->format.redMax;
			gm = cl->format.greenMax;
			bm = cl->format.blueMax;
			rr = remmina_plugin_vnc_bits(rm);
			gr = remmina_plugin_vnc_bits(gm);
			br = remmina_plugin_vnc_bits(bm);
			rl = 8 - rr;
			gl = 8 - gr;
			bl = 8 - br;
			rs = cl->format.redShift;
			gs = cl->format.greenShift;
			bs = cl->format.blueShift;
			for (iy = 0; iy < h; iy++)
			{
				destptr = dest + iy * dest_rowstride;
				srcptr = src + iy * src_rowstride;
				for (ix = 0; ix < w; ix++)
				{
					pixel = 0;
					for (i = 0; i < bytesPerPixel; i++)
						pixel += (*srcptr++) << (8 * i);
					c = (guchar)((pixel >> rs) & rm) << rl;
					for (r = rr; r < 8; r *= 2)
						c |= c >> r;
					*destptr++ = c;
					c = (guchar)((pixel >> gs) & gm) << gl;
					for (r = gr; r < 8; r *= 2)
						c |= c >> r;
					*destptr++ = c;
					c = (guchar)((pixel >> bs) & bm) << bl;
					for (r = br; r < 8; r *= 2)
						c |= c >> r;
					*destptr++ = c;
					if (mask)
						*destptr++ = (*mask++) ? 0xff : 0x00;
				}
			}
			break;
	}
}

static void remmina_plugin_vnc_rfb_updatefb(rfbClient* cl, int x, int y, int w, int h)
{
	RemminaProtocolWidget *gp;
	RemminaPluginVncData *gpdata;
	gint bytesPerPixel;
	gint rowstride;
	gint width;

	gp = (RemminaProtocolWidget*) (rfbClientGetClientData(cl, NULL));
	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	LOCK_BUFFER (TRUE)

	if (w >= 1 || h >= 1)
	{
		width = remmina_plugin_service->protocol_plugin_get_width(gp);
		bytesPerPixel = cl->format.bitsPerPixel / 8;
		rowstride = gdk_pixbuf_get_rowstride(gpdata->rgb_buffer);
		remmina_plugin_vnc_rfb_fill_buffer(cl, gdk_pixbuf_get_pixels(gpdata->rgb_buffer) + y * rowstride + x * 3,
				rowstride, gpdata->vnc_buffer + ((y * width + x) * bytesPerPixel), width * bytesPerPixel, NULL,
				w, h);
	}

	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
	{
		remmina_plugin_vnc_scale_area(gp, &x, &y, &w, &h);
	}

	UNLOCK_BUFFER (TRUE)

	remmina_plugin_vnc_queue_draw_area(gp, x, y, w, h);
}

static gboolean remmina_plugin_vnc_queue_cuttext(RemminaPluginVncCuttextParam *param)
{
	RemminaProtocolWidget *gp;
	RemminaPluginVncData *gpdata;
	GTimeVal t;
	glong diff;

	gp = param->gp;
	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (GTK_IS_WIDGET(gp) && gpdata->connected)
	{
		g_get_current_time(&t);
		diff = (t.tv_sec - gpdata->clipboard_timer.tv_sec) * 10
				+ (t.tv_usec - gpdata->clipboard_timer.tv_usec) / 100000;
		if (diff >= 10)
		{
			gpdata->clipboard_timer = t;
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), param->text, param->textlen);
		}
	}
	g_free(param->text);
	g_free(param);
	return FALSE;
}

static void remmina_plugin_vnc_rfb_cuttext(rfbClient* cl, const char *text, int textlen)
{
	RemminaPluginVncCuttextParam *param;

	param = g_new(RemminaPluginVncCuttextParam, 1);
	param->gp = (RemminaProtocolWidget*) rfbClientGetClientData(cl, NULL);
	param->text = g_malloc(textlen);
	memcpy(param->text, text, textlen);
	param->textlen = textlen;
	IDLE_ADD((GSourceFunc) remmina_plugin_vnc_queue_cuttext, param);
}

static char*
remmina_plugin_vnc_rfb_password(rfbClient *cl)
{
	RemminaProtocolWidget *gp;
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	gint ret;
	gchar *pwd = NULL;

	gp = (RemminaProtocolWidget*) (rfbClientGetClientData(cl, NULL));
	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	gpdata->auth_called = TRUE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (gpdata->auth_first)
	{
		THREADS_ENTER pwd = remmina_plugin_service->file_get_secret(remminafile, "password");
		THREADS_LEAVE
	}
	if (!pwd)
	{
		THREADS_ENTER ret = remmina_plugin_service->protocol_plugin_init_authpwd(gp, REMMINA_AUTHPWD_TYPE_PROTOCOL);
		THREADS_LEAVE

		if (ret == GTK_RESPONSE_OK)
		{
			pwd = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		}
		else
		{
			gpdata->connected = FALSE;
		}
	}
	return pwd;
}

#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
static rfbCredential*
remmina_plugin_vnc_rfb_credential (rfbClient *cl, int credentialType)
{
	rfbCredential *cred;
	RemminaProtocolWidget *gp;
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	gint ret;
	gchar *s1, *s2;

	gp = (RemminaProtocolWidget*) (rfbClientGetClientData (cl, NULL));
	gpdata = (RemminaPluginVncData*) g_object_get_data (G_OBJECT(gp), "plugin-data");
	gpdata->auth_called = TRUE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

	cred = g_new0 (rfbCredential, 1);

	switch (credentialType)
	{

		case rfbCredentialTypeUser:

		s1 = g_strdup (remmina_plugin_service->file_get_string (remminafile, "username"));
		THREADS_ENTER
		s2 = remmina_plugin_service->file_get_secret (remminafile, "password");
		THREADS_LEAVE
		if (gpdata->auth_first && s1 && s2)
		{
			cred->userCredential.username = s1;
			cred->userCredential.password = s2;
		}
		else
		{
			g_free(s1);
			g_free(s2);

			THREADS_ENTER
			ret = remmina_plugin_service->protocol_plugin_init_authuserpwd (gp, FALSE);
			THREADS_LEAVE

			if (ret == GTK_RESPONSE_OK)
			{
				cred->userCredential.username = remmina_plugin_service->protocol_plugin_init_get_username (gp);
				cred->userCredential.password = remmina_plugin_service->protocol_plugin_init_get_password (gp);
			}
			else
			{
				g_free(cred);
				cred = NULL;
				gpdata->connected = FALSE;
			}
		}
		break;

		case rfbCredentialTypeX509:
		if (gpdata->auth_first &&
				remmina_plugin_service->file_get_string (remminafile, "cacert"))
		{
			cred->x509Credential.x509CACertFile = g_strdup (remmina_plugin_service->file_get_string (remminafile, "cacert"));
			cred->x509Credential.x509CACrlFile = g_strdup (remmina_plugin_service->file_get_string (remminafile, "cacrl"));
			cred->x509Credential.x509ClientCertFile = g_strdup (remmina_plugin_service->file_get_string (remminafile, "clientcert"));
			cred->x509Credential.x509ClientKeyFile = g_strdup (remmina_plugin_service->file_get_string (remminafile, "clientkey"));
		}
		else
		{
			THREADS_ENTER
			ret = remmina_plugin_service->protocol_plugin_init_authx509 (gp);
			THREADS_LEAVE

			if (ret == GTK_RESPONSE_OK)
			{
				cred->x509Credential.x509CACertFile = remmina_plugin_service->protocol_plugin_init_get_cacert (gp);
				cred->x509Credential.x509CACrlFile = remmina_plugin_service->protocol_plugin_init_get_cacrl (gp);
				cred->x509Credential.x509ClientCertFile = remmina_plugin_service->protocol_plugin_init_get_clientcert (gp);
				cred->x509Credential.x509ClientKeyFile = remmina_plugin_service->protocol_plugin_init_get_clientkey (gp);
			}
			else
			{
				g_free(cred);
				cred = NULL;
				gpdata->connected = FALSE;
			}
		}
		break;

		default:
		g_free(cred);
		cred = NULL;
		break;
	}
	return cred;
}
#endif

static void remmina_plugin_vnc_rfb_cursor_shape(rfbClient *cl, int xhot, int yhot, int width, int height, int bytesPerPixel)
{
	RemminaProtocolWidget *gp;
	RemminaPluginVncData *gpdata;
	guchar *pixbuf_data;
	GdkPixbuf *pixbuf;

	gp = (RemminaProtocolWidget*) (rfbClientGetClientData(cl, NULL));
	if (!gtk_widget_get_window(GTK_WIDGET(gp)))
		return;
	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	if (width && height)
	{
		pixbuf_data = g_malloc(width * height * 4);
		remmina_plugin_vnc_rfb_fill_buffer(cl, pixbuf_data, width * 4, cl->rcSource,
				width * cl->format.bitsPerPixel / 8, cl->rcMask, width, height);
		pixbuf = gdk_pixbuf_new_from_data(pixbuf_data, GDK_COLORSPACE_RGB, TRUE, 8, width, height, width * 4,
				(GdkPixbufDestroyNotify) g_free, NULL);

		LOCK_BUFFER (TRUE)
		remmina_plugin_vnc_queuecursor(gp, pixbuf, xhot, yhot);
UNLOCK_BUFFER	(TRUE)
}
}

static void remmina_plugin_vnc_rfb_bell(rfbClient *cl)
{
	RemminaProtocolWidget *gp;
	GdkWindow *window;

	gp = (RemminaProtocolWidget*) (rfbClientGetClientData(cl, NULL));
	window = gtk_widget_get_window(GTK_WIDGET(gp));

	if (window)
		gdk_window_beep(window);
}

/* Translate known VNC messages. It's for intltool only, not for gcc */
#ifdef __DO_NOT_COMPILE_ME__
N_("Unable to connect to VNC server")
N_("Couldn't convert '%s' to host address")
N_("VNC connection failed: %s")
N_("Your connection has been rejected.")
#endif
/* TODO: We only store the last message at this moment. */
#define MAX_ERROR_LENGTH 1000
static gchar vnc_error[MAX_ERROR_LENGTH + 1];

static void remmina_plugin_vnc_rfb_output(const char *format, ...)
{
    va_list args;
	va_start(args, format);
	gchar *f, *p;

	/* eliminate the last \n */
	f = g_strdup (format);
	if (f[strlen (f) - 1] == '\n') f[strlen (f) - 1] = '\0';

	if (g_strcmp0(f, "VNC connection failed: %s") == 0)
	{
		p = va_arg (args, gchar*);
		g_snprintf (vnc_error, MAX_ERROR_LENGTH, _(f), _(p));
	}
	else
	{
		g_vsnprintf (vnc_error, MAX_ERROR_LENGTH, _(f), args);
	}
	g_free(f);
	va_end(args);

	remmina_plugin_service->log_printf ("[VNC]%s\n", vnc_error);
}

static void remmina_plugin_vnc_chat_on_send(RemminaProtocolWidget *gp, const gchar *text)
{
	gchar *ptr;

	/* Need to add a line-feed for UltraVNC */
	ptr = g_strdup_printf("%s\n", text);
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_CHAT_SEND, ptr, NULL, NULL);
	g_free(ptr);
}

static void remmina_plugin_vnc_chat_on_destroy(RemminaProtocolWidget *gp)
{
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_CHAT_CLOSE, NULL, NULL, NULL);
}

static gboolean remmina_plugin_vnc_close_chat(RemminaProtocolWidget *gp)
{
	remmina_plugin_service->protocol_plugin_chat_close(gp);
	return FALSE;
}

static gboolean remmina_plugin_vnc_open_chat(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	rfbClient *cl;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	cl = (rfbClient*) gpdata->client;

	remmina_plugin_service->protocol_plugin_chat_open(gp, cl->desktopName, remmina_plugin_vnc_chat_on_send,
			remmina_plugin_vnc_chat_on_destroy);
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_CHAT_OPEN, NULL, NULL, NULL);
	return FALSE;
}

static void remmina_plugin_vnc_rfb_chat(rfbClient* cl, int value, char *text)
{
	RemminaProtocolWidget *gp;

	gp = (RemminaProtocolWidget*) (rfbClientGetClientData(cl, NULL));
	switch (value)
	{
		case rfbTextChatOpen:
			IDLE_ADD((GSourceFunc) remmina_plugin_vnc_open_chat, gp);
			break;
		case rfbTextChatClose:
			/* Do nothing... but wait for the next rfbTextChatFinished signal */
			break;
		case rfbTextChatFinished:
			IDLE_ADD((GSourceFunc) remmina_plugin_vnc_close_chat, gp);
			break;
		default:
			/* value is the text length */
			THREADS_ENTER
			remmina_plugin_service->protocol_plugin_chat_receive(gp, text);
			THREADS_LEAVE
			break;
	}
}

static gboolean remmina_plugin_vnc_incoming_connection(RemminaProtocolWidget *gp, rfbClient *cl)
{
	RemminaPluginVncData *gpdata;
	fd_set fds;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	gpdata->listen_sock = ListenAtTcpPort(cl->listenPort);
	if (gpdata->listen_sock < 0)
		return FALSE;

	remmina_plugin_service->protocol_plugin_init_show_listen(gp, cl->listenPort);

	remmina_plugin_service->protocol_plugin_start_reverse_tunnel(gp, cl->listenPort);

	FD_ZERO(&fds);
	FD_SET(gpdata->listen_sock, &fds);
	select(gpdata->listen_sock + 1, &fds, NULL, NULL, NULL);

	if (!FD_ISSET(gpdata->listen_sock, &fds))
	{
		close(gpdata->listen_sock);
		gpdata->listen_sock = -1;
		return FALSE;
	}

	cl->sock = AcceptTcpConnection(gpdata->listen_sock);
	close(gpdata->listen_sock);
	gpdata->listen_sock = -1;
	if (cl->sock < 0 || !SetNonBlocking(cl->sock))
	{
		return FALSE;
	}

	return TRUE;
}

static gboolean remmina_plugin_vnc_main_loop(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	gint ret;
	rfbClient *cl;
	fd_set fds;
	struct timeval timeout;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	if (!gpdata->connected)
	{
		gpdata->running = FALSE;
		return FALSE;
	}

	cl = (rfbClient*) gpdata->client;

	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(cl->sock, &fds);
	FD_SET(gpdata->vnc_event_pipe[0], &fds);
	ret = select(MAX(cl->sock, gpdata->vnc_event_pipe[0]) + 1, &fds, NULL, NULL, &timeout);

	/* Sometimes it returns <0 when opening a modal dialog in other window. Absolutely weird */
	/* So we continue looping anyway */
	if (ret <= 0)
		return TRUE;

	if (FD_ISSET(gpdata->vnc_event_pipe[0], &fds))
	{
		remmina_plugin_vnc_process_vnc_event(gp);
	}
	if (FD_ISSET(cl->sock, &fds))
	{
		ret = HandleRFBServerMessage(cl);
		if (!ret)
		{
			gpdata->running = FALSE;
			if (gpdata->connected && !remmina_plugin_service->protocol_plugin_is_closed(gp))
			{
				IDLE_ADD((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);
			}
			return FALSE;
		}
	}

	return TRUE;
}

static gboolean remmina_plugin_vnc_main(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	rfbClient *cl = NULL;
	gchar *host;
	gchar *s = NULL;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	gpdata->running = TRUE;

	rfbClientLog = remmina_plugin_vnc_rfb_output;
	rfbClientErr = remmina_plugin_vnc_rfb_output;

	while (gpdata->connected)
	{
		gpdata->auth_called = FALSE;

		host = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, 5900, TRUE);

		if (host == NULL)
		{
			gpdata->connected = FALSE;
			break;
		}

		cl = rfbGetClient(8, 3, 4);
		cl->MallocFrameBuffer = remmina_plugin_vnc_rfb_allocfb;
		cl->canHandleNewFBSize = TRUE;
		cl->GetPassword = remmina_plugin_vnc_rfb_password;
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
		cl->GetCredential = remmina_plugin_vnc_rfb_credential;
#endif
		cl->GotFrameBufferUpdate = remmina_plugin_vnc_rfb_updatefb;
		cl->GotXCutText = (
				remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE) ?
						NULL : remmina_plugin_vnc_rfb_cuttext);
		cl->GotCursorShape = remmina_plugin_vnc_rfb_cursor_shape;
		cl->Bell = remmina_plugin_vnc_rfb_bell;
		cl->HandleTextChat = remmina_plugin_vnc_rfb_chat;
		rfbClientSetClientData(cl, NULL, gp);

		if (host[0] == '\0')
		{
			cl->serverHost = strdup(host);
			cl->listenSpecified = TRUE;
			if (remmina_plugin_service->file_get_int(remminafile, "ssh_enabled", FALSE))
			{
				/* When we use reverse tunnel, the local port does not really matter.
				 * Hardcode a default port just in case the remote port is customized
				 * to a privilege port then we will have problem listening. */
				cl->listenPort = 5500;
			}
			else
			{
				cl->listenPort = remmina_plugin_service->file_get_int(remminafile, "listenport", 5500);
			}

			remmina_plugin_vnc_incoming_connection(gp, cl);
		}
		else
		{
			remmina_plugin_service->get_server_port(host, 5900, &s, &cl->serverPort);
			cl->serverHost = strdup(s);
			g_free(s);

			/* Support short-form (:0, :1) */
			if (cl->serverPort < 100)
				cl->serverPort += 5900;
		}
		g_free(host);
		host = NULL;

		if (remmina_plugin_service->file_get_string(remminafile, "proxy"))
		{
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
			cl->destHost = cl->serverHost;
			cl->destPort = cl->serverPort;
			remmina_plugin_service->get_server_port (remmina_plugin_service->file_get_string (remminafile, "proxy"), 5900,
					&s, &cl->serverPort);
			cl->serverHost = strdup (s);
			g_free(s);
#endif
		}

		cl->appData.useRemoteCursor = (
				remmina_plugin_service->file_get_int(remminafile, "showcursor", FALSE) ? FALSE : TRUE);

		remmina_plugin_vnc_update_quality(cl, remmina_plugin_service->file_get_int(remminafile, "quality", 0));
		remmina_plugin_vnc_update_colordepth(cl, remmina_plugin_service->file_get_int(remminafile, "colordepth", 8));
		SetFormatAndEncodings(cl);

		if (remmina_plugin_service->file_get_int(remminafile, "disableencryption", FALSE))
		{
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
			SetClientAuthSchemes (cl, remmina_plugin_vnc_no_encrypt_auth_types, -1);
#endif
		}

		if (rfbInitClient(cl, NULL, NULL))
			break;

		/* If the authentication is not called, it has to be a fatel error and must quit */
		if (!gpdata->auth_called)
		{
			gpdata->connected = FALSE;
			break;
		}

		/* vnc4server reports "already in use" after authentication. Workaround here */
		if (strstr(vnc_error, "The server is already in use"))
		{
			gpdata->connected = FALSE;
			gpdata->auth_called = FALSE;
			break;
		}

		/* Otherwise, it's a password error. Try to clear saved password if any */
		remmina_plugin_service->file_set_string(remminafile, "password", NULL);

		if (!gpdata->connected)
			break;

		THREADS_ENTER
		remmina_plugin_service->protocol_plugin_init_show_retry(gp);
		THREADS_LEAVE
		/* It's safer to sleep a while before reconnect */
		sleep(2);

		gpdata->auth_first = FALSE;
	}

	if (!gpdata->connected)
	{
		if (cl && !gpdata->auth_called && !(remmina_plugin_service->protocol_plugin_has_error(gp)))
		{
			remmina_plugin_service->protocol_plugin_set_error(gp, "%s", vnc_error);
		}
		gpdata->running = FALSE;

		IDLE_ADD((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);

		return FALSE;
	}

	THREADS_ENTER
	remmina_plugin_service->protocol_plugin_init_save_cred(gp);
	THREADS_LEAVE

	gpdata->client = cl;

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");

	if (remmina_plugin_service->file_get_int(remminafile, "disableserverinput", FALSE))
	{
		PermitServerInput(cl, 1);
	}

	if (gpdata->thread)
	{
		while (remmina_plugin_vnc_main_loop(gp))
		{
		}
		gpdata->running = FALSE;
	}
	else
	{
		IDLE_ADD((GSourceFunc) remmina_plugin_vnc_main_loop, gp);
	}

	return FALSE;
}

#ifdef HAVE_PTHREAD
static gpointer
remmina_plugin_vnc_main_thread (gpointer data)
{
	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

	CANCEL_ASYNC
	remmina_plugin_vnc_main ((RemminaProtocolWidget*) data);
	return NULL;
}
#endif

static gboolean remmina_plugin_vnc_on_motion(GtkWidget *widget, GdkEventMotion *event, RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	gint x, y;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (!gpdata->connected || !gpdata->client)
		return FALSE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return FALSE;

	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
	{
		x = event->x * remmina_plugin_service->protocol_plugin_get_width(gp) / gpdata->scale_width;
		y = event->y * remmina_plugin_service->protocol_plugin_get_height(gp) / gpdata->scale_height;
	}
	else
	{
		x = event->x;
		y = event->y;
	}
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_POINTER, GINT_TO_POINTER(x), GINT_TO_POINTER(y),
			GINT_TO_POINTER(gpdata->button_mask));
	return TRUE;
}

static gboolean remmina_plugin_vnc_on_button(GtkWidget *widget, GdkEventButton *event, RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	gint x, y;
	gint mask;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (!gpdata->connected || !gpdata->client)
		return FALSE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return FALSE;

	/* We only accept 3 buttons */
	if (event->button < 1 || event->button > 3)
		return FALSE;
	/* We bypass 2button-press and 3button-press events */
	if (event->type != GDK_BUTTON_PRESS && event->type != GDK_BUTTON_RELEASE)
		return TRUE;

	mask = (1 << (event->button - 1));
	gpdata->button_mask = (event->type == GDK_BUTTON_PRESS ? (gpdata->button_mask | mask) :
	(gpdata->button_mask & (0xff - mask)))
;	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
	{
		x = event->x * remmina_plugin_service->protocol_plugin_get_width(gp) / gpdata->scale_width;
		y = event->y * remmina_plugin_service->protocol_plugin_get_height(gp) / gpdata->scale_height;
	}
	else
	{
		x = event->x;
		y = event->y;
	}
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_POINTER, GINT_TO_POINTER(x), GINT_TO_POINTER(y),
			GINT_TO_POINTER(gpdata->button_mask));
	return TRUE;
}

static gboolean remmina_plugin_vnc_on_scroll(GtkWidget *widget, GdkEventScroll *event, RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	gint x, y;
	gint mask;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (!gpdata->connected || !gpdata->client)
		return FALSE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return FALSE;

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
#ifdef GDK_SCROLL_SMOOTH
		case GDK_SCROLL_SMOOTH:
			if (event->delta_y < 0)
				mask = (1 << 3);
			if (event->delta_y > 0)
				mask = (1 << 4);
			if (event->delta_x < 0)
				mask = (1 << 5);
			if (event->delta_x > 0)
				mask = (1 << 6);
			if (!mask)
				return FALSE;
			break;
#endif
		default:
			return FALSE;
	}

	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
	{
		x = event->x * remmina_plugin_service->protocol_plugin_get_width(gp) / gpdata->scale_width;
		y = event->y * remmina_plugin_service->protocol_plugin_get_height(gp) / gpdata->scale_height;
	}
	else
	{
		x = event->x;
		y = event->y;
	}
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_POINTER, GINT_TO_POINTER(x), GINT_TO_POINTER(y),
			GINT_TO_POINTER(mask | gpdata->button_mask));
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_POINTER, GINT_TO_POINTER(x), GINT_TO_POINTER(y),
			GINT_TO_POINTER(gpdata->button_mask));

	return TRUE;
}

static void remmina_plugin_vnc_release_key(RemminaProtocolWidget *gp, guint16 keycode)
{
	RemminaPluginVncData *gpdata;
	RemminaKeyVal *k;
	gint i;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (keycode == 0)
	{
		/* Send all release key events for previously pressed keys */
		for (i = 0; i < gpdata->pressed_keys->len; i++)
		{
			k = g_ptr_array_index(gpdata->pressed_keys, i);
			remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_KEY, GUINT_TO_POINTER(k->keyval),
					GINT_TO_POINTER(FALSE), NULL);
			g_free(k);
		}
		g_ptr_array_set_size(gpdata->pressed_keys, 0);
	}
	else
	{
		/* Unregister the keycode only */
		for (i = 0; i < gpdata->pressed_keys->len; i++)
		{
			k = g_ptr_array_index(gpdata->pressed_keys, i);
			if (k->keycode == keycode)
			{
				g_free(k);
				g_ptr_array_remove_index_fast(gpdata->pressed_keys, i);
				break;
			}
		}
	}
}

static gboolean remmina_plugin_vnc_on_key(GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;
	RemminaKeyVal *k;
	guint keyval;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (!gpdata->connected || !gpdata->client)
		return FALSE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return FALSE;

	keyval = remmina_plugin_service->pref_keymap_get_keyval(remmina_plugin_service->file_get_string(remminafile, "keymap"),
			event->keyval);

	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_KEY, GUINT_TO_POINTER(keyval),
			GINT_TO_POINTER(event->type == GDK_KEY_PRESS ? TRUE : FALSE), NULL);

	/* Register/unregister the pressed key */
	if (event->type == GDK_KEY_PRESS)
	{
		k = g_new(RemminaKeyVal, 1);
		k->keyval = keyval;
		k->keycode = event->hardware_keycode;
		g_ptr_array_add(gpdata->pressed_keys, k);
	}
	else
	{
		remmina_plugin_vnc_release_key(gp, event->hardware_keycode);
	}
	return TRUE;
}

static void remmina_plugin_vnc_on_cuttext_request(GtkClipboard *clipboard, const gchar *text, RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	GTimeVal t;
	glong diff;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	if (text)
	{
		/* A timer (1 second) to avoid clipboard "loopback": text cut out from VNC won't paste back into VNC */
		g_get_current_time(&t);
		diff = (t.tv_sec - gpdata->clipboard_timer.tv_sec) * 10
				+ (t.tv_usec - gpdata->clipboard_timer.tv_usec) / 100000;
		if (diff < 10)
			return;

		gpdata->clipboard_timer = t;
		remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_CUTTEXT, (gpointer) text, NULL, NULL);
	}
}

static void remmina_plugin_vnc_on_cuttext(GtkClipboard *clipboard, GdkEvent *event, RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (!gpdata->connected || !gpdata->client)
		return;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return;

	gtk_clipboard_request_text(clipboard, (GtkClipboardTextReceivedFunc) remmina_plugin_vnc_on_cuttext_request, gp);
}

static void remmina_plugin_vnc_on_realize(RemminaProtocolWidget *gp, gpointer data)
{
	RemminaFile *remminafile;
	GdkCursor *cursor;
	GdkPixbuf *pixbuf;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (remmina_plugin_service->file_get_int(remminafile, "showcursor", FALSE))
	{
		/* Hide local cursor (show a small dot instead) */
		pixbuf = gdk_pixbuf_new_from_xpm_data(dot_cursor_xpm);
		cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), pixbuf, dot_cursor_x_hot, dot_cursor_y_hot);
		g_object_unref(pixbuf);
		gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(gp)), cursor);
#if GTK_VERSION == 3
		g_object_unref(cursor);
#else
		gdk_cursor_unref(cursor);
#endif
	}
}

/******************************************************************************************/

static gboolean remmina_plugin_vnc_open_connection(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	gpdata->connected = TRUE;

	remmina_plugin_service->protocol_plugin_register_hostkey(gp, gpdata->drawing_area);

	g_signal_connect(G_OBJECT(gp), "realize", G_CALLBACK(remmina_plugin_vnc_on_realize), NULL);
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "motion-notify-event", G_CALLBACK(remmina_plugin_vnc_on_motion), gp);
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "button-press-event", G_CALLBACK(remmina_plugin_vnc_on_button), gp);
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "button-release-event", G_CALLBACK(remmina_plugin_vnc_on_button), gp);
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "scroll-event", G_CALLBACK(remmina_plugin_vnc_on_scroll), gp);
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "key-press-event", G_CALLBACK(remmina_plugin_vnc_on_key), gp);
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "key-release-event", G_CALLBACK(remmina_plugin_vnc_on_key), gp);

	if (!remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE))
	{
		gpdata->clipboard_handler = g_signal_connect(G_OBJECT(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)),
				"owner-change", G_CALLBACK(remmina_plugin_vnc_on_cuttext), gp);
	}

#ifdef HAVE_PTHREAD
	if (pthread_create (&gpdata->thread, NULL, remmina_plugin_vnc_main_thread, gp))
	{
		/* I don't think this will ever happen... */
		g_print("Failed to initialize pthread. Falling back to non-thread mode...\n");
		g_timeout_add (0, (GSourceFunc) remmina_plugin_vnc_main, gp);
		gpdata->thread = 0;
	}
#else
	g_timeout_add(0, (GSourceFunc) remmina_plugin_vnc_main, gp);
#endif

	return TRUE;
}

static gboolean remmina_plugin_vnc_close_connection_timeout(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	/* wait until the running attribute is set to false by the VNC thread */
	if (gpdata->running)
		return TRUE;

	/* unregister the clipboard monitor */
	if (gpdata->clipboard_handler)
	{
		g_signal_handler_disconnect(G_OBJECT(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)), gpdata->clipboard_handler);
		gpdata->clipboard_handler = 0;
	}

	if (gpdata->queuecursor_handler)
	{
		g_source_remove(gpdata->queuecursor_handler);
		gpdata->queuecursor_handler = 0;
	}
	if (gpdata->queuecursor_pixbuf)
	{
		g_object_unref(gpdata->queuecursor_pixbuf);
		gpdata->queuecursor_pixbuf = NULL;
	}

	if (gpdata->queuedraw_handler)
	{
		g_source_remove(gpdata->queuedraw_handler);
		gpdata->queuedraw_handler = 0;
	}
	if (gpdata->scale_handler)
	{
		g_source_remove(gpdata->scale_handler);
		gpdata->scale_handler = 0;
	}
	if (gpdata->listen_sock >= 0)
	{
		close(gpdata->listen_sock);
	}
	if (gpdata->client)
	{
		rfbClientCleanup((rfbClient*) gpdata->client);
		gpdata->client = NULL;
	}
	if (gpdata->rgb_buffer)
	{
		g_object_unref(gpdata->rgb_buffer);
		gpdata->rgb_buffer = NULL;
	}
	if (gpdata->vnc_buffer)
	{
		g_free(gpdata->vnc_buffer);
		gpdata->vnc_buffer = NULL;
	}
	if (gpdata->scale_buffer)
	{
		g_object_unref(gpdata->scale_buffer);
		gpdata->scale_buffer = NULL;
	}
	g_ptr_array_free(gpdata->pressed_keys, TRUE);
	remmina_plugin_vnc_event_free_all(gp);
	g_queue_free(gpdata->vnc_event_queue);
	close(gpdata->vnc_event_pipe[0]);
	close(gpdata->vnc_event_pipe[1]);

#ifdef HAVE_PTHREAD
	pthread_mutex_destroy (&gpdata->buffer_mutex);
#endif

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");

	return FALSE;
}

static gboolean remmina_plugin_vnc_close_connection(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	gpdata->connected = FALSE;

#ifdef HAVE_PTHREAD
	if (gpdata->thread)
	{
		pthread_cancel (gpdata->thread);
		if (gpdata->thread) pthread_join (gpdata->thread, NULL);
		gpdata->running = FALSE;
		remmina_plugin_vnc_close_connection_timeout (gp);
	}
	else
	{
		g_timeout_add (200, (GSourceFunc) remmina_plugin_vnc_close_connection_timeout, gp);
	}
#else
	g_timeout_add(200, (GSourceFunc) remmina_plugin_vnc_close_connection_timeout, gp);
#endif

	return FALSE;
}

static gboolean remmina_plugin_vnc_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	RemminaPluginVncData *gpdata;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	switch (feature->id)
	{
		case REMMINA_PLUGIN_VNC_FEATURE_PREF_DISABLESERVERINPUT:
			return (SupportsClient2Server((rfbClient*) (gpdata->client), rfbSetServerInput) ? TRUE : FALSE);
		case REMMINA_PLUGIN_VNC_FEATURE_TOOL_CHAT:
			return (SupportsClient2Server((rfbClient*) (gpdata->client), rfbTextChat) ? TRUE : FALSE);
		default:
			return TRUE;
	}
}

static void remmina_plugin_vnc_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	RemminaPluginVncData *gpdata;
	RemminaFile *remminafile;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	switch (feature->id)
	{
		case REMMINA_PLUGIN_VNC_FEATURE_PREF_QUALITY:
			remmina_plugin_vnc_update_quality((rfbClient*) (gpdata->client),
					remmina_plugin_service->file_get_int(remminafile, "quality", 0));
			SetFormatAndEncodings((rfbClient*) (gpdata->client));
			break;
		case REMMINA_PLUGIN_VNC_FEATURE_PREF_VIEWONLY:
			break;
		case REMMINA_PLUGIN_VNC_FEATURE_PREF_DISABLESERVERINPUT:
			PermitServerInput((rfbClient*) (gpdata->client),
					remmina_plugin_service->file_get_int(remminafile, "disableserverinput", FALSE) ? 1 : 0);
			break;
		case REMMINA_PLUGIN_VNC_FEATURE_UNFOCUS:
			remmina_plugin_vnc_release_key(gp, 0);
			break;
		case REMMINA_PLUGIN_VNC_FEATURE_SCALE:
			remmina_plugin_vnc_update_scale(gp, remmina_plugin_service->file_get_int(remminafile, "scale", FALSE));
			break;
		case REMMINA_PLUGIN_VNC_FEATURE_TOOL_REFRESH:
			SendFramebufferUpdateRequest((rfbClient*) (gpdata->client), 0, 0,
					remmina_plugin_service->protocol_plugin_get_width(gp),
					remmina_plugin_service->protocol_plugin_get_height(gp), FALSE);
			break;
		case REMMINA_PLUGIN_VNC_FEATURE_TOOL_CHAT:
			remmina_plugin_vnc_open_chat(gp);
			break;
		default:
			break;
	}
}

#if GTK_VERSION == 2
static gboolean remmina_plugin_vnc_on_expose(GtkWidget *widget, GdkEventExpose *event, RemminaProtocolWidget *gp)
#else
static gboolean remmina_plugin_vnc_on_draw(GtkWidget *widget, cairo_t *context, RemminaProtocolWidget *gp)
#endif
{
	RemminaPluginVncData *gpdata;
	GdkPixbuf *buffer;
	gboolean scale;
#if GTK_VERSION == 2
	gint x, y;
	cairo_t *context;
#endif

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");

	LOCK_BUFFER (FALSE)

	scale = remmina_plugin_service->protocol_plugin_get_scale(gp);
	/* widget == gpdata->drawing_area */
	buffer = (scale ? gpdata->scale_buffer : gpdata->rgb_buffer);
	if (!buffer)
	{
		UNLOCK_BUFFER (FALSE)
		return FALSE;
	}
#if GTK_VERSION == 2
	x = event->area.x;
	y = event->area.y;

	context = gdk_cairo_create(gtk_widget_get_window (gpdata->drawing_area));
	cairo_rectangle(context, x, y, event->area.width, event->area.height);
#else
	cairo_rectangle(context, 0, 0, gtk_widget_get_allocated_width(widget), gtk_widget_get_allocated_height(widget));
#endif
	gdk_cairo_set_source_pixbuf(context, buffer, 0, 0);
	cairo_fill(context);
#if GTK_VERSION == 2
	cairo_destroy(context);
#endif

	UNLOCK_BUFFER (FALSE)
	return TRUE;
}

static gboolean remmina_plugin_vnc_on_configure(GtkWidget *widget, GdkEventConfigure *event, RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;

	gpdata = (RemminaPluginVncData*) g_object_get_data(G_OBJECT(gp), "plugin-data");
	/* We do a delayed reallocating to improve performance */
	if (gpdata->scale_handler)
		g_source_remove(gpdata->scale_handler);
	gpdata->scale_handler = g_timeout_add(1000, (GSourceFunc) remmina_plugin_vnc_update_scale_buffer_main, gp);
	return FALSE;
}

static void remmina_plugin_vnc_init(RemminaProtocolWidget *gp)
{
	RemminaPluginVncData *gpdata;
	gint flags;

	gpdata = g_new0(RemminaPluginVncData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	gpdata->drawing_area = gtk_drawing_area_new();
	gtk_widget_show(gpdata->drawing_area);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->drawing_area);

	gtk_widget_add_events(
			gpdata->drawing_area,
			GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK
					| GDK_KEY_RELEASE_MASK | GDK_SCROLL_MASK);
	gtk_widget_set_can_focus(gpdata->drawing_area, TRUE);

#if GTK_VERSION == 3
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "draw", G_CALLBACK(remmina_plugin_vnc_on_draw), gp);
#elif GTK_VERSION == 2
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "expose-event", G_CALLBACK(remmina_plugin_vnc_on_expose), gp);
#endif
	g_signal_connect(G_OBJECT(gpdata->drawing_area), "configure_event", G_CALLBACK(remmina_plugin_vnc_on_configure), gp);

	gpdata->auth_first = TRUE;
	g_get_current_time(&gpdata->clipboard_timer);
	gpdata->listen_sock = -1;
	gpdata->pressed_keys = g_ptr_array_new();
	gpdata->vnc_event_queue = g_queue_new();
	if (pipe(gpdata->vnc_event_pipe))
	{
		g_print("Error creating pipes.\n");
		gpdata->vnc_event_pipe[0] = 0;
		gpdata->vnc_event_pipe[1] = 0;
	}
	flags = fcntl(gpdata->vnc_event_pipe[0], F_GETFL, 0);
	fcntl(gpdata->vnc_event_pipe[0], F_SETFL, flags | O_NONBLOCK);
#ifdef HAVE_PTHREAD
	pthread_mutex_init (&gpdata->buffer_mutex, NULL);
#endif
}

static gpointer colordepth_list[] =
		{ "8", N_("256 colors"), "15", N_("High color (15 bit)"), "16", N_("High color (16 bit)"), "24", N_(
				"True color (24 bit)"), NULL };

static gpointer quality_list[] =
{ "0", N_("Poor (fastest)"), "1", N_("Medium"), "2", N_("Good"), "9", N_("Best (slowest)"), NULL };

static const RemminaProtocolSetting remmina_plugin_vnc_basic_settings[] =
{
{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, "_rfb._tcp", NULL },
#ifdef LIBVNCSERVER_WITH_CLIENT_TLS
		{	REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "proxy", N_("Repeater"), FALSE, NULL, NULL},
#endif
		{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "colordepth", N_("Color depth"), FALSE, colordepth_list, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "quality", N_("Quality"), FALSE, quality_list, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "showcursor", N_("Show remote cursor"), TRUE, NULL, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "viewonly", N_("View only"), FALSE, NULL, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard", N_("Disable clipboard sync"), TRUE, NULL, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableencryption", N_("Disable encryption"), FALSE, NULL, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableserverinput", N_("Disable server input"), FALSE, NULL, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL } };

static const RemminaProtocolSetting remmina_plugin_vnci_basic_settings[] =
{
{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "listenport", N_("Listen on port"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "colordepth", N_("Color depth"), FALSE, colordepth_list, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "quality", N_("Quality"), FALSE, quality_list, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "showcursor", N_("Show remote cursor"), TRUE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "viewonly", N_("View only"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard", N_("Disable clipboard sync"), TRUE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableencryption", N_("Disable encryption"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableserverinput", N_("Disable server input"), FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL } };

static const RemminaProtocolSetting remmina_plugin_vnc_advanced_settings[] =
{
{ REMMINA_PROTOCOL_SETTING_TYPE_SCALE, NULL, NULL, FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_KEYMAP, NULL, NULL, FALSE, NULL, NULL },
{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL } };

static const RemminaProtocolFeature remmina_plugin_vnc_features[] =
{
{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF, REMMINA_PLUGIN_VNC_FEATURE_PREF_QUALITY, GINT_TO_POINTER(
		REMMINA_PROTOCOL_FEATURE_PREF_RADIO), "quality", quality_list },
{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF, REMMINA_PLUGIN_VNC_FEATURE_PREF_VIEWONLY, GINT_TO_POINTER(
		REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "viewonly", N_("View only") },
{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF, REMMINA_PLUGIN_VNC_FEATURE_PREF_DISABLESERVERINPUT, GINT_TO_POINTER(
		REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "disableserverinput", N_("Disable server input") },
{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_VNC_FEATURE_TOOL_REFRESH, N_("Refresh"), GTK_STOCK_REFRESH, NULL },
{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_VNC_FEATURE_TOOL_CHAT, N_("Open Chat..."), "face-smile", NULL },
{ REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, REMMINA_PLUGIN_VNC_FEATURE_SCALE, NULL, NULL, NULL },
{ REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, REMMINA_PLUGIN_VNC_FEATURE_UNFOCUS, NULL, NULL, NULL },
{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL } };

static RemminaProtocolPlugin remmina_plugin_vnc =
{ REMMINA_PLUGIN_TYPE_PROTOCOL, "VNC", N_("VNC - Virtual Network Computing"), GETTEXT_PACKAGE, VERSION,

"remmina-vnc", "remmina-vnc-ssh", remmina_plugin_vnc_basic_settings, remmina_plugin_vnc_advanced_settings,
		REMMINA_PROTOCOL_SSH_SETTING_TUNNEL, remmina_plugin_vnc_features,

		remmina_plugin_vnc_init, remmina_plugin_vnc_open_connection, remmina_plugin_vnc_close_connection,
		remmina_plugin_vnc_query_feature, remmina_plugin_vnc_call_feature };

static RemminaProtocolPlugin remmina_plugin_vnci =
{ REMMINA_PLUGIN_TYPE_PROTOCOL, "VNCI", N_("VNC - Incoming Connection"), GETTEXT_PACKAGE, VERSION,

"remmina-vnc", "remmina-vnc-ssh", remmina_plugin_vnci_basic_settings, remmina_plugin_vnc_advanced_settings,
		REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL, remmina_plugin_vnc_features,

		remmina_plugin_vnc_init, remmina_plugin_vnc_open_connection, remmina_plugin_vnc_close_connection,
		remmina_plugin_vnc_query_feature, remmina_plugin_vnc_call_feature };

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_vnc))
	{
		return FALSE;
	}

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_vnci))
	{
		return FALSE;
	}

	return TRUE;
}

