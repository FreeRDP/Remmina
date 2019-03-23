/*
 * Remmina - The GTK+ Remote Desktop Client
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

#include "config.h"
#include "remmina_pref.h"
#include "remmina_unlock.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

#ifdef __linux__
static RemminaUnlockDialog *remmina_unlock_dialog;
#define GET_OBJ(object_name) gtk_builder_get_object(remmina_unlock_dialog->builder, object_name)

static void remmina_unlock_timer_init (gpointer user_data)
{
	TRACE_CALL(__func__);

	remmina_unlock_dialog->timer = g_timer_new();
}

static void remmina_unlock_timer_reset (gpointer user_data)
{
	TRACE_CALL(__func__);

	g_timer_reset(remmina_unlock_dialog->timer);
}

static void remmina_unlock_timer_destroy (gpointer user_data)
{
	TRACE_CALL(__func__);

	g_timer_destroy(remmina_unlock_dialog->timer);
}

static void remmina_button_unlock_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	g_timer_reset(remmina_unlock_dialog->timer);
	gtk_widget_destroy(GTK_WIDGET(remmina_unlock_dialog->dialog));
	remmina_unlock_dialog->dialog = NULL;
}

static void remmina_button_unlock_cancel_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	gtk_widget_destroy(GTK_WIDGET(remmina_unlock_dialog->dialog));
	remmina_unlock_dialog->dialog = NULL;
}

void remmina_unlock_new(GtkWindow *parent)
{
    TRACE_CALL(__func__);

    remmina_unlock_dialog = g_new0(RemminaUnlockDialog, 1);
    //remmina_unlock_dialog->priv = g_new0(RemminaUnlockDialogPriv, 1);

    //if (remmina_unlock_dialog->unlock_init)
    remmina_unlock_dialog->builder = remmina_public_gtk_builder_new_from_file("remmina_unlock.glade");
    remmina_unlock_dialog->dialog = GTK_DIALOG(gtk_builder_get_object(remmina_unlock_dialog->builder, "RemminaUnlockDialog"));
    if (parent)
        gtk_window_set_transient_for(GTK_WINDOW(remmina_unlock_dialog->dialog), parent);

    remmina_unlock_dialog->entry_unlock = GTK_ENTRY(GET_OBJ("entry_unlock"));
    remmina_unlock_dialog->button_unlock = GTK_BUTTON(GET_OBJ("button_unlock"));
    remmina_unlock_dialog->button_unlock_cancel = GTK_BUTTON(GET_OBJ("button_unlock_cancel"));

    g_signal_connect(remmina_unlock_dialog->button_unlock, "clicked",
            G_CALLBACK(remmina_button_unlock_clicked), (gpointer)remmina_unlock_dialog);
    g_signal_connect(remmina_unlock_dialog->button_unlock_cancel, "clicked",
            G_CALLBACK(remmina_button_unlock_cancel_clicked), (gpointer)remmina_unlock_dialog);

    /* Connect signals */
    gtk_builder_connect_signals(remmina_unlock_dialog->builder, NULL);

    if (remmina_pref_get_boolean("use_master_password"))
	    gtk_dialog_run(remmina_unlock_dialog->dialog);
}

#else
void remmina_unlock_new(...)
{
	TRACE_CALL(__func__);
}
#endif
