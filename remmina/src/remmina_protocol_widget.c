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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "config.h"
#include "remmina_public.h"
#include "remmina_pref.h"
#include "remmina_ssh.h"
#include "remmina_chat_window.h"
#include "remmina_plugin_manager.h"
#include "remmina_connection_window.h"
#include "remmina_protocol_widget.h"

struct _RemminaProtocolWidgetPriv
{
	GtkWidget* init_dialog;

	RemminaFile* remmina_file;
	RemminaProtocolPlugin* plugin;
	RemminaProtocolFeature* features;

	gint width;
	gint height;
	gboolean scale;
	gboolean expand;

	gboolean has_error;
	gchar* error_message;
	RemminaSSHTunnel* ssh_tunnel;
	RemminaTunnelInitFunc init_func;

	GtkWidget* chat_window;

	gboolean closed;

	RemminaHostkeyFunc hostkey_func;
	gpointer hostkey_func_data;
};

G_DEFINE_TYPE(RemminaProtocolWidget, remmina_protocol_widget, GTK_TYPE_EVENT_BOX)

enum
{
	CONNECT_SIGNAL,
	DISCONNECT_SIGNAL,
	DESKTOP_RESIZE_SIGNAL,
	UPDATE_ALIGN_SIGNAL,
	LAST_SIGNAL
};

typedef struct _RemminaProtocolWidgetSignalData
{
	RemminaProtocolWidget* gp;
	const gchar* signal_name;
} RemminaProtocolWidgetSignalData;

static guint remmina_protocol_widget_signals[LAST_SIGNAL] =
{ 0 };

static void remmina_protocol_widget_class_init(RemminaProtocolWidgetClass *klass)
{
	remmina_protocol_widget_signals[CONNECT_SIGNAL] = g_signal_new("connect", G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, connect), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_protocol_widget_signals[DISCONNECT_SIGNAL] = g_signal_new("disconnect", G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, disconnect), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_protocol_widget_signals[DESKTOP_RESIZE_SIGNAL] = g_signal_new("desktop-resize", G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, desktop_resize), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
	remmina_protocol_widget_signals[UPDATE_ALIGN_SIGNAL] = g_signal_new("update-align", G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaProtocolWidgetClass, update_align), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void remmina_protocol_widget_init_cancel(RemminaInitDialog *dialog, gint response_id, RemminaProtocolWidget* gp)
{
	if ((response_id == GTK_RESPONSE_CANCEL || response_id == GTK_RESPONSE_DELETE_EVENT)
			&& dialog->mode == REMMINA_INIT_MODE_CONNECTING)
	{
		remmina_protocol_widget_close_connection(gp);
	}
}

static void remmina_protocol_widget_show_init_dialog(RemminaProtocolWidget* gp, const gchar *name)
{
	if (gp->priv->init_dialog)
	{
		gtk_widget_destroy(gp->priv->init_dialog);
	}
	gp->priv->init_dialog = remmina_init_dialog_new(_("Connecting to '%s'..."), (name ? name : "*"));
	g_signal_connect(G_OBJECT(gp->priv->init_dialog), "response", G_CALLBACK(remmina_protocol_widget_init_cancel), gp);
	gtk_widget_show(gp->priv->init_dialog);
}

static void remmina_protocol_widget_hide_init_dialog(RemminaProtocolWidget* gp)
{
	if (gp->priv->init_dialog && GTK_IS_WIDGET(gp->priv->init_dialog))
		gtk_widget_destroy(gp->priv->init_dialog);

	gp->priv->init_dialog = NULL;
}

static void remmina_protocol_widget_destroy(RemminaProtocolWidget* gp, gpointer data)
{
	remmina_protocol_widget_hide_init_dialog(gp);
	g_free(gp->priv->features);
	g_free(gp->priv->error_message);
	g_free(gp->priv);
}

static void remmina_protocol_widget_connect(RemminaProtocolWidget* gp, gpointer data)
{
#ifdef HAVE_LIBSSH
	if (gp->priv->ssh_tunnel)
	{
		remmina_ssh_tunnel_cancel_accept (gp->priv->ssh_tunnel);
	}
#endif
	remmina_protocol_widget_hide_init_dialog(gp);
}

static void remmina_protocol_widget_disconnect(RemminaProtocolWidget* gp, gpointer data)
{
	remmina_protocol_widget_hide_init_dialog(gp);
}

void remmina_protocol_widget_grab_focus(RemminaProtocolWidget* gp)
{
	GtkWidget* child;

	child = gtk_bin_get_child(GTK_BIN(gp));

	if (child)
	{
		gtk_widget_set_can_focus(child, TRUE);
		gtk_widget_grab_focus(child);
	}
}

static void remmina_protocol_widget_init(RemminaProtocolWidget* gp)
{
	RemminaProtocolWidgetPriv *priv;

	priv = g_new0(RemminaProtocolWidgetPriv, 1);
	gp->priv = priv;

	g_signal_connect(G_OBJECT(gp), "destroy", G_CALLBACK(remmina_protocol_widget_destroy), NULL);
	g_signal_connect(G_OBJECT(gp), "connect", G_CALLBACK(remmina_protocol_widget_connect), NULL);
	g_signal_connect(G_OBJECT(gp), "disconnect", G_CALLBACK(remmina_protocol_widget_disconnect), NULL);
}

void remmina_protocol_widget_open_connection_real(gpointer data)
{
	RemminaProtocolWidget* gp = REMMINA_PROTOCOL_WIDGET(data);
	RemminaProtocolPlugin* plugin;
	RemminaFile* remminafile = gp->priv->remmina_file;
	RemminaProtocolFeature* feature;
	gint num_plugin;
	gint num_ssh;

	/* Locate the protocol plugin */
	plugin = (RemminaProtocolPlugin*) remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL,
			remmina_file_get_string(remminafile, "protocol"));

	if (!plugin || !plugin->init || !plugin->open_connection)
	{
		remmina_protocol_widget_set_error(gp, _("Protocol plugin %s is not installed."),
				remmina_file_get_string(remminafile, "protocol"));
		remmina_protocol_widget_close_connection(gp);
		return;
	}

	plugin->init(gp);

	gp->priv->plugin = plugin;

	for (num_plugin = 0, feature = (RemminaProtocolFeature*) plugin->features; feature && feature->type; num_plugin++, feature++)
	{
	}

	num_ssh = 0;
#ifdef HAVE_LIBSSH
	if (remmina_file_get_int(gp->priv->remmina_file, "ssh_enabled", FALSE))
	{
		num_ssh += 2;
	}
#endif
	if (num_plugin + num_ssh == 0)
	{
		gp->priv->features = NULL;
	}
	else
	{
		gp->priv->features = g_new0(RemminaProtocolFeature, num_plugin + num_ssh + 1);
		feature = gp->priv->features;
		if (plugin->features)
		{
			memcpy(feature, plugin->features, sizeof(RemminaProtocolFeature) * num_plugin);
			feature += num_plugin;
		}
#ifdef HAVE_LIBSSH
		if (num_ssh)
		{
			feature->type = REMMINA_PROTOCOL_FEATURE_TYPE_TOOL;
			feature->id = REMMINA_PROTOCOL_FEATURE_TOOL_SSH;
			feature->opt1 = _("Open Secure Shell in New Terminal...");
			feature->opt2 = "utilities-terminal";
			feature++;

			feature->type = REMMINA_PROTOCOL_FEATURE_TYPE_TOOL;
			feature->id = REMMINA_PROTOCOL_FEATURE_TOOL_SFTP;
			feature->opt1 = _("Open Secure File Transfer...");
			feature->opt2 = "folder-remote";
			feature++;
		}
		feature->type = REMMINA_PROTOCOL_FEATURE_TYPE_END;
#endif
	}

	if (!plugin->open_connection(gp))
	{
		remmina_protocol_widget_close_connection(gp);
	}
}

void remmina_protocol_widget_open_connection(RemminaProtocolWidget* gp, RemminaFile* remminafile)
{
	gp->priv->remmina_file = remminafile;
	gp->priv->scale = remmina_file_get_int(remminafile, "scale", FALSE);

	remmina_protocol_widget_show_init_dialog(gp, remmina_file_get_string(remminafile, "name"));

	remmina_protocol_widget_open_connection_real(gp);
}

gboolean remmina_protocol_widget_close_connection(RemminaProtocolWidget* gp)
{
#if GTK_VERSION == 3
	GdkDisplay *display;
	GdkDeviceManager *manager;
	GdkDevice *device = NULL;
#endif

	if (!GTK_IS_WIDGET(gp) || gp->priv->closed)
		return FALSE;

	gp->priv->closed = TRUE;
#if GTK_VERSION == 3
	display = gtk_widget_get_display(GTK_WIDGET(gp));
	manager = gdk_display_get_device_manager(display);
	device = gdk_device_manager_get_client_pointer(manager);
	if (device != NULL)
	{
		gdk_device_ungrab(device, GDK_CURRENT_TIME);
	}
#elif GTK_VERSION == 2
	gdk_keyboard_ungrab(GDK_CURRENT_TIME);
#endif

	if (gp->priv->chat_window)
	{
		gtk_widget_destroy(gp->priv->chat_window);
		gp->priv->chat_window = NULL;
	}

#ifdef HAVE_LIBSSH
	if (gp->priv->ssh_tunnel)
	{
		remmina_ssh_tunnel_free(gp->priv->ssh_tunnel);
		gp->priv->ssh_tunnel = NULL;
	}
#endif

	if (!gp->priv->plugin || !gp->priv->plugin->close_connection)
	{
		remmina_protocol_widget_emit_signal(gp, "disconnect");
		return FALSE;
	}

	return gp->priv->plugin->close_connection(gp);
}

static gboolean remmina_protocol_widget_emit_signal_timeout(gpointer user_data)
{
	RemminaProtocolWidgetSignalData* data = (RemminaProtocolWidgetSignalData*) user_data;
	g_signal_emit_by_name(G_OBJECT(data->gp), data->signal_name);
	g_free(data);
	return FALSE;
}

void remmina_protocol_widget_emit_signal(RemminaProtocolWidget* gp, const gchar* signal_name)
{
	RemminaProtocolWidgetSignalData* data;

	data = g_new(RemminaProtocolWidgetSignalData, 1);
	data->gp = gp;
	data->signal_name = signal_name;
	TIMEOUT_ADD(0, remmina_protocol_widget_emit_signal_timeout, data);
}

const RemminaProtocolFeature* remmina_protocol_widget_get_features(RemminaProtocolWidget* gp)
{
	return gp->priv->features;
}

const gchar* remmina_protocol_widget_get_domain(RemminaProtocolWidget* gp)
{
	return gp->priv->plugin->domain;
}

gboolean remmina_protocol_widget_query_feature_by_type(RemminaProtocolWidget* gp, RemminaProtocolFeatureType type)
{
	const RemminaProtocolFeature *feature;

#ifdef HAVE_LIBSSH
	if (type == REMMINA_PROTOCOL_FEATURE_TYPE_TOOL &&
			remmina_file_get_int (gp->priv->remmina_file, "ssh_enabled", FALSE))
	{
		return TRUE;
	}
#endif
	for (feature = gp->priv->plugin->features; feature && feature->type; feature++)
	{
		if (feature->type == type)
			return TRUE;
	}
	return FALSE;
}

gboolean remmina_protocol_widget_query_feature_by_ref(RemminaProtocolWidget* gp, const RemminaProtocolFeature *feature)
{
	return gp->priv->plugin->query_feature(gp, feature);
}

void remmina_protocol_widget_call_feature_by_type(RemminaProtocolWidget* gp, RemminaProtocolFeatureType type, gint id)
{
	const RemminaProtocolFeature *feature;

	for (feature = gp->priv->plugin->features; feature && feature->type; feature++)
	{
		if (feature->type == type && (id == 0 || feature->id == id))
		{
			remmina_protocol_widget_call_feature_by_ref(gp, feature);
			break;
		}
	}
}

void remmina_protocol_widget_call_feature_by_ref(RemminaProtocolWidget* gp, const RemminaProtocolFeature *feature)
{
	switch (feature->id)
	{
#ifdef HAVE_LIBSSH
		case REMMINA_PROTOCOL_FEATURE_TOOL_SSH:
		if (gp->priv->ssh_tunnel)
		{
			remmina_connection_window_open_from_file_full (
					remmina_file_dup_temp_protocol (gp->priv->remmina_file, "SSH"), NULL, gp->priv->ssh_tunnel, NULL);
			return;
		}
		break;

		case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
		if (gp->priv->ssh_tunnel)
		{
			remmina_connection_window_open_from_file_full (
					remmina_file_dup_temp_protocol (gp->priv->remmina_file, "SFTP"), NULL, gp->priv->ssh_tunnel, NULL);
			return;
		}
		break;
#endif
		default:
			break;
	}
	gp->priv->plugin->call_feature(gp, feature);
}

static gboolean remmina_protocol_widget_on_key_press(GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget* gp)
{
	if (gp->priv->hostkey_func)
	{
		return gp->priv->hostkey_func(gp, event->keyval, FALSE, gp->priv->hostkey_func_data);
	}
	return FALSE;
}

static gboolean remmina_protocol_widget_on_key_release(GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget* gp)
{
	if (gp->priv->hostkey_func)
	{
		return gp->priv->hostkey_func(gp, event->keyval, TRUE, gp->priv->hostkey_func_data);
	}
	return FALSE;
}

void remmina_protocol_widget_register_hostkey(RemminaProtocolWidget* gp, GtkWidget *widget)
{
	g_signal_connect(G_OBJECT(widget), "key-press-event", G_CALLBACK(remmina_protocol_widget_on_key_press), gp);
	g_signal_connect(G_OBJECT(widget), "key-release-event", G_CALLBACK(remmina_protocol_widget_on_key_release), gp);
}

void remmina_protocol_widget_set_hostkey_func(RemminaProtocolWidget* gp, RemminaHostkeyFunc func, gpointer data)
{
	gp->priv->hostkey_func = func;
	gp->priv->hostkey_func_data = data;
}

#ifdef HAVE_LIBSSH
static gboolean remmina_protocol_widget_init_tunnel (RemminaProtocolWidget* gp)
{
	RemminaSSHTunnel *tunnel;
	gint ret;

	/* Reuse existing SSH connection if it's reconnecting to destination */
	if (gp->priv->ssh_tunnel == NULL)
	{
		tunnel = remmina_ssh_tunnel_new_from_file (gp->priv->remmina_file);

		THREADS_ENTER
		remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
				_("Connecting to SSH server %s..."), REMMINA_SSH (tunnel)->server);
		THREADS_LEAVE

		if (!remmina_ssh_init_session (REMMINA_SSH (tunnel)))
		{
			remmina_protocol_widget_set_error (gp, REMMINA_SSH (tunnel)->error);
			remmina_ssh_tunnel_free (tunnel);
			return FALSE;
		}

		ret = remmina_ssh_auth_gui (REMMINA_SSH (tunnel), REMMINA_INIT_DIALOG (gp->priv->init_dialog), TRUE);
		if (ret <= 0)
		{
			if (ret == 0)
			{
				remmina_protocol_widget_set_error (gp, REMMINA_SSH (tunnel)->error);
			}
			remmina_ssh_tunnel_free (tunnel);
			return FALSE;
		}

		gp->priv->ssh_tunnel = tunnel;
	}

	return TRUE;
}
#endif

gchar* remmina_protocol_widget_start_direct_tunnel(RemminaProtocolWidget* gp, gint default_port, gboolean port_plus)
{
	const gchar *server;
	gchar *host, *dest;
	gint port;

	server = remmina_file_get_string(gp->priv->remmina_file, "server");

	if (!server)
	{
		return g_strdup("");
	}

	remmina_public_get_server_port(server, default_port, &host, &port);

	if (port_plus && port < 100)
	{
		/* Protocols like VNC supports using instance number :0, :1, etc as port number. */
		port += default_port;
	}

#ifdef HAVE_LIBSSH
	if (!remmina_file_get_int (gp->priv->remmina_file, "ssh_enabled", FALSE))
	{
		dest = g_strdup_printf("[%s]:%i", host, port);
		g_free(host);
		return dest;
	}

	if (!remmina_protocol_widget_init_tunnel (gp))
	{
		g_free(host);
		return NULL;
	}

	THREADS_ENTER
	remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
			_("Connecting to %s through SSH tunnel..."), server);
	THREADS_LEAVE

	if (remmina_file_get_int (gp->priv->remmina_file, "ssh_loopback", FALSE))
	{
		g_free(host);
		host = g_strdup ("127.0.0.1");
	}

	if (!remmina_ssh_tunnel_open (gp->priv->ssh_tunnel, host, port, remmina_pref.sshtunnel_port))
	{
		g_free(host);
		remmina_protocol_widget_set_error (gp, REMMINA_SSH (gp->priv->ssh_tunnel)->error);
		return NULL;
	}

	g_free(host);
	return g_strdup_printf("127.0.0.1:%i", remmina_pref.sshtunnel_port);

#else

	dest = g_strdup_printf("[%s]:%i", host, port);
	g_free(host);
	return dest;

#endif
}

gboolean remmina_protocol_widget_start_reverse_tunnel(RemminaProtocolWidget* gp, gint local_port)
{
#ifdef HAVE_LIBSSH
	if (!remmina_file_get_int (gp->priv->remmina_file, "ssh_enabled", FALSE))
	{
		return TRUE;
	}

	if (!remmina_protocol_widget_init_tunnel (gp))
	{
		return FALSE;
	}

	THREADS_ENTER
	remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
			_("Waiting for an incoming SSH tunnel at port %i..."), remmina_file_get_int (gp->priv->remmina_file, "listenport", 0));
	THREADS_LEAVE

	if (!remmina_ssh_tunnel_reverse (gp->priv->ssh_tunnel, remmina_file_get_int (gp->priv->remmina_file, "listenport", 0), local_port))
	{
		remmina_protocol_widget_set_error (gp, REMMINA_SSH (gp->priv->ssh_tunnel)->error);
		return FALSE;
	}
#endif

	return TRUE;
}

gboolean remmina_protocol_widget_ssh_exec(RemminaProtocolWidget* gp, gboolean wait, const gchar *fmt, ...)
{
#ifdef HAVE_LIBSSH
		RemminaSSHTunnel *tunnel = gp->priv->ssh_tunnel;
		ssh_channel channel;
		gint status;
		gboolean ret = FALSE;
		gchar *cmd, *ptr;
		va_list args;

		if ((channel = channel_new (REMMINA_SSH (tunnel)->session)) == NULL)
		{
			return FALSE;
		}

		va_start (args, fmt);
		cmd = g_strdup_vprintf (fmt, args);
		va_end (args);

		if (channel_open_session (channel) == SSH_OK &&
				channel_request_exec (channel, cmd) == SSH_OK)
		{
			if (wait)
			{
				channel_send_eof (channel);
				status = channel_get_exit_status (channel);
				ptr = strchr (cmd, ' ');
				if (ptr) *ptr = '\0';
				switch (status)
				{
					case 0:
					ret = TRUE;
					break;
					case 127:
					remmina_ssh_set_application_error (REMMINA_SSH (tunnel),
							_("Command %s not found on SSH server"), cmd);
					break;
					default:
					remmina_ssh_set_application_error (REMMINA_SSH (tunnel),
							_("Command %s failed on SSH server (status = %i)."), cmd,status);
                break;
            }
        }
        else
        {
            ret = TRUE;
        }
    }
    else
    {
        remmina_ssh_set_error (REMMINA_SSH (tunnel), _("Failed to execute command: %s"));
    }
    g_free(cmd);
    if (wait) channel_close (channel);
    channel_free (channel);
    return ret;

#else

    return FALSE;

#endif
}

#ifdef HAVE_LIBSSH
static gboolean remmina_protocol_widget_tunnel_init_callback (RemminaSSHTunnel *tunnel, gpointer data)
{
	RemminaProtocolWidget* gp = REMMINA_PROTOCOL_WIDGET (data);
	gchar *server;
	gint port;
	gboolean ret;

	remmina_public_get_server_port (remmina_file_get_string (gp->priv->remmina_file, "server"), 177, &server, &port);
	ret = ((RemminaXPortTunnelInitFunc) gp->priv->init_func) (gp,
			tunnel->remotedisplay, (tunnel->bindlocalhost ? "localhost" : server), port);
	g_free(server);

	return ret;
}

static gboolean remmina_protocol_widget_tunnel_connect_callback(RemminaSSHTunnel* tunnel, gpointer data)
{
	return TRUE;
}

static gboolean remmina_protocol_widget_tunnel_disconnect_callback(RemminaSSHTunnel* tunnel, gpointer data)
{
	RemminaProtocolWidget* gp = REMMINA_PROTOCOL_WIDGET (data);

	if (REMMINA_SSH (tunnel)->error)
	{
		remmina_protocol_widget_set_error (gp, "%s", REMMINA_SSH (tunnel)->error);
	}

	IDLE_ADD ((GSourceFunc) remmina_protocol_widget_close_connection, gp);
	return TRUE;
}
#endif

gboolean remmina_protocol_widget_start_xport_tunnel(RemminaProtocolWidget* gp, RemminaXPortTunnelInitFunc init_func)
{
#ifdef HAVE_LIBSSH
	gboolean bindlocalhost;
	gchar *server;

	if (!remmina_protocol_widget_init_tunnel (gp)) return FALSE;

	THREADS_ENTER
	remmina_init_dialog_set_status (REMMINA_INIT_DIALOG (gp->priv->init_dialog),
			_("Connecting to %s through SSH tunnel..."), remmina_file_get_string (gp->priv->remmina_file, "server"));
	THREADS_LEAVE

	gp->priv->init_func = init_func;
	gp->priv->ssh_tunnel->init_func = remmina_protocol_widget_tunnel_init_callback;
	gp->priv->ssh_tunnel->connect_func = remmina_protocol_widget_tunnel_connect_callback;
	gp->priv->ssh_tunnel->disconnect_func = remmina_protocol_widget_tunnel_disconnect_callback;
	gp->priv->ssh_tunnel->callback_data = gp;

	remmina_public_get_server_port (remmina_file_get_string (gp->priv->remmina_file, "server"), 0, &server, NULL);
	bindlocalhost = (g_strcmp0(REMMINA_SSH (gp->priv->ssh_tunnel)->server, server) == 0);
	g_free(server);

	if (!remmina_ssh_tunnel_xport (gp->priv->ssh_tunnel, bindlocalhost))
	{
		remmina_protocol_widget_set_error (gp, "Failed to open channel : %s",
				ssh_get_error (REMMINA_SSH (gp->priv->ssh_tunnel)->session));
		return FALSE;
	}

	return TRUE;

#else
	return FALSE;
#endif
}

void remmina_protocol_widget_set_display(RemminaProtocolWidget* gp, gint display)
{
#ifdef HAVE_LIBSSH
	if (gp->priv->ssh_tunnel->localdisplay) g_free(gp->priv->ssh_tunnel->localdisplay);
	gp->priv->ssh_tunnel->localdisplay = g_strdup_printf("unix:%i", display);
#endif
}

GtkWidget* remmina_protocol_widget_get_init_dialog(RemminaProtocolWidget* gp)
{
	return gp->priv->init_dialog;
}

gint remmina_protocol_widget_get_width(RemminaProtocolWidget* gp)
{
	return gp->priv->width;
}

void remmina_protocol_widget_set_width(RemminaProtocolWidget* gp, gint width)
{
	gp->priv->width = width;
}

gint remmina_protocol_widget_get_height(RemminaProtocolWidget* gp)
{
	return gp->priv->height;
}

void remmina_protocol_widget_set_height(RemminaProtocolWidget* gp, gint height)
{
	gp->priv->height = height;
}

gboolean remmina_protocol_widget_get_scale(RemminaProtocolWidget* gp)
{
	return gp->priv->scale;
}

void remmina_protocol_widget_set_scale(RemminaProtocolWidget* gp, gboolean scale)
{
	gp->priv->scale = scale;
}

gboolean remmina_protocol_widget_get_expand(RemminaProtocolWidget* gp)
{
	return gp->priv->expand;
}

void remmina_protocol_widget_set_expand(RemminaProtocolWidget* gp, gboolean expand)
{
	gp->priv->expand = expand;
}

gboolean remmina_protocol_widget_has_error(RemminaProtocolWidget* gp)
{
	return gp->priv->has_error;
}

gchar* remmina_protocol_widget_get_error_message(RemminaProtocolWidget* gp)
{
	return gp->priv->error_message;
}

void remmina_protocol_widget_set_error(RemminaProtocolWidget* gp, const gchar *fmt, ...)
{
	va_list args;

	if (gp->priv->error_message) g_free(gp->priv->error_message);

	va_start (args, fmt);
	gp->priv->error_message = g_strdup_vprintf (fmt, args);
	va_end (args);

	gp->priv->has_error = TRUE;
}

gboolean remmina_protocol_widget_is_closed(RemminaProtocolWidget* gp)
{
	return gp->priv->closed;
}

RemminaFile* remmina_protocol_widget_get_file(RemminaProtocolWidget* gp)
{
	return gp->priv->remmina_file;
}

gint remmina_protocol_widget_init_authpwd(RemminaProtocolWidget* gp, RemminaAuthpwdType authpwd_type)
{
	RemminaFile* remminafile = gp->priv->remmina_file;
	gchar* s;
	gint ret;

	switch (authpwd_type)
	{
		case REMMINA_AUTHPWD_TYPE_PROTOCOL:
			s = g_strdup_printf(_("%s password"), remmina_file_get_string(remminafile, "protocol"));
			break;
		case REMMINA_AUTHPWD_TYPE_SSH_PWD:
			s = g_strdup(_("SSH password"));
			break;
		case REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY:
			s = g_strdup(_("SSH private key passphrase"));
			break;
		default:
			s = g_strdup(_("Password"));
			break;
	}
	ret = remmina_init_dialog_authpwd(
			REMMINA_INIT_DIALOG(gp->priv->init_dialog),
			s,
			remmina_file_get_filename(remminafile) != NULL && authpwd_type != REMMINA_AUTHPWD_TYPE_SSH_PWD
					&& authpwd_type != REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY);
	g_free(s);

	return ret;
}

gint remmina_protocol_widget_init_authuserpwd(RemminaProtocolWidget* gp, gboolean want_domain)
{
	RemminaFile* remminafile = gp->priv->remmina_file;

	return remmina_init_dialog_authuserpwd(REMMINA_INIT_DIALOG(gp->priv->init_dialog), want_domain,
			remmina_file_get_string(remminafile, "username"),
			want_domain ? remmina_file_get_string(remminafile, "domain") : NULL,
			(remmina_file_get_filename(remminafile) != NULL));
}

gint remmina_protocol_widget_init_certificate(RemminaProtocolWidget* gp, const gchar* subject, const gchar* issuer, const gchar* fingerprint)
{
	return remmina_init_dialog_certificate(REMMINA_INIT_DIALOG(gp->priv->init_dialog), subject, issuer, fingerprint);
}
gint remmina_protocol_widget_changed_certificate(RemminaProtocolWidget *gp, const gchar* subject, const gchar* issuer, const gchar* new_fingerprint, const gchar* old_fingerprint)
{
	return remmina_init_dialog_certificate_changed(REMMINA_INIT_DIALOG(gp->priv->init_dialog), subject, issuer, new_fingerprint, old_fingerprint);
}

gchar* remmina_protocol_widget_init_get_username(RemminaProtocolWidget* gp)
{
	return g_strdup(REMMINA_INIT_DIALOG(gp->priv->init_dialog)->username);
}

gchar* remmina_protocol_widget_init_get_password(RemminaProtocolWidget* gp)
{
	return g_strdup(REMMINA_INIT_DIALOG(gp->priv->init_dialog)->password);
}

gchar* remmina_protocol_widget_init_get_domain(RemminaProtocolWidget* gp)
{
	return g_strdup(REMMINA_INIT_DIALOG(gp->priv->init_dialog)->domain);
}

gint remmina_protocol_widget_init_authx509(RemminaProtocolWidget* gp)
{
	RemminaFile* remminafile = gp->priv->remmina_file;

	return remmina_init_dialog_authx509(REMMINA_INIT_DIALOG(gp->priv->init_dialog),
			remmina_file_get_string(remminafile, "cacert"), remmina_file_get_string(remminafile, "cacrl"),
			remmina_file_get_string(remminafile, "clientcert"), remmina_file_get_string(remminafile, "clientkey"));
}

gchar* remmina_protocol_widget_init_get_cacert(RemminaProtocolWidget* gp)
{
	gchar* s;

	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->cacert;
	return (s && s[0] ? g_strdup(s) : NULL);
}

gchar* remmina_protocol_widget_init_get_cacrl(RemminaProtocolWidget* gp)
{
	gchar* s;

	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->cacrl;
	return (s && s[0] ? g_strdup(s) : NULL);
}

gchar* remmina_protocol_widget_init_get_clientcert(RemminaProtocolWidget* gp)
{
	gchar* s;

	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->clientcert;
	return (s && s[0] ? g_strdup(s) : NULL);
}

gchar* remmina_protocol_widget_init_get_clientkey(RemminaProtocolWidget* gp)
{
	gchar* s;

	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->clientkey;
	return (s && s[0] ? g_strdup(s) : NULL);
}

void remmina_protocol_widget_init_save_cred(RemminaProtocolWidget* gp)
{
	RemminaFile* remminafile = gp->priv->remmina_file;
	gchar* s;
	gboolean save = FALSE;

	/* Save user name and certificates if any; save the password if it's requested to do so */
	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->username;
	if (s && s[0])
	{
		remmina_file_set_string(remminafile, "username", s);
		save = TRUE;
	}
	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->cacert;
	if (s && s[0])
	{
		remmina_file_set_string(remminafile, "cacert", s);
		save = TRUE;
	}
	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->cacrl;
	if (s && s[0])
	{
		remmina_file_set_string(remminafile, "cacrl", s);
		save = TRUE;
	}
	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->clientcert;
	if (s && s[0])
	{
		remmina_file_set_string(remminafile, "clientcert", s);
		save = TRUE;
	}
	s = REMMINA_INIT_DIALOG(gp->priv->init_dialog)->clientkey;
	if (s && s[0])
	{
		remmina_file_set_string(remminafile, "clientkey", s);
		save = TRUE;
	}
	if (REMMINA_INIT_DIALOG(gp->priv->init_dialog)->save_password)
	{
		remmina_file_set_string(remminafile, "password", REMMINA_INIT_DIALOG(gp->priv->init_dialog)->password);
		save = TRUE;
	}
	if (save)
	{
		remmina_file_save_group(remminafile, REMMINA_SETTING_GROUP_CREDENTIAL);
	}
}

void remmina_protocol_widget_init_show_listen(RemminaProtocolWidget* gp, gint port)
{
	remmina_init_dialog_set_status(REMMINA_INIT_DIALOG(gp->priv->init_dialog),
			_("Listening on port %i for an incoming %s connection..."), port,
			remmina_file_get_string(gp->priv->remmina_file, "protocol"));
}

void remmina_protocol_widget_init_show_retry(RemminaProtocolWidget* gp)
{
	remmina_init_dialog_set_status_temp(REMMINA_INIT_DIALOG(gp->priv->init_dialog),
			_("Authentication failed. Trying to reconnect..."));
}

void remmina_protocol_widget_init_show(RemminaProtocolWidget* gp)
{
	gtk_widget_show(gp->priv->init_dialog);
}

void remmina_protocol_widget_init_hide(RemminaProtocolWidget* gp)
{
	gtk_widget_hide(gp->priv->init_dialog);
}

static void remmina_protocol_widget_chat_on_destroy(RemminaProtocolWidget* gp)
{
	gp->priv->chat_window = NULL;
}

void remmina_protocol_widget_chat_open(RemminaProtocolWidget* gp, const gchar *name,
		void(*on_send)(RemminaProtocolWidget* gp, const gchar *text), void(*on_destroy)(RemminaProtocolWidget* gp))
{
	if (gp->priv->chat_window)
	{
		gtk_window_present(GTK_WINDOW(gp->priv->chat_window));
	}
	else
	{
		gp->priv->chat_window = remmina_chat_window_new(GTK_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(gp))), name);
		g_signal_connect_swapped(G_OBJECT(gp->priv->chat_window), "send", G_CALLBACK(on_send), gp);
		g_signal_connect_swapped(G_OBJECT(gp->priv->chat_window), "destroy",
				G_CALLBACK(remmina_protocol_widget_chat_on_destroy), gp);
		g_signal_connect_swapped(G_OBJECT(gp->priv->chat_window), "destroy", G_CALLBACK(on_destroy), gp);
		gtk_widget_show(gp->priv->chat_window);
	}
}

void remmina_protocol_widget_chat_close(RemminaProtocolWidget* gp)
{
	if (gp->priv->chat_window)
	{
		gtk_widget_destroy(gp->priv->chat_window);
	}
}

void remmina_protocol_widget_chat_receive(RemminaProtocolWidget* gp, const gchar* text)
{
	if (gp->priv->chat_window)
	{
		remmina_chat_window_receive(REMMINA_CHAT_WINDOW(gp->priv->chat_window), _("Server"), text);
		gtk_window_present(GTK_WINDOW(gp->priv->chat_window));
	}
}

GtkWidget* remmina_protocol_widget_new(void)
{
	return GTK_WIDGET(g_object_new(REMMINA_TYPE_PROTOCOL_WIDGET, NULL));
}
