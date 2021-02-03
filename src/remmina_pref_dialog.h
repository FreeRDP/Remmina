/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2017-2021 Antenore Gatta, Giovanni Panozzo
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

#pragma once
#include <gtk/gtk.h>

/*
 * Remmina Preferences Dialog
 */

typedef struct _RemminaPrefDialogPriv {
	GtkWidget *resolutions_list;
} RemminaPrefDialogPriv;

typedef struct _RemminaPrefDialog {
	GtkBuilder *		builder;
	GtkDialog *		dialog;
	GtkNotebook *		notebook_preferences;

	GtkFileChooser *	filechooserbutton_options_datadir_path;
	GtkEntry *		entry_options_file_name;
	GtkFileChooser *	filechooserbutton_options_screenshots_path;
	GtkEntry *		entry_options_screenshot_name;
	GtkSwitch *		switch_options_deny_screenshot_clipboard;
	GtkSwitch *		switch_options_remember_last_view_mode;
	GtkSwitch *		switch_security_use_master_password;
	GtkEntry *		unlock_timeout;
	GtkEntry *		unlock_password;
	GtkEntry *		unlock_repassword;
	GtkSwitch *		switch_security_trust_all;
	GtkCheckButton *	checkbutton_options_save_settings;
	GtkCheckButton *	checkbutton_appearance_fullscreen_on_auto;
	GtkCheckButton *	checkbutton_appearance_show_tabs;
	GtkCheckButton *	checkbutton_appearance_hide_toolbar;
	GtkCheckButton *	checkbutton_appearance_hide_searchbar;
	GtkSwitch *		switch_permit_send_stats;
	GtkSwitch *		switch_permit_news;
	GtkComboBox *		comboboxtext_options_double_click;
	GtkComboBox *		comboboxtext_appearance_view_mode;
	GtkComboBox *		comboboxtext_appearance_tab_interface;
	GtkComboBox *		comboboxtext_options_scale_quality;
	GtkComboBox *		comboboxtext_options_ssh_loglevel;
	GtkComboBox *		comboboxtext_appearance_fullscreen_toolbar_visibility;
	GtkCheckButton *	checkbutton_options_ssh_parseconfig;
	GtkEntry *		entry_options_ssh_port;
	GtkEntry *		entry_options_ssh_tcp_keepidle;
	GtkEntry *		entry_options_ssh_tcp_keepintvl;
	GtkEntry *		entry_options_ssh_tcp_keepcnt;
	GtkEntry *		entry_options_ssh_tcp_usrtimeout;
	GtkEntry *		entry_options_scroll;
	GtkEntry *		entry_options_recent_items;
	GtkButton *		button_options_recent_items_clear;
	GtkButton *		button_options_resolutions;

	GtkCheckButton *	checkbutton_applet_new_connection_on_top;
	GtkCheckButton *	checkbutton_applet_hide_totals;
	GtkCheckButton *	checkbutton_applet_disable_tray;
	GtkCheckButton *	checkbutton_applet_light_tray;
	GtkCheckButton *	checkbutton_applet_start_in_tray;

	GtkButton *		button_keyboard_host_key;
	GtkButton *		button_keyboard_fullscreen;
	GtkButton *		button_keyboard_auto_fit;
	GtkButton *		button_keyboard_switch_tab_left;
	GtkButton *		button_keyboard_switch_tab_right;
	GtkButton *		button_keyboard_scaled;
	GtkButton *		button_keyboard_multimon;
	GtkButton *		button_keyboard_grab_keyboard;
	GtkButton *		button_keyboard_screenshot;
	GtkButton *		button_keyboard_viewonly;
	GtkButton *		button_keyboard_minimize;
	GtkButton *		button_keyboard_disconnect;
	GtkButton *		button_keyboard_toolbar;

	GtkSwitch *		switch_terminal_font_system;
	GtkFontButton *		fontbutton_terminal_font;
	GtkSwitch *		switch_terminal_bold;
	GtkLabel *		label_terminal_foreground;
	GtkColorButton *	colorbutton_foreground;
	GtkLabel *		label_terminal_background;
	GtkColorButton *	colorbutton_background;
	GtkEntry *		entry_scrollback_lines;
	GtkButton *		button_keyboard_copy;
	GtkButton *		button_keyboard_paste;
	GtkButton *		button_keyboard_select_all;
	GtkButton *		button_keyboard_increase_font;
	GtkButton *		button_keyboard_decrease_font;
	GtkButton *		button_keyboard_search_text;
	GtkLabel *		label_terminal_cursor_color;
	GtkLabel *		label_terminal_bold_color;
	GtkLabel *		label_terminal_normal_colors;
	GtkLabel *		label_terminal_bright_colors;
	GtkColorButton *	colorbutton_cursor;
	GtkColorButton *	colorbutton_bold;
	GtkColorButton *	colorbutton_color0;
	GtkColorButton *	colorbutton_color1;
	GtkColorButton *	colorbutton_color2;
	GtkColorButton *	colorbutton_color3;
	GtkColorButton *	colorbutton_color4;
	GtkColorButton *	colorbutton_color5;
	GtkColorButton *	colorbutton_color6;
	GtkColorButton *	colorbutton_color7;
	GtkColorButton *	colorbutton_color8;
	GtkColorButton *	colorbutton_color9;
	GtkColorButton *	colorbutton_color10;
	GtkColorButton *	colorbutton_color11;
	GtkColorButton *	colorbutton_color12;
	GtkColorButton *	colorbutton_color13;
	GtkColorButton *	colorbutton_color14;
	GtkColorButton *	colorbutton_color15;
	GtkFileChooser *	button_term_cs;

	RemminaPrefDialogPriv * priv;
} RemminaPrefDialog;

enum {
	REMMINA_PREF_OPTIONS_TAB	= 0,
	REMMINA_PREF_APPEARANCE		= 1,
	REMMINA_PREF_APPLET_TAB		= 2
};

G_BEGIN_DECLS

/* RemminaPrefDialog instance */
GtkDialog *remmina_pref_dialog_new(gint default_tab, GtkWindow *parent);
/* Get the current PrefDialog or NULL if not initialized */
GtkDialog *remmina_pref_dialog_get_dialog(void);
void remmina_prefdiag_unlock_repwd_on_changed(GtkEditable *editable, RemminaPrefDialog *dialog);
void remmina_pref_dialog_on_action_close(GSimpleAction *action, GVariant *param, gpointer data);

G_END_DECLS
