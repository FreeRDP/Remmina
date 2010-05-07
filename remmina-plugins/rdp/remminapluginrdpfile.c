/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 */

#include "common/remminaplugincommon.h"
#include "remminapluginrdpfile.h"

gboolean
remmina_plugin_rdp_file_import_test (const gchar *from_file)
{
    gchar *ext;

    ext = strrchr (from_file, '.');
    if (!ext) return FALSE;
    ext++;
    if (g_strcmp0 (ext, "RDP") == 0) return TRUE;
    if (g_strcmp0 (ext, "rdp") == 0) return TRUE;
    return FALSE;
}

static void
remmina_plugin_rdp_file_import_field (RemminaFile *remminafile, const gchar *key, const gchar *value)
{
    if (g_strcmp0 (key, "desktopwidth") == 0)
    {
        remminafile->resolution_width = atoi (value);
    }
    else if (g_strcmp0 (key, "desktopheight") == 0)
    {
        remminafile->resolution_height = atoi (value);
    }
    else if (g_strcmp0 (key, "session bpp") == 0)
    {
        remminafile->colordepth = atoi (value);
    }
    else if (g_strcmp0 (key, "keyboardhook") == 0)
    {
        remminafile->keyboard_grab = (atoi (value) == 1);
    }
    else if (g_strcmp0 (key, "full address") == 0)
    {
        remminafile->server = g_strdup (value);
    }
    else if (g_strcmp0 (key, "audiomode") == 0)
    {
        switch (atoi (value))
        {
        case 0:
            remminafile->sound = g_strdup ("local");
            break;
        case 1:
            remminafile->sound = g_strdup ("remote");
            break;
        }
    }
    else if (g_strcmp0 (key, "redirectprinters") == 0)
    {
        remminafile->shareprinter = (atoi (value) == 1);
    }
    else if (g_strcmp0 (key, "redirectclipboard") == 0)
    {
        /* TODO: Provide an option to disable clipboard sync in main program */
    }
    else if (g_strcmp0 (key, "alternate shell") == 0)
    {
        remminafile->exec = g_strdup (value);
    }
    else if (g_strcmp0 (key, "shell working directory") == 0)
    {
        remminafile->execpath = g_strdup (value);
    }
}

static RemminaFile*
remmina_plugin_rdp_file_import_channel (GIOChannel *channel)
{
    RemminaFile *remminafile;
    GError *error = NULL;
    const gchar *enc;
    guchar magic[2] = { 0 };
    gsize bytes_read = 0;
    gchar *line = NULL;
    gchar *p;

    if (g_io_channel_set_encoding (channel, NULL, &error) != G_IO_STATUS_NORMAL)
    {
        g_print ("g_io_channel_set_encoding: %s\n", error->message);
        return NULL;
    }
    /* Try to detect the UTF-16 encoding */
    if (g_io_channel_read_chars (channel, (gchar *) magic, 2, &bytes_read, &error) != G_IO_STATUS_NORMAL)
    {
        g_print ("g_io_channel_read_chars: %s\n", error->message);
        return NULL;
    }
    if (magic[0] == 0xff && magic[1] == 0xfe)
    {
        enc = "UTF-16LE";
    }
    else if (magic[0] == 0xfe && magic[1] == 0xff)
    {
        enc = "UTF-16BE";
    }
    else
    {
        enc = "UTF-8";
        if (g_io_channel_seek (channel, 0, G_SEEK_SET) != G_IO_ERROR_NONE)
        {
            g_print ("g_io_channel_seek: failed\n");
            return NULL;
        }
    }
    if (g_io_channel_set_encoding (channel, enc, &error) != G_IO_STATUS_NORMAL)
    {
        g_print ("g_io_channel_set_encoding: %s\n", error->message);
        return NULL;
    }

    remminafile = g_new0 (RemminaFile, 1);
    while (g_io_channel_read_line (channel, &line, NULL, &bytes_read, &error) == G_IO_STATUS_NORMAL)
    {
        if (line == NULL) break;
        line[bytes_read] = '\0';
        p = strchr (line, ':');
        if (p)
        {
            *p++ = '\0';
            p = strchr (p, ':');
            if (p)
            {
                p++;
                remmina_plugin_rdp_file_import_field (remminafile, line, p);
            }
        }
        g_free (line);
    }
    if (remminafile->resolution_width > 0 && remminafile->resolution_height > 0)
    {
        remminafile->resolution = g_strdup_printf ("%ix%i", remminafile->resolution_width, remminafile->resolution_height);
    }
    remminafile->name = g_strdup (remminafile->server);
    remminafile->protocol = g_strdup ("RDP");

    return remminafile;
}

RemminaFile*
remmina_plugin_rdp_file_import (const gchar *from_file)
{
    RemminaFile *remminafile;
    GIOChannel *channel;
    GError *error = NULL;

    channel = g_io_channel_new_file (from_file, "r", &error);
    if (channel == NULL)
    {
        g_print ("Failed to import %s: %s\n", from_file, error->message);
        return NULL;
    }    
    remminafile = remmina_plugin_rdp_file_import_channel (channel);
    g_io_channel_close (channel);
    return remminafile;
}

gboolean
remmina_plugin_rdp_file_export_test (RemminaFile *remminafile)
{
    if (g_strcmp0 (remminafile->protocol, "RDP") == 0) return TRUE;
    return FALSE;
}

gboolean
remmina_plugin_rdp_file_export_channel (RemminaFile *remminafile, FILE *fp)
{
    gchar *s;
    gchar *p;

    fprintf (fp, "screen mode id:i:2\r\n");
    s = g_strdup (remminafile->resolution);
    p = strchr (s, 'x');
    if (p)
    {
        *p++ = '\0';
        fprintf (fp, "desktopwidth:i:%s\r\n", s);
        fprintf (fp, "desktopheight:i:%s\r\n", p);
    }
    g_free (s);
    fprintf (fp, "session bpp:i:%i\r\n", remminafile->colordepth);
    //fprintf (fp, "winposstr:s:0,1,123,34,931,661\r\n");
    fprintf (fp, "compression:i:1\r\n");
    fprintf (fp, "keyboardhook:i:2\r\n");
    fprintf (fp, "displayconnectionbar:i:1\r\n");
    fprintf (fp, "disable wallpaper:i:1\r\n");
    fprintf (fp, "disable full window drag:i:1\r\n");
    fprintf (fp, "allow desktop composition:i:0\r\n");
    fprintf (fp, "allow font smoothing:i:0\r\n");
    fprintf (fp, "disable menu anims:i:1\r\n");
    fprintf (fp, "disable themes:i:0\r\n");
    fprintf (fp, "disable cursor setting:i:0\r\n");
    fprintf (fp, "bitmapcachepersistenable:i:1\r\n");
    fprintf (fp, "full address:s:%s\r\n", remminafile->server);
    if (g_strcmp0 (remminafile->sound, "local") == 0)
        fprintf (fp, "audiomode:i:0\r\n");
    else if (g_strcmp0 (remminafile->sound, "remote") == 0)
        fprintf (fp, "audiomode:i:1\r\n");
    else
        fprintf (fp, "audiomode:i:2\r\n");
    fprintf (fp, "redirectprinters:i:%i\r\n", remminafile->shareprinter ? 1 : 0);
    fprintf (fp, "redirectcomports:i:0\r\n");
    fprintf (fp, "redirectsmartcards:i:0\r\n");
    fprintf (fp, "redirectclipboard:i:1\r\n");
    fprintf (fp, "redirectposdevices:i:0\r\n");
    fprintf (fp, "autoreconnection enabled:i:1\r\n");
    fprintf (fp, "authentication level:i:0\r\n");
    fprintf (fp, "prompt for credentials:i:1\r\n");
    fprintf (fp, "negotiate security layer:i:1\r\n");
    fprintf (fp, "remoteapplicationmode:i:0\r\n");
    fprintf (fp, "alternate shell:s:%s\r\n", remminafile->exec ? remminafile->exec : "");
    fprintf (fp, "shell working directory:s:%s\r\n", remminafile->execpath ? remminafile->execpath : "");
    fprintf (fp, "gatewayhostname:s:\r\n");
    fprintf (fp, "gatewayusagemethod:i:4\r\n");
    fprintf (fp, "gatewaycredentialssource:i:4\r\n");
    fprintf (fp, "gatewayprofileusagemethod:i:0\r\n");
    fprintf (fp, "promptcredentialonce:i:1\r\n");
    fprintf (fp, "drivestoredirect:s:\r\n");
    return TRUE;
}

gboolean
remmina_plugin_rdp_file_export (RemminaFile *remminafile, const gchar *to_file)
{
    FILE *fp;
    gboolean ret;
    gchar *p;

    p = strrchr (to_file, '.');
    if (p && (g_strcmp0 (p + 1, "rdp") == 0 || g_strcmp0 (p + 1, "RDP") == 0))
    {
        p = g_strdup (to_file);
    }
    else
    {
        p = g_strdup_printf ("%s.rdp", to_file);
    }
    fp = g_fopen (p, "w+");
    if (fp == NULL)
    {
        g_print ("Failed to export %s\n", p);
        g_free (p);
        return FALSE;
    }    
    g_free (p);
    ret = remmina_plugin_rdp_file_export_channel (remminafile, fp);
    fclose (fp);

    return ret;
}

