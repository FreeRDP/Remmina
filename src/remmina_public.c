/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"

GtkWidget*
remmina_public_create_combo_entry(const gchar *text, const gchar *def, gboolean descending)
{
	TRACE_CALL(__func__);
	GtkWidget *combo;
	gboolean found;
	gchar *buf, *ptr1, *ptr2;
	gint i;

	combo = gtk_combo_box_text_new_with_entry();
	found = FALSE;

	if (text && text[0] != '\0') {
		buf = g_strdup(text);
		ptr1 = buf;
		i = 0;
		while (ptr1 && *ptr1 != '\0') {
			ptr2 = strchr(ptr1, CHAR_DELIMITOR);
			if (ptr2)
				*ptr2++ = '\0';

			if (descending) {
				gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(combo), ptr1);
				if (!found && g_strcmp0(ptr1, def) == 0) {
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 0);
					found = TRUE;
				}
			}else  {
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), ptr1);
				if (!found && g_strcmp0(ptr1, def) == 0) {
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
					found = TRUE;
				}
			}

			ptr1 = ptr2;
			i++;
		}

		g_free(buf);
	}

	if (!found && def && def[0] != '\0') {
		gtk_entry_set_text(GTK_ENTRY(gtk_bin_get_child(GTK_BIN(combo))), def);
	}

	return combo;
}

GtkWidget*
remmina_public_create_combo_text_d(const gchar *text, const gchar *def, const gchar *empty_choice)
{
	TRACE_CALL(__func__);
	GtkWidget *combo;
	GtkListStore *store;
	GtkCellRenderer *text_renderer;

	store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));

	text_renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_end(GTK_CELL_LAYOUT(combo), text_renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), text_renderer, "text", 1);

	remmina_public_load_combo_text_d(combo, text, def, empty_choice);

	return combo;
}

void remmina_public_load_combo_text_d(GtkWidget *combo, const gchar *text, const gchar *def, const gchar *empty_choice)
{
	TRACE_CALL(__func__);
	GtkListStore *store;
	GtkTreeIter iter;
	gint i;
	gchar *buf, *ptr1, *ptr2;

	store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));
	gtk_list_store_clear(store);

	i = 0;

	if (empty_choice) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, "", 1, empty_choice, -1);
		gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
		i++;
	}

	if (text == NULL || text[0] == '\0')
		return;

	buf = g_strdup(text);
	ptr1 = buf;
	while (ptr1 && *ptr1 != '\0') {
		ptr2 = strchr(ptr1, CHAR_DELIMITOR);
		if (ptr2)
			*ptr2++ = '\0';

		gtk_list_store_append(store, &iter);
		gtk_list_store_set(store, &iter, 0, ptr1, 1, ptr1, -1);

		if (i == 0 || g_strcmp0(ptr1, def) == 0) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i);
		}

		i++;
		ptr1 = ptr2;
	}

	g_free(buf);
}

GtkWidget*
remmina_public_create_combo(gboolean use_icon)
{
	TRACE_CALL(__func__);
	GtkWidget *combo;
	GtkListStore *store;
	GtkCellRenderer *renderer;

	if (use_icon) {
		store = gtk_list_store_new(3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	}else  {
		store = gtk_list_store_new(2, G_TYPE_STRING, G_TYPE_STRING);
	}
	combo = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));
	gtk_widget_set_hexpand(combo, TRUE);

	if (use_icon) {
		renderer = gtk_cell_renderer_pixbuf_new();
		gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, FALSE);
		gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), renderer, "icon-name", 2);
	}
	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), renderer, "text", 1);
	if (use_icon)
		g_object_set(G_OBJECT(renderer), "xpad", 5, NULL);

	return combo;
}

GtkWidget*
remmina_public_create_combo_map(const gpointer *key_value_list, const gchar *def, gboolean use_icon, const gchar *domain)
{
	TRACE_CALL(__func__);
	gint i;
	GtkWidget *combo;
	GtkListStore *store;
	GtkTreeIter iter;

	combo = remmina_public_create_combo(use_icon);
	store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo)));

	for (i = 0; key_value_list[i]; i += (use_icon ? 3 : 2)) {
		gtk_list_store_append(store, &iter);
		gtk_list_store_set(
			store,
			&iter,
			0,
			key_value_list[i],
			1,
			key_value_list[i + 1] && ((char*)key_value_list[i + 1])[0] ?
			g_dgettext(domain, key_value_list[i + 1]) : "", -1);
		if (use_icon) {
			gtk_list_store_set(store, &iter, 2, key_value_list[i + 2], -1);
		}
		if (i == 0 || g_strcmp0(key_value_list[i], def) == 0) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(combo), i / (use_icon ? 3 : 2));
		}
	}
	return combo;
}

GtkWidget*
remmina_public_create_combo_mapint(const gpointer *key_value_list, gint def, gboolean use_icon, const gchar *domain)
{
	TRACE_CALL(__func__);
	gchar buf[20];
	g_snprintf(buf, sizeof(buf), "%i", def);
	return remmina_public_create_combo_map(key_value_list, buf, use_icon, domain);
}

void remmina_public_create_group(GtkGrid *table, const gchar *group, gint row, gint rows, gint cols)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	gchar *str;

	widget = gtk_label_new(NULL);
	gtk_widget_show(widget);
	gtk_widget_set_halign(GTK_WIDGET(widget), GTK_ALIGN_START);
	gtk_widget_set_valign(GTK_WIDGET(widget), GTK_ALIGN_CENTER);
	str = g_markup_printf_escaped("<b>%s</b>", group);
	gtk_label_set_markup(GTK_LABEL(widget), str);
	g_free(str);
	gtk_grid_attach(GTK_GRID(table), widget, 0, row, 1, 2);

	widget = gtk_label_new(NULL);
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(table), widget, 0, row + 1, 1, 1);
}

gchar*
remmina_public_combo_get_active_text(GtkComboBox *combo)
{
	TRACE_CALL(__func__);
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *s;

	if (GTK_IS_COMBO_BOX_TEXT(combo)) {
		return gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
	}

	if (!gtk_combo_box_get_active_iter(combo, &iter))
		return NULL;

	model = gtk_combo_box_get_model(combo);
	gtk_tree_model_get(model, &iter, 0, &s, -1);

	return s;
}

#if !GTK_CHECK_VERSION(3, 22, 0)
void remmina_public_popup_position(GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkWidget *widget;
	gint tx, ty;
	GtkAllocation allocation;

	widget = GTK_WIDGET(user_data);
	if (gtk_widget_get_window(widget) == NULL) {
		*x = 0;
		*y = 0;
		*push_in = TRUE;
		return;
	}
	gdk_window_get_origin(gtk_widget_get_window(widget), &tx, &ty);
	gtk_widget_get_allocation(widget, &allocation);
	/* I’m unsure why the author made the check about a GdkWindow inside the
	 * widget argument. This function generally is called passing by a ToolButton
	 * which hasn’t any GdkWindow, therefore the positioning is wrong
	 * I think the gtk_widget_get_has_window() check should be removed
	 *
	 * While leaving the previous check intact I’m checking also if the provided
	 * widget is a GtkToggleToolButton and position the menu accordingly. */
	if (gtk_widget_get_has_window(widget) ||
	    g_strcmp0(gtk_widget_get_name(widget), "GtkToggleToolButton") == 0) {
		tx += allocation.x;
		ty += allocation.y;
	}

	*x = tx;
	*y = ty + allocation.height - 1;
	*push_in = TRUE;
}
#endif

gchar*
remmina_public_combine_path(const gchar *path1, const gchar *path2)
{
	TRACE_CALL(__func__);
	if (!path1 || path1[0] == '\0')
		return g_strdup(path2);
	if (path1[strlen(path1) - 1] == '/')
		return g_strdup_printf("%s%s", path1, path2);
	return g_strdup_printf("%s/%s", path1, path2);
}

void remmina_public_get_server_port(const gchar *server, gint defaultport, gchar **host, gint *port)
{
	TRACE_CALL(__func__);
	gchar *str, *ptr, *ptr2;

	str = g_strdup(server);

	if (str) {
		/* [server]:port format */
		ptr = strchr(str, '[');
		if (ptr) {
			ptr++;
			ptr2 = strchr(ptr, ']');
			if (ptr2) {
				*ptr2++ = '\0';
				if (*ptr2 == ':')
					defaultport = atoi(ptr2 + 1);
			}
			if (host)
				*host = g_strdup(ptr);
			if (port)
				*port = defaultport;
			g_free(str);
			return;
		}

		/* server:port format, IPv6 cannot use this format */
		ptr = strchr(str, ':');
		if (ptr) {
			ptr2 = strchr(ptr + 1, ':');
			if (ptr2 == NULL) {
				*ptr++ = '\0';
				defaultport = atoi(ptr);
			}
			/* More than one ':' means this is IPv6 address. Treat it as a whole address */
		}
	}

	if (host)
		*host = str;
	else
		g_free(str);
	if (port)
		*port = defaultport;
}

gboolean remmina_public_get_xauth_cookie(const gchar *display, gchar **msg)
{
	TRACE_CALL(__func__);
	gchar buf[200];
	gchar *out = NULL;
	gchar *ptr;
	GError *error = NULL;
	gboolean ret;

	if (!display)
		display = gdk_display_get_name(gdk_display_get_default());

	g_snprintf(buf, sizeof(buf), "xauth list %s", display);
	ret = g_spawn_command_line_sync(buf, &out, NULL, NULL, &error);
	if (ret) {
		if ((ptr = g_strrstr(out, "MIT-MAGIC-COOKIE-1")) == NULL) {
			*msg = g_strdup_printf("xauth returns %s", out);
			ret = FALSE;
		}else  {
			ptr += 19;
			while (*ptr == ' ')
				ptr++;
			*msg = g_strndup(ptr, 32);
		}
		g_free(out);
	}else  {
		*msg = g_strdup(error->message);
	}
	return ret;
}

gint remmina_public_open_xdisplay(const gchar *disp)
{
	TRACE_CALL(__func__);
	gchar *display;
	gchar *ptr;
	gint port;
	struct sockaddr_un addr;
	gint sock = -1;

	display = g_strdup(disp);
	ptr = g_strrstr(display, ":");
	if (ptr) {
		*ptr++ = '\0';
		/* Assume you are using a local display… might need to implement remote display in the future */
		if (display[0] == '\0' || strcmp(display, "unix") == 0) {
			port = atoi(ptr);
			sock = socket(AF_UNIX, SOCK_STREAM, 0);
			if (sock >= 0) {
				memset(&addr, 0, sizeof(addr));
				addr.sun_family = AF_UNIX;
				snprintf(addr.sun_path, sizeof(addr.sun_path), X_UNIX_SOCKET, port);
				if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
					close(sock);
					sock = -1;
				}
			}
		}
	}

	g_free(display);
	return sock;
}

/* This function was copied from GEdit (gedit-utils.c). */
guint remmina_public_get_current_workspace(GdkScreen *screen)
{
	TRACE_CALL(__func__);
#ifdef GDK_WINDOWING_X11
#if GTK_CHECK_VERSION(3, 10, 0)
	g_return_val_if_fail(GDK_IS_SCREEN(screen), 0);
	if (GDK_IS_X11_DISPLAY(gdk_screen_get_display(screen)))
		return gdk_x11_screen_get_current_desktop(screen);
	else
		return 0;

#else
	GdkWindow *root_win;
	GdkDisplay *display;
	Atom type;
	gint format;
	gulong nitems;
	gulong bytes_after;
	guint *current_desktop;
	gint err, result;
	guint ret = 0;

	g_return_val_if_fail(GDK_IS_SCREEN(screen), 0);

	root_win = gdk_screen_get_root_window(screen);
	display = gdk_screen_get_display(screen);

	gdk_error_trap_push();
	result = XGetWindowProperty(GDK_DISPLAY_XDISPLAY(display), GDK_WINDOW_XID(root_win),
		gdk_x11_get_xatom_by_name_for_display(display, "_NET_CURRENT_DESKTOP"),
		0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
		&bytes_after, (gpointer) & current_desktop);
	err = gdk_error_trap_pop();

	if (err != Success || result != Success)
		return ret;

	if (type == XA_CARDINAL && format == 32 && nitems > 0)
		ret = current_desktop[0];

	XFree(current_desktop);
	return ret;
#endif
#else
	/* FIXME: on mac etc proably there are native APIs
	 * to get the current workspace etc */
	return 0;
#endif
}

/* This function was copied from GEdit (gedit-utils.c). */
guint remmina_public_get_window_workspace(GtkWindow *gtkwindow)
{
	TRACE_CALL(__func__);
#ifdef GDK_WINDOWING_X11
#if GTK_CHECK_VERSION(3, 10, 0)
	GdkWindow *window;
	g_return_val_if_fail(GTK_IS_WINDOW(gtkwindow), 0);
	g_return_val_if_fail(gtk_widget_get_realized(GTK_WIDGET(gtkwindow)), 0);
	window = gtk_widget_get_window(GTK_WIDGET(gtkwindow));
	if (GDK_IS_X11_DISPLAY(gdk_window_get_display(window)))
		return gdk_x11_window_get_desktop(window);
	else
		return 0;
#else
	GdkWindow *window;
	GdkDisplay *display;
	Atom type;
	gint format;
	gulong nitems;
	gulong bytes_after;
	guint *workspace;
	gint err, result;
	guint ret = 0;

	g_return_val_if_fail(GTK_IS_WINDOW(gtkwindow), 0);
	g_return_val_if_fail(gtk_widget_get_realized(GTK_WIDGET(gtkwindow)), 0);

	window = gtk_widget_get_window(GTK_WIDGET(gtkwindow));
	display = gdk_window_get_display(window);

	gdk_error_trap_push();
	result = XGetWindowProperty(GDK_DISPLAY_XDISPLAY(display), GDK_WINDOW_XID(window),
		gdk_x11_get_xatom_by_name_for_display(display, "_NET_WM_DESKTOP"),
		0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
		&bytes_after, (gpointer) & workspace);
	err = gdk_error_trap_pop();

	if (err != Success || result != Success)
		return ret;

	if (type == XA_CARDINAL && format == 32 && nitems > 0)
		ret = workspace[0];

	XFree(workspace);
	return ret;
#endif
#else
	/* FIXME: on mac etc proably there are native APIs
	 * to get the current workspace etc */
	return 0;
#endif
}

/* Find hardware keycode for the requested keyval */
guint16 remmina_public_get_keycode_for_keyval(GdkKeymap *keymap, guint keyval)
{
	TRACE_CALL(__func__);
	GdkKeymapKey *keys = NULL;
	gint length = 0;
	guint16 keycode = 0;

	if (gdk_keymap_get_entries_for_keyval(keymap, keyval, &keys, &length)) {
		keycode = keys[0].keycode;
		g_free(keys);
	}
	return keycode;
}

/* Check if the requested keycode is a key modifier */
gboolean remmina_public_get_modifier_for_keycode(GdkKeymap *keymap, guint16 keycode)
{
	TRACE_CALL(__func__);
	g_return_val_if_fail(keycode > 0, FALSE);
#ifdef GDK_WINDOWING_X11
	return gdk_x11_keymap_key_is_modifier(keymap, keycode);
#else
	return FALSE;
#endif
}

/* Load a GtkBuilder object from a filename */
GtkBuilder* remmina_public_gtk_builder_new_from_file(gchar *filename)
{
	TRACE_CALL(__func__);
	GError *err = NULL;
	gchar *ui_path = g_strconcat(REMMINA_RUNTIME_UIDIR, G_DIR_SEPARATOR_S, filename, NULL);
	GtkBuilder *builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, ui_path, &err);
	if (err != NULL) {
		g_print("Error adding build from file. Error: %s", err->message);
		g_error_free(err);
	}
	g_free(ui_path);
	return builder;
}

/* Change parent container for a widget
 * If possible use this function instead of the deprecated gtk_widget_reparent */
void remmina_public_gtk_widget_reparent(GtkWidget *widget, GtkContainer *container)
{
	TRACE_CALL(__func__);
	g_object_ref(widget);
	gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(widget)), widget);
	gtk_container_add(container, widget);
	g_object_unref(widget);
}

/* Validate the inserted value for a new resolution */
gboolean remmina_public_resolution_validation_func(const gchar *new_str, gchar **error)
{
	TRACE_CALL(__func__);
	gint i;
	gint width, height;
	gboolean splitted;
	gboolean result;

	width = 0;
	height = 0;
	splitted = FALSE;
	result = TRUE;
	for (i = 0; new_str[i] != '\0'; i++) {
		if (new_str[i] == 'x') {
			if (splitted) {
				result = FALSE;
				break;
			}
			splitted = TRUE;
			continue;
		}
		if (new_str[i] < '0' || new_str[i] > '9') {
			result = FALSE;
			break;
		}
		if (splitted) {
			height = 1;
		}else  {
			width = 1;
		}
	}

	if (width == 0 || height == 0)
		result = FALSE;

	if (!result)
		*error = g_strdup(_("Please enter format 'widthxheight'."));
	return result;
}

/* Used to send desktop notifications */
void remmina_public_send_notification(const gchar *notification_id,
				      const gchar *notification_title, const gchar *notification_message)
{
	TRACE_CALL(__func__);

	GNotification *notification = g_notification_new(notification_title);
	g_notification_set_body(notification, notification_message);
#if GLIB_CHECK_VERSION(2, 42, 0)
	g_notification_set_priority(notification, G_NOTIFICATION_PRIORITY_NORMAL);
#endif
	g_application_send_notification(g_application_get_default(), notification_id, notification);
	g_object_unref(notification);
}

/* Replaces all occurences of search in a new copy of string by replacement. */
gchar* remmina_public_str_replace(const gchar *string, const gchar *search, const gchar *replacement)
{
	TRACE_CALL(__func__);
	gchar *str, **arr;

	g_return_val_if_fail(string != NULL, NULL);
	g_return_val_if_fail(search != NULL, NULL);

	if (replacement == NULL)
		replacement = "";

	arr = g_strsplit(string, search, -1);
	if (arr != NULL && arr[0] != NULL)
		str = g_strjoinv(replacement, arr);
	else
		str = g_strdup(string);

	g_strfreev(arr);
	return str;
}

/* Replaces all occurences of search in a new copy of string by replacement
 * and overwrites the original string */
gchar* remmina_public_str_replace_in_place(gchar *string, const gchar *search, const gchar *replacement)
{
	TRACE_CALL(__func__);
	gchar *new_string = remmina_public_str_replace(string, search, replacement);
	g_free(string);
	string = g_strdup(new_string);
	return string;
}

int remmina_public_split_resolution_string(const char *resolution_string, int *w, int *h)
{
	int lw, lh;

	if (resolution_string == NULL || resolution_string[0] == 0)
		return 0;
	if (sscanf(resolution_string, "%dx%d", &lw, &lh) != 2)
		return 0;
	*w = lw;
	*h = lh;
	return 1;
}

/* Return TRUE if current gtk version library in use is greater or equal than
 * the requered major.minor.micro */
gboolean remmina_gtk_check_version(guint major, guint minor, guint micro)
{
	guint rtmajor, rtminor, rtmicro;
	rtmajor = gtk_get_major_version();
	if (rtmajor > major) {
		return TRUE;
	}else if (rtmajor == major) {
		rtminor = gtk_get_minor_version();
		if (rtminor > minor) {
			return TRUE;
		}else if (rtminor == minor) {
			rtmicro = gtk_get_micro_version();
			if (rtmicro >= micro) {
				return TRUE;
			}else {
				return FALSE;
			}
		}else {
			return FALSE;
		}
	}else {
		return FALSE;
	}
}
