/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2018 Antenore Gatta, Giovanni Panozzo
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
#include <string.h>
#include <libsoup/soup.h>
#include "remmina/remmina_trace_calls.h"
#include "remmina_stats.h"
#include "remmina_pref.h"

#if !JSON_CHECK_VERSION(1,2,0)
	#define json_node_unref(x) json_node_free(x)
#endif

/* Timers */
#define PERIODIC_CHECK_1ST_MS 60000
#define PERIODIC_CHECK_INTERVAL_MS 600000

#define PERIODIC_UPLOAD_INTERVAL_SEC	604800

#define PERIODIC_UPLOAD_URL "http://s1.casa.panozzo.it/remmina/upload_stats.php"


static gint periodic_check_source;
static gint periodic_check_counter;

#define DEBUG_HTTP_SERVER_RESPONSE


static void soup_callback(SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	TRACE_CALL(__func__);
	gchar *s = (gchar *)user_data;
	SoupBuffer *sb;
	gboolean passed;
	GTimeVal t;

	g_free(s);

#ifdef DEBUG_HTTP_SERVER_RESPONSE
	printf("soup_callback status code is %d\n", msg->status_code);
#endif

	if (msg->status_code != 200)
		return;

	passed = FALSE;
	sb = soup_message_body_flatten(msg->response_body);
	if (strncmp(sb->data,"200 ", 4) != 0) {
#ifdef DEBUG_HTTP_SERVER_RESPONSE
		puts("Error from server side script:");
		puts(sb->data);
#endif
	} else {
		passed = TRUE;
	}
	soup_buffer_free(sb);

	if (passed) {
		g_get_current_time(&t);
		remmina_pref.periodic_usage_stats_last_sent = t.tv_sec;
		remmina_pref_save();
	}

}

static gboolean remmina_stats_collector_done(gpointer data)
{
	TRACE_CALL(__func__);
	JsonNode *n;
	JsonGenerator *g;
	gchar *s;
	SoupSession *ss;
	SoupMessage *msg;

	n = (JsonNode *)data;
	if (n == NULL)
		return G_SOURCE_REMOVE;

	g = json_generator_new();
	json_generator_set_root(g, n);
	s = json_generator_to_data(g, NULL);
	g_object_unref(g);
	json_node_unref(n);

	ss = soup_session_new();
	msg = soup_message_new("POST", PERIODIC_UPLOAD_URL);
	soup_message_set_request(msg, "application/json",
		SOUP_MEMORY_COPY, s, strlen (s));
	soup_session_queue_message(ss, msg, soup_callback, s);

#ifdef DEBUG_HTTP_SERVER_RESPONSE
	printf("Starting upload to url %s\n", PERIODIC_UPLOAD_URL);
#endif

	return G_SOURCE_REMOVE;
}

static gpointer remmina_stats_collector(gpointer data)
{
	TRACE_CALL(__func__);
	JsonNode *n;
	n = remmina_stats_get_all();

	/* stats collecting is done. Notify main thread calling
	 * remmina_stats_collector_done() */
	g_idle_add(remmina_stats_collector_done, n);

	return NULL;
}

void remmina_stats_sender_send()
{
	TRACE_CALL(__func__);

	g_thread_new("stats_collector", remmina_stats_collector, NULL);


}


static gboolean remmina_stats_sender_periodic_check(gpointer user_data)
{
	TRACE_CALL(__func__);
	GTimeVal t;
	glong next;

	if (!remmina_pref.periodic_usage_stats_permission_asked || !remmina_pref.periodic_usage_stats_permitted)
		return G_SOURCE_REMOVE;

	/* Calculate "next" upload time based on last sent time */
	next = remmina_pref.periodic_usage_stats_last_sent + PERIODIC_UPLOAD_INTERVAL_SEC;
	g_get_current_time(&t);
	/* If current time is after "next" or clock is going back (but > 1/1/2018), then do send stats */
	if (t.tv_sec > next || (t.tv_sec < remmina_pref.periodic_usage_stats_last_sent && t.tv_sec > 1514764800)) {
		remmina_stats_sender_send();
	} else {
#ifdef DEBUG_HTTP_SERVER_RESPONSE
		printf("Doing nothing: next upload is scheduled in %ld seconds\n", (next-t.tv_sec));
#endif
	}

	periodic_check_counter ++;
	if (periodic_check_counter <= 1) {
		/* Reschedule periodic check less frequently after 1st tick.
		 * Note that PERIODIC_CHECK_INTERVAL_MS becomes also a retry interval in case of
		 * upload failure */
		periodic_check_source = g_timeout_add_full(G_PRIORITY_LOW, PERIODIC_CHECK_INTERVAL_MS, remmina_stats_sender_periodic_check, NULL, NULL);
		return G_SOURCE_REMOVE;
	}
	return G_SOURCE_CONTINUE;
}

void remmina_stats_sender_schedule()
{
	TRACE_CALL(__func__);
	/* If permitted, schedule the 1st statistics periodic check */
	if (remmina_pref.periodic_usage_stats_permission_asked && remmina_pref.periodic_usage_stats_permitted) {
		periodic_check_counter = 0;
		periodic_check_source = g_timeout_add_full(G_PRIORITY_LOW, PERIODIC_CHECK_1ST_MS, remmina_stats_sender_periodic_check, NULL, NULL);
	} else
		periodic_check_source = 0;
}



