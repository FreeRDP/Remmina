/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
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

#include "rdp_plugin.h"
#include "rdp_ui.h"
#include "rdp_event.h"
#include "rdp_file.h"
#include "rdp_settings.h"

#include <errno.h>
#include <pthread.h>
#include <freerdp/utils/memory.h>

#define REMMINA_PLUGIN_RDP_FEATURE_TOOL_REFRESH            1
#define REMMINA_PLUGIN_RDP_FEATURE_SCALE                   2
#define REMMINA_PLUGIN_RDP_FEATURE_UNFOCUS                 3

RemminaPluginService *remmina_plugin_service = NULL;

static boolean
remmina_rdp_pre_connect (freerdp* instance)
{
    RemminaPluginRdpData *gpdata;
    RemminaProtocolWidget *gp;

    gpdata = (RemminaPluginRdpData*) instance->context;
    gp = gpdata->protocol_widget;

    remmina_rdp_ui_pre_connect (gp);
    remmina_rdp_event_pre_connect (gp);
    freerdp_channels_pre_connect (gpdata->channels, instance);

    return True;
}

static boolean
remmina_rdp_post_connect (freerdp* instance)
{
    RemminaPluginRdpData *gpdata;
    RemminaProtocolWidget *gp;

    gpdata = (RemminaPluginRdpData*) instance->context;
    gp = gpdata->protocol_widget;

    freerdp_channels_post_connect (gpdata->channels, instance);
    remmina_rdp_ui_post_connect (gp);
    remmina_rdp_event_post_connect (gp);
    remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");

    return True;
}

static boolean
remmina_rdp_authenticate (freerdp* instance, char** username, char** password, char** domain)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;
    gchar *s;
    gint ret;

    gpdata = (RemminaPluginRdpData*) instance->context;
    gp = gpdata->protocol_widget;

    THREADS_ENTER
    ret = remmina_plugin_service->protocol_plugin_init_authuserpwd (gp, TRUE);
    THREADS_LEAVE

    if (ret == GTK_RESPONSE_OK)
    {
        s = remmina_plugin_service->protocol_plugin_init_get_username (gp);
        if (s)
        {
            gpdata->settings->username = xstrdup (s);
            g_free (s);
        }
        s = remmina_plugin_service->protocol_plugin_init_get_password (gp);
        if (s)
        {
            gpdata->settings->password = xstrdup (s);
            g_free (s);
        }
        s = remmina_plugin_service->protocol_plugin_init_get_domain (gp);
        if (s)
        {
            gpdata->settings->domain = xstrdup (s);
            g_free (s);
        }
        return True;
    }
    else
    {
        gpdata->user_cancelled = TRUE;
        return False;
    }

    return True;
}

static boolean
remmina_rdp_verify_certificate(freerdp* instance, char* subject, char* issuer, char* fingerprint)
{
    /* TODO: popup dialog, ask for confirmation and store known_hosts file */
    printf("certificate details:\n");
    printf("  Subject:\n    %s\n", subject);
    printf("  Issued by:\n    %s\n", issuer);
    printf("  Fingerprint:\n    %s\n",  fingerprint);

    return True;
}

static int
remmina_rdp_receive_channel_data (freerdp* instance, int channelId, uint8* data, int size, int flags, int total_size)
{
    return freerdp_channels_data (instance, channelId, data, size, flags, total_size);
}

static void
remmina_rdp_main_loop (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    void *rfds[32];
    void *wfds[32];
    int rcount;
    int wcount;
    int i;
    int fds;
    int max_fds;
    fd_set rfds_set;
    fd_set wfds_set;

    memset (rfds, 0, sizeof(rfds));
    memset (wfds, 0, sizeof(wfds));

    gpdata = GET_DATA (gp);

    while (1)
    {
        rcount = 0;
        wcount = 0;

        if (!freerdp_get_fds (gpdata->instance, rfds, &rcount, wfds, &wcount))
        {
            break;
        }
        if (!freerdp_channels_get_fds (gpdata->channels, gpdata->instance, rfds, &rcount, wfds, &wcount))
        {
            break;
        }
        remmina_rdp_ui_get_fds (gp, rfds, &rcount);

        max_fds = 0;
        FD_ZERO (&rfds_set);
        for (i = 0; i < rcount; i++)
        {
            fds = (int) (uint64) (rfds[i]);
            if (fds > max_fds)
                max_fds = fds;
            FD_SET (fds, &rfds_set);
        }

        FD_ZERO (&wfds_set);
        for (i = 0; i < wcount; i++)
        {
            fds = GPOINTER_TO_INT (wfds[i]);
            if (fds > max_fds)
                max_fds = fds;
            FD_SET (fds, &wfds_set);
        }

        /* exit if nothing to do */
        if (max_fds == 0)
        {
            break;
        }

        /* do the wait */
        if (select (max_fds + 1, &rfds_set, &wfds_set, NULL, NULL) == -1)
        {
            /* these are not really errors */
            if (!((errno == EAGAIN) ||
                (errno == EWOULDBLOCK) ||
                (errno == EINPROGRESS) ||
                (errno == EINTR))) /* signal occurred */
            {
                break;
            }
        }

        /* check the libfreerdp fds */
        if (!freerdp_check_fds (gpdata->instance))
        {
            break;
        }
        /* check channel fds */
        if (!freerdp_channels_check_fds (gpdata->channels, gpdata->instance))
        {
            break;
        }
        /* check ui */
        if (!remmina_rdp_ui_check_fds (gp))
        {
            break;
        }
    }
}

static gboolean
remmina_rdp_main (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;
    gchar *host;
    gint port;
    gchar *s;
    gchar *value;
    gint rdpdr_num;
    gint drdynvc_num;
    gint rdpsnd_num;
    const gchar *cs;

    gpdata = GET_DATA (gp);
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    s = remmina_plugin_service->protocol_plugin_start_direct_tunnel (gp, 3389, FALSE);
    if (s == NULL)
    {
        return FALSE;
    }
    remmina_plugin_service->get_server_port (s, 3389, &host, &port);
    gpdata->settings->hostname = xstrdup (host);
    g_free (host);
    g_free (s);
    gpdata->settings->port = port;

    gpdata->settings->color_depth = remmina_plugin_service->file_get_int (remminafile, "colordepth", 8);
    gpdata->settings->width = remmina_plugin_service->file_get_int (remminafile, "resolution_width", 640);
    gpdata->settings->height = remmina_plugin_service->file_get_int (remminafile, "resolution_height", 480);
    remmina_plugin_service->protocol_plugin_set_width (gp, gpdata->settings->width);
    remmina_plugin_service->protocol_plugin_set_height (gp, gpdata->settings->height);

    if (remmina_plugin_service->file_get_string (remminafile, "username"))
    {
        gpdata->settings->username = xstrdup (remmina_plugin_service->file_get_string (remminafile, "username"));
    }

    if (remmina_plugin_service->file_get_string (remminafile, "domain"))
    {
        gpdata->settings->domain = xstrdup (remmina_plugin_service->file_get_string (remminafile, "domain"));
    }

    THREADS_ENTER
    s = remmina_plugin_service->file_get_secret (remminafile, "password");
    THREADS_LEAVE
    if (s)
    {
        gpdata->settings->password = xstrdup (s);
        gpdata->settings->autologon = 1;
        g_free (s);
    }

    if (remmina_plugin_service->file_get_string (remminafile, "clientname"))
    {
        strncpy (gpdata->settings->client_hostname, remmina_plugin_service->file_get_string (remminafile, "clientname"),
            sizeof (gpdata->settings->client_hostname) - 1);
    }
    else
    {
        strncpy (gpdata->settings->client_hostname, g_get_host_name (), sizeof (gpdata->settings->client_hostname) - 1);
    }

    if (remmina_plugin_service->file_get_string (remminafile, "exec"))
    {
        gpdata->settings->shell = xstrdup (remmina_plugin_service->file_get_string (remminafile, "exec"));
    }

    if (remmina_plugin_service->file_get_string (remminafile, "execpath"))
    {
        gpdata->settings->directory = xstrdup (remmina_plugin_service->file_get_string (remminafile, "execpath"));
    }

    s = g_strdup_printf ("rdp_quality_%i", remmina_plugin_service->file_get_int (remminafile, "quality", DEFAULT_QUALITY_0));
    value = remmina_plugin_service->pref_get_value (s);
    g_free (s);
    if (value && value[0])
    {
        gpdata->settings->performance_flags = strtoul (value, NULL, 16);
    }
    else
    {
        switch (remmina_plugin_service->file_get_int (remminafile, "quality", DEFAULT_QUALITY_0))
        {
        case 9:
            gpdata->settings->performance_flags = DEFAULT_QUALITY_9;
            break;
        case 2:
            gpdata->settings->performance_flags = DEFAULT_QUALITY_2;
            break;
        case 1:
            gpdata->settings->performance_flags = DEFAULT_QUALITY_1;
            break;
        case 0:
        default:
            gpdata->settings->performance_flags = DEFAULT_QUALITY_0;
            break;
        }
    }
    g_free (value);

    gpdata->settings->kbd_layout = remmina_rdp_settings_get_keyboard_layout();

    if (remmina_plugin_service->file_get_int (remminafile, "console", FALSE))
    {
        gpdata->settings->console_session = True;
    }

    cs = remmina_plugin_service->file_get_string (remminafile, "security");
    if (g_strcmp0 (cs, "rdp") == 0)
    {
        gpdata->settings->rdp_security = True;
        gpdata->settings->tls_security = False;
        gpdata->settings->nla_security = False;
    }
    else if (g_strcmp0 (cs, "tls") == 0)
    {
        gpdata->settings->rdp_security = False;
        gpdata->settings->tls_security = True;
        gpdata->settings->nla_security = False;
    }
    else if (g_strcmp0 (cs, "nla") == 0)
    {
        gpdata->settings->rdp_security = False;
        gpdata->settings->tls_security = False;
        gpdata->settings->nla_security = True;
    }

    gpdata->settings->compression = True;
    gpdata->settings->fastpath_input = True;
    gpdata->settings->fastpath_output = True;

    drdynvc_num = 0;
    rdpsnd_num = 0;
    cs = remmina_plugin_service->file_get_string (remminafile, "sound");
    if (g_strcmp0 (cs, "remote") == 0)
    {
        gpdata->settings->console_audio = 1;
    }
    else if (g_str_has_prefix (cs, "local"))
    {
        cs = strchr (cs, ',');
        if (cs)
        {
            snprintf (gpdata->rdpsnd_options, sizeof (gpdata->rdpsnd_options), "%s", cs + 1);
            s = strchr (gpdata->rdpsnd_options, ',');
            if (s) *s++ = '\0';

            gpdata->rdpsnd_data[rdpsnd_num].size = sizeof(RDP_PLUGIN_DATA);
            gpdata->rdpsnd_data[rdpsnd_num].data[0] = "rate";
            gpdata->rdpsnd_data[rdpsnd_num].data[1] = gpdata->rdpsnd_options;
            rdpsnd_num++;

            if (s)
            {
                gpdata->rdpsnd_data[rdpsnd_num].size = sizeof(RDP_PLUGIN_DATA);
                gpdata->rdpsnd_data[rdpsnd_num].data[0] = "channel";
                gpdata->rdpsnd_data[rdpsnd_num].data[1] = s;
                rdpsnd_num++;
            }
        }

        freerdp_channels_load_plugin (gpdata->channels, gpdata->settings, "rdpsnd", gpdata->rdpsnd_data);

        gpdata->drdynvc_data[drdynvc_num].size = sizeof(RDP_PLUGIN_DATA);
        gpdata->drdynvc_data[drdynvc_num].data[0] = "audin";
        drdynvc_num++;
    }
    if (drdynvc_num)
    {
        freerdp_channels_load_plugin (gpdata->channels, gpdata->settings, "drdynvc", gpdata->drdynvc_data);
    }

    if (!remmina_plugin_service->file_get_int (remminafile, "disableclipboard", FALSE))
    {
        freerdp_channels_load_plugin (gpdata->channels, gpdata->settings, "cliprdr", NULL);
    }

    rdpdr_num = 0;
    cs = remmina_plugin_service->file_get_string (remminafile, "sharefolder");
    if (cs && cs[0] == '/')
    {
        s = strrchr (cs, '/');
        s = (s && s[1] ? s + 1 : "root");
        gpdata->rdpdr_data[rdpdr_num].size = sizeof(RDP_PLUGIN_DATA);
        gpdata->rdpdr_data[rdpdr_num].data[0] = "disk";
        gpdata->rdpdr_data[rdpdr_num].data[1] = s;
        gpdata->rdpdr_data[rdpdr_num].data[2] = (gchar*) cs;
        rdpdr_num++;
    }
    if (remmina_plugin_service->file_get_int (remminafile, "shareprinter", FALSE))
    {
        gpdata->rdpdr_data[rdpdr_num].size = sizeof(RDP_PLUGIN_DATA);
        gpdata->rdpdr_data[rdpdr_num].data[0] = "printer";
        rdpdr_num++;
    }
    if (rdpdr_num)
    {
        freerdp_channels_load_plugin (gpdata->channels, gpdata->settings, "rdpdr", gpdata->rdpdr_data);
    }

    if (!freerdp_connect(gpdata->instance))
    {
        if (!gpdata->user_cancelled)
        {
            remmina_plugin_service->protocol_plugin_set_error (gp, _("Unable to connect to RDP server %s"),
                gpdata->settings->hostname);
        }
        return FALSE;
    }

    remmina_rdp_main_loop (gp);

    return TRUE;
}

static gpointer
remmina_rdp_main_thread (gpointer data)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    CANCEL_ASYNC
    gp = (RemminaProtocolWidget*) data;
    gpdata = GET_DATA (gp);
    remmina_rdp_main (gp);
    gpdata->thread = 0;
    IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);
    return NULL;
}

static void
remmina_rdp_init (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    freerdp* instance;

    instance = freerdp_new ();
    instance->PreConnect = remmina_rdp_pre_connect;
    instance->PostConnect = remmina_rdp_post_connect;
    instance->Authenticate = remmina_rdp_authenticate;
    instance->VerifyCertificate = remmina_rdp_verify_certificate;
    instance->ReceiveChannelData = remmina_rdp_receive_channel_data;

    instance->context_size = sizeof (RemminaPluginRdpData);
    freerdp_context_new (instance);
    gpdata = (RemminaPluginRdpData*) instance->context;

    g_object_set_data_full (G_OBJECT (gp), "plugin-data", gpdata, xfree);

    gpdata->protocol_widget = gp;
    gpdata->instance = instance;
    gpdata->settings = instance->settings;
    gpdata->channels = freerdp_channels_new ();

    pthread_mutex_init (&gpdata->mutex, NULL);

    remmina_rdp_event_init (gp);
    remmina_rdp_ui_init (gp);
}

static gboolean
remmina_rdp_open_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    gpdata->scale = remmina_plugin_service->protocol_plugin_get_scale (gp);

    if (pthread_create (&gpdata->thread, NULL, remmina_rdp_main_thread, gp))
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "%s",
            "Failed to initialize pthread. Falling back to non-thread mode...");
        gpdata->thread = 0;
        return FALSE;
    }
    return TRUE;
}

static gboolean
remmina_rdp_close_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);

    if (gpdata->thread)
    {
        pthread_cancel (gpdata->thread);
        if (gpdata->thread) pthread_join (gpdata->thread, NULL);
    }

    if (gpdata->channels && gpdata->instance)
    {
        freerdp_channels_close (gpdata->channels, gpdata->instance);
    }
    if (gpdata->instance)
    {
        freerdp_disconnect (gpdata->instance);
        freerdp_free (gpdata->instance);
        gpdata->instance = NULL;
    }
    if (gpdata->channels)
    {
        freerdp_channels_free (gpdata->channels);
        gpdata->channels = NULL;
    }
    pthread_mutex_destroy (&gpdata->mutex);

    remmina_rdp_event_uninit (gp);
    remmina_rdp_ui_uninit (gp);

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
    return FALSE;
}

static gboolean
remmina_rdp_query_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
    return TRUE;
}

static void
remmina_rdp_call_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;

    gpdata = GET_DATA (gp);
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);
    switch (feature->id)
    {
        case REMMINA_PLUGIN_RDP_FEATURE_UNFOCUS:
            remmina_rdp_event_unfocus (gp);
            break;
        case REMMINA_PLUGIN_RDP_FEATURE_SCALE:
            gpdata->scale = remmina_plugin_service->file_get_int (remminafile, "scale", FALSE);
            remmina_rdp_event_update_scale (gp);
            break;
        case REMMINA_PLUGIN_RDP_FEATURE_TOOL_REFRESH:
            gtk_widget_queue_draw_area (gpdata->drawing_area,
                0, 0,
                remmina_plugin_service->protocol_plugin_get_width (gp),
                remmina_plugin_service->protocol_plugin_get_height (gp));
            break;
        default:
            break;
    }
}

static gpointer colordepth_list[] =
{
    "8", N_("256 colors"),
    "15", N_("High color (15 bit)"),
    "16", N_("High color (16 bit)"),
    "24", N_("True color (24 bit)"),
    "32", N_("RemoteFX (32 bit)"),
    NULL
};

static gpointer quality_list[] =
{
    "0", N_("Poor (fastest)"),
    "1", N_("Medium"),
    "2", N_("Good"),
    "9", N_("Best (slowest)"),
    NULL
};

static gpointer sound_list[] =
{
    "off", N_("Off"),
    "local", N_("Local"),
    "local,11025,1", N_("Local - low quality"),
    "local,22050,2", N_("Local - medium quality"),
    "local,44100,2", N_("Local - high quality"),
    "remote", N_("Remote"),
    NULL
};

static gpointer security_list[] =
{
    "", N_("Negotiate"),
    "nla", "NLA",
    "tls", "TLS",
    "rdp", "RDP",
    NULL
};

static const RemminaProtocolSetting remmina_rdp_basic_settings[] =
{
    { REMMINA_PROTOCOL_SETTING_TYPE_SERVER, NULL, NULL, FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "username", N_("User name"), FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, NULL, NULL, FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "domain", N_("Domain"), FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION, NULL, NULL, FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "colordepth", N_("Color depth"), FALSE, colordepth_list, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_FOLDER, "sharefolder", N_("Share folder"), FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

static const RemminaProtocolSetting remmina_rdp_advanced_settings[] =
{
    { REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "quality", N_("Quality"), FALSE, quality_list, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "sound", N_("Sound"), FALSE, sound_list, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "security", N_("Security"), FALSE, security_list, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "clientname", N_("Client name"), FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "exec", N_("Startup program"), FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_TEXT, "execpath", N_("Startup path"), FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "shareprinter", N_("Share local printers"), TRUE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard", N_("Disable clipboard sync"), FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "console", N_("Attach to console (Windows 2003 / 2003 R2)"), FALSE, NULL, NULL },
    { REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

static const RemminaProtocolFeature remmina_rdp_features[] =
{
    { REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_RDP_FEATURE_TOOL_REFRESH, N_("Refresh"), GTK_STOCK_REFRESH, NULL },
    { REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, REMMINA_PLUGIN_RDP_FEATURE_SCALE, NULL, NULL, NULL },
    { REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, REMMINA_PLUGIN_RDP_FEATURE_UNFOCUS, NULL, NULL, NULL },
    { REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

static RemminaProtocolPlugin remmina_rdp =
{
    REMMINA_PLUGIN_TYPE_PROTOCOL,
    "RDP",
    N_("RDP - Windows Terminal Service"),
    GETTEXT_PACKAGE,
    VERSION,

    "remmina-rdp",
    "remmina-rdp-ssh",
    remmina_rdp_basic_settings,
    remmina_rdp_advanced_settings,
    REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,
    remmina_rdp_features,

    remmina_rdp_init,
    remmina_rdp_open_connection,
    remmina_rdp_close_connection,
    remmina_rdp_query_feature,
    remmina_rdp_call_feature
};

static RemminaFilePlugin remmina_rdpf =
{
    REMMINA_PLUGIN_TYPE_FILE,
    "RDPF",
    N_("RDP - RDP File Handler"),
    GETTEXT_PACKAGE,
    VERSION,

    remmina_rdp_file_import_test,
    remmina_rdp_file_import,
    remmina_rdp_file_export_test,
    remmina_rdp_file_export,
    NULL
};

static RemminaPrefPlugin remmina_rdps =
{
    REMMINA_PLUGIN_TYPE_PREF,
    "RDPS",
    N_("RDP - Preferences"),
    GETTEXT_PACKAGE,
    VERSION,

    "RDP",
    remmina_rdp_settings_new
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry (RemminaPluginService *service)
{
    remmina_plugin_service = service;

    bindtextdomain (GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    if (! service->register_plugin ((RemminaPlugin *) &remmina_rdp))
    {
        return FALSE;
    }
    remmina_rdpf.export_hints = _("Export connection in Windows .rdp file format");
    if (! service->register_plugin ((RemminaPlugin *) &remmina_rdpf))
    {
        return FALSE;
    }
    if (! service->register_plugin ((RemminaPlugin *) &remmina_rdps))
    {
        return FALSE;
    }

    freerdp_channels_global_init ();
    remmina_rdp_settings_init ();

    return TRUE;
}

