/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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

#include "config.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>

#include "remmina_public.h"
#include "remmina_applet_menu_item.h"
#include "remmina_applet_menu.h"
#include "remmina_file_manager.h"
#include "remmina/remmina_trace_calls.h"

G_DEFINE_TYPE( RemminaAppletMenu, remmina_applet_menu, GTK_TYPE_MENU)

struct _RemminaAppletMenuPriv {
	gboolean hide_count;
};

enum {
	LAUNCH_ITEM_SIGNAL, EDIT_ITEM_SIGNAL, LAST_SIGNAL
};

static guint remmina_applet_menu_signals[LAST_SIGNAL] =
{ 0 };

static void remmina_applet_menu_destroy(RemminaAppletMenu *menu, gpointer data)
{
	TRACE_CALL(__func__);
	g_free(menu->priv);
}

static void remmina_applet_menu_class_init(RemminaAppletMenuClass *klass)
{
	TRACE_CALL(__func__);
	remmina_applet_menu_signals[LAUNCH_ITEM_SIGNAL] = g_signal_new("launch-item", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaAppletMenuClass, launch_item), NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, G_TYPE_OBJECT);
	remmina_applet_menu_signals[EDIT_ITEM_SIGNAL] = g_signal_new("edit-item", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaAppletMenuClass, edit_item), NULL, NULL,
		g_cclosure_marshal_VOID__OBJECT, G_TYPE_NONE, 1, G_TYPE_OBJECT);
}

static void remmina_applet_menu_init(RemminaAppletMenu *menu)
{
	TRACE_CALL(__func__);
	menu->priv = g_new0(RemminaAppletMenuPriv, 1);

	g_signal_connect(G_OBJECT(menu), "destroy", G_CALLBACK(remmina_applet_menu_destroy), NULL);
}

static void remmina_applet_menu_on_item_activate(RemminaAppletMenuItem *menuitem, RemminaAppletMenu *menu)
{
	TRACE_CALL(__func__);
	g_signal_emit(G_OBJECT(menu), remmina_applet_menu_signals[LAUNCH_ITEM_SIGNAL], 0, menuitem);
}

static GtkWidget*
remmina_applet_menu_add_group(GtkWidget *menu, const gchar *group, gint position, RemminaAppletMenuItem *menuitem,
			      GtkWidget **groupmenuitem)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	GtkWidget *submenu;

	widget = gtk_menu_item_new_with_label(group);
	gtk_widget_show(widget);

	g_object_set_data_full(G_OBJECT(widget), "group", g_strdup(group), g_free);
	g_object_set_data(G_OBJECT(widget), "count", GINT_TO_POINTER(0));
	if (groupmenuitem) {
		*groupmenuitem = widget;
	}
	if (position < 0) {
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), widget);
	}else  {
		gtk_menu_shell_insert(GTK_MENU_SHELL(menu), widget, position);
	}

	submenu = gtk_menu_new();
	gtk_widget_show(submenu);
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(widget), submenu);

	return submenu;
}

static void remmina_applet_menu_increase_group_count(GtkWidget *widget)
{
	TRACE_CALL(__func__);
	gint cnt;
	gchar *s;

	cnt = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "count")) + 1;
	g_object_set_data(G_OBJECT(widget), "count", GINT_TO_POINTER(cnt));
	s = g_strdup_printf("%s (%i)", (const gchar*)g_object_get_data(G_OBJECT(widget), "group"), cnt);
	gtk_menu_item_set_label(GTK_MENU_ITEM(widget), s);
	g_free(s);
}

void remmina_applet_menu_register_item(RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem)
{
	TRACE_CALL(__func__);
	g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_applet_menu_on_item_activate), menu);
}

void remmina_applet_menu_add_item(RemminaAppletMenu *menu, RemminaAppletMenuItem *menuitem)
{
	TRACE_CALL(__func__);
	GtkWidget *submenu;
	GtkWidget *groupmenuitem;
	GtkMenuItem *submenuitem;
	gchar *s, *p1, *p2, *mstr;
	GList *childs, *child;
	gint position;

	submenu = GTK_WIDGET(menu);
	s = g_strdup(menuitem->group);
	p1 = s;
	p2 = p1 ? strchr(p1, '/') : NULL;
	if (p2)
		*p2++ = '\0';
	while (p1 && p1[0]) {
		groupmenuitem = NULL;
		childs = gtk_container_get_children(GTK_CONTAINER(submenu));
		position = -1;
		for (child = g_list_first(childs); child; child = g_list_next(child)) {
			if (!GTK_IS_MENU_ITEM(child->data))
				continue;
			position++;
			submenuitem = GTK_MENU_ITEM(child->data);
			if (gtk_menu_item_get_submenu(submenuitem)) {
				mstr = (gchar*)g_object_get_data(G_OBJECT(submenuitem), "group");
				if (g_strcmp0(p1, mstr) == 0) {
					/* Found existing group menu */
					submenu = gtk_menu_item_get_submenu(submenuitem);
					groupmenuitem = GTK_WIDGET(submenuitem);
					break;
				}else  {
					/* Redo comparison ignoring case and respecting international
					 * collation, to set menu sort order */
					if (strcoll(p1, mstr) < 0) {
						submenu = remmina_applet_menu_add_group(submenu, p1, position, menuitem,
							&groupmenuitem);
						break;
					}
				}
			}else  {
				submenu = remmina_applet_menu_add_group(submenu, p1, position, menuitem, &groupmenuitem);
				break;
			}

		}

		if (!child) {
			submenu = remmina_applet_menu_add_group(submenu, p1, -1, menuitem, &groupmenuitem);
		}
		g_list_free(childs);
		if (groupmenuitem && !menu->priv->hide_count) {
			remmina_applet_menu_increase_group_count(groupmenuitem);
		}
		p1 = p2;
		p2 = p1 ? strchr(p1, '/') : NULL;
		if (p2)
			*p2++ = '\0';
	}
	g_free(s);

	childs = gtk_container_get_children(GTK_CONTAINER(submenu));
	position = -1;
	for (child = g_list_first(childs); child; child = g_list_next(child)) {
		if (!GTK_IS_MENU_ITEM(child->data))
			continue;
		position++;
		submenuitem = GTK_MENU_ITEM(child->data);
		if (gtk_menu_item_get_submenu(submenuitem))
			continue;
		if (!REMMINA_IS_APPLET_MENU_ITEM(submenuitem))
			continue;
		if (strcoll(menuitem->name, REMMINA_APPLET_MENU_ITEM(submenuitem)->name) <= 0) {
			gtk_menu_shell_insert(GTK_MENU_SHELL(submenu), GTK_WIDGET(menuitem), position);
			break;
		}
	}
	if (!child) {
		gtk_menu_shell_append(GTK_MENU_SHELL(submenu), GTK_WIDGET(menuitem));
	}
	g_list_free(childs);
	remmina_applet_menu_register_item(menu, menuitem);
}

GtkWidget*
remmina_applet_menu_new(void)
{
	TRACE_CALL(__func__);
	RemminaAppletMenu *menu;

	menu = REMMINA_APPLET_MENU(g_object_new(REMMINA_TYPE_APPLET_MENU, NULL));

	return GTK_WIDGET(menu);
}

void remmina_applet_menu_set_hide_count(RemminaAppletMenu *menu, gboolean hide_count)
{
	TRACE_CALL(__func__);
	menu->priv->hide_count = hide_count;
}

void remmina_applet_menu_populate(RemminaAppletMenu *menu)
{
	TRACE_CALL(__func__);
	GtkWidget *menuitem;
	gchar filename[MAX_PATH_LEN];
	GDir *dir;
	gchar *remmina_data_dir;
	const gchar *name;

	remmina_data_dir = remmina_file_get_datadir();
	dir = g_dir_open(remmina_data_dir, 0, NULL);
	if (dir != NULL) {
		/* Iterate all remote desktop profiles */
		while ((name = g_dir_read_name(dir)) != NULL) {
			if (!g_str_has_suffix(name, ".remmina"))
				continue;
			g_snprintf(filename, sizeof(filename), "%s/%s", remmina_data_dir, name);

			menuitem = remmina_applet_menu_item_new(REMMINA_APPLET_MENU_ITEM_FILE, filename);
			if (menuitem != NULL) {
				remmina_applet_menu_add_item(menu, REMMINA_APPLET_MENU_ITEM(menuitem));
				gtk_widget_show(menuitem);
			}
		}
		g_dir_close(dir);
	}
	g_free(remmina_data_dir);
}

