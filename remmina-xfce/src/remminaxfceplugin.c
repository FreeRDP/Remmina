/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee 
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
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>
#include "config.h"
#include "remminaappletmenuitem.h"
#include "remminaappletmenu.h"
#include "remminaavahi.h"
#include "remminaappletutil.h"

typedef struct _RemminaXfcePlugin
{
    XfcePanelPlugin *plugin;

    GtkWidget *image;
    GtkWidget *popup_menu;
    guint32 popup_time;

    gchar *menu_group;
    gint menu_group_count;
    GtkWidget *menu_group_widget;

    RemminaAvahi *avahi;
} RemminaXfcePlugin;

static void
remmina_xfce_plugin_free (XfcePanelPlugin *plugin, RemminaXfcePlugin *rxplugin)
{
    remmina_avahi_free (rxplugin->avahi);
    panel_slice_free (RemminaXfcePlugin, rxplugin);
}

static gboolean
remmina_xfce_plugin_size_changed (XfcePanelPlugin *plugin, gint size, RemminaXfcePlugin *rxplugin)
{
    gint icon_size;

    /* Fit the exact available icon sizes */
    if (size < 22) icon_size = 16;
    else if (size < 24) icon_size = 22;
    else if (size < 32) icon_size = 24;
    else if (size < 48) icon_size = 32;
    else icon_size = 48;
    gtk_image_set_pixel_size (GTK_IMAGE (rxplugin->image), icon_size);
    
    return TRUE;
}

static void
remmina_xfce_plugin_menu_open_main (RemminaAppletMenuItem *menuitem, gpointer data)
{
    remmina_applet_util_launcher (REMMINA_LAUNCH_MAIN, NULL, NULL, NULL);
}

static void
remmina_xfce_plugin_menu_configure (XfcePanelPlugin *plugin, gpointer data)
{
    remmina_applet_util_launcher (REMMINA_LAUNCH_PREF, NULL, NULL, NULL);
}

static void
remmina_xfce_plugin_menu_on_launch_item (RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem, RemminaXfcePlugin *rxplugin)
{
    switch (menuitem->item_type)
    {
    case REMMINA_APPLET_MENU_ITEM_NEW:
        remmina_applet_util_launcher (REMMINA_LAUNCH_NEW, NULL, NULL, NULL);
        break;
    case REMMINA_APPLET_MENU_ITEM_FILE:
        remmina_applet_util_launcher (REMMINA_LAUNCH_FILE, menuitem->filename, NULL, NULL);
        break;
    case REMMINA_APPLET_MENU_ITEM_DISCOVERED:
        remmina_applet_util_launcher (REMMINA_LAUNCH_NEW, NULL, menuitem->name, menuitem->protocol);
        break;
    }
}

static void
remmina_xfce_plugin_menu_on_edit_item (RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem, RemminaXfcePlugin *rxplugin)
{
    switch (menuitem->item_type)
    {
    case REMMINA_APPLET_MENU_ITEM_NEW:
        remmina_applet_util_launcher (REMMINA_LAUNCH_NEW, NULL, NULL, NULL);
        break;
    case REMMINA_APPLET_MENU_ITEM_FILE:
        remmina_applet_util_launcher (REMMINA_LAUNCH_EDIT, menuitem->filename, NULL, NULL);
        break;
    case REMMINA_APPLET_MENU_ITEM_DISCOVERED:
        remmina_applet_util_launcher (REMMINA_LAUNCH_NEW, NULL, menuitem->name, menuitem->protocol);
        break;
    }
}

static void
remmina_xfce_plugin_popdown_menu (GtkWidget *widget, RemminaXfcePlugin *rxplugin)
{
    rxplugin->popup_menu = NULL;
    gtk_widget_set_state (GTK_WIDGET (rxplugin->plugin), GTK_STATE_NORMAL);
    if (gtk_get_current_event_time () - rxplugin->popup_time <= 500)
    {
        remmina_xfce_plugin_menu_open_main (NULL, NULL);
    }
}

static void
remmina_xfce_plugin_popup_menu_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer data)
{
    GdkScreen *screen;
    GtkRequisition req;
    gint tx, ty;
    RemminaXfcePlugin *rxplugin;

    rxplugin = (RemminaXfcePlugin*) data;
    gdk_window_get_origin (GTK_WIDGET (rxplugin->plugin)->window, &tx, &ty);
    gtk_widget_size_request (GTK_WIDGET (menu), &req);

    switch (xfce_panel_plugin_get_screen_position (rxplugin->plugin))
    {
    case XFCE_SCREEN_POSITION_SW_H:
    case XFCE_SCREEN_POSITION_S:
    case XFCE_SCREEN_POSITION_SE_H:
        ty -= req.height;
        break;
    case XFCE_SCREEN_POSITION_NW_H:
    case XFCE_SCREEN_POSITION_N:
    case XFCE_SCREEN_POSITION_NE_H:
    case XFCE_SCREEN_POSITION_FLOATING_H:
        ty += GTK_WIDGET (rxplugin->plugin)->allocation.height;
        break;
    case XFCE_SCREEN_POSITION_NE_V:
    case XFCE_SCREEN_POSITION_E:
    case XFCE_SCREEN_POSITION_SE_V:
        tx -= req.width;
        break;
    case XFCE_SCREEN_POSITION_NW_V:
    case XFCE_SCREEN_POSITION_W:
    case XFCE_SCREEN_POSITION_SW_V:
    case XFCE_SCREEN_POSITION_FLOATING_V:
        tx += GTK_WIDGET (rxplugin->plugin)->allocation.width;
        break;
    default:
        break;
    }

    screen = gdk_screen_get_default ();
    tx = MIN (gdk_screen_get_width (screen) - req.width - 1, tx);
    ty = MIN (gdk_screen_get_height (screen) - req.height - 1, ty);

    *x = tx;
    *y = ty;
    *push_in = TRUE;
}

static void
remmina_xfce_plugin_popup_menu (GtkWidget *widget, GdkEventButton *event, RemminaXfcePlugin *rxplugin)
{
    GtkWidget *menuitem;
    gint button, event_time;
    gboolean new_ontop;
    GHashTableIter iter;
    gchar *tmp;

    new_ontop = remmina_applet_util_get_pref_boolean ("applet_new_ontop");

    rxplugin->popup_time = gtk_get_current_event_time ();
    rxplugin->popup_menu = remmina_applet_menu_new (
        remmina_applet_util_get_pref_boolean ("applet_hide_count"));

    /* Iterate all discovered services from Avahi */
    if (rxplugin->avahi)
    {
        g_hash_table_iter_init (&iter, rxplugin->avahi->discovered_services);
        while (g_hash_table_iter_next (&iter, NULL, (gpointer*) &tmp))
        {
            menuitem = remmina_applet_menu_item_new (REMMINA_APPLET_MENU_ITEM_DISCOVERED, tmp);
            gtk_widget_show (menuitem);
            remmina_applet_menu_add_item (
                REMMINA_APPLET_MENU (rxplugin->popup_menu),
                REMMINA_APPLET_MENU_ITEM (menuitem));
        }
    }

    /* Separator */
    menuitem = gtk_separator_menu_item_new ();
    gtk_widget_show (menuitem);
    if (new_ontop)
    {
        gtk_menu_shell_prepend (GTK_MENU_SHELL (rxplugin->popup_menu), menuitem);
    }
    else
    {
        gtk_menu_shell_append (GTK_MENU_SHELL (rxplugin->popup_menu), menuitem);
    }

    /* New Connection */
    menuitem = remmina_applet_menu_item_new (REMMINA_APPLET_MENU_ITEM_NEW);
    gtk_widget_show (menuitem);
    remmina_applet_menu_register_item (
        REMMINA_APPLET_MENU (rxplugin->popup_menu),
        REMMINA_APPLET_MENU_ITEM (menuitem));
    if (new_ontop)
    {
        gtk_menu_shell_prepend (GTK_MENU_SHELL (rxplugin->popup_menu), menuitem);
    }
    else
    {
        gtk_menu_shell_append (GTK_MENU_SHELL (rxplugin->popup_menu), menuitem);
    }

    if (event)
    {
        button = event->button;
        event_time = event->time;
    }
    else
    {
        button = 0;
        event_time = gtk_get_current_event_time ();
    }

    gtk_widget_set_state (GTK_WIDGET (rxplugin->plugin), GTK_STATE_SELECTED);
    g_signal_connect (G_OBJECT (rxplugin->popup_menu), "deactivate", G_CALLBACK (remmina_xfce_plugin_popdown_menu), rxplugin);

    gtk_menu_attach_to_widget (GTK_MENU (rxplugin->popup_menu), widget, NULL);
    gtk_widget_realize (rxplugin->popup_menu);

    g_signal_connect (G_OBJECT (rxplugin->popup_menu), "launch-item",
        G_CALLBACK (remmina_xfce_plugin_menu_on_launch_item), rxplugin);
    g_signal_connect (G_OBJECT (rxplugin->popup_menu), "edit-item",
        G_CALLBACK (remmina_xfce_plugin_menu_on_edit_item), rxplugin);

    gtk_menu_popup (GTK_MENU (rxplugin->popup_menu), NULL, NULL,
        remmina_xfce_plugin_popup_menu_position, rxplugin, button, event_time);
}

static gboolean
remmina_xfce_plugin_button_press_event (GtkWidget *widget, GdkEventButton *event, RemminaXfcePlugin *rxplugin)
{
    if (event->button == 1)
    {
        switch (event->type)
        {
        case GDK_BUTTON_PRESS:
            remmina_xfce_plugin_popup_menu (widget, event, rxplugin);
            break;
        default:
            break;
        }
        return TRUE;
    }
    return FALSE;
}

static void
remmina_xfce_plugin_menu_enable_avahi (GtkCheckMenuItem *checkmenuitem, RemminaXfcePlugin *rxplugin)
{
    if (!rxplugin->avahi) return;

    if (gtk_check_menu_item_get_active (checkmenuitem))
    {
        remmina_applet_util_set_pref_boolean ("applet_enable_avahi", TRUE);
        if (!rxplugin->avahi->started) remmina_avahi_start (rxplugin->avahi);
    }
    else
    {
        remmina_applet_util_set_pref_boolean ("applet_enable_avahi", FALSE);
        remmina_avahi_stop (rxplugin->avahi);
    }
}

static void
remmina_xfce_plugin_menu_about (XfcePanelPlugin *plugin)
{
    remmina_applet_util_launcher (REMMINA_LAUNCH_ABOUT, NULL, NULL, NULL);
}

static void
remmina_xfce_plugin_create (XfcePanelPlugin *plugin)
{
    RemminaXfcePlugin *rxplugin;
    GtkWidget *menuitem;

    rxplugin = panel_slice_new0 (RemminaXfcePlugin);
    rxplugin->plugin = plugin;
    rxplugin->popup_menu = NULL;
    rxplugin->popup_time = 0;
    rxplugin->avahi = remmina_avahi_new ();

    rxplugin->image = gtk_image_new_from_icon_name ("remmina", GTK_ICON_SIZE_MENU);
    gtk_widget_show (rxplugin->image);

    gtk_container_add (GTK_CONTAINER (plugin), rxplugin->image);
    xfce_panel_plugin_add_action_widget (plugin, rxplugin->image);

    g_signal_connect (G_OBJECT (plugin), "free-data", G_CALLBACK (remmina_xfce_plugin_free), rxplugin);
    g_signal_connect (G_OBJECT (plugin), "button-press-event", G_CALLBACK (remmina_xfce_plugin_button_press_event), rxplugin);
    g_signal_connect (G_OBJECT (plugin), "size-changed", G_CALLBACK (remmina_xfce_plugin_size_changed), rxplugin);

    menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_EXECUTE, NULL);
    gtk_menu_item_set_label (GTK_MENU_ITEM (menuitem), _("Open Main Window"));
    gtk_widget_show (menuitem);
    g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (remmina_xfce_plugin_menu_open_main), rxplugin);
    xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM (menuitem));

#ifdef HAVE_LIBAVAHI_CLIENT
    menuitem = gtk_check_menu_item_new_with_label (_("Enable Service Discovery"));
    if (remmina_applet_util_get_pref_boolean ("applet_enable_avahi"))
    {
        gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), TRUE);
    }
    gtk_widget_show (menuitem);
    g_signal_connect (G_OBJECT (menuitem), "toggled", G_CALLBACK (remmina_xfce_plugin_menu_enable_avahi), rxplugin);
    xfce_panel_plugin_menu_insert_item (plugin, GTK_MENU_ITEM (menuitem));
#endif

    xfce_panel_plugin_menu_show_configure (plugin);
    g_signal_connect (G_OBJECT (plugin), "configure-plugin", G_CALLBACK (remmina_xfce_plugin_menu_configure), NULL);

    xfce_panel_plugin_menu_show_about (plugin);
    g_signal_connect (G_OBJECT (plugin), "about", G_CALLBACK (remmina_xfce_plugin_menu_about), NULL);

    if (remmina_applet_util_get_pref_boolean ("applet_enable_avahi"))
    {
        remmina_avahi_start (rxplugin->avahi);
    }
}

static void
remmina_xfce_plugin_construct (XfcePanelPlugin *plugin)
{
    xfce_textdomain(GETTEXT_PACKAGE, REMMINA_XFCE_LOCALEDIR, "UTF-8");
    
    gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
        REMMINA_DATADIR G_DIR_SEPARATOR_S "icons");

    remmina_xfce_plugin_create (plugin);
}

XFCE_PANEL_PLUGIN_REGISTER_INTERNAL (remmina_xfce_plugin_construct);
