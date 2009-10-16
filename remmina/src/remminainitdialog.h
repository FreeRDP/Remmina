/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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
 

#ifndef __REMMINAINITDIALG_H__
#define __REMMINAINITDIALG_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_INIT_DIALOG               (remmina_init_dialog_get_type ())
#define REMMINA_INIT_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_INIT_DIALOG, RemminaInitDialog))
#define REMMINA_INIT_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_INIT_DIALOG, RemminaInitDialogClass))
#define REMMINA_IS_INIT_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_INIT_DIALOG))
#define REMMINA_IS_INIT_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_INIT_DIALOG))
#define REMMINA_INIT_DIALOG_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_INIT_DIALOG, RemminaInitDialogClass))

enum
{
    REMMINA_INIT_MODE_CONNECTING,
    REMMINA_INIT_MODE_AUTHPWD,
    REMMINA_INIT_MODE_AUTHUSERPWD,
    REMMINA_INIT_MODE_AUTHX509
};

typedef struct _RemminaInitDialog
{
    GtkDialog dialog;

    GtkWidget *image;
    GtkWidget *content_vbox;
    GtkWidget *status_label;

    gint mode;

    gchar *title;
    gchar *status;
    gchar *username;
    gchar *password;
    gboolean save_password;
    gchar *cacert;
    gchar *cacrl;
    gchar *clientcert;
    gchar *clientkey;
} RemminaInitDialog;

typedef struct _RemminaInitDialogClass
{
    GtkDialogClass parent_class;
} RemminaInitDialogClass;

GType remmina_init_dialog_get_type (void) G_GNUC_CONST;

GtkWidget* remmina_init_dialog_new (const gchar *title_format, ...);
void remmina_init_dialog_set_status (RemminaInitDialog *dialog, const gchar *status_format, ...);
void remmina_init_dialog_set_status_temp (RemminaInitDialog *dialog, const gchar *status_format, ...);
/* Run authentication. Return GTK_RESPONSE_OK or GTK_RESPONSE_CANCEL. Caller is blocked. */
gint remmina_init_dialog_authpwd (RemminaInitDialog *dialog, const gchar *label, gboolean allow_save);
gint remmina_init_dialog_authuserpwd (RemminaInitDialog *dialog, const gchar *default_username, gboolean allow_save);
gint remmina_init_dialog_authx509 (RemminaInitDialog *dialog, const gchar *cacert, const gchar *cacrl,
    const gchar *clientcert, const gchar *clientkey);

G_END_DECLS

#endif  /* __REMMINAINITDIALG_H__  */

