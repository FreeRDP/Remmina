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
 */

#include "config.h"

#ifdef HAVE_LIBSSH

/* Define this before stdlib.h to have posix_openpt */
#define _XOPEN_SOURCE 600

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <pthread.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#include "remmina_public.h"
#include "remmina_log.h"
#include "remmina_ssh.h"

/*************************** SSH Base *********************************/

#define LOCK_SSH(ssh) pthread_mutex_lock (&REMMINA_SSH (ssh)->ssh_mutex);
#define UNLOCK_SSH(ssh) pthread_mutex_unlock (&REMMINA_SSH (ssh)->ssh_mutex);

static const gchar *common_identities[] =
{
	".ssh/id_rsa",
	".ssh/id_dsa",
	".ssh/identity",
	NULL
};

gchar*
remmina_ssh_identity_path (const gchar *id)
{
	if (id == NULL) return NULL;
	if (id[0] == '/') return g_strdup (id);
	return g_strdup_printf("%s/%s", g_get_home_dir (), id);
}

gchar*
remmina_ssh_find_identity (void)
{
	gchar *path;
	gint i;

	for (i = 0; common_identities[i]; i++)
	{
		path = remmina_ssh_identity_path (common_identities[i]);
		if (g_file_test (path, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
		{
			return path;
		}
		g_free(path);
	}
	return NULL;
}

void
remmina_ssh_set_error (RemminaSSH *ssh, const gchar *fmt)
{
	const gchar *err;

	err = ssh_get_error (ssh->session);
	ssh->error = g_strdup_printf(fmt, err);
}

void
remmina_ssh_set_application_error (RemminaSSH *ssh, const gchar *fmt, ...)
{
	va_list args;

	va_start (args, fmt);
	ssh->error = g_strdup_vprintf (fmt, args);
	va_end (args);
}

static gint
remmina_ssh_auth_password (RemminaSSH *ssh)
{
	gint ret;
	gint authlist;
	gint n;
	gint i;

	if (ssh->authenticated) return 1;
	if (ssh->password == NULL) return -1;

	authlist = ssh_userauth_list (ssh->session, NULL);
	if (authlist & SSH_AUTH_METHOD_INTERACTIVE)
	{
		while ((ret = ssh_userauth_kbdint (ssh->session, NULL, NULL)) == SSH_AUTH_INFO)
		{
			n = ssh_userauth_kbdint_getnprompts (ssh->session);
			for (i = 0; i < n; i++)
			{
				ssh_userauth_kbdint_setanswer(ssh->session, i, ssh->password);
			}
		}
	}
	else
	{
		ret = ssh_userauth_password (ssh->session, NULL, ssh->password);
	}
	if (ret != SSH_AUTH_SUCCESS)
	{
		remmina_ssh_set_error (ssh, _("SSH password authentication failed: %s"));
		return 0;
	}

	ssh->authenticated = TRUE;
	return 1;
}

static gint
remmina_ssh_auth_pubkey (RemminaSSH *ssh)
{
	gint ret;
	ssh_string pubkey;
	ssh_private_key privkey;
	gint keytype;

	if (ssh->authenticated) return 1;

	if (ssh->pubkeyfile == NULL || ssh->privkeyfile == NULL)
	{
		ssh->error = g_strdup_printf(_("SSH public key authentication failed: %s"),
				_("SSH Key file not yet set."));
		return 0;
	}

	pubkey = publickey_from_file (ssh->session, ssh->pubkeyfile, &keytype);
	if (pubkey == NULL)
	{
		remmina_ssh_set_error (ssh, _("SSH public key authentication failed: %s"));
		return 0;
	}

	privkey = privatekey_from_file (ssh->session, ssh->privkeyfile, keytype, _
			(ssh->password ? ssh->password : ""));
	if (privkey == NULL)
	{
		string_free(pubkey);
		if (ssh->password == NULL || ssh->password[0] == '\0') return -1;

		remmina_ssh_set_error (ssh, _("SSH public key authentication failed: %s"));
		return 0;
	}

	ret = ssh_userauth_pubkey (ssh->session, NULL, pubkey, privkey);
	string_free(pubkey);
	privatekey_free (privkey);

	if (ret != SSH_AUTH_SUCCESS)
	{
		remmina_ssh_set_error (ssh, _("SSH public key authentication failed: %s"));
		return 0;
	}

	ssh->authenticated = TRUE;
	return 1;
}

static gint
remmina_ssh_auth_auto_pubkey (RemminaSSH* ssh)
{
	gint ret;
	ret = ssh_userauth_autopubkey (ssh->session, "");

	if (ret != SSH_AUTH_SUCCESS)
	{
		remmina_ssh_set_error (ssh, _("SSH automatic public key authentication failed: %s"));
		return 0;
	}

	ssh->authenticated = TRUE;
	return 1;
}

gint
remmina_ssh_auth (RemminaSSH *ssh, const gchar *password)
{
	/* Check known host again to ensure it's still the original server when user forks
	 a new session from existing one */
	if (ssh_is_server_known (ssh->session) != SSH_SERVER_KNOWN_OK)
	{
		remmina_ssh_set_application_error (ssh, "SSH public key has changed!");
		return 0;
	}

	if (password)
	{
		g_free(ssh->password);
		ssh->password = g_strdup (password);
	}

	switch (ssh->auth)
	{

		case SSH_AUTH_PASSWORD:
		return remmina_ssh_auth_password (ssh);

		case SSH_AUTH_PUBLICKEY:
		return remmina_ssh_auth_pubkey (ssh);

		case SSH_AUTH_AUTO_PUBLICKEY:
		return remmina_ssh_auth_auto_pubkey (ssh);

		default:
		return 0;
	}
}

gint
remmina_ssh_auth_gui (RemminaSSH *ssh, RemminaInitDialog *dialog, gboolean threaded)
{
	gchar *tips;
	gchar *keyname;
	gint ret;
	gint len;
	guchar *pubkey;

	/* Check if the server's public key is known */
	ret = ssh_is_server_known (ssh->session);
	switch (ret)
	{
		case SSH_SERVER_KNOWN_OK:
		break;

		case SSH_SERVER_NOT_KNOWN:
		case SSH_SERVER_FILE_NOT_FOUND:
		case SSH_SERVER_KNOWN_CHANGED:
		case SSH_SERVER_FOUND_OTHER:
		len = ssh_get_pubkey_hash (ssh->session, &pubkey);
		if (len < 0)
		{
			remmina_ssh_set_error (ssh, "SSH pubkey hash failed: %s");
			return 0;
		}
		keyname = ssh_get_hexa (pubkey, len);

		if (threaded) gdk_threads_enter();
		if (ret == SSH_SERVER_NOT_KNOWN || ret == SSH_SERVER_FILE_NOT_FOUND)
		{
			ret = remmina_init_dialog_serverkey_unknown (dialog, keyname);
		}
		else
		{
			ret = remmina_init_dialog_serverkey_changed (dialog, keyname);
		}
		if (threaded)
		{	gdk_flush();gdk_threads_leave();}

		free (keyname);
		ssh_clean_pubkey_hash (&pubkey);

		if (ret != GTK_RESPONSE_OK) return -1;
		ssh_write_knownhost (ssh->session);
		break;

		case SSH_SERVER_ERROR:
		default:
		remmina_ssh_set_error (ssh, "SSH known host checking failed: %s");
		return 0;
	}

	/* Try empty password or existing password first */
	ret = remmina_ssh_auth (ssh, NULL);
	if (ret > 0) return 1;

	/* Requested for a non-empty password */
	if (ret < 0)
	{
		if (!dialog) return -1;

		switch (ssh->auth)
		{
			case SSH_AUTH_PASSWORD:
			tips = _("Authenticating %s's password to SSH server %s...");
			keyname = _("SSH password");
			break;
			case SSH_AUTH_PUBLICKEY:
			tips = _("Authenticating %s's identity to SSH server %s...");
			keyname = _("SSH private key passphrase");
			break;
			default:
			return FALSE;
		}

		if (ssh->auth != SSH_AUTH_AUTO_PUBLICKEY)
		{
			if (threaded) gdk_threads_enter();
			remmina_init_dialog_set_status (dialog, tips, ssh->user, ssh->server);

			ret = remmina_init_dialog_authpwd (dialog, keyname, FALSE);
			if (threaded)
			{	gdk_flush();gdk_threads_leave();}

			if (ret != GTK_RESPONSE_OK) return -1;
		}
		ret = remmina_ssh_auth (ssh, dialog->password);
	}

	if (ret <= 0)
	{
		return 0;
	}

	return 1;
}

void
remmina_ssh_log_callback(ssh_session session, int priority, const char *message, void *userdata)
{
	remmina_log_printf ("[SSH] %s\n", message);
}

gboolean
remmina_ssh_init_session (RemminaSSH *ssh)
{
	gint verbosity;

	ssh->callback = g_new0 (struct ssh_callbacks_struct, 1);
	ssh->callback->userdata = ssh;

	/* Init & startup the SSH session */
	ssh->session = ssh_new ();
	ssh_options_set (ssh->session, SSH_OPTIONS_HOST, ssh->server);
	ssh_options_set (ssh->session, SSH_OPTIONS_PORT, &ssh->port);
	ssh_options_set (ssh->session, SSH_OPTIONS_USER, ssh->user);
	if (remmina_log_running ())
	{
		verbosity = SSH_LOG_RARE;
		ssh_options_set (ssh->session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
		ssh->callback->log_function = remmina_ssh_log_callback;
	}
	ssh_callbacks_init (ssh->callback);
	ssh_set_callbacks(ssh->session, ssh->callback);

	if (ssh_connect (ssh->session))
	{
		remmina_ssh_set_error (ssh, _("Failed to startup SSH session: %s"));
		return FALSE;
	}

	/* Try the "none" authentication */
	if (ssh_userauth_none (ssh->session, NULL) == SSH_AUTH_SUCCESS)
	{
		ssh->authenticated = TRUE;
	}
	return TRUE;
}

gboolean
remmina_ssh_init_from_file (RemminaSSH *ssh, RemminaFile *remminafile)
{
	const gchar *ssh_server;
	const gchar *ssh_username;
	const gchar *ssh_privatekey;
	const gchar *server;
	gchar *s;

	ssh->session = NULL;
	ssh->callback = NULL;
	ssh->authenticated = FALSE;
	ssh->error = NULL;
	pthread_mutex_init (&ssh->ssh_mutex, NULL);

	/* Parse the address and port */
	ssh_server = remmina_file_get_string (remminafile, "ssh_server");
	ssh_username = remmina_file_get_string (remminafile, "ssh_username");
	ssh_privatekey = remmina_file_get_string (remminafile, "ssh_privatekey");
	server = remmina_file_get_string (remminafile, "server");
	if (ssh_server)
	{
		remmina_public_get_server_port (ssh_server, 22, &ssh->server, &ssh->port);
		if (ssh->server[0] == '\0')
		{
			g_free(ssh->server);
			remmina_public_get_server_port (server, 0, &ssh->server, NULL);
		}
	}
	else if (server == NULL)
	{
		ssh->server = g_strdup ("localhost");
		ssh->port = 22;
	}
	else
	{
		remmina_public_get_server_port (server, 0, &ssh->server, NULL);
		ssh->port = 22;
	}

	ssh->user = g_strdup (ssh_username ? ssh_username : g_get_user_name ());
	ssh->password = NULL;
	ssh->auth = remmina_file_get_int (remminafile, "ssh_auth", 0);
	ssh->charset = g_strdup (remmina_file_get_string (remminafile, "ssh_charset"));

	/* Public/Private keys */
	s = (ssh_privatekey ? g_strdup (ssh_privatekey) : remmina_ssh_find_identity ());
	if (s)
	{
		ssh->privkeyfile = remmina_ssh_identity_path (s);
		ssh->pubkeyfile = g_strdup_printf("%s.pub", ssh->privkeyfile);
		g_free(s);
	}
	else
	{
		ssh->privkeyfile = NULL;
		ssh->pubkeyfile = NULL;
	}

	return TRUE;
}

static gboolean
remmina_ssh_init_from_ssh (RemminaSSH *ssh, const RemminaSSH *ssh_src)
{
	ssh->session = NULL;
	ssh->authenticated = FALSE;
	ssh->error = NULL;
	pthread_mutex_init (&ssh->ssh_mutex, NULL);

	ssh->server = g_strdup (ssh_src->server);
	ssh->port = ssh_src->port;
	ssh->user = g_strdup (ssh_src->user);
	ssh->auth = ssh_src->auth;
	ssh->password = g_strdup (ssh_src->password);
	ssh->pubkeyfile = g_strdup (ssh_src->pubkeyfile);
	ssh->privkeyfile = g_strdup (ssh_src->privkeyfile);
	ssh->charset = g_strdup (ssh_src->charset);

	return TRUE;
}

gchar*
remmina_ssh_convert (RemminaSSH *ssh, const gchar *from)
{
	gchar *to = NULL;

	if (ssh->charset && from)
	{
		to = g_convert (from, -1, "UTF-8", ssh->charset, NULL, NULL, NULL);
	}
	if (!to) to = g_strdup (from);
	return to;
}

gchar*
remmina_ssh_unconvert (RemminaSSH *ssh, const gchar *from)
{
	gchar *to = NULL;

	if (ssh->charset && from)
	{
		to = g_convert (from, -1, ssh->charset, "UTF-8", NULL, NULL, NULL);
	}
	if (!to) to = g_strdup (from);
	return to;
}

void
remmina_ssh_free (RemminaSSH *ssh)
{
	if (ssh->session)
	{
		ssh_free (ssh->session);
		ssh->session = NULL;
	}
	g_free(ssh->callback);
	g_free(ssh->server);
	g_free(ssh->user);
	g_free(ssh->password);
	g_free(ssh->pubkeyfile);
	g_free(ssh->privkeyfile);
	g_free(ssh->charset);
	g_free(ssh->error);
	pthread_mutex_destroy (&ssh->ssh_mutex);
	g_free(ssh);
}

/*************************** SSH Tunnel *********************************/
struct _RemminaSSHTunnelBuffer
{
	gchar *data;
	gchar *ptr;
	ssize_t len;
};

static RemminaSSHTunnelBuffer*
remmina_ssh_tunnel_buffer_new (ssize_t len)
{
	RemminaSSHTunnelBuffer *buffer;

	buffer = g_new (RemminaSSHTunnelBuffer, 1);
	buffer->data = (gchar*) g_malloc (len);
	buffer->ptr = buffer->data;
	buffer->len = len;
	return buffer;
}

static void
remmina_ssh_tunnel_buffer_free (RemminaSSHTunnelBuffer *buffer)
{
	if (buffer)
	{
		g_free(buffer->data);
		g_free(buffer);
	}
}

RemminaSSHTunnel*
remmina_ssh_tunnel_new_from_file (RemminaFile *remminafile)
{
	RemminaSSHTunnel *tunnel;

	tunnel = g_new (RemminaSSHTunnel, 1);

	remmina_ssh_init_from_file (REMMINA_SSH (tunnel), remminafile);

	tunnel->tunnel_type = -1;
	tunnel->channels = NULL;
	tunnel->sockets = NULL;
	tunnel->socketbuffers = NULL;
	tunnel->num_channels = 0;
	tunnel->max_channels = 0;
	tunnel->x11_channel = NULL;
	tunnel->thread = 0;
	tunnel->running = FALSE;
	tunnel->server_sock = -1;
	tunnel->dest = NULL;
	tunnel->port = 0;
	tunnel->buffer = NULL;
	tunnel->buffer_len = 0;
	tunnel->channels_out = NULL;
	tunnel->remotedisplay = 0;
	tunnel->localdisplay = NULL;
	tunnel->init_func = NULL;
	tunnel->connect_func = NULL;
	tunnel->disconnect_func = NULL;
	tunnel->callback_data = NULL;

	return tunnel;
}

static void
remmina_ssh_tunnel_close_all_channels (RemminaSSHTunnel *tunnel)
{
	int i;

	for (i = 0; i < tunnel->num_channels; i++)
	{
		close (tunnel->sockets[i]);
		remmina_ssh_tunnel_buffer_free (tunnel->socketbuffers[i]);
		channel_close (tunnel->channels[i]);
		channel_free (tunnel->channels[i]);
	}

	g_free(tunnel->channels);
	tunnel->channels = NULL;
	g_free(tunnel->sockets);
	tunnel->sockets = NULL;
	g_free(tunnel->socketbuffers);
	tunnel->socketbuffers = NULL;

	tunnel->num_channels = 0;
	tunnel->max_channels = 0;

	if (tunnel->x11_channel)
	{
		channel_close (tunnel->x11_channel);
		channel_free (tunnel->x11_channel);
		tunnel->x11_channel = NULL;
	}
}

static void
remmina_ssh_tunnel_remove_channel (RemminaSSHTunnel *tunnel, gint n)
{
	channel_close (tunnel->channels[n]);
	channel_free (tunnel->channels[n]);
	close (tunnel->sockets[n]);
	remmina_ssh_tunnel_buffer_free (tunnel->socketbuffers[n]);
	tunnel->num_channels--;
	tunnel->channels[n] = tunnel->channels[tunnel->num_channels];
	tunnel->channels[tunnel->num_channels] = NULL;
	tunnel->sockets[n] = tunnel->sockets[tunnel->num_channels];
	tunnel->socketbuffers[n] = tunnel->socketbuffers[tunnel->num_channels];
}

/* Register the new channel/socket pair */
static void
remmina_ssh_tunnel_add_channel (RemminaSSHTunnel *tunnel, ssh_channel channel, gint sock)
{
	gint flags;
	gint i;

	i = tunnel->num_channels++;
	if (tunnel->num_channels > tunnel->max_channels)
	{
		/* Allocate an extra NULL pointer in channels for ssh_select */
		tunnel->channels = (ssh_channel*) g_realloc (tunnel->channels,
				sizeof (ssh_channel) * (tunnel->num_channels + 1));
		tunnel->sockets = (gint*) g_realloc (tunnel->sockets,
				sizeof (gint) * tunnel->num_channels);
		tunnel->socketbuffers = (RemminaSSHTunnelBuffer**) g_realloc (tunnel->socketbuffers,
				sizeof (RemminaSSHTunnelBuffer*) * tunnel->num_channels);
		tunnel->max_channels = tunnel->num_channels;

		tunnel->channels_out = (ssh_channel*) g_realloc (tunnel->channels_out,
				sizeof (ssh_channel) * (tunnel->num_channels + 1));
	}
	tunnel->channels[i] = channel;
	tunnel->channels[i + 1] = NULL;
	tunnel->sockets[i] = sock;
	tunnel->socketbuffers[i] = NULL;

	flags = fcntl (sock, F_GETFL, 0);
	fcntl (sock, F_SETFL, flags | O_NONBLOCK);
}

static gpointer
remmina_ssh_tunnel_main_thread_proc (gpointer data)
{
	RemminaSSHTunnel *tunnel = (RemminaSSHTunnel*) data;
	gchar *ptr;
	ssize_t len = 0, lenw = 0;
	fd_set set;
	struct timeval timeout;
	GTimeVal t1, t2;
	glong diff;
	ssh_channel channel = NULL;
	gboolean first = TRUE;
	gboolean disconnected;
	gint sock;
	gint maxfd;
	gint i;
	gint ret;
	struct sockaddr_in sin;

	g_get_current_time (&t1);
	t2 = t1;

	switch (tunnel->tunnel_type)
	{
		case REMMINA_SSH_TUNNEL_OPEN:
		/* Accept a local connection */
		sock = accept (tunnel->server_sock, NULL, NULL);
		if (sock < 0)
		{
			REMMINA_SSH (tunnel)->error = g_strdup ("Failed to accept local socket");
			tunnel->thread = 0;
			return NULL;
		}

		if ((channel = channel_new (tunnel->ssh.session)) == NULL)
		{
			close (sock);
			remmina_ssh_set_error (REMMINA_SSH (tunnel), "Failed to createt channel : %s");
			tunnel->thread = 0;
			return NULL;
		}
		/* Request the SSH server to connect to the destination */
		if (channel_open_forward (channel, tunnel->dest, tunnel->port, "127.0.0.1", 0) != SSH_OK)
		{
			close (sock);
			channel_close (channel);
			channel_free (channel);
			remmina_ssh_set_error (REMMINA_SSH (tunnel), _("Failed to connect to the SSH tunnel destination: %s"));
			tunnel->thread = 0;
			return NULL;
		}
		remmina_ssh_tunnel_add_channel (tunnel, channel, sock);
		break;

		case REMMINA_SSH_TUNNEL_X11:
		if ((tunnel->x11_channel = channel_new (tunnel->ssh.session)) == NULL)
		{
			remmina_ssh_set_error (REMMINA_SSH (tunnel), "Failed to create channel : %s");
			tunnel->thread = 0;
			return NULL;
		}
		if (!remmina_public_get_xauth_cookie (tunnel->localdisplay, &ptr))
		{
			remmina_ssh_set_application_error (REMMINA_SSH (tunnel), "%s", ptr);
			g_free(ptr);
			tunnel->thread = 0;
			return NULL;
		}
		if (channel_open_session (tunnel->x11_channel) ||
				channel_request_x11 (tunnel->x11_channel, TRUE, NULL, ptr,
						gdk_screen_get_number (gdk_screen_get_default ())))
		{
			g_free(ptr);
			remmina_ssh_set_error (REMMINA_SSH (tunnel), "Failed to open channel : %s");
			tunnel->thread = 0;
			return NULL;
		}
		g_free(ptr);
		if (channel_request_exec (tunnel->x11_channel, tunnel->dest))
		{
			ptr = g_strdup_printf(_("Failed to execute %s on SSH server : %%s"), tunnel->dest);
			remmina_ssh_set_error (REMMINA_SSH (tunnel), ptr);
			g_free(ptr);
			tunnel->thread = 0;
			return NULL;
		}

		if (tunnel->init_func &&
				! (*tunnel->init_func) (tunnel, tunnel->callback_data))
		{
			if (tunnel->disconnect_func)
			{
				(*tunnel->disconnect_func) (tunnel, tunnel->callback_data);
			}
			tunnel->thread = 0;
			return NULL;
		}

		break;

		case REMMINA_SSH_TUNNEL_XPORT:
		/* Detect the next available port starting from 6010 on the server */
		for (i = 10; i <= MAX_X_DISPLAY_NUMBER; i++)
		{
			if (channel_forward_listen (REMMINA_SSH (tunnel)->session,
							(tunnel->bindlocalhost ? "localhost" : NULL), 6000 + i, NULL))
			{
				continue;
			}
			else
			{
				tunnel->remotedisplay = i;
				break;
			}
		}
		if (tunnel->remotedisplay < 1)
		{
			remmina_ssh_set_error (REMMINA_SSH (tunnel), _("Failed to request port forwarding : %s"));
			if (tunnel->disconnect_func)
			{
				(*tunnel->disconnect_func) (tunnel, tunnel->callback_data);
			}
			tunnel->thread = 0;
			return NULL;
		}

		if (tunnel->init_func &&
				! (*tunnel->init_func) (tunnel, tunnel->callback_data))
		{
			if (tunnel->disconnect_func)
			{
				(*tunnel->disconnect_func) (tunnel, tunnel->callback_data);
			}
			tunnel->thread = 0;
			return NULL;
		}

		break;

		case REMMINA_SSH_TUNNEL_REVERSE:
		if (channel_forward_listen (REMMINA_SSH (tunnel)->session, NULL, tunnel->port, NULL))
		{
			remmina_ssh_set_error (REMMINA_SSH (tunnel), _("Failed to request port forwarding : %s"));
			if (tunnel->disconnect_func)
			{
				(*tunnel->disconnect_func) (tunnel, tunnel->callback_data);
			}
			tunnel->thread = 0;
			return NULL;
		}

		if (tunnel->init_func &&
				! (*tunnel->init_func) (tunnel, tunnel->callback_data))
		{
			if (tunnel->disconnect_func)
			{
				(*tunnel->disconnect_func) (tunnel, tunnel->callback_data);
			}
			tunnel->thread = 0;
			return NULL;
		}

		break;
	}

	tunnel->buffer_len = 10240;
	tunnel->buffer = g_malloc (tunnel->buffer_len);

	/* Start the tunnel data transmittion */
	while (tunnel->running)
	{
		if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_XPORT ||
				tunnel->tunnel_type == REMMINA_SSH_TUNNEL_X11 ||
				tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE)
		{
			if (first)
			{
				first = FALSE;
				/* Wait for a period of time for the first incoming connection */
				if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_X11)
				{
					channel = channel_accept_x11 (tunnel->x11_channel, 15000);
				}
				else
				{
					channel = channel_forward_accept (REMMINA_SSH (tunnel)->session, 15000);
				}
				if (!channel)
				{
					remmina_ssh_set_application_error (REMMINA_SSH (tunnel), _("No response from the server."));
					if (tunnel->disconnect_func)
					{
						(*tunnel->disconnect_func) (tunnel, tunnel->callback_data);
					}
					tunnel->thread = 0;
					return NULL;
				}
				if (tunnel->connect_func)
				{
					(*tunnel->connect_func) (tunnel, tunnel->callback_data);
				}
				if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE)
				{
					/* For reverse tunnel, we only need one connection. */
					channel_forward_cancel (REMMINA_SSH (tunnel)->session, NULL, tunnel->port);
				}
			}
			else if (tunnel->tunnel_type != REMMINA_SSH_TUNNEL_REVERSE)
			{
				/* Poll once per some period of time if no incoming connections.
				 * Don't try to poll continuously as it will significantly slow down the loop */
				g_get_current_time (&t1);
				diff = (t1.tv_sec - t2.tv_sec) * 10 + (t1.tv_usec - t2.tv_usec) / 100000;
				if (diff > 1)
				{
					if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_X11)
					{
						channel = channel_accept_x11 (tunnel->x11_channel, 0);
					}
					else
					{
						channel = channel_forward_accept (REMMINA_SSH (tunnel)->session, 0);
					}
					if (channel == NULL)
					{
						t2 = t1;
					}
				}
			}

			if (channel)
			{
				if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE)
				{
					sin.sin_family = AF_INET;
					sin.sin_port = htons (tunnel->localport);
					sin.sin_addr.s_addr = inet_addr ("127.0.0.1");
					sock = socket (AF_INET, SOCK_STREAM, 0);
					if (connect (sock, (struct sockaddr *) &sin, sizeof (sin)) < 0)
					{
						remmina_ssh_set_application_error (REMMINA_SSH (tunnel),
								"Cannot connect to local port %i.", tunnel->localport);
						close (sock);
						sock = -1;
					}
				}
				else
				{
					sock = remmina_public_open_xdisplay (tunnel->localdisplay);
				}
				if (sock >= 0)
				{
					remmina_ssh_tunnel_add_channel (tunnel, channel, sock);
				}
				else
				{
					/* Failed to create unix socket. Will this happen? */
					channel_close (channel);
					channel_free (channel);
				}
				channel = NULL;
			}
		}

		if (tunnel->num_channels <= 0)
		{
			/* No more connections. We should quit */
			break;
		}

		timeout.tv_sec = 0;
		timeout.tv_usec = 200000;

		FD_ZERO (&set);
		maxfd = 0;
		for (i = 0; i < tunnel->num_channels; i++)
		{
			if (tunnel->sockets[i] > maxfd)
			{
				maxfd = tunnel->sockets[i];
			}
			FD_SET (tunnel->sockets[i], &set);
		}

		ret = ssh_select (tunnel->channels, tunnel->channels_out, maxfd + 1, &set, &timeout);
		if (!tunnel->running) break;
		if (ret == SSH_EINTR) continue;
		if (ret == -1) break;

		i = 0;
		while (tunnel->running && i < tunnel->num_channels)
		{
			disconnected = FALSE;
			if (FD_ISSET (tunnel->sockets[i], &set))
			{
				while (!disconnected &&
						(len = read (tunnel->sockets[i], tunnel->buffer, tunnel->buffer_len)) > 0)
				{
					for (ptr = tunnel->buffer, lenw = 0; len > 0; len -= lenw, ptr += lenw)
					{
						lenw = channel_write (tunnel->channels[i], (char*) ptr, len);
						if (lenw <= 0)
						{
							disconnected = TRUE;
							break;
						}
					}
				}
				if (len == 0) disconnected = TRUE;
			}
			if (disconnected)
			{
				remmina_ssh_tunnel_remove_channel (tunnel, i);
				continue;
			}
			i++;
		}
		if (!tunnel->running) break;

		i = 0;
		while (tunnel->running && i < tunnel->num_channels)
		{
			disconnected = FALSE;

			if (!tunnel->socketbuffers[i])
			{
				len = channel_poll (tunnel->channels[i], 0);
				if (len == SSH_ERROR || len == SSH_EOF)
				{
					disconnected = TRUE;
				}
				else if (len > 0)
				{
					tunnel->socketbuffers[i] = remmina_ssh_tunnel_buffer_new (len);
					len = channel_read_nonblocking (tunnel->channels[i], tunnel->socketbuffers[i]->data, len, 0);
					if (len <= 0)
					{
						disconnected = TRUE;
					}
					else
					{
						tunnel->socketbuffers[i]->len = len;
					}
				}
			}

			if (!disconnected && tunnel->socketbuffers[i])
			{
				for (lenw = 0; tunnel->socketbuffers[i]->len > 0;
						tunnel->socketbuffers[i]->len -= lenw, tunnel->socketbuffers[i]->ptr += lenw)
				{
					lenw = write (tunnel->sockets[i], tunnel->socketbuffers[i]->ptr, tunnel->socketbuffers[i]->len);
					if (lenw == -1 && errno == EAGAIN && tunnel->running)
					{
						/* Sometimes we cannot write to a socket (always EAGAIN), probably because it's internal
						 * buffer is full. We need read the pending bytes from the socket first. so here we simply
						 * break, leave the buffer there, and continue with other data */
						break;
					}
					if (lenw <= 0)
					{
						disconnected = TRUE;
						break;
					}
				}
				if (tunnel->socketbuffers[i]->len <= 0)
				{
					remmina_ssh_tunnel_buffer_free (tunnel->socketbuffers[i]);
					tunnel->socketbuffers[i] = NULL;
				}
			}

			if (disconnected)
			{
				remmina_ssh_tunnel_remove_channel (tunnel, i);
				continue;
			}
			i++;
		}
	}

	remmina_ssh_tunnel_close_all_channels (tunnel);

	return NULL;
}

static gpointer
remmina_ssh_tunnel_main_thread (gpointer data)
{
	RemminaSSHTunnel *tunnel = (RemminaSSHTunnel*) data;

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);

	while (TRUE)
	{
		remmina_ssh_tunnel_main_thread_proc (data);
		if (tunnel->server_sock < 0 || tunnel->thread == 0 || !tunnel->running) break;
	}
	tunnel->thread = 0;
	return NULL;
}

void
remmina_ssh_tunnel_cancel_accept (RemminaSSHTunnel *tunnel)
{
	if (tunnel->server_sock >= 0)
	{
		close (tunnel->server_sock);
		tunnel->server_sock = -1;
	}
}

gboolean
remmina_ssh_tunnel_open (RemminaSSHTunnel* tunnel, const gchar *host, gint port, gint local_port)
{
	gint sock;
	gint sockopt = 1;
	struct sockaddr_in sin;

	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_OPEN;
	tunnel->dest = g_strdup (host);
	tunnel->port = port;
	if (tunnel->port == 0)
	{
		REMMINA_SSH (tunnel)->error = g_strdup ("Destination port has not been assigned");
		return FALSE;
	}

	/* Create the server socket that listens on the local port */
	sock = socket (AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		REMMINA_SSH (tunnel)->error = g_strdup ("Failed to create socket.");
		return FALSE;
	}
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof (sockopt));

	sin.sin_family = AF_INET;
	sin.sin_port = htons (local_port);
	sin.sin_addr.s_addr = inet_addr ("127.0.0.1");

	if (bind (sock, (struct sockaddr *) &sin, sizeof(sin)))
	{
		REMMINA_SSH (tunnel)->error = g_strdup ("Failed to bind on local port.");
		close (sock);
		return FALSE;
	}

	if (listen (sock, 1))
	{
		REMMINA_SSH (tunnel)->error = g_strdup ("Failed to listen on local port.");
		close (sock);
		return FALSE;
	}

	tunnel->server_sock = sock;
	tunnel->running = TRUE;

	if (pthread_create (&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel))
	{
		remmina_ssh_set_application_error (REMMINA_SSH (tunnel), "Failed to initialize pthread.");
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_x11 (RemminaSSHTunnel *tunnel, const gchar *cmd)
{
	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_X11;
	tunnel->dest = g_strdup (cmd);
	tunnel->running = TRUE;

	if (pthread_create (&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel))
	{
		remmina_ssh_set_application_error (REMMINA_SSH (tunnel), "Failed to initialize pthread.");
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_xport (RemminaSSHTunnel *tunnel, gboolean bindlocalhost)
{
	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_XPORT;
	tunnel->bindlocalhost = bindlocalhost;
	tunnel->running = TRUE;

	if (pthread_create (&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel))
	{
		remmina_ssh_set_application_error (REMMINA_SSH (tunnel), "Failed to initialize pthread.");
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_reverse (RemminaSSHTunnel *tunnel, gint port, gint local_port)
{
	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_REVERSE;
	tunnel->port = port;
	tunnel->localport = local_port;
	tunnel->running = TRUE;

	if (pthread_create (&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel))
	{
		remmina_ssh_set_application_error (REMMINA_SSH (tunnel), "Failed to initialize pthread.");
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_terminated (RemminaSSHTunnel* tunnel)
{
	return (tunnel->thread == 0);
}

void
remmina_ssh_tunnel_free (RemminaSSHTunnel* tunnel)
{
	pthread_t thread;

	thread = tunnel->thread;
	if (thread != 0)
	{
		tunnel->running = FALSE;
		pthread_cancel (thread);
		pthread_join (thread, NULL);
		tunnel->thread = 0;
	}

	if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_XPORT && tunnel->remotedisplay > 0)
	{
		channel_forward_cancel (REMMINA_SSH (tunnel)->session,
				NULL, 6000 + tunnel->remotedisplay);
	}
	if (tunnel->server_sock >= 0)
	{
		close (tunnel->server_sock);
		tunnel->server_sock = -1;
	}
	remmina_ssh_tunnel_close_all_channels (tunnel);

	g_free(tunnel->buffer);
	g_free(tunnel->channels_out);
	g_free(tunnel->dest);
	g_free(tunnel->localdisplay);

	remmina_ssh_free (REMMINA_SSH (tunnel));
}

/*************************** SFTP *********************************/

RemminaSFTP*
remmina_sftp_new_from_file (RemminaFile *remminafile)
{
	RemminaSFTP *sftp;

	sftp = g_new (RemminaSFTP, 1);

	remmina_ssh_init_from_file (REMMINA_SSH (sftp), remminafile);

	sftp->sftp_sess = NULL;

	return sftp;
}

RemminaSFTP*
remmina_sftp_new_from_ssh (RemminaSSH *ssh)
{
	RemminaSFTP *sftp;

	sftp = g_new (RemminaSFTP, 1);

	remmina_ssh_init_from_ssh (REMMINA_SSH (sftp), ssh);

	sftp->sftp_sess = NULL;

	return sftp;
}

gboolean
remmina_sftp_open (RemminaSFTP *sftp)
{
	sftp->sftp_sess = sftp_new (sftp->ssh.session);
	if (!sftp->sftp_sess)
	{
		remmina_ssh_set_error (REMMINA_SSH (sftp), _("Failed to create sftp session: %s"));
		return FALSE;
	}
	if (sftp_init (sftp->sftp_sess))
	{
		remmina_ssh_set_error (REMMINA_SSH (sftp), _("Failed to initialize sftp session: %s"));
		return FALSE;
	}
	return TRUE;
}

void
remmina_sftp_free (RemminaSFTP *sftp)
{
	if (sftp->sftp_sess)
	{
		sftp_free (sftp->sftp_sess);
		sftp->sftp_sess = NULL;
	}
	remmina_ssh_free (REMMINA_SSH (sftp));
}

/*************************** SSH Shell *********************************/

RemminaSSHShell*
remmina_ssh_shell_new_from_file (RemminaFile *remminafile)
{
	RemminaSSHShell *shell;

	shell = g_new0 (RemminaSSHShell, 1);

	remmina_ssh_init_from_file (REMMINA_SSH (shell), remminafile);

	shell->master = -1;
	shell->slave = -1;
	shell->exec = g_strdup (remmina_file_get_string (remminafile, "exec"));

	return shell;
}

RemminaSSHShell*
remmina_ssh_shell_new_from_ssh (RemminaSSH *ssh)
{
	RemminaSSHShell *shell;

	shell = g_new0 (RemminaSSHShell, 1);

	remmina_ssh_init_from_ssh (REMMINA_SSH (shell), ssh);

	shell->master = -1;
	shell->slave = -1;

	return shell;
}

static gpointer
remmina_ssh_shell_thread (gpointer data)
{
	RemminaSSHShell *shell = (RemminaSSHShell*) data;
	fd_set fds;
	struct timeval timeout;
	ssh_channel channel = NULL;
	ssh_channel ch[2], chout[2];
	gchar *buf = NULL;
	gint buf_len;
	gint len;
	gint i, ret;

	LOCK_SSH (shell)

	if ((channel = channel_new (REMMINA_SSH (shell)->session)) == NULL ||
			channel_open_session (channel))
	{
		UNLOCK_SSH (shell)
		remmina_ssh_set_error (REMMINA_SSH (shell), "Failed to open channel : %s");
		if (channel) channel_free (channel);
		shell->thread = 0;
		return NULL;
	}

	channel_request_pty (channel);
	if (shell->exec && shell->exec[0])
	{
		ret = channel_request_exec (channel, shell->exec);
	}
	else
	{
		ret = channel_request_shell (channel);
	}
	if (ret)
	{
		UNLOCK_SSH (shell)
		remmina_ssh_set_error (REMMINA_SSH (shell), "Failed to request shell : %s");
		channel_close (channel);
		channel_free (channel);
		shell->thread = 0;
		return NULL;
	}

	shell->channel = channel;

	UNLOCK_SSH (shell)

	buf_len = 1000;
	buf = g_malloc (buf_len + 1);

	ch[0] = channel;
	ch[1] = NULL;

	while (!shell->closed)
	{
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		FD_ZERO (&fds);
		FD_SET (shell->master, &fds);

		ret = ssh_select (ch, chout, shell->master + 1, &fds, &timeout);
		if (ret == SSH_EINTR) continue;
		if (ret == -1) break;

		if (FD_ISSET (shell->master, &fds))
		{
			len = read (shell->master, buf, buf_len);
			if (len <= 0) break;
			LOCK_SSH (shell)
			channel_write (channel, buf, len);
			UNLOCK_SSH (shell)
		}
		for (i = 0; i < 2; i++)
		{
			LOCK_SSH (shell)
			len = channel_poll (channel, i);
			UNLOCK_SSH (shell)
			if (len == SSH_ERROR || len == SSH_EOF)
			{
				shell->closed = TRUE;
				break;
			}
			if (len <= 0) continue;
			if (len > buf_len)
			{
				buf_len = len;
				buf = (gchar*) g_realloc (buf, buf_len + 1);
			}
			LOCK_SSH (shell)
			len = channel_read_nonblocking (channel, buf, len, i);
			UNLOCK_SSH (shell)
			if (len <= 0)
			{
				shell->closed = TRUE;
				break;
			}
			while (len > 0)
			{
				ret = write (shell->master, buf, len);
				if (ret <= 0) break;
				len -= ret;
			}
		}
	}

	LOCK_SSH (shell)
	shell->channel = NULL;
	channel_close (channel);
	channel_free (channel);
	UNLOCK_SSH (shell)

	g_free(buf);
	shell->thread = 0;

	if (shell->exit_callback) shell->exit_callback (shell->user_data);
	return NULL;
}

gboolean
remmina_ssh_shell_open (RemminaSSHShell *shell, RemminaSSHExitFunc exit_callback, gpointer data)
{
	gchar *slavedevice;
	struct termios stermios;

	shell->master = posix_openpt (O_RDWR | O_NOCTTY);
	if (shell->master == -1 ||
			grantpt (shell->master) == -1 ||
			unlockpt (shell->master) == -1 ||
			(slavedevice = ptsname (shell->master)) == NULL ||
			(shell->slave = open (slavedevice, O_RDWR | O_NOCTTY)) < 0)
	{
		REMMINA_SSH (shell)->error = g_strdup ("Failed to create pty device.");
		return FALSE;
	}

	/* These settings works fine with OpenSSH... */
	tcgetattr (shell->slave, &stermios);
	stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON);
	stermios.c_iflag &= ~(ICRNL);
	tcsetattr (shell->slave, TCSANOW, &stermios);

	shell->exit_callback = exit_callback;
	shell->user_data = data;

	/* Once the process started, we should always TRUE and assume the pthread will be created always */
	pthread_create (&shell->thread, NULL, remmina_ssh_shell_thread, shell);

	return TRUE;
}

void
remmina_ssh_shell_set_size (RemminaSSHShell *shell, gint columns, gint rows)
{
	LOCK_SSH (shell)
	if (shell->channel)
	{
		channel_change_pty_size (shell->channel, columns, rows);
	}
	UNLOCK_SSH (shell)
}

void
remmina_ssh_shell_free (RemminaSSHShell *shell)
{
	pthread_t thread = shell->thread;

	shell->exit_callback = NULL;
	if (thread)
	{
		shell->closed = TRUE;
		pthread_join (thread, NULL);
	}
	close (shell->master);
	if (shell->exec)
	{
		g_free(shell->exec);
		shell->exec = NULL;
	}
	/* It's not necessary to close shell->slave since the other end (vte) will close it */;
	remmina_ssh_free (REMMINA_SSH (shell));
}

#endif /* HAVE_LIBSSH */

