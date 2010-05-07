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

#include "remminapluginrdp.h"
#include "remminapluginrdpui.h"
#include "remminapluginrdpev.h"
#include "remminapluginrdpfile.h"
#include <freerdp/kbd.h>

RemminaPluginService *remmina_plugin_service = NULL;

static gint keyboard_layout;

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
    gchar *port;
    gchar *s;

    gpdata = GET_DATA (gp);
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    host = remmina_plugin_service->protocol_plugin_start_direct_tunnel (gp, 3389, FALSE);
    if (host == NULL)
    {
        return FALSE;
    }
    port = strchr (host, ':');
    if (port)
    {
        *port++ = '\0';
        gpdata->settings->tcp_port_rdp = atoi(port);
    }
    strncpy (gpdata->settings->server, host, sizeof (gpdata->settings->server) - 1);
    g_free (host);

    gpdata->settings->server_depth = remminafile->colordepth;
    gpdata->settings->width = remminafile->resolution_width;
    gpdata->settings->height = remminafile->resolution_height;
    remmina_plugin_service->protocol_plugin_set_width (gp, remminafile->resolution_width);
    remmina_plugin_service->protocol_plugin_set_height (gp, remminafile->resolution_height);

    if (remminafile->username && remminafile->username[0] != '\0')
    {
        strncpy (gpdata->settings->username, remminafile->username, sizeof (gpdata->settings->username) - 1);
    }

    if (remminafile->domain && remminafile->domain[0] != '\0')
    {
        strncpy (gpdata->settings->domain, remminafile->domain, sizeof (gpdata->settings->domain) - 1);
    }

    if (remminafile->password && remminafile->password[0] != '\0')
    {
        strncpy (gpdata->settings->password, remminafile->password, sizeof (gpdata->settings->password) - 1);
        gpdata->settings->autologin = 1;
    }

    if (remminafile->clientname && remminafile->clientname[0] != '\0')
    {
        strncpy (gpdata->settings->hostname, remminafile->clientname, sizeof (gpdata->settings->hostname) - 1);
    }
    else
    {
        strncpy (gpdata->settings->hostname, g_get_host_name (), sizeof (gpdata->settings->hostname) - 1);
    }

    if (remminafile->exec && remminafile->exec[0] != '\0')
    {
        strncpy (gpdata->settings->shell, remminafile->exec, sizeof (gpdata->settings->shell) - 1);
    }

    if (remminafile->execpath && remminafile->execpath[0] != '\0')
    {
        strncpy (gpdata->settings->directory, remminafile->execpath, sizeof (gpdata->settings->directory) - 1);
    }

    gpdata->settings->keyboard_layout = keyboard_layout;

    if (g_strcmp0 (remminafile->sound, "remote") == 0)
    {
        freerdp_chanman_load_plugin (gpdata->chan_man, gpdata->settings, "rdpsnd", NULL);
    }
    else if (g_strcmp0 (remminafile->sound, "local") == 0)
    {
        gpdata->settings->console_audio = 1;
    }

    if (remminafile->console)
    {
        gpdata->settings->console_session = 1;
    }

    freerdp_chanman_load_plugin (gpdata->chan_man, gpdata->settings, "cliprdr", NULL);

    if (remminafile->sharefolder && remminafile->sharefolder[0] == '/')
    {
        s = strrchr (remminafile->sharefolder, '/');
        s = (s && s[1] ? s + 1 : "root");
        gpdata->rdpdr_data[0].size = sizeof(RD_PLUGIN_DATA);
        gpdata->rdpdr_data[0].data[0] = "disk";
        gpdata->rdpdr_data[0].data[1] = s;
        gpdata->rdpdr_data[0].data[2] = remminafile->sharefolder;
        freerdp_chanman_load_plugin (gpdata->chan_man, gpdata->settings, "rdpdr", &gpdata->rdpdr_data[0]);
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
        remmina_plugin_service->protocol_plugin_set_error (gp, "inst->rdp_connect failed");
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
    gpdata->settings->rdp5_performanceflags =
        RDP5_NO_WALLPAPER | RDP5_NO_FULLWINDOWDRAG | RDP5_NO_MENUANIMATIONS;
    gpdata->settings->off_screen_bitmaps = 1;
    gpdata->settings->triblt = 0;
    gpdata->settings->new_cursors = 1;
    gpdata->settings->rdp_version = 5;

    pthread_mutex_init (&gpdata->mutex, NULL);

    remmina_plugin_rdpui_init (gp);
    remmina_plugin_rdpev_init (gp);
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

    remmina_plugin_rdpev_uninit (gp);
    remmina_plugin_rdpui_uninit (gp);

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

    remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
    return FALSE;
}

static gpointer
remmina_plugin_rdp_query_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature)
{
    switch (feature)
    {
        case REMMINA_PROTOCOL_FEATURE_UNFOCUS:
        case REMMINA_PROTOCOL_FEATURE_SCALE:
        case REMMINA_PROTOCOL_FEATURE_TOOL_REFRESH:
            return GINT_TO_POINTER (1);
        default:
            return NULL;
    }
}

static void
remmina_plugin_rdp_call_feature (RemminaProtocolWidget *gp, RemminaProtocolFeature feature, const gpointer data)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    switch (feature)
    {
        case REMMINA_PROTOCOL_FEATURE_UNFOCUS:
            remmina_plugin_rdpev_unfocus (gp);
            break;
        case REMMINA_PROTOCOL_FEATURE_SCALE:
            gpdata->scale = (data != NULL);
            remmina_plugin_rdpev_update_scale (gp);
            break;
        case REMMINA_PROTOCOL_FEATURE_TOOL_REFRESH:
            break;
        default:
            break;
    }
}

static const RemminaProtocolSetting remmina_plugin_rdp_basic_settings[] =
{
    REMMINA_PROTOCOL_SETTING_SERVER,
    REMMINA_PROTOCOL_SETTING_USERNAME,
    REMMINA_PROTOCOL_SETTING_PASSWORD,
    REMMINA_PROTOCOL_SETTING_DOMAIN,
    REMMINA_PROTOCOL_SETTING_RESOLUTION_FIXED,
    REMMINA_PROTOCOL_SETTING_COLORDEPTH,
    REMMINA_PROTOCOL_SETTING_SHAREFOLDER,
    REMMINA_PROTOCOL_SETTING_CTL_END
};

static const RemminaProtocolSetting remmina_plugin_rdp_advanced_settings[] =
{
    REMMINA_PROTOCOL_SETTING_SOUND,
    REMMINA_PROTOCOL_SETTING_CLIENTNAME,
    REMMINA_PROTOCOL_SETTING_EXEC,
    REMMINA_PROTOCOL_SETTING_EXECPATH,
    REMMINA_PROTOCOL_SETTING_SHAREPRINTER,
    REMMINA_PROTOCOL_SETTING_CONSOLE,
    REMMINA_PROTOCOL_SETTING_CTL_END
};

static RemminaProtocolPlugin remmina_plugin_rdp =
{
    REMMINA_PLUGIN_TYPE_PROTOCOL,
    "RDP",
    NULL,

    "remmina-rdp",
    "remmina-rdp-ssh",
    NULL,
    remmina_plugin_rdp_basic_settings,
    remmina_plugin_rdp_advanced_settings,
    REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,

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
    NULL,

    remmina_plugin_rdp_file_import_test,
    remmina_plugin_rdp_file_import,
    remmina_plugin_rdp_file_export_test,
    remmina_plugin_rdp_file_export,
    NULL
};

G_MODULE_EXPORT gboolean
remmina_plugin_entry (RemminaPluginService *service)
{
    remmina_plugin_service = service;

    bindtextdomain (GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
    bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

    remmina_plugin_rdp.description = _("RDP - Windows Terminal Service");
    if (! service->register_plugin ((RemminaPlugin *) &remmina_plugin_rdp))
    {
        return FALSE;
    }
    remmina_plugin_rdpf.description = _("RDP - RDP File Handler");
    remmina_plugin_rdpf.export_hints = _("Export connection in Windows .rdp file format");
    if (! service->register_plugin ((RemminaPlugin *) &remmina_plugin_rdpf))
    {
        return FALSE;
    }

    freerdp_chanman_init ();
    keyboard_layout = freerdp_kbd_init();

    return TRUE;
}

