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
 
#include "config.h"

#ifdef HAVE_LIBSSH

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <pthread.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include "remminapublic.h"
#include "remminapref.h"
#include "remminassh.h"
#include "remminawidgetpool.h"
#include "remminasftpwindow.h"

G_DEFINE_TYPE (RemminaSFTPWindow, remmina_sftp_window, REMMINA_TYPE_FTP_WINDOW)

static void
remmina_sftp_window_class_init (RemminaSFTPWindowClass *klass)
{
}

#define GET_SFTPATTR_TYPE(a,type) \
    if (a->type == 0) \
    { \
        type = ((a->permissions & 040000) ? REMMINA_FTP_FILE_TYPE_DIR : REMMINA_FTP_FILE_TYPE_FILE); \
    } \
    else \
    { \
        type = (a->type == SSH_FILEXFER_TYPE_DIRECTORY ? REMMINA_FTP_FILE_TYPE_DIR : REMMINA_FTP_FILE_TYPE_FILE); \
    }

/* ------------------------ The Task Thread routines ----------------------------- */

static gboolean remmina_sftp_window_refresh (RemminaSFTPWindow *window);

#define THREAD_CHECK_EXIT \
    (!window->taskid || window->thread_abort)

static gboolean
remmina_sftp_window_thread_update_task (RemminaSFTPWindow *window, RemminaFTPTask *task)
{
    if (THREAD_CHECK_EXIT) return FALSE;

    THREADS_ENTER
    remmina_ftp_window_update_task (REMMINA_FTP_WINDOW (window), task);
    THREADS_LEAVE
    return TRUE;
}

static void
remmina_sftp_window_thread_set_error (RemminaSFTPWindow *window, RemminaFTPTask *task, const gchar *error_format, ...)
{
    va_list args;

    task->status = REMMINA_FTP_TASK_STATUS_ERROR;
    g_free (task->tooltip);
    va_start (args, error_format);
    task->tooltip = g_strdup_vprintf (error_format, args);
    va_end (args);

    remmina_sftp_window_thread_update_task (window, task);
}

static void
remmina_sftp_window_thread_set_finish (RemminaSFTPWindow *window, RemminaFTPTask *task)
{
    task->status = REMMINA_FTP_TASK_STATUS_FINISH;
    g_free (task->tooltip);
    task->tooltip = NULL;

    remmina_sftp_window_thread_update_task (window, task);
}

static RemminaFTPTask*
remmina_sftp_window_thread_get_task (RemminaSFTPWindow *window)
{
    RemminaFTPTask *task;

    if (window->thread_abort) return NULL;

    THREADS_ENTER
    task = remmina_ftp_window_get_waiting_task (REMMINA_FTP_WINDOW (window));
    if (task)
    {
        window->taskid = task->taskid;

        task->status = REMMINA_FTP_TASK_STATUS_RUN;
        remmina_ftp_window_update_task (REMMINA_FTP_WINDOW (window), task);
    }
    THREADS_LEAVE

    return task;
}

static gboolean
remmina_sftp_window_thread_download_file (RemminaSFTPWindow *window, RemminaSFTP *sftp, RemminaFTPTask *task,
    const gchar *remote_path, const gchar *local_path, guint64 *donesize)
{
    sftp_file remote_file;
    FILE *local_file;
    gchar *tmp;
    gchar buf[20000];
    gint len;

    if (THREAD_CHECK_EXIT) return FALSE;

    /* Ensure local dir exists */
    g_strlcpy (buf, local_path, sizeof (buf));
    tmp = g_strrstr (buf, "/");
    if (tmp && tmp != buf)
    {
        *tmp = '\0';
        if (g_mkdir_with_parents (buf, 0755) < 0)
        {
            remmina_sftp_window_thread_set_error (window, task, _("Error creating directory %s."), buf);
            return FALSE;
        }
    }

    tmp = remmina_ssh_unconvert (REMMINA_SSH (sftp), remote_path);
    remote_file = sftp_open (sftp->sftp_sess, tmp, O_RDONLY, 0);
    g_free (tmp);

    if (!remote_file)
    {
        remmina_sftp_window_thread_set_error (window, task, _("Error opening file %s on server. %s"),
            remote_path, ssh_get_error (REMMINA_SSH (window->sftp)->session));
        return FALSE;
    }
    local_file = g_fopen (local_path, "wb");
    if (!local_file)
    {
        sftp_close (remote_file);
        remmina_sftp_window_thread_set_error (window, task, _("Error creating file %s."), local_path);
        return FALSE;
    }

    while (!THREAD_CHECK_EXIT && (len = sftp_read (remote_file, buf, sizeof (buf))) > 0)
    {
        if (THREAD_CHECK_EXIT) break;

        if (fwrite (buf, 1, len, local_file) < len)
        {
            sftp_close (remote_file);
            fclose (local_file);
            remmina_sftp_window_thread_set_error (window, task, _("Error writing file %s."), local_path);
            return FALSE;
        }

        *donesize += (guint64) len;
        task->donesize = (gfloat) (*donesize);

        if (!remmina_sftp_window_thread_update_task (window, task)) break;
    }

    sftp_close (remote_file);
    fclose (local_file);
    return TRUE;
}

static gboolean
remmina_sftp_window_thread_recursive_dir (RemminaSFTPWindow *window, RemminaSFTP *sftp, RemminaFTPTask *task,
    const gchar *rootdir_path, const gchar *subdir_path, GPtrArray *array)
{
    sftp_dir sftpdir;
    sftp_attributes sftpattr;
    gchar *tmp;
    gchar *dir_path;
    gchar *file_path;
    gint type;
    gboolean ret = TRUE;

    if (THREAD_CHECK_EXIT) return FALSE;

    if (subdir_path)
    {
        dir_path = remmina_public_combine_path (rootdir_path, subdir_path);
    }
    else
    {
        dir_path = g_strdup (rootdir_path);
    }
    tmp = remmina_ssh_unconvert (REMMINA_SSH (sftp), dir_path);
    sftpdir = sftp_opendir (sftp->sftp_sess, tmp);
    g_free (tmp);

    if (!sftpdir)
    {
        remmina_sftp_window_thread_set_error (window, task, _("Error opening directory %s. %s"),
            dir_path, ssh_get_error (REMMINA_SSH (window->sftp)->session));
        g_free (dir_path);
        return FALSE;
    }

    g_free (dir_path);

    while ((sftpattr = sftp_readdir (sftp->sftp_sess, sftpdir)))
    {
        if (g_strcmp0 (sftpattr->name, ".") != 0 &&
            g_strcmp0 (sftpattr->name, "..") != 0)
        {
            GET_SFTPATTR_TYPE (sftpattr, type);

            tmp = remmina_ssh_convert (REMMINA_SSH (sftp), sftpattr->name);
            if (subdir_path)
            {
                file_path = remmina_public_combine_path (subdir_path, tmp);
                g_free (tmp);
            }
            else
            {
                file_path = tmp;
            }

            if (type == REMMINA_FTP_FILE_TYPE_DIR)
            {
                ret = remmina_sftp_window_thread_recursive_dir (window, sftp, task, rootdir_path, file_path, array);
                g_free (file_path);
                if (!ret)
                {
                    sftp_attributes_free (sftpattr);
                    break;
                }
            }
            else
            {
                task->size += (gfloat) sftpattr->size;
                g_ptr_array_add (array, file_path);

                if (!remmina_sftp_window_thread_update_task (window, task))
                {
                    sftp_attributes_free (sftpattr);
                    break;
                }
            }
        }
        sftp_attributes_free (sftpattr);

        if (THREAD_CHECK_EXIT) break;
    }

    sftp_closedir (sftpdir);
    return ret;
}

static gboolean
remmina_sftp_window_thread_upload_file (RemminaSFTPWindow *window, RemminaSFTP *sftp, RemminaFTPTask *task,
    const gchar *remote_path, const gchar *local_path, guint64 *donesize)
{
    sftp_file remote_file;
    FILE *local_file;
    gchar *tmp;
    gchar buf[20000];
    gint len;

    if (THREAD_CHECK_EXIT) return FALSE;

    tmp = remmina_ssh_unconvert (REMMINA_SSH (sftp), remote_path);
    remote_file = sftp_open (sftp->sftp_sess, tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    g_free (tmp);

    if (!remote_file)
    {
        remmina_sftp_window_thread_set_error (window, task, _("Error creating file %s on server. %s"),
            remote_path, ssh_get_error (REMMINA_SSH (window->sftp)->session));
        return FALSE;
    }
    local_file = g_fopen (local_path, "rb");
    if (!local_file)
    {
        sftp_close (remote_file);
        remmina_sftp_window_thread_set_error (window, task, _("Error opening file %s."), local_path);
        return FALSE;
    }

    while (!THREAD_CHECK_EXIT && (len = fread (buf, 1, sizeof (buf), local_file)) > 0)
    {
        if (THREAD_CHECK_EXIT) break;

        if (sftp_write (remote_file, buf, len) < len)
        {
            sftp_close (remote_file);
            fclose (local_file);
            remmina_sftp_window_thread_set_error (window, task, _("Error writing file %s on server. %s"),
                remote_path, ssh_get_error (REMMINA_SSH (window->sftp)->session));
            return FALSE;
        }

        *donesize += (guint64) len;
        task->donesize = (gfloat) (*donesize);

        if (!remmina_sftp_window_thread_update_task (window, task)) break;
    }

    sftp_close (remote_file);
    fclose (local_file);
    return TRUE;
}

static gpointer
remmina_sftp_window_thread_main (gpointer data)
{
    RemminaSFTPWindow *window = REMMINA_SFTP_WINDOW (data);
    RemminaSFTP *sftp = NULL;
    RemminaFTPTask *task;
    gchar *remote, *local;
    guint64 size;
    GPtrArray *array;
    gint i;
    gchar *remote_file, *local_file;
    gboolean ret;
    gchar *refreshdir = NULL;
    gchar *tmp;
    gboolean refresh = FALSE;

    task = remmina_sftp_window_thread_get_task (window);
    while (task)
    {
        size = 0;
        if (!sftp)
        {
            sftp = remmina_sftp_new_from_ssh (REMMINA_SSH (window->sftp));
            if (!remmina_ssh_init_session (REMMINA_SSH (sftp)) ||
                remmina_ssh_auth (REMMINA_SSH (sftp), NULL) <= 0 ||
                !remmina_sftp_open (sftp))
            {
                remmina_sftp_window_thread_set_error (window, task, (REMMINA_SSH (sftp))->error);
                remmina_ftp_task_free (task);
                break;
            }
        }

        remote = remmina_public_combine_path (task->remotedir, task->name);
        local = remmina_public_combine_path (task->localdir, task->name);

        switch (task->tasktype)
        {
        case REMMINA_FTP_TASK_TYPE_DOWNLOAD:
            switch (task->type)
            {
            case REMMINA_FTP_FILE_TYPE_FILE:
                if (remmina_sftp_window_thread_download_file (window, sftp, task,
                    remote, local, &size))
                {
                    remmina_sftp_window_thread_set_finish (window, task);
                }
                break;

            case REMMINA_FTP_FILE_TYPE_DIR:
                array = g_ptr_array_new ();
                ret = remmina_sftp_window_thread_recursive_dir (window, sftp, task, remote, NULL, array);
                if (ret)
                {
                    for (i = 0; i < array->len; i++)
                    {
                        if (THREAD_CHECK_EXIT)
                        {
                            ret = FALSE;
                            break;
                        }
                        remote_file = remmina_public_combine_path (remote, (gchar*) g_ptr_array_index (array, i));
                        local_file = remmina_public_combine_path (local, (gchar*) g_ptr_array_index (array, i));
                        ret = remmina_sftp_window_thread_download_file (window, sftp, task,
                            remote_file, local_file, &size);
                        g_free (remote_file);
                        g_free (local_file);
                        if (!ret) break;
                    }
                }
                g_ptr_array_free (array, TRUE);
                if (ret)
                {
                    remmina_sftp_window_thread_set_finish (window, task);
                }
                break;
            }
            break;

        case REMMINA_FTP_TASK_TYPE_UPLOAD:
            if (remmina_sftp_window_thread_upload_file (window, sftp, task,
                remote, local, &size))
            {
                remmina_sftp_window_thread_set_finish (window, task);
                tmp = remmina_ftp_window_get_dir (REMMINA_FTP_WINDOW (window));
                if (g_strcmp0 (tmp, task->remotedir) == 0)
                {
                    refresh = TRUE;
                    g_free (refreshdir);
                    refreshdir = tmp;
                }
                else
                {
                    g_free (tmp);
                }
            }
            break;
        }

        g_free (remote);
        g_free (local);

        remmina_ftp_task_free (task);
        window->taskid = 0;

        if (window->thread_abort) break;

        task = remmina_sftp_window_thread_get_task (window);
    }

    if (sftp)
    {
        remmina_sftp_free (sftp);
    }

    if (!window->thread_abort && refresh)
    {
        tmp = remmina_ftp_window_get_dir (REMMINA_FTP_WINDOW (window));
        if (g_strcmp0 (tmp, refreshdir) == 0)
        {
            IDLE_ADD ((GSourceFunc) remmina_sftp_window_refresh, window);
        }
        g_free (tmp);
    }
    g_free (refreshdir);
    window->thread = 0;

    return NULL;
}

/* ------------------------ The SFTP Window routines ----------------------------- */

static gboolean
remmina_sftp_window_confirm_disconnect (RemminaSFTPWindow *window)
{
    GtkWidget *dialog;
    gint ret;

    if (!window->thread) return TRUE;

    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("File transfer currently in progress.\nAre you sure to quit?"));
    ret = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (ret == GTK_RESPONSE_YES) return TRUE;
    return FALSE;
}

static gboolean
remmina_sftp_window_on_delete_event (RemminaSFTPWindow *window, GdkEvent *event, gpointer data)
{
    return !remmina_sftp_window_confirm_disconnect (window);
}

static void
remmina_sftp_window_on_disconnect (RemminaSFTPWindow *window, gpointer data)
{
    if (remmina_sftp_window_confirm_disconnect (window))
    {
        gtk_widget_destroy (GTK_WIDGET (window));
    }
}

static void
remmina_sftp_window_destroy (RemminaSFTPWindow *window, gpointer data)
{
    if (window->sftp)
    {
        remmina_sftp_free (window->sftp);
        window->sftp = NULL;
    }
    window->thread_abort = TRUE;
    /* We will wait for the thread to quit itself, and hopefully the thread is handling things correctly */
    if (window->thread) pthread_join (window->thread, NULL);
}

static sftp_dir
remmina_sftp_window_sftp_session_opendir (RemminaSFTPWindow *window, const gchar *dir)
{
    sftp_dir sftpdir;
    GtkWidget *dialog;

    sftpdir = sftp_opendir (window->sftp->sftp_sess, (gchar*) dir);
    if (!sftpdir)
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("Failed to open directory %s. %s"), dir,
            ssh_get_error (REMMINA_SSH (window->sftp)->session));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return NULL;
    }
    return sftpdir;
}

static gboolean
remmina_sftp_window_sftp_session_closedir (RemminaSFTPWindow *window, sftp_dir sftpdir)
{
    GtkWidget *dialog;

    if (!sftp_dir_eof (sftpdir))
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("Failed reading directory. %s"), ssh_get_error (REMMINA_SSH (window->sftp)->session));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return FALSE;
    }
    sftp_closedir (sftpdir);
    return TRUE;
}

static void
remmina_sftp_window_on_opendir (RemminaSFTPWindow *window, gchar *dir, gpointer data)
{
    sftp_dir sftpdir;
    sftp_attributes sftpattr;
    GtkWidget *dialog;
    gchar *newdir;
    gchar *newdir_conv;
    gchar *tmp;
    gint type;

    if (!dir || dir[0] == '\0')
    {
        newdir = g_strdup (".");
    }
    else if (dir[0] == '/')
    {
        newdir = g_strdup (dir);
    }
    else
    {
        tmp = remmina_ftp_window_get_dir (REMMINA_FTP_WINDOW (window));
        if (tmp)
        {
            newdir = remmina_public_combine_path (tmp, dir);
            g_free (tmp);
        }
        else
        {
            newdir = g_strdup_printf ("./%s", dir);
        }
    }

    tmp = remmina_ssh_unconvert (REMMINA_SSH (window->sftp), newdir);
    newdir_conv = sftp_canonicalize_path (window->sftp->sftp_sess, tmp);
    g_free (tmp);
    g_free (newdir);
    newdir = remmina_ssh_convert (REMMINA_SSH (window->sftp), newdir_conv);
    if (!newdir)
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("Failed to open directory %s. %s"), dir,
            ssh_get_error (REMMINA_SSH (window->sftp)->session));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_free (newdir_conv);
        return;
    }

    sftpdir = remmina_sftp_window_sftp_session_opendir (window, newdir_conv);
    g_free (newdir_conv);
    if (!sftpdir)
    {
        g_free (newdir);
        return;
    }

    remmina_ftp_window_clear_file_list (REMMINA_FTP_WINDOW (window));

    while ((sftpattr = sftp_readdir (window->sftp->sftp_sess, sftpdir)))
    {
        if (g_strcmp0 (sftpattr->name, ".") != 0 &&
            g_strcmp0 (sftpattr->name, "..") != 0)
        {
            GET_SFTPATTR_TYPE (sftpattr, type);

            tmp = remmina_ssh_convert (REMMINA_SSH (window->sftp), sftpattr->name);
            remmina_ftp_window_add_file (REMMINA_FTP_WINDOW (window),
                REMMINA_FTP_FILE_COLUMN_TYPE, type,
                REMMINA_FTP_FILE_COLUMN_NAME, tmp,
                REMMINA_FTP_FILE_COLUMN_SIZE, (gfloat) sftpattr->size,
                REMMINA_FTP_FILE_COLUMN_USER, sftpattr->owner,
                REMMINA_FTP_FILE_COLUMN_GROUP, sftpattr->group,
                REMMINA_FTP_FILE_COLUMN_PERMISSION, sftpattr->permissions,
                -1);
            g_free (tmp);
        }
        sftp_attributes_free (sftpattr);
    }
    remmina_sftp_window_sftp_session_closedir (window, sftpdir);

    remmina_ftp_window_set_dir (REMMINA_FTP_WINDOW (window), newdir);
    g_free (newdir);
}

static void
remmina_sftp_window_on_newtask (RemminaSFTPWindow *window, gpointer data)
{
    if (window->thread) return;

    if (pthread_create (&window->thread, NULL, remmina_sftp_window_thread_main, window))
    {
        window->thread = 0;
    }
}

static gboolean
remmina_sftp_window_on_canceltask (RemminaSFTPWindow *window, gint taskid, gpointer data)
{
    GtkWidget *dialog;
    gint ret;

    if (window->taskid != taskid) return TRUE;

    dialog = gtk_message_dialog_new (GTK_WINDOW (window),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("File transfer currently in progress.\nAre you sure to cancel it?"));
    ret = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (ret == GTK_RESPONSE_YES)
    {
        /* Make sure we are still handling the same task before we clear the flag */
        if (window->taskid == taskid) window->taskid = 0;
        return TRUE;
    }
    return FALSE;
}

static gboolean
remmina_sftp_window_on_deletefile (RemminaSFTPWindow *window, gint type, gchar *name, gpointer data)
{
    GtkWidget *dialog;
    gint ret = 0;
    gchar *tmp;

    tmp = remmina_ssh_unconvert (REMMINA_SSH (window->sftp), name);
    switch (type)
    {
    case REMMINA_FTP_FILE_TYPE_DIR:
        ret = sftp_rmdir (window->sftp->sftp_sess, tmp);
        break;

    case REMMINA_FTP_FILE_TYPE_FILE:
        ret = sftp_unlink (window->sftp->sftp_sess, tmp);
        break;
    }
    g_free (tmp);

    if (ret != 0)
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("Failed to delete '%s'. %s"),
            name, ssh_get_error (REMMINA_SSH (window->sftp)->session));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return FALSE;
    }
    return TRUE;
}

static void
remmina_sftp_window_init (RemminaSFTPWindow *window)
{
    window->sftp = NULL;
    window->thread = 0;
    window->taskid = 0;
    window->thread_abort = FALSE;

    /* Setup the internal signals */
    g_signal_connect (G_OBJECT (window), "delete-event",
        G_CALLBACK (remmina_sftp_window_on_delete_event), NULL);
    g_signal_connect (G_OBJECT (window), "disconnect",
        G_CALLBACK (remmina_sftp_window_on_disconnect), NULL);
    g_signal_connect (G_OBJECT (window), "destroy",
        G_CALLBACK (remmina_sftp_window_destroy), NULL);
    g_signal_connect (G_OBJECT (window), "open-dir",
        G_CALLBACK (remmina_sftp_window_on_opendir), NULL);
    g_signal_connect (G_OBJECT (window), "new-task",
        G_CALLBACK (remmina_sftp_window_on_newtask), NULL);
    g_signal_connect (G_OBJECT (window), "cancel-task",
        G_CALLBACK (remmina_sftp_window_on_canceltask), NULL);
    g_signal_connect (G_OBJECT (window), "delete-file",
        G_CALLBACK (remmina_sftp_window_on_deletefile), NULL);

    remmina_widget_pool_register (GTK_WIDGET (window));
}

static gboolean
remmina_sftp_window_refresh (RemminaSFTPWindow *window)
{
    gdk_window_set_cursor (GTK_WIDGET (window)->window, gdk_cursor_new (GDK_WATCH));
    gdk_flush ();

    remmina_sftp_window_on_opendir (window, ".", NULL);

    gdk_window_set_cursor (GTK_WIDGET (window)->window, NULL);

    return FALSE;
}

static GtkWidget*
remmina_sftp_window_create_window (RemminaSFTP *sftp)
{
    RemminaSFTPWindow *window;
    gchar buf[200];

    window = REMMINA_SFTP_WINDOW (g_object_new (REMMINA_TYPE_SFTP_WINDOW, NULL));

    window->sftp = sftp;

    /* Title */
    g_snprintf (buf, sizeof (buf), _("Secure File Transfer: %s"), REMMINA_SSH (sftp)->server);
    gtk_window_set_title (GTK_WINDOW (window), buf);

    gtk_widget_show (GTK_WIDGET (window));
    gtk_window_present (GTK_WINDOW (window));

    return GTK_WIDGET (window);
}

GtkWidget*
remmina_sftp_window_new (RemminaSFTP *sftp)
{
    GtkWidget *window;

    window = remmina_sftp_window_create_window (sftp);
    g_idle_add ((GSourceFunc) remmina_sftp_window_refresh, window);
    return window;
}

GtkWidget*
remmina_sftp_window_new_init (RemminaSFTP *sftp)
{
    GtkWidget *window;
    GtkWidget *dialog;

    window = remmina_sftp_window_create_window (sftp);

    gdk_window_set_cursor (GTK_WIDGET (window)->window, gdk_cursor_new (GDK_WATCH));
    gdk_flush ();

    if (!remmina_ssh_init_session (REMMINA_SSH (sftp)) ||
        remmina_ssh_auth (REMMINA_SSH (sftp), NULL) <= 0 ||
        !remmina_sftp_open (sftp))
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (window),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            (REMMINA_SSH (sftp))->error, NULL);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        gtk_widget_destroy (window);
        return NULL;
    }

    gdk_window_set_cursor (GTK_WIDGET (window)->window, NULL);

    g_idle_add ((GSourceFunc) remmina_sftp_window_refresh, window);
    return window;
}

typedef struct _RemminaSFTPWindowInitParam
{
    RemminaFile *remmina_file;
    GtkWidget *init_dialog;
    pthread_t thread;
    gchar *error;
    RemminaSFTP *sftp;
} RemminaSFTPWindowInitParam;

static void
remmina_sftp_window_init_cancel (RemminaInitDialog *dialog, gint response_id, RemminaSFTPWindowInitParam *param)
{
    if ((response_id == GTK_RESPONSE_CANCEL || response_id == GTK_RESPONSE_DELETE_EVENT)
        && dialog->mode == REMMINA_INIT_MODE_CONNECTING)
    {
        gtk_widget_destroy (GTK_WIDGET (dialog));
    }
}

static void
remmina_sftp_window_init_destroy (RemminaInitDialog *dialog, RemminaSFTPWindowInitParam *param)
{
    GtkWidget *msg_dialog;

    if (param->error)
    {
        msg_dialog = gtk_message_dialog_new (NULL,
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            param->error, NULL);
        gtk_dialog_run (GTK_DIALOG (msg_dialog));
        gtk_widget_destroy (msg_dialog);

        g_free (param->error);
    }
    if (param->thread)
    {
        pthread_cancel (param->thread);
        if (param->thread) pthread_join (param->thread, NULL);
    }
    if (param->sftp)
    {
        remmina_sftp_free (param->sftp);
    }
    if (param->remmina_file)
    {
        remmina_file_free (param->remmina_file);
    }
    g_free (param);
}

static gboolean
remmina_sftp_window_open_thread_quit (RemminaSFTPWindowInitParam *param)
{
    gtk_widget_destroy (param->init_dialog);
    return FALSE;
}

static gboolean
remmina_sftp_window_open_thread_start (RemminaSFTPWindowInitParam *param)
{
    RemminaSFTP *sftp = param->sftp;

    /* Hand over the sftp session pointer from the initial param to the sftp window */
    param->sftp = NULL;

    /* Remember recent list for quick connect */
    if (param->remmina_file->filename == NULL)
    {
        remmina_pref_add_recent (param->remmina_file->protocol, param->remmina_file->server);
    }

    gtk_widget_destroy (param->init_dialog);

    remmina_sftp_window_new (sftp);
    return FALSE;
}

static gpointer
remmina_sftp_window_open_thread (gpointer data)
{
    RemminaSFTPWindowInitParam *param = (RemminaSFTPWindowInitParam*) data;
    gint ret;

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    CANCEL_ASYNC

    param->sftp = remmina_sftp_new_from_file (param->remmina_file);

    if (!remmina_ssh_init_session (REMMINA_SSH (param->sftp)))
    {
        param->error = g_strdup (REMMINA_SSH (param->sftp)->error);
        IDLE_ADD ((GSourceFunc) remmina_sftp_window_open_thread_quit, data);
        return NULL;
    }

    ret = remmina_ssh_auth_gui (REMMINA_SSH (param->sftp), REMMINA_INIT_DIALOG (param->init_dialog), TRUE);
    if (ret <= 0)
    {
        if (ret == 0)
        {
            param->error = g_strdup (REMMINA_SSH (param->sftp)->error);
        }
        IDLE_ADD ((GSourceFunc) remmina_sftp_window_open_thread_quit, data);
        return NULL;
    }

    if (!remmina_sftp_open (param->sftp))
    {
        param->error = g_strdup (REMMINA_SSH (param->sftp)->error);
        IDLE_ADD ((GSourceFunc) remmina_sftp_window_open_thread_quit, data);
        return NULL;
    }

    /* We are done initializing. Unregister the thread from the param struct and open up the sftp window */
    param->thread = 0;

    IDLE_ADD ((GSourceFunc) remmina_sftp_window_open_thread_start, data);
    return NULL;
}

void
remmina_sftp_window_open (RemminaFile *remminafile)
{
    RemminaSFTPWindowInitParam *param;

    param = g_new (RemminaSFTPWindowInitParam, 1);
    param->remmina_file = remminafile;
    param->thread = 0;
    param->error = NULL;
    param->sftp = NULL;

    param->init_dialog = remmina_init_dialog_new (_("Connecting to SFTP server '%s'..."),
        (remminafile->filename ? remminafile->name : remminafile->server));
    g_signal_connect (G_OBJECT (param->init_dialog), "response",
        G_CALLBACK (remmina_sftp_window_init_cancel), param);
    g_signal_connect (G_OBJECT (param->init_dialog), "destroy",
        G_CALLBACK (remmina_sftp_window_init_destroy), param);
    gtk_widget_show (param->init_dialog);

    g_free (remminafile->ssh_server);
    remminafile->ssh_server = g_strdup (remminafile->server);

    if (pthread_create (&param->thread, NULL, remmina_sftp_window_open_thread, param))
    {
        param->thread = 0;
        param->error = g_strdup ("Failed to initialize pthread.");
        gtk_widget_destroy (param->init_dialog);
    }
}

#endif

