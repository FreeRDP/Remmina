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

/* X11 drawings were ported from xfreerdp */

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_gdi.h"
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/kbd/kbd.h>

static void remmina_rdp_event_event_push(RemminaProtocolWidget* gp, const RemminaPluginRdpEvent* e)
{
	rfContext* rfi;
	RemminaPluginRdpEvent* event;

	rfi = GET_DATA(gp);

	if (rfi->event_queue)
	{
		event = g_memdup(e, sizeof(RemminaPluginRdpEvent));
		g_async_queue_push(rfi->event_queue, event);

		if (write(rfi->event_pipe[1], "\0", 1))
		{
		}
	}
}

static void remmina_rdp_event_release_key(RemminaProtocolWidget* gp, gint scancode)
{
	gint i, k;
	rfContext* rfi;
	RemminaPluginRdpEvent rdp_event = { 0 };

	rfi = GET_DATA(gp);
	rdp_event.type = REMMINA_RDP_EVENT_TYPE_SCANCODE;

	if (scancode == 0)
	{
		/* Send all release key events for previously pressed keys */
		rdp_event.key_event.up = True;

		for (i = 0; i < rfi->pressed_keys->len; i++)
		{
			rdp_event.key_event.key_code = g_array_index(rfi->pressed_keys, gint, i);
			remmina_rdp_event_event_push(gp, &rdp_event);
		}

		g_array_set_size(rfi->pressed_keys, 0);
	}
	else
	{
		/* Unregister the keycode only */
		for (i = 0; i < rfi->pressed_keys->len; i++)
		{
			k = g_array_index(rfi->pressed_keys, gint, i);

			if (k == scancode)
			{
				g_array_remove_index_fast(rfi->pressed_keys, i);
				break;
			}
		}
	}
}

static void remmina_rdp_event_scale_area(RemminaProtocolWidget* gp, gint* x, gint* y, gint* w, gint* h)
{
	gint width, height;
	gint sx, sy, sw, sh;
	rfContext* rfi;

	rfi = GET_DATA(gp);

	if (!rfi->rgb_surface)
		return;

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);

	if ((width == 0) || (height == 0))
		return;

	if ((rfi->scale_width == width) && (rfi->scale_height == height))
	{
		/* Same size, just copy the pixels */
		*x = MIN(MAX(0, *x), width - 1);
		*y = MIN(MAX(0, *y), height - 1);
		*w = MIN(width - *x, *w);
		*h = MIN(height - *y, *h);
		return;
	}

	/* We have to extend the scaled region one scaled pixel, to avoid gaps */

	sx = MIN(MAX(0, (*x) * rfi->scale_width / width
		- rfi->scale_width / width - 2), rfi->scale_width - 1);

	sy = MIN(MAX(0, (*y) * rfi->scale_height / height
		- rfi->scale_height / height - 2), rfi->scale_height - 1);

	sw = MIN(rfi->scale_width - sx, (*w) * rfi->scale_width / width
		+ rfi->scale_width / width + 4);

	sh = MIN(rfi->scale_height - sy, (*h) * rfi->scale_height / height
		+ rfi->scale_height / height + 4);

	*x = sx;
	*y = sy;
	*w = sw;
	*h = sh;
}

void remmina_rdp_event_update_region(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfContext* rfi;
	gint x, y, w, h;

	x = ui->region.x;
	y = ui->region.y;
	w = ui->region.width;
	h = ui->region.height;

	rfi = GET_DATA(gp);

	if (rfi->sw_gdi)
	{
		XPutImage(rfi->display, rfi->primary, rfi->gc, rfi->image, x, y, x, y, w, h);
		XCopyArea(rfi->display, rfi->primary, rfi->rgb_surface, rfi->gc, x, y, w, h, x, y);
	}

	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
		remmina_rdp_event_scale_area(gp, &x, &y, &w, &h);

	gtk_widget_queue_draw_area(rfi->drawing_area, x, y, w, h);
}

void remmina_rdp_event_update_rect(RemminaProtocolWidget* gp, gint x, gint y, gint w, gint h)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	if (rfi->sw_gdi)
	{
		XPutImage(rfi->display, rfi->primary, rfi->gc, rfi->image, x, y, x, y, w, h);
		XCopyArea(rfi->display, rfi->primary, rfi->rgb_surface, rfi->gc, x, y, w, h, x, y);
	}

	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
		remmina_rdp_event_scale_area(gp, &x, &y, &w, &h);

	gtk_widget_queue_draw_area(rfi->drawing_area, x, y, w, h);
}

static gboolean remmina_rdp_event_update_scale_factor(RemminaProtocolWidget* gp)
{
	GtkAllocation a;
	gboolean scale;
	gint width, height;
	gint hscale, vscale;
	gint gpwidth, gpheight;
	RemminaFile* remminafile;
	rfContext* rfi;

	rfi = GET_DATA(gp);
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	gtk_widget_get_allocation(GTK_WIDGET(gp), &a);
	width = a.width;
	height = a.height;
	scale = remmina_plugin_service->protocol_plugin_get_scale(gp);

	if (scale)
	{
		if ((width > 1) && (height > 1))
		{
			gpwidth = remmina_plugin_service->protocol_plugin_get_width(gp);
			gpheight = remmina_plugin_service->protocol_plugin_get_height(gp);
			hscale = remmina_plugin_service->file_get_int(remminafile, "hscale", 0);
			vscale = remmina_plugin_service->file_get_int(remminafile, "vscale", 0);

			rfi->scale_width = (hscale > 0 ?
				MAX(1, gpwidth * hscale / 100) : width);
			rfi->scale_height = (vscale > 0 ?
				MAX(1, gpheight * vscale / 100) : height);

			rfi->scale_x = (gdouble) rfi->scale_width / (gdouble) gpwidth;
			rfi->scale_y = (gdouble) rfi->scale_height / (gdouble) gpheight;
		}
	}
	else
	{
		rfi->scale_width = 0;
		rfi->scale_height = 0;
		rfi->scale_x = 0;
		rfi->scale_y = 0;
	}

	if ((width > 1) && (height > 1))
		gtk_widget_queue_draw_area(GTK_WIDGET(gp), 0, 0, width, height);

	rfi->scale_handler = 0;

	return FALSE;
}

#if GTK_VERSION == 2
static gboolean remmina_rdp_event_on_expose(GtkWidget *widget, GdkEventExpose *event, RemminaProtocolWidget *gp)
#else
static gboolean remmina_rdp_event_on_draw(GtkWidget* widget, cairo_t* context, RemminaProtocolWidget* gp)
#endif
{
	gboolean scale;
	rfContext* rfi;
#if GTK_VERSION == 2
	gint x, y;
	cairo_t *context;
#endif

	rfi = GET_DATA(gp);

	if (!rfi->rgb_cairo_surface)
		return FALSE;

	scale = remmina_plugin_service->protocol_plugin_get_scale(gp);

#if GTK_VERSION == 2
	x = event->area.x;
	y = event->area.y;

	context = gdk_cairo_create(gtk_widget_get_window (rfi->drawing_area));
	cairo_rectangle(context, x, y, event->area.width, event->area.height);
#else
	cairo_rectangle(context, 0, 0, gtk_widget_get_allocated_width(widget),
		gtk_widget_get_allocated_height(widget));
#endif

	if (scale)
		cairo_scale(context, rfi->scale_x, rfi->scale_y);

	cairo_set_source_surface(context, rfi->rgb_cairo_surface, 0, 0);
	cairo_fill(context);

#if GTK_VERSION == 2
	cairo_destroy(context);
#endif

	return TRUE;
}

static gboolean remmina_rdp_event_on_configure(GtkWidget* widget, GdkEventConfigure* event, RemminaProtocolWidget* gp)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	/* We do a delayed reallocating to improve performance */

	if (rfi->scale_handler)
		g_source_remove(rfi->scale_handler);

	rfi->scale_handler = g_timeout_add(1000, (GSourceFunc) remmina_rdp_event_update_scale_factor, gp);

	return FALSE;
}

static void remmina_rdp_event_translate_pos(RemminaProtocolWidget* gp, int ix, int iy, uint16* ox, uint16* oy)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	if ((rfi->scale) && (rfi->scale_width >= 1) && (rfi->scale_height >= 1))
	{
		*ox = (uint16) (ix * remmina_plugin_service->protocol_plugin_get_width(gp) / rfi->scale_width);
		*oy = (uint16) (iy * remmina_plugin_service->protocol_plugin_get_height(gp) / rfi->scale_height);
	}
	else
	{
		*ox = (uint16) ix;
		*oy = (uint16) iy;
	}
}

static gboolean remmina_rdp_event_on_motion(GtkWidget* widget, GdkEventMotion* event, RemminaProtocolWidget* gp)
{
	RemminaPluginRdpEvent rdp_event = { 0 };

	rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;
	rdp_event.mouse_event.flags = PTR_FLAGS_MOVE;

	remmina_rdp_event_translate_pos(gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);
	remmina_rdp_event_event_push(gp, &rdp_event);

	return TRUE;
}

static gboolean remmina_rdp_event_on_button(GtkWidget* widget, GdkEventButton* event, RemminaProtocolWidget* gp)
{
	gint flag;
	RemminaPluginRdpEvent rdp_event = { 0 };

	/* We only accept 3 buttons */
	if ((event->button < 1) || (event->button > 3))
		return FALSE;

	/* We bypass 2button-press and 3button-press events */
	if ((event->type != GDK_BUTTON_PRESS) && (event->type != GDK_BUTTON_RELEASE))
		return TRUE;

	rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;
	remmina_rdp_event_translate_pos(gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);

	flag = 0;

	if (event->type == GDK_BUTTON_PRESS)
		flag = PTR_FLAGS_DOWN;

	switch (event->button)
	{
		case 1:
			flag |= PTR_FLAGS_BUTTON1;
			break;
		case 2:
			flag |= PTR_FLAGS_BUTTON3;
			break;
		case 3:
			flag |= PTR_FLAGS_BUTTON2;
			break;
	}

	if (flag != 0)
	{
		rdp_event.mouse_event.flags = flag;
		remmina_rdp_event_event_push(gp, &rdp_event);
	}

	return TRUE;
}

static gboolean remmina_rdp_event_on_scroll(GtkWidget* widget, GdkEventScroll* event, RemminaProtocolWidget* gp)
{
	gint flag;
	RemminaPluginRdpEvent rdp_event = { 0 };

	flag = 0;
	rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;

	switch (event->direction)
	{
		case GDK_SCROLL_UP:
			flag = PTR_FLAGS_WHEEL | 0x0078;
			break;

		case GDK_SCROLL_DOWN:
			flag = PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x0088;
			break;

		default:
			return FALSE;
	}

	rdp_event.mouse_event.flags = flag;
	remmina_rdp_event_translate_pos(gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);
	remmina_rdp_event_event_push(gp, &rdp_event);

	return TRUE;
}

static gboolean remmina_rdp_event_on_key(GtkWidget* widget, GdkEventKey* event, RemminaProtocolWidget* gp)
{
	Display* display;
	KeyCode cooked_keycode;
	rfContext* rfi;
	RemminaPluginRdpEvent rdp_event;

	rfi = GET_DATA(gp);
	rdp_event.type = REMMINA_RDP_EVENT_TYPE_SCANCODE;
	rdp_event.key_event.up = (event->type == GDK_KEY_PRESS ? False : True);
	rdp_event.key_event.extended = False;

	switch (event->keyval)
	{
		case GDK_KEY_Pause:
			rdp_event.key_event.key_code = 0x1D;
			rdp_event.key_event.up = False;
			remmina_rdp_event_event_push(gp, &rdp_event);
			rdp_event.key_event.key_code = 0x45;
			rdp_event.key_event.up = False;
			remmina_rdp_event_event_push(gp, &rdp_event);
			rdp_event.key_event.key_code = 0x1D;
			rdp_event.key_event.up = True;
			remmina_rdp_event_event_push(gp, &rdp_event);
			rdp_event.key_event.key_code = 0x45;
			rdp_event.key_event.up = True;
			remmina_rdp_event_event_push(gp, &rdp_event);
			break;

		default:
			if (!rfi->use_client_keymap)
			{
				rdp_event.key_event.key_code = freerdp_kbd_get_scancode_by_keycode(event->hardware_keycode, &rdp_event.key_event.extended);
				remmina_plugin_service->log_printf("[RDP]keyval=%04X keycode=%i scancode=%i extended=%i\n",
						event->keyval, event->hardware_keycode, rdp_event.key_event.key_code, &rdp_event.key_event.extended);
			}
			else
			{
				display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default());
				cooked_keycode = XKeysymToKeycode(display, XKeycodeToKeysym(display, event->hardware_keycode, 0));
				rdp_event.key_event.key_code = freerdp_kbd_get_scancode_by_keycode(cooked_keycode, &rdp_event.key_event.extended);
				remmina_plugin_service->log_printf("[RDP]keyval=%04X raw_keycode=%i cooked_keycode=%i scancode=%i extended=%i\n",
						event->keyval, event->hardware_keycode, cooked_keycode, rdp_event.key_event.key_code, &rdp_event.key_event.extended);
			}

			if (rdp_event.key_event.key_code)
				remmina_rdp_event_event_push(gp, &rdp_event);

			break;
	}

	/* Register/unregister the pressed key */
	if (rdp_event.key_event.key_code)
	{
		if (event->type == GDK_KEY_PRESS)
			g_array_append_val(rfi->pressed_keys, rdp_event.key_event.key_code);
		else
			remmina_rdp_event_release_key(gp, rdp_event.key_event.key_code);
	}

	return TRUE;
}

void remmina_rdp_event_init(RemminaProtocolWidget* gp)
{
	gint n;
	gint i;
	gchar* s;
	gint flags;
	XPixmapFormatValues* pf;
	XPixmapFormatValues* pfs;
	rfContext* rfi;

	rfi = GET_DATA(gp);
	rfi->drawing_area = gtk_drawing_area_new();
	gtk_widget_show(rfi->drawing_area);
	gtk_container_add(GTK_CONTAINER(gp), rfi->drawing_area);

	gtk_widget_add_events(rfi->drawing_area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK
		| GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
	gtk_widget_set_can_focus(rfi->drawing_area, TRUE);

	remmina_plugin_service->protocol_plugin_register_hostkey(gp, rfi->drawing_area);

	s = remmina_plugin_service->pref_get_value("rdp_use_client_keymap");
	rfi->use_client_keymap = (s && s[0] == '1' ? TRUE : FALSE);
	g_free(s);

#if GTK_VERSION == 3
	g_signal_connect(G_OBJECT(rfi->drawing_area), "draw",
		G_CALLBACK(remmina_rdp_event_on_draw), gp);
#elif GTK_VERSION == 2
	g_signal_connect(G_OBJECT(rfi->drawing_area), "expose-event",
		G_CALLBACK(remmina_rdp_event_on_expose), gp);
#endif
	g_signal_connect(G_OBJECT(rfi->drawing_area), "configure-event",
		G_CALLBACK(remmina_rdp_event_on_configure), gp);
	g_signal_connect(G_OBJECT(rfi->drawing_area), "motion-notify-event",
		G_CALLBACK(remmina_rdp_event_on_motion), gp);
	g_signal_connect(G_OBJECT(rfi->drawing_area), "button-press-event",
		G_CALLBACK(remmina_rdp_event_on_button), gp);
	g_signal_connect(G_OBJECT(rfi->drawing_area), "button-release-event",
		G_CALLBACK(remmina_rdp_event_on_button), gp);
	g_signal_connect(G_OBJECT(rfi->drawing_area), "scroll-event",
		G_CALLBACK(remmina_rdp_event_on_scroll), gp);
	g_signal_connect(G_OBJECT(rfi->drawing_area), "key-press-event",
		G_CALLBACK(remmina_rdp_event_on_key), gp);
	g_signal_connect(G_OBJECT(rfi->drawing_area), "key-release-event",
		G_CALLBACK(remmina_rdp_event_on_key), gp);

	rfi->pressed_keys = g_array_new(FALSE, TRUE, sizeof (gint));
	rfi->event_queue = g_async_queue_new_full(g_free);
	rfi->ui_queue = g_async_queue_new();

	if (pipe(rfi->event_pipe))
	{
		g_print("Error creating pipes.\n");
		rfi->event_pipe[0] = -1;
		rfi->event_pipe[1] = -1;
	}
	else
	{
		flags = fcntl(rfi->event_pipe[0], F_GETFL, 0);
		fcntl(rfi->event_pipe[0], F_SETFL, flags | O_NONBLOCK);
	}

	rfi->object_table = g_hash_table_new_full(NULL, NULL, NULL, g_free);

	rfi->display = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
	rfi->depth = DefaultDepth(rfi->display, DefaultScreen(rfi->display));
	rfi->visual = GDK_VISUAL_XVISUAL(gdk_visual_get_best_with_depth(rfi->depth));
	pfs = XListPixmapFormats(rfi->display, &n);

	if (pfs)
	{
		for (i = 0; i < n; i++)
		{
			pf = pfs + i;

			if (pf->depth == rfi->depth)
			{
				rfi->scanline_pad = pf->scanline_pad;
				rfi->bpp = pf->bits_per_pixel;
				break;
			}
		}

		XFree(pfs);
	}
}

void remmina_rdp_event_uninit(RemminaProtocolWidget* gp)
{
	rfContext* rfi;
	RemminaPluginRdpUiObject* ui;

	rfi = GET_DATA(gp);

	if (rfi->scale_handler)
	{
		g_source_remove(rfi->scale_handler);
		rfi->scale_handler = 0;
	}
	if (rfi->ui_handler)
	{
		g_source_remove(rfi->ui_handler);
		rfi->ui_handler = 0;
	}
	while ((ui =(RemminaPluginRdpUiObject*) g_async_queue_try_pop(rfi->ui_queue)) != NULL)
	{
		rf_object_free(gp, ui);
	}

	if (rfi->gc)
	{
		XFreeGC(rfi->display, rfi->gc);
		rfi->gc = 0;
	}
	if (rfi->gc_default)
	{
		XFreeGC(rfi->display, rfi->gc_default);
		rfi->gc_default = 0;
	}
	if (rfi->rgb_cairo_surface)
	{
		cairo_surface_destroy(rfi->rgb_cairo_surface);
		rfi->rgb_cairo_surface = NULL;
	}
	if (rfi->rgb_surface)
	{
		XFreePixmap(rfi->display, rfi->rgb_surface);
		rfi->rgb_surface = 0;
	}
	if (rfi->gc_mono)
	{
		XFreeGC(rfi->display, rfi->gc_mono);
		rfi->gc_mono = 0;
	}
	if (rfi->bitmap_mono)
	{
		XFreePixmap(rfi->display, rfi->bitmap_mono);
		rfi->bitmap_mono = 0;
	}

	g_hash_table_destroy(rfi->object_table);

	g_array_free(rfi->pressed_keys, TRUE);
	g_async_queue_unref(rfi->event_queue);
	rfi->event_queue = NULL;
	g_async_queue_unref(rfi->ui_queue);
	rfi->ui_queue = NULL;
	close(rfi->event_pipe[0]);
	close(rfi->event_pipe[1]);
}

void remmina_rdp_event_update_scale(RemminaProtocolWidget* gp)
{
	gint width, height;
	gint hscale, vscale;
	RemminaFile* remminafile;
	rfContext* rfi;

	rfi = GET_DATA(gp);
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);
	hscale = remmina_plugin_service->file_get_int(remminafile, "hscale", 0);
	vscale = remmina_plugin_service->file_get_int(remminafile, "vscale", 0);

	if (rfi->scale)
	{
		gtk_widget_set_size_request(rfi->drawing_area,
			(hscale > 0 ? width * hscale / 100 : -1),
			(vscale > 0 ? height * vscale / 100 : -1));
	}
	else
	{
		gtk_widget_set_size_request(rfi->drawing_area, width, height);
	}

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "update-align");
}

static uint8 remmina_rdp_event_rop2_map[] =
{
	GXclear,		/* 0 */
	GXnor,			/* DPon */
	GXandInverted,		/* DPna */
	GXcopyInverted,		/* Pn */
	GXandReverse,		/* PDna */
	GXinvert,		/* Dn */
	GXxor,			/* DPx */
	GXnand,			/* DPan */
	GXand,			/* DPa */
	GXequiv,		/* DPxn */
	GXnoop,			/* D */
	GXorInverted,		/* DPno */
	GXcopy,			/* P */
	GXorReverse,		/* PDno */
	GXor,			/* DPo */
	GXset			/* 1 */
};

static void remmina_rdp_event_set_rop2(rfContext* rfi, gint rop2)
{
	if ((rop2 < 0x01) || (rop2 > 0x10))
	{
		remmina_plugin_service->log_printf("[RDP]unknown rop2 0x%x", rop2);
	}
	else
	{
		XSetFunction(rfi->display, rfi->gc, remmina_rdp_event_rop2_map[rop2 - 1]);
	}
}

static void remmina_rdp_event_set_rop3(rfContext* rfi, gint rop3)
{
	switch (rop3)
	{
		case 0x00: /* 0 - 0 */
			XSetFunction(rfi->display, rfi->gc, GXclear);
			break;
		case 0x05: /* ~(P | D) - DPon */
			XSetFunction(rfi->display, rfi->gc, GXnor);
			break;
		case 0x0a: /* ~P & D - DPna */
			XSetFunction(rfi->display, rfi->gc, GXandInverted);
			break;
		case 0x0f: /* ~P - Pn */
			XSetFunction(rfi->display, rfi->gc, GXcopyInverted);
			break;
		case 0x11: /* ~(S | D) - DSon */
			XSetFunction(rfi->display, rfi->gc, GXnor);
			break;
		case 0x22: /* ~S & D - DSna */
			XSetFunction(rfi->display, rfi->gc, GXandInverted);
			break;
		case 0x33: /* ~S - Sn */
			XSetFunction(rfi->display, rfi->gc, GXcopyInverted);
			break;
		case 0x44: /* S & ~D - SDna */
			XSetFunction(rfi->display, rfi->gc, GXandReverse);
			break;
		case 0x50: /* P & ~D - PDna */
			XSetFunction(rfi->display, rfi->gc, GXandReverse);
			break;
		case 0x55: /* ~D - Dn */
			XSetFunction(rfi->display, rfi->gc, GXinvert);
			break;
		case 0x5a: /* D ^ P - DPx */
			XSetFunction(rfi->display, rfi->gc, GXxor);
			break;
		case 0x5f: /* ~(P & D) - DPan */
			XSetFunction(rfi->display, rfi->gc, GXnand);
			break;
		case 0x66: /* D ^ S - DSx */
			XSetFunction(rfi->display, rfi->gc, GXxor);
			break;
		case 0x77: /* ~(S & D) - DSan */
			XSetFunction(rfi->display, rfi->gc, GXnand);
			break;
		case 0x88: /* D & S - DSa */
			XSetFunction(rfi->display, rfi->gc, GXand);
			break;
		case 0x99: /* ~(S ^ D) - DSxn */
			XSetFunction(rfi->display, rfi->gc, GXequiv);
			break;
		case 0xa0: /* P & D - DPa */
			XSetFunction(rfi->display, rfi->gc, GXand);
			break;
		case 0xa5: /* ~(P ^ D) - PDxn */
			XSetFunction(rfi->display, rfi->gc, GXequiv);
			break;
		case 0xaa: /* D - D */
			XSetFunction(rfi->display, rfi->gc, GXnoop);
			break;
		case 0xaf: /* ~P | D - DPno */
			XSetFunction(rfi->display, rfi->gc, GXorInverted);
			break;
		case 0xbb: /* ~S | D - DSno */
			XSetFunction(rfi->display, rfi->gc, GXorInverted);
			break;
		case 0xcc: /* S - S */
			XSetFunction(rfi->display, rfi->gc, GXcopy);
			break;
		case 0xdd: /* S | ~D - SDno */
			XSetFunction(rfi->display, rfi->gc, GXorReverse);
			break;
		case 0xee: /* D | S - DSo */
			XSetFunction(rfi->display, rfi->gc, GXor);
			break;
		case 0xf0: /* P - P */
			XSetFunction(rfi->display, rfi->gc, GXcopy);
			break;
		case 0xf5: /* P | ~D - PDno */
			XSetFunction(rfi->display, rfi->gc, GXorReverse);
			break;
		case 0xfa: /* P | D - DPo */
			XSetFunction(rfi->display, rfi->gc, GXor);
			break;
		case 0xff: /* 1 - 1 */
			XSetFunction(rfi->display, rfi->gc, GXset);
			break;
		default:
			remmina_plugin_service->log_printf("[RDP]unknown rop3 0x%x", rop3);
			break;
	}
}

static void remmina_rdp_event_insert_drawable(rfContext* rfi, guint object_id, Drawable obj)
{
	Drawable* p;

	p = g_new(Drawable, 1);
	*p = obj;
	g_hash_table_insert(rfi->object_table, GINT_TO_POINTER(object_id), p);
}

static Drawable remmina_rdp_event_get_drawable(rfContext* rfi, guint object_id)
{
	Drawable* p;

	p = (Drawable*) g_hash_table_lookup(rfi->object_table, GINT_TO_POINTER(object_id));

	if (!p)
		return 0;

	return *p;
}

static void remmina_rdp_event_connected(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	gtk_widget_realize(rfi->drawing_area);

	rfi->drawable = GDK_WINDOW_XID(gtk_widget_get_window(rfi->drawing_area));

	rfi->rgb_surface = XCreatePixmap(rfi->display, rfi->drawable,
		rfi->settings->width, rfi->settings->height, rfi->depth);

	rfi->rgb_cairo_surface = cairo_xlib_surface_create(rfi->display,
			rfi->rgb_surface, rfi->visual, rfi->width, rfi->height);

	rfi->drw_surface = rfi->rgb_surface;

	remmina_rdp_event_update_scale(gp);
}

static void remmina_rdp_event_rfx(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	XImage* image;
	gint i, tx, ty;
	RFX_MESSAGE* message;
	rfContext* rfi;

	rfi = GET_DATA(gp);
	message = ui->rfx.message;

	XSetFunction(rfi->display, rfi->gc, GXcopy);
	XSetFillStyle(rfi->display, rfi->gc, FillSolid);

	XSetClipRectangles(rfi->display, rfi->gc, ui->rfx.left, ui->rfx.top,
		(XRectangle*) message->rects, message->num_rects, YXBanded);

	/* Draw the tiles to primary surface, each is 64x64. */
	for (i = 0; i < message->num_tiles; i++)
	{
		image = XCreateImage(rfi->display, rfi->visual, 24, ZPixmap, 0,
			(char*) message->tiles[i]->data, 64, 64, 32, 0);

		tx = message->tiles[i]->x + ui->rfx.left;
		ty = message->tiles[i]->y + ui->rfx.top;

		XPutImage(rfi->display, rfi->rgb_surface, rfi->gc, image, 0, 0, tx, ty, 64, 64);
		XFree(image);

		remmina_rdp_event_update_rect(gp, tx, ty, message->rects[i].width, message->rects[i].height);
	}

	XSetClipMask(rfi->display, rfi->gc, None);
}

static void remmina_rdp_event_nocodec(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	XImage* image;
	rfContext* rfi;

	rfi = GET_DATA(gp);

	XSetFunction(rfi->display, rfi->gc, GXcopy);
	XSetFillStyle(rfi->display, rfi->gc, FillSolid);

	image = XCreateImage(rfi->display, rfi->visual, 24, ZPixmap, 0,
		(char*) ui->nocodec.bitmap, ui->nocodec.width, ui->nocodec.height, 32, 0);

	XPutImage(rfi->display, rfi->rgb_surface, rfi->gc, image, 0, 0,
		ui->nocodec.left, ui->nocodec.top,
		ui->nocodec.width, ui->nocodec.height);

	remmina_rdp_event_update_rect(gp,
		ui->nocodec.left, ui->nocodec.top,
		ui->nocodec.width, ui->nocodec.height);

	XSetClipMask(rfi->display, rfi->gc, None);
}

gboolean remmina_rdp_event_queue_ui(RemminaProtocolWidget* gp)
{
	rfContext* rfi;
	RemminaPluginRdpUiObject* ui;

	rfi = GET_DATA(gp);

	ui = (RemminaPluginRdpUiObject*) g_async_queue_try_pop(rfi->ui_queue);

	if (ui)
	{
		switch (ui->type)
		{
			case REMMINA_RDP_UI_UPDATE_REGION:
				remmina_rdp_event_update_region(gp, ui);
				break;

			case REMMINA_RDP_UI_CONNECTED:
				remmina_rdp_event_connected(gp, ui);
				break;

			case REMMINA_RDP_UI_RFX:
				remmina_rdp_event_rfx(gp, ui);
				break;

			case REMMINA_RDP_UI_NOCODEC:
				remmina_rdp_event_nocodec(gp, ui);
				break;

			default:
				break;
		}
		rf_object_free(gp, ui);

		return TRUE;
	}
	else
	{
		LOCK_BUFFER(FALSE)
		rfi->ui_handler = 0;
		UNLOCK_BUFFER(FALSE)
		return FALSE;
	}
}

void remmina_rdp_event_unfocus(RemminaProtocolWidget* gp)
{
	remmina_rdp_event_release_key(gp, 0);
}

