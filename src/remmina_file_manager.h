/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2017-2020 Antenore Gatta, Giovanni Panozzo
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

#include "remmina_file.h"

G_BEGIN_DECLS

typedef struct _RemminaGroupData {
	gchar * name;
	gchar * group;
	gchar * datetime;
} RemminaGroupData;

/* Initialize */
gchar *remmina_file_get_datadir(void);
void remmina_file_manager_init(void);
/* Iterate all .remmina connections in the home directory */
gint remmina_file_manager_iterate(GFunc func, gpointer user_data);
/* Get a list of groups */
gchar *remmina_file_manager_get_groups(void);
GNode *remmina_file_manager_get_group_tree(void);
void remmina_file_manager_free_group_tree(GNode *node);
/* Load or import a file */
RemminaFile *remmina_file_manager_load_file(const gchar *filename);

G_END_DECLS
