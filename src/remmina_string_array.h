/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee
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

G_BEGIN_DECLS

typedef GPtrArray RemminaStringArray;

RemminaStringArray *remmina_string_array_new(void);
#define remmina_string_array_index(array, i) (gchar *)g_ptr_array_index(array, i)
RemminaStringArray *remmina_string_array_new_from_string(const gchar *strs);
RemminaStringArray *remmina_string_array_new_from_allocated_string(gchar *strs);
void remmina_string_array_add(RemminaStringArray *array, const gchar *str);
gint remmina_string_array_find(RemminaStringArray *array, const gchar *str);
void remmina_string_array_remove_index(RemminaStringArray *array, gint i);
void remmina_string_array_remove(RemminaStringArray *array, const gchar *str);
void remmina_string_array_intersect(RemminaStringArray *array, const gchar *dest_strs);
void remmina_string_array_sort(RemminaStringArray *array);
gchar *remmina_string_array_to_string(RemminaStringArray *array);
void remmina_string_array_free(RemminaStringArray *array);

G_END_DECLS
