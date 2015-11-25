/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2014-2015 Antenore Gatta
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
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 * If you modify file(s) with this exception, you may extend this exception
 * to your version of the file(s), but you are not obligated to do so.
 * If you do not wish to do so, delete this exception statement from your
 * version.
 * If you delete this exception statement from all source
 * files in the program, then also delete it here.
 *
 */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <webkit/webkit.h>
#include <stdlib.h>
#include <ctype.h>
#include "config.h"
#include "remmina_survey.h"
#include "remmina_pref.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

static gchar* survey_uri = REMMINA_SURVEY_URI;

static RemminaSurveyDialog *remmina_survey;

static WebKitWebView* web_view;

#define GET_OBJECT(object_name) gtk_builder_get_object(remmina_survey->builder, object_name)

void remmina_survey_dialog_on_close_clicked(GtkWidget *widget, RemminaSurveyDialog *dialog)
{
	TRACE_CALL("remmina_survey_dialog_on_close_clicked");
	gtk_widget_destroy(GTK_WIDGET(remmina_survey->dialog));
}

void remmina_survey_dialog_on_submit_clicked(GtkWidget *widget, RemminaSurveyDialog *dialog)
{
	TRACE_CALL("remmina_survey_dialog_on_submit_clicked");
}

/* Show the preliminary survey dialog when remmina start */
void remmina_survey_on_startup(GtkWindow *parent)
{
	TRACE_CALL("remmina_survey");

	GtkWidget *dialog, *check;

	dialog = gtk_message_dialog_new(GTK_WINDOW (parent),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					"%s",
					/* translators: Primary message of a dialog used to notify the user about the survey */
					_("We are conducting a user survey\n would you like to take it now?"));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
			/* translators: Secondary text of a dialog used to notify the user about the survey */
			_("If not, you can always find it in the Help menu."));

	check = gtk_check_button_new_with_mnemonic (_("_Do not show this dialog again"));
	gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
			check, FALSE, FALSE, 0);
	gtk_widget_set_halign (check, GTK_ALIGN_START);
	gtk_widget_set_margin_start (check, 6);
	gtk_widget_show (check);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
	{
		remmina_survey_start(parent);
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)))
	{
		/* Save survey option */
		remmina_pref.survey = FALSE;
		remmina_pref_save();
	}

	gtk_widget_destroy(dialog);
}

void remmina_survey_start(GtkWindow *parent)
{
	TRACE_CALL("remmina_survey_start");
	remmina_survey = g_new0(RemminaSurveyDialog, 1);

	remmina_survey->builder = remmina_public_gtk_builder_new_from_file("remmina_survey.glade");
	remmina_survey->dialog
		= GTK_DIALOG (gtk_builder_get_object(remmina_survey->builder, "dialog_remmina_survey"));
	remmina_survey->scrolledwindow
		= GTK_SCROLLED_WINDOW(GET_OBJECT("scrolledwindow"));
	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	if (parent)
	{
		gtk_window_set_transient_for(GTK_WINDOW(remmina_survey->dialog), parent);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(remmina_survey->dialog), TRUE);
	}

	g_signal_connect(remmina_survey->dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_window_present(GTK_WINDOW(remmina_survey->dialog));

	gtk_container_add(GTK_CONTAINER(remmina_survey->scrolledwindow), GTK_WIDGET(web_view));
	gtk_widget_show(GTK_WIDGET(web_view));
	webkit_web_view_load_uri(web_view, survey_uri);
	g_object_unref(G_OBJECT(remmina_survey->builder));
}
