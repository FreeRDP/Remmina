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

#define GET_PLUGIN_DATA(gp) (RemminaPluginExecData *)g_object_get_data(G_OBJECT(gp), "plugin-data")

typedef struct _RemminaPluginWWWData {
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
} RemminaPluginWWWData;

static RemminaPluginService *remmina_plugin_service = NULL;

static void remmina_www_web_view_js_finished (GObject *object, GAsyncResult *result, gpointer user_data)
{
    WebKitJavascriptResult *js_result;
    JSCValue               *value;
    GError                 *error = NULL;
    gchar        *str_value;

    js_result = webkit_web_view_run_javascript_finish (WEBKIT_WEB_VIEW (object), result, &error);
    if (!js_result) {
        g_warning ("Error running javascript: %s", error->message);
        g_error_free (error);
        return;
    }

    value = webkit_javascript_result_get_js_value (js_result);
    if (jsc_value_is_string (value) || jsc_value_is_boolean (value)) {
        JSCException *exception;

        str_value = jsc_value_to_string (value);
        exception = jsc_context_get_exception (jsc_value_get_context (value));
        if (exception)
            g_warning ("Error running javascript: %s", jsc_exception_get_message (exception));
        else
            g_print ("Script result: %s\n", str_value);
        g_free (str_value);
    } else {
        str_value = jsc_value_to_string (value);
        g_warning ("javascript: unexpected return value, is not a string: %s", str_value);
        g_free (str_value);
    }
    webkit_javascript_result_unref (js_result);
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
	/* Avoid to fail if certificate is not good. TODO: Add widgets to let the user decide */
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

	gpdata->authenticated = FALSE;

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

	/* user-agent. TODO: Add option. */
	if (remmina_plugin_service->file_get_string(remminafile, "user-agent")){
		gchar* useragent = g_strdup(remmina_plugin_service->file_get_string(remminafile, "user-agent"));
		webkit_settings_set_user_agent(gpdata->settings, useragent);
		g_info("User Agent set to: %s", useragent);
		g_free(useragent);
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
	/* enable-webgl */
	if (remmina_plugin_service->file_get_int(remminafile, "enable-webgl", FALSE)) {
		webkit_settings_set_enable_webgl(gpdata->settings, TRUE);
		g_info("enable-webgl enabled");
	}

	if (remmina_plugin_service->file_get_int(remminafile, "ignore-tls-errors", FALSE)) {
		webkit_web_context_set_tls_errors_policy(gpdata->context, WEBKIT_TLS_ERRORS_POLICY_IGNORE);
		g_info("Ignore TLS errors");
	}
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

	g_info("Authenticate");

	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

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
			// User has requested to save credentials. We put all the new credentials
			// into remminafile->settings. They will be saved later, on successful connection, by
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
	gchar *s_username, *s_password, *s_js;
	RemminaPluginWWWData *gpdata;
	RemminaFile *remminafile;

	gpdata = (RemminaPluginWWWData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

	gpdata->load_event = load_event;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	g_debug("load-changed emitted");

	switch (gpdata->load_event) {
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
		 * in the html form */
		g_debug("Load finished");
		if (remmina_plugin_service->file_get_string(remminafile, "password-id")) {
			s_username = g_strdup(remmina_plugin_service->file_get_string(remminafile, "username"));
			s_password = g_strdup(remmina_plugin_service->file_get_string(remminafile, "password"));
			gchar *s_uid = g_strdup(remmina_plugin_service->file_get_string(remminafile, "username-id"));
			gchar *s_pwdid = g_strdup(remmina_plugin_service->file_get_string(remminafile, "password-id"));

			if (remmina_plugin_service->file_get_string(remminafile, "iframe-id")) {
				gchar* iframe = g_strdup(remmina_plugin_service->file_get_string(remminafile, "iframe-id"));
				/* document.getElementById('websso').contentDocument.getElementById('%s') */
				s_js = g_strdup_printf (
						"let evt = new Event('change');"
						"document.getElementById('%s').contentDocument.getElementById('%s').value = '%s';"
						"document.getElementById('%s').contentDocument.getElementById('%s').value = '%s';"
						"document.getElementById('%s').contentDocument.getElementById('%s').dispatchEvent(evt);"
						"document.getElementById('%s').contentDocument.getElementById('%s').dispatchEvent(evt);",
						iframe, s_uid, s_username, iframe, s_pwdid, s_password,
						iframe, s_uid, iframe, s_pwdid);
				g_free(iframe);
			} else {
				s_js = g_strdup_printf (
						"let evt = new Event('change');"
						"document.getElementById('%s').value = '%s';"
						"document.getElementById('%s').value = '%s';"
						"document.getElementById('%s').dispatchEvent(evt);"
						"document.getElementById('%s').dispatchEvent(evt);",
						s_uid, s_username,
						s_pwdid, s_password,
						s_uid,
						s_pwdid);
			}
			g_debug("We are trying to send this JS: %s", s_js);
			webkit_web_view_run_javascript(webview, s_js, NULL, remmina_www_web_view_js_finished, NULL);

			if(s_username) g_free(s_username);
			if(s_password) g_free(s_password);
			if(s_uid) g_free(s_uid);
			if(s_pwdid) g_free(s_pwdid);
			if(s_js) g_free(s_js);
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

	if(gpdata->url) g_free(gpdata->url);
	gpdata->authenticated = FALSE;
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
	if (!remmina_plugin_service->file_get_int(remminafile, "no-authentication", FALSE)) {
		g_debug ("Authentication is enabled");
		remmina_plugin_www_on_auth(gpdata->webview, NULL, gp);
	}

	//"signal::load-failed-with-tls-errors", G_CALLBACK(remmina_plugin_www_load_failed_tls_cb), gp,
	g_object_connect(
		G_OBJECT(gpdata->webview),
		"signal::load-changed", G_CALLBACK(remmina_plugin_www_form_auth), gp,
		"signal::authenticate", G_CALLBACK(remmina_plugin_www_on_auth), gp,
		NULL);

	gtk_widget_set_hexpand(GTK_WIDGET(gpdata->webview), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(gpdata->webview), TRUE);
	gtk_container_add(GTK_CONTAINER(gpdata->box), GTK_WIDGET(gpdata->webview));
	webkit_web_view_load_uri(gpdata->webview, gpdata->url);
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");
	gtk_widget_show_all(gpdata->box);

	return TRUE;
}

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Unused pointer
 */
static const RemminaProtocolSetting remmina_plugin_www_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "server",	       N_("Address"),		       FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "no-authentication", N_("No authentication"),	       FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "username",	       N_("Username"),		       FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "password",	       N_("Password"),		       FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "username-id",       N_("Username HTML element ID"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "password-id",       N_("Password HTML element ID"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "iframe-id",         N_("iFrame HTML element ID"),   FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,		       NULL,			       FALSE, NULL, NULL }
};

/* Array of RemminaProtocolSetting for advanced settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Unused pointer
 */
static const RemminaProtocolSetting remmina_plugin_www_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,  "user-agent",		    N_("User Agent"),				FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-smooth-scrolling",   N_("Enable smooth scrolling"),		TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-spatial-navigation", N_("Enable Spatial Navigation"),		TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-webgl",		    N_("Enable support for WebGL on pages"),	TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "enable-webaudio",	    N_("Enable support for WebAudio on pages"), TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "ignore-tls-errors",	    N_("Ignore TLS errors"),			TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disablepasswordstoring",    N_("Disable password storing"),		TRUE,  NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,   NULL,			    NULL,					FALSE, NULL, NULL }
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
	NULL                                    // Capture screenshot
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
