/*
 * Remmina - The GTK+ Remote Desktop Client
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

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <string.h>

#include <gdk/gdkx.h>

#include "remmina_public.h"
#include "remmina_file_manager.h"
#include "remmina_pref.h"
#include "remmina_protocol_widget.h"
#include "remmina_log.h"
#include "remmina_widget_pool.h"
#include "rcw.h"
#include "remmina_public.h"
#include "remmina_plugin_native.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"

gboolean remmina_plugin_native_load(RemminaPluginService* service, const char* name) {
    TRACE_CALL(__func__);
	GModule *module;
	RemminaPluginEntryFunc entry;

	module = g_module_open(name, G_MODULE_BIND_LAZY | G_MODULE_BIND_LOCAL);

	if (!module) {
		g_print("Failed to load plugin: %s.\n", name);
		g_print("Error: %s\n", g_module_error());
		return FALSE;
	}

	if (!g_module_symbol(module, "remmina_plugin_entry", (gpointer*)&entry)) {
		g_print("Failed to locate plugin entry: %s.\n", name);
		return FALSE;
	}

	if (!entry(service)) {
		g_print("Plugin entry returned false: %s.\n", name);
		return FALSE;
	}

    return TRUE;
	/* We don’t close the module because we will need it throughout the process lifetime */
}
