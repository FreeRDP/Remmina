/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2022 Antenore Gatta, Giovanni Panozzo
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

#include "config.h"

#ifdef HAVE_LIBSSH

/* To get definitions of NI_MAXHOST and NI_MAXSERV from <netdb.h> */
#define _DEFAULT_SOURCE

/* Define this before stdlib.h to have posix_openpt */
#define _XOPEN_SOURCE 600

#include <errno.h>
#define LIBSSH_STATIC 1
#include <libssh/libssh.h>
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <poll.h>
#include <stdlib.h>
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_PTY_H
#include <pty.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#include "remmina_public.h"
#include "remmina/types.h"
#include "remmina_file.h"
#include "remmina_log.h"
#include "remmina_pref.h"
#include "remmina_ssh.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"


#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>

#if defined(__FreeBSD__) || defined(__OpenBSD__)
#ifndef SOL_TCP
#define SOL_TCP IPPROTO_TCP
#endif
#endif

#endif

/*-----------------------------------------------------------------------------*
*                           SSH Base                                          *
*-----------------------------------------------------------------------------*/

#define LOCK_SSH(ssh) pthread_mutex_lock(&REMMINA_SSH(ssh)->ssh_mutex);
#define UNLOCK_SSH(ssh) pthread_mutex_unlock(&REMMINA_SSH(ssh)->ssh_mutex);

static const gchar *common_identities[] =
{
	".ssh/id_ed25519",
	".ssh/id_rsa",
	".ssh/id_dsa",
	".ssh/identity",
	NULL
};

/*-----------------------------------------------------------------------------*
*                           X11 Channels                                      *
*-----------------------------------------------------------------------------*/
#define _PATH_UNIX_X    "/tmp/.X11-unix/X%d"
#define _XAUTH_CMD      "/usr/bin/xauth list %s 2>/dev/null"

typedef struct item {
	ssh_channel channel;
	gint fd_in;
	gint fd_out;
	gboolean protected;
	pthread_t thread;
	struct item *next;
} node_t;

node_t *node = NULL;

// Mutex
pthread_mutex_t mutex;

// Linked nodes to manage channel/fd tuples
static void remmina_ssh_insert_item(ssh_channel channel, gint fd_in, gint fd_out, gboolean protected, pthread_t thread);
static void remmina_ssh_delete_item(ssh_channel channel);
static node_t * remmina_ssh_search_item(ssh_channel channel);

// X11 Display
const char * remmina_ssh_ssh_gai_strerror(int gaierr);
static int remmina_ssh_x11_get_proto(const char *display, char **_proto, char **_data);
static void remmina_ssh_set_nodelay(int fd);
static int remmina_ssh_connect_local_xsocket_path(const char *pathname);
static int remmina_ssh_connect_local_xsocket(int display_number);
static int remmina_ssh_x11_connect_display();

// Send data to channel
static int remimna_ssh_cp_to_ch_cb(int fd, int revents, void *userdata);

// Read data from channel
static int remmina_ssh_cp_to_fd_cb(ssh_session session, ssh_channel channel, void *data, uint32_t len, int is_stderr, void *userdata);

// EOF&Close channel
static void remmina_ssh_ch_close_cb(ssh_session session, ssh_channel channel, void *userdata);

// Close all X11 channel
static void remmina_ssh_close_all_x11_ch(pthread_t thread);

// X11 Request
static ssh_channel remmina_ssh_x11_open_request_cb(ssh_session session, const char *shost, int sport, void *userdata);

// SSH Channel Callbacks
struct ssh_channel_callbacks_struct channel_cb =
{
	.channel_data_function = remmina_ssh_cp_to_fd_cb,
	.channel_eof_function = remmina_ssh_ch_close_cb,
	.channel_close_function = remmina_ssh_ch_close_cb,
	.userdata = NULL
};

// SSH Event Context
short events = POLLIN | POLLPRI | POLLERR | POLLHUP | POLLNVAL;

// Functions
static void
remmina_ssh_insert_item(ssh_channel channel, gint fd_in, gint fd_out, gboolean protected, pthread_t thread)
{
	TRACE_CALL(__func__);

	pthread_mutex_lock(&mutex);

	REMMINA_DEBUG("insert node - fd_in: %d - fd_out: %d - protected %d", fd_in, fd_out, protected);

	node_t *node_iterator, *new;
	if (node == NULL) {
		/* Calloc ensure that node is full of 0 */
		node = (node_t *) calloc(1, sizeof(node_t));
		node->channel = channel;
		node->fd_in = fd_in;
		node->fd_out = fd_out;
		node->protected = protected;
		node->thread = thread;
		node->next = NULL;
	} else {
		node_iterator = node;
		while (node_iterator->next != NULL)
			node_iterator = node_iterator->next;
		/* Create the new node */
		new = (node_t *) malloc(sizeof(node_t));
		new->channel = channel;
		new->fd_in = fd_in;
		new->fd_out = fd_out;
		new->protected = protected;
		new->thread = thread;
		new->next = NULL;
		node_iterator->next = new;
	}

	pthread_mutex_unlock(&mutex);
}

static void
remmina_ssh_delete_item(ssh_channel channel)
{
	TRACE_CALL(__func__);

	REMMINA_DEBUG("delete node");

	pthread_mutex_lock(&mutex);

	node_t *current, *previous = NULL;
	for (current = node; current; previous = current, current = current->next) {
		if (current->channel != channel)
			continue;

		if (previous == NULL)
			node = current->next;
		else
			previous->next = current->next;

		free(current);
		pthread_mutex_unlock(&mutex);
		return;
	}

	pthread_mutex_unlock(&mutex);
}

static node_t *
remmina_ssh_search_item(ssh_channel channel)
{
	TRACE_CALL(__func__);

	REMMINA_DEBUG("search node");

	pthread_mutex_lock(&mutex);

	node_t *current = node;
	while (current != NULL) {
		if (current->channel == channel) {
			pthread_mutex_unlock(&mutex);
			REMMINA_DEBUG("found node - fd_in: %d - fd_out: %d - protected: %d", current->fd_in, current->fd_out, current->protected);
			return current;
		} else {
			current = current->next;
		}
	}

	pthread_mutex_unlock(&mutex);

	return NULL;
}

static void
remmina_ssh_set_nodelay(int fd)
{
	TRACE_CALL(__func__);
	int opt;
	socklen_t optlen;

	optlen = sizeof(opt);
	if (getsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, &optlen) == -1) {
		REMMINA_WARNING("getsockopt TCP_NODELAY: %.100s", strerror(errno));
		return;
	}
	if (opt == 1) {
		REMMINA_DEBUG("fd %d is TCP_NODELAY", fd);
		return;
	}
	opt = 1;
	REMMINA_DEBUG("fd %d setting TCP_NODELAY", fd);
	if (setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt)) == -1)
		REMMINA_WARNING("setsockopt TCP_NODELAY: %.100s", strerror(errno));
}

const char *
remmina_ssh_ssh_gai_strerror(int gaierr)
{
	TRACE_CALL(__func__);

	if (gaierr == EAI_SYSTEM && errno != 0)
		return strerror(errno);
	return gai_strerror(gaierr);
}

static int
remmina_ssh_x11_get_proto(const char *display, char **_proto, char **_cookie)
{
	TRACE_CALL(__func__);

	char cmd[1024], line[512], xdisplay[512];
	static char proto[512], cookie[512];
	FILE *f;
	int ret = 0, r;

	*_proto = proto;
	*_cookie = cookie;

	proto[0] = cookie[0] = '\0';

	if (strncmp(display, "localhost:", 10) == 0) {
		if ((r = snprintf(xdisplay, sizeof(xdisplay), "unix:%s", display + 10)) < 0 || (size_t)r >= sizeof(xdisplay)) {
			REMMINA_WARNING("display name too long. display: %s", display);
			return -1;
		}
		display = xdisplay;
	}

	snprintf(cmd, sizeof(cmd), _XAUTH_CMD, display);
	REMMINA_DEBUG("xauth cmd: %s", cmd);

	f = popen(cmd, "r");
	if (f && fgets(line, sizeof(line), f) && sscanf(line, "%*s %511s %511s", proto, cookie) == 2) {
		ret = 0;
	} else {
		ret = 1;
	}

	if (f) pclose(f);

	REMMINA_DEBUG("proto: %s - cookie: %s - ret: %d", proto, cookie, ret);

	return ret;
}

static int
remmina_ssh_connect_local_xsocket_path(const char *pathname)
{
	TRACE_CALL(__func__);

	int sock;
	struct sockaddr_un addr;

	sock = socket(AF_UNIX, SOCK_STREAM, 0);
	if (sock == -1)
		REMMINA_WARNING("socket: %.100s", strerror(errno));

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	addr.sun_path[0] = '\0';
	memcpy(addr.sun_path + 1, pathname, strlen(pathname));
	if (connect(sock, (struct sockaddr *)&addr, offsetof(struct sockaddr_un, sun_path) + 1 + strlen(pathname)) == 0) {
		REMMINA_DEBUG("sock: %d", sock);
		return sock;
	}

	REMMINA_WARNING("connect %.100s: %.100s", addr.sun_path, strerror(errno));
	close(sock);

	return -1;
}

static int
remmina_ssh_connect_local_xsocket(int display_number)
{
	TRACE_CALL(__func__);

	char buf[1024];
	snprintf(buf, sizeof(buf), _PATH_UNIX_X, display_number);
	return remmina_ssh_connect_local_xsocket_path(buf);
}

static int
remmina_ssh_x11_connect_display()
{
	TRACE_CALL(__func__);

	unsigned int display_number;
	const char *display;
	char buf[1024], *cp;
	struct addrinfo hints, *ai, *aitop;
	char strport[NI_MAXSERV];
	int gaierr, sock = 0;

	/* Try to open a socket for the local X server. */
	display = getenv("DISPLAY");
	if (!display) {
		return -1;
	}

	REMMINA_DEBUG("display: %s", display);

	/* Check if it is a unix domain socket. */
	if (strncmp(display, "unix:", 5) == 0 || display[0] == ':') {
		/* Connect to the unix domain socket. */
		if (sscanf(strrchr(display, ':') + 1, "%u", &display_number) != 1) {
			REMMINA_WARNING("Could not parse display number from DISPLAY: %.100s", display);
			return -1;
		}

		REMMINA_DEBUG("display_number: %d", display_number);

		/* Create a socket. */
		sock = remmina_ssh_connect_local_xsocket(display_number);

		REMMINA_DEBUG("socket: %d", sock);

		if (sock < 0)
			return -1;

		/* OK, we now have a connection to the display. */
		return sock;
	}

	/* Connect to an inet socket. */
	strncpy(buf, display, sizeof(buf) - 1);
	cp = strchr(buf, ':');
	if (!cp) {
		REMMINA_WARNING("Could not find ':' in DISPLAY: %.100s", display);
		return -1;
	}
	*cp = 0;
	if (sscanf(cp + 1, "%u", &display_number) != 1) {
		REMMINA_WARNING("Could not parse display number from DISPLAY: %.100s", display);
		return -1;
	}

	/* Look up the host address */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	snprintf(strport, sizeof(strport), "%u", 6000 + display_number);
	if ((gaierr = getaddrinfo(buf, strport, &hints, &aitop)) != 0) {
		REMMINA_WARNING("%.100s: unknown host. (%s)", buf, remmina_ssh_ssh_gai_strerror(gaierr));
		return -1;
	}
	for (ai = aitop; ai; ai = ai->ai_next) {
		/* Create a socket. */
		sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
		if (sock == -1) {
			REMMINA_WARNING("socket: %.100s", strerror(errno));
			continue;
		}
		/* Connect it to the display. */
		if (connect(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
			REMMINA_WARNING("connect %.100s port %u: %.100s", buf, 6000 + display_number, strerror(errno));
			close(sock);
			continue;
		}
		/* Success */
		break;
	}
	freeaddrinfo(aitop);
	if (!ai) {
		REMMINA_WARNING("connect %.100s port %u: %.100s", buf, 6000 + display_number, strerror(errno));
		return -1;
	}
	remmina_ssh_set_nodelay(sock);

	REMMINA_DEBUG("sock: %d", sock);

	return sock;
}

static int
remimna_ssh_cp_to_ch_cb(int fd, int revents, void *userdata)
{
	TRACE_CALL(__func__);
	ssh_channel channel = (ssh_channel)userdata;
	gchar buf[2097152];
	gint sz = 0, ret = 0;

	node_t *temp_node = remmina_ssh_search_item(channel);

	if (!channel) {
		if (!temp_node->protected) {
			shutdown(fd, SHUT_RDWR);
			close(fd);
			REMMINA_DEBUG("fd %d closed.", fd);
		}
		REMMINA_WARNING("channel does not exist.");
		return -1;
	}

	if ((revents & POLLIN) || (revents & POLLPRI)) {
		sz = read(fd, buf, sizeof(buf));
		if (sz > 0) {
			ret = ssh_channel_write(channel, buf, sz);
			REMMINA_DEBUG("ssh_channel_write ret: %d sz: %d", ret, sz);
		} else if (sz < 0) {
			REMMINA_WARNING("fd bytes read: %d", sz);
			return -1;
		} else {
			REMMINA_WARNING("Why the hell am I here?");
			if (!temp_node->protected) {
				shutdown(fd, SHUT_RDWR);
				close(fd);
				REMMINA_DEBUG("fd %d closed.", fd);
			}
			return -1;
		}
	}

	if ((revents & POLLHUP) || (revents & POLLNVAL) || (revents & POLLERR)) {
		REMMINA_DEBUG("Closing channel.");
		ssh_channel_close(channel);
		sz = -1;
	}

	return sz;
}

static int
remmina_ssh_cp_to_fd_cb(ssh_session session, ssh_channel channel, void *data, uint32_t len, int is_stderr, void *userdata)
{
	TRACE_CALL(__func__);
	(void)session;
	(void)is_stderr;
	(void)userdata;

//	RemminaSSHShell *shell = (RemminaSSHShell *)userdata;

	node_t *temp_node = remmina_ssh_search_item(channel);
	gint fd = temp_node->fd_out;
	gint sz = 0;

	sz = write(fd, data, len);
	REMMINA_DEBUG("fd bytes written: %d", sz);

	return sz;
}

static void
remmina_ssh_ch_close_cb(ssh_session session, ssh_channel channel, void *userdata)
{
	TRACE_CALL(__func__);
	(void)session;

	RemminaSSHShell *shell = (RemminaSSHShell *)userdata;

	node_t *temp_node = remmina_ssh_search_item(channel);

	if (temp_node != NULL) {
		int fd = temp_node->fd_in;

		if (!temp_node->protected) {
			remmina_ssh_delete_item(channel);
			ssh_event_remove_fd(shell->event, fd);
			shutdown(fd, SHUT_RDWR);
			close(fd);
			REMMINA_DEBUG("fd %d closed.", fd);
		}
	}
	REMMINA_DEBUG("Channel closed.");
}

static void
remmina_ssh_close_all_x11_ch(pthread_t thread)
{
	TRACE_CALL(__func__);

	REMMINA_DEBUG("Close all X11 channels");

	node_t *current = node;
	while (current != NULL) {
		if (current->thread == thread && !current->protected) {
			shutdown(current->fd_in, SHUT_RDWR);
			close(current->fd_in);
			REMMINA_DEBUG("thread: %d - fd %d closed.", thread, current->fd_in);
			if (current->fd_in != current->fd_out) {
				shutdown(current->fd_out, SHUT_RDWR);
				close(current->fd_out);
				REMMINA_DEBUG("thread: %d - fd %d closed.", thread, current->fd_out);
			}
		}
		current = current->next;
	}
}

static ssh_channel
remmina_ssh_x11_open_request_cb(ssh_session session, const char *shost, int sport, void *userdata)
{
	TRACE_CALL(__func__);

	(void)shost;
	(void)sport;

	RemminaSSHShell *shell = (RemminaSSHShell *)userdata;

	ssh_channel channel = ssh_channel_new(session);

	int sock = remmina_ssh_x11_connect_display();

	remmina_ssh_insert_item(channel, sock, sock, FALSE, shell->thread);

	ssh_event_add_fd(shell->event, sock, events, remimna_ssh_cp_to_ch_cb, channel);
	ssh_event_add_session(shell->event, session);

	ssh_add_channel_callbacks(channel, &channel_cb);

	return channel;
}

gchar *
remmina_ssh_identity_path(const gchar *id)
{
	TRACE_CALL(__func__);
	if (id == NULL) return NULL;
	if (id[0] == '/') return g_strdup(id);
	return g_strdup_printf("%s/%s", g_get_home_dir(), id);
}

gchar *
remmina_ssh_find_identity(void)
{
	TRACE_CALL(__func__);
	gchar *path;
	gint i;

	for (i = 0; common_identities[i]; i++) {
		path = remmina_ssh_identity_path(common_identities[i]);
		if (g_file_test(path, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
			return path;
		g_free(path);
	}
	return NULL;
}

void
remmina_ssh_set_error(RemminaSSH *ssh, const gchar *fmt)
{
	TRACE_CALL(__func__);
	const gchar *err;

	err = ssh_get_error(ssh->session);
	ssh->error = g_strdup_printf(fmt, err);
}

void
remmina_ssh_set_application_error(RemminaSSH *ssh, const gchar *fmt, ...)
{
	TRACE_CALL(__func__);
	va_list args;

	va_start(args, fmt);
	ssh->error = g_strdup_vprintf(fmt, args);
	va_end(args);
}

static enum remmina_ssh_auth_result
remmina_ssh_auth_interactive(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint ret;
	gint n;
	gint i;
	const gchar *name, *instruction = NULL;
	//gchar *prompt,*ptr;

	ret = SSH_AUTH_ERROR;
	if (ssh->authenticated) return REMMINA_SSH_AUTH_SUCCESS;
	/* TODO: What if I have an empty password? */
	if (ssh->password == NULL) {
		remmina_ssh_set_error(ssh, "OTP code is empty");
		REMMINA_DEBUG("OTP code is empty, returning");
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}
	REMMINA_DEBUG("OTP code has been set to: %s", ssh->password);

	ret = ssh_userauth_kbdint(ssh->session, NULL, NULL);
	while (ret == SSH_AUTH_INFO) {
		name = ssh_userauth_kbdint_getname(ssh->session);
		if (strlen(name) > 0)
			REMMINA_DEBUG("SSH kbd-interactive name: %s", name);
		else
			REMMINA_DEBUG("SSH kbd-interactive name is empty");
		instruction = ssh_userauth_kbdint_getinstruction(ssh->session);
		if (strlen(instruction) > 0)
			REMMINA_DEBUG("SSH kbd-interactive instruction: %s", instruction);
		else
			REMMINA_DEBUG("SSH kbd-interactive instruction is empty");
		n = ssh_userauth_kbdint_getnprompts(ssh->session);
		for (i = 0; i < n; i++)
			ssh_userauth_kbdint_setanswer(ssh->session, i, ssh->password);
		ret = ssh_userauth_kbdint(ssh->session, NULL, NULL);
	}


	REMMINA_DEBUG("ssh_userauth_kbdint returned %d", ret);
	switch (ret) {
	case SSH_AUTH_PARTIAL:
		if (ssh->password) {
			g_free(ssh->password);
			ssh->password = NULL;
		}
		//You've been partially authenticated, you still have to use another method
		REMMINA_DEBUG("Authenticated with SSH keyboard interactive. Another method is required. %d", ret);
		ssh->is_multiauth = TRUE;
		return REMMINA_SSH_AUTH_PARTIAL;
		break;
	case SSH_AUTH_SUCCESS:
		//Authentication success
		ssh->authenticated = TRUE;
		REMMINA_DEBUG("Authenticated with SSH keyboard interactive. %s", ssh->error);
		return REMMINA_SSH_AUTH_SUCCESS;
		break;
	case SSH_AUTH_INFO:
		//The server asked some questions. Use ssh_userauth_kbdint_getnprompts() and such.
		REMMINA_DEBUG("Authenticating aagin with SSH keyboard interactive??? %s", ssh->error);
		break;
	case SSH_AUTH_AGAIN:
		//In nonblocking mode, you've got to call this again later.
		REMMINA_DEBUG("Authenticated with keyboard interactive, Requested to authenticate again.  %s", ssh->error);
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
		break;
	case SSH_AUTH_DENIED:
	case SSH_AUTH_ERROR:
	default:
		//A serious error happened
		ssh->authenticated = FALSE;
		remmina_ssh_set_error(ssh, _("Could not authenticate with TOTP/OTP/2FA. %s"));
		REMMINA_DEBUG("Cannot authenticate with TOTP/OTP/2FA. Error is %s", ssh->error);
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}
	ssh->authenticated = FALSE;
	return REMMINA_SSH_AUTH_FATAL_ERROR;
}

static enum remmina_ssh_auth_result
remmina_ssh_auth_password(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint ret;

	REMMINA_DEBUG("Password authentication");

	ret = SSH_AUTH_ERROR;
	if (ssh->authenticated) {
		REMMINA_DEBUG("Already authenticated");
		return REMMINA_SSH_AUTH_SUCCESS;
	}
	if (ssh->password == NULL) {
		remmina_ssh_set_error(ssh, "Password is null");
		REMMINA_DEBUG("Password is null, returning");
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}

	ret = ssh_userauth_password(ssh->session, NULL, ssh->password);
	REMMINA_DEBUG("Authentication with SSH password returned: %d", ret);

	switch (ret) {
	case SSH_AUTH_PARTIAL:
		if (ssh->password) {
			g_free(ssh->password);
			ssh->password = NULL;
		}
		//You've been partially authenticated, you still have to use another method.
		REMMINA_DEBUG("Authenticated with SSH password, Another method is required. %d", ret);
		ssh->is_multiauth = TRUE;
		return REMMINA_SSH_AUTH_PARTIAL;
		break;
	case SSH_AUTH_SUCCESS:
		//The public key is accepted.
		ssh->authenticated = TRUE;
		REMMINA_DEBUG("Authenticated with SSH password. %s", ssh->error);
		return REMMINA_SSH_AUTH_SUCCESS;
		break;
	case SSH_AUTH_AGAIN:
		//In nonblocking mode, you've got to call this again later.
		REMMINA_DEBUG("Authenticated with SSH password, Requested to authenticate again.  %s", ssh->error);
		ssh->authenticated = FALSE;
		return REMMINA_SSH_AUTH_AGAIN;
		break;
	case SSH_AUTH_DENIED:
	case SSH_AUTH_ERROR:
	default:
		//A serious error happened.
		ssh->authenticated = FALSE;
		REMMINA_DEBUG("Cannot authenticate with password. Error is %s", ssh->error);
		remmina_ssh_set_error(ssh, _("Could not authenticate with SSH password. %s"));
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}
	ssh->authenticated = FALSE;
	return REMMINA_SSH_AUTH_FATAL_ERROR;
}

static enum remmina_ssh_auth_result
remmina_ssh_auth_pubkey(RemminaSSH *ssh, RemminaProtocolWidget *gp, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);

	ssh_key key = NULL;
	ssh_key cert = NULL;
	gchar pubkey[132] = { 0 }; // +".pub"
	gint ret;

	if (ssh->authenticated) return REMMINA_SSH_AUTH_SUCCESS;

	REMMINA_DEBUG("SSH certificate file: %s", ssh->certfile);
	REMMINA_DEBUG("SSH private key file: %s", ssh->privkeyfile);
	if (ssh->certfile != NULL) {
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 9, 0)
		/* First we import the private key */
		if (ssh_pki_import_privkey_file(ssh->privkeyfile, (ssh->passphrase ? ssh->passphrase : ""),
						NULL, NULL, &key) != SSH_OK) {
			if (ssh->passphrase == NULL || ssh->passphrase[0] == '\0') {
				remmina_ssh_set_error(ssh, _("No saved SSH passphrase supplied. Asking user to enter it."));
				return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
			}

			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(ssh, _("Could not authenticate with public SSH key. %s"));
			return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
		}
		REMMINA_DEBUG ("Imported private SSH key file");
		/* First we import the certificate */
		ret = ssh_pki_import_cert_file(ssh->certfile, &cert );
		if (ret != SSH_OK) {
			REMMINA_DEBUG ("Certificate import returned: %d", ret);
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(ssh, _("SSH certificate cannot be imported. %s"));
			return REMMINA_SSH_AUTH_FATAL_ERROR;
		}
		REMMINA_DEBUG ("certificate imported correctly");
		/* We copy th certificate in the private key */
		ret = ssh_pki_copy_cert_to_privkey(cert, key);
		if (ret != SSH_OK) {
			REMMINA_DEBUG ("Copy certificate into a key returned: %d", ret);
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(ssh, _("SSH certificate cannot be copied into the private SSH key. %s"));
			ssh_key_free(cert);
			return REMMINA_SSH_AUTH_FATAL_ERROR;
		}
		REMMINA_DEBUG ("%s certificate copied into the private SSH key", ssh->certfile);
		/* We try to authenticate */
		ret = ssh_userauth_try_publickey(ssh->session, NULL, cert);
		if (ret != SSH_AUTH_SUCCESS && ret != SSH_AUTH_AGAIN ) {
			REMMINA_DEBUG ("Trying to authenticate with the new key returned: %d", ret);
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(ssh, _("Could not authenticate using SSH certificate. %s"));
			ssh_key_free(key);
			ssh_key_free(cert);
			return REMMINA_SSH_AUTH_FATAL_ERROR;
		}
		REMMINA_DEBUG ("Authentication with a certificate file works, we can authenticate");
#else
		REMMINA_DEBUG ("lbssh >= 0.9.0 is required to authenticate with certificate file");
#endif
		/* if it goes well we authenticate (later on) with the key, not the cert*/
	} else {
		if (ssh->privkeyfile == NULL) {
			// TRANSLATORS: The placeholder %s is an error message
			ssh->error = g_strdup_printf(_("Could not authenticate with public SSH key. %s"),
						     _("SSH identity file not selected."));
			return REMMINA_SSH_AUTH_FATAL_ERROR;
		}

		g_snprintf(pubkey, sizeof(pubkey), "%s.pub", ssh->privkeyfile);

		/*G_FILE_TEST_EXISTS*/
		if (g_file_test(pubkey, G_FILE_TEST_EXISTS)) {
			ret = ssh_pki_import_pubkey_file(pubkey, &key);
			if (ret != SSH_OK) {
				// TRANSLATORS: The placeholder %s is an error message
				remmina_ssh_set_error(ssh, _("Public SSH key cannot be imported. %s"));
				ssh_key_free(key);
				return REMMINA_SSH_AUTH_FATAL_ERROR;
			}
		}

		if (ssh_pki_import_privkey_file(ssh->privkeyfile, (ssh->passphrase ? ssh->passphrase : ""),
						NULL, NULL, &key) != SSH_OK) {
			if (ssh->passphrase == NULL || ssh->passphrase[0] == '\0') {
				remmina_ssh_set_error(ssh, _("No saved SSH passphrase supplied. Asking user to enter it."));
				return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
			}

			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(ssh, _("Could not authenticate with public SSH key. %s"));
			return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
		}
	}

	ret = ssh_userauth_publickey(ssh->session, NULL, key);
	ssh_key_free(key);
	ssh_key_free(cert);
	REMMINA_DEBUG("Authentication with public SSH key returned: %d", ret);

	switch (ret) {
	case SSH_AUTH_PARTIAL:
		if (ssh->password) {
			g_free(ssh->password);
			ssh->password = NULL;
		}
		//You've been partially authenticated, you still have to use another method.
		REMMINA_DEBUG("Authenticated with public SSH key, Another method is required. %d", ret);
		ssh->is_multiauth = TRUE;
		return REMMINA_SSH_AUTH_PARTIAL;
		break;
	case SSH_AUTH_SUCCESS:
		//The public key is accepted.
		ssh->authenticated = TRUE;
		REMMINA_DEBUG("Authenticated with public SSH key. %s", ssh->error);
		return REMMINA_SSH_AUTH_SUCCESS;
		break;
	case SSH_AUTH_AGAIN:
		//In nonblocking mode, you've got to call this again later.
		REMMINA_DEBUG("Authenticated with public SSH key, Requested to authenticate again.  %s", ssh->error);
		ssh->authenticated = FALSE;
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
		break;
	case SSH_AUTH_DENIED:
	case SSH_AUTH_ERROR:
	default:
		//A serious error happened.
		ssh->authenticated = FALSE;
		REMMINA_DEBUG("Could not authenticate with public SSH key. %s", ssh->error);
		remmina_ssh_set_error(ssh, _("Could not authenticate with public SSH key. %s"));
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}
	return REMMINA_SSH_AUTH_FATAL_ERROR;
}

static enum remmina_ssh_auth_result
remmina_ssh_auth_auto_pubkey(RemminaSSH *ssh, RemminaProtocolWidget *gp, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);

	gint ret;
	ret = ssh_userauth_publickey_auto(ssh->session, NULL, (ssh->passphrase ? ssh->passphrase : NULL));

	REMMINA_DEBUG("Authentication with public SSH key returned: %d", ret);

	switch (ret) {
	case SSH_AUTH_PARTIAL:
		if (ssh->password) {
			g_free(ssh->password);
			ssh->password = NULL;
		}
		//You've been partially authenticated, you still have to use another method.
		REMMINA_DEBUG("Authenticated with public SSH key, Another method is required. %d", ret);
		ssh->is_multiauth = TRUE;
		return REMMINA_SSH_AUTH_PARTIAL;
		break;
	case SSH_AUTH_SUCCESS:
		//The public key is accepted.
		ssh->authenticated = TRUE;
		REMMINA_DEBUG("Authenticated with public SSH key. %s", ssh->error);
		return REMMINA_SSH_AUTH_SUCCESS;
		break;
	case SSH_AUTH_AGAIN:
		//In nonblocking mode, you've got to call this again later.
		REMMINA_DEBUG("Authenticated with public SSH key, Requested to authenticate again.  %s", ssh->error);
		ssh->authenticated = FALSE;
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
		break;
	case SSH_AUTH_DENIED:
	case SSH_AUTH_ERROR:
	default:
		//A serious error happened.
		ssh->authenticated = FALSE;
		REMMINA_DEBUG("Cannot authenticate automatically with public SSH key. %s", ssh->error);
		remmina_ssh_set_error(ssh, _("Could not authenticate automatically with public SSH key. %s"));
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}
	ssh->authenticated = FALSE;
	return REMMINA_SSH_AUTH_FATAL_ERROR;
}

static enum remmina_ssh_auth_result
remmina_ssh_auth_agent(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint ret;
	ret = ssh_userauth_agent(ssh->session, NULL);

	REMMINA_DEBUG("Authentication with SSH agent returned: %d", ret);

	switch (ret) {
	case SSH_AUTH_PARTIAL:
		if (ssh->password) {
			g_free(ssh->password);
			ssh->password = NULL;
		}
		//You've been partially authenticated, you still have to use another method.
		REMMINA_DEBUG("Authenticated with public SSH key, Another method is required. %d", ret);
		ssh->is_multiauth = TRUE;
		return REMMINA_SSH_AUTH_PARTIAL;
		break;
	case SSH_AUTH_SUCCESS:
		//The public key is accepted.
		ssh->authenticated = TRUE;
		REMMINA_DEBUG("Authenticated with public SSH key. %s", ssh->error);
		return REMMINA_SSH_AUTH_SUCCESS;
		break;
	case SSH_AUTH_AGAIN:
		//In nonblocking mode, you've got to call this again later.
		REMMINA_DEBUG("Authenticated with public SSH key, Requested to authenticate again.  %s", ssh->error);
		ssh->authenticated = FALSE;
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
		break;
	case SSH_AUTH_DENIED:
	case SSH_AUTH_ERROR:
	default:
		//A serious error happened.
		ssh->authenticated = FALSE;
		REMMINA_DEBUG("Cannot authenticate automatically with SSH agent. %s", ssh->error);
		remmina_ssh_set_error(ssh, _("Could not authenticate automatically with SSH agent. %s"));
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}
	ssh->authenticated = FALSE;
	return REMMINA_SSH_AUTH_FATAL_ERROR;

}

static enum remmina_ssh_auth_result
remmina_ssh_auth_gssapi(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint ret;

	ret = ssh_userauth_gssapi(ssh->session);
	REMMINA_DEBUG("Authentication with SSH GSSAPI/Kerberos: %d", ret);

	switch (ret) {
	case SSH_AUTH_PARTIAL:
		if (ssh->password) {
			g_free(ssh->password);
			ssh->password = NULL;
		}
		//You've been partially authenticated, you still have to use another method.
		REMMINA_DEBUG("Authenticated with public SSH key, Another method is required. %d", ret);
		ssh->is_multiauth = TRUE;
		return REMMINA_SSH_AUTH_PARTIAL;
		break;
	case SSH_AUTH_SUCCESS:
		//The public key is accepted.
		ssh->authenticated = TRUE;
		REMMINA_DEBUG("Authenticated with public SSH key. %s", ssh->error);
		return REMMINA_SSH_AUTH_SUCCESS;
		break;
	case SSH_AUTH_AGAIN:
		//In nonblocking mode, you've got to call this again later.
		REMMINA_DEBUG("Authenticated with public SSH key, Requested to authenticate again.  %s", ssh->error);
		ssh->authenticated = FALSE;
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
		break;
	case SSH_AUTH_DENIED:
	case SSH_AUTH_ERROR:
	default:
		//A serious error happened.
		ssh->authenticated = FALSE;
		REMMINA_DEBUG("Cannot authenticate with SSH GSSAPI/Kerberos. %s", ssh->error);
		remmina_ssh_set_error(ssh, _("Could not authenticate with SSH GSSAPI/Kerberos. %s"));
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}
	ssh->authenticated = FALSE;
	return REMMINA_SSH_AUTH_FATAL_ERROR;
}

enum remmina_ssh_auth_result
remmina_ssh_auth(RemminaSSH *ssh, const gchar *password, RemminaProtocolWidget *gp, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	gint method;
	enum remmina_ssh_auth_result rv = REMMINA_SSH_AUTH_NULL;

	/* Check known host again to ensure it’s still the original server when user forks
	 * a new session from existing one */
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 9, 0)
	/* TODO: Add error checking
	 * SSH_KNOWN_HOSTS_OK: The server is known and has not changed.
	 * SSH_KNOWN_HOSTS_CHANGED: The server key has changed. Either you are under attack or the administrator changed the key. You HAVE to warn the user about a possible attack.
	 * SSH_KNOWN_HOSTS_OTHER: The server gave use a key of a type while we had an other type recorded. It is a possible attack.
	 * SSH_KNOWN_HOSTS_UNKNOWN: The server is unknown. User should confirm the public key hash is correct.
	 * SSH_KNOWN_HOSTS_NOT_FOUND: The known host file does not exist. The host is thus unknown. File will be created if host key is accepted.
	 * SSH_KNOWN_HOSTS_ERROR: There had been an error checking the host.
	 */
	if (ssh_session_is_known_server(ssh->session) != SSH_KNOWN_HOSTS_OK) {
#else
	if (ssh_is_server_known(ssh->session) != SSH_SERVER_KNOWN_OK) {
#endif
		remmina_ssh_set_application_error(ssh, _("The public SSH key changed!"));
		return REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	}

	if (password) {
		if (password != ssh->password) {
			g_free(ssh->password);
			ssh->password = NULL;
		}
		if (password != ssh->passphrase) g_free(ssh->passphrase);
		ssh->password = g_strdup(password);
		ssh->passphrase = g_strdup(password);
	}

	/** @todo Here we should call
	 * gint method;
	 * method = ssh_userauth_list(ssh->session, NULL);
	 *
	 * #define SSH_AUTH_METHOD_UNKNOWN     0x0000u
	 * #define SSH_AUTH_METHOD_NONE        0x0001u
	 * #define SSH_AUTH_METHOD_PASSWORD    0x0002u
	 * #define SSH_AUTH_METHOD_PUBLICKEY   0x0004u
	 * #define SSH_AUTH_METHOD_HOSTBASED   0x0008u
	 * #define SSH_AUTH_METHOD_INTERACTIVE 0x0010u
	 * #define SSH_AUTH_METHOD_GSSAPI_MIC  0x0020u
	 *
	 * And than test both the method and the option selected by the user
	 */
	method = ssh_userauth_list(ssh->session, NULL);
	REMMINA_DEBUG("Methods supported by server: %s%s%s%s%s%s%s",
		      (method & SSH_AUTH_METHOD_NONE) ? "SSH_AUTH_METHOD_NONE " : "",
		      (method & SSH_AUTH_METHOD_UNKNOWN) ? "SSH_AUTH_METHOD_UNKNOWN " : "",
		      (method & SSH_AUTH_METHOD_PASSWORD) ? "SSH_AUTH_METHOD_PASSWORD " : "",
		      (method & SSH_AUTH_METHOD_PUBLICKEY) ? "SSH_AUTH_METHOD_PUBLICKEY " : "",
		      (method & SSH_AUTH_METHOD_HOSTBASED) ? "SSH_AUTH_METHOD_HOSTBASED " : "",
		      (method & SSH_AUTH_METHOD_INTERACTIVE) ? "SSH_AUTH_METHOD_INTERACTIVE " : "",
		      (method & SSH_AUTH_METHOD_GSSAPI_MIC) ? "SSH_AUTH_METHOD_GSSAPI_MIC " : ""
		      );
	switch (ssh->auth) {
	case SSH_AUTH_PASSWORD:
		/* This authentication method is normally disabled on SSHv2 server. You should use keyboard-interactive mode. */
		REMMINA_DEBUG("SSH_AUTH_PASSWORD (%d)", ssh->auth);
		if (ssh->authenticated)
			return REMMINA_SSH_AUTH_SUCCESS;
		if (method & SSH_AUTH_METHOD_PASSWORD) {
			REMMINA_DEBUG("SSH using remmina_ssh_auth_password");
			rv = remmina_ssh_auth_password(ssh);
		}
		if (!ssh->authenticated && (method & SSH_AUTH_METHOD_INTERACTIVE)) {
			/* SSH server is requesting us to do interactive auth. */
			REMMINA_DEBUG("SSH using remmina_ssh_auth_interactive after password has failed");
			rv = remmina_ssh_auth_interactive(ssh);
		}
		if (rv == REMMINA_SSH_AUTH_PARTIAL) {
			if (ssh->password) {
				g_free(ssh->password);
				ssh->password = NULL;
			}
			switch (ssh_userauth_list(ssh->session, NULL)) {
				case SSH_AUTH_METHOD_PASSWORD:
					ssh->auth = SSH_AUTH_PASSWORD;
					break;
				case SSH_AUTH_METHOD_PUBLICKEY:
					ssh->auth = SSH_AUTH_PUBLICKEY;
					break;
				case SSH_AUTH_METHOD_HOSTBASED:
					REMMINA_DEBUG("Host based auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_INTERACTIVE:
					ssh->auth = SSH_AUTH_KBDINTERACTIVE;
					//REMMINA_DEBUG("Interactive auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_UNKNOWN:
				default:
					REMMINA_DEBUG("User auth method not supported: %d", ssh->auth);
					return REMMINA_SSH_AUTH_FATAL_ERROR;
			}
		}
		ssh->error = g_strdup_printf(_("Could not authenticate with SSH password. %s"), "");
		return rv;
		break;

	case SSH_AUTH_KBDINTERACTIVE:
		REMMINA_DEBUG("SSH using remmina_ssh_auth_interactive");
		if (method & SSH_AUTH_METHOD_INTERACTIVE) {
			rv = remmina_ssh_auth_interactive(ssh);
			if (rv == REMMINA_SSH_AUTH_PARTIAL) {
				if (ssh->password) {
					g_free(ssh->password);
					ssh->password = NULL;
				}
				switch (ssh_userauth_list(ssh->session, NULL)) {
				case SSH_AUTH_METHOD_PASSWORD:
					ssh->auth = SSH_AUTH_PASSWORD;
					break;
				case SSH_AUTH_METHOD_PUBLICKEY:
					ssh->auth = SSH_AUTH_PUBLICKEY;
					break;
				case SSH_AUTH_METHOD_HOSTBASED:
					REMMINA_DEBUG("Host based auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_INTERACTIVE:
					ssh->auth = SSH_AUTH_KBDINTERACTIVE;
					//REMMINA_DEBUG("Interactive auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_UNKNOWN:
				default:
					REMMINA_DEBUG("User auth method not supported: %d", ssh->auth);
					return REMMINA_SSH_AUTH_FATAL_ERROR;
				}
			}
			return rv;
		}
		ssh->error = g_strdup_printf(_("Could not authenticate with keyboard-interactive. %s"), "");
		break;

	case SSH_AUTH_PUBLICKEY:
		REMMINA_DEBUG("SSH_AUTH_PUBLICKEY (%d)", ssh->auth);
		if (method & SSH_AUTH_METHOD_PUBLICKEY) {
			rv = remmina_ssh_auth_pubkey(ssh, gp, remminafile);
			if (rv == REMMINA_SSH_AUTH_PARTIAL) {
				if (ssh->password) {
					g_free(ssh->password);
					ssh->password = NULL;
				}
				switch (ssh_userauth_list(ssh->session, NULL)) {
				case SSH_AUTH_METHOD_PASSWORD:
					ssh->auth = SSH_AUTH_PASSWORD;
					break;
				case SSH_AUTH_METHOD_PUBLICKEY:
					ssh->auth = SSH_AUTH_PUBLICKEY;
					break;
				case SSH_AUTH_METHOD_HOSTBASED:
					REMMINA_DEBUG("Host based auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_INTERACTIVE:
					ssh->auth = SSH_AUTH_KBDINTERACTIVE;
					//REMMINA_DEBUG("Interactive auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_UNKNOWN:
				default:
					REMMINA_DEBUG("User auth method not supported: %d", ssh->auth);
					return REMMINA_SSH_AUTH_FATAL_ERROR;
				}
			}
			return rv;
		}
		// The real error here should be: "The SSH server %s:%d does not support public key authentication"
		ssh->error = g_strdup_printf(_("Could not authenticate with public SSH key. %s"), "");
		break;

	case SSH_AUTH_AGENT:
		REMMINA_DEBUG("SSH_AUTH_AGENT (%d)", ssh->auth);
		rv = remmina_ssh_auth_agent(ssh);
		if (rv == REMMINA_SSH_AUTH_PARTIAL) {
			if (ssh->password) {
				g_free(ssh->password);
				ssh->password = NULL;
			}
			switch (ssh_userauth_list(ssh->session, NULL)) {
				case SSH_AUTH_METHOD_PASSWORD:
					ssh->auth = SSH_AUTH_PASSWORD;
					break;
				case SSH_AUTH_METHOD_PUBLICKEY:
					ssh->auth = SSH_AUTH_PUBLICKEY;
					break;
				case SSH_AUTH_METHOD_HOSTBASED:
					REMMINA_DEBUG("Host based auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_INTERACTIVE:
					ssh->auth = SSH_AUTH_KBDINTERACTIVE;
					//REMMINA_DEBUG("Interactive auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_UNKNOWN:
				default:
					REMMINA_DEBUG("User auth method not supported: %d", ssh->auth);
					return REMMINA_SSH_AUTH_FATAL_ERROR;
			}
		}
		return rv;
		break;

	case SSH_AUTH_AUTO_PUBLICKEY:
		REMMINA_DEBUG("SSH_AUTH_AUTO_PUBLICKEY (%d)", ssh->auth);
		rv = remmina_ssh_auth_auto_pubkey(ssh, gp, remminafile);
		/* ssh_agent or none */
		if (method & SSH_AUTH_METHOD_PUBLICKEY) {
			if (rv == REMMINA_SSH_AUTH_PARTIAL) {
				if (ssh->password) {
					g_free(ssh->password);
					ssh->password = NULL;
				}
				switch (ssh_userauth_list(ssh->session, NULL)) {
					case SSH_AUTH_METHOD_PASSWORD:
						ssh->auth = SSH_AUTH_PASSWORD;
						break;
					case SSH_AUTH_METHOD_PUBLICKEY:
						ssh->auth = SSH_AUTH_PUBLICKEY;
						break;
					case SSH_AUTH_METHOD_HOSTBASED:
						REMMINA_DEBUG("Host based auth method not implemented: %d", ssh->auth);
						break;
					case SSH_AUTH_METHOD_INTERACTIVE:
						ssh->auth = SSH_AUTH_KBDINTERACTIVE;
						//REMMINA_DEBUG("Interactive auth method not implemented: %d", ssh->auth);
						break;
					case SSH_AUTH_METHOD_UNKNOWN:
					default:
						REMMINA_DEBUG("User auth method not supported: %d", ssh->auth);
						return REMMINA_SSH_AUTH_FATAL_ERROR;
				}
			}
			return rv;
		}
		// The real error here should be: "The SSH server %s:%d does not support public key authentication"
		ssh->error = g_strdup_printf(_("Could not authenticate with automatic public SSH key. %s"), "");
		break;

#if 0
	/* Not yet supported by libssh */
	case SSH_AUTH_HOSTBASED:
		if (method & SSH_AUTH_METHOD_HOSTBASED)
			//return remmina_ssh_auth_hostbased;
			return 0;
#endif

	case SSH_AUTH_GSSAPI:
		REMMINA_DEBUG("SSH_AUTH_GSSAPI (%d)", ssh->auth);
		if (method & SSH_AUTH_METHOD_GSSAPI_MIC) {
			rv = remmina_ssh_auth_gssapi(ssh);
			if (rv == REMMINA_SSH_AUTH_PARTIAL) {
				if (ssh->password) {
					g_free(ssh->password);
					ssh->password = NULL;
				}
				switch (ssh_userauth_list(ssh->session, NULL)) {
				case SSH_AUTH_METHOD_PASSWORD:
					ssh->auth = SSH_AUTH_PASSWORD;
					break;
				case SSH_AUTH_METHOD_PUBLICKEY:
					ssh->auth = SSH_AUTH_PUBLICKEY;
					break;
				case SSH_AUTH_METHOD_HOSTBASED:
					REMMINA_DEBUG("Host based auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_INTERACTIVE:
					ssh->auth = SSH_AUTH_KBDINTERACTIVE;
					//REMMINA_DEBUG("Interactive auth method not implemented: %d", ssh->auth);
					break;
				case SSH_AUTH_METHOD_UNKNOWN:
				default:
					REMMINA_DEBUG("User auth method not supported: %d", ssh->auth);
					return REMMINA_SSH_AUTH_FATAL_ERROR;
				}
			}
			return rv;
		}
		// The real error here should be: "The SSH server %s:%d does not support SSH GSSAPI/Kerberos authentication"
		ssh->error = g_strdup_printf(_("Could not authenticate with SSH GSSAPI/Kerberos. %s"), "");
		break;

	default:
		REMMINA_DEBUG("User auth method not supported: %d", ssh->auth);
		return REMMINA_SSH_AUTH_FATAL_ERROR;
	}

	// We come here after a "break". ssh->error should be already set
	return REMMINA_SSH_AUTH_FATAL_ERROR;
}

enum remmina_ssh_auth_result
remmina_ssh_auth_gui(RemminaSSH *ssh, RemminaProtocolWidget *gp, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	gchar *keyname;
	gchar *pwdfkey = NULL;
	gchar *message;
	gchar *current_pwd;
	gchar *current_user;
	const gchar *instruction = NULL;
	gint ret;
	size_t len;
	guchar *pubkey;
	ssh_key server_pubkey;
	gboolean disablepasswordstoring;
	gboolean save_password;
	gint attempt;

	/* Check if the server’s public key is known */
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 9, 0)
	/* TODO: Add error checking
	 * SSH_KNOWN_HOSTS_OK: The server is known and has not changed.
	 * SSH_KNOWN_HOSTS_CHANGED: The server key has changed. Either you are under attack or the administrator changed the key. You HAVE to warn the user about a possible attack.
	 * SSH_KNOWN_HOSTS_OTHER: The server gave use a key of a type while we had an other type recorded. It is a possible attack.
	 * SSH_KNOWN_HOSTS_UNKNOWN: The server is unknown. User should confirm the public key hash is correct.
	 * SSH_KNOWN_HOSTS_NOT_FOUND: The known host file does not exist. The host is thus unknown. File will be created if host key is accepted.
	 * SSH_KNOWN_HOSTS_ERROR: There had been an error checking the host.
	 */
	ret = ssh_session_is_known_server(ssh->session);
	switch (ret) {
	case SSH_KNOWN_HOSTS_OK:
		break;                                                  /* ok */

	/*  TODO: These are all wrong, we should deal with each of them */
	case SSH_KNOWN_HOSTS_CHANGED:
	case SSH_KNOWN_HOSTS_OTHER:
	case SSH_KNOWN_HOSTS_UNKNOWN:
	case SSH_KNOWN_HOSTS_NOT_FOUND:
#else
	ret = ssh_is_server_known(ssh->session);
	switch (ret) {
	case SSH_SERVER_KNOWN_OK:
		break;                                                                  /* ok */

	/*  fallback to SSH_SERVER_NOT_KNOWN behavior */
	case SSH_SERVER_KNOWN_CHANGED:
	case SSH_SERVER_FOUND_OTHER:
	case SSH_SERVER_NOT_KNOWN:
	case SSH_SERVER_FILE_NOT_FOUND:
#endif
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 8, 6)
		if (ssh_get_server_publickey(ssh->session, &server_pubkey) != SSH_OK) {
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(ssh, _("Could not fetch the server\'s public SSH key. %s"));
			REMMINA_DEBUG("ssh_get_server_publickey() has failed");
			return REMMINA_SSH_AUTH_FATAL_ERROR;
		}
#else
		if (ssh_get_publickey(ssh->session, &server_pubkey) != SSH_OK) {
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(ssh, _("Could not fetch public SSH key. %s"));
			REMMINA_DEBUG("ssh_get_publickey() has failed");
			return REMMINA_SSH_AUTH_FATAL_ERROR;
		}
#endif
		if (ssh_get_publickey_hash(server_pubkey, SSH_PUBLICKEY_HASH_MD5, &pubkey, &len) != 0) {
			ssh_key_free(server_pubkey);
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(ssh, _("Could not fetch checksum of the public SSH key. %s"));
			REMMINA_DEBUG("ssh_get_publickey_hash() has failed");
			return REMMINA_SSH_AUTH_FATAL_ERROR;
		}
		ssh_key_free(server_pubkey);
		keyname = ssh_get_hexa(pubkey, len);

#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 9, 0)
		if (ret == SSH_KNOWN_HOSTS_UNKNOWN || ret == SSH_KNOWN_HOSTS_NOT_FOUND) {
#else
		if (ret == SSH_SERVER_NOT_KNOWN || ret == SSH_SERVER_FILE_NOT_FOUND) {
#endif
			message = g_strdup_printf("%s\n%s\n\n%s",
						  _("The server is unknown. The public key fingerprint is:"),
						  keyname,
						  _("Do you trust the new public key?"));
		} else {
			message = g_strdup_printf("%s\n%s\n\n%s",
						  _("Warning: The server has changed its public key. This means you are either under attack,\n"
						    "or the administrator has changed the key. The new public key fingerprint is:"),
						  keyname,
						  _("Do you trust the new public key?"));
		}

		ret = remmina_protocol_widget_panel_question_yesno(gp, message);
		g_free(message);

		ssh_string_free_char(keyname);
		ssh_clean_pubkey_hash(&pubkey);
		if (ret != GTK_RESPONSE_YES) return REMMINA_SSH_AUTH_USERCANCEL;
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 9, 0)
		ssh_session_update_known_hosts(ssh->session);
#else
		ssh_write_knownhost(ssh->session);
#endif
		break;
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 9, 0)
	case SSH_KNOWN_HOSTS_ERROR:
#else
	case SSH_SERVER_ERROR:
#endif
	default:
		// TRANSLATORS: The placeholder %s is an error message
		remmina_ssh_set_error(ssh, _("Could not check list of known SSH hosts. %s"));
		REMMINA_DEBUG("Could not check list of known SSH hosts");
		return REMMINA_SSH_AUTH_FATAL_ERROR;
	}

	enum { REMMINA_SSH_AUTH_PASSWORD, REMMINA_SSH_AUTH_PKPASSPHRASE, REMMINA_SSH_AUTH_KRBTOKEN, REMMINA_SSH_AUTH_KBDINTERACTIVE } remmina_ssh_auth_type;

	switch (ssh->auth) {
	case SSH_AUTH_PASSWORD:
		keyname = _("SSH password");
		pwdfkey = ssh->is_tunnel ? "ssh_tunnel_password" : "password";
		remmina_ssh_auth_type = REMMINA_SSH_AUTH_PASSWORD;
		break;
	case SSH_AUTH_PUBLICKEY:
	case SSH_AUTH_AGENT:
	case SSH_AUTH_AUTO_PUBLICKEY:
		keyname = _("SSH private key passphrase");
		pwdfkey = ssh->is_tunnel ? "ssh_tunnel_passphrase" : "ssh_passphrase";
		remmina_ssh_auth_type = REMMINA_SSH_AUTH_PKPASSPHRASE;
		break;
	case SSH_AUTH_GSSAPI:
		keyname = _("SSH Kerberos/GSSAPI");
		pwdfkey = ssh->is_tunnel ? "ssh_tunnel_kerberos_token" : "ssh_kerberos_token";
		remmina_ssh_auth_type = REMMINA_SSH_AUTH_KRBTOKEN;
		break;
	case SSH_AUTH_KBDINTERACTIVE:
		instruction = _("Enter TOTP/OTP/2FA code");
		remmina_ssh_auth_type = REMMINA_SSH_AUTH_KBDINTERACTIVE;
		break;
	default:
		return REMMINA_SSH_AUTH_FATAL_ERROR;
	}

	disablepasswordstoring = remmina_file_get_int(remminafile, "disablepasswordstoring", FALSE);


	current_pwd = g_strdup(remmina_file_get_string(remminafile, pwdfkey));

	/* Try existing password/passphrase first */
	ret = remmina_ssh_auth(ssh, current_pwd, gp, remminafile);
	REMMINA_DEBUG("Returned %d at 1st attempt with the following message:", ret);
	REMMINA_DEBUG("%s", ssh->error);

	/* It seems that functions like ssh_userauth_password() can only be called 3 times
	 * on a ssh connection. And the 3rd failed attempt will block the calling thread forever.
	 * So we retry only 2 extra time authentication. */
	for (attempt = 0;
	     attempt < 2 && ret == REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT;
	     attempt++) {
		if (ssh->error)
			REMMINA_DEBUG("Retrying auth because %s", ssh->error);

		if (remmina_ssh_auth_type == REMMINA_SSH_AUTH_PKPASSPHRASE) {
			ret = remmina_protocol_widget_panel_auth(gp,
								 (disablepasswordstoring ? 0 :
								  REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD),
								 ssh->is_tunnel ? _("SSH tunnel credentials") : _("SSH credentials"),
								 NULL,
								 remmina_file_get_string(remminafile, pwdfkey),
								 NULL,
								 _("SSH private key passphrase"));
			if (ret == GTK_RESPONSE_OK) {
				g_free(current_pwd);
				current_pwd = remmina_protocol_widget_get_password(gp);
				save_password = remmina_protocol_widget_get_savepassword(gp);
				if (save_password)
					remmina_file_set_string(remminafile, pwdfkey, current_pwd);
				else
					remmina_file_set_string(remminafile, pwdfkey, NULL);
			} else {
				g_free(current_pwd);
				return REMMINA_SSH_AUTH_USERCANCEL;
			}
		} else if (remmina_ssh_auth_type == REMMINA_SSH_AUTH_PASSWORD) {
			/* Ask for user credentials. Username cannot be changed here,
			 * because we already sent it when opening the connection */
			REMMINA_DEBUG("Showing panel for password\n");
			current_user = g_strdup(remmina_file_get_string(remminafile, ssh->is_tunnel ? "ssh_tunnel_username" : "username"));
			ret = remmina_protocol_widget_panel_auth(gp,
								 (disablepasswordstoring ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD)
								 | REMMINA_MESSAGE_PANEL_FLAG_USERNAME
								 | (!ssh->is_tunnel ? 0 : REMMINA_MESSAGE_PANEL_FLAG_USERNAME_READONLY),
								 ssh->is_tunnel ? _("SSH tunnel credentials") : _("SSH credentials"),
								 current_user,
								 current_pwd,
								 NULL,
								 NULL);
			if (ret == GTK_RESPONSE_OK) {
				g_free(current_pwd);
				current_pwd = remmina_protocol_widget_get_password(gp);
				save_password = remmina_protocol_widget_get_savepassword(gp);
				if (save_password)
					remmina_file_set_string(remminafile, pwdfkey, current_pwd);
				else
					remmina_file_set_string(remminafile, pwdfkey, NULL);

				if (!ssh->is_tunnel && !ssh->is_multiauth) {
					g_free(current_user);
					current_user = remmina_protocol_widget_get_username(gp);
					remmina_file_set_string(remminafile, "username", current_user);
					if (ssh->user != NULL) {
						g_free(ssh->user);
						ssh->user = NULL;
					}
					ssh->user = g_strdup(current_user);
					if (ssh->password != NULL) {
						g_free(ssh->password);
						ssh->password = NULL;
					}
					ssh->password = g_strdup(current_pwd);
					g_free(current_user);
					return REMMINA_SSH_AUTH_RECONNECT;
				}
				g_free(current_user);
			} else {
				g_free(current_pwd);
				g_free(current_user);
				return REMMINA_SSH_AUTH_USERCANCEL;
			}
		} else if (remmina_ssh_auth_type == REMMINA_SSH_AUTH_KBDINTERACTIVE) {
			REMMINA_DEBUG("Showing panel for keyboard interactive login\n");
			/**
			 * gp
			 * flags
			 * title
			 * default_username
			 * default_password
			 * default_domain
			 * password_prompt
			 */
			ret = remmina_protocol_widget_panel_auth(
				gp,
				0,
				_("Keyboard interactive login, TOTP/OTP/2FA"),
				NULL,
				NULL,
				NULL,
				instruction);
			if (ret == GTK_RESPONSE_OK) {
				g_free(current_pwd);
				current_pwd = remmina_protocol_widget_get_password(gp);
				REMMINA_DEBUG("OTP code is: %s", current_pwd);
				ssh->password = g_strdup(current_pwd);
			} else {
				g_free(current_pwd);
				return REMMINA_SSH_AUTH_USERCANCEL;
			}
		} else {
			g_print("Unimplemented.");
			g_free(current_pwd);
			return REMMINA_SSH_AUTH_FATAL_ERROR;
		}
		REMMINA_DEBUG("Retrying authentication");
		ret = remmina_ssh_auth(ssh, current_pwd, gp, remminafile);
		REMMINA_DEBUG("Authentication attempt n° %d returned %d with the following message:", attempt + 2, ret);
		REMMINA_DEBUG("%s", ssh->error);
	}

	g_free(current_pwd); current_pwd = NULL;

	/* After attempting the max number of times, REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT
	 * becomes REMMINA_SSH_AUTH_FATAL_ERROR */
	if (ret == REMMINA_SSH_AUTH_AUTHFAILED_RETRY_AFTER_PROMPT || ret == REMMINA_SSH_AUTH_AGAIN) {
		REMMINA_DEBUG("SSH Authentication failed");
		ret = REMMINA_SSH_AUTH_FATAL_ERROR;
	}

	return ret;
}

void
remmina_ssh_log_callback(ssh_session session, int priority, const char *message, void *userdata)
{
	TRACE_CALL(__func__);
	REMMINA_DEBUG(message);
}

gboolean
remmina_ssh_init_session(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	gint verbosity;
	gint rc;
	gchar *parsed_config;
#ifdef HAVE_NETINET_TCP_H
	socket_t sshsock;
	gint optval;
#endif

	ssh->callback = g_new0(struct ssh_callbacks_struct, 1);

	/* Init & startup the SSH session */
	REMMINA_DEBUG("server=%s port=%d is_tunnel=%s tunnel_entrance_host=%s tunnel_entrance_port=%d",
		      ssh->server,
		      ssh->port,
		      ssh->is_tunnel ? "Yes" : "No",
		      ssh->tunnel_entrance_host, ssh->tunnel_entrance_port);

	ssh->session = ssh_new();

	/* Tunnel sanity checks */
	if (ssh->is_tunnel && ssh->tunnel_entrance_host != NULL) {
		ssh->error = g_strdup_printf("Internal error in %s: is_tunnel and tunnel_entrance != NULL", __func__);
		REMMINA_DEBUG(ssh->error);
		return FALSE;
	}
	if (!ssh->is_tunnel && ssh->tunnel_entrance_host == NULL) {
		ssh->error = g_strdup_printf("Internal error in %s: is_tunnel == false and tunnel_entrance == NULL", __func__);
		REMMINA_DEBUG(ssh->error);
		return FALSE;
	}

	/* Set connection host/port */
	if (ssh->is_tunnel) {
		ssh_options_set(ssh->session, SSH_OPTIONS_HOST, ssh->server);
		ssh_options_set(ssh->session, SSH_OPTIONS_PORT, &ssh->port);
		REMMINA_DEBUG("Setting SSH_OPTIONS_HOST to %s and SSH_OPTIONS_PORT to %d", ssh->server, ssh->port);
	} else {
		ssh_options_set(ssh->session, SSH_OPTIONS_HOST, ssh->tunnel_entrance_host);
		ssh_options_set(ssh->session, SSH_OPTIONS_PORT, &ssh->tunnel_entrance_port);
		REMMINA_DEBUG("Setting SSH_OPTIONS_HOST to %s and SSH_OPTIONS_PORT to %d", ssh->tunnel_entrance_host, ssh->tunnel_entrance_port);
	}

	if (ssh->privkeyfile && *ssh->privkeyfile != 0) {
		rc = ssh_options_set(ssh->session, SSH_OPTIONS_IDENTITY, ssh->privkeyfile);
		if (rc == 0)
			REMMINA_DEBUG("SSH_OPTIONS_IDENTITY is now %s", ssh->privkeyfile);
		else
			REMMINA_DEBUG("SSH_OPTIONS_IDENTITY is not set, by default the files “identity”, “id_dsa” and “id_rsa” are used.");
	}

#ifdef SNAP_BUILD
	ssh_options_set(ssh->session, SSH_OPTIONS_SSH_DIR, g_strdup_printf("%s/.ssh", g_getenv("SNAP_USER_COMMON")));
#endif
	ssh_callbacks_init(ssh->callback);
	if (remmina_log_running()) {
		verbosity = remmina_pref.ssh_loglevel;
		ssh_options_set(ssh->session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
		ssh->callback->log_function = remmina_ssh_log_callback;
		/* Reset libssh legacy userdata. This is a workaround for a libssh bug */
		ssh_set_log_userdata(ssh->session);
	}
	ssh->callback->userdata = ssh;
	ssh_set_callbacks(ssh->session, ssh->callback);

	/* As the latest parse the ~/.ssh/config file */
	if (g_strcmp0(ssh->tunnel_entrance_host, "127.0.0.1") == 0) {
		REMMINA_DEBUG("SSH_OPTIONS_HOST temporary set to the destination host as ssh->tunnel_entrance_host is 127.0.0.1,");
		ssh_options_set(ssh->session, SSH_OPTIONS_HOST, ssh->server);
	}
	if (remmina_pref.ssh_parseconfig) {
		if (ssh_options_parse_config(ssh->session, NULL) == 0)
			REMMINA_DEBUG("ssh_config have been correctly parsed");
		else
			REMMINA_DEBUG("Cannot parse ssh_config: %s", ssh_get_error(ssh->session));
	}
	if (g_strcmp0(ssh->tunnel_entrance_host, "127.0.0.1") == 0) {
		REMMINA_DEBUG("Setting SSH_OPTIONS_HOST to ssh->tunnel_entrance_host is 127.0.0.1,");
		ssh_options_set(ssh->session, SSH_OPTIONS_HOST, ssh->tunnel_entrance_host);
	}
	if (!ssh->user || *ssh->user == 0) {
    rc = ssh_options_get(ssh->session, SSH_OPTIONS_USER, &parsed_config);
    if (rc == SSH_OK) {
      if (ssh->user)
        g_free(ssh->user);
      ssh->user = g_strdup(parsed_config);
      ssh_string_free_char(parsed_config);
    } else {
      REMMINA_DEBUG("Parsing ssh_config for SSH_OPTIONS_USER returned an error: %s", ssh_get_error(ssh->session));
    }
  }
  ssh_options_set(ssh->session, SSH_OPTIONS_USER, ssh->user);
	REMMINA_DEBUG("SSH_OPTIONS_USER is now %s", ssh->user);

	/* SSH_OPTIONS_PROXYCOMMAND */
	rc = ssh_options_get(ssh->session, SSH_OPTIONS_PROXYCOMMAND, &parsed_config);
	if (rc == SSH_OK) {
		ssh->proxycommand = g_strdup(parsed_config);
		ssh_string_free_char(parsed_config);
	} else {
		REMMINA_DEBUG("Parsing ssh_config for SSH_OPTIONS_PROXYCOMMAND returned an error: %s", ssh_get_error(ssh->session));
	}
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_PROXYCOMMAND, ssh->proxycommand);
	if (rc == 0)
		REMMINA_DEBUG("SSH_OPTIONS_PROXYCOMMAND is now %s", ssh->proxycommand);
	else
		REMMINA_DEBUG("SSH_OPTIONS_PROXYCOMMAND does not have a valid value. %s", ssh->proxycommand);

	/* SSH_OPTIONS_HOSTKEYS */
	rc = ssh_options_get(ssh->session, SSH_OPTIONS_HOSTKEYS, &parsed_config);
	if (rc == SSH_OK) {
		ssh->hostkeytypes = g_strdup(parsed_config);
		ssh_string_free_char(parsed_config);
	} else {
		REMMINA_DEBUG("Parsing ssh_config for SSH_OPTIONS_HOSTKEYS returned an error: %s", ssh_get_error(ssh->session));
	}
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_HOSTKEYS, ssh->hostkeytypes);
	if (rc == 0)
		REMMINA_DEBUG("SSH_OPTIONS_HOSTKEYS is now %s", ssh->hostkeytypes);
	else
		REMMINA_DEBUG("SSH_OPTIONS_HOSTKEYS does not have a valid value. %s", ssh->hostkeytypes);

	/* SSH_OPTIONS_KEY_EXCHANGE */
	rc = ssh_options_get(ssh->session, SSH_OPTIONS_KEY_EXCHANGE, &parsed_config);
	if (rc == SSH_OK) {
		ssh->kex_algorithms = g_strdup(parsed_config);
		ssh_string_free_char(parsed_config);
	} else {
		REMMINA_DEBUG("Parsing ssh_config for SSH_OPTIONS_KEY_EXCHANGE returned an error: %s", ssh_get_error(ssh->session));
	}
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_KEY_EXCHANGE, ssh->kex_algorithms);
	if (rc == 0)
		REMMINA_DEBUG("SSH_OPTIONS_KEY_EXCHANGE is now %s", ssh->kex_algorithms);
	else
		REMMINA_DEBUG("SSH_OPTIONS_KEY_EXCHANGE does not have a valid value. %s", ssh->kex_algorithms);

	/* SSH_OPTIONS_CIPHERS_C_S */
	rc = ssh_options_get(ssh->session, SSH_OPTIONS_CIPHERS_C_S, &parsed_config);
	if (rc == SSH_OK) {
		ssh->ciphers = g_strdup(parsed_config);
		ssh_string_free_char(parsed_config);
	} else {
		REMMINA_DEBUG("Parsing ssh_config for SSH_OPTIONS_CIPHERS_C_S returned an error: %s", ssh_get_error(ssh->session));
	}
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_CIPHERS_C_S, ssh->ciphers);
	if (rc == 0)
		REMMINA_DEBUG("SSH_OPTIONS_CIPHERS_C_S has been set to %s", ssh->ciphers);
	else
		REMMINA_DEBUG("SSH_OPTIONS_CIPHERS_C_S does not have a valid value. %s", ssh->ciphers);
	/* SSH_OPTIONS_STRICTHOSTKEYCHECK */
	rc = ssh_options_get(ssh->session, SSH_OPTIONS_STRICTHOSTKEYCHECK, &parsed_config);
	if (rc == SSH_OK) {
		ssh->stricthostkeycheck = atoi(parsed_config);
		ssh_string_free_char(parsed_config);
	} else {
		REMMINA_DEBUG("Parsing ssh_config for SSH_OPTIONS_STRICTHOSTKEYCHECK returned an error: %s", ssh_get_error(ssh->session));
	}
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_STRICTHOSTKEYCHECK, &ssh->stricthostkeycheck);
	if (rc == 0)
		REMMINA_DEBUG("SSH_OPTIONS_STRICTHOSTKEYCHECK is now %d", ssh->stricthostkeycheck);
	else
		REMMINA_DEBUG("SSH_OPTIONS_STRICTHOSTKEYCHECK does not have a valid value. %d", ssh->stricthostkeycheck);
	/* SSH_OPTIONS_COMPRESSION */
	rc = ssh_options_get(ssh->session, SSH_OPTIONS_COMPRESSION, &parsed_config);
	if (rc == SSH_OK) {
		ssh->compression = g_strdup(parsed_config);
		ssh_string_free_char(parsed_config);
	} else {
		REMMINA_DEBUG("Parsing ssh_config for SSH_OPTIONS_COMPRESSION returned an error: %s", ssh_get_error(ssh->session));
	}
	rc = ssh_options_set(ssh->session, SSH_OPTIONS_COMPRESSION, ssh->compression);
	if (rc == 0)
		REMMINA_DEBUG("SSH_OPTIONS_COMPRESSION is now %s", ssh->compression);
	else
		REMMINA_DEBUG("SSH_OPTIONS_COMPRESSION does not have a valid value. %s", ssh->compression);

	if (ssh_connect(ssh->session)) {
		// TRANSLATORS: The placeholder %s is an error message
		remmina_ssh_set_error(ssh, _("Could not start SSH session. %s"));
		return FALSE;
	}

#ifdef HAVE_NETINET_TCP_H
	/* Set keepalive on SSH socket, so we can keep firewalls awaken and detect
	 * when we loss the tunnel */
	sshsock = ssh_get_fd(ssh->session);
	if (sshsock >= 0) {
		optval = 1;
		if (setsockopt(sshsock, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) < 0)
			REMMINA_DEBUG("TCP KeepAlive not set");
		else
			REMMINA_DEBUG("TCP KeepAlive enabled");

#ifdef TCP_KEEPIDLE
		optval = remmina_pref.ssh_tcp_keepidle;
		if (setsockopt(sshsock, IPPROTO_TCP, TCP_KEEPIDLE, &optval, sizeof(optval)) < 0)
			REMMINA_DEBUG("TCP_KEEPIDLE not set");
		else
			REMMINA_DEBUG("TCP_KEEPIDLE set to %i", optval);

#endif
#ifdef TCP_KEEPCNT
		optval = remmina_pref.ssh_tcp_keepcnt;
		if (setsockopt(sshsock, IPPROTO_TCP, TCP_KEEPCNT, &optval, sizeof(optval)) < 0)
			REMMINA_DEBUG("TCP_KEEPCNT not set");
		else
			REMMINA_DEBUG("TCP_KEEPCNT set to %i", optval);

#endif
#ifdef TCP_KEEPINTVL
		optval = remmina_pref.ssh_tcp_keepintvl;
		if (setsockopt(sshsock, IPPROTO_TCP, TCP_KEEPINTVL, &optval, sizeof(optval)) < 0)
			REMMINA_DEBUG("TCP_KEEPINTVL not set");
		else
			REMMINA_DEBUG("TCP_KEEPINTVL set to %i", optval);

#endif
#ifdef TCP_USER_TIMEOUT
		optval = remmina_pref.ssh_tcp_usrtimeout;
		if (setsockopt(sshsock, IPPROTO_TCP, TCP_USER_TIMEOUT, &optval, sizeof(optval)) < 0)
			REMMINA_DEBUG("TCP_USER_TIMEOUT not set");
		else
			REMMINA_DEBUG("TCP_USER_TIMEOUT set to %i", optval);

#endif
	}
#endif

	/* Try the "none" authentication */
	if (ssh_userauth_none(ssh->session, NULL) == SSH_AUTH_SUCCESS)
		ssh->authenticated = TRUE;
	return TRUE;
}

gboolean
remmina_ssh_init_from_file(RemminaSSH *ssh, RemminaFile *remminafile, gboolean is_tunnel)
{
	TRACE_CALL(__func__);
	const gchar *username;
	const gchar *privatekey;
	const gchar *server;
	gchar *s;

	ssh->session = NULL;
	ssh->callback = NULL;
	ssh->authenticated = FALSE;
	ssh->error = NULL;
	ssh->passphrase = NULL;
	ssh->is_tunnel = is_tunnel;
	pthread_mutex_init(&ssh->ssh_mutex, NULL);

	ssh->tunnel_entrance_host = NULL;
	ssh->tunnel_entrance_port = 0;

	username = remmina_file_get_string(remminafile, is_tunnel ? "ssh_tunnel_username" : "username");
	privatekey = remmina_file_get_string(remminafile, is_tunnel ? "ssh_tunnel_privatekey" : "ssh_privatekey");
	ssh->certfile = g_strdup(remmina_file_get_string(remminafile, is_tunnel ? "ssh_tunnel_certfile" : "ssh_certfile"));

	/* The ssh->server and ssh->port values */
	if (is_tunnel) {
		REMMINA_DEBUG("We are initializing an SSH tunnel session");
		server = remmina_file_get_string(remminafile, "ssh_tunnel_server");
		if (server == NULL || server[0] == 0) {
			// ssh_tunnel_server empty or invalid, we are opening a tunnel, it means that "Same server at port 22" has been selected
			server = remmina_file_get_string(remminafile, "server");
			if (server == NULL || server[0] == 0)
				server = "localhost";
			REMMINA_DEBUG("Calling remmina_public_get_server_port");
			remmina_public_get_server_port(server, 22, &ssh->server, &ssh->port);
			ssh->port = 22;
		} else {
			REMMINA_DEBUG("Calling remmina_public_get_server_port");
			remmina_public_get_server_port(server, 22, &ssh->server, &ssh->port);
		}
		REMMINA_DEBUG("server:port = %s, server = %s, port = %d", server, ssh->server, ssh->port);
	} else {
		REMMINA_DEBUG("We are initializing an SSH session");
		server = remmina_file_get_string(remminafile, "server");
		if (server == NULL || server[0] == 0)
			server = "localhost";
		REMMINA_DEBUG("Calling remmina_public_get_server_port");
		remmina_public_get_server_port(server, 22, &ssh->server, &ssh->port);
		REMMINA_DEBUG("server:port = %s, server = %s, port = %d", server, ssh->server, ssh->port);
	}

	if (ssh->server[0] == '\0') {
		g_free(ssh->server);
		// ???
		REMMINA_DEBUG("Calling remmina_public_get_server_port");
		remmina_public_get_server_port(server, 0, &ssh->server, NULL);
	}

	REMMINA_DEBUG("Initialized SSH struct from file with ssh->server = %s and SSH->port = %d", ssh->server, ssh->port);

	ssh->user = g_strdup(username ? username : NULL);
	ssh->password = NULL;
	ssh->auth = remmina_file_get_int(remminafile, is_tunnel ? "ssh_tunnel_auth" : "ssh_auth", 0);
	ssh->charset = g_strdup(remmina_file_get_string(remminafile, "ssh_charset"));
	ssh->kex_algorithms = g_strdup(remmina_file_get_string(remminafile, is_tunnel ? "ssh_tunnel_kex_algorithms" : "ssh_kex_algorithms"));
	ssh->ciphers = g_strdup(remmina_file_get_string(remminafile, is_tunnel ? "ssh_tunnel_ciphers" : "ssh_ciphers"));
	ssh->hostkeytypes = g_strdup(remmina_file_get_string(remminafile, is_tunnel ? "ssh_tunnel_hostkeytypes" : "ssh_hostkeytypes"));
	ssh->proxycommand = g_strdup(remmina_file_get_string(remminafile, is_tunnel ? "ssh_tunnel_proxycommand" : "ssh_proxycommand"));
	ssh->stricthostkeycheck = remmina_file_get_int(remminafile, is_tunnel ? "ssh_tunnel_stricthostkeycheck" : "ssh_stricthostkeycheck", 0);
	gint c = remmina_file_get_int(remminafile, is_tunnel ? "ssh_tunnel_compression" : "ssh_compression", 0);
	ssh->compression = (c == 1) ? "yes" : "no";

	REMMINA_DEBUG("ssh->user: %s", ssh->user);
	REMMINA_DEBUG("ssh->password: %s", ssh->password);
	REMMINA_DEBUG("ssh->auth: %d", ssh->auth);
	REMMINA_DEBUG("ssh->charset: %s", ssh->charset);
	REMMINA_DEBUG("ssh->kex_algorithms: %s", ssh->kex_algorithms);
	REMMINA_DEBUG("ssh->ciphers: %s", ssh->ciphers);
	REMMINA_DEBUG("ssh->hostkeytypes: %s", ssh->hostkeytypes);
	REMMINA_DEBUG("ssh->proxycommand: %s", ssh->proxycommand);
	REMMINA_DEBUG("ssh->stricthostkeycheck: %d", ssh->stricthostkeycheck);
	REMMINA_DEBUG("ssh->compression: %s", ssh->compression);

	/* Public/Private keys */
	s = (privatekey ? g_strdup(privatekey) : remmina_ssh_find_identity());
	if (s) {
		ssh->privkeyfile = remmina_ssh_identity_path(s);
		REMMINA_DEBUG("ssh->privkeyfile: %s", ssh->privkeyfile);
		g_free(s);
	} else {
		ssh->privkeyfile = NULL;
	}

	return TRUE;
}

static gboolean
remmina_ssh_init_from_ssh(RemminaSSH *ssh, const RemminaSSH *ssh_src)
{
	TRACE_CALL(__func__);
	ssh->session = NULL;
	ssh->authenticated = FALSE;
	ssh->error = NULL;
	pthread_mutex_init(&ssh->ssh_mutex, NULL);

	ssh->is_tunnel = ssh_src->is_tunnel;
	ssh->server = g_strdup(ssh_src->server);
	ssh->port = ssh_src->port;
	ssh->user = g_strdup(ssh_src->user ? ssh_src->user : NULL);
	ssh->auth = ssh_src->auth;
	ssh->password = g_strdup(ssh_src->password);
	ssh->passphrase = g_strdup(ssh_src->passphrase);
	ssh->privkeyfile = g_strdup(ssh_src->privkeyfile);
	ssh->certfile = g_strdup(ssh_src->certfile);
	ssh->charset = g_strdup(ssh_src->charset);
	ssh->proxycommand = g_strdup(ssh_src->proxycommand);
	ssh->kex_algorithms = g_strdup(ssh_src->kex_algorithms);
	ssh->ciphers = g_strdup(ssh_src->ciphers);
	ssh->hostkeytypes = g_strdup(ssh_src->hostkeytypes);
	ssh->stricthostkeycheck = ssh_src->stricthostkeycheck;
	ssh->compression = ssh_src->compression;
	ssh->tunnel_entrance_host = g_strdup(ssh_src->tunnel_entrance_host);
	ssh->tunnel_entrance_port = ssh_src->tunnel_entrance_port;

	return TRUE;
}

gchar *
remmina_ssh_convert(RemminaSSH *ssh, const gchar *from)
{
	TRACE_CALL(__func__);
	gchar *to = NULL;

	if (ssh->charset && from)
		to = g_convert(from, -1, "UTF-8", ssh->charset, NULL, NULL, NULL);
	if (!to) to = g_strdup(from);
	return to;
}

gchar *
remmina_ssh_unconvert(RemminaSSH *ssh, const gchar *from)
{
	TRACE_CALL(__func__);
	gchar *to = NULL;

	if (ssh->charset && from)
		to = g_convert(from, -1, ssh->charset, "UTF-8", NULL, NULL, NULL);
	if (!to) to = g_strdup(from);
	return to;
}

void
remmina_ssh_free(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	if (ssh->session) {
		REMMINA_DEBUG("Disconnecting SSH session");
		ssh_disconnect(ssh->session);
		ssh_free(ssh->session);
		ssh->session = NULL;
	}
	g_free(ssh->callback);
	g_free(ssh->server);
	g_free(ssh->user);
	g_free(ssh->password);
	g_free(ssh->privkeyfile);
	g_free(ssh->certfile);
	g_free(ssh->charset);
	g_free(ssh->error);
	pthread_mutex_destroy(&ssh->ssh_mutex);
	g_free(ssh);
}

/*-----------------------------------------------------------------------------*
*                           SSH Tunnel                                        *
*-----------------------------------------------------------------------------*/
struct _RemminaSSHTunnelBuffer {
	gchar * data;
	gchar * ptr;
	ssize_t len;
};

static RemminaSSHTunnelBuffer *
remmina_ssh_tunnel_buffer_new(ssize_t len)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnelBuffer *buffer;

	buffer = g_new(RemminaSSHTunnelBuffer, 1);
	buffer->data = (gchar *)g_malloc(len);
	buffer->ptr = buffer->data;
	buffer->len = len;
	return buffer;
}

static void
remmina_ssh_tunnel_buffer_free(RemminaSSHTunnelBuffer *buffer)
{
	TRACE_CALL(__func__);
	if (buffer) {
		g_free(buffer->data);
		g_free(buffer);
	}
}

RemminaSSHTunnel *
remmina_ssh_tunnel_new_from_file(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel;

	tunnel = g_new(RemminaSSHTunnel, 1);

	remmina_ssh_init_from_file(REMMINA_SSH(tunnel), remminafile, TRUE);

	tunnel->tunnel_type = -1;
	tunnel->channels = NULL;
	tunnel->sockets = NULL;
	tunnel->socketbuffers = NULL;
	tunnel->num_channels = 0;
	tunnel->max_channels = 0;
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
remmina_ssh_tunnel_close_all_channels(RemminaSSHTunnel *tunnel)
{
	TRACE_CALL(__func__);
	int i;

	for (i = 0; i < tunnel->num_channels; i++) {
		close(tunnel->sockets[i]);
		remmina_ssh_tunnel_buffer_free(tunnel->socketbuffers[i]);
		ssh_channel_close(tunnel->channels[i]);
		ssh_channel_send_eof(tunnel->channels[i]);
		ssh_channel_free(tunnel->channels[i]);
	}

	g_free(tunnel->channels);
	tunnel->channels = NULL;
	g_free(tunnel->sockets);
	tunnel->sockets = NULL;
	g_free(tunnel->socketbuffers);
	tunnel->socketbuffers = NULL;

	tunnel->num_channels = 0;
	tunnel->max_channels = 0;
}

static void
remmina_ssh_tunnel_remove_channel(RemminaSSHTunnel *tunnel, gint n)
{
	TRACE_CALL(__func__);
	ssh_channel_close(tunnel->channels[n]);
	ssh_channel_send_eof(tunnel->channels[n]);
	ssh_channel_free(tunnel->channels[n]);
	close(tunnel->sockets[n]);
	remmina_ssh_tunnel_buffer_free(tunnel->socketbuffers[n]);
	tunnel->num_channels--;
	tunnel->channels[n] = tunnel->channels[tunnel->num_channels];
	tunnel->channels[tunnel->num_channels] = NULL;
	tunnel->sockets[n] = tunnel->sockets[tunnel->num_channels];
	tunnel->socketbuffers[n] = tunnel->socketbuffers[tunnel->num_channels];
}

/* Register the new channel/socket pair */
static void
remmina_ssh_tunnel_add_channel(RemminaSSHTunnel *tunnel, ssh_channel channel, gint sock)
{
	TRACE_CALL(__func__);
	gint flags;
	gint i;

	i = tunnel->num_channels++;
	if (tunnel->num_channels > tunnel->max_channels) {
		/* Allocate an extra NULL pointer in channels for ssh_select */
		tunnel->channels = (ssh_channel *)g_realloc(tunnel->channels,
							    sizeof(ssh_channel) * (tunnel->num_channels + 1));
		tunnel->sockets = (gint *)g_realloc(tunnel->sockets,
						    sizeof(gint) * tunnel->num_channels);
		tunnel->socketbuffers = (RemminaSSHTunnelBuffer **)g_realloc(tunnel->socketbuffers,
									     sizeof(RemminaSSHTunnelBuffer *) * tunnel->num_channels);
		tunnel->max_channels = tunnel->num_channels;

		tunnel->channels_out = (ssh_channel *)g_realloc(tunnel->channels_out,
								sizeof(ssh_channel) * (tunnel->num_channels + 1));
	}
	tunnel->channels[i] = channel;
	tunnel->channels[i + 1] = NULL;
	tunnel->sockets[i] = sock;
	tunnel->socketbuffers[i] = NULL;

	flags = fcntl(sock, F_GETFL, 0);
	fcntl(sock, F_SETFL, flags | O_NONBLOCK);
}

static int
remmina_ssh_tunnel_accept_local_connection(RemminaSSHTunnel *tunnel, gboolean blocking)
{
	gint sock, sock_flags;

	sock_flags = fcntl(tunnel->server_sock, F_GETFL, 0);
	if (blocking)
		sock_flags &= ~O_NONBLOCK;
	else
		sock_flags |= O_NONBLOCK;
	fcntl(tunnel->server_sock, F_SETFL, sock_flags);

	/* Accept a local connection */
	sock = accept(tunnel->server_sock, NULL, NULL);
	if (sock < 0) {
		if (blocking) {
			g_free(REMMINA_SSH(tunnel)->error);
			REMMINA_SSH(tunnel)->error = g_strdup("Local socket not accepted");
		}
	}

	return sock;
}

static ssh_channel
remmina_ssh_tunnel_create_forward_channel(RemminaSSHTunnel *tunnel)
{
	ssh_channel channel = NULL;

	channel = ssh_channel_new(tunnel->ssh.session);
	if (!channel) {
		// TRANSLATORS: The placeholder %s is an error message
		remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not create channel. %s"));
		return NULL;
	}

	/* Request the SSH server to connect to the destination */
	REMMINA_DEBUG("SSH tunnel destination is %s", tunnel->dest);
	if (ssh_channel_open_forward(channel, tunnel->dest, tunnel->port, "127.0.0.1", 0) != SSH_OK) {
		ssh_channel_close(channel);
		ssh_channel_send_eof(channel);
		ssh_channel_free(channel);
		// TRANSLATORS: The placeholder %s is an error message
		remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not connect to SSH tunnel. %s"));
		return NULL;
	}

	return channel;
}

static gpointer
remmina_ssh_tunnel_main_thread_proc(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel = (RemminaSSHTunnel *)data;
	gchar *ptr;
	ssize_t len = 0, lenw = 0;
	fd_set set;
	struct timeval timeout;
	g_autoptr(GDateTime) t1 = NULL;
	g_autoptr(GDateTime) t2 = NULL;
	GTimeSpan diff;                                                 // microseconds
	ssh_channel channel = NULL;
	gboolean first = TRUE;
	gboolean disconnected;
	gint sock;
	gint maxfd;
	gint i;
	gint ret;
	struct sockaddr_in sin;

	t1 = t2 = g_date_time_new_now_local();

	switch (tunnel->tunnel_type) {
	case REMMINA_SSH_TUNNEL_OPEN:
		sock = remmina_ssh_tunnel_accept_local_connection(tunnel, TRUE);
		if (sock < 0) {
			if (tunnel)
				tunnel->thread = 0;
			return NULL;
		}

		channel = remmina_ssh_tunnel_create_forward_channel(tunnel);
		if (!tunnel) {
			close(sock);
			tunnel->thread = 0;
			return NULL;
		}

		remmina_ssh_tunnel_add_channel(tunnel, channel, sock);
		break;

	case REMMINA_SSH_TUNNEL_XPORT:
		/* Detect the next available port starting from 6010 on the server */
		for (i = 10; i <= MAX_X_DISPLAY_NUMBER; i++) {
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 0)
			if (ssh_channel_listen_forward(REMMINA_SSH(tunnel)->session, (tunnel->bindlocalhost ? "localhost" : NULL), 6000 + i, NULL)) {
				continue;
			} else {
				tunnel->remotedisplay = i;
				break;
			}
#else
			if (ssh_forward_listen(REMMINA_SSH(tunnel)->session, (tunnel->bindlocalhost ? "localhost" : NULL), 6000 + i, NULL)) {
				continue;
			} else {
				tunnel->remotedisplay = i;
				break;
			}
#endif
		}
		if (tunnel->remotedisplay < 1) {
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not request port forwarding. %s"));
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}

		if (tunnel->init_func &&
		    !(*tunnel->init_func)(tunnel, tunnel->callback_data)) {
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}

		break;

	case REMMINA_SSH_TUNNEL_REVERSE:
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 0)
		if (ssh_channel_listen_forward(REMMINA_SSH(tunnel)->session, NULL, tunnel->port, NULL)) {
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not request port forwarding. %s"));
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}
#else
		if (ssh_forward_listen(REMMINA_SSH(tunnel)->session, NULL, tunnel->port, NULL)) {
			// TRANSLATORS: The placeholder %s is an error message
			remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not request port forwarding. %s"));
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}
#endif

		if (tunnel->init_func &&
		    !(*tunnel->init_func)(tunnel, tunnel->callback_data)) {
			if (tunnel->disconnect_func)
				(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
			tunnel->thread = 0;
			return NULL;
		}

		break;
	}

	tunnel->buffer_len = 10240;
	tunnel->buffer = g_malloc(tunnel->buffer_len);

	/* Start the tunnel data transmission */
	while (tunnel->running) {
		if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_XPORT ||
		    tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE) {
			if (first) {
				first = FALSE;
				channel = ssh_channel_accept_forward(REMMINA_SSH(tunnel)->session, 15000, &tunnel->port);
				if (!channel) {
					remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("The server did not respond."));
					if (tunnel->disconnect_func)
						(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);
					tunnel->thread = 0;
					return NULL;
				}
				if (tunnel->connect_func)
					(*tunnel->connect_func)(tunnel, tunnel->callback_data);
				if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE) {
					/* For reverse tunnel, we only need one connection. */
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 0)
					ssh_channel_cancel_forward(REMMINA_SSH(tunnel)->session, NULL, tunnel->port);
#else
					ssh_forward_cancel(REMMINA_SSH(tunnel)->session, NULL, tunnel->port);
#endif
				}
			} else if (tunnel->tunnel_type != REMMINA_SSH_TUNNEL_REVERSE) {
				/* Poll once per some period of time if no incoming connections.
				 * Don’t try to poll continuously as it will significantly slow down the loop */
				t1 = g_date_time_new_now_local();
				diff = g_date_time_difference(t1, t2) * 10000000
				       + g_date_time_difference(t1, t2) / 100000;
				if (diff > 1) {
					REMMINA_DEBUG("Polling tunnel channels");
						channel = ssh_channel_accept_forward(REMMINA_SSH(tunnel)->session, 0, &tunnel->port);
					if (channel == NULL)
						t2 = t1;
				}
				g_date_time_unref(t1);
				g_date_time_unref(t2);
			}

			if (channel) {
				if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_REVERSE) {
					sin.sin_family = AF_INET;
					sin.sin_port = htons(tunnel->localport);
					sin.sin_addr.s_addr = inet_addr("127.0.0.1");
					sock = socket(AF_INET, SOCK_STREAM, 0);
					if (connect(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
						remmina_ssh_set_application_error(REMMINA_SSH(tunnel),
										  _("Cannot connect to local port %i."), tunnel->localport);
						close(sock);
						sock = -1;
					}
				} else
					sock = remmina_public_open_xdisplay(tunnel->localdisplay);
				if (sock >= 0)
					remmina_ssh_tunnel_add_channel(tunnel, channel, sock);
				else {
					/* Failed to create unix socket. Will this happen? */
					ssh_channel_close(channel);
					ssh_channel_send_eof(channel);
					ssh_channel_free(channel);
				}
				channel = NULL;
			}
		}

		if (tunnel->num_channels <= 0)
			/* No more connections. We should quit */
			break;

		timeout.tv_sec = 0;
		timeout.tv_usec = 200000;

		FD_ZERO(&set);
		maxfd = 0;
		for (i = 0; i < tunnel->num_channels; i++) {
			if (tunnel->sockets[i] > maxfd)
				maxfd = tunnel->sockets[i];
			FD_SET(tunnel->sockets[i], &set);
		}

		ret = ssh_select(tunnel->channels, tunnel->channels_out, maxfd + 1, &set, &timeout);
		if (!tunnel->running) break;
		if (ret == SSH_EINTR) continue;
		if (ret == -1) break;

		i = 0;
		while (tunnel->running && i < tunnel->num_channels) {
			disconnected = FALSE;
			if (FD_ISSET(tunnel->sockets[i], &set)) {
				while (!disconnected &&
				       (len = read(tunnel->sockets[i], tunnel->buffer, tunnel->buffer_len)) > 0) {
					for (ptr = tunnel->buffer, lenw = 0; len > 0; len -= lenw, ptr += lenw) {
						lenw = ssh_channel_write(tunnel->channels[i], (char *)ptr, len);
						if (lenw <= 0) {
							disconnected = TRUE;
							// TRANSLATORS: The placeholder %s is an error message
							remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not write to SSH channel. %s"));
							break;
						}
					}
				}
				if (len == 0) {
					// TRANSLATORS: The placeholder %s is an error message
					remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not read from tunnel listening socket. %s"));
					disconnected = TRUE;
				}
			}
			if (disconnected) {
				REMMINA_DEBUG("tunnel disconnected because %s", REMMINA_SSH(tunnel)->error);
				remmina_ssh_tunnel_remove_channel(tunnel, i);
				continue;
			}
			i++;
		}
		if (!tunnel->running) break;

		i = 0;
		while (tunnel->running && i < tunnel->num_channels) {
			disconnected = FALSE;
			if (!tunnel->socketbuffers[i]) {
				len = ssh_channel_poll(tunnel->channels[i], 0);
				if (len == SSH_ERROR || len == SSH_EOF) {
					// TRANSLATORS: The placeholder %s is an error message
					remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not poll SSH channel. %s"));
					disconnected = TRUE;
				} else if (len > 0) {
					tunnel->socketbuffers[i] = remmina_ssh_tunnel_buffer_new(len);
					len = ssh_channel_read_nonblocking(tunnel->channels[i], tunnel->socketbuffers[i]->data, len, 0);
					if (len <= 0) {
						// TRANSLATORS: The placeholder %s is an error message
						remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not read SSH channel in a non-blocking way. %s"));
						disconnected = TRUE;
					} else {
						tunnel->socketbuffers[i]->len = len;
					}
				}
			}

			if (!disconnected && tunnel->socketbuffers[i]) {
				for (lenw = 0; tunnel->socketbuffers[i]->len > 0;
				     tunnel->socketbuffers[i]->len -= lenw, tunnel->socketbuffers[i]->ptr += lenw) {
					lenw = write(tunnel->sockets[i], tunnel->socketbuffers[i]->ptr, tunnel->socketbuffers[i]->len);
					if (lenw == -1 && errno == EAGAIN && tunnel->running)
						/* Sometimes we cannot write to a socket (always EAGAIN), probably because it’s internal
						 * buffer is full. We need read the pending bytes from the socket first. so here we simply
						 * break, leave the buffer there, and continue with other data */
						break;
					if (lenw <= 0) {
						// TRANSLATORS: The placeholder %s is an error message
						remmina_ssh_set_error(REMMINA_SSH(tunnel), _("Could not send data to tunnel listening socket. %s"));
						disconnected = TRUE;
						break;
					}
				}
				if (tunnel->socketbuffers[i]->len <= 0) {
					remmina_ssh_tunnel_buffer_free(tunnel->socketbuffers[i]);
					tunnel->socketbuffers[i] = NULL;
				}
			}

			if (disconnected) {
				REMMINA_DEBUG("Connection to SSH tunnel dropped. %s", REMMINA_SSH(tunnel)->error);
				remmina_ssh_tunnel_remove_channel(tunnel, i);
				continue;
			}
			i++;
		}
		/**
		 * Some protocols may open new connections during the session.
		 * e.g: SPICE opens a new connection for some channels.
		 */
		sock = remmina_ssh_tunnel_accept_local_connection(tunnel, FALSE);
		if (sock > 0) {
			channel = remmina_ssh_tunnel_create_forward_channel(tunnel);
			if (!channel) {
				REMMINA_DEBUG("Could not open new SSH connection. %s", REMMINA_SSH(tunnel)->error);
				close(sock);
				/* Leave thread loop */
				tunnel->running = FALSE;
			} else {
				remmina_ssh_tunnel_add_channel(tunnel, channel, sock);
			}
		}
	}

	remmina_ssh_tunnel_close_all_channels(tunnel);

	tunnel->running = FALSE;

	/* Notify tunnel owner of disconnection */
	if (tunnel->disconnect_func)
		(*tunnel->disconnect_func)(tunnel, tunnel->callback_data);

	return NULL;
}

static gboolean remmina_ssh_notify_tunnel_main_thread_end(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel = (RemminaSSHTunnel *)data;

	/* Ask tunnel owner to destroy tunnel object */
	if (tunnel->destroy_func)
		(*tunnel->destroy_func)(tunnel, tunnel->destroy_func_callback_data);

	return FALSE;
}

static gpointer
remmina_ssh_tunnel_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaSSHTunnel *tunnel = (RemminaSSHTunnel *)data;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

	while (TRUE) {
		remmina_ssh_tunnel_main_thread_proc(data);
		if (tunnel->server_sock < 0 || tunnel->thread == 0 || !tunnel->running) break;
	}
	tunnel->thread = 0;

	/* Do after tunnel thread cleanup */
	IDLE_ADD((GSourceFunc)remmina_ssh_notify_tunnel_main_thread_end, (gpointer)tunnel);

	return NULL;
}


void
remmina_ssh_tunnel_cancel_accept(RemminaSSHTunnel *tunnel)
{
	TRACE_CALL(__func__);
	if (tunnel->server_sock >= 0) {
		close(tunnel->server_sock);
		tunnel->server_sock = -1;
	}
}

gboolean
remmina_ssh_tunnel_open(RemminaSSHTunnel *tunnel, const gchar *host, gint port, gint local_port)
{
	TRACE_CALL(__func__);
	gint sock;
	gint sockopt = 1;
	struct sockaddr_in sin;

	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_OPEN;
	tunnel->dest = g_strdup(host);
	tunnel->port = port;
	if (tunnel->port == 0) {
		REMMINA_SSH(tunnel)->error = g_strdup(_("Assign a destination port."));
		return FALSE;
	}

	/* Create the server socket that listens on the local port */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		REMMINA_SSH(tunnel)->error = g_strdup(_("Could not create socket."));
		return FALSE;
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));

	sin.sin_family = AF_INET;
	sin.sin_port = htons(local_port);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin))) {
		REMMINA_SSH(tunnel)->error = g_strdup(_("Could not bind server socket to local port."));
		close(sock);
		return FALSE;
	}

	if (listen(sock, 1)) {
		REMMINA_SSH(tunnel)->error = g_strdup(_("Could not listen to local port."));
		close(sock);
		return FALSE;
	}

	tunnel->server_sock = sock;
	tunnel->running = TRUE;

	if (pthread_create(&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel)) {
		// TRANSLATORS: Do not translate pthread
		remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("Could not start pthread."));
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_xport(RemminaSSHTunnel *tunnel, gboolean bindlocalhost)
{
	TRACE_CALL(__func__);
	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_XPORT;
	tunnel->bindlocalhost = bindlocalhost;
	tunnel->running = TRUE;

	if (pthread_create(&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel)) {
		// TRANSLATORS: Do not translate pthread
		remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("Could not start pthread."));
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_reverse(RemminaSSHTunnel *tunnel, gint port, gint local_port)
{
	TRACE_CALL(__func__);
	tunnel->tunnel_type = REMMINA_SSH_TUNNEL_REVERSE;
	tunnel->port = port;
	tunnel->localport = local_port;
	tunnel->running = TRUE;

	if (pthread_create(&tunnel->thread, NULL, remmina_ssh_tunnel_main_thread, tunnel)) {
		// TRANSLATORS: Do not translate pthread
		remmina_ssh_set_application_error(REMMINA_SSH(tunnel), _("Could not start pthread."));
		tunnel->thread = 0;
		return FALSE;
	}
	return TRUE;
}

gboolean
remmina_ssh_tunnel_terminated(RemminaSSHTunnel *tunnel)
{
	TRACE_CALL(__func__);
	return tunnel->thread == 0;
}

void
remmina_ssh_tunnel_free(RemminaSSHTunnel *tunnel)
{
	TRACE_CALL(__func__);
	pthread_t thread;

	REMMINA_DEBUG("tunnel->thread = %lX\n", tunnel->thread);

	thread = tunnel->thread;
	if (thread != 0) {
		tunnel->running = FALSE;
		pthread_cancel(thread);
		pthread_join(thread, NULL);
		tunnel->thread = 0;
	}

	if (tunnel->tunnel_type == REMMINA_SSH_TUNNEL_XPORT && tunnel->remotedisplay > 0) {
#if LIBSSH_VERSION_INT >= SSH_VERSION_INT(0, 7, 0)
		ssh_channel_cancel_forward(REMMINA_SSH(tunnel)->session, NULL, 6000 + tunnel->remotedisplay);
#else
		ssh_forward_cancel(REMMINA_SSH(tunnel)->session, NULL, 6000 + tunnel->remotedisplay);
#endif
	}
	if (tunnel->server_sock >= 0) {
		close(tunnel->server_sock);
		tunnel->server_sock = -1;
	}

	remmina_ssh_tunnel_close_all_channels(tunnel);

	g_free(tunnel->buffer);
	g_free(tunnel->channels_out);
	g_free(tunnel->dest);
	g_free(tunnel->localdisplay);

	remmina_ssh_free((RemminaSSH *)tunnel);
}

/*-----------------------------------------------------------------------------*
*                           SSH SFTP                                          *
*-----------------------------------------------------------------------------*/
RemminaSFTP *
remmina_sftp_new_from_file(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaSFTP *sftp;

	sftp = g_new(RemminaSFTP, 1);

	remmina_ssh_init_from_file(REMMINA_SSH(sftp), remminafile, FALSE);

	sftp->sftp_sess = NULL;

	return sftp;
}

RemminaSFTP *
remmina_sftp_new_from_ssh(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	RemminaSFTP *sftp;

	sftp = g_new(RemminaSFTP, 1);

	remmina_ssh_init_from_ssh(REMMINA_SSH(sftp), ssh);

	sftp->sftp_sess = NULL;

	return sftp;
}

gboolean
remmina_sftp_open(RemminaSFTP *sftp)
{
	TRACE_CALL(__func__);
	sftp->sftp_sess = sftp_new(sftp->ssh.session);
	if (!sftp->sftp_sess) {
		// TRANSLATORS: The placeholder %s is an error message
		remmina_ssh_set_error(REMMINA_SSH(sftp), _("Could not create SFTP session. %s"));
		return FALSE;
	}
	if (sftp_init(sftp->sftp_sess)) {
		// TRANSLATORS: The placeholder %s is an error message
		remmina_ssh_set_error(REMMINA_SSH(sftp), _("Could not start SFTP session. %s"));
		return FALSE;
	}
	return TRUE;
}

void
remmina_sftp_free(RemminaSFTP *sftp)
{
	TRACE_CALL(__func__);
	if (sftp->sftp_sess) {
		sftp_free(sftp->sftp_sess);
		sftp->sftp_sess = NULL;
	}
	remmina_ssh_free(REMMINA_SSH(sftp));
}

/*-----------------------------------------------------------------------------*
*                           SSH Shell                                         *
*-----------------------------------------------------------------------------*/
RemminaSSHShell *
remmina_ssh_shell_new_from_file(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaSSHShell *shell;

	shell = g_new0(RemminaSSHShell, 1);

	remmina_ssh_init_from_file(REMMINA_SSH(shell), remminafile, FALSE);

	shell->master = -1;
	shell->slave = -1;
	shell->exec = g_strdup(remmina_file_get_string(remminafile, "exec"));
	shell->run_line = g_strdup(remmina_file_get_string(remminafile, "run_line"));

	return shell;
}

RemminaSSHShell *
remmina_ssh_shell_new_from_ssh(RemminaSSH *ssh)
{
	TRACE_CALL(__func__);
	RemminaSSHShell *shell;

	shell = g_new0(RemminaSSHShell, 1);

	remmina_ssh_init_from_ssh(REMMINA_SSH(shell), ssh);

	shell->master = -1;
	shell->slave = -1;

	return shell;
}

static gboolean
remmina_ssh_call_exit_callback_on_main_thread(gpointer data)
{
	TRACE_CALL(__func__);

	RemminaSSHShell *shell = (RemminaSSHShell *)data;
	if (shell->exit_callback)
		shell->exit_callback(shell->user_data);
	if (shell) {
		remmina_ssh_shell_free(shell);
		shell = NULL;
	}
	return FALSE;
}

static gpointer
remmina_ssh_shell_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaSSHShell *shell = (RemminaSSHShell *)data;
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)shell->user_data;
	RemminaFile *remminafile;
	remminafile = remmina_protocol_widget_get_file(gp);
	ssh_channel channel = NULL;
	gint ret;
	gchar *filename;
	const gchar *dir;
	const gchar *sshlogname;
	FILE *fp = NULL;

	//gint screen;

	LOCK_SSH(shell)

	if ((channel = ssh_channel_new(REMMINA_SSH(shell)->session)) == NULL ||
	    ssh_channel_open_session(channel)) {
		UNLOCK_SSH(shell)
		// TRANSLATORS: The placeholder %s is an error message
		remmina_ssh_set_error(REMMINA_SSH(shell), _("Could not open channel. %s"));
		if (channel) ssh_channel_free(channel);
		shell->thread = 0;
		return NULL;
	}

	ssh_channel_request_pty(channel);

	// SSH Callbacks
	struct ssh_callbacks_struct cb =
	{
		.channel_open_request_x11_function = remmina_ssh_x11_open_request_cb,
		.userdata = shell,
	};

	if (remmina_file_get_int(remminafile, "ssh_forward_x11", FALSE)) {
		ssh_callbacks_init(&cb);
		ssh_set_callbacks(REMMINA_SSH(shell)->session, &cb);

		const char *display = getenv("DISPLAY");
		char *proto = NULL, *cookie = NULL;
		if (remmina_ssh_x11_get_proto(display, &proto, &cookie) != 0) {
			REMMINA_DEBUG("Using fake authentication data for X11 forwarding");
			proto = NULL;
			cookie = NULL;
		}

		REMMINA_DEBUG("proto: %s - cookie: %s", proto, cookie);
		ret = ssh_channel_request_x11(channel, 0, proto, cookie, 0);
		if (ret != SSH_OK) return NULL;
	}

	if (shell->exec && shell->exec[0]) {
		REMMINA_DEBUG ("Requesting an SSH exec channel");
		ret = ssh_channel_request_exec(channel, shell->exec);
	} else {
		REMMINA_DEBUG ("Requesting an SSH shell channel");
		ret = ssh_channel_request_shell(channel);
	}
	if (ret != SSH_OK) {
		UNLOCK_SSH(shell)
		REMMINA_WARNING ("Could not request shell");
		// TRANSLATORS: The placeholder %s is an error message
		remmina_ssh_set_error(REMMINA_SSH(shell), _("Could not request shell. %s"));
		ssh_channel_close(channel);
		ssh_channel_send_eof(channel);
		ssh_channel_free(channel);
		shell->thread = 0;
		return NULL;
	}

	shell->channel = channel;

	UNLOCK_SSH(shell)

	GFile *rf = g_file_new_for_path(remminafile->filename);

	if (remmina_file_get_string(remminafile, "sshlogfolder") == NULL)
		dir = g_build_path("/", g_get_user_cache_dir(), "remmina", NULL);
	else
		dir = remmina_file_get_string(remminafile, "sshlogfolder");

	if (remmina_file_get_string(remminafile, "sshlogname") == NULL)
		sshlogname = g_strconcat(g_file_get_basename(rf), ".", "log", NULL);
	else
		sshlogname = remmina_file_get_string(remminafile, "sshlogname");
	sshlogname = remmina_file_format_properties(remminafile, sshlogname);
	filename = g_strconcat(dir, "/", sshlogname, NULL);

	if (remmina_file_get_int (remminafile, "sshsavesession", FALSE)) {
		REMMINA_DEBUG("Saving session log to %s", filename);
		fp = fopen(filename, "w");
	}

	g_free(filename);

	REMMINA_DEBUG("Run_line: %s", shell->run_line);
	if (!shell->closed && shell->run_line && shell->run_line[0]) {
		LOCK_SSH(shell)
		//TODO: Confirm assumption - assuming null terminated gchar string
		ssh_channel_write(channel, shell->run_line, (gint)strlen(shell->run_line));
		ssh_channel_write(channel, "\n", (gint)1); //TODO: Test this
		UNLOCK_SSH(shell)
		REMMINA_DEBUG("Run_line written to channel");
	}

	LOCK_SSH(shell)

	// Create new event context.
	shell->event = ssh_event_new();
	if (shell->event == NULL) {
		REMMINA_WARNING("Internal error in %s: Couldn't get a event.", __func__);
		return NULL;
	}

	REMMINA_DEBUG("shell->slave: %d", shell->slave);

	// Add the fd to the event and assign it the callback.
	if (ssh_event_add_fd(shell->event, shell->slave, events, remimna_ssh_cp_to_ch_cb, channel) != SSH_OK) {
		REMMINA_WARNING("Internal error in %s: Couldn't add an fd to the event.", __func__);
		return NULL;
	}

	// Remove the poll handle from session and assign them to the event.
	if (ssh_event_add_session(shell->event, REMMINA_SSH(shell)->session) != SSH_OK) {
		REMMINA_WARNING("Internal error in %s: Couldn't add the session to the event.", __func__);
		return NULL;
	}

	remmina_ssh_insert_item(shell->channel, shell->slave, shell->slave, TRUE, shell->thread);

	// Initializes the ssh_callbacks_struct.
	channel_cb.userdata = &shell;
	ssh_callbacks_init(&channel_cb);
	// Set the channel callback functions.
	ssh_set_channel_callbacks(shell->channel, &channel_cb);
	UNLOCK_SSH(shell)

	do {
		ssh_event_dopoll(shell->event, 1000);
	} while(!ssh_channel_is_closed(shell->channel));

	// Close all OPENED X11 channel
	remmina_ssh_close_all_x11_ch(shell->thread);

	shell->closed = TRUE;

	LOCK_SSH(shell)

	// Remove socket fd from event context.
	ret = ssh_event_remove_fd(shell->event, shell->slave);
	REMMINA_DEBUG("Remove socket fd from event context: %d", ret);

	// Remove session object from event context.
	ret = ssh_event_remove_session(shell->event, REMMINA_SSH(shell)->session);
	REMMINA_DEBUG("Remove session object from event context: %d", ret);

	// Free event context.
	ssh_event_free(shell->event);
	REMMINA_DEBUG("Free event context");

	// Remove channel callback.
	ret = ssh_remove_channel_callbacks(shell->channel, &channel_cb);
	REMMINA_DEBUG("Remove channel callback: %d", ret);

	if (remmina_file_get_int (remminafile, "sshsavesession", FALSE))
		fclose(fp);
	shell->channel = NULL;
	ssh_channel_close(channel);
	ssh_channel_send_eof(channel);
	ssh_channel_free(channel);
	UNLOCK_SSH(shell)

	shell->thread = 0;

	if (shell->exit_callback)
		IDLE_ADD((GSourceFunc)remmina_ssh_call_exit_callback_on_main_thread, (gpointer)shell);
	return NULL;
}

gboolean
remmina_ssh_shell_open(RemminaSSHShell *shell, RemminaSSHExitFunc exit_callback, gpointer data)
{
	TRACE_CALL(__func__);
	gchar *slavedevice;
	struct termios stermios;

	shell->master = posix_openpt(O_RDWR | O_NOCTTY);
	if (shell->master == -1 ||
	    grantpt(shell->master) == -1 ||
	    unlockpt(shell->master) == -1 ||
	    (slavedevice = ptsname(shell->master)) == NULL ||
	    (shell->slave = open(slavedevice, O_RDWR | O_NOCTTY)) < 0) {
		REMMINA_SSH(shell)->error = g_strdup(_("Could not create PTY device."));
		return FALSE;
	}

	/* As per libssh documentation */
	tcgetattr(shell->slave, &stermios);
	stermios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
	stermios.c_oflag &= ~OPOST;
	stermios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	stermios.c_cflag &= ~(CSIZE | PARENB);
	stermios.c_cflag |= CS8;
	tcsetattr(shell->slave, TCSANOW, &stermios);

	shell->exit_callback = exit_callback;
	shell->user_data = data;

	/* Once the process started, we should always TRUE and assume the pthread will be created always */
	pthread_create(&shell->thread, NULL, remmina_ssh_shell_thread, shell);

	return TRUE;
}

void
remmina_ssh_shell_set_size(RemminaSSHShell *shell, gint columns, gint rows)
{
	TRACE_CALL(__func__);
	LOCK_SSH(shell)
	if (shell->channel)
		ssh_channel_change_pty_size(shell->channel, columns, rows);
	UNLOCK_SSH(shell)
}

void
remmina_ssh_shell_free(RemminaSSHShell *shell)
{
	TRACE_CALL(__func__);
	//pthread_t thread = shell->thread;

	// Close all OPENED X11 channel
	remmina_ssh_close_all_x11_ch(shell->thread);

	shell->exit_callback = NULL;
	shell->closed = TRUE;
	REMMINA_DEBUG("Cancelling the shell thread if needed");
	if (shell->thread) {
		pthread_cancel(shell->thread);
		if (shell->thread) pthread_join(shell->thread, NULL);
	}
	close(shell->slave);
	if (shell->exec) {
		g_free(shell->exec);
		shell->exec = NULL;
	}
	if (shell->run_line) {
		g_free(shell->run_line);
		shell->run_line = NULL;
	}
	/* It’s not necessary to close shell->slave since the other end (vte) will close it */;
	remmina_ssh_free(REMMINA_SSH(shell));
}

#endif /* HAVE_LIBSSH */
