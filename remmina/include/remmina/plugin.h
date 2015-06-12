/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINA_PLUGIN_H__
#define __REMMINA_PLUGIN_H__

#include <remmina/types.h>
#include "remmina/remmina_trace_calls.h"

G_BEGIN_DECLS

typedef enum
{
    REMMINA_PLUGIN_TYPE_PROTOCOL = 0,
    REMMINA_PLUGIN_TYPE_ENTRY = 1,
    REMMINA_PLUGIN_TYPE_FILE = 2,
    REMMINA_PLUGIN_TYPE_TOOL = 3,
    REMMINA_PLUGIN_TYPE_PREF = 4,
    REMMINA_PLUGIN_TYPE_SECRET = 5
} RemminaPluginType;

typedef struct _RemminaPlugin
{
    RemminaPluginType type;
    const gchar *name;
    const gchar *description;
    const gchar *domain;
    const gchar *version;
} RemminaPlugin;

typedef struct _RemminaProtocolPlugin
{
    RemminaPluginType type;
    const gchar *name;
    const gchar *description;
    const gchar *domain;
    const gchar *version;

    const gchar *icon_name;
    const gchar *icon_name_ssh;
    const RemminaProtocolSetting *basic_settings;
    const RemminaProtocolSetting *advanced_settings;
    RemminaProtocolSSHSetting ssh_setting;
    const RemminaProtocolFeature *features;

    void (* init) (RemminaProtocolWidget *gp);
    gboolean (* open_connection) (RemminaProtocolWidget *gp);
    gboolean (* close_connection) (RemminaProtocolWidget *gp);
    gboolean (* query_feature) (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
    void (* call_feature) (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature);
    void (* send_keystrokes) (RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen);
} RemminaProtocolPlugin;

typedef struct _RemminaEntryPlugin
{
    RemminaPluginType type;
    const gchar *name;
    const gchar *description;
    const gchar *domain;
    const gchar *version;

    void (* entry_func) (void);
} RemminaEntryPlugin;

typedef struct _RemminaFilePlugin
{
    RemminaPluginType type;
    const gchar *name;
    const gchar *description;
    const gchar *domain;
    const gchar *version;

    gboolean (* import_test_func) (const gchar *from_file);
    RemminaFile* (* import_func) (const gchar *from_file);
    gboolean (* export_test_func) (RemminaFile *file);
    gboolean (* export_func) (RemminaFile *file, const gchar *to_file);
    const gchar *export_hints;
} RemminaFilePlugin;

typedef struct _RemminaToolPlugin
{
    RemminaPluginType type;
    const gchar *name;
    const gchar *description;
    const gchar *domain;
    const gchar *version;

    void (* exec_func) (void);
} RemminaToolPlugin;

typedef struct _RemminaPrefPlugin
{
    RemminaPluginType type;
    const gchar *name;
    const gchar *description;
    const gchar *domain;
    const gchar *version;

    const gchar *pref_label;
    GtkWidget* (* get_pref_body) (void);
} RemminaPrefPlugin;

typedef struct _RemminaSecretPlugin
{
    RemminaPluginType type;
    const gchar *name;
    const gchar *description;
    const gchar *domain;
    const gchar *version;

    gboolean trusted;
    void (* store_password) (RemminaFile *remminafile, const gchar *key, const gchar *password);
    gchar* (* get_password) (RemminaFile *remminafile, const gchar *key);
    void (* delete_password) (RemminaFile *remminafile, const gchar *key);
} RemminaSecretPlugin;

/* Plugin Service is a struct containing a list of function pointers,
 * which is passed from Remmina main program to the plugin module
 * through the plugin entry function remmina_plugin_entry() */
typedef struct _RemminaPluginService
{
    gboolean     (* register_plugin)                      (RemminaPlugin *plugin);

    gint         (* protocol_plugin_get_width)            (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_set_width)            (RemminaProtocolWidget *gp, gint width);
    gint         (* protocol_plugin_get_height)           (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_set_height)           (RemminaProtocolWidget *gp, gint height);
    gboolean     (* protocol_plugin_get_scale)            (RemminaProtocolWidget *gp);
    gboolean     (* protocol_plugin_get_expand)           (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_set_expand)           (RemminaProtocolWidget *gp, gboolean expand);
    gboolean     (* protocol_plugin_has_error)            (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_set_error)            (RemminaProtocolWidget *gp, const gchar *fmt, ...);
    gboolean     (* protocol_plugin_is_closed)            (RemminaProtocolWidget *gp);
    RemminaFile* (* protocol_plugin_get_file)             (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_emit_signal)          (RemminaProtocolWidget *gp, const gchar *signal_name);
    void         (* protocol_plugin_register_hostkey)     (RemminaProtocolWidget *gp, GtkWidget *widget);
    gchar*       (* protocol_plugin_start_direct_tunnel)  (RemminaProtocolWidget *gp, gint default_port, gboolean port_plus);
    gboolean     (* protocol_plugin_start_reverse_tunnel) (RemminaProtocolWidget *gp, gint local_port);
    gboolean     (* protocol_plugin_start_xport_tunnel)   (RemminaProtocolWidget *gp, RemminaXPortTunnelInitFunc init_func);
    void         (* protocol_plugin_set_display)          (RemminaProtocolWidget *gp, gint display);
    gboolean     (* protocol_plugin_close_connection)     (RemminaProtocolWidget *gp);
    gint         (* protocol_plugin_init_authpwd)         (RemminaProtocolWidget *gp, RemminaAuthpwdType authpwd_type, gboolean allow_password_saving);
    gint         (* protocol_plugin_init_authuserpwd)     (RemminaProtocolWidget *gp, gboolean want_domain, gboolean allow_password_saving);
    gint         (* protocol_plugin_init_certificate)     (RemminaProtocolWidget *gp, const gchar* subject, const gchar* issuer, const gchar* fingerprint);
    gint         (* protocol_plugin_changed_certificate)  (RemminaProtocolWidget *gp, const gchar* subject, const gchar* issuer, const gchar* new_fingerprint, const gchar* old_fingerprint);
    gchar*       (* protocol_plugin_init_get_username)    (RemminaProtocolWidget *gp);
    gchar*       (* protocol_plugin_init_get_password)    (RemminaProtocolWidget *gp);
    gchar*       (* protocol_plugin_init_get_domain)      (RemminaProtocolWidget *gp);
    gboolean     (* protocol_plugin_init_get_savepassword) (RemminaProtocolWidget *gp);
    gint         (* protocol_plugin_init_authx509)        (RemminaProtocolWidget *gp);
    gchar*       (* protocol_plugin_init_get_cacert)      (RemminaProtocolWidget *gp);
    gchar*       (* protocol_plugin_init_get_cacrl)       (RemminaProtocolWidget *gp);
    gchar*       (* protocol_plugin_init_get_clientcert)  (RemminaProtocolWidget *gp);
    gchar*       (* protocol_plugin_init_get_clientkey)   (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_init_save_cred)       (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_init_show_listen)     (RemminaProtocolWidget *gp, gint port);
    void         (* protocol_plugin_init_show_retry)      (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_init_show)            (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_init_hide)            (RemminaProtocolWidget *gp);
    gboolean     (* protocol_plugin_ssh_exec)             (RemminaProtocolWidget *gp, gboolean wait, const gchar *fmt, ...);
    void         (* protocol_plugin_chat_open)            (RemminaProtocolWidget *gp, const gchar *name,
                                                           void(*on_send)(RemminaProtocolWidget *gp, const gchar *text),
                                                           void(*on_destroy)(RemminaProtocolWidget *gp));
    void         (* protocol_plugin_chat_close)           (RemminaProtocolWidget *gp);
    void         (* protocol_plugin_chat_receive)         (RemminaProtocolWidget *gp, const gchar *text);
    void         (* protocol_plugin_send_keys_signals)    (GtkWidget *widget, const guint *keyvals, int length, GdkEventType action);

    RemminaFile* (* file_new)                             (void);
    const gchar* (* file_get_path)                        (RemminaFile *remminafile);
    void         (* file_set_string)                      (RemminaFile *remminafile, const gchar *setting, const gchar *value);
    const gchar* (* file_get_string)                      (RemminaFile *remminafile, const gchar *setting);
    gchar*       (* file_get_secret)                      (RemminaFile *remminafile, const gchar *setting);
    void         (* file_set_int)                         (RemminaFile *remminafile, const gchar *setting, gint value);
    gint         (* file_get_int)                         (RemminaFile *remminafile, const gchar *setting, gint default_value);
    void         (* file_unsave_password)                 (RemminaFile *remminafile);

    void         (* pref_set_value)                       (const gchar *key, const gchar *value);
    gchar*       (* pref_get_value)                       (const gchar *key);
    gint         (* pref_get_scale_quality)               (void);
    gint         (* pref_get_sshtunnel_port)              (void);
    guint        (* pref_keymap_get_keyval)               (const gchar *keymap, guint keyval);

    void         (* log_print)                            (const gchar *text);
    void         (* log_printf)                           (const gchar *fmt, ...);

    void         (* ui_register)                          (GtkWidget *widget);

    GtkWidget*   (* open_connection)                      (RemminaFile *remminafile, GCallback disconnect_cb, gpointer data, guint *handler);
    void         (* get_server_port)                      (const gchar *server, gint defaultport, gchar **host, gint *port);
    gboolean     (* is_main_thread)                       (void);

} RemminaPluginService;

/* "Prototype" of the plugin entry function */
typedef gboolean (*RemminaPluginEntryFunc) (RemminaPluginService *service);

G_END_DECLS

#endif /* __REMMINA_PLUGIN_H__ */

