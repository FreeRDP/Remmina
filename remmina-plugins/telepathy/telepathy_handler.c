/*  See LICENSE and COPYING files for copyright and license details. */

#include "common/remmina_plugin.h"
#include <telepathy-glib/dbus.h>
#include <telepathy-glib/defs.h>
#include <telepathy-glib/svc-client.h>
#include "telepathy_channel_handler.h"
#include "telepathy_handler.h"

extern RemminaPluginService *remmina_plugin_telepathy_service;

#define REMMINA_TP_BUS_NAME TP_CLIENT_BUS_NAME_BASE "Remmina"
#define REMMINA_TP_OBJECT_PATH TP_CLIENT_OBJECT_PATH_BASE "Remmina"

static void remmina_tp_handler_iface_init(gpointer g_iface, gpointer iface_data);

G_DEFINE_TYPE_WITH_CODE (RemminaTpHandler, remmina_tp_handler, G_TYPE_OBJECT,
		G_IMPLEMENT_INTERFACE (TP_TYPE_SVC_CLIENT, NULL);
		G_IMPLEMENT_INTERFACE( TP_TYPE_SVC_CLIENT_HANDLER, remmina_tp_handler_iface_init);
		);

		static void remmina_tp_handler_class_init(RemminaTpHandlerClass *klass)
		{
			TRACE_CALL("remmina_tp_handler_class_init");
		}

		static void remmina_tp_handler_init(RemminaTpHandler *handler)
		{
			TRACE_CALL("remmina_tp_handler_init");
		}

		static void remmina_tp_handler_handle_channels(TpSvcClientHandler *handler, const char *account_path,
				const char *connection_path, const GPtrArray *channels, const GPtrArray *requests_satisfied,
				guint64 user_action_time, GHashTable *handler_info, DBusGMethodInvocation *context)
		{
			TRACE_CALL("remmina_tp_handler_handle_channels");
			gint i;
			GValueArray *array;

			for (i = 0; i < channels->len; i++)
			{
				array = g_ptr_array_index(channels, i);
				remmina_tp_channel_handler_new(account_path, connection_path,
						(const gchar *) g_value_get_boxed(g_value_array_get_nth(array, 0)),
						(GHashTable *) g_value_get_boxed(g_value_array_get_nth(array, 1)), context);
			}
		}

		static void remmina_tp_handler_iface_init(gpointer g_iface, gpointer iface_data)
		{
			TRACE_CALL("remmina_tp_handler_iface_init");
			TpSvcClientHandlerClass *klass = (TpSvcClientHandlerClass *) g_iface;

#define IMPLEMENT(x) tp_svc_client_handler_implement_##x (klass, remmina_tp_handler_##x)
			IMPLEMENT(handle_channels);
#undef IMPLEMENT
		}

		static gboolean remmina_tp_handler_register(RemminaTpHandler *handler)
		{
			TRACE_CALL("remmina_tp_handler_register");
			TpDBusDaemon *bus;
			GError *error = NULL;

			bus = tp_dbus_daemon_dup(&error);
			if (bus == NULL)
			{
				g_print("tp_dbus_daemon_dup: %s", error->message);
				return FALSE;
			}
			if (!tp_dbus_daemon_request_name (bus, REMMINA_TP_BUS_NAME, FALSE, &error))
			{
				g_object_unref(bus);
				g_print("tp_dbus_daemon_request_name: %s", error->message);
				return FALSE;
			} dbus_g_connection_register_G_OBJECT(
					tp_proxy_get_dbus_connection (TP_PROXY (bus)),
					REMMINA_TP_OBJECT_PATH, G_OBJECT(handler));
			g_object_unref(bus);
			g_print("remmina_tp_handler_register: bus_name " REMMINA_TP_BUS_NAME
					" object_path " REMMINA_TP_OBJECT_PATH "\n");
			return TRUE;
		}

		RemminaTpHandler*
		remmina_tp_handler_new(void)
		{
			TRACE_CALL("remmina_tp_handler_new");
			RemminaTpHandler *handler;

			handler = REMMINA_TP_HANDLER(g_object_new(REMMINA_TYPE_TP_HANDLER, NULL));
			remmina_tp_handler_register(handler);
			return handler;
		}

