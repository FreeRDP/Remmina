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

#ifndef __REMMINAEXEC_H__
#define __REMMINAEXEC_H__

G_BEGIN_DECLS

typedef enum
{
	REMMINA_COMMAND_NONE = 0,
	REMMINA_COMMAND_MAIN = 1,
	REMMINA_COMMAND_PREF = 2,
	REMMINA_COMMAND_NEW = 3,
	REMMINA_COMMAND_CONNECT = 4,
	REMMINA_COMMAND_EDIT = 5,
	REMMINA_COMMAND_ABOUT = 6,
	REMMINA_COMMAND_PLUGIN = 7
} RemminaCommandType;

void remmina_exec_command(RemminaCommandType command, const gchar* data);

G_END_DECLS

#endif  /* __REMMINAEXEC_H__  */

