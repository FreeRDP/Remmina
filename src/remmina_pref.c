/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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

#include <glib/gstdio.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "remmina_sodium.h"

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
	TRACE_CALL(__func__);
	guchar s[32];
	gint i;
	GKeyFile *gkeyfile;
	g_autofree gchar *content = NULL;
	gsize length;

	for (i = 0; i < 32; i++)
		s[i] = (guchar)(randombytes_uniform(257));
	remmina_pref.secret = g_base64_encode(s, 32);

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	g_key_file_set_string(gkeyfile, "remmina_pref", "secret", remmina_pref.secret);
	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
}

static guint remmina_pref_get_keyval_from_str(const gchar *str)
{
	TRACE_CALL(__func__);
	guint k;

	if (!str)
		return 0;

	k = gdk_keyval_from_name(str);
	if (!k)
		if (sscanf(str, "%x", &k) < 1)
			k = 0;
	return k;
}

static void remmina_pref_init_keymap(void)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gchar **groups;
	gchar **gptr;
	gchar **keys;
	gchar **kptr;
	gsize nkeys;
	g_autofree gchar *value = NULL;
	guint *table;
	guint *tableptr;
	guint k1, k2;

	if (remmina_keymap_table)
		g_hash_table_destroy(remmina_keymap_table);
	remmina_keymap_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);

	gkeyfile = g_key_file_new();
	if (!g_key_file_load_from_file(gkeyfile, remmina_keymap_file, G_KEY_FILE_NONE, NULL)) {
		if (!g_key_file_load_from_data(gkeyfile, default_keymap_data, strlen(default_keymap_data), G_KEY_FILE_NONE,
					       NULL)) {
			g_print("Failed to initialize keymap table\n");
			g_key_file_free(gkeyfile);
			return;
		}
	}

	groups = g_key_file_get_groups(gkeyfile, NULL);
	gptr = groups;
	while (*gptr) {
		keys = g_key_file_get_keys(gkeyfile, *gptr, &nkeys, NULL);
		table = g_new0(guint, nkeys * 2 + 1);
		g_hash_table_insert(remmina_keymap_table, g_strdup(*gptr), table);

		kptr = keys;
		tableptr = table;
		while (*kptr) {
			k1 = remmina_pref_get_keyval_from_str(*kptr);
			if (k1) {
				value = g_key_file_get_string(gkeyfile, *gptr, *kptr, NULL);
				k2 = remmina_pref_get_keyval_from_str(value);
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

/** @todo remmina_pref_file_do_copy and remmina_file_manager_do_copy to remmina_files_copy */
static gboolean remmina_pref_file_do_copy(const char *src_path, const char *dst_path)
{
	GFile *src = g_file_new_for_path(src_path), *dst = g_file_new_for_path(dst_path);
	/* We don’t overwrite the target if it exists, because overwrite is not set */
	const gboolean ok = g_file_copy(src, dst, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);

	g_object_unref(dst);
	g_object_unref(src);

	return ok;
}

void remmina_pref_file_load_colors(GKeyFile *gkeyfile, RemminaColorPref *color_pref)
{
	const struct {
		const char *	name;
		char **		setting;
		char *		fallback;
	} colors[] = {
		{ "background",		  &color_pref->background,	     "#d5ccba" },
		{ "cursor",		  &color_pref->cursor,		     "#45373c" },
		{ "cursor_foreground",	  &color_pref->cursor_foreground,    "#d5ccba" },
		{ "highlight",		  &color_pref->highlight,	     "#45373c" },
		{ "highlight_foreground", &color_pref->highlight_foreground, "#d5ccba" },
		{ "colorBD",		  &color_pref->colorBD,		     "#45373c" },
		{ "foreground",		  &color_pref->foreground,	     "#45373c" },
		{ "color0",		  &color_pref->color0,		     "#20111b" },
		{ "color1",		  &color_pref->color1,		     "#be100e" },
		{ "color2",		  &color_pref->color2,		     "#858162" },
		{ "color3",		  &color_pref->color3,		     "#eaa549" },
		{ "color4",		  &color_pref->color4,		     "#426a79" },
		{ "color5",		  &color_pref->color5,		     "#97522c" },
		{ "color6",		  &color_pref->color6,		     "#989a9c" },
		{ "color7",		  &color_pref->color7,		     "#968c83" },
		{ "color8",		  &color_pref->color8,		     "#5e5252" },
		{ "color9",		  &color_pref->color9,		     "#be100e" },
		{ "color10",		  &color_pref->color10,		     "#858162" },
		{ "color11",		  &color_pref->color11,		     "#eaa549" },
		{ "color12",		  &color_pref->color12,		     "#426a79" },
		{ "color13",		  &color_pref->color13,		     "#97522c" },
		{ "color14",		  &color_pref->color14,		     "#989a9c" },
		{ "color15",		  &color_pref->color15,		     "#d5ccba" },
	};

	int i;

	for (i = 0; i < (sizeof(colors) / sizeof(colors[0])); i++) {
		if (g_key_file_has_key(gkeyfile, "ssh_colors", colors[i].name, NULL))
			*colors[i].setting = g_key_file_get_string(gkeyfile, "ssh_colors", colors[i].name,
								   NULL);
		else
			*colors[i].setting = colors[i].fallback;
	}
}

void remmina_pref_init(void)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gchar *remmina_dir;
	const gchar *filename = "remmina.pref";
	const gchar *colors_filename = "remmina.colors";
	g_autofree gchar *remmina_colors_file = NULL;
	GDir *dir;
	const gchar *legacy = ".remmina";
	int i;

	remmina_dir = g_build_path("/", g_get_user_config_dir(), "remmina", NULL);
	/* Create the XDG_CONFIG_HOME directory */
	g_mkdir_with_parents(remmina_dir, 0750);

	g_free(remmina_dir), remmina_dir = NULL;
	/* Legacy ~/.remmina we copy the existing remmina.pref file inside
	 * XDG_CONFIG_HOME */
	remmina_dir = g_build_path("/", g_get_home_dir(), legacy, NULL);
	if (g_file_test(remmina_dir, G_FILE_TEST_IS_DIR)) {
		dir = g_dir_open(remmina_dir, 0, NULL);
		remmina_pref_file_do_copy(
			g_build_path("/", remmina_dir, filename, NULL),
			g_build_path("/", g_get_user_config_dir(),
				     "remmina", filename, NULL));
	}

	/* /usr/local/etc/remmina */
	const gchar *const *dirs = g_get_system_config_dirs();

	g_free(remmina_dir), remmina_dir = NULL;
	for (i = 0; dirs[i] != NULL; ++i) {
		remmina_dir = g_build_path("/", dirs[i], "remmina", NULL);
		if (g_file_test(remmina_dir, G_FILE_TEST_IS_DIR)) {
			dir = g_dir_open(remmina_dir, 0, NULL);
			while ((filename = g_dir_read_name(dir)) != NULL) {
				remmina_pref_file_do_copy(
					g_build_path("/", remmina_dir, filename, NULL),
					g_build_path("/", g_get_user_config_dir(),
						     "remmina", filename, NULL));
			}
			g_free(remmina_dir), remmina_dir = NULL;
		}
	}

	/* The last case we use  the home ~/.config/remmina */
	if (remmina_dir != NULL)
		g_free(remmina_dir), remmina_dir = NULL;
	remmina_dir = g_build_path("/", g_get_user_config_dir(),
				   "remmina", NULL);

	remmina_pref_file = g_strdup_printf("%s/remmina.pref", remmina_dir);
	/* remmina.colors */
	remmina_colors_file = g_strdup_printf("%s/%s", remmina_dir, colors_filename);

	remmina_keymap_file = g_strdup_printf("%s/remmina.keymap", remmina_dir);

	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "save_view_mode", NULL))
		remmina_pref.save_view_mode = g_key_file_get_boolean(gkeyfile, "remmina_pref", "save_view_mode", NULL);
	else
		remmina_pref.save_view_mode = TRUE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "use_master_password", NULL))
		remmina_pref.use_master_password = g_key_file_get_boolean(gkeyfile, "remmina_pref", "use_master_password", NULL);
	else
		remmina_pref.use_master_password = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "unlock_timeout", NULL))
		remmina_pref.unlock_timeout = g_key_file_get_integer(gkeyfile, "remmina_pref", "unlock_timeout", NULL);
	else
		remmina_pref.unlock_timeout = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "unlock_password", NULL))
		remmina_pref.unlock_password = g_key_file_get_string(gkeyfile, "remmina_pref", "unlock_password", NULL);
	else
		remmina_pref.unlock_password = g_strdup("");

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "trust_all", NULL))
		remmina_pref.trust_all = g_key_file_get_boolean(gkeyfile, "remmina_pref", "trust_all", NULL);
	else
		remmina_pref.trust_all = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "fullscreen_on_auto", NULL))
		remmina_pref.fullscreen_on_auto = g_key_file_get_boolean(gkeyfile, "remmina_pref", "fullscreen_on_auto", NULL);
	else
		remmina_pref.fullscreen_on_auto = TRUE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "floating_toolbar_placement", NULL))
		remmina_pref.floating_toolbar_placement = g_key_file_get_integer(gkeyfile, "remmina_pref", "floating_toolbar_placement", NULL);
	else
		remmina_pref.floating_toolbar_placement = FLOATING_TOOLBAR_PLACEMENT_TOP;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "prevent_snap_welcome_message", NULL))
		remmina_pref.prevent_snap_welcome_message = g_key_file_get_boolean(gkeyfile, "remmina_pref", "prevent_snap_welcome_message", NULL);
	else
		remmina_pref.prevent_snap_welcome_message = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "last_quickconnect_protocol", NULL))
		remmina_pref.last_quickconnect_protocol = g_key_file_get_string(gkeyfile, "remmina_pref", "last_quickconnect_protocol", NULL);
	else
		remmina_pref.last_quickconnect_protocol = g_strdup("");

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

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "hide_searchbar", NULL))
		remmina_pref.hide_searchbar = g_key_file_get_boolean(gkeyfile, "remmina_pref",
								     "hide_searchbar", NULL);
	else
		remmina_pref.hide_searchbar = FALSE;

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

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "deny_screenshot_clipboard", NULL))
		remmina_pref.deny_screenshot_clipboard = g_key_file_get_boolean(gkeyfile, "remmina_pref", "deny_screenshot_clipboard", NULL);
	else
		remmina_pref.deny_screenshot_clipboard = TRUE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "datadir_path", NULL))
		remmina_pref.datadir_path = g_key_file_get_string(gkeyfile, "remmina_pref", "datadir_path", NULL);
	else
		remmina_pref.datadir_path = g_strdup("");

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "remmina_file_name", NULL))
		remmina_pref.remmina_file_name = g_key_file_get_string(gkeyfile, "remmina_pref", "remmina_file_name", NULL);
	else
		remmina_pref.remmina_file_name = g_strdup("%G_%P_%N_%h");

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "screenshot_path", NULL)) {
		remmina_pref.screenshot_path = g_key_file_get_string(gkeyfile, "remmina_pref", "screenshot_path", NULL);
	} else {
		remmina_pref.screenshot_path = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
		if (remmina_pref.screenshot_path == NULL)
			remmina_pref.screenshot_path = g_get_home_dir();
	}

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "screenshot_name", NULL))
		remmina_pref.screenshot_name = g_key_file_get_string(gkeyfile, "remmina_pref", "screenshot_name", NULL);
	else
		remmina_pref.screenshot_name = g_strdup("remmina_%p_%h_%Y%m%d-%H%M%S");

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "ssh_parseconfig", NULL))
		remmina_pref.ssh_parseconfig = g_key_file_get_boolean(gkeyfile, "remmina_pref", "ssh_parseconfig", NULL);
	else
		remmina_pref.ssh_parseconfig = DEFAULT_SSH_PARSECONFIG;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "sshtunnel_port", NULL))
		remmina_pref.sshtunnel_port = g_key_file_get_integer(gkeyfile, "remmina_pref", "sshtunnel_port", NULL);
	else
		remmina_pref.sshtunnel_port = DEFAULT_SSHTUNNEL_PORT;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "ssh_tcp_keepidle", NULL))
		remmina_pref.ssh_tcp_keepidle = g_key_file_get_integer(gkeyfile, "remmina_pref", "ssh_tcp_keepidle", NULL);
	else
		remmina_pref.ssh_tcp_keepidle = SSH_SOCKET_TCP_KEEPIDLE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "ssh_tcp_keepintvl", NULL))
		remmina_pref.ssh_tcp_keepintvl = g_key_file_get_integer(gkeyfile, "remmina_pref", "ssh_tcp_keepintvl", NULL);
	else
		remmina_pref.ssh_tcp_keepintvl = SSH_SOCKET_TCP_KEEPINTVL;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "ssh_tcp_keepcnt", NULL))
		remmina_pref.ssh_tcp_keepcnt = g_key_file_get_integer(gkeyfile, "remmina_pref", "ssh_tcp_keepcnt", NULL);
	else
		remmina_pref.ssh_tcp_keepcnt = SSH_SOCKET_TCP_KEEPCNT;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "ssh_tcp_usrtimeout", NULL))
		remmina_pref.ssh_tcp_usrtimeout = g_key_file_get_integer(gkeyfile, "remmina_pref", "ssh_tcp_usrtimeout", NULL);
	else
		remmina_pref.ssh_tcp_usrtimeout = SSH_SOCKET_TCP_USER_TIMEOUT;

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

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "dark_theme", NULL))
		remmina_pref.dark_theme = g_key_file_get_boolean(gkeyfile, "remmina_pref", "dark_theme", NULL);
	else
		remmina_pref.dark_theme = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "grab_color_switch", NULL))
		remmina_pref.grab_color_switch = g_key_file_get_boolean(gkeyfile, "remmina_pref", "grab_color_switch", NULL);
	else
		remmina_pref.grab_color_switch = FALSE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "grab_color", NULL))
		remmina_pref.grab_color = g_key_file_get_string(gkeyfile, "remmina_pref", "grab_color", NULL);
	else
		remmina_pref.grab_color = "#00ff00";

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

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_viewonly", NULL))
		remmina_pref.shortcutkey_viewonly = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_viewonly", NULL);
	else
		remmina_pref.shortcutkey_viewonly = GDK_KEY_m;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "shortcutkey_multimon", NULL))
		remmina_pref.shortcutkey_multimon = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_multimon", NULL);
	else
		remmina_pref.shortcutkey_multimon = GDK_KEY_Page_Up;

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

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_font", NULL))
		remmina_pref.vte_font = g_key_file_get_string(gkeyfile, "remmina_pref", "vte_font", NULL);
	else
		remmina_pref.vte_font = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_allow_bold_text", NULL))
		remmina_pref.vte_allow_bold_text = g_key_file_get_boolean(gkeyfile, "remmina_pref", "vte_allow_bold_text",
									  NULL);
	else
		remmina_pref.vte_allow_bold_text = TRUE;

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

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_shortcutkey_increase_font", NULL))
		remmina_pref.vte_shortcutkey_increase_font = g_key_file_get_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_increase_font",
										    NULL);
	else
		remmina_pref.vte_shortcutkey_increase_font = GDK_KEY_Page_Up;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_shortcutkey_decrease_font", NULL))
		remmina_pref.vte_shortcutkey_decrease_font = g_key_file_get_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_decrease_font",
										    NULL);
	else
		remmina_pref.vte_shortcutkey_decrease_font = GDK_KEY_Page_Down;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_shortcutkey_search_text", NULL))
		remmina_pref.vte_shortcutkey_search_text = g_key_file_get_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_search_text",
										  NULL);
	else
		remmina_pref.vte_shortcutkey_search_text = GDK_KEY_g;


	remmina_pref_file_load_colors(gkeyfile, &remmina_pref.color_pref);

	if (g_key_file_has_key(gkeyfile, "usage_stats", "periodic_usage_stats_permitted", NULL))
		remmina_pref.periodic_usage_stats_permitted = g_key_file_get_boolean(gkeyfile, "usage_stats", "periodic_usage_stats_permitted", NULL);
	else
		remmina_pref.periodic_usage_stats_permitted = FALSE;

	if (g_key_file_has_key(gkeyfile, "usage_stats", "periodic_usage_stats_last_sent", NULL))
		remmina_pref.periodic_usage_stats_last_sent = g_key_file_get_int64(gkeyfile, "usage_stats", "periodic_usage_stats_last_sent", NULL);
	else
		remmina_pref.periodic_usage_stats_last_sent = 0;

	if (g_key_file_has_key(gkeyfile, "usage_stats", "periodic_usage_stats_uuid_prefix", NULL))
		remmina_pref.periodic_usage_stats_uuid_prefix = g_key_file_get_string(gkeyfile, "usage_stats", "periodic_usage_stats_uuid_prefix", NULL);
	else
		remmina_pref.periodic_usage_stats_uuid_prefix = NULL;

	/** RMNEWS_ENABLE_NEWS is equal to 0 (FALSE) when compiled with -DWiTH_NEWS=OFF,
	 * otherwise is value is 1 (TRUE), that is the default value
	 */
	if (RMNEWS_ENABLE_NEWS == 0)
		remmina_pref.periodic_news_permitted = RMNEWS_ENABLE_NEWS;
	else if (g_key_file_has_key(gkeyfile, "remmina_news", "periodic_news_permitted", NULL))
		remmina_pref.periodic_news_permitted = g_key_file_get_boolean(gkeyfile, "remmina_news", "periodic_news_permitted", NULL);
	else
		remmina_pref.periodic_news_permitted = RMNEWS_ENABLE_NEWS;


	if (g_key_file_has_key(gkeyfile, "remmina_news", "periodic_rmnews_last_get", NULL)) {
		remmina_pref.periodic_rmnews_last_get = g_key_file_get_int64(gkeyfile, "remmina_news", "periodic_rmnews_last_get", NULL);
		g_debug("(%s) - periodic_rmnews_last_get set to %ld", __func__, remmina_pref.periodic_rmnews_last_get);
	} else {
		remmina_pref.periodic_rmnews_last_get = 0;
		g_debug("(%s) - periodic_rmnews_last_get set to 0", __func__);
	}

	if (g_key_file_has_key(gkeyfile, "remmina_news", "periodic_rmnews_get_count", NULL))
		remmina_pref.periodic_rmnews_get_count = g_key_file_get_int64(gkeyfile, "remmina_news", "periodic_rmnews_get_count", NULL);
	else
		remmina_pref.periodic_rmnews_get_count = 0;

	if (g_key_file_has_key(gkeyfile, "remmina_news", "periodic_rmnews_uuid_prefix", NULL))
		remmina_pref.periodic_rmnews_uuid_prefix = g_key_file_get_string(gkeyfile, "remmina_news", "periodic_rmnews_uuid_prefix", NULL);
	else
		remmina_pref.periodic_rmnews_uuid_prefix = NULL;

	/* If we have a color scheme file, we switch to it, GIO will merge it in the
	 * remmina.pref file */
	if (g_file_test(remmina_colors_file, G_FILE_TEST_IS_REGULAR)) {
		g_key_file_load_from_file(gkeyfile, remmina_colors_file, G_KEY_FILE_NONE, NULL);
		g_remove(remmina_colors_file);
	}

	/* Default settings */
	if (!g_key_file_has_key(gkeyfile, "remmina", "name", NULL)) {
		g_key_file_set_string(gkeyfile, "remmina", "name", "");
		g_key_file_set_integer(gkeyfile, "remmina", "ignore-tls-errors", 1);
		g_key_file_set_integer(gkeyfile, "remmina", "enable-plugins", 1);
		remmina_pref_save();
	}

	g_key_file_free(gkeyfile);

	if (remmina_pref.secret == NULL)
		remmina_pref_gen_secret();

	remmina_pref_init_keymap();
}

gboolean remmina_pref_is_rw(void)
{
	TRACE_CALL(__func__);
	if (access(remmina_pref_file, W_OK) == 0)
		return TRUE;
	else
		return FALSE;
	return FALSE;
}

gboolean remmina_pref_save(void)
{
	TRACE_CALL(__func__);

	if (remmina_pref_is_rw() == FALSE) {
		g_debug("remmina.pref is not writable, returning");
		return FALSE;
	}
	GKeyFile *gkeyfile;
	GError *error = NULL;
	g_autofree gchar *content = NULL;
	gsize length;

	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	g_key_file_set_string(gkeyfile, "remmina_pref", "datadir_path", remmina_pref.datadir_path);
	g_key_file_set_string(gkeyfile, "remmina_pref", "remmina_file_name", remmina_pref.remmina_file_name);
	g_key_file_set_string(gkeyfile, "remmina_pref", "screenshot_path", remmina_pref.screenshot_path);
	g_key_file_set_string(gkeyfile, "remmina_pref", "screenshot_name", remmina_pref.screenshot_name);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "deny_screenshot_clipboard", remmina_pref.deny_screenshot_clipboard);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "save_view_mode", remmina_pref.save_view_mode);
#if SODIUM_VERSION_INT >= 90200
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "use_master_password", remmina_pref.use_master_password);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "unlock_timeout", remmina_pref.unlock_timeout);
	g_key_file_set_string(gkeyfile, "remmina_pref", "unlock_password", remmina_pref.unlock_password);
#else
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "use_master_password", FALSE);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "unlock_timeout", 0);
	g_key_file_set_string(gkeyfile, "remmina_pref", "unlock_password", g_strdup(""));
#endif
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "trust_all", remmina_pref.trust_all);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "floating_toolbar_placement", remmina_pref.floating_toolbar_placement);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "toolbar_placement", remmina_pref.toolbar_placement);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "prevent_snap_welcome_message", remmina_pref.prevent_snap_welcome_message);
	g_key_file_set_string(gkeyfile, "remmina_pref", "last_quickconnect_protocol", remmina_pref.last_quickconnect_protocol);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "fullscreen_on_auto", remmina_pref.fullscreen_on_auto);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "always_show_tab", remmina_pref.always_show_tab);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_connection_toolbar", remmina_pref.hide_connection_toolbar);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_searchbar", remmina_pref.hide_searchbar);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "default_action", remmina_pref.default_action);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "scale_quality", remmina_pref.scale_quality);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "ssh_loglevel", remmina_pref.ssh_loglevel);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "ssh_parseconfig", remmina_pref.ssh_parseconfig);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_toolbar", remmina_pref.hide_toolbar);
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
	g_key_file_set_integer(gkeyfile, "remmina_pref", "ssh_tcp_keepidle", remmina_pref.ssh_tcp_keepidle);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "ssh_tcp_keepintvl", remmina_pref.ssh_tcp_keepintvl);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "ssh_tcp_keepcnt", remmina_pref.ssh_tcp_keepcnt);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "ssh_tcp_usrtimeout", remmina_pref.ssh_tcp_usrtimeout);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "applet_new_ontop", remmina_pref.applet_new_ontop);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "applet_hide_count", remmina_pref.applet_hide_count);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "applet_enable_avahi", remmina_pref.applet_enable_avahi);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "disable_tray_icon", remmina_pref.disable_tray_icon);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "dark_theme", remmina_pref.dark_theme);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "recent_maximum", remmina_pref.recent_maximum);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "default_mode", remmina_pref.default_mode);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "tab_mode", remmina_pref.tab_mode);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "fullscreen_toolbar_visibility", remmina_pref.fullscreen_toolbar_visibility);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "auto_scroll_step", remmina_pref.auto_scroll_step);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "hostkey", remmina_pref.hostkey);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_fullscreen", remmina_pref.shortcutkey_fullscreen);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_autofit", remmina_pref.shortcutkey_autofit);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_nexttab", remmina_pref.shortcutkey_nexttab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_prevtab", remmina_pref.shortcutkey_prevtab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_scale", remmina_pref.shortcutkey_scale);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_grab", remmina_pref.shortcutkey_grab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_multimon", remmina_pref.shortcutkey_multimon);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_viewonly", remmina_pref.shortcutkey_viewonly);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_screenshot", remmina_pref.shortcutkey_screenshot);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_minimize", remmina_pref.shortcutkey_minimize);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_disconnect", remmina_pref.shortcutkey_disconnect);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_toolbar", remmina_pref.shortcutkey_toolbar);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_copy", remmina_pref.vte_shortcutkey_copy);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_paste", remmina_pref.vte_shortcutkey_paste);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_select_all", remmina_pref.vte_shortcutkey_select_all);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_increase_font", remmina_pref.vte_shortcutkey_increase_font);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_decrease_font", remmina_pref.vte_shortcutkey_decrease_font);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_shortcutkey_search_text", remmina_pref.vte_shortcutkey_search_text);
	g_key_file_set_string(gkeyfile, "remmina_pref", "vte_font", remmina_pref.vte_font ? remmina_pref.vte_font : "");
	g_key_file_set_string(gkeyfile, "remmina_pref", "grab_color", remmina_pref.grab_color ? remmina_pref.grab_color : "");
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "grab_color_switch", remmina_pref.grab_color_switch);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "vte_allow_bold_text", remmina_pref.vte_allow_bold_text);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_lines", remmina_pref.vte_lines);
	g_key_file_set_string(gkeyfile, "ssh_colors", "background", remmina_pref.color_pref.background ? remmina_pref.color_pref.background : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "cursor", remmina_pref.color_pref.cursor ? remmina_pref.color_pref.cursor : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "cursor_foreground", remmina_pref.color_pref.cursor_foreground ? remmina_pref.color_pref.cursor_foreground : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "highlight", remmina_pref.color_pref.highlight ? remmina_pref.color_pref.highlight : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "highlight_foreground", remmina_pref.color_pref.highlight_foreground ? remmina_pref.color_pref.highlight_foreground : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "colorBD", remmina_pref.color_pref.colorBD ? remmina_pref.color_pref.colorBD : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "foreground", remmina_pref.color_pref.foreground ? remmina_pref.color_pref.foreground : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color0", remmina_pref.color_pref.color0 ? remmina_pref.color_pref.color0 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color1", remmina_pref.color_pref.color1 ? remmina_pref.color_pref.color1 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color2", remmina_pref.color_pref.color2 ? remmina_pref.color_pref.color2 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color3", remmina_pref.color_pref.color3 ? remmina_pref.color_pref.color3 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color4", remmina_pref.color_pref.color4 ? remmina_pref.color_pref.color4 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color5", remmina_pref.color_pref.color5 ? remmina_pref.color_pref.color5 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color6", remmina_pref.color_pref.color6 ? remmina_pref.color_pref.color6 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color7", remmina_pref.color_pref.color7 ? remmina_pref.color_pref.color7 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color8", remmina_pref.color_pref.color8 ? remmina_pref.color_pref.color8 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color9", remmina_pref.color_pref.color9 ? remmina_pref.color_pref.color9 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color10", remmina_pref.color_pref.color10 ? remmina_pref.color_pref.color10 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color11", remmina_pref.color_pref.color11 ? remmina_pref.color_pref.color11 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color12", remmina_pref.color_pref.color12 ? remmina_pref.color_pref.color12 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color13", remmina_pref.color_pref.color13 ? remmina_pref.color_pref.color13 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color14", remmina_pref.color_pref.color14 ? remmina_pref.color_pref.color14 : "");
	g_key_file_set_string(gkeyfile, "ssh_colors", "color15", remmina_pref.color_pref.color15 ? remmina_pref.color_pref.color15 : "");

	g_key_file_set_boolean(gkeyfile, "usage_stats", "periodic_usage_stats_permitted", remmina_pref.periodic_usage_stats_permitted);
	g_key_file_set_int64(gkeyfile, "usage_stats", "periodic_usage_stats_last_sent", remmina_pref.periodic_usage_stats_last_sent);
	g_key_file_set_string(gkeyfile, "usage_stats", "periodic_usage_stats_uuid_prefix",
			      remmina_pref.periodic_usage_stats_uuid_prefix ? remmina_pref.periodic_usage_stats_uuid_prefix : "");
	g_key_file_set_boolean(gkeyfile, "remmina_news", "periodic_news_permitted", remmina_pref.periodic_news_permitted);
	g_debug("(%s) - Setting periodic_rmnews_last_get to %ld", __func__, remmina_pref.periodic_rmnews_last_get);
	g_key_file_set_int64(gkeyfile, "remmina_news", "periodic_rmnews_last_get", remmina_pref.periodic_rmnews_last_get);
	g_key_file_set_integer(gkeyfile, "remmina_news", "periodic_rmnews_get_count", remmina_pref.periodic_rmnews_get_count);
	g_key_file_set_string(gkeyfile, "remmina_news", "periodic_rmnews_uuid_prefix",
			      remmina_pref.periodic_rmnews_uuid_prefix ? remmina_pref.periodic_rmnews_uuid_prefix : "");

	/* Default settings */
	g_key_file_set_string(gkeyfile, "remmina", "name", "");
	g_key_file_set_integer(gkeyfile, "remmina", "ignore-tls-errors", 1);

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, &error);

	if (error != NULL) {
		g_warning("remmina_pref_save error: %s", error->message);
		g_clear_error(&error);
		g_key_file_free(gkeyfile);
		return FALSE;
	}
	g_key_file_free(gkeyfile);
	return TRUE;
}

void remmina_pref_add_recent(const gchar *protocol, const gchar *server)
{
	TRACE_CALL(__func__);
	RemminaStringArray *array;
	GKeyFile *gkeyfile;
	gchar key[20];
	g_autofree gchar *val = NULL;
	g_autofree gchar *content = NULL;
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
		remmina_string_array_remove_index(array, 0);
	remmina_string_array_add(array, server);

	/* Save */
	val = remmina_string_array_to_string(array);
	g_key_file_set_string(gkeyfile, "remmina_pref", key, val);

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
}

gchar *
remmina_pref_get_recent(const gchar *protocol)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gchar key[20];
	gchar *val = NULL;

	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	g_snprintf(key, sizeof(key), "recent_%s", protocol);
	val = g_key_file_get_string(gkeyfile, "remmina_pref", key, NULL);

	g_key_file_free(gkeyfile);

	return val;
}

void remmina_pref_clear_recent(void)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gchar **keys;
	gint i;
	g_autofree gchar *content = NULL;
	gsize length;

	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	keys = g_key_file_get_keys(gkeyfile, "remmina_pref", NULL, NULL);
	if (keys) {
		for (i = 0; keys[i]; i++)
			if (strncmp(keys[i], "recent_", 7) == 0)
				g_key_file_set_string(gkeyfile, "remmina_pref", keys[i], "");
		g_strfreev(keys);
	}

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
}

guint remmina_pref_keymap_get_keyval(const gchar *keymap, guint keyval)
{
	TRACE_CALL(__func__);
	guint *table;
	gint i;

	if (!keymap || keymap[0] == '\0')
		return keyval;

	table = (guint *)g_hash_table_lookup(remmina_keymap_table, keymap);
	if (!table)
		return keyval;
	for (i = 0; table[i] > 0; i += 2)
		if (table[i] == keyval)
			return table[i + 1];
	return keyval;
}

gchar **
remmina_pref_keymap_groups(void)
{
	TRACE_CALL(__func__);
	GList *list;
	guint len;
	gchar **keys;
	guint i;

	list = g_hash_table_get_keys(remmina_keymap_table);
	len = g_list_length(list);

	keys = g_new0(gchar *, (len + 1) * 2 + 1);
	keys[0] = g_strdup("");
	keys[1] = g_strdup("");
	for (i = 0; i < len; i++) {
		keys[(i + 1) * 2] = g_strdup((gchar *)g_list_nth_data(list, i));
		keys[(i + 1) * 2 + 1] = g_strdup((gchar *)g_list_nth_data(list, i));
	}
	g_list_free(list);

	return keys;
}

gint remmina_pref_get_scale_quality(void)
{
	TRACE_CALL(__func__);
	/* Paranoid programming */
	if (remmina_pref.scale_quality < 0)
		remmina_pref.scale_quality = 0;
	return remmina_pref.scale_quality;
}

gint remmina_pref_get_ssh_loglevel(void)
{
	TRACE_CALL(__func__);
	return remmina_pref.ssh_loglevel;
}

gboolean remmina_pref_get_ssh_parseconfig(void)
{
	TRACE_CALL(__func__);
	return remmina_pref.ssh_parseconfig;
}

gint remmina_pref_get_sshtunnel_port(void)
{
	TRACE_CALL(__func__);
	return remmina_pref.sshtunnel_port;
}

gint remmina_pref_get_ssh_tcp_keepidle(void)
{
	TRACE_CALL(__func__);
	return remmina_pref.ssh_tcp_keepidle;
}

gint remmina_pref_get_ssh_tcp_keepintvl(void)
{
	TRACE_CALL(__func__);
	return remmina_pref.ssh_tcp_keepintvl;
}

gint remmina_pref_get_ssh_tcp_keepcnt(void)
{
	TRACE_CALL(__func__);
	return remmina_pref.ssh_tcp_keepcnt;
}

gint remmina_pref_get_ssh_tcp_usrtimeout(void)
{
	TRACE_CALL(__func__);
	return remmina_pref.ssh_tcp_usrtimeout;
}

void remmina_pref_set_value(const gchar *key, const gchar *value)
{
	TRACE_CALL(__func__);
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

gchar *remmina_pref_get_value(const gchar *key)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gchar *value = NULL;

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	value = g_key_file_get_string(gkeyfile, "remmina_pref", key, NULL);
	g_key_file_free(gkeyfile);

	return value;
}

gboolean remmina_pref_get_boolean(const gchar *key)
{
	TRACE_CALL(__func__);
	GKeyFile *gkeyfile;
	gboolean value;

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	value = g_key_file_get_boolean(gkeyfile, "remmina_pref", key, NULL);
	g_key_file_free(gkeyfile);

	return value;
}
