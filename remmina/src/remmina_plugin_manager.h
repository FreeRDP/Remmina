/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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
 */

#ifndef __REMMINAPLUGINMANAGER_H__
#define __REMMINAPLUGINMANAGER_H__

#include "remmina/plugin.h"

G_BEGIN_DECLS

typedef gboolean (*RemminaPluginFunc)(gchar *name, RemminaPlugin *plugin, gpointer data);

void remmina_plugin_manager_init(void);
RemminaPlugin* remmina_plugin_manager_get_plugin(RemminaPluginType type, const gchar *name);
void remmina_plugin_manager_for_each_plugin(RemminaPluginType type, RemminaPluginFunc func, gpointer data);
void remmina_plugin_manager_show(GtkWindow *parent);
RemminaFilePlugin* remmina_plugin_manager_get_import_file_handler(const gchar *file);
RemminaFilePlugin* remmina_plugin_manager_get_export_file_handler(RemminaFile *remminafile);
RemminaSecretPlugin* remmina_plugin_manager_get_secret_plugin(void);

extern RemminaPluginService remmina_plugin_manager_service;

G_END_DECLS

#endif /* __REMMINAPLUGINMANAGER_H__ */

