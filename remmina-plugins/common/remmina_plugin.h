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

#ifndef __REMMINAPLUGINCOMMON_H__
#define __REMMINAPLUGINCOMMON_H__

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <remmina/plugin.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#ifdef HAVE_PTHREAD
#include <pthread.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

typedef void (*PThreadCleanupFunc)(void*);

/* Wrapper macros to make the compiler happy on both signle/multi-threaded mode */
#ifdef HAVE_PTHREAD
#define IDLE_ADD        gdk_threads_add_idle
#define TIMEOUT_ADD     gdk_threads_add_timeout
#define CANCEL_ASYNC    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);pthread_testcancel();
#define CANCEL_DEFER    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);
#define THREADS_ENTER   gdk_threads_enter();pthread_cleanup_push((PThreadCleanupFunc)gdk_threads_leave,NULL);
#define THREADS_LEAVE   pthread_cleanup_pop(TRUE);
#else
#define IDLE_ADD        g_idle_add
#define TIMEOUT_ADD     g_timeout_add
#define CANCEL_ASYNC
#define CANCEL_DEFER
#define THREADS_ENTER
#define THREADS_LEAVE
#endif

#define MAX_X_DISPLAY_NUMBER 99
#define X_UNIX_SOCKET "/tmp/.X11-unix/X%d"

#define INCLUDE_GET_AVAILABLE_XDISPLAY static gint \
remmina_get_available_xdisplay (void) \
{ \
    gint i; \
    gint display = 0; \
    gchar fn[200]; \
    for (i = 1; i < MAX_X_DISPLAY_NUMBER; i++) \
    { \
        g_snprintf (fn, sizeof (fn), X_UNIX_SOCKET, i); \
        if (!g_file_test (fn, G_FILE_TEST_EXISTS)) \
        { \
            display = i; \
            break; \
        } \
    } \
    return display; \
}

#endif /* __REMMINAPLUGINCOMMON_H__ */

