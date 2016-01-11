/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2014-2015 Antenore Gatta
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

#include <ctype.h>
#include <errno.h>
#include <gio/gio.h>
#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdlib.h>
#include <webkit2/webkit2.h>
#include "config.h"
#include "remmina_file.h"
#include "remmina_pref.h"
#include "remmina_public.h"
#include "remmina_survey.h"
#include "remmina/remmina_trace_calls.h"

static RemminaSurveyDialog *remmina_survey;

static WebKitWebView* web_view;

gchar *remmina_pref_file;
RemminaPref remmina_pref;

#define GET_OBJECT(object_name) gtk_builder_get_object(remmina_survey->builder, object_name)

/* Taken from http://creativeandcritical.net/str-replace-c */
static char *repl_str(const char *str, const char *old, const char *new)
{
	/* Increment positions cache size initially by this number. */
	size_t cache_sz_inc = 16;
	/* Thereafter, each time capacity needs to be increased,
	 * multiply the increment by this factor. */
	const size_t cache_sz_inc_factor = 3;
	/* But never increment capacity by more than this number. */
	const size_t cache_sz_inc_max = 1048576;

	char *pret, *ret = NULL;
	const char *pstr2, *pstr = str;
	size_t i, count = 0;
	ptrdiff_t *pos_cache = NULL;
	size_t cache_sz = 0;
	size_t cpylen, orglen, retlen, newlen, oldlen = strlen(old);

	/* Find all matches and cache their positions. */
	while ((pstr2 = strstr(pstr, old)) != NULL) {
		count++;

		/* Increase the cache size when necessary. */
		if (cache_sz < count) {
			cache_sz += cache_sz_inc;
			pos_cache = realloc(pos_cache, sizeof(*pos_cache) * cache_sz);
			if (pos_cache == NULL) {
				goto end_repl_str;
			}
			cache_sz_inc *= cache_sz_inc_factor;
			if (cache_sz_inc > cache_sz_inc_max) {
				cache_sz_inc = cache_sz_inc_max;
			}
		}

		pos_cache[count-1] = pstr2 - str;
		pstr = pstr2 + oldlen;
	}

	orglen = pstr - str + strlen(pstr);

	/* Allocate memory for the post-replacement string. */
	if (count > 0) {
		newlen = strlen(new);
		retlen = orglen + (newlen - oldlen) * count;
	} else	retlen = orglen;
	ret = malloc(retlen + 1);
	if (ret == NULL) {
		goto end_repl_str;
	}

	if (count == 0) {
		/* If no matches, then just duplicate the string. */
		strcpy(ret, str);
	} else {
		/* Otherwise, duplicate the string whilst performing
		 * the replacements using the position cache. */
		pret = ret;
		memcpy(pret, str, pos_cache[0]);
		pret += pos_cache[0];
		for (i = 0; i < count; i++) {
			memcpy(pret, new, newlen);
			pret += newlen;
			pstr = str + pos_cache[i] + oldlen;
			cpylen = (i == count-1 ? orglen : pos_cache[i+1]) - pos_cache[i] - oldlen;
			memcpy(pret, pstr, cpylen);
			pret += cpylen;
		}
		ret[retlen] = '\0';
	}

end_repl_str:
	/* Free the cache and return the post-replacement string,
	 * which will be NULL in the event of an error. */
	free(pos_cache);
	return ret;
}

/* Get the dirname where remmina files are stored TODO: fix xdg in all files */
static const gchar *remmina_get_datadir()
{
	TRACE_CALL("remmina_get_datadir");
	GDir *dir;
	static gchar remdir[PATH_MAX];
	GError *gerror = NULL;

	g_snprintf(remdir, sizeof(remdir), "%s/.%s", g_get_home_dir(), "remmina");
	dir = g_dir_open(remdir, 0, &gerror);
	if (gerror != NULL)
	{
		g_message("Cannot open %s, with error: %s", remdir, gerror->message);
		g_error_free(gerror);
		g_snprintf(remdir, sizeof(remdir),
				"%s/%s", g_get_user_data_dir(), "remmina");
	}else{
		g_dir_close(dir);
	}
	return remdir;
}

/* Insert setting name and its count in an hashtable */
static void remmina_survey_ret_stat_from_setting(GHashTable *hash_table, gchar *name)
{
	TRACE_CALL("remmina_survey_ret_stat_from_setting");
	gint count_name;

	count_name = GPOINTER_TO_INT(g_hash_table_lookup(hash_table, name));

	if (count_name)
	{
		count_name++;
		g_hash_table_replace(hash_table,
		                     g_strdup(name),
		                     GINT_TO_POINTER(count_name));
	} else {
		count_name = 1;
		g_hash_table_replace(hash_table,
		                     g_strdup(name),
		                     GINT_TO_POINTER(count_name));
	}
}

static gchar *remmina_survey_files_iter_setting()
{
	TRACE_CALL("remmina_survey_files_iter_setting");

	GDir *dir;
	gchar *dirname;
	gchar filename[PATH_MAX];
	const gchar *dir_entry;

	gint count_file=0;

	gchar *name;
	gchar *setting_name[] = {"protocol", "group", "ssh_enabled", "resolution"};

	gchar *ret = NULL;

	int i;
	int count_name = sizeof(setting_name) / sizeof(setting_name[0]);

	GKeyFile* gkeyfile;
	GHashTable *hash_table;
	GHashTableIter iter;
	gpointer key, value;

	dirname = g_strdup_printf("%s", remmina_get_datadir());
	dir = g_dir_open(dirname, 0, NULL);

	if (!dir)
		return FALSE;
	gkeyfile = g_key_file_new();
	hash_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

	while ((dir_entry = g_dir_read_name(dir)) != NULL) {
		/* Olny *.remmina files */
		if (!g_str_has_suffix(dir_entry, ".remmina\0"))
			continue;
		g_snprintf(filename, PATH_MAX, "%s/%s", dirname, dir_entry);

		if (!g_key_file_load_from_file(gkeyfile, filename, G_KEY_FILE_NONE, NULL))
			g_key_file_free(gkeyfile);

		if (!g_key_file_has_key(gkeyfile, "remmina", "name", NULL))
			return FALSE;
		count_file++;
		for (i = 0; i < count_name; i++) {
			if (setting_name[i])
				name = g_key_file_get_string(gkeyfile,
							     "remmina",
							     g_strdup_printf("%s", setting_name[i]),
							     NULL);
			/* Some settings exists only for certain plugin */
			if (name && !(*name == 0)){
				remmina_survey_ret_stat_from_setting(hash_table,
						g_strdup_printf("%s_%s",
								setting_name[i],
								name));
				g_free(name);
			}
		}
	}

	ret = g_strjoin(NULL,
			g_strdup_printf("ID: %s\n",
					remmina_pref.uid),
					ret,
					NULL);

	ret = g_strjoin(NULL,
			g_strdup_printf("Number of profiles: %d\n", count_file),
			ret,
			NULL);
	g_hash_table_iter_init (&iter, hash_table);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		ret = g_strjoin(NULL,
				g_strdup_printf("%s: %d\n", (gchar *)key, GPOINTER_TO_INT(value)),
				ret,
				NULL);
	}
	g_key_file_free(gkeyfile);
	g_hash_table_destroy(hash_table);
	g_dir_close(dir);
	return ret;
}

/* Create an html file from a template */
static void remmina_survey_stats_create_html_form()
{
	TRACE_CALL("remmina_survey_stats_create_html_form");

	gchar *dirname = NULL;
	const gchar *templateuri = REMMINA_SURVEY_URI;
	const gchar *output_file_name = "local_remmina_form.html";
	gchar *output_file_path = NULL;
	GFile *template;
	GFile *output_file;
	gchar *tpl_content = NULL;
	gchar *form = NULL;

	GError *gerror = NULL;

	const gchar old[] = "<!-- STATS HOLDER -->";
	gchar *new;

	dirname = g_strdup_printf("%s", remmina_get_datadir());

	output_file_path = g_strdup_printf("%s/%s", dirname, output_file_name);

	template = g_file_new_for_uri(templateuri);
	output_file = g_file_new_for_path(output_file_path);

	g_file_load_contents(template, NULL, &tpl_content, NULL, NULL, &gerror);
	g_object_unref(template);
	if (gerror != NULL)
	{
		g_message ("Cannot open %s, because of: %s", templateuri, gerror->message);
		g_error_free(gerror);
	}else{
		/* We get the remmina data we need */
		new = remmina_survey_files_iter_setting();

		/* We replace the placeholder in the template, with the remmina data */
		form = repl_str(tpl_content, old, new);

		g_file_replace_contents (output_file, form, strlen(form)+1,
				NULL,
				FALSE,
				G_FILE_CREATE_REPLACE_DESTINATION,
				NULL, NULL, &gerror);
		g_object_unref(output_file);
		if (gerror != NULL)
		{
			g_message ("Cannot open %s, because of: %s", output_file_path, gerror->message);
			g_error_free(gerror);
		}
	}
	g_free(tpl_content);
	g_free(form);
}

void remmina_survey_dialog_on_close_clicked(GtkWidget *widget, RemminaSurveyDialog *dialog)
{
	TRACE_CALL("remmina_survey_dialog_on_close_clicked");
	gtk_widget_destroy(GTK_WIDGET(remmina_survey->dialog));
}

static void remmina_survey_submit_form_callback(WebKitWebView *web_view, WebKitFormSubmissionRequest *request, gpointer user_data)
{
	TRACE_CALL("remmina_survey_submit_form_callback");

	/* CLEAN: just for test */
	gchar *name, *mail;

	/* This function return only html text fields (no textarea for instance) */
	GHashTable *formsdata = webkit_form_submission_request_get_text_fields (request);

	/* CLEAN: just for test */
	name = g_hash_table_lookup (formsdata, "name");
	mail = g_hash_table_lookup (formsdata, "email");

	/* Finally we submit the form */
	webkit_form_submission_request_submit(request);

}

/* Show the preliminary survey dialog when remmina start */
void remmina_survey_on_startup(GtkWindow *parent)
{
	TRACE_CALL("remmina_survey");

	GtkWidget *dialog, *check;

	dialog = gtk_message_dialog_new(GTK_WINDOW (parent),
					GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					"%s",
					/* translators: Primary message of a dialog used to notify the user about the survey */
					_("We are conducting a user survey\n would you like to take it now?"));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog), "%s",
			/* translators: Secondary text of a dialog used to notify the user about the survey */
			_("If not, you can always find it in the Help menu."));

	check = gtk_check_button_new_with_mnemonic (_("_Do not show this dialog again"));
	gtk_box_pack_end (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))),
			check, FALSE, FALSE, 0);
	gtk_widget_set_halign (check, GTK_ALIGN_START);
	gtk_widget_set_margin_start (check, 6);
	gtk_widget_show (check);

	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_YES)
	{
		remmina_survey_start(parent);
	}

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check)))
	{
		/* Save survey option */
		remmina_pref.survey = FALSE;
		remmina_pref_save();
	}

	gtk_widget_destroy(dialog);
}

void remmina_survey_start(GtkWindow *parent)
{
	TRACE_CALL("remmina_survey_start");

	GDir *dir;
	gchar *dirname;
	gchar localurl[PATH_MAX];

	dirname = g_strdup_printf("%s", remmina_get_datadir());
	dir = g_dir_open(dirname, 0, NULL);

	remmina_survey = g_new0(RemminaSurveyDialog, 1);

	/* setting up the form */
	remmina_survey_stats_create_html_form();

	remmina_survey->builder = remmina_public_gtk_builder_new_from_file("remmina_survey.glade");
	remmina_survey->dialog
		= GTK_DIALOG (gtk_builder_get_object(remmina_survey->builder, "dialog_remmina_survey"));
	remmina_survey->scrolledwindow
		= GTK_SCROLLED_WINDOW(GET_OBJECT("scrolledwindow"));
	/* Connect signals */
	gtk_builder_connect_signals(remmina_survey->builder, NULL);

	web_view = WEBKIT_WEB_VIEW(webkit_web_view_new());

	WebKitSettings *web_view_settings
		= webkit_settings_new_with_settings ("enable-caret-browsing", TRUE,
						     "enable-fullscreen", FALSE,
						     "enable-java", FALSE,
						     "enable-media-stream", FALSE,
						     "enable-plugins", FALSE,
						     "enable-private-browsing", TRUE,
						     "enable-offline-web-application-cache", FALSE,
						     "enable-page-cache", FALSE,
						     NULL);

	webkit_web_view_set_settings(web_view, web_view_settings);

	/* Intercept submit signal from WebKitWebView keeping the data so that we can deal with it */
	g_signal_connect(web_view, "submit-form", G_CALLBACK(remmina_survey_submit_form_callback), NULL);

	if (parent)
	{
		gtk_window_set_transient_for(GTK_WINDOW(remmina_survey->dialog), parent);
		gtk_window_set_destroy_with_parent(GTK_WINDOW(remmina_survey->dialog), TRUE);
	}

	g_signal_connect(remmina_survey->dialog, "response", G_CALLBACK(gtk_widget_destroy), NULL);
	gtk_window_present(GTK_WINDOW(remmina_survey->dialog));

	gtk_container_add(GTK_CONTAINER(remmina_survey->scrolledwindow), GTK_WIDGET(web_view));
	gtk_widget_show(GTK_WIDGET(web_view));
	g_snprintf(localurl, PATH_MAX, "%s%s/%s", "file://", dirname, "local_remmina_form.html");
	webkit_web_view_load_uri(web_view, localurl);
	g_object_unref(G_OBJECT(remmina_survey->builder));
}
