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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
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

G_DEFINE_TYPE( RemminaPrefDialog, remmina_pref_dialog, GTK_TYPE_DIALOG)

static const gpointer default_action_list[] =
{ "0", N_("Open connection"), "1", N_("Edit settings"), NULL };

static const gpointer scale_quality_list[] =
{ "0", N_("Nearest"), "1", N_("Tiles"), "2", N_("Bilinear"), "3", N_("Hyper"), NULL };

static const gpointer default_mode_list[] =
{ "0", N_("Automatic"), "1", N_("Scrolled window"), "3", N_("Scrolled fullscreen"), "4", N_("Viewport fullscreen"), NULL };

static const gpointer tab_mode_list[] =
{ "0", N_("Tab by groups"), "1", N_("Tab by protocols"), "8", N_("Tab all connections"), "9", N_("Do not use tabs"), NULL };

struct _RemminaPrefDialogPriv
{
	GtkWidget *notebook;
	GtkWidget *save_view_mode_check;
	GtkWidget *save_when_connect_check;
	GtkWidget *invisible_toolbar_check;
	GtkWidget *always_show_tab_check;
	GtkWidget *hide_connection_toolbar_check;
	GtkWidget *default_action_combo;
	GtkWidget *default_mode_combo;
	GtkWidget *tab_mode_combo;
	GtkWidget *scale_quality_combo;
	GtkWidget *sshtunnel_port_entry;
	GtkWidget *auto_scroll_step_entry;
	GtkWidget *recent_maximum_entry;
	GtkWidget *resolutions_list;
	GtkWidget *applet_new_ontop_check;
	GtkWidget *applet_hide_count_check;
	GtkWidget *disable_tray_icon_check;
	GtkWidget *autostart_tray_icon_check;
	GtkWidget *minimize_to_tray_check;
	GtkWidget *hostkey_chooser;
	GtkWidget *shortcutkey_fullscreen_chooser;
	GtkWidget *shortcutkey_autofit_chooser;
	GtkWidget *shortcutkey_nexttab_chooser;
	GtkWidget *shortcutkey_prevtab_chooser;
	GtkWidget *shortcutkey_scale_chooser;
	GtkWidget *shortcutkey_grab_chooser;
	GtkWidget *shortcutkey_minimize_chooser;
	GtkWidget *shortcutkey_disconnect_chooser;
	GtkWidget *shortcutkey_toolbar_chooser;
	GtkWidget *vte_font_check;
	GtkWidget *vte_font_button;
	GtkWidget *vte_allow_bold_text_check;
	GtkWidget *vte_lines_entry;
	GtkWidget *vte_shortcutkey_copy_chooser;
	GtkWidget *vte_shortcutkey_paste_chooser;
};

static void remmina_pref_dialog_class_init(RemminaPrefDialogClass *klass)
{
}

static gboolean remmina_pref_resolution_validation_func(const gchar *new_str, gchar **error)
{
	gint i;
	gint width, height;
	gboolean splitted;
	gboolean result;

	width = 0;
	height = 0;
	splitted = FALSE;
	result = TRUE;
	for (i = 0; new_str[i] != '\0'; i++)
	{
		if (new_str[i] == 'x')
		{
			if (splitted)
			{
				result = FALSE;
				break;
			}
			splitted = TRUE;
			continue;
		}
		if (new_str[i] < '0' || new_str[i] > '9')
		{
			result = FALSE;
			break;
		}
		if (splitted)
		{
			height = 1;
		}
		else
		{
			width = 1;
		}
	}

	if (width == 0 || height == 0)
		result = FALSE;

	if (!result)
		*error = g_strdup(_("Please enter format 'widthxheight'."));
	return result;
}

static void remmina_pref_dialog_clear_recent(GtkWidget *widget, gpointer data)
{
	GtkWidget *dialog;

	remmina_pref_clear_recent();
	dialog = gtk_message_dialog_new(GTK_WINDOW(data), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			_("Recent lists cleared."));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void remmina_pref_dialog_on_close_clicked(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void remmina_pref_dialog_destroy(GtkWidget *widget, gpointer data)
{
	gchar *s;
	gboolean b;

	RemminaPrefDialogPriv *priv = REMMINA_PREF_DIALOG(widget)->priv;

	remmina_pref.save_view_mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->save_view_mode_check));
	remmina_pref.save_when_connect = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->save_when_connect_check));
	remmina_pref.invisible_toolbar = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->invisible_toolbar_check));
	remmina_pref.always_show_tab = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->always_show_tab_check));
	remmina_pref.hide_connection_toolbar = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(priv->hide_connection_toolbar_check));

	s = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->default_action_combo));
	remmina_pref.default_action = atoi(s);
	g_free(s);
	s = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->default_mode_combo));
	remmina_pref.default_mode = atoi(s);
	g_free(s);
	s = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->tab_mode_combo));
	remmina_pref.tab_mode = atoi(s);
	g_free(s);
	s = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->scale_quality_combo));
	remmina_pref.scale_quality = atoi(s);
	g_free(s);

	remmina_pref.sshtunnel_port = atoi(gtk_entry_get_text(GTK_ENTRY(priv->sshtunnel_port_entry)));
	if (remmina_pref.sshtunnel_port <= 0)
		remmina_pref.sshtunnel_port = DEFAULT_SSHTUNNEL_PORT;

	remmina_pref.auto_scroll_step = atoi(gtk_entry_get_text(GTK_ENTRY(priv->auto_scroll_step_entry)));
	if (remmina_pref.auto_scroll_step < 10)
		remmina_pref.auto_scroll_step = 10;
	else
		if (remmina_pref.auto_scroll_step > 500)
			remmina_pref.auto_scroll_step = 500;

	remmina_pref.recent_maximum = atoi(gtk_entry_get_text(GTK_ENTRY(priv->recent_maximum_entry)));
	if (remmina_pref.recent_maximum < 0)
		remmina_pref.recent_maximum = 0;

	g_free(remmina_pref.resolutions);
	s = remmina_string_list_get_text(REMMINA_STRING_LIST(priv->resolutions_list));
	if (s[0] == '\0')
		s = g_strdup(default_resolutions);
	remmina_pref.resolutions = s;

	remmina_pref.applet_new_ontop = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->applet_new_ontop_check));
	remmina_pref.applet_hide_count = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->applet_hide_count_check));
	b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->disable_tray_icon_check));
	if (remmina_pref.disable_tray_icon != b)
	{
		remmina_pref.disable_tray_icon = b;
		remmina_icon_init();
	}
	if (b)
	{
		remmina_pref.minimize_to_tray = FALSE;
	}
	else
	{
		remmina_pref.minimize_to_tray = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->minimize_to_tray_check));
	}
	if (b)
	{
		b = FALSE;
	}
	else
	{
		b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->autostart_tray_icon_check));
	}
	remmina_icon_set_autostart(b);

	remmina_pref.hostkey = REMMINA_KEY_CHOOSER(priv->hostkey_chooser)->keyval;
	remmina_pref.shortcutkey_fullscreen = REMMINA_KEY_CHOOSER(priv->shortcutkey_fullscreen_chooser)->keyval;
	remmina_pref.shortcutkey_autofit = REMMINA_KEY_CHOOSER(priv->shortcutkey_autofit_chooser)->keyval;
	remmina_pref.shortcutkey_nexttab = REMMINA_KEY_CHOOSER(priv->shortcutkey_nexttab_chooser)->keyval;
	remmina_pref.shortcutkey_prevtab = REMMINA_KEY_CHOOSER(priv->shortcutkey_prevtab_chooser)->keyval;
	remmina_pref.shortcutkey_scale = REMMINA_KEY_CHOOSER(priv->shortcutkey_scale_chooser)->keyval;
	remmina_pref.shortcutkey_grab = REMMINA_KEY_CHOOSER(priv->shortcutkey_grab_chooser)->keyval;
	remmina_pref.shortcutkey_minimize = REMMINA_KEY_CHOOSER(priv->shortcutkey_minimize_chooser)->keyval;
	remmina_pref.shortcutkey_disconnect = REMMINA_KEY_CHOOSER(priv->shortcutkey_disconnect_chooser)->keyval;
	remmina_pref.shortcutkey_toolbar = REMMINA_KEY_CHOOSER(priv->shortcutkey_toolbar_chooser)->keyval;

	g_free(remmina_pref.vte_font);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->vte_font_check)))
	{
		remmina_pref.vte_font = NULL;
	}
	else
	{
		remmina_pref.vte_font = g_strdup(gtk_font_button_get_font_name(GTK_FONT_BUTTON(priv->vte_font_button)));
	}
	remmina_pref.vte_allow_bold_text = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->vte_allow_bold_text_check));
	remmina_pref.vte_lines = atoi(gtk_entry_get_text(GTK_ENTRY(priv->vte_lines_entry)));
	remmina_pref.vte_shortcutkey_copy = REMMINA_KEY_CHOOSER(priv->vte_shortcutkey_copy_chooser)->keyval;
	remmina_pref.vte_shortcutkey_paste = REMMINA_KEY_CHOOSER(priv->vte_shortcutkey_paste_chooser)->keyval;

	remmina_pref_save();
	g_free(priv);
}

static gboolean remmina_pref_dialog_add_pref_plugin(gchar *name, RemminaPlugin *plugin, gpointer data)
{
	RemminaPrefDialogPriv *priv;
	RemminaPrefPlugin *pref_plugin;
	GtkWidget *vbox;
	GtkWidget *widget;

	priv = REMMINA_PREF_DIALOG(data)->priv;
	pref_plugin = (RemminaPrefPlugin *) plugin;

	widget = gtk_label_new(pref_plugin->pref_label);
	gtk_widget_show(widget);

#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(priv->notebook), vbox, widget);

	widget = pref_plugin->get_pref_body();
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

	return FALSE;
}

static void remmina_pref_dialog_vte_font_on_toggled(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	gtk_widget_set_sensitive(dialog->priv->vte_font_button, !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void remmina_pref_dialog_disable_tray_icon_on_toggled(GtkWidget *widget, RemminaPrefDialog *dialog)
{
	gboolean b;

	b = !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	gtk_widget_set_sensitive(dialog->priv->autostart_tray_icon_check, b);
	gtk_widget_set_sensitive(dialog->priv->minimize_to_tray_check, b);
}

static void remmina_pref_dialog_init(RemminaPrefDialog *dialog)
{
	RemminaPrefDialogPriv *priv;
	GtkWidget *notebook;
	GtkWidget *tablabel;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *table;
	GtkWidget *widget;
	gchar buf[100];

	priv = g_new(RemminaPrefDialogPriv, 1);
	dialog->priv = priv;

	/* Create the dialog */
	gtk_window_set_title(GTK_WINDOW(dialog), _("Remmina Preferences"));
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
	widget = gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_pref_dialog_on_close_clicked), dialog);
	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_CLOSE);

	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(remmina_pref_dialog_destroy), NULL);

	/* Create the notebook */
	notebook = gtk_notebook_new();
	gtk_widget_show(notebook);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), notebook, TRUE, TRUE, 4);
	gtk_container_set_border_width(GTK_CONTAINER(notebook), 4);
	priv->notebook = notebook;

	/* Options tab */
	tablabel = gtk_label_new(_("Options"));
	gtk_widget_show(tablabel);

	/* Options body */
#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, tablabel);

	table = gtk_table_new(12, 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);
	gtk_container_set_border_width(GTK_CONTAINER(table), 8);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	widget = gtk_check_button_new_with_label(_("Remember last view mode for each connection"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 0, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.save_view_mode);
	priv->save_view_mode_check = widget;

	widget = gtk_check_button_new_with_label(_("Save settings when starting the connection"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 1, 2);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.save_when_connect);
	priv->save_when_connect_check = widget;

	widget = gtk_check_button_new_with_label(_("Invisible toolbar in fullscreen mode"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 2, 3);
	if (gtk_widget_is_composited(GTK_WIDGET(dialog)))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.invisible_toolbar);
	}
	else
	{
		gtk_widget_set_sensitive(widget, FALSE);
	}
	priv->invisible_toolbar_check = widget;

	widget = gtk_check_button_new_with_label(_("Always show tabs"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 3, 4);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.always_show_tab);
	priv->always_show_tab_check = widget;

	widget = gtk_check_button_new_with_label(_("Hide toolbar in tabbed interface"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 4, 5);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.hide_connection_toolbar);
	priv->hide_connection_toolbar_check = widget;

	widget = gtk_label_new(_("Double-click action"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 5, 6);

	widget = remmina_public_create_combo_mapint(default_action_list, remmina_pref.default_action, FALSE, NULL);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 5, 6);
	priv->default_action_combo = widget;

	widget = gtk_label_new(_("Default view mode"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 6, 7);

	widget = remmina_public_create_combo_mapint(default_mode_list, remmina_pref.default_mode, FALSE, NULL);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 6, 7);
	priv->default_mode_combo = widget;

	widget = gtk_label_new(_("Tab interface"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 7, 8);

	widget = remmina_public_create_combo_mapint(tab_mode_list, remmina_pref.tab_mode, FALSE, NULL);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 7, 8);
	priv->tab_mode_combo = widget;

	widget = gtk_label_new(_("Scale quality"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 8, 9);

	widget = remmina_public_create_combo_mapint(scale_quality_list, remmina_pref.scale_quality, FALSE, NULL);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 8, 9);
	priv->scale_quality_combo = widget;

	widget = gtk_label_new(_("SSH tunnel local port"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 9, 10);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 9, 10);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 5);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.sshtunnel_port);
	gtk_entry_set_text(GTK_ENTRY(widget), buf);
	priv->sshtunnel_port_entry = widget;

	widget = gtk_label_new(_("Auto scroll step size"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 10, 11);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 10, 11);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 3);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.auto_scroll_step);
	gtk_entry_set_text(GTK_ENTRY(widget), buf);
	priv->auto_scroll_step_entry = widget;

	widget = gtk_label_new(_("Maximum recent items"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 11, 12);

#if GTK_VERSION == 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#elif GTK_VERSION == 2
	hbox = gtk_hbox_new(FALSE, 2);
#endif
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 11, 12);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 2);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.recent_maximum);
	gtk_entry_set_text(GTK_ENTRY(widget), buf);
	priv->recent_maximum_entry = widget;

	widget = gtk_button_new_with_label(_("Clear"));
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_pref_dialog_clear_recent), dialog);

	/* Resolutions tab */
	tablabel = gtk_label_new(_("Resolutions"));
	gtk_widget_show(tablabel);

	/* Resolutions body */
#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 2);
#endif
	gtk_widget_show(vbox);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 8);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, tablabel);

	widget = remmina_string_list_new();
	gtk_widget_show(widget);
	gtk_container_set_border_width(GTK_CONTAINER(widget), 8);
	gtk_widget_set_size_request(widget, 350, -1);
	remmina_string_list_set_validation_func(REMMINA_STRING_LIST(widget), remmina_pref_resolution_validation_func);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 0);
	remmina_string_list_set_text(REMMINA_STRING_LIST(widget), remmina_pref.resolutions);
	priv->resolutions_list = widget;

	/* Applet tab */
	tablabel = gtk_label_new(_("Applet"));
	gtk_widget_show(tablabel);

	/* Applet body */
#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, tablabel);

	table = gtk_table_new(5, 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);
	gtk_container_set_border_width(GTK_CONTAINER(table), 8);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	widget = gtk_check_button_new_with_label(_("Show new connection on top of the menu"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 0, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.applet_new_ontop);
	priv->applet_new_ontop_check = widget;

	widget = gtk_check_button_new_with_label(_("Hide total count in group menu"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 1, 2);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.applet_hide_count);
	priv->applet_hide_count_check = widget;

	widget = gtk_check_button_new_with_label(_("Disable tray icon"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 2, 3);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.disable_tray_icon);
	priv->disable_tray_icon_check = widget;

	widget = gtk_check_button_new_with_label(_("Start tray icon automatically"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 3, 4);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_icon_is_autostart());
	gtk_widget_set_sensitive(widget, !remmina_pref.disable_tray_icon);
	priv->autostart_tray_icon_check = widget;

	widget = gtk_check_button_new_with_label(_("Minimize windows to tray"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 2, 4, 5);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.minimize_to_tray);
	gtk_widget_set_sensitive(widget, !remmina_pref.disable_tray_icon);
	priv->minimize_to_tray_check = widget;

	g_signal_connect(G_OBJECT(priv->disable_tray_icon_check), "toggled",
			G_CALLBACK(remmina_pref_dialog_disable_tray_icon_on_toggled), dialog);

	/* Keyboard tab */
	tablabel = gtk_label_new(_("Keyboard"));
	gtk_widget_show(tablabel);

	/* Keyboard body */
#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, tablabel);

	table = gtk_table_new(9, 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);
	gtk_container_set_border_width(GTK_CONTAINER(table), 8);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	widget = gtk_label_new(_("Host key"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	widget = remmina_key_chooser_new(remmina_pref.hostkey);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 0, 1);
	priv->hostkey_chooser = widget;

	widget = gtk_label_new(_("Toggle fullscreen mode"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_fullscreen);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 1, 2);
	priv->shortcutkey_fullscreen_chooser = widget;

	widget = gtk_label_new(_("Auto-fit window"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_autofit);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 2, 3);
	priv->shortcutkey_autofit_chooser = widget;

	widget = gtk_label_new(_("Switch tab pages"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);

#if GTK_VERSION == 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#elif GTK_VERSION == 2
	hbox = gtk_hbox_new(TRUE, 2);
#endif
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 3, 4);

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_prevtab);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	priv->shortcutkey_prevtab_chooser = widget;

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_nexttab);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	priv->shortcutkey_nexttab_chooser = widget;

	widget = gtk_label_new(_("Toggle scaled mode"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 4, 5, GTK_FILL, 0, 0, 0);

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_scale);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 4, 5);
	priv->shortcutkey_scale_chooser = widget;

	widget = gtk_label_new(_("Grab keyboard"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 5, 6, GTK_FILL, 0, 0, 0);

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_grab);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 5, 6);
	priv->shortcutkey_grab_chooser = widget;

	widget = gtk_label_new(_("Minimize window"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 6, 7, GTK_FILL, 0, 0, 0);

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_minimize);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 6, 7);
	priv->shortcutkey_minimize_chooser = widget;

	widget = gtk_label_new(_("Disconnect"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 7, 8, GTK_FILL, 0, 0, 0);

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_disconnect);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 7, 8);
	priv->shortcutkey_disconnect_chooser = widget;

	widget = gtk_label_new(_("Show / hide toolbar"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 8, 9, GTK_FILL, 0, 0, 0);

	widget = remmina_key_chooser_new(remmina_pref.shortcutkey_toolbar);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 8, 9);
	priv->shortcutkey_toolbar_chooser = widget;

	/* Terminal tab */
	tablabel = gtk_label_new(_("Terminal"));
	gtk_widget_show(tablabel);

	/* Terminal body */
#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_widget_show(vbox);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, tablabel);

	table = gtk_table_new(6, 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);
	gtk_container_set_border_width(GTK_CONTAINER(table), 8);
	gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);

	widget = gtk_label_new(_("Font"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	widget = gtk_check_button_new_with_label(_("Use system default font"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 0, 1);
	priv->vte_font_check = widget;
	if (!(remmina_pref.vte_font && remmina_pref.vte_font[0]))
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
	}

	widget = gtk_font_button_new();
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 1, 2);
	priv->vte_font_button = widget;
	if (remmina_pref.vte_font && remmina_pref.vte_font[0])
	{
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(widget), remmina_pref.vte_font);
	}
	else
	{
		gtk_font_button_set_font_name(GTK_FONT_BUTTON(widget), "Monospace 12");
		gtk_widget_set_sensitive(widget, FALSE);
	}
	g_signal_connect(G_OBJECT(priv->vte_font_check), "toggled", G_CALLBACK(remmina_pref_dialog_vte_font_on_toggled),
			dialog);

	widget = gtk_check_button_new_with_label(_("Allow bold text"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 2, 3);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_pref.vte_allow_bold_text);
	priv->vte_allow_bold_text_check = widget;

	widget = gtk_label_new(_("Scrollback lines"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 3, 4);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 5);
	g_snprintf(buf, sizeof(buf), "%i", remmina_pref.vte_lines);
	gtk_entry_set_text(GTK_ENTRY(widget), buf);
	priv->vte_lines_entry = widget;

	widget = gtk_label_new(_("Keyboard"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 4, 5, GTK_FILL, 0, 0, 0);

#if GTK_VERSION == 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#elif GTK_VERSION == 2
	hbox = gtk_hbox_new(FALSE, 2);
#endif
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 4, 5);

	widget = gtk_label_new(_("Copy"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_set_size_request(widget, 100, -1);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	g_snprintf(buf, sizeof(buf), "%s + ", _("Host key"));
	widget = gtk_label_new(buf);
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 1.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	widget = remmina_key_chooser_new(remmina_pref.vte_shortcutkey_copy);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	priv->vte_shortcutkey_copy_chooser = widget;

#if GTK_VERSION == 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 2);
#elif GTK_VERSION == 2
	hbox = gtk_hbox_new(FALSE, 2);
#endif
	gtk_widget_show(hbox);
	gtk_table_attach_defaults(GTK_TABLE(table), hbox, 1, 2, 5, 6);

	widget = gtk_label_new(_("Paste"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_set_size_request(widget, 100, -1);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	widget = gtk_label_new(buf);
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 1.0, 0.5);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, TRUE, 0);

	widget = remmina_key_chooser_new(remmina_pref.vte_shortcutkey_paste);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	priv->vte_shortcutkey_paste_chooser = widget;

	remmina_plugin_manager_for_each_plugin(REMMINA_PLUGIN_TYPE_PREF, remmina_pref_dialog_add_pref_plugin, dialog);

	remmina_widget_pool_register(GTK_WIDGET(dialog));
}

GtkWidget*
remmina_pref_dialog_new(gint default_tab)
{
	RemminaPrefDialog *dialog;

	dialog = REMMINA_PREF_DIALOG(g_object_new(REMMINA_TYPE_PREF_DIALOG, NULL));
	if (default_tab > 0)
		gtk_notebook_set_current_page(GTK_NOTEBOOK(dialog->priv->notebook), default_tab);

	return GTK_WIDGET(dialog);
}

