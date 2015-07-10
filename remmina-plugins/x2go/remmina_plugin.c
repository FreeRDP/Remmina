/*
 *     Project: Remmina Plugin X2Go
 * Description: Remmina protocol plugin to connect via X2Go using PyHoca
 *              Based on Fabio Castelli Team Viewer Plugin
 *              Copyright: 2013-2014 Fabio Castelli (Muflone)
 *      Author: Antenore Gatta <antenore@simbiosi.org>
 *   Copyright: 2015 Antenore Gatta
 *     License: GPL-2+
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

#include "plugin_config.h"
#include <common/remmina_plugin.h>
#if GTK_VERSION == 3
	# include <gtk/gtkx.h>
#endif

static RemminaPluginService *remmina_plugin_service = NULL;

static void remmina_plugin_init(RemminaProtocolWidget *gp)
{
	remmina_plugin_service->log_printf("[%s] remmina_plugin_init\n", PLUGIN_NAME);
}

static gboolean remmina_plugin_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_open_connection");
	remmina_plugin_service->log_printf("[%s] remmina_plugin_open_connection\n", PLUGIN_NAME);
	#define GET_PLUGIN_STRING(value) \
		g_strdup(remmina_plugin_service->file_get_string(remminafile, value))

	RemminaFile *remminafile;
	gboolean ret;
	GPid pid;
	GError *error = NULL;
	gchar *argv[50];
	gint argc;
	gint i;

	gchar *option_str;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	argc = 0;
	argv[argc++] = g_strdup("pyhoca-cli");
	argv[argc++] = g_strdup("-P");
	option_str = GET_PLUGIN_STRING("session");
	argv[argc++] = g_strdup(option_str);

	argv[argc++] = NULL;

	ret = g_spawn_async (NULL, argv, NULL, G_SPAWN_SEARCH_PATH,
		NULL, NULL, &pid, &error);

	for (i = 0; i < argc; i++)
	g_free (argv[i]);

	if (!ret)
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s", error->message);

	return FALSE;
}

static gboolean remmina_plugin_close_connection(RemminaProtocolWidget *gp)
{
	remmina_plugin_service->log_printf("[%s] remmina_plugin_close_connection\n", PLUGIN_NAME);
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");
	return FALSE;
}

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Unused pointer
 */
static const RemminaProtocolSetting remmina_plugin_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "session", N_("Session name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
	/* We will add also user, password, servername and command
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
	*/
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,              // Type
	PLUGIN_NAME,                               // Name
	PLUGIN_DESCRIPTION,                        // Description
	GETTEXT_PACKAGE,                           // Translation domain
	PLUGIN_VERSION,                            // Version number
	PLUGIN_APPICON,                            // Icon for normal connection
	PLUGIN_APPICON,                            // Icon for SSH connection
	remmina_plugin_basic_settings,             // Array for basic settings
	NULL,                                      // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,          // SSH settings type
	/* REMMINA_PROTOCOL_SSH_SETTING_NONE,         // SSH settings type */
	NULL,                                      // Array for available features
	remmina_plugin_init,                       // Plugin initialization
	remmina_plugin_open_connection,            // Plugin open connection
	remmina_plugin_close_connection,           // Plugin close connection
	NULL,                                      // Query for available features
	NULL,                                      // Call a feature
	/*  remmina_plugin_keystroke               // Send a keystroke    */
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL("remmina_plugin_entry");
	remmina_plugin_service = service;

	/*
	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	*/

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin))
	{
		return FALSE;
	}

	return TRUE;
}
