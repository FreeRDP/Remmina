/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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

#pragma once

/*
 * Remmina Perference Loader
 */

G_BEGIN_DECLS

enum {
	REMMINA_VIEW_FILE_LIST,
	REMMINA_VIEW_FILE_TREE
};

enum {
	REMMINA_ACTION_CONNECT	= 0,
	REMMINA_ACTION_EDIT	= 1
};

enum {
	UNDEFINED_MODE			= 0,
	SCROLLED_WINDOW_MODE		= 1,
	FULLSCREEN_MODE			= 2,
	SCROLLED_FULLSCREEN_MODE	= 3,
	VIEWPORT_FULLSCREEN_MODE	= 4
};

enum {
	FLOATING_TOOLBAR_PLACEMENT_TOP		= 0,
	FLOATING_TOOLBAR_PLACEMENT_BOTTOM	= 1
};

enum {
	TOOLBAR_PLACEMENT_TOP		= 0,
	TOOLBAR_PLACEMENT_RIGHT		= 1,
	TOOLBAR_PLACEMENT_BOTTOM	= 2,
	TOOLBAR_PLACEMENT_LEFT		= 3
};

enum {
	REMMINA_TAB_BY_GROUP	= 0,
	REMMINA_TAB_BY_PROTOCOL = 1,
	REMMINA_TAB_ALL		= 2,
	REMMINA_TAB_NONE	= 3
};

enum {
	FLOATING_TOOLBAR_VISIBILITY_PEEKING	= 0,
	FLOATING_TOOLBAR_VISIBILITY_INVISIBLE	= 1, //"Invisible" corresponds to the "Hidden" option in the drop-down
	FLOATING_TOOLBAR_VISIBILITY_DISABLE	= 2
};

typedef struct _RemminaColorPref {
	/* Color palette for VTE terminal */
	gchar * background;
	gchar * cursor;
	gchar * foreground;
	gchar * color0;
	gchar * color1;
	gchar * color2;
	gchar * color3;
	gchar * color4;
	gchar * color5;
	gchar * color6;
	gchar * color7;
	gchar * color8;
	gchar * color9;
	gchar * color10;
	gchar * color11;
	gchar * color12;
	gchar * color13;
	gchar * color14;
	gchar * color15;
} RemminaColorPref;

typedef struct _RemminaPref {
	/* In RemminaPrefDialog options tab */
	const gchar *		datadir_path;
	const gchar *		remmina_file_name;
	const gchar *		screenshot_path;
	gboolean		deny_screenshot_clipboard;
	const gchar *		screenshot_name;
	gboolean		save_view_mode;
	gboolean		use_master_password;
	const gchar *		unlock_password;
	const gchar *		unlock_repassword;
	gint			unlock_timeout;
	gint			default_action;
	gint			scale_quality;
	gint			auto_scroll_step;
	gint			recent_maximum;
	gchar *			resolutions;
	gchar *			keystrokes;
	/* In RemminaPrefDialog appearance tab */
	gboolean		fullscreen_on_auto;
	gboolean		always_show_tab;
	gboolean		hide_connection_toolbar;
	gboolean		hide_searchbar;
	gint			default_mode;
	gint			tab_mode;
	gint			fullscreen_toolbar_visibility;
	/* In RemminaPrefDialog applet tab */
	gboolean		applet_new_ontop;
	gboolean		applet_hide_count;
	gboolean		disable_tray_icon;
	gboolean		dark_tray_icon;
	/* In RemminaPrefDialog SSH Option tab */
	gint			ssh_loglevel;
	gboolean		ssh_parseconfig;
	gint			sshtunnel_port;
	gint			ssh_tcp_keepidle;
	gint			ssh_tcp_keepintvl;
	gint			ssh_tcp_keepcnt;
	gint			ssh_tcp_usrtimeout;
	/* In RemminaPrefDialog keyboard tab */
	guint			hostkey;
	guint			shortcutkey_fullscreen;
	guint			shortcutkey_autofit;
	guint			shortcutkey_prevtab;
	guint			shortcutkey_nexttab;
	guint			shortcutkey_dynres;
	guint			shortcutkey_scale;
	guint			shortcutkey_grab;
	guint			shortcutkey_viewonly;
	guint			shortcutkey_screenshot;
	guint			shortcutkey_minimize;
	guint			shortcutkey_disconnect;
	guint			shortcutkey_toolbar;
	/* In RemminaPrefDialog terminal tab */
	gchar *			vte_font;
	gboolean		vte_allow_bold_text;
	gboolean		vte_system_colors;
	gint			vte_lines;
	guint			vte_shortcutkey_copy;
	guint			vte_shortcutkey_paste;
	guint			vte_shortcutkey_select_all;
	/* In View menu */
	gboolean		hide_toolbar;
	gboolean		small_toolbutton;
	gint			view_file_mode;
	/* In tray icon */
	gboolean		applet_enable_avahi;
	/* Auto */
	gint			main_width;
	gint			main_height;
	gboolean		main_maximize;
	gint			main_sort_column_id;
	gint			main_sort_order;
	gchar *			expanded_group;
	gboolean		toolbar_pin_down;
	gint			floating_toolbar_placement;
	gint			toolbar_placement;
	gboolean		prevent_snap_welcome_message;
	guint64			suppress_xinput_welcome_message_time_limit;
	gchar *			last_quickconnect_protocol;

	/* Crypto */
	gchar *			secret;

	/* UID */
	gchar *			uid;

	RemminaColorPref	color_pref;

	/* Usage stats */
	gboolean		periodic_usage_stats_permitted;
	glong			periodic_usage_stats_last_sent;
	gchar *			periodic_usage_stats_uuid_prefix;
	gchar *			last_success;

	/* Remmina news */
	glong			periodic_rmnews_last_get;
	glong			periodic_rmnews_get_count;
	gchar *			periodic_rmnews_uuid_prefix;
} RemminaPref;

#define DEFAULT_SSH_PARSECONFIG TRUE
#define DEFAULT_SSHTUNNEL_PORT 4732
#define DEFAULT_SSH_PORT 22
#define DEFAULT_SSH_LOGLEVEL 1
#define SSH_SOCKET_TCP_KEEPIDLE 20
#define SSH_SOCKET_TCP_KEEPINTVL 10
#define SSH_SOCKET_TCP_KEEPCNT 3
#define SSH_SOCKET_TCP_USER_TIMEOUT 60000 // 60 seconds

extern const gchar *default_resolutions;
extern gchar *remmina_pref_file;
extern gchar *remmina_colors_file;
extern RemminaPref remmina_pref;

void remmina_pref_init(void);
gboolean remmina_pref_is_rw(void);
gboolean remmina_pref_save(void);

void remmina_pref_add_recent(const gchar *protocol, const gchar *server);
gchar *remmina_pref_get_recent(const gchar *protocol);
void remmina_pref_clear_recent(void);

guint remmina_pref_keymap_get_keyval(const gchar *keymap, guint keyval);
gchar **remmina_pref_keymap_groups(void);

gint remmina_pref_get_scale_quality(void);
gint remmina_pref_get_ssh_loglevel(void);
gboolean remmina_pref_get_ssh_parseconfig(void);
gint remmina_pref_get_sshtunnel_port(void);
void remmina_pref_file_load_colors(GKeyFile *gkeyfile, RemminaColorPref *color_pref);
gint remmina_pref_get_ssh_tcp_keepidle(void);
gint remmina_pref_get_ssh_tcp_keepintvl(void);
gint remmina_pref_get_ssh_tcp_keepcnt(void);
gint remmina_pref_get_ssh_tcp_usrtimeout(void);

void remmina_pref_set_value(const gchar *key, const gchar *value);
gchar *remmina_pref_get_value(const gchar *key);
gboolean remmina_pref_get_boolean(const gchar *key);

G_END_DECLS
