/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2012-2012 Jean-Louis Dupond
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

#pragma once


#include <freerdp/freerdp.h>
#include "rdp_plugin.h"

void remmina_rdp_clipboard_init(rfContext *rfi);
void remmina_rdp_clipboard_free(rfContext *rfi);
void remmina_rdp_cliprdr_init(rfContext *rfi, CliprdrClientContext *cliprdr);
void remmina_rdp_channel_cliprdr_process(RemminaProtocolWidget *gp, wMessage *event);
void remmina_rdp_event_process_clipboard(RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui);
CLIPRDR_FORMAT_LIST *remmina_rdp_cliprdr_get_client_format_list(RemminaProtocolWidget *gp);
void remmina_rdp_cliprdr_detach_owner(RemminaProtocolWidget *gp);
void remmina_rdp_clipboard_abort_transfer(rfContext *rfi);
