/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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

#include <glib.h>
#include <gmodule.h>
#include <string.h>
#include "remmina_string_array.h"
#include "remmina/remmina_trace_calls.h"

RemminaStringArray*
remmina_string_array_new(void)
{
	TRACE_CALL(__func__);
	return g_ptr_array_new();
}

RemminaStringArray*
remmina_string_array_new_from_string(const gchar *strs)
{
	TRACE_CALL(__func__);
	RemminaStringArray *array;
	gchar *buf, *ptr1, *ptr2;

	array = remmina_string_array_new();
	if (!strs || strs[0] == '\0')
		return array;

	buf = g_strdup(strs);
	ptr1 = buf;
	while (ptr1) {
		ptr2 = strchr(ptr1, ',');
		if (ptr2)
			*ptr2++ = '\0';
		remmina_string_array_add(array, ptr1);
		ptr1 = ptr2;
	}

	g_free(buf);

	return array;
}

RemminaStringArray*
remmina_string_array_new_from_allocated_string(gchar *strs)
{
	TRACE_CALL(__func__);
	RemminaStringArray *array;
	array = remmina_string_array_new_from_string(strs);
	g_free(strs);
	return array;
}

void remmina_string_array_add(RemminaStringArray* array, const gchar *str)
{
	TRACE_CALL(__func__);
	g_ptr_array_add(array, g_strdup(str));
}

gint remmina_string_array_find(RemminaStringArray* array, const gchar *str)
{
	TRACE_CALL(__func__);
	gint i;

	for (i = 0; i < array->len; i++) {
		if (g_strcmp0(remmina_string_array_index(array, i), str) == 0)
			return i;
	}
	return -1;
}

void remmina_string_array_remove_index(RemminaStringArray* array, gint i)
{
	TRACE_CALL(__func__);
	g_ptr_array_remove_index(array, i);
}

void remmina_string_array_remove(RemminaStringArray* array, const gchar *str)
{
	TRACE_CALL(__func__);
	gint i;

	i = remmina_string_array_find(array, str);
	if (i >= 0) {
		remmina_string_array_remove_index(array, i);
	}
}

void remmina_string_array_intersect(RemminaStringArray* array, const gchar *dest_strs)
{
	TRACE_CALL(__func__);
	RemminaStringArray *dest_array;
	gint i, j;

	dest_array = remmina_string_array_new_from_string(dest_strs);

	i = 0;
	while (i < array->len) {
		j = remmina_string_array_find(dest_array, remmina_string_array_index(array, i));
		if (j < 0) {
			remmina_string_array_remove_index(array, i);
			continue;
		}
		i++;
	}

	remmina_string_array_free(dest_array);
}

static gint remmina_string_array_compare_func(const gchar **a, const gchar **b)
{
	TRACE_CALL(__func__);
	return g_strcmp0(*a, *b);
}

void remmina_string_array_sort(RemminaStringArray *array)
{
	TRACE_CALL(__func__);
	g_ptr_array_sort(array, (GCompareFunc)remmina_string_array_compare_func);
}

gchar*
remmina_string_array_to_string(RemminaStringArray* array)
{
	TRACE_CALL(__func__);
	GString *gstr;
	gint i;

	gstr = g_string_new("");
	for (i = 0; i < array->len; i++) {
		if (i > 0)
			g_string_append_c(gstr, ',');
		g_string_append(gstr, remmina_string_array_index(array, i));
	}
	return g_string_free(gstr, FALSE);
}

void remmina_string_array_free(RemminaStringArray *array)
{
	TRACE_CALL(__func__);
	g_ptr_array_foreach(array, (GFunc)g_free, NULL);
	g_ptr_array_free(array, TRUE);
}

