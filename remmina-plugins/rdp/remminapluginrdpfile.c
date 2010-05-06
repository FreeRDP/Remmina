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

RemminaFile*
remmina_plugin_rdp_file_import (const gchar *from_file)
{
    g_print ("import %s\n", from_file);
    return NULL;
}

void
remmina_plugin_rdp_file_export (RemminaFile *file, const gchar *to_file)
{
    g_print ("export %s file to %s\n", file->protocol, to_file);
}

