/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
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

#pragma once

#include <stdarg.h>
#include <remmina/types.h>
#include "remmina/remmina_trace_calls.h"

G_BEGIN_DECLS

typedef enum {
	REMMINA_PLUGIN_TYPE_PROTOCOL	= 0,
	REMMINA_PLUGIN_TYPE_ENTRY	= 1,
	REMMINA_PLUGIN_TYPE_FILE	= 2,
	REMMINA_PLUGIN_TYPE_TOOL	= 3,
	REMMINA_PLUGIN_TYPE_PREF	= 4,
	REMMINA_PLUGIN_TYPE_SECRET	= 5
} RemminaPluginType;

typedef struct _RemminaPlugin {
	RemminaPluginType	type;
	const gchar *		name;
	const gchar *		description;
	const gchar *		domain;
	const gchar *		version;
} RemminaPlugin;

typedef struct _RemminaProtocolPlugin _RemminaProtocolPlugin;
typedef struct _RemminaProtocolPlugin {
	RemminaPluginType		type;
	const gchar *			name;
	const gchar *			description;
	const gchar *			domain;
	const gchar *			version;

	const gchar *			icon_name;
	const gchar *			icon_name_ssh;
	const RemminaProtocolSetting *	basic_settings;
	const RemminaProtocolSetting *	advanced_settings;
	RemminaProtocolSSHSetting	ssh_setting;
	const RemminaProtocolFeature *	features;

	void (*init)(RemminaProtocolWidget *gp);
	gboolean (*open_connection)(RemminaProtocolWidget *gp);
	gboolean (*close_connection)(RemminaProtocolWidget *gp);
	gboolean (*query_feature)(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
	void (*call_feature)(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
	void (*send_keystrokes)(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen);
	gboolean (*get_plugin_screenshot)(RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd);
	gboolean (*map_event)(RemminaProtocolWidget *gp);
	gboolean (*unmap_event)(RemminaProtocolWidget *gp);
} RemminaProtocolPlugin;

typedef struct _RemminaEntryPlugin {
	RemminaPluginType	type;
	const gchar *		name;
	const gchar *		description;
	const gchar *		domain;
	const gchar *		version;

	void (*entry_func)(void);
} RemminaEntryPlugin;

typedef struct _RemminaFilePlugin {
	RemminaPluginType	type;
	const gchar *		name;
	const gchar *		description;
	const gchar *		domain;
	const gchar *		version;

	gboolean (*import_test_func)(const gchar *from_file);
	RemminaFile * (*import_func)(const gchar * from_file);
	gboolean (*export_test_func)(RemminaFile *file);
	gboolean (*export_func)(RemminaFile *file, const gchar *to_file);
	const gchar *		export_hints;
} RemminaFilePlugin;

typedef struct _RemminaToolPlugin {
	RemminaPluginType	type;
	const gchar *		name;
	const gchar *		description;
	const gchar *		domain;
	const gchar *		version;

	void (*exec_func)(void);
} RemminaToolPlugin;

typedef struct _RemminaPrefPlugin {
	RemminaPluginType	type;
	const gchar *		name;
	const gchar *		description;
	const gchar *		domain;
	const gchar *		version;

	const gchar *		pref_label;
	GtkWidget * (*get_pref_body)(void);
} RemminaPrefPlugin;

typedef struct _RemminaSecretPlugin {
	RemminaPluginType	type;
	const gchar *		name;
	const gchar *		description;
	const gchar *		domain;
	const gchar *		version;
	int			init_order;

	gboolean (*init)(void);
	gboolean (*is_service_available)(void);
	void (*store_password)(RemminaFile *remminafile, const gchar *key, const gchar *password);
	gchar * (*get_password)(RemminaFile * remminafile, const gchar *key);
	void (*delete_password)(RemminaFile *remminafile, const gchar *key);
} RemminaSecretPlugin;

/* Plugin Service is a struct containing a list of function pointers,
 * which is passed from Remmina main program to the plugin module
 * through the plugin entry function remmina_plugin_entry() */
typedef struct _RemminaPluginService {
	gboolean (*register_plugin)(RemminaPlugin *plugin);

	gint (*protocol_plugin_get_width)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_set_width)(RemminaProtocolWidget *gp, gint width);
	gint (*protocol_plugin_get_height)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_set_height)(RemminaProtocolWidget *gp, gint height);
	RemminaScaleMode (*remmina_protocol_widget_get_current_scale_mode)(RemminaProtocolWidget *gp);
	gboolean (*protocol_plugin_get_expand)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_set_expand)(RemminaProtocolWidget *gp, gboolean expand);
	gboolean (*protocol_plugin_has_error)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_set_error)(RemminaProtocolWidget *gp, const gchar *fmt, ...);
	gboolean (*protocol_plugin_is_closed)(RemminaProtocolWidget *gp);
	RemminaFile * (*protocol_plugin_get_file)(RemminaProtocolWidget * gp);
	void (*protocol_plugin_emit_signal)(RemminaProtocolWidget *gp, const gchar *signal_name);
	void (*protocol_plugin_register_hostkey)(RemminaProtocolWidget *gp, GtkWidget *widget);
	gchar *       (*protocol_plugin_start_direct_tunnel)(RemminaProtocolWidget * gp, gint default_port, gboolean port_plus);
	gboolean (*protocol_plugin_start_reverse_tunnel)(RemminaProtocolWidget *gp, gint local_port);
	gboolean (*protocol_plugin_start_xport_tunnel)(RemminaProtocolWidget *gp, RemminaXPortTunnelInitFunc init_func);
	void (*protocol_plugin_set_display)(RemminaProtocolWidget *gp, gint display);
	void (*protocol_plugin_signal_connection_closed)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_signal_connection_opened)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_update_align)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_lock_dynres)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_unlock_dynres)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_desktop_resize)(RemminaProtocolWidget *gp);
	gint (*protocol_plugin_init_auth)(RemminaProtocolWidget *gp, RemminaMessagePanelFlags pflags, const gchar *title, const gchar *default_username, const gchar *default_password, const gchar *default_domain, const gchar *password_prompt);
	gint (*protocol_plugin_init_certificate)(RemminaProtocolWidget *gp, const gchar *subject, const gchar *issuer, const gchar *fingerprint);
	gint (*protocol_plugin_changed_certificate)(RemminaProtocolWidget *gp, const gchar *subject, const gchar *issuer, const gchar *new_fingerprint, const gchar *old_fingerprint);
	gchar *       (*protocol_plugin_init_get_username)(RemminaProtocolWidget * gp);
	gchar *       (*protocol_plugin_init_get_password)(RemminaProtocolWidget * gp);
	gchar *       (*protocol_plugin_init_get_domain)(RemminaProtocolWidget * gp);
	gboolean (*protocol_plugin_init_get_savepassword)(RemminaProtocolWidget *gp);
	gint (*protocol_plugin_init_authx509)(RemminaProtocolWidget *gp);
	gchar *       (*protocol_plugin_init_get_cacert)(RemminaProtocolWidget * gp);
	gchar *       (*protocol_plugin_init_get_cacrl)(RemminaProtocolWidget * gp);
	gchar *       (*protocol_plugin_init_get_clientcert)(RemminaProtocolWidget * gp);
	gchar *       (*protocol_plugin_init_get_clientkey)(RemminaProtocolWidget * gp);
	void (*protocol_plugin_init_save_cred)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_init_show_listen)(RemminaProtocolWidget *gp, gint port);
	void (*protocol_plugin_init_show_retry)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_init_show)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_init_hide)(RemminaProtocolWidget *gp);
	gboolean (*protocol_plugin_ssh_exec)(RemminaProtocolWidget *gp, gboolean wait, const gchar *fmt, ...);
	void (*protocol_plugin_chat_open)(RemminaProtocolWidget *gp, const gchar *name, void (*on_send)(RemminaProtocolWidget *gp, const gchar *text), void (*on_destroy)(RemminaProtocolWidget *gp));
	void (*protocol_plugin_chat_close)(RemminaProtocolWidget *gp);
	void (*protocol_plugin_chat_receive)(RemminaProtocolWidget *gp, const gchar *text);
	void (*protocol_plugin_send_keys_signals)(GtkWidget *widget, const guint *keyvals, int length, GdkEventType action);

	gchar *       (*file_get_user_datadir)(void);

	RemminaFile * (*file_new)(void);
	const gchar * (*file_get_path)(RemminaFile * remminafile);
	void (*file_set_string)(RemminaFile *remminafile, const gchar *setting, const gchar *value);
	const gchar * (*file_get_string)(RemminaFile * remminafile, const gchar *setting);
	gchar *       (*file_get_secret)(RemminaFile * remminafile, const gchar *setting);
	void (*file_set_int)(RemminaFile *remminafile, const gchar *setting, gint value);
	gint (*file_get_int)(RemminaFile *remminafile, const gchar *setting, gint default_value);
	void (*file_unsave_passwords)(RemminaFile *remminafile);

	void (*pref_set_value)(const gchar *key, const gchar *value);
	gchar *       (*pref_get_value)(const gchar * key);
	gint (*pref_get_scale_quality)(void);
	gint (*pref_get_sshtunnel_port)(void);
	gint (*pref_get_ssh_loglevel)(void);
	gboolean (*pref_get_ssh_parseconfig)(void);
	guint (*pref_keymap_get_keyval)(const gchar *keymap, guint keyval);

	void (*_remmina_debug)(const gchar *func, const gchar *fmt, ...);
	void (*log_print)(const gchar *text);
	void (*log_printf)(const gchar *fmt, ...);

	void (*ui_register)(GtkWidget *widget);

	GtkWidget *   (*open_connection)(RemminaFile * remminafile, GCallback disconnect_cb, gpointer data, guint *handler);
	gint (*open_unix_sock)(const char *unixsock);
	void (*get_server_port)(const gchar *server, gint defaultport, gchar **host, gint *port);
	gboolean (*is_main_thread)(void);
	gboolean (*gtksocket_available)(void);
	gint (*get_profile_remote_width)(RemminaProtocolWidget *gp);
	gint (*get_profile_remote_height)(RemminaProtocolWidget *gp);
} RemminaPluginService;

/* "Prototype" of the plugin entry function */
typedef gboolean (*RemminaPluginEntryFunc) (RemminaPluginService *service);

G_END_DECLS
