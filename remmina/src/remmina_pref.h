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

#ifndef __REMMINAPREF_H__
#define __REMMINAPREF_H__

/*
 * Remmina Perference Loader
 */

G_BEGIN_DECLS

enum
{
	REMMINA_VIEW_FILE_LIST,
	REMMINA_VIEW_FILE_TREE
};

enum
{
	REMMINA_ACTION_CONNECT = 0,
	REMMINA_ACTION_EDIT = 1
};

enum
{
	AUTO_MODE = 0,
	SCROLLED_WINDOW_MODE = 1,
	FULLSCREEN_MODE = 2,
	SCROLLED_FULLSCREEN_MODE = 3,
	VIEWPORT_FULLSCREEN_MODE = 4
};

enum
{
	REMMINA_TAB_BY_GROUP = 0,
	REMMINA_TAB_BY_PROTOCOL = 1,
	REMMINA_TAB_ALL = 8,
	REMMINA_TAB_NONE = 9
};

typedef struct _RemminaPref
{
	/* In RemminaPrefDialog */
	gboolean save_view_mode;
	gboolean save_when_connect;
	gboolean invisible_toolbar;
	gboolean always_show_tab;
	gboolean hide_connection_toolbar;
	gint default_action;
	gint scale_quality;
	gchar *resolutions;
	gint sshtunnel_port;
	gint recent_maximum;
	gint default_mode;
	gint tab_mode;
	gint auto_scroll_step;

	gboolean applet_new_ontop;
	gboolean applet_hide_count;
	gboolean applet_enable_avahi;
	gboolean disable_tray_icon;
	gboolean minimize_to_tray;

	guint hostkey;
	guint shortcutkey_fullscreen;
	guint shortcutkey_autofit;
	guint shortcutkey_nexttab;
	guint shortcutkey_prevtab;
	guint shortcutkey_scale;
	guint shortcutkey_grab;
	guint shortcutkey_minimize;
	guint shortcutkey_disconnect;
	guint shortcutkey_toolbar;

	/* In View menu */
	gboolean hide_toolbar;
	gboolean hide_statusbar;
	gboolean show_quick_search;
	gboolean small_toolbutton;
	gint view_file_mode;

	/* Auto */
	gint main_width;
	gint main_height;
	gboolean main_maximize;
	gint main_sort_column_id;
	gint main_sort_order;
	gchar *expanded_group;
	gboolean toolbar_pin_down;

	/* Crypto */
	gchar *secret;

	/* VTE */
	gchar *vte_font;
	gboolean vte_allow_bold_text;
	gint vte_lines;
	guint vte_shortcutkey_copy;
	guint vte_shortcutkey_paste;
} RemminaPref;

#define DEFAULT_SSHTUNNEL_PORT 4732
#define DEFAULT_SSH_PORT 22

extern const gchar *default_resolutions;
extern gchar *remmina_pref_file;
extern RemminaPref remmina_pref;

void remmina_pref_init(void);
void remmina_pref_save(void);

void remmina_pref_add_recent(const gchar *protocol, const gchar *server);
gchar* remmina_pref_get_recent(const gchar *protocol);
void remmina_pref_clear_recent(void);

guint remmina_pref_keymap_get_keyval(const gchar *keymap, guint keyval);
gchar** remmina_pref_keymap_groups(void);

gint remmina_pref_get_scale_quality(void);
gint remmina_pref_get_sshtunnel_port(void);

void remmina_pref_set_value(const gchar *key, const gchar *value);
gchar* remmina_pref_get_value(const gchar *key);

G_END_DECLS

#endif  /* __REMMINAPREF_H__  */

