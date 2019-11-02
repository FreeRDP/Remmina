/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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
#include "remmina/remmina_trace_calls.h"

#ifdef HAVE_LIBSSH

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remmina_public.h"
#include "remmina_sftp_client.h"
#include "remmina_plugin_manager.h"
#include "remmina_ssh.h"
#include "remmina_protocol_widget.h"
#include "remmina_sftp_plugin.h"

#define REMMINA_PLUGIN_SFTP_FEATURE_PREF_SHOW_HIDDEN   1
#define REMMINA_PLUGIN_SFTP_FEATURE_PREF_OVERWRITE_ALL 2

#define REMMINA_PLUGIN_SFTP_FEATURE_PREF_OVERWRITE_ALL_KEY "overwrite_all"

#define GET_PLUGIN_DATA(gp) (RemminaPluginSftpData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

typedef struct _RemminaPluginSftpData {
	GtkWidget *	client;
	pthread_t	thread;
	RemminaSFTP *	sftp;
} RemminaPluginSftpData;

static RemminaPluginService *remmina_plugin_service = NULL;

static gpointer
remmina_plugin_sftp_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;
	RemminaPluginSftpData *gpdata;
	RemminaFile *remminafile;
	RemminaSSH *ssh;
	RemminaSFTP *sftp = NULL;
	gboolean cont = FALSE;
	gint ret;
	const gchar *cs;
	const gchar *saveserver;
	const gchar *saveusername;
	gchar *hostport;
	gchar tunneluser[33];           /* On linux a username can have a 32 char lenght */
	gchar tunnelserver[256];        /* On linux a servername can have a 255 char lenght */
	gchar tunnelport[6];            /* A TCP port can have a maximum value of 65535 */
	gchar *host;
	gint port;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	CANCEL_ASYNC

		gpdata = GET_PLUGIN_DATA(gp);

	/* remmina_plugin_service->protocol_plugin_start_direct_tunnel start the
	 * SSH Tunnel and return the server + port string
	 * Therefore we set the SSH Tunnel username here, before the tunnel
	 * is established and than set it back to the destination SSH user.
	 *
	 * */
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	/* We save the SSH server name, so that we can restore it at the end of the connection */
	saveserver = remmina_plugin_service->file_get_string(remminafile, "ssh_server");
	remmina_plugin_service->file_set_string(remminafile, "save_ssh_server", g_strdup(saveserver));
	/* We save the SSH username, so that we can restore it at the end of the connection */
	saveusername = remmina_plugin_service->file_get_string(remminafile, "ssh_username");
	remmina_plugin_service->file_set_string(remminafile, "save_ssh_username", g_strdup(saveusername));

	if (saveserver) {
		/* if the server string contains the character @ we extract the user
		 * and the server string (host:port)
		 * */
		if (strchr(saveserver, '@')) {
			sscanf(saveserver, "%[_a-zA-Z0-9.]@%[_a-zA-Z0-9.]:%[0-9]",
			       tunneluser, tunnelserver, tunnelport);
			g_print("Username: %s, tunneluser: %s\n",
				remmina_plugin_service->file_get_string(remminafile, "ssh_username"), tunneluser);
			if (saveusername != NULL && tunneluser[0] != '\0')
				remmina_plugin_service->file_set_string(remminafile, "ssh_username", NULL);
			remmina_plugin_service->file_set_string(remminafile, "ssh_username",
								g_strdup(tunneluser));
			remmina_plugin_service->file_set_string(remminafile, "ssh_server",
								g_strconcat(tunnelserver, ":", tunnelport, NULL));
		}
	}

	hostport = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, 22, FALSE);
	/* We restore the SSH username as the tunnel is set */
	remmina_plugin_service->file_set_string(remminafile, "ssh_username", g_strdup(saveusername));
	if (hostport == NULL)
		return FALSE;
	remmina_plugin_service->get_server_port(hostport, 22, &host, &port);

	ssh = g_object_get_data(G_OBJECT(gp), "user-data");
	if (ssh) {
		/* Create SFTP connection based on existing SSH session */
		sftp = remmina_sftp_new_from_ssh(ssh);
		if (remmina_ssh_init_session(REMMINA_SSH(sftp)) &&
		    remmina_ssh_auth(REMMINA_SSH(sftp), NULL, gp, remminafile) > 0 &&
		    remmina_sftp_open(sftp))
			cont = TRUE;
	} else {
		/* New SFTP connection */
		if (remmina_plugin_service->file_get_int(remminafile, "ssh_enabled", FALSE))
			remmina_plugin_service->file_set_string(remminafile, "ssh_server", g_strdup(hostport));
		else
			remmina_plugin_service->file_set_string(remminafile, "ssh_server",
								remmina_plugin_service->file_get_string(remminafile, "server"));
		g_free(hostport);
		g_free(host);

		sftp = remmina_sftp_new_from_file(remminafile);
		while (1) {
			if (!remmina_ssh_init_session(REMMINA_SSH(sftp))) {
				remmina_plugin_service->protocol_plugin_set_error(gp, "%s", REMMINA_SSH(sftp)->error);
				break;
			}

			ret = remmina_ssh_auth_gui(REMMINA_SSH(sftp), gp, remminafile);
			if (ret == 0)
				remmina_plugin_service->protocol_plugin_set_error(gp, "%s", REMMINA_SSH(sftp)->error);
			if (ret <= 0) break;

			if (!remmina_sftp_open(sftp)) {
				remmina_plugin_service->protocol_plugin_set_error(gp, "%s", REMMINA_SSH(sftp)->error);
				break;
			}

			cs = remmina_plugin_service->file_get_string(remminafile, "execpath");
			if (cs && cs[0])
				remmina_ftp_client_set_dir(REMMINA_FTP_CLIENT(gpdata->client), cs);

			cont = TRUE;
			break;
		}

		/* We restore the ssh_server name */
		remmina_plugin_service->file_set_string(remminafile, "ssh_server",
							remmina_plugin_service->file_get_string(remminafile, "save_ssh_server"));
	}
	if (!cont) {
		if (sftp) remmina_sftp_free(sftp);
		remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
		return NULL;
	}

	remmina_sftp_client_open(REMMINA_SFTP_CLIENT(gpdata->client), sftp);
	/* RemminaSFTPClient owns the object, we just take the reference */
	gpdata->sftp = sftp;

	remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);

	gpdata->thread = 0;
	return NULL;
}

static void
remmina_plugin_sftp_client_on_realize(GtkWidget *widget, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	remmina_ftp_client_load_state(REMMINA_FTP_CLIENT(widget), remminafile);
}

static void
remmina_plugin_sftp_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSftpData *gpdata;
	RemminaFile *remminafile;

	gpdata = g_new0(RemminaPluginSftpData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	gpdata->client = remmina_sftp_client_new();
	gtk_widget_show(gpdata->client);
	gtk_container_add(GTK_CONTAINER(gp), gpdata->client);

	remmina_ftp_client_set_show_hidden(REMMINA_FTP_CLIENT(gpdata->client),
					   remmina_plugin_service->file_get_int(remminafile, "showhidden", FALSE));

	remmina_ftp_client_set_overwrite_status(REMMINA_FTP_CLIENT(gpdata->client),
						remmina_plugin_service->file_get_int(remminafile,
										     REMMINA_PLUGIN_SFTP_FEATURE_PREF_OVERWRITE_ALL_KEY, FALSE));

	remmina_plugin_service->protocol_plugin_register_hostkey(gp, gpdata->client);

	g_signal_connect(G_OBJECT(gpdata->client),
			 "realize", G_CALLBACK(remmina_plugin_sftp_client_on_realize), gp);
}

static gboolean
remmina_plugin_sftp_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSftpData *gpdata = GET_PLUGIN_DATA(gp);

	remmina_plugin_service->protocol_plugin_set_expand(gp, TRUE);
	remmina_plugin_service->protocol_plugin_set_width(gp, 640);
	remmina_plugin_service->protocol_plugin_set_height(gp, 480);

	if (pthread_create(&gpdata->thread, NULL, remmina_plugin_sftp_main_thread, gp)) {
		remmina_plugin_service->protocol_plugin_set_error(gp,
								  "Failed to initialize pthread. Falling back to non-thread mode…");
		gpdata->thread = 0;
		return FALSE;
	} else {
		return TRUE;
	}
	return TRUE;
}

static gboolean
remmina_plugin_sftp_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSftpData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	if (gpdata->thread) {
		pthread_cancel(gpdata->thread);
		if (gpdata->thread) pthread_join(gpdata->thread, NULL);
	}

	remmina_ftp_client_save_state(REMMINA_FTP_CLIENT(gpdata->client), remminafile);
	remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
	/* The session preference overwrite_all is always saved to FALSE in order
	 * to avoid unwanted overwriting.
	 * If we'd change idea just remove the next line to save the preference. */
	remmina_file_set_int(remminafile,
			     REMMINA_PLUGIN_SFTP_FEATURE_PREF_OVERWRITE_ALL_KEY, FALSE);
	return FALSE;
}

static gboolean
remmina_plugin_sftp_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	return TRUE;
}

static void
remmina_plugin_sftp_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	RemminaPluginSftpData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	switch (feature->id) {
	case REMMINA_PROTOCOL_FEATURE_TOOL_SSH:
		remmina_plugin_service->open_connection(
			remmina_file_dup_temp_protocol(remmina_plugin_service->protocol_plugin_get_file(gp), "SSH"),
			NULL, gpdata->sftp, NULL);
		return;
	case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
		remmina_plugin_service->open_connection(
			remmina_file_dup_temp_protocol(remmina_plugin_service->protocol_plugin_get_file(gp), "SFTP"),
			NULL, gpdata->sftp, NULL);
		return;
	case REMMINA_PLUGIN_SFTP_FEATURE_PREF_SHOW_HIDDEN:
		remmina_ftp_client_set_show_hidden(REMMINA_FTP_CLIENT(gpdata->client),
						   remmina_plugin_service->file_get_int(remminafile, "showhidden", FALSE));
		return;
	case REMMINA_PLUGIN_SFTP_FEATURE_PREF_OVERWRITE_ALL:
		remmina_ftp_client_set_overwrite_status(REMMINA_FTP_CLIENT(gpdata->client),
							remmina_plugin_service->file_get_int(remminafile,
											     REMMINA_PLUGIN_SFTP_FEATURE_PREF_OVERWRITE_ALL_KEY, FALSE));
		return;
	}
}

static gpointer ssh_auth[] =
{
	"0", N_("Password"),
	"1", N_("SSH identity file"),
	"2", N_("SSH agent"),
	"3", N_("Public key (automatic)"),
	"4", N_("Kerberos (GSSAPI)"),
	NULL
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_plugin_sftp_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF, REMMINA_PLUGIN_SFTP_FEATURE_PREF_SHOW_HIDDEN,   GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "showhidden",
	  N_("Show Hidden Files") },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF, REMMINA_PLUGIN_SFTP_FEATURE_PREF_OVERWRITE_ALL, GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK),
	  REMMINA_PLUGIN_SFTP_FEATURE_PREF_OVERWRITE_ALL_KEY, N_("Overwrite all") },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END,  0,					      NULL,						    NULL,
	  NULL }
};

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_sftp_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER,	  "ssh_server",	    NULL,			  FALSE, "_sftp-ssh._tcp", NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "ssh_username",   N_("Username"),		  FALSE, NULL,		   NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "ssh_password",   N_("User password"),	  FALSE, NULL,		   NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "ssh_auth",	    N_("Authentication type"),	  FALSE, ssh_auth,	   NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_FILE,	  "ssh_privatekey", N_("Identity file"),	  FALSE, NULL,		   NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "ssh_passphrase", N_("Private key passphrase"), FALSE, NULL,		   NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,		    NULL,			  FALSE, NULL,		   NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin_sftp =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   // Type
	"SFTP",                                         // Name
	N_("SFTP - Secure File Transfer"),              // Description
	GETTEXT_PACKAGE,                                // Translation domain
	VERSION,                                        // Version number
	"remmina-sftp-symbolic",                        // Icon for normal connection
	"remmina-sftp-symbolic",                        // Icon for SSH connection
	remmina_sftp_basic_settings,                    // Array for basic settings
	NULL,                                           // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,            // SSH settings type
	remmina_plugin_sftp_features,                   // Array for available features
	remmina_plugin_sftp_init,                       // Plugin initialization
	remmina_plugin_sftp_open_connection,            // Plugin open connection
	remmina_plugin_sftp_close_connection,           // Plugin close connection
	remmina_plugin_sftp_query_feature,              // Query for available features
	remmina_plugin_sftp_call_feature,               // Call a feature
	NULL,                                           // Send a keystroke
	NULL                                            // No screenshot support available
};

void
remmina_sftp_plugin_register(void)
{
	TRACE_CALL(__func__);
	remmina_plugin_service = &remmina_plugin_manager_service;
	remmina_plugin_service->register_plugin((RemminaPlugin *)&remmina_plugin_sftp);
}

#else

void remmina_sftp_plugin_register(void)
{
	TRACE_CALL(__func__);
}

#endif
