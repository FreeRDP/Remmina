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
#include <string.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_icon.h"
#include "remmina_log.h"
#include "remmina_pref.h"
#include "remmina_sysinfo.h"
#include "remmina_utils.h"
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

JsonNode *remmina_stats_get_os_info()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;

	gchar *os_name;
	gchar *os_release;
	gchar *os_arch;

	/** @warning this function is usually executed on a dedicated thread,
	 * not on the main thread */

	b = json_builder_new();
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "os_name");

	os_name = g_strdup_printf("%s", remmina_utils_get_os_name());
	if (!os_name || os_name[0] == '\0')
		os_name = "Unknown";
	json_builder_add_string_value(b, os_name);

	json_builder_set_member_name(b, "os_release");
	os_release = g_strdup_printf("%s", remmina_utils_get_os_release());
	if (!os_release || os_release[0] == '\0')
		os_release = "Unknown";
	json_builder_add_string_value(b, os_release);

	json_builder_set_member_name(b, "os_arch");
	os_arch = g_strdup_printf("%s", remmina_utils_get_os_arch());
	if (!os_arch || os_arch[0] == '\0')
		os_arch = "Unknown";
	json_builder_add_string_value(b, os_arch);

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
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
	JsonBuilder *b;
	JsonNode *r;
	gchar *wmver;
	gchar *wmname;

	b = json_builder_new();
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "window_manager");

	/** We try to get the Gnome SHELL version */
	wmver = remmina_sysinfo_get_gnome_shell_version();
	if (!wmver || wmver[0] == '\0') {
		remmina_log_print("Gnome Shell not found\n");
	}else {
		remmina_log_printf("Gnome Shell version: %s\n", wmver);
		json_builder_add_string_value(b, "Gnome Shell");
		json_builder_set_member_name(b, "gnome_shell_ver");
		json_builder_add_string_value(b,
				g_strdup_printf("Gnome Shell version: %s", wmver));
		goto end;
	}

	wmname = remmina_sysinfo_get_wm_name();
	if (!wmname || wmname[0] == '\0') {
		/** When everything else fails with set the WM name to NULL **/
		remmina_log_print("Cannot determine the Window Manger name\n");
		json_builder_add_string_value(b, "Unknown");
	}else {
		remmina_log_printf("Window Manger names %s\n", wmname);
		json_builder_add_string_value(b, wmname);
	}

end:
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

JsonNode *remmina_stats_get_indicator()
{
	TRACE_CALL(__func__);
	JsonBuilder *b;
	JsonNode *r;
	gboolean sni;           /** Support for StatusNotifier or AppIndicator */
	gboolean sni_active;    /** remmina_icon_is_available */

	b = json_builder_new();
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "appindicator_supported");
	sni = remmina_sysinfo_is_appindicator_available();
	if (sni) {
		/** StatusNotifier/Appindicator supported by desktop */
		json_builder_add_int_value(b, 1);
#ifdef HAVE_LIBAPPINDICATOR
		/** libappindicator is compiled in remmina. */
		json_builder_set_member_name(b, "appindicator_compiled");
		json_builder_add_int_value(b, 1);
#else
		/** Remmina not compiled with -DWITH_APPINDICATOR=on */
		json_builder_add_int_value(b, 0);
#endif
	} else {
		/** StatusNotifier/Appindicator NOT supported by desktop */
		json_builder_add_int_value(b, 0);
		json_builder_set_member_name(b, "icon_is_active");
		sni_active = remmina_icon_is_available();
		if (remmina_icon_is_available())
		{
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
	}
	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;
}

JsonNode *remmina_stats_get_profiles()
{
	TRACE_CALL(__func__);

	JsonBuilder *b;
	JsonNode *r;

	GDir *dir;
	gint c;

	b = json_builder_new();
	json_builder_begin_object(b);

	json_builder_set_member_name(b, "profile_count");

	//c = remmina_file_manager_iterate((GFunc)remmina_stats_file_list_cb, (gpointer)stats);
	c = 2345;
	json_builder_add_int_value(b, 3456);

	json_builder_end_object(b);
	r = json_builder_get_root(b);
	g_object_unref(b);
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

	if (uname(&u) == -1)
		g_print("uname:");

	n = remmina_stats_get_os_info();
	json_builder_set_member_name(b, "SYSTEM");
	json_builder_add_value(b, n);

	n = remmina_stats_get_gtk_version();
	json_builder_set_member_name(b, "GTKVERSION");
	json_builder_add_value(b, n);

	n = remmina_stats_get_gtk_backend();
	json_builder_set_member_name(b, "GTKBACKEND");
	json_builder_add_value(b, n);

	n = remmina_stats_get_wm_name();
	json_builder_set_member_name(b, "WINDOWMANAGER");
	json_builder_add_value(b, n);

	n = remmina_stats_get_indicator();
	json_builder_set_member_name(b, "APPINDICATOR");
	json_builder_add_value(b, n);

	n = remmina_stats_get_profiles();
	json_builder_set_member_name(b, "PROFILES");
	json_builder_add_value(b, n);

	json_builder_end_object(b);
	n = json_builder_get_root(b);
	g_object_unref(b);

	return n;

}
