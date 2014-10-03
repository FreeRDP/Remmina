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

#ifndef __REMMINASTRINGLIST_H__
#define __REMMINASTRINGLIST_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_STRING_LIST               (remmina_string_list_get_type ())
#define REMMINA_STRING_LIST(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_STRING_LIST, RemminaStringList))
#define REMMINA_STRING_LIST_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_STRING_LIST, RemminaStringListClass))
#define REMMINA_IS_STRING_LIST(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_STRING_LIST))
#define REMMINA_IS_STRING_LIST_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_STRING_LIST))
#define REMMINA_STRING_LIST_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_STRING_LIST, RemminaStringListClass))

typedef gboolean (*RemminaStringListValidationFunc)(const gchar *new_str, gchar **error);

typedef struct _RemminaStringList
{
	GtkGrid table;

	GtkListStore *store;
	GtkWidget *list;
	GtkWidget *status_label;
	const gchar *hints;

	GtkWidget *up_button;
	GtkWidget *down_button;

	RemminaStringListValidationFunc validation_func;
} RemminaStringList;

typedef struct _RemminaStringListClass
{
	GtkGridClass parent_class;
} RemminaStringListClass;

GType remmina_string_list_get_type(void)
G_GNUC_CONST;

GtkWidget* remmina_string_list_new(void);

void remmina_string_list_set_auto_sort(RemminaStringList *gsl, gboolean auto_sort);

void remmina_string_list_set_hints(RemminaStringList *gsl, const gchar *hints);

void remmina_string_list_set_text(RemminaStringList *gsl, const gchar *text);

void remmina_string_list_set_validation_func(RemminaStringList *gsl, RemminaStringListValidationFunc func);

/* The returned text is newly allocated */
gchar* remmina_string_list_get_text(RemminaStringList *gsl);

G_END_DECLS

#endif  /* __REMMINASTRINGLIST_H__  */

