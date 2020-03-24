/*
 * Remmina - The GTK+ Remote Desktop Client
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

#include <stdlib.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "config.h"
#include "remmina_sodium.h"
#include "remmina_pref.h"
#include "remmina_unlock.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

#if SODIUM_VERSION_INT >= 90200
static RemminaUnlockDialog *remmina_unlock_dialog;
#define GET_OBJ(object_name) gtk_builder_get_object(remmina_unlock_dialog->builder, object_name)

GTimer *timer;
gboolean isinit;

static void remmina_unlock_timer_init()
{
	TRACE_CALL(__func__);

	timer = g_timer_new();
	g_info("Unlock Master Password timer initialized");
}

static void remmina_unlock_timer_reset(gpointer user_data)
{
	TRACE_CALL(__func__);

	g_timer_reset(timer);
	g_info("Unlock Master Password timer reset");
}

void remmina_unlock_timer_destroy()
{
	TRACE_CALL(__func__);

	g_timer_destroy(timer);
}

static void remmina_unlock_unlock_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	//g_timer_reset(remmina_unlock_dialog->timer);

	gchar *unlock_password;
	const gchar *entry_passwd;
	gint rc;

	unlock_password = remmina_pref_get_value("unlock_password");
	entry_passwd = gtk_entry_get_text(remmina_unlock_dialog->entry_unlock);
	rc = remmina_sodium_pwhash_str_verify(unlock_password, entry_passwd);
	g_info("remmina_sodium_pwhash_str_verify returned %i", rc);

	if (rc == 0) {
		g_info("Passphrase veryfied successfully");
		remmina_unlock_timer_reset(remmina_unlock_dialog);
		gtk_widget_destroy(GTK_WIDGET(remmina_unlock_dialog->dialog));
		remmina_unlock_dialog->dialog = NULL;
	} else {
		g_warning ("Passphrase is wrong, to reset it, you can edit the remmina.pref file by hand");
	}
}

static void remmina_unlock_cancel_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	remmina_unlock_dialog->retval = 0;
	gtk_widget_destroy(GTK_WIDGET(remmina_unlock_dialog->dialog));
	remmina_unlock_dialog->dialog = NULL;
}

gint remmina_unlock_new(GtkWindow *parent)
{
	TRACE_CALL(__func__);

	gdouble unlock_timeout;
	gdouble elapsed = 0.0;
	gboolean lock = TRUE;

	unlock_timeout = remmina_pref.unlock_timeout;


	remmina_unlock_dialog = g_new0(RemminaUnlockDialog, 1);
	remmina_unlock_dialog->retval = 1;

	if (timer == NULL)
		remmina_unlock_timer_init();
	if (timer != NULL)
		elapsed = g_timer_elapsed(timer, NULL);
	if (((int)unlock_timeout - elapsed) < 0) lock = TRUE;
	if (((int)unlock_timeout - elapsed) >= 0) lock = FALSE;
	/* timer & timout = 0 */
	if (timer != NULL && (int)unlock_timeout == 0) lock = FALSE;
	/* first time and timout = 30 */
	if (isinit == 0 && (int)unlock_timeout >= 0) {
		lock = TRUE;
		isinit = TRUE;
	}
	/* first time and timout = 0 */
	if (isinit == 0 && (int)unlock_timeout == 0) {
		lock = TRUE;
		isinit = TRUE;
	}
	g_info("Based on settings and current status, the unlock dialog is set to %d", lock);

	remmina_unlock_dialog->builder = remmina_public_gtk_builder_new_from_file("remmina_unlock.glade");
	remmina_unlock_dialog->dialog = GTK_DIALOG(gtk_builder_get_object(remmina_unlock_dialog->builder, "RemminaUnlockDialog"));
	if (parent)
		gtk_window_set_transient_for(GTK_WINDOW(remmina_unlock_dialog->dialog), parent);

	remmina_unlock_dialog->entry_unlock = GTK_ENTRY(GET_OBJ("entry_unlock"));
	gtk_entry_set_activates_default(GTK_ENTRY(remmina_unlock_dialog->entry_unlock), TRUE);
	remmina_unlock_dialog->button_unlock = GTK_BUTTON(GET_OBJ("button_unlock"));
	gtk_widget_set_can_default(GTK_WIDGET(remmina_unlock_dialog->button_unlock), TRUE);
	gtk_widget_grab_default(GTK_WIDGET(remmina_unlock_dialog->button_unlock));
	remmina_unlock_dialog->button_unlock_cancel = GTK_BUTTON(GET_OBJ("button_unlock_cancel"));

	g_signal_connect(remmina_unlock_dialog->button_unlock, "clicked",
			 G_CALLBACK(remmina_unlock_unlock_clicked), (gpointer)remmina_unlock_dialog);
	g_signal_connect(remmina_unlock_dialog->button_unlock_cancel, "clicked",
			 G_CALLBACK(remmina_unlock_cancel_clicked), (gpointer)remmina_unlock_dialog);
	g_signal_connect (remmina_unlock_dialog->dialog, "close",
                  G_CALLBACK (remmina_unlock_cancel_clicked), (gpointer)remmina_unlock_dialog);

	/* Connect signals */
	gtk_builder_connect_signals(remmina_unlock_dialog->builder, NULL);

	if (remmina_pref_get_boolean("use_master_password")
			&& (g_strcmp0(remmina_pref_get_value("unlock_password"), "") != 0)
			&& lock != 0)
		gtk_dialog_run(remmina_unlock_dialog->dialog);
	return(remmina_unlock_dialog->retval);
}

#else
gint remmina_unlock_new(GtkWindow *parent)
{
	return 1;
}
#endif
