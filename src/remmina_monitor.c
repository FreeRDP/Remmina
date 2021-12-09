/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2021 Antenore Gatta
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
#include "remmina_monitor.h"
#include "remmina_log.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

RemminaMonitor *rm_monitor;

static void remmina_monitor_can_reach_cb (GNetworkMonitor *netmonitor, GAsyncResult *result, RemminaMonitor *monitor)
{
	g_autoptr (GError) error = NULL;

	gchar *status = NULL;

	gboolean is_reachable = g_network_monitor_can_reach_finish (netmonitor, result, &error);

	const gchar *addr_tostr = g_strdup(g_socket_connectable_to_string (monitor->addr));
	//gchar *value = (gchar *)g_hash_table_lookup (monitor->server_status, addr_tostr);

	if (is_reachable) {

		REMMINA_DEBUG ("Network object %s is reachable", g_socket_connectable_to_string (monitor->addr));
		status = g_strdup ("online");

	} else {

		REMMINA_DEBUG ("Network object %s is not reachable", g_socket_connectable_to_string (monitor->addr));
		status = g_strdup ("offline");
	}

	if (g_hash_table_replace (monitor->server_status, g_strdup(addr_tostr), g_strdup(status))) {
		REMMINA_DEBUG ("Inserting %s -> %s", addr_tostr, status);
	} else {
		REMMINA_DEBUG ("Replacing %s -> %s", addr_tostr, status);
	}

	/* Cannot use remminafile here because is freed by remmina_file_manager_iterate */
	//if (remminafile)
		//remmina_file_set_state_int (remminafile, "reachable", reachable);
	g_free (status);
}

gchar *remmina_monitor_can_reach(RemminaFile *remminafile, RemminaMonitor *monitor)
{
	TRACE_CALL(__func__);

	const gchar *server;
	const gchar *ssh_tunnel_server;
	const gchar *addr_tostr;
	gchar *status = NULL;
	gchar *ssh_tunnel_host, *srv_host;
	gint netmonit, srv_port, ssh_tunnel_port;
	const gchar *protocol;
	gint default_port = 0;


	if (!remminafile) {
		status = g_strdup ("I/O Error");
		REMMINA_DEBUG (status);
		return NULL;
	}

	netmonit = remmina_file_get_int(remminafile, "enable-netmonit", FALSE);

	if (!netmonit) {
		status = g_strdup ("Monitoring disabled");
		REMMINA_DEBUG (status);
		return NULL;
	}

	protocol = remmina_file_get_string (remminafile, "protocol");

	if (protocol && protocol[0] != '\0') {
		REMMINA_DEBUG ("Evaluating protocol %s for monitoring", protocol);
		if (g_strcmp0("RDP", protocol) == 0)
			default_port = 3389;
		if (g_strcmp0("VNC", protocol) == 0)
			default_port = 5900;
		if (g_strcmp0("GVNC", protocol) == 0)
			default_port = 5900;
		if (g_strcmp0("SPICE", protocol) == 0)
			default_port = 5900;
		if (g_strcmp0("WWW", protocol) == 0)
			default_port = 443;
		if (g_strcmp0("X2GO", protocol) == 0)
			default_port = 22;
		if (g_strcmp0("SSH", protocol) == 0)
			default_port = 22;
		if (g_strcmp0("SFTP", protocol) == 0)
			default_port = 22;
		if (g_strcmp0("EXEC", protocol) == 0)
			default_port = -1;

		if (default_port == 0) {
			status = g_strdup ("Unknown protocol");
			REMMINA_DEBUG (status);
			return NULL;
		}
		if (default_port < 0) {
			status = g_strdup ("Cannot monitor");
			REMMINA_DEBUG (status);
			return NULL;
		}

		ssh_tunnel_server = remmina_file_get_string(remminafile, "ssh_tunnel_server");
		if (remmina_file_get_int(remminafile, "ssh_tunnel_enabled", FALSE)) {
			remmina_public_get_server_port(ssh_tunnel_server, 22, &ssh_tunnel_host, &ssh_tunnel_port);
			monitor->addr = g_network_address_new (ssh_tunnel_host, ssh_tunnel_port);
			g_free(ssh_tunnel_host), ssh_tunnel_host = NULL;
		} else {
			server = remmina_file_get_string(remminafile, "server");
			remmina_public_get_server_port(server, default_port, &srv_host, &srv_port);
			monitor->addr = g_network_address_new (srv_host, srv_port);
			g_free(srv_host), srv_host = NULL;
		}
		addr_tostr = g_strdup(g_socket_connectable_to_string (monitor->addr));

		REMMINA_DEBUG ("addr is %s", addr_tostr);
		if (monitor->connected && netmonit) {
			REMMINA_DEBUG ("Testing for %s", addr_tostr);
			g_network_monitor_can_reach_async (
					monitor->netmonitor,
					monitor->addr,
					NULL,
					(GAsyncReadyCallback) remmina_monitor_can_reach_cb,
					monitor);
		}


		status = (gchar *)g_hash_table_lookup (monitor->server_status, addr_tostr);
		//if (!status)
			//g_hash_table_insert (monitor->server_status, g_strdup(addr_tostr), "offline");

	}

	if (!status) {
		return g_strdup(addr_tostr);
	} else
		return status;

	//g_free(ssh_tunnel_host), ssh_tunnel_host = NULL;
	//g_free(srv_host), srv_host = NULL;
	//g_free(dest), dest = NULL;

}

gboolean remmina_network_monitor_status (RemminaMonitor *rm_monitor)
{
	TRACE_CALL(__func__);

	gboolean status = g_network_monitor_get_connectivity (rm_monitor->netmonitor);

	rm_monitor->server_status = g_hash_table_new_full(
			g_str_hash,
			g_str_equal,
			(GDestroyNotify)g_free,
			(GDestroyNotify)g_free);

	switch (status)
	{
		case G_NETWORK_CONNECTIVITY_LOCAL:
			REMMINA_DEBUG ("G_NETWORK_CONNECTIVITY_LOCAL");
			rm_monitor->connected = FALSE;
			break;

		case G_NETWORK_CONNECTIVITY_LIMITED:
			REMMINA_DEBUG ("G_NETWORK_CONNECTIVITY_LIMITED");
			rm_monitor->connected = FALSE;
			break;

		case G_NETWORK_CONNECTIVITY_PORTAL:
			REMMINA_DEBUG ("G_NETWORK_CONNECTIVITY_PORTAL");
			rm_monitor->connected = FALSE;
			break;

		case G_NETWORK_CONNECTIVITY_FULL:
			REMMINA_DEBUG ("G_NETWORK_CONNECTIVITY_FULL");
			rm_monitor->connected = TRUE;
			break;
	}

	return status;
}


RemminaMonitor *remmina_network_monitor_new ()
{
	TRACE_CALL(__func__);

	rm_monitor = g_new0(RemminaMonitor, 1);

	rm_monitor->netmonitor = g_network_monitor_get_default ();

	return rm_monitor;
}
