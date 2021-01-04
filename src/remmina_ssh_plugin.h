/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
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

#ifdef HAVE_LIBVTE
#include <vte/vte.h>
#endif

G_BEGIN_DECLS

void remmina_ssh_plugin_register(void);

typedef struct _RemminaProtocolSettingOpt {
	RemminaProtocolSettingType	type;
	const gchar *			name;
	const gchar *			label;
	gboolean			compact;
	gpointer			opt1;
	gpointer			opt2;
} RemminaProtocolSettingOpt;

/* For callback in main thread */
#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)
void remmina_plugin_ssh_vte_terminal_set_encoding_and_pty(VteTerminal *terminal, const char *codeset, int master, int slave);
void remmina_plugin_ssh_vte_select_all(GtkMenuItem *menuitem, gpointer vte);
void remmina_plugin_ssh_vte_copy_clipboard(GtkMenuItem *menuitem, gpointer vte);
void remmina_plugin_ssh_vte_paste_clipboard(GtkMenuItem *menuitem, gpointer vte);
void remmina_plugin_ssh_vte_decrease_font(GtkMenuItem *menuitem, gpointer vte);
void remmina_plugin_ssh_vte_increase_font(GtkMenuItem *menuitem, gpointer vte);
gboolean remmina_ssh_plugin_popup_menu(GtkWidget *widget, GdkEvent *event, GtkWidget *menu);
#endif

G_END_DECLS
