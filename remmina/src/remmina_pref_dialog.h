/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

#ifndef __REMMINAPREFDIALOG_H__
#define __REMMINAPREFDIALOG_H__

/*
 * Remmina Preferences Dialog
 */

typedef struct _RemminaPrefDialogPriv
{
	GtkWidget *resolutions_list;
} RemminaPrefDialogPriv;

typedef struct _RemminaPrefDialog
{
	GtkBuilder *builder;
	GtkDialog *dialog;
	GtkNotebook *notebook_preferences;
	
	GtkCheckButton *checkbutton_options_remember_last_view_mode;
	GtkCheckButton *checkbutton_options_save_settings;
	GtkCheckButton *checkbutton_appearance_invisible_toolbar;
	GtkCheckButton *checkbutton_appearance_show_tabs;
	GtkCheckButton *checkbutton_appearance_hide_toolbar;
	GtkComboBox *comboboxtext_options_double_click;
	GtkComboBox *comboboxtext_appearance_view_mode;
	GtkComboBox *comboboxtext_appearance_tab_interface;
	GtkComboBox *comboboxtext_appearance_show_buttons_icons;
	GtkComboBox *comboboxtext_appearance_show_menu_icons;
	GtkComboBox *comboboxtext_options_scale_quality;
	GtkEntry *entry_options_ssh_port;
	GtkEntry *entry_options_scroll;
	GtkEntry *entry_options_recent_items;
	GtkButton *button_options_recent_items_clear;
	GtkButton *button_options_resolutions;
	
	GtkCheckButton *checkbutton_applet_new_connection_on_top;
	GtkCheckButton *checkbutton_applet_hide_totals;
	GtkCheckButton *checkbutton_applet_disable_tray;
	GtkCheckButton *checkbutton_applet_start_in_tray;
	
	GtkButton *button_keyboard_host_key;
	GtkButton *button_keyboard_fullscreen;
	GtkButton *button_keyboard_auto_fit;
	GtkButton *button_keyboard_switch_tab_left;
	GtkButton *button_keyboard_switch_tab_right;
	GtkButton *button_keyboard_scaled;
	GtkButton *button_keyboard_grab_keyboard;
	GtkButton *button_keyboard_minimize;
	GtkButton *button_keyboard_disconnect;
	GtkButton *button_keyboard_toolbar;
	
	GtkCheckButton *checkbutton_terminal_font_system;
	GtkFontButton *fontbutton_terminal_font;
	GtkCheckButton *checkbutton_terminal_bold;
	GtkCheckButton *checkbutton_terminal_system_colors;
	GtkLabel *label_terminal_foreground;
	GtkColorButton *colorbutton_foreground;
	GtkLabel *label_terminal_background;
	GtkColorButton *colorbutton_background;
	GtkEntry *entry_scrollback_lines;
	GtkButton *button_keyboard_copy;
	GtkButton *button_keyboard_paste;
	
	RemminaPrefDialogPriv *priv;
} RemminaPrefDialog;

enum
{
	REMMINA_PREF_OPTIONS_TAB = 0,
	REMMINA_PREF_APPEARANCE = 1,
	REMMINA_PREF_APPLET_TAB = 2
};

G_BEGIN_DECLS

/* RemminaPrefDialog instance */
GtkDialog* remmina_pref_dialog_new(gint default_tab, GtkWindow *parent);

G_END_DECLS

#endif  /* __REMMINAPREFDIALOG_H__  */

