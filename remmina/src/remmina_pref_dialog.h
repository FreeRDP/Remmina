/*  See LICENSE and COPYING files for copyright and license details. */

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
/* Get the current PrefDialog or NULL if not initialized */
GtkDialog* remmina_pref_dialog_get_dialog(void);

G_END_DECLS

#endif  /* __REMMINAPREFDIALOG_H__  */

