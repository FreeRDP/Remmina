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

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_gdi.h"
#include "rdp_cliprdr.h"
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/locale/keyboard.h>
#include <X11/XKBlib.h>

static void remmina_rdp_event_on_focus_in(GtkWidget* widget, GdkEventKey* event, RemminaProtocolWidget* gp)
{
	rfContext* rfi;
	rdpInput* input;
	GdkModifierType state;
#if GTK_VERSION == 3
	GdkDeviceManager *manager;
	GdkDevice *keyboard = NULL;
#endif

	rfi = GET_DATA(gp);
	if ( !rfi )
		return;

	input = rfi->instance->input;
	UINT32 toggle_keys_state = 0;

#if GTK_VERSION == 3
	manager = gdk_display_get_device_manager(gdk_display_get_default());
	keyboard = gdk_device_manager_get_client_pointer(manager);
	gdk_window_get_device_position(gdk_get_default_root_window(), keyboard, NULL, NULL, &state);
#else
	gdk_window_get_pointer(gdk_get_default_root_window(), NULL, NULL, &state);
#endif

	if (state & GDK_LOCK_MASK)
	{
		toggle_keys_state |= KBD_SYNC_CAPS_LOCK;
	}
	if (state & GDK_MOD2_MASK)
	{
		toggle_keys_state |= KBD_SYNC_NUM_LOCK;
	}
	if (state & GDK_MOD5_MASK)
	{
		toggle_keys_state |= KBD_SYNC_SCROLL_LOCK;
	}

	input->SynchronizeEvent(input, toggle_keys_state);
	input->KeyboardEvent(input, KBD_FLAGS_RELEASE, 0x0F);
}

static void remmina_rdp_event_event_push(RemminaProtocolWidget* gp, const RemminaPluginRdpEvent* e)
{
	rfContext* rfi;
	RemminaPluginRdpEvent* event;

	rfi = GET_DATA(gp);
	if ( !rfi )
		return;

	if (rfi->event_queue)
	{
		event = g_memdup(e, sizeof(RemminaPluginRdpEvent));
		g_async_queue_push(rfi->event_queue, event);

		if (write(rfi->event_pipe[1], "\0", 1))
		{
		}
	}
}

static void remmina_rdp_event_release_key(RemminaProtocolWidget* gp, DWORD scancode)
{
	gint i, k;
	rfContext* rfi;
	RemminaPluginRdpEvent rdp_event = { 0 };
	DWORD pressed_scancode;

	rfi = GET_DATA(gp);
	rdp_event.type = REMMINA_RDP_EVENT_TYPE_SCANCODE;

	if (scancode == 0)
	{
		/* Send all release key events for previously pressed keys */
		rdp_event.key_event.up = True;

		for (i = 0; i < rfi->pressed_keys->len; i++)
		{
			pressed_scancode = g_array_index(rfi->pressed_keys, DWORD, i);
			rdp_event.key_event.key_code = pressed_scancode & 0xFF;
			rdp_event.key_event.extended = pressed_scancode & 0x100;
			rdp_event.key_event.up = 1;
			remmina_rdp_event_event_push(gp, &rdp_event);
		}

		g_array_set_size(rfi->pressed_keys, 0);
	}
	else
	{
		/* Unregister the keycode only */
		for (i = 0; i < rfi->pressed_keys->len; i++)
		{
			k = g_array_index(rfi->pressed_keys, DWORD, i);

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

	if (!rfi->surface)
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

	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
		remmina_rdp_event_scale_area(gp, &x, &y, &w, &h);

	gtk_widget_queue_draw_area(rfi->drawing_area, x, y, w, h);
}

void remmina_rdp_event_update_rect(RemminaProtocolWidget* gp, gint x, gint y, gint w, gint h)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

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

	if (!rfi->surface)
		return FALSE;

	scale = remmina_plugin_service->protocol_plugin_get_scale(gp);

#if GTK_VERSION == 2
	x = event->area.x;
	y = event->area.y;

	context = gdk_cairo_create(gtk_widget_get_window (rfi->drawing_area));
	cairo_rectangle(context, x, y, event->area.width, event->area.height);
#endif

	if (scale)
		cairo_scale(context, rfi->scale_x, rfi->scale_y);

	cairo_set_source_surface(context, rfi->surface, 0, 0);

#if GTK_VERSION == 2
	cairo_fill(context);
	cairo_destroy(context);
#else
	cairo_set_operator (context, CAIRO_OPERATOR_SOURCE);	// Ignore alpha channel from FreeRDP
	cairo_paint(context);
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

static void remmina_rdp_event_translate_pos(RemminaProtocolWidget* gp, int ix, int iy, UINT16* ox, UINT16* oy)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	if ((rfi->scale) && (rfi->scale_width >= 1) && (rfi->scale_height >= 1))
	{
		*ox = (UINT16) (ix * remmina_plugin_service->protocol_plugin_get_width(gp) / rfi->scale_width);
		*oy = (UINT16) (iy * remmina_plugin_service->protocol_plugin_get_height(gp) / rfi->scale_height);
	}
	else
	{
		*ox = (UINT16) ix;
		*oy = (UINT16) iy;
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

#ifdef GDK_SCROLL_SMOOTH
		case GDK_SCROLL_SMOOTH:
			if (event->delta_y < 0)
				flag = PTR_FLAGS_WHEEL | 0x0078;
			if (event->delta_y > 0)
				flag = PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x0088;
			if (!flag)
				return FALSE;
			break;
#endif

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
	GdkDisplay* display;
	guint16 cooked_keycode;
	rfContext* rfi;
	RemminaPluginRdpEvent rdp_event;
	DWORD scancode;

	rfi = GET_DATA(gp);
	if ( !rfi ) return TRUE;

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
				scancode = freerdp_keyboard_get_rdp_scancode_from_x11_keycode(event->hardware_keycode);
				rdp_event.key_event.key_code = scancode & 0xFF;
				rdp_event.key_event.extended = scancode & 0x100;
				remmina_plugin_service->log_printf("[RDP]keyval=%02X keycode=%02X scancode=%02X extended=%s\n",
						event->keyval, event->hardware_keycode, rdp_event.key_event.key_code, rdp_event.key_event.extended ? "true" : "false");
			}
			else
			{
				//TODO: Port to GDK functions
				display = gdk_display_get_default();
				//cooked_keycode = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(display), XKeycodeToKeysym(GDK_DISPLAY_XDISPLAY(display), event->hardware_keycode, 0));
				cooked_keycode = XKeysymToKeycode(GDK_DISPLAY_XDISPLAY(display), XkbKeycodeToKeysym(GDK_DISPLAY_XDISPLAY(display), event->hardware_keycode, 0, 0));
				scancode = freerdp_keyboard_get_rdp_scancode_from_x11_keycode(cooked_keycode);
				rdp_event.key_event.key_code = scancode & 0xFF;
				rdp_event.key_event.extended = scancode & 0x100;
				remmina_plugin_service->log_printf("[RDP]keyval=%02X raw_keycode=%02X cooked_keycode=%02X scancode=%02X extended=%s\n",
						event->keyval, event->hardware_keycode, cooked_keycode, rdp_event.key_event.key_code, rdp_event.key_event.extended ? "true" : "false");
			}

			if (rdp_event.key_event.key_code)
				remmina_rdp_event_event_push(gp, &rdp_event);

			break;
	}

	/* Register/unregister the pressed key */
	if (rdp_event.key_event.key_code)
	{
		if (event->type == GDK_KEY_PRESS)
			g_array_append_val(rfi->pressed_keys, scancode);
		else
			remmina_rdp_event_release_key(gp, scancode);
	}

	return TRUE;
}

gboolean remmina_rdp_event_on_clipboard(GtkClipboard *clipboard, GdkEvent *event, RemminaProtocolWidget *gp)
{
	RemminaPluginRdpUiObject* ui;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_FORMATLIST;
	rf_queue_ui(gp, ui);

	return TRUE;
}

void remmina_rdp_event_init(RemminaProtocolWidget* gp)
{
	gchar* s;
	gint flags;
	rfContext* rfi;
	GtkClipboard* clipboard;

	rfi = GET_DATA(gp);
	if ( !rfi ) return;

	rfi->drawing_area = gtk_drawing_area_new();
	gtk_widget_show(rfi->drawing_area);
	gtk_container_add(GTK_CONTAINER(gp), rfi->drawing_area);

	gtk_widget_add_events(rfi->drawing_area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK
		| GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK | GDK_SCROLL_MASK | GDK_FOCUS_CHANGE_MASK);
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
	g_signal_connect(G_OBJECT(rfi->drawing_area), "focus-in-event",
		G_CALLBACK(remmina_rdp_event_on_focus_in), gp);

	RemminaFile* remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (!remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE))
	{
		clipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
		rfi->clipboard_handler = g_signal_connect(clipboard, "owner-change", G_CALLBACK(remmina_rdp_event_on_clipboard), gp);
	}

	rfi->pressed_keys = g_array_new(FALSE, TRUE, sizeof (DWORD));
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

	rfi->display = gdk_display_get_default();
	rfi->bpp = gdk_visual_get_best_depth();
}

void remmina_rdp_event_uninit(RemminaProtocolWidget* gp)
{
	rfContext* rfi;
	RemminaPluginRdpUiObject* ui;

	rfi = GET_DATA(gp);
	if ( !rfi ) return;

	/* unregister the clipboard monitor */
	if (rfi->clipboard_handler)
	{
		g_signal_handler_disconnect(G_OBJECT(gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD)), rfi->clipboard_handler);
		rfi->clipboard_handler = 0;
	}
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
	if (rfi->surface)
	{
		cairo_surface_destroy(rfi->surface);
		rfi->surface = NULL;
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

static void remmina_rdp_event_connected(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfContext* rfi;
	int stride;

	rfi = GET_DATA(gp);

	gtk_widget_realize(rfi->drawing_area);

	stride = cairo_format_stride_for_width(rfi->cairo_format, rfi->width);
	rfi->surface = cairo_image_surface_create_for_data((unsigned char*) rfi->primary_buffer, rfi->cairo_format, rfi->width, rfi->height, stride);
	gtk_widget_queue_draw_area(rfi->drawing_area, 0, 0, rfi->width, rfi->height);

	if (rfi->clipboard_handler)
	{
		remmina_rdp_event_on_clipboard(NULL, NULL, gp);
	}
	remmina_rdp_event_update_scale(gp);
}

static void remmina_rdp_event_create_cursor(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	GdkPixbuf* pixbuf;
	rfContext* rfi = GET_DATA(gp);
	rdpPointer* pointer = (rdpPointer*)ui->cursor.pointer;
#if GTK_VERSION == 3
	cairo_surface_t* surface;
	UINT8* data = malloc(pointer->width * pointer->height * 4);
#else
	guchar *data = g_malloc0(pointer->width * pointer->height * 4);
#endif

	freerdp_alpha_cursor_convert(data, pointer->xorMaskData, pointer->andMaskData, pointer->width, pointer->height, pointer->xorBpp, rfi->clrconv);
#if GTK_VERSION == 3
	surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, pointer->width, pointer->height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, pointer->width));
	pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, pointer->width, pointer->height);
	cairo_surface_destroy(surface);
#else
	pixbuf = gdk_pixbuf_new_from_data(data, GDK_COLORSPACE_RGB, TRUE, 8, pointer->width, pointer->height, (pointer->width * 4), NULL, NULL);
#endif
	((rfPointer*)ui->cursor.pointer)->cursor = gdk_cursor_new_from_pixbuf(rfi->display, pixbuf, pointer->xPos, pointer->yPos);
}

static void remmina_rdp_event_free_cursor(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfContext* rfi = GET_DATA(gp);

	g_mutex_lock(rfi->gmutex);
#if GTK_VERSION == 3
	g_object_unref(ui->cursor.pointer->cursor);
#else
	gdk_cursor_unref(ui->cursor.pointer->cursor);
#endif
	ui->cursor.pointer->cursor = NULL;
	g_cond_signal(rfi->gcond);
	g_mutex_unlock(rfi->gmutex);
}

static void remmina_rdp_event_cursor(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfContext* rfi = GET_DATA(gp);

	switch (ui->cursor.type)
	{
		case REMMINA_RDP_POINTER_NEW:
			remmina_rdp_event_create_cursor(gp, ui);
			break;

		case REMMINA_RDP_POINTER_FREE:
			remmina_rdp_event_free_cursor(gp, ui);
			break;

		case REMMINA_RDP_POINTER_SET:
			gdk_window_set_cursor(gtk_widget_get_window(rfi->drawing_area), ui->cursor.pointer->cursor);
			break;

		case REMMINA_RDP_POINTER_NULL:
			gdk_window_set_cursor(gtk_widget_get_window(rfi->drawing_area), gdk_cursor_new(GDK_BLANK_CURSOR));
			break;

		case REMMINA_RDP_POINTER_DEFAULT:
			gdk_window_set_cursor(gtk_widget_get_window(rfi->drawing_area), NULL);
			break;
	}
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

			case REMMINA_RDP_UI_CURSOR:
				remmina_rdp_event_cursor(gp, ui);
				break;

			case REMMINA_RDP_UI_CLIPBOARD:
				remmina_rdp_event_process_clipboard(gp, ui);
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
