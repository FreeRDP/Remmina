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
 

#ifndef __REMMINAAPPLETUTIL_H__
#define __REMMINAAPPLETUTIL_H__

G_BEGIN_DECLS

#define MAX_PATH_LEN 255

typedef enum
{
    REMMINA_LAUNCH_MAIN,
    REMMINA_LAUNCH_PREF,
    REMMINA_LAUNCH_QUICK,
    REMMINA_LAUNCH_FILE,
    REMMINA_LAUNCH_EDIT,
    REMMINA_LAUNCH_NEW
} RemminaLaunchType;

void remmina_applet_util_launcher (RemminaLaunchType launch_type, const gchar *filename, const gchar *server, const gchar *protocol);

gboolean remmina_applet_util_get_pref_boolean (const gchar *key);
void remmina_applet_util_set_pref_boolean (const gchar *key, gboolean value);

G_END_DECLS

#endif  /* __REMMINAAPPLETUTIL_H__  */

