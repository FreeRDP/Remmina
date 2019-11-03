/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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

#pragma once

G_BEGIN_DECLS

#define REMMINA_TYPE_FTP_CLIENT               (remmina_ftp_client_get_type())
#define REMMINA_FTP_CLIENT(obj)               (G_TYPE_CHECK_INSTANCE_CAST((obj), REMMINA_TYPE_FTP_CLIENT, RemminaFTPClient))
#define REMMINA_FTP_CLIENT_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST((klass), REMMINA_TYPE_FTP_CLIENT, RemminaFTPClientClass))
#define REMMINA_IS_FTP_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_TYPE((obj), REMMINA_TYPE_FTP_CLIENT))
#define REMMINA_IS_FTP_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE((klass), REMMINA_TYPE_FTP_CLIENT))
#define REMMINA_FTP_CLIENT_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS((obj), REMMINA_TYPE_FTP_CLIENT, RemminaFTPClientClass))

typedef struct _RemminaFTPClientPriv RemminaFTPClientPriv;

typedef struct _RemminaFTPClient {
	GtkVBox			vbox;

	RemminaFTPClientPriv *	priv;
} RemminaFTPClient;

typedef struct _RemminaFTPClientClass {
	GtkVBoxClass parent_class;

	void (*open_dir)(RemminaFTPClient *client);
	void (*new_task)(RemminaFTPClient *client);
	void (*cancel_task)(RemminaFTPClient *client);
	void (*delete_file)(RemminaFTPClient *client);
} RemminaFTPClientClass;

GType remmina_ftp_client_get_type(void)
G_GNUC_CONST;

enum {
	REMMINA_FTP_FILE_TYPE_DIR, REMMINA_FTP_FILE_TYPE_FILE, REMMINA_FTP_FILE_N_TYPES,
};

enum {
	REMMINA_FTP_FILE_COLUMN_TYPE,
	REMMINA_FTP_FILE_COLUMN_NAME,
	REMMINA_FTP_FILE_COLUMN_SIZE,
	REMMINA_FTP_FILE_COLUMN_USER,
	REMMINA_FTP_FILE_COLUMN_GROUP,
	REMMINA_FTP_FILE_COLUMN_PERMISSION,
	REMMINA_FTP_FILE_COLUMN_NAME_SORT, /* Auto populate */
	REMMINA_FTP_FILE_N_COLUMNS
};

enum {
	REMMINA_FTP_TASK_TYPE_DOWNLOAD, REMMINA_FTP_TASK_TYPE_UPLOAD, REMMINA_FTP_TASK_N_TYPES
};

enum {
	REMMINA_FTP_TASK_STATUS_WAIT,
	REMMINA_FTP_TASK_STATUS_RUN,
	REMMINA_FTP_TASK_STATUS_FINISH,
	REMMINA_FTP_TASK_STATUS_ERROR,
	REMMINA_FTP_TASK_N_STATUSES
};

enum {
	REMMINA_FTP_TASK_COLUMN_TYPE,
	REMMINA_FTP_TASK_COLUMN_NAME,
	REMMINA_FTP_TASK_COLUMN_SIZE,
	REMMINA_FTP_TASK_COLUMN_TASKID,
	REMMINA_FTP_TASK_COLUMN_TASKTYPE,
	REMMINA_FTP_TASK_COLUMN_REMOTEDIR,
	REMMINA_FTP_TASK_COLUMN_LOCALDIR,
	REMMINA_FTP_TASK_COLUMN_STATUS,
	REMMINA_FTP_TASK_COLUMN_DONESIZE,
	REMMINA_FTP_TASK_COLUMN_TOOLTIP,
	REMMINA_FTP_TASK_N_COLUMNS
};

typedef struct _RemminaFTPTask {
	/* Read-only */
	gint			type;
	gchar *			name;
	gint			taskid;
	gint			tasktype;
	gchar *			remotedir;
	gchar *			localdir;
	GtkTreeRowReference *	rowref;
	/* Updatable */
	gfloat			size;
	gint			status;
	gfloat			donesize;
	gchar *			tooltip;
} RemminaFTPTask;

GtkWidget *remmina_ftp_client_new(void);

void remmina_ftp_client_save_state(RemminaFTPClient *client, RemminaFile *remminafile);
void remmina_ftp_client_load_state(RemminaFTPClient *client, RemminaFile *remminafile);

void remmina_ftp_client_set_show_hidden(RemminaFTPClient *client, gboolean show_hidden);
void remmina_ftp_client_clear_file_list(RemminaFTPClient *client);
/* column, value, …, -1 */
void remmina_ftp_client_add_file(RemminaFTPClient *client, ...);
/* Set the current directory. Should be called by opendir signal handler */
void remmina_ftp_client_set_dir(RemminaFTPClient *client, const gchar *dir);
/* Get the current directory as newly allocated string */
gchar *remmina_ftp_client_get_dir(RemminaFTPClient *client);
/* Get the next waiting task */
RemminaFTPTask *remmina_ftp_client_get_waiting_task(RemminaFTPClient *client);
/* Update the task */
void remmina_ftp_client_update_task(RemminaFTPClient *client, RemminaFTPTask *task);
/* Free the RemminaFTPTask object */
void remmina_ftp_task_free(RemminaFTPTask *task);
/* Get/Set Set overwrite_all status */
void remmina_ftp_client_set_overwrite_status(RemminaFTPClient *client, gboolean status);
gboolean remmina_ftp_client_get_overwrite_status(RemminaFTPClient *client);

G_END_DECLS
