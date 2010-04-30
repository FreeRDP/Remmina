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

#include "common/remminaplugincommon.h"
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/svc-client.h>

static RemminaPluginService *remmina_plugin_service = NULL;

/**** RemminaTpHandler - Implementing the Telepathy Handler ****/

#define REMMINA_TP_BUS_NAME TP_CLIENT_BUS_NAME_BASE "Remmina"
#define REMMINA_TP_OBJECT_PATH TP_CLIENT_OBJECT_PATH_BASE "Remmina"

#define REMMINA_TYPE_TP_HANDLER           (remmina_tp_handler_get_type ())
#define REMMINA_TP_HANDLER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_TP_HANDLER, RemminaTpHandler))
#define REMMINA_TP_HANDLER_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), REMMINA_TYPE_TP_HANDLER, RemminaTpHandlerClass))
#define REMMINA_IS_TP_HANDLER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_TP_HANDLER))
#define REMMINA_IS_TP_HANDLER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), REMMINA_TYPE_TP_HANDLER))
#define REMMINA_TP_HANDLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_TP_HANDLER, RemminaTpHandlerClass))

typedef struct _RemminaTpHandler
{
    GObject parent;

    TpDBusDaemon *bus;
} RemminaTpHandler;

typedef struct _RemminaTpHandlerClass
{
    GObjectClass parent_class;
} RemminaTpHandlerClass;

static void remmina_tp_handler_iface_init (gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (RemminaTpHandler, remmina_tp_handler, G_TYPE_OBJECT,
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT, NULL);
    G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT_HANDLER, remmina_tp_handler_iface_init);
    );

static void
remmina_tp_handler_class_init (RemminaTpHandlerClass *klass)
{
}

static void
remmina_tp_handler_init (RemminaTpHandler *handler)
{
}

static void
remmina_tp_handler_on_response (GtkDialog *dialog, gint response_id, gpointer data)
{
    if (response_id == GTK_RESPONSE_YES)
    {
        g_print ("Telepathy accepted\n");
    }
}

static void
remmina_tp_handler_handle_channels (
    TpSvcClientHandler    *handler,
    const char            *account_path,
    const char            *connection_path,
    const GPtrArray       *channels,
    const GPtrArray       *requests_satisfied,
    guint64                user_action_time,
    GHashTable            *handler_info,
    DBusGMethodInvocation *context)
{
    g_print ("handle_channels\n");
    remmina_plugin_service->ui_confirm (REMMINA_UI_CONFIRM_TYPE_SHARE_DESKTOP, NULL, "me",
        G_CALLBACK (remmina_tp_handler_on_response), NULL);
}

static void
remmina_tp_handler_iface_init (gpointer g_iface, gpointer iface_data)
{
    TpSvcClientHandlerClass *klass = (TpSvcClientHandlerClass *) g_iface;

#define IMPLEMENT(x) tp_svc_client_handler_implement_##x (klass, remmina_tp_handler_##x)
    IMPLEMENT (handle_channels);
#undef IMPLEMENT
}

static gboolean
remmina_tp_handler_register (RemminaTpHandler *handler)
{
    GError *error = NULL;

    handler->bus = tp_dbus_daemon_dup (&error);
	if (handler->bus == NULL)
	{
		g_print ("tp_dbus_daemon_dup: %s", error->message);
        return FALSE;
	}
    if (!tp_dbus_daemon_request_name (handler->bus, REMMINA_TP_BUS_NAME, FALSE, &error))
    {
		g_print ("tp_dbus_daemon_request_name: %s", error->message);
        return FALSE;
    }
    dbus_g_connection_register_g_object (
        tp_proxy_get_dbus_connection (TP_PROXY (handler->bus)),
        REMMINA_TP_OBJECT_PATH, G_OBJECT (handler));
    g_print("remmina_tp_handler_register: bus_name " REMMINA_TP_BUS_NAME
        " object_path " REMMINA_TP_OBJECT_PATH "\n");
    return TRUE;
}

static RemminaTpHandler*
remmina_tp_handler_new (void)
{
    RemminaTpHandler *handler;

    handler = REMMINA_TP_HANDLER (g_object_new (REMMINA_TYPE_TP_HANDLER, NULL));
    remmina_tp_handler_register (handler);
    return handler;
}

/**** RemminaTpHandler - End ****/

static RemminaTpHandler *remmina_tp_handler = NULL;

void
remmina_plugin_telepathy_entry (void)
{
    g_print ("Telepathy entry\n");
    if (remmina_tp_handler == NULL)
    {
        remmina_tp_handler = remmina_tp_handler_new ();
    }
}

static RemminaEntryPlugin remmina_plugin_telepathy =
{
    REMMINA_PLUGIN_TYPE_ENTRY,
    "telepathy",
    "Telepathy",

    remmina_plugin_telepathy_entry
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry (RemminaPluginService *service)
{
    remmina_plugin_service = service;

    if (! service->register_plugin ((RemminaPlugin *) &remmina_plugin_telepathy))
    {
        return FALSE;
    }
    return TRUE;
}


