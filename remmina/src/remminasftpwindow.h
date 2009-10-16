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
 

#ifndef __REMMINASFTPWINDOW_H__
#define __REMMINASFTPWINDOW_H__

#include "config.h"

#ifdef HAVE_LIBSSH

#include "remminafile.h"
#include "remminaftpwindow.h"
#include "remminassh.h"

G_BEGIN_DECLS

#define REMMINA_TYPE_SFTP_WINDOW               (remmina_sftp_window_get_type ())
#define REMMINA_SFTP_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_SFTP_WINDOW, RemminaSFTPWindow))
#define REMMINA_SFTP_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_SFTP_WINDOW, RemminaSFTPWindowClass))
#define REMMINA_IS_SFTP_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_SFTP_WINDOW))
#define REMMINA_IS_SFTP_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_SFTP_WINDOW))
#define REMMINA_SFTP_WINDOW_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_SFTP_WINDOW, RemminaSFTPWindowClass))

typedef struct _RemminaSFTPWindow
{
    RemminaFTPWindow window;

    RemminaSFTP *sftp;

    pthread_t thread;
    gint taskid;
    gboolean thread_abort;
} RemminaSFTPWindow;

typedef struct _RemminaSFTPWindowClass
{
    RemminaFTPWindowClass parent_class;

} RemminaSFTPWindowClass;

GType remmina_sftp_window_get_type (void) G_GNUC_CONST;

/* Create a new SFTP window using an existing sftp session */
GtkWidget* remmina_sftp_window_new (RemminaSFTP *sftp);

/* Create a new SFTP window using an not-yet-initialized sftp session */
GtkWidget* remmina_sftp_window_new_init (RemminaSFTP *sftp);

/* Create a stand-alone SFTP window using a connection profile */
void remmina_sftp_window_open (RemminaFile *remminafile);

G_END_DECLS

#endif  /* HAVE_LIBSSH */

#endif  /* __REMMINASFTPWINDOW_H__  */

