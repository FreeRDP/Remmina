/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2012-2012 Jean-Louis Dupond
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

#include "rdp_plugin.h"
#include "rdp_cliprdr.h"
#include "rdp_channels.h"
#include "rdp_event.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/gdi/gfx.h>

void remmina_rdp_OnChannelConnectedEventHandler(rdpContext* context, ChannelConnectedEventArgs* e)
{
	TRACE_CALL(__func__);

	rfContext* rfi = (rfContext*)context;

	if (g_strcmp0(e->name, RDPEI_DVC_CHANNEL_NAME) == 0) {
		g_print("Unimplemented: channel %s connected but we can’t use it\n", e->name);
		// xfc->rdpei = (RdpeiClientContext*) e->pInterface;
	}else if (g_strcmp0(e->name, TSMF_DVC_CHANNEL_NAME) == 0) {
		g_print("Unimplemented: channel %s connected but we can’t use it\n", e->name);
		// xf_tsmf_init(xfc, (TsmfClientContext*) e->pInterface);
	}else if (g_strcmp0(e->name, RDPGFX_DVC_CHANNEL_NAME) == 0) {
	   if (rfi->settings->SoftwareGdi) {
			rfi->rdpgfxchan = TRUE;
			gdi_graphics_pipeline_init(context->gdi, (RdpgfxClientContext*) e->pInterface);
	   }
	   else
			g_print("Unimplemented: channel %s connected but libfreerdp is in HardwareGdi mode\n", e->name);
	}else if (g_strcmp0(e->name, RAIL_SVC_CHANNEL_NAME) == 0) {
		g_print("Unimplemented: channel %s connected but we can’t use it\n", e->name);
		// xf_rail_init(xfc, (RailClientContext*) e->pInterface);
	}else if (g_strcmp0(e->name, CLIPRDR_SVC_CHANNEL_NAME) == 0) {
		remmina_rdp_cliprdr_init( rfi, (CliprdrClientContext*)e->pInterface);
	}else if (g_strcmp0(e->name, ENCOMSP_SVC_CHANNEL_NAME) == 0) {
		g_print("Unimplemented: channel %s connected but we can’t use it\n", e->name);
		// xf_encomsp_init(xfc, (EncomspClientContext*) e->pInterface);
	}else if (g_strcmp0(e->name, DISP_DVC_CHANNEL_NAME) == 0) {
		// "disp" channel connected, save its context pointer
		rfi->dispcontext = (DispClientContext*)e->pInterface;
		// Notify rcw to unlock dynres capability
		remmina_plugin_service->protocol_plugin_unlock_dynres(rfi->protocol_widget);
		// Send monitor layout message here to ask for resize of remote desktop now
		if (rfi->scale == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES) {
			remmina_rdp_event_send_delayed_monitor_layout(rfi->protocol_widget);
		}
	}REMMINA_PLUGIN_DEBUG("Channel %s has been opened", e->name);
}

void remmina_rdp_OnChannelDisconnectedEventHandler(rdpContext* context, ChannelConnectedEventArgs* e)
{
	TRACE_CALL(__func__);
	rfContext* rfi = (rfContext*)context;

	if (strcmp(e->name, RDPGFX_DVC_CHANNEL_NAME) == 0) {
		if (rfi->settings->SoftwareGdi)
			gdi_graphics_pipeline_uninit(context->gdi, (RdpgfxClientContext*) e->pInterface);
	}
	REMMINA_PLUGIN_DEBUG("Channel %s has been closed", e->name);

}
