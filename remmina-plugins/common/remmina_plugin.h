/*  See LICENSE and COPYING files for copyright and license details. */

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
#include <pthread.h>
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
#include "remmina/remmina_trace_calls.h"

typedef void (*PThreadCleanupFunc)(void*);


#define IDLE_ADD        gdk_threads_add_idle
#define TIMEOUT_ADD     gdk_threads_add_timeout
#define CANCEL_ASYNC    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS,NULL);pthread_testcancel();
#define CANCEL_DEFER    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,NULL);

#define THREADS_ENTER _Pragma("GCC error \"THREADS_ENTER has been deprecated in Remmina 1.2\"")
#define THREADS_LEAVE _Pragma("GCC error \"THREADS_LEAVE has been deprecated in Remmina 1.2\"")

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

