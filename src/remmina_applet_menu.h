/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2017-2019 Antenore Gatta, Giovanni Panozzo
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

#define REMMINA_TYPE_APPLET_MENU            (remmina_applet_menu_get_type())
#define REMMINA_APPLET_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), REMMINA_TYPE_APPLET_MENU, RemminaAppletMenu))
#define REMMINA_APPLET_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), REMMINA_TYPE_APPLET_MENU, RemminaAppletMenuClass))
#define REMMINA_IS_APPLET_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), REMMINA_TYPE_APPLET_MENU))
#define REMMINA_IS_APPLET_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), REMMINA_TYPE_APPLET_MENU))
#define REMMINA_APPLET_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), REMMINA_TYPE_APPLET_MENU, RemminaAppletMenuClass))

typedef enum {
	REMMINA_APPLET_MENU_NEW_CONNECTION_NONE,
	REMMINA_APPLET_MENU_NEW_CONNECTION_TOP,
	REMMINA_APPLET_MENU_NEW_CONNECTION_BOTTOM
} RemminaAppletMenuNewConnectionType;

typedef struct _RemminaAppletMenuPriv RemminaAppletMenuPriv;

typedef struct _RemminaAppletMenu {
	GtkMenu			menu;

	RemminaAppletMenuPriv * priv;
} RemminaAppletMenu;

typedef struct _RemminaAppletMenuClass {
	GtkMenuClass parent_class;

	void (*launch_item)(RemminaAppletMenu *menu);
	void (*edit_item)(RemminaAppletMenu *menu);
} RemminaAppletMenuClass;

GType remmina_applet_menu_get_type(void)
G_GNUC_CONST;

void remmina_applet_menu_register_item(RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem);
void remmina_applet_menu_add_item(RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem);
GtkWidget *remmina_applet_menu_new(void);
void remmina_applet_menu_set_hide_count(RemminaAppletMenu *menu, gboolean hide_count);
void remmina_applet_menu_populate(RemminaAppletMenu *menu);

G_END_DECLS
