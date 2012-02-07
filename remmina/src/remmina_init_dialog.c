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
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remmina_widget_pool.h"
#include "remmina_init_dialog.h"

G_DEFINE_TYPE( RemminaInitDialog, remmina_init_dialog, GTK_TYPE_DIALOG)

static void remmina_init_dialog_class_init(RemminaInitDialogClass *klass)
{
}

static void remmina_init_dialog_destroy(RemminaInitDialog *dialog, gpointer data)
{
	g_free(dialog->title);
	g_free(dialog->status);
	g_free(dialog->username);
	g_free(dialog->domain);
	g_free(dialog->password);
	g_free(dialog->cacert);
	g_free(dialog->cacrl);
	g_free(dialog->clientcert);
	g_free(dialog->clientkey);
}

static void remmina_init_dialog_init(RemminaInitDialog *dialog)
{
	GtkWidget *hbox;
	GtkWidget *widget;

	dialog->image = NULL;
	dialog->content_vbox = NULL;
	dialog->status_label = NULL;
	dialog->mode = REMMINA_INIT_MODE_CONNECTING;
	dialog->title = NULL;
	dialog->status = NULL;
	dialog->username = NULL;
	dialog->domain = NULL;
	dialog->password = NULL;
	dialog->save_password = FALSE;
	dialog->cacert = NULL;
	dialog->cacrl = NULL;
	dialog->clientcert = NULL;
	dialog->clientkey = NULL;

	gtk_dialog_add_buttons(GTK_DIALOG(dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_set_alternative_button_order(GTK_DIALOG(dialog), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1);

	gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_OK);

	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER_ALWAYS);
	gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);

	/**** Create the dialog content from here ****/

	/* Create top-level hbox */
	hbox = gtk_hbox_new(FALSE, 4);
	gtk_widget_show(hbox);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 15);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox, TRUE, TRUE, 0);

	/* Icon */
	widget = gtk_image_new_from_stock(GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 4);
	dialog->image = widget;

	/* Create vbox for other dialog content */
	widget = gtk_vbox_new(FALSE, 4);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 4);
	dialog->content_vbox = widget;

	/* Entries */
	widget = gtk_label_new(dialog->title);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), widget, TRUE, TRUE, 4);
	dialog->status_label = widget;

	g_signal_connect(G_OBJECT(dialog), "destroy", G_CALLBACK(remmina_init_dialog_destroy), NULL);

	remmina_widget_pool_register(GTK_WIDGET(dialog));
}

static void remmina_init_dialog_connecting(RemminaInitDialog *dialog)
{
	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));
	gtk_image_set_from_stock(GTK_IMAGE(dialog->image), GTK_STOCK_DIALOG_INFO, GTK_ICON_SIZE_DIALOG);
	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, FALSE);

	dialog->mode = REMMINA_INIT_MODE_CONNECTING;
}

GtkWidget* remmina_init_dialog_new(const gchar *title_format, ...)
{
	RemminaInitDialog *dialog;
	va_list args;

	dialog = REMMINA_INIT_DIALOG (g_object_new (REMMINA_TYPE_INIT_DIALOG, NULL));

	va_start (args, title_format);
	dialog->title = g_strdup_vprintf (title_format, args);
	va_end (args);
	gtk_window_set_title (GTK_WINDOW(dialog), dialog->title);

	remmina_init_dialog_connecting (dialog);

	return GTK_WIDGET (dialog);
}

void remmina_init_dialog_set_status(RemminaInitDialog *dialog, const gchar *status_format, ...)
{
	va_list args;

	if (status_format)
	{
		if (dialog->status)
			g_free(dialog->status);

		va_start(args, status_format);
		dialog->status = g_strdup_vprintf(status_format, args);
		va_end(args);

		gtk_label_set_text(GTK_LABEL(dialog->status_label), dialog->status);
	}
}

void remmina_init_dialog_set_status_temp(RemminaInitDialog *dialog, const gchar *status_format, ...)
{
	gchar* s;
	va_list args;

	if (status_format)
	{
		va_start(args, status_format);
		s = g_strdup_vprintf(status_format, args);
		va_end(args);

		gtk_label_set_text(GTK_LABEL (dialog->status_label), s);
		g_free(s);
	}
}

gint remmina_init_dialog_authpwd(RemminaInitDialog *dialog, const gchar *label, gboolean allow_save)
{
	GtkWidget *table;
	GtkWidget *password_entry;
	GtkWidget *save_password_check;
	GtkWidget *widget;
	gint ret;
	gchar *s;

	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));

	/* Create table */
	table = gtk_table_new(2, 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 8);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	/* Icon */
	gtk_image_set_from_stock(GTK_IMAGE(dialog->image), GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);

	/* Entries */
	widget = gtk_label_new(label);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	password_entry = gtk_entry_new();
	gtk_widget_show(password_entry);
	gtk_table_attach_defaults(GTK_TABLE(table), password_entry, 1, 2, 0, 1);
	gtk_entry_set_max_length(GTK_ENTRY(password_entry), 100);
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(password_entry), TRUE);

	s = g_strdup_printf(_("Save %s"), label);
	save_password_check = gtk_check_button_new_with_label(s);
	g_free(s);
	gtk_widget_show(save_password_check);
	gtk_table_attach_defaults(GTK_TABLE(table), save_password_check, 0, 2, 1, 2);
	if (allow_save)
	{
		if (dialog->save_password)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_password_check), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(save_password_check, FALSE);
	}

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), table, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	gtk_widget_grab_focus(password_entry);

	dialog->mode = REMMINA_INIT_MODE_AUTHPWD;

	/* Now run it */
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	if (ret == GTK_RESPONSE_OK)
	{
		dialog->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(password_entry)));
		dialog->save_password = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(save_password_check));
	}

	gtk_container_remove(GTK_CONTAINER(dialog->content_vbox), table);

	remmina_init_dialog_connecting(dialog);

	return ret;
}

gint remmina_init_dialog_authuserpwd(RemminaInitDialog *dialog, gboolean want_domain, const gchar *default_username,
		const gchar *default_domain, gboolean allow_save)
{
	GtkWidget *table;
	GtkWidget *username_entry;
	GtkWidget *password_entry;
	GtkWidget *domain_entry = NULL;
	GtkWidget *save_password_check;
	GtkWidget *widget;
	gint ret;

	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));

	/* Create table */
	table = gtk_table_new(4, 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 8);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	/* Icon */
	gtk_image_set_from_stock(GTK_IMAGE(dialog->image), GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);

	/* Entries */
	widget = gtk_label_new(_("User name"));
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	username_entry = gtk_entry_new();
	gtk_widget_show(username_entry);
	gtk_table_attach_defaults(GTK_TABLE(table), username_entry, 1, 2, 0, 1);
	gtk_entry_set_max_length(GTK_ENTRY(username_entry), 100);
	if (default_username && default_username[0] != '\0')
	{
		gtk_entry_set_text(GTK_ENTRY(username_entry), default_username);
	}

	widget = gtk_label_new(_("Password"));
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

	password_entry = gtk_entry_new();
	gtk_widget_show(password_entry);
	gtk_table_attach_defaults(GTK_TABLE(table), password_entry, 1, 2, 1, 2);
	gtk_entry_set_max_length(GTK_ENTRY(password_entry), 100);
	gtk_entry_set_visibility(GTK_ENTRY(password_entry), FALSE);
	gtk_entry_set_activates_default(GTK_ENTRY(password_entry), TRUE);

	save_password_check = gtk_check_button_new_with_label(_("Save password"));
	gtk_widget_show(save_password_check);
	gtk_table_attach_defaults(GTK_TABLE(table), save_password_check, 0, 2, 2, 3);
	if (allow_save)
	{
		if (dialog->save_password)
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(save_password_check), TRUE);
	}
	else
	{
		gtk_widget_set_sensitive(save_password_check, FALSE);
	}

	if (want_domain)
	{
		widget = gtk_label_new(_("Domain"));
		gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
		gtk_widget_show(widget);
		gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);

		domain_entry = gtk_entry_new();
		gtk_widget_show(domain_entry);
		gtk_table_attach_defaults(GTK_TABLE(table), domain_entry, 1, 2, 3, 4);
		gtk_entry_set_max_length(GTK_ENTRY(domain_entry), 100);
		if (default_domain && default_domain[0] != '\0')
		{
			gtk_entry_set_text(GTK_ENTRY(domain_entry), default_domain);
		}
	}

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), table, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	if (default_username && default_username[0] != '\0')
	{
		gtk_widget_grab_focus(password_entry);
	}
	else
	{
		gtk_widget_grab_focus(username_entry);
	}

	dialog->mode = REMMINA_INIT_MODE_AUTHUSERPWD;

	/* Now run it */
	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	if (ret == GTK_RESPONSE_OK)
	{
		dialog->username = g_strdup(gtk_entry_get_text(GTK_ENTRY(username_entry)));
		dialog->password = g_strdup(gtk_entry_get_text(GTK_ENTRY(password_entry)));
		dialog->save_password = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(save_password_check));

		if (want_domain)
			dialog->domain = g_strdup(gtk_entry_get_text(GTK_ENTRY(domain_entry)));
	}

	gtk_container_remove(GTK_CONTAINER(dialog->content_vbox), table);

	remmina_init_dialog_connecting(dialog);

	return ret;
}

gint remmina_init_dialog_certificate(RemminaInitDialog* dialog, const gchar* subject, const gchar* issuer, const gchar* fingerprint)
{
	gint status;
	GtkWidget* table;
	GtkWidget* widget;

	gtk_label_set_text(GTK_LABEL(dialog->status_label), "Certificate Details:");

	/* Create table */
	table = gtk_table_new(5, 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 8);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	/* Icon */
	gtk_image_set_from_stock(GTK_IMAGE(dialog->image), GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);

	/* Entries */
	widget = gtk_label_new(_("Subject:"));
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

	widget = gtk_label_new(subject);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, 0, 1, GTK_FILL, 0, 0, 0);

	widget = gtk_label_new(_("Issuer:"));
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

	widget = gtk_label_new(issuer);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);

	widget = gtk_label_new(_("Fingerprint:"));
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);

	widget = gtk_label_new(fingerprint);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);

	widget = gtk_label_new(_("Accept Certificate?"));
	gtk_misc_set_alignment(GTK_MISC(widget), 1.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(GTK_TABLE(table), widget, 0, 2, 3, 4, GTK_FILL, 0, 0, 0);

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), table, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	dialog->mode = REMMINA_INIT_MODE_CERTIFICATE;

	/* Now run it */
	status = gtk_dialog_run(GTK_DIALOG(dialog));

	if (status == GTK_RESPONSE_OK)
	{

	}

	gtk_container_remove(GTK_CONTAINER(dialog->content_vbox), table);

	return status;
}

static GtkWidget* remmina_init_dialog_create_file_button(GtkTable *table, const gchar *label, gint row, const gchar *filename)
{
	GtkWidget *widget;
	gchar *pkidir;

	widget = gtk_label_new(label);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_table_attach(table, widget, 0, 1, row, row + 1, GTK_FILL, 0, 0, 0);

	widget = gtk_file_chooser_button_new(label, GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_button_set_width_chars(GTK_FILE_CHOOSER_BUTTON(widget), 25);
	gtk_widget_show(widget);
	gtk_table_attach_defaults(table, widget, 1, 2, row, row + 1);
	if (filename && filename[0] != '\0')
	{
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(widget), filename);
	}
	else
	{
		pkidir = g_strdup_printf("%s/.pki", g_get_home_dir());
		if (g_file_test(pkidir, G_FILE_TEST_IS_DIR))
		{
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(widget), pkidir);
		}
		g_free(pkidir);
	}

	return widget;
}

gint remmina_init_dialog_authx509(RemminaInitDialog *dialog, const gchar *cacert, const gchar *cacrl, const gchar *clientcert,
		const gchar *clientkey)
{
	GtkWidget *table;
	GtkWidget *cacert_button;
	GtkWidget *cacrl_button;
	GtkWidget *clientcert_button;
	GtkWidget *clientkey_button;
	gint ret;

	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));

	/* Create table */
	table = gtk_table_new(4, 2, FALSE);
	gtk_widget_show(table);
	gtk_table_set_row_spacings(GTK_TABLE(table), 8);
	gtk_table_set_col_spacings(GTK_TABLE(table), 8);

	/* Icon */
	gtk_image_set_from_stock(GTK_IMAGE(dialog->image), GTK_STOCK_DIALOG_AUTHENTICATION, GTK_ICON_SIZE_DIALOG);

	/* Buttons for choosing the certificates */
	cacert_button = remmina_init_dialog_create_file_button(GTK_TABLE(table), _("CA certificate"), 0, cacert);
	cacrl_button = remmina_init_dialog_create_file_button(GTK_TABLE(table), _("CA CRL"), 1, cacrl);
	clientcert_button = remmina_init_dialog_create_file_button(GTK_TABLE(table), _("Client certificate"), 2, clientcert);
	clientkey_button = remmina_init_dialog_create_file_button(GTK_TABLE(table), _("Client key"), 3, clientkey);

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), table, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	dialog->mode = REMMINA_INIT_MODE_AUTHX509;

	/* Now run it */
	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	if (ret == GTK_RESPONSE_OK)
	{
		dialog->cacert = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(cacert_button));
		dialog->cacrl = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(cacrl_button));
		dialog->clientcert = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(clientcert_button));
		dialog->clientkey = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(clientkey_button));
	}

	gtk_container_remove(GTK_CONTAINER(dialog->content_vbox), table);

	remmina_init_dialog_connecting(dialog);

	return ret;
}

static gint remmina_init_dialog_serverkey_confirm(RemminaInitDialog *dialog, const gchar *serverkey, const gchar *prompt)
{
	GtkWidget *vbox;
	GtkWidget *widget;
	gint ret;

	gtk_label_set_text(GTK_LABEL(dialog->status_label), (dialog->status ? dialog->status : dialog->title));

	/* Create vbox */
	vbox = gtk_vbox_new(FALSE, 4);
	gtk_widget_show(vbox);

	/* Icon */
	gtk_image_set_from_stock(GTK_IMAGE(dialog->image), GTK_STOCK_DIALOG_WARNING, GTK_ICON_SIZE_DIALOG);

	/* Entries */
	widget = gtk_label_new(prompt);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 4);

	widget = gtk_label_new(serverkey);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 4);

	widget = gtk_label_new(_("Do you trust the new public key?"));
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(vbox), widget, TRUE, TRUE, 4);

	/* Pack it into the dialog */
	gtk_box_pack_start(GTK_BOX(dialog->content_vbox), vbox, TRUE, TRUE, 4);

	gtk_dialog_set_response_sensitive(GTK_DIALOG(dialog), GTK_RESPONSE_OK, TRUE);

	dialog->mode = REMMINA_INIT_MODE_SERVERKEY_CONFIRM;

	/* Now run it */
	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_container_remove(GTK_CONTAINER(dialog->content_vbox), vbox);
	remmina_init_dialog_connecting(dialog);

	return ret;
}

gint remmina_init_dialog_serverkey_unknown(RemminaInitDialog *dialog, const gchar *serverkey)
{
	return remmina_init_dialog_serverkey_confirm(dialog, serverkey,
			_("The server is unknown. The public key fingerprint is:"));
}

gint remmina_init_dialog_serverkey_changed(RemminaInitDialog *dialog, const gchar *serverkey)
{
	return remmina_init_dialog_serverkey_confirm(dialog, serverkey,
			_("WARNING: The server has changed its public key. This means either you are under attack,\n"
					"or the administrator has changed the key. The new public key fingerprint is:"));
}

