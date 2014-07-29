/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2011 Vic Lee
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

#include "config.h"
#include <gtk/gtk.h>
#include <gnome-keyring.h>
#include <remmina/plugin.h>

static RemminaPluginService *remmina_plugin_service = NULL;

static GnomeKeyringPasswordSchema remmina_file_secret_schema =
{ GNOME_KEYRING_ITEM_GENERIC_SECRET,
{
{ "filename", GNOME_KEYRING_ATTRIBUTE_TYPE_STRING },
{ "key", GNOME_KEYRING_ATTRIBUTE_TYPE_STRING },
{ NULL, 0 } } };

void remmina_plugin_gkeyring_store_password(RemminaFile *remminafile, const gchar *key, const gchar *password)
{
	GnomeKeyringResult r;
	const gchar *path;
	gchar *s;

	path = remmina_plugin_service->file_get_path(remminafile);
	s = g_strdup_printf("Remmina: %s - %s", remmina_plugin_service->file_get_string(remminafile, "name"), key);
	r = gnome_keyring_store_password_sync(&remmina_file_secret_schema, GNOME_KEYRING_DEFAULT, s, password, "filename", path,
			"key", key, NULL);
	g_free(s);
	if (r == GNOME_KEYRING_RESULT_OK)
	{
		remmina_plugin_service->log_printf("[GKEYRING] password saved for file %s\n", path);
	}
	else
	{
		remmina_plugin_service->log_printf("[GKEYRING] password cannot be saved for file %s: %s\n", path,
				gnome_keyring_result_to_message(r));
	}
}

gchar*
remmina_plugin_gkeyring_get_password(RemminaFile *remminafile, const gchar *key)
{
	GnomeKeyringResult r;
	const gchar *path;
	gchar *password;
	gchar *p;

	path = remmina_plugin_service->file_get_path(remminafile);
	r = gnome_keyring_find_password_sync(&remmina_file_secret_schema, &password, "filename", path, "key", key, NULL);
	if (r == GNOME_KEYRING_RESULT_OK)
	{
		remmina_plugin_service->log_printf("[GKEYRING] found password for file %s\n", path);
		p = g_strdup(password);
		gnome_keyring_free_password(password);
		return p;
	}
	else
	{
		remmina_plugin_service->log_printf("[GKEYRING] password cannot be found for file %s: %s\n", path,
				gnome_keyring_result_to_message(r));
		return NULL;
	}
}

void remmina_plugin_gkeyring_delete_password(RemminaFile *remminafile, const gchar *key)
{
	GnomeKeyringResult r;
	const gchar *path;

	path = remmina_plugin_service->file_get_path(remminafile);
	r = gnome_keyring_delete_password_sync(&remmina_file_secret_schema, "filename", path, "key", key, NULL);
	if (r == GNOME_KEYRING_RESULT_OK)
	{
		remmina_plugin_service->log_printf("[GKEYRING] password deleted for file %s\n", path);
	}
	else
	{
		remmina_plugin_service->log_printf("[GKEYRING] password cannot be deleted for file %s: %s\n", path,
				gnome_keyring_result_to_message(r));
	}
}

static RemminaSecretPlugin remmina_plugin_gkeyring =
{ REMMINA_PLUGIN_TYPE_SECRET, "GKEYRING", "GNOME Keyring", NULL, VERSION,

TRUE, remmina_plugin_gkeyring_store_password, remmina_plugin_gkeyring_get_password, remmina_plugin_gkeyring_delete_password };

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	remmina_plugin_service = service;

	if (!service->register_plugin((RemminaPlugin *) &remmina_plugin_gkeyring))
	{
		return FALSE;
	}
	return TRUE;
}

