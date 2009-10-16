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
 

#ifndef __REMMINASSH_H__
#define __REMMINASSH_H__

#include "config.h"

#ifdef HAVE_LIBSSH

#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <pthread.h>
#include "remminafile.h"
#include "remminainitdialog.h"

G_BEGIN_DECLS

/* ----------------- SSH Base --------------- */

#define REMMINA_SSH(a) ((RemminaSSH*)a)

typedef struct _RemminaSSH
{
    ssh_session session;
    gboolean authenticated;

    gchar *server;
    guint port;
    gchar *user;
    gint auth;
    gchar *password;
    gchar *pubkeyfile;
    gchar *privkeyfile;

    gchar *charset;
    gchar *error;

    pthread_mutex_t ssh_mutex;
} RemminaSSH;

gchar* remmina_ssh_identity_path (const gchar *id);

/* Auto-detect commonly used private key identities */
gchar* remmina_ssh_find_identity (void);

/* Initialize the ssh object */
gboolean remmina_ssh_init_from_file (RemminaSSH *ssh, RemminaFile *remminafile);

/* Initialize the SSH session */
gboolean remmina_ssh_init_session (RemminaSSH *ssh);

/* Authenticate SSH session */
/* -1: Require password; 0: Failed; 1: Succeeded */
gint remmina_ssh_auth (RemminaSSH *ssh, const gchar *password);

/* -1: Cancelled; 0: Failed; 1: Succeeded */
gint remmina_ssh_auth_gui (RemminaSSH *ssh, RemminaInitDialog *dialog, gboolean threaded);

/* Error handling */
#define remmina_ssh_has_error(ssh) (((RemminaSSH*)ssh)->error!=NULL)
void remmina_ssh_set_error (RemminaSSH *ssh, const gchar *fmt);
#define remmina_ssh_set_application_error(ssh,msg) ((RemminaSSH*)ssh)->error = g_strdup (msg);

/* Converts a string to/from UTF-8, or simply duplicate it if no conversion */
gchar* remmina_ssh_convert (RemminaSSH *ssh, const gchar *from);
gchar* remmina_ssh_unconvert (RemminaSSH *ssh, const gchar *from);

void remmina_ssh_free (RemminaSSH *ssh);

/* ------------------- SSH Tunnel ---------------------- */
typedef struct _RemminaSSHTunnel RemminaSSHTunnel;
typedef struct _RemminaSSHTunnelBuffer RemminaSSHTunnelBuffer;

typedef gboolean (*RemminaSSHTunnelCallback) (RemminaSSHTunnel*, gpointer);

enum
{
    REMMINA_SSH_TUNNEL_OPEN,
    REMMINA_SSH_TUNNEL_X11,
    REMMINA_SSH_TUNNEL_XPORT
};

struct _RemminaSSHTunnel
{
    RemminaSSH ssh;

    gint tunnel_type;

    ssh_channel *channels;
    gint *sockets;
    RemminaSSHTunnelBuffer **socketbuffers;
    gint num_channels;
    gint max_channels;

    pthread_t thread;
    gboolean running;

    gchar *buffer;
    gint buffer_len;
    ssh_channel *channels_out;

    gint server_sock;
    gchar *dest;
    gint port;

    gint remotedisplay;
    gboolean bindlocalhost;
    gchar *localdisplay;

    RemminaSSHTunnelCallback init_func;
    RemminaSSHTunnelCallback connect_func;
    RemminaSSHTunnelCallback disconnect_func;
    gpointer callback_data;
};

/* Create a new SSH Tunnel session and connects to the SSH server */
RemminaSSHTunnel* remmina_ssh_tunnel_new_from_file (RemminaFile *remminafile);

/* Open the tunnel. A new thread will be started and listen on a local port.
 * dest: The host:port of the remote destination
 * local_port: The listening local port for the tunnel
 */
gboolean remmina_ssh_tunnel_open (RemminaSSHTunnel *tunnel, const gchar *dest, gint local_port);

/* Accept the X11 tunnel. A new thread will be started and connect to local display.
 * cmd: The remote X11 application to be executed
 */
gboolean remmina_ssh_tunnel_x11 (RemminaSSHTunnel *tunnel, const gchar *cmd);

/* start X Port Forwarding */
gboolean remmina_ssh_tunnel_xport (RemminaSSHTunnel *tunnel, gint display, gboolean bindlocalhost);

/* Tells if the tunnel is terminated after start */
gboolean remmina_ssh_tunnel_terminated (RemminaSSHTunnel *tunnel);

/* Free the tunnel */
void remmina_ssh_tunnel_free (RemminaSSHTunnel *tunnel);

/*----------------------- SFTP ------------------------*/

typedef struct _RemminaSFTP
{
    RemminaSSH ssh;

    sftp_session sftp_sess;
} RemminaSFTP;

/* Create a new SFTP session object from RemminaFile */
RemminaSFTP* remmina_sftp_new_from_file (RemminaFile *remminafile);

/* Create a new SFTP session object from existing SSH session */
RemminaSFTP* remmina_sftp_new_from_ssh (RemminaSSH *ssh);

/* open the SFTP session, assuming the session already authenticated */
gboolean remmina_sftp_open (RemminaSFTP *sftp);

/* Free the SFTP session */
void remmina_sftp_free (RemminaSFTP *sftp);

/*----------------------- SSH Terminal ------------------------*/

typedef struct _RemminaSSHTerminal
{
    RemminaSSH ssh;

    gint master;
    gint slave;
    pthread_t thread;
    GtkWidget *window;
    gboolean closed;
} RemminaSSHTerminal;

/* Create a new SSH Terminal session object from existing SSH session */
RemminaSSHTerminal* remmina_ssh_terminal_new_from_ssh (RemminaSSH *ssh);

/* open the SSH Terminal (init -> auth -> term) */
gboolean remmina_ssh_terminal_open (RemminaSSHTerminal *term);

G_END_DECLS

#else

#define RemminaSSH void
#define RemminaSSHTunnel void
#define RemminaSFTP void
#define RemminaTerminal void
typedef void (*RemminaSSHTunnelCallback) (void);

#endif /* HAVE_LIBSSH */

#endif  /* __REMMINASSH_H__  */

