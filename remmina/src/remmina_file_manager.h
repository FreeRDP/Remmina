/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAFILEMANAGER_H__
#define __REMMINAFILEMANAGER_H__

#include "remmina_file.h"

G_BEGIN_DECLS

typedef struct _RemminaGroupData
{
	gchar *name;
	gchar *group;
} RemminaGroupData;

/* Initialize */
void remmina_file_manager_init(void);
/* Iterate all .remmina connections in the home directory */
gint remmina_file_manager_iterate(GFunc func, gpointer user_data);
/* Get a list of groups */
gchar* remmina_file_manager_get_groups(void);
GNode* remmina_file_manager_get_group_tree(void);
void remmina_file_manager_free_group_tree(GNode *node);
/* Load or import a file */
RemminaFile* remmina_file_manager_load_file(const gchar *filename);

G_END_DECLS

#endif  /* __REMMINAFILEMANAGER_H__  */

