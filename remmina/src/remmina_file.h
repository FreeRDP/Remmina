/*  See LICENSE and COPYING files for copyright and license details. */

#include "remmina/types.h"

#ifndef __REMMINAFILE_H__
#define __REMMINAFILE_H__

G_BEGIN_DECLS

struct _RemminaFile
{
	gchar *filename;
	GHashTable *settings;
};

enum
{
	SSH_AUTH_PASSWORD, SSH_AUTH_PUBLICKEY, SSH_AUTH_AUTO_PUBLICKEY
};

typedef enum
{
	REMMINA_SETTING_GROUP_NONE,
	REMMINA_SETTING_GROUP_PROFILE,
	REMMINA_SETTING_GROUP_CREDENTIAL,
	REMMINA_SETTING_GROUP_RUNTIME,
	REMMINA_SETTING_GROUP_ALL
} RemminaSettingGroup;

#define TOOLBAR_OPACITY_LEVEL 8
#define TOOLBAR_OPACITY_MIN 0.2

/* Create a empty .remmina file */
RemminaFile* remmina_file_new(void);
RemminaFile* remmina_file_copy(const gchar *filename);
void remmina_file_generate_filename(RemminaFile *remminafile);
void remmina_file_set_filename(RemminaFile *remminafile, const gchar *filename);
const gchar* remmina_file_get_filename(RemminaFile *remminafile);
/* Load a new .remmina file and return the allocated RemminaFile object */
RemminaFile* remmina_file_load(const gchar *filename);
/* Settings get/set functions */
void remmina_file_set_string(RemminaFile *remminafile, const gchar *setting, const gchar *value);
void remmina_file_set_string_ref(RemminaFile *remminafile, const gchar *setting, gchar *value);
const gchar* remmina_file_get_string(RemminaFile *remminafile, const gchar *setting);
gchar* remmina_file_get_secret(RemminaFile *remminafile, const gchar *setting);
void remmina_file_set_int(RemminaFile *remminafile, const gchar *setting, gint value);
gint remmina_file_get_int(RemminaFile *remminafile, const gchar *setting, gint default_value);
/* Create or overwrite the .remmina file */
void remmina_file_save_group(RemminaFile *remminafile, RemminaSettingGroup group);
void remmina_file_save_all(RemminaFile *remminafile);
/* Free the RemminaFile object */
void remmina_file_free(RemminaFile *remminafile);
/* Duplicate a RemminaFile object */
RemminaFile* remmina_file_dup(RemminaFile *remminafile);
/* Update the screen width and height members */
void remmina_file_update_screen_resolution(RemminaFile *remminafile);
/* Get the protocol icon name */
const gchar* remmina_file_get_icon_name(RemminaFile *remminafile);
/* Duplicate a temporary RemminaFile and change the protocol */
RemminaFile* remmina_file_dup_temp_protocol(RemminaFile *remminafile, const gchar *new_protocol);
/* Delete a .remmina file */
void remmina_file_delete(const gchar *filename);
/* Delete a "password" field and save into .remmina file */
void remmina_file_unsave_password(RemminaFile *remminafile);

G_END_DECLS

#endif  /* __REMMINAFILE_H__  */

