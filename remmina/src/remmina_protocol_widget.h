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
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
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

#ifndef __REMMINAPROTOCOLWIDGET_H__
#define __REMMINAPROTOCOLWIDGET_H__

#include "remmina_init_dialog.h"
#include "remmina_file.h"
#include "remmina_ssh.h"

G_BEGIN_DECLS

#define REMMINA_PROTOCOL_FEATURE_TOOL_SSH  -1
#define REMMINA_PROTOCOL_FEATURE_TOOL_SFTP -2

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

	void (*connect)(RemminaProtocolWidget *gp);
	void (*disconnect)(RemminaProtocolWidget *gp);
	void (*desktop_resize)(RemminaProtocolWidget *gp);
	void (*update_align)(RemminaProtocolWidget *gp);
};

GType remmina_protocol_widget_get_type(void)
G_GNUC_CONST;

GtkWidget* remmina_protocol_widget_new(void);

GtkWidget* remmina_protocol_widget_get_init_dialog(RemminaProtocolWidget *gp);

gint remmina_protocol_widget_get_width(RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_width(RemminaProtocolWidget *gp, gint width);
gint remmina_protocol_widget_get_height(RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_height(RemminaProtocolWidget *gp, gint height);
gboolean remmina_protocol_widget_get_scale(RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_scale(RemminaProtocolWidget *gp, gboolean scale);
gboolean remmina_protocol_widget_get_expand(RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_expand(RemminaProtocolWidget *gp, gboolean expand);
gboolean remmina_protocol_widget_has_error(RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_get_error_message(RemminaProtocolWidget *gp);
void remmina_protocol_widget_set_error(RemminaProtocolWidget *gp, const gchar *fmt, ...);
gboolean remmina_protocol_widget_is_closed(RemminaProtocolWidget *gp);
RemminaFile* remmina_protocol_widget_get_file(RemminaProtocolWidget *gp);

void remmina_protocol_widget_open_connection(RemminaProtocolWidget *gp, RemminaFile *remminafile);
gboolean remmina_protocol_widget_close_connection(RemminaProtocolWidget *gp);
void remmina_protocol_widget_grab_focus(RemminaProtocolWidget *gp);
const RemminaProtocolFeature* remmina_protocol_widget_get_features(RemminaProtocolWidget *gp);
const gchar* remmina_protocol_widget_get_domain(RemminaProtocolWidget *gp);
gboolean remmina_protocol_widget_query_feature_by_type(RemminaProtocolWidget *gp, RemminaProtocolFeatureType type);
gboolean remmina_protocol_widget_query_feature_by_ref(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
void remmina_protocol_widget_call_feature_by_type(RemminaProtocolWidget *gp, RemminaProtocolFeatureType type, gint id);
void remmina_protocol_widget_call_feature_by_ref(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
/* Provide thread-safe way to emit signals */
void remmina_protocol_widget_emit_signal(RemminaProtocolWidget *gp, const gchar *signal);
void remmina_protocol_widget_register_hostkey(RemminaProtocolWidget *gp, GtkWidget *widget);

typedef gboolean (*RemminaHostkeyFunc)(RemminaProtocolWidget *gp, guint keyval, gboolean release, gpointer data);
void remmina_protocol_widget_set_hostkey_func(RemminaProtocolWidget *gp, RemminaHostkeyFunc func, gpointer data);

gboolean remmina_protocol_widget_ssh_exec(RemminaProtocolWidget *gp, gboolean wait, const gchar *fmt, ...);

/* Start a SSH tunnel if it's enabled. Returns a newly allocated string indicating:
 * 1. The actual destination (host:port) if SSH tunnel is disable
 * 2. The tunnel local destination (127.0.0.1:port) if SSH tunnel is enabled
 */
gchar* remmina_protocol_widget_start_direct_tunnel(RemminaProtocolWidget *gp, gint default_port, gboolean port_plus);

gboolean remmina_protocol_widget_start_reverse_tunnel(RemminaProtocolWidget *gp, gint local_port);
gboolean remmina_protocol_widget_start_xport_tunnel(RemminaProtocolWidget *gp, RemminaXPortTunnelInitFunc init_func);
void remmina_protocol_widget_set_display(RemminaProtocolWidget *gp, gint display);

gint remmina_protocol_widget_init_authpwd(RemminaProtocolWidget *gp, RemminaAuthpwdType authpwd_type);
gint remmina_protocol_widget_init_authuserpwd(RemminaProtocolWidget *gp, gboolean want_domain);
gint remmina_protocol_widget_init_certificate(RemminaProtocolWidget* gp, const gchar* subject, const gchar* issuer, const gchar* fingerprint);
gint remmina_protocol_widget_changed_certificate(RemminaProtocolWidget *gp, const gchar* subject, const gchar* issuer, const gchar* new_fingerprint, const gchar* old_fingerprint);
gchar* remmina_protocol_widget_init_get_username(RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_password(RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_domain(RemminaProtocolWidget *gp);
gint remmina_protocol_widget_init_authx509(RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_cacert(RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_cacrl(RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_clientcert(RemminaProtocolWidget *gp);
gchar* remmina_protocol_widget_init_get_clientkey(RemminaProtocolWidget *gp);
void remmina_protocol_widget_init_save_cred(RemminaProtocolWidget *gp);
void remmina_protocol_widget_init_show_listen(RemminaProtocolWidget *gp, gint port);
void remmina_protocol_widget_init_show_retry(RemminaProtocolWidget *gp);
void remmina_protocol_widget_init_show(RemminaProtocolWidget *gp);
void remmina_protocol_widget_init_hide(RemminaProtocolWidget *gp);

void remmina_protocol_widget_chat_open(RemminaProtocolWidget *gp, const gchar *name,
		void(*on_send)(RemminaProtocolWidget *gp, const gchar *text), void(*on_destroy)(RemminaProtocolWidget *gp));
void remmina_protocol_widget_chat_close(RemminaProtocolWidget *gp);
void remmina_protocol_widget_chat_receive(RemminaProtocolWidget *gp, const gchar *text);

G_END_DECLS

#endif  /* __REMMINAPROTOCOLWIDGET_H__  */

