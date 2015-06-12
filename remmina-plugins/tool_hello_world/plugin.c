/*  See LICENSE and COPYING files for copyright and license details. */

#include "common/remmina_plugin.h"
#if GTK_VERSION == 3
#  include <gtk/gtkx.h>
#endif
#include "plugin.h"

static RemminaPluginService *remmina_plugin_service = NULL;

/* Protocol plugin definition and features */
static RemminaToolPlugin remmina_plugin_tool =
{
	REMMINA_PLUGIN_TYPE_TOOL,                     // Type
	"HELLO",                                      // Name
	"Hello world!",                               // Description
	GETTEXT_PACKAGE,                              // Translation domain
	VERSION,                                      // Version number
	remmina_plugin_tool_activate                  // Plugin activation callback
};

/* Tool plugin activation */
static void remmina_plugin_tool_activate(void)
{
	GtkDialog *dialog;
	dialog = GTK_DIALOG(gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
		GTK_MESSAGE_INFO, GTK_BUTTONS_OK, remmina_plugin_tool.description));
	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
}

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL("remmina_plugin_entry");
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_tool))
	{
		return FALSE;
	}

	return TRUE;
}
