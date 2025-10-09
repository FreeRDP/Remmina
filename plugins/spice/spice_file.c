/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2017-2023 Antenore Gatta, Giovanni Panozzo
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

#include "remmina/plugin.h"
#include "spice_plugin.h"
#include "spice_file.h"


gboolean remmina_spice_file_import_test(RemminaFilePlugin *plugin, const gchar *from_file)
{
	TRACE_CALL(__func__);
	gchar *ext;

	ext = strrchr(from_file, '.');

	if (!ext)
		return FALSE;

	ext++;

	if (g_strcmp0(ext, "vv") == 0)
		return TRUE;

	return FALSE;
}


static void remmina_spice_file_import_field(RemminaFile *remminafile, const gchar *key, const gchar *value)
{
	TRACE_CALL(__func__);
    int viewmode = 0;

	if (g_strcmp0(key, "fullscreen") == 0) {
        viewmode = atoi(value);
        if (viewmode == 1){
            viewmode = 2;
        }
        else{
            viewmode = 1;
        }
		remmina_plugin_service->file_set_int(remminafile, "viewmode", viewmode);
	} else if (g_strcmp0(key, "username") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "username", value);
	} else if (g_strcmp0(key, "title") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "name", value);
	} else if (g_strcmp0(key, "proxy") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "proxy", value);
    }
}

static RemminaFile *remmina_spice_file_import_channel(GIOChannel *channel)
{
	TRACE_CALL(__func__);
	gchar *p;
	const gchar *enc;
	gchar *line = NULL;
	GError *error = NULL;
	gsize bytes_read = 0;
	RemminaFile *remminafile;
	guchar magic[2] = { 0 };

	if (g_io_channel_set_encoding(channel, NULL, &error) != G_IO_STATUS_NORMAL) {
		g_print("g_io_channel_set_encoding: %s\n", error->message);
		return NULL;
	}

	/* Try to detect the UTF-16 encoding */
	if (g_io_channel_read_chars(channel, (gchar *)magic, 2, &bytes_read, &error) != G_IO_STATUS_NORMAL) {
		g_print("g_io_channel_read_chars: %s\n", error->message);
		return NULL;
	}

	if (magic[0] == 0xFF && magic[1] == 0xFE) {
		enc = "UTF-16LE";
	} else if (magic[0] == 0xFE && magic[1] == 0xFF) {
		enc = "UTF-16BE";
	} else {
		enc = "UTF-8";
		if (g_io_channel_seek_position(channel, 0, G_SEEK_SET, &error) != G_IO_STATUS_NORMAL) {
			g_print("g_io_channel_seek: failed\n");
			return NULL;
		}
	}

	if (g_io_channel_set_encoding(channel, enc, &error) != G_IO_STATUS_NORMAL) {
		g_print("g_io_channel_set_encoding: %s\n", error->message);
		return NULL;
	}

	remminafile = remmina_plugin_service->file_new();


    //host and port are stored seperately in vv file, so save each as we get to it to append 
    //together in the remmina server format
    const char *host = NULL, *port = NULL;
    char *final = NULL;

	while (g_io_channel_read_line(channel, &line, NULL, &bytes_read, &error) == G_IO_STATUS_NORMAL) {
		if (line == NULL)
			break;

		line[bytes_read] = '\0';
		p = strchr(line, '=');

		if (p) {
			*p++ = '\0';
            remmina_spice_file_import_field(remminafile, line, p);
            if (g_strcmp0(line, "host") == 0) {
                host = p;
            }
            if (g_strcmp0(line, "port") == 0) {
                port = p;
            }
		}

		g_free(line);
	}
    if (host != NULL && port != NULL){
        final = g_alloca(strlen(host)+strlen(port)+1);
        strcat(final, host);
        strcat(final + strlen(host), port);
        remmina_plugin_service->file_set_string(remminafile, "server", final);
        g_free(final);
    }
    else if (host != NULL) {
        remmina_plugin_service->file_set_string(remminafile, "server", host);
    }

    if (remmina_plugin_service->file_get_string(remminafile, "name") == NULL){
        remmina_plugin_service->file_set_string(remminafile, "name",
						remmina_plugin_service->file_get_string(remminafile, "server"));
    }
	
	remmina_plugin_service->file_set_string(remminafile, "protocol", "SPICE");

	return remminafile;
}




RemminaFile *remmina_spice_file_import(RemminaFilePlugin *plugin,const gchar *from_file)
{
	TRACE_CALL(__func__);
	GIOChannel *channel;
	GError *error = NULL;
	RemminaFile *remminafile;

	channel = g_io_channel_new_file(from_file, "r", &error);

	if (channel == NULL) {
		g_print("Failed to import %s: %s\n", from_file, error->message);
		return NULL;
	}

	remminafile = remmina_spice_file_import_channel(channel);
	g_io_channel_shutdown(channel, TRUE, &error);

	return remminafile;
}



gboolean remmina_spice_file_export_channel(RemminaFile *remminafile, FILE *fp)
{
	TRACE_CALL(__func__);
	const gchar *cs;
    gchar *csm;
    const gchar *port;
    const gchar *host;
    const gchar *unix_string = "unix://";
	int w;

	fprintf(fp, "[virt-viewer]\r\n");
    fprintf(fp, "type=spice\r\n");
    csm = g_strdup(remmina_plugin_service->file_get_string(remminafile, "server"));
    if (csm){
        if(strncmp(csm, unix_string, strlen(unix_string)) == 0){
            fprintf(fp, "host=%s\r\n", csm);
        }
        else {
            host = strtok(csm, ":");
            port = strtok(NULL, ":");
            if (port){
                fprintf(fp, "host=%s\r\n", host);
                fprintf(fp, "port=%s\r\n", port);
            }
            else{
                fprintf(fp, "host=%s\r\n", host);
            }
        }
        g_free(csm);
    }

    w = remmina_plugin_service->file_get_int(remminafile, "viewmode", 0);
    if (w >= 2){
         fprintf(fp, "fullscreen=%d\r\n", 1);
    }
    else{
        fprintf(fp, "fullscreen=%d\r\n", 0);
    }
    cs = remmina_plugin_service->file_get_string(remminafile, "username");
    if (cs) {
        fprintf(fp, "username=%s\r\n", cs);
    }
    cs = remmina_plugin_service->file_get_string(remminafile, "name");
    if (cs) {
        fprintf(fp, "title=%s\r\n", cs);
    }
    cs = remmina_plugin_service->file_get_string(remminafile, "proxy");
    if (cs) {
        fprintf(fp, "proxy=%s\r\n", cs);
    }
    

	return TRUE;
}


gboolean remmina_spice_file_export_test(RemminaFilePlugin *plugin, RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	if (g_strcmp0(remmina_plugin_service->file_get_string(remminafile, "protocol"), "SPICE") == 0)
		return TRUE;

	return FALSE;
}



gboolean remmina_spice_file_export(RemminaFilePlugin *plugin, RemminaFile *remminafile, const gchar *to_file)
{
	TRACE_CALL(__func__);
	FILE *fp;
	gboolean ret;

	fp = g_fopen(to_file, "w+");

	if (fp == NULL) {
		g_print("Failed to export %s\n", to_file);
		return FALSE;
	}

	ret = remmina_spice_file_export_channel(remminafile, fp);
	fclose(fp);

	return ret;
}