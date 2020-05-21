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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remmina_public.h"
#include "remmina_log.h"
#include "remmina_stats_sender.h"
#include "remmina/remmina_trace_calls.h"

/***** Define the log window GUI *****/
#define REMMINA_TYPE_LOG_WINDOW               (remmina_log_window_get_type())
#define REMMINA_LOG_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj), REMMINA_TYPE_LOG_WINDOW, RemminaLogWindow))
#define REMMINA_LOG_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass), REMMINA_TYPE_LOG_WINDOW, RemminaLogWindowClass))
#define REMMINA_IS_LOG_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), REMMINA_TYPE_LOG_WINDOW))
#define REMMINA_IS_LOG_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass), REMMINA_TYPE_LOG_WINDOW))
#define REMMINA_LOG_WINDOW_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj), REMMINA_TYPE_LOG_WINDOW, RemminaLogWindowClass))

typedef struct _RemminaLogWindow {
	GtkWindow window;

	GtkWidget *log_view;
	GtkTextBuffer *log_buffer;
} RemminaLogWindow;

typedef struct _RemminaLogWindowClass {
	GtkWindowClass parent_class;
} RemminaLogWindowClass;

GType remmina_log_window_get_type(void)
G_GNUC_CONST;

G_DEFINE_TYPE(RemminaLogWindow, remmina_log_window, GTK_TYPE_WINDOW)

static void remmina_log_window_class_init(RemminaLogWindowClass *klass)
{
	TRACE_CALL(__func__);
}

/* We will always only have one log window per instance */
static GtkWidget *log_window = NULL;

static gboolean remmina_log_on_keypress(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	TRACE_CALL(__func__);
	GdkEventKey *e = (GdkEventKey *)event;

	if (!log_window)
		return FALSE;

	if ((e->state & GDK_CONTROL_MASK) == GDK_CONTROL_MASK) {
		if ((e->keyval == GDK_KEY_s || e->keyval == GDK_KEY_t) && remmina_stat_sender_can_send()) {
			remmina_stats_sender_send(e->keyval != GDK_KEY_s);
		}
		return TRUE;
	}

	return FALSE;
}

static void remmina_log_window_init(RemminaLogWindow *logwin)
{
	TRACE_CALL(__func__);
	GtkWidget *scrolledwindow;
	GtkWidget *widget;

	gtk_container_set_border_width(GTK_CONTAINER(logwin), 4);

	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_show(scrolledwindow);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_ALWAYS);
	gtk_container_add(GTK_CONTAINER(logwin), scrolledwindow);

	widget = gtk_text_view_new();
	gtk_widget_show(widget);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widget), GTK_WRAP_WORD_CHAR);
	gtk_text_view_set_editable(GTK_TEXT_VIEW(widget), FALSE);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), widget);
	logwin->log_view = widget;
	logwin->log_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));

	g_signal_connect(G_OBJECT(logwin->log_view), "key-press-event", G_CALLBACK(remmina_log_on_keypress), (gpointer)logwin);
}

static GtkWidget*
remmina_log_window_new(void)
{
	TRACE_CALL(__func__);
	return GTK_WIDGET(g_object_new(REMMINA_TYPE_LOG_WINDOW, NULL));
}

static void remmina_log_end(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	log_window = NULL;
}

void remmina_log_start(void)
{
	TRACE_CALL(__func__);
	if (log_window) {
		gtk_window_present(GTK_WINDOW(log_window));
	}else  {
		log_window = remmina_log_window_new();
		gtk_window_set_default_size(GTK_WINDOW(log_window), 640, 480);
		g_signal_connect(G_OBJECT(log_window), "destroy", G_CALLBACK(remmina_log_end), NULL);
		gtk_widget_show(log_window);
	}
	if (remmina_stat_sender_can_send())
		remmina_log_print("Shortcut keys for stats:\n"
			"\tCTRL+S: collect, show and send stats\n"
			"\tCTRL+T: collect and show stats\n");
}

gboolean remmina_log_running(void)
{
	TRACE_CALL(__func__);
	return (log_window != NULL);
}

static gboolean remmina_log_scroll_to_end(gpointer data)
{
	TRACE_CALL(__func__);
	GtkTextIter iter;

	if (log_window) {
		gtk_text_buffer_get_end_iter(REMMINA_LOG_WINDOW(log_window)->log_buffer, &iter);
		gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(REMMINA_LOG_WINDOW(log_window)->log_view), &iter, 0.0, FALSE, 0.0,
			0.0);
	}
	return FALSE;
}

static gboolean remmina_log_print_real(gpointer data)
{
	TRACE_CALL(__func__);
	GtkTextIter iter;

	if (log_window) {
		gtk_text_buffer_get_end_iter(REMMINA_LOG_WINDOW(log_window)->log_buffer, &iter);
		gtk_text_buffer_insert(REMMINA_LOG_WINDOW(log_window)->log_buffer, &iter, (const gchar*)data, -1);
		IDLE_ADD(remmina_log_scroll_to_end, NULL);
	}
	g_free(data);
	return FALSE;
}

void remmina_log_print(const gchar *text)
{
	TRACE_CALL(__func__);
	if (!log_window)
		return;

	IDLE_ADD(remmina_log_print_real, g_strdup(text));
}

/**
 * Print a string in the Remmina Debug Windows and in the terminal.
 * The string will be visible in the terminal if G_MESSAGES_DEBUG=all
 * Variadic function of REMMINA_DEBUG
 */
void _remmina_debug(const gchar *fun, const gchar *fmt, ...)
{
	TRACE_CALL(__func__);

	va_list args;
	gchar *text;
	va_start(args, fmt);
	text = g_strdup_vprintf(fmt, args);
	va_end(args);
	g_autofree gchar *buf = g_strconcat("(", fun, ") - ", text, NULL);

	g_debug ("%s", g_strdup(buf));

	buf = g_strconcat (buf, "\n", NULL);

	if (!log_window)
		return;
	IDLE_ADD(remmina_log_print_real, g_strdup(buf));
}

void remmina_log_printf(const gchar *fmt, ...)
{
	TRACE_CALL(__func__);
	va_list args;
	gchar *text;

	if (!log_window) return;

	va_start(args, fmt);
	text = g_strdup_vprintf(fmt, args);
	va_end(args);

	IDLE_ADD(remmina_log_print_real, text);
}

