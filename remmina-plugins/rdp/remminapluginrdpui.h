/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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

#ifndef __REMMINAPLUGINRDPUI_H__
#define __REMMINAPLUGINRDPUI_H__

void remmina_plugin_rdpui_init (RemminaProtocolWidget *gp);
void remmina_plugin_rdpui_pre_connect (RemminaProtocolWidget *gp);
void remmina_plugin_rdpui_post_connect (RemminaProtocolWidget *gp);
void remmina_plugin_rdpui_uninit (RemminaProtocolWidget *gp);
void remmina_plugin_rdpui_get_fds (RemminaProtocolWidget *gp, void ** read_fds, int * read_count);
int remmina_plugin_rdpui_check_fds (RemminaProtocolWidget *gp);
void remmina_plugin_rdpui_update_rect (RemminaProtocolWidget *gp, int x, int y, int w, int h);

#endif

