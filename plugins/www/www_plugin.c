/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2019 Antenore Gatta & Giovanni Panozzo
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

#include "www_config.h"

#include "common/remmina_plugin.h"

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <stdlib.h>

#include <webkit2/webkit2.h>
#if WEBKIT_CHECK_VERSION(2, 21, 1)
#include <jsc/jsc.h>
#endif
#include "www_utils.h"
#include "www_plugin.h"


#define GET_PLUGIN_DATA(gp) (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data")

typedef struct _RemminaPluginWWWData {
	WWWWebViewDocumentType		document_type;
	GtkWidget *			box;
	WebKitSettings *		settings;
	WebKitWebContext *		context;
	WebKitWebsiteDataManager *	data_mgr;
	WebKitCredential *		credentials;
	WebKitAuthenticationRequest *	request;
	WebKitWebView *			webview;
	WebKitLoadEvent			load_event;

	gchar *				url;
	gboolean			authenticated;
	gboolean			formauthenticated;
} RemminaPluginWWWData;

static RemminaPluginService *remmina_plugin_service = NULL;

void remmina_plugin_www_download_started(WebKitWebContext *context,
					 WebKitDownload *download, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	webkit_download_set_allow_overwrite(download, TRUE);
	g_signal_connect(G_OBJECT(download), "notify::response",
			 G_CALLBACK(remmina_plugin_www_response_received), gp);
	g_signal_connect(download, "created-destination",
			G_CALLBACK(remmina_plugin_www_notify_download), gp);
}

void remmina_plugin_www_response_received(WebKitDownload *download, GParamSpec *ps, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	g_debug("Download response received");
}

void remmina_plugin_www_notify_download(WebKitDownload *download, gchar *destination, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	g_debug("Download is finished");
	const gchar *dest = webkit_download_get_destination(download);
	www_utils_send_notification("www-plugin-download-completed-id", _("File downloaded"), dest);
	//download(gp, webkit_download_get_response(download));
	//webkit_download_cancel(download);
}

static gboolean remmina_plugin_www_decide_policy_cb(
	WebKitWebView *			webview,
	WebKitPolicyDecision *		decision,
	WebKitPolicyDecisionType	decision_type,
	RemminaProtocolWidget *		gp)
{
	TRACE_CALL(__func__);

	gboolean res = TRUE;

	switch (decision_type) {
	case WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION:
		remmina_plugin_www_decide_nav(decision, gp);
		break;
	case WEBKIT_POLICY_DECISION_TYPE_NEW_WINDOW_ACTION:
		remmina_plugin_www_decide_newwin(decision, gp);
		break;
	case WEBKIT_POLICY_DECISION_TYPE_RESPONSE:
		res = remmina_plugin_www_decide_resource(decision, gp);
		break;
	default:
		webkit_policy_decision_ignore(decision);
		break;
	}
	return res;
}
void remmina_plugin_www_decide_nav(WebKitPolicyDecision *decision, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	g_debug("Policy decision navigation");
	const gchar *url = NULL;
	WebKitNavigationAction *a =
		webkit_navigation_policy_decision_get_navigation_action(
			WEBKIT_NAVIGATION_POLICY_DECISION(decision));

	switch (webkit_navigation_action_get_navigation_type(a)) {
	case WEBKIT_NAVIGATION_TYPE_LINK_CLICKED:
		g_debug("WEBKIT_NAVIGATION_TYPE_LINK_CLICKED");
		url = webkit_uri_request_get_uri(
			webkit_navigation_action_get_request(a));
		g_debug("url is %s ", url);
		break;
	case WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED:
		g_debug("WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED");
		break;
	case WEBKIT_NAVIGATION_TYPE_BACK_FORWARD:
		g_debug("WEBKIT_NAVIGATION_TYPE_BACK_FORWARD");
		break;
	case WEBKIT_NAVIGATION_TYPE_RELOAD:
		g_debug("WEBKIT_NAVIGATION_TYPE_RELOAD");
		break;
	case WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED:
		g_debug("WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED");
		break;
	case WEBKIT_NAVIGATION_TYPE_OTHER:
		g_debug("WEBKIT_NAVIGATION_TYPE_OTHER");
		break;
	default:
		/* Do not navigate to links with a "_blank" target (popup) */
		if (webkit_navigation_policy_decision_get_frame_name(
			    WEBKIT_NAVIGATION_POLICY_DECISION(decision))) {
			webkit_policy_decision_ignore(decision);
		} else {
			/* Filter out navigation to different domain ? */
			/* get action→urirequest, copy and load in new window+view
			 * on Ctrl+Click ? */
			webkit_policy_decision_use(decision);
		}
		break;
	}
}
void remmina_plugin_www_decide_newwin(WebKitPolicyDecision *decision, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	g_debug("Policy decision new window");

	const gchar *url = NULL;

	RemminaPluginWWWData *gpdata;
	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

	WebKitNavigationAction *a =
		webkit_navigation_policy_decision_get_navigation_action(
			WEBKIT_NAVIGATION_POLICY_DECISION(decision));

	switch (webkit_navigation_action_get_navigation_type(a)) {
	case WEBKIT_NAVIGATION_TYPE_LINK_CLICKED:
		g_debug("WEBKIT_NAVIGATION_TYPE_LINK_CLICKED");
		url = webkit_uri_request_get_uri(
			webkit_navigation_action_get_request(a));
		g_debug("Downloading url %s ", url);
		WebKitDownload *d = webkit_web_view_download_uri(gpdata->webview, url);


		break;
	case WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED:
		g_debug("WEBKIT_NAVIGATION_TYPE_FORM_SUBMITTED");
		break;
	case WEBKIT_NAVIGATION_TYPE_BACK_FORWARD:
		g_debug("WEBKIT_NAVIGATION_TYPE_BACK_FORWARD");
		break;
	case WEBKIT_NAVIGATION_TYPE_RELOAD:
		g_debug("WEBKIT_NAVIGATION_TYPE_RELOAD");
		break;
	case WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED:
		g_debug("WEBKIT_NAVIGATION_TYPE_FORM_RESUBMITTED");
		/* Filter domains here */
		/* If the value of “mouse-button” is not 0, then the navigation was triggered by a mouse event.
		 * test for link clicked but no button ? */
		url = webkit_uri_request_get_uri(
			webkit_navigation_action_get_request(a));
		g_debug("Trying to open url: %s", url);
		webkit_web_view_load_uri(gpdata->webview, url);
		break;
	case WEBKIT_NAVIGATION_TYPE_OTHER:         /* fallthrough */
		g_debug("WEBKIT_NAVIGATION_TYPE_OTHER");
		/* Filter domains here */
		/* If the value of “mouse-button” is not 0, then the navigation was triggered by a mouse event.
		 * test for link clicked but no button ? */
		url = webkit_uri_request_get_uri(
			webkit_navigation_action_get_request(a));
		g_debug("Trying to open url: %s", url);
		webkit_web_view_load_uri(gpdata->webview, url);
		break;
	default:
		break;
	}
	g_debug("WEBKIT_NAVIGATION_TYPE is %d", webkit_navigation_action_get_navigation_type(a));

	webkit_policy_decision_ignore(decision);
}
static gboolean
remmina_plugin_www_decide_resource(WebKitPolicyDecision *decision, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	g_debug("Policy decision resource");
	WebKitResponsePolicyDecision *response_decision =
		WEBKIT_RESPONSE_POLICY_DECISION(decision);
	WebKitURIResponse *response =
		webkit_response_policy_decision_get_response(response_decision);;
	const gchar *request_uri = webkit_uri_response_get_uri(response);

	WebKitURIRequest *request;
	WebKitWebResource *main_resource;
	WWWWebViewDocumentType type;
	const char *mime_type;

	RemminaPluginWWWData *gpdata;
	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

	mime_type = webkit_uri_response_get_mime_type(response);

	g_debug("The media type is %s", mime_type);

	/* If WebKit can't handle the media type, start the download
	 * process */
	if (webkit_response_policy_decision_is_mime_type_supported(response_decision))
		return FALSE;

	/* If it's not the main resource we don't need to set the document type. */
	request = webkit_response_policy_decision_get_request(response_decision);
	request_uri = webkit_uri_request_get_uri(request);
	main_resource = webkit_web_view_get_main_resource(gpdata->webview);
	if (g_strcmp0(webkit_web_resource_get_uri(main_resource), request_uri) != 0)
		return FALSE;

	type = WWW_WEB_VIEW_DOCUMENT_OTHER;
	if (!strcmp(mime_type, "text/html") || !strcmp(mime_type, "text/plain"))
		type = WWW_WEB_VIEW_DOCUMENT_HTML;
	else if (!strcmp(mime_type, "application/xhtml+xml"))
		type = WWW_WEB_VIEW_DOCUMENT_XML;
	else if (!strncmp(mime_type, "image/", 6))
		type = WWW_WEB_VIEW_DOCUMENT_IMAGE;
	else if (!strncmp(mime_type, "application/octet-stream", 6))
		type = WWW_WEB_VIEW_DOCUMENT_OCTET_STREAM;

	g_debug("Document type is %i", type);

	/* FIXME: Maybe it makes more sense to have an API to query the media
	 * type when the load of a page starts than doing this here.
	 */
	if (gpdata->document_type != type) {
		gpdata->document_type = type;

		//g_object_notify_by_pspec (G_OBJECT (webview), obj_properties[PROP_DOCUMENT_TYPE]);
	}

	webkit_policy_decision_download(decision);
	return TRUE;
}

static void remmina_www_web_view_js_finished(GObject *object, GAsyncResult *result, gpointer user_data)
{
	TRACE_CALL(__func__);

	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)user_data;
	RemminaPluginWWWData *gpdata = GET_PLUGIN_DATA(gp);
	WebKitJavascriptResult *js_result;
	GError *error = NULL;

	js_result = webkit_web_view_run_javascript_finish(WEBKIT_WEB_VIEW(object), result, &error);
	if (!js_result) {
		g_warning("Could not run JavaScript code: %s", error->message);
		g_error_free(error);
		return;
	}

#if WEBKIT_CHECK_VERSION(2, 21, 1)
	gchar *str_value;
	JSCValue *value = webkit_javascript_result_get_js_value(js_result);
	if (jsc_value_is_string(value) || jsc_value_is_boolean(value)) {
		JSCException *exception;

		str_value = jsc_value_to_string(value);
		exception = jsc_context_get_exception(jsc_value_get_context(value));
		if (exception)
			g_warning("Could not run JavaScript code: %s", jsc_exception_get_message(exception));
		else
			g_print("Script result: %s\n", str_value);
		g_free(str_value);
	} else {
		str_value = jsc_value_to_string(value);
		g_warning("Received something other than a string from JavaScript: %s", str_value);
		g_free(str_value);
	}
#endif
	if (js_result) webkit_javascript_result_unref(js_result);
	gpdata->formauthenticated = TRUE;
}

static gboolean remmina_www_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	return TRUE;
}

static gboolean remmina_plugin_www_load_failed_tls_cb(WebKitWebView *webview,
						      gchar *failing_uri, GTlsCertificate *certificate,
						      GTlsCertificateFlags errors, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	/* Avoid failing if certificate is not good. TODO: Add widgets to let the user decide */
	g_debug("Ignoring certificate and return TRUE");
	return TRUE;
}

static void remmina_plugin_www_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	RemminaPluginWWWData *gpdata;
	RemminaFile *remminafile;
	gchar *datapath;
	gchar *cache_dir;

	gpdata = g_new0(RemminaPluginWWWData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	/* RemminaPluginWWWData initialization */

	gpdata->authenticated = FALSE;
	gpdata->formauthenticated = FALSE;
	gpdata->document_type = WWW_WEB_VIEW_DOCUMENT_HTML;

	datapath = g_build_path("/",
				g_path_get_dirname(remmina_plugin_service->file_get_path(remminafile)),
				PLUGIN_NAME,
				NULL);
	cache_dir = g_build_path("/", datapath, "cache", NULL);
	g_debug("WWW data path is %s", datapath);

	if (datapath) {
		gchar *indexeddb_dir = g_build_filename(datapath, "indexeddb", NULL);
		gchar *local_storage_dir = g_build_filename(datapath, "local_storage", NULL);
		gchar *applications_dir = g_build_filename(datapath, "applications", NULL);
		gchar *websql_dir = g_build_filename(datapath, "websql", NULL);
		gpdata->data_mgr = webkit_website_data_manager_new(
			"disk-cache-directory", cache_dir,
			"indexeddb-directory", indexeddb_dir,
			"local-storage-directory", local_storage_dir,
			"offline-application-cache-directory", applications_dir,
			"websql-directory", websql_dir,
			NULL
			);
		g_free(indexeddb_dir);
		g_free(local_storage_dir);
		g_free(applications_dir);
		g_free(websql_dir);
		g_free(datapath);
	} else {
		gpdata->data_mgr = webkit_website_data_manager_new_ephemeral();
	}


	if (remmina_plugin_service->file_get_string(remminafile, "server"))
		gpdata->url = g_strdup(remmina_plugin_service->file_get_string(remminafile, "server"));
	else
		gpdata->url = "https://remmina.org";
	g_info("URL is set to %s", gpdata->url);

	gpdata->settings = webkit_settings_new();
	gpdata->context = webkit_web_context_new_with_website_data_manager(gpdata->data_mgr);

	/* enable-fullscreen, default TRUE, TODO: Try FALSE */

#ifdef DEBUG
	/* Turn on the developer extras */
	webkit_settings_set_enable_developer_extras(gpdata->settings, TRUE);
#endif

	/* user-agent. TODO: Add option. */
	if (remmina_plugin_service->file_get_string(remminafile, "user-agent")) {
		gchar *useragent = g_strdup(remmina_plugin_service->file_get_string(remminafile, "user-agent"));
		webkit_settings_set_user_agent(gpdata->settings, useragent);
		g_info("User Agent set to: %s", useragent);
		g_free(useragent);
	}
	/* enable-java */
	if (remmina_plugin_service->file_get_int(remminafile, "enable-java", FALSE)) {
		webkit_settings_set_enable_java(gpdata->settings, TRUE);
		g_info("Enable Java");
	}
	/* enable-smooth-scrolling */
	if (remmina_plugin_service->file_get_int(remminafile, "enable-smooth-scrolling", FALSE)) {
		webkit_settings_set_enable_smooth_scrolling(gpdata->settings, TRUE);
		g_info("enable-smooth-scrolling enabled");
	}
	/* enable-spatial-navigation */
	if (remmina_plugin_service->file_get_int(remminafile, "enable-spatial-navigation", FALSE)) {
		webkit_settings_set_enable_spatial_navigation(gpdata->settings, TRUE);
		g_info("enable-spatial-navigation enabled");
	}
	/* enable-webaudio */
	if (remmina_plugin_service->file_get_int(remminafile, "enable-webaudio", FALSE)) {
		webkit_settings_set_enable_webaudio(gpdata->settings, TRUE);
		g_info("enable-webaudio enabled");
	}
	/* enable-plugins */
	if (remmina_plugin_service->file_get_int(remminafile, "enable-plugins", FALSE)) {
		webkit_settings_set_enable_plugins(gpdata->settings, TRUE);
		g_info("Enable plugins");
	}
	/* enable-webgl */
	if (remmina_plugin_service->file_get_int(remminafile, "enable-webgl", FALSE)) {
		webkit_settings_set_enable_webgl(gpdata->settings, TRUE);
		g_info("enable-webgl enabled");
	}

	if (remmina_plugin_service->file_get_int(remminafile, "ignore-tls-errors", FALSE)) {
		webkit_web_context_set_tls_errors_policy(gpdata->context, WEBKIT_TLS_ERRORS_POLICY_IGNORE);
		g_info("Ignore TLS errors");
	}

	webkit_web_context_set_automation_allowed(gpdata->context, TRUE);
	webkit_settings_set_javascript_can_open_windows_automatically(gpdata->settings, TRUE);
	webkit_settings_set_allow_modal_dialogs(gpdata->settings, TRUE);

	g_signal_connect(G_OBJECT(gpdata->context), "download-started",
			 G_CALLBACK(remmina_plugin_www_download_started), gp);
}

static gboolean remmina_plugin_www_on_auth(WebKitWebView *webview, WebKitAuthenticationRequest *request, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	gchar *s_username, *s_password;
	gint ret;
	RemminaPluginWWWData *gpdata;
	gboolean save;
	gboolean disablepasswordstoring;
	RemminaFile *remminafile;


	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

	g_info("Authenticate");

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);
	ret = remmina_plugin_service->protocol_plugin_init_authuserpwd(gp, FALSE, !disablepasswordstoring);

	if (ret == GTK_RESPONSE_OK) {
		s_username = remmina_plugin_service->protocol_plugin_init_get_username(gp);
		s_password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		if (remmina_plugin_service->protocol_plugin_init_get_savepassword(gp))
			remmina_plugin_service->file_set_string(remminafile, "password", s_password);
		if (request) {
			gpdata->credentials = webkit_credential_new(
				g_strdup(s_username),
				g_strdup(s_password),
				WEBKIT_CREDENTIAL_PERSISTENCE_FOR_SESSION);
			webkit_authentication_request_authenticate(request, gpdata->credentials);
			webkit_credential_free(gpdata->credentials);
		}

		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save) {
			// The user has requested to save credentials. We put all the new credentials
			// into remminafile->settings. They will be saved later, upon connection, by
			// rcw.c

			remmina_plugin_service->file_set_string(remminafile, "username", s_username);
			remmina_plugin_service->file_set_string(remminafile, "password", s_password);
		}

		if (s_username) g_free(s_username);
		if (s_password) g_free(s_password);

		/* Free credentials */

		gpdata->authenticated = TRUE;
	} else {
		gpdata->authenticated = FALSE;
	}

	return gpdata->authenticated;
}

static void remmina_plugin_www_form_auth(WebKitWebView *webview,
					 WebKitLoadEvent load_event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	gchar *s_js;
	GString *jsstr;
	RemminaPluginWWWData *gpdata;
	RemminaFile *remminafile;
	gchar *remmina_dir;
	gchar *www_js_file = NULL;
	GError *error = NULL;

	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");
	if (gpdata && !gpdata->formauthenticated)
		gpdata->formauthenticated = FALSE;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	g_debug("load-changed emitted");


	const gchar *const *dirs = g_get_system_data_dirs();

	unsigned int i = 0;
	for (i = 0; dirs[i] != NULL; ++i) {
		remmina_dir = g_build_path("/", dirs[i], "remmina", "res", NULL);
		GDir *system_data_dir = g_dir_open(remmina_dir, 0, &error);
		// ignoring this error is ok, because the folder may not exists
		if (error) {
			g_error_free(error);
			error = NULL;
		} else {
			if (system_data_dir) {
				g_dir_close(system_data_dir);
				g_free(www_js_file);
				www_js_file = g_strdup_printf("%s/www-js.js", remmina_dir);
				if (g_file_test(www_js_file, G_FILE_TEST_EXISTS))
					break;
			}
		}
		g_free(remmina_dir);
	}

	switch (load_event) {
	case WEBKIT_LOAD_STARTED:
		break;
	case WEBKIT_LOAD_REDIRECTED:
		break;
	case WEBKIT_LOAD_COMMITTED:
		/* The load is being performed. Current URI is
		 * the final one and it won't change unless a new
		 * load is requested or a navigation within the
		 * same page is performed
		 * uri = webkit_web_view_get_uri (webview); */
		break;
	case WEBKIT_LOAD_FINISHED:
		/* Load finished, we can now set user/password
		 * in the HTML form */
		g_debug("Load finished");
		if (gpdata && gpdata->formauthenticated == TRUE)
			break;

		if (remmina_plugin_service->file_get_string(remminafile, "username") ||
		    remmina_plugin_service->file_get_string(remminafile, "password")) {
			g_debug("Authentication is enabled");
			g_file_get_contents(www_js_file, &s_js, NULL, NULL);
			jsstr = g_string_new(s_js);
			if (remmina_plugin_service->file_get_string(remminafile, "username"))
				www_utils_string_replace_all(jsstr, "USRPLACEHOLDER",
							     remmina_plugin_service->file_get_string(remminafile, "username"));
			if (remmina_plugin_service->file_get_string(remminafile, "password"))
				www_utils_string_replace_all(jsstr, "PWDPLACEHOLDER",
							     remmina_plugin_service->file_get_string(remminafile, "password"));
			s_js = g_string_free(jsstr, FALSE);

			if (!s_js || s_js[0] == '\0') {
				break;
			} else {
				g_debug("Trying to send this JavaScript: %s", s_js);
				webkit_web_view_run_javascript(
						webview,
						s_js,
						NULL,
						remmina_www_web_view_js_finished,
						gp);
				g_free(s_js);
			}
		}
		break;
	}
}

static gboolean remmina_plugin_www_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginWWWData *gpdata;
	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

	webkit_web_view_try_close(gpdata->webview);

	if (gpdata->url) g_free(gpdata->url);
	gpdata->authenticated = FALSE;
	gpdata->formauthenticated = FALSE;
	gpdata->webview = NULL;
	gpdata->data_mgr = NULL;
	gpdata->settings = NULL;
	gpdata->context = NULL;

	/* Remove instance->context from gp object data to avoid double free */
	g_object_steal_data(G_OBJECT(gp), "plugin-data");

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");
	return FALSE;
}

static gboolean remmina_plugin_www_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	RemminaPluginWWWData *gpdata;
	RemminaFile *remminafile;

	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	gpdata->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->box);

	remmina_plugin_service->protocol_plugin_register_hostkey(gp, gpdata->box);

	gpdata->webview = WEBKIT_WEB_VIEW(webkit_web_view_new_with_context(gpdata->context));
	webkit_web_view_set_settings(gpdata->webview, gpdata->settings);

	if (remmina_plugin_service->file_get_string(remminafile, "username") ||
	    remmina_plugin_service->file_get_string(remminafile, "password")) {
		g_debug("Authentication is enabled");
		remmina_plugin_www_on_auth(gpdata->webview, NULL, gp);
	}

	//"signal::load-failed-with-tls-errors", G_CALLBACK(remmina_plugin_www_load_failed_tls_cb), gp,
	g_object_connect(
		G_OBJECT(gpdata->webview),
		"signal::load-changed", G_CALLBACK(remmina_plugin_www_form_auth), gp,
		"signal::authenticate", G_CALLBACK(remmina_plugin_www_on_auth), gp,
		"signal::decide-policy", G_CALLBACK(remmina_plugin_www_decide_policy_cb), gp,
		NULL);

	gtk_widget_set_hexpand(GTK_WIDGET(gpdata->webview), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(gpdata->webview), TRUE);
	gtk_container_add(GTK_CONTAINER(gpdata->box), GTK_WIDGET(gpdata->webview));
	webkit_web_view_load_uri(gpdata->webview, gpdata->url);
#ifdef DEBUG
	if (remmina_plugin_service->file_get_int(remminafile, "enable-webinspector", FALSE)) {
		g_info("WebInspector enabled");
		WebKitWebInspector *inspector = webkit_web_view_get_inspector(WEBKIT_WEB_VIEW(gpdata->webview));
		webkit_web_inspector_attach(inspector);
		webkit_web_inspector_show(WEBKIT_WEB_INSPECTOR(inspector));
	}
#endif
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");
	gtk_widget_show_all(gpdata->box);

	return TRUE;
}

static void remmina_plugin_www_save_snapshot(GObject *object, GAsyncResult *result, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	WebKitWebView *webview = WEBKIT_WEB_VIEW(object);

	RemminaFile *remminafile;

	GError *err = NULL;
	cairo_surface_t *surface;
	//unsigned char* buffer;
	int width;
	int height;
	GdkPixbuf *screenshot;
	GString *pngstr;
	gchar *pngname;
	//cairo_forma_t* cairo_format;
	GDateTime *date = g_date_time_new_now_utc();

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	surface = webkit_web_view_get_snapshot_finish(WEBKIT_WEB_VIEW(webview), result, &err);
	if (err)
		g_warning("An error happened generating the snapshot: %s\n", err->message);
	//buffer = cairo_image_surface_get_data (surface);
	width = cairo_image_surface_get_width(surface);
	height = cairo_image_surface_get_height(surface);
	//cairo_format = cairo_image_surface_get_format (surface);

	screenshot = gdk_pixbuf_get_from_surface(surface, 0, 0, width, height);
	if (screenshot == NULL)
		g_debug("WWW: gdk_pixbuf_get_from_surface failed");

	// Transfer the PixBuf in the main clipboard selection
	gchar *value = remmina_plugin_service->pref_get_value("deny_screenshot_clipboard");
	if (value && value == FALSE) {
		GtkClipboard *c = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
		gtk_clipboard_set_image(c, screenshot);
	}

	pngstr = g_string_new(g_strdup_printf("%s/%s.png",
					      remmina_plugin_service->pref_get_value("screenshot_path"),
					      remmina_plugin_service->pref_get_value("screenshot_name")));
	www_utils_string_replace_all(pngstr, "%p",
				     remmina_plugin_service->file_get_string(remminafile, "name"));
	www_utils_string_replace_all(pngstr, "%h", "URL");
	www_utils_string_replace_all(pngstr, "%Y",
				     g_strdup_printf("%d", g_date_time_get_year(date)));
	www_utils_string_replace_all(pngstr, "%m", g_strdup_printf("%d",
								   g_date_time_get_month(date)));
	www_utils_string_replace_all(pngstr, "%d",
				     g_strdup_printf("%d", g_date_time_get_day_of_month(date)));
	www_utils_string_replace_all(pngstr, "%H",
				     g_strdup_printf("%d", g_date_time_get_hour(date)));
	www_utils_string_replace_all(pngstr, "%M",
				     g_strdup_printf("%d", g_date_time_get_minute(date)));
	www_utils_string_replace_all(pngstr, "%S",
				     g_strdup_printf("%f", g_date_time_get_seconds(date)));
	g_date_time_unref(date);
	pngname = g_string_free(pngstr, FALSE);
	g_debug("Saving screenshot as %s", pngname);

	cairo_surface_write_to_png(surface, pngname);
	if (g_file_test(pngname, G_FILE_TEST_EXISTS))
		www_utils_send_notification("www-plugin-screenshot-is-ready-id", _("Screenshot taken"), pngname);

	cairo_surface_destroy(surface);
}
static gboolean remmina_plugin_www_get_snapshot(RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd)
{
	TRACE_CALL(__func__);
	RemminaPluginWWWData *gpdata;
	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

	webkit_web_view_get_snapshot(gpdata->webview,
				     WEBKIT_SNAPSHOT_REGION_FULL_DOCUMENT,
				     WEBKIT_SNAPSHOT_OPTIONS_NONE,
				     NULL,
				     (GAsyncReadyCallback)remmina_plugin_www_save_snapshot,
				     gp);
	return FALSE;
}

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_www_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "server",   N_("URL"),	FALSE, NULL, N_("http://address or https://address") },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "username", N_("Username"),   FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "password", N_("Password"),   FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,	      NULL,		FALSE, NULL, NULL }
};

/* Array of RemminaProtocolSetting for advanced settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_plugin_www_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,  "user-agent",		    N_("User agent"),		       FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-java",		    N_("Turn on Java support"),	       TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-smooth-scrolling",   N_("Turn on smooth scrolling"),     TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-spatial-navigation", N_("Turn on Spatial Navigation"),   TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-plugins",	    N_("Turn on plugin support"),  TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-webgl",		    N_("Turn on WebGL support"),    TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-webaudio",	    N_("Turn on HTML5 audio support"), TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "ignore-tls-errors",	    N_("Ignore TLS errors"),	       TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disablepasswordstoring",    N_("No password storage"),    TRUE,  NULL, NULL },
#ifdef DEBUG
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-webinspector",	    N_("Turn on Web Inspector"),	       TRUE,  NULL, NULL },
#endif
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,   NULL,			    NULL,			       FALSE, NULL, NULL }
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_www_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,           // Type
	PLUGIN_NAME,                            // Name
	PLUGIN_DESCRIPTION,                     // Description
	GETTEXT_PACKAGE,                        // Translation domain
	PLUGIN_VERSION,                         // Version number
	PLUGIN_APPICON,                         // Icon for normal connection
	NULL,                                   // Icon for SSH connection
	remmina_plugin_www_basic_settings,      // Array for basic settings
	remmina_plugin_www_advanced_settings,   // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_NONE,      // SSH settings type
	remmina_www_features,                   // Array for available features
	remmina_plugin_www_init,                // Plugin initialization
	remmina_plugin_www_open_connection,     // Plugin open connection
	remmina_plugin_www_close_connection,    // Plugin close connection
	remmina_www_query_feature,              // Query for available features
	NULL,                                   // Call a feature
	NULL,                                   // Send a keystroke
	remmina_plugin_www_get_snapshot         // Capture screenshot
};

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService *service)
{
	TRACE_CALL(__func__);
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");


	if (!service->register_plugin((RemminaPlugin *)&remmina_plugin))
		return FALSE;
	return TRUE;
}
