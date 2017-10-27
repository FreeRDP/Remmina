/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2017 Antenore Gatta, Giovanni Panozzo
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

#define KEY_MODIFIER_SHIFT _("Shift+")
#define KEY_MODIFIER_CTRL _("Ctrl+")
#define KEY_MODIFIER_ALT _("Alt+")
#define KEY_MODIFIER_SUPER _("Super+")
#define KEY_MODIFIER_HYPER _("Hyper+")
#define KEY_MODIFIER_META _("Meta+")
#define KEY_CHOOSER_NONE _("<None>")

typedef struct _RemminaKeyChooserArguments {
	guint keyval;
	guint state;
	gboolean use_modifiers;
	gint response;
} RemminaKeyChooserArguments;

G_BEGIN_DECLS

/* Show a key chooser dialog and return the keyval for the selected key */
RemminaKeyChooserArguments* remmina_key_chooser_new(GtkWindow *parent_window, gboolean use_modifiers);
/* Get the uppercase character value of a keyval */
gchar* remmina_key_chooser_get_value(guint keyval, guint state);
/* Get the keyval of a (lowercase) character value */
guint remmina_key_chooser_get_keyval(const gchar *value);

G_END_DECLS


