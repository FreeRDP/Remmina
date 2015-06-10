/*  See LICENSE and COPYING files for copyright and license details. */

#include "rdp_plugin.h"
#include "rdp_cliprdr.h"
#include "rdp_channels.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/client/cliprdr.h>

void remmina_rdp_OnChannelConnectedEventHandler(rdpContext* context, ChannelConnectedEventArgs* e)
{
	rfContext* rfi = (rfContext*) context;

	if (g_strcmp0(e->name, RDPEI_DVC_CHANNEL_NAME) == 0)
	{
		g_print("Unimplemented: channel %s connected but we can't use it\n", e->name);
		// xfc->rdpei = (RdpeiClientContext*) e->pInterface;
	}
	else if (g_strcmp0(e->name, TSMF_DVC_CHANNEL_NAME) == 0)
	{
		g_print("Unimplemented: channel %s connected but we can't use it\n", e->name);
		// xf_tsmf_init(xfc, (TsmfClientContext*) e->pInterface);
	}
	else if (g_strcmp0(e->name, RDPGFX_DVC_CHANNEL_NAME) == 0)
	{
		g_print("Unimplemented: channel %s connected but we can't use it\n", e->name);
		/*
		if (settings->SoftwareGdi)
			gdi_graphics_pipeline_init(context->gdi, (RdpgfxClientContext*) e->pInterface);
		else
			xf_graphics_pipeline_init(xfc, (RdpgfxClientContext*) e->pInterface);
		*/
	}
	else if (g_strcmp0(e->name, RAIL_SVC_CHANNEL_NAME) == 0)
	{
		g_print("Unimplemented: channel %s connected but we can't use it\n", e->name);
		// xf_rail_init(xfc, (RailClientContext*) e->pInterface);
	}
	else if (g_strcmp0(e->name, CLIPRDR_SVC_CHANNEL_NAME) == 0)
	{
		remmina_rdp_cliprdr_init( rfi, (CliprdrClientContext*) e->pInterface);
	}
	else if (g_strcmp0(e->name, ENCOMSP_SVC_CHANNEL_NAME) == 0)
	{
		g_print("Unimplemented: channel %s connected but we can't use it\n", e->name);
		// xf_encomsp_init(xfc, (EncomspClientContext*) e->pInterface);
	}
}

void remmina_rdp_OnChannelDisconnectedEventHandler(rdpContext* context, ChannelConnectedEventArgs* e)
{

}
