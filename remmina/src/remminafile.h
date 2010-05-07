/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee 
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

#include "remmina/types.h" 

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

extern const gpointer colordepth_list[];
extern const gpointer colordepth2_list[];
extern const gpointer quality_list[];

/* Create a empty .remmina file */
RemminaFile* remmina_file_new (void);
RemminaFile* remmina_file_new_temp (void);
RemminaFile* remmina_file_copy (const gchar *filename);
gchar* remmina_file_generate_filename (void);
/* Load a new .remmina file and return the allocated RemminaFile object */
RemminaFile* remmina_file_load (const gchar *filename);
/* Create or overwrite the .remmina file */
void remmina_file_save_profile (RemminaFile *gf);
void remmina_file_save_credential (RemminaFile *gf);
void remmina_file_save_runtime (RemminaFile *gf);
void remmina_file_save_all (RemminaFile *gf);
/* Free the RemminaFile object */
void remmina_file_free (RemminaFile *remminafile);
/* Duplicate a RemminaFile object */
RemminaFile* remmina_file_dup (RemminaFile *remminafile);
/* Update the screen width and height members */
void remmina_file_update_screen_resolution (RemminaFile *remminafile);
/* Get the protocol icon name */
const gchar* remmina_file_get_icon_name (RemminaFile *remminafile);
/* Duplicate a temporary RemminaFile and change the protocol */
RemminaFile* remmina_file_dup_temp_protocol (RemminaFile *remminafile, const gchar *new_protocol);

G_END_DECLS

#endif  /* __REMMINAFILE_H__  */

