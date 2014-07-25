/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

#include "remmina_string_array.h"

#ifndef __REMMINAMAIN_H__
#define __REMMINAMAIN_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_MAIN               (remmina_main_get_type ())
#define REMMINA_MAIN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_MAIN, RemminaMain))
#define REMMINA_MAIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_MAIN, RemminaMainClass))
#define REMMINA_IS_MAIN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_MAIN))
#define REMMINA_IS_MAIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_MAIN))
#define REMMINA_MAIN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_MAIN, RemminaMainClass))

typedef struct _RemminaMainPriv RemminaMainPriv;

typedef struct _RemminaMain
{
	GtkWindow window;

	RemminaMainPriv *priv;
} RemminaMain;

struct _RemminaMainPriv
{
	GtkWidget *file_list;
	GtkTreeModel *file_model;
	GtkTreeModel *file_model_filter;
	GtkTreeModel *file_model_sort;
	GtkUIManager *uimanager;
	GtkWidget *toolbar;
	GtkWidget *statusbar;

	GtkToolItem *quick_search_separator;
	GtkToolItem *quick_search_item;
	GtkWidget *quick_search_entry;

	GtkWidget *quickconnect_protocol;
	GtkWidget *quickconnect_server;

	GtkTreeViewColumn *group_column;

	GtkActionGroup *main_group;
	GtkActionGroup *file_sensitive_group;

	gboolean initialized;

	gchar *selected_filename;
	gchar *selected_name;
	RemminaStringArray *expanded_group;
};

typedef struct _RemminaMainClass
{
	GtkWindowClass parent_class;
} RemminaMainClass;

GType remmina_main_get_type(void)
G_GNUC_CONST;

/* Create the main Remmina window */
GtkWidget* remmina_main_new(void);

G_END_DECLS

#endif  /* __REMMINAMAIN_H__  */

