/*  See LICENSE and COPYING files for copyright and license details. */

#include "common/remmina_plugin.h"
#include "telepathy_handler.h"

RemminaPluginService *remmina_plugin_telepathy_service = NULL;

static RemminaTpHandler *remmina_tp_handler = NULL;

void remmina_plugin_telepathy_entry(void)
{
	TRACE_CALL("remmina_plugin_telepathy_entry");
	if (remmina_tp_handler == NULL)
	{
		remmina_tp_handler = remmina_tp_handler_new();
	}
}

/* Entry plugin definition and features */
static RemminaEntryPlugin remmina_plugin_telepathy =
{
	REMMINA_PLUGIN_TYPE_ENTRY,                    // Type
	"telepathy",                                  // Name
	N_("Telepathy - Desktop Sharing"),            // Description
	GETTEXT_PACKAGE,                              // Translation domain
	VERSION,                                      // Version number
	remmina_plugin_telepathy_entry                // Plugin entry function
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL("remmina_plugin_entry");
	remmina_plugin_telepathy_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_telepathy))
	{
		return FALSE;
	}
	return TRUE;
}

