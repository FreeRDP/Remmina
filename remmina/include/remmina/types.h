/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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

#ifndef __REMMINA_TYPES_H__
#define __REMMINA_TYPES_H__

G_BEGIN_DECLS

typedef struct _RemminaFile
{
    /* Profile settings */
    gchar *filename;
    gchar *name;
    gchar *group;

    /* Protocol settings */
    gchar *server;
    gchar *protocol;
    gchar *username;
    gchar *password;
    gchar *domain;
    gchar *clientname;
    gchar *resolution;
    gchar *keymap;
    gchar *gkeymap;
    gchar *exec;
    gchar *execpath;
    gchar *sound;
    gchar *arguments;
    gchar *cacert;
    gchar *cacrl;
    gchar *clientcert;
    gchar *clientkey;
    gint colordepth;
    gint quality;
    gint listenport;
    gint sharefolder;
    gint hscale;
    gint vscale;
    gboolean bitmapcaching;
    gboolean compression;
    gboolean showcursor;
    gboolean viewonly;
    gboolean console;
    gboolean disableserverinput;
    gboolean aspectscale;
    gboolean shareprinter;
    gboolean once;

    /* SSH settings */
    gboolean ssh_enabled;
    gchar *ssh_server;
    gint ssh_auth;
    gchar *ssh_username;
    gchar *ssh_privatekey;
    gchar *ssh_charset;

    /* Run-time settings */
    gint viewmode;
    gboolean scale;
    gboolean keyboard_grab;

    gint window_width;
    gint window_height;
    gboolean window_maximize;

    gint toolbar_opacity;

    gint resolution_width;
    gint resolution_height;
} RemminaFile;

typedef enum
{
    REMMINA_PROTOCOL_FEATURE_PREF,
    REMMINA_PROTOCOL_FEATURE_PREF_COLORDEPTH,
    REMMINA_PROTOCOL_FEATURE_PREF_QUALITY,
    REMMINA_PROTOCOL_FEATURE_PREF_VIEWONLY,
    REMMINA_PROTOCOL_FEATURE_PREF_DISABLESERVERINPUT,
    REMMINA_PROTOCOL_FEATURE_TOOL,
    REMMINA_PROTOCOL_FEATURE_TOOL_CHAT,
    REMMINA_PROTOCOL_FEATURE_TOOL_SFTP,
    REMMINA_PROTOCOL_FEATURE_TOOL_SSHTERM,
    REMMINA_PROTOCOL_FEATURE_UNFOCUS,
    REMMINA_PROTOCOL_FEATURE_SCALE
} RemminaProtocolFeature;

typedef struct _RemminaProtocolWidgetClass RemminaProtocolWidgetClass;
typedef struct _RemminaProtocolWidget RemminaProtocolWidget;
typedef gboolean (*RemminaXPortTunnelInitFunc) (RemminaProtocolWidget *gp,
    gint remotedisplay, const gchar *server, gint port);

typedef enum
{
    REMMINA_PROTOCOL_SETTING_CTL_END,
    REMMINA_PROTOCOL_SETTING_CTL_CONCAT,
    REMMINA_PROTOCOL_SETTING_CTL_CONCAT_END,
    REMMINA_PROTOCOL_SETTING_SERVER,
    REMMINA_PROTOCOL_SETTING_PASSWORD,
    REMMINA_PROTOCOL_SETTING_RESOLUTION,
    REMMINA_PROTOCOL_SETTING_USERNAME,
    REMMINA_PROTOCOL_SETTING_DOMAIN,
    REMMINA_PROTOCOL_SETTING_COLORDEPTH,
    REMMINA_PROTOCOL_SETTING_SHAREFOLDER,
    REMMINA_PROTOCOL_SETTING_SOUND,
    REMMINA_PROTOCOL_SETTING_KEYMAP,
    REMMINA_PROTOCOL_SETTING_CLIENTNAME,
    REMMINA_PROTOCOL_SETTING_EXEC,
    REMMINA_PROTOCOL_SETTING_EXECPATH,
    REMMINA_PROTOCOL_SETTING_SHAREPRINTER,
    REMMINA_PROTOCOL_SETTING_CONSOLE,
    REMMINA_PROTOCOL_SETTING_BITMAPCACHING,
    REMMINA_PROTOCOL_SETTING_COMPRESSION,
    REMMINA_PROTOCOL_SETTING_ARGUMENTS,
    REMMINA_PROTOCOL_SETTING_LISTENPORT,
    REMMINA_PROTOCOL_SETTING_QUALITY,
    REMMINA_PROTOCOL_SETTING_SHOWCURSOR,
    REMMINA_PROTOCOL_SETTING_VIEWONLY,
    REMMINA_PROTOCOL_SETTING_DISABLESERVERINPUT,
    REMMINA_PROTOCOL_SETTING_SCALE,
    REMMINA_PROTOCOL_SETTING_GKEYMAP,
    REMMINA_PROTOCOL_SETTING_ONCE
} RemminaProtocolSetting;

G_END_DECLS

#endif /* __REMMINA_TYPES_H__ */

