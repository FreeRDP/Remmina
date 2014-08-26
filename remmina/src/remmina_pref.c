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
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <string.h>
#include "remmina_string_array.h"
#include "remmina_pref.h"

const gchar *default_resolutions = "640x480,800x600,1024x768,1152x864,1280x960,1400x1050";

gchar *remmina_pref_file;
RemminaPref remmina_pref;

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
}

void remmina_pref_init(void)
{
	GKeyFile *gkeyfile;

	remmina_pref_file = g_strdup_printf("%s/.remmina/remmina.pref", g_get_home_dir());
	remmina_keymap_file = g_strdup_printf("%s/.remmina/remmina.keymap", g_get_home_dir());

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "save_view_mode", NULL))
		remmina_pref.save_view_mode = g_key_file_get_boolean(gkeyfile, "remmina_pref", "save_view_mode", NULL);
	else
		remmina_pref.save_view_mode = TRUE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "save_when_connect", NULL))
		remmina_pref.save_when_connect = g_key_file_get_boolean(gkeyfile, "remmina_pref", "save_when_connect", NULL);
	else
		remmina_pref.save_when_connect = TRUE;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "invisible_toolbar", NULL))
		remmina_pref.invisible_toolbar = g_key_file_get_boolean(gkeyfile, "remmina_pref", "invisible_toolbar", NULL);
	else
		remmina_pref.invisible_toolbar = FALSE;

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

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "show_quick_search", NULL))
		remmina_pref.show_quick_search = g_key_file_get_boolean(gkeyfile, "remmina_pref", "show_quick_search", NULL);
	else
		remmina_pref.show_quick_search = FALSE;

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

#ifdef ENABLE_MINIMIZE_TO_TRAY
	if (g_key_file_has_key(gkeyfile, "remmina_pref", "minimize_to_tray", NULL))
		remmina_pref.minimize_to_tray = g_key_file_get_boolean(gkeyfile, "remmina_pref", "minimize_to_tray", NULL);
	else
		remmina_pref.minimize_to_tray = FALSE;
#endif

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
		remmina_pref.shortcutkey_minimize = g_key_file_get_integer(gkeyfile, "remmina_pref", "shortcutkey_minimize",
				NULL);
	else
		remmina_pref.shortcutkey_minimize = GDK_KEY_F9;

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

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_font", NULL))
		remmina_pref.vte_font = g_key_file_get_string(gkeyfile, "remmina_pref", "vte_font", NULL);
	else
		remmina_pref.vte_font = NULL;

	if (g_key_file_has_key(gkeyfile, "remmina_pref", "vte_allow_bold_text", NULL))
		remmina_pref.vte_allow_bold_text = g_key_file_get_integer(gkeyfile, "remmina_pref", "vte_allow_bold_text",
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

	g_key_file_free(gkeyfile);

	if (remmina_pref.secret == NULL)
		remmina_pref_gen_secret();

	remmina_pref_init_keymap();
}

void remmina_pref_save(void)
{
	GKeyFile *gkeyfile;
	gchar *content;
	gsize length;

	gkeyfile = g_key_file_new();

	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

	g_key_file_set_boolean(gkeyfile, "remmina_pref", "save_view_mode", remmina_pref.save_view_mode);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "save_when_connect", remmina_pref.save_when_connect);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "invisible_toolbar", remmina_pref.invisible_toolbar);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "always_show_tab", remmina_pref.always_show_tab);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_connection_toolbar", remmina_pref.hide_connection_toolbar);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "default_action", remmina_pref.default_action);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "scale_quality", remmina_pref.scale_quality);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_toolbar", remmina_pref.hide_toolbar);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "hide_statusbar", remmina_pref.hide_statusbar);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "show_quick_search", remmina_pref.show_quick_search);
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "small_toolbutton", remmina_pref.small_toolbutton);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "view_file_mode", remmina_pref.view_file_mode);
	g_key_file_set_string(gkeyfile, "remmina_pref", "resolutions", remmina_pref.resolutions);
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
#ifdef ENABLE_MINIMIZE_TO_TRAY
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "minimize_to_tray", remmina_pref.minimize_to_tray);
#else
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "minimize_to_tray", FALSE);
#endif
	g_key_file_set_integer(gkeyfile, "remmina_pref", "recent_maximum", remmina_pref.recent_maximum);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "default_mode", remmina_pref.default_mode);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "tab_mode", remmina_pref.tab_mode);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "auto_scroll_step", remmina_pref.auto_scroll_step);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "hostkey", remmina_pref.hostkey);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_fullscreen", remmina_pref.shortcutkey_fullscreen);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_autofit", remmina_pref.shortcutkey_autofit);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_nexttab", remmina_pref.shortcutkey_nexttab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_prevtab", remmina_pref.shortcutkey_prevtab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_scale", remmina_pref.shortcutkey_scale);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_grab", remmina_pref.shortcutkey_grab);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_minimize", remmina_pref.shortcutkey_minimize);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_disconnect", remmina_pref.shortcutkey_disconnect);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "shortcutkey_toolbar", remmina_pref.shortcutkey_toolbar);
	g_key_file_set_string(gkeyfile, "remmina_pref", "vte_font", remmina_pref.vte_font ? remmina_pref.vte_font : "");
	g_key_file_set_boolean(gkeyfile, "remmina_pref", "vte_allow_bold_text", remmina_pref.vte_allow_bold_text);
	g_key_file_set_integer(gkeyfile, "remmina_pref", "vte_lines", remmina_pref.vte_lines);

	content = g_key_file_to_data(gkeyfile, &length, NULL);
	g_file_set_contents(remmina_pref_file, content, length, NULL);

	g_key_file_free(gkeyfile);
	g_free(content);
}

void remmina_pref_add_recent(const gchar *protocol, const gchar *server)
{
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
	return remmina_pref.scale_quality;
}

gint remmina_pref_get_sshtunnel_port(void)
{
	return remmina_pref.sshtunnel_port;
}

void remmina_pref_set_value(const gchar *key, const gchar *value)
{
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
	GKeyFile *gkeyfile;
	gchar *value;

	gkeyfile = g_key_file_new();
	g_key_file_load_from_file(gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
	value = g_key_file_get_string(gkeyfile, "remmina_pref", key, NULL);
	g_key_file_free(gkeyfile);

	return value;
}

