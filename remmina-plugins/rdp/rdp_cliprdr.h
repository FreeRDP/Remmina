/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2012-2012 Jean-Louis Dupond
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

#ifndef __REMMINA_RDP_CLIPRDR_H__
#define __REMMINA_RDP_CLIPRDR_H__

G_BEGIN_DECLS

RDP_EVENT* remmina_rdp_cliprdr_get_event(uint16 event_type);
int remmina_rdp_cliprdr_send_format_list_event(RemminaProtocolWidget* gp);
void remmina_handle_channel_event(RemminaProtocolWidget* gp, RDP_EVENT* event);

G_END_DECLS

#endif
