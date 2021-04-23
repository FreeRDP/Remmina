/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

#include "plugin_config.h"

#include "common/remmina_plugin.h"

#include <gtk/gtkx.h>
#include <gdk/gdkx.h>

static RemminaPluginService *remmina_plugin_service = NULL;
#define REMMINA_PLUGIN_DEBUG(fmt, ...) remmina_plugin_service->_remmina_debug(__func__, fmt, ##__VA_ARGS__)

static void remmina_plugin_tool_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("[%s] Plugin init", PLUGIN_NAME);
}

static gboolean remmina_plugin_tool_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("[%s] Plugin open connection", PLUGIN_NAME);

	GtkDialog *dialog;
	dialog = GTK_DIALOG(gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL | GTK_DIALOG_USE_HEADER_BAR,
			GTK_MESSAGE_INFO, GTK_BUTTONS_OK, PLUGIN_DESCRIPTION));
	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));
	return FALSE;
}

static gboolean remmina_plugin_tool_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	REMMINA_PLUGIN_DEBUG("[%s] Plugin close connection", PLUGIN_NAME);
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
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_tool_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin = {
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   // Type
	PLUGIN_NAME,                                    // Name
	PLUGIN_DESCRIPTION,                             // Description
	GETTEXT_PACKAGE,                                // Translation domain
	PLUGIN_VERSION,                                 // Version number
	PLUGIN_APPICON,                                 // Icon for normal connection
	PLUGIN_APPICON,                                 // Icon for SSH connection
	remmina_plugin_tool_basic_settings,             // Array for basic settings
	NULL,                                           // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_NONE,              // SSH settings type
	NULL,                                           // Array for available features
	remmina_plugin_tool_init,                       // Plugin initialization
	remmina_plugin_tool_open_connection,            // Plugin open connection
	remmina_plugin_tool_close_connection,           // Plugin close connection
	NULL,                                           // Query for available features
	NULL,                                           // Call a feature
	NULL,                                           // Send a keystroke
	NULL,                                           // No screenshot support available
	NULL,                                           // RCW map event
	NULL                                            // RCW unmap event
};

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin*)&remmina_plugin)) {
		return FALSE;
	}

	return TRUE;
}
