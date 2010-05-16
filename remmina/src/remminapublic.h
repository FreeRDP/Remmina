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
 */
 

#ifndef __REMMINAPUBLIC_H__
#define __REMMINAPUBLIC_H__

#include "config.h"

/* Wrapper marcos to make the compiler happy on both signle/multi-threaded mode */
#ifdef HAVE_PTHREAD
#define IDLE_ADD        gdk_threads_add_idle
#define TIMEOUT_ADD     gdk_threads_add_timeout
#define CANCEL_ASYNC    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);pthread_testcancel();
#define CANCEL_DEFER    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
#define THREADS_ENTER   gdk_threads_enter();pthread_cleanup_push(remmina_public_threads_leave,NULL);
#define THREADS_LEAVE   pthread_cleanup_pop(TRUE);
#else
#define IDLE_ADD        g_idle_add
#define TIMEOUT_ADD     g_timeout_add
#define CANCEL_ASYNC
#define CANCEL_DEFER
#define THREADS_ENTER
#define THREADS_LEAVE
#endif

#define MAX_PATH_LEN 255

#define MAX_X_DISPLAY_NUMBER 99
#define X_UNIX_SOCKET "/tmp/.X11-unix/X%d"

#define STRING_DELIMITOR ','

G_BEGIN_DECLS

/* items is separated by STRING_DELIMTOR */
GtkWidget* remmina_public_create_combo_entry (const gchar *text, const gchar *def, gboolean descending);
GtkWidget* remmina_public_create_combo_text_d (const gchar *text, const gchar *def, const gchar *empty_choice);
void remmina_public_load_combo_text_d (GtkWidget *combo, const gchar *text, const gchar *def, const gchar *empty_choice);
GtkWidget* remmina_public_create_combo (gboolean use_icon);
GtkWidget* remmina_public_create_combo_map (const gpointer *key_value_list, const gchar *def, gboolean use_icon);
GtkWidget* remmina_public_create_combo_mapint (const gpointer *key_value_list, gint def, gboolean use_icon);

void remmina_public_create_group (GtkTable* table, const gchar *group, gint row, gint rows, gint cols);

/* A function for gtk_menu_popup to get the position right below the widget specified by user_data */
void remmina_public_popup_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data);

/* Combine two paths into one by correctly handling trailing slash. Return newly allocated string */
gchar* remmina_public_combine_path (const gchar *path1, const gchar *path2);

/* Parse a server entry with server name and port */
void remmina_public_get_server_port (const gchar *server, gint defaultport, gchar **host, gint *port);

/* X */
gboolean remmina_public_get_xauth_cookie (const gchar *display, gchar **msg);
gint remmina_public_open_xdisplay (const gchar *disp);
guint remmina_public_get_current_workspace (GdkScreen *screen);
guint remmina_public_get_window_workspace (GtkWindow *gtkwindow);

void remmina_public_threads_leave (void* data);

G_END_DECLS

#endif  /* __REMMINAPUBLIC_H__  */

