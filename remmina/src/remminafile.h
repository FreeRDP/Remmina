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
 

#ifndef __REMMINAFILE_H__
#define __REMMINAFILE_H__

G_BEGIN_DECLS

enum
{
    SSH_AUTH_PASSWORD,
    SSH_AUTH_PUBLICKEY,
    SSH_AUTH_AUTO_PUBLICKEY
};

#define TOOLBAR_OPACITY_LEVEL 8
#define TOOLBAR_OPACITY_MIN 0.2

typedef struct _RemminaFile
{
    /* Profile settings */
    gchar *filename;
    gchar *name;
    gchar *group;

    /* Protocol settings */
    gchar *server;
    gchar *protocol;
    gchar *username;
    gchar *password;
    gchar *domain;
    gchar *clientname;
    gchar *resolution;
    gchar *keymap;
    gchar *gkeymap;
    gchar *exec;
    gchar *execpath;
    gchar *sound;
    gchar *arguments;
    gchar *cacert;
    gchar *cacrl;
    gchar *clientcert;
    gchar *clientkey;
    gint colordepth;
    gint quality;
    gint listenport;
    gint sharefolder;
    gint hscale;
    gint vscale;
    gboolean bitmapcaching;
    gboolean compression;
    gboolean showcursor;
    gboolean viewonly;
    gboolean console;
    gboolean disableserverinput;
    gboolean aspectscale;
    gboolean shareprinter;
    gboolean once;

    /* SSH settings */
    gboolean ssh_enabled;
    gchar *ssh_server;
    gint ssh_auth;
    gchar *ssh_username;
    gchar *ssh_privatekey;
    gchar *ssh_charset;

    /* Run-time settings */
    gint viewmode;
    gboolean scale;
    gboolean keyboard_grab;

    gint window_width;
    gint window_height;
    gboolean window_maximize;

    gint toolbar_opacity;

    gint resolution_width;
    gint resolution_height;
} RemminaFile;

extern const gpointer colordepth_list[];
extern const gpointer colordepth2_list[];
extern const gpointer quality_list[];

/* Create a empty .remmina file */
RemminaFile* remmina_file_new (void);
RemminaFile* remmina_file_new_temp (void);
RemminaFile* remmina_file_copy (const gchar *filename);
/* Load a new .remmina file and return the allocated RemminaFile object */
RemminaFile* remmina_file_load (const gchar *filename);
/* Create or overwrite the .remmina file */
void remmina_file_save (RemminaFile *gf);
/* Free the RemminaFile object */
void remmina_file_free (RemminaFile *remminafile);
/* Duplicate a RemminaFile object */
RemminaFile* remmina_file_dup (RemminaFile *remminafile);
/* Update the screen width and height members */
void remmina_file_update_screen_resolution (RemminaFile *remminafile);
/* Get the protocol icon name */
const gchar* remmina_file_get_icon_name (RemminaFile *remminafile);
/* Is an incoming protocol */
gboolean remmina_file_is_incoming (RemminaFile *remminafile);

G_END_DECLS

#endif  /* __REMMINAFILE_H__  */

