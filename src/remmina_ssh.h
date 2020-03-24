/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#pragma once

#include "config.h"

#ifdef HAVE_LIBSSH

#define LIBSSH_STATIC 1
#include <libssh/libssh.h>
#include <libssh/callbacks.h>
#include <libssh/sftp.h>
#include <pthread.h>
#include "remmina_file.h"
#include "rcw.h"

G_BEGIN_DECLS

/*-----------------------------------------------------------------------------*
*                           SSH Base                                          *
*-----------------------------------------------------------------------------*/

#define REMMINA_SSH(a) ((RemminaSSH *)a)

typedef struct _RemminaSSH {
	ssh_session	session;
	ssh_callbacks	callback;
	gboolean	authenticated;

	gchar *		server;
	gint		port;
	gchar *		user;
	gint		auth;
	gchar *		password;
	gchar *		privkeyfile;

	gchar *		charset;
	const gchar *	kex_algorithms;
	gchar *		ciphers;
	gchar *		hostkeytypes;
	gchar *		proxycommand;
	gint		stricthostkeycheck;
	const gchar *	compression;

	gchar *		error;

	pthread_mutex_t ssh_mutex;

	gchar *		passphrase;

	gboolean	is_tunnel;
	gchar *		tunnel_entrance_host;
	gint		tunnel_entrance_port;

} RemminaSSH;

gchar *remmina_ssh_identity_path(const gchar *id);

/* Auto-detect commonly used private key identities */
gchar *remmina_ssh_find_identity(void);

/* Initialize the ssh object */
gboolean remmina_ssh_init_from_file(RemminaSSH *ssh, RemminaFile *remminafile, gboolean is_tunnel);

/* Initialize the SSH session */
gboolean remmina_ssh_init_session(RemminaSSH *ssh);

/* Authenticate SSH session */


enum remmina_ssh_auth_result {
	REMMINA_SSH_AUTH_SUCCESS,
	REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT,
	REMMINA_SSH_AUTH_USERCANCEL,
	REMMINA_SSH_AUTH_FATAL_ERROR
};

enum remmina_ssh_auth_result remmina_ssh_auth(RemminaSSH *ssh, const gchar *password, RemminaProtocolWidget *gp, RemminaFile *remminafile);

enum remmina_ssh_auth_result remmina_ssh_auth_gui(RemminaSSH *ssh, RemminaProtocolWidget *gp, RemminaFile *remminafile);

/* Error handling */
#define remmina_ssh_has_error(ssh) (((RemminaSSH *)ssh)->error != NULL)
void remmina_ssh_set_error(RemminaSSH *ssh, const gchar *fmt);
void remmina_ssh_set_application_error(RemminaSSH *ssh, const gchar *fmt, ...);

/* Converts a string to/from UTF-8, or simply duplicate it if no conversion */
gchar *remmina_ssh_convert(RemminaSSH *ssh, const gchar *from);
gchar *remmina_ssh_unconvert(RemminaSSH *ssh, const gchar *from);

void remmina_ssh_free(RemminaSSH *ssh);

/*-----------------------------------------------------------------------------*
*                           SSH Tunnel                                        *
*-----------------------------------------------------------------------------*/
typedef struct _RemminaSSHTunnel RemminaSSHTunnel;
typedef struct _RemminaSSHTunnelBuffer RemminaSSHTunnelBuffer;

typedef gboolean (*RemminaSSHTunnelCallback) (RemminaSSHTunnel *, gpointer);

enum {
	REMMINA_SSH_TUNNEL_OPEN,
	REMMINA_SSH_TUNNEL_X11,
	REMMINA_SSH_TUNNEL_XPORT,
	REMMINA_SSH_TUNNEL_REVERSE
};


struct _RemminaSSHTunnel {
	RemminaSSH			ssh;

	gint				tunnel_type;

	ssh_channel *			channels;
	gint *				sockets;
	RemminaSSHTunnelBuffer **	socketbuffers;
	gint				num_channels;
	gint				max_channels;

	ssh_channel			x11_channel;

	pthread_t			thread;
	gboolean			running;

	gchar *				buffer;
	gint				buffer_len;
	ssh_channel *			channels_out;

	gint				server_sock;
	gchar *				dest;
	gint				port;
	gint				localport;

	gint				remotedisplay;
	gboolean			bindlocalhost;
	gchar *				localdisplay;

	RemminaSSHTunnelCallback	init_func;
	RemminaSSHTunnelCallback	connect_func;
	RemminaSSHTunnelCallback	disconnect_func;
	gpointer			callback_data;

	RemminaSSHTunnelCallback	destroy_func;
	gpointer	destroy_func_callback_data;

};

/* Create a new SSH Tunnel session and connects to the SSH server */
RemminaSSHTunnel *remmina_ssh_tunnel_new_from_file(RemminaFile *remminafile);

/* Open the tunnel. A new thread will be started and listen on a local port.
 * dest: The host:port of the remote destination
 * local_port: The listening local port for the tunnel
 */
gboolean remmina_ssh_tunnel_open(RemminaSSHTunnel *tunnel, const gchar *host, gint port, gint local_port);

/* Cancel accepting any incoming tunnel request.
 * Typically called after the connection has already been establish.
 */
void remmina_ssh_tunnel_cancel_accept(RemminaSSHTunnel *tunnel);

/* Accept the X11 tunnel. A new thread will be started and connect to local display.
 * cmd: The remote X11 application to be executed
 */
gboolean remmina_ssh_tunnel_x11(RemminaSSHTunnel *tunnel, const gchar *cmd);

/* start X Port Forwarding */
gboolean remmina_ssh_tunnel_xport(RemminaSSHTunnel *tunnel, gboolean bindlocalhost);

/* start reverse tunnel. A new thread will be started and waiting for incoming connection.
 * port: the port listening on the remote server side.
 * local_port: the port listening on the local side. When connection on the server side comes
 *             in, it will connect to the local port and create the tunnel. The caller should
 *             start listening on the local port before calling it or in connect_func callback.
 */
gboolean remmina_ssh_tunnel_reverse(RemminaSSHTunnel *tunnel, gint port, gint local_port);

/* Tells if the tunnel is terminated after start */
gboolean remmina_ssh_tunnel_terminated(RemminaSSHTunnel *tunnel);

/* Free the tunnel */
void remmina_ssh_tunnel_free(RemminaSSHTunnel *tunnel);

/*-----------------------------------------------------------------------------*
*                           SSH sFTP                                          *
*-----------------------------------------------------------------------------*/

typedef struct _RemminaSFTP {
	RemminaSSH	ssh;

	sftp_session	sftp_sess;
} RemminaSFTP;

/* Create a new SFTP session object from RemminaFile */
RemminaSFTP *remmina_sftp_new_from_file(RemminaFile *remminafile);

/* Create a new SFTP session object from existing SSH session */
RemminaSFTP *remmina_sftp_new_from_ssh(RemminaSSH *ssh);

/* open the SFTP session, assuming the session already authenticated */
gboolean remmina_sftp_open(RemminaSFTP *sftp);

/* Free the SFTP session */
void remmina_sftp_free(RemminaSFTP *sftp);

/*-----------------------------------------------------------------------------*
*                           SSH Shell                                         *
*-----------------------------------------------------------------------------*/
typedef void (*RemminaSSHExitFunc) (gpointer data);

typedef struct _RemminaSSHShell {
	RemminaSSH		ssh;

	gint			master;
	gint			slave;
	gchar *			exec;
	pthread_t		thread;
	ssh_channel		channel;
	gboolean		closed;
	RemminaSSHExitFunc	exit_callback;
	gpointer		user_data;
} RemminaSSHShell;

/* Create a new SSH Shell session object from RemminaFile */
RemminaSSHShell *remmina_ssh_shell_new_from_file(RemminaFile *remminafile);

/* Create a new SSH Shell session object from existing SSH session */
RemminaSSHShell *remmina_ssh_shell_new_from_ssh(RemminaSSH *ssh);

/* open the SSH Shell, assuming the session already authenticated */
gboolean remmina_ssh_shell_open(RemminaSSHShell *shell, RemminaSSHExitFunc exit_callback, gpointer data);

/* Change the SSH Shell terminal size */
void remmina_ssh_shell_set_size(RemminaSSHShell *shell, gint columns, gint rows);

/* Free the SFTP session */
void remmina_ssh_shell_free(RemminaSSHShell *shell);

G_END_DECLS

#else

#define RemminaSSH void
#define RemminaSSHTunnel void
#define RemminaSFTP void
#define RemminaSSHShell void
typedef void (*RemminaSSHTunnelCallback)(void);

#endif /* HAVE_LIBSSH */
