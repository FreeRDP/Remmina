/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

#pragma once

#include "remmina/plugin.h"
#include "json-glib/json-glib.h"


#define DOWNLOAD_URL "https://plugins.remmina.org/plugins/plugin_download"
#define LIST_URL "https://plugins.remmina.org/plugins/get_list"

#define ON_DOWNLOAD 1
#define ON_CLOSE 2
#define FAIL 0
#define PARTIAL_SUCCESS 1
#define SUCCESS 2
#define MAX_PLUGIN_NAME_SIZE 100
#define MAX_PLUGINS_ALLOWED_TO_DOWNLOAD 20

G_BEGIN_DECLS

typedef gboolean (*RemminaPluginFunc)(gchar *name, RemminaPlugin *plugin, gpointer data);

void remmina_plugin_manager_init(void);
JsonNode *remmina_plugin_manager_get_installed_plugins(void);
RemminaPlugin *remmina_plugin_manager_get_plugin(RemminaPluginType type, const gchar *name);
gboolean remmina_plugin_manager_query_feature_by_type(RemminaPluginType ptype, const gchar *name, RemminaProtocolFeatureType ftype);
void remmina_plugin_manager_for_each_plugin(RemminaPluginType type, RemminaPluginFunc func, gpointer data);
void remmina_plugin_manager_show(GtkWindow *parent);
void remmina_plugin_manager_for_each_plugin_stdout(RemminaPluginType type, RemminaPluginFunc func, gpointer data);
void remmina_plugin_manager_show_stdout();
void* remmina_plugin_manager_get_available_plugins();
gboolean remmina_plugin_manager_parse_plugin_list(gpointer user_data);
gboolean remmina_plugin_manager_download_plugins(gpointer user_data);
RemminaFilePlugin *remmina_plugin_manager_get_import_file_handler(const gchar *file);
RemminaFilePlugin *remmina_plugin_manager_get_export_file_handler(RemminaFile *remminafile);
RemminaSecretPlugin *remmina_plugin_manager_get_secret_plugin(void);
const gchar *remmina_plugin_manager_get_canonical_setting_name(const RemminaProtocolSetting *setting);
gboolean remmina_plugin_manager_is_encrypted_setting(RemminaProtocolPlugin *pp, const char *setting);
gboolean remmina_gtksocket_available();
gboolean remmina_plugin_manager_verify_duplicate_plugins(RemminaPlugin *test_plugin);
void remmina_plugin_manager_toggle_checkbox(GtkCellRendererToggle *cell, gchar *path, GtkListStore *model);
void remmina_plugin_manager_on_response(GtkDialog *dialog, gint response_id, gpointer user_data);
void remmina_append_json_objects_from_response_str(JsonReader* response_str, GArray* data_array);
guint remmina_plugin_manager_deserialize_plugin_response(GArray *name_array);
gboolean remmina_attempt_to_write_plugin_data_to_disk(RemminaServerPluginResponse *plugin);
GFile* remmina_create_plugin_file(const gchar* plugin_name, const gchar* plugin_version);
JsonNode *remmina_plugin_manager_plugin_stats_get_all(void);
extern RemminaPluginService remmina_plugin_manager_service;

typedef gboolean (*RemminaPluginLoaderFunc)(RemminaPluginService*, const gchar* name);

typedef struct {
    char* filetype;
    RemminaPluginLoaderFunc func;
}  RemminaPluginLoader;

typedef struct {
    GtkWidget *label;
    GtkListStore *store;
    GtkWidget *spinner;
    gboolean downloading;
} SignalData;

gboolean remmina_plugin_manager_loader_supported(const char *filetype);
void remmina_plugin_manager_add_loader(char *filetype, RemminaPluginLoaderFunc func);

G_END_DECLS
