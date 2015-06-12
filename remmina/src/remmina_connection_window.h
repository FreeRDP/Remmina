/*  See LICENSE and COPYING files for copyright and license details. */

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

