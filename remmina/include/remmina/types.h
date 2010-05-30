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
    gchar *domain;
    gchar *clientname;
    gchar *resolution;
    gchar *keymap;
    gchar *gkeymap;
    gchar *exec;
    gchar *execpath;
    gchar *sound;
    gchar *arguments;
    gchar *proxy;
    gchar *sharefolder;
    gint colordepth;
    gint quality;
    gint listenport;
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
    gboolean disableencryption;
    gboolean disableclipboard;

    /* SSH settings */
    gboolean ssh_enabled;
    gboolean ssh_loopback;
    gchar *ssh_server;
    gint ssh_auth;
    gchar *ssh_username;
    gchar *ssh_privatekey;
    gchar *ssh_charset;

    /* Credential settings */
    gchar *username;
    gchar *password;
    gchar *cacert;
    gchar *cacrl;
    gchar *clientcert;
    gchar *clientkey;

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
    REMMINA_PROTOCOL_FEATURE_TOOL_REFRESH,
    REMMINA_PROTOCOL_FEATURE_TOOL_CHAT,
    REMMINA_PROTOCOL_FEATURE_TOOL_SFTP,
    REMMINA_PROTOCOL_FEATURE_TOOL_SSHTERM,
    REMMINA_PROTOCOL_FEATURE_UNFOCUS,
    REMMINA_PROTOCOL_FEATURE_SCALE
} RemminaProtocolFeature;

typedef struct _RemminaProtocolWidgetClass RemminaProtocolWidgetClass;
typedef struct _RemminaProtocolWidget RemminaProtocolWidget;
typedef gpointer RemminaTunnelInitFunc;
typedef gboolean (*RemminaXPortTunnelInitFunc) (RemminaProtocolWidget *gp,
    gint remotedisplay, const gchar *server, gint port);

typedef gpointer RemminaProtocolSetting;
#define REMMINA_PROTOCOL_SETTING_CTL_END                "0"
#define REMMINA_PROTOCOL_SETTING_CTL_CONCAT             "1"
#define REMMINA_PROTOCOL_SETTING_CTL_CONCAT_END         "2"
#define REMMINA_PROTOCOL_SETTING_SERVER                 "3"
#define REMMINA_PROTOCOL_SETTING_PASSWORD               "4"
#define REMMINA_PROTOCOL_SETTING_RESOLUTION_FIXED       "5"
#define REMMINA_PROTOCOL_SETTING_RESOLUTION_FLEXIBLE    "6"
#define REMMINA_PROTOCOL_SETTING_USERNAME               "7"
#define REMMINA_PROTOCOL_SETTING_DOMAIN                 "8"
#define REMMINA_PROTOCOL_SETTING_COLORDEPTH             "9"
#define REMMINA_PROTOCOL_SETTING_COLORDEPTH2            "10"
#define REMMINA_PROTOCOL_SETTING_SHAREFOLDER            "11"
#define REMMINA_PROTOCOL_SETTING_SOUND                  "12"
#define REMMINA_PROTOCOL_SETTING_KEYMAP                 "13"
#define REMMINA_PROTOCOL_SETTING_CLIENTNAME             "14"
#define REMMINA_PROTOCOL_SETTING_EXEC                   "15"
#define REMMINA_PROTOCOL_SETTING_EXEC_CUSTOM            "16"
#define REMMINA_PROTOCOL_SETTING_EXECPATH               "17"
#define REMMINA_PROTOCOL_SETTING_SHAREPRINTER           "18"
#define REMMINA_PROTOCOL_SETTING_CONSOLE                "19"
#define REMMINA_PROTOCOL_SETTING_BITMAPCACHING          "20"
#define REMMINA_PROTOCOL_SETTING_COMPRESSION            "21"
#define REMMINA_PROTOCOL_SETTING_ARGUMENTS              "22"
#define REMMINA_PROTOCOL_SETTING_LISTENPORT             "23"
#define REMMINA_PROTOCOL_SETTING_QUALITY                "24"
#define REMMINA_PROTOCOL_SETTING_SHOWCURSOR_REMOTE      "25"
#define REMMINA_PROTOCOL_SETTING_SHOWCURSOR_LOCAL       "26"
#define REMMINA_PROTOCOL_SETTING_VIEWONLY               "27"
#define REMMINA_PROTOCOL_SETTING_DISABLESERVERINPUT     "28"
#define REMMINA_PROTOCOL_SETTING_SCALE                  "29"
#define REMMINA_PROTOCOL_SETTING_GKEYMAP                "30"
#define REMMINA_PROTOCOL_SETTING_ONCE                   "31"
#define REMMINA_PROTOCOL_SETTING_DISABLEENCRYPTION      "32"
#define REMMINA_PROTOCOL_SETTING_SSH_PRIVATEKEY         "33"
#define REMMINA_PROTOCOL_SETTING_PROXY_DEST             "34"
#define REMMINA_PROTOCOL_SETTING_DISABLECLIPBOARD       "35"

#define REMMINA_PROTOCOL_SETTING_VALUE_CTL_END                0
#define REMMINA_PROTOCOL_SETTING_VALUE_CTL_CONCAT             1
#define REMMINA_PROTOCOL_SETTING_VALUE_CTL_CONCAT_END         2
#define REMMINA_PROTOCOL_SETTING_VALUE_SERVER                 3
#define REMMINA_PROTOCOL_SETTING_VALUE_PASSWORD               4
#define REMMINA_PROTOCOL_SETTING_VALUE_RESOLUTION_FIXED       5
#define REMMINA_PROTOCOL_SETTING_VALUE_RESOLUTION_FLEXIBLE    6
#define REMMINA_PROTOCOL_SETTING_VALUE_USERNAME               7
#define REMMINA_PROTOCOL_SETTING_VALUE_DOMAIN                 8
#define REMMINA_PROTOCOL_SETTING_VALUE_COLORDEPTH             9
#define REMMINA_PROTOCOL_SETTING_VALUE_COLORDEPTH2            10
#define REMMINA_PROTOCOL_SETTING_VALUE_SHAREFOLDER            11
#define REMMINA_PROTOCOL_SETTING_VALUE_SOUND                  12
#define REMMINA_PROTOCOL_SETTING_VALUE_KEYMAP                 13
#define REMMINA_PROTOCOL_SETTING_VALUE_CLIENTNAME             14
#define REMMINA_PROTOCOL_SETTING_VALUE_EXEC                   15
#define REMMINA_PROTOCOL_SETTING_VALUE_EXEC_CUSTOM            16
#define REMMINA_PROTOCOL_SETTING_VALUE_EXECPATH               17
#define REMMINA_PROTOCOL_SETTING_VALUE_SHAREPRINTER           18
#define REMMINA_PROTOCOL_SETTING_VALUE_CONSOLE                19
#define REMMINA_PROTOCOL_SETTING_VALUE_BITMAPCACHING          20
#define REMMINA_PROTOCOL_SETTING_VALUE_COMPRESSION            21
#define REMMINA_PROTOCOL_SETTING_VALUE_ARGUMENTS              22
#define REMMINA_PROTOCOL_SETTING_VALUE_LISTENPORT             23
#define REMMINA_PROTOCOL_SETTING_VALUE_QUALITY                24
#define REMMINA_PROTOCOL_SETTING_VALUE_SHOWCURSOR_REMOTE      25
#define REMMINA_PROTOCOL_SETTING_VALUE_SHOWCURSOR_LOCAL       26
#define REMMINA_PROTOCOL_SETTING_VALUE_VIEWONLY               27
#define REMMINA_PROTOCOL_SETTING_VALUE_DISABLESERVERINPUT     28
#define REMMINA_PROTOCOL_SETTING_VALUE_SCALE                  29
#define REMMINA_PROTOCOL_SETTING_VALUE_GKEYMAP                30
#define REMMINA_PROTOCOL_SETTING_VALUE_ONCE                   31
#define REMMINA_PROTOCOL_SETTING_VALUE_DISABLEENCRYPTION      32
#define REMMINA_PROTOCOL_SETTING_VALUE_SSH_PRIVATEKEY         33
#define REMMINA_PROTOCOL_SETTING_VALUE_PROXY_DEST             34
#define REMMINA_PROTOCOL_SETTING_VALUE_DISABLECLIPBOARD       35

typedef enum
{
    REMMINA_PROTOCOL_SSH_SETTING_NONE,
    REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,
    REMMINA_PROTOCOL_SSH_SETTING_SSH,
    REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL,
    REMMINA_PROTOCOL_SSH_SETTING_SFTP
} RemminaProtocolSSHSetting;

typedef enum
{
    REMMINA_AUTHPWD_TYPE_PROTOCOL,
    REMMINA_AUTHPWD_TYPE_SSH_PWD,
    REMMINA_AUTHPWD_TYPE_SSH_PRIVKEY
} RemminaAuthpwdType;

G_END_DECLS

#endif /* __REMMINA_TYPES_H__ */

