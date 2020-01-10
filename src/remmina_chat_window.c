/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include "remmina_chat_window.h"
#include "remmina/remmina_trace_calls.h"

G_DEFINE_TYPE( RemminaChatWindow, remmina_chat_window, GTK_TYPE_WINDOW)

enum {
	SEND_SIGNAL,
	LAST_SIGNAL
};

static guint remmina_chat_window_signals[LAST_SIGNAL] = { 0 };

static void remmina_chat_window_class_init(RemminaChatWindowClass* klass)
{
	TRACE_CALL(__func__);
	remmina_chat_window_signals[SEND_SIGNAL] = g_signal_new("send", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaChatWindowClass, send), NULL, NULL,
		g_cclosure_marshal_VOID__STRING, G_TYPE_NONE, 1, G_TYPE_STRING);
}

static void remmina_chat_window_init(RemminaChatWindow* window)
{
	TRACE_CALL(__func__);
	window->history_text = NULL;
	window->send_text = NULL;
}

static void remmina_chat_window_clear_send_text(GtkWidget* widget, RemminaChatWindow* window)
{
	TRACE_CALL(__func__);
	GtkTextBuffer* buffer;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->send_text));
	gtk_text_buffer_set_text(buffer, "", -1);
	gtk_widget_grab_focus(window->send_text);
}

static gboolean remmina_chat_window_scroll_proc(RemminaChatWindow* window)
{
	TRACE_CALL(__func__);
	GtkTextBuffer* buffer;
	GtkTextIter iter;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->history_text));
	gtk_text_buffer_get_end_iter(buffer, &iter);
	gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(window->history_text), &iter, 0.0, FALSE, 0.0, 0.0);

	return FALSE;
}

static void remmina_chat_window_append_text(RemminaChatWindow* window, const gchar* name, const gchar* tagname,
					    const gchar* text)
{
	TRACE_CALL(__func__);
	GtkTextBuffer* buffer;
	GtkTextIter iter;
	gchar* ptr;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->history_text));

	if (name) {
		ptr = g_strdup_printf("(%s) ", name);
		gtk_text_buffer_get_end_iter(buffer, &iter);
		if (tagname) {
			gtk_text_buffer_insert_with_tags_by_name(buffer, &iter, ptr, -1, tagname, NULL);
		}else  {
			gtk_text_buffer_insert(buffer, &iter, ptr, -1);
		}
		g_free(ptr);
	}

	if (text && text[0] != 0) {
		gtk_text_buffer_get_end_iter(buffer, &iter);
		if (text[strlen(text) - 1] == '\n') {
			gtk_text_buffer_insert(buffer, &iter, text, -1);
		}else  {
			ptr = g_strdup_printf("%s\n", text);
			gtk_text_buffer_insert(buffer, &iter, ptr, -1);
			g_free(ptr);
		}
	}

	/* Use g_idle_add to make the scroll happen after the text has been actually updated */
	g_idle_add((GSourceFunc)remmina_chat_window_scroll_proc, window);
}

static void remmina_chat_window_send(GtkWidget* widget, RemminaChatWindow* window)
{
	TRACE_CALL(__func__);
	GtkTextBuffer* buffer;
	GtkTextIter start, end;
	gchar* text;

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(window->send_text));
	gtk_text_buffer_get_bounds(buffer, &start, &end);
	text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	if (!text || text[0] == '\0')
		return;

	g_signal_emit_by_name(G_OBJECT(window), "send", text);

	remmina_chat_window_append_text(window, g_get_user_name(), "sender-foreground", text);

	g_free(text);

	remmina_chat_window_clear_send_text(widget, window);
}

static gboolean remmina_chat_window_send_text_on_key(GtkWidget* widget, GdkEventKey* event, RemminaChatWindow* window)
{
	TRACE_CALL(__func__);
	if (event->keyval == GDK_KEY_Return) {
		remmina_chat_window_send(widget, window);
		return TRUE;
	}
	return FALSE;
}

GtkWidget*
remmina_chat_window_new(GtkWindow* parent, const gchar* chat_with)
{
	TRACE_CALL(__func__);
	RemminaChatWindow* window;
	gchar buf[100];
	GtkWidget* grid;
	GtkWidget* scrolledwindow;
	GtkWidget* widget;
	GtkWidget* image;
	GtkTextBuffer* buffer;

	window = REMMINA_CHAT_WINDOW(g_object_new(REMMINA_TYPE_CHAT_WINDOW, NULL));

	if (parent) {
		gtk_window_set_transient_for(GTK_WINDOW(window), parent);
	}

	/* Title */
	g_snprintf(buf, sizeof(buf), _("Chat with %s"), chat_with);
	gtk_window_set_title(GTK_WINDOW(window), buf);
	gtk_window_set_default_size(GTK_WINDOW(window), 450, 300);

	/* Main container */
	grid = gtk_grid_new();
	gtk_widget_show(grid);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 4);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
	gtk_container_add(GTK_CONTAINER(window), grid);

	/* Chat history */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolledwindow), 100);
	gtk_widget_set_hexpand(GTK_WIDGET(scrolledwindow), TRUE);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_grid_attach(GTK_GRID(grid), scrolledwindow, 0, 0, 3, 1);

	widget = gtk_text_view_new();
	gtk_widget_show(widget);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widget), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(widget), FALSE);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), widget);

	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
	gtk_text_buffer_create_tag(buffer, "sender-foreground", "foreground", "blue", NULL);
	gtk_text_buffer_create_tag(buffer, "receiver-foreground", "foreground", "red", NULL);

	window->history_text = widget;

	/* Chat message to be sent */
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scrolledwindow), 100);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_widget_set_hexpand(GTK_WIDGET(scrolledwindow), TRUE);
	gtk_grid_attach(GTK_GRID(grid), scrolledwindow, 0, 1, 3, 1);

	widget = gtk_text_view_new();
	gtk_widget_show(widget);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widget), GTK_WRAP_WORD_CHAR);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), widget);
	g_signal_connect(G_OBJECT(widget), "key-press-event", G_CALLBACK(remmina_chat_window_send_text_on_key), window);

	window->send_text = widget;

	/* Send button */
	image = gtk_image_new_from_icon_name("remmina-document-send-symbolic", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(image);

	widget = gtk_button_new_with_mnemonic(_("_Send"));
	gtk_widget_show(widget);
	gtk_button_set_image(GTK_BUTTON(widget), image);
	gtk_grid_attach(GTK_GRID(grid), widget, 2, 2, 1, 1);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_chat_window_send), window);

	/* Clear button */
	image = gtk_image_new_from_icon_name("edit-clear", GTK_ICON_SIZE_BUTTON);
	gtk_widget_show(image);

	widget = gtk_button_new_with_mnemonic(_("_Clear"));
	gtk_widget_show(widget);
	gtk_button_set_image(GTK_BUTTON(widget), image);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 2, 1, 1);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_chat_window_clear_send_text), window);

	gtk_widget_grab_focus(window->send_text);

	return GTK_WIDGET(window);
}

void remmina_chat_window_receive(RemminaChatWindow* window, const gchar* name, const gchar* text)
{
	TRACE_CALL(__func__);
	remmina_chat_window_append_text(window, name, "receiver-foreground", text);
}

