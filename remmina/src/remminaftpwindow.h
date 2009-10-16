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
 

#ifndef __REMMINAFTPWINDOW_H__
#define __REMMINAFTPWINDOW_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_FTP_WINDOW               (remmina_ftp_window_get_type ())
#define REMMINA_FTP_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_FTP_WINDOW, RemminaFTPWindow))
#define REMMINA_FTP_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_FTP_WINDOW, RemminaFTPWindowClass))
#define REMMINA_IS_FTP_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_FTP_WINDOW))
#define REMMINA_IS_FTP_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_FTP_WINDOW))
#define REMMINA_FTP_WINDOW_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_FTP_WINDOW, RemminaFTPWindowClass))

typedef struct _RemminaFTPWindowPriv RemminaFTPWindowPriv;

typedef struct _RemminaFTPWindow
{
    GtkWindow window;

    RemminaFTPWindowPriv *priv;
} RemminaFTPWindow;

typedef struct _RemminaFTPWindowClass
{
    GtkWindowClass parent_class;

    void (* disconnect) (RemminaFTPWindow *window);
    void (* open_dir) (RemminaFTPWindow *window);
    void (* new_task) (RemminaFTPWindow *window);
    void (* cancel_task) (RemminaFTPWindow *window);
    void (* delete_file) (RemminaFTPWindow *window);
} RemminaFTPWindowClass;

GType remmina_ftp_window_get_type (void) G_GNUC_CONST;

enum
{
    REMMINA_FTP_FILE_TYPE_DIR,
    REMMINA_FTP_FILE_TYPE_FILE,
    REMMINA_FTP_FILE_N_TYPES,
};

enum
{
    REMMINA_FTP_FILE_COLUMN_TYPE,
    REMMINA_FTP_FILE_COLUMN_NAME,
    REMMINA_FTP_FILE_COLUMN_SIZE,
    REMMINA_FTP_FILE_COLUMN_USER,
    REMMINA_FTP_FILE_COLUMN_GROUP,
    REMMINA_FTP_FILE_COLUMN_PERMISSION,
    REMMINA_FTP_FILE_COLUMN_NAME_SORT, /* Auto populate */
    REMMINA_FTP_FILE_N_COLUMNS
};

enum
{
    REMMINA_FTP_TASK_TYPE_DOWNLOAD,
    REMMINA_FTP_TASK_TYPE_UPLOAD,
    REMMINA_FTP_TASK_N_TYPES
};

enum
{
    REMMINA_FTP_TASK_STATUS_WAIT,
    REMMINA_FTP_TASK_STATUS_RUN,
    REMMINA_FTP_TASK_STATUS_FINISH,
    REMMINA_FTP_TASK_STATUS_ERROR,
    REMMINA_FTP_TASK_N_STATUSES
};

enum
{
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

typedef struct _RemminaFTPTask
{
    /* Read-only */
    gint type;
    gchar *name;
    gint taskid;
    gint tasktype;
    gchar *remotedir;
    gchar *localdir;
    GtkTreeRowReference *rowref;
    /* Updatable */
    gfloat size;
    gint status;
    gfloat donesize;
    gchar *tooltip;
} RemminaFTPTask;

GtkWidget* remmina_ftp_window_new (GtkWindow *parent, const gchar *server_name);

void remmina_ftp_window_clear_file_list (RemminaFTPWindow *window);
/* column, value, ..., -1 */
void remmina_ftp_window_add_file (RemminaFTPWindow *window, ...);
/* Set the current directory. Should be called by opendir signal handler */
void remmina_ftp_window_set_dir (RemminaFTPWindow *window, const gchar *dir);
/* Get the current directory as newly allocated string */
gchar* remmina_ftp_window_get_dir (RemminaFTPWindow *window);
/* Get the next waiting task */
RemminaFTPTask* remmina_ftp_window_get_waiting_task (RemminaFTPWindow *window);
/* Update the task */
void remmina_ftp_window_update_task (RemminaFTPWindow *window, RemminaFTPTask* task);
/* Free the RemminaFTPTask object */
void remmina_ftp_task_free (RemminaFTPTask *task);

G_END_DECLS

#endif  /* __REMMINAFTPWINDOW_H__  */

