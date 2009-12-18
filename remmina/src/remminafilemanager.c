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

#include <gtk/gtk.h>
#include "remminapublic.h"
#include "remminastringarray.h"
#include "remminafilemanager.h"

void
remmina_file_manager_init (void)
{
    gchar dirname[MAX_PATH_LEN];

    g_snprintf (dirname, MAX_PATH_LEN, "%s/.remmina", g_get_home_dir ());
    g_mkdir_with_parents (dirname, 0700);
}

gint
remmina_file_manager_iterate (GFunc func, gpointer user_data)
{
    gchar dirname[MAX_PATH_LEN];
    gchar filename[MAX_PATH_LEN];
    GDir *dir;
    const gchar *name;
    RemminaFile *remminafile;
    gint n;

    g_snprintf (dirname, MAX_PATH_LEN, "%s/.remmina", g_get_home_dir ());
    dir = g_dir_open (dirname, 0, NULL);
    if (dir == NULL) return 0;

    n = 0;
    while ((name = g_dir_read_name (dir)) != NULL)
    {
        if (!g_str_has_suffix (name, ".remmina")) continue;
        g_snprintf (filename, MAX_PATH_LEN, "%s/%s", dirname, name);
        remminafile = remmina_file_load (filename);
        (*func) (remminafile, user_data);
        remmina_file_free (remminafile);
        n++;
    }
    g_dir_close (dir);
    return n;
}

gchar*
remmina_file_manager_get_groups (void)
{
    gchar dirname[MAX_PATH_LEN];
    gchar filename[MAX_PATH_LEN];
    GDir *dir;
    const gchar *name;
    RemminaFile *remminafile;
    RemminaStringArray *array;
    gchar *groups;

    array = remmina_string_array_new ();
    
    g_snprintf (dirname, MAX_PATH_LEN, "%s/.remmina", g_get_home_dir ());
    dir = g_dir_open (dirname, 0, NULL);
    if (dir == NULL) return 0;
    while ((name = g_dir_read_name (dir)) != NULL)
    {
        if (!g_str_has_suffix (name, ".remmina")) continue;
        g_snprintf (filename, MAX_PATH_LEN, "%s/%s", dirname, name);
        remminafile = remmina_file_load (filename);
        if (remminafile->group && remminafile->group[0] != '\0' && remmina_string_array_find (array, remminafile->group) < 0)
        {
            remmina_string_array_add (array, remminafile->group);
        }
        remmina_file_free (remminafile);
    }
    remmina_string_array_sort (array);
    groups = remmina_string_array_to_string (array);
    remmina_string_array_free (array);
    return groups;
}

