/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#include <pthread.h>
#include "remmina_protocol_widget.h"
#include "remmina_sftp_client.h"
#include "remmina_ftp_client.h"
#include "remmina_ssh_plugin.h"

typedef struct remmina_masterthread_exec_data {
	enum { FUNC_GTK_LABEL_SET_TEXT,
	       FUNC_INIT_SAVE_CRED, FUNC_CHAT_RECEIVE, FUNC_FILE_GET_STRING,
	       FUNC_FTP_CLIENT_UPDATE_TASK, FUNC_FTP_CLIENT_GET_WAITING_TASK,
	       FUNC_SFTP_CLIENT_CONFIRM_RESUME,
	       FUNC_PROTOCOLWIDGET_EMIT_SIGNAL,
	       FUNC_PROTOCOLWIDGET_MPPROGRESS,
	       FUNC_PROTOCOLWIDGET_MPDESTROY,
	       FUNC_PROTOCOLWIDGET_MPSHOWRETRY,
	       FUNC_PROTOCOLWIDGET_PANELSHOWLISTEN,
	       FUNC_VTE_TERMINAL_SET_ENCODING_AND_PTY } func;

	union {
		struct {
			GtkLabel *	label;
			const gchar *	str;
		} gtk_label_set_text;
		struct {
			RemminaProtocolWidget *gp;
		} init_save_creds;
		struct {
			RemminaProtocolWidget * gp;
			const gchar *		text;
		} chat_receive;
		struct {
			RemminaFile *	remminafile;
			const gchar *	setting;
			const gchar *	retval;
		} file_get_string;
		struct {
			RemminaFTPClient *	client;
			RemminaFTPTask *	task;
		} ftp_client_update_task;
		struct {
			RemminaFTPClient *	client;
			RemminaFTPTask *	retval;
		} ftp_client_get_waiting_task;
		struct {
			RemminaProtocolWidget * gp;
			const gchar *		signal_name;
		} protocolwidget_emit_signal;
		struct {
			RemminaConnectionObject *	cnnobj;
			const gchar *			message;
			RemminaMessagePanelCallback	response_callback;
			gpointer			response_callback_data;
			RemminaMessagePanel *		ret_mp;
		} protocolwidget_mpprogress;
		struct {
			RemminaConnectionObject *	cnnobj;
			RemminaMessagePanel *		mp;
		} protocolwidget_mpdestroy;
		struct {
			RemminaProtocolWidget *gp;
		} protocolwidget_mpshowretry;
		struct {
			RemminaProtocolWidget * gp;
			int			port;
		} protocolwidget_panelshowlisten;
#ifdef HAVE_LIBSSH
		struct {
			RemminaSFTPClient *	client;
			const gchar *		path;
			gint			retval;
		} sftp_client_confirm_resume;
#endif
#ifdef HAVE_LIBVTE
		struct {
			VteTerminal *	terminal;
			const char *	codeset;
			int		master;
			int		slave;
		} vte_terminal_set_encoding_and_pty;
#endif
	} p;

	/* Mutex for thread synchronization */
	pthread_mutex_t pt_mutex;
	pthread_cond_t	pt_cond;
	/* Flag to catch cancellations */
	gboolean	cancelled;
	gboolean	complete;
} RemminaMTExecData;

void remmina_masterthread_exec_and_wait(RemminaMTExecData *d);

void remmina_masterthread_exec_save_main_thread_id(void);
gboolean remmina_masterthread_exec_is_main_thread(void);
