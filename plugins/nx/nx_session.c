/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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

#include <errno.h>
#include <pthread.h>
#include "common/remmina_plugin.h"
#include <glib/gstdio.h>
#define LIBSSH_STATIC 1
#include <libssh/libssh.h>
#include "nx_session.h"

/* Some missing stuff in libssh */
#define REMMINA_SSH_TYPE_DSS 1
#define REMMINA_SSH_TYPE_RSA 2

static gboolean remmina_get_keytype(const gchar *private_key_file, gint *keytype, gboolean *encrypted)
{
	TRACE_CALL(__func__);
	FILE *fp;
	gchar buf1[100], buf2[100];

	if ((fp = g_fopen(private_key_file, "r")) == NULL) {
		return FALSE;
	}
	if (!fgets(buf1, sizeof(buf1), fp) || !fgets(buf2, sizeof(buf2), fp)) {
		fclose(fp);
		return FALSE;
	}
	fclose(fp);

	if (strstr(buf1, "BEGIN RSA"))
		*keytype = REMMINA_SSH_TYPE_RSA;
	else if (strstr(buf1, "BEGIN DSA"))
		*keytype = REMMINA_SSH_TYPE_DSS;
	else
		return FALSE;

	*encrypted = (strstr(buf2, "ENCRYPTED") ? TRUE : FALSE);

	return TRUE;
}

/*****/

static const gchar nx_default_private_key[] = "-----BEGIN DSA PRIVATE KEY-----\n"
					      "MIIBuwIBAAKBgQCXv9AzQXjxvXWC1qu3CdEqskX9YomTfyG865gb4D02ZwWuRU/9\n"
					      "C3I9/bEWLdaWgJYXIcFJsMCIkmWjjeSZyTmeoypI1iLifTHUxn3b7WNWi8AzKcVF\n"
					      "aBsBGiljsop9NiD1mEpA0G+nHHrhvTXz7pUvYrsrXcdMyM6rxqn77nbbnwIVALCi\n"
					      "xFdHZADw5KAVZI7r6QatEkqLAoGBAI4L1TQGFkq5xQ/nIIciW8setAAIyrcWdK/z\n"
					      "5/ZPeELdq70KDJxoLf81NL/8uIc4PoNyTRJjtT3R4f8Az1TsZWeh2+ReCEJxDWgG\n"
					      "fbk2YhRqoQTtXPFsI4qvzBWct42WonWqyyb1bPBHk+JmXFscJu5yFQ+JUVNsENpY\n"
					      "+Gkz3HqTAoGANlgcCuA4wrC+3Cic9CFkqiwO/Rn1vk8dvGuEQqFJ6f6LVfPfRTfa\n"
					      "QU7TGVLk2CzY4dasrwxJ1f6FsT8DHTNGnxELPKRuLstGrFY/PR7KeafeFZDf+fJ3\n"
					      "mbX5nxrld3wi5titTnX+8s4IKv29HJguPvOK/SI7cjzA+SqNfD7qEo8CFDIm1xRf\n"
					      "8xAPsSKs6yZ6j1FNklfu\n"
					      "-----END DSA PRIVATE KEY-----\n";

static const gchar nx_hello_server_msg[] = "hello nxserver - version ";

struct _RemminaNXSession {
	/* Common SSH members */
	ssh_session session;
	ssh_channel channel;
	gchar *server;
	gchar *error;
	RemminaNXLogCallback log_callback;

	/* Tunnel related members */
	pthread_t thread;
	gboolean running;
	gint server_sock;

	/* NX related members */
	GHashTable *session_parameters;

	GString *response;
	gint response_pos;
	gint status;
	gint encryption;
	gint localport;

	gchar *version;
	gchar *session_id;
	gint session_display;
	gchar *proxy_cookie;

	gboolean allow_start;
	GtkListStore *session_list;
	gint session_list_state;

	GPid proxy_pid;
	guint proxy_watch_source;
};

RemminaNXSession*
remmina_nx_session_new(void)
{
	TRACE_CALL(__func__);
	RemminaNXSession *nx;

	nx = g_new0(RemminaNXSession, 1);

	nx->session_parameters = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	nx->response = g_string_new(NULL);
	nx->status = -1;
	nx->encryption = 1;
	nx->server_sock = -1;

	return nx;
}

void remmina_nx_session_free(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	pthread_t thread;

	if (nx->proxy_watch_source) {
		g_source_remove(nx->proxy_watch_source);
		nx->proxy_watch_source = 0;
	}
	if (nx->proxy_pid) {
		kill(nx->proxy_pid, SIGTERM);
		g_spawn_close_pid(nx->proxy_pid);
		nx->proxy_pid = 0;
	}
	thread = nx->thread;
	if (thread) {
		nx->running = FALSE;
		pthread_cancel(thread);
		pthread_join(thread, NULL);
		nx->thread = 0;
	}
	if (nx->channel) {
		ssh_channel_close(nx->channel);
		ssh_channel_free(nx->channel);
	}
	if (nx->server_sock >= 0) {
		close(nx->server_sock);
		nx->server_sock = -1;
	}

	g_free(nx->server);
	g_free(nx->error);
	g_hash_table_destroy(nx->session_parameters);
	g_string_free(nx->response, TRUE);
	g_free(nx->version);
	g_free(nx->session_id);
	g_free(nx->proxy_cookie);

	if (nx->session_list) {
		g_object_unref(nx->session_list);
		nx->session_list = NULL;
	}
	if (nx->session) {
		ssh_free(nx->session);
		nx->session = NULL;
	}
	g_free(nx);
}

static void remmina_nx_session_set_error(RemminaNXSession *nx, const gchar *fmt)
{
	TRACE_CALL(__func__);
	const gchar *err;

	if (nx->error)
		g_free(nx->error);
	err = ssh_get_error(nx->session);
	nx->error = g_strdup_printf(fmt, err);
}

static void remmina_nx_session_set_application_error(RemminaNXSession *nx, const gchar *fmt, ...)
{
	TRACE_CALL(__func__);
	va_list args;

	if (nx->error) g_free(nx->error);
	va_start(args, fmt);
	nx->error = g_strdup_vprintf(fmt, args);
	va_end(args);
}

gboolean remmina_nx_session_has_error(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	return (nx->error != NULL);
}

const gchar*
remmina_nx_session_get_error(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	return nx->error;
}

void remmina_nx_session_clear_error(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	if (nx->error) {
		g_free(nx->error);
		nx->error = NULL;
	}
}

void remmina_nx_session_set_encryption(RemminaNXSession *nx, gint encryption)
{
	TRACE_CALL(__func__);
	nx->encryption = encryption;
}

void remmina_nx_session_set_localport(RemminaNXSession *nx, gint localport)
{
	TRACE_CALL(__func__);
	nx->localport = localport;
}

void remmina_nx_session_set_log_callback(RemminaNXSession *nx, RemminaNXLogCallback log_callback)
{
	TRACE_CALL(__func__);
	nx->log_callback = log_callback;
}

static gboolean remmina_nx_session_get_response(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	struct timeval timeout;
	ssh_channel ch[2];
	gchar *buffer;
	gint len;
	gint is_stderr;

	timeout.tv_sec = 60;
	timeout.tv_usec = 0;
	ch[0] = nx->channel;
	ch[1] = NULL;
	ssh_channel_select(ch, NULL, NULL, &timeout);

	is_stderr = 0;
	while (is_stderr <= 1) {
		len = ssh_channel_poll(nx->channel, is_stderr);
		if (len == SSH_ERROR) {
			remmina_nx_session_set_error(nx, "Error reading channel: %s");
			return FALSE;
		}
		if (len > 0)
			break;
		is_stderr++;
	}
	if (is_stderr > 1)
		return FALSE;

	buffer = g_malloc(sizeof(*buffer) * len);
	len = ssh_channel_read(nx->channel, buffer, len, is_stderr);
	if (len <= 0) {
		remmina_nx_session_set_application_error(nx, "Channel closed.");
		return FALSE;
	}

	g_string_append_len(nx->response, buffer, len);

	g_free(buffer);
	return TRUE;
}

static void remmina_nx_session_parse_session_list_line(RemminaNXSession *nx, const gchar *line)
{
	TRACE_CALL(__func__);
	gchar *p1, *p2;
	gchar *val;
	gint i;
	GtkTreeIter iter;

	p1 = (char*)line;
	while (*p1 == ' ')
		p1++;
	if (*p1 == '\0')
		return;

	gtk_list_store_append(nx->session_list, &iter);

	p1 = (char*)line;
	for (i = 0; i < 7; i++) {
		p2 = strchr(p1, ' ');
		if (!p2)
			return;
		val = g_strndup(p1, (gint)(p2 - p1));
		switch (i) {
		case 0:
			gtk_list_store_set(nx->session_list, &iter, REMMINA_NX_SESSION_COLUMN_DISPLAY, val, -1);
			break;
		case 1:
			gtk_list_store_set(nx->session_list, &iter, REMMINA_NX_SESSION_COLUMN_TYPE, val, -1);
			break;
		case 2:
			gtk_list_store_set(nx->session_list, &iter, REMMINA_NX_SESSION_COLUMN_ID, val, -1);
			break;
		case 6:
			gtk_list_store_set(nx->session_list, &iter, REMMINA_NX_SESSION_COLUMN_STATUS, val, -1);
			break;
		default:
			break;
		}
		g_free(val);

		while (*p2 == ' ')
			p2++;
		p1 = p2;
	}
	/* The last name column might contains space so it’s not in the above loop. We simply rtrim it here. */
	i = strlen(p1);
	if (i < 1)
		return;
	p2 = p1 + i - 1;
	while (*p2 == ' ' && p2 > p1)
		p2--;
	val = g_strndup(p1, (gint)(p2 - p1 + 1));
	gtk_list_store_set(nx->session_list, &iter, REMMINA_NX_SESSION_COLUMN_NAME, val, -1);
	g_free(val);
}

static gint remmina_nx_session_parse_line(RemminaNXSession *nx, const gchar *line, gchar **valueptr)
{
	TRACE_CALL(__func__);
	gchar *s;
	gchar *ptr;
	gint status;

	*valueptr = NULL;

	/* Get the server version from the initial line */
	if (!nx->version) {
		s = g_ascii_strdown(line, -1);
		ptr = strstr(s, nx_hello_server_msg);
		if (!ptr) {
			/* Try to use a default version */
			nx->version = g_strdup("3.3.0");
		} else {
			nx->version = g_strdup(ptr + strlen(nx_hello_server_msg));
			ptr = strchr(nx->version, ' ');
			if (ptr)
				*ptr = '\0';
			/* NoMachine NX append a dash+subversion. Need to be removed. */
			ptr = strchr(nx->version, '-');
			if (ptr)
				*ptr = '\0';
		}
		g_free(s);
		return nx->status;
	}

	if (sscanf(line, "NX> %i ", &status) < 1) {
		if (nx->session_list_state && nx->session_list) {
			if (nx->session_list_state == 1 && strncmp(line, "----", 4) == 0) {
				nx->session_list_state = 2;
			} else if (nx->session_list_state == 2) {
				remmina_nx_session_parse_session_list_line(nx, line);
			}
			return -1;
		}
		return nx->status;
	}

	nx->session_list_state = 0;
	nx->status = status;
	ptr = strchr(line, ':');
	if (!ptr)
		return status;
	*valueptr = ptr + 2;
	return status;
}

static gchar*
remmina_nx_session_get_line(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	gchar *line;
	gchar *pos, *ptr;
	gint len;
	gint l;

	if (nx->response_pos >= nx->response->len)
		return NULL;

	pos = nx->response->str + nx->response_pos;
	if ((ptr = strchr(pos, '\n')) == NULL)
		return NULL;

	len = ((gint)(ptr - pos)) + 1;
	line = g_strndup(pos, len - 1);

	l = strlen(line);
	if (l > 0 && line[l - 1] == '\r') {
		line[l - 1] = '\0';
	}

	nx->response_pos += len;

	return line;
}

static gint remmina_nx_session_parse_response(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	gchar *line;
	gchar *pos, *p;
	gint status = -1;

	if (nx->response_pos >= nx->response->len)
		return -1;

	while ((line = remmina_nx_session_get_line(nx)) != NULL) {
		if (nx->log_callback)
			nx->log_callback(line);

		status = remmina_nx_session_parse_line(nx, line, &p);
		if (status == 500) {
			/* 500: Last operation failed. Should be ignored. */
		} else if (status >= 400 && status <= 599) {
			remmina_nx_session_set_application_error(nx, "%s", line);
		} else {
			switch (status) {
			case 127: /* Session list */
				nx->session_list_state = 1;
				break;
			case 148: /* Server capacity not reached for user xxx */
				nx->session_list_state = 0;
				nx->allow_start = TRUE;
				break;
			case 700:
				nx->session_id = g_strdup(p);
				break;
			case 705:
				nx->session_display = atoi(p);
				break;
			case 701:
				nx->proxy_cookie = g_strdup(p);
				break;
			}
		}
		g_free(line);

		nx->status = status;
	}

	pos = nx->response->str + nx->response_pos;
	if (sscanf(pos, "NX> %i ", &status) < 1) {
		status = nx->status;
	} else {
		if (nx->log_callback)
			nx->log_callback(pos);
		nx->response_pos += 8;
	}
	nx->status = -1;
	return status;
}

static gint remmina_nx_session_expect_status2(RemminaNXSession *nx, gint status, gint status2)
{
	TRACE_CALL(__func__);
	gint response;

	while ((response = remmina_nx_session_parse_response(nx)) != status && response != status2) {
		if (response == 999)
			break;
		if (!remmina_nx_session_get_response(nx))
			return -1;
	}
	nx->session_list_state = 0;
	if (remmina_nx_session_has_error(nx))
		return -1;
	return response;
}

static gboolean remmina_nx_session_expect_status(RemminaNXSession *nx, gint status)
{
	TRACE_CALL(__func__);
	return (remmina_nx_session_expect_status2(nx, status, 0) == status);
}

static void remmina_nx_session_send_command(RemminaNXSession *nx, const gchar *cmdfmt, ...)
{
	TRACE_CALL(__func__);
	va_list args;
	gchar *cmd;

	va_start(args, cmdfmt);
	cmd = g_strdup_vprintf(cmdfmt, args);
	ssh_channel_write(nx->channel, cmd, strlen(cmd));
	g_free(cmd);

	ssh_set_fd_towrite(nx->session);
	ssh_channel_write(nx->channel, "\n", 1);
	va_end(args);
}

gboolean remmina_nx_session_open(RemminaNXSession *nx, const gchar *server, guint port, const gchar *private_key_file,
				 RemminaNXPassphraseCallback passphrase_func, gpointer userdata)
{
	TRACE_CALL(__func__);
	gint ret;
	ssh_key priv_key;
	gint keytype;
	gboolean encrypted;
	gchar *passphrase = NULL;

	nx->session = ssh_new();
	ssh_options_set(nx->session, SSH_OPTIONS_HOST, server);
	ssh_options_set(nx->session, SSH_OPTIONS_PORT, &port);
	ssh_options_set(nx->session, SSH_OPTIONS_USER, "nx");

	if (private_key_file && private_key_file[0]) {
		if (!remmina_get_keytype(private_key_file, &keytype, &encrypted)) {
			remmina_nx_session_set_application_error(nx, "Invalid private key file.");
			return FALSE;
		}
		if (encrypted && !passphrase_func(&passphrase, userdata)) {
			return FALSE;
		}
		if ( ssh_pki_import_privkey_file(private_key_file, (passphrase ? passphrase : ""), NULL, NULL, &priv_key) != SSH_OK ) {
			remmina_nx_session_set_application_error(nx, "Error importing private key from file.");
			g_free(passphrase);
			return FALSE;
		}
		g_free(passphrase);
	} else {
		/* Use NoMachine’s default nx private key */
		if ( ssh_pki_import_privkey_base64(nx_default_private_key, NULL, NULL, NULL, &priv_key) != SSH_OK ) {
			remmina_nx_session_set_application_error(nx, "Failed to import NX default private key.");
			return FALSE;
		}
	}

	if (ssh_connect(nx->session)) {
		ssh_key_free(priv_key);
		remmina_nx_session_set_error(nx, "Failed to startup SSH session: %s");
		return FALSE;
	}

	ret = ssh_userauth_publickey(nx->session, NULL, priv_key);

	ssh_key_free(priv_key);

	if (ret != SSH_AUTH_SUCCESS) {
		remmina_nx_session_set_error(nx, "NX SSH authentication failed: %s");
		return FALSE;
	}

	if ((nx->channel = ssh_channel_new(nx->session)) == NULL || ssh_channel_open_session(nx->channel) != SSH_OK) {
		return FALSE;
	}

	if (ssh_channel_request_shell(nx->channel) != SSH_OK) {
		return FALSE;
	}

	/* NX server starts the session with an initial 105 status */
	if (!remmina_nx_session_expect_status(nx, 105))
		return FALSE;

	/* Say hello to the NX server */
	remmina_nx_session_send_command(nx, "HELLO NXCLIENT - Version %s", nx->version);
	if (!remmina_nx_session_expect_status(nx, 105))
		return FALSE;

	/* Set the NX session environment */
	remmina_nx_session_send_command(nx, "SET SHELL_MODE SHELL");
	if (!remmina_nx_session_expect_status(nx, 105))
		return FALSE;
	remmina_nx_session_send_command(nx, "SET AUTH_MODE PASSWORD");
	if (!remmina_nx_session_expect_status(nx, 105))
		return FALSE;

	nx->server = g_strdup(server);

	return TRUE;
}

gboolean remmina_nx_session_login(RemminaNXSession *nx, const gchar *username, const gchar *password)
{
	TRACE_CALL(__func__);
	gint response;

	/* Login to the NX server */
	remmina_nx_session_send_command(nx, "login");
	if (!remmina_nx_session_expect_status(nx, 101))
		return FALSE;
	remmina_nx_session_send_command(nx, username);
	/* NoMachine Testdrive does not prompt for password, in which case 105 response is received without 102 */
	response = remmina_nx_session_expect_status2(nx, 102, 105);
	if (response == 102) {
		remmina_nx_session_send_command(nx, password);
		if (!remmina_nx_session_expect_status(nx, 105))
			return FALSE;
	} else if (response != 105) {
		return FALSE;
	}

	return TRUE;
}

void remmina_nx_session_add_parameter(RemminaNXSession *nx, const gchar *name, const gchar *valuefmt, ...)
{
	TRACE_CALL(__func__);
	va_list args;
	gchar *value;

	va_start(args, valuefmt);
	value = g_strdup_vprintf(valuefmt, args);
	g_hash_table_insert(nx->session_parameters, g_strdup(name), value);
	va_end(args);
}

static gboolean remmina_nx_session_send_session_command(RemminaNXSession *nx, const gchar *cmd_type, gint response)
{
	TRACE_CALL(__func__);
	GString *cmd;
	GHashTableIter iter;
	gchar *key, *value;

	cmd = g_string_new(cmd_type);
	g_hash_table_iter_init(&iter, nx->session_parameters);
	while (g_hash_table_iter_next(&iter, (gpointer*)&key, (gpointer*)&value)) {
		g_string_append_printf(cmd, " --%s=\"%s\"", key, value);
	}

	remmina_nx_session_send_command(nx, cmd->str);
	g_string_free(cmd, TRUE);

	g_hash_table_remove_all(nx->session_parameters);

	return remmina_nx_session_expect_status(nx, response);
}

gboolean remmina_nx_session_list(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	gboolean ret;

	if (nx->session_list == NULL) {
		nx->session_list = gtk_list_store_new(REMMINA_NX_SESSION_N_COLUMNS, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
			G_TYPE_STRING, G_TYPE_STRING);
	} else {
		gtk_list_store_clear(nx->session_list);
	}
	ret = remmina_nx_session_send_session_command(nx, "listsession", 105);

	return ret;
}

void remmina_nx_session_set_tree_view(RemminaNXSession *nx, GtkTreeView *tree)
{
	TRACE_CALL(__func__);
	gtk_tree_view_set_model(tree, GTK_TREE_MODEL(nx->session_list));
}

gboolean remmina_nx_session_iter_first(RemminaNXSession *nx, GtkTreeIter *iter)
{
	TRACE_CALL(__func__);
	if (!nx->session_list)
		return FALSE;
	return gtk_tree_model_get_iter_first(GTK_TREE_MODEL(nx->session_list), iter);
}

gboolean remmina_nx_session_iter_next(RemminaNXSession *nx, GtkTreeIter *iter)
{
	TRACE_CALL(__func__);
	if (!nx->session_list)
		return FALSE;
	return gtk_tree_model_iter_next(GTK_TREE_MODEL(nx->session_list), iter);
}

gchar*
remmina_nx_session_iter_get(RemminaNXSession *nx, GtkTreeIter *iter, gint column)
{
	TRACE_CALL(__func__);
	gchar *val;

	gtk_tree_model_get(GTK_TREE_MODEL(nx->session_list), iter, column, &val, -1);
	return val;
}

void remmina_nx_session_iter_set(RemminaNXSession *nx, GtkTreeIter *iter, gint column, const gchar *data)
{
	TRACE_CALL(__func__);
	gtk_list_store_set(nx->session_list, iter, column, data, -1);
}

gboolean remmina_nx_session_allow_start(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	return nx->allow_start;
}

static void remmina_nx_session_add_common_parameters(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	gchar *value;

	/* Add fixed session parameters for startsession */
	remmina_nx_session_add_parameter(nx, "cache", "16M");
	remmina_nx_session_add_parameter(nx, "images", "64M");
	remmina_nx_session_add_parameter(nx, "render", "1");
	remmina_nx_session_add_parameter(nx, "backingstore", "1");
	remmina_nx_session_add_parameter(nx, "agent_server", "");
	remmina_nx_session_add_parameter(nx, "agent_user", "");
	remmina_nx_session_add_parameter(nx, "agent_password", "");

	value = g_strdup_printf("%i", nx->encryption);
	remmina_nx_session_add_parameter(nx, "encryption", value);
	g_free(value);
}

gboolean remmina_nx_session_start(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	remmina_nx_session_add_common_parameters(nx);
	return remmina_nx_session_send_session_command(nx, "startsession", 105);
}

gboolean remmina_nx_session_attach(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	remmina_nx_session_add_common_parameters(nx);
	return remmina_nx_session_send_session_command(nx, "attachsession", 105);
}

gboolean remmina_nx_session_restore(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	remmina_nx_session_add_common_parameters(nx);
	return remmina_nx_session_send_session_command(nx, "restoresession", 105);
}

gboolean remmina_nx_session_terminate(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	return remmina_nx_session_send_session_command(nx, "terminate", 105);
}

static gpointer remmina_nx_session_tunnel_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaNXSession *nx = (RemminaNXSession*)data;
	gchar *ptr;
	ssize_t len = 0, lenw = 0;
	fd_set set;
	struct timeval timeout;
	ssh_channel channels[2];
	ssh_channel channels_out[2];
	gint sock;
	gint ret;
	gchar buffer[10240];
	gchar socketbuffer[10240];
	gchar *socketbuffer_ptr = NULL;
	gint socketbuffer_len = 0;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

	/* Accept a local connection */
	sock = accept(nx->server_sock, NULL, NULL);
	if (sock < 0) {
		remmina_nx_session_set_application_error(nx, "Failed to accept local socket");
		nx->thread = 0;
		return NULL;
	}
	close(nx->server_sock);
	nx->server_sock = -1;

	channels[0] = nx->channel;
	channels[1] = NULL;

	/* Start the tunnel data transmission */
	while (nx->running) {
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		FD_ZERO(&set);
		FD_SET(sock, &set);

		ret = ssh_select(channels, channels_out, sock + 1, &set, &timeout);
		if (!nx->running)
			break;
		if (ret == SSH_EINTR)
			continue;
		if (ret == -1)
			break;

		if (FD_ISSET(sock, &set)) {
			len = read(sock, buffer, sizeof(buffer));
			if (len == 0)
				nx->running = FALSE;
			else if (len > 0) {
				for (ptr = buffer, lenw = 0; len > 0; len -= lenw, ptr += lenw) {
					ssh_set_fd_towrite(nx->session);
					lenw = ssh_channel_write(channels[0], (char*)ptr, len);
					if (lenw <= 0) {
						nx->running = FALSE;
						break;
					}
				}
			}
		}

		if (!nx->running)
			break;

		if (channels_out[0] && socketbuffer_len <= 0) {
			len = ssh_channel_read_nonblocking(channels_out[0], socketbuffer, sizeof(socketbuffer), 0);
			if (len == SSH_ERROR || len == SSH_EOF) {
				nx->running = FALSE;
				break;
			} else if (len > 0) {
				socketbuffer_ptr = socketbuffer;
				socketbuffer_len = len;
			} else {
				/* Clean up the stderr buffer in case FreeNX send something there */
				len = ssh_channel_read_nonblocking(channels_out[0], buffer, sizeof(buffer), 1);
				if (len == SSH_ERROR || len == SSH_EOF) {
					nx->running = FALSE;
					break;
				}
			}
		}

		if (nx->running && socketbuffer_len > 0) {
			for (lenw = 0; socketbuffer_len > 0; socketbuffer_len -= lenw, socketbuffer_ptr += lenw) {
				lenw = write(sock, socketbuffer_ptr, socketbuffer_len);
				if (lenw == -1 && errno == EAGAIN && nx->running) {
					/* Sometimes we cannot write to a socket (always EAGAIN), probably because it’s internal
					 * buffer is full. We need read the pending bytes from the socket first. so here we simply
					 * break, leave the buffer there, and continue with other data */
					break;
				}
				if (lenw <= 0) {
					nx->running = FALSE;
					break;
				}
			}
		}
	}

	nx->thread = 0;

	return NULL;
}

gboolean remmina_nx_session_tunnel_open(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	gint port;
	gint sock;
	gint sockopt = 1;
	struct sockaddr_in sin;

	if (!nx->encryption)
		return TRUE;

	remmina_nx_session_send_command(nx, "bye");
	if (!remmina_nx_session_expect_status(nx, 999)) {
		/* Shoud not happen, just in case */
		remmina_nx_session_set_application_error(nx, "Server won’t say bye to us?");
		return FALSE;
	}

	port = (nx->localport ? nx->localport : nx->session_display) + 4000;

	/* Create the server socket that listens on the local port */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0) {
		remmina_nx_session_set_application_error(nx, "Failed to create socket.");
		return FALSE;
	}
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof(sockopt));

	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin))) {
		remmina_nx_session_set_application_error(nx, "Failed to bind on local port.");
		close(sock);
		return FALSE;
	}

	if (listen(sock, 1)) {
		remmina_nx_session_set_application_error(nx, "Failed to listen on local port.");
		close(sock);
		return FALSE;
	}

	nx->server_sock = sock;
	nx->running = TRUE;

	if (pthread_create(&nx->thread, NULL, remmina_nx_session_tunnel_main_thread, nx)) {
		remmina_nx_session_set_application_error(nx, "Failed to initialize pthread.");
		nx->thread = 0;
		return FALSE;
	}
	return TRUE;
}

static gchar*
remmina_nx_session_get_proxy_option(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	if (nx->encryption) {
		return g_strdup_printf("nx,session=%s,cookie=%s,id=%s,shmem=1,shpix=1,connect=127.0.0.1:%i",
			(gchar*)g_hash_table_lookup(nx->session_parameters, "session"), nx->proxy_cookie,
			nx->session_id, (nx->localport ? nx->localport : nx->session_display));
	} else {
		return g_strdup_printf("nx,session=%s,cookie=%s,id=%s,shmem=1,shpix=1,connect=%s:%i",
			(gchar*)g_hash_table_lookup(nx->session_parameters, "session"), nx->proxy_cookie,
			nx->session_id, nx->server, nx->session_display);
	}
}

gboolean remmina_nx_session_invoke_proxy(RemminaNXSession *nx, gint display, GChildWatchFunc exit_func, gpointer user_data)
{
	TRACE_CALL(__func__);
	gchar *argv[50];
	gint argc;
	GError *error = NULL;
	gboolean ret;
	gchar **envp;
	gchar *s;
	gint i;

	/* Copy all current environment variable, but change DISPLAY. Assume we should always have DISPLAY… */
	if (display >= 0) {
		envp = g_listenv();
		for (i = 0; envp[i]; i++) {
			if (g_strcmp0(envp[i], "DISPLAY") == 0) {
				s = g_strdup_printf("DISPLAY=:%i", display);
			} else {
				s = g_strdup_printf("%s=%s", envp[i], g_getenv(envp[i]));
			}
			g_free(envp[i]);
			envp[i] = s;
		}
	} else {
		envp = NULL;
	}

	argc = 0;
	argv[argc++] = g_strdup("nxproxy");
	argv[argc++] = g_strdup("-S");
	argv[argc++] = remmina_nx_session_get_proxy_option(nx);
	argv[argc++] = NULL;

	ret = g_spawn_async(NULL, argv, envp, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &nx->proxy_pid,
		&error);
	g_strfreev(envp);
	for (i = 0; i < argc; i++)
		g_free(argv[i]);

	if (!ret) {
		remmina_nx_session_set_application_error(nx, "%s", error->message);
		return FALSE;
	}

	if (exit_func) {
		nx->proxy_watch_source = g_child_watch_add(nx->proxy_pid, exit_func, user_data);
	}

	return TRUE;
}

void remmina_nx_session_bye(RemminaNXSession *nx)
{
	TRACE_CALL(__func__);
	remmina_nx_session_send_command(nx, "bye");
	remmina_nx_session_get_response(nx);
}

