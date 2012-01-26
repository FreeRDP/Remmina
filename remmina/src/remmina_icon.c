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
#include "remmina_widget_pool.h"
#include "remmina_pref.h"
#include "remmina_exec.h"
#include "remmina_avahi.h"
#include "remmina_applet_menu_item.h"
#include "remmina_applet_menu.h"
#include "remmina_connection_window.h"
#include "remmina_icon.h"

#ifdef HAVE_LIBAPPINDICATOR
#include <libappindicator/app-indicator.h>
#endif

typedef struct _RemminaIcon
{
#ifdef HAVE_LIBAPPINDICATOR
	AppIndicator *icon;
#else
	GtkStatusIcon *icon;
#endif
	RemminaAvahi *avahi;
	guint32 popup_time;
	gchar *autostart_file;
} RemminaIcon;

static RemminaIcon remmina_icon =
{ 0 };

static void remmina_icon_destroy(void)
{
	if (remmina_icon.icon)
	{
#ifdef HAVE_LIBAPPINDICATOR
		app_indicator_set_status (remmina_icon.icon, APP_INDICATOR_STATUS_PASSIVE);
#else
		gtk_status_icon_set_visible(remmina_icon.icon, FALSE);
#endif
		remmina_widget_pool_hold(FALSE);
	}
	if (remmina_icon.avahi)
	{
		remmina_avahi_free(remmina_icon.avahi);
		remmina_icon.avahi = NULL;
	}
	if (remmina_icon.autostart_file)
	{
		g_free(remmina_icon.autostart_file);
		remmina_icon.autostart_file = NULL;
	}
}

static void remmina_icon_main(void)
{
	remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);
}

static void remmina_icon_preferences(void)
{
	remmina_exec_command(REMMINA_COMMAND_PREF, "2");
}

static void remmina_icon_about(void)
{
	remmina_exec_command(REMMINA_COMMAND_ABOUT, NULL);
}

static void remmina_icon_enable_avahi(GtkCheckMenuItem *checkmenuitem, gpointer data)
{
	if (!remmina_icon.avahi)
		return;

	if (gtk_check_menu_item_get_active(checkmenuitem))
	{
		remmina_pref.applet_enable_avahi = TRUE;
		if (!remmina_icon.avahi->started)
			remmina_avahi_start(remmina_icon.avahi);
	}
	else
	{
		remmina_pref.applet_enable_avahi = FALSE;
		remmina_avahi_stop(remmina_icon.avahi);
	}
	remmina_pref_save();
}

static void remmina_icon_populate_additional_menu_item(GtkWidget *menu)
{
	GtkWidget *menuitem;

	menuitem = gtk_image_menu_item_new_with_label(_("Open Main Window"));
	gtk_widget_show(menuitem);
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menuitem),
			gtk_image_new_from_icon_name("remmina", GTK_ICON_SIZE_MENU));
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_main), NULL);

	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_PREFERENCES, NULL);
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_preferences), NULL);

	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_ABOUT, NULL);
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_about), NULL);

	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

#ifdef HAVE_LIBAVAHI_CLIENT
	menuitem = gtk_check_menu_item_new_with_label (_("Enable Service Discovery"));
	if (remmina_pref.applet_enable_avahi)
	{
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), TRUE);
	}
	gtk_widget_show(menuitem);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(remmina_icon_enable_avahi), NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
#endif

	menuitem = gtk_image_menu_item_new_from_stock(GTK_STOCK_QUIT, NULL);
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_destroy), NULL);
}

static void remmina_icon_on_launch_item(RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem, gpointer data)
{
	gchar *s;

	switch (menuitem->item_type)
	{
		case REMMINA_APPLET_MENU_ITEM_NEW:
			remmina_exec_command(REMMINA_COMMAND_NEW, NULL);
			break;
		case REMMINA_APPLET_MENU_ITEM_FILE:
			remmina_exec_command(REMMINA_COMMAND_CONNECT, menuitem->filename);
			break;
		case REMMINA_APPLET_MENU_ITEM_DISCOVERED:
			s = g_strdup_printf("%s,%s", menuitem->protocol, menuitem->name);
			remmina_exec_command(REMMINA_COMMAND_NEW, s);
			g_free(s);
			break;
	}
}

static void remmina_icon_on_edit_item(RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem, gpointer data)
{
	gchar *s;

	switch (menuitem->item_type)
	{
		case REMMINA_APPLET_MENU_ITEM_NEW:
			remmina_exec_command(REMMINA_COMMAND_NEW, NULL);
			break;
		case REMMINA_APPLET_MENU_ITEM_FILE:
			remmina_exec_command(REMMINA_COMMAND_EDIT, menuitem->filename);
			break;
		case REMMINA_APPLET_MENU_ITEM_DISCOVERED:
			s = g_strdup_printf("%s,%s", menuitem->protocol, menuitem->name);
			remmina_exec_command(REMMINA_COMMAND_NEW, s);
			g_free(s);
			break;
	}
}

static void remmina_icon_on_activate_window(GtkMenuItem *menuitem, GtkWidget *widget)
{
	if (GTK_IS_WINDOW(widget))
	{
		gtk_window_present(GTK_WINDOW(widget));
		gtk_window_deiconify(GTK_WINDOW(widget));
	}
}

static gboolean remmina_icon_foreach_window(GtkWidget *widget, gpointer data)
{
	GtkWidget *popup_menu = GTK_WIDGET(data);
	GtkWidget *menuitem;
	GdkScreen *screen;
	gint screen_number;

	if (G_TYPE_CHECK_INSTANCE_TYPE(widget, REMMINA_TYPE_CONNECTION_WINDOW))
	{
		screen = gdk_screen_get_default();
		screen_number = gdk_screen_get_number(screen);
		if (screen_number == gdk_screen_get_number(gtk_window_get_screen(GTK_WINDOW(widget))))
		{
			menuitem = gtk_menu_item_new_with_label(gtk_window_get_title(GTK_WINDOW(widget)));
			gtk_widget_show(menuitem);
			gtk_menu_shell_prepend(GTK_MENU_SHELL(popup_menu), menuitem);
			g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_on_activate_window), widget);
			return TRUE;
		}
	}
	return FALSE;
}

static void remmina_icon_populate_extra_menu_item(GtkWidget *menu)
{
	GtkWidget *menuitem;
	gboolean new_ontop;
	GHashTableIter iter;
	gchar *tmp;
	gint n;

	new_ontop = remmina_pref.applet_new_ontop;

	/* Iterate all discovered services from Avahi */
	if (remmina_icon.avahi)
	{
		g_hash_table_iter_init(&iter, remmina_icon.avahi->discovered_services);
		while (g_hash_table_iter_next(&iter, NULL, (gpointer*) &tmp))
		{
			menuitem = remmina_applet_menu_item_new(REMMINA_APPLET_MENU_ITEM_DISCOVERED, tmp);
			gtk_widget_show(menuitem);
			remmina_applet_menu_add_item(REMMINA_APPLET_MENU(menu), REMMINA_APPLET_MENU_ITEM(menuitem));
		}
	}

	/* Separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	if (new_ontop)
	{
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	}
	else
	{
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	/* New Connection */
	menuitem = remmina_applet_menu_item_new(REMMINA_APPLET_MENU_ITEM_NEW);
	gtk_widget_show(menuitem);
	remmina_applet_menu_register_item(REMMINA_APPLET_MENU(menu), REMMINA_APPLET_MENU_ITEM(menuitem));
	if (new_ontop)
	{
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	}
	else
	{
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	/* Existing window */
	if (remmina_pref.minimize_to_tray)
	{
		n = remmina_widget_pool_foreach(remmina_icon_foreach_window, menu);
		if (n > 0)
		{
			/* Separator */
			menuitem = gtk_separator_menu_item_new();
			gtk_widget_show(menuitem);
			gtk_menu_shell_insert(GTK_MENU_SHELL(menu), menuitem, n);
		}
	}

	g_signal_connect(G_OBJECT(menu), "launch-item", G_CALLBACK(remmina_icon_on_launch_item), NULL);
	g_signal_connect(G_OBJECT(menu), "edit-item", G_CALLBACK(remmina_icon_on_edit_item), NULL);
}

#ifdef HAVE_LIBAPPINDICATOR

void
remmina_icon_populate_menu (void)
{
	GtkWidget *menu;
	GtkWidget *menuitem;

	menu = remmina_applet_menu_new ();
	app_indicator_set_menu (remmina_icon.icon, GTK_MENU (menu));

	remmina_applet_menu_set_hide_count (REMMINA_APPLET_MENU (menu), remmina_pref.applet_hide_count);
	remmina_applet_menu_populate (REMMINA_APPLET_MENU (menu));
	remmina_icon_populate_extra_menu_item (menu);

	menuitem = gtk_separator_menu_item_new ();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

	remmina_icon_populate_additional_menu_item (menu);
}

#else

void remmina_icon_populate_menu(void)
{
}

static void remmina_icon_popdown_menu(GtkWidget *widget, gpointer data)
{
	if (gtk_get_current_event_time() - remmina_icon.popup_time <= 500)
	{
		remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);
	}
}

static void remmina_icon_on_activate(GtkStatusIcon *icon, gpointer user_data)
{
	GtkWidget *menu;
	gint button, event_time;

	remmina_icon.popup_time = gtk_get_current_event_time();
	menu = remmina_applet_menu_new();
	remmina_applet_menu_set_hide_count(REMMINA_APPLET_MENU(menu), remmina_pref.applet_hide_count);
	remmina_applet_menu_populate(REMMINA_APPLET_MENU(menu));

	remmina_icon_populate_extra_menu_item(menu);

	button = 0;
	event_time = gtk_get_current_event_time();

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(remmina_icon_popdown_menu), NULL);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, remmina_icon.icon, button, event_time);
}

static void remmina_icon_on_popup_menu(GtkStatusIcon *icon, guint button, guint activate_time, gpointer user_data)
{
	GtkWidget *menu;

	menu = gtk_menu_new();

	remmina_icon_populate_additional_menu_item(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, remmina_icon.icon, button, activate_time);
}

#endif

static void remmina_icon_save_autostart_file(GKeyFile *gkeyfile)
{
	gchar *content;
	gsize length;

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_icon.autostart_file, content, length, NULL);
	g_free(content);
}

static void remmina_icon_create_autostart_file(void)
{
	GKeyFile *gkeyfile;

	if (!g_file_test(remmina_icon.autostart_file, G_FILE_TEST_EXISTS))
	{
		gkeyfile = g_key_file_new();
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Version", "1.0");
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Name", _("Remmina Applet"));
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Comment",
				_("Connect to remote desktops through the applet menu"));
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Icon", "remmina");
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Exec", "remmina -i");
		g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Terminal", FALSE);
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Type", "Application");
		g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Hidden", TRUE);
		remmina_icon_save_autostart_file(gkeyfile);
		g_key_file_free(gkeyfile);
	}
}

void remmina_icon_init(void)
{
	if (!remmina_icon.icon && !remmina_pref.disable_tray_icon)
	{
#ifdef HAVE_LIBAPPINDICATOR
		remmina_icon.icon = app_indicator_new ("remmina-icon", "remmina", APP_INDICATOR_CATEGORY_OTHER);

		app_indicator_set_status (remmina_icon.icon, APP_INDICATOR_STATUS_ACTIVE);
		remmina_icon_populate_menu ();
#else
		remmina_icon.icon = gtk_status_icon_new_from_icon_name("remmina");

		gtk_status_icon_set_title(remmina_icon.icon, _("Remmina Remote Desktop Client"));
		gtk_status_icon_set_tooltip_text(remmina_icon.icon, _("Remmina Remote Desktop Client"));

		g_signal_connect(G_OBJECT(remmina_icon.icon), "popup-menu", G_CALLBACK(remmina_icon_on_popup_menu), NULL);
		g_signal_connect(G_OBJECT(remmina_icon.icon), "activate", G_CALLBACK(remmina_icon_on_activate), NULL);
#endif
		remmina_widget_pool_hold(TRUE);
	}
	else
		if (remmina_icon.icon)
		{
#ifdef HAVE_LIBAPPINDICATOR
			app_indicator_set_status (remmina_icon.icon, remmina_pref.disable_tray_icon ?
					APP_INDICATOR_STATUS_PASSIVE : APP_INDICATOR_STATUS_ACTIVE);
#else
			gtk_status_icon_set_visible(remmina_icon.icon, !remmina_pref.disable_tray_icon);
#endif
			remmina_widget_pool_hold(!remmina_pref.disable_tray_icon);
		}
	if (!remmina_icon.avahi)
	{
		remmina_icon.avahi = remmina_avahi_new();
	}
	if (remmina_icon.avahi)
	{
		if (remmina_pref.applet_enable_avahi)
		{
			if (!remmina_icon.avahi->started)
				remmina_avahi_start(remmina_icon.avahi);
		}
		else
		{
			remmina_avahi_stop(remmina_icon.avahi);
		}
	}
	if (!remmina_icon.autostart_file)
	{
		remmina_icon.autostart_file = g_strdup_printf("%s/.config/autostart/remmina-applet.desktop", g_get_home_dir());
		remmina_icon_create_autostart_file();
	}
}

gboolean remmina_icon_is_autostart(void)
{
	GKeyFile *gkeyfile;
	gboolean b;

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_icon.autostart_file, G_KEY_FILE_NONE, NULL);
	b = !g_key_file_get_boolean(gkeyfile, "Desktop Entry", "Hidden", NULL);
	g_key_file_free(gkeyfile);
	return b;
}

void remmina_icon_set_autostart(gboolean autostart)
{
	GKeyFile *gkeyfile;
	gboolean b;

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_icon.autostart_file, G_KEY_FILE_NONE, NULL);
	b = !g_key_file_get_boolean(gkeyfile, "Desktop Entry", "Hidden", NULL);
	if (b != autostart)
	{
		g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Hidden", !autostart);
		/* Refresh it in case translation is updated */
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Name", _("Remmina Applet"));
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Comment",
				_("Connect to remote desktops through the applet menu"));
		remmina_icon_save_autostart_file(gkeyfile);
	}
	g_key_file_free(gkeyfile);
}

