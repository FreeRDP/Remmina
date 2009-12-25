/*
 * Remmina - GTK+/Gnome Remote Desktop Client
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

#include "common/remminaplugincommon.h"
#include <glib/gstdio.h>
#include <libssh/libssh.h>
#include "remminanxsession.h"

/* Some missing stuff in libssh */
#define REMMINA_SSH_TYPE_DSS 1
#define REMMINA_SSH_TYPE_RSA 2

static gboolean
remmina_get_keytype (const gchar *private_key_file, gint *keytype, gboolean *encrypted)
{
    FILE *fp;
    gchar buf1[100], buf2[100];

    if ((fp = g_fopen (private_key_file, "r")) == NULL)
    {
        return FALSE;
    }
    if (!fgets (buf1, sizeof (buf1), fp) || !fgets (buf2, sizeof (buf2), fp))
    {
        fclose (fp);
        return FALSE;
    }
    fclose (fp);

    if (strstr (buf1, "BEGIN RSA")) *keytype = REMMINA_SSH_TYPE_RSA;
    else if (strstr (buf1, "BEGIN DSA")) *keytype = REMMINA_SSH_TYPE_DSS;
    else return FALSE;

    *encrypted = (strstr (buf2, "ENCRYPTED") ? TRUE : FALSE);

    return TRUE;
}

/*****/

static const gchar nx_default_private_key[] =
"-----BEGIN DSA PRIVATE KEY-----\n"
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

struct _RemminaNXSession
{
    /* Common SSH members */
    ssh_session session;
    ssh_channel channel;
    gchar *server;
    gchar *error;

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

    GPid proxy_pid;
    guint proxy_watch_source;
};

RemminaNXSession*
remmina_nx_session_new (void)
{
    RemminaNXSession *nx;

    nx = g_new0 (RemminaNXSession, 1);
    
    nx->session_parameters = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    nx->response = g_string_new (NULL);
    nx->status = -1;
    nx->encryption = 1;
    nx->server_sock = -1;

    return nx;
}

void
remmina_nx_session_free (RemminaNXSession *nx)
{
    pthread_t thread;

    if (nx->proxy_watch_source)
    {
        g_source_remove (nx->proxy_watch_source);
        nx->proxy_watch_source = 0;
    }
    if (nx->proxy_pid)
    {
        kill (nx->proxy_pid, SIGTERM);
        g_spawn_close_pid (nx->proxy_pid);
        nx->proxy_pid = 0;
    }
    thread = nx->thread;
    if (thread)
    {
        nx->running = FALSE;
        pthread_cancel (thread);
        pthread_join (thread, NULL);
        nx->thread = 0;
    }
    if (nx->channel)
    {
        channel_close (nx->channel);
        channel_free (nx->channel);
    }
    if (nx->server_sock >= 0)
    {
        close (nx->server_sock);
        nx->server_sock = -1;
    }

    g_free (nx->server);
    g_free (nx->error);
    g_hash_table_destroy (nx->session_parameters);
    g_string_free (nx->response, TRUE);
    g_free (nx->version);
    g_free (nx->session_id);
    g_free (nx->proxy_cookie);

    if (nx->session)
    {
        ssh_free (nx->session);
        nx->session = NULL;
    }
    g_free (nx);
}

static void
remmina_nx_session_set_error (RemminaNXSession *nx, const gchar *fmt)
{
    const gchar *err;

    err = ssh_get_error (nx->session);
    nx->error = g_strdup_printf (fmt, err);
}

static void
remmina_nx_session_set_application_error (RemminaNXSession *nx, const gchar *fmt, ...)
{
    va_list args;

    va_start (args, fmt);
    nx->error = g_strdup_vprintf (fmt, args);
    va_end (args);
}

gboolean
remmina_nx_session_has_error (RemminaNXSession *nx)
{
    return (nx->error != NULL);
}

const gchar*
remmina_nx_session_get_error (RemminaNXSession *nx)
{
    return nx->error;
}

void
remmina_nx_session_set_encryption (RemminaNXSession *nx, gint encryption)
{
    nx->encryption = encryption;
}

void
remmina_nx_session_set_localport (RemminaNXSession *nx, gint localport)
{
    nx->localport = localport;
}

static gboolean
remmina_nx_session_get_response (RemminaNXSession *nx)
{
    struct timeval timeout;
    ssh_channel ch[2];
    ssh_buffer buffer;
    gint len;

    timeout.tv_sec = 60;
    timeout.tv_usec = 0;
    ch[0] = nx->channel;
    ch[1] = NULL;
    channel_select (ch, NULL, NULL, &timeout);

    len = channel_poll (nx->channel, 0);
    if (len == SSH_ERROR)
    {
        remmina_nx_session_set_error (nx, "Error reading channel: %s");
        return FALSE;
    }
    if (len <= 0) return TRUE;

    buffer = buffer_new ();
    len = channel_read_buffer (nx->channel, buffer, len, FALSE);
    if (len <= 0)
    {
        remmina_nx_session_set_application_error (nx, "Channel closed.");
        return FALSE;
    }
    if (len > 0)
    {
        g_string_append_len (nx->response, (const gchar*) buffer_get (buffer), len);
    }

gchar *str;
str = g_new0 (gchar, len + 1);
memcpy (str, buffer_get (buffer), len);
g_print ("%s", str);
g_free (str);

    buffer_free (buffer);
    return TRUE;
}

static gint
remmina_nx_session_parse_line (RemminaNXSession *nx, const gchar *line, gchar **valueptr)
{
    gchar *s;
    gchar *ptr;
    gint status;

    *valueptr = NULL;

    /* Get the server version from the initial line */
    if (!nx->version)
    {
        s = g_ascii_strdown (line, -1);
        ptr = strstr (s, nx_hello_server_msg);
        if (!ptr)
        {
            /* Try to use a default version */
            nx->version = g_strdup ("3.3.0");
        }
        else
        {
            nx->version = g_strdup (ptr + strlen (nx_hello_server_msg));
            ptr = strchr (nx->version, ' ');
            if (ptr) *ptr = '\0';
            /* NoMachine NX append a dash+subversion. Need to be removed. */
            ptr = strchr (nx->version, '-');
            if (ptr) *ptr = '\0';
        }
        g_free (s);
        return nx->status;
    }

    if (sscanf (line, "NX> %i ", &status) < 1) return nx->status;
    nx->status = status;
    ptr = strchr (line, ':');
    if (!ptr) return status;
    *valueptr = ptr + 2;
    return status;
}

static gint
remmina_nx_session_parse_response (RemminaNXSession *nx)
{
    gchar *line;
    gchar *pos, *ptr, *p;
    gint len, l;
    gint status;

    if (nx->response_pos >= nx->response->len) return -1;

    pos = nx->response->str + nx->response_pos;
    while ((ptr = strchr (pos, '\n')) != NULL)
    {
        len = ((gint) (ptr - pos)) + 1;

        line = g_strndup (pos, len - 1);
        l = strlen (line);
        if (l > 0 && line[l - 1] == '\r')
        {
            line[l - 1] = '\0';
        }
        status = remmina_nx_session_parse_line (nx, line, &p);
        if (status >= 400 && status <= 599)
        {
            remmina_nx_session_set_application_error (nx, "%s", line);
            return status;
        }
        switch (status)
        {
        case 700:
            nx->session_id = g_strdup (p);
            break;
        case 705:
            nx->session_display = atoi (p);
            break;
        case 701:
            nx->proxy_cookie = g_strdup (p);
            break;
        }

        g_free (line);
        nx->response_pos += len;
        pos += len;
    }
    if (sscanf (pos, "NX> %i ", &status) < 1)
    {
        status = nx->status;
    }
    else
    {
        nx->response_pos += 8;
    }
    nx->status = -1;
    return status;
}

static gboolean
remmina_nx_session_expect_status (RemminaNXSession *nx, gint status)
{
    while (remmina_nx_session_parse_response (nx) != status)
    {
        if (remmina_nx_session_has_error (nx)) return FALSE;
        if (!remmina_nx_session_get_response (nx)) return FALSE;
    }
    return TRUE;
}

static void
remmina_nx_session_send_command (RemminaNXSession *nx, const gchar *cmdfmt, ...)
{
    va_list args;
    gchar *cmd;

    va_start (args, cmdfmt);
    cmd = g_strdup_vprintf (cmdfmt, args);
    channel_write (nx->channel, cmd, strlen (cmd));
    g_free (cmd);

    channel_write (nx->channel, "\n", 1);
}

gboolean
remmina_nx_session_open (RemminaNXSession *nx, const gchar *server, guint port,
    const gchar *private_key_file, RemminaNXPassphraseCallback passphrase_func, gpointer userdata)
{
    gint ret;
    ssh_private_key privkey;
    ssh_public_key pubkey;
    ssh_string pubkeystr;
    gint keytype;
    gboolean encrypted;
    gchar *passphrase = NULL;
    gchar tmpfile[L_tmpnam + 1];

    nx->session = ssh_new ();
    ssh_options_set (nx->session, SSH_OPTIONS_HOST, server);
    ssh_options_set (nx->session, SSH_OPTIONS_PORT, &port);
    ssh_options_set (nx->session, SSH_OPTIONS_USER, "nx");

    if (private_key_file && private_key_file[0])
    {
        if (!remmina_get_keytype (private_key_file, &keytype, &encrypted))
        {
            remmina_nx_session_set_application_error (nx, "Invalid private key file.");
            return FALSE;
        }
        if (encrypted && !passphrase_func (&passphrase, userdata))
        {
            return FALSE;
        }
        privkey = privatekey_from_file (nx->session, private_key_file, keytype, (passphrase ? passphrase : ""));
        g_free (passphrase);
    }
    else
    {
        /* Use NoMachine's default nx private key */
        if ((tmpnam (tmpfile)) == NULL ||
            !g_file_set_contents (tmpfile, nx_default_private_key, -1, NULL))
        {
            remmina_nx_session_set_application_error (nx, "Failed to create temporary private key file.");
            return FALSE;
        }
        privkey = privatekey_from_file (nx->session, tmpfile, REMMINA_SSH_TYPE_DSS, "");
        g_unlink (tmpfile);
    }

    if (privkey == NULL)
    {
        remmina_nx_session_set_error (nx, "Invalid private key file: %s");
        return FALSE;
    }
    pubkey = publickey_from_privatekey (privkey);
    pubkeystr = publickey_to_string (pubkey);
    publickey_free (pubkey);

    if (ssh_connect (nx->session))
    {
        string_free (pubkeystr);
        privatekey_free (privkey);
        remmina_nx_session_set_error (nx, "Failed to startup SSH session: %s");
        return FALSE;
    }

    ret = ssh_userauth_pubkey (nx->session, NULL, pubkeystr, privkey);
    string_free (pubkeystr);
    privatekey_free (privkey);

    if (ret != SSH_AUTH_SUCCESS)
    {
        remmina_nx_session_set_error (nx, "NX SSH authentication failed: %s");
        return FALSE;
    }

    if ((nx->channel = channel_new (nx->session)) == NULL ||
        channel_open_session (nx->channel))
    {
        return FALSE;
    }

    if (channel_request_shell(nx->channel))
    {
        return FALSE;
    }

    /* NX server starts the session with an initial 105 status */
    if (!remmina_nx_session_expect_status (nx, 105)) return FALSE;

    /* Say hello to the NX server */
    remmina_nx_session_send_command (nx, "HELLO NXCLIENT - Version %s", nx->version);
    if (!remmina_nx_session_expect_status (nx, 105)) return FALSE;

    /* Set the NX session environment */
    remmina_nx_session_send_command (nx, "SET SHELL_MODE SHELL");
    if (!remmina_nx_session_expect_status (nx, 105)) return FALSE;
    remmina_nx_session_send_command (nx, "SET AUTH_MODE PASSWORD");
    if (!remmina_nx_session_expect_status (nx, 105)) return FALSE;

    nx->server = g_strdup (server);

    return TRUE;
}

gboolean
remmina_nx_session_login (RemminaNXSession *nx, const gchar *username, const gchar *password)
{
    /* Login to the NX server */
    remmina_nx_session_send_command (nx, "login");
    if (!remmina_nx_session_expect_status (nx, 101)) return FALSE;
    remmina_nx_session_send_command (nx, username);
    if (!remmina_nx_session_expect_status (nx, 102)) return FALSE;
    remmina_nx_session_send_command (nx, password);
    if (!remmina_nx_session_expect_status (nx, 105)) return FALSE;

    return TRUE;
}

void
remmina_nx_session_add_parameter (RemminaNXSession *nx, const gchar *name, const gchar *valuefmt, ...)
{
    va_list args;
    gchar *value;

    va_start (args, valuefmt);
    value = g_strdup_vprintf (valuefmt, args);
    g_hash_table_insert (nx->session_parameters, g_strdup (name), value);
}

static gboolean
remmina_nx_session_send_session_command (RemminaNXSession *nx, const gchar *cmd_type)
{
    GString *cmd;
    GHashTableIter iter;
    gchar *key, *value;

    cmd = g_string_new (cmd_type);
    g_hash_table_iter_init (&iter, nx->session_parameters);
    while (g_hash_table_iter_next (&iter, (gpointer*) &key, (gpointer*) &value)) 
    {
        g_string_append_printf (cmd, " --%s=\"%s\"", key, value);
    }

    remmina_nx_session_send_command (nx, cmd->str);
    g_string_free (cmd, TRUE);

    g_hash_table_remove_all (nx->session_parameters);

    return remmina_nx_session_expect_status (nx, 105);
}

gboolean
remmina_nx_session_list (RemminaNXSession *nx)
{
    return remmina_nx_session_send_session_command (nx, "listsession");
}

gboolean
remmina_nx_session_start (RemminaNXSession *nx)
{
    gchar *value;

    /* Add fixed session parameters for startsession */
    remmina_nx_session_add_parameter (nx, "cache", "16M");
    remmina_nx_session_add_parameter (nx, "images", "64M");
    remmina_nx_session_add_parameter (nx, "render", "1");
    remmina_nx_session_add_parameter (nx, "backingstore", "when_requested");
    remmina_nx_session_add_parameter (nx, "imagecompressionmethod", "2");
    remmina_nx_session_add_parameter (nx, "agent_server", "");
    remmina_nx_session_add_parameter (nx, "agent_user", "");
    remmina_nx_session_add_parameter (nx, "agent_password", "");

    value = g_strdup_printf ("%i", nx->encryption);
    remmina_nx_session_add_parameter (nx, "encryption", value);
    g_free (value);

    return remmina_nx_session_send_session_command (nx, "startsession");
}

static gpointer
remmina_nx_session_tunnel_main_thread (gpointer data)
{
    RemminaNXSession *nx = (RemminaNXSession*) data;
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

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    /* Accept a local connection */
    sock = accept (nx->server_sock, NULL, NULL);
    if (sock < 0)
    {
        remmina_nx_session_set_application_error (nx, "Failed to accept local socket");
        nx->thread = 0;
        return NULL;
    }
    close (nx->server_sock);
    nx->server_sock = -1;

    channels[0] = nx->channel;
    channels[1] = NULL;

    /* Start the tunnel data transmittion */
    while (nx->running)
    {
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        FD_ZERO (&set);
        FD_SET (sock, &set);

        ret = ssh_select (channels, channels_out, FD_SETSIZE, &set, &timeout);
        if (!nx->running) break;
        if (ret == SSH_EINTR) continue;
        if (ret == -1) break;

        if (FD_ISSET (sock, &set))
        {
            len = read (sock, buffer, sizeof (buffer));
            if (len == 0) nx->running = FALSE;
            else if (len > 0)
            {
                for (ptr = buffer, lenw = 0; len > 0; len -= lenw, ptr += lenw)
                {
                    lenw = channel_write (channels[0], (char*) ptr, len);
                    if (lenw <= 0)
                    {
                        nx->running = FALSE;
                        break;
                    }
                }
            }
        }

        if (!nx->running) break;

        if (channels_out[0] && socketbuffer_len <= 0)
        {
            len = channel_read_nonblocking (channels_out[0], socketbuffer, sizeof (socketbuffer), 0);
            if (len == SSH_ERROR || len == SSH_EOF)
            {
                nx->running = FALSE;
                break;
            }
            else if (len > 0)
            {
                socketbuffer_ptr = socketbuffer;
                socketbuffer_len = len;
            }
        }

        if (nx->running && socketbuffer_len > 0)
        {
            for (lenw = 0; socketbuffer_len > 0; socketbuffer_len -= lenw, socketbuffer_ptr += lenw)
            {
                lenw = write (sock, socketbuffer_ptr, socketbuffer_len);
                if (lenw == -1 && errno == EAGAIN && nx->running)
                {
                    /* Sometimes we cannot write to a socket (always EAGAIN), probably because it's internal
                     * buffer is full. We need read the pending bytes from the socket first. so here we simply
                     * break, leave the buffer there, and continue with other data */
                    break;
                }
                if (lenw <= 0)
                {
                    nx->running = FALSE;
                    break;
                }
            }
        }
    }

    nx->thread = 0;

    return NULL;
}

gboolean
remmina_nx_session_tunnel_open (RemminaNXSession *nx)
{
    gint port;
    gint sock;
    gint sockopt = 1;
    struct sockaddr_in sin;

    if (!nx->encryption) return TRUE;

    port = (nx->localport ? nx->localport : nx->session_display) + 4000;

    /* Create the server socket that listens on the local port */
    sock = socket (AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        remmina_nx_session_set_application_error (nx, "Failed to create socket.");
        return FALSE;
    }
    setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &sockopt, sizeof (sockopt));

    sin.sin_family = AF_INET;
    sin.sin_port = htons (port);
    sin.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind (sock, (struct sockaddr *) &sin, sizeof(sin)))
    {
        remmina_nx_session_set_application_error (nx, "Failed to bind on local port.");
        close (sock);
        return FALSE;
    }

    if (listen (sock, 1))
    {
        remmina_nx_session_set_application_error (nx, "Failed to listen on local port.");
        close (sock);
        return FALSE;
    }

    nx->server_sock = sock;
    nx->running = TRUE;

    if (pthread_create (&nx->thread, NULL, remmina_nx_session_tunnel_main_thread, nx))
    {
        remmina_nx_session_set_application_error (nx, "Failed to initialize pthread.");
        nx->thread = 0;
        return FALSE;
    }
    return TRUE;
}

static gchar*
remmina_nx_session_get_proxy_option (RemminaNXSession *nx)
{
    if (nx->encryption)
    {
        remmina_nx_session_send_command (nx, "bye");
        if (!remmina_nx_session_expect_status (nx, 999))
        {
            /* Shoud not happen, just in case */
            remmina_nx_session_set_application_error (nx, "Server won't say bye to us?");
            return FALSE;
        }

        return g_strdup_printf ("nx,session=%s,cookie=%s,id=%s,connect=127.0.0.1:%i",
            (gchar*) g_hash_table_lookup (nx->session_parameters, "session"),
            nx->proxy_cookie, nx->session_id, (nx->localport ? nx->localport : nx->session_display));
    }
    else
    {
        return g_strdup_printf ("nx,session=%s,cookie=%s,id=%s,connect=%s:%i",
            (gchar*) g_hash_table_lookup (nx->session_parameters, "session"),
            nx->proxy_cookie, nx->session_id,
            nx->server, nx->session_display);
    }
}

gboolean
remmina_nx_session_invoke_proxy (RemminaNXSession *nx, const gint display,
    GChildWatchFunc exit_func, gpointer user_data)
{
    gchar *argv[50];
    gint argc;
    GError *error = NULL;
    gboolean ret;
    gchar **envp;
    gchar *s;
    gint i;

    /* Copy all current environment variable, but change DISPLAY. Assume we should always have DISPLAY... */
    envp = g_listenv ();
    for (i = 0; envp[i]; i++)
    {
        if (g_strcmp0 (envp[i], "DISPLAY") == 0)
        {
            s = g_strdup_printf ("DISPLAY=:%i", display);
        }
        else
        {
            s = g_strdup_printf ("%s=%s", envp[i], g_getenv (envp[i]));
        }
        g_free (envp[i]);
        envp[i] = s;
    }

    argc = 0;
    argv[argc++] = g_strdup ("nxproxy");
    argv[argc++] = g_strdup ("-S");
    argv[argc++] = remmina_nx_session_get_proxy_option (nx);
    argv[argc++] = NULL;

    ret = g_spawn_async (NULL, argv, envp, G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
        NULL, NULL, &nx->proxy_pid, &error);
    g_strfreev (envp);
    for (i = 0; i < argc; i++) g_free (argv[i]);

    if (!ret)
    {
        remmina_nx_session_set_application_error (nx, "%s", error->message);
        return FALSE;
    }

    if (exit_func)
    {
        nx->proxy_watch_source = g_child_watch_add (nx->proxy_pid, exit_func, user_data);
    }

    return TRUE;
}

void
remmina_nx_session_bye (RemminaNXSession *nx)
{
    remmina_nx_session_send_command (nx, "bye");
    remmina_nx_session_get_response (nx);
}

