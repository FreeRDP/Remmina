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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/utsname.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "remmina_public.h"
#include "remmina_string_array.h"
#include "remmina_pref.h"
#include "remmina/remmina_trace_calls.h"

const gchar *default_resolutions = "640x480,800x600,1024x768,1152x864,1280x960,1400x1050";
const gchar *default_keystrokes = "Send hello world§hello world\\n";

gchar *remmina_keymap_file;
static GHashTable *remmina_keymap_table = NULL;

/* We could customize this further if there are more requirements */
static const gchar *default_keymap_data = "# Please check gdk/gdkkeysyms.h for a full list of all key names or hex key values\n"
        "\n"
        "[Map Meta Keys]\n"
        "Super_L = Meta_L\n"
        "Super_R = Meta_R\n"
        "Meta_L = Super_L\n"
        "Meta_R = Super_R\n";

static void remmina_pref_gen_secret(void)
{
	TRACE_CALL("remmina_pref_gen_secret");
	guchar s[32];
	gint i;
	GTimeVal gtime;
	GKeyFile *gkeyfile;
	gchar *content;
	gsize length;

	g_get_current_time(&gtime);
	srand(gtime.tv_sec);

	for (i = 0; i < 32; i++)
	{
		s[i] = (guchar)(rand() % 256);
	}
	remmina_pref.secret = g_base64_encode(s, 32);

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	g_key_file_set_string(gkeyfile, "remmina_pref", "secret", remmina_pref.secret);
	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
	g_free(content);
}

static guint remmina_pref_get_keyval_from_str(const gchar *str)
{
	TRACE_CALL("remmina_pref_get_keyval_from_str");
	guint k;

	if (!str)
		return 0;

	k = gdk_keyval_from_name(str);
	if (!k)
	{
		if (sscanf(str, "%x", &k) < 1)
			k = 0;
	}
	return k;
}

static void remmina_pref_init_keymap(void)
{
	TRACE_CALL("remmina_pref_init_keymap");
	GKeyFile *gkeyfile;
	gchar **groups;
	gchar **gptr;
	gchar **keys;
	gchar **kptr;
	gsize nkeys;
	gchar *value;
	guint *table;
	guint *tableptr;
	guint k1, k2;

	if (remmina_keymap_table)
		g_hash_table_destroy(remmina_keymap_table);
	remmina_keymap_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	gkeyfile = g_key_file_new();
	if (!g_key_file_load_from_file(gkeyfile, remmina_keymap_file, G_KEY_FILE_NONE, NULL))
	{
		if (!g_key_file_load_from_data(gkeyfile, default_keymap_data, strlen(default_keymap_data), G_KEY_FILE_NONE,
		                               NULL))
		{
			g_print("Failed to initialize keymap table\n");
			g_key_file_free(gkeyfile);
			return;
		}
	}

	groups = g_key_file_get_groups(gkeyfile, NULL);
	gptr = groups;
	while (*gptr)
	{
		keys = g_key_file_get_keys(gkeyfile, *gptr, &nkeys, NULL);
		table = g_new0(guint, nkeys * 2 + 1);
		g_hash_table_insert(remmina_keymap_table, g_strdup(*gptr), table);

		kptr = keys;
		tableptr = table;
		while (*kptr)
		{
			k1 = remmina_pref_get_keyval_from_str(*kptr);
			if (k1)
			{
				value = g_key_file_get_string(gkeyfile, *gptr, *kptr, NULL);
				k2 = remmina_pref_get_keyval_from_str(value);
				g_free(value);
				*tableptr++ = k1;
				*tableptr++ = k2;
			}
			kptr++;
		}
		g_strfreev(keys);
		gptr++;
	}
	g_strfreev(groups);
	g_key_file_free(gkeyfile);
}

/* TODO: remmina_pref_file_do_copy and remmina_file_manager_do_copy to remmina_files_copy */
static gboolean remmina_pref_file_do_copy(const char *src_path, const char *dst_path)
{
	GFile *src = g_file_new_for_path(src_path), *dst = g_file_new_for_path(dst_path);
	/* We don't overwrite the target if it exists, because overwrite is not set */
	const gboolean ok = g_file_copy(src, dst, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);
	g_object_unref(dst);
	g_object_unref(src);

	return ok;
}

void remmina_pref_init(void)
{
	TRACE_CALL("remmina_pref_init");
	GKeyFile *gkeyfile;
	gchar *remmina_dir;
	const gchar *filename = g_strdup_printf("%s.pref", g_get_prgname());
	GDir *dir;
	gchar *legacy = g_strdup_printf (".%s", g_get_prgname ());
	int i;

	remmina_dir = g_build_path ( "/", g_get_user_config_dir (), g_get_prgname (), NULL);
	/* Create the XDG_CONFIG_HOME directory */
	g_mkdir_with_parents (remmina_dir, 0750);

	g_free (remmina_dir), remmina_dir = NULL;
	/* Legacy ~/.remmina we copy the existing remmina.pref file inside
	 * XDG_CONFIG_HOME */
	remmina_dir = g_build_path ("/", g_get_home_dir(), legacy, NULL);
	if (g_file_test (remmina_dir, G_FILE_TEST_IS_DIR))
	{
		dir = g_dir_open(remmina_dir, 0, NULL);
		remmina_pref_file_do_copy(
				g_build_path ( "/", remmina_dir, filename, NULL),
				g_build_path ( "/", g_get_user_config_dir (),
					g_get_prgname (), filename, NULL));
	}

	/* /usr/local/etc/remmina */
	const gchar * const *dirs = g_get_system_config_dirs ();
	g_free (remmina_dir), remmina_dir = NULL;
	for (i = 0; dirs[i] != NULL; ++i)
	{
		remmina_dir = g_build_path ( "/", dirs[i], g_get_prgname (), NULL);
		if (g_file_test (remmina_dir, G_FILE_TEST_IS_DIR))
		{
			dir = g_dir_open(remmina_dir, 0, NULL);
			while ((filename = g_dir_read_name (dir)) != NULL) {
				remmina_pref_file_do_copy (
						g_build_path ( "/", remmina_dir, filename, NULL),
						g_build_path ( "/", g_get_user_config_dir (),
							g_get_prgname (), filename, NULL));
			}
			g_free (remmina_dir), remmina_dir = NULL;
		}
	}

	/* The last case we use  the home ~/.config/remmina */
	if (remmina_dir != NULL)
		g_free (remmina_dir), remmina_dir = NULL;
	remmina_dir = g_build_path ( "/", g_get_user_config_dir (),
			g_get_prgname (), NULL);

	remmina_pref_file = g_strdup_printf("%s/remmina.pref", remmina_dir);
	remmina_keymap_file = g_strdup_printf("%s/remmina.keymap", remmina_dir);

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "save_view_mode", NULL))
		remmina_pref.save_view_mode = g_key_file_get_boolean(gkeyfile, "remmina_pref", "save_view_mode", NULL);
	else
		remmina_pref.save_view_mode = TRUE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "fullscreen_on_auto", NULL))
		remmina_pref.fullscreen_on_auto = g_key_file_get_boolean(gkeyfile, "remmina_pref", "fullscreen_on_auto", NULL);
	else
		remmina_pref.fullscreen_on_auto = TRUE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "floating_toolbar_placement", NULL))
		remmina_pref.floating_toolbar_placement = g_key_file_get_integer(gkeyfile, "remmina_pref", "floating_toolbar_placement", NULL);
	else
		remmina_pref.floating_toolbar_placement = FLOATING_TOOLBAR_PLACEMENT_TOP;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "toolbar_placement", NULL))
		remmina_pref.toolbar_placement = g_key_file_get_integer(gkeyfile, "remmina_pref", "toolbar_placement", NULL);
	else
		remmina_pref.toolbar_placement = TOOLBAR_PLACEMENT_LEFT;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "always_show_tab", NULL))
		remmina_pref.always_show_tab = g_key_file_get_boolean(gkeyfile, "remmina_pref", "always_show_tab", NULL);
	else
		remmina_pref.always_show_tab = TRUE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "hide_connection_toolbar", NULL))
		remmina_pref.hide_connection_toolbar = g_key_file_get_boolean(gkeyfile, "remmina_pref",
		                                       "hide_connection_toolbar", NULL);
	else
		remmina_pref.hide_connection_toolbar = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "default_action", NULL))
		remmina_pref.default_action = g_key_file_get_integer(gkeyfile, "remmina_pref", "default_action", NULL);
	else
		remmina_pref.default_action = REMMINA_ACTION_CONNECT;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "scale_quality", NULL))
		remmina_pref.scale_quality = g_key_file_get_integer(gkeyfile, "remmina_pref", "scale_quality", NULL);
	else
		remmina_pref.scale_quality = GDK_INTERP_HYPER;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "hide_toolbar", NULL))
		remmina_pref.hide_toolbar = g_key_file_get_boolean(gkeyfile, "remmina_pref", "hide_toolbar", NULL);
	else
		remmina_pref.hide_toolbar = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "hide_statusbar", NULL))
		remmina_pref.hide_statusbar = g_key_file_get_boolean(gkeyfile, "remmina_pref", "hide_statusbar", NULL);
	else
		remmina_pref.hide_statusbar = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "small_toolbutton", NULL))
		remmina_pref.small_toolbutton = g_key_file_get_boolean(gkeyfile, "remmina_pref", "small_toolbutton", NULL);
	else
		remmina_pref.small_toolbutton = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "view_file_mode", NULL))
		remmina_pref.view_file_mode = g_key_file_get_integer(gkeyfile, "remmina_pref", "view_file_mode", NULL);
	else
		remmina_pref.view_file_mode = REMMINA_VIEW_FILE_LIST;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "resolutions", NULL))
		remmina_pref.resolutions = g_key_file_get_string(gkeyfile, "remmina_pref", "resolutions", NULL);
	else
		remmina_pref.resolutions = g_strdup(default_resolutions);

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "keystrokes", NULL))
		remmina_pref.keystrokes = g_key_file_get_string(gkeyfile, "remmina_pref", "keystrokes", NULL);
	else
		remmina_pref.keystrokes = g_strdup(default_keystrokes);

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "main_width", NULL))
		remmina_pref.main_width = MAX(600, g_key_file_get_integer(gkeyfile, "remmina_pref", "main_width", NULL));
	else
		remmina_pref.main_width = 600;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "main_height", NULL))
		remmina_pref.main_height = MAX(400, g_key_file_get_integer(gkeyfile, "remmina_pref", "main_height", NULL));
	else
		remmina_pref.main_height = 400;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "main_maximize", NULL))
		remmina_pref.main_maximize = g_key_file_get_boolean(gkeyfile, "remmina_pref", "main_maximize", NULL);
	else
		remmina_pref.main_maximize = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "main_sort_column_id", NULL))
		remmina_pref.main_sort_column_id = g_key_file_get_integer(gkeyfile, "remmina_pref", "main_sort_column_id",
		                                   NULL);
	else
		remmina_pref.main_sort_column_id = 1;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "main_sort_order", NULL))
		remmina_pref.main_sort_order = g_key_file_get_integer(gkeyfile, "remmina_pref", "main_sort_order", NULL);
	else
		remmina_pref.main_sort_order = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "expanded_group", NULL))
		remmina_pref.expanded_group = g_key_file_get_string(gkeyfile, "remmina_pref", "expanded_group", NULL);
	else
		remmina_pref.expanded_group = g_strdup("");

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "toolbar_pin_down", NULL))
		remmina_pref.toolbar_pin_down = g_key_file_get_boolean(gkeyfile, "remmina_pref", "toolbar_pin_down", NULL);
	else
		remmina_pref.toolbar_pin_down = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "ssh_loglevel", NULL))
		remmina_pref.ssh_loglevel = g_key_file_get_integer(gkeyfile, "remmina_pref", "ssh_loglevel", NULL);
	else
		remmina_pref.ssh_loglevel = DEFAULT_SSH_LOGLEVEL;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "screenshot_path", NULL)) {
		remmina_pref.screenshot_path = g_key_file_get_string(gkeyfile, "remmina_pref", "screenshot_path", NULL);
	}else{
		remmina_pref.screenshot_path = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
		if (remmina_pref.screenshot_path == NULL)
			remmina_pref.screenshot_path = g_get_home_dir();
	}

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "ssh_parseconfig", NULL))
		remmina_pref.ssh_parseconfig = g_key_file_get_boolean(gkeyfile, "remmina_pref", "ssh_parseconfig", NULL);
	else
		remmina_pref.ssh_parseconfig = DEFAULT_SSH_PARSECONFIG;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "sshtunnel_port", NULL))
		remmina_pref.sshtunnel_port = g_key_file_get_integer(gkeyfile, "remmina_pref", "sshtunnel_port", NULL);
	else
		remmina_pref.sshtunnel_port = DEFAULT_SSHTUNNEL_PORT;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "applet_new_ontop", NULL))
		remmina_pref.applet_new_ontop = g_key_file_get_boolean(gkeyfile, "remmina_pref", "applet_new_ontop", NULL);
	else
		remmina_pref.applet_new_ontop = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "applet_hide_count", NULL))
		remmina_pref.applet_hide_count = g_key_file_get_boolean(gkeyfile, "remmina_pref", "applet_hide_count", NULL);
	else
		remmina_pref.applet_hide_count = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "applet_enable_avahi", NULL))
		remmina_pref.applet_enable_avahi = g_key_file_get_boolean(gkeyfile, "remmina_pref", "applet_enable_avahi",
		                                   NULL);
	else
		remmina_pref.applet_enable_avahi = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "disable_tray_icon", NULL))
		remmina_pref.disable_tray_icon = g_key_file_get_boolean(gkeyfile, "remmina_pref", "disable_tray_icon", NULL);
	else
		remmina_pref.disable_tray_icon = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "dark_tray_icon", NULL))
		remmina_pref.dark_tray_icon = g_key_file_get_boolean(gkeyfile, "remmina_pref", "dark_tray_icon", NULL);
	else
		remmina_pref.dark_tray_icon = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "recent_maximum", NULL))
		remmina_pref.recent_maximum = g_key_file_get_integer(gkeyfile, "remmina_pref", "recent_maximum", NULL);
	else
		remmina_pref.recent_maximum = 10;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "default_mode", NULL))
		remmina_pref.default_mode = g_key_file_get_integer(gkeyfile, "remmina_pref", "default_mode", NULL);
	else
		remmina_pref.default_mode = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "tab_mode", NULL))
		remmina_pref.tab_mode = g_key_file_get_integer(gkeyfile, "remmina_pref", "tab_mode", NULL);
	else
		remmina_pref.tab_mode = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "fullscreen_toolbar_visibility", NULL))
		remmina_pref.fullscreen_toolbar_visibility = g_key_file_get_integer(gkeyfile, "remmina_pref", "fullscreen_toolbar_visibility", NULL);
	else
		remmina_pref.fullscreen_toolbar_visibility = FLOATING_TOOLBAR_VISIBILITY_PEEKING;
	/* Show buttons icons */
	if (g_key_file_has_key(gkeyfile, "remmina_pref", "show_buttons_icons", NULL))
	{
		remmina_pref.show_buttons_icons = g_key_file_get_integer(gkeyfile, "remmina_pref", "show_buttons_icons", NULL);
		if (remmina_pref.show_buttons_icons)
		{
			g_object_set(gtk_settings_get_default(), "gtk-button-images", remmina_pref.show_buttons_icons == 1, NULL);
		}
	}
	else
		remmina_pref.show_buttons_icons = 0;
	/* Show menu icons */
	if (g_key_file_has_key(gkeyfile, "remmina_pref", "show_menu_icons", NULL))
	{
		remmina_pref.show_menu_icons = g_key_file_get_integer(gkeyfile, "remmina_pref", "show_menu_icons", NULL);
		if (remmina_pref.show_menu_icons)
		{
			g_object_set(gtk_settings_get_default(), "gtk-menu-images", remmina_pref.show_menu_icons == 1, NULL);
		}
	}
	else
		remmina_pref.show_menu_icons = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "auto_scroll_step", NULL))
		remmina_pref.auto_scroll_step = g_key_file_get_integer(gkeyfile, "remmina_pref", "auto_scroll_step", NULL);
	else
		remmina_pref.auto_scroll_step = 10;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "hostkey", NULL))
		remmina_pref.hostkey = g_key_file_get_integer(gkeyfile, "remmina_pref", "hostkey", NULL);
	else
		remmina_pref.hostkey = GDK_KEY_Control_R;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_fullscreen", NULL))
		remmina_pref.shortcutkey_fullscreen = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_fullscreen",
		                                      NULL);
	else
		remmina_pref.shortcutkey_fullscreen = GDK_KEY_f;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_autofit", NULL))
		remmina_pref.shortcutkey_autofit = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_autofit",
		                                   NULL);
	else
		remmina_pref.shortcutkey_autofit = GDK_KEY_1;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_nexttab", NULL))
		remmina_pref.shortcutkey_nexttab = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_nexttab",
		                                   NULL);
	else
		remmina_pref.shortcutkey_nexttab = GDK_KEY_Right;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_prevtab", NULL))
		remmina_pref.shortcutkey_prevtab = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_prevtab",
		                                   NULL);
	else
		remmina_pref.shortcutkey_prevtab = GDK_KEY_Left;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_scale", NULL))
		remmina_pref.shortcutkey_scale = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_scale", NULL);
	else
		remmina_pref.shortcutkey_scale = GDK_KEY_s;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_grab", NULL))
		remmina_pref.shortcutkey_grab = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_grab", NULL);
	else
		remmina_pref.shortcutkey_grab = GDK_KEY_Control_R;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_minimize", NULL))
		remmina_pref.shortcutkey_minimize = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_minimize", NULL);
	else
		remmina_pref.shortcutkey_minimize = GDK_KEY_F9;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_screenshot", NULL))
		remmina_pref.shortcutkey_screenshot = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_screenshot", NULL);
	else
		remmina_pref.shortcutkey_screenshot = GDK_KEY_F12;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_disconnect", NULL))
		remmina_pref.shortcutkey_disconnect = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_disconnect",
		                                      NULL);
	else
		remmina_pref.shortcutkey_disconnect = GDK_KEY_F4;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_toolbar", NULL))
		remmina_pref.shortcutkey_toolbar = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_toolbar",
		                                   NULL);
	else
		remmina_pref.shortcutkey_toolbar = GDK_KEY_t;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "secret", NULL))
		remmina_pref.secret = g_key_file_get_string(gkeyfile, "remmina_pref", "secret", NULL);
	else
		remmina_pref.secret = NULL;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "uid", NULL))
		remmina_pref.uid = g_key_file_get_string(gkeyfile, "remmina_pref", "uid", NULL);
	else
		remmina_pref.uid = NULL;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "bdate", NULL))
		remmina_pref.bdate = g_key_file_get_integer(gkeyfile, "remmina_pref", "bdate", NULL);
	else
		remmina_pref.bdate = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_font", NULL))
		remmina_pref.vte_font = g_key_file_get_string(gkeyfile, "remmina_pref", "vte_font", NULL);
	else
		remmina_pref.vte_font = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_allow_bold_text", NULL))
		remmina_pref.vte_allow_bold_text = g_key_file_get_boolean(gkeyfile, "remmina_pref", "vte_allow_bold_text",
		                                   NULL);
	else
		remmina_pref.vte_allow_bold_text = TRUE;
	/* Default system theme colors or default vte colors */
	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_system_colors", NULL))
		remmina_pref.vte_system_colors = g_key_file_get_boolean(gkeyfile, "remmina_pref", "vte_system_colors", NULL);
	else
		remmina_pref.vte_system_colors = FALSE;
	/* Customized vte foreground color */
	if (g_key_file_has_key (gkeyfile, "remmina_pref", "vte_foreground_color", NULL))
		remmina_pref.vte_foreground_color = g_key_file_get_string (gkeyfile, "remmina_pref", "vte_foreground_color", NULL);
	else
		remmina_pref.vte_foreground_color = "rgb(192,192,192)";
	/* Customized vte background color */
	if (g_key_file_has_key (gkeyfile, "remmina_pref", "vte_background_color", NULL))
		remmina_pref.vte_background_color = g_key_file_get_string (gkeyfile, "remmina_pref", "vte_background_color", NULL);
	else
		remmina_pref.vte_background_color = "rgb(0,0,0)";

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_lines", NULL))
		remmina_pref.vte_lines = g_key_file_get_integer(gkeyfile, "remmina_pref", "vte_lines", NULL);
	else
		remmina_pref.vte_lines = 512;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_shortcutkey_copy", NULL))
		remmina_pref.vte_shortcutkey_copy = g_key_file_get_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_copy",
		                                    NULL);
	else
		remmina_pref.vte_shortcutkey_copy = GDK_KEY_c;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_shortcutkey_paste", NULL))
		remmina_pref.vte_shortcutkey_paste = g_key_file_get_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_paste",
		                                     NULL);
	else
		remmina_pref.vte_shortcutkey_paste = GDK_KEY_v;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_shortcutkey_select_all", NULL))
		remmina_pref.vte_shortcutkey_select_all = g_key_file_get_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_select_all",
		                                     NULL);
	else
		remmina_pref.vte_shortcutkey_select_all = GDK_KEY_a;

	g_key_file_free(gkeyfile);

	if (remmina_pref.secret == NULL)
		remmina_pref_gen_secret();

	remmina_pref_init_keymap();
}

void remmina_pref_save(void)
{
	TRACE_CALL("remmina_pref_save");
	GKeyFile *gkeyfile;
	gchar *content;
	gsize length;

	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	g_key_file_set_boolean(gkeyfile, "remmina_pref", "save_view_mode", remmina_pref.save_view_mode);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "floating_toolbar_placement", remmina_pref.floating_toolbar_placement);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "toolbar_placement", remmina_pref.toolbar_placement);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "always_show_tab", remmina_pref.always_show_tab);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_connection_toolbar", remmina_pref.hide_connection_toolbar);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "default_action", remmina_pref.default_action);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "scale_quality", remmina_pref.scale_quality);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "ssh_loglevel", remmina_pref.ssh_loglevel);
	g_key_file_set_string(gkeyfile, "remmina_pref", "screenshot_path", remmina_pref.screenshot_path);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "ssh_parseconfig", remmina_pref.ssh_parseconfig);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_toolbar", remmina_pref.hide_toolbar);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_statusbar", remmina_pref.hide_statusbar);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "small_toolbutton", remmina_pref.small_toolbutton);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "view_file_mode", remmina_pref.view_file_mode);
	g_key_file_set_string(gkeyfile, "remmina_pref", "resolutions", remmina_pref.resolutions);
	g_key_file_set_string(gkeyfile, "remmina_pref", "keystrokes", remmina_pref.keystrokes);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "main_width", remmina_pref.main_width);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "main_height", remmina_pref.main_height);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "main_maximize", remmina_pref.main_maximize);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "main_sort_column_id", remmina_pref.main_sort_column_id);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "main_sort_order", remmina_pref.main_sort_order);
	g_key_file_set_string(gkeyfile, "remmina_pref", "expanded_group", remmina_pref.expanded_group);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "toolbar_pin_down", remmina_pref.toolbar_pin_down);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "sshtunnel_port", remmina_pref.sshtunnel_port);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "applet_new_ontop", remmina_pref.applet_new_ontop);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "applet_hide_count", remmina_pref.applet_hide_count);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "applet_enable_avahi", remmina_pref.applet_enable_avahi);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "disable_tray_icon", remmina_pref.disable_tray_icon);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "dark_tray_icon", remmina_pref.dark_tray_icon);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "recent_maximum", remmina_pref.recent_maximum);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "default_mode", remmina_pref.default_mode);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "tab_mode", remmina_pref.tab_mode);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "fullscreen_toolbar_visibility", remmina_pref.fullscreen_toolbar_visibility);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "show_buttons_icons", remmina_pref.show_buttons_icons);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "show_menu_icons", remmina_pref.show_menu_icons);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "auto_scroll_step", remmina_pref.auto_scroll_step);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "hostkey", remmina_pref.hostkey);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_fullscreen", remmina_pref.shortcutkey_fullscreen);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_autofit", remmina_pref.shortcutkey_autofit);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_nexttab", remmina_pref.shortcutkey_nexttab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_prevtab", remmina_pref.shortcutkey_prevtab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_scale", remmina_pref.shortcutkey_scale);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_grab", remmina_pref.shortcutkey_grab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_screenshot", remmina_pref.shortcutkey_screenshot);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_minimize", remmina_pref.shortcutkey_minimize);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_disconnect", remmina_pref.shortcutkey_disconnect);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_toolbar", remmina_pref.shortcutkey_toolbar);
	g_key_file_set_string(gkeyfile, "remmina_pref", "vte_font", remmina_pref.vte_font ? remmina_pref.vte_font : "");
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "vte_allow_bold_text", remmina_pref.vte_allow_bold_text);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_lines", remmina_pref.vte_lines);
	g_key_file_set_boolean (gkeyfile, "remmina_pref", "vte_system_colors", remmina_pref.vte_system_colors);
	g_key_file_set_string(gkeyfile, "remmina_pref", "vte_foreground_color", remmina_pref.vte_foreground_color ? remmina_pref.vte_foreground_color : "");
	g_key_file_set_string(gkeyfile, "remmina_pref", "vte_background_color", remmina_pref.vte_background_color ? remmina_pref.vte_background_color : "");

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
	g_free(content);
}

void remmina_pref_add_recent(const gchar *protocol, const gchar *server)
{
	TRACE_CALL("remmina_pref_add_recent");
	RemminaStringArray *array;
	GKeyFile *gkeyfile;
	gchar key[20];
	gchar *val;
	gchar *content;
	gsize length;

	if (remmina_pref.recent_maximum <= 0 || server == NULL || server[0] == 0)
		return;

	/* Load original value into memory */
	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	g_snprintf(key, sizeof(key), "recent_%s", protocol);
	array = remmina_string_array_new_from_allocated_string(g_key_file_get_string(gkeyfile, "remmina_pref", key, NULL));

	/* Add the new value */
	remmina_string_array_remove(array, server);
	while (array->len >= remmina_pref.recent_maximum)
	{
		remmina_string_array_remove_index(array, 0);
	}
	remmina_string_array_add(array, server);

	/* Save */
	val = remmina_string_array_to_string(array);
	g_key_file_set_string(gkeyfile, "remmina_pref", key, val);
	g_free(val);

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
	g_free(content);
}

gchar*
remmina_pref_get_recent(const gchar *protocol)
{
	TRACE_CALL("remmina_pref_get_recent");
	GKeyFile *gkeyfile;
	gchar key[20];
	gchar *val;

	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	g_snprintf(key, sizeof(key), "recent_%s", protocol);
	val = g_key_file_get_string(gkeyfile, "remmina_pref", key, NULL);

	g_key_file_free(gkeyfile);

	return val;
}

void remmina_pref_clear_recent(void)
{
	TRACE_CALL("remmina_pref_clear_recent");
	GKeyFile *gkeyfile;
	gchar **keys;
	gint i;
	gchar *content;
	gsize length;

	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	keys = g_key_file_get_keys(gkeyfile, "remmina_pref", NULL, NULL);
	if (keys)
	{
		for (i = 0; keys[i]; i++)
		{
			if (strncmp(keys[i], "recent_", 7) == 0)
			{
				g_key_file_set_string(gkeyfile, "remmina_pref", keys[i], "");
			}
		}
		g_strfreev(keys);
	}

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
	g_free(content);
}

guint remmina_pref_keymap_get_keyval(const gchar *keymap, guint keyval)
{
	TRACE_CALL("remmina_pref_keymap_get_keyval");
	guint *table;
	gint i;

	if (!keymap || keymap[0] == '\0')
		return keyval;

	table = (guint*) g_hash_table_lookup(remmina_keymap_table, keymap);
	if (!table)
		return keyval;
	for (i = 0; table[i] > 0; i += 2)
	{
		if (table[i] == keyval)
			return table[i + 1];
	}
	return keyval;
}

gchar**
remmina_pref_keymap_groups(void)
{
	TRACE_CALL("remmina_pref_keymap_groups");
	GList *list;
	guint len;
	gchar **keys;
	guint i;

	list = g_hash_table_get_keys(remmina_keymap_table);
	len = g_list_length(list);

	keys = g_new0 (gchar*, (len + 1) * 2 + 1);
	keys[0] = g_strdup("");
	keys[1] = g_strdup("");
	for (i = 0; i < len; i++)
	{
		keys[(i + 1) * 2] = g_strdup((gchar*) g_list_nth_data(list, i));
		keys[(i + 1) * 2 + 1] = g_strdup((gchar*) g_list_nth_data(list, i));
	}
	g_list_free(list);

	return keys;
}

gint remmina_pref_get_scale_quality(void)
{
	TRACE_CALL("remmina_pref_get_scale_quality");
	/* Paranoid programming */
	if (remmina_pref.scale_quality < 0)
	{
		remmina_pref.scale_quality = 0;
	}
	return remmina_pref.scale_quality;
}

gint remmina_pref_get_ssh_loglevel(void)
{
	TRACE_CALL("remmina_pref_get_ssh_loglevel");
	return remmina_pref.ssh_loglevel;
}

gboolean remmina_pref_get_ssh_parseconfig(void)
{
	TRACE_CALL("remmina_pref_get_ssh_parseconfig");
	return remmina_pref.ssh_parseconfig;
}

gint remmina_pref_get_sshtunnel_port(void)
{
	TRACE_CALL("remmina_pref_get_sshtunnel_port");
	return remmina_pref.sshtunnel_port;
}

void remmina_pref_set_value(const gchar *key, const gchar *value)
{
	TRACE_CALL("remmina_pref_set_value");
	GKeyFile *gkeyfile;
	gchar *content;
	gsize length;

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	g_key_file_set_string(gkeyfile, "remmina_pref", key, value);
	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
	g_free(content);
}

gchar*
remmina_pref_get_value(const gchar *key)
{
	TRACE_CALL("remmina_pref_get_value");
	GKeyFile *gkeyfile;
	gchar *value;

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	value = g_key_file_get_string(gkeyfile, "remmina_pref", key, NULL);
	g_key_file_free(gkeyfile);

	return value;
}
