/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2018 Denis Ollier
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

#include "common/remmina_plugin.h"
#include <spice-client.h>
#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
#    include <spice-client-gtk.h>
#  else
#    include <spice-widget.h>
#    include <usb-device-widget.h>
#  endif
#else
#  include <spice-widget.h>
#  include <usb-device-widget.h>
#endif

#define GET_PLUGIN_DATA(gp) (RemminaPluginSpiceData *)g_object_get_data(G_OBJECT(gp), "plugin-data")

extern RemminaPluginService *remmina_plugin_service;
#define REMMINA_PLUGIN_DEBUG(fmt, ...) remmina_plugin_service->_remmina_debug(__func__, fmt, ##__VA_ARGS__)

typedef struct _RemminaPluginSpiceData {
	SpiceAudio *		audio;
	SpiceDisplay *		display;
	SpiceDisplayChannel *	display_channel;
	SpiceGtkSession *	gtk_session;
	SpiceMainChannel *	main_channel;
	SpiceSession *		session;

#ifdef SPICE_GTK_CHECK_VERSION
#  if SPICE_GTK_CHECK_VERSION(0, 31, 0)
	/* key: SpiceFileTransferTask, value: RemminaPluginSpiceXferWidgets */
	GHashTable *		file_transfers;
	GtkWidget *		file_transfer_dialog;
#  endif        /* SPICE_GTK_CHECK_VERSION(0, 31, 0) */
#endif          /* SPICE_GTK_CHECK_VERSION */
} RemminaPluginSpiceData;
