/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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
#include "remminawidgetpool.h"
#include "remminaui.h"

GtkWidget*
remmina_ui_confirm (RemminaUIConfirmType type, const gchar *s, GCallback callback, gpointer data)
{
    GtkWidget *dialog = NULL;

    switch (type)
    {
    case REMMINA_UI_CONFIRM_TYPE_SHARE_DESKTOP:
        dialog = gtk_message_dialog_new (NULL,
            GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
            _("%s wants to share his/her desktop.\nDo you accept the invitation?"), s);
        g_signal_connect (G_OBJECT (dialog), "response", callback, data);
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_window_set_title (GTK_WINDOW (dialog), _("Desktop sharing invitation"));
        remmina_widget_pool_register (dialog);
        gtk_widget_show (dialog);
        break;
    }
    return dialog;
}

