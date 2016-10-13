/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_cliprdr.h"
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/locale/keyboard.h>
#include <X11/XKBlib.h>

static void remmina_rdp_event_on_focus_in(GtkWidget* widget, GdkEventKey* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_on_focus_in");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	rdpInput* input;
	GdkModifierType state;
	GdkDeviceManager *manager;
	GdkDevice *keyboard = NULL;

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;

	input = rfi->instance->input;
	UINT32 toggle_keys_state = 0;

	manager = gdk_display_get_device_manager(gdk_display_get_default());
	keyboard = gdk_device_manager_get_client_pointer(manager);
	gdk_window_get_device_position(gdk_get_default_root_window(), keyboard, NULL, NULL, &state);

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
	TRACE_CALL("remmina_rdp_event_event_push");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent* event;

	/* Here we push mouse/keyboard events in the event_queue */

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
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
	TRACE_CALL("remmina_rdp_event_release_key");
	gint i, k;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event = { 0 };
	DWORD pressed_scancode;

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
	TRACE_CALL("remmina_rdp_event_scale_area");
	gint width, height;
	gint sx, sy, sw, sh;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	if (!rfi || !rfi->connected || rfi->is_reconnecting || !rfi->surface)
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
	TRACE_CALL("remmina_rdp_event_update_region");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	gint x, y, w, h;

	x = ui->region.x;
	y = ui->region.y;
	w = ui->region.width;
	h = ui->region.height;

	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
		remmina_rdp_event_scale_area(gp, &x, &y, &w, &h);

	gtk_widget_queue_draw_area(rfi->drawing_area, x, y, w, h);
}

void remmina_rdp_event_update_rect(RemminaProtocolWidget* gp, gint x, gint y, gint w, gint h)
{
	TRACE_CALL("remmina_rdp_event_update_rect");
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	if (remmina_plugin_service->protocol_plugin_get_scale(gp))
		remmina_rdp_event_scale_area(gp, &x, &y, &w, &h);

	gtk_widget_queue_draw_area(rfi->drawing_area, x, y, w, h);
}

static void remmina_rdp_event_update_scale_factor(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_update_scale_factor");
	GtkAllocation a;
	gboolean scale;
	gint rdwidth, rdheight;
	gint gpwidth, gpheight;
	RemminaFile* remminafile;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	gtk_widget_get_allocation(GTK_WIDGET(gp), &a);
	gpwidth = a.width;
	gpheight = a.height;
	scale = remmina_plugin_service->protocol_plugin_get_scale(gp);

	if (scale)
	{
		if ((gpwidth > 1) && (gpheight > 1))
		{
			rdwidth = remmina_plugin_service->protocol_plugin_get_width(gp);
			rdheight = remmina_plugin_service->protocol_plugin_get_height(gp);

			rfi->scale_width = gpwidth;
			rfi->scale_height = gpheight;

			rfi->scale_x = (gdouble) rfi->scale_width / (gdouble) rdwidth;
			rfi->scale_y = (gdouble) rfi->scale_height / (gdouble) rdheight;
		}
	}
	else
	{
		rfi->scale_width = 0;
		rfi->scale_height = 0;
		rfi->scale_x = 0;
		rfi->scale_y = 0;
	}

	/* Now we have scaling vars calculated, resize drawing_area accordingly */

	if ((gpwidth > 1) && (gpheight > 1))
			gtk_widget_queue_draw_area(GTK_WIDGET(gp), 0, 0, gpwidth, gpheight);

}

static gboolean remmina_rdp_event_update_scale_factor_async(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_update_scale_factor_async");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	rfi->scale_handler = 0;
	remmina_rdp_event_update_scale_factor(gp);
	return FALSE;
}

static gboolean remmina_rdp_event_on_draw(GtkWidget* widget, cairo_t* context, RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_on_draw");
	gboolean scale;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	guint width, height;
	gchar *msg;
	cairo_text_extents_t extents;

	if (!rfi || !rfi->connected)
		return FALSE;


	if (rfi->is_reconnecting)
	{
		/* freerdp is reconnecting, just show a message to the user */

		width = gtk_widget_get_allocated_width(widget);
		height = gtk_widget_get_allocated_height (widget);

		/* Draw text */
		msg = g_strdup_printf(_("Reconnection in progress. Attempt %d of %d..."),
			rfi->reconnect_nattempt, rfi->reconnect_maxattempts);

		cairo_select_font_face(context, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(context, 24);
		cairo_set_source_rgb(context, 0.9, 0.9, 0.9);
		cairo_text_extents(context, msg, &extents);
		cairo_move_to(context, (width - (extents.width + extents.x_bearing)) / 2, (height - (extents.height + extents.y_bearing)) / 2);
		cairo_show_text(context, msg);

	}
	else
	{
		/* Standard drawing: we copy the surface from RDP */

		if (!rfi->surface)
			return FALSE;

		scale = remmina_plugin_service->protocol_plugin_get_scale(gp);

		if (scale)
			cairo_scale(context, rfi->scale_x, rfi->scale_y);

		cairo_set_source_surface(context, rfi->surface, 0, 0);

		cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);	// Ignore alpha channel from FreeRDP
		cairo_paint(context);
	}

	return TRUE;
}

static gboolean remmina_rdp_event_on_configure(GtkWidget* widget, GdkEventConfigure* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_on_configure");
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return FALSE;

	/* We do a delayed reallocating to improve performance */

	if (rfi->scale_handler)
		g_source_remove(rfi->scale_handler);

	rfi->scale_handler = g_timeout_add(300, (GSourceFunc) remmina_rdp_event_update_scale_factor_async, gp);

	return FALSE;
}

static void remmina_rdp_event_translate_pos(RemminaProtocolWidget* gp, int ix, int iy, UINT16* ox, UINT16* oy)
{
	TRACE_CALL("remmina_rdp_event_translate_pos");
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	/*
	 * Translate a position from local window coordinates (ix,iy) to
	 * RDP coordinates and put result on (*ox,*uy)
	 * */

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;

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

static void remmina_rdp_event_reverse_translate_pos_reverse(RemminaProtocolWidget* gp, int ix, int iy, int* ox, int* oy)
{
	TRACE_CALL("remmina_rdp_event_reverse_translate_pos_reverse");
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	/*
	 * Translate a position from RDP coordinates (ix,iy) to
	 * local window coordinates and put result on (*ox,*uy)
	 * */

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;

	if ((rfi->scale) && (rfi->scale_width >= 1) && (rfi->scale_height >= 1))
	{
		*ox = (ix * rfi->scale_width) / remmina_plugin_service->protocol_plugin_get_width(gp);
		*oy = (iy * rfi->scale_height) / remmina_plugin_service->protocol_plugin_get_height(gp);
	}
	else
	{
		*ox = ix;
		*oy = iy;
	}
}

static gboolean remmina_rdp_event_on_motion(GtkWidget* widget, GdkEventMotion* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_on_motion");
	RemminaPluginRdpEvent rdp_event = { 0 };

	rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;
	rdp_event.mouse_event.flags = PTR_FLAGS_MOVE;
	rdp_event.mouse_event.extended = FALSE;

	remmina_rdp_event_translate_pos(gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);
	remmina_rdp_event_event_push(gp, &rdp_event);

	return TRUE;
}

static gboolean remmina_rdp_event_on_button(GtkWidget* widget, GdkEventButton* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_on_button");
	gint flag;
	gboolean extended = FALSE;
	RemminaPluginRdpEvent rdp_event = { 0 };

	/* We bypass 2button-press and 3button-press events */
	if ((event->type != GDK_BUTTON_PRESS) && (event->type != GDK_BUTTON_RELEASE))
		return TRUE;

	flag = 0;

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
		case 8:		/* back */
		case 97:	/* Xming */
			extended = TRUE;
			flag |= PTR_XFLAGS_BUTTON1;
			break;
		case 9:		/* forward */
		case 112:	/* Xming */
			extended = TRUE;
			flag |= PTR_XFLAGS_BUTTON2;
			break;
		default:
			return FALSE;
	}

	if (event->type == GDK_BUTTON_PRESS)
	{
		if (extended)
			flag |= PTR_XFLAGS_DOWN;
		else
			flag |= PTR_FLAGS_DOWN;
	}

	rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;
	remmina_rdp_event_translate_pos(gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);

	if (flag != 0)
	{
		rdp_event.mouse_event.flags = flag;
		rdp_event.mouse_event.extended = extended;
		remmina_rdp_event_event_push(gp, &rdp_event);
	}

	return TRUE;
}

static gboolean remmina_rdp_event_on_scroll(GtkWidget* widget, GdkEventScroll* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_on_scroll");
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
	rdp_event.mouse_event.extended = FALSE;
	remmina_rdp_event_translate_pos(gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);
	remmina_rdp_event_event_push(gp, &rdp_event);

	return TRUE;
}

static gboolean remmina_rdp_event_on_key(GtkWidget* widget, GdkEventKey* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_on_key");
	GdkDisplay* display;
	KeyCode cooked_keycode;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event;
	DWORD scancode = 0;

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return FALSE;

#ifdef ENABLE_GTK_INSPECTOR_KEY
	/* GTK inspector key is propagated up. Disabled by default.
	 * enable it by defining ENABLE_GTK_INSPECTOR_KEY */
	if ( ( event->state & GDK_CONTROL_MASK ) != 0 && ( event->keyval == GDK_KEY_I || event->keyval == GDK_KEY_D ) ) {
		   return FALSE;
	}
#endif

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

gboolean remmina_rdp_event_on_clipboard(GtkClipboard *gtkClipboard, GdkEvent *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_rdp_event_on_clipboard");
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	rfClipboard* clipboard;

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return FALSE;

	clipboard = &(rfi->clipboard);

	if ( clipboard->sync ) {
		ui = g_new0(RemminaPluginRdpUiObject, 1);
		ui->type = REMMINA_RDP_UI_CLIPBOARD;
		ui->clipboard.clipboard = clipboard;
		ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_FORMATLIST;
		remmina_rdp_event_queue_ui(gp, ui);
	}

	return TRUE;
}

void remmina_rdp_event_init(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_init");
	gchar* s;
	gint flags;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	GtkClipboard* clipboard;

	if (!rfi) return;

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

	g_signal_connect(G_OBJECT(rfi->drawing_area), "draw",
		G_CALLBACK(remmina_rdp_event_on_draw), gp);
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
		rfi->clipboard.clipboard_handler = g_signal_connect(clipboard, "owner-change", G_CALLBACK(remmina_rdp_event_on_clipboard), gp);
	}

	rfi->pressed_keys = g_array_new(FALSE, TRUE, sizeof (DWORD));
	rfi->event_queue = g_async_queue_new_full(g_free);
	rfi->ui_queue = g_async_queue_new();
	pthread_mutex_init(&rfi->ui_queue_mutex, NULL);

	if (pipe(rfi->event_pipe))
	{
		g_print("Error creating pipes.\n");
		rfi->event_pipe[0] = -1;
		rfi->event_pipe[1] = -1;
		rfi->event_handle = NULL;
	}
	else
	{
		flags = fcntl(rfi->event_pipe[0], F_GETFL, 0);
		fcntl(rfi->event_pipe[0], F_SETFL, flags | O_NONBLOCK);
		rfi->event_handle = CreateFileDescriptorEvent(NULL, FALSE, FALSE, rfi->event_pipe[0], WINPR_FD_READ);
		if (!rfi->event_handle)
		{
			g_print("CreateFileDescriptorEvent() failed\n");
		}
	}

	rfi->object_table = g_hash_table_new_full(NULL, NULL, NULL, g_free);

	rfi->display = gdk_display_get_default();
	rfi->bpp = gdk_visual_get_best_depth();
}



void remmina_rdp_event_free_event(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* obj)
{
	TRACE_CALL("remmina_rdp_event_free_event");
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	switch (obj->type)
	{
		case REMMINA_RDP_UI_RFX:
			rfx_message_free(rfi->rfx_context, obj->rfx.message);
			break;

		case REMMINA_RDP_UI_NOCODEC:
			free(obj->nocodec.bitmap);
			break;

		default:
			break;
	}

	g_free(obj);
}


void remmina_rdp_event_uninit(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_uninit");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpUiObject* ui;

	if (!rfi) return;

	/* unregister the clipboard monitor */
	if (rfi->clipboard.clipboard_handler)
	{
		g_signal_handler_disconnect(G_OBJECT(gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD)), rfi->clipboard.clipboard_handler);
		rfi->clipboard.clipboard_handler = 0;
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
		remmina_rdp_event_free_event(gp, ui);
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
	pthread_mutex_destroy(&rfi->ui_queue_mutex);

	if (rfi->event_handle)
	{
		CloseHandle(rfi->event_handle);
		rfi->event_handle = NULL;
	}

	close(rfi->event_pipe[0]);
	close(rfi->event_pipe[1]);
}

static void remmina_rdp_event_create_cairo_surface(rfContext* rfi)
{
	int stride;
	if (rfi->surface) {
		cairo_surface_destroy(rfi->surface);
		rfi->surface = NULL;
	}
	stride = cairo_format_stride_for_width(rfi->cairo_format, rfi->width);
	rfi->surface = cairo_image_surface_create_for_data((unsigned char*) rfi->primary_buffer, rfi->cairo_format, rfi->width, rfi->height, stride);
}

void remmina_rdp_event_update_scale(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_update_scale");
	gint width, height;
	RemminaFile* remminafile;
	rdpGdi* gdi;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);

	/* See if we also must rellocate rfi->surface with different width and height,
	 * this usually happens after a DesktopResize RDP event*/
	if ( rfi->surface && (width != cairo_image_surface_get_width(rfi->surface) ||
		height != cairo_image_surface_get_height(rfi->surface) )) {
		/* Destroys and recreate rfi->surface with new width and height,
		 * calls gdi_resize and save new gdi->primary buffer pointer */
		if (rfi->surface) {
			cairo_surface_destroy(rfi->surface);
			rfi->surface = NULL;
		}
		rfi->width = width;
		rfi->height = height;
		gdi = ((rdpContext *)rfi)->gdi;
		gdi_resize(gdi, width, height);
		rfi->primary_buffer = gdi->primary_buffer;
		remmina_rdp_event_create_cairo_surface(rfi);
	}

	remmina_rdp_event_update_scale_factor(gp);

	if (rfi->scale)
	{
		/* In scaled mode, drawing_area will get its dimensions from its parent */
		gtk_widget_set_size_request(rfi->drawing_area, -1, -1 );
	}
	else
	{
		/* In non scaled mode, the plugins forces dimensions of drawing area */
		gtk_widget_set_size_request(rfi->drawing_area, width, height);
	}

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "update-align");
}

static void remmina_rdp_event_connected(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_event_connected");
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");

	gtk_widget_realize(rfi->drawing_area);

	remmina_rdp_event_create_cairo_surface(rfi);
	gtk_widget_queue_draw_area(rfi->drawing_area, 0, 0, rfi->width, rfi->height);

	if (rfi->clipboard.clipboard_handler)
	{
		remmina_rdp_event_on_clipboard(NULL, NULL, gp);
	}
	remmina_rdp_event_update_scale(gp);
}

static void remmina_rdp_event_reconnect_progress(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_event_reconnect_progress");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	gdk_window_invalidate_rect(gtk_widget_get_window(rfi->drawing_area), NULL, TRUE);
}

static BOOL remmina_rdp_event_create_cursor(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_event_create_cursor");
	GdkPixbuf* pixbuf;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	rdpPointer* pointer = (rdpPointer*)ui->cursor.pointer;
	cairo_surface_t* surface;
	UINT8* data = malloc(pointer->width * pointer->height * 4);

	if (freerdp_image_copy_from_pointer_data(
			(BYTE*) data, PIXEL_FORMAT_BGRA32,
			pointer->width * 4, 0, 0, pointer->width, pointer->height,
			pointer->xorMaskData, pointer->lengthXorMask,
			pointer->andMaskData, pointer->lengthAndMask,
			pointer->xorBpp, &(ui->cursor.context->gdi->palette)) < 0)
	{
		free(data);
		return FALSE;
	}

	surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, pointer->width, pointer->height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, pointer->width));
	pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, pointer->width, pointer->height);
	cairo_surface_destroy(surface);
	free(data);
	((rfPointer*)ui->cursor.pointer)->cursor = gdk_cursor_new_from_pixbuf(rfi->display, pixbuf, pointer->xPos, pointer->yPos);

	return TRUE;
}

static void remmina_rdp_event_free_cursor(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_event_free_cursor");
	g_object_unref(ui->cursor.pointer->cursor);
	ui->cursor.pointer->cursor = NULL;
}

static BOOL remmina_rdp_event_set_pointer_position(RemminaProtocolWidget *gp, gint x, gint y)
{
	TRACE_CALL("remmina_rdp_event_set_pointer_position");
	GdkWindow *w, *nw;
	gint nx, ny, wx, wy;
	GdkDeviceManager *manager;
	GdkDevice *dev;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	if (rfi == NULL)
		return FALSE;

	w = gtk_widget_get_window(rfi->drawing_area);
	manager = gdk_display_get_device_manager(gdk_display_get_default());
	dev = gdk_device_manager_get_client_pointer(manager);
	nw = gdk_device_get_window_at_position(dev, NULL, NULL);

	if (nw == w) {
		nx = 0;
		ny = 0;
		remmina_rdp_event_reverse_translate_pos_reverse(gp, x, y, &nx, &ny);
		gdk_window_get_root_coords(w, nx, ny, &wx, &wy);
		gdk_device_warp(dev, gdk_window_get_screen(w), wx, wy);
	}
	return TRUE;
}

static void remmina_rdp_event_cursor(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_event_cursor");
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	switch (ui->cursor.type)
	{
		case REMMINA_RDP_POINTER_NEW:
			ui->retval = remmina_rdp_event_create_cursor(gp, ui) ? 1 : 0;
			break;

		case REMMINA_RDP_POINTER_FREE:
			remmina_rdp_event_free_cursor(gp, ui);
			break;

		case REMMINA_RDP_POINTER_SET:
			gdk_window_set_cursor(gtk_widget_get_window(rfi->drawing_area), ui->cursor.pointer->cursor);
			ui->retval = 1;
			break;

		case REMMINA_RDP_POINTER_SETPOS:
			ui->retval = remmina_rdp_event_set_pointer_position(gp, ui->pos.x, ui->pos.y) ? 1 : 0;
			break;

		case REMMINA_RDP_POINTER_NULL:
			gdk_window_set_cursor(gtk_widget_get_window(rfi->drawing_area),
					gdk_cursor_new_for_display(gdk_display_get_default(),
						GDK_BLANK_CURSOR));
			ui->retval = 1;
			break;

		case REMMINA_RDP_POINTER_DEFAULT:
			gdk_window_set_cursor(gtk_widget_get_window(rfi->drawing_area), NULL);
			ui->retval = 1;
			break;
	}
}

static void remmina_rdp_ui_event_update_scale(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_ui_event_update_scale");
	remmina_rdp_event_update_scale(gp);
}

void remmina_rdp_event_unfocus(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_unfocus");
	remmina_rdp_event_release_key(gp, 0);
}

static void remmina_rdp_event_process_event(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_event_process_event");
	switch (ui->event.type)
	{
		case REMMINA_RDP_UI_EVENT_UPDATE_SCALE:
			remmina_rdp_ui_event_update_scale(gp, ui);
			break;
	}
}

static void remmina_rdp_event_process_ui_event(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_event_process_ui_event");
	switch (ui->type)
	{
		case REMMINA_RDP_UI_UPDATE_REGION:
			remmina_rdp_event_update_region(gp, ui);
			break;

		case REMMINA_RDP_UI_CONNECTED:
			remmina_rdp_event_connected(gp, ui);
			break;

		case REMMINA_RDP_UI_RECONNECT_PROGRESS:
			remmina_rdp_event_reconnect_progress(gp, ui);
			break;

		case REMMINA_RDP_UI_CURSOR:
			remmina_rdp_event_cursor(gp, ui);
			break;

		case REMMINA_RDP_UI_CLIPBOARD:
			remmina_rdp_event_process_clipboard(gp, ui);
			break;

		case REMMINA_RDP_UI_EVENT:
			remmina_rdp_event_process_event(gp,ui);
			break;

		default:
			break;
	}
}

static gboolean remmina_rdp_event_process_ui_queue(RemminaProtocolWidget* gp)
{
	TRACE_CALL("remmina_rdp_event_process_ui_queue");

	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpUiObject* ui;

	pthread_mutex_lock(&rfi->ui_queue_mutex);
	ui = (RemminaPluginRdpUiObject*) g_async_queue_try_pop(rfi->ui_queue);
	if (ui)
	{
		pthread_mutex_lock(&ui->sync_wait_mutex);
		if (!rfi->thread_cancelled)
		{
			remmina_rdp_event_process_ui_event(gp, ui);
		}
		// Should we signal the subthread to unlock ?
		if (ui->sync) {

			ui->complete = TRUE;
			pthread_cond_signal(&ui->sync_wait_cond);
			pthread_mutex_unlock(&ui->sync_wait_mutex);

		} else {
			remmina_rdp_event_free_event(gp, ui);
		}

		pthread_mutex_unlock(&rfi->ui_queue_mutex);

		return TRUE;
	}
	else
	{
		pthread_mutex_unlock(&rfi->ui_queue_mutex);
		rfi->ui_handler = 0;
		return FALSE;
	}
}

int remmina_rdp_event_queue_ui(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL("remmina_rdp_event_queue_ui");
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	gboolean ui_sync_save;
	int oldcanceltype;
	int rc;

	if (remmina_plugin_service->is_main_thread()) {
		/* Direct call */
		remmina_rdp_event_process_ui_event(gp, ui);
		rc = ui->retval;
		remmina_rdp_event_free_event(gp, ui);
		return rc;
	}

	if (rfi->thread_cancelled) {
		remmina_rdp_event_free_event(gp, ui);
		return 0;
	}

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldcanceltype);
	pthread_mutex_lock(&rfi->ui_queue_mutex);

	ui_sync_save = ui->sync;
	ui->complete = FALSE;

	if (ui_sync_save) {
		pthread_mutex_init(&ui->sync_wait_mutex,NULL);
		pthread_cond_init(&ui->sync_wait_cond, NULL);
	}

	ui->complete = FALSE;

	g_async_queue_push(rfi->ui_queue, ui);

	if (!rfi->ui_handler)
		rfi->ui_handler = IDLE_ADD((GSourceFunc) remmina_rdp_event_process_ui_queue, gp);

	if (ui_sync_save) {
		/* Wait for main thread function completion before returning */
		pthread_mutex_lock(&ui->sync_wait_mutex);
		pthread_mutex_unlock(&rfi->ui_queue_mutex);
		while(!ui->complete) {
			pthread_cond_wait(&ui->sync_wait_cond, &ui->sync_wait_mutex);
		}
		rc = ui->retval;
		pthread_cond_destroy(&ui->sync_wait_cond);
		pthread_mutex_destroy(&ui->sync_wait_mutex);
		remmina_rdp_event_free_event(gp, ui);
	} else {
		pthread_mutex_unlock(&rfi->ui_queue_mutex);
		rc = 0;
	}
	pthread_setcanceltype(oldcanceltype, NULL);
	return rc;
}
