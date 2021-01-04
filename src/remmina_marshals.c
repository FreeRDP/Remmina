/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2011 Marc-Andre Moreau
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

#include        <glib-object.h>
#include "remmina/remmina_trace_calls.h"

#ifdef G_ENABLE_DEBUG
#define g_marshal_value_peek_boolean(v)  g_value_get_boolean(v)
#define g_marshal_value_peek_char(v)     g_value_get_char(v)
#define g_marshal_value_peek_uchar(v)    g_value_get_uchar(v)
#define g_marshal_value_peek_int(v)      g_value_get_int(v)
#define g_marshal_value_peek_uint(v)     g_value_get_uint(v)
#define g_marshal_value_peek_long(v)     g_value_get_long(v)
#define g_marshal_value_peek_ulong(v)    g_value_get_ulong(v)
#define g_marshal_value_peek_int64(v)    g_value_get_int64(v)
#define g_marshal_value_peek_uint64(v)   g_value_get_uint64(v)
#define g_marshal_value_peek_enum(v)     g_value_get_enum(v)
#define g_marshal_value_peek_flags(v)    g_value_get_flags(v)
#define g_marshal_value_peek_float(v)    g_value_get_float(v)
#define g_marshal_value_peek_double(v)   g_value_get_double(v)
#define g_marshal_value_peek_string(v)   (char*)g_value_get_string(v)
#define g_marshal_value_peek_param(v)    g_value_get_param(v)
#define g_marshal_value_peek_boxed(v)    g_value_get_boxed(v)
#define g_marshal_value_peek_pointer(v)  g_value_get_pointer(v)
#define g_marshal_value_peek_object(v)   g_value_get_object(v)
#else /* !G_ENABLE_DEBUG */
/* WARNING: This code accesses GValues directly, which is UNSUPPORTED API.
 *          Do not access GValues directly in your code. Instead, use the
 *          g_value_get_*() functions
 */
#define g_marshal_value_peek_boolean(v)  (v)->data[0].v_int
#define g_marshal_value_peek_char(v)     (v)->data[0].v_int
#define g_marshal_value_peek_uchar(v)    (v)->data[0].v_uint
#define g_marshal_value_peek_int(v)      (v)->data[0].v_int
#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_long(v)     (v)->data[0].v_long
#define g_marshal_value_peek_ulong(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_int64(v)    (v)->data[0].v_int64
#define g_marshal_value_peek_uint64(v)   (v)->data[0].v_uint64
#define g_marshal_value_peek_enum(v)     (v)->data[0].v_long
#define g_marshal_value_peek_flags(v)    (v)->data[0].v_ulong
#define g_marshal_value_peek_float(v)    (v)->data[0].v_float
#define g_marshal_value_peek_double(v)   (v)->data[0].v_double
#define g_marshal_value_peek_string(v)   (v)->data[0].v_pointer
#define g_marshal_value_peek_param(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_boxed(v)    (v)->data[0].v_pointer
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer
#define g_marshal_value_peek_object(v)   (v)->data[0].v_pointer
#endif /* !G_ENABLE_DEBUG */

/* BOOLEAN:INT (remminamarshals.list:4) */
void
remmina_marshal_BOOLEAN__INT(GClosure     *closure,
			     GValue       *return_value G_GNUC_UNUSED,
			     guint n_param_values,
			     const GValue *param_values,
			     gpointer invocation_hint G_GNUC_UNUSED,
			     gpointer marshal_data)
{
	TRACE_CALL(__func__);
	typedef gboolean (*GMarshalFunc_BOOLEAN__INT) (gpointer data1,
						       gint arg_1,
						       gpointer data2);
	register GMarshalFunc_BOOLEAN__INT callback;
	register GCClosure *cc = (GCClosure*)closure;
	register gpointer data1, data2;
	gboolean v_return;

	g_return_if_fail(return_value != NULL);
	g_return_if_fail(n_param_values == 2);

	if (G_CCLOSURE_SWAP_DATA(closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer(param_values + 0);
	}else  {
		data1 = g_value_peek_pointer(param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_BOOLEAN__INT)(marshal_data ? marshal_data : cc->callback);

	v_return = callback(data1,
		g_marshal_value_peek_int(param_values + 1)
		,
		data2);

	g_value_set_boolean(return_value, v_return);
}

/* BOOLEAN:INT,STRING (remminamarshals.list:5) */
void
remmina_marshal_BOOLEAN__INT_STRING(GClosure *closure,
				    GValue *return_value G_GNUC_UNUSED,
				    guint n_param_values,
				    const GValue *param_values,
				    gpointer invocation_hint G_GNUC_UNUSED,
				    gpointer marshal_data)
{
	TRACE_CALL(__func__);
	typedef gboolean (*GMarshalFunc_BOOLEAN__INT_STRING) (gpointer data1,
							      gint arg_1,
							      gpointer arg_2,
							      gpointer data2);
	register GMarshalFunc_BOOLEAN__INT_STRING callback;
	register GCClosure *cc = (GCClosure*)closure;
	register gpointer data1, data2;
	gboolean v_return;

	g_return_if_fail(return_value != NULL);
	g_return_if_fail(n_param_values == 3);

	if (G_CCLOSURE_SWAP_DATA(closure)) {
		data1 = closure->data;
		data2 = g_value_peek_pointer(param_values + 0);
	}else  {
		data1 = g_value_peek_pointer(param_values + 0);
		data2 = closure->data;
	}
	callback = (GMarshalFunc_BOOLEAN__INT_STRING)(marshal_data ? marshal_data : cc->callback);

	v_return = callback(data1,
		g_marshal_value_peek_int(param_values + 1),
		g_marshal_value_peek_string(param_values + 2),
		data2);

	g_value_set_boolean(return_value, v_return);
}

