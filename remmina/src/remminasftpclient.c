/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee 
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
 
#define _FILE_OFFSET_BITS 64
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
#include "remminasftpclient.h"

G_DEFINE_TYPE (RemminaSFTPClient, remmina_sftp_client, REMMINA_TYPE_FTP_CLIENT)

static void
remmina_sftp_client_class_init (RemminaSFTPClientClass *klass)
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

static gboolean remmina_sftp_client_refresh (RemminaSFTPClient *client);

#define THREAD_CHECK_EXIT \
    (!client->taskid || client->thread_abort)

static gboolean
remmina_sftp_client_thread_update_task (RemminaSFTPClient *client, RemminaFTPTask *task)
{
    if (THREAD_CHECK_EXIT) return FALSE;

    THREADS_ENTER
    remmina_ftp_client_update_task (REMMINA_FTP_CLIENT (client), task);
    THREADS_LEAVE
    return TRUE;
}

static void
remmina_sftp_client_thread_set_error (RemminaSFTPClient *client, RemminaFTPTask *task, const gchar *error_format, ...)
{
    va_list args;

    task->status = REMMINA_FTP_TASK_STATUS_ERROR;
    g_free (task->tooltip);
    va_start (args, error_format);
    task->tooltip = g_strdup_vprintf (error_format, args);
    va_end (args);

    remmina_sftp_client_thread_update_task (client, task);
}

static void
remmina_sftp_client_thread_set_finish (RemminaSFTPClient *client, RemminaFTPTask *task)
{
    task->status = REMMINA_FTP_TASK_STATUS_FINISH;
    g_free (task->tooltip);
    task->tooltip = NULL;

    remmina_sftp_client_thread_update_task (client, task);
}

static RemminaFTPTask*
remmina_sftp_client_thread_get_task (RemminaSFTPClient *client)
{
    RemminaFTPTask *task;

    if (client->thread_abort) return NULL;

    THREADS_ENTER
    task = remmina_ftp_client_get_waiting_task (REMMINA_FTP_CLIENT (client));
    if (task)
    {
        client->taskid = task->taskid;

        task->status = REMMINA_FTP_TASK_STATUS_RUN;
        remmina_ftp_client_update_task (REMMINA_FTP_CLIENT (client), task);
    }
    THREADS_LEAVE

    return task;
}

static gboolean
remmina_sftp_client_thread_download_file (RemminaSFTPClient *client, RemminaSFTP *sftp, RemminaFTPTask *task,
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
            remmina_sftp_client_thread_set_error (client, task, _("Error creating directory %s."), buf);
            return FALSE;
        }
    }

    tmp = remmina_ssh_unconvert (REMMINA_SSH (sftp), remote_path);
    remote_file = sftp_open (sftp->sftp_sess, tmp, O_RDONLY, 0);
    g_free (tmp);

    if (!remote_file)
    {
        remmina_sftp_client_thread_set_error (client, task, _("Error opening file %s on server. %s"),
            remote_path, ssh_get_error (REMMINA_SSH (client->sftp)->session));
        return FALSE;
    }
    local_file = g_fopen (local_path, "wb");
    if (!local_file)
    {
        sftp_close (remote_file);
        remmina_sftp_client_thread_set_error (client, task, _("Error creating file %s."), local_path);
        return FALSE;
    }

    while (!THREAD_CHECK_EXIT && (len = sftp_read (remote_file, buf, sizeof (buf))) > 0)
    {
        if (THREAD_CHECK_EXIT) break;

        if (fwrite (buf, 1, len, local_file) < len)
        {
            sftp_close (remote_file);
            fclose (local_file);
            remmina_sftp_client_thread_set_error (client, task, _("Error writing file %s."), local_path);
            return FALSE;
        }

        *donesize += (guint64) len;
        task->donesize = (gfloat) (*donesize);

        if (!remmina_sftp_client_thread_update_task (client, task)) break;
    }

    sftp_close (remote_file);
    fclose (local_file);
    return TRUE;
}

static gboolean
remmina_sftp_client_thread_recursive_dir (RemminaSFTPClient *client, RemminaSFTP *sftp, RemminaFTPTask *task,
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
        remmina_sftp_client_thread_set_error (client, task, _("Error opening directory %s. %s"),
            dir_path, ssh_get_error (REMMINA_SSH (client->sftp)->session));
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
                ret = remmina_sftp_client_thread_recursive_dir (client, sftp, task, rootdir_path, file_path, array);
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

                if (!remmina_sftp_client_thread_update_task (client, task))
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
remmina_sftp_client_thread_recursive_localdir (RemminaSFTPClient *client, RemminaFTPTask *task,
    const gchar *rootdir_path, const gchar *subdir_path, GPtrArray *array)
{
    GDir *dir;
    gchar *path;
    const gchar *name;
    gchar *relpath;
    gchar *abspath;
    struct stat st;
    gboolean ret = TRUE;

    path = g_build_filename (rootdir_path, subdir_path, NULL);
    dir = g_dir_open (path, 0, NULL);
    if (dir == NULL)
    {
        g_free (path);
        return FALSE;
    }
    while ((name = g_dir_read_name (dir)) != NULL)
    {
        if (THREAD_CHECK_EXIT)
        {
            ret = FALSE;
            break;
        }
        if (g_strcmp0 (name, ".") == 0 || g_strcmp0 (name, "..") == 0) continue;
        abspath = g_build_filename (path, name, NULL);
        if (g_stat (abspath, &st) < 0)
        {
            g_free (abspath);
            continue;
        }
        relpath = g_build_filename (subdir_path ? subdir_path : "", name, NULL);
        g_ptr_array_add (array, relpath);
        if (g_file_test (abspath, G_FILE_TEST_IS_DIR))
        {
            ret = remmina_sftp_client_thread_recursive_localdir (client, task, rootdir_path, relpath, array);
            if (!ret) break;
        }
        else
        {
            task->size += (gfloat) st.st_size;
        }
        g_free (abspath);
    }
    g_free (path);
    g_dir_close (dir);
    return ret;
}

static gboolean
remmina_sftp_client_thread_mkdir (RemminaSFTPClient *client, RemminaSFTP *sftp, RemminaFTPTask *task, const gchar *path)
{
    sftp_attributes sftpattr;

    sftpattr = sftp_stat (sftp->sftp_sess, path);
    if (sftpattr != NULL)
    {
        sftp_attributes_free (sftpattr);
        return TRUE;
    }
    if (sftp_mkdir (sftp->sftp_sess, path, 0755) < 0)
    {
        remmina_sftp_client_thread_set_error (client, task, _("Error creating folder %s on server. %s"),
            path, ssh_get_error (REMMINA_SSH (client->sftp)->session));
        return FALSE;
    }
    return TRUE;
}

static gboolean
remmina_sftp_client_thread_upload_file (RemminaSFTPClient *client, RemminaSFTP *sftp, RemminaFTPTask *task,
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
        remmina_sftp_client_thread_set_error (client, task, _("Error creating file %s on server. %s"),
            remote_path, ssh_get_error (REMMINA_SSH (client->sftp)->session));
        return FALSE;
    }
    local_file = g_fopen (local_path, "rb");
    if (!local_file)
    {
        sftp_close (remote_file);
        remmina_sftp_client_thread_set_error (client, task, _("Error opening file %s."), local_path);
        return FALSE;
    }

    while (!THREAD_CHECK_EXIT && (len = fread (buf, 1, sizeof (buf), local_file)) > 0)
    {
        if (THREAD_CHECK_EXIT) break;

        if (sftp_write (remote_file, buf, len) < len)
        {
            sftp_close (remote_file);
            fclose (local_file);
            remmina_sftp_client_thread_set_error (client, task, _("Error writing file %s on server. %s"),
                remote_path, ssh_get_error (REMMINA_SSH (client->sftp)->session));
            return FALSE;
        }

        *donesize += (guint64) len;
        task->donesize = (gfloat) (*donesize);

        if (!remmina_sftp_client_thread_update_task (client, task)) break;
    }

    sftp_close (remote_file);
    fclose (local_file);
    return TRUE;
}

static gpointer
remmina_sftp_client_thread_main (gpointer data)
{
    RemminaSFTPClient *client = REMMINA_SFTP_CLIENT (data);
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

    task = remmina_sftp_client_thread_get_task (client);
    while (task)
    {
        size = 0;
        if (!sftp)
        {
            sftp = remmina_sftp_new_from_ssh (REMMINA_SSH (client->sftp));
            if (!remmina_ssh_init_session (REMMINA_SSH (sftp)) ||
                remmina_ssh_auth (REMMINA_SSH (sftp), NULL) <= 0 ||
                !remmina_sftp_open (sftp))
            {
                remmina_sftp_client_thread_set_error (client, task, (REMMINA_SSH (sftp))->error);
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
                ret = remmina_sftp_client_thread_download_file (client, sftp, task,
                    remote, local, &size);
                break;

            case REMMINA_FTP_FILE_TYPE_DIR:
                array = g_ptr_array_new ();
                ret = remmina_sftp_client_thread_recursive_dir (client, sftp, task, remote, NULL, array);
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
                        ret = remmina_sftp_client_thread_download_file (client, sftp, task,
                            remote_file, local_file, &size);
                        g_free (remote_file);
                        g_free (local_file);
                        if (!ret) break;
                    }
                }
                g_ptr_array_foreach (array, (GFunc) g_free, NULL);
                g_ptr_array_free (array, TRUE);
                break;

            default:
                ret = 0;
                break;
            }
            if (ret)
            {
                remmina_sftp_client_thread_set_finish (client, task);
            }
            break;

        case REMMINA_FTP_TASK_TYPE_UPLOAD:
            switch (task->type)
            {
            case REMMINA_FTP_FILE_TYPE_FILE:
                ret = remmina_sftp_client_thread_upload_file (client, sftp, task,
                    remote, local, &size);
                break;

            case REMMINA_FTP_FILE_TYPE_DIR:
                ret = remmina_sftp_client_thread_mkdir (client, sftp, task, remote);
                if (!ret) break;
                array = g_ptr_array_new ();
                ret = remmina_sftp_client_thread_recursive_localdir (client, task, local, NULL, array);
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
                        local_file = g_build_filename (local, (gchar*) g_ptr_array_index (array, i), NULL);
                        if (g_file_test (local_file, G_FILE_TEST_IS_DIR))
                        {
                            ret = remmina_sftp_client_thread_mkdir (client, sftp, task, remote_file);
                        }
                        else
                        {
                            ret = remmina_sftp_client_thread_upload_file (client, sftp, task,
                                remote_file, local_file, &size);
                        }
                        g_free (remote_file);
                        g_free (local_file);
                        if (!ret) break;
                    }
                }
                g_ptr_array_foreach (array, (GFunc) g_free, NULL);
                g_ptr_array_free (array, TRUE);
                break;

            default:
                ret = 0;
                break;
            }
            if (ret)
            {
                remmina_sftp_client_thread_set_finish (client, task);
                tmp = remmina_ftp_client_get_dir (REMMINA_FTP_CLIENT (client));
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
        client->taskid = 0;

        if (client->thread_abort) break;

        task = remmina_sftp_client_thread_get_task (client);
    }

    if (sftp)
    {
        remmina_sftp_free (sftp);
    }

    if (!client->thread_abort && refresh)
    {
        tmp = remmina_ftp_client_get_dir (REMMINA_FTP_CLIENT (client));
        if (g_strcmp0 (tmp, refreshdir) == 0)
        {
            IDLE_ADD ((GSourceFunc) remmina_sftp_client_refresh, client);
        }
        g_free (tmp);
    }
    g_free (refreshdir);
    client->thread = 0;

    return NULL;
}

/* ------------------------ The SFTP Client routines ----------------------------- */

static void
remmina_sftp_client_destroy (RemminaSFTPClient *client, gpointer data)
{
    if (client->sftp)
    {
        remmina_sftp_free (client->sftp);
        client->sftp = NULL;
    }
    client->thread_abort = TRUE;
    /* We will wait for the thread to quit itself, and hopefully the thread is handling things correctly */
    while (client->thread)
    {
        gdk_threads_leave ();
        sleep (1);
        gdk_threads_enter ();
    }
}

static sftp_dir
remmina_sftp_client_sftp_session_opendir (RemminaSFTPClient *client, const gchar *dir)
{
    sftp_dir sftpdir;
    GtkWidget *dialog;

    sftpdir = sftp_opendir (client->sftp->sftp_sess, (gchar*) dir);
    if (!sftpdir)
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (client))),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("Failed to open directory %s. %s"), dir,
            ssh_get_error (REMMINA_SSH (client->sftp)->session));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return NULL;
    }
    return sftpdir;
}

static gboolean
remmina_sftp_client_sftp_session_closedir (RemminaSFTPClient *client, sftp_dir sftpdir)
{
    GtkWidget *dialog;

    if (!sftp_dir_eof (sftpdir))
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (client))),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("Failed reading directory. %s"), ssh_get_error (REMMINA_SSH (client->sftp)->session));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return FALSE;
    }
    sftp_closedir (sftpdir);
    return TRUE;
}

static void
remmina_sftp_client_on_opendir (RemminaSFTPClient *client, gchar *dir, gpointer data)
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
        tmp = remmina_ftp_client_get_dir (REMMINA_FTP_CLIENT (client));
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

    tmp = remmina_ssh_unconvert (REMMINA_SSH (client->sftp), newdir);
    newdir_conv = sftp_canonicalize_path (client->sftp->sftp_sess, tmp);
    g_free (tmp);
    g_free (newdir);
    newdir = remmina_ssh_convert (REMMINA_SSH (client->sftp), newdir_conv);
    if (!newdir)
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (client))),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("Failed to open directory %s. %s"), dir,
            ssh_get_error (REMMINA_SSH (client->sftp)->session));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        g_free (newdir_conv);
        return;
    }

    sftpdir = remmina_sftp_client_sftp_session_opendir (client, newdir_conv);
    g_free (newdir_conv);
    if (!sftpdir)
    {
        g_free (newdir);
        return;
    }

    remmina_ftp_client_clear_file_list (REMMINA_FTP_CLIENT (client));

    while ((sftpattr = sftp_readdir (client->sftp->sftp_sess, sftpdir)))
    {
        if (g_strcmp0 (sftpattr->name, ".") != 0 &&
            g_strcmp0 (sftpattr->name, "..") != 0)
        {
            GET_SFTPATTR_TYPE (sftpattr, type);

            tmp = remmina_ssh_convert (REMMINA_SSH (client->sftp), sftpattr->name);
            remmina_ftp_client_add_file (REMMINA_FTP_CLIENT (client),
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
    remmina_sftp_client_sftp_session_closedir (client, sftpdir);

    remmina_ftp_client_set_dir (REMMINA_FTP_CLIENT (client), newdir);
    g_free (newdir);
}

static void
remmina_sftp_client_on_newtask (RemminaSFTPClient *client, gpointer data)
{
    if (client->thread) return;

    if (pthread_create (&client->thread, NULL, remmina_sftp_client_thread_main, client))
    {
        client->thread = 0;
    }
}

static gboolean
remmina_sftp_client_on_canceltask (RemminaSFTPClient *client, gint taskid, gpointer data)
{
    GtkWidget *dialog;
    gint ret;

    if (client->taskid != taskid) return TRUE;

    dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (client))),
        GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
        _("File transfer currently in progress.\nAre you sure to cancel it?"));
    ret = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (ret == GTK_RESPONSE_YES)
    {
        /* Make sure we are still handling the same task before we clear the flag */
        if (client->taskid == taskid) client->taskid = 0;
        return TRUE;
    }
    return FALSE;
}

static gboolean
remmina_sftp_client_on_deletefile (RemminaSFTPClient *client, gint type, gchar *name, gpointer data)
{
    GtkWidget *dialog;
    gint ret = 0;
    gchar *tmp;

    tmp = remmina_ssh_unconvert (REMMINA_SSH (client->sftp), name);
    switch (type)
    {
    case REMMINA_FTP_FILE_TYPE_DIR:
        ret = sftp_rmdir (client->sftp->sftp_sess, tmp);
        break;

    case REMMINA_FTP_FILE_TYPE_FILE:
        ret = sftp_unlink (client->sftp->sftp_sess, tmp);
        break;
    }
    g_free (tmp);

    if (ret != 0)
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (client))),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            _("Failed to delete '%s'. %s"),
            name, ssh_get_error (REMMINA_SSH (client->sftp)->session));
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return FALSE;
    }
    return TRUE;
}

static void
remmina_sftp_client_init (RemminaSFTPClient *client)
{
    client->sftp = NULL;
    client->thread = 0;
    client->taskid = 0;
    client->thread_abort = FALSE;

    /* Setup the internal signals */
    g_signal_connect (G_OBJECT (client), "destroy",
        G_CALLBACK (remmina_sftp_client_destroy), NULL);
    g_signal_connect (G_OBJECT (client), "open-dir",
        G_CALLBACK (remmina_sftp_client_on_opendir), NULL);
    g_signal_connect (G_OBJECT (client), "new-task",
        G_CALLBACK (remmina_sftp_client_on_newtask), NULL);
    g_signal_connect (G_OBJECT (client), "cancel-task",
        G_CALLBACK (remmina_sftp_client_on_canceltask), NULL);
    g_signal_connect (G_OBJECT (client), "delete-file",
        G_CALLBACK (remmina_sftp_client_on_deletefile), NULL);
}

static gboolean
remmina_sftp_client_refresh (RemminaSFTPClient *client)
{
    gdk_window_set_cursor (GTK_WIDGET (client)->window, gdk_cursor_new (GDK_WATCH));
    gdk_flush ();

    remmina_sftp_client_on_opendir (client, ".", NULL);

    gdk_window_set_cursor (GTK_WIDGET (client)->window, NULL);

    return FALSE;
}

GtkWidget*
remmina_sftp_client_new (void)
{
    return GTK_WIDGET (g_object_new (REMMINA_TYPE_SFTP_CLIENT, NULL));
}

void
remmina_sftp_client_open (RemminaSFTPClient *client, RemminaSFTP *sftp)
{
    client->sftp = sftp;

    g_idle_add ((GSourceFunc) remmina_sftp_client_refresh, client);
}

GtkWidget*
remmina_sftp_client_new_init (RemminaSFTP *sftp)
{
    GtkWidget *client;
    GtkWidget *dialog;

    client = remmina_sftp_client_new ();

    gdk_window_set_cursor (GTK_WIDGET (client)->window, gdk_cursor_new (GDK_WATCH));
    gdk_flush ();

    if (!remmina_ssh_init_session (REMMINA_SSH (sftp)) ||
        remmina_ssh_auth (REMMINA_SSH (sftp), NULL) <= 0 ||
        !remmina_sftp_open (sftp))
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (gtk_widget_get_toplevel (client)),
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            (REMMINA_SSH (sftp))->error, NULL);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        gtk_widget_destroy (client);
        return NULL;
    }

    gdk_window_set_cursor (GTK_WIDGET (client)->window, NULL);

    g_idle_add ((GSourceFunc) remmina_sftp_client_refresh, client);
    return client;
}

#endif

