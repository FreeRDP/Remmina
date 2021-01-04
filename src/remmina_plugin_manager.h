/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
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

#include "remmina/plugin.h"

G_BEGIN_DECLS

typedef gboolean (*RemminaPluginFunc)(gchar *name, RemminaPlugin *plugin, gpointer data);

void remmina_plugin_manager_init(void);
RemminaPlugin *remmina_plugin_manager_get_plugin(RemminaPluginType type, const gchar *name);
gboolean remmina_plugin_manager_query_feature_by_type(RemminaPluginType ptype, const gchar *name, RemminaProtocolFeatureType ftype);
void remmina_plugin_manager_for_each_plugin(RemminaPluginType type, RemminaPluginFunc func, gpointer data);
void remmina_plugin_manager_show(GtkWindow *parent);
void remmina_plugin_manager_for_each_plugin_stdout(RemminaPluginType type, RemminaPluginFunc func, gpointer data);
void remmina_plugin_manager_show_stdout();
RemminaFilePlugin *remmina_plugin_manager_get_import_file_handler(const gchar *file);
RemminaFilePlugin *remmina_plugin_manager_get_export_file_handler(RemminaFile *remminafile);
RemminaSecretPlugin *remmina_plugin_manager_get_secret_plugin(void);
const gchar *remmina_plugin_manager_get_canonical_setting_name(const RemminaProtocolSetting *setting);
gboolean remmina_plugin_manager_is_encrypted_setting(RemminaProtocolPlugin *pp, const char *setting);
gboolean remmina_gtksocket_available();

extern RemminaPluginService remmina_plugin_manager_service;

typedef gboolean (*RemminaPluginLoaderFunc)(RemminaPluginService*, const gchar* name);

typedef struct {
    char* filetype;
    RemminaPluginLoaderFunc func;
}  RemminaPluginLoader;

gboolean remmina_plugin_manager_supported(const char *filetype);
void remmina_plugin_manager_add_loader(char *filetype, RemminaPluginLoaderFunc func);

G_END_DECLS
