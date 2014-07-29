/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include "config.h"
#include "remmina_public.h"
#include "remmina_pref.h"
#include "remmina_crypt.h"
#include "remmina_plugin_manager.h"
#include "remmina_file.h"

#define MIN_WINDOW_WIDTH 10
#define MIN_WINDOW_HEIGHT 10

typedef struct _RemminaSetting
{
	const gchar *setting;
	RemminaSettingGroup group;
	gboolean encrypted;
} RemminaSetting;

const RemminaSetting remmina_system_settings[] =
{
{ "resolution_width", REMMINA_SETTING_GROUP_NONE, FALSE },
{ "resolution_height", REMMINA_SETTING_GROUP_NONE, FALSE },

{ "username", REMMINA_SETTING_GROUP_CREDENTIAL, FALSE },
{ "password", REMMINA_SETTING_GROUP_CREDENTIAL, TRUE },
{ "cacert", REMMINA_SETTING_GROUP_CREDENTIAL, FALSE },
{ "cacrl", REMMINA_SETTING_GROUP_CREDENTIAL, FALSE },
{ "clientcert", REMMINA_SETTING_GROUP_CREDENTIAL, FALSE },
{ "clientkey", REMMINA_SETTING_GROUP_CREDENTIAL, FALSE },

{ "viewmode", REMMINA_SETTING_GROUP_RUNTIME, FALSE },
{ "scale", REMMINA_SETTING_GROUP_RUNTIME, FALSE },
{ "keyboard_grab", REMMINA_SETTING_GROUP_RUNTIME, FALSE },
{ "window_width", REMMINA_SETTING_GROUP_RUNTIME, FALSE },
{ "window_height", REMMINA_SETTING_GROUP_RUNTIME, FALSE },
{ "window_maximize", REMMINA_SETTING_GROUP_RUNTIME, FALSE },
{ "toolbar_opacity", REMMINA_SETTING_GROUP_RUNTIME, FALSE },

{ NULL, 0, FALSE } };

static RemminaSettingGroup remmina_setting_get_group(const gchar *setting, gboolean *encrypted)
{
	gint i;

	for (i = 0; remmina_system_settings[i].setting; i++)
	{
		if (g_strcmp0(setting, remmina_system_settings[i].setting) == 0)
		{
			if (encrypted)
				*encrypted = remmina_system_settings[i].encrypted;
			return remmina_system_settings[i].group;
		}
	}
	if (encrypted)
		*encrypted = FALSE;
	return REMMINA_SETTING_GROUP_PROFILE;
}

static RemminaFile*
remmina_file_new_empty(void)
{
	RemminaFile *remminafile;

	remminafile = g_new0(RemminaFile, 1);
	remminafile->settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	return remminafile;
}

RemminaFile*
remmina_file_new(void)
{
	RemminaFile *remminafile;

	/* Try to load from the preference file for default settings first */
	remminafile = remmina_file_load(remmina_pref_file);

	if (remminafile)
	{
		g_free(remminafile->filename);
		remminafile->filename = NULL;
	}
	else
	{
		remminafile = remmina_file_new_empty();
	}

	return remminafile;
}

void remmina_file_generate_filename(RemminaFile *remminafile)
{
	GTimeVal gtime;

	g_free(remminafile->filename);
	g_get_current_time(&gtime);
	remminafile->filename = g_strdup_printf("%s/.remmina/%li%03li.remmina", g_get_home_dir(), gtime.tv_sec,
			gtime.tv_usec / 1000);
}

void remmina_file_set_filename(RemminaFile *remminafile, const gchar *filename)
{
	g_free(remminafile->filename);
	remminafile->filename = g_strdup(filename);
}

const gchar*
remmina_file_get_filename(RemminaFile *remminafile)
{
	return remminafile->filename;
}

RemminaFile*
remmina_file_copy(const gchar *filename)
{
	RemminaFile *remminafile;

	remminafile = remmina_file_load(filename);
	remmina_file_generate_filename(remminafile);

	return remminafile;
}

RemminaFile*
remmina_file_load(const gchar *filename)
{
	GKeyFile *gkeyfile;
	RemminaFile *remminafile;
	gchar **keys;
	gchar *key;
	gint i;
	gchar *s;
	gboolean encrypted;

	gkeyfile = g_key_file_new();

	if (!g_key_file_load_from_file(gkeyfile, filename, G_KEY_FILE_NONE, NULL))
		return NULL;

	if (g_key_file_has_key(gkeyfile, "remmina", "name", NULL))
	{
		remminafile = remmina_file_new_empty();

		remminafile->filename = g_strdup(filename);
		keys = g_key_file_get_keys(gkeyfile, "remmina", NULL, NULL);
		if (keys)
		{
			for (i = 0; keys[i]; i++)
			{
				key = keys[i];
				encrypted = FALSE;
				remmina_setting_get_group(key, &encrypted);
				if (encrypted)
				{
					s = g_key_file_get_string(gkeyfile, "remmina", key, NULL);
					if (g_strcmp0(s, ".") == 0)
					{
						remmina_file_set_string(remminafile, key, s);
					}
					else
					{
						remmina_file_set_string_ref(remminafile, key, remmina_crypt_decrypt(s));
					}
					g_free(s);
				}
				else
				{
					remmina_file_set_string_ref(remminafile, key,
							g_key_file_get_string(gkeyfile, "remmina", key, NULL));
				}
			}
			g_strfreev(keys);
		}
	}
	else
	{
		remminafile = NULL;
	}

	g_key_file_free(gkeyfile);

	return remminafile;
}

void remmina_file_set_string(RemminaFile *remminafile, const gchar *setting, const gchar *value)
{
	remmina_file_set_string_ref(remminafile, setting, g_strdup(value));
}

void remmina_file_set_string_ref(RemminaFile *remminafile, const gchar *setting, gchar *value)
{
	if (value)
	{
		g_hash_table_insert(remminafile->settings, g_strdup(setting), value);
	}
	else
	{
		g_hash_table_insert(remminafile->settings, g_strdup(setting), g_strdup(""));
	}
}

const gchar*
remmina_file_get_string(RemminaFile *remminafile, const gchar *setting)
{
	gchar *value;

	value = (gchar*) g_hash_table_lookup(remminafile->settings, setting);
	return value && value[0] ? value : NULL;
}

gchar*
remmina_file_get_secret(RemminaFile *remminafile, const gchar *setting)
{
	RemminaSecretPlugin *plugin;
	const gchar *cs;

	plugin = remmina_plugin_manager_get_secret_plugin();
	cs = remmina_file_get_string(remminafile, setting);
	if (plugin && g_strcmp0(cs, ".") == 0)
	{
		if (remminafile->filename)
		{
			return plugin->get_password(remminafile, setting);
		}
		else
		{
			return NULL;
		}
	}
	else
	{
		return g_strdup(cs);
	}
}

void remmina_file_set_int(RemminaFile *remminafile, const gchar *setting, gint value)
{
	g_hash_table_insert(remminafile->settings, g_strdup(setting), g_strdup_printf("%i", value));
}

gint remmina_file_get_int(RemminaFile *remminafile, const gchar *setting, gint default_value)
{
	gchar *value;

	value = g_hash_table_lookup(remminafile->settings, setting);
	return value == NULL ? default_value : (value[0] == 't' ? TRUE : atoi(value));
}

static void remmina_file_store_group(RemminaFile *remminafile, GKeyFile *gkeyfile, RemminaSettingGroup group)
{
	RemminaSecretPlugin *plugin;
	GHashTableIter iter;
	const gchar *key, *value;
	gchar *s;
	gboolean encrypted;
	RemminaSettingGroup g;

	plugin = remmina_plugin_manager_get_secret_plugin();
	g_hash_table_iter_init(&iter, remminafile->settings);
	while (g_hash_table_iter_next(&iter, (gpointer*) &key, (gpointer*) &value))
	{
		encrypted = FALSE;
		g = remmina_setting_get_group(key, &encrypted);
		if (g != REMMINA_SETTING_GROUP_NONE && (group == REMMINA_SETTING_GROUP_ALL || group == g))
		{
			if (encrypted)
			{
				if (remminafile->filename && g_strcmp0(remminafile->filename, remmina_pref_file))
				{
					if (plugin)
					{
						if (value && value[0])
						{
							if (g_strcmp0(value, ".") != 0)
							{
								plugin->store_password(remminafile, key, value);
							}
							g_key_file_set_string(gkeyfile, "remmina", key, ".");
						}
						else
						{
							g_key_file_set_string(gkeyfile, "remmina", key, "");
							plugin->delete_password(remminafile, key);
						}
					}
					else
					{
						if (value && value[0])
						{
							s = remmina_crypt_encrypt(value);
							g_key_file_set_string(gkeyfile, "remmina", key, s);
							g_free(s);
						}
						else
						{
							g_key_file_set_string(gkeyfile, "remmina", key, "");
						}
					}
				}
			}
			else
			{
				g_key_file_set_string(gkeyfile, "remmina", key, value);
			}
		}
	}
}

static GKeyFile*
remmina_file_get_keyfile(RemminaFile *remminafile)
{
	GKeyFile *gkeyfile;

	if (remminafile->filename == NULL)
		return NULL;
	gkeyfile = g_key_file_new();
	if (!g_key_file_load_from_file(gkeyfile, remminafile->filename, G_KEY_FILE_NONE, NULL))
	{
		/* it will fail if it's a new file, but shouldn't matter. */
	}
	return gkeyfile;
}

static void remmina_file_save_flush(RemminaFile *remminafile, GKeyFile *gkeyfile)
{
	gchar *content;
	gsize length = 0;

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remminafile->filename, content, length, NULL);
	g_free(content);
}

void remmina_file_save_group(RemminaFile *remminafile, RemminaSettingGroup group)
{
	GKeyFile *gkeyfile;

	if ((gkeyfile = remmina_file_get_keyfile(remminafile)) == NULL)
		return;
	remmina_file_store_group(remminafile, gkeyfile, group);
	remmina_file_save_flush(remminafile, gkeyfile);
	g_key_file_free(gkeyfile);
}

void remmina_file_save_all(RemminaFile *remminafile)
{
	remmina_file_save_group(remminafile, REMMINA_SETTING_GROUP_ALL);
}

void remmina_file_free(RemminaFile *remminafile)
{
	if (remminafile == NULL)
		return;

	g_free(remminafile->filename);
	g_hash_table_destroy(remminafile->settings);
	g_free(remminafile);
}

RemminaFile*
remmina_file_dup(RemminaFile *remminafile)
{
	RemminaFile *dupfile;
	GHashTableIter iter;
	const gchar *key, *value;

	dupfile = remmina_file_new_empty();
	dupfile->filename = g_strdup(remminafile->filename);

	g_hash_table_iter_init(&iter, remminafile->settings);
	while (g_hash_table_iter_next(&iter, (gpointer*) &key, (gpointer*) &value))
	{
		remmina_file_set_string(dupfile, key, value);
	}

	return dupfile;
}

void remmina_file_update_screen_resolution(RemminaFile *remminafile)
{
#if GTK_VERSION == 3
	GdkDisplay *display;
	GdkDeviceManager *device_manager;
	GdkDevice *device;
#endif
	GdkScreen *screen;
	gchar *pos;
	gchar *resolution;
	gint x, y;
	gint monitor;
	GdkRectangle rect;

	resolution = g_strdup(remmina_file_get_string(remminafile, "resolution"));
	if (resolution == NULL || strchr(resolution, 'x') == NULL)
	{
#if GTK_VERSION == 3
		display = gdk_display_get_default();
		device_manager = gdk_display_get_device_manager(display);
		device = gdk_device_manager_get_client_pointer(device_manager);
		gdk_device_get_position(device, &screen, &x, &y);
#elif GTK_VERSION == 2
		gdk_display_get_pointer(gdk_display_get_default(), &screen, &x, &y, NULL);
#endif
		monitor = gdk_screen_get_monitor_at_point(screen, x, y);
		gdk_screen_get_monitor_geometry(screen, monitor, &rect);
		remmina_file_set_int(remminafile, "resolution_width", rect.width);
		remmina_file_set_int(remminafile, "resolution_height", rect.height);
	}
	else
	{
		pos = strchr(resolution, 'x');
		*pos++ = '\0';
		remmina_file_set_int(remminafile, "resolution_width", MAX(100, MIN(4096, atoi(resolution))));
		remmina_file_set_int(remminafile, "resolution_height", MAX(100, MIN(4096, atoi(pos))));
	}
	g_free(resolution);
}

const gchar*
remmina_file_get_icon_name(RemminaFile *remminafile)
{
	RemminaProtocolPlugin *plugin;

	plugin = (RemminaProtocolPlugin *) remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL,
			remmina_file_get_string(remminafile, "protocol"));
	if (!plugin)
		return "remmina";

	return (remmina_file_get_int(remminafile, "ssh_enabled", FALSE) ? plugin->icon_name_ssh : plugin->icon_name);
}

RemminaFile*
remmina_file_dup_temp_protocol(RemminaFile *remminafile, const gchar *new_protocol)
{
	RemminaFile *tmp;

	tmp = remmina_file_dup(remminafile);
	g_free(tmp->filename);
	tmp->filename = NULL;
	remmina_file_set_string(tmp, "protocol", new_protocol);
	return tmp;
}

void remmina_file_delete(const gchar *filename)
{
	RemminaSecretPlugin *plugin;
	RemminaFile *remminafile;
	gint i;

	remminafile = remmina_file_load(filename);
	if (remminafile)
	{
		plugin = remmina_plugin_manager_get_secret_plugin();
		if (plugin)
		{
			for (i = 0; remmina_system_settings[i].setting; i++)
			{
				if (remmina_system_settings[i].encrypted)
				{
					plugin->delete_password(remminafile, remmina_system_settings[i].setting);
				}
			}
		}
		remmina_file_free(remminafile);
	}
	g_unlink(filename);
}

