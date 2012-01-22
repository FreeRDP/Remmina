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

#ifndef __REMMINACONNECTIONWINDOW_H__
#define __REMMINACONNECTIONWINDOW_H__

#include "remmina_file.h"

G_BEGIN_DECLS

#define REMMINA_TYPE_CONNECTION_WINDOW               (remmina_connection_window_get_type ())
#define REMMINA_CONNECTION_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_CONNECTION_WINDOW, RemminaConnectionWindow))
#define REMMINA_CONNECTION_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_CONNECTION_WINDOW, RemminaConnectionWindowClass))
#define REMMINA_IS_CONNECTION_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_CONNECTION_WINDOW))
#define REMMINA_IS_CONNECTION_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_CONNECTION_WINDOW))
#define REMMINA_CONNECTION_WINDOW_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_CONNECTION_WINDOW, RemminaConnectionWindowClass))

typedef struct _RemminaConnectionWindowPriv RemminaConnectionWindowPriv;

typedef struct _RemminaConnectionWindow
{
	GtkWindow window;

	RemminaConnectionWindowPriv* priv;
} RemminaConnectionWindow;

typedef struct _RemminaConnectionWindowClass
{
	GtkWindowClass parent_class;
} RemminaConnectionWindowClass;

GType remmina_connection_window_get_type(void)
G_GNUC_CONST;

/* Open a new connection window for a .remmina file */
gboolean remmina_connection_window_open_from_filename(const gchar* filename);
/* Open a new connection window for a given RemminaFile struct. The struct will be freed after the call */
void remmina_connection_window_open_from_file(RemminaFile* remminafile);
GtkWidget* remmina_connection_window_open_from_file_full(RemminaFile* remminafile, GCallback disconnect_cb, gpointer data,
		guint* handler);

G_END_DECLS

#endif  /* __REMMINACONNECTIONWINDOW_H__  */

