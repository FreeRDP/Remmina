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
#include "config.h"
#include "remmina_widget_pool.h"
#include "remmina_about.h"

void remmina_about_open(GtkWindow* parent)
{
	const gchar* authors[] =
	{
		N_("Maintainers:"),
		"Vic Lee <llyzs@163.com>",
		"",
		N_("Contributors:"),
		"Alex Chateau <ash@zednet.lv>",
		"Alexander Logvinov <avl@logvinov.com>",
		"Antenore Gatta <antenore@simbiosi.org>",
		"Fabio Castelli (Muflone) <muflone@vbsimple.net>",
		"Giovanni Panozzo <giovanni@panozzo.it>",
		"Harun Trefry <aihtdikh@gmail.com>",
		"Nikolay Botev <bono8106@gmail.com>",
		NULL
	};

	const gchar* artists[] =
	{
		"Martin Lettner <m.lettner@gmail.com>",
		NULL
	};

	const gchar* licenses[] =
	{
		N_("Remmina is free software; you can redistribute it and/or modify "
		"it under the terms of the GNU General Public License as published by "
		"the Free Software Foundation; either version 2 of the License, or "
		"(at your option) any later version."),
		N_("Remmina is distributed in the hope that it will be useful, "
		"but WITHOUT ANY WARRANTY; without even the implied warranty of "
		"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
		"GNU General Public License for more details."), N_(
		"You should have received a copy of the GNU General Public License "
		"along with this program; if not, write to the Free Software "
		"Foundation, Inc., 59 Temple Place, Suite 330, "
		"Boston, MA 02111-1307, USA.")
	};

	gchar* license = g_strjoin("\n\n", _(licenses[0]), _(licenses[1]), _(licenses[2]), NULL);

	GtkWidget* dialog;
	dialog = gtk_about_dialog_new();

	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(dialog), "Remmina");
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), "Copyright (C) 2009-2012 Vic Lee");
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), _("The GTK+ Remote Desktop Client")), gtk_about_dialog_set_license(
			GTK_ABOUT_DIALOG(dialog), license);
	gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(dialog), TRUE);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog), "http://freerdp.github.io/Remmina/");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(dialog), artists);
	gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(dialog), _("translator-credits"));
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(dialog), "remmina");

	if (parent)
	{
		gtk_window_set_transient_for(GTK_WINDOW(dialog), parent);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(dialog), TRUE);
	}

	g_signal_connect(dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_window_present(GTK_WINDOW(dialog));

	remmina_widget_pool_register(dialog);

	g_free(license);
}
