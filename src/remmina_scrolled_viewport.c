/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee
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

#include <gtk/gtk.h>
#include "config.h"
#include "remmina_scrolled_viewport.h"
#include "remmina_pref.h"
#include "remmina/remmina_trace_calls.h"

G_DEFINE_TYPE( RemminaScrolledViewport, remmina_scrolled_viewport, GTK_TYPE_EVENT_BOX)

static void remmina_scrolled_viewport_get_preferred_width(GtkWidget* widget, gint* minimum_width, gint* natural_width)
{
	TRACE_CALL(__func__);
	/* Just return a fake small size, so gtk_window_fullscreen() will not fail
	 * because our content is too big*/
	if (minimum_width != NULL) *minimum_width = 100;
	if (natural_width != NULL) *natural_width = 100;
}

static void remmina_scrolled_viewport_get_preferred_height(GtkWidget* widget, gint* minimum_height, gint* natural_height)
{
	TRACE_CALL(__func__);
	/* Just return a fake small size, so gtk_window_fullscreen() will not fail
	 * because our content is too big*/
	if (minimum_height != NULL) *minimum_height = 100;
	if (natural_height != NULL) *natural_height = 100;
}

/* Event handler when mouse move on borders */
static gboolean remmina_scrolled_viewport_motion_timeout(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaScrolledViewport *gsv;
	GtkWidget *child;
	GdkDisplay *display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *device_manager;
#endif
	GdkDevice *pointer;
	GdkScreen *screen;
	GdkWindow *gsvwin;
	gint x, y, mx, my, w, h, rootx, rooty;
	GtkAdjustment *adj;
	gdouble value;

	if (!REMMINA_IS_SCROLLED_VIEWPORT(data))
		return FALSE;
	if (!GTK_IS_BIN(data))
		return FALSE;
	gsv = REMMINA_SCROLLED_VIEWPORT(data);
	if (!gsv->viewport_motion)
		return FALSE;
	child = gtk_bin_get_child(GTK_BIN(gsv));
	if (!GTK_IS_VIEWPORT(child))
		return FALSE;

	gsvwin = gtk_widget_get_window(GTK_WIDGET(gsv));
	if (!gsv)
		return FALSE;

	display = gdk_display_get_default();
	if (!display)
		return FALSE;
#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(display);
	pointer = gdk_seat_get_pointer(seat);
#else
	device_manager = gdk_display_get_device_manager(display);
	pointer = gdk_device_manager_get_client_pointer(device_manager);
#endif
	gdk_device_get_position(pointer, &screen, &x, &y);

	w = gdk_window_get_width(gsvwin) + SCROLL_BORDER_SIZE;   // Add 2px of black scroll border
	h = gdk_window_get_height(gsvwin) + SCROLL_BORDER_SIZE;  // Add 2px of black scroll border

	gdk_window_get_root_origin(gsvwin, &rootx, &rooty );

	x -= rootx;
	y -= rooty;

	mx = (x <= 0 ? -1 : (x >= w - 1 ? 1 : 0));
	my = (y <= 0 ? -1 : (y >= h - 1 ? 1 : 0));
	if (mx != 0) {
		gint step = MAX(10, MIN(remmina_pref.auto_scroll_step, w / 5));
		adj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(child));
		value = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj)) + (gdouble)(mx * step);
		value = MAX(0, MIN(value, gtk_adjustment_get_upper(GTK_ADJUSTMENT(adj)) - (gdouble)w + 2.0));
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), value);
	}
	if (my != 0) {
		gint step = MAX(10, MIN(remmina_pref.auto_scroll_step, h / 5));
		adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(child));
		value = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj)) + (gdouble)(my * step);
		value = MAX(0, MIN(value, gtk_adjustment_get_upper(GTK_ADJUSTMENT(adj)) - (gdouble)h + 2.0));
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), value);
	}
	return TRUE;
}

static gboolean remmina_scrolled_viewport_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	TRACE_CALL(__func__);
	remmina_scrolled_viewport_remove_motion(REMMINA_SCROLLED_VIEWPORT(widget));
	return FALSE;
}

static gboolean remmina_scrolled_viewport_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaScrolledViewport *gsv = REMMINA_SCROLLED_VIEWPORT(widget);
	gsv->viewport_motion = TRUE;
	gsv->viewport_motion_handler = g_timeout_add(20, remmina_scrolled_viewport_motion_timeout, gsv);
	return FALSE;
}

static void remmina_scrolled_viewport_destroy(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	remmina_scrolled_viewport_remove_motion(REMMINA_SCROLLED_VIEWPORT(widget));
}

static void remmina_scrolled_viewport_class_init(RemminaScrolledViewportClass *klass)
{
	TRACE_CALL(__func__);
	GtkWidgetClass *widget_class;
	widget_class = (GtkWidgetClass*)klass;

	widget_class->get_preferred_width = remmina_scrolled_viewport_get_preferred_width;
	widget_class->get_preferred_height = remmina_scrolled_viewport_get_preferred_height;

}

static void remmina_scrolled_viewport_init(RemminaScrolledViewport *gsv)
{
	TRACE_CALL(__func__);
}

void remmina_scrolled_viewport_remove_motion(RemminaScrolledViewport *gsv)
{
	TRACE_CALL(__func__);
	if (gsv->viewport_motion) {
		gsv->viewport_motion = FALSE;
		g_source_remove(gsv->viewport_motion_handler);
		gsv->viewport_motion_handler = 0;
	}
}

GtkWidget*
remmina_scrolled_viewport_new(void)
{
	TRACE_CALL(__func__);
	RemminaScrolledViewport *gsv;

	gsv = REMMINA_SCROLLED_VIEWPORT(g_object_new(REMMINA_TYPE_SCROLLED_VIEWPORT, NULL));

	gsv->viewport_motion = FALSE;
	gsv->viewport_motion_handler = 0;

	gtk_widget_set_size_request(GTK_WIDGET(gsv), 1, 1);
	gtk_widget_add_events(GTK_WIDGET(gsv), GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK);
	g_signal_connect(G_OBJECT(gsv), "destroy", G_CALLBACK(remmina_scrolled_viewport_destroy), NULL);
	g_signal_connect(G_OBJECT(gsv), "enter-notify-event", G_CALLBACK(remmina_scrolled_viewport_enter), NULL);
	g_signal_connect(G_OBJECT(gsv), "leave-notify-event", G_CALLBACK(remmina_scrolled_viewport_leave), NULL);

	return GTK_WIDGET(gsv);
}

