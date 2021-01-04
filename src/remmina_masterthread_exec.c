/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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

/* Support for execution on main thread of some GTK related
 * functions (due to threads deprecations in GTK) */

#include <gtk/gtk.h>

#include "remmina_masterthread_exec.h"

static pthread_t gMainThreadID;

static gboolean remmina_masterthread_exec_callback(RemminaMTExecData *d)
{

	/* This function is called on main GTK Thread via gdk_threads_add_idlde()
	 * from remmina_masterthread_exec_and_wait() */

	if (!d->cancelled) {
		switch (d->func) {
		case FUNC_INIT_SAVE_CRED:
			remmina_protocol_widget_save_cred(d->p.init_save_creds.gp);
			break;
		case FUNC_CHAT_RECEIVE:
			remmina_protocol_widget_chat_receive(d->p.chat_receive.gp, d->p.chat_receive.text);
			break;
		case FUNC_FILE_GET_STRING:
			d->p.file_get_string.retval = remmina_file_get_string( d->p.file_get_string.remminafile, d->p.file_get_string.setting );
			break;
		case FUNC_GTK_LABEL_SET_TEXT:
			gtk_label_set_text( d->p.gtk_label_set_text.label, d->p.gtk_label_set_text.str );
			break;
		case FUNC_FTP_CLIENT_UPDATE_TASK:
			remmina_ftp_client_update_task( d->p.ftp_client_update_task.client, d->p.ftp_client_update_task.task );
			break;
		case FUNC_FTP_CLIENT_GET_WAITING_TASK:
			d->p.ftp_client_get_waiting_task.retval = remmina_ftp_client_get_waiting_task( d->p.ftp_client_get_waiting_task.client );
			break;
		case FUNC_PROTOCOLWIDGET_EMIT_SIGNAL:
			remmina_protocol_widget_emit_signal(d->p.protocolwidget_emit_signal.gp, d->p.protocolwidget_emit_signal.signal_name);
			break;
		case FUNC_PROTOCOLWIDGET_MPPROGRESS:
			d->p.protocolwidget_mpprogress.ret_mp = remmina_protocol_widget_mpprogress(d->p.protocolwidget_mpprogress.cnnobj, d->p.protocolwidget_mpprogress.message,
				d->p.protocolwidget_mpprogress.response_callback, d->p.protocolwidget_mpprogress.response_callback_data);
			break;
		case FUNC_PROTOCOLWIDGET_MPDESTROY:
			remmina_protocol_widget_mpdestroy(d->p.protocolwidget_mpdestroy.cnnobj, d->p.protocolwidget_mpdestroy.mp);
			break;
		case FUNC_PROTOCOLWIDGET_MPSHOWRETRY:
			remmina_protocol_widget_panel_show_retry(d->p.protocolwidget_mpshowretry.gp);
			break;
		case FUNC_PROTOCOLWIDGET_PANELSHOWLISTEN:
			remmina_protocol_widget_panel_show_listen(d->p.protocolwidget_panelshowlisten.gp, d->p.protocolwidget_panelshowlisten.port);
			break;
		case FUNC_SFTP_CLIENT_CONFIRM_RESUME:
#ifdef HAVE_LIBSSH
			d->p.sftp_client_confirm_resume.retval = remmina_sftp_client_confirm_resume( d->p.sftp_client_confirm_resume.client,
				d->p.sftp_client_confirm_resume.path );
#endif
			break;
		case FUNC_VTE_TERMINAL_SET_ENCODING_AND_PTY:
#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)
			remmina_plugin_ssh_vte_terminal_set_encoding_and_pty( d->p.vte_terminal_set_encoding_and_pty.terminal,
				d->p.vte_terminal_set_encoding_and_pty.codeset,
				d->p.vte_terminal_set_encoding_and_pty.master,
				d->p.vte_terminal_set_encoding_and_pty.slave);
#endif
			break;

		}
		pthread_mutex_lock(&d->pt_mutex);
		d->complete = TRUE;
		pthread_cond_signal(&d->pt_cond);
		pthread_mutex_unlock(&d->pt_mutex);
	}else  {
		/* thread has been cancelled, so we must free d memory here */
		g_free(d);
	}
	return G_SOURCE_REMOVE;
}

static void remmina_masterthread_exec_cleanup_handler(gpointer data)
{
	RemminaMTExecData *d = data;

	d->cancelled = TRUE;
}

void remmina_masterthread_exec_and_wait(RemminaMTExecData *d)
{
	d->cancelled = FALSE;
	d->complete = FALSE;
	pthread_cleanup_push(remmina_masterthread_exec_cleanup_handler, (void*)d);
	pthread_mutex_init(&d->pt_mutex, NULL);
	pthread_cond_init(&d->pt_cond, NULL);
	gdk_threads_add_idle((GSourceFunc)remmina_masterthread_exec_callback, (gpointer)d);
	pthread_mutex_lock(&d->pt_mutex);
	while (!d->complete)
		pthread_cond_wait(&d->pt_cond, &d->pt_mutex);
	pthread_cleanup_pop(0);
	pthread_mutex_destroy(&d->pt_mutex);
	pthread_cond_destroy(&d->pt_cond);
}

void remmina_masterthread_exec_save_main_thread_id()
{
	/* To be called from main thread at startup */
	gMainThreadID = pthread_self();
}

gboolean remmina_masterthread_exec_is_main_thread()
{
	return pthread_equal(gMainThreadID, pthread_self()) != 0;
}

