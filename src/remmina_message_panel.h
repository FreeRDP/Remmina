/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

G_BEGIN_DECLS

#define REMMINA_TYPE_MESSAGE_PANEL             (remmina_message_panel_get_type())
G_DECLARE_DERIVABLE_TYPE(RemminaMessagePanel, remmina_message_panel, REMMINA, MESSAGE_PANEL, GtkBox)

struct _RemminaMessagePanelClass {
	GtkBoxClass	parent_class;
	void		(*response) (RemminaMessagePanel *mp, gint response_id);
};


/* Widgets ID for dialog fields */
enum {
	REMMINA_MESSAGE_PANEL_USERNAME=1,
	REMMINA_MESSAGE_PANEL_PASSWORD,
	REMMINA_MESSAGE_PANEL_DOMAIN,
	REMMINA_MESSAGE_PANEL_SAVEPASSWORD,
	REMMINA_MESSAGE_PANEL_BUTTONTOFOCUS,
	REMMINA_MESSAGE_PANEL_CACERTFILE,
	REMMINA_MESSAGE_PANEL_CACRLFILE,
	REMMINA_MESSAGE_PANEL_CLIENTCERTFILE,
	REMMINA_MESSAGE_PANEL_CLIENTKEYFILE,
	REMMINA_MESSAGE_PANEL_MAXWIDGETID
};

/* Callback function type to receive buttons notification */
typedef void (*RemminaMessagePanelCallback)(void *user_data, int button);

RemminaMessagePanel *remmina_message_panel_new(void);
void remmina_message_panel_setup_progress(RemminaMessagePanel *mp, const gchar *message, RemminaMessagePanelCallback response_callback, gpointer response_callback_data);
void remmina_message_panel_setup_message(RemminaMessagePanel *mp, const gchar *message, RemminaMessagePanelCallback response_callback, gpointer response_callback_data);
void remmina_message_panel_setup_question(RemminaMessagePanel *mp, const gchar *message, RemminaMessagePanelCallback response_callback, gpointer response_callback_data);
void remmina_message_panel_setup_auth(RemminaMessagePanel *mp, RemminaMessagePanelCallback response_callback, gpointer response_callback_data, const gchar *title, const gchar *password_prompt, unsigned flags);
void remmina_message_panel_setup_auth_x509(RemminaMessagePanel *mp, RemminaMessagePanelCallback response_callback, gpointer response_callback_data);
void remmina_message_panel_focus_auth_entry(RemminaMessagePanel *mp);
void remmina_message_panel_field_set_string(RemminaMessagePanel *mp, int entryid, const gchar *text);
gchar *remmina_message_panel_field_get_string(RemminaMessagePanel *mp, int entryid);
void remmina_message_panel_field_set_switch(RemminaMessagePanel *mp, int entryid, gboolean state);
gboolean remmina_message_panel_field_get_switch_state(RemminaMessagePanel *mp, int entryid);
void remmina_message_panel_field_set_filename(RemminaMessagePanel *mp, int entryid, const gchar *filename);
gchar *remmina_message_panel_field_get_filename(RemminaMessagePanel *mp, int entryid);
void remmina_message_panel_response(RemminaMessagePanel *mp, gint response_id);


G_END_DECLS
