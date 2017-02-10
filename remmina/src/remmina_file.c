/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

#include <sys/stat.h>

#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <langinfo.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include "remmina_public.h"
#include "remmina_log.h"
#include "remmina_crypt.h"
#include "remmina_file_manager.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref.h"
#include "remmina_main.h"
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"

#define MIN_WINDOW_WIDTH 10
#define MIN_WINDOW_HEIGHT 10

static struct timespec times[2];

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

	{ NULL, 0, FALSE }
};

static RemminaSettingGroup remmina_setting_get_group(const gchar *setting, gboolean *encrypted)
{
	TRACE_CALL("remmina_setting_get_group");
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
	TRACE_CALL("remmina_file_new_empty");
	RemminaFile *remminafile;

	remminafile = g_new0(RemminaFile, 1);
	remminafile->settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	return remminafile;
}

RemminaFile*
remmina_file_new(void)
{
	TRACE_CALL("remmina_file_new");
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
	TRACE_CALL("remmina_file_generate_filename");
	GTimeVal gtime;
	GDir *dir;

	g_free(remminafile->filename);
	g_get_current_time(&gtime);

	dir = g_dir_open(remmina_file_get_datadir(), 0, NULL);
	if (dir != NULL)
		remminafile->filename = g_strdup_printf("%s/%li%03li.remmina", remmina_file_get_datadir(), gtime.tv_sec,
		                                        gtime.tv_usec / 1000);
	else
		remminafile->filename = NULL;
	g_dir_close(dir);
}

void remmina_file_set_filename(RemminaFile *remminafile, const gchar *filename)
{
	TRACE_CALL("remmina_file_set_filename");
	g_free(remminafile->filename);
	remminafile->filename = g_strdup(filename);
}

const gchar*
remmina_file_get_filename(RemminaFile *remminafile)
{
	TRACE_CALL("remmina_file_get_filename");
	return remminafile->filename;
}

RemminaFile*
remmina_file_copy(const gchar *filename)
{
	TRACE_CALL("remmina_file_copy");
	RemminaFile *remminafile;

	remminafile = remmina_file_load(filename);
	remmina_file_generate_filename(remminafile);

	return remminafile;
}

RemminaFile*
remmina_file_load(const gchar *filename)
{
	TRACE_CALL("remmina_file_load");
	GKeyFile *gkeyfile;
	RemminaFile *remminafile;
	gchar **keys;
	gchar *key;
	gint i;
	gchar *s;
	gboolean encrypted;

	gkeyfile = g_key_file_new();

	if (!g_key_file_load_from_file(gkeyfile, filename, G_KEY_FILE_NONE, NULL))
	{
		g_key_file_free(gkeyfile);
		return NULL;
	}

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
	TRACE_CALL("remmina_file_set_string");
	remmina_file_set_string_ref(remminafile, setting, g_strdup(value));
}

void remmina_file_set_string_ref(RemminaFile *remminafile, const gchar *setting, gchar *value)
{
	TRACE_CALL("remmina_file_set_string_ref");
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
	TRACE_CALL("remmina_file_get_string");
	gchar *value;

	value = (gchar*) g_hash_table_lookup(remminafile->settings, setting);
	return value && value[0] ? value : NULL;
}

gchar*
remmina_file_get_secret(RemminaFile *remminafile, const gchar *setting)
{
	TRACE_CALL("remmina_file_get_secret");
	/* This function can be called from a non main thread */

	RemminaSecretPlugin *plugin;
	const gchar *cs;

	if ( !remmina_masterthread_exec_is_main_thread() )
	{
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		gchar *retval;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_FILE_GET_SECRET;
		d->p.file_get_secret.remminafile = remminafile;
		d->p.file_get_secret.setting = setting;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.file_get_secret.retval;
		g_free(d);
		return retval;
	}

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
	TRACE_CALL("remmina_file_set_int");
	g_hash_table_insert(remminafile->settings, g_strdup(setting), g_strdup_printf("%i", value));
}

gint remmina_file_get_int(RemminaFile *remminafile, const gchar *setting, gint default_value)
{
	TRACE_CALL("remmina_file_get_int");
	gchar *value;

	value = g_hash_table_lookup(remminafile->settings, setting);
	return value == NULL ? default_value : (value[0] == 't' ? TRUE : atoi(value));
}

static void remmina_file_store_group(RemminaFile *remminafile, GKeyFile *gkeyfile, RemminaSettingGroup group)
{
	TRACE_CALL("remmina_file_store_group");
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
	TRACE_CALL("remmina_file_get_keyfile");
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
	TRACE_CALL("remmina_file_save_flush");
	gchar *content;
	gsize length = 0;

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remminafile->filename, content, length, NULL);
	g_free(content);

	remmina_main_update_file_datetime(remminafile);
}

void remmina_file_save_group(RemminaFile *remminafile, RemminaSettingGroup group)
{
	TRACE_CALL("remmina_file_save_group");
	GKeyFile *gkeyfile;

	if ((gkeyfile = remmina_file_get_keyfile(remminafile)) == NULL)
		return;
	remmina_file_store_group(remminafile, gkeyfile, group);
	remmina_file_save_flush(remminafile, gkeyfile);
	g_key_file_free(gkeyfile);
}

void remmina_file_save_all(RemminaFile *remminafile)
{
	TRACE_CALL("remmina_file_save_all");
	remmina_file_save_group(remminafile, REMMINA_SETTING_GROUP_ALL);
}

void remmina_file_free(RemminaFile *remminafile)
{
	TRACE_CALL("remmina_file_free");
	if (remminafile == NULL)
		return;

	g_free(remminafile->filename);
	g_hash_table_destroy(remminafile->settings);
	g_free(remminafile);
}

RemminaFile*
remmina_file_dup(RemminaFile *remminafile)
{
	TRACE_CALL("remmina_file_dup");
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
	TRACE_CALL("remmina_file_update_screen_resolution");
	GdkDisplay *display;
#if GTK_CHECK_VERSION(3, 20, 0)
	/* TODO: rename to "seat" */
	GdkSeat *seat;
	GdkDevice *device;
#else
	GdkDeviceManager *device_manager;
	GdkDevice *device;
#endif
	GdkScreen *screen;
#if GTK_CHECK_VERSION(3, 22, 0)
	GdkMonitor *monitor;
#else
	gint monitor;
#endif
	gchar *pos;
	gchar *resolution;
	gint x, y;
	GdkRectangle rect;

	resolution = g_strdup(remmina_file_get_string(remminafile, "resolution"));
	if (resolution == NULL || strchr(resolution, 'x') == NULL)
	{
		display = gdk_display_get_default();
		/* gdk_display_get_device_manager deprecated since 3.20, Use gdk_display_get_default_seat */
#if GTK_CHECK_VERSION(3, 20, 0)
		seat = gdk_display_get_default_seat(display);
		device = gdk_seat_get_pointer(seat);
#else
		device_manager = gdk_display_get_device_manager(display);
		device = gdk_device_manager_get_client_pointer(device_manager);
#endif
		gdk_device_get_position(device, &screen, &x, &y);
#if GTK_CHECK_VERSION(3, 22, 0)
		monitor = gdk_display_get_monitor_at_point(display, x, y);
		gdk_monitor_get_geometry(monitor, &rect);
#else
		monitor = gdk_screen_get_monitor_at_point(screen, x, y);
		gdk_screen_get_monitor_geometry(screen, monitor, &rect);
#endif
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
	TRACE_CALL("remmina_file_get_icon_name");
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
	TRACE_CALL("remmina_file_dup_temp_protocol");
	RemminaFile *tmp;

	tmp = remmina_file_dup(remminafile);
	g_free(tmp->filename);
	tmp->filename = NULL;
	remmina_file_set_string(tmp, "protocol", new_protocol);
	return tmp;
}

void remmina_file_delete(const gchar *filename)
{
	TRACE_CALL("remmina_file_delete");
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

void remmina_file_unsave_password(RemminaFile *remminafile)
{
	TRACE_CALL("remmina_file_unsave_password");
	remmina_file_set_string(remminafile, "password", NULL);
	remmina_file_save_group(remminafile, REMMINA_SETTING_GROUP_CREDENTIAL);
}

gchar*
remmina_file_get_datetime(RemminaFile *remminafile)
{
	TRACE_CALL("remmina_file_get_datetime");

	GFile *file = g_file_new_for_path (remminafile->filename);
	GFileInfo *info;

	struct timeval tv;
	struct tm* ptm;
	char time_string[256];

	guint64 mtime;
	gchar *modtime_string;

	info = g_file_query_info (file,
			G_FILE_ATTRIBUTE_TIME_MODIFIED,
			G_FILE_QUERY_INFO_NONE,
			NULL,
			NULL);

	if (info == NULL) {
		g_print("couldn't get time info\n");
		return "26/01/1976 23:30:00";
	}

	mtime = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
	tv.tv_sec = mtime;

	ptm = localtime(&tv.tv_sec);
	strftime(time_string, sizeof(time_string), "%F - %T", ptm);

	modtime_string = g_locale_to_utf8(time_string, -1, NULL, NULL, NULL);

	g_object_unref (info);

	return modtime_string;
}

/* Function used to update the atime and mtime of a given remmina file, partially
 * taken from suckless sbase */
void
remmina_file_touch(RemminaFile *remminafile)
{
	TRACE_CALL("remmina_file_touch");
	int fd;
	struct stat st;
	int r;

	if ((r = stat(remminafile->filename, &st)) < 0) {
		if (errno != ENOENT)
			remmina_log_printf("stat %s:", remminafile->filename);
	} else if (!r) {
		times[0] = st.st_atim;
		times[1] = st.st_mtim;
		if (utimensat(AT_FDCWD, remminafile->filename, times, 0) < 0)
			remmina_log_printf("utimensat %s:", remminafile->filename);
		return;
	}

	if ((fd = open(remminafile->filename, O_CREAT | O_EXCL, 0644)) < 0)
		remmina_log_printf("open %s:", remminafile->filename);
	close(fd);

	remmina_file_touch(remminafile);
}

