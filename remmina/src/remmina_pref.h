/*  See LICENSE and COPYING files for copyright and license details. */

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
	FLOATING_TOOLBAR_PLACEMENT_TOP, FLOATING_TOOLBAR_PLACEMENT_BOTTOM
};

enum
{
	REMMINA_TAB_BY_GROUP = 0,
	REMMINA_TAB_BY_PROTOCOL = 1,
	REMMINA_TAB_ALL = 2,
	REMMINA_TAB_NONE = 3
};

typedef struct _RemminaPref
{
	/* In RemminaPrefDialog options tab */
	gboolean save_view_mode;
	gboolean save_when_connect;
	gint default_action;
	gint scale_quality;
	gint sshtunnel_port;
	gint auto_scroll_step;
	gint recent_maximum;
	gchar *resolutions;
	gchar *keystrokes;
	/* In RemminaPrefDialog appearance tab */
	gboolean invisible_toolbar;
	gint floating_toolbar_placement;
	gboolean always_show_tab;
	gboolean hide_connection_toolbar;
	gint default_mode;
	gint tab_mode;
	gint show_buttons_icons;
	gint show_menu_icons;
	/* In RemminaPrefDialog applet tab */
	gboolean applet_new_ontop;
	gboolean applet_hide_count;
	gboolean disable_tray_icon;
	/* In RemminaPrefDialog keyboard tab */
	guint hostkey;
	guint shortcutkey_fullscreen;
	guint shortcutkey_autofit;
	guint shortcutkey_prevtab;
	guint shortcutkey_nexttab;
	guint shortcutkey_scale;
	guint shortcutkey_grab;
	guint shortcutkey_minimize;
	guint shortcutkey_disconnect;
	guint shortcutkey_toolbar;
	/* In RemminaPrefDialog terminal tab */
	gchar *vte_font;
	gboolean vte_allow_bold_text;
	gboolean vte_system_colors;
	gchar *vte_foreground_color;
	gchar *vte_background_color;
	gint vte_lines;
	guint vte_shortcutkey_copy;
	guint vte_shortcutkey_paste;
	/* In View menu */
	gboolean hide_toolbar;
	gboolean hide_statusbar;
	gboolean show_quick_search;
	gboolean hide_quick_connect;
	gboolean small_toolbutton;
	gint view_file_mode;
	/* In tray icon */
	gboolean applet_enable_avahi;
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

