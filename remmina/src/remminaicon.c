/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remminawidgetpool.h"
#include "remminaexec.h"
#include "remminaicon.h"

static GtkStatusIcon *remmina_icon = NULL;

static void
remmina_icon_destroy (void)
{
    if (remmina_icon)
    {
        gtk_status_icon_set_visible (remmina_icon, FALSE);
        remmina_widget_pool_hold (FALSE);
    }
}

static void
remmina_icon_main (void)
{
    remmina_exec_command (REMMINA_COMMAND_MAIN, NULL);
}

static void
remmina_icon_preferences (void)
{
    remmina_exec_command (REMMINA_COMMAND_PREF, "0");
}

static void
remmina_icon_about (void)
{
    remmina_exec_command (REMMINA_COMMAND_ABOUT, NULL);
}

static void
remmina_icon_on_popup_menu (GtkStatusIcon *icon, guint button, guint activate_time, gpointer user_data)
{
    GtkWidget *menu;
    GtkWidget *menuitem;

    menu = gtk_menu_new ();

    menuitem = gtk_image_menu_item_new_with_label (_("Open Main Window"));
    gtk_widget_show (menuitem);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem),
        gtk_image_new_from_icon_name ("remmina", GTK_ICON_SIZE_MENU));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (remmina_icon_main), NULL);

    menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
    gtk_widget_show (menuitem);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (remmina_icon_preferences), NULL);

    menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
    gtk_widget_show (menuitem);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (remmina_icon_about), NULL);

    menuitem = gtk_separator_menu_item_new ();
    gtk_widget_show (menuitem);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

    menuitem = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
    gtk_widget_show (menuitem);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
    g_signal_connect (G_OBJECT (menuitem), "activate", G_CALLBACK (remmina_icon_destroy), NULL);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
        gtk_status_icon_position_menu, remmina_icon, button, activate_time);
}

void
remmina_icon_create (void)
{
    if (!remmina_icon)
    {
        remmina_icon = gtk_status_icon_new_from_icon_name ("remmina");
        remmina_widget_pool_hold (TRUE);

        gtk_status_icon_set_title (remmina_icon, _("Remmina Remote Desktop Client"));
        gtk_status_icon_set_tooltip_text (remmina_icon, _("Remmina Remote Desktop Client"));

        g_signal_connect (G_OBJECT (remmina_icon), "popup-menu", G_CALLBACK (remmina_icon_on_popup_menu), NULL);
    }
    else
    {
        gtk_status_icon_set_visible (remmina_icon, TRUE);
    }
}

