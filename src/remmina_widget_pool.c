/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee
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

#include <gtk/gtk.h>
#include <gmodule.h>
#include "remmina_public.h"
#include "remmina_widget_pool.h"
#include "remmina/remmina_trace_calls.h"

static GPtrArray *remmina_widget_pool = NULL;

void remmina_widget_pool_init(void)
{
	TRACE_CALL(__func__);
	remmina_widget_pool = g_ptr_array_new();
}

static void remmina_widget_pool_on_widget_destroy(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	g_ptr_array_remove(remmina_widget_pool, widget);
}

void remmina_widget_pool_register(GtkWidget *widget)
{
	TRACE_CALL(__func__);
	g_ptr_array_add(remmina_widget_pool, widget);
	g_signal_connect(G_OBJECT(widget), "destroy", G_CALLBACK(remmina_widget_pool_on_widget_destroy), NULL);
}

GtkWidget*
remmina_widget_pool_find(GType type, const gchar *tag)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	gint i;

	if (remmina_widget_pool == NULL)
		return NULL;

	for (i = 0; i < remmina_widget_pool->len; i++) {
		widget = GTK_WIDGET(g_ptr_array_index(remmina_widget_pool, i));
		if (!G_TYPE_CHECK_INSTANCE_TYPE(widget, type))
			continue;
		if (tag && g_strcmp0((const gchar*)g_object_get_data(G_OBJECT(widget), "tag"), tag) != 0)
			continue;
		return widget;
	}
	return NULL;
}

GtkWidget*
remmina_widget_pool_find_by_window(GType type, GdkWindow *window)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	gint i;
	GdkWindow *parent;

	if (window == NULL || remmina_widget_pool == NULL)
		return NULL;

	for (i = 0; i < remmina_widget_pool->len; i++) {
		widget = GTK_WIDGET(g_ptr_array_index(remmina_widget_pool, i));
		if (!G_TYPE_CHECK_INSTANCE_TYPE(widget, type))
			continue;
		/* gdk_window_get_toplevel won’t work here, if the window is an embedded client. So we iterate the window tree */
		for (parent = window; parent && parent != GDK_WINDOW_ROOT; parent = gdk_window_get_parent(parent)) {
			if (gtk_widget_get_window(widget) == parent)
				return widget;
		}
	}
	return NULL;
}

gint remmina_widget_pool_foreach(RemminaWidgetPoolForEachFunc callback, gpointer data)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	gint i;
	gint n = 0;
	GPtrArray *wpcpy = NULL;

	if (remmina_widget_pool == NULL)
		return 0;

	/* Make a copy of remmina_widget_pool, so we can survive when callback()
	 * remove an element from remmina_widget_pool */

	wpcpy = g_ptr_array_sized_new(remmina_widget_pool->len);

	for (i = 0; i < remmina_widget_pool->len; i++)
		g_ptr_array_add(wpcpy, g_ptr_array_index(remmina_widget_pool, i));

	/* Scan the remmina_widget_pool and call callbac() on every element */
	for (i = 0; i < wpcpy->len; i++) {
		widget = GTK_WIDGET(g_ptr_array_index(wpcpy, i));
		if (callback(widget, data))
			n++;
	}

	/* Free the copy */
	g_ptr_array_unref(wpcpy);
	return n;
}

gint remmina_widget_pool_count()
{
	TRACE_CALL(__func__);
	return remmina_widget_pool->len;
}

