/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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
#include "config.h"
#include "remmina_public.h"
#include "remmina_widget_pool.h"
#include "remmina_message_panel.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"


typedef struct
{

	RemminaMessagePanelCallback response_callback;
	void *response_callback_data;
	GtkWidget *w[REMMINA_MESSAGE_PANEL_MAXWIDGETID];

} RemminaMessagePanelPrivate;
G_DEFINE_TYPE_WITH_PRIVATE (RemminaMessagePanel, remmina_message_panel, GTK_TYPE_BOX)


enum {
  RESPONSE,
  LAST_SIGNAL
};

static guint messagepanel_signals[LAST_SIGNAL];

static const gchar btn_response_key[] = "btn_response";

static void remmina_message_panel_init (RemminaMessagePanel *mp)
{
	TRACE_CALL(__func__);
}

static void remmina_message_panel_class_init(RemminaMessagePanelClass *class)
{
	TRACE_CALL(__func__);
	// class->transform_text = my_app_label_real_transform_text;

	messagepanel_signals[RESPONSE] =
		g_signal_new ("response",
			G_OBJECT_CLASS_TYPE (class),
			G_SIGNAL_RUN_LAST,
			G_STRUCT_OFFSET (RemminaMessagePanelClass, response),
			NULL, NULL,
			NULL,
			G_TYPE_NONE, 1,
			G_TYPE_INT);
}

RemminaMessagePanel *remmina_message_panel_new()
{
	TRACE_CALL(__func__);
	RemminaMessagePanelPrivate *priv;
	RemminaMessagePanel* mp;
	mp = (RemminaMessagePanel*)g_object_new(REMMINA_TYPE_MESSAGE_PANEL,
		"orientation", GTK_ORIENTATION_VERTICAL,
		"spacing", 0,
		NULL);

	priv = remmina_message_panel_get_instance_private(mp);

	priv->response_callback = NULL;
	priv->response_callback_data = NULL;

	/* Set widget class, for CSS styling */
	// gtk_widget_set_name(GTK_WIDGET(mp), "remmina-cw-message-panel");
	gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(mp)), "message_panel");
	gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(mp)), "background");

	return mp;
}

static void remmina_message_panel_button_clicked_callback(
	GtkButton *button, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaMessagePanel *mp = (RemminaMessagePanel*)user_data;
	RemminaMessagePanelPrivate *priv = remmina_message_panel_get_instance_private(mp);

	gint btn_data;

	btn_data = (gint)((gint64)g_object_get_data(G_OBJECT(button), btn_response_key));

	/* Calls the callback, if defined */
	if (priv->response_callback != NULL)
		(*priv->response_callback)(priv->response_callback_data, btn_data);

}

void remmina_message_panel_setup_progress(RemminaMessagePanel *mp, const gchar *message, RemminaMessagePanelCallback response_callback, gpointer response_callback_data)
{
	/*
	 * Setup a message panel to show a spinner, a message like "Connecting…",
	 * and a button to cancel the action in progress
	 *
	 */

	TRACE_CALL(__func__);
	GtkBox *hbox;
	GtkWidget *w;
	RemminaMessagePanelPrivate *priv = remmina_message_panel_get_instance_private(mp);

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		printf("WARNING: %s called in a subthread. This should not happen.\n", __func__);
		raise(SIGINT);
	}

	hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

	/* A spinner */
	w = gtk_spinner_new();
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 0);
	gtk_spinner_start(GTK_SPINNER(w));

	/* A message */
	w = gtk_label_new(message);
	gtk_box_pack_start(hbox, w, TRUE, TRUE, 0);

	priv->response_callback = response_callback;
	priv->response_callback_data = response_callback_data;

	/* A button to cancel the action. The cancel button is available
	 * only when a response_callback function is defined. */
	if (response_callback) {
		w = gtk_button_new_with_label(_("Cancel"));
		gtk_box_pack_end(hbox, w, FALSE, FALSE, 0);
		g_object_set_data(G_OBJECT(w), btn_response_key, (void *)GTK_RESPONSE_CANCEL);
		g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);
	}

	gtk_box_pack_start(GTK_BOX(mp), GTK_WIDGET(hbox), TRUE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(mp));

}

void remmina_message_panel_setup_message(RemminaMessagePanel *mp, const gchar *message, RemminaMessagePanelCallback response_callback, gpointer response_callback_data)
{
	/*
	 * Setup a message panel to a message to read like "Cannot connect…",
	 * and a button to close the panel
	 *
	 */

	TRACE_CALL(__func__);
	GtkBox *hbox;
	GtkWidget *w;
	RemminaMessagePanelPrivate *priv = remmina_message_panel_get_instance_private(mp);

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		printf("WARNING: %s called in a subthread. This should not happen.\n", __func__);
	}

	hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

	/* A message */
	w = gtk_label_new(message);
	gtk_box_pack_start(hbox, w, TRUE, TRUE, 0);

	/* A button to confirm reading */
	w = gtk_button_new_with_label(_("Close"));
	gtk_box_pack_end(hbox, w, FALSE, FALSE, 0);

	priv->response_callback = response_callback;
	priv->response_callback_data = response_callback_data;

	g_object_set_data(G_OBJECT(w), btn_response_key, (void *)GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);

	gtk_box_pack_start(GTK_BOX(mp), GTK_WIDGET(hbox), TRUE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(mp));

}

void remmina_message_panel_setup_question(RemminaMessagePanel *mp, const gchar *message, RemminaMessagePanelCallback response_callback, gpointer response_callback_data)
{
	/*
	 * Setup a message panel to a message to read like "Do you accept ?",
	 * and a pair of button for Yes and No
	 * message is an HTML string
	 * Callback will receive GTK_RESPONSE_NO for No, GTK_RESPONSE_YES for Yes
	 *
	 */

	TRACE_CALL(__func__);
	GtkWidget *grid;
	GtkWidget *bbox;
	GtkWidget *w;
	RemminaMessagePanelPrivate *priv = remmina_message_panel_get_instance_private(mp);

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		printf("WARNING: %s called in a subthread. This should not happen. Raising SIGINT for debugging.\n", __func__);
		raise(SIGINT);
	}

	/* Create grid */
	grid = gtk_grid_new();
	gtk_widget_set_halign(GTK_WIDGET(grid), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(grid), GTK_ALIGN_CENTER);
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

	/* A message, in HTML format */
	w = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(w), message);

	gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(w), 18);
	gtk_widget_set_margin_bottom (GTK_WIDGET(w), 9);
	gtk_widget_set_margin_start (GTK_WIDGET(w), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(w), 18);
	gtk_widget_show(w);
	gtk_grid_attach(GTK_GRID(grid), w, 0, 0, 2, 1);

	/* A button for yes and one for no */
	bbox = gtk_button_box_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(bbox), GTK_BUTTONBOX_START);
	gtk_grid_attach(GTK_GRID(grid), bbox, 0, 1, 1, 1);
	w = gtk_button_new_with_label(_("Yes"));
	gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_CENTER);
	g_object_set_data(G_OBJECT(w), btn_response_key, (void *)GTK_RESPONSE_YES);

	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);
	gtk_container_add(GTK_CONTAINER(bbox), w);

	w = gtk_button_new_with_label(_("No"));
	gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_CENTER);
	g_object_set_data(G_OBJECT(w), btn_response_key, (void *)GTK_RESPONSE_NO);

	priv->response_callback = response_callback;
	priv->response_callback_data = response_callback_data;

	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);
	gtk_container_add(GTK_CONTAINER(bbox), w);

	gtk_box_pack_start(GTK_BOX(mp), GTK_WIDGET(grid), TRUE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(mp));

}

void remmina_message_panel_setup_auth(RemminaMessagePanel *mp, RemminaMessagePanelCallback response_callback, gpointer response_callback_data, const gchar *title, const gchar *password_prompt, unsigned flags)
{
	TRACE_CALL(__func__);
	GtkWidget *grid;
	GtkWidget *password_entry;
	GtkWidget *username_entry;
	GtkWidget *domain_entry;
	GtkWidget *save_password_switch;
	GtkWidget *widget;
	GtkWidget *bbox;
	GtkWidget *button_ok;
	GtkWidget *button_cancel;
	int grid_row;

	RemminaMessagePanelPrivate *priv = remmina_message_panel_get_instance_private(mp);

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		printf("WARNING: %s called in a subthread. This should not happen. Raising SIGINT to debug.\n", __func__);
		raise(SIGINT);
	}

	/* Create grid */
	grid = gtk_grid_new();
	gtk_widget_set_halign(GTK_WIDGET(grid), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(grid), GTK_ALIGN_CENTER);
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);
	//gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

	/* Entries */

	grid_row = 0;
	widget = gtk_label_new(title);
	gtk_style_context_add_class(gtk_widget_get_style_context(widget), "title_label");
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 9);
	gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(widget), 18);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 3, 1);
	grid_row++;


	if (flags & REMMINA_MESSAGE_PANEL_FLAG_USERNAME) {
		widget = gtk_label_new(_("Username"));
		gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
		gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
		gtk_widget_set_margin_top (GTK_WIDGET(widget), 9);
		gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 3);
		gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
		gtk_widget_set_margin_end (GTK_WIDGET(widget), 6);
		gtk_widget_show(widget);
		gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);

		username_entry = gtk_entry_new();
		// gtk_style_context_add_class(gtk_widget_get_style_context(username_entry), "panel_entry");
		gtk_widget_set_halign(GTK_WIDGET(username_entry), GTK_ALIGN_FILL);
		gtk_widget_set_valign(GTK_WIDGET(username_entry), GTK_ALIGN_FILL);
		gtk_widget_set_margin_top (GTK_WIDGET(username_entry), 9);
		gtk_widget_set_margin_bottom (GTK_WIDGET(username_entry), 3);
		gtk_widget_set_margin_start (GTK_WIDGET(username_entry), 6);
		gtk_widget_set_margin_end (GTK_WIDGET(username_entry), 18);
		//gtk_entry_set_activates_default (GTK_ENTRY(username_entry), TRUE);
		gtk_grid_attach(GTK_GRID(grid), username_entry, 1, grid_row, 2, 1);
		gtk_entry_set_max_length(GTK_ENTRY(username_entry), 100);

		if (flags & REMMINA_MESSAGE_PANEL_FLAG_USERNAME_READONLY) {
			g_object_set(username_entry, "editable", FALSE, NULL);
		}

		/*
		if (default_username && default_username[0] != '\0') {
			gtk_entry_set_text(GTK_ENTRY(username_entry), default_username);
		}
		*/
		grid_row++;
	} else {
		username_entry = NULL;
	}

	/* The password/key field */
	widget = gtk_label_new(password_prompt);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_margin_top (GTK_WIDGET(widget), 3);
	gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(widget), 6);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);

	password_entry = gtk_entry_new();
	gtk_widget_set_halign(GTK_WIDGET(password_entry), GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(password_entry), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(password_entry), 3);
	gtk_widget_set_margin_bottom (GTK_WIDGET(password_entry), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(password_entry), 6);
	gtk_widget_set_margin_end (GTK_WIDGET(password_entry), 18);
	gtk_entry_set_activates_default (GTK_ENTRY(password_entry), TRUE);
	gtk_grid_attach(GTK_GRID(grid), password_entry, 1, grid_row, 2, 1);
	gtk_entry_set_max_length(GTK_ENTRY(password_entry), 100);
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);

	grid_row++;

	if (flags & REMMINA_MESSAGE_PANEL_FLAG_DOMAIN) {
		widget = gtk_label_new(_("Domain"));
		gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
		gtk_widget_set_margin_top (GTK_WIDGET(widget), 3);
		gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 3);
		gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
		gtk_widget_set_margin_end (GTK_WIDGET(widget), 6);
		gtk_widget_show(widget);
		gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);

		domain_entry = gtk_entry_new();
		gtk_widget_set_halign(GTK_WIDGET(domain_entry), GTK_ALIGN_FILL);
		gtk_widget_set_valign(GTK_WIDGET(domain_entry), GTK_ALIGN_FILL);
		gtk_widget_set_margin_top (GTK_WIDGET(domain_entry), 3);
		gtk_widget_set_margin_bottom (GTK_WIDGET(domain_entry), 3);
		gtk_widget_set_margin_start (GTK_WIDGET(domain_entry), 6);
		gtk_widget_set_margin_end (GTK_WIDGET(domain_entry), 18);
		gtk_widget_show(domain_entry);
		gtk_grid_attach(GTK_GRID(grid), domain_entry, 1, grid_row, 2, 1);
		gtk_entry_set_max_length(GTK_ENTRY(domain_entry), 100);
		/* if (default_domain && default_domain[0] != '\0') {
			gtk_entry_set_text(GTK_ENTRY(domain_entry), default_domain);
		} */
		grid_row ++;
	} else {
		domain_entry = NULL;
	}


	widget = gtk_label_new(_("Save password"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_margin_top (GTK_WIDGET(widget), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 9);
	gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(widget), 6);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);
	save_password_switch = gtk_switch_new();
	gtk_widget_set_halign(GTK_WIDGET(save_password_switch), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(save_password_switch), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(save_password_switch), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(save_password_switch), 9);
	gtk_widget_set_margin_start (GTK_WIDGET(save_password_switch), 6);
	gtk_widget_set_margin_end (GTK_WIDGET(save_password_switch), 18);
	gtk_grid_attach(GTK_GRID(grid), save_password_switch, 1, grid_row, 2, 1);
	if (flags & REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD) {
		gtk_switch_set_active(GTK_SWITCH(save_password_switch), TRUE);
	}else  {
		gtk_switch_set_active(GTK_SWITCH(save_password_switch), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(save_password_switch), FALSE);
	}
	grid_row ++;

	/* Buttons, ok and cancel */
	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_EDGE);
	gtk_box_set_spacing (GTK_BOX (bbox), 40);
	gtk_widget_set_margin_top (GTK_WIDGET(bbox), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(bbox), 18);
	gtk_widget_set_margin_start (GTK_WIDGET(bbox), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(bbox), 18);
	button_ok = gtk_button_new_with_label(_("_OK"));
	gtk_button_set_use_underline(GTK_BUTTON(button_ok), TRUE);
	gtk_widget_set_can_default(button_ok, TRUE);
	gtk_container_add (GTK_CONTAINER (bbox), button_ok);
	/* Buttons, ok and cancel */
	button_cancel = gtk_button_new_with_label(_("_Cancel"));
	gtk_button_set_use_underline(GTK_BUTTON(button_cancel), TRUE);
	gtk_container_add (GTK_CONTAINER (bbox), button_cancel);
	gtk_grid_attach(GTK_GRID(grid), bbox, 0, grid_row, 3, 1);
	/* Pack it into the panel */
	gtk_box_pack_start(GTK_BOX(mp), grid, TRUE, TRUE, 4);

	priv->w[REMMINA_MESSAGE_PANEL_USERNAME] = username_entry;
	priv->w[REMMINA_MESSAGE_PANEL_PASSWORD] = password_entry;
	priv->w[REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD] = save_password_switch;
	priv->w[REMMINA_MESSAGE_PANEL_DOMAIN] = domain_entry;
	priv->w[REMMINA_MESSAGE_PANEL_BUTTONTOFOCUS] = button_ok;

	priv->response_callback = response_callback;
	priv->response_callback_data = response_callback_data;

	if (username_entry) g_signal_connect_swapped (username_entry, "activate", (GCallback)gtk_widget_grab_focus, password_entry);
	g_signal_connect_swapped (password_entry, "activate", (GCallback)gtk_widget_grab_focus, button_ok);
	g_object_set_data(G_OBJECT(button_cancel), btn_response_key, (void *)GTK_RESPONSE_CANCEL);
	g_signal_connect(G_OBJECT(button_cancel), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);
	g_object_set_data(G_OBJECT(button_ok), btn_response_key, (void *)GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(button_ok), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);
}

void remmina_message_panel_setup_auth_x509(RemminaMessagePanel *mp, RemminaMessagePanelCallback response_callback, gpointer response_callback_data)
{
	TRACE_CALL(__func__);

	GtkWidget *grid;
	GtkWidget *widget;
	GtkWidget *bbox;
	GtkWidget *button_ok;
	GtkWidget *button_cancel;
	GtkWidget *cacert_file;
	GtkWidget *cacrl_file;
	GtkWidget *clientcert_file;
	GtkWidget *clientkey_file;
	int grid_row;

	RemminaMessagePanelPrivate *priv = remmina_message_panel_get_instance_private(mp);

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		printf("WARNING: %s called in a subthread. This should not happen. Raising SIGINT to debug.\n", __func__);
		raise(SIGINT);
	}

	/* Create grid */
	grid = gtk_grid_new();
	gtk_widget_set_halign(GTK_WIDGET(grid), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(grid), GTK_ALIGN_CENTER);
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 6);

	/* Entries */
	grid_row = 0;
	widget = gtk_label_new(_("Enter certificate authentication files"));
	gtk_style_context_add_class(gtk_widget_get_style_context(widget), "title_label");
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 9);
	gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(widget), 18);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 3, 1);
	grid_row++;

	const gchar *lbl_cacert = _("CA Certificate File");
	widget = gtk_label_new(lbl_cacert);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top (GTK_WIDGET(widget), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(widget), 6);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);
	cacert_file = gtk_file_chooser_button_new(lbl_cacert, GTK_FILE_CHOOSER_ACTION_OPEN);
	// gtk_style_context_add_class(gtk_widget_get_style_context(username_entry), "panel_entry");
	gtk_widget_show(cacert_file);
	gtk_widget_set_halign(GTK_WIDGET(cacert_file), GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(cacert_file), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(cacert_file), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(cacert_file), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(cacert_file), 6);
	gtk_widget_set_margin_end (GTK_WIDGET(cacert_file), 18);
	gtk_grid_attach(GTK_GRID(grid), cacert_file, 1, grid_row, 2, 1);
	grid_row++;

	const gchar *lbl_cacrl = _("CA CRL File");
	widget = gtk_label_new(lbl_cacrl);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top (GTK_WIDGET(widget), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(widget), 6);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);
	cacrl_file = gtk_file_chooser_button_new(lbl_cacrl, GTK_FILE_CHOOSER_ACTION_OPEN);
	// gtk_style_context_add_class(gtk_widget_get_style_context(username_entry), "panel_entry");
	gtk_widget_show(cacrl_file);
	gtk_widget_set_halign(GTK_WIDGET(cacrl_file), GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(cacrl_file), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(cacrl_file), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(cacrl_file), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(cacrl_file), 6);
	gtk_widget_set_margin_end (GTK_WIDGET(cacrl_file), 18);
	gtk_grid_attach(GTK_GRID(grid), cacrl_file, 1, grid_row, 2, 1);
	grid_row++;

	const gchar *lbl_clicert = _("Client Certificate File");
	widget = gtk_label_new(lbl_clicert);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top (GTK_WIDGET(widget), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(widget), 6);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);
	clientcert_file = gtk_file_chooser_button_new(lbl_clicert, GTK_FILE_CHOOSER_ACTION_OPEN);
	// gtk_style_context_add_class(gtk_widget_get_style_context(username_entry), "panel_entry");
	gtk_widget_show(clientcert_file);
	gtk_widget_set_halign(GTK_WIDGET(clientcert_file), GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(clientcert_file), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(clientcert_file), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(clientcert_file), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(clientcert_file), 6);
	gtk_widget_set_margin_end (GTK_WIDGET(clientcert_file), 18);
	gtk_grid_attach(GTK_GRID(grid), clientcert_file, 1, grid_row, 2, 1);
	grid_row++;

	const gchar *lbl_clikey = _("Client Certificate Key");
	widget = gtk_label_new(lbl_clikey);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_set_margin_top (GTK_WIDGET(widget), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(widget), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(widget), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(widget), 6);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);
	clientkey_file = gtk_file_chooser_button_new(lbl_clikey, GTK_FILE_CHOOSER_ACTION_OPEN);
	// gtk_style_context_add_class(gtk_widget_get_style_context(username_entry), "panel_entry");
	gtk_widget_show(clientkey_file);
	gtk_widget_set_halign(GTK_WIDGET(clientkey_file), GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(clientkey_file), GTK_ALIGN_FILL);
	gtk_widget_set_margin_top (GTK_WIDGET(clientkey_file), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(clientkey_file), 3);
	gtk_widget_set_margin_start (GTK_WIDGET(clientkey_file), 6);
	gtk_widget_set_margin_end (GTK_WIDGET(clientkey_file), 18);
	gtk_grid_attach(GTK_GRID(grid), clientkey_file, 1, grid_row, 2, 1);
	grid_row++;

	/* Buttons, ok and cancel */
	bbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_button_box_set_layout (GTK_BUTTON_BOX (bbox), GTK_BUTTONBOX_EDGE);
	gtk_box_set_spacing (GTK_BOX (bbox), 40);
	gtk_widget_set_margin_top (GTK_WIDGET(bbox), 9);
	gtk_widget_set_margin_bottom (GTK_WIDGET(bbox), 18);
	gtk_widget_set_margin_start (GTK_WIDGET(bbox), 18);
	gtk_widget_set_margin_end (GTK_WIDGET(bbox), 18);
	button_ok = gtk_button_new_with_label(_("_OK"));
	gtk_widget_set_can_default (button_ok, TRUE);

	gtk_button_set_use_underline(GTK_BUTTON(button_ok), TRUE);
	//gtk_widget_show(button_ok);
	gtk_container_add (GTK_CONTAINER (bbox), button_ok);
	//gtk_grid_attach(GTK_GRID(grid), button_ok, 0, grid_row, 1, 1);
	/* Buttons, ok and cancel */
	button_cancel = gtk_button_new_with_label(_("_Cancel"));
	gtk_button_set_use_underline(GTK_BUTTON(button_cancel), TRUE);
	//gtk_widget_show(button_cancel);
	gtk_container_add (GTK_CONTAINER (bbox), button_cancel);
	gtk_grid_attach(GTK_GRID(grid), bbox, 0, grid_row, 3, 1);
	/* Pack it into the panel */
	gtk_box_pack_start(GTK_BOX(mp), grid, TRUE, TRUE, 4);

	priv->response_callback = response_callback;
	priv->response_callback_data = response_callback_data;

	priv->w[REMMINA_MESSAGE_PANEL_CACERTFILE] = cacert_file;
	priv->w[REMMINA_MESSAGE_PANEL_CACRLFILE] = cacrl_file;
	priv->w[REMMINA_MESSAGE_PANEL_CLIENTCERTFILE] = clientcert_file;
	priv->w[REMMINA_MESSAGE_PANEL_CLIENTKEYFILE] = clientkey_file;
	priv->w[REMMINA_MESSAGE_PANEL_BUTTONTOFOCUS] = button_ok;

	g_object_set_data(G_OBJECT(button_cancel), btn_response_key, (void *)GTK_RESPONSE_CANCEL);
	g_signal_connect(G_OBJECT(button_cancel), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);
	g_object_set_data(G_OBJECT(button_ok), btn_response_key, (void *)GTK_RESPONSE_OK);
	g_signal_connect(G_OBJECT(button_ok), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);

}

void remmina_message_panel_focus_auth_entry(RemminaMessagePanel *mp)
{
	TRACE_CALL(__func__);

	RemminaMessagePanelPrivate *priv;
	GtkWidget *w;
	const gchar *username;

	if (mp == NULL)
		return;
	priv = remmina_message_panel_get_instance_private(mp);

	/* Activate default button */
	w = priv->w[REMMINA_MESSAGE_PANEL_BUTTONTOFOCUS];
	if (w && G_TYPE_CHECK_INSTANCE_TYPE(w, gtk_button_get_type()))
		gtk_widget_grab_default(w);

	w = priv->w[REMMINA_MESSAGE_PANEL_USERNAME];
	if (w == NULL)
	{
		w = priv->w[REMMINA_MESSAGE_PANEL_PASSWORD];
	}else {
		username = gtk_entry_get_text(GTK_ENTRY(w));
		if (username[0] != 0)
			w = priv->w[REMMINA_MESSAGE_PANEL_PASSWORD];
	}
	if (w == NULL)
		return;

	if (!G_TYPE_CHECK_INSTANCE_TYPE(w, gtk_entry_get_type()))
		return;

	gtk_widget_grab_focus(w);
}

void remmina_message_panel_field_set_string(RemminaMessagePanel *mp, int entryid, const gchar *text)
{
	RemminaMessagePanelPrivate *priv;

	if (mp == NULL)
		return;
	priv = remmina_message_panel_get_instance_private(mp);

	if (priv->w[entryid] == NULL)
		return;
	if (!G_TYPE_CHECK_INSTANCE_TYPE(priv->w[entryid], gtk_entry_get_type()))
		return;

	gtk_entry_set_text(GTK_ENTRY(priv->w[entryid]), text != NULL ? text : "");
}

gchar* remmina_message_panel_field_get_string(RemminaMessagePanel *mp, int entryid)
{
	TRACE_CALL(__func__);

	RemminaMessagePanelPrivate *priv;

	if (mp == NULL)
		return NULL;
	priv = remmina_message_panel_get_instance_private(mp);

	if (priv->w[entryid] == NULL)
		return NULL;
	if (!G_TYPE_CHECK_INSTANCE_TYPE(priv->w[entryid], gtk_entry_get_type()))
		return NULL;

	return g_strdup(gtk_entry_get_text(GTK_ENTRY(priv->w[entryid])));
}

void remmina_message_panel_field_set_switch(RemminaMessagePanel *mp, int entryid, gboolean state)
{
	TRACE_CALL(__func__);

	RemminaMessagePanelPrivate *priv;

	if (mp == NULL)
		return;
	priv = remmina_message_panel_get_instance_private(mp);

	if (priv->w[entryid] == NULL)
		return;
	if (!G_TYPE_CHECK_INSTANCE_TYPE(priv->w[entryid], gtk_switch_get_type()))
		return;

	gtk_switch_set_state(GTK_SWITCH(priv->w[entryid]), state);
}

gboolean remmina_message_panel_field_get_switch_state(RemminaMessagePanel *mp, int entryid)
{
	TRACE_CALL(__func__);

	RemminaMessagePanelPrivate *priv;

	if (mp == NULL)
		return FALSE;
	priv = remmina_message_panel_get_instance_private(mp);

	if (priv->w[entryid] == NULL)
		return FALSE;
	if (!G_TYPE_CHECK_INSTANCE_TYPE(priv->w[entryid], gtk_switch_get_type()))
		return FALSE;

	return gtk_switch_get_state(GTK_SWITCH(priv->w[entryid]));
}


void remmina_message_panel_field_set_filename(RemminaMessagePanel *mp, int entryid, const gchar *filename)
{
	TRACE_CALL(__func__);

	RemminaMessagePanelPrivate *priv;

	if (mp == NULL)
		return;
	priv = remmina_message_panel_get_instance_private(mp);
	if (priv->w[entryid] == NULL)
		return;
	if (!G_TYPE_CHECK_INSTANCE_TYPE(priv->w[entryid], gtk_file_chooser_button_get_type()))
		return;

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(priv->w[entryid]), filename);
}

gchar* remmina_message_panel_field_get_filename(RemminaMessagePanel *mp, int entryid)
{
	TRACE_CALL(__func__);

	RemminaMessagePanelPrivate *priv;

	if (mp == NULL)
		return NULL;
	priv = remmina_message_panel_get_instance_private(mp);

	if (priv->w[entryid] == NULL)
		return NULL;
	if (!G_TYPE_CHECK_INSTANCE_TYPE(priv->w[entryid], gtk_file_chooser_button_get_type()))
		return NULL;

	return gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(priv->w[entryid]));
}

void remmina_message_panel_response(RemminaMessagePanel *mp, gint response_id)
{
	g_signal_emit(mp, messagepanel_signals[RESPONSE], 0, response_id);
}

