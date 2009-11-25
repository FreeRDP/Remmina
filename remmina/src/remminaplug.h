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
 

#ifndef __REMMINAPLUG_H__
#define __REMMINAPLUG_H__

#include "remminainitdialog.h"
#include "remminafile.h"
#include "remminassh.h"

G_BEGIN_DECLS

#define REMMINA_TYPE_PLUG                  (remmina_plug_get_type ())
#define REMMINA_PLUG(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_PLUG, RemminaPlug))
#define REMMINA_PLUG_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_PLUG, RemminaPlugClass))
#define REMMINA_IS_PLUG(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_PLUG))
#define REMMINA_IS_PLUG_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_PLUG))
#define REMMINA_PLUG_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_PLUG, RemminaPlugClass))

#define MAX_ERROR_LENGTH 1000

typedef enum
{
    REMMINA_PLUG_FEATURE_PREF,
    REMMINA_PLUG_FEATURE_PREF_COLORDEPTH,
    REMMINA_PLUG_FEATURE_PREF_QUALITY,
    REMMINA_PLUG_FEATURE_PREF_VIEWONLY,
    REMMINA_PLUG_FEATURE_PREF_DISABLESERVERINPUT,
    REMMINA_PLUG_FEATURE_TOOL,
    REMMINA_PLUG_FEATURE_TOOL_CHAT,
    REMMINA_PLUG_FEATURE_TOOL_SFTP,
    REMMINA_PLUG_FEATURE_TOOL_SSHTERM,
    REMMINA_PLUG_FEATURE_UNFOCUS,
    REMMINA_PLUG_FEATURE_SCALE
} RemminaPlugFeature;

typedef struct _RemminaPlug
{
    GtkEventBox event_box;

    GtkWidget *init_dialog;

    RemminaFile *remmina_file;

    gint width;
    gint height;
    gboolean scale;

    gboolean has_error;
    gchar error_message[MAX_ERROR_LENGTH + 1];
    RemminaSSHTunnel *ssh_tunnel;

    GtkWidget *sftp_window;

    gboolean closed;

    GPtrArray *printers;
} RemminaPlug;

typedef struct _RemminaPlugClass
{
    GtkEventBoxClass parent_class;

    void (* connect) (RemminaPlug *gp);
    void (* disconnect) (RemminaPlug *gp);
    void (* desktop_resize) (RemminaPlug *gp);

    /* "Virtual" functions have to be implemented in sub-classes */
    gboolean (* open_connection) (RemminaPlug *gp);
    gboolean (* close_connection) (RemminaPlug *gp);
    gpointer (* query_feature) (RemminaPlug *gp, RemminaPlugFeature feature);
    void (* call_feature) (RemminaPlug *gp, RemminaPlugFeature feature, const gpointer data);
} RemminaPlugClass;

GType remmina_plug_get_type (void) G_GNUC_CONST;

void remmina_plug_open_connection (RemminaPlug *gp, RemminaFile *remminafile);
gboolean remmina_plug_close_connection (RemminaPlug *gp);
void remmina_plug_grab_focus (RemminaPlug *gp);
gpointer remmina_plug_query_feature (RemminaPlug *gp, RemminaPlugFeature feature);
void remmina_plug_call_feature (RemminaPlug *gp, RemminaPlugFeature feature, const gpointer data);
/* Provide thread-safe way to emit signals */
void remmina_plug_emit_signal (RemminaPlug *gp, const gchar *signal);

/* Start a SSH tunnel if it's enabled. Returns a newly allocated string indicating:
 * 1. The actual destination (host:port) if SSH tunnel is disable
 * 2. The tunnel local destination (127.0.0.1:port) if SSH tunnel is enabled
 */
gchar* remmina_plug_start_direct_tunnel (RemminaPlug *gp, gint default_port);

gboolean remmina_plug_start_xport_tunnel (RemminaPlug *gp, gint display,
    RemminaSSHTunnelCallback init_func,
    RemminaSSHTunnelCallback connect_func,
    RemminaSSHTunnelCallback disconnect_func,
    gpointer callback_data);

G_END_DECLS

#endif  /* __REMMINAPLUG_H__  */

