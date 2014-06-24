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

#ifndef __REMMINAEXTERNALTOOLS_H__
#define __REMMINAEXTERNALTOOLS_H__

#include <gtk/gtk.h>
#include "remmina_file.h"
#include "remmina_main.h"

G_BEGIN_DECLS

/* Open a new connection window for a .remmina file */
gboolean remmina_external_tools_from_filename(RemminaMain *remminamain,gchar* filename);
gboolean remmina_external_tools_launcher(const gchar* filename,const gchar* scriptname);

G_END_DECLS

#endif  /* __REMMINAEXTERNALTOOLS_H__  */

