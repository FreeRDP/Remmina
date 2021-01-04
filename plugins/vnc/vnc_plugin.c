/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
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
#include <gmodule.h>
#include "vnc_plugin.h"
#include <rfb/rfbclient.h>

#define REMMINA_PLUGIN_VNC_FEATURE_PREF_QUALITY            1
#define REMMINA_PLUGIN_VNC_FEATURE_PREF_VIEWONLY           2
#define REMMINA_PLUGIN_VNC_FEATURE_PREF_DISABLESERVERINPUT 3
#define REMMINA_PLUGIN_VNC_FEATURE_TOOL_REFRESH            4
#define REMMINA_PLUGIN_VNC_FEATURE_TOOL_CHAT               5
#define REMMINA_PLUGIN_VNC_FEATURE_SCALE                   6
#define REMMINA_PLUGIN_VNC_FEATURE_UNFOCUS                 7
#define REMMINA_PLUGIN_VNC_FEATURE_TOOL_SENDCTRLALTDEL     8

#define GET_PLUGIN_DATA(gp) (RemminaPluginVncData *)g_object_get_data(G_OBJECT(gp), "plugin-data")

static RemminaPluginService *remmina_plugin_service = NULL;
#define REMMINA_PLUGIN_DEBUG(fmt, ...) remmina_plugin_service->_remmina_debug(__func__, fmt, ##__VA_ARGS__)

static int dot_cursor_x_hot = 2;
static int dot_cursor_y_hot = 2;
static const gchar *dot_cursor_xpm[] =
{ "5 5 3 1", "  c None", ".	c #000000", "+	c #FFFFFF", " ... ", ".+++.", ".+ +.", ".+++.", " ... " };


#define LOCK_BUFFER(t)      if (t) { CANCEL_DEFER } pthread_mutex_lock(&gpdata->buffer_mutex);
#define UNLOCK_BUFFER(t)    pthread_mutex_unlock(&gpdata->buffer_mutex); if (t) { CANCEL_ASYNC }

struct onMainThread_cb_data {
	enum { FUNC_UPDATE_SCALE } func;
	GtkWidget *		widget;
	gint			x, y, width, height;
	RemminaProtocolWidget * gp;
	gboolean		scale;

	/* Mutex for thread synchronization */
	pthread_mutex_t		mu;
	/* Flag to catch cancellations */
	gboolean		cancelled;
};

static gboolean onMainThread_cb(struct onMainThread_cb_data *d)
{
	TRACE_CALL(__func__);
	if (!d->cancelled) {
		switch (d->func) {
		case FUNC_UPDATE_SCALE:
			remmina_plugin_vnc_update_scale(d->gp, d->scale);
			break;
		}
		pthread_mutex_unlock(&d->mu);
	} else {
		/* thread has been cancelled, so we must free d memory here */
		g_free(d);
	}
	return G_SOURCE_REMOVE;
}


static void onMainThread_cleanup_handler(gpointer data)
{
	TRACE_CALL(__func__);
	struct onMainThread_cb_data *d = data;
	d->cancelled = TRUE;
}


static void onMainThread_schedule_callback_and_wait(struct onMainThread_cb_data *d)
{
	TRACE_CALL(__func__);
	d->cancelled = FALSE;
	pthread_cleanup_push(onMainThread_cleanup_handler, d);
	pthread_mutex_init(&d->mu, NULL);
	pthread_mutex_lock(&d->mu);
	gdk_threads_add_idle((GSourceFunc)onMainThread_cb, (gpointer)d);

	pthread_mutex_lock(&d->mu);

	pthread_cleanup_pop(0);
	pthread_mutex_unlock(&d->mu);
	pthread_mutex_destroy(&d->mu);
}

/**
   Function check_for_endianness() returns 1, if architecture
   is little endian, 0 in case of big endian.
 */
static gboolean check_for_endianness()
{
  unsigned int x = 1;
  char *c = (char*) &x;
  return (int)*c;
}

static void remmina_plugin_vnc_event_push(RemminaProtocolWidget *gp, gint event_type, gpointer p1, gpointer p2, gpointer p3)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaPluginVncEvent *event;

	event = g_new(RemminaPluginVncEvent, 1);
	event->event_type = event_type;
	switch (event_type) {
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
		event->event_data.text.text = g_strdup((char *)p1);
		break;
	default:
		break;
	}

	pthread_mutex_lock(&gpdata->vnc_event_queue_mutex);
	g_queue_push_tail(gpdata->vnc_event_queue, event);
	pthread_mutex_unlock(&gpdata->vnc_event_queue_mutex);

	if (write(gpdata->vnc_event_pipe[1], "\0", 1)) {
		/* Ignore */
	}
}

static void remmina_plugin_vnc_event_free(RemminaPluginVncEvent *event)
{
	TRACE_CALL(__func__);
	switch (event->event_type) {
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
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaPluginVncEvent *event;

	/* This is called from main thread after plugin thread has
	 * been closed, so no queue locking is necessary here */
	while ((event = g_queue_pop_head(gpdata->vnc_event_queue)) != NULL)
		remmina_plugin_vnc_event_free(event);
}

static void remmina_plugin_vnc_scale_area(RemminaProtocolWidget *gp, gint *x, gint *y, gint *w, gint *h)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	GtkAllocation widget_allocation;
	gint width, height;
	gint sx, sy, sw, sh;

	if (gpdata->rgb_buffer == NULL)
		return;

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);

	gtk_widget_get_allocation(GTK_WIDGET(gp), &widget_allocation);

	if (widget_allocation.width == width && widget_allocation.height == height)
		return; /* Same size, no scaling */

	/* We have to extend the scaled region 2 scaled pixels, to avoid gaps */
	sx = MIN(MAX(0, (*x) * widget_allocation.width / width - widget_allocation.width / width - 2), widget_allocation.width - 1);
	sy = MIN(MAX(0, (*y) * widget_allocation.height / height - widget_allocation.height / height - 2), widget_allocation.height - 1);
	sw = MIN(widget_allocation.width - sx, (*w) * widget_allocation.width / width + widget_allocation.width / width + 4);
	sh = MIN(widget_allocation.height - sy, (*h) * widget_allocation.height / height + widget_allocation.height / height + 4);

	*x = sx;
	*y = sy;
	*w = sw;
	*h = sh;
}

static void remmina_plugin_vnc_update_scale(RemminaProtocolWidget *gp, gboolean scale)
{
	TRACE_CALL(__func__);
	/* This function can be called from a non main thread */

	RemminaPluginVncData *gpdata;
	gint width, height;

	if (!remmina_plugin_service->is_main_thread()) {
		struct onMainThread_cb_data *d;
		d = (struct onMainThread_cb_data *)g_malloc(sizeof(struct onMainThread_cb_data));
		d->func = FUNC_UPDATE_SCALE;
		d->gp = gp;
		d->scale = scale;
		onMainThread_schedule_callback_and_wait(d);
		g_free(d);
		return;
	}

	gpdata = GET_PLUGIN_DATA(gp);

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);
	if (scale)
		/* In scaled mode, drawing_area will get its dimensions from its parent */
		gtk_widget_set_size_request(GTK_WIDGET(gpdata->drawing_area), -1, -1);
	else
		/* In non scaled mode, the plugins forces dimensions of drawing area */
		gtk_widget_set_size_request(GTK_WIDGET(gpdata->drawing_area), width, height);

	remmina_plugin_service->protocol_plugin_update_align(gp);
}

gboolean remmina_plugin_vnc_setcursor(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	GdkCursor *cur;

	LOCK_BUFFER(FALSE);
	gpdata->queuecursor_handler = 0;

	if (gpdata->queuecursor_surface) {
		cur = gdk_cursor_new_from_surface(gdk_display_get_default(), gpdata->queuecursor_surface, gpdata->queuecursor_x,
						  gpdata->queuecursor_y);
		gdk_window_set_cursor(gtk_widget_get_window(gpdata->drawing_area), cur);
		g_object_unref(cur);
		cairo_surface_destroy(gpdata->queuecursor_surface);
		gpdata->queuecursor_surface = NULL;
	} else {
		gdk_window_set_cursor(gtk_widget_get_window(gpdata->drawing_area), NULL);
	}
	UNLOCK_BUFFER(FALSE);

	return FALSE;
}

static void remmina_plugin_vnc_queuecursor(RemminaProtocolWidget *gp, cairo_surface_t *surface, gint x, gint y)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);

	if (gpdata->queuecursor_surface)
		cairo_surface_destroy(gpdata->queuecursor_surface);
	gpdata->queuecursor_surface = surface;
	gpdata->queuecursor_x = x;
	gpdata->queuecursor_y = y;
	if (!gpdata->queuecursor_handler)
		gpdata->queuecursor_handler = IDLE_ADD((GSourceFunc)remmina_plugin_vnc_setcursor, gp);
}

typedef struct _RemminaKeyVal {
	guint	keyval;
	guint16 keycode;
} RemminaKeyVal;

/***************************** LibVNCClient related codes *********************************/

static const uint32_t remmina_plugin_vnc_no_encrypt_auth_types[] =
{ rfbNoAuth, rfbVncAuth, rfbMSLogon, 0 };

static RemminaPluginVncEvent *remmina_plugin_vnc_event_queue_pop_head(RemminaPluginVncData *gpdata)
{
	RemminaPluginVncEvent *event;

	CANCEL_DEFER;
	pthread_mutex_lock(&gpdata->vnc_event_queue_mutex);

	event = g_queue_pop_head(gpdata->vnc_event_queue);

	pthread_mutex_unlock(&gpdata->vnc_event_queue_mutex);
	CANCEL_ASYNC;

	return event;
}

static void remmina_plugin_vnc_process_vnc_event(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncEvent *event;
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	rfbClient *cl;
	gchar buf[100];

	cl = (rfbClient *)gpdata->client;
	while ((event = remmina_plugin_vnc_event_queue_pop_head(gpdata)) != NULL) {
		if (cl) {
			switch (event->event_type) {
			case REMMINA_PLUGIN_VNC_EVENT_KEY:
				SendKeyEvent(cl, event->event_data.key.keyval, event->event_data.key.pressed);
				break;
			case REMMINA_PLUGIN_VNC_EVENT_POINTER:
				SendPointerEvent(cl, event->event_data.pointer.x, event->event_data.pointer.y,
						event->event_data.pointer.button_mask);
				break;
			case REMMINA_PLUGIN_VNC_EVENT_CUTTEXT:
				if (event->event_data.text.text) {
					rfbClientLog("sending clipboard text '%s'\n", event->event_data.text.text);
					SendClientCutText(cl, event->event_data.text.text, strlen(event->event_data.text.text));
				}
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
			default:
				rfbClientLog("Ignoring VNC event: 0x%x\n", event->event_type);
				break;
			}
		}
		remmina_plugin_vnc_event_free(event);
	}
	if (read(gpdata->vnc_event_pipe[0], buf, sizeof(buf))) {
		/* Ignore */
	}
}

typedef struct _RemminaPluginVncCuttextParam {
	RemminaProtocolWidget * gp;
	gchar *			text;
	gint			textlen;
} RemminaPluginVncCuttextParam;

static void remmina_plugin_vnc_update_quality(rfbClient *cl, gint quality)
{
	TRACE_CALL(__func__);
	/**
	 * "0", "Poor (fastest)
	 * "1", "Medium"
	 * "2", "Good"
	 * "9", "Best
	 */
	switch (quality) {
	case 9:
		cl->appData.useBGR233 = 0;
		cl->appData.encodingsString = "copyrect zlib hextile raw";
		cl->appData.compressLevel = 1;
		cl->appData.qualityLevel = 9;
		break;
	case 2:
		cl->appData.useBGR233 = 0;
		cl->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
		cl->appData.compressLevel = 2;
		cl->appData.qualityLevel = 7;
		break;
	case 1:
		cl->appData.useBGR233 = 0;
		cl->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
		cl->appData.compressLevel = 3;
		cl->appData.qualityLevel = 5;
		break;
	case 0:
	default:
		cl->appData.useBGR233 = 1;
		cl->appData.encodingsString = "tight zrle ultra copyrect hextile zlib corre rre raw";
		cl->appData.compressLevel = 9;
		cl->appData.qualityLevel = 1;
		break;
	}
}

static void remmina_plugin_vnc_update_colordepth(rfbClient *cl, gint colordepth)
{
	TRACE_CALL(__func__);

	cl->format.depth = colordepth;
	cl->appData.requestedDepth = colordepth;

	cl->format.trueColour = 1;
	cl->format.bigEndian = check_for_endianness()?FALSE:TRUE;

	switch (colordepth) {
	case 8:
		cl->format.depth = 8;
		cl->format.bitsPerPixel = 8;
		cl->format.blueMax = 3;
		cl->format.blueShift = 6;
		cl->format.greenMax = 7;
		cl->format.greenShift = 3;
		cl->format.redMax = 7;
		cl->format.redShift = 0;
		break;
	case 16:
		cl->format.depth = 15;
		cl->format.bitsPerPixel = 16;
		cl->format.redShift = 11;
		cl->format.greenShift = 6;
		cl->format.blueShift = 1;
		cl->format.redMax = 31;
		cl->format.greenMax = 31;
		cl->format.blueMax = 31;
		break;
	case 32:
	default:
		cl->format.depth = 24;
		cl->format.bitsPerPixel = 32;
		cl->format.blueShift = 0;
		cl->format.redShift = 16;
		cl->format.greenShift = 8;
		cl->format.blueMax = 0xff;
		cl->format.redMax = 0xff;
		cl->format.greenMax = 0xff;
		break;
	}

	rfbClientLog ("colordepth          = %d\n", colordepth);
	rfbClientLog ("format.depth        = %d\n", cl->format.depth);
	rfbClientLog ("format.bitsPerPixel = %d\n", cl->format.bitsPerPixel);
	rfbClientLog ("format.blueShift    = %d\n", cl->format.blueShift);
	rfbClientLog ("format.redShift     = %d\n", cl->format.redShift);
	rfbClientLog ("format.trueColour   = %d\n", cl->format.trueColour);
	rfbClientLog ("format.greenShift   = %d\n", cl->format.greenShift);
	rfbClientLog ("format.blueMax      = %d\n", cl->format.blueMax);
	rfbClientLog ("format.redMax       = %d\n", cl->format.redMax);
	rfbClientLog ("format.greenMax     = %d\n", cl->format.greenMax);
	rfbClientLog ("format.bigEndian    = %d\n", cl->format.bigEndian);
}

static rfbBool remmina_plugin_vnc_rfb_allocfb(rfbClient *cl)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = rfbClientGetClientData(cl, NULL);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	gint width, height, depth, size;
	gboolean scale;
	cairo_surface_t *new_surface, *old_surface;

	width = cl->width;
	height = cl->height;
	depth = cl->format.bitsPerPixel;
	size = width * height * (depth / 8);

	new_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	if (cairo_surface_status(new_surface) != CAIRO_STATUS_SUCCESS)
		return FALSE;
	old_surface = gpdata->rgb_buffer;

	LOCK_BUFFER(TRUE);

	remmina_plugin_service->protocol_plugin_set_width(gp, width);
	remmina_plugin_service->protocol_plugin_set_height(gp, height);

	gpdata->rgb_buffer = new_surface;

	if (gpdata->vnc_buffer)
		g_free(gpdata->vnc_buffer);
	gpdata->vnc_buffer = (guchar *)g_malloc(size);
	cl->frameBuffer = gpdata->vnc_buffer;

	UNLOCK_BUFFER(TRUE);

	if (old_surface)
		cairo_surface_destroy(old_surface);

	scale = (remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp) != REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE);
	remmina_plugin_vnc_update_scale(gp, scale);

	/* Notify window of change so that scroll border can be hidden or shown if needed */
	remmina_plugin_service->protocol_plugin_desktop_resize(gp);

	/* Refresh the client’s updateRect - bug in xvncclient */
	cl->updateRect.w = width;
	cl->updateRect.h = height;

	return TRUE;
}

static gint remmina_plugin_vnc_bits(gint n)
{
	TRACE_CALL(__func__);
	gint b = 0;
	while (n) {
		b++;
		n >>= 1;
	}
	return b ? b : 1;
}

static gboolean remmina_plugin_vnc_queue_draw_area_real(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	gint x, y, w, h;

	if (GTK_IS_WIDGET(gp) && gpdata->connected) {
		LOCK_BUFFER(FALSE);
		x = gpdata->queuedraw_x;
		y = gpdata->queuedraw_y;
		w = gpdata->queuedraw_w;
		h = gpdata->queuedraw_h;
		gpdata->queuedraw_handler = 0;
		UNLOCK_BUFFER(FALSE);

		gtk_widget_queue_draw_area(GTK_WIDGET(gp), x, y, w, h);
	}
	return FALSE;
}

static void remmina_plugin_vnc_queue_draw_area(RemminaProtocolWidget *gp, gint x, gint y, gint w, gint h)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	gint nx2, ny2, ox2, oy2;

	LOCK_BUFFER(TRUE);
	if (gpdata->queuedraw_handler) {
		nx2 = x + w;
		ny2 = y + h;
		ox2 = gpdata->queuedraw_x + gpdata->queuedraw_w;
		oy2 = gpdata->queuedraw_y + gpdata->queuedraw_h;
		gpdata->queuedraw_x = MIN(gpdata->queuedraw_x, x);
		gpdata->queuedraw_y = MIN(gpdata->queuedraw_y, y);
		gpdata->queuedraw_w = MAX(ox2, nx2) - gpdata->queuedraw_x;
		gpdata->queuedraw_h = MAX(oy2, ny2) - gpdata->queuedraw_y;
	} else {
		gpdata->queuedraw_x = x;
		gpdata->queuedraw_y = y;
		gpdata->queuedraw_w = w;
		gpdata->queuedraw_h = h;
		gpdata->queuedraw_handler = IDLE_ADD((GSourceFunc)remmina_plugin_vnc_queue_draw_area_real, gp);
	}
	UNLOCK_BUFFER(TRUE);
}

static void remmina_plugin_vnc_rfb_fill_buffer(rfbClient *cl, guchar *dest, gint dest_rowstride, guchar *src,
					       gint src_rowstride, guchar *mask, gint w, gint h)
{
	TRACE_CALL(__func__);
	guchar *srcptr;
	gint bytesPerPixel;
	guint32 src_pixel;
	gint ix, iy;
	gint i;
	guchar c;
	gint rs, gs, bs, rm, gm, bm, rl, gl, bl, rr, gr, br;
	gint r;
	guint32 *destptr;
	union {
		struct {
			guchar a, r, g, b;
		} colors;
		guint32 argb;
	} dst_pixel;

	bytesPerPixel = cl->format.bitsPerPixel / 8;
	switch (cl->format.bitsPerPixel) {
	case 32:
		/* The following codes fill in the Alpha channel swap red/green value */
		for (iy = 0; iy < h; iy++) {
			destptr = (guint32 *)(dest + iy * dest_rowstride);
			srcptr = src + iy * src_rowstride;
			for (ix = 0; ix < w; ix++) {
				if (!mask || *mask++) {
					dst_pixel.colors.a = 0xff;
					dst_pixel.colors.r = *(srcptr + 2);
					dst_pixel.colors.g = *(srcptr + 1);
					dst_pixel.colors.b = *srcptr;
					*destptr++ = ntohl(dst_pixel.argb);
				} else {
					*destptr++ = 0;
				}
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
		for (iy = 0; iy < h; iy++) {
			destptr = (guint32 *)(dest + iy * dest_rowstride);
			srcptr = src + iy * src_rowstride;
			for (ix = 0; ix < w; ix++) {
				src_pixel = 0;
				for (i = 0; i < bytesPerPixel; i++)
					src_pixel += (*srcptr++) << (8 * i);

				if (!mask || *mask++) {
					dst_pixel.colors.a = 0xff;
					c = (guchar)((src_pixel >> rs) & rm) << rl;
					for (r = rr; r < 8; r *= 2)
						c |= c >> r;
					dst_pixel.colors.r = c;
					c = (guchar)((src_pixel >> gs) & gm) << gl;
					for (r = gr; r < 8; r *= 2)
						c |= c >> r;
					dst_pixel.colors.g = c;
					c = (guchar)((src_pixel >> bs) & bm) << bl;
					for (r = br; r < 8; r *= 2)
						c |= c >> r;
					dst_pixel.colors.b = c;
					*destptr++ = ntohl(dst_pixel.argb);
				} else {
					*destptr++ = 0;
				}
			}
		}
		break;
	}
}

static void remmina_plugin_vnc_rfb_updatefb(rfbClient *cl, int x, int y, int w, int h)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = rfbClientGetClientData(cl, NULL);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	gint bytesPerPixel;
	gint rowstride;
	gint width;

	LOCK_BUFFER(TRUE);

	if (w >= 1 || h >= 1) {
		width = remmina_plugin_service->protocol_plugin_get_width(gp);
		bytesPerPixel = cl->format.bitsPerPixel / 8;
		rowstride = cairo_image_surface_get_stride(gpdata->rgb_buffer);
		cairo_surface_flush(gpdata->rgb_buffer);
		remmina_plugin_vnc_rfb_fill_buffer(cl, cairo_image_surface_get_data(gpdata->rgb_buffer) + y * rowstride + x * 4,
						   rowstride, gpdata->vnc_buffer + ((y * width + x) * bytesPerPixel), width * bytesPerPixel, NULL,
						   w, h);
		cairo_surface_mark_dirty(gpdata->rgb_buffer);
	}

	if ((remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp) != REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE))
		remmina_plugin_vnc_scale_area(gp, &x, &y, &w, &h);

	UNLOCK_BUFFER(TRUE);

	remmina_plugin_vnc_queue_draw_area(gp, x, y, w, h);
}

static gboolean remmina_plugin_vnc_queue_cuttext(RemminaPluginVncCuttextParam *param)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = param->gp;
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	GDateTime *t;
	glong diff;
	const char *cur_charset;
	gchar *text;
	gsize br, bw;

	if (GTK_IS_WIDGET(gp) && gpdata->connected) {
		t = g_date_time_new_now_utc();
		diff = g_date_time_difference(t, gpdata->clipboard_timer) / 100000; // tenth of second
		if (diff >= 10) {
			g_date_time_unref(gpdata->clipboard_timer);
			gpdata->clipboard_timer = t;
			/* Convert text from VNC latin-1 to current GTK charset (usually UTF-8) */
			g_get_charset(&cur_charset);
			text = g_convert_with_fallback(param->text, param->textlen, cur_charset, "ISO-8859-1", "?", &br, &bw, NULL);
			gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD), text, bw);
			g_free(text);
		} else
			g_date_time_unref(t);
	}
	g_free(param->text);
	g_free(param);
	return FALSE;
}

static void remmina_plugin_vnc_rfb_cuttext(rfbClient *cl, const char *text, int textlen)
{
	TRACE_CALL(__func__);
	RemminaPluginVncCuttextParam *param;

	param = g_new(RemminaPluginVncCuttextParam, 1);
	param->gp = (RemminaProtocolWidget *)rfbClientGetClientData(cl, NULL);
	param->text = g_malloc(textlen);
	memcpy(param->text, text, textlen);
	param->textlen = textlen;
	IDLE_ADD((GSourceFunc)remmina_plugin_vnc_queue_cuttext, param);
}

static char *
remmina_plugin_vnc_rfb_password(rfbClient *cl)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = rfbClientGetClientData(cl, NULL);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	gint ret;
	gchar *pwd = NULL;
	gboolean disablepasswordstoring;

	gpdata->auth_called = TRUE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (gpdata->auth_first)
		pwd = g_strdup(remmina_plugin_service->file_get_string(remminafile, "password"));
	if (!pwd) {
		gboolean save;
		disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);
		ret = remmina_plugin_service->protocol_plugin_init_auth(gp,
			(disablepasswordstoring ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD),
			_("Enter VNC password"),
			NULL,
			remmina_plugin_service->file_get_string(remminafile, "password"),
			NULL,
			NULL);
		if (ret != GTK_RESPONSE_OK) {
			gpdata->connected = FALSE;
			return NULL;
		}
		pwd = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save)
			remmina_plugin_service->file_set_string(remminafile, "password", pwd);
		else
			remmina_plugin_service->file_set_string(remminafile, "password", NULL);
	}
	return pwd;
}

static rfbCredential *
remmina_plugin_vnc_rfb_credential(rfbClient *cl, int credentialType)
{
	TRACE_CALL(__func__);
	rfbCredential *cred;
	RemminaProtocolWidget *gp = rfbClientGetClientData(cl, NULL);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	gint ret;
	gchar *s1, *s2;
	gboolean disablepasswordstoring;

	gpdata->auth_called = TRUE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	cred = g_new0(rfbCredential, 1);

	switch (credentialType) {
	case rfbCredentialTypeUser:

		s1 = g_strdup(remmina_plugin_service->file_get_string(remminafile, "username"));

		s2 = g_strdup(remmina_plugin_service->file_get_string(remminafile, "password"));

		if (gpdata->auth_first && s1 && s2) {
			cred->userCredential.username = s1;
			cred->userCredential.password = s2;
		} else {
			g_free(s1);
			g_free(s2);

			disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);
			ret = remmina_plugin_service->protocol_plugin_init_auth(gp,
				(disablepasswordstoring ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD) | REMMINA_MESSAGE_PANEL_FLAG_USERNAME,
				_("Enter VNC authentication credentials"),
				remmina_plugin_service->file_get_string(remminafile, "username"),
				remmina_plugin_service->file_get_string(remminafile, "password"),
				NULL,
				NULL);
			if (ret == GTK_RESPONSE_OK) {
				gboolean save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
				cred->userCredential.username = remmina_plugin_service->protocol_plugin_init_get_username(gp);
				cred->userCredential.password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
				if (save) {
					remmina_plugin_service->file_set_string(remminafile, "username", cred->userCredential.username);
					remmina_plugin_service->file_set_string(remminafile, "password", cred->userCredential.password);
				} else {
					remmina_plugin_service->file_set_string(remminafile, "username", NULL);
					remmina_plugin_service->file_set_string(remminafile, "password", NULL);
				}
			} else {
				g_free(cred);
				cred = NULL;
				gpdata->connected = FALSE;
			}
		}
		break;

	case rfbCredentialTypeX509:
		if (gpdata->auth_first &&
		    remmina_plugin_service->file_get_string(remminafile, "cacert")) {
			cred->x509Credential.x509CACertFile = g_strdup(remmina_plugin_service->file_get_string(remminafile, "cacert"));
			cred->x509Credential.x509CACrlFile = g_strdup(remmina_plugin_service->file_get_string(remminafile, "cacrl"));
			cred->x509Credential.x509ClientCertFile = g_strdup(remmina_plugin_service->file_get_string(remminafile, "clientcert"));
			cred->x509Credential.x509ClientKeyFile = g_strdup(remmina_plugin_service->file_get_string(remminafile, "clientkey"));
		} else {
			ret = remmina_plugin_service->protocol_plugin_init_authx509(gp);

			if (ret == GTK_RESPONSE_OK) {
				cred->x509Credential.x509CACertFile = remmina_plugin_service->protocol_plugin_init_get_cacert(gp);
				cred->x509Credential.x509CACrlFile = remmina_plugin_service->protocol_plugin_init_get_cacrl(gp);
				cred->x509Credential.x509ClientCertFile = remmina_plugin_service->protocol_plugin_init_get_clientcert(gp);
				cred->x509Credential.x509ClientKeyFile = remmina_plugin_service->protocol_plugin_init_get_clientkey(gp);
			} else {
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

static void remmina_plugin_vnc_rfb_cursor_shape(rfbClient *cl, int xhot, int yhot, int width, int height, int bytesPerPixel)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = rfbClientGetClientData(cl, NULL);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	int stride;
	guchar *data;
	cairo_surface_t *surface;

	if (!gtk_widget_get_window(GTK_WIDGET(gp)))
		return;

	if (width && height) {
		stride = cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, width);
		data = g_malloc(stride * height);
		remmina_plugin_vnc_rfb_fill_buffer(cl, data, stride, cl->rcSource,
						   width * cl->format.bitsPerPixel / 8, cl->rcMask, width, height);
		surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, width, height, stride);
		if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
			g_free(data);
			return;
		}
		if (cairo_surface_set_user_data(surface, NULL, NULL, g_free) != CAIRO_STATUS_SUCCESS) {
			g_free(data);
			return;
		}

		LOCK_BUFFER(TRUE);
		remmina_plugin_vnc_queuecursor(gp, surface, xhot, yhot);
		UNLOCK_BUFFER(TRUE);
	}
}

static void remmina_plugin_vnc_rfb_bell(rfbClient *cl)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp;
	GdkWindow *window;

	gp = (RemminaProtocolWidget *)(rfbClientGetClientData(cl, NULL));
	window = gtk_widget_get_window(GTK_WIDGET(gp));

	if (window)
		gdk_window_beep(window);
}

/* Translate known VNC messages. It’s for intltool only, not for gcc */
#ifdef __DO_NOT_COMPILE_ME__
N_("Unable to connect to VNC server")
N_("Couldn’t convert '%s' to host address")
N_("VNC connection failed: %s")
N_("Your connection has been rejected.")
#endif
/** @todo We only store the last message at this moment. */
#define MAX_ERROR_LENGTH 1000
static gchar vnc_error[MAX_ERROR_LENGTH + 1];
static gboolean vnc_encryption_disable_requested;

static void remmina_plugin_vnc_rfb_output(const char *format, ...)
{
	TRACE_CALL(__func__);
	va_list args;
	va_start(args, format);
	gchar *f, *p, *ff;

	if(!rfbEnableClientLogging)
		return;
	/* eliminate the last \n */
	f = g_strdup(format);
	if (f[strlen(f) - 1] == '\n') f[strlen(f) - 1] = '\0';

	if (g_strcmp0(f, "VNC connection failed: %s") == 0) {
		p = va_arg(args, gchar *);
		g_snprintf(vnc_error, MAX_ERROR_LENGTH, _(f), _(p));
	} else if (g_strcmp0(f, "The VNC server requested an unknown authentication method. %s") == 0) {
		p = va_arg(args, gchar *);
		if (vnc_encryption_disable_requested) {
			ff = g_strconcat(_("The VNC server requested an unknown authentication method. %s"),
					 ". ",
					 _("Please retry after turning on encryption for this profile."),
					 NULL);
			g_snprintf(vnc_error, MAX_ERROR_LENGTH, ff, p);
			g_free(ff);
		} else
			g_snprintf(vnc_error, MAX_ERROR_LENGTH, _(f), p);
	} else
		g_vsnprintf(vnc_error, MAX_ERROR_LENGTH, _(f), args);
	g_free(f);
	va_end(args);

	REMMINA_PLUGIN_DEBUG("VNC returned: %s", vnc_error);
}

static void remmina_plugin_vnc_chat_on_send(RemminaProtocolWidget *gp, const gchar *text)
{
	TRACE_CALL(__func__);
	gchar *ptr;

	/* Need to add a line-feed for UltraVNC */
	ptr = g_strdup_printf("%s\n", text);
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_CHAT_SEND, ptr, NULL, NULL);
	g_free(ptr);
}

static void remmina_plugin_vnc_chat_on_destroy(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_CHAT_CLOSE, NULL, NULL, NULL);
}

/* Send CTRL+ALT+DEL keys keystrokes to the plugin drawing_area widget */
static void remmina_plugin_vnc_send_ctrlaltdel(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	guint keys[] = { GDK_KEY_Control_L, GDK_KEY_Alt_L, GDK_KEY_Delete };
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);

	remmina_plugin_service->protocol_plugin_send_keys_signals(gpdata->drawing_area,
								  keys, G_N_ELEMENTS(keys), GDK_KEY_PRESS | GDK_KEY_RELEASE);
}

static gboolean remmina_plugin_vnc_close_chat(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	remmina_plugin_service->protocol_plugin_chat_close(gp);
	return FALSE;
}

static gboolean remmina_plugin_vnc_open_chat(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	rfbClient *cl;

	cl = (rfbClient *)gpdata->client;

	remmina_plugin_service->protocol_plugin_chat_open(gp, cl->desktopName, remmina_plugin_vnc_chat_on_send,
							  remmina_plugin_vnc_chat_on_destroy);
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_CHAT_OPEN, NULL, NULL, NULL);
	return FALSE;
}

static void remmina_plugin_vnc_rfb_chat(rfbClient *cl, int value, char *text)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp;

	gp = (RemminaProtocolWidget *)(rfbClientGetClientData(cl, NULL));
	switch (value) {
	case rfbTextChatOpen:
		IDLE_ADD((GSourceFunc)remmina_plugin_vnc_open_chat, gp);
		break;
	case rfbTextChatClose:
		/* Do nothing… but wait for the next rfbTextChatFinished signal */
		break;
	case rfbTextChatFinished:
		IDLE_ADD((GSourceFunc)remmina_plugin_vnc_close_chat, gp);
		break;
	default:
		/* value is the text length */
		remmina_plugin_service->protocol_plugin_chat_receive(gp, text);
		break;
	}
}

static gboolean remmina_plugin_vnc_incoming_connection(RemminaProtocolWidget *gp, rfbClient *cl)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	fd_set fds;

	/**
	 * @fixme This may fail or not working as expected with multiple network interfaces,
	 * change with ListenAtTcpPortAndAddress
	 */
	gpdata->listen_sock = ListenAtTcpPort(cl->listenPort);
	if (gpdata->listen_sock < 0)
		return FALSE;

	remmina_plugin_service->protocol_plugin_init_show_listen(gp, cl->listenPort);

	remmina_plugin_service->protocol_plugin_start_reverse_tunnel(gp, cl->listenPort);

	FD_ZERO(&fds);
	if (gpdata->listen_sock >= 0)
		FD_SET(gpdata->listen_sock, &fds);

	select(gpdata->listen_sock + 1, &fds, NULL, NULL, NULL);

	if (!FD_ISSET(gpdata->listen_sock, &fds)) {
		close(gpdata->listen_sock);
		gpdata->listen_sock = -1;
		return FALSE;
	}

	if (FD_ISSET(gpdata->listen_sock, &fds))
		cl->sock = AcceptTcpConnection(gpdata->listen_sock);
	if (cl->sock >= 0) {
		close(gpdata->listen_sock);
		gpdata->listen_sock = -1;
	}
	if (cl->sock < 0 || !SetNonBlocking(cl->sock))
		return FALSE;

	return TRUE;
}


static gboolean remmina_plugin_vnc_main_loop(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	gint ret;
	gint i;
	rfbClient *cl;
	fd_set fds;
	struct timeval timeout;

	if (!gpdata->connected) {
		gpdata->running = FALSE;
		return FALSE;
	}

	cl = (rfbClient *)gpdata->client;

	/*
	 * Do not explicitly wait while data is on the buffer, see:
	 * - https://jira.glyptodon.com/browse/GUAC-1056
	 * - https://jira.glyptodon.com/browse/GUAC-1056?focusedCommentId=14348&page=com.atlassian.jira.plugin.system.issuetabpanels:comment-tabpanel#comment-14348
	 * - https://github.com/apache/guacamole-server/blob/67680bd2d51e7949453f0f7ffc7f4234a1136715/src/protocols/vnc/vnc.c#L155
	 */
	if (cl->buffered)
		goto handle_buffered;

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
		remmina_plugin_vnc_process_vnc_event(gp);
	if (FD_ISSET(cl->sock, &fds)) {
		i = WaitForMessage(cl, 500);
		if (i < 0)
			return TRUE;
handle_buffered:
		if (!HandleRFBServerMessage(cl)) {
			gpdata->running = FALSE;
			if (gpdata->connected && !remmina_plugin_service->protocol_plugin_is_closed(gp))
				remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
			return FALSE;
		}
	}

	return TRUE;
}

static gboolean remmina_plugin_vnc_main(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	rfbClient *cl = NULL;
	gchar *host;
	gchar *s = NULL;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	gpdata->running = TRUE;

	rfbClientLog = rfbClientErr = remmina_plugin_vnc_rfb_output;

	gint colordepth = remmina_plugin_service->file_get_int(remminafile, "colordepth", 32);
	gint quality = remmina_plugin_service->file_get_int(remminafile, "quality", 9);

	while (gpdata->connected) {
		gpdata->auth_called = FALSE;

		host = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, 5900, TRUE);

		if (host == NULL) {
			gpdata->connected = FALSE;
			break;
		}

		/* int bitsPerSample,int samplesPerPixel, int bytesPerPixel */
		switch (colordepth) {
		case 8:
			cl = rfbGetClient(2, 3, 1);
			break;
		case 15:
		case 16:
			cl = rfbGetClient(5, 3, 2);
			break;
		case 24:
			cl = rfbGetClient(6, 3, 3);
			break;
		case 32:
		default:
			cl = rfbGetClient(8, 3, 4);
			break;
		}
		cl->MallocFrameBuffer = remmina_plugin_vnc_rfb_allocfb;
		cl->canHandleNewFBSize = TRUE;
		cl->GetPassword = remmina_plugin_vnc_rfb_password;
		cl->GetCredential = remmina_plugin_vnc_rfb_credential;
		cl->GotFrameBufferUpdate = remmina_plugin_vnc_rfb_updatefb;
		cl->GotXCutText = (
			remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE) ?
			NULL : remmina_plugin_vnc_rfb_cuttext);
		cl->GotCursorShape = remmina_plugin_vnc_rfb_cursor_shape;
		cl->Bell = remmina_plugin_vnc_rfb_bell;
		cl->HandleTextChat = remmina_plugin_vnc_rfb_chat;
		rfbClientSetClientData(cl, NULL, gp);

		if (host[0] == '\0') {
			cl->serverHost = g_strdup(host);
			cl->listenSpecified = TRUE;
			if (remmina_plugin_service->file_get_int(remminafile, "ssh_tunnel_enabled", FALSE))
				/* When we use reverse tunnel, the local port does not really matter.
				 * Hardcode a default port just in case the remote port is customized
				 * to a privilege port then we will have problem listening. */
				cl->listenPort = 5500;
			else
				cl->listenPort = remmina_plugin_service->file_get_int(remminafile, "listenport", 5500);

			remmina_plugin_vnc_incoming_connection(gp, cl);
		} else {
			remmina_plugin_service->get_server_port(host, 5900, &s, &cl->serverPort);
			cl->serverHost = g_strdup(s);
			g_free(s);

			/* Support short-form (:0, :1) */
			if (cl->serverPort < 100)
				cl->serverPort += 5900;
		}
		g_free(host);
		host = NULL;

		if (remmina_plugin_service->file_get_string(remminafile, "proxy")) {
			cl->destHost = cl->serverHost;
			cl->destPort = cl->serverPort;
			remmina_plugin_service->get_server_port(remmina_plugin_service->file_get_string(remminafile, "proxy"), 5900,
								&s, &cl->serverPort);
			cl->serverHost = g_strdup(s);
			g_free(s);
		}

		cl->appData.useRemoteCursor = (
			remmina_plugin_service->file_get_int(remminafile, "showcursor", FALSE) ? FALSE : TRUE);

		remmina_plugin_vnc_update_quality(cl, quality);
		remmina_plugin_vnc_update_colordepth(cl, colordepth);
		if ((cl->format.depth == 8) && (quality == 9))
			cl->appData.encodingsString = "copyrect zlib hextile raw";
		else if ((cl->format.depth == 8) && (quality == 2))
			cl->appData.encodingsString = "zrle ultra copyrect hextile zlib corre rre raw";
		else if ((cl->format.depth == 8) && (quality == 1))
			cl->appData.encodingsString = "zrle ultra copyrect hextile zlib corre rre raw";
		else if ((cl->format.depth == 8) && (quality == 0))
			cl->appData.encodingsString = "zrle ultra copyrect hextile zlib corre rre raw";
		SetFormatAndEncodings(cl);

		if (remmina_plugin_service->file_get_int(remminafile, "disableencryption", FALSE)) {
			vnc_encryption_disable_requested = TRUE;
			SetClientAuthSchemes(cl, remmina_plugin_vnc_no_encrypt_auth_types, -1);
		} else {
			vnc_encryption_disable_requested = FALSE;
		}

		if (rfbInitClient(cl, NULL, NULL))
			break;

		/* If the authentication is not called, it has to be a fatel error and must quit */
		if (!gpdata->auth_called) {
			gpdata->connected = FALSE;
			break;
		}

		/* vnc4server reports "already in use" after authentication. Workaround here */
		if (strstr(vnc_error, "The server is already in use")) {
			gpdata->connected = FALSE;
			gpdata->auth_called = FALSE;
			break;
		}

		/* Otherwise, it’s a password error. Try to clear saved password if any */
		remmina_plugin_service->file_set_string(remminafile, "password", NULL);

		if (!gpdata->connected)
			break;

		remmina_plugin_service->protocol_plugin_init_show_retry(gp);

		/* It’s safer to sleep a while before reconnect */
		sleep(2);

		gpdata->auth_first = FALSE;
	}

	if (!gpdata->connected) {
		if (cl && !gpdata->auth_called && !(remmina_plugin_service->protocol_plugin_has_error(gp)))
			remmina_plugin_service->protocol_plugin_set_error(gp, "%s", vnc_error);
		gpdata->running = FALSE;

		remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);

		return FALSE;
	}

	remmina_plugin_service->protocol_plugin_init_save_cred(gp);

	gpdata->client = cl;

	remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);

	if (remmina_plugin_service->file_get_int(remminafile, "disableserverinput", FALSE))
		PermitServerInput(cl, 1);

	if (gpdata->thread) {
		while (remmina_plugin_vnc_main_loop(gp)) {
		}
		gpdata->running = FALSE;
	} else {
		IDLE_ADD((GSourceFunc)remmina_plugin_vnc_main_loop, gp);
	}

	return FALSE;
}


static gpointer
remmina_plugin_vnc_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	CANCEL_ASYNC
	remmina_plugin_vnc_main((RemminaProtocolWidget *)data);
	return NULL;
}


static RemminaPluginVncCoordinates remmina_plugin_vnc_scale_coordinates(GtkWidget *widget, RemminaProtocolWidget *gp, gint x, gint y)
{
	GtkAllocation widget_allocation;
	RemminaPluginVncCoordinates result;

	if ((remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp) != REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE)) {
		gtk_widget_get_allocation(widget, &widget_allocation);
		result.x = x * remmina_plugin_service->protocol_plugin_get_width(gp) / widget_allocation.width;
		result.y = y * remmina_plugin_service->protocol_plugin_get_height(gp) / widget_allocation.height;
	} else {
		result.x = x;
		result.y = y;
	}

	return result;
}

static gboolean remmina_plugin_vnc_on_motion(GtkWidget *widget, GdkEventMotion *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	RemminaPluginVncCoordinates coordinates;

	if (!gpdata->connected || !gpdata->client)
		return FALSE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return FALSE;

	coordinates = remmina_plugin_vnc_scale_coordinates(widget, gp, event->x, event->y);
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_POINTER, GINT_TO_POINTER(coordinates.x), GINT_TO_POINTER(coordinates.y),
				      GINT_TO_POINTER(gpdata->button_mask));
	return TRUE;
}

static gboolean remmina_plugin_vnc_on_button(GtkWidget *widget, GdkEventButton *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	RemminaPluginVncCoordinates coordinates;
	gint mask;

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
			       (gpdata->button_mask & (0xff - mask)));

	coordinates = remmina_plugin_vnc_scale_coordinates(widget, gp, event->x, event->y);
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_POINTER, GINT_TO_POINTER(coordinates.x), GINT_TO_POINTER(coordinates.y),
				      GINT_TO_POINTER(gpdata->button_mask));
	return TRUE;
}

static gint delta_to_mask(float delta, float *accum, gint mask_plus, gint mask_minus)
{
	*accum += delta;
	if (*accum >= 1.0) {
		*accum = 0.0;
		return mask_plus;
	} else if (*accum <= -1.0) {
		*accum = 0.0;
		return mask_minus;
	}
	return 0;
}

static gboolean remmina_plugin_vnc_on_scroll(GtkWidget *widget, GdkEventScroll *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	RemminaPluginVncCoordinates coordinates;
	gint mask;

	if (!gpdata->connected || !gpdata->client)
		return FALSE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return FALSE;

	switch (event->direction) {
	case GDK_SCROLL_UP:
		mask = (1 << 3);
		gpdata->scroll_y_accumulator = 0;
		break;
	case GDK_SCROLL_DOWN:
		mask = (1 << 4);
		gpdata->scroll_y_accumulator = 0;
		break;
	case GDK_SCROLL_LEFT:
		mask = (1 << 5);
		gpdata->scroll_x_accumulator = 0;
		break;
	case GDK_SCROLL_RIGHT:
		mask = (1 << 6);
		gpdata->scroll_x_accumulator = 0;
		break;
#if GTK_CHECK_VERSION(3, 4, 0)
	case GDK_SCROLL_SMOOTH:
		/* RFB does not seems to support SMOOTH scroll, so we accumulate GTK delta requested
		 * up to 1.0 and then send a normal RFB wheel scroll when the accumulator reaches 1.0 */
		mask = delta_to_mask(event->delta_y, &(gpdata->scroll_y_accumulator), (1 << 4), (1 << 3));
		mask |= delta_to_mask(event->delta_x, &(gpdata->scroll_x_accumulator), (1 << 6), (1 << 5));
		if (!mask)
			return FALSE;
		break;
#endif
	default:
		return FALSE;
	}

	coordinates = remmina_plugin_vnc_scale_coordinates(widget, gp, event->x, event->y);
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_POINTER, GINT_TO_POINTER(coordinates.x), GINT_TO_POINTER(coordinates.y),
				      GINT_TO_POINTER(mask | gpdata->button_mask));
	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_POINTER, GINT_TO_POINTER(coordinates.x), GINT_TO_POINTER(coordinates.y),
				      GINT_TO_POINTER(gpdata->button_mask));

	return TRUE;
}

static void remmina_plugin_vnc_release_key(RemminaProtocolWidget *gp, guint16 keycode)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaKeyVal *k;
	gint i;

	if (!gpdata)
		return;

	if (keycode == 0) {
		/* Send all release key events for previously pressed keys */
		for (i = 0; i < gpdata->pressed_keys->len; i++) {
			k = g_ptr_array_index(gpdata->pressed_keys, i);
			remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_KEY, GUINT_TO_POINTER(k->keyval),
						      GINT_TO_POINTER(FALSE), NULL);
			g_free(k);
		}
		g_ptr_array_set_size(gpdata->pressed_keys, 0);
	} else {
		/* Unregister the keycode only */
		for (i = 0; i < gpdata->pressed_keys->len; i++) {
			k = g_ptr_array_index(gpdata->pressed_keys, i);
			if (k->keycode == keycode) {
				g_free(k);
				g_ptr_array_remove_index_fast(gpdata->pressed_keys, i);
				break;
			}
		}
	}
}

static gboolean remmina_plugin_vnc_on_key(GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	RemminaKeyVal *k;
	guint event_keyval;
	guint keyval;
	int i;

	if (!gpdata->connected || !gpdata->client)
		return FALSE;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return FALSE;

	gpdata->scroll_x_accumulator = 0;
	gpdata->scroll_y_accumulator = 0;

	/* When sending key release, try first to find out a previously sent keyval
	 * to workaround bugs like https://bugs.freedesktop.org/show_bug.cgi?id=7430 */

	event_keyval = event->keyval;
	if (event->type == GDK_KEY_RELEASE) {
		for (i = 0; i < gpdata->pressed_keys->len; i++) {
			k = g_ptr_array_index(gpdata->pressed_keys, i);
			if (k->keycode == event->hardware_keycode) {
				event_keyval = k->keyval;
				break;
			}
		}
	}

	keyval = remmina_plugin_service->pref_keymap_get_keyval(remmina_plugin_service->file_get_string(remminafile, "keymap"),
								event_keyval);

	remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_KEY, GUINT_TO_POINTER(keyval),
				      GINT_TO_POINTER(event->type == GDK_KEY_PRESS ? TRUE : FALSE), NULL);

	/* Register/unregister the pressed key */
	if (event->type == GDK_KEY_PRESS) {
		k = g_new(RemminaKeyVal, 1);
		k->keyval = keyval;
		k->keycode = event->hardware_keycode;
		g_ptr_array_add(gpdata->pressed_keys, k);
	} else {
		remmina_plugin_vnc_release_key(gp, event->hardware_keycode);
	}
	return TRUE;
}

static void remmina_plugin_vnc_on_cuttext_request(GtkClipboard *clipboard, const gchar *text, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	GDateTime *t;
	glong diff;
	gsize br, bw;
	gchar *latin1_text;
	const char *cur_charset;

	if (text) {
		/* A timer (1 second) to avoid clipboard "loopback": text cut out from VNC won’t paste back into VNC */
		t = g_date_time_new_now_utc();
		diff = g_date_time_difference(t, gpdata->clipboard_timer) / 100000; // tenth of second
		if (diff < 10)
			return;
		g_date_time_unref(gpdata->clipboard_timer);
		gpdata->clipboard_timer = t;
		/* Convert text from current charset to latin-1 before sending to remote server.
		 * See RFC6143 7.5.6 */
		g_get_charset(&cur_charset);
		latin1_text = g_convert_with_fallback(text, -1, "ISO-8859-1", cur_charset, "?", &br, &bw, NULL);
		remmina_plugin_vnc_event_push(gp, REMMINA_PLUGIN_VNC_EVENT_CUTTEXT, (gpointer)latin1_text, NULL, NULL);
		g_free(latin1_text);
	}
}

static void remmina_plugin_vnc_on_cuttext(GtkClipboard *clipboard, GdkEvent *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;

	if (!gpdata->connected || !gpdata->client)
		return;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (remmina_plugin_service->file_get_int(remminafile, "viewonly", FALSE))
		return;

	gtk_clipboard_request_text(clipboard, (GtkClipboardTextReceivedFunc)remmina_plugin_vnc_on_cuttext_request, gp);
}

static void remmina_plugin_vnc_on_realize(RemminaProtocolWidget *gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;
	GdkCursor *cursor;
	GdkPixbuf *pixbuf;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (remmina_plugin_service->file_get_int(remminafile, "showcursor", FALSE)) {
		/* Hide local cursor (show a small dot instead) */
		pixbuf = gdk_pixbuf_new_from_xpm_data(dot_cursor_xpm);
		cursor = gdk_cursor_new_from_pixbuf(gdk_display_get_default(), pixbuf, dot_cursor_x_hot, dot_cursor_y_hot);
		g_object_unref(pixbuf);
		gdk_window_set_cursor(gtk_widget_get_window(GTK_WIDGET(gp)), cursor);
		g_object_unref(cursor);
	}
}

/******************************************************************************************/

static gboolean remmina_plugin_vnc_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;

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
		gpdata->clipboard_handler = g_signal_connect(G_OBJECT(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)),
							     "owner-change", G_CALLBACK(remmina_plugin_vnc_on_cuttext), gp);


	if (pthread_create(&gpdata->thread, NULL, remmina_plugin_vnc_main_thread, gp)) {
		/* I don’t think this will ever happen… */
		g_print("Failed to initialize pthread. Falling back to non-thread mode…\n");
		g_timeout_add(0, (GSourceFunc)remmina_plugin_vnc_main, gp);
		gpdata->thread = 0;
	}

	return TRUE;
}

static gboolean remmina_plugin_vnc_close_connection_timeout(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);

	/* wait until the running attribute is set to false by the VNC thread */
	if (gpdata->running)
		return TRUE;

	/* unregister the clipboard monitor */
	if (gpdata->clipboard_handler) {
		g_signal_handler_disconnect(G_OBJECT(gtk_clipboard_get(GDK_SELECTION_CLIPBOARD)), gpdata->clipboard_handler);
		gpdata->clipboard_handler = 0;
	}

	if (gpdata->queuecursor_handler) {
		g_source_remove(gpdata->queuecursor_handler);
		gpdata->queuecursor_handler = 0;
	}
	if (gpdata->queuecursor_surface) {
		cairo_surface_destroy(gpdata->queuecursor_surface);
		gpdata->queuecursor_surface = NULL;
	}

	if (gpdata->queuedraw_handler) {
		g_source_remove(gpdata->queuedraw_handler);
		gpdata->queuedraw_handler = 0;
	}
	if (gpdata->listen_sock >= 0)
		close(gpdata->listen_sock);
	if (gpdata->client) {
		rfbClientCleanup((rfbClient *)gpdata->client);
		gpdata->client = NULL;
	}
	if (gpdata->rgb_buffer) {
		cairo_surface_destroy(gpdata->rgb_buffer);
		gpdata->rgb_buffer = NULL;
	}
	if (gpdata->vnc_buffer) {
		g_free(gpdata->vnc_buffer);
		gpdata->vnc_buffer = NULL;
	}
	g_ptr_array_free(gpdata->pressed_keys, TRUE);
    g_date_time_unref(gpdata->clipboard_timer);
	remmina_plugin_vnc_event_free_all(gp);
	g_queue_free(gpdata->vnc_event_queue);
	pthread_mutex_destroy(&gpdata->vnc_event_queue_mutex);
	close(gpdata->vnc_event_pipe[0]);
	close(gpdata->vnc_event_pipe[1]);


	pthread_mutex_destroy(&gpdata->buffer_mutex);
	remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);

	return FALSE;
}

static gboolean remmina_plugin_vnc_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);

	gpdata->connected = FALSE;

	if (gpdata->thread) {
		pthread_cancel(gpdata->thread);
		if (gpdata->thread) pthread_join(gpdata->thread, NULL);
		gpdata->running = FALSE;
		remmina_plugin_vnc_close_connection_timeout(gp);
	} else {
		g_timeout_add(200, (GSourceFunc)remmina_plugin_vnc_close_connection_timeout, gp);
	}

	return FALSE;
}

static gboolean remmina_plugin_vnc_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);

	switch (feature->id) {
	case REMMINA_PLUGIN_VNC_FEATURE_PREF_DISABLESERVERINPUT:
		return SupportsClient2Server((rfbClient *)(gpdata->client), rfbSetServerInput) ? TRUE : FALSE;
	case REMMINA_PLUGIN_VNC_FEATURE_TOOL_CHAT:
		return SupportsClient2Server((rfbClient *)(gpdata->client), rfbTextChat) ? TRUE : FALSE;
	default:
		return TRUE;
	}
}

static void remmina_plugin_vnc_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	switch (feature->id) {
	case REMMINA_PLUGIN_VNC_FEATURE_PREF_QUALITY:
		remmina_plugin_vnc_update_quality((rfbClient *)(gpdata->client),
						  remmina_plugin_service->file_get_int(remminafile, "quality", 9));
		remmina_plugin_vnc_update_colordepth((rfbClient *)(gpdata->client),
						     remmina_plugin_service->file_get_int(remminafile, "colordepth", 32));
		SetFormatAndEncodings((rfbClient *)(gpdata->client));
		break;
	case REMMINA_PLUGIN_VNC_FEATURE_PREF_VIEWONLY:
		break;
	case REMMINA_PLUGIN_VNC_FEATURE_PREF_DISABLESERVERINPUT:
		PermitServerInput((rfbClient *)(gpdata->client),
				  remmina_plugin_service->file_get_int(remminafile, "disableserverinput", FALSE) ? 1 : 0);
		break;
	case REMMINA_PLUGIN_VNC_FEATURE_UNFOCUS:
		remmina_plugin_vnc_release_key(gp, 0);
		break;
	case REMMINA_PLUGIN_VNC_FEATURE_SCALE:
		remmina_plugin_vnc_update_scale(gp, remmina_plugin_service->file_get_int(remminafile, "scale", FALSE));
		break;
	case REMMINA_PLUGIN_VNC_FEATURE_TOOL_REFRESH:
		SendFramebufferUpdateRequest((rfbClient *)(gpdata->client), 0, 0,
					     remmina_plugin_service->protocol_plugin_get_width(gp),
					     remmina_plugin_service->protocol_plugin_get_height(gp), FALSE);
		break;
	case REMMINA_PLUGIN_VNC_FEATURE_TOOL_CHAT:
		remmina_plugin_vnc_open_chat(gp);
		break;
	case REMMINA_PLUGIN_VNC_FEATURE_TOOL_SENDCTRLALTDEL:
		remmina_plugin_vnc_send_ctrlaltdel(gp);
		break;
	default:
		break;
	}
}

/* Send a keystroke to the plugin window */
static void remmina_plugin_vnc_keystroke(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	remmina_plugin_service->protocol_plugin_send_keys_signals(gpdata->drawing_area,
								  keystrokes, keylen, GDK_KEY_PRESS | GDK_KEY_RELEASE);
	return;
}

static gboolean remmina_plugin_vnc_on_draw(GtkWidget *widget, cairo_t *context, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata = GET_PLUGIN_DATA(gp);
	cairo_surface_t *surface;
	gint width, height;
	GtkAllocation widget_allocation;

	LOCK_BUFFER(FALSE);

	surface = gpdata->rgb_buffer;
	if (!surface) {
		UNLOCK_BUFFER(FALSE);
		return FALSE;
	}

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);

	if ((remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp) != REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE)) {
		gtk_widget_get_allocation(widget, &widget_allocation);
		cairo_scale(context,
			    (double)widget_allocation.width / width,
			    (double)widget_allocation.height / height);
	}

	cairo_rectangle(context, 0, 0, width, height);
	cairo_set_source_surface(context, surface, 0, 0);
	cairo_fill(context);

	UNLOCK_BUFFER(FALSE);
	return TRUE;
}

static void remmina_plugin_vnc_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginVncData *gpdata;
	gint flags;

	gpdata = g_new0(RemminaPluginVncData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	gpdata->drawing_area = gtk_drawing_area_new();
	gtk_widget_show(gpdata->drawing_area);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->drawing_area);

	gtk_widget_add_events(
		gpdata->drawing_area,
		GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK
		| GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK
		| GDK_KEY_RELEASE_MASK
#if GTK_CHECK_VERSION(3, 4, 0)
		| GDK_SMOOTH_SCROLL_MASK
#endif
		| GDK_SCROLL_MASK);
	gtk_widget_set_can_focus(gpdata->drawing_area, TRUE);


	g_signal_connect(G_OBJECT(gpdata->drawing_area), "draw", G_CALLBACK(remmina_plugin_vnc_on_draw), gp);

	gpdata->auth_first = TRUE;
	gpdata->clipboard_timer = g_date_time_new_now_utc();
	gpdata->listen_sock = -1;
	gpdata->pressed_keys = g_ptr_array_new();
	gpdata->vnc_event_queue = g_queue_new();
	pthread_mutex_init(&gpdata->vnc_event_queue_mutex, NULL);
	if (pipe(gpdata->vnc_event_pipe)) {
		g_print("Error creating pipes.\n");
		gpdata->vnc_event_pipe[0] = 0;
		gpdata->vnc_event_pipe[1] = 0;
	}
	flags = fcntl(gpdata->vnc_event_pipe[0], F_GETFL, 0);
	fcntl(gpdata->vnc_event_pipe[0], F_SETFL, flags | O_NONBLOCK);

	pthread_mutex_init(&gpdata->buffer_mutex, NULL);
}

/* Array of key/value pairs for color depths */
static gpointer colordepth_list[] =
{
	"32", N_("True color (32 bpp)"),
	"16", N_("High color (16 bpp)"),
	"8",  N_("256 colors (8 bpp)"),
	NULL
};

/* Array of key/value pairs for quality selection */
static gpointer quality_list[] =
{
	"9", N_("Best (slowest)"),
	"2", N_("Good"),
	"1", N_("Medium"),
	"0", N_("Poor (fastest)"),
	NULL
};

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_vnc_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER,	  "server",	NULL,		     FALSE, "_rfb._tcp",     NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "proxy",	N_("Repeater"),	     FALSE, NULL,	     NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "username",	N_("Username"),	     FALSE, NULL,	     NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "password",	N_("User password"), FALSE, NULL,	     NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "colordepth", N_("Color depth"),   FALSE, colordepth_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "quality",	N_("Quality"),	     FALSE, quality_list,    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_KEYMAP,	  "keymap",	NULL,		     FALSE, NULL,	     NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,		NULL,		     FALSE, NULL,	     NULL }
};

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_vnci_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "listenport", N_("Listen on port"), FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "username",	N_("Username"),	      FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "password",	N_("User password"),  FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "colordepth", N_("Color depth"),    FALSE, colordepth_list, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "quality",	N_("Quality"),	      FALSE, quality_list,    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_KEYMAP,	  "keymap",		NULL,		      FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,		NULL,		      FALSE, NULL,	      NULL }
};

/* Array of RemminaProtocolSetting for advanced settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_vnc_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "showcursor",		 N_("Show remote cursor"),	 TRUE,	NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "viewonly",		 N_("View only"),		 FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard",	 N_("Disable clipboard sync"),	 TRUE,	NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableencryption",	 N_("Disable encryption"),	 FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableserverinput",	 N_("Disable server input"),	 TRUE,	NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disablepasswordstoring", N_("Forget passwords after use"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,   NULL,			 NULL,				 FALSE, NULL, NULL }
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_plugin_vnc_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF,	 REMMINA_PLUGIN_VNC_FEATURE_PREF_QUALITY,	     GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_RADIO), "quality",
	  quality_list },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF,	 REMMINA_PLUGIN_VNC_FEATURE_PREF_VIEWONLY,	     GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "viewonly",
	  N_("View only") },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF,	 REMMINA_PLUGIN_VNC_FEATURE_PREF_DISABLESERVERINPUT, GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "disableserverinput",N_("Disable server input")  },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL,	 REMMINA_PLUGIN_VNC_FEATURE_TOOL_REFRESH,	     N_("Refresh"),					   NULL,		NULL			    },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL,	 REMMINA_PLUGIN_VNC_FEATURE_TOOL_CHAT,		     N_("Open Chat…"),					   "face-smile",	NULL			    },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL,	 REMMINA_PLUGIN_VNC_FEATURE_TOOL_SENDCTRLALTDEL,     N_("Send Ctrl+Alt+Delete"),			   NULL,		NULL			    },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_SCALE,	 REMMINA_PLUGIN_VNC_FEATURE_SCALE,		     NULL,						   NULL,		NULL			    },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, REMMINA_PLUGIN_VNC_FEATURE_UNFOCUS,		     NULL,						   NULL,		NULL			    },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END,	 0,						     NULL,						   NULL,		NULL			    }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin_vnc =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   // Type
	VNC_PLUGIN_NAME,                                // Name
	VNC_PLUGIN_DESCRIPTION,                         // Description
	GETTEXT_PACKAGE,                                // Translation domain
	VNC_PLUGIN_VERSION,                             // Version number
	VNC_PLUGIN_APPICON,                             // Icon for normal connection
	VNC_PLUGIN_SSH_APPICON,                         // Icon for SSH connection
	remmina_plugin_vnc_basic_settings,              // Array for basic settings
	remmina_plugin_vnc_advanced_settings,           // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,            // SSH settings type
	remmina_plugin_vnc_features,                    // Array for available features
	remmina_plugin_vnc_init,                        // Plugin initialization
	remmina_plugin_vnc_open_connection,             // Plugin open connection
	remmina_plugin_vnc_close_connection,            // Plugin close connection
	remmina_plugin_vnc_query_feature,               // Query for available features
	remmina_plugin_vnc_call_feature,                // Call a feature
	remmina_plugin_vnc_keystroke                    // Send a keystroke
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin_vnci =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   // Type
	VNCI_PLUGIN_NAME,                               // Name
	VNCI_PLUGIN_DESCRIPTION,                        // Description
	GETTEXT_PACKAGE,                                // Translation domain
	VERSION,                                        // Version number
	VNCI_PLUGIN_APPICON,                            // Icon for normal connection
	VNCI_PLUGIN_SSH_APPICON,                        // Icon for SSH connection
	remmina_plugin_vnci_basic_settings,             // Array for basic settings
	remmina_plugin_vnc_advanced_settings,           // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL,    // SSH settings type
	remmina_plugin_vnc_features,                    // Array for available features
	remmina_plugin_vnc_init,                        // Plugin initialization
	remmina_plugin_vnc_open_connection,             // Plugin open connection
	remmina_plugin_vnc_close_connection,            // Plugin close connection
	remmina_plugin_vnc_query_feature,               // Query for available features
	remmina_plugin_vnc_call_feature,                // Call a feature
	remmina_plugin_vnc_keystroke,                   // Send a keystroke
	NULL                                            // No screenshot support available
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *)&remmina_plugin_vnc))
		return FALSE;

	if (!service->register_plugin((RemminaPlugin *)&remmina_plugin_vnci))
		return FALSE;

	return TRUE;
}
