/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee 
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
 

#ifndef __REMMINAPROTOCOLWIDGET_H__
#define __REMMINAPROTOCOLWIDGET_H__

#include "remminainitdialog.h"
#include "remminafile.h"
#include "remminassh.h"

G_BEGIN_DECLS

#define REMMINA_TYPE_PROTOCOL_WIDGET                  (remmina_protocol_widget_get_type ())
#define REMMINA_PROTOCOL_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_PROTOCOL_WIDGET, RemminaProtocolWidget))
#define REMMINA_PROTOCOL_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_PROTOCOL_WIDGET, RemminaProtocolWidgetClass))
#define REMMINA_IS_PROTOCOL_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_PROTOCOL_WIDGET))
#define REMMINA_IS_PROTOCOL_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_PROTOCOL_WIDGET))
#define REMMINA_PROTOCOL_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_PROTOCOL_WIDGET, RemminaProtocolWidgetClass))

typedef struct _RemminaProtocolWidgetPriv RemminaProtocolWidgetPriv;

struct _RemminaProtocolWidget
{
    GtkEventBox event_box;

    RemminaProtocolWidgetPriv *priv;
};

struct _RemminaProtocolWidgetClass
{
    GtkEventBoxClass parent_class;

    void (* connect) (RemminaProtocolWidget *gp);
    void (* disconnect) (RemminaProtocolWidget *gp);
    void (* desktop_resize) (RemminaProtocolWidget *gp);
};

GType remmina_protocol_widget_get_type (void) G_GNUC_CONST;

GtkWidget* remmina_protocol_widget_new (void);

GtkWidget* remmina_protocol_widget_get_init_dialog (RemminaProtocolWidget *gp);

gint remmina_protocol_widget_get_width (RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_width (RemminaProtocolWidget *gp, gint width);
gint remmina_protocol_widget_get_height (RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_height (RemminaProtocolWidget *gp, gint height);
gboolean remmina_protocol_widget_get_scale (RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_scale (RemminaProtocolWidget *gp, gboolean scale);
gboolean remmina_protocol_widget_get_expand (RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_expand (RemminaProtocolWidget *gp, gboolean expand);
gboolean remmina_protocol_widget_has_error (RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_get_error_message (RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_error (RemminaProtocolWidget *gp, const gchar *fmt, ...);
gboolean remmina_protocol_widget_is_closed (RemminaProtocolWidget *gp);
RemminaFile* remmina_protocol_widget_get_file (RemminaProtocolWidget *gp);
GPtrArray* remmina_protocol_widget_get_printers (RemminaProtocolWidget *gp);

void remmina_protocol_widget_open_connection (RemminaProtocolWidget *gp, RemminaFile *remminafile);
gboolean remmina_protocol_widget_close_connection (RemminaProtocolWidget *gp);
void remmina_protocol_widget_grab_focus (RemminaProtocolWidget *gp);
gpointer remmina_protocol_widget_query_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature);
void remmina_protocol_widget_call_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature, const gpointer data);
/* Provide thread-safe way to emit signals */
void remmina_protocol_widget_emit_signal (RemminaProtocolWidget *gp, const gchar *signal);

gboolean remmina_protocol_widget_ssh_exec (RemminaProtocolWidget *gp, const gchar *fmt, ...);

/* Start a SSH tunnel if it's enabled. Returns a newly allocated string indicating:
 * 1. The actual destination (host:port) if SSH tunnel is disable
 * 2. The tunnel local destination (127.0.0.1:port) if SSH tunnel is enabled
 */
gchar* remmina_protocol_widget_start_direct_tunnel (RemminaProtocolWidget *gp, gint default_port);

gboolean remmina_protocol_widget_start_xport_tunnel (RemminaProtocolWidget *gp, gint display,
    RemminaXPortTunnelInitFunc init_func);

gint remmina_protocol_widget_init_authpwd (RemminaProtocolWidget *gp);
gint remmina_protocol_widget_init_authuserpwd (RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_username (RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_password (RemminaProtocolWidget *gp);
gint remmina_protocol_widget_init_authx509 (RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_cacert (RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_cacrl (RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_clientcert (RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_clientkey (RemminaProtocolWidget *gp);
void remmina_protocol_widget_init_save_cred (RemminaProtocolWidget *gp);
void remmina_protocol_widget_init_show_listen (RemminaProtocolWidget *gp, gint port);
void remmina_protocol_widget_init_show_retry (RemminaProtocolWidget *gp);

void remmina_protocol_widget_chat_open (RemminaProtocolWidget *gp, const gchar *name,
    void(*on_send)(RemminaProtocolWidget *gp, const gchar *text),
    void(*on_destroy)(RemminaProtocolWidget *gp));
void remmina_protocol_widget_chat_close (RemminaProtocolWidget *gp);
void remmina_protocol_widget_chat_receive (RemminaProtocolWidget *gp, const gchar *text);

G_END_DECLS

#endif  /* __REMMINAPROTOCOLWIDGET_H__  */

