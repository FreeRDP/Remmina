/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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
#include "config.h"
#include "remminaabout.h"

void
remmina_about_open (GtkWindow *parent)
{
    const gchar *authors[] =
    {
        N_("Maintainers:"),
        "Vic Lee <llyzs@163.com>",
        "",
        N_("Contributors:"),
        "Nikolay Botev <bono8106@gmail.com>",
        "Alex Chateau <ash@zednet.lv>",
        NULL
    };
    const gchar *licenses[] =
    {
        N_("Remmina is free software; you can redistribute it and/or modify "
           "it under the terms of the GNU General Public License as published by "
           "the Free Software Foundation; either version 2 of the License, or "
           "(at your option) any later version."),
        N_("Remmina is distributed in the hope that it will be useful, "
           "but WITHOUT ANY WARRANTY; without even the implied warranty of "
           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the "
           "GNU General Public License for more details."),
        N_("You should have received a copy of the GNU General Public License "
           "along with this program; if not, write to the Free Software "
           "Foundation, Inc., 59 Temple Place, Suite 330, "
           "Boston, MA 02111-1307, USA.")
    };
    gchar *license = g_strjoin ("\n\n", _(licenses[0]), _(licenses[1]), _(licenses[2]), NULL);

    gtk_show_about_dialog (parent,
        "program-name", "Remmina",
        "version", PACKAGE_VERSION,
        "comments", _("The GTK+ Remote Desktop Client"),
        "authors", authors,
        "translator-credits", _("translator-credits"),
        "copyright", "Copyright (C) 2009 - Vic Lee",
        "license", license,
        "wrap-license", TRUE,
        "logo-icon-name", "remmina",
        "website", "http://remmina.sourceforge.net",
        NULL);

    g_free (license);
}

