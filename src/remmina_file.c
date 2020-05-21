/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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
#include "remmina_utils.h"
#include "remmina/remmina_trace_calls.h"

#define MIN_WINDOW_WIDTH 10
#define MIN_WINDOW_HEIGHT 10

#define KEYFILE_GROUP_REMMINA "remmina"

static struct timespec times[2];

static RemminaFile *
remmina_file_new_empty(void)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;

	remminafile = g_new0(RemminaFile, 1);
	remminafile->settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	/* spsettings contains settings that are loaded from the secure_plugin.
	 * it’s used by remmina_file_store_secret_plugin_password() to know
	 * where to change */
	remminafile->spsettings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	remminafile->prevent_saving = FALSE;
	return remminafile;
}

RemminaFile *
remmina_file_new(void)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;

	/* Try to load from the preference file for default settings first */
	remminafile = remmina_file_load(remmina_pref_file);

	if (remminafile) {
		g_free(remminafile->filename);
		remminafile->filename = NULL;
	} else {
		remminafile = remmina_file_new_empty();
	}

	return remminafile;
}

void remmina_file_generate_filename(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	GDir *dir;

	/* File name restrictions:
	 * - Do not start with space
	 * - Do not end with space or dot
	 * - No more than 255 chars
	 * - Do not contain \0
	 * - Avoid % and $
	 * - Avoid underscores and spaces for interoperabiility with everything else
	 * - Better all lowercase
	 */
	gchar *invalid_chars = "\\%|/$?<>:*. \"";
	GString *filenamestr;
	gchar *filename;
	const gchar *s;


	/** functions we can use
	 * g_strstrip( string )
	 *	Removes leading and trailing whitespace from a string
	 * g_strdelimit (str, invalid_chars, '-'))
	 *	Convert each invalid_chars in a hyphen
	 * g_ascii_strdown(string)
	 *	all lowercase
	 * To be safe we should remove control characters as well (but I'm lazy)
	 * https://rosettacode.org/wiki/Strip_control_codes_and_extended_characters_from_a_string#C
	 * g_utf8_strncpy (gchar *dest, const gchar *src, gsize n);
	 *	copies a given number of characters instead of a given number of bytes. The src string must be valid UTF-8 encoded text.
	 * g_utf8_validate (const gchar *str, gssize max_len, const gchar **end);
	 *	Validates UTF-8 encoded text.
	 */

	g_free(remminafile->filename);

	filenamestr = g_string_new(g_strdup_printf("%s",
						   remmina_pref.remmina_file_name));
	if ((s = remmina_file_get_string(remminafile, "name")) == NULL) s = "name";
	if (g_strstr_len(filenamestr->str, -1, "%N") != NULL)
		remmina_utils_string_replace_all(filenamestr, "%N", s);

	if ((s = remmina_file_get_string(remminafile, "group")) == NULL) s = "group";
	if (g_strstr_len(filenamestr->str, -1, "%G") != NULL)
		remmina_utils_string_replace_all(filenamestr, "%G", s);

	if ((s = remmina_file_get_string(remminafile, "protocol")) == NULL) s = "proto";
	if (g_strstr_len(filenamestr->str, -1, "%P") != NULL)
		remmina_utils_string_replace_all(filenamestr, "%P", s);

	if ((s = remmina_file_get_string(remminafile, "server")) == NULL) s = "host";
	if (g_strstr_len(filenamestr->str, -1, "%h") != NULL)
		remmina_utils_string_replace_all(filenamestr, "%h", s);

	s = NULL;

	filename = g_strdelimit(g_ascii_strdown(g_strstrip(g_string_free(filenamestr, FALSE)), -1),
				invalid_chars, '-');

	dir = g_dir_open(remmina_file_get_datadir(), 0, NULL);
	if (dir != NULL)
		remminafile->filename = g_strdup_printf("%s/%s.remmina", remmina_file_get_datadir(), filename);
	else
		remminafile->filename = NULL;
	g_dir_close(dir);
}

void remmina_file_set_filename(RemminaFile *remminafile, const gchar *filename)
{
	TRACE_CALL(__func__);
	g_free(remminafile->filename);
	remminafile->filename = g_strdup(filename);
}

const gchar *
remmina_file_get_filename(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	return remminafile->filename;
}

RemminaFile *
remmina_file_copy(const gchar *filename)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;

	remminafile = remmina_file_load(filename);
	remmina_file_set_string(remminafile,
			"name",
			g_strdup_printf(
				"COPY %s",
				remmina_file_get_string(remminafile, "name")));

	if (remminafile)
		remmina_file_generate_filename(remminafile);

	return remminafile;
}

const RemminaProtocolSetting *find_protocol_setting(const gchar *name, RemminaProtocolPlugin *protocol_plugin)
{
	TRACE_CALL(__func__);
	const RemminaProtocolSetting *setting_iter;

	if (protocol_plugin == NULL)
		return NULL;

	setting_iter = protocol_plugin->basic_settings;
	if (setting_iter) {
		while (setting_iter->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
			if (strcmp(name, remmina_plugin_manager_get_canonical_setting_name(setting_iter)) == 0)
				return setting_iter;
			setting_iter++;
		}
	}

	setting_iter = protocol_plugin->advanced_settings;
	if (setting_iter) {
		while (setting_iter->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
			if (strcmp(name, remmina_plugin_manager_get_canonical_setting_name(setting_iter)) == 0)
				return setting_iter;
			setting_iter++;
		}
	}

	return NULL;
}


static void upgrade_sshkeys_202001_mig_common_setting(RemminaFile *remminafile, gboolean protocol_is_ssh, gboolean ssh_enabled, gchar *suffix)
{
	gchar *src_key;
	gchar *dst_key;
	const gchar *val;

	src_key = g_strdup_printf("ssh_%s", suffix);
	dst_key = g_strdup_printf("ssh_tunnel_%s", suffix);

	val = remmina_file_get_string(remminafile, src_key);
	if (!val) {
		g_free(dst_key);
		g_free(src_key);
		return;
	}

	if (ssh_enabled && val && val[0] != 0)
		remmina_file_set_string(remminafile, dst_key, val);

	if (!protocol_is_ssh)
		remmina_file_set_string(remminafile, src_key, NULL);

	g_free(dst_key);
	g_free(src_key);
}

static void upgrade_sshkeys_202001(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);

	gboolean protocol_is_ssh;
	gboolean ssh_enabled;
	const gchar *val;

	if (remmina_file_get_string(remminafile, "ssh_enabled")) {

		/* Upgrade ssh params from remmina pre 1.4 */

		ssh_enabled = remmina_file_get_int(remminafile, "ssh_enabled", 0);
		val = remmina_file_get_string(remminafile, "protocol");
		protocol_is_ssh = (strcmp(val, "SSH") == 0);

		upgrade_sshkeys_202001_mig_common_setting(remminafile, protocol_is_ssh, ssh_enabled, "stricthostkeycheck");
		upgrade_sshkeys_202001_mig_common_setting(remminafile, protocol_is_ssh, ssh_enabled, "kex_algorithms");
		upgrade_sshkeys_202001_mig_common_setting(remminafile, protocol_is_ssh, ssh_enabled, "hostkeytypes");
		upgrade_sshkeys_202001_mig_common_setting(remminafile, protocol_is_ssh, ssh_enabled, "ciphers");
		upgrade_sshkeys_202001_mig_common_setting(remminafile, protocol_is_ssh, ssh_enabled, "proxycommand");
		upgrade_sshkeys_202001_mig_common_setting(remminafile, protocol_is_ssh, ssh_enabled, "passphrase");
		upgrade_sshkeys_202001_mig_common_setting(remminafile, protocol_is_ssh, ssh_enabled, "auth");
		upgrade_sshkeys_202001_mig_common_setting(remminafile, protocol_is_ssh, ssh_enabled, "privatekey");

		val = remmina_file_get_string(remminafile, "ssh_loopback");
		if (val) {
			remmina_file_set_string(remminafile, "ssh_tunnel_loopback", val);
			remmina_file_set_string(remminafile, "ssh_loopback", NULL);
		}

		val = remmina_file_get_string(remminafile, "ssh_username");
		if (val) {
			remmina_file_set_string(remminafile, "ssh_tunnel_username", val);
			if (protocol_is_ssh)
				remmina_file_set_string(remminafile, "username", val);
			remmina_file_set_string(remminafile, "ssh_username", NULL);
		}

		val = remmina_file_get_string(remminafile, "ssh_password");
		if (val) {
			remmina_file_set_string(remminafile, "ssh_tunnel_password", val);
			if (protocol_is_ssh)
				remmina_file_set_string(remminafile, "password", val);
			remmina_file_set_string(remminafile, "ssh_password", NULL);
		}

		val = remmina_file_get_string(remminafile, "ssh_server");
		if (val) {
			remmina_file_set_string(remminafile, "ssh_tunnel_server", val);
			remmina_file_set_string(remminafile, "ssh_server", NULL);
		}

		/* Real key removal will be done by remmina_file_save() */

		remmina_file_set_int(remminafile, "ssh_tunnel_enabled", ssh_enabled);

	}

}

RemminaFile *
remmina_file_load(const gchar *filename)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	RemminaFile *remminafile;
	gchar *proto;
	gchar **keys;
	gchar *key;
	gchar *resolution_str;
	gint i;
	gchar *s, *sec;
	RemminaProtocolPlugin *protocol_plugin;
	RemminaSecretPlugin *secret_plugin;
	gboolean secret_service_available;
	int w, h;

	gkeyfile = g_key_file_new();

	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS)) {
		if (!g_key_file_load_from_file(gkeyfile, filename, G_KEY_FILE_NONE, NULL)) {
			g_key_file_free(gkeyfile);
			REMMINA_DEBUG ("Unable to load remmina profile file %s: g_key_file_load_from_file() returned NULL.\n", filename);
			return NULL;
		}
	}

	if (g_key_file_has_key(gkeyfile, KEYFILE_GROUP_REMMINA, "name", NULL)) {

		remminafile = remmina_file_new_empty();

		protocol_plugin = NULL;

		/* Identify the protocol plugin and get pointers to its RemminaProtocolSetting structs */
		proto = g_key_file_get_string(gkeyfile, KEYFILE_GROUP_REMMINA, "protocol", NULL);
		if (proto) {
			protocol_plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL, proto);
			g_free(proto);
		}

		secret_plugin = remmina_plugin_manager_get_secret_plugin();
		secret_service_available = secret_plugin && secret_plugin->is_service_available();

		remminafile->filename = g_strdup(filename);
		keys = g_key_file_get_keys(gkeyfile, KEYFILE_GROUP_REMMINA, NULL, NULL);
		if (keys) {

			for (i = 0; keys[i]; i++) {
				key = keys[i];
				if (protocol_plugin && remmina_plugin_manager_is_encrypted_setting(protocol_plugin, key)) {
					s = g_key_file_get_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, NULL);
					if (g_strcmp0(s, ".") == 0) {
						if (secret_service_available) {
							sec = secret_plugin->get_password(remminafile, key);
							remmina_file_set_string(remminafile, key, sec);
							/* Annotate in spsettings that this value comes from secret_plugin */
							g_hash_table_insert(remminafile->spsettings, g_strdup(key), NULL);
							g_free(sec);
						} else {
							remmina_file_set_string(remminafile, key, s);
						}
					} else {
						remmina_file_set_string_ref(remminafile, key, remmina_crypt_decrypt(s));
					}
					g_free(s);
				} else {
					/* If we find "resolution", then we split it in two */
					if (strcmp(key, "resolution") == 0) {
						resolution_str = g_key_file_get_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, NULL);
						if (remmina_public_split_resolution_string(resolution_str, &w, &h)) {
							remmina_file_set_string_ref(remminafile, "resolution_width", g_strdup_printf("%i", w));
							remmina_file_set_string_ref(remminafile, "resolution_height", g_strdup_printf("%i", h));
						} else {
							remmina_file_set_string_ref(remminafile, "resolution_width", NULL);
							remmina_file_set_string_ref(remminafile, "resolution_height", NULL);
						}
						g_free(resolution_str);
					} else {
						remmina_file_set_string_ref(remminafile, key,
									    g_key_file_get_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, NULL));
					}
				}
			}

			upgrade_sshkeys_202001(remminafile);

		}
	} else {
		REMMINA_DEBUG ("Unable to load remmina profile file %s: cannot find key name= in section remmina.\n", filename);
		remminafile = NULL;
	}

	g_key_file_free(gkeyfile);

	return remminafile;
}

void remmina_file_set_string(RemminaFile *remminafile, const gchar *setting, const gchar *value)
{
	TRACE_CALL(__func__);
	remmina_file_set_string_ref(remminafile, setting, g_strdup(value));
}

void remmina_file_set_string_ref(RemminaFile *remminafile, const gchar *setting, gchar *value)
{
	TRACE_CALL(__func__);
	const gchar *message;

	if (value) {
		/* We refuse to accept to set the "resolution" field */
		if (strcmp(setting, "resolution") == 0) {
			message = "WARNING: the \"resolution\" setting in .pref files is deprecated, but some code in remmina or in a plugin is trying to set it.\n";
			fputs(message, stdout);
			remmina_main_show_warning_dialog(message);
			return;
		}
		g_hash_table_insert(remminafile->settings, g_strdup(setting), value);
	} else {
		g_hash_table_insert(remminafile->settings, g_strdup(setting), g_strdup(""));
	}
}

const gchar *
remmina_file_get_string(RemminaFile *remminafile, const gchar *setting)
{
	TRACE_CALL(__func__);
	gchar *value;
	const gchar *message;

	/* Returned value is a pointer to the string stored on the hash table,
	 * please do not free it or the hash table will contain invalid pointer */
	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread
		 * (plugins needs it to have user credentials)*/
		RemminaMTExecData *d;
		const gchar *retval;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_FILE_GET_STRING;
		d->p.file_get_string.remminafile = remminafile;
		d->p.file_get_string.setting = setting;
		remmina_masterthread_exec_and_wait(d);
		retval = d->p.file_get_string.retval;
		g_free(d);
		return retval;
	}

	if (strcmp(setting, "resolution") == 0) {
		message = "WARNING: the \"resolution\" setting in .pref files is deprecated, but some code in remmina or in a plugin is trying to read it.\n";
		fputs(message, stdout);
		remmina_main_show_warning_dialog(message);
		return NULL;
	}

	value = (gchar *)g_hash_table_lookup(remminafile->settings, setting);
	return value && value[0] ? value : NULL;
}

gchar *
remmina_file_get_secret(RemminaFile *remminafile, const gchar *setting)
{
	TRACE_CALL(__func__);

	/* This function is in the RemminaPluginService table, we cannot remove it
	 * without breaking plugin API */
	g_warning("remmina_file_get_secret(remminafile,\"%s\") is deprecated and must not be called. Use remmina_file_get_string() and do not deallocate returned memory.\n", setting);
	return g_strdup(remmina_file_get_string(remminafile, setting));
}

gchar *remmina_file_format_properties(RemminaFile *remminafile, const gchar *setting)
{
	gchar *res = NULL;
	GString *fmt_str;
	GDateTime *now;
	gchar *date_str = NULL;

	fmt_str = g_string_new(setting);
	remmina_utils_string_replace_all(fmt_str, "%h", remmina_file_get_string(remminafile, "server"));
	remmina_utils_string_replace_all(fmt_str, "%t", remmina_file_get_string(remminafile, "ssh_tunnel_server"));
	remmina_utils_string_replace_all(fmt_str, "%u", remmina_file_get_string(remminafile, "username"));
	remmina_utils_string_replace_all(fmt_str, "%U", remmina_file_get_string(remminafile, "ssh_tunnel_username"));
	remmina_utils_string_replace_all(fmt_str, "%p", remmina_file_get_string(remminafile, "name"));
	remmina_utils_string_replace_all(fmt_str, "%g", remmina_file_get_string(remminafile, "group"));

	now = g_date_time_new_now_local ();
	date_str = g_date_time_format(now, "YYYY-MM-DDTHH-MM-SSZ");
	remmina_utils_string_replace_all(fmt_str, "%d", date_str);
	g_free(date_str);

	res = g_string_free(fmt_str, FALSE);
	return res;
}

void remmina_file_set_int(RemminaFile *remminafile, const gchar *setting, gint value)
{
	TRACE_CALL(__func__);
	g_hash_table_insert(remminafile->settings, g_strdup(setting), g_strdup_printf("%i", value));
}

gint remmina_file_get_int(RemminaFile *remminafile, const gchar *setting, gint default_value)
{
	TRACE_CALL(__func__);
	gchar *value;

	value = g_hash_table_lookup(remminafile->settings, setting);
	return value == NULL ? default_value : (value[0] == 't' ? TRUE : atoi(value));
}

static GKeyFile *
remmina_file_get_keyfile(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;

	if (remminafile->filename == NULL)
		return NULL;
	gkeyfile = g_key_file_new();
	if (!g_key_file_load_from_file(gkeyfile, remminafile->filename, G_KEY_FILE_NONE, NULL)) {
		/* it will fail if it’s a new file, but shouldn’t matter. */
	}
	return gkeyfile;
}

void remmina_file_free(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	if (remminafile == NULL)
		return;

	g_free(remminafile->filename);
	g_hash_table_destroy(remminafile->settings);
	g_hash_table_destroy(remminafile->spsettings);
	g_free(remminafile);
}


void remmina_file_save(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaSecretPlugin *secret_plugin;
	gboolean secret_service_available;
	RemminaProtocolPlugin *protocol_plugin;
	GHashTableIter iter;
	const gchar *key, *value;
	gchar *s, *proto, *content;
	gint nopasswdsave;
	GKeyFile *gkeyfile;
	gsize length = 0;
	GError *err = NULL;

	if (remminafile->prevent_saving)
		return;

	if ((gkeyfile = remmina_file_get_keyfile(remminafile)) == NULL)
		return;

	REMMINA_DEBUG ("Saving profile");
	/* get disablepasswordstoring */
	nopasswdsave = remmina_file_get_int(remminafile, "disablepasswordstoring", 0);
	/* Identify the protocol plugin and get pointers to its RemminaProtocolSetting structs */
	proto = (gchar *)g_hash_table_lookup(remminafile->settings, "protocol");
	if (proto) {
		protocol_plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL, proto);
	} else {
		g_warning("Saving settings for unknown protocol, because remminafile has non proto key\n");
		protocol_plugin = NULL;
	}

	secret_plugin = remmina_plugin_manager_get_secret_plugin();
	secret_service_available = secret_plugin && secret_plugin->is_service_available();

	g_hash_table_iter_init(&iter, remminafile->settings);
	while (g_hash_table_iter_next(&iter, (gpointer *)&key, (gpointer *)&value)) {
		if (remmina_plugin_manager_is_encrypted_setting(protocol_plugin, key)) {
			if (remminafile->filename && g_strcmp0(remminafile->filename, remmina_pref_file)) {
				if (secret_service_available && nopasswdsave == 0) {
					REMMINA_DEBUG ("We have a secret and disablepasswordstoring=0");
					if (value && value[0]) {
						if (g_strcmp0(value, ".") != 0)
							secret_plugin->store_password(remminafile, key, value);
						g_key_file_set_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, ".");
					} else {
						g_key_file_set_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, "");
						secret_plugin->delete_password(remminafile, key);
					}
				} else {
					REMMINA_DEBUG ("We have a password and disablepasswordstoring=0");
					if (value && value[0] && nopasswdsave == 0) {
						s = remmina_crypt_encrypt(value);
						g_key_file_set_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, s);
						g_free(s);
					} else {
						g_key_file_set_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, "");
					}
				}
				if (secret_service_available && nopasswdsave == 1) {
					if (value && value[0]) {
						if (g_strcmp0(value, ".") != 0) {
							REMMINA_DEBUG ("Deleting the secret in the keyring as disablepasswordstoring=1");
							secret_plugin->delete_password(remminafile, key);
							g_key_file_set_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, ".");
						}
					}
				}
			}
		} else {
			g_key_file_set_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, value);
		}
	}

	/* Avoid storing redundant and deprecated "resolution" field */
	g_key_file_remove_key(gkeyfile, KEYFILE_GROUP_REMMINA, "resolution", NULL);

	/* Delete old pre-1.4 ssh keys */
	g_key_file_remove_key(gkeyfile, KEYFILE_GROUP_REMMINA, "ssh_enabled", NULL);
	g_key_file_remove_key(gkeyfile, KEYFILE_GROUP_REMMINA, "save_ssh_server", NULL);
	g_key_file_remove_key(gkeyfile, KEYFILE_GROUP_REMMINA, "save_ssh_username", NULL);

	/* Store gkeyfile to disk (password are already sent to keyring) */
	content = g_key_file_to_data(gkeyfile, &length, NULL);

	if (g_file_set_contents(remminafile->filename, content, length, &err)) {
		REMMINA_DEBUG ("Profile saved");
	} else {
		g_warning("Remmina connection profile cannot be saved, with error %d (%s)", err->code, err->message);
	}
	if (err != NULL)
		g_error_free (err);

	g_free(content);
	g_key_file_free(gkeyfile);

	remmina_main_update_file_datetime(remminafile);
}

void remmina_file_store_secret_plugin_password(RemminaFile *remminafile, const gchar *key, const gchar *value)
{
	TRACE_CALL(__func__);

	/* Only change the password in the keyring. This function
	 * is a shortcut which avoids updating of date/time of .pref file
	 * when possible, and is used by the mpchanger */
	RemminaSecretPlugin *plugin;

	if (g_hash_table_lookup_extended(remminafile->spsettings, g_strdup(key), NULL, NULL)) {
		plugin = remmina_plugin_manager_get_secret_plugin();
		plugin->store_password(remminafile, key, value);
	} else {
		remmina_file_set_string(remminafile, key, value);
		remmina_file_save(remminafile);
	}
}

RemminaFile *
remmina_file_dup(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaFile *dupfile;
	GHashTableIter iter;
	const gchar *key, *value;

	dupfile = remmina_file_new_empty();
	dupfile->filename = g_strdup(remminafile->filename);

	g_hash_table_iter_init(&iter, remminafile->settings);
	while (g_hash_table_iter_next(&iter, (gpointer *)&key, (gpointer *)&value))
		remmina_file_set_string(dupfile, key, value);

	return dupfile;
}

const gchar *
remmina_file_get_icon_name(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	RemminaProtocolPlugin *plugin;

	plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL,
									    remmina_file_get_string(remminafile, "protocol"));
	if (!plugin)
		return g_strconcat (REMMINA_APP_ID, "-symbolic", NULL);

	return remmina_file_get_int(remminafile, "ssh_tunnel_enabled", FALSE) ? plugin->icon_name_ssh : plugin->icon_name;
}

RemminaFile *
remmina_file_dup_temp_protocol(RemminaFile *remminafile, const gchar *new_protocol)
{
	TRACE_CALL(__func__);
	RemminaFile *tmp;

	tmp = remmina_file_dup(remminafile);
	g_free(tmp->filename);
	tmp->filename = NULL;
	remmina_file_set_string(tmp, "protocol", new_protocol);
	return tmp;
}

void remmina_file_delete(const gchar *filename)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;

	remminafile = remmina_file_load(filename);
	if (remminafile) {
		remmina_file_unsave_passwords(remminafile);
		remmina_file_free(remminafile);
	}
	g_unlink(filename);
}

void remmina_file_unsave_passwords(RemminaFile *remminafile)
{
	/* Delete all saved secrets for this profile */

	TRACE_CALL(__func__);
	const RemminaProtocolSetting *setting_iter;
	RemminaProtocolPlugin *protocol_plugin;
	gchar *proto;
	protocol_plugin = NULL;

	remmina_file_set_string(remminafile, "password", NULL);

	proto = (gchar *)g_hash_table_lookup(remminafile->settings, "protocol");
	if (proto) {
		protocol_plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL, proto);
		if (protocol_plugin) {
			setting_iter = protocol_plugin->basic_settings;
			if (setting_iter) {
				while (setting_iter->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
					g_debug("setting name: %s", setting_iter->name);
					if (setting_iter->name == NULL) {
						g_error("Internal error: a setting name in protocol plugin %s is null. Please fix RemminaProtocolSetting struct content.", proto);
					} else {
						if (remmina_plugin_manager_is_encrypted_setting(protocol_plugin, setting_iter->name))
							remmina_file_set_string(remminafile, remmina_plugin_manager_get_canonical_setting_name(setting_iter), NULL);
					}
					setting_iter++;
				}
			}
			setting_iter = protocol_plugin->advanced_settings;
			if (setting_iter) {
				while (setting_iter->type != REMMINA_PROTOCOL_SETTING_TYPE_END) {
					if (remmina_plugin_manager_is_encrypted_setting(protocol_plugin, setting_iter->name))
						remmina_file_set_string(remminafile, remmina_plugin_manager_get_canonical_setting_name(setting_iter), NULL);
					setting_iter++;
				}
			}
			remmina_file_save(remminafile);
		}
	}
}

/**
 * Return the string date of the last time a file has been modified.
 *
 * This is used to return the modification date of a file and it’s used
 * to return the modification date and time of a givwn remmina file.
 * If it fails it will return "26/01/1976 23:30:00", that is just a date to don't
 * return an empty string (challenge: what was happened that day at that time?).
 * @return A date string in the form "%d/%m/%Y %H:%M:%S".
 * @todo This should be moved to remmina_utils.c
 */
gchar *
remmina_file_get_datetime(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);

	GFile *file;
	GFileInfo *info;

	struct timeval tv;
	struct tm *ptm;
	char time_string[256];

	guint64 mtime;
	gchar *modtime_string;

	file = g_file_new_for_path(remminafile->filename);

	info = g_file_query_info(file,
				 G_FILE_ATTRIBUTE_TIME_MODIFIED,
				 G_FILE_QUERY_INFO_NONE,
				 NULL,
				 NULL);

	g_object_unref(file);

	if (info == NULL) {
		g_print("couldn’t get time info\n");
		return "26/01/1976 23:30:00";
	}

	mtime = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
	tv.tv_sec = mtime;

	ptm = localtime(&tv.tv_sec);
	strftime(time_string, sizeof(time_string), "%F - %T", ptm);

	modtime_string = g_locale_to_utf8(time_string, -1, NULL, NULL, NULL);

	g_object_unref(info);

	return modtime_string;
}

/**
 * Update the atime and mtime of a given filename.
 * Function used to update the atime and mtime of a given remmina file, partially
 * taken from suckless sbase
 * @see https://git.suckless.org/sbase/tree/touch.c
 * @todo This should be moved to remmina_utils.c
 */
void
remmina_file_touch(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	int fd;
	struct stat st;
	int r;

	if ((r = stat(remminafile->filename, &st)) < 0) {
		if (errno != ENOENT)
			REMMINA_DEBUG("stat %s:", remminafile->filename);
	} else if (!r) {
		times[0] = st.st_atim;
		times[1] = st.st_mtim;
		if (utimensat(AT_FDCWD, remminafile->filename, times, 0) < 0)
			REMMINA_DEBUG("utimensat %s:", remminafile->filename);
		return;
	}

	if ((fd = open(remminafile->filename, O_CREAT | O_EXCL, 0644)) < 0)
		REMMINA_DEBUG("open %s:", remminafile->filename);
	close(fd);

	remmina_file_touch(remminafile);
}
