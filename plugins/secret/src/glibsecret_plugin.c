/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2011 Vic Lee
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
#include "glibsecret_plugin.h"
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <libsecret/secret.h>
#include <remmina/plugin.h>

static RemminaPluginService *remmina_plugin_service = NULL;

static SecretSchema remmina_file_secret_schema =
{ "org.remmina.Password", SECRET_SCHEMA_NONE,
  {
	  { "filename",	  SECRET_SCHEMA_ATTRIBUTE_STRING },
	  { "key",	  SECRET_SCHEMA_ATTRIBUTE_STRING },
	  { NULL,	  0 }
  } };


#ifdef LIBSECRET_VERSION_0_18
static SecretService* secretservice;
static SecretCollection* defaultcollection;
#endif


gboolean remmina_plugin_glibsecret_is_service_available()
{
#ifdef LIBSECRET_VERSION_0_18
	if (secretservice && defaultcollection)
		return TRUE;
	else
		return FALSE;
#else
	return FALSE;
#endif
}

static void remmina_plugin_glibsecret_unlock_secret_service()
{
	TRACE_CALL(__func__);

#ifdef LIBSECRET_VERSION_0_18

	GError *error = NULL;
	GList *objects, *ul;
	gchar* lbl;

	if (secretservice && defaultcollection) {
		if (secret_collection_get_locked(defaultcollection)) {
			lbl = secret_collection_get_label(defaultcollection);
			remmina_plugin_service->log_printf("[glibsecret] requesting unlock of the default '%s' collection\n", lbl);
			objects = g_list_append(NULL, defaultcollection);
			secret_service_unlock_sync(secretservice, objects, NULL, &ul, &error);
			g_list_free(objects);
			g_list_free(ul);
		}
	}
#endif
	return;
}

void remmina_plugin_glibsecret_store_password(RemminaFile *remminafile, const gchar *key, const gchar *password)
{
	TRACE_CALL(__func__);
	GError *r = NULL;
	const gchar *path;
	gchar *s;

	path = remmina_plugin_service->file_get_path(remminafile);
	s = g_strdup_printf("Remmina: %s - %s", remmina_plugin_service->file_get_string(remminafile, "name"), key);
	secret_password_store_sync(&remmina_file_secret_schema, SECRET_COLLECTION_DEFAULT, s, password,
		NULL, &r, "filename", path, "key", key, NULL);
	g_free(s);
	if (r == NULL) {
		remmina_plugin_service->log_printf("[glibsecret] password \"%s\" saved for file %s\n", key, path);
	}else  {
		remmina_plugin_service->log_printf("[glibsecret] password \"%s\" cannot be saved for file %s\n", key, path);
		g_error_free(r);
	}
}

gchar*
remmina_plugin_glibsecret_get_password(RemminaFile *remminafile, const gchar *key)
{
	TRACE_CALL(__func__);
	GError *r = NULL;
	const gchar *path;
	gchar *password;
	gchar *p;

	path = remmina_plugin_service->file_get_path(remminafile);
	password = secret_password_lookup_sync(&remmina_file_secret_schema, NULL, &r, "filename", path, "key", key, NULL);
	if (r == NULL) {
		// remmina_plugin_service->log_printf("[glibsecret] found password for file %s\n", path);
		p = g_strdup(password);
		secret_password_free(password);
		return p;
	}else  {
		remmina_plugin_service->log_printf("[glibsecret] password cannot be found for file %s\n", path);
		return NULL;
	}
}

void remmina_plugin_glibsecret_delete_password(RemminaFile *remminafile, const gchar *key)
{
	TRACE_CALL(__func__);
	GError *r = NULL;
	const gchar *path;

	path = remmina_plugin_service->file_get_path(remminafile);
	secret_password_clear_sync(&remmina_file_secret_schema, NULL, &r, "filename", path, "key", key, NULL);
	if (r == NULL) {
		remmina_plugin_service->log_printf("[glibsecret] password \"%s\" deleted for file %s\n", key, path);
	}else  {
		remmina_plugin_service->log_printf("[glibsecret] password \"%s\" cannot be deleted for file %s\n", key, path);
	}
}

gboolean remmina_plugin_glibsecret_init()
{
#ifdef LIBSECRET_VERSION_0_18
	GError *error;
	error = NULL;
	secretservice = secret_service_get_sync(SECRET_SERVICE_LOAD_COLLECTIONS, NULL, &error);
	if (error) {
		g_print("[glibsecret] unable to get secret service: %s\n", error->message);
		return FALSE;
	}
	if (secretservice == NULL) {
		g_print("[glibsecret] unable to get secret service: Unknown error.\n");
		return FALSE;
	}

	defaultcollection = secret_collection_for_alias_sync(secretservice, SECRET_COLLECTION_DEFAULT, SECRET_COLLECTION_NONE, NULL, &error);
	if (error) {
		g_print("[glibsecret] unable to get secret service default collection: %s\n", error->message);
		return FALSE;
	}

	remmina_plugin_glibsecret_unlock_secret_service();
	return TRUE;

#else
	g_print("Libsecret was too old during compilation, disabling secret service.\n");
	return FALSE;
#endif
}

static RemminaSecretPlugin remmina_plugin_glibsecret =
{ REMMINA_PLUGIN_TYPE_SECRET,
  "glibsecret",
  N_("Secured password storage in the GNOME keyring"),
  NULL,
  VERSION,
  2000,
  remmina_plugin_glibsecret_init,
  remmina_plugin_glibsecret_is_service_available,
  remmina_plugin_glibsecret_store_password,
  remmina_plugin_glibsecret_get_password,
  remmina_plugin_glibsecret_delete_password
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);

	/* This function should only register the secret plugin. No init action
	 * should be performed here. Initialization will be done later
	 * with remmina_plugin_xxx_init() . */

	remmina_plugin_service = service;

	if (!service->register_plugin((RemminaPlugin*)&remmina_plugin_glibsecret)) {
		return FALSE;
	}

	return TRUE;

}

