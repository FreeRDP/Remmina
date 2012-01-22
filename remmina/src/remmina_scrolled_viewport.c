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
#include "remmina_scrolled_viewport.h"
#include "remmina_pref.h"

G_DEFINE_TYPE( RemminaScrolledViewport, remmina_scrolled_viewport, GTK_TYPE_EVENT_BOX)

/* Event handler when mouse move on borders */
static gboolean remmina_scrolled_viewport_motion_timeout(gpointer data)
{
	RemminaScrolledViewport *gsv;
	GtkWidget *child;
	GdkDisplay *display;
	GdkScreen *screen;
	gint x, y, mx, my, w, h;
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

	display = gdk_display_get_default();
	if (!display)
		return FALSE;
	gdk_display_get_pointer(display, &screen, &x, &y, NULL);

	w = gdk_screen_get_width(screen);
	h = gdk_screen_get_height(screen);
	mx = (x == 0 ? -1 : (x >= w - 1 ? 1 : 0));
	my = (y == 0 ? -1 : (y >= h - 1 ? 1 : 0));
	if (mx != 0)
	{
		gint step = MAX(10, MIN(remmina_pref.auto_scroll_step, w / 5));
		adj = gtk_viewport_get_hadjustment(GTK_VIEWPORT(child));
		value = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj)) + (gdouble)(mx * step);
		value = MAX(0, MIN(value, gtk_adjustment_get_upper(GTK_ADJUSTMENT(adj)) - (gdouble) w + 2.0));
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), value);
	}
	if (my != 0)
	{
		gint step = MAX(10, MIN(remmina_pref.auto_scroll_step, h / 5));
		adj = gtk_viewport_get_vadjustment(GTK_VIEWPORT(child));
		value = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj)) + (gdouble)(my * step);
		value = MAX(0, MIN(value, gtk_adjustment_get_upper(GTK_ADJUSTMENT(adj)) - (gdouble) h + 2.0));
		gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), value);
	}
	return TRUE;
}

static gboolean remmina_scrolled_viewport_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	remmina_scrolled_viewport_remove_motion(REMMINA_SCROLLED_VIEWPORT(widget));
	return FALSE;
}

static gboolean remmina_scrolled_viewport_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer data)
{
	RemminaScrolledViewport *gsv = REMMINA_SCROLLED_VIEWPORT(widget);
	gsv->viewport_motion = TRUE;
	gsv->viewport_motion_handler = g_timeout_add(20, remmina_scrolled_viewport_motion_timeout, gsv);
	return FALSE;
}

static void remmina_scrolled_viewport_destroy(GtkWidget *widget, gpointer data)
{
	remmina_scrolled_viewport_remove_motion(REMMINA_SCROLLED_VIEWPORT(widget));
}

static void remmina_scrolled_viewport_class_init(RemminaScrolledViewportClass *klass)
{
}

static void remmina_scrolled_viewport_init(RemminaScrolledViewport *gsv)
{
}

void remmina_scrolled_viewport_remove_motion(RemminaScrolledViewport *gsv)
{
	if (gsv->viewport_motion)
	{
		gsv->viewport_motion = FALSE;
		g_source_remove(gsv->viewport_motion_handler);
		gsv->viewport_motion_handler = 0;
	}
}

GtkWidget*
remmina_scrolled_viewport_new(void)
{
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

