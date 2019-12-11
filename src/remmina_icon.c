/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remmina_widget_pool.h"
#include "remmina_pref.h"
#include "remmina_exec.h"
#include "remmina_avahi.h"
#include "remmina_applet_menu_item.h"
#include "remmina_applet_menu.h"
#include "rcw.h"
#include "remmina_icon.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_sysinfo.h"

#ifdef HAVE_LIBAPPINDICATOR
	#ifdef HAVE_AYATANA_LIBAPPINDICATOR
		#include <libayatana-appindicator/app-indicator.h>
	#else
		#include <libappindicator/app-indicator.h>
	#endif
#endif

typedef struct _RemminaIcon {
#ifdef HAVE_LIBAPPINDICATOR
	AppIndicator *icon;
#else
	GtkStatusIcon *icon;
#endif
	RemminaAvahi *avahi;
	guint32 popup_time;
	gchar *autostart_file;
	gchar *gsversion;       // GnomeShell version string, or null if not available
} RemminaIcon;

static RemminaIcon remmina_icon =
{ 0 };

void remmina_icon_destroy(void)
{
	TRACE_CALL(__func__);
	if (remmina_icon.icon) {
#ifdef HAVE_LIBAPPINDICATOR
		app_indicator_set_status(remmina_icon.icon, APP_INDICATOR_STATUS_PASSIVE);
#else
		gtk_status_icon_set_visible(remmina_icon.icon, FALSE);
#endif
		remmina_icon.icon = NULL;
	}
	if (remmina_icon.avahi) {
		remmina_avahi_free(remmina_icon.avahi);
		remmina_icon.avahi = NULL;
	}
	if (remmina_icon.autostart_file) {
		g_free(remmina_icon.autostart_file);
		remmina_icon.autostart_file = NULL;
	}
	if (remmina_icon.gsversion) {
		g_free(remmina_icon.gsversion);
		remmina_icon.gsversion = NULL;
	}
}

static void remmina_icon_main(void)
{
	TRACE_CALL(__func__);
	remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);
}

static void remmina_icon_preferences(void)
{
	TRACE_CALL(__func__);
	remmina_exec_command(REMMINA_COMMAND_PREF, "2");
}

static void remmina_icon_about(void)
{
	TRACE_CALL(__func__);
	remmina_exec_command(REMMINA_COMMAND_ABOUT, NULL);
}

static void remmina_icon_enable_avahi(GtkCheckMenuItem *checkmenuitem, gpointer data)
{
	TRACE_CALL(__func__);
	if (!remmina_icon.avahi)
		return;

	if (gtk_check_menu_item_get_active(checkmenuitem)) {
		remmina_pref.applet_enable_avahi = TRUE;
		if (!remmina_icon.avahi->started)
			remmina_avahi_start(remmina_icon.avahi);
	}else  {
		remmina_pref.applet_enable_avahi = FALSE;
		remmina_avahi_stop(remmina_icon.avahi);
	}
	remmina_pref_save();
}

static void remmina_icon_populate_additional_menu_item(GtkWidget *menu)
{
	TRACE_CALL(__func__);
	GtkWidget *menuitem;

	menuitem = gtk_menu_item_new_with_label(_("Open Main Window"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_main), NULL);

	menuitem = gtk_menu_item_new_with_mnemonic(_("_Preferences"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_preferences), NULL);

	menuitem = gtk_menu_item_new_with_mnemonic(_("_About"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_about), NULL);

	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

#ifdef HAVE_LIBAVAHI_CLIENT
	menuitem = gtk_check_menu_item_new_with_label(_("Enable Service Discovery"));
	if (remmina_pref.applet_enable_avahi) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	gtk_widget_show(menuitem);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(remmina_icon_enable_avahi), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
#endif

	menuitem = gtk_menu_item_new_with_mnemonic(_("_Quit"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_exec_exitremmina), NULL);
}

static void remmina_icon_on_launch_item(RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem, gpointer data)
{
	TRACE_CALL(__func__);
	gchar *s;

	switch (menuitem->item_type) {
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
	TRACE_CALL(__func__);
	gchar *s;

	switch (menuitem->item_type) {
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

static void remmina_icon_populate_extra_menu_item(GtkWidget *menu)
{
	TRACE_CALL(__func__);
	GtkWidget *menuitem;
	gboolean new_ontop;
	GHashTableIter iter;
	gchar *tmp;

	new_ontop = remmina_pref.applet_new_ontop;

	/* Iterate all discovered services from Avahi */
	if (remmina_icon.avahi) {
		g_hash_table_iter_init(&iter, remmina_icon.avahi->discovered_services);
		while (g_hash_table_iter_next(&iter, NULL, (gpointer*)&tmp)) {
			menuitem = remmina_applet_menu_item_new(REMMINA_APPLET_MENU_ITEM_DISCOVERED, tmp);
			gtk_widget_show(menuitem);
			remmina_applet_menu_add_item(REMMINA_APPLET_MENU(menu), REMMINA_APPLET_MENU_ITEM(menuitem));
		}
	}

	/* Separator */
	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	if (new_ontop) {
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	}else  {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	/* New Connection */
	menuitem = remmina_applet_menu_item_new(REMMINA_APPLET_MENU_ITEM_NEW);
	gtk_widget_show(menuitem);
	remmina_applet_menu_register_item(REMMINA_APPLET_MENU(menu), REMMINA_APPLET_MENU_ITEM(menuitem));
	if (new_ontop) {
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	}else  {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}

	g_signal_connect(G_OBJECT(menu), "launch-item", G_CALLBACK(remmina_icon_on_launch_item), NULL);
	g_signal_connect(G_OBJECT(menu), "edit-item", G_CALLBACK(remmina_icon_on_edit_item), NULL);
}

#ifdef HAVE_LIBAPPINDICATOR

void
remmina_icon_populate_menu(void)
{
	TRACE_CALL(__func__);
	GtkWidget *menu;
	GtkWidget *menuitem;

	if (remmina_icon.icon && !remmina_pref.disable_tray_icon) {
		menu = remmina_applet_menu_new();
		app_indicator_set_menu(remmina_icon.icon, GTK_MENU(menu));

		remmina_applet_menu_set_hide_count(REMMINA_APPLET_MENU(menu), remmina_pref.applet_hide_count);
		remmina_applet_menu_populate(REMMINA_APPLET_MENU(menu));
		remmina_icon_populate_extra_menu_item(menu);

		menuitem = gtk_separator_menu_item_new();
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		remmina_icon_populate_additional_menu_item(menu);
	}
}

#else

void remmina_icon_populate_menu(void)
{
	TRACE_CALL(__func__);
}

static void remmina_icon_popdown_menu(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	if (gtk_get_current_event_time() - remmina_icon.popup_time <= 500) {
		remmina_exec_command(REMMINA_COMMAND_MAIN, NULL);
	}
}

static void remmina_icon_on_activate(GtkStatusIcon *icon, gpointer user_data)
{
	TRACE_CALL(__func__);
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
	TRACE_CALL(__func__);
	GtkWidget *menu;

	menu = gtk_menu_new();

	remmina_icon_populate_additional_menu_item(menu);

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, gtk_status_icon_position_menu, remmina_icon.icon, button, activate_time);
}

#endif

static void remmina_icon_save_autostart_file(GKeyFile *gkeyfile)
{
	TRACE_CALL(__func__);
	gchar *content;
	gsize length;

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_icon.autostart_file, content, length, NULL);
	g_free(content);
}

static void remmina_icon_create_autostart_file(void)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;

	if (g_file_test(remmina_icon.autostart_file, G_FILE_TEST_EXISTS))
		return;

	gkeyfile = g_key_file_new();
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Version", "1.0");
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Name", _("Remmina Applet"));
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Comment",
		_("Connect to remote desktops through the applet menu"));
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Icon", REMMINA_APP_ID);
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Exec", "remmina -i");
	g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Terminal", FALSE);
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Type", "Application");
	g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Hidden", TRUE);
	remmina_icon_save_autostart_file(gkeyfile);
	g_key_file_free(gkeyfile);
}

/**
 * Determine whenever the Remmina icon is available.
 * Return TRUE if a remmina_icon (status indicator/systray menu) is
 * available and shown to the user, so the user can continue
 * its work without the remmina main window.
 * @return TRUE if the Remmina icon is available.
 */
gboolean remmina_icon_is_available(void)
{
	TRACE_CALL(__func__);
	gchar *gsversion;
	unsigned int gsv_maj, gsv_min, gsv_seq;
	gboolean gshell_has_legacyTray;

	if (!remmina_icon.icon)
		return FALSE;
	if (remmina_pref.disable_tray_icon)
		return FALSE;

	/* Special treatmen under Gnome Shell */
	if ((gsversion = remmina_sysinfo_get_gnome_shell_version()) != NULL) {
		if (sscanf(gsversion, "%u.%u", &gsv_maj, &gsv_min) == 2)
			gsv_seq = gsv_maj << 16 | gsv_min << 8;
		else
			gsv_seq = 0x030000;
		g_free(gsversion);

		gshell_has_legacyTray = FALSE;
		if (gsv_seq >= 0x031000 && gsv_seq <= 0x031800) {
			/* Gnome shell from 3.16 to 3.24, Status Icon (GtkStatusIcon) is visible on the drawer
			 * at the bottom left of the screen */
			gshell_has_legacyTray = TRUE;
		}


#ifdef HAVE_LIBAPPINDICATOR
		/** Gnome Shell with compiled in LIBAPPINDICATOR:
		 * ensure have also a working appindicator extension available.
		 */
		if (remmina_sysinfo_is_appindicator_available()) {
			/* No libappindicator extension for gnome shell, no remmina_icon */
			return TRUE;
		} else if (gshell_has_legacyTray) {
			return TRUE;
		} else {
			return FALSE;
		}
#endif
		/* Gnome Shell without LIBAPPINDICATOR */
		if (gshell_has_legacyTray) {
			return TRUE;
		}else  {
			/* Gnome shell < 3.16, Status Icon (GtkStatusIcon) is hidden
			 * on the message tray */
			return FALSE;
		}
	}
	return TRUE;
}

void remmina_icon_init(void)
{
	TRACE_CALL(__func__);

	gchar remmina_panel[23];
	gboolean sni_supported;
	char msg[200];

	if (remmina_pref.dark_tray_icon)
		g_stpcpy(remmina_panel, "remmina-panel-inverted");
	else
		g_stpcpy(remmina_panel, "remmina-panel");


	/* Print on stdout the availability of appindicators on DBUS */
	sni_supported = remmina_sysinfo_is_appindicator_available();

	strcpy(msg, "StatusNotifier/Appindicator support: ");
	if (sni_supported) {
		strcat(msg, "your desktop does support it");
#ifdef HAVE_LIBAPPINDICATOR
		strcat(msg, " and libappindicator is compiled in remmina. Good!");
#else
		strcat(msg, ", but you did not compile remmina with cmake’s -DWITH_APPINDICATOR=on");
#endif
	} else {
#ifdef HAVE_LIBAPPINDICATOR
		strcat(msg, "not supported by desktop. libappindicator will try to fallback to GtkStatusIcon/xembed");
#else
		strcat(msg, "not supported by desktop. Remmina will try to fallback to GtkStatusIcon/xembed");
#endif
	}
	strcat(msg, "\n");
	fputs(msg, stderr);

	remmina_icon.gsversion = remmina_sysinfo_get_gnome_shell_version();
	if (remmina_icon.gsversion != NULL) {
		printf("Running under Gnome Shell version %s\n", remmina_icon.gsversion);
	}

	if (!remmina_icon.icon && !remmina_pref.disable_tray_icon) {
#ifdef HAVE_LIBAPPINDICATOR
		remmina_icon.icon = app_indicator_new("remmina-icon", remmina_panel, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
		app_indicator_set_icon_theme_path(remmina_icon.icon, REMMINA_RUNTIME_DATADIR G_DIR_SEPARATOR_S "icons");

		app_indicator_set_status(remmina_icon.icon, APP_INDICATOR_STATUS_ACTIVE);
		app_indicator_set_title(remmina_icon.icon, "Remmina");
		remmina_icon_populate_menu();
#else
		remmina_icon.icon = gtk_status_icon_new_from_icon_name(remmina_panel);

		gtk_status_icon_set_title(remmina_icon.icon, _("Remmina Remote Desktop Client"));
		gtk_status_icon_set_tooltip_text(remmina_icon.icon, _("Remmina Remote Desktop Client"));

		g_signal_connect(G_OBJECT(remmina_icon.icon), "popup-menu", G_CALLBACK(remmina_icon_on_popup_menu), NULL);
		g_signal_connect(G_OBJECT(remmina_icon.icon), "activate", G_CALLBACK(remmina_icon_on_activate), NULL);
#endif
	}else if (remmina_icon.icon) {
#ifdef HAVE_LIBAPPINDICATOR
		app_indicator_set_status(remmina_icon.icon, remmina_pref.disable_tray_icon ?
			APP_INDICATOR_STATUS_PASSIVE : APP_INDICATOR_STATUS_ACTIVE);
		/* With libappindicator we can also change the icon on the fly */
		app_indicator_set_icon(remmina_icon.icon, remmina_panel);
#else
		gtk_status_icon_set_visible(remmina_icon.icon, !remmina_pref.disable_tray_icon);

#endif
	}
	if (!remmina_icon.avahi) {
		remmina_icon.avahi = remmina_avahi_new();
	}
	if (remmina_icon.avahi) {
		if (remmina_pref.applet_enable_avahi) {
			if (!remmina_icon.avahi->started)
				remmina_avahi_start(remmina_icon.avahi);
		}else  {
			remmina_avahi_stop(remmina_icon.avahi);
		}
	}
	if (!remmina_icon.autostart_file) {
		remmina_icon.autostart_file = g_strdup_printf("%s/.config/autostart/remmina-applet.desktop", g_get_home_dir());
		remmina_icon_create_autostart_file();
	}
}

gboolean remmina_icon_is_autostart(void)
{
	TRACE_CALL(__func__);
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
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gboolean b;

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_icon.autostart_file, G_KEY_FILE_NONE, NULL);
	b = !g_key_file_get_boolean(gkeyfile, "Desktop Entry", "Hidden", NULL);
	if (b != autostart) {
		g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Hidden", !autostart);
		/* Refresh it in case translation is updated */
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Name", _("Remmina Applet"));
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Comment",
			_("Connect to remote desktops through the applet menu"));
		remmina_icon_save_autostart_file(gkeyfile);
	}
	g_key_file_free(gkeyfile);
}

