/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINA_RDP_EVENT_H__
#define __REMMINA_RDP_EVENT_H__

G_BEGIN_DECLS

void remmina_rdp_event_init(RemminaProtocolWidget* gp);
void remmina_rdp_event_uninit(RemminaProtocolWidget* gp);
void remmina_rdp_event_update_scale(RemminaProtocolWidget* gp);
gboolean remmina_rdp_event_queue_ui(RemminaProtocolWidget* gp);
void remmina_rdp_event_unfocus(RemminaProtocolWidget* gp);
void remmina_rdp_event_update_rect(RemminaProtocolWidget* gp, gint x, gint y, gint w, gint h);

G_END_DECLS

#endif

