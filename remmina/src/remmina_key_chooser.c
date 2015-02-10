/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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
#include "remmina_key_chooser.h"
#include "remmina/remmina_trace_calls.h"

G_DEFINE_TYPE( RemminaKeyChooser, remmina_key_chooser, GTK_TYPE_BUTTON)

static void remmina_key_chooser_class_init(RemminaKeyChooserClass *klass)
{
	TRACE_CALL("remmina_key_chooser_class_init");
}

static void remmina_key_chooser_update_label(RemminaKeyChooser *kc)
{
	TRACE_CALL("remmina_key_chooser_update_label");
	gchar *s;

	if (kc->keyval)
	{
		s = gdk_keyval_name(gdk_keyval_to_upper(kc->keyval));
	}
	else
	{
		s = _("<None>");
	}
	gtk_button_set_label(GTK_BUTTON(kc), s);
}

void remmina_key_chooser_set_keyval(RemminaKeyChooser *kc, guint keyval)
{
	TRACE_CALL("remmina_key_chooser_set_keyval");
	kc->keyval = keyval;
	remmina_key_chooser_update_label(kc);
}

static gboolean remmina_key_chooser_dialog_on_key_press(GtkWidget *widget, GdkEventKey *event, RemminaKeyChooser *kc)
{
	TRACE_CALL("remmina_key_chooser_dialog_on_key_press");
	remmina_key_chooser_set_keyval(kc, gdk_keyval_to_lower(event->keyval));
	gtk_dialog_response(GTK_DIALOG(gtk_widget_get_toplevel(widget)), GTK_RESPONSE_OK);
	return TRUE;
}

static void remmina_key_chooser_on_clicked(RemminaKeyChooser *kc, gpointer data)
{
	TRACE_CALL("remmina_key_chooser_on_clicked");
	GtkWidget *dialog;
	GtkWidget *eventbox;
	GtkWidget *widget;
	gint ret;

	dialog = gtk_dialog_new_with_buttons(_("Choose a new key"), GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(kc))),
			GTK_DIALOG_MODAL, _("_Cancel"), GTK_RESPONSE_CANCEL, _("_Remove"), GTK_RESPONSE_REJECT, NULL);

	eventbox = gtk_event_box_new();
	gtk_widget_show(eventbox);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), eventbox, TRUE, TRUE, 0);
	gtk_widget_add_events(eventbox, GDK_KEY_PRESS_MASK);
	g_signal_connect(G_OBJECT(eventbox), "key-press-event", G_CALLBACK(remmina_key_chooser_dialog_on_key_press), kc);
	gtk_widget_set_can_focus(eventbox, TRUE);

	widget = gtk_label_new(_("Please press the new key..."));
	gtk_misc_set_alignment(GTK_MISC(widget), 0.5, 0.5);
	gtk_widget_set_size_request(widget, 250, 150);
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(eventbox), widget);

	gtk_widget_grab_focus(eventbox);

	ret = gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);

	if (ret == GTK_RESPONSE_REJECT)
	{
		remmina_key_chooser_set_keyval(kc, 0);
	}
}

static void remmina_key_chooser_init(RemminaKeyChooser *kc)
{
	TRACE_CALL("remmina_key_chooser_init");
	remmina_key_chooser_set_keyval(kc, 0);

	g_signal_connect(G_OBJECT(kc), "clicked", G_CALLBACK(remmina_key_chooser_on_clicked), NULL);
}

GtkWidget*
remmina_key_chooser_new(guint keyval)
{
	TRACE_CALL("remmina_key_chooser_new");
	RemminaKeyChooser *kc;

	kc = REMMINA_KEY_CHOOSER(g_object_new(REMMINA_TYPE_KEY_CHOOSER, NULL));
	remmina_key_chooser_set_keyval(kc, keyval);

	return GTK_WIDGET(kc);
}

