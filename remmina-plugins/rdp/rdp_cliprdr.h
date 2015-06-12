/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINA_RDP_CLIPRDR_H__
#define __REMMINA_RDP_CLIPRDR_H__


#include <freerdp/freerdp.h>
#include "rdp_plugin.h"

void remmina_rdp_clipboard_init(rfContext* rfi);
void remmina_rdp_clipboard_free(rfContext* rfi);
void remmina_rdp_cliprdr_init(rfContext* rfc, CliprdrClientContext* cliprdr);
void remmina_rdp_channel_cliprdr_process(RemminaProtocolWidget* gp, wMessage* event);
void remmina_rdp_event_process_clipboard(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui);


#endif
