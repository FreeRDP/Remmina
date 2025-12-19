/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 * If you modify file(s) with this exception, you may extend this exception
 * to your version of the file(s), but you are not obligated to do so.
 * If you do not wish to do so, delete this exception statement from your
 * version.
 * If you delete this exception statement from all source
 * files in the program, then also delete it here.
 *
 */

#include <glib/gi18n.h>
#include <string.h>
#include "remmina_log.h"
#include "remmina_bug_report.h"
#include "remmina_public.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_info.h"
#include "remmina_curl_connector.h"
#include "remmina_utils.h"

#if !JSON_CHECK_VERSION(1, 2, 0)
	#define json_node_unref(x) json_node_free(x)
#endif

#define BUG_REPORT_UPLOAD_URL "https://info.remmina.org/bug_report/bug_report"
static RemminaBugReportDialog *remmina_bug_report_dialog;

#define GET_OBJECT(object_name) gtk_builder_get_object(remmina_bug_report_dialog->builder, object_name)

#define TEMP_LOG_TEXT_FILE_NAME "remmina_shorter_logs.log"
#define COMPRESSED_LOG_FILE_NAME "remmina_log_file.gz"
#define LOG_RECENT_WINDOW 4000
#define MAX_DESCRIPTION_LENGTH 8000
const char *bug_report_preview_text_md = \
"## Problem Description\n"
"\n"
"Write a detailed description of the problem.\n"
"\n"
"## What is the expected correct behavior?\n"
"\n"
"(What you want to see instead.)\n"
"\n"
"## Remote System Description\n"
"\n"
"* Server (OS name and version):\n"
"* Special notes regarding the remote system (i.e. gateways, tunnel, etc.):\n"
"\n"
"## Debugging\n"
"\n"
"Please check the Include Debug Data box to allow for debug log to be sent with this bug report\n"
"\n"
"## Local System Description\n"
"\n"
"* Client (OS name and version):\n"
"* Remmina version ( ```remmina --version``` ):\n"
"* Installation(s):\n"
"  - [ ] Distribution package.\n"
"  - [ ] PPA.\n"
"  - [ ] Snap.\n"
"  - [ ] Flatpak.\n"
"  - [ ] Compiled from sources.\n"
"  - [ ] Other - detail:\n"
"* Desktop environment (GNOME, Unity, KDE, ..):\n"
"* Plugin(s):\n"
"  - [ ] RDP - freerdp version ( ```xfreerdp --version``` ):\n"
"  - [ ] VNC\n"
"  - [ ] SSH\n"
"  - [ ] SFTP\n"
"  - [ ] SPICE\n"
"  - [ ] WWW\n"
"  - [ ] EXEC\n"
"  - [ ] Other (please specify):\n"
"* GTK backend (Wayland, Xorg):\n"
"\n";


/* Show the bug report dialog from the file ui/remmina_bug_report.glade */
void remmina_bug_report_open(GtkWindow *parent)
{
	TRACE_CALL(__func__);
	GtkTextBuffer* bug_report_preview_text_buffer;
	remmina_bug_report_dialog = g_new0(RemminaBugReportDialog, 1);

	remmina_bug_report_dialog->builder = remmina_public_gtk_builder_new_from_resource ("/org/remmina/Remmina/src/../data/ui/remmina_bug_report.glade");
	remmina_bug_report_dialog->dialog = GTK_WIDGET(gtk_builder_get_object(remmina_bug_report_dialog->builder, "RemminaBugReportDialog"));
	remmina_bug_report_dialog->bug_report_submit_button = GTK_BUTTON(GET_OBJECT("bug_report_submit_button"));
	remmina_bug_report_dialog->bug_report_name_entry = GTK_ENTRY(GET_OBJECT("bug_report_name_entry"));
	remmina_bug_report_dialog->bug_report_email_entry = GTK_ENTRY(GET_OBJECT("bug_report_email_entry"));
	remmina_bug_report_dialog->bug_report_title_entry = GTK_ENTRY(GET_OBJECT("bug_report_title_entry"));
	remmina_bug_report_dialog->bug_report_description_textview = GTK_TEXT_VIEW(GET_OBJECT("bug_report_description_textview"));
	remmina_bug_report_dialog->bug_report_submit_status_label = GTK_LABEL(GET_OBJECT("bug_report_submit_status_label"));
	remmina_bug_report_dialog->bug_report_debug_data_check_button = GTK_CHECK_BUTTON(GET_OBJECT("bug_report_debug_data_check_button"));
	remmina_bug_report_dialog->bug_report_include_system_info_check_button = GTK_CHECK_BUTTON(GET_OBJECT("bug_report_include_system_info_check_button"));

	// Set bug report markdown text in bug description window
	bug_report_preview_text_buffer = gtk_text_buffer_new(NULL);
	gtk_text_buffer_set_text(bug_report_preview_text_buffer, bug_report_preview_text_md, strlen(bug_report_preview_text_md));
	gtk_text_view_set_buffer(remmina_bug_report_dialog->bug_report_description_textview, bug_report_preview_text_buffer);

	if (parent) {
		gtk_window_set_transient_for(GTK_WINDOW(remmina_bug_report_dialog->dialog), parent);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(remmina_bug_report_dialog->dialog), TRUE);
	}

	g_signal_connect(remmina_bug_report_dialog->bug_report_submit_button, "clicked",
			G_CALLBACK(remmina_bug_report_dialog_on_action_submit), (gpointer)remmina_bug_report_dialog);

	gtk_window_present(GTK_WINDOW(remmina_bug_report_dialog->dialog));
	g_object_unref(bug_report_preview_text_buffer);
}


void remmina_bug_report_dialog_on_action_submit(GSimpleAction *action, GVariant *param, gpointer data)
{
	TRACE_CALL(__func__);
	JsonGenerator *g;
	JsonObject *o;
	gchar *b;

	REMMINA_DEBUG("Submit Button Clicked. Uploading Bug Report data.");

	gchar *markup = GRAY_TEXT("Sending Bug Report...");
	gtk_label_set_markup(remmina_bug_report_dialog->bug_report_submit_status_label, markup);
	g_free(markup);

	// Store bug report info in JSON blob and encode before submitting
	JsonNode *bug_report_data = remmina_bug_report_get_all();

	if (bug_report_data == NULL || (o = json_node_get_object(bug_report_data)) == NULL) {
		REMMINA_DEBUG("Failed to grab bug report data, no request is sent");
		gchar *markup = RED_TEXT("Failure: Unable to generate bug report message. Please try again.");
		gtk_label_set_markup(remmina_bug_report_dialog->bug_report_submit_status_label, markup);
		g_free(markup);
		json_node_unref(bug_report_data);
		return;
	} else if (strcmp(json_object_get_string_member(json_node_get_object(bug_report_data), "Name"), "") == 0) {
		REMMINA_DEBUG("No text in name entry of bug report data, no request is sent");
		gchar *markup = RED_TEXT("Failure: Name/Username is required. Please enter a Name/Username.");
		gtk_label_set_markup(remmina_bug_report_dialog->bug_report_submit_status_label, markup);
		g_free(markup);
		json_node_unref(bug_report_data);
		return;
	} else {
		gchar *email_text = g_strdup(json_object_get_string_member(json_node_get_object(bug_report_data), "Email"));
		if (strcmp(email_text, "") == 0) {
			REMMINA_DEBUG("No text in email entry of bug report data, no request is sent");
			gchar *markup = RED_TEXT("Failure: Email is required. Please enter an email.");
			gtk_label_set_markup(remmina_bug_report_dialog->bug_report_submit_status_label, markup);
			g_free(markup);
			g_free(email_text);
			json_node_unref(bug_report_data);
			return;
		} else {
			gchar *save_ptr;
			gchar *check_for_at_symbol = strtok_r(email_text, "@", &save_ptr);
			gchar *check_for_dot = strtok_r(NULL, ".", &save_ptr);
			gchar *check_for_domain = strtok_r(NULL, "", &save_ptr);
			if (check_for_at_symbol == NULL || check_for_dot == NULL || check_for_domain == NULL) {
				REMMINA_DEBUG("Text in email entry of bug report data is not a valid email, no request is sent");
				gchar *markup = RED_TEXT("Failure: A valid email is required. Email is missing a prefix or domain.");
				gtk_label_set_markup(remmina_bug_report_dialog->bug_report_submit_status_label, markup);
				g_free(markup);
				g_free(email_text);
				json_node_unref(bug_report_data);
				return;
			} else if (strpbrk(check_for_at_symbol, "@.") != NULL || strpbrk(check_for_dot, "@.") != NULL || strpbrk(check_for_domain, "@.") != NULL) {
				REMMINA_DEBUG("Text in email entry of bug report data is not a valid email, no request is sent");
				gchar *markup = RED_TEXT("Failure: A valid email is required. Email contains extra @ and . characters.");
				gtk_label_set_markup(remmina_bug_report_dialog->bug_report_submit_status_label, markup);
				g_free(markup);
				g_free(email_text);
				json_node_unref(bug_report_data);
				return;
			}
		}
		g_free(email_text);
		if (strcmp(json_object_get_string_member(json_node_get_object(bug_report_data), "Bug_Title"), "") == 0) {
			REMMINA_DEBUG("No text in bug title entry of bug report data, no request is sent");
			gchar *markup = RED_TEXT("Failure: Bug Title is required. Please enter a Bug Title.");
			gtk_label_set_markup(remmina_bug_report_dialog->bug_report_submit_status_label, markup);
			g_free(markup);
			json_node_unref(bug_report_data);
			return;
		} else if (strcmp(json_object_get_string_member(json_node_get_object(bug_report_data), "Bug_Description"), "") == 0 || 
				strcmp(json_object_get_string_member(json_node_get_object(bug_report_data), "Bug_Description"), bug_report_preview_text_md) == 0) {
			REMMINA_DEBUG("No text in bug description of bug report data, no request is sent");
			gchar *markup = RED_TEXT("Failure: Bug Description is required. Please fill out the template.");
			gtk_label_set_markup(remmina_bug_report_dialog->bug_report_submit_status_label, markup);
			g_free(markup);
			json_node_unref(bug_report_data);
			return;
		}
	}

	g = json_generator_new();
	json_generator_set_root(g, bug_report_data);
	b = json_generator_to_data(g, NULL);
	g_object_unref(g);

	remmina_curl_compose_message(b, "POST", BUG_REPORT_UPLOAD_URL, remmina_bug_report_dialog->bug_report_submit_status_label);
	json_node_unref(bug_report_data);
}

JsonNode *remmina_bug_report_get_all(void)
{
	TRACE_CALL(__func__);
	JsonBuilder *b_inner, *b_outer; // holds entire JSON blob
	JsonGenerator *g;
	JsonNode *n; // temp JSON node
	gchar *unenc_b, *enc_b;
	gchar *uid;

	// Initialize JSON blob
	b_outer = json_builder_new();
	if (b_outer == NULL) {
		g_object_unref(b_outer);
		return NULL;
	}
	json_builder_begin_object(b_outer);

	gboolean include_debug_data = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_bug_report_dialog->bug_report_debug_data_check_button));
	gboolean include_system_info = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(remmina_bug_report_dialog->bug_report_include_system_info_check_button));

	if (include_debug_data || include_system_info) {
		b_inner = json_builder_new();
		if (b_inner == NULL) {
			g_object_unref(b_inner);
			g_object_unref(b_outer);
			return NULL;
		}
		json_builder_begin_object(b_inner);
		
		if (include_system_info){
			n = remmina_info_stats_get_all();

			json_builder_set_member_name(b_outer, "System_Info_Enabled");
			json_builder_add_string_value(b_outer, "ON");

			json_builder_set_member_name(b_inner, "System_Info");
			json_builder_add_value(b_inner, n);
		}
		else{
			json_builder_set_member_name(b_outer, "System_Info_Enabled");
			json_builder_add_string_value(b_outer, "OFF");
		}

		if (include_debug_data){
			gboolean log_file_result;
			gchar *log_text;
			gsize log_length;
			GError *log_read_error;
			gchar *log_filename = g_build_filename(g_get_tmp_dir(), LOG_FILE_NAME, NULL);
			log_file_result = g_file_get_contents(log_filename, &log_text, &log_length, &log_read_error);
			g_free(log_filename);
			if (!log_file_result && log_read_error == NULL) {
				REMMINA_DEBUG("Failed to grab entire log text, read error: %s", log_read_error->message);
			}
			gboolean shorter_logs_result;
			gchar *temp_log_filename = g_build_filename(g_get_tmp_dir(), TEMP_LOG_TEXT_FILE_NAME, NULL);
			gchar *recent_log_text;
			if (log_length <= LOG_RECENT_WINDOW) {
				recent_log_text = log_text;
				shorter_logs_result = g_file_set_contents(temp_log_filename, recent_log_text, log_length, NULL);
			}
			else {
				recent_log_text = log_text + (log_length - LOG_RECENT_WINDOW);
				shorter_logs_result = g_file_set_contents(temp_log_filename, recent_log_text, LOG_RECENT_WINDOW, NULL);
			}
			g_free(log_text);
			if (!shorter_logs_result) {
				REMMINA_DEBUG("Failed to write recent window of logs to temp file.");
			}

			gchar *comp_log_filename = g_build_filename(g_get_tmp_dir(), COMPRESSED_LOG_FILE_NAME, NULL);
			GFile *in = g_file_new_for_path(temp_log_filename);
			GFile *out = g_file_new_for_path(comp_log_filename);
			// Clear out old compressed log file to prevent appending
			g_file_delete(out, NULL, NULL);
			remmina_compress_from_file_to_file(in, out);
			g_free(temp_log_filename);

			gboolean com_log_file_result;
			gchar *compressed_logs;
			gsize com_log_length;
			GError *com_log_read_error;
			com_log_file_result = g_file_get_contents(comp_log_filename, &compressed_logs, &com_log_length, &com_log_read_error);
			n = json_node_alloc();
			if (!com_log_file_result && com_log_read_error != NULL) {
				REMMINA_DEBUG("Failed to grab entire compressed log text, read error: %s", com_log_read_error->message);
				json_node_init_string(n, "");
			}
			else {
				gchar *logs_base64 = g_base64_encode((guchar *)compressed_logs, com_log_length);
				json_node_init_string(n, logs_base64);
				g_free(logs_base64);
			}

			json_builder_set_member_name(b_outer, "Debug_Log_Enabled");
			json_builder_add_string_value(b_outer, "ON");
			json_builder_set_member_name(b_inner, "Compressed_Logs");
			json_builder_add_value(b_inner, n);
			g_free(compressed_logs);
			g_free(comp_log_filename);

		}
		else{
			json_builder_set_member_name(b_outer, "Debug_Log_Enabled");
			json_builder_add_string_value(b_outer, "OFF");
		}

		// Wrap up by setting n to the final JSON blob
		json_builder_end_object(b_inner);
		n = json_builder_get_root(b_inner);
		g_object_unref(b_inner);

		g = json_generator_new();
		json_generator_set_root(g, n);
		json_node_unref(n);
		unenc_b = json_generator_to_data(g, NULL);	// unenc_b=bug report data
		g_object_unref(g);

		// Now base64 encode report data
		enc_b = g_base64_encode((guchar *)unenc_b, strlen(unenc_b));
		if (enc_b == NULL) {
			g_object_unref(b_outer);
			g_free(unenc_b);
			REMMINA_DEBUG("Failed to encode inner JSON");
			return NULL;
		}
		g_free(unenc_b);

		json_builder_set_member_name(b_outer, "encdata");
		json_builder_add_string_value(b_outer, enc_b);
		g_free(enc_b);
	}
	else {
		json_builder_set_member_name(b_outer, "System_Info_Enabled");
		json_builder_add_string_value(b_outer, "OFF");
		json_builder_set_member_name(b_outer, "Debug_Log_Enabled");
		json_builder_add_string_value(b_outer, "OFF");	
	}

	n = remmina_info_stats_get_uid();
	uid = g_strdup(json_node_get_string(n));
	json_builder_set_member_name(b_outer, "UID");
	json_builder_add_string_value(b_outer, uid);

	// Allocate new json node for each text entry and add to total blob
	n = json_node_alloc();
	json_node_init_string(n, gtk_entry_get_text(remmina_bug_report_dialog->bug_report_name_entry));
	json_builder_set_member_name(b_outer, "Name");
	json_builder_add_value(b_outer, n);

	n = json_node_alloc();
	json_node_init_string(n, gtk_entry_get_text(remmina_bug_report_dialog->bug_report_email_entry));
	json_builder_set_member_name(b_outer, "Email");
	json_builder_add_value(b_outer, n);

	n = json_node_alloc();
	json_node_init_string(n, gtk_entry_get_text(remmina_bug_report_dialog->bug_report_title_entry));
	json_builder_set_member_name(b_outer, "Bug_Title");
	json_builder_add_value(b_outer, n);

	GtkTextBuffer *buffer;
	gchar *description_text;
	GtkTextIter start, end;
	buffer = gtk_text_view_get_buffer(remmina_bug_report_dialog->bug_report_description_textview);
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_iter_at_offset(buffer, &end, MAX_DESCRIPTION_LENGTH);
	description_text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);

	n = json_node_alloc();
	json_node_init_string(n, description_text);
	json_builder_set_member_name(b_outer, "Bug_Description");
	json_builder_add_value(b_outer, n);

	json_builder_end_object(b_outer);
	n = json_builder_get_root(b_outer);
	g_object_unref(b_outer);

	g_free(uid);

	return n;
}
