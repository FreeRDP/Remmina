/*  See LICENSE and COPYING files for copyright and license details. */

#include <glib.h>
#include <string.h>
#include "remmina_string_array.h"
#include "remmina/remmina_trace_calls.h"

RemminaStringArray*
remmina_string_array_new(void)
{
	TRACE_CALL("remmina_string_array_new");
	return g_ptr_array_new();
}

RemminaStringArray*
remmina_string_array_new_from_string(const gchar *strs)
{
	TRACE_CALL("remmina_string_array_new_from_string");
	RemminaStringArray *array;
	gchar *buf, *ptr1, *ptr2;

	array = remmina_string_array_new();
	if (!strs || strs[0] == '\0')
		return array;

	buf = g_strdup(strs);
	ptr1 = buf;
	while (ptr1)
	{
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
	TRACE_CALL("remmina_string_array_new_from_allocated_string");
	RemminaStringArray *array;
	array = remmina_string_array_new_from_string(strs);
	g_free(strs);
	return array;
}

void remmina_string_array_add(RemminaStringArray* array, const gchar *str)
{
	TRACE_CALL("remmina_string_array_add");
	g_ptr_array_add(array, g_strdup(str));
}

gint remmina_string_array_find(RemminaStringArray* array, const gchar *str)
{
	TRACE_CALL("remmina_string_array_find");
	gint i;

	for (i = 0; i < array->len; i++)
	{
		if (g_strcmp0(remmina_string_array_index(array, i), str) == 0)
			return i;
	}
	return -1;
}

void remmina_string_array_remove_index(RemminaStringArray* array, gint i)
{
	TRACE_CALL("remmina_string_array_remove_index");
	g_ptr_array_remove_index(array, i);
}

void remmina_string_array_remove(RemminaStringArray* array, const gchar *str)
{
	TRACE_CALL("remmina_string_array_remove");
	gint i;

	i = remmina_string_array_find(array, str);
	if (i >= 0)
	{
		remmina_string_array_remove_index(array, i);
	}
}

void remmina_string_array_intersect(RemminaStringArray* array, const gchar *dest_strs)
{
	TRACE_CALL("remmina_string_array_intersect");
	RemminaStringArray *dest_array;
	gint i, j;

	dest_array = remmina_string_array_new_from_string(dest_strs);

	i = 0;
	while (i < array->len)
	{
		j = remmina_string_array_find(dest_array, remmina_string_array_index(array, i));
		if (j < 0)
		{
			remmina_string_array_remove_index(array, i);
			continue;
		}
		i++;
	}

	remmina_string_array_free(dest_array);
}

static gint remmina_string_array_compare_func(const gchar **a, const gchar **b)
{
	TRACE_CALL("remmina_string_array_compare_func");
	return g_strcmp0(*a, *b);
}

void remmina_string_array_sort(RemminaStringArray *array)
{
	TRACE_CALL("remmina_string_array_sort");
	g_ptr_array_sort(array, (GCompareFunc) remmina_string_array_compare_func);
}

gchar*
remmina_string_array_to_string(RemminaStringArray* array)
{
	TRACE_CALL("remmina_string_array_to_string");
	GString *gstr;
	gint i;

	gstr = g_string_new("");
	for (i = 0; i < array->len; i++)
	{
		if (i > 0)
			g_string_append_c(gstr, ',');
		g_string_append(gstr, remmina_string_array_index(array, i));
	}
	return g_string_free(gstr, FALSE);
}

void remmina_string_array_free(RemminaStringArray *array)
{
	TRACE_CALL("remmina_string_array_free");
	g_ptr_array_foreach(array, (GFunc) g_free, NULL);
	g_ptr_array_free(array, TRUE);
}

