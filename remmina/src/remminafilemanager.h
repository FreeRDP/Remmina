/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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
 

#ifndef __REMMINAFILEMANAGER_H__
#define __REMMINAFILEMANAGER_H__

#include "remminafile.h"

G_BEGIN_DECLS

/* Initialize */
void remmina_file_manager_init (void);
/* Iterate all .remmina connections in the home directory */
gint remmina_file_manager_iterate (GFunc func, gpointer user_data);
/* Get a list of groups */
gchar* remmina_file_manager_get_groups (void);
/* Load or import a file */
RemminaFile* remmina_file_manager_load_file (const gchar *filename);

G_END_DECLS

#endif  /* __REMMINAFILEMANAGER_H__  */

