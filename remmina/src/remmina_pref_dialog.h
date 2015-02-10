/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

#ifndef __REMMINAPREFDIALOG_H__
#define __REMMINAPREFDIALOG_H__

/*
 * Remmina Perference Dialog
 */

G_BEGIN_DECLS

#define REMMINA_TYPE_PREF_DIALOG               (remmina_pref_dialog_get_type ())
#define REMMINA_PREF_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_PREF_DIALOG, RemminaPrefDialog))
#define REMMINA_PREF_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_PREF_DIALOG, RemminaPrefDialogClass))
#define REMMINA_IS_PREF_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_PREF_DIALOG))
#define REMMINA_IS_PREF_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_PREF_DIALOG))
#define REMMINA_PREF_DIALOG_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_PREF_DIALOG, RemminaPrefDialogClass))

typedef struct _RemminaPrefDialogPriv RemminaPrefDialogPriv;

typedef struct _RemminaPrefDialog
{
	GtkDialog dialog;

	RemminaPrefDialogPriv *priv;
} RemminaPrefDialog;

typedef struct _RemminaPrefDialogClass
{
	GtkDialogClass parent_class;
} RemminaPrefDialogClass;

GType remmina_pref_dialog_get_type(void)
G_GNUC_CONST;

enum
{
	REMMINA_PREF_OPTIONS_TAB = 0, REMMINA_PREF_RESOLUTIONS_TAB = 1, REMMINA_PREF_APPLET_TAB = 2
};

GtkWidget* remmina_pref_dialog_new(gint default_tab);

G_END_DECLS

#endif  /* __REMMINAPREFDIALOG_H__  */

