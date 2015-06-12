/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAPLUGINMANAGER_H__
#define __REMMINAPLUGINMANAGER_H__

#include "remmina/plugin.h"

G_BEGIN_DECLS

typedef gboolean (*RemminaPluginFunc)(gchar *name, RemminaPlugin *plugin, gpointer data);

void remmina_plugin_manager_init(void);
RemminaPlugin* remmina_plugin_manager_get_plugin(RemminaPluginType type, const gchar *name);
gboolean remmina_plugin_manager_query_feature_by_type(RemminaPluginType ptype, const gchar* name, RemminaProtocolFeatureType ftype);
void remmina_plugin_manager_for_each_plugin(RemminaPluginType type, RemminaPluginFunc func, gpointer data);
void remmina_plugin_manager_show(GtkWindow *parent);
RemminaFilePlugin* remmina_plugin_manager_get_import_file_handler(const gchar *file);
RemminaFilePlugin* remmina_plugin_manager_get_export_file_handler(RemminaFile *remminafile);
RemminaSecretPlugin* remmina_plugin_manager_get_secret_plugin(void);

extern RemminaPluginService remmina_plugin_manager_service;

G_END_DECLS

#endif /* __REMMINAPLUGINMANAGER_H__ */

