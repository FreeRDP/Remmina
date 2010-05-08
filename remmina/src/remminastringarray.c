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

#include <glib.h>
#include <string.h>
#include "remminastringarray.h"

RemminaStringArray*
remmina_string_array_new (void)
{
    return g_ptr_array_new ();
}

RemminaStringArray*
remmina_string_array_new_from_string (const gchar *strs)
{
    RemminaStringArray *array;
    gchar *buf, *ptr1, *ptr2;

    array = remmina_string_array_new ();
    if (!strs || strs[0] == '\0') return array;

    buf = g_strdup (strs);
    ptr1 = buf;
    while (ptr1)
    {
        ptr2 = strchr (ptr1, ',');
        if (ptr2) *ptr2++ = '\0';
        remmina_string_array_add (array, ptr1);
        ptr1 = ptr2;
    }
    
    g_free (buf);

    return array;
}

RemminaStringArray*
remmina_string_array_new_from_allocated_string (gchar *strs)
{
    RemminaStringArray *array;
    array = remmina_string_array_new_from_string (strs);
    g_free (strs);
    return array;
}

void
remmina_string_array_add (RemminaStringArray* array, const gchar *str)
{
    g_ptr_array_add (array, g_strdup (str));
}

gint
remmina_string_array_find (RemminaStringArray* array, const gchar *str)
{
    gint i;

    for (i = 0; i < array->len; i++)
    {
        if (g_strcmp0 (remmina_string_array_index (array, i), str) == 0) return i;
    }
    return -1;
}

void remmina_string_array_remove_index (RemminaStringArray* array, gint i)
{
    g_ptr_array_remove_index (array, i);
}

void
remmina_string_array_remove (RemminaStringArray* array, const gchar *str)
{
    gint i;

    i = remmina_string_array_find (array, str);
    if (i >= 0)
    {
        remmina_string_array_remove_index (array, i);
    }
}

void
remmina_string_array_intersect (RemminaStringArray* array, const gchar *dest_strs)
{
    RemminaStringArray *dest_array;
    gint i, j;

    dest_array = remmina_string_array_new_from_string (dest_strs);

    i = 0;
    while (i < array->len)
    {
        j = remmina_string_array_find (dest_array, remmina_string_array_index (array, i));
        if (j < 0)
        {
            remmina_string_array_remove_index (array, i);
            continue;
        }
        i++;
    }

    remmina_string_array_free (dest_array);
}

static gint
remmina_string_array_compare_func (const gchar **a, const gchar **b)
{
    return g_strcmp0 (*a, *b);
}

void
remmina_string_array_sort (RemminaStringArray *array)
{
    g_ptr_array_sort (array, (GCompareFunc) remmina_string_array_compare_func);
}

gchar*
remmina_string_array_to_string (RemminaStringArray* array)
{
    GString *gstr;
    gint i;

    gstr = g_string_new ("");
    for (i = 0; i < array->len; i++)
    {
        if (i > 0) g_string_append_c (gstr, ',');
        g_string_append (gstr, remmina_string_array_index (array, i));
    }
    return g_string_free (gstr, FALSE);
}

void
remmina_string_array_free (RemminaStringArray *array)
{
    g_ptr_array_foreach (array, (GFunc) g_free, NULL);
    g_ptr_array_free (array, TRUE);
}

