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

#include "common/remmina_plugin.h"
#include "telepathy_handler.h"

RemminaPluginService *remmina_plugin_telepathy_service = NULL;

static RemminaTpHandler *remmina_tp_handler = NULL;

void remmina_plugin_telepathy_entry(void)
{
	if (remmina_tp_handler == NULL)
	{
		remmina_tp_handler = remmina_tp_handler_new();
	}
}

static RemminaEntryPlugin remmina_plugin_telepathy =
{ REMMINA_PLUGIN_TYPE_ENTRY, "telepathy", N_("Telepathy - Desktop Sharing"), GETTEXT_PACKAGE, VERSION,

remmina_plugin_telepathy_entry };

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	remmina_plugin_telepathy_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_telepathy))
	{
		return FALSE;
	}
	return TRUE;
}

