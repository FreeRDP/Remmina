/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee 
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
 

#ifndef __REMMINASFTPCLIENT_H__
#define __REMMINASFTPCLIENT_H__

#include "config.h"

#ifdef HAVE_LIBSSH

#include "remminafile.h"
#include "remminaftpclient.h"
#include "remminassh.h"

G_BEGIN_DECLS

#define REMMINA_TYPE_SFTP_CLIENT               (remmina_sftp_client_get_type ())
#define REMMINA_SFTP_CLIENT(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_SFTP_CLIENT, RemminaSFTPClient))
#define REMMINA_SFTP_CLIENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_SFTP_CLIENT, RemminaSFTPClientClass))
#define REMMINA_IS_SFTP_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_SFTP_CLIENT))
#define REMMINA_IS_SFTP_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_SFTP_CLIENT))
#define REMMINA_SFTP_CLIENT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_SFTP_CLIENT, RemminaSFTPClientClass))

typedef struct _RemminaSFTPClient
{
    RemminaFTPClient client;

    RemminaSFTP *sftp;

    pthread_t thread;
    gint taskid;
    gboolean thread_abort;
} RemminaSFTPClient;

typedef struct _RemminaSFTPClientClass
{
    RemminaFTPClientClass parent_class;

} RemminaSFTPClientClass;

GType remmina_sftp_client_get_type (void) G_GNUC_CONST;

GtkWidget* remmina_sftp_client_new (void);

void remmina_sftp_client_open (RemminaSFTPClient *client, RemminaSFTP *sftp);

G_END_DECLS

#endif  /* HAVE_LIBSSH */

#endif  /* __REMMINASFTPCLIENT_H__  */

