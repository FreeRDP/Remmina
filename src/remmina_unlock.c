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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib.h>
#include <glib/gprintf.h>

#include "config.h"
#include "remmina_sodium.h"
#include "remmina_pref.h"
#include "remmina_log.h"
#include "remmina_unlock.h"
#include "remmina_passwd.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

#if SODIUM_VERSION_INT >= 90200
static RemminaUnlockDialog *remmina_unlock_dialog;
#define GET_OBJ(object_name) gtk_builder_get_object(remmina_unlock_dialog->builder, object_name)

GTimer *timer;
gboolean unlocked;

static void remmina_unlock_timer_init()
{
	TRACE_CALL(__func__);

	timer = g_timer_new();
	REMMINA_DEBUG("Validity timer for Remmina password started");
}

static void remmina_unlock_timer_reset(gpointer user_data)
{
	TRACE_CALL(__func__);

	g_timer_reset(timer);
	REMMINA_DEBUG("Validity timer for Remmina password reset");
}

void remmina_unlock_timer_destroy()
{
	TRACE_CALL(__func__);

	g_timer_destroy(timer);
}

static void remmina_unlock_unlock_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	//g_timer_reset(NULL);

	gchar *unlock_password;
	const gchar *entry_passwd;
	gint rc;

	unlock_password = remmina_pref_get_value("unlock_password");
	entry_passwd = gtk_entry_get_text(remmina_unlock_dialog->entry_unlock);
	rc = remmina_sodium_pwhash_str_verify(unlock_password, entry_passwd);
	//REMMINA_DEBUG("remmina_sodium_pwhash_str_verify returned %i", rc);

	if (rc == 0) {
		REMMINA_DEBUG("Password verified");
		//unlocked = FALSE;
		remmina_unlock_dialog->retval = TRUE;
	} else {
		REMMINA_WARNING ("The password is wrong. Reset it by editing the remmina.pref file by hand");
		remmina_unlock_timer_reset(NULL);
		remmina_unlock_dialog->retval = FALSE;
	}
	gtk_dialog_response(remmina_unlock_dialog->dialog, GTK_RESPONSE_ACCEPT);
}

static void remmina_unlock_cancel_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	gtk_dialog_response(remmina_unlock_dialog->dialog, GTK_RESPONSE_CANCEL);
}

gint remmina_unlock_new(GtkWindow *parent)
{
	TRACE_CALL(__func__);

	gdouble unlock_timeout;
	gdouble elapsed = 0.0;
	gboolean lock = FALSE;
	gboolean rc;

	/* We don't have a timer, so this is the first time
	 * or the timer has been destroyed
	 */
	if (timer == NULL) {
		remmina_unlock_timer_init();
		unlocked = FALSE;
	}

	/* We have a timer, we get the elapsed time since its start */
	if (timer != NULL)
		elapsed = g_timer_elapsed(timer, NULL);

	/* We stop the timer while we enter the password) */
	if (timer) g_timer_stop(timer);

	unlock_timeout = remmina_pref.unlock_timeout;
	REMMINA_DEBUG ("unlock_timeout = %f", unlock_timeout);
	REMMINA_DEBUG ("elapsed = %f", elapsed);
	REMMINA_DEBUG ("INT unlock_timeout = %d", (int)unlock_timeout);
	REMMINA_DEBUG ("INT elapsed = %d", (int)elapsed);
	/* always LOCK and ask password */
	if ((elapsed) && ((int)unlock_timeout - elapsed) <= 0) {
		unlocked = FALSE;
		remmina_unlock_timer_reset(NULL);
	}
	/* We don't lock as it has been already requested */
	//if (!unlocked && ((int)unlock_timeout - elapsed) > 0) unlocked = TRUE;

	REMMINA_DEBUG("Based on settings and current status, the unlocking dialog is set to %d", unlocked);

	if (unlocked) {
		REMMINA_DEBUG ("No need to request a password");
		rc = TRUE;
		return rc;
	} else {

		remmina_unlock_dialog = g_new0(RemminaUnlockDialog, 1);
		remmina_unlock_dialog->retval = FALSE;

		remmina_unlock_dialog->builder = remmina_public_gtk_builder_new_from_resource("/org/remmina/Remmina/src/../data/ui/remmina_unlock.glade");
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

		 g_object_set_data_full (G_OBJECT(remmina_unlock_dialog->dialog), "builder", remmina_unlock_dialog->builder, g_object_unref);

		gchar *unlock_password = NULL;
		unlock_password = g_strdup(remmina_pref_get_value("unlock_password"));
		//REMMINA_DEBUG ("Password from preferences is: %s", unlock_password);
		if ((unlock_password == NULL) || (g_strcmp0(unlock_password, "") == 0)) {
			if (remmina_passwd (GTK_WINDOW(remmina_unlock_dialog->dialog), &unlock_password)) {
				//REMMINA_DEBUG ("Password is: %s", unlock_password);
				remmina_pref_set_value("unlock_password", g_strdup(unlock_password));
				remmina_unlock_dialog->retval = TRUE;
			} else {
				remmina_unlock_dialog->retval = FALSE;
			}
		}

		int result = GTK_RESPONSE_NONE;
		if (g_strcmp0(unlock_password, "") != 0) {
			result = gtk_dialog_run (GTK_DIALOG (remmina_unlock_dialog->dialog));
		} else
			remmina_unlock_dialog->retval = lock;

		switch (result)
		{
			case GTK_RESPONSE_ACCEPT:
				if (!remmina_unlock_dialog->retval)
					REMMINA_DEBUG ("Wrong password");
				else {
					REMMINA_DEBUG ("Correct password. Unlocking…");
					unlocked = TRUE;
				}
				REMMINA_DEBUG ("retval: %d", remmina_unlock_dialog->retval);
				break;
			default:
				//unlocked = FALSE;
				remmina_unlock_dialog->retval = FALSE;
				remmina_unlock_timer_destroy ();
				REMMINA_DEBUG ("Password not requested. Return value: %d", remmina_unlock_dialog->retval);
				break;
		}

		rc = remmina_unlock_dialog->retval;

		g_free(unlock_password), unlock_password = NULL;
		gtk_widget_destroy(GTK_WIDGET(remmina_unlock_dialog->dialog));
		remmina_unlock_dialog->dialog = NULL;
	}
	if (timer) g_timer_start(timer);
	return(rc);
}

#else
gint remmina_unlock_new(GtkWindow *parent)
{
	return 1;
}
#endif
