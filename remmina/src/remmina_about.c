/*  See LICENSE and COPYING files for copyright and license details. */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remmina_about.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

/* Show the about dialog from the file ui/remmina_about.glade */
void remmina_about_open(GtkWindow *parent)
{
	TRACE_CALL("remmina_about_open");
	GtkBuilder *builder = remmina_public_gtk_builder_new_from_file("remmina_about.glade");
	GtkDialog *dialog = GTK_DIALOG (gtk_builder_get_object(builder, "dialog_remmina_about"));

	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), _("translator-credits"));

	if (parent)
	{
		gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	}

	g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_window_present(GTK_WINDOW(dialog));

	g_object_unref(G_OBJECT(builder));
}
