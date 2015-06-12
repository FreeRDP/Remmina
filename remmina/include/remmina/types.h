/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINA_TYPES_H__
#define __REMMINA_TYPES_H__

G_BEGIN_DECLS

typedef struct _RemminaFile RemminaFile;

typedef enum
{
    REMMINA_PROTOCOL_FEATURE_TYPE_END,
    REMMINA_PROTOCOL_FEATURE_TYPE_PREF,
    REMMINA_PROTOCOL_FEATURE_TYPE_TOOL,
    REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS,
    REMMINA_PROTOCOL_FEATURE_TYPE_SCALE
} RemminaProtocolFeatureType;

#define REMMINA_PROTOCOL_FEATURE_PREF_RADIO 1
#define REMMINA_PROTOCOL_FEATURE_PREF_CHECK 2

typedef struct _RemminaProtocolFeature
{
    RemminaProtocolFeatureType type;
    gint id;
    gpointer opt1;
    gpointer opt2;
    gpointer opt3;
} RemminaProtocolFeature;

typedef struct _RemminaProtocolWidgetClass RemminaProtocolWidgetClass;
typedef struct _RemminaProtocolWidget RemminaProtocolWidget;
typedef gpointer RemminaTunnelInitFunc;
typedef gboolean (*RemminaXPortTunnelInitFunc) (RemminaProtocolWidget *gp,
    gint remotedisplay, const gchar *server, gint port);

typedef enum
{
    REMMINA_PROTOCOL_SETTING_TYPE_END,

    REMMINA_PROTOCOL_SETTING_TYPE_SERVER,
    REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD,
    REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION,
    REMMINA_PROTOCOL_SETTING_TYPE_KEYMAP,

    REMMINA_PROTOCOL_SETTING_TYPE_TEXT,
    REMMINA_PROTOCOL_SETTING_TYPE_SELECT,
    REMMINA_PROTOCOL_SETTING_TYPE_COMBO,
    REMMINA_PROTOCOL_SETTING_TYPE_CHECK,
    REMMINA_PROTOCOL_SETTING_TYPE_FILE,
    REMMINA_PROTOCOL_SETTING_TYPE_FOLDER
} RemminaProtocolSettingType;

typedef struct _RemminaProtocolSetting
{
    RemminaProtocolSettingType type;
    const gchar *name;
    const gchar *label;
    gboolean compact;
    const gpointer opt1;
    const gpointer opt2;
} RemminaProtocolSetting;

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

