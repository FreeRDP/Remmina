/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee
 * Copyright (C) 2017-2020 Antenore Gatta, Giovanni Panozzo
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

#define REMMINA_TYPE_CHAT_WINDOW               (remmina_chat_window_get_type())
#define REMMINA_CHAT_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj), REMMINA_TYPE_CHAT_WINDOW, RemminaChatWindow))
#define REMMINA_CHAT_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass), REMMINA_TYPE_CHAT_WINDOW, RemminaChatWindowClass))
#define REMMINA_IS_CHAT_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), REMMINA_TYPE_CHAT_WINDOW))
#define REMMINA_IS_CHAT_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass), REMMINA_TYPE_CHAT_WINDOW))
#define REMMINA_CHAT_WINDOW_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj), REMMINA_TYPE_CHAT_WINDOW, RemminaChatWindowClass))

typedef struct _RemminaChatWindow {
	GtkWindow	window;

	GtkWidget *	history_text;
	GtkWidget *	send_text;
} RemminaChatWindow;

typedef struct _RemminaChatWindowClass {
	GtkWindowClass parent_class;

	void (*send)(RemminaChatWindow *window);
} RemminaChatWindowClass;

GType remmina_chat_window_get_type(void)
G_GNUC_CONST;

GtkWidget *remmina_chat_window_new(GtkWindow *parent, const gchar *chat_with);
void remmina_chat_window_receive(RemminaChatWindow *window, const gchar *name, const gchar *text);

G_END_DECLS
