/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
 * Copyright (C) 2023-2024 Hiroyuki Tanaka, Sunil Bhat
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

/**
 * @file remmina_info.c
 * @brief Remmina usage statistics module.
 * @author Antenore Gatta and Giovanni Panozzo
 * @date 12 Feb 2018
 *
 * Since October 29, 2021 data is not collected nor sent to remmina.org anymore. All the
 * code intended to send data has been removed. The following documentation
 * has to be kept for those with versions of Remmina older than 1.4.22.
 *
 * When Remmina starts asks the user to share some usage statistics
 * with the Remmina developers. As per the opt-in model
 * (https://en.wikipedia.org/wiki/Opt-in_email), without the consent of the user,
 * no data will be collected.
 * Additionally a user can ask, at any moment, that any data linked to their
 * profile to be deleted, and can change the Remmina settings to stop
 * collecting and sharing usage statistics.
 *
 * All the data are encrypted at client side using RSA, through the OpenSSL
 * libraries, and decrypted offline to maximize security.
 *
 * The following example show which kind of data are collected.
 *
 * @code
 * {
 *
 *    "UID": "P0M20TXN03DWF4-9a1e6da2ad"
 *    "REMMINAVERSION": {
 *        "version": "1.2.0-rcgit-26"
 *        "git_revision": "9c5c4805"
 *        "snap_build": 0
 *    }
 *    "SYSTEM": {
 *        "kernel_name": "Linux"
 *        "kernel_release": "4.14.11-200.fc26.x86_64"
 *        "kernel_arch": "x86_64"
 *        "lsb_distributor": "Fedora"
 *        "lsb_distro_description": "Fedora release 26 (Twenty Six)"
 *        "lsb_distro_release": "26"
 *        "lsb_distro_codename": "TwentySix"
 *        "etc_release": "Fedora release 26 (Twenty Six)"
 *    }
 *    "GTKVERSION": {
 *        "major": 3
 *        "minor": 22
 *        "micro": 21
 *    }
 *    "GTKBACKEND": "X11"
 *    "WINDOWMANAGER": {
 *        "window_manager": "GNOME i3-gnome"
 *    }
 *    "APPINDICATOR": {
 *        "appindicator_supported": 0
 *        "appindicator_compiled": 1
 *        "icon_is_active": 1
 *        "appindicator_type": "AppIndicator on GtkStatusIcon/xembed"
 *    }
 *    "PROFILES": {
 *        "profile_count": 457
 *        "SSH": 431
 *        "NX": 1
 *        "RDP": 7
 *        "TERMINAL": 2
 *        "X2GO": 5
 *        "SFTP": 4
 *        "PYTHON_SIMPLE": 4
 *        "SPICE": 3
 *        "DATE_SSH": "20180209"
 *        "DATE_NX": ""
 *        "DATE_RDP": "20180208"
 *        "DATE_TERMINAL": ""
 *        "DATE_X2GO": ""
 *        "DATE_SFTP": ""
 *        "DATE_PYTHON_SIMPLE": ""
 *        "DATE_SPICE": ""
 *    }
 *    "ENVIRONMENT": {
 *        "language": "en_US.utf8"
 *    }
 *    "ACTIVESECRETPLUGIN": {
 *        "plugin_name": "kwallet"
 *    }
 *    "HASPRIMARYPASSWORD": {
 *        "primary_password_status": "OFF"
 *    }
 *
 * }
 * @endcode
 *
 * All of these data are solely transmitted to understand:
 *  - On which type of system Remmina is used
 *	- Operating System
 *	- Architecture (32/64bit)
 *	- Linux distributor or OS vendor
 *  - Desktop Environment type.
 *  - Main library versions installed on the system in use by Remmina.
 *  - Protocols used
 *  - Last time each protocol has been used (globally).
 *
 * @see https://www.remmina.org for more info.
 */

#include "config.h"
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>


#include "remmina.h"
#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_icon.h"
#include "remmina_log.h"
#include "remmina_pref.h"
#include "remmina_curl_connector.h"
#include "remmina_sysinfo.h"
#include "remmina_utils.h"
#include "remmina_scheduler.h"
#include "remmina_public.h"
#include "remmina_main.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_plugin_manager.h"

#ifdef GDK_WINDOWING_WAYLAND
	#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
	#include <gdk/gdkx.h>
#endif
#include "remmina_info.h"

#define MAX_ENV_LEN	10000

gboolean info_disable_stats = 1;
gboolean info_disable_news = 1;
gboolean info_disable_tip = 1;


struct ProfilesData {
	GHashTable *proto_count;
	GHashTable *proto_date;
	const gchar *protocol;          /** Key in the proto_count hash table.*/
	const gchar *pdatestr;          /** Date in string format in the proto_date hash table. */
	gint pcount;
	gchar datestr;
};

#if !JSON_CHECK_VERSION(1, 2, 0)
	#define json_node_unref(x) json_node_free(x)
#endif

/* Timers */
#define INFO_PERIODIC_CHECK_1ST_MS 1000
#define INFO_PERIODIC_CHECK_INTERVAL_MS 86400000

#define PERIODIC_UPLOAD_URL "https://info.remmina.org/info/upload_stats"
#define INFO_REQUEST_URL "https://info.remmina.org/info/handshake"

 
static RemminaInfoDialog *remmina_info_dialog;
#define GET_OBJ(object_name) gtk_builder_get_object(remmina_info_dialog->builder, object_name)

typedef struct {
	gboolean send_stats;
	JsonNode *statsroot;
} sc_tdata;

JsonNode *remmina_info_stats_get_uid()
{
	TRACE_CALL(__func__);
	JsonNode *r;
	GChecksum *chs;
	const gchar *uname, *hname;
	const gchar *uid_suffix;
	gchar *uid_prefix;
	gchar *uid;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread
	 */

	if (remmina_pref.info_uid_prefix == NULL || remmina_pref.info_uid_prefix[0] == 0) {
		/* Generate a new UUID_PREFIX for this installation */
		uid_prefix = remmina_gen_random_uuid();
		if (remmina_pref.info_uid_prefix) {
			g_free(remmina_pref.info_uid_prefix);
		}
		remmina_pref.info_uid_prefix = uid_prefix;
		remmina_pref_save();
	}

	uname = g_get_user_name();
	hname = g_get_host_name();
	chs = g_checksum_new(G_CHECKSUM_SHA256);
	g_checksum_update(chs, (const guchar *)uname, strlen(uname));
	g_checksum_update(chs, (const guchar *)hname, strlen(hname));
	uid_suffix = g_checksum_get_string(chs);

	uid = g_strdup_printf("%s-%.10s", remmina_pref.info_uid_prefix, uid_suffix);
	g_checksum_free(chs);

	r = json_node_alloc();
	json_node_init_string(r, uid);

	g_free(uid);

	return r;
}

JsonNode *remmina_info_stats_get_os_info()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	gchar *kernel_name;
	gchar *kernel_release;
	gchar *kernel_arch;
	gchar *id;
	gchar *description;
	GHashTable *etc_release;
	gchar *release;
	gchar *codename;
	gchar *plist;
	GHashTableIter iter;
	gchar *key, *value;
	gchar *mage;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread */
	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "kernel_name");
	kernel_name = remmina_utils_get_kernel_name();
	if (!kernel_name || kernel_name[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, kernel_name);
	}
	g_free(kernel_name);

	json_builder_set_member_name(b, "kernel_release");
	kernel_release = remmina_utils_get_kernel_release();
	if (!kernel_release || kernel_release[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, kernel_release);
	}
	g_free(kernel_release);

	json_builder_set_member_name(b, "kernel_arch");
	kernel_arch = remmina_utils_get_kernel_arch();
	if (!kernel_arch || kernel_arch[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, kernel_arch);
	}
	g_free(kernel_arch);

	json_builder_set_member_name(b, "lsb_distributor");
	id = remmina_utils_get_lsb_id();
	if (!id || id[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, id);
	}
	g_free(id);

	json_builder_set_member_name(b, "lsb_distro_description");
	description = remmina_utils_get_lsb_description();
	if (!description || description[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, description);
	}
	g_free(description);

	json_builder_set_member_name(b, "lsb_distro_release");
	release = remmina_utils_get_lsb_release();
	if (!release || release[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, release);
	}
	g_free(release);

	json_builder_set_member_name(b, "lsb_distro_codename");
	codename = remmina_utils_get_lsb_codename();
	if (!codename || codename[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, codename);
	}
	g_free(codename);

	json_builder_set_member_name(b, "process_list");
	plist = remmina_utils_get_process_list();
	if (!plist || plist[0] == '\0') {
		json_builder_add_null_value(b);
	}else {
		json_builder_add_string_value(b, plist);
	}
	g_free(plist);

	etc_release = remmina_utils_get_etc_release();
	json_builder_set_member_name(b, "etc_release");
	if (etc_release) {
		json_builder_begin_object(b);
		g_hash_table_iter_init (&iter, etc_release);
		while (g_hash_table_iter_next (&iter, (gpointer)&key, (gpointer)&value)) {
			json_builder_set_member_name(b, key);
			json_builder_add_string_value(b, value);
		}
		json_builder_end_object(b);
		g_hash_table_remove_all(etc_release);
		g_hash_table_unref(etc_release);
	}else {
		json_builder_add_null_value(b);
	}

	mage = remmina_utils_get_mage();
	json_builder_set_member_name(b, "mage");
	json_builder_add_string_value(b, mage);
	g_free(mage);


	/** @todo Add other means to identify a release name/description
	 *        to cover as much OS as possible, like /etc/issue
	 */

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

/**
 * Gets the following user environment:
 *   - Gets the user’s locale (or NULL by default) corresponding to LC_ALL.
 *
 * @return a JSON Node structure containing the user’s environment.
 */
JsonNode *remmina_info_stats_get_user_env()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	gchar *language;
	gchar **environment;
	gchar **uenv;
	gchar *str;
	gsize env_len = 0;

	language = remmina_utils_get_lang();

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}

	environment = g_get_environ();
	uenv = environment;

	json_builder_begin_object(b);
	json_builder_set_member_name(b, "language");
	json_builder_add_string_value(b, language);

	gchar *safe_ptr;
	while (*uenv && env_len <= MAX_ENV_LEN) {
		str = strtok_r(*uenv, "=", &safe_ptr);
		if (str != NULL) {
			env_len += strlen(str);
			json_builder_set_member_name(b, str);
		}
		str = strtok_r(NULL, "\n", &safe_ptr);
		if (str != NULL) {
			env_len += strlen(str);
			json_builder_add_string_value(b, str);
		}
		else{
			json_builder_add_string_value(b, "");
		}
		uenv++;
	}

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	g_strfreev(environment);
	return r;
}

JsonNode *remmina_info_stats_get_host() {

	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	gchar *logical, *physical;

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}

	json_builder_begin_object(b);
	json_builder_set_member_name(b, "host");
	logical = remmina_utils_get_logical();
	if (!logical || logical[0] == '\0') {
		json_builder_add_null_value(b);
	}
	else {
		json_builder_add_string_value(b, logical);
	}
	g_free(logical);

	json_builder_set_member_name(b, "hw");
	physical = remmina_utils_get_link();
	if (!physical || physical[0] == '\0') {
		json_builder_add_null_value(b);
	}
	else {
		json_builder_add_string_value(b, physical);
	}
	g_free(physical);

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

JsonNode *remmina_info_stats_get_version()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	gchar *flatpak_info;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread */

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}
	json_builder_begin_object(b);
	json_builder_set_member_name(b, "version");
	json_builder_add_string_value(b, VERSION);
	json_builder_set_member_name(b, "git_revision");
	json_builder_add_string_value(b, REMMINA_GIT_REVISION);
	json_builder_set_member_name(b, "snap_build");
#ifdef SNAP_BUILD
	json_builder_add_int_value(b, 1);
#else
	json_builder_add_int_value(b, 0);
#endif

	/**
	 * Detect if Remmina is running under Flatpak
	 */
	json_builder_set_member_name(b, "flatpak_build");
	/* Flatpak sandbox should contain the file ${XDG_RUNTIME_DIR}/flatpak-info */
	flatpak_info = g_build_filename(g_get_user_runtime_dir(), "flatpak-info", NULL);
	if (g_file_test(flatpak_info, G_FILE_TEST_EXISTS)) {
		json_builder_add_int_value(b, 1);
	} else {
		json_builder_add_int_value(b, 0);
	}
	g_free(flatpak_info);

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

JsonNode *remmina_info_stats_get_gtk_version()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread
	 */

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}

	json_builder_begin_object(b);
	json_builder_set_member_name(b, "major");
	json_builder_add_int_value(b, gtk_get_major_version());
	json_builder_set_member_name(b, "minor");
	json_builder_add_int_value(b, gtk_get_minor_version());
	json_builder_set_member_name(b, "micro");
	json_builder_add_int_value(b, gtk_get_micro_version());
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

JsonNode *remmina_info_stats_get_gtk_backend()
{
	TRACE_CALL(__func__);
	JsonNode *r;
	GdkDisplay *disp;
	gchar *bkend;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread
	 */

	disp = gdk_display_get_default();

#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY(disp)) {
		bkend = "Wayland";
	}else
#endif
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(disp)) {
		bkend = "X11";
	}   else
#endif
	bkend = "n/a";

	r = json_node_alloc();
	json_node_init_string(r, bkend);

	return r;

}

JsonNode *remmina_info_stats_get_wm_name()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	gchar *wmver;
	gchar *wmname;

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}

	json_builder_begin_object(b);

	json_builder_set_member_name(b, "window_manager");

	/** We try to get the GNOME Shell version */
	wmver = remmina_sysinfo_get_gnome_shell_version();
	if (!wmver || wmver[0] == '\0') {
	}else {
		json_builder_add_string_value(b, "GNOME Shell");
		json_builder_set_member_name(b, "gnome_shell_ver");
		json_builder_add_string_value(b, wmver);
		goto end;
	}
	g_free(wmver);

	wmname = remmina_sysinfo_get_wm_name();
	if (!wmname || wmname[0] == '\0') {
		json_builder_add_string_value(b, "n/a");
	}else {
		json_builder_add_string_value(b, wmname);
	}
	g_free(wmname);

 end:
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

JsonNode *remmina_info_stats_get_indicator()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	gboolean sni;           /** Support for StatusNotifier or AppIndicator */

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}

	json_builder_begin_object(b);

	json_builder_set_member_name(b, "appindicator_supported");
	sni = remmina_sysinfo_is_appindicator_available();
	if (sni) {
		/** StatusNotifier/Appindicator supported by desktop */
		json_builder_add_int_value(b, 1);
		json_builder_set_member_name(b, "appindicator_compiled");
#ifdef HAVE_LIBAPPINDICATOR
		/** libappindicator is compiled in remmina. */
		json_builder_add_int_value(b, 1);
#else
		/** Remmina not compiled with -DWITH_APPINDICATOR=on */
		json_builder_add_int_value(b, 0);
#endif
	}
	else{
		/** StatusNotifier/Appindicator NOT supported by desktop */
		json_builder_add_int_value(b, 0);
	}
	
	json_builder_set_member_name(b, "icon_is_active");
	if (remmina_icon_is_available()) {
		/** Remmina icon is active */
		json_builder_add_int_value(b, 1);
		json_builder_set_member_name(b, "appindicator_type");
#ifdef HAVE_LIBAPPINDICATOR
		/** libappindicator fallback to GtkStatusIcon/xembed"); */
		json_builder_add_string_value(b, "AppIndicator on GtkStatusIcon/xembed");
#else
		/** Remmina fallback to GtkStatusIcon/xembed */
		json_builder_add_string_value(b, "Remmina icon on GtkStatusIcon/xembed");
#endif
	}else {
		/** Remmina icon is NOT active */
		json_builder_add_int_value(b, 0);
	}
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

/**
 * Given a remmina file, fills a structure containing profiles keys/value tuples.
 *
 * This is used as a callback function with remmina_file_manager_iterate.
 * @todo Move this in a separate file.
 */
static void remmina_info_profiles_get_data(RemminaFile *remminafile, gpointer user_data)
{
	TRACE_CALL(__func__);

	gint count = 0;
	gpointer pcount, kpo;
	gpointer pdate;
	gchar *hday, *hmonth, *hyear;
	gchar *pday, *pmonth, *pyear;

	GDateTime *prof_gdate;          /** Source date -> from profile */
	GDateTime *pdata_gdate;          /** Destination date -> The date in the pdata structure */

	struct ProfilesData* pdata;
	pdata = (struct ProfilesData*)user_data;

	pdata->protocol = remmina_file_get_string(remminafile, "protocol");
	const gchar *last_success = remmina_file_get_string(remminafile, "last_success");

	prof_gdate = pdata_gdate = NULL;
	if (last_success && last_success[0] != '\0' && strlen(last_success) >= 6) {
		pyear = g_strdup_printf("%.4s", last_success);
		pmonth = g_strdup_printf("%.2s", last_success + 4);
		pday = g_strdup_printf("%.2s", last_success + 6);
		prof_gdate = g_date_time_new_local(
				atoi(pyear),
				atoi(pmonth),
				atoi(pday), 0, 0, 0);
		g_free(pyear);
		g_free(pmonth);
		g_free(pday);
	}


	if (pdata->protocol && pdata->protocol[0] != '\0') {
		if (g_hash_table_lookup_extended(pdata->proto_count, pdata->protocol, &kpo, &pcount)) {
			count = GPOINTER_TO_INT(pcount) + 1;
		}else {
			count = 1;
			g_hash_table_insert(pdata->proto_count, g_strdup(pdata->protocol), GINT_TO_POINTER(count));
		}
		g_hash_table_replace(pdata->proto_count, g_strdup(pdata->protocol), GINT_TO_POINTER(count));
		pdate = NULL;
		if (g_hash_table_lookup_extended(pdata->proto_date, pdata->protocol, NULL, &pdate)) {

			pdata_gdate = NULL;
			if (pdate && strlen(pdate) >= 6) {
				pdata->pdatestr = g_strdup(pdate);
				hyear = g_strdup_printf("%.4s", (char*)pdate);
				hmonth = g_strdup_printf("%.2s", (char*)pdate + 4);
				hday = g_strdup_printf("%.2s", (char*)pdate + 6);
				pdata_gdate = g_date_time_new_local(
						atoi(hyear),
						atoi(hmonth),
						atoi(hday), 0, 0, 0);
				g_free(hyear);
				g_free(hmonth);
				g_free(hday);
			}

			/** When both date in the hash and in the profile are valid we compare the date */
			if (prof_gdate != NULL && pdata_gdate != NULL ) {
				gint res = g_date_time_compare( pdata_gdate, prof_gdate );
				/** If the date in the hash less than the date in the profile, we take the latter */
				if (res < 0 ) {
					g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), g_strdup(last_success));
				} else {
					g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), g_strdup(pdata->pdatestr));
				}

			}
			/** If the date in the profile is NOT valid and the date in the hash is valid we keep the latter */
			if (prof_gdate == NULL && pdata_gdate != NULL) {
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), g_strdup(pdata->pdatestr));
			}

			/** If the date in the hash is NOT valid and the date in the profile is valid we keep the latter */
			if (prof_gdate != NULL && pdata_gdate == NULL) {
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), g_strdup(last_success));
			}
			/** If both date are NULL, we insert NULL for that protocol */
			if ((prof_gdate == NULL && pdata_gdate == NULL) && pdata->pdatestr) {
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), NULL);
			}
		} else {
			/** If there is not the protocol in the hash, we add it */
			/** If the date in the profile is not NULL we use it */
			if (pdata->pdatestr) {
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), g_strdup(pdata->pdatestr));
			}else {
				/** Otherwise we set it to NULL */
				g_hash_table_replace(pdata->proto_date, g_strdup(pdata->protocol), NULL);
			}
		}
	}
	if (pdata_gdate)
		g_date_time_unref(pdata_gdate);
	if (prof_gdate)
		g_date_time_unref(prof_gdate);
}

/**
 * Add a JSON member profile_count with a child for each protocol used by the user.
 * Count how many profiles are in use and for each protocol in use counts of how many
 * profiles that uses such protocol.
 *
 * The data can be expressed as follows:
 *
 * | PROTO  | PROF COUNT |
 * |:-------|-----------:|
 * | RDP    |  2560      |
 * | SPICE  |  334       |
 * | SSH    |  1540      |
 * | VNC    |  2         |
 *
 * | PROTO  | LAST USED |
 * |:-------|----------:|
 * | RDP    |  20180129 |
 * | SPICE  |  20171122 |
 * | SSH    |  20180111 |
 *
 * @return a JSON Node structure containing the protocol usage statistics.
 *
 */
JsonNode *remmina_info_stats_get_profiles()
{
	TRACE_CALL(__func__);

	JsonBuilder *b;
	JsonNode *r;
	gchar *s;

	gint profiles_count;
	GHashTableIter pcountiter, pdateiter;
	gpointer pcountkey, pcountvalue;
	gpointer pdatekey, pdatevalue;

	struct ProfilesData *pdata;
	pdata = g_malloc0(sizeof(struct ProfilesData));
	if (pdata == NULL) {
		return NULL;
	}

	b = json_builder_new();
	if (b == NULL) {
		g_free(pdata);
		return NULL;
	}
	 
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "profile_count");

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread */

	pdata->proto_date = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, (GDestroyNotify)g_free);
	pdata->proto_count = g_hash_table_new_full(g_str_hash, g_str_equal,
		(GDestroyNotify)g_free, NULL);

	profiles_count = remmina_file_manager_iterate(
		(GFunc)remmina_info_profiles_get_data,
		(gpointer)pdata);

	json_builder_add_int_value(b, profiles_count);

	g_hash_table_iter_init(&pcountiter, pdata->proto_count);
	while (g_hash_table_iter_next(&pcountiter, &pcountkey, &pcountvalue)) {
		json_builder_set_member_name(b, (gchar*)pcountkey);
		json_builder_add_int_value(b, GPOINTER_TO_INT(pcountvalue));
	}

	g_hash_table_iter_init(&pdateiter, pdata->proto_date);
	while (g_hash_table_iter_next(&pdateiter, &pdatekey, &pdatevalue)) {
		s = g_strdup_printf("DATE_%s", (gchar*)pdatekey);
		json_builder_set_member_name(b, s);
		g_free(s);
		json_builder_add_string_value(b, (gchar*)pdatevalue);
	}

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);

	g_hash_table_remove_all(pdata->proto_date);
	g_hash_table_unref(pdata->proto_date);
	g_hash_table_remove_all(pdata->proto_count);
	g_hash_table_unref(pdata->proto_count);

	g_free(pdata);

	return r;
}

/**
 * Add a JSON member ACTIVESECRETPLUGIN which shows the current secret plugin in use by Remmina.
 *
 * @return a JSON Node structure containing the secret plugin in use
 *
 */
JsonNode *remmina_info_stats_get_secret_plugin()
{
	TRACE_CALL(__func__);

	JsonBuilder *b;
	JsonNode *r;
	RemminaSecretPlugin *secret_plugin;
	secret_plugin = remmina_plugin_manager_get_secret_plugin();

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}

	json_builder_begin_object(b);

	if (secret_plugin && secret_plugin->is_service_available) {
		json_builder_set_member_name(b, "plugin_name");
		json_builder_add_string_value(b, secret_plugin->name);
	}
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);

	return r;
}

/**
 * Add a JSON member HASPRIMARYPASSWORD which shows the status of the master password.
 *
 * @return a JSON Node structure containing the status of the primary password
 *
 */
JsonNode *remmina_info_stats_get_primary_password_status()
{
	TRACE_CALL(__func__);

	JsonBuilder *b;
	JsonNode *r;

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}

	json_builder_begin_object(b);

	json_builder_set_member_name(b, "primary_password_status");
	if (remmina_pref_get_boolean("use_primary_password")) {
		json_builder_add_string_value(b, "ON");
	} else {
		json_builder_add_string_value(b, "OFF");
	}

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);

	return r;
}

/**
 * Add a json member KIOSK which shows the status of the kiosk.
 *
 * @return a JSON Node structure containing the status of the primary password
 *
 */
JsonNode *remmina_info_stats_get_kiosk_mode()
{
	TRACE_CALL(__func__);

	JsonBuilder *b;
	JsonNode *r;

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}

	json_builder_begin_object(b);

	json_builder_set_member_name(b, "kiosk_status");
	if (!kioskmode && kioskmode == FALSE) {
		json_builder_add_string_value(b, "OFF");
	}else {
		json_builder_add_string_value(b, "ON");
	}

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);

	return r;
}

JsonNode *remmina_info_stats_get_python()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	gchar *version;

	version = remmina_utils_get_python();
	b = json_builder_new();
	if(b != NULL)
	{
		json_builder_begin_object(b);
		json_builder_set_member_name(b, "version");
		if (!version || version[0] == '\0') {
			json_builder_add_null_value(b);
		} else {
			json_builder_add_string_value(b, version);
		}
		
		json_builder_end_object(b);
		r = json_builder_get_root(b);
		g_object_unref(b);
		return r;
	}
	else
	{
		return NULL;
	}
}

/**
 * Get all statistics in JSON format to send periodically to the server.
 * The caller should free the returned buffer with g_free()
 * @warning This function is usually executed on a dedicated thread,
 * not on the main thread.
 * @return a pointer to the JSON string.
 */
JsonNode *remmina_info_stats_get_all()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *n;

	b = json_builder_new();
	if (b == NULL) {
		return NULL;
	}
	 
	json_builder_begin_object(b);

	n = remmina_info_stats_get_uid();
	json_builder_set_member_name(b, "UID");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_version();
	json_builder_set_member_name(b, "REMMINAVERSION");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_os_info();
	json_builder_set_member_name(b, "SYSTEM");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_user_env();
	json_builder_set_member_name(b, "ENVIRONMENT");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_gtk_version();
	json_builder_set_member_name(b, "GTKVERSION");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_gtk_backend();
	json_builder_set_member_name(b, "GTKBACKEND");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_wm_name();
	json_builder_set_member_name(b, "WINDOWMANAGER");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_indicator();
	json_builder_set_member_name(b, "APPINDICATOR");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_profiles();
	json_builder_set_member_name(b, "PROFILES");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_secret_plugin();
	json_builder_set_member_name(b, "ACTIVESECRETPLUGIN");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_primary_password_status();
	json_builder_set_member_name(b, "HASPRIMARYPASSWORD");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_kiosk_mode();
	json_builder_set_member_name(b, "KIOSK");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_python();
	json_builder_set_member_name(b, "PYTHON");
	json_builder_add_value(b, n);

	n = remmina_info_stats_get_host();
	json_builder_set_member_name(b, "HOST");
	json_builder_add_value(b, n);

	json_builder_end_object(b);
	n = json_builder_get_root(b);
	g_object_unref(b);

	return n;
}

void remmina_info_schedule()
{
	sc_tdata *data;

	data = g_malloc(sizeof(sc_tdata));
	if (data == NULL) {
		return;
	}
	data->send_stats = !info_disable_stats;

	remmina_scheduler_setup(remmina_info_periodic_check,
			data,
			INFO_PERIODIC_CHECK_1ST_MS,
			INFO_PERIODIC_CHECK_INTERVAL_MS);
}

static void remmina_info_close_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	if (remmina_info_dialog->dialog) {
		gtk_widget_destroy(GTK_WIDGET(remmina_info_dialog->dialog));
	}
	remmina_info_dialog->dialog = NULL;
	g_free(remmina_info_dialog);
	remmina_info_dialog = NULL;
}

static gboolean remmina_info_dialog_deleted(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	gtk_widget_destroy(GTK_WIDGET(remmina_info_dialog->dialog));
	remmina_info_dialog->dialog = NULL;
	g_free(remmina_info_dialog);
	remmina_info_dialog = NULL;

	return FALSE;
}



gboolean remmina_info_show_response(gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkWindow * parent = remmina_main_get_window();
	remmina_info_dialog = g_new0(RemminaInfoDialog, 1);
	remmina_info_dialog->retval = 1;

	RemminaInfoMessage *message = (RemminaInfoMessage*)user_data;

	remmina_info_dialog->builder = remmina_public_gtk_builder_new_from_resource("/org/remmina/Remmina/src/../data/ui/remmina_info.glade");
	remmina_info_dialog->dialog = GTK_DIALOG(gtk_builder_get_object(remmina_info_dialog->builder, "RemminaInfoDialog"));

	remmina_info_dialog->remmina_info_text_view = GTK_TEXT_VIEW(GET_OBJ("remmina_info_text_view"));
	remmina_info_dialog->remmina_info_label = GTK_LABEL(GET_OBJ("remmina_info_label"));

	remmina_info_dialog->remmina_info_button_close = GTK_BUTTON(GET_OBJ("remmina_info_button_close"));
	gtk_widget_set_can_default(GTK_WIDGET(remmina_info_dialog->remmina_info_button_close), TRUE);
	gtk_widget_grab_default(GTK_WIDGET(remmina_info_dialog->remmina_info_button_close));

	gtk_label_set_markup(remmina_info_dialog->remmina_info_label, message->info_string);

	g_signal_connect(remmina_info_dialog->remmina_info_button_close, "clicked",
			 G_CALLBACK(remmina_info_close_clicked), (gpointer)remmina_info_dialog);
	g_signal_connect(remmina_info_dialog->dialog, "close",
			 G_CALLBACK(remmina_info_close_clicked), NULL);
	g_signal_connect(remmina_info_dialog->dialog, "delete-event",
			 G_CALLBACK(remmina_info_dialog_deleted), NULL);

	/* Connect signals */
	gtk_builder_connect_signals(remmina_info_dialog->builder, NULL);

	/* Show the modal news dialog */
	gtk_widget_show_all(GTK_WIDGET(remmina_info_dialog->dialog));
	gtk_window_present(GTK_WINDOW(remmina_info_dialog->dialog));
	if (parent) {
		gtk_window_set_transient_for(GTK_WINDOW(remmina_info_dialog->dialog), parent);
	}
	gtk_window_set_modal(GTK_WINDOW(remmina_info_dialog->dialog), TRUE);
	gtk_window_set_title(GTK_WINDOW(remmina_info_dialog->dialog), message->title_string);
	g_free(message);
	return FALSE;
}


/**
 * Post request to info server
 * 
 * @return gboolean 
 */
static gboolean remmina_info_stats_collector_done(gpointer data)
{
	JsonNode *n;
	JsonGenerator *g;
	gchar *unenc_s, *enc_s;
	JsonBuilder *b;
	JsonObject *o;
	gchar *uid;
	JsonNode *root;

	root = (JsonNode *)data;
	if (root == NULL) {
		return G_SOURCE_REMOVE;
	}

	n = root;

	if ((o = json_node_get_object(n)) == NULL) {
		g_free(data);
		return G_SOURCE_REMOVE;
	}

	uid = g_strdup(json_object_get_string_member(o, "UID"));

	g = json_generator_new();
	json_generator_set_root(g, n);
	json_node_unref(n);
	unenc_s = json_generator_to_data(g, NULL);
	g_object_unref(g);

	EVP_PKEY *remmina_pubkey = remmina_get_pubkey(RSA_KEYTYPE, remmina_RSA_PubKey_v1);
	if (remmina_pubkey == NULL) {
		g_free(uid);
		g_free(unenc_s);
		return G_SOURCE_REMOVE;
	}
	enc_s = remmina_rsa_encrypt_string(remmina_pubkey, unenc_s);
	if (enc_s == NULL) {
		g_free(uid);
		g_free(unenc_s);
		return G_SOURCE_REMOVE;
	}
	
	b = json_builder_new();
	if (b == NULL) {
		return G_SOURCE_REMOVE;
	}


	EVP_PKEY_free(remmina_pubkey);
	json_builder_begin_object(b);
	json_builder_set_member_name(b, "keyversion");
	json_builder_add_int_value(b, 1);
	json_builder_set_member_name(b, "encdata");
	json_builder_add_string_value(b, enc_s);
	json_builder_set_member_name(b, "UID");
	json_builder_add_string_value(b, uid);
	json_builder_set_member_name(b, "news_enabled");
	json_builder_add_int_value(b, TRUE ? 0 : 1);
	json_builder_set_member_name(b, "stats_enabled");
	json_builder_add_int_value(b, TRUE ? 0 : 1);
	json_builder_set_member_name(b, "tip_enabled");
	json_builder_add_int_value(b, TRUE ? 0 : 1);


	json_builder_end_object(b);
	n = json_builder_get_root(b);

	g_free(unenc_s);
	g_object_unref(b);
	g_free(uid);
	g_free(enc_s);

	g = json_generator_new();
	json_generator_set_root(g, n);
	enc_s = json_generator_to_data(g, NULL);
	g_object_unref(g);

	remmina_curl_compose_message(enc_s, "POST",  PERIODIC_UPLOAD_URL, NULL);

	json_node_unref(n);

	return G_SOURCE_REMOVE;
}

/**
 * Collect stats and notify when collection is done
 * 
 * @return gpointer
 */
gpointer remmina_info_stats_collector()
{
	JsonNode *n;

	n = remmina_info_stats_get_all();

	/* Stats collecting is done. Notify main thread calling
	 * remmina_info_stats_collector_done() */
	g_idle_add(remmina_info_stats_collector_done, n);

	return NULL;
}

void remmina_info_request(gpointer data)
{
	// send initial handshake here, in a callback decide how to handle response

	JsonNode *n;
	JsonGenerator *g;
	gchar *enc_s;
	JsonBuilder *b;
	const gchar *uid = "000000";

	n = remmina_info_stats_get_uid();
	if (n != NULL){
		uid = json_node_get_string(n);
	}

	b = json_builder_new();
	if (b == NULL) {
		return;
	}

	json_builder_begin_object(b);
	json_builder_set_member_name(b, "keyversion");
	json_builder_add_int_value(b, 1);
	json_builder_set_member_name(b, "UID");
	json_builder_add_string_value(b, uid);
	json_builder_set_member_name(b, "news_enabled");
	json_builder_add_int_value(b, info_disable_news ? 0 : 1);
	json_builder_set_member_name(b, "stats_enabled");
	json_builder_add_int_value(b, info_disable_stats ? 0 : 1);
	json_builder_set_member_name(b, "tip_enabled");
	json_builder_add_int_value(b, info_disable_tip ? 0 : 1);


	json_builder_end_object(b);
	n = json_builder_get_root(b);

	g_object_unref(b);


	g = json_generator_new();
	json_generator_set_root(g, n);
	enc_s = json_generator_to_data(g, NULL);
	g_object_unref(g);
	remmina_curl_compose_message(enc_s, "POST", INFO_REQUEST_URL, NULL);
	json_node_unref(n);
}

gboolean remmina_info_periodic_check(gpointer user_data)
{
	remmina_info_request(user_data);
	return G_SOURCE_CONTINUE;
}
