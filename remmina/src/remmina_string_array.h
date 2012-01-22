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

#ifndef __REMMINASTRINGARRAY_H__
#define __REMMINASTRINGARRAY_H__

G_BEGIN_DECLS

typedef GPtrArray RemminaStringArray;

RemminaStringArray* remmina_string_array_new(void);
#define remmina_string_array_index(array,i) (gchar*)g_ptr_array_index(array,i)
RemminaStringArray* remmina_string_array_new_from_string(const gchar *strs);
RemminaStringArray* remmina_string_array_new_from_allocated_string(gchar *strs);
void remmina_string_array_add(RemminaStringArray *array, const gchar *str);
gint remmina_string_array_find(RemminaStringArray *array, const gchar *str);
void remmina_string_array_remove_index(RemminaStringArray *array, gint i);
void remmina_string_array_remove(RemminaStringArray *array, const gchar *str);
void remmina_string_array_intersect(RemminaStringArray *array, const gchar *dest_strs);
void remmina_string_array_sort(RemminaStringArray *array);
gchar* remmina_string_array_to_string(RemminaStringArray *array);
void remmina_string_array_free(RemminaStringArray *array);

G_END_DECLS

#endif  /* __REMMINASTRINGARRAY_H__  */

