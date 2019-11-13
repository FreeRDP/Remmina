/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "config.h"
#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)
#include <vte/vte.h>
#endif
#include "remmina_sodium.h"
#include "remmina_public.h"
#include "remmina_string_list.h"
#include "remmina_widget_pool.h"
#include "remmina_key_chooser.h"
#include "remmina_plugin_manager.h"
#include "remmina_icon.h"
#include "remmina_pref.h"
#include "remmina_pref_dialog.h"
#include "remmina/remmina_trace_calls.h"

static RemminaPrefDialog *remmina_pref_dialog;

#define GET_OBJECT(object_name) gtk_builder_get_object(remmina_pref_dialog->builder, object_name)

/* Show a key chooser dialog */
void remmina_pref_dialog_on_key_chooser(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaKeyChooserArguments *arguments;

	g_return_if_fail(GTK_IS_BUTTON(widget));

	arguments = remmina_key_chooser_new(GTK_WINDOW(remmina_pref_dialog->dialog), FALSE);
	if (arguments->response != GTK_RESPONSE_CANCEL && arguments->response != GTK_RESPONSE_DELETE_EVENT) {
		gchar *val = remmina_key_chooser_get_value(arguments->keyval, arguments->state);
		gtk_button_set_label(GTK_BUTTON(widget), val);
		g_free(val);
	}
	g_free(arguments);
}

/* Show the available resolutions list dialog */
void remmina_pref_on_button_resolutions_clicked(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkDialog *dialog = remmina_string_list_new(FALSE, NULL);
	remmina_string_list_set_validation_func(remmina_public_resolution_validation_func);
	remmina_string_list_set_text(remmina_pref.resolutions, TRUE);
	remmina_string_list_set_titles(_("Resolutions"), _("Configure the available resolutions"));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(remmina_pref_dialog->dialog));
	gtk_dialog_run(dialog);
	g_free(remmina_pref.resolutions);
	remmina_pref.resolutions = remmina_string_list_get_text();
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

/* Re-initialize the remmina_pref_init to reload the color scheme when a color scheme
 * file is selected*/
void remmina_pref_on_color_scheme_selected(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	gchar *sourcepath;
	gchar *remmina_dir;
	gchar *destpath;
	GFile *source;
	GFile *destination;

	sourcepath = gtk_file_chooser_get_filename(remmina_pref_dialog->button_term_cs);
	source = g_file_new_for_path(sourcepath);

	remmina_dir = g_build_path( "/", g_get_user_config_dir(), "remmina", NULL);
	/* /home/foo/.config/remmina */
	destpath = g_strdup_printf("%s/remmina.colors", remmina_dir);
	destination = g_file_new_for_path(destpath);

	if (g_file_test(sourcepath, G_FILE_TEST_IS_REGULAR)) {
		g_file_copy(   source,
			destination,
			G_FILE_COPY_OVERWRITE,
			NULL,
			NULL,
			NULL,
			NULL);
		/* Here we should reinitialize the widget */
	}
	g_free(sourcepath);
	g_object_unref(source);
}

void remmina_pref_dialog_clear_recent(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkDialog *dialog;

	remmina_pref_clear_recent();
	dialog = GTK_DIALOG(gtk_message_dialog_new(GTK_WINDOW(remmina_pref_dialog->dialog),
			GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			_("Recent lists cleared.")));
	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

/* Configure custom keystrokes to send to the plugins */
void remmina_pref_on_button_keystrokes_clicked(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkDialog *dialog = remmina_string_list_new(TRUE, STRING_DELIMITOR2);
	remmina_string_list_set_text(remmina_pref.keystrokes, TRUE);
	remmina_string_list_set_titles(_("Keystrokes"), _("Configure the keystrokes"));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(remmina_pref_dialog->dialog));
	gtk_dialog_run(dialog);
	g_free(remmina_pref.keystrokes);
	remmina_pref.keystrokes = remmina_string_list_get_text();
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void remmina_prefdiag_unlock_repwd_on_changed(GtkEditable* editable, RemminaPrefDialog *dialog)
{
	TRACE_CALL(__func__);
	GtkCssProvider  *provider;
	const gchar *color;
	const gchar *password;
	const gchar *repassword;

	provider = gtk_css_provider_new();

	password = gtk_entry_get_text(remmina_pref_dialog->unlock_password);
	repassword = gtk_entry_get_text(remmina_pref_dialog->unlock_repassword);
	if (g_strcmp0(password, repassword) == 0) {
		color = g_strdup("green");
	} else {
		color = g_strdup("red");
	}

	if (repassword == NULL || repassword[0] == '\0')
		color = g_strdup("inherit");

	gtk_css_provider_load_from_data(provider,
			g_strdup_printf (
			".unlock_repassword {\n"
			"  color: %s;\n"
			"}\n"
			, color)
			, -1, NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
		GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	gtk_widget_queue_draw(GTK_WIDGET(remmina_pref_dialog->unlock_repassword));
	g_object_unref(provider);

}

void remmina_pref_dialog_on_close_clicked(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	TRACE_CALL(__func__);
	gtk_widget_destroy(GTK_WIDGET(remmina_pref_dialog->dialog));
}

void remmina_pref_on_dialog_destroy(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	gboolean b;
	GdkRGBA color;
	gboolean rebuild_remmina_icon = FALSE;

	remmina_pref.datadir_path = gtk_file_chooser_get_filename(remmina_pref_dialog->filechooserbutton_options_datadir_path);
	if (remmina_pref.datadir_path == NULL)
		remmina_pref.datadir_path = g_strdup("");
	remmina_pref.remmina_file_name = gtk_entry_get_text(remmina_pref_dialog->entry_options_file_name);
	remmina_pref.screenshot_path = gtk_file_chooser_get_filename(remmina_pref_dialog->filechooserbutton_options_screenshots_path);
	remmina_pref.screenshot_name = gtk_entry_get_text(remmina_pref_dialog->entry_options_screenshot_name);
	remmina_pref.deny_screenshot_clipboard = gtk_switch_get_active(GTK_SWITCH(remmina_pref_dialog->switch_options_deny_screenshot_clipboard));
	remmina_pref.save_view_mode = gtk_switch_get_active(GTK_SWITCH(remmina_pref_dialog->switch_options_remember_last_view_mode));
	remmina_pref.use_master_password = gtk_switch_get_active(GTK_SWITCH(remmina_pref_dialog->switch_security_use_master_password));
#if SODIUM_VERSION_INT >= 90200
	remmina_pref.unlock_repassword = gtk_entry_get_text(remmina_pref_dialog->unlock_repassword);
	if (gtk_entry_get_text_length(remmina_pref_dialog->unlock_repassword) != 0)
		remmina_pref.unlock_password = remmina_sodium_pwhash_str(gtk_entry_get_text(remmina_pref_dialog->unlock_password));
#endif
	remmina_pref.screenshot_path = gtk_file_chooser_get_filename(remmina_pref_dialog->filechooserbutton_options_screenshots_path);
	remmina_pref.fullscreen_on_auto = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_fullscreen_on_auto));
	remmina_pref.always_show_tab = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_show_tabs));
	remmina_pref.hide_connection_toolbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_hide_toolbar));
	remmina_pref.hide_searchbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_hide_searchbar));

	b = gtk_switch_get_active(GTK_SWITCH(remmina_pref_dialog->switch_permit_send_stats));
	remmina_pref.periodic_usage_stats_permitted = b;

	remmina_pref.default_action = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_options_double_click);
	remmina_pref.default_mode = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_appearance_view_mode);
	remmina_pref.tab_mode = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_appearance_tab_interface);
	remmina_pref.fullscreen_toolbar_visibility = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_appearance_fullscreen_toolbar_visibility);
	remmina_pref.scale_quality = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_options_scale_quality);
	remmina_pref.ssh_loglevel = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_options_ssh_loglevel);
	remmina_pref.sshtunnel_port = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_options_ssh_port));
	if (remmina_pref.sshtunnel_port <= 0)
		remmina_pref.sshtunnel_port = DEFAULT_SSHTUNNEL_PORT;
	remmina_pref.ssh_tcp_keepidle = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_options_ssh_tcp_keepidle));
	if (remmina_pref.ssh_tcp_keepidle <= 0)
		remmina_pref.ssh_tcp_keepidle = SSH_SOCKET_TCP_KEEPIDLE;
	remmina_pref.ssh_tcp_keepintvl = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_options_ssh_tcp_keepintvl));
	if (remmina_pref.ssh_tcp_keepintvl <= 0)
		remmina_pref.ssh_tcp_keepintvl = SSH_SOCKET_TCP_KEEPINTVL;
	remmina_pref.ssh_tcp_keepcnt = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_options_ssh_tcp_keepcnt));
	if (remmina_pref.ssh_tcp_keepcnt <= 0)
		remmina_pref.ssh_tcp_keepcnt = SSH_SOCKET_TCP_KEEPCNT;
	remmina_pref.ssh_tcp_usrtimeout = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_options_ssh_tcp_usrtimeout));
	if (remmina_pref.ssh_tcp_usrtimeout <= 0)
		remmina_pref.ssh_tcp_usrtimeout = SSH_SOCKET_TCP_USER_TIMEOUT;
	remmina_pref.ssh_parseconfig = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_options_ssh_parseconfig));
#if SODIUM_VERSION_INT >= 90200
	remmina_pref.unlock_timeout = atoi(gtk_entry_get_text(remmina_pref_dialog->unlock_timeout));
	if (remmina_pref.unlock_timeout < 0)
		remmina_pref.unlock_timeout = 0;
#endif

	remmina_pref.auto_scroll_step = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_options_scroll));
	if (remmina_pref.auto_scroll_step < 10)
		remmina_pref.auto_scroll_step = 10;
	else if (remmina_pref.auto_scroll_step > 500)
		remmina_pref.auto_scroll_step = 500;

	remmina_pref.recent_maximum = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_options_recent_items));
	if (remmina_pref.recent_maximum < 0)
		remmina_pref.recent_maximum = 0;

	remmina_pref.applet_new_ontop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_new_connection_on_top));
	remmina_pref.applet_hide_count = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_hide_totals));
	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_light_tray));
	if (remmina_pref.dark_tray_icon != b) {
		remmina_pref.dark_tray_icon = b;
		rebuild_remmina_icon = TRUE;
	}

	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_disable_tray));
	if (remmina_pref.disable_tray_icon != b) {
		remmina_pref.disable_tray_icon = b;
		rebuild_remmina_icon = TRUE;
	}
	if (b) {
		b = FALSE;
	}else  {
		b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_start_in_tray));
	}
	remmina_icon_set_autostart(b);

	if (rebuild_remmina_icon) {
		remmina_icon_init();
		remmina_icon_populate_menu();
	}

	remmina_pref.hostkey = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_host_key));
	remmina_pref.shortcutkey_fullscreen = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_fullscreen));
	remmina_pref.shortcutkey_autofit = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_auto_fit));
	remmina_pref.shortcutkey_prevtab = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_switch_tab_left));
	remmina_pref.shortcutkey_nexttab = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_switch_tab_right));
	remmina_pref.shortcutkey_scale = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_scaled));
	remmina_pref.shortcutkey_grab = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_grab_keyboard));
	remmina_pref.shortcutkey_screenshot = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_screenshot));
	remmina_pref.shortcutkey_viewonly = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_viewonly));
	remmina_pref.shortcutkey_minimize = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_minimize));
	remmina_pref.shortcutkey_disconnect = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_disconnect));
	remmina_pref.shortcutkey_toolbar = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_toolbar));

	g_free(remmina_pref.vte_font);
	if (gtk_switch_get_active(GTK_SWITCH(remmina_pref_dialog->switch_terminal_font_system))) {
		remmina_pref.vte_font = NULL;
	}else  {
		remmina_pref.vte_font = g_strdup(gtk_font_chooser_get_font(GTK_FONT_CHOOSER(remmina_pref_dialog->fontbutton_terminal_font)));
	}
	remmina_pref.vte_allow_bold_text = gtk_switch_get_active(GTK_SWITCH(remmina_pref_dialog->switch_terminal_bold));
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_foreground), &color);
	remmina_pref.color_pref.foreground = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_background), &color);
	remmina_pref.color_pref.background = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_cursor), &color);
	remmina_pref.color_pref.cursor = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color0), &color);
	remmina_pref.color_pref.color0 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color1), &color);
	remmina_pref.color_pref.color1 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color2), &color);
	remmina_pref.color_pref.color2 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color3), &color);
	remmina_pref.color_pref.color3 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color4), &color);
	remmina_pref.color_pref.color4 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color5), &color);
	remmina_pref.color_pref.color5 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color6), &color);
	remmina_pref.color_pref.color6 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color7), &color);
	remmina_pref.color_pref.color7 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color8), &color);
	remmina_pref.color_pref.color8 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color9), &color);
	remmina_pref.color_pref.color9 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color10), &color);
	remmina_pref.color_pref.color10 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color11), &color);
	remmina_pref.color_pref.color11 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color12), &color);
	remmina_pref.color_pref.color12 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color13), &color);
	remmina_pref.color_pref.color13 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color14), &color);
	remmina_pref.color_pref.color14 = gdk_rgba_to_string(&color);
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color15), &color);
	remmina_pref.color_pref.color15 = gdk_rgba_to_string(&color);
	remmina_pref.vte_lines = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_scrollback_lines));
	remmina_pref.vte_lines = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_scrollback_lines));
	remmina_pref.vte_shortcutkey_copy = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_copy));
	remmina_pref.vte_shortcutkey_paste = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_paste));
	remmina_pref.vte_shortcutkey_select_all = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_select_all));

	remmina_pref_save();
	remmina_pref_init();

	remmina_pref_dialog->dialog = NULL;
}

static gboolean remmina_pref_dialog_add_pref_plugin(gchar *name, RemminaPlugin *plugin, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaPrefPlugin *pref_plugin;
	GtkWidget *vbox;
	GtkWidget *widget;

	pref_plugin = (RemminaPrefPlugin*)plugin;

	widget = gtk_label_new(pref_plugin->pref_label);
	gtk_widget_show(widget);

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(remmina_pref_dialog->notebook_preferences), vbox, widget);

	widget = pref_plugin->get_pref_body();
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	return FALSE;
}

void remmina_pref_dialog_vte_font_on_toggled(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	TRACE_CALL(__func__);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->fontbutton_terminal_font), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

void remmina_pref_dialog_disable_tray_icon_on_toggled(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	TRACE_CALL(__func__);
	gboolean b;

	b = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->checkbutton_applet_start_in_tray), b);
}

/* Helper function for remmina_pref_dialog_init() */
static void remmina_pref_dialog_set_button_label(GtkButton *button, guint keyval)
{
	gchar *val;

	val = remmina_key_chooser_get_value(keyval, 0);
	gtk_button_set_label(button, val);
	g_free(val);
}

/* Remmina preferences initialization */
static void remmina_pref_dialog_init(void)
{
	TRACE_CALL(__func__);
	gchar buf[100];
	GdkRGBA color;

#if !defined (HAVE_LIBSSH) || !defined (HAVE_LIBVTE)
	GtkWidget *align;
#endif

#if !defined (HAVE_LIBVTE)
	align = GTK_WIDGET(GET_OBJECT("alignment_terminal"));
	gtk_widget_set_sensitive(align, FALSE);
#endif

#if !defined (HAVE_LIBSSH)
	align = GTK_WIDGET(GET_OBJECT("alignment_ssh"));
	gtk_widget_set_sensitive(align, FALSE);
#endif

	gtk_dialog_set_default_response(GTK_DIALOG(remmina_pref_dialog->dialog), GTK_RESPONSE_CLOSE);

	gtk_switch_set_active(GTK_SWITCH(remmina_pref_dialog->switch_options_remember_last_view_mode), remmina_pref.save_view_mode);
#if SODIUM_VERSION_INT >= 90200
	gtk_switch_set_active(GTK_SWITCH(remmina_pref_dialog->switch_security_use_master_password), remmina_pref.use_master_password);
	if (remmina_pref.unlock_password != NULL) {
		gtk_entry_set_text(remmina_pref_dialog->unlock_password, remmina_pref.unlock_password);
	}else{
		gtk_entry_set_text(remmina_pref_dialog->unlock_password, "");
	}
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->switch_security_use_master_password), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->unlock_password), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->unlock_repassword), TRUE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->unlock_timeout), TRUE);
#else
	gtk_switch_set_active(GTK_SWITCH(remmina_pref_dialog->switch_security_use_master_password), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->switch_security_use_master_password), FALSE);
	// TRANSLATORS: Do not translate libsodium, is the name of a library
	gtk_widget_set_tooltip_text (GTK_WIDGET(remmina_pref_dialog->switch_security_use_master_password), _("libsodium >= 1.9.0 is required to use master password"));
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->unlock_password), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->unlock_repassword), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->unlock_timeout), FALSE);
#endif

	gtk_switch_set_active(GTK_SWITCH(remmina_pref_dialog->switch_options_deny_screenshot_clipboard), remmina_pref.deny_screenshot_clipboard);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_fullscreen_on_auto), remmina_pref.fullscreen_on_auto);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_show_tabs), remmina_pref.always_show_tab);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_hide_toolbar), remmina_pref.hide_connection_toolbar);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_hide_searchbar), remmina_pref.hide_searchbar);

	gtk_switch_set_active(GTK_SWITCH(remmina_pref_dialog->switch_permit_send_stats), remmina_pref.periodic_usage_stats_permitted);

	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.sshtunnel_port);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_ssh_port, buf);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.ssh_tcp_keepidle);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_ssh_tcp_keepidle, buf);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.ssh_tcp_keepintvl);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_ssh_tcp_keepintvl, buf);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.ssh_tcp_keepcnt);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_ssh_tcp_keepcnt, buf);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.ssh_tcp_usrtimeout);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_ssh_tcp_usrtimeout, buf);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.auto_scroll_step);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_scroll, buf);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.recent_maximum);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_recent_items, buf);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_new_connection_on_top), remmina_pref.applet_new_ontop);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_hide_totals), remmina_pref.applet_hide_count);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_disable_tray), remmina_pref.disable_tray_icon);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_light_tray), remmina_pref.dark_tray_icon);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_start_in_tray), remmina_icon_is_autostart());
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->checkbutton_applet_start_in_tray), !remmina_pref.disable_tray_icon);

	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_host_key, remmina_pref.hostkey);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_fullscreen, remmina_pref.shortcutkey_fullscreen);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_auto_fit, remmina_pref.shortcutkey_autofit);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_switch_tab_left, remmina_pref.shortcutkey_prevtab);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_switch_tab_right, remmina_pref.shortcutkey_nexttab);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_scaled, remmina_pref.shortcutkey_scale);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_grab_keyboard, remmina_pref.shortcutkey_grab);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_screenshot, remmina_pref.shortcutkey_screenshot);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_viewonly, remmina_pref.shortcutkey_viewonly);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_minimize, remmina_pref.shortcutkey_minimize);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_disconnect, remmina_pref.shortcutkey_disconnect);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_toolbar, remmina_pref.shortcutkey_toolbar);

	if (!(remmina_pref.vte_font && remmina_pref.vte_font[0])) {
		gtk_switch_set_active(GTK_SWITCH(remmina_pref_dialog->switch_terminal_font_system), TRUE);
	}
	if (remmina_pref.vte_font && remmina_pref.vte_font[0]) {
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(remmina_pref_dialog->fontbutton_terminal_font), remmina_pref.vte_font);
	}else  {
		gtk_font_chooser_set_font(GTK_FONT_CHOOSER(remmina_pref_dialog->fontbutton_terminal_font), "Monospace 12");
		gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->fontbutton_terminal_font), FALSE);
	}
	gtk_switch_set_active(GTK_SWITCH(remmina_pref_dialog->switch_terminal_bold), remmina_pref.vte_allow_bold_text);

	/* Foreground color option */
	gdk_rgba_parse(&color, remmina_pref.color_pref.foreground);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_foreground), &color);
	/* Background color option */
	gdk_rgba_parse(&color, remmina_pref.color_pref.background);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_background), &color);
	/* Cursor color option */
	gdk_rgba_parse(&color, remmina_pref.color_pref.cursor);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_cursor), &color);
	/* 16 colors */
	gdk_rgba_parse(&color, remmina_pref.color_pref.color0);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color0), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color1);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color1), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color2);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color2), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color3);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color3), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color4);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color4), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color5);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color5), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color6);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color6), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color7);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color7), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color8);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color8), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color9);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color9), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color10);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color10), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color11);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color11), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color12);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color12), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color13);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color13), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color14);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color14), &color);
	gdk_rgba_parse(&color, remmina_pref.color_pref.color15);
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_color15), &color);
#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)
#if !VTE_CHECK_VERSION(0, 38, 0)
	/* Disable color scheme buttons if old version of VTE */
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_cursor), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color0), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color1), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color2), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color3), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color4), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color5), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color6), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color7), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color8), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color9), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color10), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color11), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color12), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color13), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color14), FALSE);
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->colorbutton_color15), FALSE);
#endif
#endif

	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.vte_lines);
	gtk_entry_set_text(remmina_pref_dialog->entry_scrollback_lines, buf);

#if SODIUM_VERSION_INT >= 90200
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.unlock_timeout);
	gtk_entry_set_text(remmina_pref_dialog->unlock_timeout, buf);
#endif

	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_options_double_click, remmina_pref.default_action);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_appearance_view_mode, remmina_pref.default_mode);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_appearance_tab_interface, remmina_pref.tab_mode);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_appearance_fullscreen_toolbar_visibility, remmina_pref.fullscreen_toolbar_visibility);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_options_scale_quality, remmina_pref.scale_quality);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_options_ssh_loglevel, remmina_pref.ssh_loglevel);
	if (remmina_pref.datadir_path != NULL && strlen(remmina_pref.datadir_path) > 0) {
		gtk_file_chooser_set_filename(remmina_pref_dialog->filechooserbutton_options_datadir_path, remmina_pref.datadir_path);
	}
	if (remmina_pref.remmina_file_name != NULL) {
		gtk_entry_set_text(remmina_pref_dialog->entry_options_file_name, remmina_pref.remmina_file_name);
	}else{
		gtk_entry_set_text(remmina_pref_dialog->entry_options_file_name, "%G_%P_%N_%h.remmina");
	}
	if (remmina_pref.screenshot_path != NULL) {
		gtk_file_chooser_set_filename(remmina_pref_dialog->filechooserbutton_options_screenshots_path, remmina_pref.screenshot_path);
	}else{
		gtk_file_chooser_set_filename(remmina_pref_dialog->filechooserbutton_options_screenshots_path, g_get_home_dir());
	}
	if (remmina_pref.screenshot_name != NULL) {
		gtk_entry_set_text(remmina_pref_dialog->entry_options_screenshot_name, remmina_pref.screenshot_name);
	}else{
		gtk_entry_set_text(remmina_pref_dialog->entry_options_screenshot_name, "remmina_%p_%h_%Y%m%d-%H%M%S");
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_options_ssh_parseconfig), remmina_pref.ssh_parseconfig);

	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_copy, remmina_pref.vte_shortcutkey_copy);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_paste, remmina_pref.vte_shortcutkey_paste);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_select_all, remmina_pref.vte_shortcutkey_select_all);

	remmina_plugin_manager_for_each_plugin(REMMINA_PLUGIN_TYPE_PREF, remmina_pref_dialog_add_pref_plugin, remmina_pref_dialog->dialog);

	g_object_set_data(G_OBJECT(remmina_pref_dialog->dialog), "tag", "remmina-pref-dialog");
	remmina_widget_pool_register(GTK_WIDGET(remmina_pref_dialog->dialog));
}

/* RemminaPrefDialog instance */
GtkDialog* remmina_pref_dialog_new(gint default_tab, GtkWindow *parent)
{
	TRACE_CALL(__func__);

	remmina_pref_dialog = g_new0(RemminaPrefDialog, 1);
	remmina_pref_dialog->priv = g_new0(RemminaPrefDialogPriv, 1);

	remmina_pref_dialog->builder = remmina_public_gtk_builder_new_from_file("remmina_preferences.glade");
	remmina_pref_dialog->dialog = GTK_DIALOG(gtk_builder_get_object(remmina_pref_dialog->builder, "RemminaPrefDialog"));
	if (parent)
		gtk_window_set_transient_for(GTK_WINDOW(remmina_pref_dialog->dialog), parent);

	remmina_pref_dialog->notebook_preferences = GTK_NOTEBOOK(GET_OBJECT("notebook_preferences"));

	remmina_pref_dialog->filechooserbutton_options_datadir_path = GTK_FILE_CHOOSER(GET_OBJECT("filechooserbutton_options_datadir_path"));
	remmina_pref_dialog->entry_options_file_name = GTK_ENTRY(GET_OBJECT("entry_options_file_name"));
	remmina_pref_dialog->filechooserbutton_options_screenshots_path = GTK_FILE_CHOOSER(GET_OBJECT("filechooserbutton_options_screenshots_path"));
	remmina_pref_dialog->entry_options_screenshot_name = GTK_ENTRY(GET_OBJECT("entry_options_screenshot_name"));
	remmina_pref_dialog->switch_options_deny_screenshot_clipboard = GTK_SWITCH(GET_OBJECT("switch_options_deny_screenshot_clipboard"));
	remmina_pref_dialog->switch_options_remember_last_view_mode = GTK_SWITCH(GET_OBJECT("switch_options_remember_last_view_mode"));
	remmina_pref_dialog->switch_security_use_master_password = GTK_SWITCH(GET_OBJECT("switch_security_use_master_password"));
	remmina_pref_dialog->unlock_password = GTK_ENTRY(GET_OBJECT("unlock_password"));
	remmina_pref_dialog->unlock_repassword = GTK_ENTRY(GET_OBJECT("unlock_repassword"));
	remmina_pref_dialog->checkbutton_options_save_settings = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_options_save_settings"));
	remmina_pref_dialog->checkbutton_appearance_fullscreen_on_auto = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_appearance_fullscreen_on_auto"));
	remmina_pref_dialog->checkbutton_appearance_show_tabs = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_appearance_show_tabs"));
	remmina_pref_dialog->checkbutton_appearance_hide_toolbar = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_appearance_hide_toolbar"));
	remmina_pref_dialog->checkbutton_appearance_hide_searchbar = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_appearance_hide_searchbar"));
	remmina_pref_dialog->switch_permit_send_stats = GTK_SWITCH(GET_OBJECT("switch_permit_send_stats"));
	remmina_pref_dialog->comboboxtext_options_double_click = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_options_double_click"));
	remmina_pref_dialog->comboboxtext_appearance_view_mode = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_appearance_view_mode"));
	remmina_pref_dialog->comboboxtext_appearance_tab_interface = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_appearance_tab_interface"));
	remmina_pref_dialog->comboboxtext_appearance_fullscreen_toolbar_visibility = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_appearance_fullscreen_toolbar_visibility"));
	remmina_pref_dialog->comboboxtext_options_scale_quality = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_options_scale_quality"));
	remmina_pref_dialog->checkbutton_options_ssh_parseconfig = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_options_ssh_parseconfig"));
	remmina_pref_dialog->comboboxtext_options_ssh_loglevel = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_options_ssh_loglevel"));
	remmina_pref_dialog->entry_options_ssh_port = GTK_ENTRY(GET_OBJECT("entry_options_ssh_port"));
	remmina_pref_dialog->entry_options_ssh_tcp_keepidle = GTK_ENTRY(GET_OBJECT("entry_options_ssh_tcp_keepidle"));
	remmina_pref_dialog->entry_options_ssh_tcp_keepintvl = GTK_ENTRY(GET_OBJECT("entry_options_ssh_tcp_keepintvl"));
	remmina_pref_dialog->entry_options_ssh_tcp_keepcnt = GTK_ENTRY(GET_OBJECT("entry_options_ssh_tcp_keepcnt"));
	remmina_pref_dialog->entry_options_ssh_tcp_usrtimeout = GTK_ENTRY(GET_OBJECT("entry_options_ssh_tcp_usrtimeout"));
	remmina_pref_dialog->entry_options_scroll = GTK_ENTRY(GET_OBJECT("entry_options_scroll"));
	remmina_pref_dialog->entry_options_recent_items = GTK_ENTRY(GET_OBJECT("entry_options_recent_items"));
	remmina_pref_dialog->button_options_recent_items_clear = GTK_BUTTON(GET_OBJECT("button_options_recent_items_clear"));
	remmina_pref_dialog->unlock_timeout = GTK_ENTRY(GET_OBJECT("unlock_timeout"));

	remmina_pref_dialog->checkbutton_applet_new_connection_on_top = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_new_connection_on_top"));
	remmina_pref_dialog->checkbutton_applet_hide_totals = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_hide_totals"));
	remmina_pref_dialog->checkbutton_applet_disable_tray = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_disable_tray"));
	remmina_pref_dialog->checkbutton_applet_light_tray = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_light_tray"));
	remmina_pref_dialog->checkbutton_applet_start_in_tray = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_start_in_tray"));

	remmina_pref_dialog->button_keyboard_host_key = GTK_BUTTON(GET_OBJECT("button_keyboard_host_key"));
	remmina_pref_dialog->button_keyboard_fullscreen = GTK_BUTTON(GET_OBJECT("button_keyboard_fullscreen"));
	remmina_pref_dialog->button_keyboard_auto_fit = GTK_BUTTON(GET_OBJECT("button_keyboard_auto_fit"));
	remmina_pref_dialog->button_keyboard_switch_tab_left = GTK_BUTTON(GET_OBJECT("button_keyboard_switch_tab_left"));
	remmina_pref_dialog->button_keyboard_switch_tab_right = GTK_BUTTON(GET_OBJECT("button_keyboard_switch_tabright"));
	remmina_pref_dialog->button_keyboard_scaled = GTK_BUTTON(GET_OBJECT("button_keyboard_scaled"));
	remmina_pref_dialog->button_keyboard_grab_keyboard = GTK_BUTTON(GET_OBJECT("button_keyboard_grab_keyboard"));
	remmina_pref_dialog->button_keyboard_screenshot = GTK_BUTTON(GET_OBJECT("button_keyboard_screenshot"));
	remmina_pref_dialog->button_keyboard_viewonly = GTK_BUTTON(GET_OBJECT("button_keyboard_viewonly"));
	remmina_pref_dialog->button_keyboard_minimize = GTK_BUTTON(GET_OBJECT("button_keyboard_minimize"));
	remmina_pref_dialog->button_keyboard_disconnect = GTK_BUTTON(GET_OBJECT("button_keyboard_disconnect"));
	remmina_pref_dialog->button_keyboard_toolbar = GTK_BUTTON(GET_OBJECT("button_keyboard_toolbar"));

	remmina_pref_dialog->switch_terminal_font_system = GTK_SWITCH(GET_OBJECT("switch_terminal_font_system"));
	remmina_pref_dialog->fontbutton_terminal_font = GTK_FONT_BUTTON(GET_OBJECT("fontbutton_terminal_font"));
	remmina_pref_dialog->switch_terminal_bold = GTK_SWITCH(GET_OBJECT("switch_terminal_bold"));
	remmina_pref_dialog->entry_scrollback_lines = GTK_ENTRY(GET_OBJECT("entry_scrollback_lines"));
	remmina_pref_dialog->button_keyboard_copy = GTK_BUTTON(GET_OBJECT("button_keyboard_copy"));
	remmina_pref_dialog->button_keyboard_paste = GTK_BUTTON(GET_OBJECT("button_keyboard_paste"));
	remmina_pref_dialog->button_keyboard_select_all = GTK_BUTTON(GET_OBJECT("button_keyboard_select_all"));
	remmina_pref_dialog->label_terminal_foreground = GTK_LABEL(GET_OBJECT("label_terminal_foreground"));
	remmina_pref_dialog->colorbutton_foreground = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_foreground"));
	remmina_pref_dialog->label_terminal_background = GTK_LABEL(GET_OBJECT("label_terminal_background"));
	remmina_pref_dialog->colorbutton_background = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_background"));
	remmina_pref_dialog->label_terminal_cursor_color = GTK_LABEL(GET_OBJECT("label_terminal_cursor_color"));
	remmina_pref_dialog->colorbutton_cursor = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_cursor"));
	remmina_pref_dialog->label_terminal_normal_colors = GTK_LABEL(GET_OBJECT("label_terminal_normal_colors"));
	remmina_pref_dialog->colorbutton_color0 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color0"));
	remmina_pref_dialog->colorbutton_color1 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color1"));
	remmina_pref_dialog->colorbutton_color2 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color2"));
	remmina_pref_dialog->colorbutton_color3 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color3"));
	remmina_pref_dialog->colorbutton_color4 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color4"));
	remmina_pref_dialog->colorbutton_color5 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color5"));
	remmina_pref_dialog->colorbutton_color6 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color6"));
	remmina_pref_dialog->colorbutton_color7 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color7"));
	remmina_pref_dialog->label_terminal_bright_colors = GTK_LABEL(GET_OBJECT("label_terminal_bright_colors"));
	remmina_pref_dialog->colorbutton_color8 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color8"));
	remmina_pref_dialog->colorbutton_color9 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color9"));
	remmina_pref_dialog->colorbutton_color10 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color10"));
	remmina_pref_dialog->colorbutton_color11 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color11"));
	remmina_pref_dialog->colorbutton_color12 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color12"));
	remmina_pref_dialog->colorbutton_color13 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color13"));
	remmina_pref_dialog->colorbutton_color14 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color14"));
	remmina_pref_dialog->colorbutton_color15 = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_color15"));
#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)
#if VTE_CHECK_VERSION(0, 38, 0)
	remmina_pref_dialog->button_term_cs = GTK_FILE_CHOOSER(GET_OBJECT("button_term_cs"));
	gtk_file_chooser_set_current_folder( GTK_FILE_CHOOSER(remmina_pref_dialog->button_term_cs), REMMINA_RUNTIME_TERM_CS_DIR);
#endif
#endif

	/* Connect signals */
	gtk_builder_connect_signals(remmina_pref_dialog->builder, NULL);
	/* Initialize the window and load the preferences */
	remmina_pref_dialog_init();

	if (default_tab > 0)
		gtk_notebook_set_current_page(remmina_pref_dialog->notebook_preferences, default_tab);
	return remmina_pref_dialog->dialog;
}

GtkDialog* remmina_pref_dialog_get_dialog()
{
	if (!remmina_pref_dialog)
		return NULL;
	return remmina_pref_dialog->dialog;
}
