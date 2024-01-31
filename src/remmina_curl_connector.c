/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
 * Copyright (C) 2023-2024 Hiroyuki Tanaka, Sunil Bhat
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

#include <curl/curl.h>
#include <gtk/gtk.h>
#include <json-glib/json-glib.h>

#include "remmina.h"
#include "remmina_info.h"
#include "remmina_log.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref.h"
#include "remmina_curl_connector.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_utils.h"


#ifdef GDK_WINDOWING_WAYLAND
	#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
	#include <gdk/gdkx.h>
#endif

extern gboolean info_disable_stats;
extern gboolean info_disable_news;
extern gboolean info_disable_tip;

struct curl_msg {
	gchar *body;
	gchar *url;
	gpointer *user_data;
	char* response;
	size_t size;
};

enum ShowValue {ShowNews, ShowTip, ShowNone};


// If we receive a response from the server parse out the message
// and call the appropiate functions to handle the message.
void handle_resp(struct curl_msg * message){
	gchar *news_checksum = NULL;
	JsonParser *parser = NULL;
	enum ShowValue to_show = ShowNone;

	if (!imode) {
		parser = json_parser_new();
		if (!parser) {
			return;
		}
		if (!json_parser_load_from_data(parser, message->response , message->size, NULL)){
			g_object_unref(parser);
			return;
		}
		JsonReader *reader = json_reader_new(json_parser_get_root(parser));
		if (!reader) {
			g_object_unref(parser);
			return;
		}
		const gchar *json_tip_string;
		const gchar *json_news_string;
		gint64 	json_stats_int = 0;

		if (!json_reader_read_element(reader, 0)) {
			g_object_unref(parser);
			g_object_unref(reader);
			return;
		}

		if (!json_reader_read_member(reader, "TIP")) {
			json_tip_string = NULL;
		}
		else {
			json_tip_string = json_reader_get_string_value(reader);
		}
		json_reader_end_member(reader);

		if (!json_reader_read_member(reader, "NEWS")) {
			json_news_string = NULL;
		}
		else {
			json_news_string = json_reader_get_string_value(reader);

		}
		json_reader_end_member(reader);

		if (!json_reader_read_member(reader, "STATS")) {
			json_stats_int = 0;
		}
		else {
			json_stats_int = json_reader_get_int_value(reader);
		}
		json_reader_end_member(reader);
		
		if (json_reader_read_member(reader, "LIST")){
			g_idle_add(remmina_plugin_manager_parse_plugin_list, json_reader_new(json_parser_get_root(parser)));
		}
		json_reader_end_member(reader);

		if (json_reader_read_member(reader, "PLUGINS")){
			g_idle_add(remmina_plugin_manager_download_plugins, json_reader_new(json_parser_get_root(parser)));
		}
	
		json_reader_end_member(reader);
		json_reader_end_element(reader);
		g_object_unref(reader);

		if (json_news_string == NULL) {
			if (json_tip_string == NULL) {
				to_show = ShowNone;
			}
			else if (info_disable_tip == 0) {
				to_show = ShowTip;
			}
		}
		else {
			news_checksum = remmina_sha256_buffer((const guchar *)json_news_string, strlen(json_news_string));
			if (news_checksum == NULL) {
				REMMINA_DEBUG("News checksum is NULL");
			}
			else if ((remmina_pref.periodic_news_last_checksum == NULL) || strcmp(news_checksum, remmina_pref.periodic_news_last_checksum) != 0) {
				REMMINA_DEBUG("Latest news checksum: %s", news_checksum);
				remmina_pref.periodic_news_last_checksum = g_strdup(news_checksum);
				remmina_pref_save();
				if (info_disable_news == 0) {
					to_show = ShowNews;
				}
			}
			
			if (to_show == ShowNone && json_tip_string != NULL && info_disable_tip == 0) {
				to_show = ShowTip;
			}
			g_free(news_checksum);
		}

		if (to_show == ShowTip) {
			RemminaInfoMessage *message = malloc(sizeof(RemminaInfoMessage));
			message->info_string= g_strdup(json_tip_string);
			message->title_string= g_strdup("Tip of the Day");
			g_idle_add(remmina_info_show_response, message);
		}
		else if (to_show == ShowNews) {
			RemminaInfoMessage *message = malloc(sizeof(RemminaInfoMessage));
			message->info_string= g_strdup(json_news_string);
			message->title_string= g_strdup("NEWS");
			g_idle_add(remmina_info_show_response, message);
		}

		if (json_stats_int){
			remmina_info_stats_collector();
		}

		g_object_unref(parser);
	}

}


// Allocate memory to hold response from the server in msg->response
size_t remmina_curl_process_response(void *buffer, size_t size, size_t nmemb, void *userp){
	size_t realsize = size * nmemb;
	struct curl_msg *msg = (struct curl_msg *)userp;
 
	char *ptr = realloc(msg->response, msg->size + realsize + 1);
	if (!ptr) {
		return 0;  /* out of memory! */
	}
	
	msg->response = ptr;
	memcpy(&(msg->response[msg->size]), buffer, realsize);
	msg->size += realsize;
	msg->response[msg->size] = 0;
	
	return realsize;
}


void remmina_curl_send_message(gpointer data)
{
	gchar *result_message = NULL;
    gchar *marked_up_message = NULL;
	struct curl_msg* message = (struct curl_msg*)data;
	message->size = 0;
	message->response = NULL;
	CURL* curl = curl_easy_init();
	if (curl){
		CURLcode res;
		curl_easy_setopt(curl, CURLOPT_URL, message->url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, message->body);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, remmina_curl_process_response);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, message);
		curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60);
		res = curl_easy_perform(curl);
		if(res != CURLE_OK){
            curl_easy_strerror(res);
			result_message = "Failure: Transport error occurred - see debug or logs for more information";
			marked_up_message = RED_TEXT(result_message);
		}
		else{
			handle_resp(message);
			result_message = "Success: Message sent successfully, please wait for report to automatically open a new issue. This may take a little while.";
			marked_up_message = GREEN_TEXT(result_message);

		}
		if (message->user_data != NULL){
			gtk_label_set_markup(GTK_LABEL(message->user_data), marked_up_message);
			
		}
		if (message->body){
			g_free(message->body);
		}
		if (message->response){
			g_free(message->response);
		}
		g_free(message);
		g_free(marked_up_message);
		curl_easy_cleanup(curl);
	}
}



void remmina_curl_compose_message(gchar* body, char* type, char* url, gpointer data)
{
	if (!imode){
		struct curl_msg* message = (struct curl_msg*)malloc(sizeof(struct curl_msg));
		message->body = body;
		message->url = url;
		message->user_data = data;
		g_thread_new("send_curl_message", (GThreadFunc)remmina_curl_send_message, message);
	}
	
}
