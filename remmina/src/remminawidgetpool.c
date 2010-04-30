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
#include "remminapublic.h"
#include "remminawidgetpool.h"

static GPtrArray *remmina_widget_pool = NULL;

static guint remmina_widget_pool_try_quit_handler = 0;

static gboolean
remmina_widget_pool_try_quit (gpointer data)
{
    if (remmina_widget_pool->len == 0)
    {
        gtk_main_quit ();
    }
    remmina_widget_pool_try_quit_handler = 0;
    return FALSE;
}

void
remmina_widget_pool_init (void)
{
    remmina_widget_pool = g_ptr_array_new ();
    remmina_widget_pool_try_quit_handler = g_timeout_add (15000, remmina_widget_pool_try_quit, NULL);
}

static void
remmina_widget_pool_on_widget_destroy (GtkWidget *widget, gpointer data)
{
    g_ptr_array_remove (remmina_widget_pool, widget);
    if (remmina_widget_pool->len == 0 && remmina_widget_pool_try_quit_handler == 0)
    {
        /* Wait for a while to make sure no more windows will open before we quit the application */
        remmina_widget_pool_try_quit_handler = g_timeout_add (10000, remmina_widget_pool_try_quit, NULL);
    }
}

void
remmina_widget_pool_register (GtkWidget *widget)
{
    g_ptr_array_add (remmina_widget_pool, widget);
    g_signal_connect (G_OBJECT (widget), "destroy", G_CALLBACK (remmina_widget_pool_on_widget_destroy), NULL);
    if (remmina_widget_pool_try_quit_handler)
    {
        g_source_remove (remmina_widget_pool_try_quit_handler);
        remmina_widget_pool_try_quit_handler = 0;
    }
}

GtkWidget*
remmina_widget_pool_find (GType type, const gchar *tag)
{
    GtkWidget *widget;
    gint i;
    GdkScreen *screen;
    gint screen_number;
    guint workspace;

    screen = gdk_screen_get_default ();
    screen_number = gdk_screen_get_number (screen);
    workspace = remmina_public_get_current_workspace (screen);

    if (remmina_widget_pool == NULL) return NULL;

    for (i = 0; i < remmina_widget_pool->len; i++)
    {
        widget = GTK_WIDGET (g_ptr_array_index (remmina_widget_pool, i));
        if (!G_TYPE_CHECK_INSTANCE_TYPE (widget, type)) continue;
        if (screen_number != gdk_screen_get_number (gtk_window_get_screen (GTK_WINDOW (widget)))) continue;
        if (workspace != remmina_public_get_window_workspace (GTK_WINDOW (widget))) continue;
        if (tag && g_strcmp0 ((const gchar*) g_object_get_data (G_OBJECT (widget), "tag"), tag) != 0) continue;
        return widget;
    }
    return NULL;
}

GtkWidget*
remmina_widget_pool_find_by_window (GType type, GdkWindow *window)
{
    GtkWidget *widget;
    gint i;
    GdkWindow *parent;

    if (window == NULL || remmina_widget_pool == NULL) return NULL;

    for (i = 0; i < remmina_widget_pool->len; i++)
    {
        widget = GTK_WIDGET (g_ptr_array_index (remmina_widget_pool, i));
        if (!G_TYPE_CHECK_INSTANCE_TYPE (widget, type)) continue;
        /* gdk_window_get_toplevel won't work here, if the window is an embedded client. So we iterate the window tree */
        for (parent = window; parent && parent != GDK_WINDOW_ROOT; parent = gdk_window_get_parent (parent))
        {
            if (gtk_widget_get_window (widget) == parent) return widget;
        }
    }
    return NULL;
}

