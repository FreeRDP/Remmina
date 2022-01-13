/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2022 Antenore Gatta, Giovanni Panozzo
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

#include <stdlib.h>

#include <glib/gi18n.h>

#include "config.h"
#include "remmina_public.h"
#include "remmina_log.h"
#include "remmina_sodium.h"
#include "remmina_passwd.h"
#include "remmina/remmina_trace_calls.h"

static RemminaPasswdDialog *remmina_passwd_dialog;
#define GET_OBJ(object_name) gtk_builder_get_object(remmina_passwd_dialog->builder, object_name)

void remmina_passwd_repwd_on_changed(GtkEditable *editable, RemminaPasswdDialog *dialog)
{
	TRACE_CALL(__func__);
	GtkCssProvider *provider;
	const gchar *color;
	const gchar *password;
	const gchar *verify;
	gboolean sensitive = FALSE;

	provider = gtk_css_provider_new();

	password = gtk_entry_get_text(remmina_passwd_dialog->entry_password);
	verify = gtk_entry_get_text(remmina_passwd_dialog->entry_verify);
	if (g_strcmp0(password, verify) == 0) {
		color = g_strdup("green");
		sensitive = TRUE;
	} else
		color = g_strdup("red");

	gtk_widget_set_sensitive (GTK_WIDGET(remmina_passwd_dialog->button_submit), sensitive);

	if (verify == NULL || verify[0] == '\0')
		color = g_strdup("inherit");

	gtk_css_provider_load_from_data(provider,
					g_strdup_printf(
						".entry_verify {\n"
						"  color: %s;\n"
						"}\n"
						, color)
					, -1, NULL);
	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
						  GTK_STYLE_PROVIDER(provider),
						  GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	gtk_widget_queue_draw(GTK_WIDGET(remmina_passwd_dialog->entry_verify));
	g_object_unref(provider);
}

static void remmina_passwd_password_activate(GtkEntry *entry, gpointer user_data)
{
	TRACE_CALL(__func__);

	 gtk_widget_grab_focus(GTK_WIDGET(remmina_passwd_dialog->entry_verify));

}

static void remmina_passwd_cancel_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	gtk_dialog_response (remmina_passwd_dialog->dialog, GTK_RESPONSE_CANCEL);
}

static void remmina_passwd_submit_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	remmina_passwd_dialog->password = gtk_entry_get_text(
			GTK_ENTRY(remmina_passwd_dialog->entry_verify));
	gtk_dialog_response (remmina_passwd_dialog->dialog, GTK_RESPONSE_ACCEPT);
}

gboolean remmina_passwd(GtkWindow *parent, gchar **unlock_password)
{
	TRACE_CALL(__func__);

	remmina_passwd_dialog = g_new0(RemminaPasswdDialog, 1);
	gboolean rc = FALSE;

	remmina_passwd_dialog->builder = remmina_public_gtk_builder_new_from_resource("/org/remmina/Remmina/src/../data/ui/remmina_passwd.glade");
	remmina_passwd_dialog->dialog = GTK_DIALOG(GET_OBJ("RemminaPasswdDialog"));
	if (parent)
		gtk_window_set_transient_for(GTK_WINDOW(remmina_passwd_dialog->dialog), parent);

	remmina_passwd_dialog->entry_password = GTK_ENTRY(GET_OBJ("entry_password"));
	remmina_passwd_dialog->entry_verify = GTK_ENTRY(GET_OBJ("entry_verify"));
	gtk_entry_set_activates_default(GTK_ENTRY(remmina_passwd_dialog->entry_verify), TRUE);
	remmina_passwd_dialog->button_submit = GTK_BUTTON(GET_OBJ("button_submit"));
	gtk_widget_set_can_default(GTK_WIDGET(remmina_passwd_dialog->button_submit), TRUE);
	gtk_widget_grab_default(GTK_WIDGET(remmina_passwd_dialog->button_submit));
	remmina_passwd_dialog->button_cancel = GTK_BUTTON(GET_OBJ("button_cancel"));

	g_signal_connect(remmina_passwd_dialog->entry_password, "activate",
			 G_CALLBACK(remmina_passwd_password_activate), (gpointer)remmina_passwd_dialog);
	g_signal_connect(remmina_passwd_dialog->button_submit, "clicked",
			 G_CALLBACK(remmina_passwd_submit_clicked), (gpointer)remmina_passwd_dialog);
	g_signal_connect(remmina_passwd_dialog->button_cancel, "clicked",
			 G_CALLBACK(remmina_passwd_cancel_clicked), (gpointer)remmina_passwd_dialog);

	/* Connect signals */
	gtk_builder_connect_signals(remmina_passwd_dialog->builder, NULL);

	int result = gtk_dialog_run(remmina_passwd_dialog->dialog);
	switch (result)
	{
		case GTK_RESPONSE_ACCEPT:
#if SODIUM_VERSION_INT >= 90200
			//REMMINA_DEBUG ("Password before encryption: %s", remmina_passwd_dialog->password);
			*unlock_password = remmina_sodium_pwhash_str(remmina_passwd_dialog->password);
#else
			*unlock_password = g_strdup(remmina_passwd_dialog->password);
#endif
			//REMMINA_DEBUG ("Password after encryption is: %s", *unlock_password);
			remmina_passwd_dialog->password = NULL;
			rc = TRUE;
			break;
		default:
			remmina_passwd_dialog->password = NULL;
			*unlock_password = NULL;
			rc = FALSE;
			break;
	}
	gtk_widget_destroy(GTK_WIDGET(remmina_passwd_dialog->dialog));
	remmina_passwd_dialog->dialog = NULL;
	return rc;
}
