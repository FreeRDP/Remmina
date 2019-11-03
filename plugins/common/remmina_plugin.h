/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
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

typedef void (*PThreadCleanupFunc)(void *);


#define IDLE_ADD        gdk_threads_add_idle
#define TIMEOUT_ADD     gdk_threads_add_timeout
#define CANCEL_ASYNC    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL); pthread_testcancel();
#define CANCEL_DEFER    pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED, NULL);

#define THREADS_ENTER _Pragma("GCC error \"THREADS_ENTER has been deprecated in Remmina 1.2\"")
#define THREADS_LEAVE _Pragma("GCC error \"THREADS_LEAVE has been deprecated in Remmina 1.2\"")

#define MAX_X_DISPLAY_NUMBER 99
#define X_UNIX_SOCKET "/tmp/.X11-unix/X%d"

#define INCLUDE_GET_AVAILABLE_XDISPLAY static gint \
	remmina_get_available_xdisplay(void) \
	{ \
		gint i; \
		gint display = 0; \
		gchar fn[200]; \
		for (i = 1; i < MAX_X_DISPLAY_NUMBER; i++) \
		{ \
			g_snprintf(fn, sizeof(fn), X_UNIX_SOCKET, i); \
			if (!g_file_test(fn, G_FILE_TEST_EXISTS)) \
			{ \
				display = i; \
				break; \
			} \
		} \
		return display; \
	}
