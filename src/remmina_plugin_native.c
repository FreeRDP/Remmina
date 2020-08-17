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
