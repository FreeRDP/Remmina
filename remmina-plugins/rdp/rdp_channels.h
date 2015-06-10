/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINA_RDP_CHANNELS_H
#define __REMMINA_RDP_CHANNELS_H

#include <freerdp/freerdp.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/rdpei.h>
#include <freerdp/client/tsmf.h>
#include <freerdp/client/rail.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/rdpgfx.h>
#include <freerdp/client/encomsp.h>

G_BEGIN_DECLS

void remmina_rdp_OnChannelConnectedEventHandler(rdpContext* context, ChannelConnectedEventArgs* e);
void remmina_rdp_OnChannelDisconnectedEventHandler(rdpContext* context, ChannelConnectedEventArgs* e);


G_END_DECLS

#endif
