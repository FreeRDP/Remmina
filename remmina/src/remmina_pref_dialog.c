/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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
	TRACE_CALL("remmina_pref_dialog_on_key_chooser");
	RemminaKeyChooserArguments *arguments;

	g_return_if_fail(GTK_IS_BUTTON(widget));

	arguments = remmina_key_chooser_new(GTK_WINDOW(remmina_pref_dialog->dialog), FALSE);
	if (arguments->response != GTK_RESPONSE_CANCEL && arguments->response != GTK_RESPONSE_DELETE_EVENT)
	{
		gchar *val = remmina_key_chooser_get_value(arguments->keyval, arguments->state);
		gtk_button_set_label(GTK_BUTTON(widget), val);
		g_free(val);
	}
	g_free(arguments);
}

/* Show the available resolutions list dialog */
void remmina_pref_on_button_resolutions_clicked(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL("remmina_pref_on_button_resolutions_clicked");
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

void remmina_pref_dialog_clear_recent(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL("remmina_pref_dialog_clear_recent");
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
	TRACE_CALL("remmina_pref_on_button_keystrokes_clicked");
	GtkDialog *dialog = remmina_string_list_new(TRUE, STRING_DELIMITOR2);
	remmina_string_list_set_text(remmina_pref.keystrokes, TRUE);
	remmina_string_list_set_titles(_("Keystrokes"), _("Configure the keystrokes"));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(remmina_pref_dialog->dialog));
	gtk_dialog_run(dialog);
	g_free(remmina_pref.keystrokes);
	remmina_pref.keystrokes = remmina_string_list_get_text();
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

void remmina_pref_dialog_on_close_clicked(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	TRACE_CALL("remmina_pref_dialog_on_close_clicked");
	gtk_widget_destroy(GTK_WIDGET(remmina_pref_dialog->dialog));
}

void remmina_pref_on_dialog_destroy(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL("remmina_pref_on_dialog_destroy");
	gboolean b;
	GdkRGBA color;

	remmina_pref.save_view_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_options_remember_last_view_mode));
	remmina_pref.save_when_connect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_options_save_settings));
	remmina_pref.invisible_toolbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_invisible_toolbar));
	remmina_pref.always_show_tab = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_show_tabs));
	remmina_pref.hide_connection_toolbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_hide_toolbar));

	remmina_pref.default_action = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_options_double_click);
	remmina_pref.default_mode = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_appearance_view_mode);
	remmina_pref.tab_mode = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_appearance_tab_interface);
	remmina_pref.show_buttons_icons = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_appearance_show_buttons_icons);
	remmina_pref.show_menu_icons = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_appearance_show_menu_icons);
	remmina_pref.scale_quality = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_options_scale_quality);
	remmina_pref.ssh_loglevel = gtk_combo_box_get_active(remmina_pref_dialog->comboboxtext_options_ssh_loglevel);
	remmina_pref.ssh_parseconfig = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_options_ssh_parseconfig));

	remmina_pref.sshtunnel_port = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_options_ssh_port));
	if (remmina_pref.sshtunnel_port <= 0)
		remmina_pref.sshtunnel_port = DEFAULT_SSHTUNNEL_PORT;

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
	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_disable_tray));
	if (remmina_pref.disable_tray_icon != b)
	{
		remmina_pref.disable_tray_icon = b;
		remmina_icon_init();
	}
	if (b)
	{
		b = FALSE;
	}
	else
	{
		b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_start_in_tray));
	}
	remmina_icon_set_autostart(b);

	remmina_pref.hostkey = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_host_key));
	remmina_pref.shortcutkey_fullscreen = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_fullscreen));
	remmina_pref.shortcutkey_autofit = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_auto_fit));
	remmina_pref.shortcutkey_prevtab = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_switch_tab_left));
	remmina_pref.shortcutkey_nexttab = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_switch_tab_right));
	remmina_pref.shortcutkey_scale = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_scaled));
	remmina_pref.shortcutkey_grab = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_grab_keyboard));
	remmina_pref.shortcutkey_minimize = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_minimize));
	remmina_pref.shortcutkey_disconnect = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_disconnect));
	remmina_pref.shortcutkey_toolbar = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_toolbar));

	g_free(remmina_pref.vte_font);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_terminal_font_system)))
	{
		remmina_pref.vte_font = NULL;
	}
	else
	{
		remmina_pref.vte_font = g_strdup(gtk_font_button_get_font_name(remmina_pref_dialog->fontbutton_terminal_font));
	}
	remmina_pref.vte_allow_bold_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_terminal_bold));
	remmina_pref.vte_system_colors = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_terminal_system_colors));
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_foreground), &color);
#else
	gtk_color_button_get_rgba(remmina_pref_dialog->colorbutton_foreground, &color);
#endif
	remmina_pref.vte_foreground_color = gdk_rgba_to_string(&color);
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_background), &color);
#else
	gtk_color_button_get_rgba(remmina_pref_dialog->colorbutton_background, &color);
#endif
	remmina_pref.vte_background_color = gdk_rgba_to_string(&color);
	remmina_pref.vte_lines = atoi(gtk_entry_get_text(remmina_pref_dialog->entry_scrollback_lines));
	remmina_pref.vte_shortcutkey_copy = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_copy));
	remmina_pref.vte_shortcutkey_paste = remmina_key_chooser_get_keyval(gtk_button_get_label(remmina_pref_dialog->button_keyboard_paste));

	remmina_pref_save();

	remmina_pref_dialog->dialog = NULL;
}

static gboolean remmina_pref_dialog_add_pref_plugin(gchar *name, RemminaPlugin *plugin, gpointer user_data)
{
	TRACE_CALL("remmina_pref_dialog_add_pref_plugin");
	RemminaPrefPlugin *pref_plugin;
	GtkWidget *vbox;
	GtkWidget *widget;

	pref_plugin = (RemminaPrefPlugin *) plugin;

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
	TRACE_CALL("remmina_pref_dialog_vte_font_on_toggled");
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->fontbutton_terminal_font), !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void remmina_pref_dialog_vte_color_on_toggled(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	TRACE_CALL("remmina_pref_dialog_vte_color_on_toggled");
	gboolean status = !gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget));
	gtk_widget_set_sensitive (GTK_WIDGET(remmina_pref_dialog->label_terminal_foreground), status);
	gtk_widget_set_sensitive (GTK_WIDGET(remmina_pref_dialog->colorbutton_foreground), status);
	gtk_widget_set_sensitive (GTK_WIDGET(remmina_pref_dialog->label_terminal_background), status);
	gtk_widget_set_sensitive (GTK_WIDGET(remmina_pref_dialog->colorbutton_background), status);
}

void remmina_pref_dialog_disable_tray_icon_on_toggled(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	TRACE_CALL("remmina_pref_dialog_disable_tray_icon_on_toggled");
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
	TRACE_CALL("remmina_pref_dialog_init");
	gchar buf[100];
	GdkRGBA color;

	gtk_dialog_set_default_response(GTK_DIALOG(remmina_pref_dialog->dialog), GTK_RESPONSE_CLOSE);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_options_remember_last_view_mode), remmina_pref.save_view_mode);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_options_save_settings), remmina_pref.save_when_connect);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_invisible_toolbar), remmina_pref.invisible_toolbar);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_show_tabs), remmina_pref.always_show_tab);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_appearance_hide_toolbar), remmina_pref.hide_connection_toolbar);

	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.sshtunnel_port);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_ssh_port, buf);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.auto_scroll_step);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_scroll, buf);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.recent_maximum);
	gtk_entry_set_text(remmina_pref_dialog->entry_options_recent_items, buf);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_new_connection_on_top), remmina_pref.applet_new_ontop);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_hide_totals), remmina_pref.applet_hide_count);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_disable_tray), remmina_pref.disable_tray_icon);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_applet_start_in_tray), remmina_icon_is_autostart());
	gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->checkbutton_applet_start_in_tray), !remmina_pref.disable_tray_icon);

	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_host_key, remmina_pref.hostkey);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_fullscreen, remmina_pref.shortcutkey_fullscreen);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_auto_fit, remmina_pref.shortcutkey_autofit);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_switch_tab_left, remmina_pref.shortcutkey_prevtab);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_switch_tab_right, remmina_pref.shortcutkey_nexttab);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_scaled, remmina_pref.shortcutkey_scale);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_grab_keyboard, remmina_pref.shortcutkey_grab);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_minimize, remmina_pref.shortcutkey_minimize);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_disconnect, remmina_pref.shortcutkey_disconnect);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_toolbar, remmina_pref.shortcutkey_toolbar);

	if (!(remmina_pref.vte_font && remmina_pref.vte_font[0]))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_terminal_font_system), TRUE);
	}
	if (remmina_pref.vte_font && remmina_pref.vte_font[0])
	{
		gtk_font_button_set_font_name(remmina_pref_dialog->fontbutton_terminal_font, remmina_pref.vte_font);
	}
	else
	{
		gtk_font_button_set_font_name(remmina_pref_dialog->fontbutton_terminal_font, "Monospace 12");
		gtk_widget_set_sensitive(GTK_WIDGET(remmina_pref_dialog->fontbutton_terminal_font), FALSE);
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_terminal_bold), remmina_pref.vte_allow_bold_text);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_terminal_system_colors), remmina_pref.vte_system_colors);

	/* Foreground color option */
	gdk_rgba_parse(&color, remmina_pref.vte_foreground_color);
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_foreground), &color);
#else
	gtk_color_button_set_rgba(remmina_pref_dialog->colorbutton_foreground, &color);
#endif
	/* Background color option */
	gdk_rgba_parse(&color, remmina_pref.vte_background_color);
#if GTK_CHECK_VERSION(3, 4, 0)
	gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(remmina_pref_dialog->colorbutton_background), &color);
#else
	gtk_color_button_set_rgba(remmina_pref_dialog->colorbutton_background, &color);
#endif

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_terminal_system_colors), remmina_pref.vte_system_colors);
	remmina_pref_dialog_vte_color_on_toggled(GTK_WIDGET(remmina_pref_dialog->checkbutton_terminal_system_colors), NULL);

	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.vte_lines);
	gtk_entry_set_text(remmina_pref_dialog->entry_scrollback_lines, buf);

	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_options_double_click, remmina_pref.default_action);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_appearance_view_mode, remmina_pref.default_mode);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_appearance_tab_interface, remmina_pref.tab_mode);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_appearance_show_buttons_icons, remmina_pref.show_buttons_icons);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_appearance_show_menu_icons, remmina_pref.show_menu_icons);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_options_scale_quality, remmina_pref.scale_quality);
	gtk_combo_box_set_active(remmina_pref_dialog->comboboxtext_options_ssh_loglevel, remmina_pref.ssh_loglevel);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(remmina_pref_dialog->checkbutton_options_ssh_parseconfig), remmina_pref.ssh_parseconfig);

	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_copy, remmina_pref.vte_shortcutkey_copy);
	remmina_pref_dialog_set_button_label(remmina_pref_dialog->button_keyboard_paste, remmina_pref.vte_shortcutkey_paste);

	remmina_plugin_manager_for_each_plugin(REMMINA_PLUGIN_TYPE_PREF, remmina_pref_dialog_add_pref_plugin, remmina_pref_dialog->dialog);

	g_object_set_data(G_OBJECT(remmina_pref_dialog->dialog), "tag", "remmina-pref-dialog");
	remmina_widget_pool_register(GTK_WIDGET(remmina_pref_dialog->dialog));
}

/* RemminaPrefDialog instance */
GtkDialog* remmina_pref_dialog_new(gint default_tab, GtkWindow *parent)
{
	TRACE_CALL("remmina_pref_dialog_new");
	remmina_pref_dialog = g_new0(RemminaPrefDialog, 1);
	remmina_pref_dialog->priv = g_new0(RemminaPrefDialogPriv, 1);

	remmina_pref_dialog->builder = remmina_public_gtk_builder_new_from_file("remmina_preferences.glade");
	remmina_pref_dialog->dialog = GTK_DIALOG(gtk_builder_get_object(remmina_pref_dialog->builder, "RemminaPrefDialog"));
	if (parent)
		gtk_window_set_transient_for(GTK_WINDOW(remmina_pref_dialog->dialog), parent);

	remmina_pref_dialog->notebook_preferences = GTK_NOTEBOOK(GET_OBJECT("notebook_preferences"));

	remmina_pref_dialog->checkbutton_options_remember_last_view_mode = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_options_remember_last_view_mode"));
	remmina_pref_dialog->checkbutton_options_save_settings = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_options_save_settings"));
	remmina_pref_dialog->checkbutton_appearance_invisible_toolbar = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_appearance_invisible_toolbar"));
	remmina_pref_dialog->checkbutton_appearance_show_tabs = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_appearance_show_tabs"));
	remmina_pref_dialog->checkbutton_appearance_hide_toolbar = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_appearance_hide_toolbar"));
	remmina_pref_dialog->comboboxtext_options_double_click = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_options_double_click"));
	remmina_pref_dialog->comboboxtext_appearance_view_mode = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_appearance_view_mode"));
	remmina_pref_dialog->comboboxtext_appearance_tab_interface = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_appearance_tab_interface"));
	remmina_pref_dialog->comboboxtext_appearance_show_buttons_icons = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_appearance_show_buttons_icons"));
	remmina_pref_dialog->comboboxtext_appearance_show_menu_icons = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_appearance_show_menu_icons"));
	remmina_pref_dialog->comboboxtext_options_scale_quality = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_options_scale_quality"));
	remmina_pref_dialog->checkbutton_options_ssh_parseconfig = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_options_ssh_parseconfig"));
	remmina_pref_dialog->comboboxtext_options_ssh_loglevel = GTK_COMBO_BOX(GET_OBJECT("comboboxtext_options_ssh_loglevel"));
	remmina_pref_dialog->entry_options_ssh_port = GTK_ENTRY(GET_OBJECT("entry_options_ssh_port"));
	remmina_pref_dialog->entry_options_scroll = GTK_ENTRY(GET_OBJECT("entry_options_scroll"));
	remmina_pref_dialog->entry_options_recent_items = GTK_ENTRY(GET_OBJECT("entry_options_recent_items"));
	remmina_pref_dialog->button_options_recent_items_clear = GTK_BUTTON(GET_OBJECT("button_options_recent_items_clear"));

	remmina_pref_dialog->checkbutton_applet_new_connection_on_top = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_new_connection_on_top"));
	remmina_pref_dialog->checkbutton_applet_hide_totals = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_hide_totals"));
	remmina_pref_dialog->checkbutton_applet_disable_tray = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_disable_tray"));
	remmina_pref_dialog->checkbutton_applet_start_in_tray = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_applet_start_in_tray"));

	remmina_pref_dialog->button_keyboard_host_key = GTK_BUTTON(GET_OBJECT("button_keyboard_host_key"));
	remmina_pref_dialog->button_keyboard_fullscreen = GTK_BUTTON(GET_OBJECT("button_keyboard_fullscreen"));
	remmina_pref_dialog->button_keyboard_auto_fit = GTK_BUTTON(GET_OBJECT("button_keyboard_auto_fit"));
	remmina_pref_dialog->button_keyboard_switch_tab_left = GTK_BUTTON(GET_OBJECT("button_keyboard_switch_tab_left"));
	remmina_pref_dialog->button_keyboard_switch_tab_right = GTK_BUTTON(GET_OBJECT("button_keyboard_switch_tabright"));
	remmina_pref_dialog->button_keyboard_scaled = GTK_BUTTON(GET_OBJECT("button_keyboard_scaled"));
	remmina_pref_dialog->button_keyboard_grab_keyboard = GTK_BUTTON(GET_OBJECT("button_keyboard_grab_keyboard"));
	remmina_pref_dialog->button_keyboard_minimize = GTK_BUTTON(GET_OBJECT("button_keyboard_minimize"));
	remmina_pref_dialog->button_keyboard_disconnect = GTK_BUTTON(GET_OBJECT("button_keyboard_disconnect"));
	remmina_pref_dialog->button_keyboard_toolbar = GTK_BUTTON(GET_OBJECT("button_keyboard_toolbar"));

	remmina_pref_dialog->checkbutton_terminal_font_system = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_terminal_font_system"));
	remmina_pref_dialog->fontbutton_terminal_font = GTK_FONT_BUTTON(GET_OBJECT("fontbutton_terminal_font"));
	remmina_pref_dialog->checkbutton_terminal_bold = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_terminal_bold"));
	remmina_pref_dialog->checkbutton_terminal_system_colors = GTK_CHECK_BUTTON(GET_OBJECT("checkbutton_terminal_system_colors"));
	remmina_pref_dialog->label_terminal_foreground = GTK_LABEL(GET_OBJECT("label_terminal_foreground"));
	remmina_pref_dialog->colorbutton_foreground = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_foreground"));
	remmina_pref_dialog->label_terminal_background = GTK_LABEL(GET_OBJECT("label_terminal_background"));
	remmina_pref_dialog->colorbutton_background = GTK_COLOR_BUTTON(GET_OBJECT("colorbutton_background"));
	remmina_pref_dialog->entry_scrollback_lines = GTK_ENTRY(GET_OBJECT("entry_scrollback_lines"));
	remmina_pref_dialog->button_keyboard_copy = GTK_BUTTON(GET_OBJECT("button_keyboard_copy"));
	remmina_pref_dialog->button_keyboard_paste = GTK_BUTTON(GET_OBJECT("button_keyboard_paste"));

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


