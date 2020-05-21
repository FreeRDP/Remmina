/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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

#include <glib.h>
#include <webkit2/webkit2.h>

typedef enum {
	WWW_WEB_VIEW_DOCUMENT_HTML,
	WWW_WEB_VIEW_DOCUMENT_XML,
	WWW_WEB_VIEW_DOCUMENT_IMAGE,
	WWW_WEB_VIEW_DOCUMENT_OCTET_STREAM,
	WWW_WEB_VIEW_DOCUMENT_OTHER
} WWWWebViewDocumentType;

extern RemminaPluginService *remmina_plugin_service;
#define REMMINA_PLUGIN_DEBUG(fmt, ...) remmina_plugin_service->_remmina_debug(__func__, fmt, ##__VA_ARGS__)


G_BEGIN_DECLS
void remmina_plugin_www_decide_nav(WebKitPolicyDecision *decision, RemminaProtocolWidget *gp);
void remmina_plugin_www_decide_newwin(WebKitPolicyDecision *decision, RemminaProtocolWidget *gp);
gboolean remmina_plugin_www_decide_resource(WebKitPolicyDecision *decision, RemminaProtocolWidget *gp);
void remmina_plugin_www_response_received(WebKitDownload *download, GParamSpec *ps, RemminaProtocolWidget *gp);
void remmina_plugin_www_notify_download(WebKitDownload *download, gchar *destination, RemminaProtocolWidget *gp);

G_END_DECLS
