/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2018 Antenore Gatta, Giovanni Panozzo
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
	/* mutex to have thread locked on this subpanel waiting for an answer */
	pthread_mutex_t pt_mutex;
	pthread_cond_t pt_cond;
	int pt_complete, pt_retcode;
} RemminaMessagePanelPrivate;
G_DEFINE_TYPE_WITH_PRIVATE (RemminaMessagePanel, remmina_message_panel, GTK_TYPE_BOX)

static const gchar btn_response_key[] = "btn_response";

static void remmina_message_panel_init (RemminaMessagePanel *mp)
{
	TRACE_CALL(__func__);
}

static void remmina_message_panel_class_init(RemminaMessagePanelClass *class)
{
	TRACE_CALL(__func__);
	// class->transform_text = my_app_label_real_transform_text;
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

	pthread_mutex_init(&priv->pt_mutex, NULL);
	pthread_cond_init(&priv->pt_cond, NULL);

	/* Set widget class, for CSS styling */
	// gtk_widget_set_name(GTK_WIDGET(mp), "remmina-cw-message-panel");
	gtk_style_context_add_class(gtk_widget_get_style_context(GTK_WIDGET(mp)), "message_panel");

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

	pthread_mutex_lock(&priv->pt_mutex);
	priv->pt_complete = TRUE;
	priv->pt_retcode = btn_data;
	pthread_cond_signal(&priv->pt_cond);
	pthread_mutex_unlock(&priv->pt_mutex);

}

void remmina_message_panel_setup_progress(RemminaMessagePanel *mp, gchar *message)
{
	/*
	 * Setup a message panel to show a spinner, a message like "Connecting...",
	 * and a button to cancel the action in progress
	 *
	 */

	TRACE_CALL(__func__);
	GtkBox *hbox;
	GtkWidget *w;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		printf("WARNING: %s called in a subthread. This should not happen.\n", __func__);
	}

	hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

	/* A spinner */
	w = gtk_spinner_new();
	gtk_box_pack_start(hbox, w, FALSE, FALSE, 0);
	gtk_spinner_start(GTK_SPINNER(w));

	/* A message */
	w = gtk_label_new(message);
	gtk_box_pack_start(hbox, w, TRUE, TRUE, 0);

	/* A button to cancel the action */
	w = gtk_button_new_with_label(_("Cancel"));
	gtk_box_pack_end(hbox, w, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(w), btn_response_key, 0);

	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);

	gtk_box_pack_start(GTK_BOX(mp), GTK_WIDGET(hbox), TRUE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(mp));

}

void remmina_message_panel_setup_message(RemminaMessagePanel *mp, gchar *message)
{
	/*
	 * Setup a message panel to a message to read like "Cannot connect...",
	 * and a button to close the panel
	 *
	 */

	TRACE_CALL(__func__);
	GtkBox *hbox;
	GtkWidget *w;

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
	g_object_set_data(G_OBJECT(w), btn_response_key, 0);

	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);

	gtk_box_pack_start(GTK_BOX(mp), GTK_WIDGET(hbox), TRUE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(mp));

}

void remmina_message_panel_setup_question(RemminaMessagePanel *mp, gchar *message)
{
	/*
	 * Setup a message panel to a message to read like "Do you accept ?",
	 * and a pair of button for Yes and No
	 * Callback will receive 0 for No, 1 for Yes
	 *
	 */

	TRACE_CALL(__func__);
	GtkBox *hbox;
	GtkWidget *w;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		printf("WARNING: %s called in a subthread. This should not happen.\n", __func__);
	}

	hbox = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));

	/* A message */
	w = gtk_label_new(message);
	gtk_box_pack_start(hbox, w, TRUE, TRUE, 0);

	/* A button for yes and one for no */
	w = gtk_button_new_with_label(_("Yes"));
	gtk_box_pack_end(hbox, w, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(w), btn_response_key, (gpointer)1);

	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);

	w = gtk_button_new_with_label(_("No"));
	gtk_box_pack_end(hbox, w, FALSE, FALSE, 0);
	g_object_set_data(G_OBJECT(w), btn_response_key, (gpointer)0);

	g_signal_connect(G_OBJECT(w), "clicked", G_CALLBACK(remmina_message_panel_button_clicked_callback), mp);

	gtk_box_pack_start(GTK_BOX(mp), GTK_WIDGET(hbox), TRUE, TRUE, 0);

	gtk_widget_show_all(GTK_WIDGET(mp));

}

void remmina_message_panel_setup_auth(RemminaMessagePanel *mp, gchar *message, unsigned flags)
{
	TRACE_CALL(__func__);
	GtkWidget *grid;
	GtkWidget *password_entry;
	GtkWidget *username_entry;
	GtkWidget *domain_entry;
	GtkWidget *save_password_check;
	GtkWidget *widget;
	GtkWidget *button_ok;
	GtkWidget *button_cancel;
	int grid_row;

	/* ToDo: remove this debug call */
	printf("%s, flags = %u\n", __func__ , flags);

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_MESSAGE_PANEL_SETUP_AUTH;
		d->p.message_panel_setup_auth.mp = mp;
		d->p.message_panel_setup_auth.message = message;
		d->p.message_panel_setup_auth.flags = flags;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return ;
	}

	/* Create grid */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

	/* Entries */

	grid_row = 0;
	widget = gtk_label_new(message);
	gtk_style_context_add_class(gtk_widget_get_style_context(widget), "title_label");
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 2, 1);
	grid_row++;


	if (flags & REMMINA_MESSAGE_PANEL_FLAG_USERNAME) {
		widget = gtk_label_new(_("User name"));
		gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
		gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
		gtk_widget_show(widget);
		gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);

		username_entry = gtk_entry_new();
		// gtk_style_context_add_class(gtk_widget_get_style_context(username_entry), "panel_entry");
		gtk_widget_show(username_entry);
		gtk_grid_attach(GTK_GRID(grid), username_entry, 1, grid_row, 1, 1);
		gtk_entry_set_max_length(GTK_ENTRY(username_entry), 100);

		/*
		if (default_username && default_username[0] != '\0') {
			gtk_entry_set_text(GTK_ENTRY(username_entry), default_username);
		}
		*/
		grid_row++;
	} else {
		username_entry = NULL;
	}

	/* ToDo: this "Password" can be "KEY" sometimes, we should introduce a flag */
	widget = gtk_label_new(_("Password"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);

	password_entry = gtk_entry_new();
	gtk_widget_show(password_entry);
	gtk_grid_attach(GTK_GRID(grid), password_entry, 1, grid_row, 1, 1);
	gtk_entry_set_max_length(GTK_ENTRY(password_entry), 100);
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(password_entry), TRUE);


	grid_row++;


	if (flags & REMMINA_MESSAGE_PANEL_FLAG_DOMAIN) {
		widget = gtk_label_new(_("Domain"));
		gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
		gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
		gtk_widget_show(widget);
		gtk_grid_attach(GTK_GRID(grid), widget, 0, grid_row, 1, 1);

		domain_entry = gtk_entry_new();
		gtk_widget_show(domain_entry);
		gtk_grid_attach(GTK_GRID(grid), domain_entry, 1, grid_row, 1, 1);
		gtk_entry_set_max_length(GTK_ENTRY(domain_entry), 100);
		gtk_entry_set_activates_default(GTK_ENTRY(domain_entry), TRUE);
		/* if (default_domain && default_domain[0] != '\0') {
			gtk_entry_set_text(GTK_ENTRY(domain_entry), default_domain);
		} */
		grid_row ++;
	} else {
		domain_entry = NULL;
	}


	save_password_check = gtk_check_button_new_with_label(_("Save password"));
	if (flags & REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSRORD) {
		gtk_widget_show(save_password_check);
		gtk_grid_attach(GTK_GRID(grid), save_password_check, 1, grid_row, 1, 1);
		/*
		if (dialog->save_password)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_password_check), TRUE);
		*/
	}else  {
		gtk_widget_set_sensitive(save_password_check, FALSE);
	}
	grid_row ++;

	/* Buttons, ok and cancel */
	button_ok = gtk_button_new_with_label(_("_OK"));
	gtk_button_set_use_underline(button_ok, TRUE);
	gtk_widget_show(button_ok);
	gtk_grid_attach(GTK_GRID(grid), button_ok, 2, 0, 1, 1);
	/* Buttons, ok and cancel */
	button_cancel = gtk_button_new_with_label(_("_Cancel"));
	gtk_button_set_use_underline(button_cancel, TRUE);
	gtk_widget_show(button_cancel);
	gtk_grid_attach(GTK_GRID(grid), button_cancel, 2, 1, 1, 1);
	/* Pack it into the panel */
	gtk_box_pack_start(GTK_BOX(mp), grid, TRUE, TRUE, 4);

	// gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	if (username_entry) {
		gtk_widget_grab_focus(username_entry);
	}else  {
		gtk_widget_grab_focus(password_entry);
	}

}

void remmina_message_panel_setup_cert(RemminaMessagePanel *mp, const gchar* subject, const gchar* issuer, const gchar* new_fingerprint, const gchar* old_fingerprint)
{
	/* Used both to show a new or a changed certificate.
	 * - If old_fingerprint is null, we are showing a new certificate
	 * * - If old_fingerprint is not null, we are showing a changed */

	/* See below remmina_init_dialog_certificate and remmina_init_dialog_certificate_changed */
}

void remmina_message_panel_setup_auth_x509(RemminaMessagePanel *mp, const gchar *cacert, const gchar *cacrl, const gchar *clientcert, const gchar *clientkey)
{
	/* Used for VNC x509 authentication. */

	/* See below remmina_init_dialog_certificate and remmina_init_dialog_certificate_changed */
}


int remmina_message_panel_wait_user_answer(RemminaMessagePanel *mp)
{
	/* Wait for the user to click on a button of the message panel.
	 * "Wait" is different when executed...
	 * - on the master thread: ???
	 * - on a subthread: block the subthread until the user presses
	 *   the button or mp is destroyed
	 */
	TRACE_CALL(__func__);
	RemminaMessagePanelPrivate *priv = remmina_message_panel_get_instance_private(mp);

	printf("Entering wait_user_answer\n");

	if(remmina_masterthread_exec_is_main_thread()) {
		/* On master thread */
		printf("ERROR: %s calls inside master thread is not implemented\n", __func__);
		return -1;
	}

	/* On a subthread, block until we receive something to unlock */
	pthread_mutex_lock(&priv->pt_mutex);
	priv->pt_complete = FALSE;
	while (!priv->pt_complete)
		pthread_cond_wait(&priv->pt_cond, &priv->pt_mutex);

	return priv->pt_retcode;

}

#ifdef GARBAGE
typedef struct _RemminaMessagePanelPriv {

	RemminaConnectionObject *parent_cnnobj;

	GtkWidget *image;
	GtkWidget *content_vbox;
	GtkWidget *status_label;

	gint mode;

	gchar *title;
	gchar *status;
	gchar *username;
	gchar *domain;
	gchar *password;
	gboolean save_password;
	gchar *cacert;
	gchar *cacrl;
	gchar *clientcert;
	gchar *clientkey;
} RemminaMessagePanel;

static void remmina_message_panel_class_init(RemminaMessagePanelClass *klass)
{
	TRACE_CALL(__func__);
}

static void remmina_message_panel_destroy(RemminaMessagePanel *dialog, gpointer data)
{
	TRACE_CALL(__func__);


	g_free(mp->priv->title);
	g_free(mp->priv->status);
	g_free(mp->priv->username);
	g_free(mp->priv->domain);
	g_free(mp->priv->password);
	g_free(mp->priv->cacert);
	g_free(mp->priv->cacrl);
	g_free(mp->priv->clientcert);
	g_free(mp->priv->clientkey);

	g_free(mp->priv);
	mp->priv = NULL;

}

static void remmina_message_panel_init(RemminaMessagePanel *mp)
{
	TRACE_CALL(__func__);
	RemminaMessagePanelPriv *priv;

	mp->mode = REMMINA_PANEL_MODE_IDLE;

	priv = g_new0(RemminaConnectionWindowPriv, 1);
	mp->priv = priv;


	GtkWidget *hbox = NULL;
	GtkWidget *widget;

	mp->image = NULL;
	mp->content_vbox = NULL;
	mp->status_label = NULL;
	mp->title = NULL;
	mp->status = NULL;
	mp->username = NULL;
	mp->domain = NULL;
	mp->password = NULL;
	mp->save_password = FALSE;
	mp->cacert = NULL;
	mp->cacrl = NULL;
	mp->clientcert = NULL;
	mp->clientkey = NULL;
}

RemminaMessagePanel* remmina_message_panel_init(RemminaConnectionObject* parent_cnnobj)
{
	RemminaMessagePanel *mp;
	mp = GTK_WIDGET(g_object_new(REMMINA_TYPE_PROTOCOL_WIDGET, NULL));
	mp->parent_cnnobj = parent_cnnobj;
	return mp;:
}



	gtk_dialog_add_buttons(GTK_DIALOG(dialog),  _("_Cancel"), GTK_RESPONSE_CANCEL, _("_OK"), GTK_RESPONSE_OK, NULL);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	/**** Create the dialog content from here ****/

	/* Create top-level hbox */
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_widget_show(hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox, TRUE, TRUE, 0);

	/* Icon */
	widget = gtk_image_new_from_icon_name("dialog-information", GTK_ICON_SIZE_DIALOG);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 4);
	dialog->image = widget;

	/* Create vbox for other dialog content */
	widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 4);
	dialog->content_vbox = widget;

	/* Entries */
	widget = gtk_label_new(dialog->title);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), widget, TRUE, TRUE, 4);
	dialog->status_label = widget;

	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(remmina_message_panel_destroy), NULL);



	remmina_widget_pool_register(GTK_WIDGET(dialog));
}


static void remmina_message_panel_connecting(RemminaInitDialog *dialog)
{
	TRACE_CALL(__func__);
	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));
	gtk_image_set_from_icon_name(GTK_IMAGE(dialog->image), "dialog-information", GTK_ICON_SIZE_DIALOG);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, FALSE);

	dialog->mode = REMMINA_INIT_MODE_CONNECTING;
}

GtkWidget* remmina_message_panel_new(const gchar *title_format, ...)
{
	TRACE_CALL(__func__);
	RemminaInitDialog *dialog;
	va_list args;

	dialog = REMMINA_INIT_DIALOG(g_object_new(REMMINA_TYPE_INIT_DIALOG, NULL));

	va_start(args, title_format);
	dialog->title = g_strdup_vprintf(title_format, args);
	va_end(args);
	gtk_window_set_title(GTK_WINDOW(dialog), dialog->title);

	remmina_message_panel_connecting(dialog);

	return GTK_WIDGET(dialog);
}

void remmina_message_panel_set_status(RemminaInitDialog *dialog, const gchar *status_format, ...)
{
	TRACE_CALL(__func__);
	/* This function can be called from a non main thread */

	va_list args;

	if (!dialog)
		return;

	if (status_format) {
		if (dialog->status)
			g_free(dialog->status);

		va_start(args, status_format);
		dialog->status = g_strdup_vprintf(status_format, args);
		va_end(args);

		if ( remmina_masterthread_exec_is_main_thread() ) {
			gtk_label_set_text(GTK_LABEL(dialog->status_label), dialog->status);
		}else  {
			RemminaMTExecData *d;
			d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
			d->func = FUNC_GTK_LABEL_SET_TEXT;
			d->p.gtk_label_set_text.label = GTK_LABEL(dialog->status_label);
			d->p.gtk_label_set_text.str = dialog->status;
			remmina_masterthread_exec_and_wait(d);
			g_free(d);
		}
	}
}

void remmina_message_panel_set_status_temp(RemminaInitDialog *dialog, const gchar *status_format, ...)
{
	TRACE_CALL(__func__);

	/* This function can be called from a non main thread */

	gchar* s;
	va_list args;

	if (status_format) {
		va_start(args, status_format);
		s = g_strdup_vprintf(status_format, args);
		va_end(args);

		if ( remmina_masterthread_exec_is_main_thread() ) {
			gtk_label_set_text(GTK_LABEL(dialog->status_label), dialog->status);
		}else  {
			RemminaMTExecData *d;
			d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
			d->func = FUNC_GTK_LABEL_SET_TEXT;
			d->p.gtk_label_set_text.label = GTK_LABEL(dialog->status_label);
			d->p.gtk_label_set_text.str = s;
			remmina_masterthread_exec_and_wait(d);
			g_free(d);
		}

		g_free(s);
	}
}

gint remmina_message_panel_authpwd(RemminaMessagePanel *mp, const gchar *message, const gchar *keylabel, gboolean allow_save)
{
	TRACE_CALL(__func__);

	GtkWidget *grid;
	GtkWidget *password_entry;
	GtkWidget *save_password_check;
	GtkWidget *widget;
	gint ret;
	gchar *s;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		gint retval;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_DIALOG_AUTHPWD;
		d->p.dialog_authpwd.dialog = dialog;
		d->p.dialog_authpwd.label = label;
		d->p.dialog_authpwd.allow_save = allow_save;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.dialog_authpwd.retval;
		g_free(d);
		return retval;
	}

	if (mp->mode != REMMINA_PANEL_MODE_IDLE) {
		printf("INTERNAL ERROR: message panel is not in IDLE mode and a new panel has been requested\n");
	}

	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));

	/* Create grid (was a table) */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

	/* Text message */
	widget = gtk_label_new(message);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 0, 2, 1);

	/* Icon */
	/* gtk_image_set_from_icon_name(GTK_IMAGE(dialog->image), "dialog-password", GTK_ICON_SIZE_DIALOG); */

	/* Entries */
	widget = gtk_label_new(label);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 1, 1, 1);

	password_entry = gtk_entry_new();
	gtk_widget_show(password_entry);
	gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 1, 2, 1);
	gtk_entry_set_max_length(GTK_ENTRY(password_entry), 100);
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(password_entry), TRUE);

	s = g_strdup_printf(_("Save %s"), keylabel);
	save_password_check = gtk_check_button_new_with_label(s);
	g_free(s);
	gtk_widget_show(save_password_check);
	gtk_grid_attach(GTK_GRID(grid), save_password_check, 0, 2, 2, 1);
	if (dialog->save_password)
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_password_check), TRUE);
	gtk_widget_set_sensitive(save_password_check, allow_save);

	/* Pack it into the RemminaMessagePanel (which is a GtkBox) */
	gtk_box_pack_start(GTK_BOX(mb), grid, TRUE, TRUE, 4);

	gtk_widget_grab_focus(password_entry);

	dialog->mode = REMMINA_INIT_MODE_AUTHPWD;

	/* Show it */
	remmina_connection_object_show_message_panel(mp->priv->parent_cnnobj);

	/* Now run it */
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	if (ret == GTK_RESPONSE_OK) {
		dialog->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(password_entry)));
		dialog->save_password = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(save_password_check));
	}

	gtk_widget_destroy(grid);

	remmina_message_panel_connecting(dialog);

	return ret;
}

gint remmina_message_panel_wait_user_answer(RemminaMessagePanel *mp)
{
	/* This function is very similar to gtk_dialog_run() for a dialog box */

}

gint remmina_message_panel_authuserpwd(RemminaInitDialog *dialog, gboolean want_domain, const gchar *default_username,
				     const gchar *default_domain, gboolean allow_save)
{
	TRACE_CALL(__func__);

	GtkWidget *grid;
	GtkWidget *username_entry;
	GtkWidget *password_entry;
	GtkWidget *domain_entry = NULL;
	GtkWidget *save_password_check;
	GtkWidget *widget;
	gint ret;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		gint retval;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_DIALOG_AUTHUSERPWD;
		d->p.dialog_authuserpwd.dialog = dialog;
		d->p.dialog_authuserpwd.want_domain = want_domain;
		d->p.dialog_authuserpwd.default_username = default_username;
		d->p.dialog_authuserpwd.default_domain = default_domain;
		d->p.dialog_authuserpwd.allow_save = allow_save;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.dialog_authuserpwd.retval;
		g_free(d);
		return retval;
	}

	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));

	/* Create grid */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), TRUE);

	/* Icon */
	gtk_image_set_from_icon_name(GTK_IMAGE(dialog->image), "dialog-password", GTK_ICON_SIZE_DIALOG);

	/* Entries */
	widget = gtk_label_new(_("User name"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 0, 1, 1);

	username_entry = gtk_entry_new();
	gtk_widget_show(username_entry);
	gtk_grid_attach(GTK_GRID(grid), username_entry, 1, 0, 2, 1);
	gtk_entry_set_max_length(GTK_ENTRY(username_entry), 100);
	if (default_username && default_username[0] != '\0') {
		gtk_entry_set_text(GTK_ENTRY(username_entry), default_username);
	}

	widget = gtk_label_new(_("Password"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 2, 1, 1);

	password_entry = gtk_entry_new();
	gtk_widget_show(password_entry);
	gtk_grid_attach(GTK_GRID(grid), password_entry, 1, 2, 2, 1);
	gtk_entry_set_max_length(GTK_ENTRY(password_entry), 100);
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(password_entry), TRUE);


	if (want_domain) {
		widget = gtk_label_new(_("Domain"));
		gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
		gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
		gtk_widget_show(widget);
		gtk_grid_attach(GTK_GRID(grid), widget, 0, 3, 1, 1);

		domain_entry = gtk_entry_new();
		gtk_widget_show(domain_entry);
		gtk_grid_attach(GTK_GRID(grid), domain_entry, 1, 3, 2, 1);
		gtk_entry_set_max_length(GTK_ENTRY(domain_entry), 100);
		gtk_entry_set_activates_default(GTK_ENTRY(domain_entry), TRUE);
		if (default_domain && default_domain[0] != '\0') {
			gtk_entry_set_text(GTK_ENTRY(domain_entry), default_domain);
		}
	}

	save_password_check = gtk_check_button_new_with_label(_("Save password"));
	if (allow_save) {
		gtk_widget_show(save_password_check);
		gtk_grid_attach(GTK_GRID(grid), save_password_check, 0, 4, 2, 3);
		if (dialog->save_password)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_password_check), TRUE);
	}else  {
		gtk_widget_set_sensitive(save_password_check, FALSE);
	}

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), grid, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	if (default_username && default_username[0] != '\0') {
		gtk_widget_grab_focus(password_entry);
	}else  {
		gtk_widget_grab_focus(username_entry);
	}

	dialog->mode = REMMINA_INIT_MODE_AUTHUSERPWD;

	/* Now run it */
	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	if (ret == GTK_RESPONSE_OK) {
		dialog->username = g_strdup(gtk_entry_get_text(GTK_ENTRY(username_entry)));
		dialog->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(password_entry)));
		dialog->save_password = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(save_password_check));

		if (want_domain)
			dialog->domain = g_strdup(gtk_entry_get_text(GTK_ENTRY(domain_entry)));
	}

	gtk_widget_destroy(grid);

	remmina_message_panel_connecting(dialog);

	return ret;
}

gint remmina_message_panel_certificate(RemminaInitDialog* dialog, const gchar* subject, const gchar* issuer, const gchar* fingerprint)
{
	TRACE_CALL(__func__);

	gint status;
	GtkWidget* grid;
	GtkWidget* widget;
	gchar* s;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		gint retval;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_DIALOG_CERT;
		d->p.dialog_certificate.dialog = dialog;
		d->p.dialog_certificate.subject = subject;
		d->p.dialog_certificate.issuer = issuer;
		d->p.dialog_certificate.fingerprint = fingerprint;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.dialog_certificate.retval;
		g_free(d);
		return retval;
	}

	gtk_label_set_text(GTK_LABEL(dialog->status_label), _("Certificate Details:"));

	/* Create table */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
	//gtk_grid_set_column_homogeneous (GTK_GRID(grid), TRUE);

	/* Icon */
	gtk_image_set_from_icon_name(GTK_IMAGE(dialog->image), "dialog-password", GTK_ICON_SIZE_DIALOG);

	/* Entries */
	widget = gtk_label_new(_("Subject:"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 0, 1, 1);

	widget = gtk_label_new(subject);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 0, 2, 1);

	widget = gtk_label_new(_("Issuer:"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 1, 1, 1);

	widget = gtk_label_new(issuer);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 1, 2, 1);

	widget = gtk_label_new(_("Fingerprint:"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 2, 1, 1);

	widget = gtk_label_new(fingerprint);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 2, 2, 1);

	widget = gtk_label_new(NULL);
	s = g_strdup_printf("<span size=\"large\"><b>%s</b></span>", _("Accept Certificate?"));
	gtk_label_set_markup(GTK_LABEL(widget), s);
	g_free(s);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 3, 3, 1);

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), grid, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	dialog->mode = REMMINA_INIT_MODE_CERTIFICATE;

	/* Now run it */
	status = gtk_dialog_run(GTK_DIALOG(dialog));

	if (status == GTK_RESPONSE_OK) {

	}

	gtk_widget_destroy(grid);

	return status;
}
gint remmina_message_panel_certificate_changed(RemminaInitDialog* dialog, const gchar* subject, const gchar* issuer, const gchar* new_fingerprint, const gchar* old_fingerprint)
{
	TRACE_CALL(__func__);

	gint status;
	GtkWidget* grid;
	GtkWidget* widget;
	gchar* s;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		gint retval;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_DIALOG_CERTCHANGED;
		d->p.dialog_certchanged.dialog = dialog;
		d->p.dialog_certchanged.subject = subject;
		d->p.dialog_certchanged.issuer = issuer;
		d->p.dialog_certchanged.new_fingerprint = new_fingerprint;
		d->p.dialog_certchanged.old_fingerprint = old_fingerprint;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.dialog_certchanged.retval;
		g_free(d);
		return retval;
	}

	gtk_label_set_text(GTK_LABEL(dialog->status_label), _("Certificate Changed! Details:"));

	/* Create table */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

	/* Icon */
	gtk_image_set_from_icon_name(GTK_IMAGE(dialog->image), "dialog-password", GTK_ICON_SIZE_DIALOG);

	/* Entries */
	/* Not tested */
	widget = gtk_label_new(_("Subject:"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 0, 1, 1);


	widget = gtk_label_new(subject);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 0, 2, 1);

	widget = gtk_label_new(_("Issuer:"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 1, 1, 1);

	widget = gtk_label_new(issuer);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 1, 2, 1);

	widget = gtk_label_new(_("Old Fingerprint:"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 2, 1, 1);

	widget = gtk_label_new(old_fingerprint);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 2, 2, 1);

	widget = gtk_label_new(_("New Fingerprint:"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 3, 1, 1);

	widget = gtk_label_new(new_fingerprint);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 3, 2, 1);

	widget = gtk_label_new(NULL);
	s = g_strdup_printf("<span size=\"large\"><b>%s</b></span>", _("Accept Changed Certificate?"));
	gtk_label_set_markup(GTK_LABEL(widget), s);
	g_free(s);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 4, 3, 1);

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), grid, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	dialog->mode = REMMINA_INIT_MODE_CERTIFICATE;

	/* Now run it */
	status = gtk_dialog_run(GTK_DIALOG(dialog));

	if (status == GTK_RESPONSE_OK) {

	}

	gtk_widget_destroy(grid);

	return status;
}

/* NOT TESTED */
static GtkWidget* remmina_message_panel_create_file_button(GtkGrid *grid, const gchar *label, gint row, const gchar *filename)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	gchar *pkidir;

	widget = gtk_label_new(label);
	gtk_widget_show(widget);
	gtk_grid_attach(grid, widget, 0, row, 1, row + 1);

	widget = gtk_file_chooser_button_new(label, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_width_chars(GTK_FILE_CHOOSER_BUTTON(widget), 25);
	gtk_widget_show(widget);
	gtk_grid_attach(grid, widget, 1, row, 2, row + 1);
	if (filename && filename[0] != '\0') {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widget), filename);
	}else  {
		pkidir = g_strdup_printf("%s/.pki", g_get_home_dir());
		if (g_file_test(pkidir, G_FILE_TEST_IS_DIR)) {
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widget), pkidir);
		}
		g_free(pkidir);
	}

	return widget;
}

gint remmina_message_panel_authx509(RemminaInitDialog *dialog, const gchar *cacert, const gchar *cacrl, const gchar *clientcert,
				  const gchar *clientkey)
{
	TRACE_CALL(__func__);

	GtkWidget *grid;
	GtkWidget *cacert_button;
	GtkWidget *cacrl_button;
	GtkWidget *clientcert_button;
	GtkWidget *clientkey_button;
	gint ret;


	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		gint retval;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_DIALOG_AUTHX509;
		d->p.dialog_authx509.dialog = dialog;
		d->p.dialog_authx509.cacert = cacert;
		d->p.dialog_authx509.cacrl = cacrl;
		d->p.dialog_authx509.clientcert = clientcert;
		d->p.dialog_authx509.clientkey = clientkey;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.dialog_authx509.retval;
		g_free(d);
		return retval;
	}

	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));

	/* Create table */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 8);

	/* Icon */
	gtk_image_set_from_icon_name(GTK_IMAGE(dialog->image), "dialog-password", GTK_ICON_SIZE_DIALOG);

	/* Buttons for choosing the certificates */
	cacert_button = remmina_message_panel_create_file_button(GTK_GRID(grid), _("CA certificate"), 0, cacert);
	cacrl_button = remmina_message_panel_create_file_button(GTK_GRID(grid), _("CA CRL"), 1, cacrl);
	clientcert_button = remmina_message_panel_create_file_button(GTK_GRID(grid), _("Client certificate"), 2, clientcert);
	clientkey_button = remmina_message_panel_create_file_button(GTK_GRID(grid), _("Client key"), 3, clientkey);

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), grid, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	dialog->mode = REMMINA_INIT_MODE_AUTHX509;

	/* Now run it */
	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	if (ret == GTK_RESPONSE_OK) {
		dialog->cacert = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(cacert_button));
		dialog->cacrl = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(cacrl_button));
		dialog->clientcert = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(clientcert_button));
		dialog->clientkey = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(clientkey_button));
	}

	gtk_widget_destroy(grid);

	remmina_message_panel_connecting(dialog);

	return ret;
}

gint remmina_message_panel_serverkey_confirm(RemminaInitDialog *dialog, const gchar *serverkey, const gchar *prompt)
{
	TRACE_CALL(__func__);

	GtkWidget *vbox = NULL;
	GtkWidget *widget;
	gint ret;

	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		gint retval;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_DIALOG_SERVERKEY_CONFIRM;
		d->p.dialog_serverkey_confirm.dialog = dialog;
		d->p.dialog_serverkey_confirm.serverkey = serverkey;
		d->p.dialog_serverkey_confirm.prompt = prompt;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.dialog_serverkey_confirm.retval;
		g_free(d);
		return retval;
	}

	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));

	/* Create vbox */
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
	gtk_widget_show(vbox);

	/* Icon */
	gtk_image_set_from_icon_name(GTK_IMAGE(dialog->image), "dialog-warning", GTK_ICON_SIZE_DIALOG);

	/* Entries */
	widget = gtk_label_new(prompt);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 4);

	widget = gtk_label_new(serverkey);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 4);

	widget = gtk_label_new(_("Do you trust the new public key?"));
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 4);

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), vbox, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	dialog->mode = REMMINA_INIT_MODE_SERVERKEY_CONFIRM;

	/* Now run it */
	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(vbox);
	remmina_message_panel_connecting(dialog);

	return ret;
}


gint remmina_message_panel_serverkey_unknown(RemminaInitDialog *dialog, const gchar *serverkey)
{
	TRACE_CALL(__func__);
	/* This function can be called from a non main thread */

	return remmina_message_panel_serverkey_confirm(dialog, serverkey,
		_("The server is unknown. The public key fingerprint is:"));
}

gint remmina_message_panel_serverkey_changed(RemminaInitDialog *dialog, const gchar *serverkey)
{
	TRACE_CALL(__func__);
	/* This function can be called from a non main thread */

	return remmina_message_panel_serverkey_confirm(dialog, serverkey,
		_("WARNING: The server has changed its public key. This means either you are under attack,\n"
			"or the administrator has changed the key. The new public key fingerprint is:"));
}

#endif


