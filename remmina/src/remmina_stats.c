/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2018 Antenore Gatta, Giovanni Panozzo
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
#include <unistd.h>
#include <sys/utsname.h>
#include <gtk/gtk.h>
#include <string.h>
#include "remmina_log.h"
#include "remmina_pref.h"
#include "remmina_utils.h"
#include "remmina_sysinfo.h"
#include "remmina/remmina_trace_calls.h"

#ifdef GDK_WINDOWING_WAYLAND
	#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
	#include <gdk/gdkx.h>
#endif
#include "remmina_stats.h"

struct utsname u;

static gchar* remmina_stats_gen_random_uuid_prefix()
{
	TRACE_CALL(__func__);
	GRand *rand;
	GTimeVal t;
	gchar *result;
	int i;
	static char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	result = g_malloc0(15);

	g_get_current_time(&t);
	rand = g_rand_new_with_seed((guint32)t.tv_sec ^ (guint32)t.tv_usec);

	for (i = 0; i < 7; i++) {
		result[i] = alpha[g_rand_int_range(rand, 0, sizeof(alpha) - 1)];
	}

	g_rand_set_seed(rand, (guint32)t.tv_usec);
	for (i = 0; i < 7; i++) {
		result[i + 7] = alpha[g_rand_int_range(rand, 0, sizeof(alpha) - 1)];
	}
	g_rand_free(rand);

	return result;
}

JsonNode *remmina_stats_get_uid()
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

	if (remmina_pref.periodic_usage_stats_uuid_prefix == NULL || remmina_pref.periodic_usage_stats_uuid_prefix[0] == 0) {
		/* Generate a new UUID_PREFIX for this installation */
		uid_prefix = remmina_stats_gen_random_uuid_prefix();
		if (remmina_pref.periodic_usage_stats_uuid_prefix)
			g_free(remmina_pref.periodic_usage_stats_uuid_prefix);
		remmina_pref.periodic_usage_stats_uuid_prefix = uid_prefix;
		remmina_pref_save();
	}

	uname = g_get_user_name();
	hname = g_get_host_name();
	chs = g_checksum_new(G_CHECKSUM_SHA256);
	g_checksum_update(chs, (const guchar*)uname, strlen(uname));
	g_checksum_update(chs, (const guchar*)hname, strlen(hname));
	uid_suffix = g_checksum_get_string(chs);

	uid = g_strdup_printf("%s-%.10s", remmina_pref.periodic_usage_stats_uuid_prefix, uid_suffix);
	g_checksum_free(chs);

	r = json_node_alloc();
	json_node_init_string(r, uid);

	return r;

}

JsonNode *remmina_stats_get_os_name()
{
	TRACE_CALL(__func__);
	JsonNode *r;
	gchar *os_name;

	os_name = g_strdup_printf("%s", remmina_utils_get_os_name());
	g_print("OS name: %s\n", os_name);
	if (!os_name || os_name[0] == '\0')
		os_name = "Unknown";

	r = json_node_alloc();
	json_node_init_string(r, os_name);

	g_free(os_name);

	return r;
}

JsonNode *remmina_stats_get_os_release()
{
	TRACE_CALL(__func__);
	JsonNode *r;
	gchar *os_release;

	os_release = g_strdup_printf("%s", remmina_utils_get_os_release());
	if (!os_release || os_release[0] == '\0')
		os_release = "Unknown";

	r = json_node_alloc();
	json_node_init_string(r, os_release);

	g_free(os_release);

	return r;
}

JsonNode *remmina_stats_get_os_arch()
{
	TRACE_CALL(__func__);
	JsonNode *r;
	gchar *os_arch;

	os_arch = g_strdup_printf("%s", remmina_utils_get_os_arch());
	if (!os_arch || os_arch[0] == '\0')
		os_arch = "Unknown";

	r = json_node_alloc();
	json_node_init_string(r, os_arch);

	g_free(os_arch);

	return r;
}

JsonNode *remmina_stats_get_version()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread */

	b = json_builder_new();
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
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;

}

JsonNode *remmina_stats_get_gtk_version()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread
	 */

	b = json_builder_new();
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

JsonNode *remmina_stats_get_gtk_backend()
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
	bkend = "Unknown";

	r = json_node_alloc();
	json_node_init_string(r, bkend);

	return r;

}

JsonNode *remmina_stats_get_wm_name()
{
	TRACE_CALL(__func__);
	JsonNode *r;
	gchar *wmver;
	gchar *wmname;

	/** We try to get the Gnome SHELL version */
	wmver = remmina_sysinfo_get_gnome_shell_version();
	if (!wmver || wmver[0] == '\0') {
		remmina_log_print("Gnome Shell not found\n");
	}else {
		remmina_log_printf("Gnome Shell version: %s\n", wmver);
		wmname = g_strdup_printf("Gnome Shell version: %s", wmver);
		goto end;
	}

	wmname = remmina_sysinfo_get_wm_name();
	if (!wmname || wmname[0] == '\0') {
		/** When everything else fails with set the WM name to NULL **/
		wmver = "Unknown";
		wmname = g_strdup_printf("%s", wmver);
	}

end:
	r = json_node_alloc();
	json_node_init_string(r, wmname);
	return r;
}



/**
 * Get all statistics in json format to send periodically to the PHP server.
 * The caller should free the returned buffer with g_free()
 * @warning This function is usually executed on a dedicated thread,
 * not on the main thread.
 * @return a pointer to the JSON string.
 */
JsonNode *remmina_stats_get_all()
{
	TRACE_CALL(__func__);

	if (uname(&u) == -1)
		g_print("uname:");

	JsonBuilder *b;
	JsonNode *n;
	b = json_builder_new();
	json_builder_begin_object(b);

	n = remmina_stats_get_uid();
	json_builder_set_member_name(b, "UID");
	json_builder_add_value(b, n);

	n = remmina_stats_get_version();
	json_builder_set_member_name(b, "REMMINAVERSION");
	json_builder_add_value(b, n);

	n = remmina_stats_get_os_name();
	json_builder_set_member_name(b, "OSNAME");
	json_builder_add_value(b, n);

	n = remmina_stats_get_os_release();
	json_builder_set_member_name(b, "OSRELEASE");
	json_builder_add_value(b, n);

	n = remmina_stats_get_os_arch();
	json_builder_set_member_name(b, "OSARCH");
	json_builder_add_value(b, n);

	n = remmina_stats_get_gtk_version();
	json_builder_set_member_name(b, "GTKVERSION");
	json_builder_add_value(b, n);

	n = remmina_stats_get_gtk_backend();
	json_builder_set_member_name(b, "GTKBACKEND");
	json_builder_add_value(b, n);

	n = remmina_stats_get_wm_name();
	json_builder_set_member_name(b, "WMNAME");
	json_builder_add_value(b, n);

	json_builder_end_object(b);
	n = json_builder_get_root(b);
	g_object_unref(b);

	return n;

}
