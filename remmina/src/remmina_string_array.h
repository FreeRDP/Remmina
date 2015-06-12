/*  See LICENSE and COPYING files for copyright and license details. */

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

