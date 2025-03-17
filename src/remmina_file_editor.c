/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
 * Copyright (C) 2023-2024 Hiroyuki Tanaka, Sunil Bhat
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

#include <ctype.h>
#include "config.h"
#ifdef HAVE_LIBAVAHI_UI
#include <avahi-ui/avahi-ui.h>
#endif
#include "remmina_public.h"
#include "remmina_pref.h"
#include "rcw.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_file.h"
#include "remmina_file_editor.h"
#include "remmina_file_manager.h"
#include "remmina_icon.h"
#include "remmina_main.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref_dialog.h"
#include "remmina_ssh.h"
#include "remmina_string_list.h"
#include "remmina_unlock.h"
#include "remmina_widget_pool.h"

G_DEFINE_TYPE(RemminaFileEditor, remmina_file_editor, GTK_TYPE_DIALOG)

static const gchar *server_tips = N_("<big>"
				     "Supported formats\n"
				     "• server\n"
				     "• server[:port]\n"
				     "VNC additional formats\n"
				     "• ID:repeater ID number\n"
				     "• unix:///path/socket.sock"
				     "</big>");

static const gchar *cmd_tips = N_("<big>"
				  "• command in PATH args %h\n"
				  "• /path/to/foo -options %h %u\n"
				  "• %h is substituted with the server name\n"
				  "• %t is substituted with the SSH server name\n"
				  "• %u is substituted with the username\n"
				  "• %U is substituted with the SSH username\n"
				  "• %p is substituted with Remmina profile name\n"
				  "• %g is substituted with Remmina profile group name\n"
				  "• %d is substituted with local date and time in ISO 8601 format\n"
				  "Do not run in background if you want the command to be executed before connecting.\n"
				  "</big>");

#ifdef HAVE_LIBSSH
static const gchar *server_tips2 = N_("<big>"
				      "Supported formats\n"
				      "• server\n"
				      "• server[:port]\n"
				      "• username@server[:port] (SSH protocol only)"
				      "</big>");
#endif

struct _RemminaFileEditorPriv {
	RemminaFile *		remmina_file;
	RemminaProtocolPlugin * plugin;
	const gchar *		avahi_service_type;

	GtkWidget *		name_entry;
	GtkWidget *		labels_entry;
	GtkWidget *		group_combo;
	GtkWidget *		protocol_combo;
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

	GtkWidget *		assistance_toggle;
	GtkWidget *		assistance_file;
	GtkWidget *		assistance_password;
	GtkWidget *		assistance_file_label;
	GtkWidget *		assistance_password_label;

	GtkWidget *		behavior_autostart_check;
	GtkWidget *		behavior_precommand_entry;
	GtkWidget *		behavior_postcommand_entry;
	GtkWidget *		behavior_lock_check;
	GtkWidget *		behavior_disconnect;

	GtkWidget *		ssh_tunnel_enabled_check;
	GtkWidget *		ssh_tunnel_loopback_check;
	GtkWidget *		ssh_tunnel_server_default_radio;
	GtkWidget *		ssh_tunnel_server_custom_radio;
	GtkWidget *		ssh_tunnel_server_entry;
	GtkWidget *		ssh_tunnel_command_entry;
	GtkWidget *		ssh_tunnel_auth_agent_radio;
	GtkWidget *		ssh_tunnel_auth_password_radio;
	GtkWidget *		ssh_tunnel_auth_password;
	GtkWidget *		ssh_tunnel_passphrase;
	GtkWidget *		ssh_tunnel_auth_publickey_radio;
	GtkWidget *		ssh_tunnel_auth_auto_publickey_radio;
	GtkWidget *		ssh_tunnel_auth_combo;
	GtkWidget *		ssh_tunnel_username_entry;
	GtkWidget *		ssh_tunnel_privatekey_chooser;
	GtkWidget *		ssh_tunnel_certfile_chooser;

	GHashTable *		setting_widgets;
};

static void remmina_file_editor_class_init(RemminaFileEditorClass *klass)
{
	TRACE_CALL(__func__);
}

/**
 * @brief Shows a tooltip-like window which tells the user what they did wrong
 * 		  to trigger the validation function of a ProtocolSetting widget.
 *
 * @param gfe GtkWindow gfe
 * @param failed_widget Widget which failed validation
 * @param err Contains error message for user
 *
 *
 * Mouse click and focus-loss will delete the window. \n
 * TODO: when Remmina Editor's content is scrollable and failed_widget is not even
 *	 visible anymore, the window gets shown where failed_widget would be if
 *       the Remmina Editor was big enough. \n
 * TODO: Responsive text size and line wrap.
 */
static void remmina_file_editor_show_validation_error_popup(RemminaFileEditor * gfe,
							    GtkWidget *		failed_widget,
							    GError *		err)
{
	if (!err) {
		err = NULL; // g_set_error doesn't like overwriting errors.
		g_set_error(&err, 1, 1, _("Input is invalid."));
	}

	if (!gfe || !failed_widget) {
		g_critical("(%s): Parameters RemminaFileEditor 'gfe' or "
			   "GtkWidget* 'failed_widget' are 'NULL'!",
			   __func__);
		return;
	}

	gint widget_width = gtk_widget_get_allocated_width(failed_widget);
	gint widget_height = gtk_widget_get_allocated_height(failed_widget);

	GtkWidget *err_label = gtk_label_new("");
	GtkWidget *alert_icon = NULL;
	GtkWindow *err_window = GTK_WINDOW(gtk_window_new(GTK_WINDOW_TOPLEVEL));
	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	GdkWindow *window = gtk_widget_get_window(failed_widget);

	GtkAllocation allocation;
	gint failed_widget_x, failed_widget_y;

	gchar *markup = g_strdup_printf("<span size='large'>%s</span>", err->message);

	// Setup err_window
	gtk_window_set_decorated(err_window, FALSE);
	gtk_window_set_type_hint(err_window, GDK_WINDOW_TYPE_HINT_TOOLTIP);
	gtk_window_set_default_size(err_window, widget_width, widget_height);
	gtk_window_set_title(err_window, "Error");
	gtk_window_set_resizable(err_window, TRUE);

	// Move err_window under failed_widget
	gtk_window_set_attached_to(err_window, failed_widget);
	gtk_window_set_transient_for(err_window, GTK_WINDOW(gfe));
	gdk_window_get_origin(GDK_WINDOW(window), &failed_widget_x, &failed_widget_y);
	gtk_widget_get_allocation(failed_widget, &allocation);
	failed_widget_x += allocation.x;
	failed_widget_y += allocation.y + allocation.height;
	gtk_window_move(err_window, failed_widget_x, failed_widget_y);

	// Setup label
	gtk_label_set_selectable(GTK_LABEL(err_label), FALSE);
	gtk_label_set_max_width_chars(GTK_LABEL(err_label), 1);
	gtk_widget_set_hexpand(GTK_WIDGET(err_label), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(err_label), TRUE);
	gtk_label_set_ellipsize(GTK_LABEL(err_label), PANGO_ELLIPSIZE_END);
	gtk_label_set_line_wrap(GTK_LABEL(err_label), TRUE);
	gtk_label_set_line_wrap_mode(GTK_LABEL(err_label), PANGO_WRAP_WORD_CHAR);
	gtk_label_set_markup(GTK_LABEL(err_label), markup);

	alert_icon = gtk_image_new_from_icon_name("dialog-warning-symbolic",
						  GTK_ICON_SIZE_DND);

	// Fill icon and label into a box.
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(alert_icon), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), GTK_WIDGET(err_label), TRUE, TRUE, 5);

	// Attach box to err_window
	gtk_container_add(GTK_CONTAINER(err_window), GTK_WIDGET(box));

	// Display everything.
	gtk_widget_show_all(GTK_WIDGET(err_window));

	// Mouse click and focus-loss will delete the err_window.
	g_signal_connect(G_OBJECT(err_window), "focus-out-event",
			 G_CALLBACK(gtk_window_close), NULL);
	g_signal_connect(G_OBJECT(err_window), "button-press-event",
			 G_CALLBACK(gtk_window_close), NULL);
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


static void remmina_file_editor_assistance_enabled_check_on_toggled(GtkToggleButton *togglebutton,
								    RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	gboolean enabled = TRUE;

	if (gfe->priv->assistance_toggle) {
		enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->assistance_toggle));
		if (gfe->priv->assistance_file)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->assistance_file), enabled);
		if (gfe->priv->assistance_password)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->assistance_password), enabled);
		if (gfe->priv->assistance_file_label)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->assistance_file_label), enabled);
		if (gfe->priv->assistance_password_label)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->assistance_password_label), enabled);
	}
}

#ifdef HAVE_LIBSSH

static void remmina_file_editor_ssh_tunnel_server_custom_radio_on_toggled(GtkToggleButton *togglebutton, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_tunnel_server_entry),
				 gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->ssh_tunnel_enabled_check)) &&
				 (gfe->priv->ssh_tunnel_server_custom_radio == NULL ||
				  gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->ssh_tunnel_server_custom_radio))));
}


static void remmina_file_editor_ssh_tunnel_enabled_check_on_toggled(GtkToggleButton *togglebutton,
								    RemminaFileEditor *gfe, RemminaProtocolSSHSetting ssh_setting)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	gboolean enabled = TRUE;
	gchar *p;
	const gchar *cp;
	const gchar *s = NULL;

	if (gfe->priv->ssh_tunnel_enabled_check) {
		enabled = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gfe->priv->ssh_tunnel_enabled_check));
		if (gfe->priv->ssh_tunnel_loopback_check)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_tunnel_loopback_check), enabled);
		if (gfe->priv->ssh_tunnel_server_default_radio)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_tunnel_server_default_radio), enabled);
		if (gfe->priv->ssh_tunnel_server_custom_radio)
			gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_tunnel_server_custom_radio), enabled);
		remmina_file_editor_ssh_tunnel_server_custom_radio_on_toggled(NULL, gfe);
		p = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->protocol_combo));
		gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_tunnel_username_entry), enabled);
		gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_tunnel_auth_password), enabled);
		gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_tunnel_auth_combo), enabled);
		gtk_widget_set_sensitive(GTK_WIDGET(gfe->priv->ssh_tunnel_command_entry), enabled);
		
		g_free(p);
	}
	s = remmina_file_get_string(gfe->priv->remmina_file, "ssh_tunnel_privatekey");
	if (s)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gfe->priv->ssh_tunnel_privatekey_chooser), s);
	s = remmina_file_get_string(gfe->priv->remmina_file, "ssh_tunnel_certfile");
	if (s)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gfe->priv->ssh_tunnel_certfile_chooser), s);

	if (gfe->priv->ssh_tunnel_username_entry)
		if (enabled && gtk_entry_get_text(GTK_ENTRY(gfe->priv->ssh_tunnel_username_entry))[0] == '\0') {
			cp = remmina_file_get_string(priv->remmina_file, "ssh_tunnel_username");
			gtk_entry_set_text(GTK_ENTRY(gfe->priv->ssh_tunnel_username_entry), cp ? cp : "");
		}

	if (gfe->priv->ssh_tunnel_auth_password) {
		if (enabled && gtk_entry_get_text(GTK_ENTRY(gfe->priv->ssh_tunnel_auth_password))[0] == '\0') {
			cp = remmina_file_get_string(priv->remmina_file, "ssh_tunnel_password");
			gtk_entry_set_text(GTK_ENTRY(gfe->priv->ssh_tunnel_auth_password), cp ? cp : "");
		}
	}
	if (gfe->priv->ssh_tunnel_passphrase) {
		if (enabled && gtk_entry_get_text(GTK_ENTRY(gfe->priv->ssh_tunnel_passphrase))[0] == '\0') {
			cp = remmina_file_get_string(priv->remmina_file, "ssh_tunnel_passphrase");
			gtk_entry_set_text(GTK_ENTRY(gfe->priv->ssh_tunnel_passphrase), cp ? cp : "");
		}
	}
	if (gfe->priv->ssh_tunnel_command_entry) {
		if (enabled && gtk_entry_get_text(GTK_ENTRY(gfe->priv->ssh_tunnel_command_entry))[0] == '\0') {
			cp = remmina_file_get_string(priv->remmina_file, "ssh_tunnel_command");
			gtk_entry_set_text(GTK_ENTRY(gfe->priv->ssh_tunnel_command_entry), cp ? cp : "");
		}
	}
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


static GtkWidget *remmina_file_editor_create_password(RemminaFileEditor *gfe, GtkWidget *grid, gint row, gint col, const gchar *label, const gchar *value, gchar *setting_name)
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
	gtk_entry_set_max_length(GTK_ENTRY(widget), 0);
	gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_entry_set_activates_default(GTK_ENTRY(widget), TRUE);
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);

	if (value)
		gtk_entry_set_text(GTK_ENTRY(widget), value);
	/* Password view Toogle*/
	if (setting_name) {
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(widget), GTK_ENTRY_ICON_SECONDARY, "org.remmina.Remmina-password-reveal-symbolic");
		gtk_entry_set_icon_activatable(GTK_ENTRY(widget), GTK_ENTRY_ICON_SECONDARY, TRUE);
		g_signal_connect(widget, "icon-press", G_CALLBACK(remmina_main_toggle_password_view), NULL);
	}
	return widget;
}

static void remmina_file_editor_update_resolution(GtkWidget *widget, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	gchar *res_str;
	res_str = g_strdup_printf("%dx%d",
				  remmina_file_get_int(gfe->priv->remmina_file, "resolution_width", 0),
				  remmina_file_get_int(gfe->priv->remmina_file, "resolution_height", 0));
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


static void remmina_file_editor_create_assistance(RemminaFileEditor *gfe, const RemminaProtocolSetting *setting,
						  GtkWidget *grid, gint row)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;



	widget = gtk_toggle_button_new_with_label(_("Assistance Mode"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), remmina_file_get_int(gfe->priv->remmina_file, "assistance_mode", 0));
	gfe->priv->assistance_toggle = widget;
	g_signal_connect(widget, "toggled", G_CALLBACK(remmina_file_editor_assistance_enabled_check_on_toggled), gfe);


	widget = gtk_label_new("Assistance file");
	gtk_widget_set_halign(widget, GTK_ALIGN_END);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row+1, 1, 1);
	gfe->priv->assistance_file_label = widget;

	widget = gtk_entry_new();
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_widget_show(widget);
	
	if (remmina_file_get_string(gfe->priv->remmina_file, "assistance_file") != NULL) {
		gtk_entry_set_text(GTK_ENTRY(widget), remmina_file_get_string(gfe->priv->remmina_file, "assistance_file"));
	}
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row+1, 1, 1);
	gfe->priv->assistance_file = widget;

	widget = gtk_label_new("Assistance Password");
	gtk_widget_set_halign(widget, GTK_ALIGN_END);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row+2, 1, 1);
	gfe->priv->assistance_password_label = widget;

	widget = gtk_entry_new();
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_widget_show(widget);

	if (remmina_file_get_string(gfe->priv->remmina_file, "assistance_pass") != NULL) {
		gtk_entry_set_text(GTK_ENTRY(widget), remmina_file_get_string(gfe->priv->remmina_file, "assistance_pass"));
	}
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row+2, 1, 1);
	gfe->priv->assistance_password = widget;

	remmina_file_editor_assistance_enabled_check_on_toggled(NULL, gfe);

}


static GtkWidget *remmina_file_editor_create_text2(RemminaFileEditor *gfe, GtkWidget *grid,
						   gint row, gint col, const gchar *label, const gchar *value, gint left,
						   gint right, gchar *setting_name)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
#if GTK_CHECK_VERSION(3, 12, 0)
	gtk_widget_set_margin_start(widget, left);
	gtk_widget_set_margin_end(widget, right);
#else
	gtk_widget_set_margin_left(widget, left);
	gtk_widget_set_margin_right(widget, right);
#endif
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, col, row, 1, 1);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, col + 1, row, 1, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 300);
	gtk_widget_set_hexpand(widget, TRUE);
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);

	if (value)
		gtk_entry_set_text(GTK_ENTRY(widget), value);

	return widget;
}

static GtkWidget *remmina_file_editor_create_text(RemminaFileEditor *gfe, GtkWidget *grid,
						  gint row, gint col, const gchar *label, const gchar *value,
						  gchar *setting_name)
{
	TRACE_CALL(__func__);
	return remmina_file_editor_create_text2(gfe, grid, row, col, label, value, 0, 40,
						setting_name);
}

static GtkWidget *remmina_file_editor_create_textarea(RemminaFileEditor *gfe, GtkWidget *grid,
						      gint row, gint col, const gchar *label, const gchar *value,
						      gchar *setting_name)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	GtkTextView *view;
	GtkTextBuffer *buffer;
	GtkTextIter start;

	widget = gtk_text_view_new();
	view = GTK_TEXT_VIEW(widget);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
	gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 20);
	gtk_text_view_set_bottom_margin(GTK_TEXT_VIEW(view), 20);
	gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 20);
	gtk_text_view_set_right_margin(GTK_TEXT_VIEW(view), 20);
	gtk_text_view_set_monospace(view, TRUE);
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);
	if (value) {
		buffer = gtk_text_view_get_buffer(view);
		gtk_text_buffer_set_text(buffer, value, -1);
		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_place_cursor(buffer, &start);
	}
	gtk_widget_show(widget);
	gtk_widget_set_hexpand(widget, TRUE);
	gtk_widget_set_size_request(GTK_WIDGET(view), 320, 300);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	return widget;
}

static GtkWidget *remmina_file_editor_create_select(RemminaFileEditor *gfe, GtkWidget *grid,
						    gint row, gint col, const gchar *label, const gpointer *list,
						    const gchar *value, gchar *setting_name)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);

	widget = remmina_public_create_combo_map(list, value, FALSE, gfe->priv->plugin->domain);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);

	return widget;
}

static GtkWidget *remmina_file_editor_create_combo(RemminaFileEditor *gfe, GtkWidget *grid,
						   gint row, gint col, const gchar *label, const gchar *list,
						   const gchar *value, gchar *setting_name)
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
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 1, 1);

	return widget;
}

static GtkWidget *remmina_file_editor_create_check(RemminaFileEditor *gfe, GtkWidget *grid,
						   gint row, gint top, const gchar *label, gboolean value,
						   gchar *setting_name)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	widget = gtk_check_button_new_with_label(label);
	gtk_widget_show(widget);
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);
	gtk_grid_attach(GTK_GRID(grid), widget, top, row, 1, 1);

	if (value)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);

	return widget;
}

static void remmina_file_editor_export_settings(RemminaFileEditor* rfe)
{
	RemminaFileEditorPriv *priv = rfe->priv;
}

static void remmina_file_editor_save_ssh_tunnel_tab(RemminaFileEditor *gfe, RemminaFile* ssh_file)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	gboolean ssh_tunnel_enabled;
	int ssh_tunnel_auth;
	RemminaFile* remminafile;
	if (ssh_file == NULL){
		remminafile = priv->remmina_file;
	}
	else{
		remminafile = ssh_file;
	}
	

	ssh_tunnel_enabled = (priv->ssh_tunnel_enabled_check ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->ssh_tunnel_enabled_check)) : FALSE);
	remmina_file_set_int(remminafile,
			     "ssh_tunnel_loopback",
			     (priv->ssh_tunnel_loopback_check ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->ssh_tunnel_loopback_check)) : FALSE));
	remmina_file_set_int(remminafile, "ssh_tunnel_enabled", ssh_tunnel_enabled);
	remmina_file_set_string(remminafile, "ssh_tunnel_auth",
				remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->ssh_tunnel_auth_combo)));
	remmina_file_set_string(remminafile, "ssh_tunnel_username",
				(ssh_tunnel_enabled ? gtk_entry_get_text(GTK_ENTRY(priv->ssh_tunnel_username_entry)) : NULL));
	remmina_file_set_string(
		remminafile,
		"ssh_tunnel_server",
		(ssh_tunnel_enabled && priv->ssh_tunnel_server_entry && (priv->ssh_tunnel_server_custom_radio == NULL || gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->ssh_tunnel_server_custom_radio))) ? gtk_entry_get_text(GTK_ENTRY(priv->ssh_tunnel_server_entry)) : NULL));

	ssh_tunnel_auth = gtk_combo_box_get_active(GTK_COMBO_BOX(priv->ssh_tunnel_auth_combo));

	remmina_file_set_int(
		remminafile,
		"ssh_tunnel_auth",
		ssh_tunnel_auth);
	
	// If box is unchecked for private key and certfile file choosers,
	// set the string to NULL in the remmina file 
	if (gtk_widget_get_sensitive(priv->ssh_tunnel_privatekey_chooser)) {
		remmina_file_set_string(
		remminafile,
		"ssh_tunnel_privatekey",
		(priv->ssh_tunnel_privatekey_chooser ? gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(priv->ssh_tunnel_privatekey_chooser)) : NULL));
	}
	else {
		remmina_file_set_string(remminafile, "ssh_tunnel_privatekey", NULL);
	}
	
	if (gtk_widget_get_sensitive(priv->ssh_tunnel_certfile_chooser)) {
		remmina_file_set_string(
			remminafile,
			"ssh_tunnel_certfile",
			(priv->ssh_tunnel_certfile_chooser ? gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(priv->ssh_tunnel_certfile_chooser)) : NULL));
	}
	else {
		remmina_file_set_string(remminafile, "ssh_tunnel_certfile", NULL);
	}

	remmina_file_set_string(
		remminafile,
		"ssh_tunnel_password",
		(ssh_tunnel_enabled && (ssh_tunnel_auth == SSH_AUTH_PASSWORD)) ? gtk_entry_get_text(GTK_ENTRY(priv->ssh_tunnel_auth_password)) : NULL);

	const char* command = gtk_entry_get_text(GTK_ENTRY(priv->ssh_tunnel_command_entry));
	remmina_file_set_string(
		remminafile,
		"ssh_tunnel_command", command);

	remmina_file_set_string(
		remminafile,
		"ssh_tunnel_passphrase",
		(ssh_tunnel_enabled && (ssh_tunnel_auth == SSH_AUTH_PUBLICKEY || ssh_tunnel_auth == SSH_AUTH_AUTO_PUBLICKEY)) ? gtk_entry_get_text(GTK_ENTRY(priv->ssh_tunnel_passphrase)) : NULL);
}

static void remmina_file_editor_run_saver_dialog(GtkButton* self, gpointer user_data)
{

	RemminaFileEditor* gfe = (RemminaFileEditor*)user_data;
	RemminaFileEditorPriv *priv = gfe->priv;
	GtkWidget* dialog = gtk_file_chooser_dialog_new("Save to...", NULL, GTK_FILE_CHOOSER_ACTION_SAVE, _("_Cancel"),
		GTK_RESPONSE_CANCEL, _("Save"), GTK_RESPONSE_ACCEPT, NULL);

	GtkFileFilter *filter;


	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Remmina Files"));
	gtk_file_filter_add_pattern(filter, "*.remssh");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), _("ssh_export.remssh"));
	gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), remmina_pref.datadir_path);
	gint response = gtk_dialog_run(GTK_DIALOG(dialog));
	

	if (response == GTK_RESPONSE_ACCEPT){
		RemminaFile* export_rf = (RemminaFile*)remmina_file_new();
		remmina_file_editor_save_ssh_tunnel_tab(gfe, export_rf);
		
		export_rf->filename = g_file_get_path(gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog)));
		remmina_file_save(export_rf);

	}
	gtk_widget_destroy(dialog);
	
}


static GtkWidget* remmina_file_editor_create_sssh_import_export(RemminaFileEditor *gfe, GtkWidget *grid, gint row, gint col, const gchar *label,
				   const gchar *value, gint type, gchar *setting_name)
{
	GtkWidget* widget = gtk_button_new_with_label(label);
	gtk_widget_set_name(widget, label);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	
	

	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_run_saver_dialog), gfe);

	return widget;

}


/**
 * Create checkbox + gtk_file_chooser for open files and select folders
 *
 * The code is wrong, because if the checkbox is not active, the value should be set to NULL
 * and remove it from the remmina file. The problem is that this function knows nothing about
 * the remmina file.
 * This should be rewritten in a more generic way
 * Please use REMMINA_PROTOCOL_SETTING_TYPE_TEXT
 */
static GtkWidget *
remmina_file_editor_create_chooser(RemminaFileEditor *gfe, GtkWidget *grid, gint row, gint col, const gchar *label,
				   const gchar *value, gint type, gchar *setting_name)
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
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);
	gtk_widget_show(widget);
	if (value)
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widget), value);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(remmina_file_editor_button_on_toggled), widget);
	remmina_file_editor_button_on_toggled(GTK_TOGGLE_BUTTON(check), widget);

	return widget;
}

// used to filter out invalid characters for REMMINA_PROTOCOL_SETTING_TYPE_INT
void remmina_file_editor_int_setting_filter(GtkEditable *editable, const gchar *text,
					    gint length, gint *position, gpointer data)
{
	for (int i = 0; i < length; i++) {
		if (!isdigit(text[i]) && text[i] != '-') {
			g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
			return;
		}
	}
}

// used to filter out invalid characters for REMMINA_PROTOCOL_SETTING_TYPE_DOUBLE
// '.' and ',' can't be used interchangeably! It depends on the language setting
// of the user.
void remmina_file_editor_double_setting_filter(GtkEditable *editable, const gchar *text,
					       gint length, gint *position, gpointer data)
{
	for (int i = 0; i < length; i++) {
		if (!isdigit(text[i]) && text[i] != '-' && text[i] != '.' && text[i] != ',') {
			g_signal_stop_emission_by_name(G_OBJECT(editable), "insert-text");
			return;
		}
	}
}

static GtkWidget *remmina_file_editor_create_int(RemminaFileEditor *gfe, GtkWidget *grid,
						 gint row, gint col, const gchar *label, const gint value,
						 gint left, gint right, gchar *setting_name)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
#if GTK_CHECK_VERSION(3, 12, 0)
	gtk_widget_set_margin_start(widget, left);
	gtk_widget_set_margin_end(widget, right);
#else
	gtk_widget_set_margin_left(widget, left);
	gtk_widget_set_margin_right(widget, right);
#endif
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, col, row, 1, 1);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, col + 1, row, 1, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 300);
	gtk_widget_set_hexpand(widget, TRUE);
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);

	// Convert int to str.
	int length = snprintf(NULL, 0, "%d", value) + 1; // +1 '\0' byte
	char *str = malloc(length);
	snprintf(str, length, "%d", value);

	gtk_entry_set_text(GTK_ENTRY(widget), str);
	free(str);

	g_signal_connect(G_OBJECT(widget), "insert-text",
			 G_CALLBACK(remmina_file_editor_int_setting_filter), NULL);

	return widget;
}

static GtkWidget *remmina_file_editor_create_double(RemminaFileEditor *gfe,
						    GtkWidget *grid, gint row, gint col,
						    const gchar *label, gdouble value, gint left,
						    gint right, gchar *setting_name)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
#if GTK_CHECK_VERSION(3, 12, 0)
	gtk_widget_set_margin_start(widget, left);
	gtk_widget_set_margin_end(widget, right);
#else
	gtk_widget_set_margin_left(widget, left);
	gtk_widget_set_margin_right(widget, right);
#endif
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, col, row, 1, 1);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, col + 1, row, 1, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 300);
	gtk_widget_set_hexpand(widget, TRUE);
	if (setting_name)
		gtk_widget_set_name(widget, setting_name);

	// Convert double to str.
	int length = snprintf(NULL, 0, "%.8g", value) + 1; // +1 '\0' byte
	char *str = malloc(length);
	snprintf(str, length, "%f", value);

	gtk_entry_set_text(GTK_ENTRY(widget), str);
	free(str);

	g_signal_connect(G_OBJECT(widget), "insert-text",
			 G_CALLBACK(remmina_file_editor_double_setting_filter), NULL);

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
	const gchar *escaped;

	while (settings->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
		setting_name = (gchar *)(remmina_plugin_manager_get_canonical_setting_name(settings));
		switch (settings->type) {
		case REMMINA_PROTOCOL_SETTING_TYPE_SERVER:
			remmina_file_editor_create_server(gfe, settings, grid, grid_row);
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD:
			widget = remmina_file_editor_create_password(gfe, grid, grid_row, 0,
								     g_dgettext(priv->plugin->domain, settings->label),
								     remmina_file_get_string(priv->remmina_file, setting_name),
								     setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			grid_row++;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION:
			remmina_file_editor_create_resolution(gfe, settings, grid, grid_row);
			grid_row ++;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_ASSISTANCE:
			remmina_file_editor_create_assistance(gfe, settings, grid, grid_row);
			grid_row += 3;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_KEYMAP:
			strarr = remmina_pref_keymap_groups();
			priv->keymap_combo = remmina_file_editor_create_select(gfe, grid,
									       grid_row + 1, 0,
									       _("Keyboard mapping"), (const gpointer *)strarr,
									       remmina_file_get_string(priv->remmina_file, "keymap"),
									       setting_name);
			g_strfreev(strarr);
			grid_row++;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_TEXT:
			widget = remmina_file_editor_create_text(gfe, grid, grid_row, 0,
								 g_dgettext(priv->plugin->domain, settings->label),
								 remmina_file_get_string(priv->remmina_file, setting_name),
								 setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text(widget, _((const gchar *)settings->opt2));
			if (settings->opt1){
				gtk_entry_set_placeholder_text(GTK_ENTRY(widget), _((const gchar *)settings->opt1));
			}
			grid_row++;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_TEXTAREA:
			escaped = remmina_file_get_string(priv->remmina_file, setting_name);
			escaped = g_uri_unescape_string(escaped, NULL);
			widget = remmina_file_editor_create_textarea(gfe, grid, grid_row, 0,
								     g_dgettext(priv->plugin->domain, settings->label), escaped,
								     setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			grid_row++;
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_SELECT:
			widget = remmina_file_editor_create_select(gfe, grid, grid_row, 0,
								   g_dgettext(priv->plugin->domain, settings->label),
								   (const gpointer *)settings->opt1,
								   remmina_file_get_string(priv->remmina_file, setting_name),
								   setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text(widget, _((const gchar *)settings->opt2));
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_COMBO:
			widget = remmina_file_editor_create_combo(gfe, grid, grid_row, 0,
								  g_dgettext(priv->plugin->domain, settings->label),
								  (const gchar *)settings->opt1,
								  remmina_file_get_string(priv->remmina_file, setting_name),
								  setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text(widget, _((const gchar *)settings->opt2));
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_CHECK:
			widget = remmina_file_editor_create_check(gfe, grid, grid_row, grid_column,
								  g_dgettext(priv->plugin->domain, settings->label),
								  remmina_file_get_int(priv->remmina_file, setting_name, FALSE),
								  setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text(widget, _((const gchar *)settings->opt2));
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_FILE:
			widget = remmina_file_editor_create_chooser(gfe, grid, grid_row, 0,
								    g_dgettext(priv->plugin->domain, settings->label),
								    remmina_file_get_string(priv->remmina_file, setting_name),
								    GTK_FILE_CHOOSER_ACTION_OPEN, setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text(widget, _((const gchar *)settings->opt2));
			break;

		case REMMINA_PROTOCOL_SETTING_TYPE_FOLDER:
			widget = remmina_file_editor_create_chooser(gfe, grid, grid_row, 0,
								    g_dgettext(priv->plugin->domain, settings->label),
								    remmina_file_get_string(priv->remmina_file, setting_name),
								    GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
								    setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text(widget, _((const gchar *)settings->opt2));
			break;
		case REMMINA_PROTOCOL_SETTING_TYPE_INT:
			widget = remmina_file_editor_create_int(gfe, grid, grid_row, 0,
								g_dgettext(priv->plugin->domain, settings->label),
								remmina_file_get_int(priv->remmina_file, setting_name, 0),
								0, 40, setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text(widget, _((const gchar *)settings->opt2));
			grid_row++;
			break;
		case REMMINA_PROTOCOL_SETTING_TYPE_DOUBLE:
			widget = remmina_file_editor_create_double(gfe, grid, grid_row, 0,
								   g_dgettext(priv->plugin->domain, settings->label),
								   remmina_file_get_double(priv->remmina_file, setting_name, 0.0f),
								   0, 40, setting_name);
			g_hash_table_insert(priv->setting_widgets, setting_name, widget);
			if (settings->opt2)
				gtk_widget_set_tooltip_text(widget, _((const gchar *)settings->opt2));
			grid_row++;
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

static void remmina_file_editor_create_behavior_tab(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	GtkWidget *grid;
	GtkWidget *widget;
	const gchar *cs;

	/* The Behavior tab (implementation) */
	grid = remmina_file_editor_create_notebook_tab(gfe, NULL, _("Behavior"), 20, 2);

	/* Execute Command frame */
	remmina_public_create_group(GTK_GRID(grid), _("Execute a Command"), 0, 1, 2);

	/* Sandbox disclaimer*/
	widget = gtk_label_new(NULL);
	gtk_widget_show(widget);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_label_set_text(GTK_LABEL(widget), "* Local commands may not function properly if run in a sandboxed environment *");
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 2, 2, 2);

	/* PRE connection command */
	cs = remmina_file_get_string(priv->remmina_file, "precommand");
	widget = remmina_file_editor_create_text2(gfe, grid, 4, 0, _("Before connecting"), cs, 24, 26, "precommand");
	priv->behavior_precommand_entry = widget;
	gtk_entry_set_placeholder_text(GTK_ENTRY(widget), _("command %h %u %t %U %p %g --option"));
	gtk_widget_set_tooltip_markup(widget, _(cmd_tips));

	/* POST connection command */
	cs = remmina_file_get_string(priv->remmina_file, "postcommand");
	widget = remmina_file_editor_create_text2(gfe, grid, 5, 0, _("After connecting"), cs, 24, 16, "postcommand");
	priv->behavior_postcommand_entry = widget;
	gtk_entry_set_placeholder_text(GTK_ENTRY(widget), _("/path/to/command -opt1 arg %h %u %t -opt2 %U %p %g"));
	gtk_widget_set_tooltip_markup(widget, _(cmd_tips));

	/* Startup frame */
	remmina_public_create_group(GTK_GRID(grid), _("Start-up"), 6, 1, 2);

	/* Autostart profile option */
	priv->behavior_autostart_check = remmina_file_editor_create_check(gfe, grid, 8, 1, _("Auto-start this profile"),
									  remmina_file_get_int(priv->remmina_file, "enable-autostart", FALSE), "enable-autostart");

	/* Startup frame */
	remmina_public_create_group(GTK_GRID(grid), _("Connection profile security"), 10, 1, 2);

	/* Autostart profile option */
	priv->behavior_lock_check = remmina_file_editor_create_check(gfe, grid, 12, 1, _("Require password to connect or edit the profile"),
								     remmina_file_get_int(priv->remmina_file, "profile-lock", FALSE), "profile-lock");

									 /* Startup frame */
	remmina_public_create_group(GTK_GRID(grid), _("Unexpected disconnect"), 14, 1, 2);

	/* Autostart profile option */
	priv->behavior_disconnect = remmina_file_editor_create_check(gfe, grid, 18, 1, _("Keep window from closing if not disconnected by Remmina"),
								     remmina_file_get_int(priv->remmina_file, "disconnect-prompt", FALSE), "disconnect-prompt");
}

#ifdef HAVE_LIBSSH
static gpointer ssh_tunnel_auth_list[] =
{
	"0", N_("Password"),
	"1", N_("SSH identity file"),
	"2", N_("SSH agent"),
	"3", N_("Public key (automatic)"),
	"4", N_("Kerberos (GSSAPI)"),
	NULL
};
#endif

static gboolean
remmina_file_editor_ssh_tunnel_import_settings(GtkFileChooserButton* self, gpointer user_data){
	g_debug("Importing settings from %s", gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self)));
}

static void remmina_file_editor_create_ssh_tunnel_tab(RemminaFileEditor *gfe, RemminaProtocolSSHSetting ssh_setting)
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

	if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_NONE)
		return;

	/* The SSH tab (implementation) */
	grid = remmina_file_editor_create_notebook_tab(gfe, NULL,
						       _("SSH Tunnel"), 12, 3);
	widget = gtk_toggle_button_new_with_label(_("Enable SSH tunnel"));
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	g_signal_connect(G_OBJECT(widget), "toggled",
			 G_CALLBACK(remmina_file_editor_ssh_tunnel_enabled_check_on_toggled), gfe);
	priv->ssh_tunnel_enabled_check = widget;

	widget = gtk_check_button_new_with_label(_("Tunnel via loopback address"));
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 2, 1);
	priv->ssh_tunnel_loopback_check = widget;

	// 1
	row++;
	/* SSH Server group */

	switch (ssh_setting) {
	case REMMINA_PROTOCOL_SSH_SETTING_TUNNEL:
		s = g_strdup_printf(_("Same server at port %i"), DEFAULT_SSH_PORT);
		widget = gtk_radio_button_new_with_label(NULL, s);
		g_free(s);
		gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 3, 1);
		priv->ssh_tunnel_server_default_radio = widget;
		// 2
		row++;

		widget = gtk_radio_button_new_with_label_from_widget(
			GTK_RADIO_BUTTON(priv->ssh_tunnel_server_default_radio), _("Custom"));
		gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
		g_signal_connect(G_OBJECT(widget), "toggled",
				 G_CALLBACK(remmina_file_editor_ssh_tunnel_server_custom_radio_on_toggled), gfe);
		priv->ssh_tunnel_server_custom_radio = widget;

		widget = gtk_entry_new();
		gtk_entry_set_max_length(GTK_ENTRY(widget), 100);
		gtk_widget_set_tooltip_markup(widget, _(server_tips2));
		gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 2, 1);
		priv->ssh_tunnel_server_entry = widget;
		// 3
		row++;
		break;

	case REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL:
		priv->ssh_tunnel_server_default_radio = NULL;
		priv->ssh_tunnel_server_custom_radio = NULL;

		priv->ssh_tunnel_server_entry = remmina_file_editor_create_text(gfe, grid, 1, 0,
										_("Server"), NULL, "ssh_reverse_tunnel_server");
		gtk_widget_set_tooltip_markup(priv->ssh_tunnel_server_entry, _(server_tips));
		// 2
		row++;
		break;
	case REMMINA_PROTOCOL_SSH_SETTING_SSH:
	case REMMINA_PROTOCOL_SSH_SETTING_SFTP:
		priv->ssh_tunnel_server_default_radio = NULL;
		priv->ssh_tunnel_server_custom_radio = NULL;
		priv->ssh_tunnel_server_entry = NULL;

		break;

	default:
		break;
	}

	/* This is not used? */
	p = remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->protocol_combo));
	if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_SFTP) {
		widget = remmina_file_editor_create_text(gfe, grid, row, 1,
							 _("Start-up path"), NULL, "start-up-path");
		cs = remmina_file_get_string(priv->remmina_file, "execpath");
		gtk_entry_set_text(GTK_ENTRY(widget), cs ? cs : "");
		g_hash_table_insert(priv->setting_widgets, "execpath", widget);
		// 2
		row++;
	}

	/* SSH Authentication frame */
	remmina_public_create_group(GTK_GRID(grid), _("SSH Authentication"), row, 6, 1);
	// 5
	row += 2;

	priv->ssh_tunnel_auth_combo = remmina_file_editor_create_select(gfe, grid, row, 0,
									_("Authentication type"),
									(const gpointer *)ssh_tunnel_auth_list,
									remmina_file_get_string(priv->remmina_file, "ssh_tunnel_auth"), "ssh_tunnel_auth");
	row++;

	if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_TUNNEL ||
	    ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL) {
		priv->ssh_tunnel_username_entry =
			remmina_file_editor_create_text(gfe, grid, row, 0,
							_("Username"), NULL, "ssh_tunnel_username");
		// 5
		row++;
	}

	widget = remmina_file_editor_create_password(gfe, grid, row, 0,
						     _("Password"),
						     remmina_file_get_string(priv->remmina_file, "ssh_tunnel_password"),
						     "ssh_tunnel_password");
	priv->ssh_tunnel_auth_password = widget;
	row++;

	priv->ssh_tunnel_privatekey_chooser = remmina_file_editor_create_chooser(gfe, grid, row, 0,
										 _("SSH private key file"),
										 remmina_file_get_string(priv->remmina_file, "ssh_tunnel_privatekey"),
										 GTK_FILE_CHOOSER_ACTION_OPEN, "ssh_tunnel_privatekey");
	row++;

	priv->ssh_tunnel_certfile_chooser = remmina_file_editor_create_chooser(gfe, grid, row, 0,
									       _("SSH certificate file"),
									       remmina_file_get_string(priv->remmina_file, "ssh_tunnel_certfile"),
									       GTK_FILE_CHOOSER_ACTION_OPEN, "ssh_tunnel_certfile");
	row++;

	widget = gtk_label_new(_("Password to unlock private key"));
	gtk_grid_attach(GTK_GRID(grid), widget, 0, row, 1, 1);
	widget = gtk_entry_new();
	gtk_grid_attach(GTK_GRID(grid), widget, 1, row, 2, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 300);
	gtk_entry_set_visibility(GTK_ENTRY(widget), FALSE);
	gtk_widget_set_hexpand(widget, TRUE);
	priv->ssh_tunnel_passphrase = widget;
	row++;

	/* Set the values */
	cs = remmina_file_get_string(priv->remmina_file, "ssh_tunnel_server");
	if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_TUNNEL) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->ssh_tunnel_enabled_check),
					     remmina_file_get_int(priv->remmina_file, "ssh_tunnel_enabled", FALSE));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->ssh_tunnel_loopback_check),
					     remmina_file_get_int(priv->remmina_file, "ssh_tunnel_loopback", FALSE));

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(cs ? priv->ssh_tunnel_server_custom_radio : priv->ssh_tunnel_server_default_radio), TRUE);
		gtk_entry_set_text(GTK_ENTRY(priv->ssh_tunnel_server_entry),
				   cs ? cs : "");
	} else if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->ssh_tunnel_enabled_check),
					     remmina_file_get_int(priv->remmina_file, "ssh_tunnel_enabled", FALSE));
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->ssh_tunnel_loopback_check),
					     remmina_file_get_int(priv->remmina_file, "ssh_tunnel_loopback", FALSE));
		gtk_entry_set_text(GTK_ENTRY(priv->ssh_tunnel_server_entry),
				   cs ? cs : "");
	}


	remmina_public_create_group(GTK_GRID(grid), _("SSH Command"), row, 2, 1);
	row += 2;
	priv->ssh_tunnel_command_entry =
			remmina_file_editor_create_text(gfe, grid, row, 0,
							_("Startup command"), NULL, "ssh_tunnel_command");
	cs = remmina_file_get_string(priv->remmina_file, "ssh_tunnel_command");
	gtk_entry_set_text(GTK_ENTRY(priv->ssh_tunnel_command_entry),
				   cs ? cs : "");
	row++;

	widget = remmina_file_editor_create_sssh_import_export(gfe, grid, row, 0,
									       _("Export ssh tunnel settings"),
									       remmina_file_get_string(priv->remmina_file, "ssh_saved_profile"),
									       GTK_FILE_CHOOSER_ACTION_SAVE, "ssh_export_profile");
	row++;

	widget = remmina_file_editor_create_sssh_import_export(gfe, grid, row, 1,
									       _("Import ssh tunnel settings"),
									       remmina_file_get_string(priv->remmina_file, "ssh_saved_profile"),
									       GTK_FILE_CHOOSER_ACTION_OPEN, "ssh_import_profile");

	// row++;



	row++;




	remmina_file_editor_ssh_tunnel_enabled_check_on_toggled(NULL, gfe, ssh_setting);
	gtk_widget_show_all(grid);
	g_free(p);
#endif
}

static void remmina_file_editor_create_all_settings(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	GtkWidget *grid;

	static const RemminaProtocolSetting notes_settings[] =
	{
		{ REMMINA_PROTOCOL_SETTING_TYPE_TEXTAREA, "notes_text", NULL, FALSE, NULL, NULL },
		{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,		NULL, FALSE, NULL, NULL }
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

	/* The Behavior tab */
	remmina_file_editor_create_behavior_tab(gfe);

	/* The SSH tab */
	remmina_file_editor_create_ssh_tunnel_tab(gfe, priv->plugin->ssh_setting);

	/* Notes tab */
	grid = remmina_file_editor_create_notebook_tab(gfe, NULL, _("Notes"), 1, 1);
	remmina_file_editor_create_settings(gfe, grid, notes_settings);
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

	priv->ssh_tunnel_enabled_check = NULL;
	priv->ssh_tunnel_loopback_check = NULL;
	priv->ssh_tunnel_server_default_radio = NULL;
	priv->ssh_tunnel_server_custom_radio = NULL;
	priv->ssh_tunnel_server_entry = NULL;
	priv->ssh_tunnel_username_entry = NULL;
	priv->ssh_tunnel_command_entry = NULL;
	priv->ssh_tunnel_auth_combo = NULL;
	priv->ssh_tunnel_auth_password = NULL;
	priv->ssh_tunnel_privatekey_chooser = NULL;
	priv->ssh_tunnel_certfile_chooser = NULL;

	g_hash_table_remove_all(priv->setting_widgets);

	protocol = remmina_public_combo_get_active_text(combo);
	if (protocol) {
		priv->plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL,
											  protocol);
		g_free(protocol);
		remmina_file_editor_create_all_settings(gfe);
	}
}

static void remmina_file_editor_save_behavior_tab(RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;

	remmina_file_set_string(priv->remmina_file, "precommand", gtk_entry_get_text(GTK_ENTRY(priv->behavior_precommand_entry)));
	remmina_file_set_string(priv->remmina_file, "postcommand", gtk_entry_get_text(GTK_ENTRY(priv->behavior_postcommand_entry)));

	gboolean autostart_enabled = (priv->behavior_autostart_check ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->behavior_autostart_check)) : FALSE);
	remmina_file_set_int(priv->remmina_file, "enable-autostart", autostart_enabled);
	gboolean lock_enabled = (priv->behavior_lock_check ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->behavior_lock_check)) : FALSE);
	remmina_file_set_int(priv->remmina_file, "profile-lock", lock_enabled);
	gboolean disconect_prompt = (priv->behavior_disconnect ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->behavior_disconnect)) : FALSE);
	remmina_file_set_int(priv->remmina_file, "disconnect-prompt", disconect_prompt);
}


static gboolean remmina_file_editor_validate_settings(RemminaFileEditor *	gfe,
						      gchar *			setting_name_to_validate,
						      gconstpointer		value,
						      GError **			err)
{
	if (!setting_name_to_validate || !value || !gfe) {
		if (!setting_name_to_validate) {
			g_critical(_("(%s: %i): Can't validate setting '%s' since 'value' or 'gfe' "
				     "are NULL!"),
				   __func__, __LINE__, setting_name_to_validate);
		} else {
			g_critical(_("(%s: %i): Can't validate user input since "
				     "'setting_name_to_validate', 'value' or 'gfe' are NULL!"),
				   __func__, __LINE__);
		}
		g_set_error(err, 1, 1, _("Internal error."));
		return FALSE;
	}

	if (strcmp(setting_name_to_validate, "notes_text") == 0) {
		// Not a plugin setting. Bail out early.
		return TRUE;
	}

	const RemminaProtocolSetting *setting_iter;
	RemminaProtocolPlugin *protocol_plugin;
	RemminaFileEditorPriv *priv = gfe->priv;
	protocol_plugin = priv->plugin;

	setting_iter = protocol_plugin->basic_settings;
	if (setting_iter) {
		while (setting_iter->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
			if (setting_iter->name == NULL) {
				g_error("Internal error: a setting name in protocol plugin %s is "
					"null. Please fix RemminaProtocolSetting struct content.",
					protocol_plugin->name);
			} else if ((gchar *)setting_name_to_validate) {
				if (strcmp((gchar *)setting_name_to_validate, setting_iter->name) == 0) {

					gpointer validator_data = setting_iter->validator_data;
					GCallback validator = setting_iter->validator;

					// Default behaviour is that everything is valid,
					// except a validator is given and its returned GError is not NULL.
					GError *err_ret = NULL;

					g_debug("Checking setting '%s' for validation.", setting_iter->name);
					if (validator != NULL) {
						// Looks weird but it calls the setting's validator
						// function using setting_name_to_validate, value and
						// validator_data as parameters and it returns a GError*.
						err_ret = ((GError * (*)(gpointer, gconstpointer, gpointer)) validator)(setting_name_to_validate, value, validator_data);
					}

					if (err_ret) {
						g_debug("it has a validator function and it had an error!");
						// pass err (returned value) to function caller.
						*err = err_ret;
						return FALSE;
					}

					break;
				}
			}
			setting_iter++;
		}
	}

	return TRUE;
}

static GError *remmina_file_editor_update_settings(RemminaFileEditor *	gfe,
						   GtkWidget **		failed_widget)
{
	TRACE_CALL(__func__);
	RemminaFileEditorPriv *priv = gfe->priv;
	GHashTableIter iter;
	gpointer key;
	gpointer widget;
	GtkTextBuffer *buffer;
	gchar *escaped, *unescaped;
	GtkTextIter start, end;

	GError *err = NULL;
	*failed_widget = NULL;

	g_hash_table_iter_init(&iter, priv->setting_widgets);
	while (g_hash_table_iter_next(&iter, &key, &widget)) {
		
		// We don't want to save or validate grayed-out settings.
		// If widget is a file chooser, it was made not sensitive because
		// the box was unchecked. In that case, don't continue. The 
		// relevant file strings will be set to NULL in the remmina file.
		if (!gtk_widget_get_sensitive(GTK_WIDGET(widget)) && !GTK_IS_FILE_CHOOSER(widget)) {
			g_debug("Grayed-out setting-widget '%s' will not be saved.",
			gtk_widget_get_name(widget));
			continue;
		}

		if (GTK_IS_ENTRY(widget)) {
			const gchar *value = gtk_entry_get_text(GTK_ENTRY(widget));

			if (!remmina_file_editor_validate_settings(gfe, (gchar *)key, value, &err)) {
				// Error while validating!
				// err should be set now.
				*failed_widget = widget;
				break;
			}

			remmina_file_set_string(priv->remmina_file, (gchar *)key, value);
		} else if (GTK_IS_TEXT_VIEW(widget)) {
			buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
			gtk_text_buffer_get_start_iter(buffer, &start);
			gtk_text_buffer_get_end_iter(buffer, &end);
			unescaped = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
			escaped = g_uri_escape_string(unescaped, NULL, TRUE);

			if (!remmina_file_editor_validate_settings(gfe, (gchar *)key, escaped, &err)) {
				// Error while validating!
				// err should be set now.
				*failed_widget = widget;
				break;
			}

			remmina_file_set_string(priv->remmina_file, (gchar *)key, escaped);
			g_free(escaped);
		} else if (GTK_IS_COMBO_BOX(widget)) {
			gchar *value = remmina_public_combo_get_active_text(GTK_COMBO_BOX(widget));

			if (!remmina_file_editor_validate_settings(gfe, (gchar *)key, value, &err)) {
				// Error while validating!
				// err should be set now.
				*failed_widget = widget;
				break;
			}

			remmina_file_set_string(priv->remmina_file, (gchar *)key, value);
		} else if (GTK_IS_FILE_CHOOSER(widget)) {
			gchar *value = gtk_widget_get_sensitive(GTK_WIDGET(widget)) ? gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)) : NULL;

			if (!gtk_widget_get_sensitive(GTK_WIDGET(widget))) {
				remmina_file_set_string(priv->remmina_file, (gchar *)key, value);
				continue;
			}

			if (!remmina_file_editor_validate_settings(gfe, (gchar *)key, value, &err)) {
				// Error while validating!
				// err should be set now.
				g_free(value);
				*failed_widget = widget;
				break;
			}

			remmina_file_set_string(priv->remmina_file, (gchar *)key, value);
			g_free(value);
		} else if (GTK_IS_TOGGLE_BUTTON(widget)) {
			gboolean value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));

			if (!remmina_file_editor_validate_settings(gfe, (gchar *)key, &value, &err)) {
				// Error while validating!
				// err should be set now.
				*failed_widget = widget;
				break;
			}

			remmina_file_set_int(priv->remmina_file, (gchar *)key, value);
		}
	}

	if (err) {
		return err;
	}

	return NULL;
}

static GError *remmina_file_editor_update(RemminaFileEditor *	gfe,
					  GtkWidget **		failed_widget)
{
	TRACE_CALL(__func__);
	int res_w, res_h;
	gchar *custom_resolution;
	RemminaProtocolWidgetResolutionMode res_mode;

	RemminaFileEditorPriv *priv = gfe->priv;

	remmina_file_set_string(priv->remmina_file, "name", gtk_entry_get_text(GTK_ENTRY(priv->name_entry)));

	remmina_file_set_string(priv->remmina_file, "labels", gtk_entry_get_text(GTK_ENTRY(priv->labels_entry)));

	remmina_file_set_string(priv->remmina_file, "group",
				(priv->group_combo ? remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->group_combo)) : NULL));

	remmina_file_set_string(priv->remmina_file, "protocol",
				remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->protocol_combo)));

	remmina_file_set_string(priv->remmina_file, "server",
				(priv->server_combo ? remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->server_combo)) : NULL));

	if (priv->resolution_auto_radio) {
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->resolution_auto_radio))) {
			/* Resolution is set to auto (which means: Use client fullscreen resolution, aka use client resolution) */
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

	if (priv->assistance_toggle){
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(priv->assistance_toggle))) {
			remmina_file_set_string(priv->remmina_file, "assistance_file", gtk_entry_get_text(GTK_ENTRY(priv->assistance_file)));
			remmina_file_set_string(priv->remmina_file, "assistance_pass", gtk_entry_get_text(GTK_ENTRY(priv->assistance_password)));
			remmina_file_set_int(priv->remmina_file, "assistance_mode", 1);
		}else{
			remmina_file_set_int(priv->remmina_file, "assistance_mode", 0);
		}
		
	}

	if (priv->keymap_combo)
		remmina_file_set_string(priv->remmina_file, "keymap",
					remmina_public_combo_get_active_text(GTK_COMBO_BOX(priv->keymap_combo)));

	remmina_file_editor_save_behavior_tab(gfe);
	remmina_file_editor_save_ssh_tunnel_tab(gfe, NULL);
	return remmina_file_editor_update_settings(gfe, failed_widget);
}

static void remmina_file_editor_on_default(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFile *gf;
	GtkWidget *dialog;

	GtkWidget *failed_widget = NULL;
	GError *err = remmina_file_editor_update(gfe, &failed_widget);
	if (err) {
		g_warning(_("Couldn't validate user input. %s"), err->message);
		remmina_file_editor_show_validation_error_popup(gfe, failed_widget, err);
		return;
	}

	gf = remmina_file_dup(gfe->priv->remmina_file);

	remmina_file_set_filename(gf, remmina_pref_file);

	/* Clear properties that should never be default */
	remmina_file_set_string(gf, "name", NULL);
	remmina_file_set_string(gf, "server", NULL);
	remmina_file_set_string(gf, "password", NULL);
	remmina_file_set_string(gf, "precommand", NULL);
	remmina_file_set_string(gf, "postcommand", NULL);

	remmina_file_set_string(gf, "ssh_tunnel_server", NULL);
	remmina_file_set_string(gf, "ssh_tunnel_password", NULL);
	remmina_file_set_string(gf, "ssh_tunnel_passphrase", NULL);

	remmina_file_save(gf);
	remmina_file_free(gf);

	dialog = gtk_message_dialog_new(GTK_WINDOW(gfe), GTK_DIALOG_MODAL, GTK_MESSAGE_INFO,
					GTK_BUTTONS_OK, _("Default settings saved."));
	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void remmina_file_editor_on_save(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);

	GtkWidget *failed_widget = NULL;
	GError *err = remmina_file_editor_update(gfe, &failed_widget);
	if (err) {
		g_warning(_("Couldn't validate user input. %s"), err->message);
		remmina_file_editor_show_validation_error_popup(gfe, failed_widget, err);
		return;
	}

	remmina_file_editor_file_save(gfe);

	remmina_file_save(gfe->priv->remmina_file);
	remmina_icon_populate_menu();

	gtk_widget_destroy(GTK_WIDGET(gfe));
}

static void remmina_file_editor_on_connect(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	RemminaFile *gf;

	GtkWidget *failed_widget = NULL;
	GError *err = remmina_file_editor_update(gfe, &failed_widget);
	if (err) {
		g_warning(_("Couldn't validate user input. %s"), err->message);
		remmina_file_editor_show_validation_error_popup(gfe, failed_widget, err);
		return;
	}

	gf = remmina_file_dup(gfe->priv->remmina_file);
	/* Put server into name for "Quick Connect" */
	if (remmina_file_get_filename(gf) == NULL)
		remmina_file_set_string(gf, "name", remmina_file_get_string(gf, "server"));
	gtk_widget_destroy(GTK_WIDGET(gfe));
	gf->prevent_saving = TRUE;
	rcw_open_from_file(gf);
}

static void remmina_file_editor_on_save_connect(GtkWidget *button, RemminaFileEditor *gfe)
{
	TRACE_CALL(__func__);
	/** @TODO: Call remmina_file_editor_on_save */
	RemminaFile *gf;

	GtkWidget *failed_widget = NULL;
	GError *err = remmina_file_editor_update(gfe, &failed_widget);
	if (err) {
		g_warning(_("Couldn't validate user input. %s"), err->message);
		remmina_file_editor_show_validation_error_popup(gfe, failed_widget, err);
		return;
	}

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
	gtk_window_set_title(GTK_WINDOW(gfe), _("Remote Connection Profile"));

	widget = gtk_dialog_add_button(GTK_DIALOG(gfe), (_("_Cancel")), GTK_RESPONSE_CANCEL);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_file_editor_on_cancel), gfe);

	/* Default button */
	widget = gtk_dialog_add_button(GTK_DIALOG(gfe), (_("Save as Default")), GTK_RESPONSE_OK);
	gtk_widget_set_tooltip_text(GTK_WIDGET(widget), _("Use the current settings as the default for all new connection profiles"));
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
		/* TODO: Probably to be removed */
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

	/* Create the "Profile" group on the top (for name and protocol) */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(gfe))), grid, FALSE, FALSE, 2);

	gboolean profile_file_exists = (remmina_file_get_filename(remminafile) != NULL);

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

	if (!profile_file_exists) {
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

	/* Profile: Labels */
	widget = gtk_label_new(_("Labels"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 9, 2, 1);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 10);

	widget = gtk_entry_new();
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 9, 3, 1);
	gtk_entry_set_max_length(GTK_ENTRY(widget), 255);
	priv->labels_entry = widget;

	if (!profile_file_exists) {
		gtk_widget_set_tooltip_text(widget, _("Label1,Label2"));
#if GTK_CHECK_VERSION(3, 16, 0)
		gtk_entry_grab_focus_without_selecting(GTK_ENTRY(widget));
#endif
		g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(remmina_file_editor_entry_on_changed), gfe);
	} else {
		cs = remmina_file_get_string(remminafile, "labels");
		gtk_entry_set_text(GTK_ENTRY(widget), cs ? cs : "");
	}

	/* Profile: Protocol */
	widget = gtk_label_new(_("Protocol"));
	gtk_widget_show(widget);
	gtk_widget_set_valign(widget, GTK_ALIGN_START);
	gtk_widget_set_halign(widget, GTK_ALIGN_START);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 12, 2, 1);

	widget = remmina_public_create_combo(TRUE);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 12, 3, 1);
	priv->protocol_combo = widget;
	remmina_plugin_manager_for_each_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL, remmina_file_editor_iterate_protocol, gfe);
	g_signal_connect(G_OBJECT(widget), "changed", G_CALLBACK(remmina_file_editor_protocol_combo_on_changed), gfe);

	/* Create the "Preference" frame */
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
						_("Could not find the file “%s”."), filename);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}
}

GtkWidget *remmina_file_editor_new_from_filename(const gchar *filename)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;

	remminafile = remmina_file_manager_load_file(filename);
	if (remminafile) {
		if (remmina_file_get_int(remminafile, "profile-lock", FALSE) && remmina_unlock_new(remmina_main_get_window()) == 0)
			return NULL;
		return remmina_file_editor_new_from_file(remminafile);
	} else {
		GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
							   _("Could not find the file “%s”."), filename);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		return NULL;
	}
}
