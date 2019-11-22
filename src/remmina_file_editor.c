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
#ifdef HAVE_LIBAVAHI_UI
#include <avahi-ui/avahi-ui.h>
#endif
#include "remmina_public.h"
#include "remmina_pref.h"
#include "rcw.h"
#include "remmina_string_list.h"
#include "remmina_pref_dialog.h"
#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_ssh.h"
#include "remmina_widget_pool.h"
#include "remmina_plugin_manager.h"
#include "remmina_icon.h"
#include "remmina_file_editor.h"
#include "remmina/remmina_trace_calls.h"

G_DEFINE_TYPE(RemminaFileEditor, remmina_file_editor, GTK_TYPE_DIALOG)

#ifdef HAVE_LIBSSH
static const gchar * charset_list = "ASCII,BIG5,"
				    "CP437,CP720,CP737,CP775,CP850,CP852,CP855,"
				    "CP857,CP858,CP862,CP866,CP874,CP1125,CP1250,"
				    "CP1251,CP1252,CP1253,CP1254,CP1255,CP1256,"
				    "CP1257,CP1258,"
				    "EUC-JP,EUC-KR,GBK,"
				    "ISO-8859-1,ISO-8859-2,ISO-8859-3,ISO-8859-4,"
				    "ISO-8859-5,ISO-8859-6,ISO-8859-7,ISO-8859-8,"
				    "ISO-8859-9,ISO-8859-10,ISO-8859-11,ISO-8859-12,"
				    "ISO-8859-13,ISO-8859-14,ISO-8859-15,ISO-8859-16,"
				    "KOI8-R,SJIS,UTF-8";
#endif

static const gchar * server_tips = N_("<tt><big>"
				      "Supported formats\n"
				      "* server\n"
				      "* server:port\n"
				      "* [server]:port"
				      "</big></tt>");

static const gchar *cmd_tips = N_("<tt><big>"
				  "* command in PATH args %h\n"
				  "* /path/to/foo -options %h %u\n"
				  "* %h is substituted with the server name\n"
				  "* %t is substituted with the SSH server name\n"
				  "* %u is substituted with the username\n"
				  "* %U is substituted with the SSH username\n"
				  "* %p is substituted with Remmina profile name\n"
				  "* %g is substituted with Remmina profile group name\n"
				  "Do not run in background if you want the command to be executed before connecting.\n"
				  "</big></tt>");

#ifdef HAVE_LIBSSH
static const gchar *server_tips2 = N_("<tt><big>"
				      "Supported formats\n"
				      "* :port\n"
				      "* server\n"
				      "* server:port\n"
				      "* [server]:port\n"
				      "* username@server:port (SSH protocol only)"
				      "</big></tt>");
#endif

struct _RemminaFileEditorPriv {
	RemminaFile *		remmina_file;
	RemminaProtocolPlugin * plugin;
	const gchar *		avahi_service_type;

	GtkWidget *		name_entry;
	GtkWidget *		group_combo;
	GtkWidget *		protocol_combo;
	GtkWidget *		precommand_entry;
	GtkWidget *		postcommand_entry;
	GtkWidget *		save_button;

	GtkWidget *		config_box;
	GtkWidget *		config_scrollable;
	GtkWidget *		config_viewport;
	GtkWidget *		config_container;

	GtkWidget *		server_combo;
	GtkWidget *		resolution_iws_radio;
	GtkWidget *		resolution_auto_radio;
	GtkWidget *		resolution_custom_radio;
	GtkWidget *		resolution_custom_combo;
	GtkWidget *		keymap_combo;

	GtkWidget *		ssh_enabled_check;
	GtkWidget *		ssh_loopback_check;
	GtkWidget *		ssh_server_default_radio;
	GtkWidget *		ssh_server_custom_radio;
	GtkWidget *		ssh_server_entry;
	GtkWidget *		ssh_auth_agent_radio;
	GtkWidget *		ssh_auth_password_radio;
	GtkWidget *		ssh_auth_password;
	GtkWidget *		ssh_auth_publickey_radio;
	GtkWidget *		ssh_auth_auto_publickey_radio;
	GtkWidget *		ssh_username_entry;
	GtkWidget *		ssh_privatekey_chooser;
	GtkWidget *		ssh_charset_combo;

	GHashTable *		setting_widgets;
};

static void remmina_file_editor_class_init(RemminaFileEditorClass *klass)
{
	TRACE_CALL(__func__);
}

#ifdef HAVE_LIBAVAHI_UI

static void remmina_file_editor_browse_avahi(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	GtkWidget *dialog;
	gchar *host;

	dialog = aui_service_dialog_new(_("Choose a Remote Desktop Server"),
					GTK_WINDOW(gfe),
					_("_Cancel"), GTK_RESPONSE_CANCEL,
					_("_OK"), GTK_RESPONSE_ACCEPT,
					NULL);

	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gfe));
	aui_service_dialog_set_resolve_service(AUI_SERVICE_DIALOG(dialog), TRUE);
	aui_service_dialog_set_resolve_host_name(AUI_SERVICE_DIALOG(dialog), TRUE);
	aui_service_dialog_set_browse_service_types(AUI_SERVICE_DIALOG(dialog),
						    gfe->priv->avahi_service_type, NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		host = g_strdup_printf("[%s]:%i",
				       aui_service_dialog_get_host_name(AUI_SERVICE_DIALOG(dialog)),
				       aui_service_dialog_get_port(AUI_SERVICE_DIALOG(dialog)));
	} else {
		host = NULL;
	}
	gtk_widget_destroy(dialog);

	if (host) {
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(gfe->priv->server_combo))), host);
		g_free(host);
	}
}
#endif

static void remmina_file_editor_on_realize(GtkWidget *widget, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaFileEditor *gfe;
	GtkWidget *defaultwidget;

	gfe = REMMINA_FILE_EDITOR(widget);

	defaultwidget = gfe->priv->server_combo;

	if (defaultwidget) {
		if (GTK_IS_EDITABLE(defaultwidget))
			gtk_editable_select_region(GTK_EDITABLE(defaultwidget), 0, -1);
		gtk_widget_grab_focus(defaultwidget);
	}
}

static void remmina_file_editor_destroy(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	remmina_file_free(REMMINA_FILE_EDITOR(widget)->priv->remmina_file);
	g_hash_table_destroy(REMMINA_FILE_EDITOR(widget)->priv->setting_widgets);
	g_free(REMMINA_FILE_EDITOR(widget)->priv);
}

static void remmina_file_editor_button_on_toggled(GtkToggleButton *togglebutton, GtkWidget *widget)
{
	TRACE_CALL(__func__);
	gtk_widget_set_sensitive(widget, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton)));
}

static void remmina_file_editor_create_notebook_container(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	/* Create the notebook */
	gfe->priv->config_container = gtk_notebook_new();
	gfe->priv->config_viewport = gtk_viewport_new(NULL, NULL);
	gfe->priv->config_scrollable = gtk_scrolled_window_new(NULL, NULL);
	gtk_container_set_border_width(GTK_CONTAINER(gfe->priv->config_scrollable), 2);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gfe->priv->config_scrollable),
				       GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_widget_show(gfe->priv->config_scrollable);

	gtk_container_add(GTK_CONTAINER(gfe->priv->config_viewport), gfe->priv->config_container);
	gtk_container_set_border_width(GTK_CONTAINER(gfe->priv->config_viewport), 2);
	gtk_widget_show(gfe->priv->config_viewport);
	gtk_container_add(GTK_CONTAINER(gfe->priv->config_scrollable), gfe->priv->config_viewport);
	gtk_container_set_border_width(GTK_CONTAINER(gfe->priv->config_container), 2);
	gtk_widget_show(gfe->priv->config_container);

	gtk_container_add(GTK_CONTAINER(gfe->priv->config_box), gfe->priv->config_scrollable);
}

static GtkWidget *remmina_file_editor_create_notebook_tab(RemminaFileEditor *gfe,
							  const gchar *stock_id, const gchar *label, gint rows, gint cols)
{
	TRACE_CALL(__func__);
	GtkWidget *tablabel;
	GtkWidget *tabbody;
	GtkWidget *grid;
	GtkWidget *widget;

	tablabel = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(tablabel);

	widget = gtk_image_new_from_icon_name(stock_id, GTK_ICON_SIZE_BUTTON);
	gtk_box_pack_start(GTK_BOX(tablabel), widget, FALSE, FALSE, 0);
	gtk_widget_show(widget);

	widget = gtk_label_new(label);
	gtk_box_pack_start(GTK_BOX(tablabel), widget, FALSE, FALSE, 0);
	gtk_widget_show(widget);

	tabbody = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(tabbody);
	gtk_notebook_append_page(GTK_NOTEBOOK(gfe->priv->config_container), tabbody, tablabel);

	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 15);
	gtk_box_pack_start(GTK_BOX(tabbody), grid, FALSE, FALSE, 0);

	return grid;
}

#ifdef HAVE_LIBSSH

static void remmina_file_editor_ssh_server_custom_radio_on_toggled(GtkToggleButton *togglebutton, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_server_entry),
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->ssh_enabled_check)) &&
				 (gfe->priv->ssh_server_custom_radio == NULL ||
				  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->ssh_server_custom_radio)))
				 );
}

static void remmina_file_editor_ssh_auth_publickey_radio_on_toggled(GtkToggleButton *togglebutton, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	gboolean b;
	const gchar *s;

	b = ((!gfe->priv->ssh_enabled_check ||
	      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->ssh_enabled_check))) &&
	     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->ssh_auth_publickey_radio)));
	gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_privatekey_chooser), b);

	if (b && (s = remmina_file_get_string(gfe->priv->remmina_file, "ssh_privatekey")))
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gfe->priv->ssh_privatekey_chooser), s);
}

static void remmina_file_editor_ssh_enabled_check_on_toggled(GtkToggleButton *togglebutton,
							     RemminaFileEditor *gfe, RemminaProtocolSSHSetting ssh_setting)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	gboolean enabled = TRUE;
	gchar *p;
	const gchar *cp;

	if (gfe->priv->ssh_enabled_check) {
		enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->ssh_enabled_check));
		if (gfe->priv->ssh_loopback_check)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_loopback_check), enabled);
		if (gfe->priv->ssh_server_default_radio)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_server_default_radio), enabled);
		if (gfe->priv->ssh_server_custom_radio)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_server_custom_radio), enabled);
		remmina_file_editor_ssh_server_custom_radio_on_toggled(NULL, gfe);
		p = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->protocol_combo));
		if (!(g_strcmp0(p, "SFTP") == 0 || g_strcmp0(p, "SSH") == 0)) {
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_charset_combo), enabled);
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_username_entry), enabled);
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_auth_agent_radio), enabled);
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_auth_password_radio), enabled);
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_auth_password), enabled);
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_auth_publickey_radio), enabled);
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_auth_auto_publickey_radio), enabled);
		}
		g_free(p);
	}
	remmina_file_editor_ssh_auth_publickey_radio_on_toggled(NULL, gfe);

	if (gfe->priv->ssh_username_entry)
		if (enabled && gtk_entry_get_text(GTK_ENTRY(gfe->priv->ssh_username_entry)) [0] == '\0') {
			cp = remmina_file_get_string(priv->remmina_file, "ssh_username");
			gtk_entry_set_text(GTK_ENTRY(gfe->priv->ssh_username_entry), cp ? cp : "");
		}

	if (gfe->priv->ssh_auth_password) {
		if (enabled && gtk_entry_get_text(GTK_ENTRY(gfe->priv->ssh_auth_password)) [0] == '\0') {
			cp = remmina_file_get_string(priv->remmina_file, "ssh_password");
			gtk_entry_set_text(GTK_ENTRY(gfe->priv->ssh_auth_password), cp ? cp : "");
		}
	}
}

static void remmina_file_editor_create_ssh_privatekey(RemminaFileEditor *gfe, GtkWidget *grid, gint row, gint column)
{
	TRACE_CALL(__func__);
	gchar *s;
	GtkWidget *widget;
	GtkWidget *dialog;
	const gchar *ssh_privatekey;
	RemminaFileEditorPriv *priv = gfe->priv;

	widget = gtk_radio_button_new_with_label_from_widget(
		GTK_RADIO_BUTTON(priv->ssh_auth_password_radio), _("Identity file"));
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(remmina_file_editor_ssh_auth_publickey_radio_on_toggled), gfe);
	priv->ssh_auth_publickey_radio = widget;
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row + 22, 1, 1);

	dialog = gtk_file_chooser_dialog_new(_("Identity file"), GTK_WINDOW(gfe), GTK_FILE_CHOOSER_ACTION_OPEN,
					     _("_Cancel"), GTK_RESPONSE_CANCEL,
					     _("_Open"), GTK_RESPONSE_ACCEPT,
					     NULL);

	widget = gtk_file_chooser_button_new_with_dialog(dialog);
#ifdef SNAP_BUILD
	s = g_strdup_printf("%s/.ssh", g_getenv("SNAP_USER_COMMON"));
#else
	s = g_strdup_printf("%s/.ssh", g_get_home_dir());
#endif
	if (g_file_test(s, G_FILE_TEST_IS_DIR))
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widget), s);
	g_free(s);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, column + 1, row + 22, 1, 1);
	priv->ssh_privatekey_chooser = widget;

	ssh_privatekey = remmina_file_get_string(priv->remmina_file, "ssh_privatekey");
	if (ssh_privatekey &&
	    g_file_test(ssh_privatekey, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(priv->ssh_privatekey_chooser),
					      ssh_privatekey);
	else
		remmina_file_set_string(priv->remmina_file, "ssh_privatekey", NULL);
}
#endif

static void remmina_file_editor_create_server(RemminaFileEditor *gfe, const RemminaProtocolSetting *setting, GtkWidget *grid,
					      gint row)
{
	TRACE_CALL(__func__);
	RemminaProtocolPlugin *plugin = gfe->priv->plugin;
	GtkWidget *widget;
#ifdef HAVE_LIBAVAHI_UI
	GtkWidget *hbox;
#endif
	gchar *s;

	widget = gtk_label_new(_("Server"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, row + 1);

	s = remmina_pref_get_recent(plugin->name);
	widget = remmina_public_create_combo_entry(s, remmina_file_get_string(gfe->priv->remmina_file, "server"), TRUE);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_widget_show(widget);
	gtk_widget_set_tooltip_markup(widget, _(server_tips));
	gtk_entry_set_activates_default(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(widget))), TRUE);
	gfe->priv->server_combo = widget;
	g_free(s);

#ifdef HAVE_LIBAVAHI_UI
	if (setting->opt1) {
		gfe->priv->avahi_service_type = (const gchar *)setting->opt1;

		hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_show(hbox);
		gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

		widget = gtk_button_new_with_label("…");
		s = g_strdup_printf(_("Browse the network to find a %s server"), plugin->name);
		gtk_widget_set_tooltip_text(widget, s);
		g_free(s);
		gtk_widget_show(widget);
		gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
		g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_browse_avahi), gfe);

		gtk_grid_attach(GTK_GRID(grid), hbox, 1, row, 1, 1);
	} else
#endif
	{
		gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
	}
}

static GtkWidget *remmina_file_editor_create_password(RemminaFileEditor *gfe, GtkWidget *grid,
						      gint row, gint col, const gchar *label, const gchar *value)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
#if GTK_CHECK_VERSION(3, 12, 0)
	gtk_widget_set_margin_end(widget, 40);
#else
	gtk_widget_set_margin_right(widget, 40);
#endif
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 100);
	gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(widget), TRUE);

	if (value)
		gtk_entry_set_text(GTK_ENTRY(widget), value);
	return widget;
}

static void remmina_file_editor_update_resolution(GtkWidget *widget, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	gchar *res_str;
	res_str = g_strdup_printf("%dx%d",
				  remmina_file_get_int(gfe->priv->remmina_file, "resolution_width", 0),
				  remmina_file_get_int(gfe->priv->remmina_file, "resolution_height", 0)
				  );
	remmina_public_load_combo_text_d(gfe->priv->resolution_custom_combo, remmina_pref.resolutions,
					 res_str, NULL);
	g_free(res_str);
}

static void remmina_file_editor_browse_resolution(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);

	GtkDialog *dialog = remmina_string_list_new(FALSE, NULL);
	remmina_string_list_set_validation_func(remmina_public_resolution_validation_func);
	remmina_string_list_set_text(remmina_pref.resolutions, TRUE);
	remmina_string_list_set_titles(_("Resolutions"), _("Configure the available resolutions"));
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(gfe));
	gtk_dialog_run(dialog);
	g_free(remmina_pref.resolutions);
	remmina_pref.resolutions = remmina_string_list_get_text();
	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(remmina_file_editor_update_resolution), gfe);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

static void remmina_file_editor_create_resolution(RemminaFileEditor *gfe, const RemminaProtocolSetting *setting,
						  GtkWidget *grid, gint row)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	GtkWidget *hbox;
	int resolution_w, resolution_h;
	gchar *res_str;
	RemminaProtocolWidgetResolutionMode res_mode;

	res_mode = remmina_file_get_int(gfe->priv->remmina_file, "resolution_mode", RES_INVALID);
	resolution_w = remmina_file_get_int(gfe->priv->remmina_file, "resolution_width", -1);
	resolution_h = remmina_file_get_int(gfe->priv->remmina_file, "resolution_height", -1);

	/* If resolution_mode is non-existent (-1), then we try to calculate it
	 * as we did before having resolution_mode */
	if (res_mode == RES_INVALID) {
		if (resolution_w <= 0 || resolution_h <= 0)
			res_mode = RES_USE_INITIAL_WINDOW_SIZE;
		else
			res_mode = RES_USE_CUSTOM;
	}
	if (res_mode == RES_USE_CUSTOM)
		res_str = g_strdup_printf("%dx%d", resolution_w, resolution_h);
	else
		res_str = NULL;

	widget = gtk_label_new(_("Resolution"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	widget = gtk_radio_button_new_with_label(NULL, _("Use initial window size"));
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gfe->priv->resolution_iws_radio = widget;
	widget = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(gfe->priv->resolution_iws_radio), _("Use client resolution"));
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gfe->priv->resolution_auto_radio = widget;
	gtk_grid_attach(GTK_GRID(grid), hbox, 1, row, 1, 1);
	gtk_widget_show(hbox);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_grid_attach(GTK_GRID(grid), hbox, 1, row + 1, 1, 1);

	widget = gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(gfe->priv->resolution_iws_radio), _("Custom"));
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
	gfe->priv->resolution_custom_radio = widget;

	widget = remmina_public_create_combo_text_d(remmina_pref.resolutions, res_str, NULL);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);
	gfe->priv->resolution_custom_combo = widget;

	widget = gtk_button_new_with_label("…");
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_browse_resolution), gfe);

	g_signal_connect(G_OBJECT(gfe->priv->resolution_custom_radio), "toggled",
			 G_CALLBACK(remmina_file_editor_button_on_toggled), gfe->priv->resolution_custom_combo);

	if (res_mode == RES_USE_CUSTOM)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfe->priv->resolution_custom_radio), TRUE);
	else if (res_mode == RES_USE_CLIENT)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfe->priv->resolution_auto_radio), TRUE);
	else
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gfe->priv->resolution_iws_radio), TRUE);

	gtk_widget_set_sensitive(gfe->priv->resolution_custom_combo, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->resolution_custom_radio)));

	g_free(res_str);
}

static GtkWidget *remmina_file_editor_create_text(RemminaFileEditor *gfe, GtkWidget *grid,
						  gint row, gint col, const gchar *label, const gchar *value)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
#if GTK_CHECK_VERSION(3, 12, 0)
	gtk_widget_set_margin_end(widget, 40);
#else
	gtk_widget_set_margin_right(widget, 40);
#endif
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 300);
	gtk_widget_set_hexpand(widget, TRUE);

	if (value)
		gtk_entry_set_text(GTK_ENTRY(widget), value);

	return widget;
}

static GtkWidget *remmina_file_editor_create_select(RemminaFileEditor *gfe, GtkWidget *grid,
						    gint row, gint col, const gchar *label, const gpointer *list, const gchar *value)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

	widget = remmina_public_create_combo_map(list, value, FALSE, gfe->priv->plugin->domain);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);

	return widget;
}

static GtkWidget *remmina_file_editor_create_combo(RemminaFileEditor *gfe, GtkWidget *grid,
						   gint row, gint col, const gchar *label, const gchar *list, const gchar *value)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

	widget = remmina_public_create_combo_entry(list, value, FALSE);
	gtk_widget_show(widget);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);

	return widget;
}

static GtkWidget *remmina_file_editor_create_check(RemminaFileEditor *gfe, GtkWidget *grid,
						   gint row, gint top, const gchar *label, gboolean value)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	widget = gtk_check_button_new_with_label(label);
	gtk_widget_show(widget);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 1);
	gtk_grid_attach(GTK_GRID(grid), widget, top, row, 1, 1);

	if (value)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);

	return widget;
}

static GtkWidget *
remmina_file_editor_create_chooser(RemminaFileEditor *gfe, GtkWidget *grid, gint row, gint col, const gchar *label,
				   const gchar *value, gint type)
{
	TRACE_CALL(__func__);
	GtkWidget *check;
	GtkWidget *widget;
	GtkWidget *hbox;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_grid_attach(GTK_GRID(grid), hbox, 1, row, 1, 1);

	check = gtk_check_button_new();
	gtk_widget_show(check);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(check), (value && value[0] == '/'));
	gtk_box_pack_start(GTK_BOX(hbox), check, FALSE, FALSE, 0);

	widget = gtk_file_chooser_button_new(label, type);
	gtk_widget_show(widget);
	if (value)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widget), value);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(remmina_file_editor_button_on_toggled), widget);
	remmina_file_editor_button_on_toggled(GTK_TOGGLE_BUTTON(check), widget);

	return widget;
}

static void remmina_file_editor_create_settings(RemminaFileEditor *gfe, GtkWidget *grid,
						const RemminaProtocolSetting *settings)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	GtkWidget *widget;
	gint grid_row = 0;
	gint grid_column = 0;
	gchar **strarr;
	gchar *setting_name;


	while (settings->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
		setting_name = (gchar *)(remmina_plugin_manager_get_canonical_setting_name(settings));
		switch (settings->type) {
		case REMMINA_PROTOCOL_SETTING_TYPE_SERVER:
			remmina_file_editor_create_server(gfe, settings, grid, grid_row);
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD:
			widget = remmina_file_editor_create_password(gfe, grid, grid_row, 0,
								     g_dgettext(priv->plugin->domain, settings->label),
								     remmina_file_get_string(priv->remmina_file, setting_name));
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			grid_row++;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION:
			remmina_file_editor_create_resolution(gfe, settings, grid, grid_row);
			grid_row += 1;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_KEYMAP:
			strarr = remmina_pref_keymap_groups();
			priv->keymap_combo = remmina_file_editor_create_select(gfe, grid,
									       grid_row + 1, 0,
									       _("Keyboard mapping"), (const gpointer *)strarr,
									       remmina_file_get_string(priv->remmina_file, "keymap"));
			g_strfreev(strarr);
			grid_row++;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_TEXT:
			widget = remmina_file_editor_create_text(gfe, grid, grid_row, 0,
								 g_dgettext(priv->plugin->domain, settings->label),
								 remmina_file_get_string(priv->remmina_file, setting_name));
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text (widget, (const gchar *)settings->opt2);
			grid_row++;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_SELECT:
			widget = remmina_file_editor_create_select(gfe, grid, grid_row, 0,
								   g_dgettext(priv->plugin->domain, settings->label),
								   (const gpointer *)settings->opt1,
								   remmina_file_get_string(priv->remmina_file, setting_name));
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text (widget, (const gchar *)settings->opt2);
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_COMBO:
			widget = remmina_file_editor_create_combo(gfe, grid, grid_row, 0,
								  g_dgettext(priv->plugin->domain, settings->label),
								  (const gchar *)settings->opt1,
								  remmina_file_get_string(priv->remmina_file, setting_name));
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text (widget, (const gchar *)settings->opt2);
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_CHECK:
			widget = remmina_file_editor_create_check(gfe, grid, grid_row, grid_column,
								  g_dgettext(priv->plugin->domain, settings->label),
								  remmina_file_get_int(priv->remmina_file, setting_name, FALSE));
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text (widget, (const gchar *)settings->opt2);
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_FILE:
			widget = remmina_file_editor_create_chooser(gfe, grid, grid_row, 0,
								    g_dgettext(priv->plugin->domain, settings->label),
								    remmina_file_get_string(priv->remmina_file, setting_name),
								    GTK_FILE_CHOOSER_ACTION_OPEN);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text (widget, (const gchar *)settings->opt2);
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_FOLDER:
			widget = remmina_file_editor_create_chooser(gfe, grid, grid_row, 0,
								    g_dgettext(priv->plugin->domain, settings->label),
								    remmina_file_get_string(priv->remmina_file, setting_name),
								    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text (widget, (const gchar *)settings->opt2);
			break;

		default:
			break;
		}
		/* If the setting wants compactness, move to the next column */
		if (settings->compact)
			grid_column++;
		/* Add a new settings row and move to the first column
		 * if the setting doesn’t want the compactness
		 * or we already have two columns */
		if (!settings->compact || grid_column > 1) {
			grid_row++;
			grid_column = 0;
		}
		settings++;
	}
}

static void remmina_file_editor_create_ssh_tab(RemminaFileEditor *gfe, RemminaProtocolSSHSetting ssh_setting)
{
	TRACE_CALL(__func__);
#ifdef HAVE_LIBSSH
	RemminaFileEditorPriv *priv = gfe->priv;
	GtkWidget *grid;
	GtkWidget *widget;
	const gchar *cs;
	gchar *s;
	gchar *p;
	gint row = 0;

	if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_NONE) return;

	/* The SSH tab (implementation) */
	grid = remmina_file_editor_create_notebook_tab(gfe, NULL,
						       _("SSH Tunnel"), 9, 3);
	widget = gtk_toggle_button_new_with_label(_("Enable SSH tunnel"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 0, 1, 1);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(remmina_file_editor_ssh_enabled_check_on_toggled), gfe);
	priv->ssh_enabled_check = widget;

	widget = gtk_check_button_new_with_label(_("Tunnel via loopback address"));
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 0, 2, 1);
	priv->ssh_loopback_check = widget;

	row++;
	/* SSH Server group */

	switch (ssh_setting) {
	case REMMINA_PROTOCOL_SSH_SETTING_TUNNEL:
		s = g_strdup_printf(_("Same server at port %i"), DEFAULT_SSH_PORT);
		widget = gtk_radio_button_new_with_label(NULL, s);
		g_free(s);
		gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 3, 1);
		priv->ssh_server_default_radio = widget;
		row++;

		widget = gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(priv->ssh_server_default_radio), _("Custom"));
		gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
		g_signal_connect(G_OBJECT(widget), "toggled",
				 G_CALLBACK(remmina_file_editor_ssh_server_custom_radio_on_toggled), gfe);
		priv->ssh_server_custom_radio = widget;

		widget = gtk_entry_new();
		gtk_entry_set_max_length(GTK_ENTRY(widget), 100);
		gtk_widget_set_tooltip_markup(widget, _(server_tips2));
		gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 2, 1);
		priv->ssh_server_entry = widget;
		row++;
		break;

	case REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL:
		priv->ssh_server_default_radio = NULL;
		priv->ssh_server_custom_radio = NULL;

		priv->ssh_server_entry = remmina_file_editor_create_text(gfe, grid, 1, 0,
									 _("Server"), NULL);
		gtk_widget_set_tooltip_markup(priv->ssh_server_entry, _(server_tips));
		row++;
		break;
	case REMMINA_PROTOCOL_SSH_SETTING_SSH:
	case REMMINA_PROTOCOL_SSH_SETTING_SFTP:
		priv->ssh_server_default_radio = NULL;
		priv->ssh_server_custom_radio = NULL;
		priv->ssh_server_entry = NULL;

		break;

	default:
		break;
	}

	p = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->protocol_combo));
	if (!(g_strcmp0(p, "SFTP") == 0 || g_strcmp0(p, "SSH") == 0)) {
		priv->ssh_charset_combo = remmina_file_editor_create_combo(gfe, grid, row + 3, 0,
									   _("Character set"), charset_list, remmina_file_get_string(priv->remmina_file, "ssh_charset"));
		row++;
	}
	if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_SFTP) {
		widget = remmina_file_editor_create_text(gfe, grid, row + 8, 1,
							 _("Startup path"), NULL);
		cs = remmina_file_get_string(priv->remmina_file, "execpath");
		gtk_entry_set_text(GTK_ENTRY(widget), cs ? cs : "");
		g_hash_table_insert(priv->setting_widgets, "execpath", widget);
		row++;
	}

	/* SSH Authentication frame */
	if (!(g_strcmp0(p, "SFTP") == 0 || g_strcmp0(p, "SSH") == 0)) {
		remmina_public_create_group(GTK_GRID(grid), _("SSH Authentication"), row + 8, 6, 1);
		row++;

		if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_TUNNEL ||
		    ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL) {
			priv->ssh_username_entry =
				remmina_file_editor_create_text(gfe, grid, row + 10, 0,
								_("Username"), NULL);
			row++;
		}
		widget = gtk_radio_button_new_with_label(NULL, _("SSH Agent (automatic)"));
		gtk_grid_attach(GTK_GRID(grid), widget, 0, row + 19, 1, 1);
		priv->ssh_auth_agent_radio = widget;
		row++;

		widget = gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(priv->ssh_auth_agent_radio), _("Password"));
		gtk_grid_attach(GTK_GRID(grid), widget, 0, row + 21, 1, 1);
		priv->ssh_auth_password_radio = widget;

		widget = gtk_entry_new();
		gtk_grid_attach(GTK_GRID(grid), widget, 1, row + 21, 2, 1);
		gtk_entry_set_max_length(GTK_ENTRY(widget), 300);
		gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
		gtk_widget_set_hexpand(widget, TRUE);
		priv->ssh_auth_password = widget;
		row++;

		widget = gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(priv->ssh_auth_password_radio), _("Public key (automatic)"));
		gtk_grid_attach(GTK_GRID(grid), widget, 0, row + 22, 1, 1);
		priv->ssh_auth_auto_publickey_radio = widget;
		row++;

		remmina_file_editor_create_ssh_privatekey(gfe, grid, row + 1, 0);
	}
	row++;

	/* Set the values */
	cs = remmina_file_get_string(priv->remmina_file, "ssh_server");
	if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_TUNNEL) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->ssh_enabled_check),
					     remmina_file_get_int(priv->remmina_file, "ssh_enabled", FALSE));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->ssh_loopback_check),
					     remmina_file_get_int(priv->remmina_file, "ssh_loopback", FALSE));

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cs ?
							       priv->ssh_server_custom_radio : priv->ssh_server_default_radio), TRUE);
		gtk_entry_set_text(GTK_ENTRY(priv->ssh_server_entry),
				   cs ? cs : "");
	} else if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->ssh_enabled_check),
					     remmina_file_get_int(priv->remmina_file, "ssh_enabled", FALSE));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->ssh_loopback_check),
					     remmina_file_get_int(priv->remmina_file, "ssh_loopback", FALSE));
		gtk_entry_set_text(GTK_ENTRY(priv->ssh_server_entry),
				   cs ? cs : "");
	}

	if (!(g_strcmp0(p, "SFTP") == 0 || g_strcmp0(p, "SSH") == 0)) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						     remmina_file_get_int(priv->remmina_file, "ssh_auth", 0) == SSH_AUTH_PUBLICKEY ?
						     priv->ssh_auth_publickey_radio :
						     remmina_file_get_int(priv->remmina_file, "ssh_auth", 0) == SSH_AUTH_AUTO_PUBLICKEY ?
						     priv->ssh_auth_auto_publickey_radio :
						     remmina_file_get_int(priv->remmina_file, "ssh_auth", 0) == SSH_AUTH_AGENT ?
						     priv->ssh_auth_agent_radio :
						     priv->ssh_auth_password_radio), TRUE);

		remmina_file_editor_ssh_enabled_check_on_toggled(NULL, gfe, ssh_setting);
	}
	gtk_widget_show_all(grid);
	g_free(p);
#endif
}

static void remmina_file_editor_create_all_settings(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	GtkWidget *grid;

	static const RemminaProtocolSetting autostart_settings[] =
	{
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-autostart", N_("Turn on autostart for this profile"),	       FALSE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
	};

	remmina_file_editor_create_notebook_container(gfe);

	/* The Basic tab */
	if (priv->plugin->basic_settings) {
		grid = remmina_file_editor_create_notebook_tab(gfe, NULL, _("Basic"), 20, 2);
		remmina_file_editor_create_settings(gfe, grid, priv->plugin->basic_settings);
	}

	/* The Advanced tab */
	if (priv->plugin->advanced_settings) {
		grid = remmina_file_editor_create_notebook_tab(gfe, NULL, _("Advanced"), 20, 2);
		remmina_file_editor_create_settings(gfe, grid, priv->plugin->advanced_settings);
	}

	/* Autostart tab */
	grid = remmina_file_editor_create_notebook_tab(gfe, NULL, _("Autostart"), 20, 2);
	remmina_file_editor_create_settings(gfe, grid, autostart_settings);

	/* The SSH tab */
	remmina_file_editor_create_ssh_tab(gfe, priv->plugin->ssh_setting);

}

static void remmina_file_editor_protocol_combo_on_changed(GtkComboBox *combo, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	gchar *protocol;

	if (priv->config_container) {
		gtk_widget_destroy(priv->config_container);
		priv->config_container = NULL;
		gtk_widget_destroy(priv->config_viewport);
		priv->config_viewport = NULL;
		gtk_widget_destroy(priv->config_scrollable);
		priv->config_scrollable = NULL;
	}

	priv->server_combo = NULL;
	priv->resolution_iws_radio = NULL;
	priv->resolution_auto_radio = NULL;
	priv->resolution_custom_radio = NULL;
	priv->resolution_custom_combo = NULL;
	priv->keymap_combo = NULL;

	priv->ssh_enabled_check = NULL;
	priv->ssh_loopback_check = NULL;
	priv->ssh_server_default_radio = NULL;
	priv->ssh_server_custom_radio = NULL;
	priv->ssh_server_entry = NULL;
	priv->ssh_username_entry = NULL;
	priv->ssh_auth_agent_radio = NULL;
	priv->ssh_auth_password_radio = NULL;
	priv->ssh_auth_publickey_radio = NULL;
	priv->ssh_auth_auto_publickey_radio = NULL;
	priv->ssh_privatekey_chooser = NULL;
	priv->ssh_charset_combo = NULL;

	g_hash_table_remove_all(priv->setting_widgets);

	protocol = remmina_public_combo_get_active_text(combo);
	if (protocol) {
		priv->plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL,
											  protocol);
		g_free(protocol);
		remmina_file_editor_create_all_settings(gfe);
	}
}

static void remmina_file_editor_update_ssh(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	gboolean ssh_enabled;
	int ssh_auth;

	if (priv->ssh_charset_combo)
		remmina_file_set_string_ref(priv->remmina_file, "ssh_charset",
					    remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->ssh_charset_combo)));

	ssh_enabled = (priv->ssh_enabled_check ?
		       gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->ssh_enabled_check)) : FALSE);
	remmina_file_set_int(priv->remmina_file,
			     "ssh_loopback",
			     (priv->ssh_loopback_check ?
			      gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->ssh_loopback_check)) :
			      FALSE));
	remmina_file_set_int(priv->remmina_file, "ssh_enabled", ssh_enabled);
	remmina_file_set_string(priv->remmina_file, "ssh_username",
				(ssh_enabled ? gtk_entry_get_text(GTK_ENTRY(priv->ssh_username_entry)) : NULL));
	remmina_file_set_string(
		priv->remmina_file,
		"ssh_server",
		(ssh_enabled && priv->ssh_server_entry
		 && (priv->ssh_server_custom_radio == NULL
		     || gtk_toggle_button_get_active(
			     GTK_TOGGLE_BUTTON(priv->ssh_server_custom_radio))) ?
		 gtk_entry_get_text(GTK_ENTRY(priv->ssh_server_entry)) : NULL));

	ssh_auth = (priv->ssh_auth_publickey_radio
		    && gtk_toggle_button_get_active(
			    GTK_TOGGLE_BUTTON(priv->ssh_auth_publickey_radio)) ?
		    SSH_AUTH_PUBLICKEY :
		    priv->ssh_auth_auto_publickey_radio
		    && gtk_toggle_button_get_active(
			    GTK_TOGGLE_BUTTON(priv->ssh_auth_auto_publickey_radio)) ?
		    SSH_AUTH_AUTO_PUBLICKEY :
		    priv->ssh_auth_agent_radio
		    && gtk_toggle_button_get_active(
			    GTK_TOGGLE_BUTTON(priv->ssh_auth_agent_radio)) ?
		    SSH_AUTH_AGENT : SSH_AUTH_PASSWORD);

	remmina_file_set_int(
		priv->remmina_file,
		"ssh_auth",
		ssh_auth
		);
	remmina_file_set_string(
		priv->remmina_file,
		"ssh_privatekey",
		(priv->ssh_privatekey_chooser ?
		 gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(priv->ssh_privatekey_chooser)) : NULL));

	remmina_file_set_string(
		priv->remmina_file,
		"ssh_password",
		(ssh_enabled && (ssh_auth == SSH_AUTH_PASSWORD)) ? gtk_entry_get_text(GTK_ENTRY(priv->ssh_auth_password)) : NULL);
}

static void remmina_file_editor_update_settings(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init(&iter, priv->setting_widgets);
	while (g_hash_table_iter_next(&iter, &key, &value)) {
		if (GTK_IS_ENTRY(value)) {
			remmina_file_set_string(priv->remmina_file, (gchar *)key, gtk_entry_get_text(GTK_ENTRY(value)));
		} else if (GTK_IS_COMBO_BOX(value)) {
			remmina_file_set_string_ref(priv->remmina_file, (gchar *)key,
						    remmina_public_combo_get_active_text(GTK_COMBO_BOX(value)));
		} else if (GTK_IS_FILE_CHOOSER(value)) {
			remmina_file_set_string(
				priv->remmina_file,
				(gchar *)key,
				gtk_widget_get_sensitive(GTK_WIDGET(value)) ?
				gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(value)) :
				NULL);
		} else if (GTK_IS_TOGGLE_BUTTON(value)) {
			remmina_file_set_int(priv->remmina_file, (gchar *)key,
					     gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(value)));
		}
	}
}

static void remmina_file_editor_update(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	int res_w, res_h;
	gchar *custom_resolution;
	RemminaProtocolWidgetResolutionMode res_mode;

	RemminaFileEditorPriv *priv = gfe->priv;

	remmina_file_set_string(priv->remmina_file, "name", gtk_entry_get_text(GTK_ENTRY(priv->name_entry)));

	remmina_file_set_string_ref(priv->remmina_file, "group",
				    (priv->group_combo ? remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->group_combo)) : NULL));

	remmina_file_set_string_ref(priv->remmina_file, "protocol",
				    remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->protocol_combo)));

	remmina_file_set_string(priv->remmina_file, "precommand", gtk_entry_get_text(GTK_ENTRY(priv->precommand_entry)));
	remmina_file_set_string(priv->remmina_file, "postcommand", gtk_entry_get_text(GTK_ENTRY(priv->postcommand_entry)));

	remmina_file_set_string_ref(priv->remmina_file, "server",
				    (priv->server_combo ? remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->server_combo)) : NULL));

	if (priv->resolution_auto_radio) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->resolution_auto_radio))) {
			/* Resolution is set to auto (which means: use client fullscreen resolution, aka use client resolution) */
			res_w = res_h = 0;
			res_mode = RES_USE_CLIENT;
		} else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->resolution_iws_radio))) {
			/* Resolution is set to initial window size */
			res_w = res_h = 0;
			res_mode = RES_USE_INITIAL_WINDOW_SIZE;
		} else {
			/* Resolution is set to a value from the list */
			custom_resolution = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->resolution_custom_combo));
			if (remmina_public_split_resolution_string(custom_resolution, &res_w, &res_h))
				res_mode = RES_USE_CUSTOM;
			else
				res_mode = RES_USE_INITIAL_WINDOW_SIZE;
			g_free(custom_resolution);
		}
		remmina_file_set_int(priv->remmina_file, "resolution_mode", res_mode);
		remmina_file_set_int(priv->remmina_file, "resolution_width", res_w);
		remmina_file_set_int(priv->remmina_file, "resolution_height", res_h);
	}

	if (priv->keymap_combo)
		remmina_file_set_string_ref(priv->remmina_file, "keymap",
					    remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->keymap_combo)));

	remmina_file_editor_update_ssh(gfe);
	remmina_file_editor_update_settings(gfe);
}

static void remmina_file_editor_on_default(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFile *gf;
	GtkWidget *dialog;

	remmina_file_editor_update(gfe);

	gf = remmina_file_dup(gfe->priv->remmina_file);

	remmina_file_set_filename(gf, remmina_pref_file);

	/* Clear properties that should never be default */
	remmina_file_set_string(gf, "name", NULL);
	remmina_file_set_string(gf, "server", NULL);
	remmina_file_set_string(gf, "password", NULL);
	remmina_file_set_string(gf, "precommand", NULL);
	remmina_file_set_string(gf, "postcommand", NULL);

	remmina_file_save(gf);
	remmina_file_free(gf);

	dialog = gtk_message_dialog_new(GTK_WINDOW(gfe), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
					_("Default settings saved."));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void remmina_file_editor_on_save(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);

	remmina_file_editor_update(gfe);
	remmina_file_editor_file_save(gfe);

	remmina_file_save(gfe->priv->remmina_file);
	remmina_icon_populate_menu();

	gtk_widget_destroy(GTK_WIDGET(gfe));
}

static void remmina_file_editor_on_connect(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFile *gf;

	remmina_file_editor_update(gfe);

	gf = remmina_file_dup(gfe->priv->remmina_file);
	/* Put server into name for Quick Connect */
	if (remmina_file_get_filename(gf) == NULL)
		remmina_file_set_string(gf, "name", remmina_file_get_string(gf, "server"));
	gtk_widget_destroy(GTK_WIDGET(gfe));
	gf->prevent_saving = TRUE;
	rcw_open_from_file(gf);
}

static void remmina_file_editor_on_save_connect(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	/** @todo Call remmina_file_editor_on_save */
	RemminaFile *gf;

	remmina_file_editor_update(gfe);
	remmina_file_editor_file_save(gfe);

	remmina_file_save(gfe->priv->remmina_file);
	remmina_icon_populate_menu();

	gf = remmina_file_dup(gfe->priv->remmina_file);
	/* Put server into name for Quick Connect */
	if (remmina_file_get_filename(gf) == NULL)
		remmina_file_set_string(gf, "name", remmina_file_get_string(gf, "server"));
	gtk_widget_destroy(GTK_WIDGET(gfe));
	rcw_open_from_file(gf);
}

static void remmina_file_editor_on_cancel(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	gtk_widget_destroy(GTK_WIDGET(gfe));
}

static void remmina_file_editor_init(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv;
	GtkWidget *widget;

	priv = g_new0(RemminaFileEditorPriv, 1);
	gfe->priv = priv;

	/* Create the editor dialog */
	gtk_window_set_title(GTK_WINDOW(gfe), _("Remote Desktop Preference"));

	widget = gtk_dialog_add_button(GTK_DIALOG(gfe), (_("_Cancel")), GTK_RESPONSE_CANCEL);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_on_cancel), gfe);

	/* Default button */
	widget = gtk_dialog_add_button(GTK_DIALOG(gfe), (_("Save as Default")), GTK_RESPONSE_OK);
	gtk_widget_set_tooltip_text (GTK_WIDGET(widget), _("Use the current settings as the default for all new connection profiles"));
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_on_default), gfe);

	widget = gtk_dialog_add_button(GTK_DIALOG(gfe), (_("_Save")), GTK_RESPONSE_APPLY);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_on_save), gfe);
	gtk_widget_set_sensitive(widget, FALSE);
	priv->save_button = widget;

	widget = gtk_dialog_add_button(GTK_DIALOG(gfe), (_("Connect")), GTK_RESPONSE_ACCEPT);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_on_connect), gfe);

	widget = gtk_dialog_add_button(GTK_DIALOG(gfe), (_("_Save and Connect")), GTK_RESPONSE_OK);
	gtk_widget_set_can_default(widget, TRUE);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_on_save_connect), gfe);

	gtk_dialog_set_default_response(GTK_DIALOG(gfe), GTK_RESPONSE_OK);
	gtk_window_set_default_size(GTK_WINDOW(gfe), 800, 600);

	g_signal_connect(G_OBJECT(gfe), "destroy", G_CALLBACK(remmina_file_editor_destroy), NULL);
	g_signal_connect(G_OBJECT(gfe), "realize", G_CALLBACK(remmina_file_editor_on_realize), NULL);

	priv->setting_widgets = g_hash_table_new(g_str_hash, g_str_equal);

	remmina_widget_pool_register(GTK_WIDGET(gfe));
}

static gboolean remmina_file_editor_iterate_protocol(gchar *protocol, RemminaPlugin *plugin, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaFileEditor *gfe = REMMINA_FILE_EDITOR(data);
	GtkListStore *store;
	GtkTreeIter iter;
	gboolean first;

	store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(gfe->priv->protocol_combo)));

	first = !gtk_tree_model_get_iter_first(GTK_TREE_MODEL(store), &iter);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, protocol, 1, g_dgettext(plugin->domain, plugin->description), 2,
			   ((RemminaProtocolPlugin *)plugin)->icon_name, -1);

	if (first || g_strcmp0(protocol, remmina_file_get_string(gfe->priv->remmina_file, "protocol")) == 0)
		gtk_combo_box_set_active_iter(GTK_COMBO_BOX(gfe->priv->protocol_combo), &iter);

	return FALSE;
}

void remmina_file_editor_check_profile(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv;

	priv = gfe->priv;
	gtk_widget_set_sensitive(priv->group_combo, TRUE);
	gtk_widget_set_sensitive(priv->save_button, TRUE);
}

static void remmina_file_editor_entry_on_changed(GtkEditable *editable, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv;

	priv = gfe->priv;
	if (remmina_file_get_filename(priv->remmina_file) == NULL) {
		remmina_file_generate_filename(priv->remmina_file);
		/* TODO Probably to be removed */
		remmina_file_editor_check_profile(gfe);
	} else {
		remmina_file_delete(remmina_file_get_filename(priv->remmina_file));
		remmina_file_generate_filename(priv->remmina_file);
		remmina_file_editor_check_profile(gfe);
	}
}

void remmina_file_editor_file_save(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv;

	priv = gfe->priv;
	if (remmina_file_get_filename(priv->remmina_file) == NULL) {
		remmina_file_generate_filename(priv->remmina_file);
	} else {
		remmina_file_delete(remmina_file_get_filename(priv->remmina_file));
		remmina_file_generate_filename(priv->remmina_file);
	}
}


GtkWidget *remmina_file_editor_new_from_file(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaFileEditor *gfe;
	RemminaFileEditorPriv *priv;
	GtkWidget *grid;
	GtkWidget *widget;
	gchar *groups;
	gchar *s;
	const gchar *cs;

	gfe = REMMINA_FILE_EDITOR(g_object_new(REMMINA_TYPE_FILE_EDITOR, NULL));
	priv = gfe->priv;
	priv->remmina_file = remminafile;

	if (remmina_file_get_filename(remminafile) == NULL)
		gtk_dialog_set_response_sensitive(GTK_DIALOG(gfe), GTK_RESPONSE_APPLY, FALSE);

	/* Create the Profile group on the top (for name and protocol) */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(gfe))), grid, FALSE, FALSE, 2);

	//remmina_public_create_group(GTK_GRID(grid), _("Profile"), 0, 4, 3);

	/* Profile: Name */
	widget = gtk_label_new(_("Name"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 3, 2, 1);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 3, 3, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 100);
	priv->name_entry = widget;

	if (remmina_file_get_filename(remminafile) == NULL) {
		gtk_entry_set_text(GTK_ENTRY(widget), _("Quick Connect"));
#if GTK_CHECK_VERSION(3, 16, 0)
		gtk_entry_grab_focus_without_selecting(GTK_ENTRY(widget));
#endif
		g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(remmina_file_editor_entry_on_changed), gfe);
	} else {
		cs = remmina_file_get_string(remminafile, "name");
		gtk_entry_set_text(GTK_ENTRY(widget), cs ? cs : "");
	}

	/* Profile: Group */
	widget = gtk_label_new(_("Group"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 6, 2, 1);

	groups = remmina_file_manager_get_groups();
	priv->group_combo = remmina_public_create_combo_entry(groups, remmina_file_get_string(remminafile, "group"), FALSE);
	g_free(groups);
	gtk_widget_show(priv->group_combo);
	gtk_grid_attach(GTK_GRID(grid), priv->group_combo, 1, 6, 3, 1);
	gtk_widget_set_sensitive(priv->group_combo, FALSE);

	s = g_strdup_printf(_("Use '%s' as subgroup delimiter"), "/");
	gtk_widget_set_tooltip_text(priv->group_combo, s);
	g_free(s);

	/* Profile: Protocol */
	widget = gtk_label_new(_("Protocol"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 9, 2, 1);

	widget = remmina_public_create_combo(TRUE);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 9, 3, 1);
	priv->protocol_combo = widget;
	remmina_plugin_manager_for_each_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL, remmina_file_editor_iterate_protocol, gfe);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(remmina_file_editor_protocol_combo_on_changed), gfe);

	/* Prior Connection Command */
	widget = gtk_label_new(_("Pre Command"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 12, 3, 1);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 12, 3, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 200);
	priv->precommand_entry = widget;
	cs = remmina_file_get_string(remminafile, "precommand");
	gtk_entry_set_text(GTK_ENTRY(widget), cs ? cs : "");
	gtk_entry_set_placeholder_text(GTK_ENTRY(widget), "command %h %u %t %U %p %g --option");
	gtk_widget_set_tooltip_markup(widget, _(cmd_tips));

	/* POST Connection Command */
	widget = gtk_label_new(_("Post Command"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 15, 3, 1);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 15, 3, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 200);
	priv->postcommand_entry = widget;
	cs = remmina_file_get_string(remminafile, "postcommand");
	gtk_entry_set_text(GTK_ENTRY(widget), cs ? cs : "");
	gtk_entry_set_placeholder_text(GTK_ENTRY(widget), "/path/to/command -opt1 arg %h %u %t -opt2 %U %p %g");
	gtk_widget_set_tooltip_markup(widget, _(cmd_tips));

	/* Create the Preference frame */
	widget = gtk_event_box_new();
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(gfe))), widget, TRUE, TRUE, 2);
	priv->config_box = widget;

	priv->config_container = NULL;
	priv->config_scrollable = NULL;

	remmina_file_editor_protocol_combo_on_changed(GTK_COMBO_BOX(priv->protocol_combo), gfe);

	remmina_file_editor_check_profile(gfe);

	return GTK_WIDGET(gfe);
}

GtkWidget *remmina_file_editor_new(void)
{
	TRACE_CALL(__func__);
	return remmina_file_editor_new_full(NULL, NULL);
}

GtkWidget *remmina_file_editor_new_full(const gchar *server, const gchar *protocol)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;

	remminafile = remmina_file_new();
	if (server)
		remmina_file_set_string(remminafile, "server", server);
	if (protocol)
		remmina_file_set_string(remminafile, "protocol", protocol);

	return remmina_file_editor_new_from_file(remminafile);
}

GtkWidget *remmina_file_editor_new_copy(const gchar *filename)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;
	GtkWidget *dialog;

	remminafile = remmina_file_copy(filename);

	if (remminafile) {
		return remmina_file_editor_new_from_file(remminafile);
	} else {
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						_("File %s not found."), filename);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}
}

GtkWidget *remmina_file_editor_new_from_filename(const gchar *filename)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;
	GtkWidget *dialog;

	remminafile = remmina_file_manager_load_file(filename);
	if (remminafile) {
		return remmina_file_editor_new_from_file(remminafile);
	} else {
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						_("File %s not found."), filename);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}
}
