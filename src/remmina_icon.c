/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
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

#include <glib/gi18n.h>

#include "remmina_icon.h"

#ifdef HAVE_LIBAPPINDICATOR
#  ifdef HAVE_AYATANA_LIBAPPINDICATOR
#    include <libayatana-appindicator/app-indicator.h>
#  else
#    include <libappindicator/app-indicator.h>
#  endif
#include "remmina_widget_pool.h"
#include "remmina_pref.h"
#include "remmina_exec.h"
#ifdef HAVE_LIBAVAHI_CLIENT
#include "remmina_avahi.h"
#endif
#include "remmina_applet_menu_item.h"
#include "remmina_applet_menu.h"
#include "rcw.h"
#include "remmina_log.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_sysinfo.h"

typedef struct _RemminaIcon {
	AppIndicator *	icon;
	gboolean	indicator_connected;
#ifdef HAVE_LIBAVAHI_CLIENT
	RemminaAvahi *	avahi;
#endif
	guint32		popup_time;
	gchar *		autostart_file;
} RemminaIcon;

static RemminaIcon remmina_icon =
{ 0 };

void remmina_icon_destroy(void)
{
	TRACE_CALL(__func__);
	if (remmina_icon.icon) {
		app_indicator_set_status(remmina_icon.icon, APP_INDICATOR_STATUS_PASSIVE);
		remmina_icon.icon = NULL;
	}
#ifdef HAVE_LIBAVAHI_CLIENT
	if (remmina_icon.avahi) {
		remmina_avahi_free(remmina_icon.avahi);
		remmina_icon.avahi = NULL;
	}
#endif
	if (remmina_icon.autostart_file) {
		g_free(remmina_icon.autostart_file);
		remmina_icon.autostart_file = NULL;
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

#ifdef HAVE_LIBAVAHI_CLIENT
static void remmina_icon_enable_avahi(GtkCheckMenuItem *checkmenuitem, gpointer data)
{
	TRACE_CALL(__func__);
	if (!remmina_icon.avahi)
		return;

	if (gtk_check_menu_item_get_active(checkmenuitem)) {
		remmina_pref.applet_enable_avahi = TRUE;
		if (!remmina_icon.avahi->started)
			remmina_avahi_start(remmina_icon.avahi);
	} else {
		remmina_pref.applet_enable_avahi = FALSE;
		remmina_avahi_stop(remmina_icon.avahi);
	}
	remmina_pref_save();
}
#endif

static void remmina_icon_populate_additional_menu_item(GtkWidget *menu)
{
	TRACE_CALL(__func__);
	GtkWidget *menuitem;


#ifdef HAVE_LIBAVAHI_CLIENT

	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);

	menuitem = gtk_check_menu_item_new_with_label(_("Enable Service Discovery"));
	if (remmina_pref.applet_enable_avahi)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	gtk_widget_show(menuitem);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(remmina_icon_enable_avahi), NULL);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);


#endif

	menuitem = gtk_separator_menu_item_new();
	gtk_widget_show(menuitem);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);

	menuitem = gtk_menu_item_new_with_mnemonic(_("_Quit"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_exec_exitremmina_one_confirm), NULL);

	menuitem = gtk_menu_item_new_with_mnemonic(_("_About"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_about), NULL);

	menuitem = gtk_menu_item_new_with_mnemonic(_("_Preferences"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_preferences), NULL);

	menuitem = gtk_menu_item_new_with_label(_("Open Main Window"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_icon_main), NULL);


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

	new_ontop = remmina_pref.applet_new_ontop;

#ifdef HAVE_LIBAVAHI_CLIENT
	GHashTableIter iter;
	gchar *tmp;
	/* Iterate all discovered services from Avahi */
	if (remmina_icon.avahi) {
		g_hash_table_iter_init(&iter, remmina_icon.avahi->discovered_services);
		while (g_hash_table_iter_next(&iter, NULL, (gpointer *)&tmp)) {
			menuitem = remmina_applet_menu_item_new(REMMINA_APPLET_MENU_ITEM_DISCOVERED, tmp);
			gtk_widget_show(menuitem);
			remmina_applet_menu_add_item(REMMINA_APPLET_MENU(menu), REMMINA_APPLET_MENU_ITEM(menuitem));
		}
	}
#endif

	/* New Connection */
	menuitem = remmina_applet_menu_item_new(REMMINA_APPLET_MENU_ITEM_NEW);
	gtk_widget_show(menuitem);
	remmina_applet_menu_register_item(REMMINA_APPLET_MENU(menu), REMMINA_APPLET_MENU_ITEM(menuitem));
	if (new_ontop)
		gtk_menu_shell_prepend(GTK_MENU_SHELL(menu), menuitem);
	else
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	g_signal_connect(G_OBJECT(menu), "launch-item", G_CALLBACK(remmina_icon_on_launch_item), NULL);
	g_signal_connect(G_OBJECT(menu), "edit-item", G_CALLBACK(remmina_icon_on_edit_item), NULL);
}

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
		

		menuitem = gtk_separator_menu_item_new();
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		remmina_icon_populate_additional_menu_item(menu);
		remmina_icon_populate_extra_menu_item(menu);
	}
}

static void remmina_icon_save_autostart_file(GKeyFile *gkeyfile)
{
	TRACE_CALL(__func__);
	gchar *content;
	gsize length;

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	if (remmina_icon.autostart_file != NULL) {
		g_file_set_contents(remmina_icon.autostart_file, content, length, NULL);
	}
	else {
		REMMINA_WARNING("Cannot save remmina icon autostart file. Uncheck Preferences -> Applet -> No Tray Icon to recreate it.");
	}
	g_free(content);
}

static void remmina_icon_create_autostart_file(void)
{
	TRACE_CALL(__func__);

	gchar *autostart_dir;
	autostart_dir = g_strdup_printf("%s/.config/autostart", g_get_home_dir());

	// If autostart file already exists, return
	// If not, check if the autostart directory exists
	if (g_file_test(remmina_icon.autostart_file, G_FILE_TEST_EXISTS)) {
		g_free(autostart_dir);
		return;
	}
	else if (!g_file_test(autostart_dir, G_FILE_TEST_IS_DIR)) {
		gint result = g_mkdir_with_parents(autostart_dir, 0755);
		if (result != 0) {
			REMMINA_WARNING("Unable to create autostart directory and autostart file.");
			g_free(autostart_dir);
			return;
		}
	}

	GKeyFile *gkeyfile;

	gkeyfile = g_key_file_new();
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Version", "1.0");
	// TRANSLATORS: Applet name as per the Freedesktop Desktop entry specification https://specifications.freedesktop.org/desktop-entry-spec/latest/
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Name", _("Remmina Applet"));
	// TRANSLATORS: Applet comment/description as per the Freedesktop Desktop entry specification https://specifications.freedesktop.org/desktop-entry-spec/latest/
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Comment", _("Connect to remote desktops through the applet menu"));
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Icon", REMMINA_APP_ID);
	if (getenv("FLATPAK_ID")){
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Exec", "flatpak run org.remmina.Remmina -i");
	}
	else{
		g_key_file_set_string(gkeyfile, "Desktop Entry", "Exec", "remmina -i");
	}
	g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Terminal", FALSE);
	g_key_file_set_string(gkeyfile, "Desktop Entry", "Type", "Application");
	g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Hidden", FALSE);
	remmina_icon_save_autostart_file(gkeyfile);
	g_key_file_free(gkeyfile);
	g_free(autostart_dir);
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

	if (!remmina_icon.icon)
		return FALSE;
	if (remmina_pref.disable_tray_icon)
		return FALSE;

	if (remmina_icon.indicator_connected == FALSE) {
		REMMINA_DEBUG("Indicator is not connected to panel, thus it cannot be displayed.");
		return FALSE;
	} else {
		REMMINA_DEBUG("Indicator is connected to panel, thus it can be displayed.");
		return TRUE;
	}
	/** Special treatment under GNOME Shell
	 * Remmina > v1.4.18 won't be shipped in distributions with GNOME Shell <= 3.18
	 * therefore checking the the GNOME Shell version is useless.
	 * We just return TRUE
	 */
	return TRUE;
}

static void
remmina_icon_connection_changed_cb(AppIndicator *indicator, gboolean connected, gpointer data)
{
	TRACE_CALL(__func__);
	REMMINA_DEBUG("Indicator connection changed to: %d", connected);
	remmina_icon.indicator_connected = connected;
}

void remmina_icon_init(void)
{
	TRACE_CALL(__func__);

	gchar remmina_panel[29];
	gboolean sni_supported;

	g_stpcpy(remmina_panel, "org.remmina.Remmina-status");

	/* Print on stdout the availability of appindicators on DBUS */
	sni_supported = remmina_sysinfo_is_appindicator_available();

	g_autofree gchar *wmnameu = remmina_sysinfo_get_wm_name();
	g_autofree gchar *wmname = g_ascii_strdown(wmnameu, -1);
	//TRANSLATORS: These are Linux desktop components to show icons in the system tray, after the “ there's the Desktop Name (like GNOME).
	g_autofree gchar *msg = g_strconcat(
		_("StatusNotifier/Appindicator support in “"),
		wmname,
		"”:",
		NULL);

	if (sni_supported) {
		//TRANSLATORS: %s is a placeholder for "StatusNotifier/Appindicator suppor in “DESKTOP NAME”: "
		REMMINA_INFO(_("%s your desktop does support it"), msg);
		//TRANSLATORS: %s is a placeholder for "StatusNotifier/Appindicator suppor in “DESKTOP NAME”: "
		REMMINA_INFO(_("%s and Remmina has built-in (compiled) support for libappindicator."), msg);
	} else {
		//TRANSLATORS: %s is a placeholder for "StatusNotifier/Appindicator suppor in “DESKTOP NAME”: "
		REMMINA_INFO(_("%s not supported natively by your Desktop Environment. libappindicator will try to fallback to GtkStatusIcon/xembed"), msg);
	}
	if (g_strrstr(wmname, "mate") != NULL)
		//TRANSLATORS: %s is a placeholder for "StatusNotifier/Appindicator suppor in “DESKTOP NAME”: "
		REMMINA_INFO(_("%s You may need to install, and use XApp Status Applet"), msg);
	if (g_strrstr(wmname, "kde") != NULL)
		//TRANSLATORS: %s is a placeholder for "StatusNotifier/Appindicator suppor in “DESKTOP NAME”: "
		REMMINA_INFO(_("%s You may need to install, and use KStatusNotifierItem"), msg);
	if (g_strrstr(wmname, "plasma") != NULL)
		//TRANSLATORS: %s is a placeholder for "StatusNotifier/Appindicator suppor in “DESKTOP NAME”: "
		REMMINA_INFO(_("%s You may need to install, and use XEmbed SNI Proxy"), msg);
	if (g_strrstr(wmname, "gnome") != NULL)
		//TRANSLATORS: %s is a placeholder for "StatusNotifier/Appindicator suppor in “DESKTOP NAME”: "
		REMMINA_INFO(_("%s You may need to install, and use Gnome Shell Extension Appindicator"), msg);

	if (!remmina_icon.icon && !remmina_pref.disable_tray_icon) {
		remmina_icon.icon = app_indicator_new("remmina-icon", remmina_panel, APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
		app_indicator_set_status(remmina_icon.icon, APP_INDICATOR_STATUS_ACTIVE);
		app_indicator_set_title(remmina_icon.icon, "Remmina");
		remmina_icon_populate_menu();
	} else if (remmina_icon.icon) {
		app_indicator_set_status(remmina_icon.icon, remmina_pref.disable_tray_icon ?
					 APP_INDICATOR_STATUS_PASSIVE : APP_INDICATOR_STATUS_ACTIVE);
		/* With libappindicator we can also change the icon on the fly */
		app_indicator_set_icon(remmina_icon.icon, remmina_panel);
	}
	remmina_icon.indicator_connected = TRUE;
#ifdef HAVE_LIBAVAHI_CLIENT
	if (!remmina_icon.avahi)
		remmina_icon.avahi = remmina_avahi_new();
	if (remmina_icon.avahi) {
		if (remmina_pref.applet_enable_avahi) {
			if (!remmina_icon.avahi->started)
				remmina_avahi_start(remmina_icon.avahi);
		} else {
			remmina_avahi_stop(remmina_icon.avahi);
		}
	}
#endif
	if (!remmina_icon.autostart_file && !remmina_pref.disable_tray_icon) {
		remmina_icon.autostart_file = g_strdup_printf("%s/.config/autostart/remmina-applet.desktop", g_get_home_dir());
		remmina_icon_create_autostart_file();
	}
	// "connected" property means a visible indicator, otherwise could be hidden. or fall back to GtkStatusIcon
	if (remmina_icon.icon)
		g_signal_connect(G_OBJECT(remmina_icon.icon), "connection-changed", G_CALLBACK(remmina_icon_connection_changed_cb), NULL);
}

gboolean remmina_icon_is_autostart(void)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gboolean b;

	gkeyfile = g_key_file_new();

	if (remmina_icon.autostart_file != NULL) {
		g_key_file_load_from_file(gkeyfile, remmina_icon.autostart_file, G_KEY_FILE_NONE, NULL);
	}
	else {
		REMMINA_WARNING("Cannot load remmina icon autostart file. Uncheck Preferences -> Applet -> No Tray Icon to recreate it.");
	}

	b = !g_key_file_get_boolean(gkeyfile, "Desktop Entry", "Hidden", NULL);
	g_key_file_free(gkeyfile);
	return b;
}

void remmina_icon_set_autostart(gboolean autostart)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gboolean b;

	if (remmina_icon.autostart_file != NULL) {
		gkeyfile = g_key_file_new();
		g_key_file_load_from_file(gkeyfile, remmina_icon.autostart_file, G_KEY_FILE_NONE, NULL);
		b = !g_key_file_get_boolean(gkeyfile, "Desktop Entry", "Hidden", NULL);
		if (b != autostart) {
			g_key_file_set_boolean(gkeyfile, "Desktop Entry", "Hidden", !autostart);
			/* Refresh it in case translation is updated */
			// TRANSLATORS: Applet Name as per the Freedesktop Desktop entry specification https://specifications.freedesktop.org/desktop-entry-spec/latest/
			g_key_file_set_string(gkeyfile, "Desktop Entry", "Name", _("Remmina Applet"));
			// TRANSLATORS: Applet comment/description as per the Freedesktop Desktop entry specification https://specifications.freedesktop.org/desktop-entry-spec/latest/
			g_key_file_set_string(gkeyfile, "Desktop Entry", "Comment", _("Connect to remote desktops through the applet menu"));
			remmina_icon_save_autostart_file(gkeyfile);
			g_key_file_free(gkeyfile);
		}
	}
	else {
		REMMINA_WARNING("Cannot load remmina icon autostart file. Uncheck Preferences -> Applet -> No Tray Icon to recreate it.");
	}
}

#else
void remmina_icon_init(void) {};
void remmina_icon_destroy(void) {};
gboolean remmina_icon_is_available(void) {return FALSE;};
void remmina_icon_populate_menu(void) {};
void remmina_icon_set_autostart(gboolean autostart) {} ;
gboolean remmina_icon_is_autostart(void) {return FALSE;};
#endif
