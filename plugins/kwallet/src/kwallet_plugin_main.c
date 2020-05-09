/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2020 Antenore Gatta & Giovanni Panozzo
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
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 * If you modify file(s) with this exception, you may extend this exception
 * to your version of the file(s), but you are not obligated to do so.
 * If you do not wish to do so, delete this exception statement from your
 * version.
 * If you delete this exception statement from all source
 * files in the program, then also delete it here.
 *
 */

#include "config.h"
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <remmina/plugin.h>

#include "kwallet_plugin.h"

static RemminaPluginService *remmina_plugin_service = NULL;

gboolean remmina_plugin_kwallet_is_service_available()
{
	return rp_kwallet_is_service_available();
}


static gchar *build_kwallet_key(RemminaFile *remminafile, const gchar *key)
{
	const gchar *path;
	gchar *kwkey;
	size_t kwkey_sz;

	path = remmina_plugin_service->file_get_path(remminafile);

	kwkey_sz = strlen(key) + 1 + strlen(path) + 1;
	kwkey = g_malloc(kwkey_sz);

	strcpy(kwkey, key);
	strcat(kwkey, ";");
	strcat(kwkey, path);

	return kwkey;
}

void remmina_plugin_kwallet_store_password(RemminaFile *remminafile, const gchar *key, const gchar *password)
{
	TRACE_CALL(__func__);
	gchar *kwkey;
	kwkey = build_kwallet_key(remminafile, key);
	rp_kwallet_store_password(kwkey, password);
	g_free(kwkey);
}

gchar*
remmina_plugin_kwallet_get_password(RemminaFile *remminafile, const gchar *key)
{
	TRACE_CALL(__func__);
	gchar *kwkey, *password;

	kwkey = build_kwallet_key(remminafile, key);
	password = rp_kwallet_get_password(kwkey);
	g_free(kwkey);

	return password;
}

void remmina_plugin_kwallet_delete_password(RemminaFile *remminafile, const gchar *key)
{
	TRACE_CALL(__func__);
	gchar *kwkey;
	kwkey = build_kwallet_key(remminafile, key);
	rp_kwallet_delete_password(kwkey);
	g_free(kwkey);
}

gboolean remmina_plugin_kwallet_init()
{
	/* Activates only when KDE is running */
	const gchar *envvar;

	envvar = g_environ_getenv(g_get_environ(), "XDG_CURRENT_DESKTOP");
	if (!envvar || strcmp(envvar, "KDE") != 0)
		return FALSE;

	return rp_kwallet_init();

}

static RemminaSecretPlugin remmina_plugin_kwallet =
{ REMMINA_PLUGIN_TYPE_SECRET,
  "kwallet",
  N_("Secured password storage in KWallet"),
  NULL,
  VERSION,
  1000,
  remmina_plugin_kwallet_init,
  remmina_plugin_kwallet_is_service_available,
  remmina_plugin_kwallet_store_password,
  remmina_plugin_kwallet_get_password,
  remmina_plugin_kwallet_delete_password,
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);

	/* This function should only register the secret plugin. No init action
	 * should be performed here. Initialization will be done later
	 * with remmina_plugin_xxx_init() . */

	remmina_plugin_service = service;
	if (!service->register_plugin((RemminaPlugin*)&remmina_plugin_kwallet)) {
		return FALSE;
	}

	return TRUE;

}

