/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINA_RDP_FILE_H__
#define __REMMINA_RDP_FILE_H__

G_BEGIN_DECLS

gboolean remmina_rdp_file_import_test(const gchar* from_file);
RemminaFile* remmina_rdp_file_import(const gchar* from_file);
gboolean remmina_rdp_file_export_test(RemminaFile* remminafile);
gboolean remmina_rdp_file_export(RemminaFile* remminafile, const gchar* to_file);

G_END_DECLS

#endif

