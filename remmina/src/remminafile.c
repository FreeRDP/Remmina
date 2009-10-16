/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "remminapublic.h"
#include "remminapref.h"
#include "remminacrypt.h"
#include "remminafile.h"

#define MIN_WINDOW_WIDTH 10
#define MIN_WINDOW_HEIGHT 10

#define DEFAULT_LISTEN_PORT 5500

const gpointer colordepth_list[] =
{
    "8", N_("256 Colors"),
    "15", N_("High Color (15 bit)"),
    "16", N_("High Color (16 bit)"),
    "24", N_("True Color (24 bit)"),
    NULL
};

const gpointer colordepth2_list[] =
{
    "0", N_("Default"),
    "2", N_("Grayscale"),
    "8", N_("256 Colors"),
    "16", N_("High Color (16 bit)"),
    "24", N_("True Color (24 bit)"),
    NULL
};

const gpointer quality_list[] =
{
    "0", N_("Poor (Fastest)"),
    "1", N_("Medium"),
    "2", N_("Good"),
    "9", N_("Best (Slowest)"),
    NULL
};

RemminaFile*
remmina_file_new_temp (void)
{
    RemminaFile *gf;

    /* Try to load from the preference file for default settings first */
    gf = remmina_file_load (remmina_pref_file);

    if (!gf)
    {
        gf = (RemminaFile*) g_malloc (sizeof (RemminaFile));

        gf->filename = NULL;
        gf->name = NULL;
        gf->group = NULL;
        gf->server = NULL;
        gf->protocol = g_strdup ("RDP");
        gf->username = NULL;
        gf->password = NULL;
        gf->domain = NULL;
        gf->clientname = NULL;
        gf->resolution = g_strdup ("AUTO");
        gf->keymap = NULL;
        gf->gkeymap = NULL;
        gf->exec = NULL;
        gf->execpath = NULL;
        gf->sound = NULL;
        gf->arguments = NULL;
        gf->cacert = NULL;
        gf->cacrl = NULL;
        gf->clientcert = NULL;
        gf->clientkey = NULL;
        gf->colordepth = 8;
        gf->quality = 0;
        gf->listenport = DEFAULT_LISTEN_PORT;
        gf->sharefolder = 0;
        gf->hscale = 0;
        gf->vscale = 0;

        gf->bitmapcaching = FALSE;
        gf->compression = FALSE;
        gf->showcursor = FALSE;
        gf->viewonly = FALSE;
        gf->console = FALSE;
        gf->disableserverinput = FALSE;
        gf->aspectscale = FALSE;
        gf->shareprinter = FALSE;
        gf->once = FALSE;

        gf->ssh_enabled = FALSE;
        gf->ssh_server = NULL;
        gf->ssh_auth = SSH_AUTH_PASSWORD;
        gf->ssh_username = NULL;
        gf->ssh_privatekey = NULL;
        gf->ssh_charset = NULL;
    }

    g_free (gf->name);
    gf->name = g_strdup (_("New Connection"));
    g_free (gf->filename);
    gf->filename = NULL;

    gf->viewmode = remmina_pref.default_mode;
    gf->scale = FALSE;
    gf->keyboard_grab = FALSE;
    gf->window_width = 640;
    gf->window_height = 480;
    gf->window_maximize = FALSE;
    gf->toolbar_opacity = 0;

    return gf;
}

static gchar*
remmina_file_generate_filename (void)
{
    GTimeVal gtime;

    g_get_current_time (&gtime);
    return g_strdup_printf ("%s/.remmina/%li%03li.remmina", g_get_home_dir (),
        gtime.tv_sec, gtime.tv_usec / 1000);
}

RemminaFile*
remmina_file_new (void)
{
    RemminaFile *gf;

    gf = remmina_file_new_temp ();
    gf->filename = remmina_file_generate_filename ();

    return gf;
}

RemminaFile*
remmina_file_copy (const gchar *filename)
{
    RemminaFile *gf;

    gf = remmina_file_load (filename);
    g_free (gf->filename);
    gf->filename = remmina_file_generate_filename ();

    return gf;
}

RemminaFile*
remmina_file_load (const gchar *filename)
{
    GKeyFile *gkeyfile;
    RemminaFile *gf;
    gchar *s;

    gkeyfile = g_key_file_new ();

    if (!g_key_file_load_from_file (gkeyfile, filename, G_KEY_FILE_NONE, NULL)) return NULL;

    if (g_key_file_has_key (gkeyfile, "remmina", "name", NULL))
    {
        gf = (RemminaFile*) g_malloc (sizeof (RemminaFile));

        gf->filename = g_strdup (filename);
        gf->name = g_key_file_get_string (gkeyfile, "remmina", "name", NULL);
        gf->group = g_key_file_get_string (gkeyfile, "remmina", "group", NULL);
        gf->server = g_key_file_get_string (gkeyfile, "remmina", "server", NULL);
        gf->protocol = g_key_file_get_string (gkeyfile, "remmina", "protocol", NULL);
        gf->username = g_key_file_get_string (gkeyfile, "remmina", "username", NULL);

        s = g_key_file_get_string (gkeyfile, "remmina", "password", NULL);
        gf->password = remmina_crypt_decrypt (s);
        g_free (s);

        gf->domain = g_key_file_get_string (gkeyfile, "remmina", "domain", NULL);
        gf->clientname = g_key_file_get_string (gkeyfile, "remmina", "clientname", NULL);
        gf->resolution = g_key_file_get_string (gkeyfile, "remmina", "resolution", NULL);
        gf->keymap = g_key_file_get_string (gkeyfile, "remmina", "keymap", NULL);
        gf->gkeymap = g_key_file_get_string (gkeyfile, "remmina", "gkeymap", NULL);
        gf->exec = g_key_file_get_string (gkeyfile, "remmina", "exec", NULL);
        gf->execpath = g_key_file_get_string (gkeyfile, "remmina", "execpath", NULL);
        gf->sound = g_key_file_get_string (gkeyfile, "remmina", "sound", NULL);
        gf->arguments = g_key_file_get_string (gkeyfile, "remmina", "arguments", NULL);
        gf->cacert = g_key_file_get_string (gkeyfile, "remmina", "cacert", NULL);
        gf->cacrl = g_key_file_get_string (gkeyfile, "remmina", "cacrl", NULL);
        gf->clientcert = g_key_file_get_string (gkeyfile, "remmina", "clientcert", NULL);
        gf->clientkey = g_key_file_get_string (gkeyfile, "remmina", "clientkey", NULL);

        gf->colordepth = g_key_file_get_integer (gkeyfile, "remmina", "colordepth", NULL);
        gf->quality = g_key_file_get_integer (gkeyfile, "remmina", "quality", NULL);
        gf->listenport = (g_key_file_has_key (gkeyfile, "remmina", "listenport", NULL) ?
            g_key_file_get_integer (gkeyfile, "remmina", "listenport", NULL) : DEFAULT_LISTEN_PORT);
        gf->sharefolder = g_key_file_get_integer (gkeyfile, "remmina", "sharefolder", NULL);
        gf->hscale = g_key_file_get_integer (gkeyfile, "remmina", "hscale", NULL);
        gf->vscale = g_key_file_get_integer (gkeyfile, "remmina", "vscale", NULL);

        gf->bitmapcaching = g_key_file_get_boolean (gkeyfile, "remmina", "bitmapcaching", NULL);
        gf->compression = g_key_file_get_boolean (gkeyfile, "remmina", "compression", NULL);
        gf->showcursor = g_key_file_get_boolean (gkeyfile, "remmina", "showcursor", NULL);
        gf->viewonly = g_key_file_get_boolean (gkeyfile, "remmina", "viewonly", NULL);
        gf->console = g_key_file_get_boolean (gkeyfile, "remmina", "console", NULL);
        gf->disableserverinput = g_key_file_get_boolean (gkeyfile, "remmina", "disableserverinput", NULL);
        gf->aspectscale = g_key_file_get_boolean (gkeyfile, "remmina", "aspectscale", NULL);
        gf->shareprinter = g_key_file_get_boolean (gkeyfile, "remmina", "shareprinter", NULL);
        gf->once = g_key_file_get_boolean (gkeyfile, "remmina", "once", NULL);

        gf->ssh_enabled = g_key_file_get_boolean (gkeyfile, "remmina", "ssh_enabled", NULL);
        gf->ssh_server = g_key_file_get_string (gkeyfile, "remmina", "ssh_server", NULL);
        gf->ssh_auth = g_key_file_get_integer (gkeyfile, "remmina", "ssh_auth", NULL);
        gf->ssh_username = g_key_file_get_string (gkeyfile, "remmina", "ssh_username", NULL);
        gf->ssh_privatekey = g_key_file_get_string (gkeyfile, "remmina", "ssh_privatekey", NULL);
        gf->ssh_charset = g_key_file_get_string (gkeyfile, "remmina", "ssh_charset", NULL);

        gf->viewmode = g_key_file_get_integer (gkeyfile, "remmina", "viewmode", NULL);
        gf->scale = g_key_file_get_boolean (gkeyfile, "remmina", "scale", NULL);
        gf->keyboard_grab = g_key_file_get_boolean (gkeyfile, "remmina", "keyboard_grab", NULL);
        gf->window_width = MAX (MIN_WINDOW_WIDTH, g_key_file_get_integer (gkeyfile, "remmina", "window_width", NULL));
        gf->window_height = MAX (MIN_WINDOW_HEIGHT, g_key_file_get_integer (gkeyfile, "remmina", "window_height", NULL));
        gf->window_maximize = (g_key_file_has_key (gkeyfile, "remmina", "window_maximize", NULL) ?
            g_key_file_get_boolean (gkeyfile, "remmina", "window_maximize", NULL) : TRUE);
        gf->toolbar_opacity = MIN (TOOLBAR_OPACITY_LEVEL,
            MAX (0, g_key_file_get_integer (gkeyfile, "remmina", "toolbar_opacity", NULL)));
    }
    else
    {
        gf = NULL;
    }

    g_key_file_free (gkeyfile);

    return gf;
}

void
remmina_file_save (RemminaFile *gf)
{
    GKeyFile *gkeyfile;
    gchar *s;
    gchar *content;
    gsize length;

    if (gf->filename == NULL) return;

    gkeyfile = g_key_file_new ();

    g_key_file_load_from_file (gkeyfile, gf->filename, G_KEY_FILE_NONE, NULL);

    g_key_file_set_string (gkeyfile, "remmina", "name", (gf->name ? gf->name : ""));
    g_key_file_set_string (gkeyfile, "remmina", "group", (gf->group ? gf->group : ""));
    g_key_file_set_string (gkeyfile, "remmina", "server", (gf->server ? gf->server : ""));
    g_key_file_set_string (gkeyfile, "remmina", "protocol", (gf->protocol ? gf->protocol : ""));
    g_key_file_set_string (gkeyfile, "remmina", "username", (gf->username ? gf->username : ""));

    s = remmina_crypt_encrypt (gf->password);
    g_key_file_set_string (gkeyfile, "remmina", "password", (s ? s : ""));
    g_free (s);

    g_key_file_set_string (gkeyfile, "remmina", "domain", (gf->domain ? gf->domain : ""));
    g_key_file_set_string (gkeyfile, "remmina", "clientname", (gf->clientname ? gf->clientname : ""));
    g_key_file_set_string (gkeyfile, "remmina", "resolution", (gf->resolution ? gf->resolution : ""));
    g_key_file_set_string (gkeyfile, "remmina", "keymap", (gf->keymap ? gf->keymap : ""));
    g_key_file_set_string (gkeyfile, "remmina", "gkeymap", (gf->gkeymap ? gf->gkeymap : ""));
    g_key_file_set_string (gkeyfile, "remmina", "exec", (gf->exec ? gf->exec : ""));
    g_key_file_set_string (gkeyfile, "remmina", "execpath", (gf->execpath ? gf->execpath : ""));
    g_key_file_set_string (gkeyfile, "remmina", "sound", (gf->sound ? gf->sound : ""));
    g_key_file_set_string (gkeyfile, "remmina", "arguments", (gf->arguments ? gf->arguments : ""));
    g_key_file_set_string (gkeyfile, "remmina", "cacert", (gf->cacert ? gf->cacert : ""));
    g_key_file_set_string (gkeyfile, "remmina", "cacrl", (gf->cacrl ? gf->cacrl : ""));
    g_key_file_set_string (gkeyfile, "remmina", "clientcert", (gf->clientcert ? gf->clientcert : ""));
    g_key_file_set_string (gkeyfile, "remmina", "clientkey", (gf->clientkey ? gf->clientkey : ""));

    g_key_file_set_integer (gkeyfile, "remmina", "colordepth", gf->colordepth);
    g_key_file_set_integer (gkeyfile, "remmina", "quality", gf->quality);
    g_key_file_set_integer (gkeyfile, "remmina", "listenport", gf->listenport);
    g_key_file_set_integer (gkeyfile, "remmina", "sharefolder", gf->sharefolder);
    g_key_file_set_integer (gkeyfile, "remmina", "hscale", gf->hscale);
    g_key_file_set_integer (gkeyfile, "remmina", "vscale", gf->vscale);

    g_key_file_set_boolean (gkeyfile, "remmina", "bitmapcaching", gf->bitmapcaching);
    g_key_file_set_boolean (gkeyfile, "remmina", "compression", gf->compression);
    g_key_file_set_boolean (gkeyfile, "remmina", "showcursor", gf->showcursor);
    g_key_file_set_boolean (gkeyfile, "remmina", "viewonly", gf->viewonly);
    g_key_file_set_boolean (gkeyfile, "remmina", "console", gf->console);
    g_key_file_set_boolean (gkeyfile, "remmina", "disableserverinput", gf->disableserverinput);
    g_key_file_set_boolean (gkeyfile, "remmina", "aspectscale", gf->aspectscale);
    g_key_file_set_boolean (gkeyfile, "remmina", "shareprinter", gf->shareprinter);
    g_key_file_set_boolean (gkeyfile, "remmina", "once", gf->once);

    g_key_file_set_boolean (gkeyfile, "remmina", "ssh_enabled", gf->ssh_enabled);
    g_key_file_set_string (gkeyfile, "remmina", "ssh_server", (gf->ssh_server ? gf->ssh_server : ""));
    g_key_file_set_integer (gkeyfile, "remmina", "ssh_auth", gf->ssh_auth);
    g_key_file_set_string (gkeyfile, "remmina", "ssh_username", (gf->ssh_username ? gf->ssh_username : ""));
    g_key_file_set_string (gkeyfile, "remmina", "ssh_privatekey", (gf->ssh_privatekey ? gf->ssh_privatekey : ""));
    g_key_file_set_string (gkeyfile, "remmina", "ssh_charset", (gf->ssh_charset ? gf->ssh_charset : ""));

    g_key_file_set_integer (gkeyfile, "remmina", "viewmode", gf->viewmode);
    g_key_file_set_boolean (gkeyfile, "remmina", "scale", gf->scale);
    g_key_file_set_boolean (gkeyfile, "remmina", "keyboard_grab", gf->keyboard_grab);
    g_key_file_set_integer (gkeyfile, "remmina", "window_width", gf->window_width);
    g_key_file_set_integer (gkeyfile, "remmina", "window_height", gf->window_height);
    g_key_file_set_boolean (gkeyfile, "remmina", "window_maximize", gf->window_maximize);
    g_key_file_set_integer (gkeyfile, "remmina", "toolbar_opacity", gf->toolbar_opacity);

    content = g_key_file_to_data (gkeyfile, &length, NULL);
    g_file_set_contents (gf->filename, content, length, NULL);

    g_key_file_free (gkeyfile);
    g_free (content);
}

void
remmina_file_free (RemminaFile *remminafile)
{
    if (remminafile == NULL) return;

    g_free (remminafile->filename);
    g_free (remminafile->name);
    g_free (remminafile->group);
    g_free (remminafile->server);
    g_free (remminafile->protocol);
    g_free (remminafile->username);
    g_free (remminafile->password);
    g_free (remminafile->domain);
    g_free (remminafile->clientname);
    g_free (remminafile->resolution);
    g_free (remminafile->keymap);
    g_free (remminafile->gkeymap);
    g_free (remminafile->exec);
    g_free (remminafile->execpath);
    g_free (remminafile->sound);
    g_free (remminafile->arguments);
    g_free (remminafile->cacert);
    g_free (remminafile->cacrl);
    g_free (remminafile->clientcert);
    g_free (remminafile->clientkey);

    g_free (remminafile->ssh_server);
    g_free (remminafile->ssh_username);
    g_free (remminafile->ssh_privatekey);
    g_free (remminafile->ssh_charset);
    g_free (remminafile);
}

RemminaFile*
remmina_file_dup (RemminaFile *remminafile)
{
    RemminaFile *gf;

    gf = (RemminaFile*) g_malloc (sizeof (RemminaFile));
    gf->filename = g_strdup (remminafile->filename);
    gf->name = g_strdup (remminafile->name);
    gf->group = g_strdup (remminafile->group);
    gf->server = g_strdup (remminafile->server);
    gf->protocol = g_strdup (remminafile->protocol);
    gf->username = g_strdup (remminafile->username);
    gf->password = g_strdup (remminafile->password);
    gf->domain = g_strdup (remminafile->domain);
    gf->clientname = g_strdup (remminafile->clientname);
    gf->resolution = g_strdup (remminafile->resolution);
    gf->keymap = g_strdup (remminafile->keymap);
    gf->gkeymap = g_strdup (remminafile->gkeymap);
    gf->exec = g_strdup (remminafile->exec);
    gf->execpath = g_strdup (remminafile->execpath);
    gf->sound = g_strdup (remminafile->sound);
    gf->arguments = g_strdup (remminafile->arguments);
    gf->cacert = g_strdup (remminafile->cacert);
    gf->cacrl = g_strdup (remminafile->cacrl);
    gf->clientcert = g_strdup (remminafile->clientcert);
    gf->clientkey = g_strdup (remminafile->clientkey);

    gf->colordepth = remminafile->colordepth;
    gf->quality = remminafile->quality;
    gf->listenport = remminafile->listenport;
    gf->sharefolder = remminafile->sharefolder;
    gf->hscale = remminafile->hscale;
    gf->vscale = remminafile->vscale;

    gf->bitmapcaching = remminafile->bitmapcaching;
    gf->compression = remminafile->compression;
    gf->showcursor = remminafile->showcursor;
    gf->viewonly = remminafile->viewonly;
    gf->console = remminafile->console;
    gf->disableserverinput = remminafile->disableserverinput;
    gf->aspectscale = remminafile->aspectscale;
    gf->shareprinter = remminafile->shareprinter;
    gf->once = remminafile->once;

    gf->ssh_enabled = remminafile->ssh_enabled;
    gf->ssh_server = g_strdup (remminafile->ssh_server);
    gf->ssh_auth = remminafile->ssh_auth;
    gf->ssh_username = g_strdup (remminafile->ssh_username);
    gf->ssh_privatekey = g_strdup (remminafile->ssh_privatekey);
    gf->ssh_charset = g_strdup (remminafile->ssh_charset);

    gf->viewmode = remminafile->viewmode;
    gf->scale = remminafile->scale;
    gf->keyboard_grab = remminafile->keyboard_grab;
    gf->window_width = remminafile->window_width;
    gf->window_height = remminafile->window_height;
    gf->window_maximize = remminafile->window_maximize;
    gf->toolbar_opacity = remminafile->toolbar_opacity;

    return gf;
}

void
remmina_file_update_screen_resolution (RemminaFile *remminafile)
{
    GdkScreen *screen;
    gchar *tmp, *pos;


    if (remminafile->resolution == NULL ||
        g_strcmp0 (remminafile->resolution, "AUTO") == 0 ||
        g_strrstr (remminafile->resolution, "x") == NULL)
    {
        screen = gdk_screen_get_default ();
        remminafile->resolution_width = gdk_screen_get_width (screen);
        remminafile->resolution_height = gdk_screen_get_height (screen);
    }
    else
    {
        tmp = g_strdup (remminafile->resolution);
        pos = g_strrstr (tmp, "x");
        *pos++ = '\0';
        remminafile->resolution_width = MAX (100, MIN (4096, atoi (tmp)));
        remminafile->resolution_height = MAX (100, MIN (4096, atoi (pos)));
    }
}

const gchar*
remmina_file_get_icon_name (RemminaFile *remminafile)
{
    if (g_strcmp0 (remminafile->protocol, "SFTP") == 0)
    {
        return "remmina-sftp";
    }
    else if (g_strcmp0 (remminafile->protocol, "RDP") == 0)
    {
        return (remminafile->ssh_enabled ? "remmina-rdp-ssh" : "remmina-rdp");
    }
    else if (strncmp (remminafile->protocol, "VNC", 3) == 0)
    {
        return (remminafile->ssh_enabled ? "remmina-vnc-ssh" : "remmina-vnc");
    }
    else if (g_strcmp0 (remminafile->protocol, "XDMCP") == 0)
    {
        return (remminafile->ssh_enabled ? "remmina-xdmcp-ssh" : "remmina-xdmcp");
    }
    else
    {
        return "remmina";
    }
}

gboolean
remmina_file_is_incoming (RemminaFile *remminafile)
{
    return (strncmp (remminafile->protocol, "VNC", 3) == 0 && remminafile->protocol[3] == 'I');
}

