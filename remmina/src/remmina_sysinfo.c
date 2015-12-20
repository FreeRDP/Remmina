/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remmina/remmina_trace_calls.h"
#include "remmina_sysinfo.h"

gboolean remmina_sysinfo_is_appindicator_available()
{
    /* Check if we have an appindicator available (which uses
     * DBUS KDE StatusNotifier)
     */

	TRACE_CALL("remmina_sysinfo_is_appindicator_available");
    GDBusConnection *con;
    GVariant *v;
    GError *error;
    gboolean available;

	available = FALSE;
    con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (con) {
		error = NULL;
		v = g_dbus_connection_call_sync(con,
			"org.kde.StatusNotifierWatcher",
			"/StatusNotifierWatcher",
			"org.freedesktop.DBus.Introspectable",
			"Introspect",
			NULL,
			NULL,
			G_DBUS_CALL_FLAGS_NONE,
			-1,
			NULL,
			&error);
		if (v) {
			available = TRUE;
			g_variant_unref(v);
		}
		g_object_unref(con);
	}
    return available;
}

gchar *remmina_sysinfo_get_gnome_shell_version()
{
    /* Query DBUS to get gnome shell version. Return NULL
     * if error or no gnome shell found.
     * The returned string must be freed with g_free */

	TRACE_CALL("remmina_sysinfo_get_gnome_shell_version");
    GDBusConnection *con;
    GDBusProxy *p;
    GVariant *v;
    GError *error;
    gsize sz;
    gchar *ret;

    ret = NULL;

    con = g_bus_get_sync(G_BUS_TYPE_SESSION, NULL, NULL);
    if (con) {
		error = NULL;
		p = g_dbus_proxy_new_sync(con,
			G_DBUS_PROXY_FLAGS_DO_NOT_CONNECT_SIGNALS | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START,
			NULL,
			"org.gnome.Shell",
			"/org/gnome/Shell",
			"org.gnome.Shell",
			NULL,
			&error);
		if (p) {
			v = g_dbus_proxy_get_cached_property(p, "ShellVersion");
			if (v) {
				if (g_variant_is_of_type(v, G_VARIANT_TYPE_STRING)) {
					ret = g_strdup(g_variant_get_string(v, &sz));
				}
				g_variant_unref(v);
			}
			g_object_unref(p);
		}
		g_object_unref(con);
	}
    return ret;
}

