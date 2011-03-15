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

#include "remminapluginrdp.h"
#include "remminapluginrdpui.h"
#include "remminapluginrdpev.h"
#include "remminapluginrdpfile.h"
#include "remminapluginrdpset.h"

#define REMMINA_PLUGIN_RDP_FEATURE_TOOL_REFRESH            1
#define REMMINA_PLUGIN_RDP_FEATURE_SCALE                   2
#define REMMINA_PLUGIN_RDP_FEATURE_UNFOCUS                 3

RemminaPluginService *remmina_plugin_service = NULL;

static void
remmina_plugin_rdp_main_loop (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    void *read_fds[32];
    void *write_fds[32];
    int read_count;
    int write_count;
    int index;
    int sck;
    int max_sck;
    fd_set rfds;
    fd_set wfds;

    gpdata = GET_DATA (gp);
    while (1)
    {
        read_count = 0;
        write_count = 0;
        /* get libfreerdp fds */
        if (gpdata->inst->rdp_get_fds (gpdata->inst, read_fds, &read_count, write_fds, &write_count) != 0)
        {
            gpdata->inst->ui_error (gpdata->inst, "inst->rdp_get_fds failed\n");
            break;
        }
        /* get channel fds */
        if (freerdp_chanman_get_fds (gpdata->chan_man, gpdata->inst, read_fds, &read_count, write_fds, &write_count) != 0)
        {
            gpdata->inst->ui_error (gpdata->inst, "freerdp_chanman_get_fds failed\n");
            break;
        }
        remmina_plugin_rdpui_get_fds (gp, read_fds, &read_count);
        max_sck = 0;
        FD_ZERO (&rfds);
        for (index = 0; index < read_count; index++)
        {
            sck = (int) (read_fds[index]);
            if (sck > max_sck)
                max_sck = sck;
            FD_SET (sck, &rfds);
        }
        /* setup write fds */
        FD_ZERO (&wfds);
        for (index = 0; index < write_count; index++)
        {
            sck = (int) (write_fds[index]);
            if (sck > max_sck)
                max_sck = sck;
            FD_SET (sck, &wfds);
        }
        /* exit if nothing to do */
        if (max_sck == 0)
        {
            gpdata->inst->ui_error (gpdata->inst, "max_sck is zero\n");
            break;
        }
        /* do the wait */
        if (select (max_sck + 1, &rfds, &wfds, NULL, NULL) == -1)
        {
            /* these are not really errors */
            if (!((errno == EAGAIN) ||
                (errno == EWOULDBLOCK) ||
                (errno == EINPROGRESS) ||
                (errno == EINTR))) /* signal occurred */
            {
                gpdata->inst->ui_error (gpdata->inst, "select failed\n");
                break;
            }
        }
        /* check the libfreerdp fds */
        if (gpdata->inst->rdp_check_fds (gpdata->inst) != 0)
        {
            gpdata->inst->ui_error (gpdata->inst, "inst->rdp_check_fds failed\n");
            break;
        }
        /* check channel fds */
        if (freerdp_chanman_check_fds (gpdata->chan_man, gpdata->inst) != 0)
        {
            gpdata->inst->ui_error (gpdata->inst, "freerdp_chanman_check_fds failed\n");
            break;
        }
        /* check ui */
        if (remmina_plugin_rdpui_check_fds (gp) != 0)
        {
            gpdata->inst->ui_error (gpdata->inst, "remmina_plugin_rdpui_check_fds failed\n");
            break;
        }
    }
}

static gboolean
remmina_plugin_rdp_main (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;
    gchar *host;
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
    remmina_plugin_service->get_server_port (s, 3389, &host, &gpdata->settings->tcp_port_rdp);
    strncpy (gpdata->settings->server, host, sizeof (gpdata->settings->server) - 1);
    g_free (host);
    g_free (s);

    gpdata->settings->server_depth = remmina_plugin_service->file_get_int (remminafile, "colordepth", 8);
    gpdata->settings->width = remmina_plugin_service->file_get_int (remminafile, "resolution_width", 640);
    gpdata->settings->height = remmina_plugin_service->file_get_int (remminafile, "resolution_height", 480);
    remmina_plugin_service->protocol_plugin_set_width (gp, gpdata->settings->width);
    remmina_plugin_service->protocol_plugin_set_height (gp, gpdata->settings->height);

    if (remmina_plugin_service->file_get_string (remminafile, "username"))
    {
        strncpy (gpdata->settings->username, remmina_plugin_service->file_get_string (remminafile, "username"),
            sizeof (gpdata->settings->username) - 1);
    }

    if (remmina_plugin_service->file_get_string (remminafile, "domain"))
    {
        strncpy (gpdata->settings->domain, remmina_plugin_service->file_get_string (remminafile, "domain"),
            sizeof (gpdata->settings->domain) - 1);
    }

    THREADS_ENTER
    s = remmina_plugin_service->file_get_secret (remminafile, "password");
    THREADS_LEAVE
    if (s)
    {
        strncpy (gpdata->settings->password, s, sizeof (gpdata->settings->password) - 1);
        gpdata->settings->autologin = 1;
        g_free (s);
    }

    if (remmina_plugin_service->file_get_string (remminafile, "clientname"))
    {
        strncpy (gpdata->settings->hostname, remmina_plugin_service->file_get_string (remminafile, "clientname"),
            sizeof (gpdata->settings->hostname) - 1);
    }
    else
    {
        strncpy (gpdata->settings->hostname, g_get_host_name (), sizeof (gpdata->settings->hostname) - 1);
    }

    if (remmina_plugin_service->file_get_string (remminafile, "exec"))
    {
        strncpy (gpdata->settings->shell, remmina_plugin_service->file_get_string (remminafile, "exec"),
            sizeof (gpdata->settings->shell) - 1);
    }

    if (remmina_plugin_service->file_get_string (remminafile, "execpath"))
    {
        strncpy (gpdata->settings->directory, remmina_plugin_service->file_get_string (remminafile, "execpath"),
            sizeof (gpdata->settings->directory) - 1);
    }

    s = g_strdup_printf ("rdp_quality_%i", remmina_plugin_service->file_get_int (remminafile, "quality", DEFAULT_QUALITY_0));
    value = remmina_plugin_service->pref_get_value (s);
    g_free (s);
    if (value && value[0])
    {
        gpdata->settings->performanceflags = strtoul (value, NULL, 16);
    }
    else
    {
        switch (remmina_plugin_service->file_get_int (remminafile, "quality", DEFAULT_QUALITY_0))
        {
        case 9:
            gpdata->settings->performanceflags = DEFAULT_QUALITY_9;
            break;
        case 2:
            gpdata->settings->performanceflags = DEFAULT_QUALITY_2;
            break;
        case 1:
            gpdata->settings->performanceflags = DEFAULT_QUALITY_1;
            break;
        case 0:
        default:
            gpdata->settings->performanceflags = DEFAULT_QUALITY_0;
            break;
        }
    }
    g_free (value);

    gpdata->settings->keyboard_layout = remmina_plugin_rdpset_get_keyboard_layout();

    if (remmina_plugin_service->file_get_int (remminafile, "console", FALSE))
    {
        gpdata->settings->console_session = 1;
    }

    cs = remmina_plugin_service->file_get_string (remminafile, "security");
    if (g_strcmp0 (cs, "rdp") == 0)
    {
        gpdata->settings->rdp_security = 1;
        gpdata->settings->tls_security = 0;
        gpdata->settings->nla_security = 0;
    }
    else if (g_strcmp0 (cs, "tls") == 0)
    {
        gpdata->settings->rdp_security = 0;
        gpdata->settings->tls_security = 1;
        gpdata->settings->nla_security = 0;
    }
    else if (g_strcmp0 (cs, "nla") == 0)
    {
        gpdata->settings->rdp_security = 0;
        gpdata->settings->tls_security = 0;
        gpdata->settings->nla_security = 1;
    }

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

            gpdata->rdpsnd_data[rdpsnd_num].size = sizeof(RD_PLUGIN_DATA);
            gpdata->rdpsnd_data[rdpsnd_num].data[0] = "rate";
            gpdata->rdpsnd_data[rdpsnd_num].data[1] = gpdata->rdpsnd_options;
            rdpsnd_num++;

            if (s)
            {
                gpdata->rdpsnd_data[rdpsnd_num].size = sizeof(RD_PLUGIN_DATA);
                gpdata->rdpsnd_data[rdpsnd_num].data[0] = "channel";
                gpdata->rdpsnd_data[rdpsnd_num].data[1] = s;
                rdpsnd_num++;
            }
        }

        freerdp_chanman_load_plugin (gpdata->chan_man, gpdata->settings, "rdpsnd", gpdata->rdpsnd_data);

        gpdata->drdynvc_data[drdynvc_num].size = sizeof(RD_PLUGIN_DATA);
        gpdata->drdynvc_data[drdynvc_num].data[0] = "audin";
        drdynvc_num++;
    }
    if (drdynvc_num)
    {
        freerdp_chanman_load_plugin (gpdata->chan_man, gpdata->settings, "drdynvc", gpdata->drdynvc_data);
    }

    if (!remmina_plugin_service->file_get_int (remminafile, "disableclipboard", FALSE))
    {
        freerdp_chanman_load_plugin (gpdata->chan_man, gpdata->settings, "cliprdr", NULL);
    }

    rdpdr_num = 0;
    cs = remmina_plugin_service->file_get_string (remminafile, "sharefolder");
    if (cs && cs[0] == '/')
    {
        s = strrchr (cs, '/');
        s = (s && s[1] ? s + 1 : "root");
        gpdata->rdpdr_data[rdpdr_num].size = sizeof(RD_PLUGIN_DATA);
        gpdata->rdpdr_data[rdpdr_num].data[0] = "disk";
        gpdata->rdpdr_data[rdpdr_num].data[1] = s;
        gpdata->rdpdr_data[rdpdr_num].data[2] = (gchar*) cs;
        rdpdr_num++;
    }
    if (remmina_plugin_service->file_get_int (remminafile, "shareprinter", FALSE))
    {
        gpdata->rdpdr_data[rdpdr_num].size = sizeof(RD_PLUGIN_DATA);
        gpdata->rdpdr_data[rdpdr_num].data[0] = "printer";
        rdpdr_num++;
    }
    if (rdpdr_num)
    {
        freerdp_chanman_load_plugin (gpdata->chan_man, gpdata->settings, "rdpdr", gpdata->rdpdr_data);
    }

    gpdata->inst = freerdp_new (gpdata->settings);
    if (gpdata->inst == NULL)
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "freerdp_new failed");
        return FALSE;
    }
    if (gpdata->inst->version != FREERDP_INTERFACE_VERSION ||
        gpdata->inst->size != sizeof (rdpInst))
    {
        remmina_plugin_service->protocol_plugin_set_error (gp,
            "freerdp_new size, version / size do not "
            "match expecting v %d s %d got v %d s %d\n",
            FREERDP_INTERFACE_VERSION, sizeof (rdpInst),
            gpdata->inst->version, gpdata->inst->size);
        return FALSE;
    }
    SET_WIDGET (gpdata->inst, gp);
    remmina_plugin_rdpui_pre_connect (gp);
    remmina_plugin_rdpev_pre_connect (gp);
    if (freerdp_chanman_pre_connect (gpdata->chan_man, gpdata->inst) != 0)
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "freerdp_chanman_pre_connect failed");
        return FALSE;
    }
    if (gpdata->inst->rdp_connect (gpdata->inst) != 0)
    {
        if (!gpdata->user_cancelled)
        {
            remmina_plugin_service->protocol_plugin_set_error (gp, _("Unable to connect to RDP server %s"),
                gpdata->settings->server);
        }
        return FALSE;
    }
    if (freerdp_chanman_post_connect (gpdata->chan_man, gpdata->inst) != 0)
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "freerdp_chanman_post_connect failed");
        return FALSE;
    }
    remmina_plugin_rdpui_post_connect (gp);
    remmina_plugin_rdpev_post_connect (gp);
    remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");
    remmina_plugin_rdp_main_loop (gp);

    return TRUE;
}

static gpointer
remmina_plugin_rdp_main_thread (gpointer data)
{
    RemminaProtocolWidget *gp;
    RemminaPluginRdpData *gpdata;

    pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
    CANCEL_ASYNC
    gp = (RemminaProtocolWidget*) data;
    gpdata = GET_DATA (gp);
    remmina_plugin_rdp_main (gp);
    gpdata->thread = 0;
    IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);
    return NULL;
}

static void
remmina_plugin_rdp_init (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = g_new0 (RemminaPluginRdpData, 1);
    memset (gpdata, 0, sizeof (RemminaPluginRdpData));
    g_object_set_data_full (G_OBJECT (gp), "plugin-data", gpdata, g_free);

    gpdata->settings = (rdpSet *) malloc (sizeof (rdpSet));
    memset (gpdata->settings, 0, sizeof (rdpSet));
    gpdata->chan_man = freerdp_chanman_new ();

    gpdata->settings->tcp_port_rdp = 3389;
    gpdata->settings->encryption = 1;
    gpdata->settings->bitmap_cache = 1;
    gpdata->settings->bitmap_compression = 1;
    gpdata->settings->desktop_save = 0;
    gpdata->settings->performanceflags = 0;
    gpdata->settings->off_screen_bitmaps = 1;
    gpdata->settings->triblt = 0;
    gpdata->settings->new_cursors = 1;
    gpdata->settings->rdp_version = 5;
    gpdata->settings->rdp_security = 1;
    gpdata->settings->tls_security = 1;
    gpdata->settings->nla_security = 1;

    pthread_mutex_init (&gpdata->mutex, NULL);

    remmina_plugin_rdpev_init (gp);
    remmina_plugin_rdpui_init (gp);
}

static gboolean
remmina_plugin_rdp_open_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    gpdata->scale = remmina_plugin_service->protocol_plugin_get_scale (gp);

    if (pthread_create (&gpdata->thread, NULL, remmina_plugin_rdp_main_thread, gp))
    {
        remmina_plugin_service->protocol_plugin_set_error (gp, "%s",
            "Failed to initialize pthread. Falling back to non-thread mode...");
        gpdata->thread = 0;
        return FALSE;
    }
    return TRUE;
}

static gboolean
remmina_plugin_rdp_close_connection (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);

    if (gpdata->thread)
    {
        pthread_cancel (gpdata->thread);
        if (gpdata->thread) pthread_join (gpdata->thread, NULL);
    }

    if (gpdata->chan_man && gpdata->inst)
    {
        freerdp_chanman_close (gpdata->chan_man, gpdata->inst);
    }
    if (gpdata->inst)
    {
        gpdata->inst->rdp_disconnect (gpdata->inst);
        freerdp_free (gpdata->inst);
        gpdata->inst = NULL;
    }
    if (gpdata->settings)
    {
        free (gpdata->settings);
        gpdata->settings = NULL;
    }
    if (gpdata->chan_man)
    {
        freerdp_chanman_free (gpdata->chan_man);
        gpdata->chan_man = NULL;
    }
    pthread_mutex_destroy (&gpdata->mutex);

    remmina_plugin_rdpev_uninit (gp);
    remmina_plugin_rdpui_uninit (gp);

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
    return FALSE;
}

static gboolean
remmina_plugin_rdp_query_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
    return TRUE;
}

static void
remmina_plugin_rdp_call_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;

    gpdata = GET_DATA (gp);
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);
    switch (feature->id)
    {
        case REMMINA_PLUGIN_RDP_FEATURE_UNFOCUS:
            remmina_plugin_rdpev_unfocus (gp);
            break;
        case REMMINA_PLUGIN_RDP_FEATURE_SCALE:
            gpdata->scale = remmina_plugin_service->file_get_int (remminafile, "scale", FALSE);
            remmina_plugin_rdpev_update_scale (gp);
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

static const RemminaProtocolSetting remmina_plugin_rdp_basic_settings[] =
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

static const RemminaProtocolSetting remmina_plugin_rdp_advanced_settings[] =
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

static const RemminaProtocolFeature remmina_plugin_rdp_features[] =
{
    { REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_RDP_FEATURE_TOOL_REFRESH, N_("Refresh"), GTK_STOCK_REFRESH, NULL },
    { REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, REMMINA_PLUGIN_RDP_FEATURE_SCALE, NULL, NULL, NULL },
    { REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, REMMINA_PLUGIN_RDP_FEATURE_UNFOCUS, NULL, NULL, NULL },
    { REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

static RemminaProtocolPlugin remmina_plugin_rdp =
{
    REMMINA_PLUGIN_TYPE_PROTOCOL,
    "RDP",
    N_("RDP - Windows Terminal Service"),
    GETTEXT_PACKAGE,
    VERSION,

    "remmina-rdp",
    "remmina-rdp-ssh",
    remmina_plugin_rdp_basic_settings,
    remmina_plugin_rdp_advanced_settings,
    REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,
    remmina_plugin_rdp_features,

    remmina_plugin_rdp_init,
    remmina_plugin_rdp_open_connection,
    remmina_plugin_rdp_close_connection,
    remmina_plugin_rdp_query_feature,
    remmina_plugin_rdp_call_feature
};

static RemminaFilePlugin remmina_plugin_rdpf =
{
    REMMINA_PLUGIN_TYPE_FILE,
    "RDPF",
    N_("RDP - RDP File Handler"),
    GETTEXT_PACKAGE,
    VERSION,

    remmina_plugin_rdp_file_import_test,
    remmina_plugin_rdp_file_import,
    remmina_plugin_rdp_file_export_test,
    remmina_plugin_rdp_file_export,
    NULL
};

static RemminaPrefPlugin remmina_plugin_rdps =
{
    REMMINA_PLUGIN_TYPE_PREF,
    "RDPS",
    N_("RDP - Preferences"),
    GETTEXT_PACKAGE,
    VERSION,

    "RDP",
    remmina_plugin_rdpset_new
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry (RemminaPluginService *service)
{
    remmina_plugin_service = service;

    bindtextdomain (GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    if (! service->register_plugin ((RemminaPlugin *) &remmina_plugin_rdp))
    {
        return FALSE;
    }
    remmina_plugin_rdpf.export_hints = _("Export connection in Windows .rdp file format");
    if (! service->register_plugin ((RemminaPlugin *) &remmina_plugin_rdpf))
    {
        return FALSE;
    }
    if (! service->register_plugin ((RemminaPlugin *) &remmina_plugin_rdps))
    {
        return FALSE;
    }

    freerdp_chanman_init ();
    remmina_plugin_rdpset_init ();

    return TRUE;
}

