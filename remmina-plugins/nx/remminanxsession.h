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

#ifndef __REMMINANXSESSION_H__
#define __REMMINANXSESSION_H__

G_BEGIN_DECLS

typedef struct _RemminaNXSession RemminaNXSession;

typedef gboolean (*RemminaNXPassphraseCallback) (gchar **passphrase, gpointer userdata);

RemminaNXSession* remmina_nx_session_new (void);

void remmina_nx_session_free (RemminaNXSession *nx);

gboolean remmina_nx_session_has_error (RemminaNXSession *nx);

const gchar* remmina_nx_session_get_error (RemminaNXSession *nx);

void remmina_nx_session_set_encryption (RemminaNXSession *nx, gint encryption);

void remmina_nx_session_set_localport (RemminaNXSession *nx, gint localport);

gboolean remmina_nx_session_open (RemminaNXSession *nx, const gchar *server, guint port,
    const gchar *private_key_file, RemminaNXPassphraseCallback passphrase_func, gpointer userdata);

gboolean remmina_nx_session_login (RemminaNXSession *nx, const gchar *username, const gchar *password);

void remmina_nx_session_add_parameter (RemminaNXSession *nx, const gchar *name, const gchar *valuefmt, ...);

gboolean remmina_nx_session_start (RemminaNXSession *nx);

gboolean remmina_nx_session_tunnel_open (RemminaNXSession *nx);

gboolean remmina_nx_session_invoke_proxy (RemminaNXSession *nx, const gint display,
    GChildWatchFunc exit_func, gpointer user_data);

void remmina_nx_session_bye (RemminaNXSession *nx);

G_END_DECLS

#endif  /* __REMMINANXSESSION_H__  */

