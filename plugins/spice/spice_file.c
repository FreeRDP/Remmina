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
	if (g_strcmp0(key, "desktopwidth") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "resolution_width", value);
	} else if (g_strcmp0(key, "desktopheight") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "resolution_height", value);
	} else if (g_strcmp0(key, "session bpp") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "colordepth", value);
	} else if (g_strcmp0(key, "keyboardhook") == 0) {
		remmina_plugin_service->file_set_int(remminafile, "keyboard_grab", (atoi(value) == 1));
	} else if (g_strcmp0(key, "full address") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "server", value);
	} else if (g_strcmp0(key, "audiomode") == 0) {
		switch (atoi(value)) {
		case 0:
			remmina_plugin_service->file_set_string(remminafile, "sound", "local");
			break;
		case 1:
			remmina_plugin_service->file_set_string(remminafile, "sound", "remote");
			break;
		}
	} else if (g_strcmp0(key, "redirectprinters") == 0) {
		remmina_plugin_service->file_set_int(remminafile, "shareprinter", (atoi(value) == 1));
	} else if (g_strcmp0(key, "redirectsmartcard") == 0) {
		remmina_plugin_service->file_set_int(remminafile, "sharesmartcard", (atoi(value) == 1));
	} else if (g_strcmp0(key, "redirectclipboard") == 0) {
		remmina_plugin_service->file_set_int(remminafile, "disableclipboard", (atoi(value) != 1));
	} else if (g_strcmp0(key, "alternate shell") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "exec", value);
	} else if (g_strcmp0(key, "shell working directory") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "execpath", value);
	} else if (g_strcmp0(key, "loadbalanceinfo") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "loadbalanceinfo", value);
	} else if (g_strcmp0(key, "gatewayhostname") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "gateway_server", value);
	} else if (g_strcmp0(key, "gatewayaccesstoken") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "gatewayaccesstoken", value);
	} else if (g_strcmp0(key, "authentication level") == 0) {
		remmina_plugin_service->file_set_int(remminafile, "authentication level", atoi(value));
	}
	/* tsclient fields, import only */
	else if (g_strcmp0(key, "client hostname") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "clientname", value);
	} else if (g_strcmp0(key, "domain") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "domain", value);
	} else if (g_strcmp0(key, "username") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "username", value);
	} else if (g_strcmp0(key, "password") == 0) {
		remmina_plugin_service->file_set_string(remminafile, "password", value);
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

	while (g_io_channel_read_line(channel, &line, NULL, &bytes_read, &error) == G_IO_STATUS_NORMAL) {
		if (line == NULL)
			break;

		line[bytes_read] = '\0';
		p = strchr(line, ':');

		if (p) {
			*p++ = '\0';
			p = strchr(p, ':');

			if (p) {
				p++;
				remmina_spice_file_import_field(remminafile, line, p);
			}
		}

		g_free(line);
	}

	remmina_plugin_service->file_set_string(remminafile, "name",
						remmina_plugin_service->file_get_string(remminafile, "server"));
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
    const gchar *port;
    const gchar *host;
	int w, h;

	fprintf(fp, "[virt-viewer]\r\n");
    fprintf(fp, "type=spice\r\n");
    cs = remmina_plugin_service->file_get_string(remminafile, "server");
    if (cs){
        host = strtok(cs, ":");
        port = strtok(NULL, ":");
        if (port){
            fprintf(fp, "host=%s\r\n", host);
            fprintf(fp, "port=%s\r\n", port);
        }
        else{
            fprintf(fp, "host=%s\r\n", host);
        }
    }

    w = remmina_plugin_service->file_get_int(remminafile, "viewmode", 0);
    if (w >= 2){
         fprintf(fp, "fullscreen=%d\r\n", 1);
    }
    else{
        fprintf(fp, "fullscreen=%d\r\n", 0);
    }
   
	
	// w = remmina_plugin_service->file_get_int(remminafile, "resolution_width", -1);
	// h = remmina_plugin_service->file_get_int(remminafile, "resolution_height", -1);
	// if (w > 0 && h > 0) {
	// 	fprintf(fp, "desktopwidth:i:%d\r\n", w);
	// 	fprintf(fp, "desktopheight:i:%d\r\n", h);
	// }

	// fprintf(fp, "session bpp:i:%i\r\n", remmina_plugin_service->file_get_int(remminafile, "colordepth", 8));
	// fprintf(fp, "compression:i:1\r\n");
	// fprintf(fp, "keyboardhook:i:2\r\n");
	// fprintf(fp, "displayconnectionbar:i:1\r\n");
	// fprintf(fp, "disable wallpaper:i:1\r\n");
	// fprintf(fp, "disable full window drag:i:1\r\n");
	// fprintf(fp, "allow desktop composition:i:0\r\n");
	// fprintf(fp, "allow font smoothing:i:0\r\n");
	// fprintf(fp, "disable menu anims:i:1\r\n");
	// fprintf(fp, "disable themes:i:0\r\n");
	// fprintf(fp, "disable cursor setting:i:0\r\n");
	// fprintf(fp, "bitmapcachepersistenable:i:1\r\n");
	// cs = remmina_plugin_service->file_get_string(remminafile, "server");
	// fprintf(fp, "full address:s:%s\r\n", cs ? cs : "");
	// if (g_strcmp0(remmina_plugin_service->file_get_string(remminafile, "sound"), "local") == 0)
	// 	fprintf(fp, "audiomode:i:0\r\n");
	// else if (g_strcmp0(remmina_plugin_service->file_get_string(remminafile, "sound"), "remote") == 0)
	// 	fprintf(fp, "audiomode:i:1\r\n");
	// else
	// 	fprintf(fp, "audiomode:i:2\r\n");
	// if (g_strcmp0(remmina_plugin_service->file_get_string(remminafile, "microphone"), "") == 0)
	// 	fprintf(fp, "audiocapturemode:i:0\r\n");
	// else if (g_strcmp0(remmina_plugin_service->file_get_string(remminafile, "microphone"), "0") == 0)
	// 	fprintf(fp, "audiocapturemode:i:1\r\n");
	// else
	// 	fprintf(fp, "audiocapturemode:i:1\r\n");
	// fprintf(fp, "redirectprinters:i:%i\r\n", remmina_plugin_service->file_get_int(remminafile, "shareprinter", FALSE) ? 1 : 0);
	// fprintf(fp, "redirectsmartcard:i:%i\r\n", remmina_plugin_service->file_get_int(remminafile, "sharesmartcard", FALSE) ? 1 : 0);
	// fprintf(fp, "redirectcomports:i:0\r\n");
	// fprintf(fp, "redirectsmartcards:i:0\r\n");
	// fprintf(fp, "redirectclipboard:i:1\r\n");
	// fprintf(fp, "redirectposdevices:i:0\r\n");
	// fprintf(fp, "autoreconnection enabled:i:1\r\n");
	// fprintf(fp, "authentication level:i:0\r\n");
	// fprintf(fp, "prompt for credentials:i:1\r\n");
	// fprintf(fp, "negotiate security layer:i:1\r\n");
	// fprintf(fp, "remoteapplicationmode:i:0\r\n");
	// cs = remmina_plugin_service->file_get_string(remminafile, "exec");
	// fprintf(fp, "alternate shell:s:%s\r\n", cs ? cs : "");
	// cs = remmina_plugin_service->file_get_string(remminafile, "execpath");
	// fprintf(fp, "shell working directory:s:%s\r\n", cs ? cs : "");
	// cs = remmina_plugin_service->file_get_string(remminafile, "gateway_server");
	// fprintf(fp, "gatewayhostname:s:%s\r\n", cs ? cs : "");
	// fprintf(fp, "gatewayusagemethod:i:4\r\n");
	// fprintf(fp, "gatewaycredentialssource:i:4\r\n");
	// fprintf(fp, "gatewayprofileusagemethod:i:0\r\n");
	// fprintf(fp, "precommand:s:\r\n");
	// fprintf(fp, "promptcredentialonce:i:1\r\n");
	// fprintf(fp, "drivestoredirect:s:\r\n");

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