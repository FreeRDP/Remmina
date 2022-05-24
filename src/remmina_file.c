/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2022 Antenore Gatta, Giovanni Panozzo
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


#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
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

#include "remmina/remmina_trace_calls.h"
#include "remmina_crypt.h"
#include "remmina_file_manager.h"
#include "remmina_log.h"
#include "remmina_main.h"
#include "remmina_masterthread_exec.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref.h"
#include "remmina_public.h"
#include "remmina_sodium.h"
#include "remmina_utils.h"

#define MIN_WINDOW_WIDTH 10
#define MIN_WINDOW_HEIGHT 10

#define KEYFILE_GROUP_REMMINA "remmina"
#define KEYFILE_GROUP_STATE "Remmina Connection States"

static struct timespec times[2];

static RemminaFile *
remmina_file_new_empty(void)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;

	remminafile = g_new0(RemminaFile, 1);
	remminafile->settings = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	remminafile->states = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
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

/**
 * Generate a new Remmina connection profile file name.
 */
void remmina_file_generate_filename(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);

	/** File name restrictions:
	 * - Do not start with space.
	 * - Do not end with space or dot.
	 * - No more than 255 chars.
	 * - Do not contain \0.
	 * - Avoid % and $.
	 * - Avoid underscores and spaces for interoperabiility with everything else.
	 * - Better all lowercase.
	 */
	gchar *invalid_chars = "\\%|/$?<>:*. \"";
	GString *filenamestr;
	const gchar *s;


	/* functions we can use
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

	//g_free(remminafile->filename), remminafile->filename = NULL;

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

	g_autofree gchar *filename = g_strdelimit(g_ascii_strdown(g_strstrip(g_string_free(filenamestr, FALSE)), -1),
						  invalid_chars, '-');

	GDir *dir = g_dir_open(remmina_file_get_datadir(), 0, NULL);

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

void remmina_file_set_statefile(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);

	if (!remminafile)
		return;
	else
		g_free(remminafile->statefile);

	gchar *basename = g_path_get_basename(remminafile->filename);
	gchar *cachedir = g_build_path("/", g_get_user_cache_dir(), "remmina", NULL);
	GString *fname = g_string_new(g_strdup(basename));

	remminafile->statefile = g_strdup_printf("%s/%s.state", cachedir, fname->str);
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
	gchar *buf;

	remminafile = remmina_file_load(filename);
	buf = g_strdup_printf( "COPY %s",
			remmina_file_get_string(remminafile, "name"));
	remmina_file_set_string(remminafile, "name", buf);
	g_free(buf);

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
	gchar *key;
	gchar *s;
	RemminaProtocolPlugin *protocol_plugin;
	RemminaSecretPlugin *secret_plugin;
	gboolean secret_service_available;
	int w, h;

	gkeyfile = g_key_file_new();

	if (g_file_test(filename, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS)) {
		if (!g_key_file_load_from_file(gkeyfile, filename, G_KEY_FILE_NONE, NULL)) {
			g_key_file_free(gkeyfile);
			REMMINA_DEBUG("Unable to load remmina profile file %s: g_key_file_load_from_file() returned NULL.\n", filename);
			return NULL;
		}
	}

	if (!g_key_file_has_key(gkeyfile, KEYFILE_GROUP_REMMINA, "name", NULL)) {

		REMMINA_DEBUG("Unable to load remmina profile file %s: cannot find key name= in section remmina.\n", filename);
		remminafile = NULL;
		remmina_file_set_statefile(remminafile);

		g_key_file_free(gkeyfile);

		return remminafile;
	}
	remminafile = remmina_file_new_empty();

	protocol_plugin = NULL;

	/* Identify the protocol plugin and get pointers to its RemminaProtocolSetting structs */
	gchar *proto = g_key_file_get_string(gkeyfile, KEYFILE_GROUP_REMMINA, "protocol", NULL);
	if (proto) {
		protocol_plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL, proto);
		g_free(proto);
	}

	secret_plugin = remmina_plugin_manager_get_secret_plugin();
	secret_service_available = secret_plugin && secret_plugin->is_service_available(secret_plugin);

	remminafile->filename = g_strdup(filename);
	gsize nkeys = 0;
	gint keyindex;
	GError *err = NULL;
	gchar **keys = g_key_file_get_keys(gkeyfile, KEYFILE_GROUP_REMMINA, &nkeys, &err);
	if (keys == NULL) {
		g_clear_error(&err);
	}
	for (keyindex = 0; keyindex < nkeys; ++keyindex) {
		key = keys[keyindex];
		/* It may contain an encrypted password
		 * - password = .         // secret_service
		 * - password = $argon2id$v=19$m=262144,t=3,p=…    // libsodium
		 */
		if (protocol_plugin && remmina_plugin_manager_is_encrypted_setting(protocol_plugin, key)) {
			s = g_key_file_get_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, NULL);
#if 0
			switch (remmina_pref.enc_mode) {
				case RM_ENC_MODE_SODIUM_INTERACTIVE:
				case RM_ENC_MODE_SODIUM_MODERATE:
				case RM_ENC_MODE_SODIUM_SENSITIVE:
#if SODIUM_VERSION_INT >= 90200
#endif
					break;
				case RM_ENC_MODE_GCRYPT:
					break;
				case RM_ENC_MODE_SECRET:
				default:
					break;
			}
#endif
			if ((g_strcmp0(s, ".") == 0) && (secret_service_available)) {
				gchar *sec = secret_plugin->get_password(secret_plugin, remminafile, key);
				remmina_file_set_string(remminafile, key, sec);
				/* Annotate in spsettings that this value comes from secret_plugin */
				g_hash_table_insert(remminafile->spsettings, g_strdup(key), NULL);
				g_free(sec);
			} else {
				gchar *decrypted;
				decrypted = remmina_crypt_decrypt(s);
				remmina_file_set_string(remminafile, key, decrypted);
				g_free(decrypted);
			}
			g_free(s), s = NULL;
		} else {
			/* If we find "resolution", then we split it in two */
			if (strcmp(key, "resolution") == 0) {
				gchar *resolution_str = g_key_file_get_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, NULL);
				if (remmina_public_split_resolution_string(resolution_str, &w, &h)) {
					gchar *buf;
					buf = g_strdup_printf("%i", w); remmina_file_set_string(remminafile, "resolution_width", buf); g_free(buf);
					buf = g_strdup_printf("%i", h); remmina_file_set_string(remminafile, "resolution_height", buf); g_free(buf);
				} else {
					remmina_file_set_string(remminafile, "resolution_width", NULL);
					remmina_file_set_string(remminafile, "resolution_height", NULL);
				}
				g_free(resolution_str);
			} else {
				remmina_file_set_string(remminafile, key,
						g_key_file_get_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, NULL));
			}
		}
	}

	upgrade_sshkeys_202001(remminafile);
	g_strfreev(keys);
	remmina_file_set_statefile(remminafile);
	g_key_file_free(gkeyfile);
	return remminafile;
}

void remmina_file_set_string(RemminaFile *remminafile, const gchar *setting, const gchar *value)
{
	TRACE_CALL(__func__);

	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread
		 * (plugins needs it to have user credentials)*/
		RemminaMTExecData *d;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_FILE_SET_STRING;
		d->p.file_set_string.remminafile = remminafile;
		d->p.file_set_string.setting = setting;
		d->p.file_set_string.value = value;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}

	if (value) {
		/* We refuse to accept to set the "resolution" field */
		if (strcmp(setting, "resolution") == 0) {
			// TRANSLATORS: This is a message that pops up when an external Remmina plugin tries to set the window resolution using a legacy parameter.
			const gchar *message = _("Using the «resolution» parameter in the Remmina preferences file is deprecated.\n");
			REMMINA_CRITICAL(message);
			remmina_main_show_warning_dialog(message);
			return;
		}
		g_hash_table_insert(remminafile->settings, g_strdup(setting), g_strdup(value));
	} else {
		g_hash_table_insert(remminafile->settings, g_strdup(setting), g_strdup(""));
	}
}

void remmina_file_set_state(RemminaFile *remminafile, const gchar *setting, const gchar *value)
{
	TRACE_CALL(__func__);

	if (value && value[0] != 0)
		g_hash_table_insert(remminafile->states, g_strdup(setting), g_strdup(value));
	else
		g_hash_table_insert(remminafile->states, g_strdup(setting), g_strdup(""));
}

const gchar *
remmina_file_get_string(RemminaFile *remminafile, const gchar *setting)
{
	TRACE_CALL(__func__);
	gchar *value;

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
		// TRANSLATORS: This is a message that pop-up when an external Remmina plugin tries to set the windows resolution using a legacy parameter.
		const gchar *message = _("Using the «resolution» parameter in the Remmina preferences file is deprecated.\n");
		REMMINA_CRITICAL(message);
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
	g_warning("remmina_file_get_secret(remminafile,“%s”) is deprecated and must not be called. Use remmina_file_get_string() and do not deallocate returned memory.\n", setting);
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

	now = g_date_time_new_now_local();
	date_str = g_date_time_format(now, "%FT%TZ");
	remmina_utils_string_replace_all(fmt_str, "%d", date_str);
	g_free(date_str);

	res = g_string_free(fmt_str, FALSE);
	return res;
}

void remmina_file_set_int(RemminaFile *remminafile, const gchar *setting, gint value)
{
	TRACE_CALL(__func__);
	if (remminafile)
		g_hash_table_insert(remminafile->settings,
				    g_strdup(setting),
				    g_strdup_printf("%i", value));
}

void remmina_file_set_state_int(RemminaFile *remminafile, const gchar *setting, gint value)
{
	TRACE_CALL(__func__);
	if (remminafile)
		g_hash_table_insert(remminafile->states,
				    g_strdup(setting),
				    g_strdup_printf("%i", value));
}

gint remmina_file_get_int(RemminaFile *remminafile, const gchar *setting, gint default_value)
{
	TRACE_CALL(__func__);
	gchar *value;
	gint r;

	value = g_hash_table_lookup(remminafile->settings, setting);
	r = value == NULL ? default_value : (value[0] == 't' ? TRUE : atoi(value));
	// TOO verbose: REMMINA_DEBUG ("Integer value is: %d", r);
	return r;
}

gint remmina_file_get_state_int(RemminaFile *remminafile, const gchar *setting, gint default_value)
{
	TRACE_CALL(__func__);
	gchar *value;
	gint r;

	value = g_hash_table_lookup(remminafile->states, setting);
	r = value == NULL ? default_value : (value[0] == 't' ? TRUE : atoi(value));
	// TOO verbose: REMMINA_DEBUG ("Integer value is: %d", r);
	return r;
}

// sscanf uses the set language to convert the float.
// therefore '.' and ',' cannot be used interchangeably.
gdouble remmina_file_get_double(RemminaFile *	remminafile,
				const gchar *	setting,
				gdouble		default_value)
{
	TRACE_CALL(__func__);
	gchar *value;

	value = g_hash_table_lookup(remminafile->settings, setting);
	if (!value)
		return default_value;

	// str to double.
	// https://stackoverflow.com/questions/10075294/converting-string-to-a-double-variable-in-c
	gdouble d;
	gint ret = sscanf(value, "%lf", &d);

	if (ret != 1)
		// failed.
		d = default_value;

	// TOO VERBOSE: REMMINA_DEBUG("Double value is: %lf", d);
	return d;
}

// sscanf uses the set language to convert the float.
// therefore '.' and ',' cannot be used interchangeably.
gdouble remmina_file_get_state_double(RemminaFile *	remminafile,
				      const gchar *	setting,
				      gdouble		default_value)
{
	TRACE_CALL(__func__);
	gchar *value;

	value = g_hash_table_lookup(remminafile->states, setting);
	if (!value)
		return default_value;

	// str to double.
	// https://stackoverflow.com/questions/10075294/converting-string-to-a-double-variable-in-c
	gdouble d;
	gint ret = sscanf(value, "%lf", &d);

	if (ret != 1)
		// failed.
		d = default_value;

	// TOO VERBOSE: REMMINA_DEBUG("Double value is: %lf", d);
	return d;
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

static GKeyFile *
remmina_file_get_keystate(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;

	if (remminafile->statefile == NULL)
		return NULL;
	gkeyfile = g_key_file_new();
	if (!g_key_file_load_from_file(gkeyfile, remminafile->statefile, G_KEY_FILE_NONE, NULL)) {
		/* it will fail if it’s a new file, but shouldn’t matter. */
	}
	return gkeyfile;
}

void remmina_file_free(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	if (remminafile == NULL)
		return;

	if (remminafile->filename)
		g_free(remminafile->filename);
	if (remminafile->statefile)
		g_free(remminafile->statefile);
	if (remminafile->settings)
		g_hash_table_destroy(remminafile->settings);
	if (remminafile->spsettings)
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
	GKeyFile *gkeystate;
	gsize length = 0;
	GError *err = NULL;

	if (remminafile->prevent_saving)
		return;

	if ((gkeyfile = remmina_file_get_keyfile(remminafile)) == NULL)
		return;

	if ((gkeystate = remmina_file_get_keystate(remminafile)) == NULL)
		return;

	REMMINA_DEBUG("Saving profile");
	/* get disablepasswordstoring */
	nopasswdsave = remmina_file_get_int(remminafile, "disablepasswordstoring", 0);
	/* Identify the protocol plugin and get pointers to its RemminaProtocolSetting structs */
	proto = (gchar *)g_hash_table_lookup(remminafile->settings, "protocol");
	if (proto) {
		protocol_plugin = (RemminaProtocolPlugin *)remmina_plugin_manager_get_plugin(REMMINA_PLUGIN_TYPE_PROTOCOL, proto);
	} else {
		REMMINA_CRITICAL("Saving settings for unknown protocol:", proto);
		protocol_plugin = NULL;
	}

	secret_plugin = remmina_plugin_manager_get_secret_plugin();
	secret_service_available = secret_plugin && secret_plugin->is_service_available(secret_plugin);

	g_hash_table_iter_init(&iter, remminafile->settings);
	while (g_hash_table_iter_next(&iter, (gpointer *)&key, (gpointer *)&value)) {
		if (remmina_plugin_manager_is_encrypted_setting(protocol_plugin, key)) {
			if (remminafile->filename && g_strcmp0(remminafile->filename, remmina_pref_file)) {
				if (secret_service_available && nopasswdsave == 0) {
					REMMINA_DEBUG("We have a secret and disablepasswordstoring=0");
					if (value && value[0]) {
						if (g_strcmp0(value, ".") != 0)
							secret_plugin->store_password(secret_plugin, remminafile, key, value);
						g_key_file_set_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, ".");
					} else {
						g_key_file_set_string(gkeyfile, KEYFILE_GROUP_REMMINA, key, "");
						secret_plugin->delete_password(secret_plugin, remminafile, key);
					}
				} else {
					REMMINA_DEBUG("We have a password and disablepasswordstoring=0");
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
							REMMINA_DEBUG("Deleting the secret in the keyring as disablepasswordstoring=1");
							secret_plugin->delete_password(secret_plugin, remminafile, key);
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

	if (g_file_set_contents(remminafile->filename, content, length, &err))
		REMMINA_DEBUG("Profile saved");
	else
		REMMINA_WARNING("Remmina connection profile cannot be saved, with error %d (%s)", err->code, err->message);
	if (err != NULL)
		g_error_free(err);

	g_free(content), content = NULL;
	/* Saving states */
	g_hash_table_iter_init(&iter, remminafile->states);
	while (g_hash_table_iter_next(&iter, (gpointer *)&key, (gpointer *)&value))
		g_key_file_set_string(gkeyfile, KEYFILE_GROUP_STATE, key, value);
	content = g_key_file_to_data(gkeystate, &length, NULL);
	if (g_file_set_contents(remminafile->statefile, content, length, &err))
		REMMINA_DEBUG("Connection profile states saved");
	else
		REMMINA_WARNING("Remmina connection profile cannot be saved, with error %d (%s)", err->code, err->message);
	if (err != NULL)
		g_error_free(err);
	g_free(content), content = NULL;
	g_key_file_free(gkeyfile);
	g_key_file_free(gkeystate);

	if (!remmina_pref.list_refresh_workaround)
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
		plugin->store_password(plugin, remminafile, key, value);
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

	remmina_file_set_statefile(dupfile);
	remmina_file_touch(dupfile);
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
		return g_strconcat(REMMINA_APP_ID, "-symbolic", NULL);

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

const gchar *
remmina_file_get_state(RemminaFile *remminafile, const gchar *setting)
{
	TRACE_CALL(__func__);
	g_autoptr(GError) error = NULL;
	g_autoptr(GKeyFile) key_file = g_key_file_new();

	if (!g_key_file_load_from_file(key_file, remminafile->statefile, G_KEY_FILE_NONE, &error)) {
		if (!g_error_matches(error, G_FILE_ERROR, G_FILE_ERROR_NOENT))
			REMMINA_CRITICAL("Could not load the state file. %s", error->message);
		return NULL;
	}

	g_autofree gchar *val = g_key_file_get_string(key_file, KEYFILE_GROUP_STATE, setting, &error);

	if (val == NULL &&
	    !g_error_matches(error, G_KEY_FILE_ERROR, G_KEY_FILE_ERROR_KEY_NOT_FOUND)) {
		REMMINA_CRITICAL("Could not find  \"%s\" in the \"%s\" state file. %s",
				 setting, remminafile->statefile, error->message);
		return NULL;
	}
	return val && val[0] ? val : NULL;
}

void remmina_file_state_last_success(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);

	g_autoptr(GKeyFile) key_statefile = g_key_file_new();
	g_autoptr(GKeyFile) key_remminafile = g_key_file_new();
	GError *error = NULL;

	const gchar *date = NULL;
	GDateTime *d = g_date_time_new_now_utc();

	date = g_strdup_printf("%d%02d%02d",
			       g_date_time_get_year(d),
			       g_date_time_get_month(d),
			       g_date_time_get_day_of_month(d));

	g_key_file_set_string(key_statefile, KEYFILE_GROUP_STATE, "last_success", date);

	REMMINA_DEBUG("State file %s.", remminafile->statefile);
	if (!g_key_file_save_to_file(key_statefile, remminafile->statefile, &error)) {
		REMMINA_CRITICAL("Could not save the key file. %s", error->message);
		g_error_free(error);
		error = NULL;
		return;
	}
	/* Delete old pre-1.5 keys */
	g_key_file_remove_key(key_remminafile, KEYFILE_GROUP_REMMINA, "last_success", NULL);
	REMMINA_DEBUG("Last connection made on %s.", date);
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
					// TOO VERBOSE: g_debug("setting name: %s", setting_iter->name);
					if (setting_iter->name == NULL)
						g_error("Internal error: a setting name in protocol plugin %s is null. Please fix RemminaProtocolSetting struct content.", proto);
					else
						if (remmina_plugin_manager_is_encrypted_setting(protocol_plugin, setting_iter->name))
							remmina_file_set_string(remminafile, remmina_plugin_manager_get_canonical_setting_name(setting_iter), NULL);
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
 * Return the string date of the last time a Remmina state file has been modified.
 *
 * This is used to return the modification date of a file and it’s used
 * to return the modification date and time of a given Remmina file.
 * If it fails it will return "Fri, 16 Oct 2009 07:04:46 GMT", that is just a date to don't
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

	if (remminafile->statefile)
		//REMMINA_DEBUG ("remminafile->statefile: %s", remminafile->statefile);
		file = g_file_new_for_path(remminafile->statefile);
	else
		file = g_file_new_for_path(remminafile->filename);

	info = g_file_query_info(file,
				 G_FILE_ATTRIBUTE_TIME_MODIFIED,
				 G_FILE_QUERY_INFO_NONE,
				 NULL,
				 NULL);

	g_object_unref(file);

	if (info == NULL) {
		//REMMINA_DEBUG("could not get time info");

		// The BDAY "Fri, 16 Oct 2009 07:04:46 GMT"
		mtime = 1255676686;
		const gchar *last_success = remmina_file_get_string(remminafile, "last_success");
		if (last_success) {
			//REMMINA_DEBUG ("Last success is %s", last_success);
			GDateTime *dt;
			dt = g_date_time_new_from_iso8601(g_strconcat(last_success, "T00:00:00Z", NULL), NULL);
			if (dt) {
				//REMMINA_DEBUG("Converting last_success");
				mtime = g_ascii_strtoull(g_date_time_format(dt, "%s"), NULL, 10);
				g_date_time_unref(dt);
			} else {
				//REMMINA_DEBUG("dt was null");
				mtime = 191543400;
			}
		}
	} else {
		mtime = g_file_info_get_attribute_uint64(info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
		g_object_unref(info);
	}

	tv.tv_sec = mtime;

	ptm = localtime(&tv.tv_sec);
	strftime(time_string, sizeof(time_string), "%F - %T", ptm);

	gchar *modtime_string = g_locale_to_utf8(time_string, -1, NULL, NULL, NULL);

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

	if ((r = stat(remminafile->statefile, &st)) < 0) {
		if (errno != ENOENT)
			REMMINA_DEBUG("stat %s:", remminafile->statefile);
	} else if (!r) {
		times[0] = st.st_atim;
		times[1] = st.st_mtim;
		if (utimensat(AT_FDCWD, remminafile->statefile, times, 0) < 0)
			REMMINA_DEBUG("utimensat %s:", remminafile->statefile);
		return;
	}

	if ((fd = open(remminafile->statefile, O_CREAT | O_EXCL, 0644)) < 0)
		REMMINA_DEBUG("open %s:", remminafile->statefile);
	close(fd);

	remmina_file_touch(remminafile);
}
