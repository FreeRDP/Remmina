/*  See LICENSE and COPYING files for copyright and license details. */

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

