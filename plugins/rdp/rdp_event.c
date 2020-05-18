/*
 * Remmina - The GTK Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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
#include "rdp_settings.h"
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/locale/keyboard.h>

static gboolean remmina_rdp_event_on_focus_in(GtkWidget* widget, GdkEventKey* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	rdpInput* input;
	GdkModifierType state;

#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkDevice *keyboard = NULL;

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return FALSE;

	input = rfi->instance->input;
	UINT32 toggle_keys_state = 0;

#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(gdk_display_get_default());
	keyboard = gdk_seat_get_pointer(seat);
#else
	manager = gdk_display_get_device_manager(gdk_display_get_default());
	keyboard = gdk_device_manager_get_client_pointer(manager);
#endif
	gdk_window_get_device_position(gdk_get_default_root_window(), keyboard, NULL, NULL, &state);

	if (state & GDK_LOCK_MASK) {
		toggle_keys_state |= KBD_SYNC_CAPS_LOCK;
	}
	if (state & GDK_MOD2_MASK) {
		toggle_keys_state |= KBD_SYNC_NUM_LOCK;
	}
	if (state & GDK_MOD5_MASK) {
		toggle_keys_state |= KBD_SYNC_SCROLL_LOCK;
	}

	input->SynchronizeEvent(input, toggle_keys_state);
	input->KeyboardEvent(input, KBD_FLAGS_RELEASE, 0x0F);

	return FALSE;
}

void remmina_rdp_event_event_push(RemminaProtocolWidget* gp, const RemminaPluginRdpEvent* e)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent* event;

	/* Called by the main GTK thread to send an event to the libfreerdp thread */

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;

	if (rfi->event_queue) {
		event = g_memdup(e, sizeof(RemminaPluginRdpEvent));
		g_async_queue_push(rfi->event_queue, event);

		if (write(rfi->event_pipe[1], "\0", 1)) {
		}
	}
}

static void remmina_rdp_event_release_all_keys(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event = { 0 };
	int i;

	/* Send all release key events for previously pressed keys */
	for (i = 0; i < rfi->pressed_keys->len; i++) {
		rdp_event = g_array_index(rfi->pressed_keys, RemminaPluginRdpEvent, i);
		if ((rdp_event.type == REMMINA_RDP_EVENT_TYPE_SCANCODE ||
		     rdp_event.type == REMMINA_RDP_EVENT_TYPE_SCANCODE_UNICODE) &&
		    rdp_event.key_event.up == False) {
			rdp_event.key_event.up = True;
			remmina_rdp_event_event_push(gp, &rdp_event);
		}
	}

	g_array_set_size(rfi->pressed_keys, 0);
}

static void remmina_rdp_event_release_key(RemminaProtocolWidget* gp, RemminaPluginRdpEvent rdp_event)
{
	TRACE_CALL(__func__);
	gint i;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event_2 = { 0 };

	rdp_event_2.type = REMMINA_RDP_EVENT_TYPE_SCANCODE;

	if ((rdp_event.type == REMMINA_RDP_EVENT_TYPE_SCANCODE ||
	     rdp_event.type == REMMINA_RDP_EVENT_TYPE_SCANCODE_UNICODE) &&
	    rdp_event.key_event.up ) {
		/* Unregister the keycode only */
		for (i = 0; i < rfi->pressed_keys->len; i++) {
			rdp_event_2 = g_array_index(rfi->pressed_keys, RemminaPluginRdpEvent, i);

			if (rdp_event_2.key_event.key_code == rdp_event.key_event.key_code &&
			    rdp_event_2.key_event.unicode_code == rdp_event.key_event.unicode_code &&
			    rdp_event_2.key_event.extended == rdp_event.key_event.extended) {
				g_array_remove_index_fast(rfi->pressed_keys, i);
				break;
			}
		}
	}
}

static void keypress_list_add(RemminaProtocolWidget *gp, RemminaPluginRdpEvent rdp_event)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	if (!rdp_event.key_event.key_code)
		return;

	if (rdp_event.key_event.up) {
		remmina_rdp_event_release_key(gp, rdp_event);
	} else {
		g_array_append_val(rfi->pressed_keys, rdp_event);
	}

}


static void remmina_rdp_event_scale_area(RemminaProtocolWidget* gp, gint* x, gint* y, gint* w, gint* h)
{
	TRACE_CALL(__func__);
	gint width, height;
	gint sx, sy, sw, sh;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	if (!rfi || !rfi->connected || rfi->is_reconnecting || !rfi->surface)
		return;

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);

	if ((width == 0) || (height == 0))
		return;

	if ((rfi->scale_width == width) && (rfi->scale_height == height)) {
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

void remmina_rdp_event_update_regions(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	gint x, y, w, h, i;

	for(i = 0; i < ui->reg.ninvalid; i++) {
		x = ui->reg.ureg[i].x;
		y = ui->reg.ureg[i].y;
		w = ui->reg.ureg[i].w;
		h = ui->reg.ureg[i].h;

		if (rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED)
			remmina_rdp_event_scale_area(gp, &x, &y, &w, &h);

		gtk_widget_queue_draw_area(rfi->drawing_area, x, y, w, h);
	}
	g_free(ui->reg.ureg);
}

void remmina_rdp_event_update_rect(RemminaProtocolWidget* gp, gint x, gint y, gint w, gint h)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	if (rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED)
		remmina_rdp_event_scale_area(gp, &x, &y, &w, &h);

	gtk_widget_queue_draw_area(rfi->drawing_area, x, y, w, h);
}

static void remmina_rdp_event_update_scale_factor(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	GtkAllocation a;
	gint rdwidth, rdheight;
	gint gpwidth, gpheight;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	gtk_widget_get_allocation(GTK_WIDGET(gp), &a);
	gpwidth = a.width;
	gpheight = a.height;

	if (rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED) {
		if ((gpwidth > 1) && (gpheight > 1)) {
			rdwidth = remmina_plugin_service->protocol_plugin_get_width(gp);
			rdheight = remmina_plugin_service->protocol_plugin_get_height(gp);

			rfi->scale_width = gpwidth;
			rfi->scale_height = gpheight;

			rfi->scale_x = (gdouble)rfi->scale_width / (gdouble)rdwidth;
			rfi->scale_y = (gdouble)rfi->scale_height / (gdouble)rdheight;
		}
	}else  {
		rfi->scale_width = 0;
		rfi->scale_height = 0;
		rfi->scale_x = 0;
		rfi->scale_y = 0;
	}

}

static gboolean remmina_rdp_event_on_draw(GtkWidget* widget, cairo_t* context, RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	guint width, height;
	gchar *msg;
	cairo_text_extents_t extents;

	if (!rfi || !rfi->connected)
		return FALSE;


	if (rfi->is_reconnecting) {
		/* FreeRDP is reconnecting, just show a message to the user */

		width = gtk_widget_get_allocated_width(widget);
		height = gtk_widget_get_allocated_height(widget);

		/* Draw text */
		msg = g_strdup_printf(_("Reconnection attempt %d of %d…"),
			rfi->reconnect_nattempt, rfi->reconnect_maxattempts);

		cairo_select_font_face(context, "Sans", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD);
		cairo_set_font_size(context, 24);
		cairo_set_source_rgb(context, 0.9, 0.9, 0.9);
		cairo_text_extents(context, msg, &extents);
		cairo_move_to(context, (width - (extents.width + extents.x_bearing)) / 2, (height - (extents.height + extents.y_bearing)) / 2);
		cairo_show_text(context, msg);
		g_free(msg);

	}else  {
		/* Standard drawing: We copy the surface from RDP */

		if (!rfi->surface)
			return FALSE;

		if (rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED)
			cairo_scale(context, rfi->scale_x, rfi->scale_y);

		cairo_set_source_surface(context, rfi->surface, 0, 0);

		cairo_set_operator(context, CAIRO_OPERATOR_SOURCE);     // Ignore alpha channel from FreeRDP
		cairo_paint(context);
	}

	return TRUE;
}

static gboolean remmina_rdp_event_delayed_monitor_layout(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	RemminaPluginRdpEvent rdp_event = { 0 };
	GtkAllocation a;
	gint desktopOrientation, desktopScaleFactor, deviceScaleFactor;

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return FALSE;

	if (rfi->scale != REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES)
		return FALSE;

	rfi->delayed_monitor_layout_handler = 0;
	gint gpwidth, gpheight, prevwidth, prevheight;

	if (rfi->dispcontext && rfi->dispcontext->SendMonitorLayout) {
		remmina_rdp_settings_get_orientation_scale_prefs(&desktopOrientation, &desktopScaleFactor, &deviceScaleFactor);
		gtk_widget_get_allocation(GTK_WIDGET(gp), &a);
		gpwidth = a.width;
		gpheight = a.height;
		prevwidth = remmina_plugin_service->protocol_plugin_get_width(gp);
		prevheight = remmina_plugin_service->protocol_plugin_get_height(gp);

		if ((gpwidth != prevwidth || gpheight != prevheight) &&
		    gpwidth >= 200 && gpwidth < 8192 &&
		    gpheight >= 200 && gpheight < 8192
		    ) {
			if (rfi->rdpgfxchan) {
				/* Workaround for FreeRDP issue #5417 */
				if (gpwidth < AVC_MIN_DESKTOP_WIDTH)
					gpwidth = AVC_MIN_DESKTOP_WIDTH;
				if (gpheight < AVC_MIN_DESKTOP_HEIGHT)
					gpheight = AVC_MIN_DESKTOP_HEIGHT;
			}
			rdp_event.type = REMMINA_RDP_EVENT_TYPE_SEND_MONITOR_LAYOUT;
			rdp_event.monitor_layout.width = gpwidth;
			rdp_event.monitor_layout.height = gpheight;
			rdp_event.monitor_layout.desktopOrientation = desktopOrientation;
			rdp_event.monitor_layout.desktopScaleFactor = desktopScaleFactor;
			rdp_event.monitor_layout.deviceScaleFactor = deviceScaleFactor;
			remmina_rdp_event_event_push(gp, &rdp_event);
		}
	}

	return FALSE;
}

void remmina_rdp_event_send_delayed_monitor_layout(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;
	if (rfi->delayed_monitor_layout_handler) {
		g_source_remove(rfi->delayed_monitor_layout_handler);
		rfi->delayed_monitor_layout_handler = 0;
	}
	if (rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES) {
		rfi->delayed_monitor_layout_handler = g_timeout_add(500, (GSourceFunc)remmina_rdp_event_delayed_monitor_layout, gp);
	}

}

static gboolean remmina_rdp_event_on_configure(GtkWidget* widget, GdkEventConfigure* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	/* Called when gp changes its size or position */

	rfContext* rfi = GET_PLUGIN_DATA(gp);
	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return FALSE;

	remmina_rdp_event_update_scale_factor(gp);

	/* If the scaler is not active, schedule a delayed remote resolution change */
	remmina_rdp_event_send_delayed_monitor_layout(gp);


	return FALSE;
}

static void remmina_rdp_event_translate_pos(RemminaProtocolWidget* gp, int ix, int iy, UINT16* ox, UINT16* oy)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	/*
	 * Translate a position from local window coordinates (ix,iy) to
	 * RDP coordinates and put result on (*ox,*uy)
	 * */

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;

	if ((rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED) && (rfi->scale_width >= 1) && (rfi->scale_height >= 1)) {
		*ox = (UINT16)(ix * remmina_plugin_service->protocol_plugin_get_width(gp) / rfi->scale_width);
		*oy = (UINT16)(iy * remmina_plugin_service->protocol_plugin_get_height(gp) / rfi->scale_height);
	}else  {
		*ox = (UINT16)ix;
		*oy = (UINT16)iy;
	}
}

static void remmina_rdp_event_reverse_translate_pos_reverse(RemminaProtocolWidget* gp, int ix, int iy, int* ox, int* oy)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	/*
	 * Translate a position from RDP coordinates (ix,iy) to
	 * local window coordinates and put result on (*ox,*uy)
	 * */

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;

	if ((rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED) && (rfi->scale_width >= 1) && (rfi->scale_height >= 1)) {
		*ox = (ix * rfi->scale_width) / remmina_plugin_service->protocol_plugin_get_width(gp);
		*oy = (iy * rfi->scale_height) / remmina_plugin_service->protocol_plugin_get_height(gp);
	}else  {
		*ox = ix;
		*oy = iy;
	}
}

static gboolean remmina_rdp_event_on_motion(GtkWidget* widget, GdkEventMotion* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
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
	TRACE_CALL(__func__);
	gint flag;
	gboolean extended = FALSE;
	RemminaPluginRdpEvent rdp_event = { 0 };

	/* We bypass 2button-press and 3button-press events */
	if ((event->type != GDK_BUTTON_PRESS) && (event->type != GDK_BUTTON_RELEASE))
		return TRUE;

	flag = 0;

	switch (event->button) {
	case 1:
		flag |= PTR_FLAGS_BUTTON1;
		break;
	case 2:
		flag |= PTR_FLAGS_BUTTON3;
		break;
	case 3:
		flag |= PTR_FLAGS_BUTTON2;
		break;
	case 8:                 /* back */
	case 97:                /* Xming */
		extended = TRUE;
		flag |= PTR_XFLAGS_BUTTON1;
		break;
	case 9:                 /* forward */
	case 112:               /* Xming */
		extended = TRUE;
		flag |= PTR_XFLAGS_BUTTON2;
		break;
	default:
		return FALSE;
	}

	if (event->type == GDK_BUTTON_PRESS) {
		if (extended)
			flag |= PTR_XFLAGS_DOWN;
		else
			flag |= PTR_FLAGS_DOWN;
	}

	rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;
	remmina_rdp_event_translate_pos(gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);

	if (flag != 0) {
		rdp_event.mouse_event.flags = flag;
		rdp_event.mouse_event.extended = extended;
		remmina_rdp_event_event_push(gp, &rdp_event);
	}

	return TRUE;
}

static gboolean remmina_rdp_event_on_scroll(GtkWidget* widget, GdkEventScroll* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	gint flag;
	RemminaPluginRdpEvent rdp_event = { 0 };

	flag = 0;
	rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;

	switch (event->direction) {
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

static void remmina_rdp_event_init_keymap(rfContext* rfi, const gchar* strmap)
{
	long int v1, v2;
	const char *s;
	char *endptr;
	RemminaPluginRdpKeymapEntry ke;

	if (strmap == NULL || strmap[0] == 0) {
		rfi->keymap = NULL;
		return;
	}
	s = strmap;
	rfi->keymap = g_array_new(FALSE, TRUE, sizeof(RemminaPluginRdpKeymapEntry));
	while(1) {
		v1 = strtol(s, &endptr, 10);
		if (endptr == s) break;
		s = endptr;
		if (*s != ':') break;
		s++;
		v2 = strtol(s, &endptr, 10);
		if (endptr == s) break;
		s = endptr;
		ke.orig_keycode = v1 & 0x7fffffff;
		ke.translated_keycode = v2 & 0x7fffffff;
		g_array_append_val(rfi->keymap, ke);
		if (*s != ',') break;
		s++;
	}
	if (rfi->keymap->len == 0) {
		g_array_unref(rfi->keymap);
		rfi->keymap = NULL;
	}

}

static gboolean remmina_rdp_event_on_key(GtkWidget* widget, GdkEventKey* event, RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	guint32 unicode_keyval;
	guint16 hardware_keycode;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event;
	RemminaPluginRdpKeymapEntry* kep;
	DWORD scancode = 0;
	int ik;

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

	switch (event->keyval) {
	case GDK_KEY_Pause:
		/*
		 * See https://msdn.microsoft.com/en-us/library/cc240584.aspx
		 * 2.2.8.1.1.3.1.1.1 Keyboard Event (TS_KEYBOARD_EVENT)
		 * for pause key management
		 */
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
		if (!rfi->use_client_keymap) {
			hardware_keycode = event->hardware_keycode;
			if (rfi->keymap) {
				for(ik = 0; ik < rfi->keymap->len; ik++) {
					kep = &g_array_index(rfi->keymap, RemminaPluginRdpKeymapEntry, ik);
					if (hardware_keycode == kep->orig_keycode) {
						hardware_keycode = kep->translated_keycode;
						break;
					}
				}
			}
			scancode = freerdp_keyboard_get_rdp_scancode_from_x11_keycode(hardware_keycode);
			if (scancode) {
				rdp_event.key_event.key_code = scancode & 0xFF;
				rdp_event.key_event.extended = scancode & 0x100;
				remmina_rdp_event_event_push(gp, &rdp_event);
				keypress_list_add(gp, rdp_event);
			}
		} else {
			unicode_keyval = gdk_keyval_to_unicode(event->keyval);
			/* Decide when whe should send a keycode or a Unicode character.
			 * - All non char keys (Shift, Alt, Super) should be sent as keycode
			 * - Space should be sent as keycode (see issue #1364)
			 * - All special keys (F1-F10, numeric pad, Home/End/Arrows/PgUp/PgDn/Insert/Delete) keycode
			 * - All key pressed while Ctrl or Alt or Super is down are not decoded by gdk_keyval_to_unicode(), so send it as keycode
			 * - All keycodes not translatable to unicode chars, as keycode
			 * - The rest as Unicode char
			 */
			if (event->keyval >= 0xfe00 ||                                                  // Arrows, Shift, Alt, Fn, num keypad…
				event->hardware_keycode == 0x41 ||											// Spacebar
			    unicode_keyval == 0 ||                                                      // Impossible to translate
			    (event->state & (GDK_MOD1_MASK | GDK_CONTROL_MASK | GDK_SUPER_MASK)) != 0   // A modifier not recognized by gdk_keyval_to_unicode()
			    ) {
				scancode = freerdp_keyboard_get_rdp_scancode_from_x11_keycode(event->hardware_keycode);
				rdp_event.key_event.key_code = scancode & 0xFF;
				rdp_event.key_event.extended = scancode & 0x100;
				if (rdp_event.key_event.key_code) {
					remmina_rdp_event_event_push(gp, &rdp_event);
					keypress_list_add(gp, rdp_event);
				}
			} else {
				rdp_event.type = REMMINA_RDP_EVENT_TYPE_SCANCODE_UNICODE;
				rdp_event.key_event.unicode_code = unicode_keyval;
				rdp_event.key_event.extended = False;
				remmina_rdp_event_event_push(gp, &rdp_event);
				keypress_list_add(gp, rdp_event);
			}
		}
		break;
	}

	return TRUE;
}

gboolean remmina_rdp_event_on_clipboard(GtkClipboard *gtkClipboard, GdkEvent *event, RemminaProtocolWidget *gp)
{
	/* Signal handler for GTK clipboard owner-change */
	TRACE_CALL(__func__);
	RemminaPluginRdpEvent rdp_event = { 0 };
	CLIPRDR_FORMAT_LIST* pFormatList;

	/* Usually "owner-change" is fired when a user pres "COPY" on the client
	 * OR when this plugin calls gtk_clipboard_set_with_owner()
	 * after receivina a RDP server format list in remmina_rdp_cliprdr_server_format_list()
	 * In the latter case, we must ignore owner change */

	remmina_plugin_service->debug("owner-change event received");

	rfContext *rfi = GET_PLUGIN_DATA(gp);
	if (rfi)
		remmina_rdp_clipboard_abort_transfer(rfi);

	if (gtk_clipboard_get_owner(gtkClipboard) != (GObject*)gp) {
		/* To do: avoid this when the new owner is another remmina protocol widget of
		 * the same remmina application */
		remmina_plugin_service->debug("     new owner is different than me: new=%p me=%p. Sending local clipboard format list to server.",
				gtk_clipboard_get_owner(gtkClipboard), (GObject*)gp);

		pFormatList = remmina_rdp_cliprdr_get_client_format_list(gp);
		rdp_event.type = REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_LIST;
		rdp_event.clipboard_formatlist.pFormatList = pFormatList;
		remmina_rdp_event_event_push(gp, &rdp_event);
	} else {
		remmina_plugin_service->debug("    ... but I'm the owner!");
	}
	return TRUE;
}

void remmina_rdp_event_init(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	gchar* s;
	gint flags;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	GtkClipboard* clipboard;
	RemminaFile* remminafile;

	if (!rfi) return;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

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

	/* Read special keymap from profile file, if exists */
	remmina_rdp_event_init_keymap(rfi, remmina_plugin_service->pref_get_value("rdp_map_keycode"));

	if (rfi->use_client_keymap && rfi->keymap) {
		fprintf(stderr, "RDP profile error: you cannot define both rdp_map_hardware_keycode and have 'Use client keuboard mapping' enabled\n");
	}

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

	if (!remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE)) {
		clipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
		rfi->clipboard.clipboard_handler = g_signal_connect(clipboard, "owner-change", G_CALLBACK(remmina_rdp_event_on_clipboard), gp);
	}

	rfi->pressed_keys = g_array_new(FALSE, TRUE, sizeof(RemminaPluginRdpEvent));
	rfi->event_queue = g_async_queue_new_full(g_free);
	rfi->ui_queue = g_async_queue_new();
	pthread_mutex_init(&rfi->ui_queue_mutex, NULL);

	if (pipe(rfi->event_pipe)) {
		g_print("Error creating pipes.\n");
		rfi->event_pipe[0] = -1;
		rfi->event_pipe[1] = -1;
		rfi->event_handle = NULL;
	}else  {
		flags = fcntl(rfi->event_pipe[0], F_GETFL, 0);
		fcntl(rfi->event_pipe[0], F_SETFL, flags | O_NONBLOCK);
		rfi->event_handle = CreateFileDescriptorEvent(NULL, FALSE, FALSE, rfi->event_pipe[0], WINPR_FD_READ);
		if (!rfi->event_handle) {
			g_print("CreateFileDescriptorEvent() failed\n");
		}
	}

	rfi->object_table = g_hash_table_new_full(NULL, NULL, NULL, g_free);

	rfi->display = gdk_display_get_default();

#if GTK_CHECK_VERSION(3, 22, 0)
	GdkVisual *visual = gdk_screen_get_system_visual(
			gdk_display_get_default_screen(rfi->display));
	rfi->bpp = gdk_visual_get_depth (visual);
#else
	rfi->bpp = gdk_visual_get_best_depth();
#endif
}



void remmina_rdp_event_free_event(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* obj)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	switch (obj->type) {
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
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpUiObject* ui;

	if (!rfi) return;

	/* unregister the clipboard monitor */
	if (rfi->clipboard.clipboard_handler) {
		g_signal_handler_disconnect(G_OBJECT(gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD)), rfi->clipboard.clipboard_handler);
		rfi->clipboard.clipboard_handler = 0;
	}
	if (rfi->delayed_monitor_layout_handler) {
		g_source_remove(rfi->delayed_monitor_layout_handler);
		rfi->delayed_monitor_layout_handler = 0;
	}
	if (rfi->ui_handler) {
		g_source_remove(rfi->ui_handler);
		rfi->ui_handler = 0;
	}
	while ((ui = (RemminaPluginRdpUiObject*)g_async_queue_try_pop(rfi->ui_queue)) != NULL) {
		remmina_rdp_event_free_event(gp, ui);
	}
	if (rfi->surface) {
		cairo_surface_destroy(rfi->surface);
		rfi->surface = NULL;
	}

	g_hash_table_destroy(rfi->object_table);

	g_array_free(rfi->pressed_keys, TRUE);
	if (rfi->keymap) {
		g_array_free(rfi->keymap, TRUE);
		rfi->keymap = NULL;
	}
	g_async_queue_unref(rfi->event_queue);
	rfi->event_queue = NULL;
	g_async_queue_unref(rfi->ui_queue);
	rfi->ui_queue = NULL;
	pthread_mutex_destroy(&rfi->ui_queue_mutex);

	if (rfi->event_handle) {
		CloseHandle(rfi->event_handle);
		rfi->event_handle = NULL;
	}

	close(rfi->event_pipe[0]);
	close(rfi->event_pipe[1]);
}

static void remmina_rdp_event_create_cairo_surface(rfContext* rfi)
{
	int stride;
	rdpGdi* gdi;

	if (!rfi)
		return;

	gdi = ((rdpContext *)rfi)->gdi;

	if (!gdi)
		return;

	if (rfi->surface) {
		cairo_surface_destroy(rfi->surface);
		rfi->surface = NULL;
	}
	stride = cairo_format_stride_for_width(rfi->cairo_format, gdi->width);
	rfi->surface = cairo_image_surface_create_for_data((unsigned char*)gdi->primary_buffer, rfi->cairo_format, gdi->width, gdi->height, stride);
}

void remmina_rdp_event_update_scale(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	gint width, height;
	rdpGdi* gdi;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	width = remmina_plugin_service->protocol_plugin_get_width(gp);
	height = remmina_plugin_service->protocol_plugin_get_height(gp);

	gdi = ((rdpContext*)rfi)->gdi;

	rfi->scale = remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp);

	/* See if we also must rellocate rfi->surface with different width and height,
	 * this usually happens after a DesktopResize RDP event*/

	if ( rfi->surface && (cairo_image_surface_get_width(rfi->surface) != gdi->width ||
		cairo_image_surface_get_height(rfi->surface) != gdi->height) ) {
		/* Destroys and recreate rfi->surface with new width and height */
		cairo_surface_destroy(rfi->surface);
		rfi->surface = NULL;
		remmina_rdp_event_create_cairo_surface(rfi);
	} else if ( rfi->surface == NULL ) {
		remmina_rdp_event_create_cairo_surface(rfi);
	}

	/* Send gdi->width and gdi->height obtained from remote server to gp plugin,
	 * so they will be saved when closing connection */
	if (width != gdi->width)
		remmina_plugin_service->protocol_plugin_set_width(gp, gdi->width);
	if (height != gdi->height)
		remmina_plugin_service->protocol_plugin_set_height(gp, gdi->height);

	remmina_rdp_event_update_scale_factor(gp);

	if (rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED || rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES) {
		/* In scaled mode and autores mode, drawing_area will get its dimensions from its parent */
		gtk_widget_set_size_request(rfi->drawing_area, -1, -1 );
	}else  {
		/* In non scaled mode, the plugins forces dimensions of drawing area */
		gtk_widget_set_size_request(rfi->drawing_area, width, height);
	}
	remmina_plugin_service->protocol_plugin_update_align(gp);
}

static void remmina_rdp_event_connected(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	rdpGdi* gdi;

	gdi = ((rdpContext *)rfi)->gdi;

	gtk_widget_realize(rfi->drawing_area);

	remmina_rdp_event_create_cairo_surface(rfi);
	gtk_widget_queue_draw_area(rfi->drawing_area, 0, 0, gdi->width, gdi->height);

	remmina_rdp_event_update_scale(gp);

    remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);
}

static void remmina_rdp_event_reconnect_progress(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	gdk_window_invalidate_rect(gtk_widget_get_window(rfi->drawing_area), NULL, TRUE);
}

static BOOL remmina_rdp_event_create_cursor(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	GdkPixbuf* pixbuf;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	rdpPointer* pointer = (rdpPointer*)ui->cursor.pointer;
	cairo_surface_t* surface;
	UINT8* data = malloc(pointer->width * pointer->height * 4);

	if (freerdp_image_copy_from_pointer_data(
		    (BYTE*)data, PIXEL_FORMAT_BGRA32,
		    pointer->width * 4, 0, 0, pointer->width, pointer->height,
		    pointer->xorMaskData, pointer->lengthXorMask,
		    pointer->andMaskData, pointer->lengthAndMask,
		    pointer->xorBpp, &(ui->cursor.context->gdi->palette)) < 0) {
		free(data);
		return FALSE;
	}

	surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_ARGB32, pointer->width, pointer->height, cairo_format_stride_for_width(CAIRO_FORMAT_ARGB32, pointer->width));
	pixbuf = gdk_pixbuf_get_from_surface(surface, 0, 0, pointer->width, pointer->height);
	cairo_surface_destroy(surface);
	free(data);
	((rfPointer*)ui->cursor.pointer)->cursor = gdk_cursor_new_from_pixbuf(rfi->display, pixbuf, pointer->xPos, pointer->yPos);
	g_object_unref(pixbuf);

	return TRUE;
}

static void remmina_rdp_event_free_cursor(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	g_object_unref(ui->cursor.pointer->cursor);
	ui->cursor.pointer->cursor = NULL;
}

static BOOL remmina_rdp_event_set_pointer_position(RemminaProtocolWidget *gp, gint x, gint y)
{
	TRACE_CALL(__func__);
	GdkWindow *w, *nw;
	gint nx, ny, wx, wy;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkDevice *dev;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	if (rfi == NULL)
		return FALSE;

	w = gtk_widget_get_window(rfi->drawing_area);
#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(gdk_display_get_default());
	dev = gdk_seat_get_pointer(seat);
#else
	manager = gdk_display_get_device_manager(gdk_display_get_default());
	dev = gdk_device_manager_get_client_pointer(manager);
#endif

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
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	switch (ui->cursor.type) {
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
	TRACE_CALL(__func__);
	remmina_rdp_event_update_scale(gp);
}

void remmina_rdp_event_unfocus(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;
	remmina_rdp_event_release_all_keys(gp);
}

static void remmina_rdp_ui_event_destroy_cairo_surface(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	cairo_surface_destroy(rfi->surface);
	rfi->surface = NULL;
}

static void remmina_rdp_event_process_event(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	switch (ui->event.type) {
	case REMMINA_RDP_UI_EVENT_UPDATE_SCALE:
		remmina_rdp_ui_event_update_scale(gp, ui);
		break;
	case REMMINA_RDP_UI_EVENT_DESTROY_CAIRO_SURFACE:
		remmina_rdp_ui_event_destroy_cairo_surface(gp, ui);
		break;
	}
}

static void remmina_rdp_event_process_ui_event(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	switch (ui->type) {
	case REMMINA_RDP_UI_UPDATE_REGIONS:
		remmina_rdp_event_update_regions(gp, ui);
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
		remmina_rdp_event_process_event(gp, ui);
		break;

	default:
		break;
	}
}

static gboolean remmina_rdp_event_process_ui_queue(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);

	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpUiObject* ui;

	pthread_mutex_lock(&rfi->ui_queue_mutex);
	ui = (RemminaPluginRdpUiObject*)g_async_queue_try_pop(rfi->ui_queue);
	if (ui) {
		pthread_mutex_lock(&ui->sync_wait_mutex);
		if (!rfi->thread_cancelled) {
			remmina_rdp_event_process_ui_event(gp, ui);
		}
		// Should we signal the caller thread to unlock ?
		if (ui->sync) {
			ui->complete = TRUE;
			pthread_cond_signal(&ui->sync_wait_cond);
			pthread_mutex_unlock(&ui->sync_wait_mutex);

		} else {
			remmina_rdp_event_free_event(gp, ui);
		}

		pthread_mutex_unlock(&rfi->ui_queue_mutex);
		return TRUE;
	}else  {
		rfi->ui_handler = 0;
		pthread_mutex_unlock(&rfi->ui_queue_mutex);
		return FALSE;
	}
}

static void remmina_rdp_event_queue_ui(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	gboolean ui_sync_save;
	int oldcanceltype;

	if (rfi->thread_cancelled) {
		return;
	}

	if (remmina_plugin_service->is_main_thread()) {
		remmina_rdp_event_process_ui_event(gp, ui);
		return;
	}

	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldcanceltype);

	pthread_mutex_lock(&rfi->ui_queue_mutex);

	ui_sync_save = ui->sync;
	ui->complete = FALSE;

	if (ui_sync_save) {
		pthread_mutex_init(&ui->sync_wait_mutex, NULL);
		pthread_cond_init(&ui->sync_wait_cond, NULL);
	}

	ui->complete = FALSE;

	g_async_queue_push(rfi->ui_queue, ui);

	if (!rfi->ui_handler) {
		rfi->ui_handler = IDLE_ADD((GSourceFunc)remmina_rdp_event_process_ui_queue, gp);
	}

	if (ui_sync_save) {
		/* Wait for main thread function completion before returning */
		pthread_mutex_lock(&ui->sync_wait_mutex);
		pthread_mutex_unlock(&rfi->ui_queue_mutex);
		while (!ui->complete) {
			pthread_cond_wait(&ui->sync_wait_cond, &ui->sync_wait_mutex);
		}
		pthread_cond_destroy(&ui->sync_wait_cond);
		pthread_mutex_destroy(&ui->sync_wait_mutex);
	} else {
		pthread_mutex_unlock(&rfi->ui_queue_mutex);
	}
	pthread_setcanceltype(oldcanceltype, NULL);
}

void remmina_rdp_event_queue_ui_async(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	ui->sync = FALSE;
	remmina_rdp_event_queue_ui(gp, ui);
}

int remmina_rdp_event_queue_ui_sync_retint(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	int retval;
	ui->sync = TRUE;
	remmina_rdp_event_queue_ui(gp, ui);
	retval = ui->retval;
	remmina_rdp_event_free_event(gp, ui);
	return retval;
}

void *remmina_rdp_event_queue_ui_sync_retptr(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	void *rp;
	ui->sync = TRUE;
	remmina_rdp_event_queue_ui(gp, ui);
	rp = ui->retptr;
	remmina_rdp_event_free_event(gp, ui);
	return rp;
}
