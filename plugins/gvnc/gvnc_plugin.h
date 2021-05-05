/*
 * Remmina - The GTK+ Remote Desktop Client
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

#include "common/remmina_plugin.h"

#include <gdk/gdkkeysyms.h>
#include <vncdisplay.h>
#include <vncutil.h>
#include <vncaudiopulse.h>


#ifndef GDK_Return
#define GDK_Return GDK_KEY_Return
#endif
#ifndef GDK_Escape
#define GDK_Escape GDK_KEY_Escape
#endif
#ifndef GDK_BackSpace
#define GDK_BackSpace GDK_KEY_BackSpace
#endif
#ifndef GDK_Delete
#define GDK_Delete GDK_KEY_Delete
#endif
#ifndef GDK_Control_L
#define GDK_Control_L GDK_KEY_Control_L
#endif
#ifndef GDK_Alt_L
#define GDK_Alt_L GDK_KEY_Alt_L
#endif
#ifndef GDK_F1
#define GDK_F1 GDK_KEY_F1
#endif
#ifndef GDK_F2
#define GDK_F2 GDK_KEY_F2
#endif
#ifndef GDK_F3
#define GDK_F3 GDK_KEY_F3
#endif
#ifndef GDK_F4
#define GDK_F4 GDK_KEY_F4
#endif
#ifndef GDK_F5
#define GDK_F5 GDK_KEY_F5
#endif
#ifndef GDK_F6
#define GDK_F6 GDK_KEY_F6
#endif
#ifndef GDK_F7
#define GDK_F7 GDK_KEY_F7
#endif
#ifndef GDK_F8
#define GDK_F8 GDK_KEY_F8
#endif
#ifndef GDK_F11
#define GDK_F11 GDK_KEY_F11
#endif

#define GET_PLUGIN_DATA(gp) (GVncPluginData *)g_object_get_data(G_OBJECT(gp), "plugin-data")
#define REMMINA_PLUGIN_DEBUG(fmt, ...) remmina_plugin_service->_remmina_debug(__func__, fmt, ## __VA_ARGS__)

typedef struct _GVncPluginData {
	GtkWidget *	box;
	GtkWidget *	vnc;
	VncConnection * conn;
	VncAudioPulse * pa;
	gchar *		error_msg;
	gchar *		clipstr;
	gulong		signal_clipboard;
	gint		depth_profile;
	gint		shared;
	gboolean	lossy_encoding;
	gboolean	viewonly;
	gint		width;
	gint		height;
	gint		fd;
	gchar *		addr;
} GVncPluginData;

G_BEGIN_DECLS
G_END_DECLS
