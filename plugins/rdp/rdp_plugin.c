/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2022 Antenore Gatta, Giovanni Panozzo
 * Copyright (C) 2022-2023 Antenore Gatta, Giovanni Panozzo, Hiroyuki Tanaka
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
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


#define _GNU_SOURCE

#include "remmina/plugin.h"
#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_graphics.h"
#include "rdp_file.h"
#include "rdp_settings.h"
#include "rdp_cliprdr.h"
#include "rdp_monitor.h"
#include "rdp_channels.h"

#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#ifdef GDK_WINDOWING_X11
#include <cairo/cairo-xlib.h>
#else
#include <cairo/cairo.h>
#endif
#include <ctype.h>
#include <freerdp/addin.h>
#include <freerdp/assistance.h>
#if FREERDP_VERSION_MAJOR >= 3
#include <freerdp/channels/rdp2tcp.h>
#else
#define RDP2TCP_DVC_CHANNEL_NAME "rdp2tcp"
#endif
#include <freerdp/client/channels.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/constants.h>
#include <freerdp/error.h>
#include <freerdp/event.h>
#include <freerdp/freerdp.h>
#include <freerdp/settings.h>
#include <winpr/cmdline.h>
#include <winpr/memory.h>

#ifdef HAVE_CUPS
#include <cups/cups.h>
#endif

#include <unistd.h>
#include <string.h>

#ifdef GDK_WINDOWING_X11
#include <X11/Xlib.h>
#include <X11/XKBlib.h>
#include <gdk/gdkx.h>
#elif defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdkwayland.h>
#endif

#if defined(__FreeBSD__)
#include <pthread_np.h>
#endif

#include <freerdp/locale/keyboard.h>

#define REMMINA_RDP_FEATURE_TOOL_REFRESH         1
#define REMMINA_RDP_FEATURE_SCALE                2
#define REMMINA_RDP_FEATURE_UNFOCUS              3
#define REMMINA_RDP_FEATURE_TOOL_SENDCTRLALTDEL  4
#define REMMINA_RDP_FEATURE_DYNRESUPDATE         5
#define REMMINA_RDP_FEATURE_MULTIMON             6
#define REMMINA_RDP_FEATURE_VIEWONLY             7

#define REMMINA_CONNECTION_TYPE_NONE             0

#if FREERDP_VERSION_MAJOR >= 3
	#define CLPARAM const char
#else
	#define CLPARAM char
#endif

#if FREERDP_VERSION_MAJOR < 3
static HANDLE freerdp_abort_event(rdpContext* context) {
#ifdef WINPR_ASSERT
	WINPR_ASSERT(context);
#endif
	return context->abortEvent;
}

static BOOL freerdp_settings_set_pointer_len(rdpSettings* settings, size_t id, const void* data, size_t len)
{
	switch(id) {
	case FreeRDP_LoadBalanceInfo:
		free(settings->LoadBalanceInfo);
		settings->LoadBalanceInfo = (BYTE *)data;
		freerdp_settings_set_uint32(settings, FreeRDP_LoadBalanceInfoLength, strlen(data));

		return TRUE;
	default:
		return FALSE;
	}
}

static void freerdp_abort_connect_context(rdpContext* context) {
#ifdef WINPR_ASSERT
	WINPR_ASSERT(context);
#endif
	freerdp_abort_connect(context->instance);
}
#endif

RemminaPluginService *remmina_plugin_service = NULL;

static BOOL gfx_h264_available = FALSE;

/* Compatibility: these functions have been introduced with https://github.com/FreeRDP/FreeRDP/commit/8c5d96784d
 * and are missing on older FreeRDP, so we add them here.
 * They should be removed from here after all distributed versions of FreeRDP (libwinpr) will have
 * CommandLineParseCommaSeparatedValuesEx() onboard.
 *
 * (C) Copyright goes to the FreeRDP authors.
 */
static CLPARAM **remmina_rdp_CommandLineParseCommaSeparatedValuesEx(const char *name, const char *list, size_t *count)
{
	TRACE_CALL(__func__);
#if FREERDP_CHECK_VERSION(2, 0, 0)
	return (CLPARAM **)CommandLineParseCommaSeparatedValuesEx(name, list, count);
#else
	char **p;
	char *str;
	size_t nArgs;
	size_t index;
	size_t nCommas;
	size_t prefix, len;

	nCommas = 0;

	if (count == NULL)
		return NULL;

	*count = 0;

	if (!list) {
		if (name) {
			size_t len = strlen(name);
			p = (char **)calloc(2UL + len, sizeof(char *));

			if (p) {
				char *dst = (char *)&p[1];
				p[0] = dst;
				sprintf_s(dst, len + 1, "%s", name);
				*count = 1;
				return p;
			}
		}

		return NULL;
	}

	{
		const char *it = list;

		while ((it = strchr(it, ',')) != NULL) {
			it++;
			nCommas++;
		}
	}

	nArgs = nCommas + 1;

	if (name)
		nArgs++;

	prefix = (nArgs + 1UL) * sizeof(char *);
	len = strlen(list);
	p = (char **)calloc(len + prefix + 1, sizeof(char *));

	if (!p)
		return NULL;

	str = &((char *)p)[prefix];
	memcpy(str, list, len);

	if (name)
		p[0] = (char *)name;

	for (index = name ? 1 : 0; index < nArgs; index++) {
		char *comma = strchr(str, ',');
		p[index] = str;

		if (comma) {
			str = comma + 1;
			*comma = '\0';
		}
	}

	*count = nArgs;
	return p;
#endif
}

static CLPARAM **remmina_rdp_CommandLineParseCommaSeparatedValues(const char *list, size_t *count)
{
	TRACE_CALL(__func__);
	return remmina_rdp_CommandLineParseCommaSeparatedValuesEx(NULL, list, count);
}

/*
 * End of CommandLineParseCommaSeparatedValuesEx() compatibility and copyright
 */
static BOOL rf_process_event_queue(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	UINT16 flags;
	rdpInput *input;
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent *event;
	DISPLAY_CONTROL_MONITOR_LAYOUT *dcml;
	CLIPRDR_FORMAT_DATA_RESPONSE response = { 0 };
	RemminaFile *remminafile;

	if (rfi->event_queue == NULL)
		return true;

	input = rfi->clientContext.context.input;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	while ((event = (RemminaPluginRdpEvent *)g_async_queue_try_pop(rfi->event_queue)) != NULL) {
		time(&(rfi->last_time)); //update last user interaction time
		time(&(rfi->last_time_idle_keypress));
		switch (event->type) {
		case REMMINA_RDP_EVENT_TYPE_SCANCODE:
			
			if (event->key_event.extended1){
				flags = KBD_FLAGS_EXTENDED1;
			}
			else{
				flags = event->key_event.extended ? KBD_FLAGS_EXTENDED : 0;
			}
			flags |= event->key_event.up ? KBD_FLAGS_RELEASE : KBD_FLAGS_DOWN;
			input->KeyboardEvent(input, flags, event->key_event.key_code);
			break;

		case REMMINA_RDP_EVENT_TYPE_SCANCODE_UNICODE:
			/*
			 * TS_UNICODE_KEYBOARD_EVENT RDP message, see https://msdn.microsoft.com/en-us/library/cc240585.aspx
			 */
			flags = event->key_event.up ? KBD_FLAGS_RELEASE : KBD_FLAGS_DOWN;
			input->UnicodeKeyboardEvent(input, flags, event->key_event.unicode_code);
			break;

		case REMMINA_RDP_EVENT_TYPE_MOUSE:
			if (event->mouse_event.extended)
				input->ExtendedMouseEvent(input, event->mouse_event.flags,
							  event->mouse_event.x, event->mouse_event.y);
			else
				input->MouseEvent(input, event->mouse_event.flags,
						  event->mouse_event.x, event->mouse_event.y);
			break;

		case REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_LIST:
			if(rfi->clipboard.context != NULL){
				rfi->clipboard.context->ClientFormatList(rfi->clipboard.context, event->clipboard_formatlist.pFormatList);
			}
			
			free(event->clipboard_formatlist.pFormatList);
			break;

		case REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_DATA_RESPONSE:
		{
			UINT32 msgFlags = (event->clipboard_formatdataresponse.data) ? CB_RESPONSE_OK : CB_RESPONSE_FAIL;
#if FREERDP_VERSION_MAJOR >= 3
			response.common.msgFlags = msgFlags;
			response.common.dataLen = event->clipboard_formatdataresponse.size;
#else
			response.msgFlags = msgFlags;
			response.dataLen = event->clipboard_formatdataresponse.size;
#endif
			response.requestedFormatData = event->clipboard_formatdataresponse.data;
			rfi->clipboard.context->ClientFormatDataResponse(rfi->clipboard.context, &response);
		}
			break;

		case REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_DATA_REQUEST:
			REMMINA_PLUGIN_DEBUG("Sending client FormatDataRequest to server");
			gettimeofday(&(rfi->clipboard.clientformatdatarequest_tv), NULL);
			rfi->clipboard.context->ClientFormatDataRequest(rfi->clipboard.context, event->clipboard_formatdatarequest.pFormatDataRequest);
			free(event->clipboard_formatdatarequest.pFormatDataRequest);
			break;

		case REMMINA_RDP_EVENT_TYPE_SEND_MONITOR_LAYOUT:
			if (remmina_plugin_service->file_get_int(remminafile, "multimon", FALSE)) {
				freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_UseMultimon, TRUE);
				/* TODO Add an option for this */
				freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ForceMultimon, TRUE);
				freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_Fullscreen, TRUE);
				/* got some crashes with g_malloc0, to be investigated */
				dcml = calloc(freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_MonitorCount), sizeof(DISPLAY_CONTROL_MONITOR_LAYOUT));
				REMMINA_PLUGIN_DEBUG("REMMINA_RDP_EVENT_TYPE_SEND_MONITOR_LAYOUT:");
				if (!dcml)
					break;

				const rdpMonitor *base = freerdp_settings_get_pointer(rfi->clientContext.context.settings, FreeRDP_MonitorDefArray);
				for (gint i = 0; i < freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_MonitorCount); ++i) {
					const rdpMonitor *current = &base[i];
					REMMINA_PLUGIN_DEBUG("Sending display layout for monitor n° %d", i);
					dcml[i].Flags = (current->is_primary ? DISPLAY_CONTROL_MONITOR_PRIMARY : 0);
					REMMINA_PLUGIN_DEBUG("Monitor %d is primary: %d", i, dcml[i].Flags);
					dcml[i].Left = current->x;
					REMMINA_PLUGIN_DEBUG("Monitor %d x: %d", i, dcml[i].Left);
					dcml[i].Top = current->y;
					REMMINA_PLUGIN_DEBUG("Monitor %d y: %d", i, dcml[i].Top);
					dcml[i].Width = current->width;
					REMMINA_PLUGIN_DEBUG("Monitor %d width: %d", i, dcml[i].Width);
					dcml[i].Height = current->height;
					REMMINA_PLUGIN_DEBUG("Monitor %d height: %d", i, dcml[i].Height);
					dcml[i].PhysicalWidth = current->attributes.physicalWidth;
					REMMINA_PLUGIN_DEBUG("Monitor %d physical width: %d", i, dcml[i].PhysicalWidth);
					dcml[i].PhysicalHeight = current->attributes.physicalHeight;
					REMMINA_PLUGIN_DEBUG("Monitor %d physical height: %d", i, dcml[i].PhysicalHeight);
					if (current->attributes.orientation)
						dcml[i].Orientation = current->attributes.orientation;
					else
						dcml[i].Orientation = event->monitor_layout.desktopOrientation;
					REMMINA_PLUGIN_DEBUG("Monitor %d orientation: %d", i, dcml[i].Orientation);
					dcml[i].DesktopScaleFactor = event->monitor_layout.desktopScaleFactor;
					dcml[i].DeviceScaleFactor = event->monitor_layout.deviceScaleFactor;
				}
				rfi->dispcontext->SendMonitorLayout(rfi->dispcontext, freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_MonitorCount), dcml);
				g_free(dcml);
			} else {
				dcml = g_malloc0(sizeof(DISPLAY_CONTROL_MONITOR_LAYOUT));
				if (dcml) {
					dcml->Flags = DISPLAY_CONTROL_MONITOR_PRIMARY;
					dcml->Width = event->monitor_layout.width;
					dcml->Height = event->monitor_layout.height;
					dcml->Orientation = event->monitor_layout.desktopOrientation;
					dcml->DesktopScaleFactor = event->monitor_layout.desktopScaleFactor;
					dcml->DeviceScaleFactor = event->monitor_layout.deviceScaleFactor;
					rfi->dispcontext->SendMonitorLayout(rfi->dispcontext, 1, dcml);
					g_free(dcml); \
				}
			}
			break;
		case REMMINA_RDP_EVENT_DISCONNECT:
			/* Disconnect requested via GUI (i.e: tab destroy/close) */
			freerdp_abort_connect_context(&rfi->clientContext.context);
			break;
		}

		g_free(event);
	}

	return true;
}

static gboolean remmina_rdp_tunnel_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	/* Opens the optional SSH tunnel if needed.
	 * Used also when reopening the same tunnel for a FreeRDP reconnect.
	 * Returns TRUE if all OK, and setups correct rfi->Settings values
	 * with connection and certificate parameters */

	gchar *hostport;
	gchar *s;
	gchar *host;
	gchar *cert_host;
	gint cert_port;
	gint port;

	rfContext *rfi = GET_PLUGIN_DATA(gp);

	REMMINA_PLUGIN_DEBUG("Tunnel init");
	hostport = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, 3389, FALSE);
	if (hostport == NULL)
		return FALSE;

	remmina_plugin_service->get_server_port(hostport, 3389, &host, &port);

	if (host[0] == 0)
		return FALSE;

	REMMINA_PLUGIN_DEBUG("protocol_plugin_start_direct_tunnel() returned %s", hostport);

	cert_host = host;
	cert_port = port;

	if (!rfi->is_reconnecting) {
		/* settings->CertificateName and settings->ServerHostname is created
		 * only on 1st connect, not on reconnections */

		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname, host);

		if (cert_port == 3389) {
			freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_CertificateName, cert_host);
		} else {
			s = g_strdup_printf("%s:%d", cert_host, cert_port);
			freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_CertificateName, s);
			g_free(s);
		}
	}

	REMMINA_PLUGIN_DEBUG("Tunnel has been optionally initialized. Now connecting to %s:%d", host, port);

	if (cert_host != host) g_free(cert_host);
	g_free(host);
	g_free(hostport);

	freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ServerPort, port);

	return TRUE;
}

static BOOL rf_auto_reconnect(rfContext *rfi)
{
	TRACE_CALL(__func__);
	rdpSettings *settings = rfi->clientContext.context.settings;
	RemminaPluginRdpUiObject *ui;
	time_t treconn;
	gchar *cval;
	gint maxattempts;

	RemminaProtocolWidget *gp = rfi->protocol_widget;
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	rfi->is_reconnecting = TRUE;
	rfi->stop_reconnecting_requested = FALSE;

	/* Get the value set in FreeRDP_AutoReconnectMaxRetries (20) */
	maxattempts = freerdp_settings_get_uint32(settings, FreeRDP_AutoReconnectMaxRetries);
	REMMINA_PLUGIN_DEBUG("maxattempts from default: %d", maxattempts);
	/* Get the value from the global preferences if any */
	if ((cval = remmina_plugin_service->pref_get_value("rdp_reconnect_attempts")) != NULL)
		maxattempts = atoi(cval);
	REMMINA_PLUGIN_DEBUG("maxattempts from general preferences: %d", maxattempts);
	/* Get the value from the profile if any, otherwise uses the value of maxattempts */
	maxattempts = remmina_plugin_service->file_get_int(remminafile, "rdp_reconnect_attempts", maxattempts);
	REMMINA_PLUGIN_DEBUG("maxattempts from general plugin: %d", maxattempts);
	/* If maxattemps is <= 0, we get the value from FreeRDP_AutoReconnectMaxRetries (20) */
	if (maxattempts <= 0)
		maxattempts = freerdp_settings_get_uint32(settings, FreeRDP_AutoReconnectMaxRetries);
	freerdp_settings_set_uint32(settings, FreeRDP_AutoReconnectMaxRetries, maxattempts);
	REMMINA_PLUGIN_DEBUG("maxattempts set to: %d", maxattempts);

	rfi->reconnect_maxattempts = maxattempts;
	rfi->reconnect_nattempt = 0;

	/* Only auto reconnect on network disconnects. */
	switch (freerdp_error_info(rfi->clientContext.context.instance)) {
	case ERRINFO_GRAPHICS_SUBSYSTEM_FAILED:
		/* Disconnected by server hitting a bug or resource limit */
		break;
	case ERRINFO_SUCCESS:
		/* A network disconnect was detected */
		break;
	default:
		rfi->is_reconnecting = FALSE;
		return FALSE;
	}

	if (!freerdp_settings_get_bool(settings, FreeRDP_AutoReconnectionEnabled)) {
		/* No auto-reconnect - just quit */
		rfi->is_reconnecting = FALSE;
		return FALSE;
	}

	/* A network disconnect was detected and we should try to reconnect */
	REMMINA_PLUGIN_DEBUG("[%s] network disconnection detected, initiating reconnection attempt",
		freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_RECONNECT_PROGRESS;
	remmina_rdp_event_queue_ui_async(rfi->protocol_widget, ui);

	/* Sleep half a second to allow:
	 *  - processing of the UI event we just pushed on the queue
	 *  - better network conditions
	 *  Remember: We hare on a thread, so the main gui won’t lock */
	usleep(500000);

	/* Perform an auto-reconnect. */
	while (TRUE) {
		/* Quit retrying if max retries has been exceeded */
		if (rfi->reconnect_nattempt++ >= rfi->reconnect_maxattempts) {
			REMMINA_PLUGIN_DEBUG("[%s] maximum number of reconnection attempts exceeded.",
				freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
			break;
		}

		if (rfi->stop_reconnecting_requested) {
			REMMINA_PLUGIN_DEBUG("[%s] reconnect request loop interrupted by user.",
						 freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
			break;
		}

		/* Attempt the next reconnect */
		REMMINA_PLUGIN_DEBUG("[%s] reconnection, attempt #%d of %d",
					 freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname), rfi->reconnect_nattempt, rfi->reconnect_maxattempts);

		ui = g_new0(RemminaPluginRdpUiObject, 1);
		ui->type = REMMINA_RDP_UI_RECONNECT_PROGRESS;
		remmina_rdp_event_queue_ui_async(rfi->protocol_widget, ui);

		treconn = time(NULL);

		/* Reconnect the SSH tunnel, if needed */
		if (!remmina_rdp_tunnel_init(rfi->protocol_widget)) {
			REMMINA_PLUGIN_DEBUG("[%s] unable to recreate tunnel with remmina_rdp_tunnel_init.",
						 freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
		} else {
			if (freerdp_reconnect(rfi->clientContext.context.instance)) {
				/* Reconnection is successful */
				REMMINA_PLUGIN_DEBUG("[%s] reconnected.", freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				rfi->is_reconnecting = FALSE;
				return TRUE;
			}
		}

		/* Wait until 5 secs have elapsed from last reconnect attempt, while checking for rfi->stop_reconnecting_requested */
		while (time(NULL) - treconn < 5) {
			if (rfi->stop_reconnecting_requested)
				break;
			usleep(200000); // 200ms sleep
		}
	}

	rfi->is_reconnecting = FALSE;
	return FALSE;
}

static BOOL rf_begin_paint(rdpContext *context)
{
	TRACE_CALL(__func__);
	rdpGdi *gdi;

	if (!context)
		return FALSE;

	gdi = context->gdi;
	if (!gdi || !gdi->primary || !gdi->primary->hdc || !gdi->primary->hdc->hwnd)
		return FALSE;

	return TRUE;
}

static BOOL rf_end_paint(rdpContext *context)
{
	TRACE_CALL(__func__);
	rdpGdi *gdi;
	rfContext *rfi;
	RemminaPluginRdpUiObject *ui;
	int i, ninvalid;
	region *reg;
	HGDI_RGN cinvalid;

	gdi = context->gdi;
	rfi = (rfContext *)context;

	if (gdi == NULL || gdi->primary == NULL || gdi->primary->hdc == NULL || gdi->primary->hdc->hwnd == NULL)
		return TRUE;

	if (gdi->primary->hdc->hwnd->invalid->null)
		return TRUE;

	if (gdi->primary->hdc->hwnd->ninvalid < 1)
		return TRUE;

	ninvalid = gdi->primary->hdc->hwnd->ninvalid;
	cinvalid = gdi->primary->hdc->hwnd->cinvalid;
	reg = (region *)g_malloc(sizeof(region) * ninvalid);
	for (i = 0; i < ninvalid; i++) {
		reg[i].x = cinvalid[i].x;
		reg[i].y = cinvalid[i].y;
		reg[i].w = cinvalid[i].w;
		reg[i].h = cinvalid[i].h;
	}

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_UPDATE_REGIONS;
	ui->reg.ninvalid = ninvalid;
	ui->reg.ureg = reg;

	remmina_rdp_event_queue_ui_async(rfi->protocol_widget, ui);


	gdi->primary->hdc->hwnd->invalid->null = TRUE;
	gdi->primary->hdc->hwnd->ninvalid = 0;


	return TRUE;
}

static BOOL rf_desktop_resize(rdpContext *context)
{
	TRACE_CALL(__func__);
	rfContext *rfi;
	RemminaProtocolWidget *gp;
	RemminaPluginRdpUiObject *ui;
	UINT32 w, h;

	rfi = (rfContext *)context;
	gp = rfi->protocol_widget;

	w = freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth);
	h = freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight);
	remmina_plugin_service->protocol_plugin_set_width(gp, w);
	remmina_plugin_service->protocol_plugin_set_height(gp, h);

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_EVENT;
	ui->event.type = REMMINA_RDP_UI_EVENT_DESTROY_CAIRO_SURFACE;
	remmina_rdp_event_queue_ui_sync_retint(gp, ui);

	/* Tell libfreerdp to change its internal GDI bitmap width and height,
	 * this will also destroy gdi->primary_buffer, making our rfi->surface invalid */
	gdi_resize(((rdpContext *)rfi)->gdi, w, h);

	/* Call to remmina_rdp_event_update_scale(gp) on the main UI thread,
	 * this will recreate rfi->surface from gdi->primary_buffer */

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_EVENT;
	ui->event.type = REMMINA_RDP_UI_EVENT_UPDATE_SCALE;
	remmina_rdp_event_queue_ui_sync_retint(gp, ui);

	remmina_plugin_service->protocol_plugin_desktop_resize(gp);

	return TRUE;
}

static BOOL rf_play_sound(rdpContext *context, const PLAY_SOUND_UPDATE *play_sound)
{
	TRACE_CALL(__func__);
	rfContext *rfi;
	RemminaProtocolWidget *gp;
	GdkDisplay *disp;

	rfi = (rfContext *)context;
	gp = rfi->protocol_widget;

	disp = gtk_widget_get_display(GTK_WIDGET(gp));
	gdk_display_beep(disp);

	return TRUE;
}

static BOOL rf_keyboard_set_indicators(rdpContext *context, UINT16 led_flags)
{
	TRACE_CALL(__func__);
	rfContext *rfi;
	RemminaProtocolWidget *gp;
	GdkDisplay *disp;

	rfi = (rfContext *)context;
	gp = rfi->protocol_widget;
	disp = gtk_widget_get_display(GTK_WIDGET(gp));

#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY(disp)) {
		/* TODO: We are not on the main thread. Will X.Org complain? */
		Display *x11_display;
		x11_display = gdk_x11_display_get_xdisplay(disp);
		XkbLockModifiers(x11_display, XkbUseCoreKbd,
				 LockMask | Mod2Mask,
				 ((led_flags & KBD_SYNC_CAPS_LOCK) ? LockMask : 0) |
				 ((led_flags & KBD_SYNC_NUM_LOCK) ? Mod2Mask : 0)
				 );

		/* TODO: Add support to KANA_LOCK and SCROLL_LOCK */
	}
#endif

	return TRUE;
}

static BOOL rf_keyboard_set_ime_status(rdpContext *context, UINT16 imeId, UINT32 imeState,
				UINT32 imeConvMode)
{
	TRACE_CALL(__func__);
	if (!context)
		return FALSE;

	/* Unimplemented, we ignore it */

	return TRUE;
}

static BOOL remmina_rdp_pre_connect(freerdp *instance)
{
	TRACE_CALL(__func__);
	rdpChannels *channels;
	rdpSettings *settings;
	rdpContext *context = instance->context;

	settings = context->settings;
	channels = context->channels;
	freerdp_settings_set_uint32(settings, FreeRDP_OsMajorType, OSMAJORTYPE_UNIX);
	freerdp_settings_set_uint32(settings, FreeRDP_OsMinorType, OSMINORTYPE_UNSPECIFIED);
	freerdp_settings_set_bool(settings, FreeRDP_BitmapCacheEnabled, TRUE);
	freerdp_settings_set_uint32(settings, FreeRDP_OffscreenSupportLevel, 1);

	PubSub_SubscribeChannelConnected(instance->context->pubSub,
					 remmina_rdp_OnChannelConnectedEventHandler);
	PubSub_SubscribeChannelDisconnected(instance->context->pubSub,
						remmina_rdp_OnChannelDisconnectedEventHandler);

	if (!freerdp_client_load_addins(channels, settings))
		return FALSE;

	return true;
}

static BOOL remmina_rdp_post_connect(freerdp *instance)
{
	TRACE_CALL(__func__);
	rfContext *rfi;
	RemminaProtocolWidget *gp;
	RemminaPluginRdpUiObject *ui;
	UINT32 freerdp_local_color_format;

	rfi = (rfContext *)instance->context;
	gp = rfi->protocol_widget;
	rfi->postconnect_error = REMMINA_POSTCONNECT_ERROR_OK;

	rfi->attempt_interactive_authentication = FALSE; // We authenticated!

	rfi->srcBpp = freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth);

	if (freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_RemoteFxCodec) == FALSE)
		rfi->sw_gdi = TRUE;

	rf_register_graphics(instance->context->graphics);

	REMMINA_PLUGIN_DEBUG("bpp: %d", rfi->bpp);
	switch (rfi->bpp) {
	case 24:
		REMMINA_PLUGIN_DEBUG("CAIRO_FORMAT_RGB24");
		freerdp_local_color_format = PIXEL_FORMAT_BGRX32;
		rfi->cairo_format = CAIRO_FORMAT_RGB24;
		break;
	case 32:
		/** Do not use alpha as it's not used with the desktop
		 * CAIRO_FORMAT_ARGB32
		 * See https://gitlab.com/Remmina/Remmina/-/issues/2456
		 */
		REMMINA_PLUGIN_DEBUG("CAIRO_FORMAT_RGB24");
		freerdp_local_color_format = PIXEL_FORMAT_BGRA32;
		rfi->cairo_format = CAIRO_FORMAT_RGB24;
		break;
	default:
		REMMINA_PLUGIN_DEBUG("CAIRO_FORMAT_RGB16_565");
		freerdp_local_color_format = PIXEL_FORMAT_RGB16;
		rfi->cairo_format = CAIRO_FORMAT_RGB16_565;
		break;
	}

	if (!gdi_init(instance, freerdp_local_color_format)) {
		rfi->postconnect_error = REMMINA_POSTCONNECT_ERROR_GDI_INIT;
		return FALSE;
	}

	if (instance->context->codecs->h264 == NULL && freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_GfxH264)) {
		gdi_free(instance);
		rfi->postconnect_error = REMMINA_POSTCONNECT_ERROR_NO_H264;
		return FALSE;
	}

	rdpUpdate *update = instance->context->update;
	update->BeginPaint = rf_begin_paint;
	update->EndPaint = rf_end_paint;
	update->DesktopResize = rf_desktop_resize;

	update->PlaySound = rf_play_sound;
	update->SetKeyboardIndicators = rf_keyboard_set_indicators;
	update->SetKeyboardImeStatus = rf_keyboard_set_ime_status;

	remmina_rdp_clipboard_init(rfi);
	rfi->connected = true;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CONNECTED;
	remmina_rdp_event_queue_ui_async(gp, ui);

	return TRUE;
}

#if !defined(FREERDP_VERSION_MAJOR) || (FREERDP_VERSION_MAJOR < 3)
static BOOL remmina_rdp_authenticate(freerdp *instance, char **username, char **password, char **domain)
{
	TRACE_CALL(__func__);
	gchar *s_username, *s_password, *s_domain;
	gint ret;
	rfContext *rfi;
	RemminaProtocolWidget *gp;
	gboolean save;
	gboolean disablepasswordstoring;
	RemminaFile *remminafile;

	rfi = (rfContext *)instance->context;
	gp = rfi->protocol_widget;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);

	ret = remmina_plugin_service->protocol_plugin_init_auth(gp,
								(disablepasswordstoring ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD) | REMMINA_MESSAGE_PANEL_FLAG_USERNAME | REMMINA_MESSAGE_PANEL_FLAG_DOMAIN,
								_("Enter RDP authentication credentials"),
								remmina_plugin_service->file_get_string(remminafile, "username"),
								remmina_plugin_service->file_get_string(remminafile, "password"),
								remmina_plugin_service->file_get_string(remminafile, "domain"),
								NULL);
	if (ret == GTK_RESPONSE_OK) {
		s_username = remmina_plugin_service->protocol_plugin_init_get_username(gp);
		if (s_username) freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Username, s_username);

		s_password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		if (s_password) freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Password, s_password);

		s_domain = remmina_plugin_service->protocol_plugin_init_get_domain(gp);
		if (s_domain) freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Domain, s_domain);

		remmina_plugin_service->file_set_string(remminafile, "username", s_username);
		remmina_plugin_service->file_set_string(remminafile, "domain", s_domain);

		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save) {
			// User has requested to save credentials. We put the password
			// into remminafile->settings. It will be saved later, on successful connection, by
			// rcw.c
			remmina_plugin_service->file_set_string(remminafile, "password", s_password);
		} else {
			remmina_plugin_service->file_set_string(remminafile, "password", NULL);
		}


		if (s_username) g_free(s_username);
		if (s_password) g_free(s_password);
		if (s_domain) g_free(s_domain);

		return TRUE;
	} else {
		return FALSE;
	}

	return TRUE;
}

static BOOL remmina_rdp_gw_authenticate(freerdp *instance, char **username, char **password, char **domain)
{
	TRACE_CALL(__func__);
	gchar *s_username, *s_password, *s_domain;
	gint ret;
	rfContext *rfi;
	RemminaProtocolWidget *gp;
	gboolean save;
	gboolean disablepasswordstoring;
	gboolean basecredforgw;
	RemminaFile *remminafile;

	rfi = (rfContext *)instance->context;
	gp = rfi->protocol_widget;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (!remmina_plugin_service->file_get_string(remminafile, "gateway_server"))
		return false;
	disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);
	basecredforgw = remmina_plugin_service->file_get_int(remminafile, "base-cred-for-gw", FALSE);

	if (basecredforgw) {
		ret = remmina_plugin_service->protocol_plugin_init_auth(gp,
									(disablepasswordstoring ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD) | REMMINA_MESSAGE_PANEL_FLAG_USERNAME | REMMINA_MESSAGE_PANEL_FLAG_DOMAIN,
									_("Enter RDP authentication credentials"),
									remmina_plugin_service->file_get_string(remminafile, "username"),
									remmina_plugin_service->file_get_string(remminafile, "password"),
									remmina_plugin_service->file_get_string(remminafile, "domain"),
									NULL);
	} else {
		ret = remmina_plugin_service->protocol_plugin_init_auth(gp,
									(disablepasswordstoring ? 0 : REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD) | REMMINA_MESSAGE_PANEL_FLAG_USERNAME | REMMINA_MESSAGE_PANEL_FLAG_DOMAIN,
									_("Enter RDP gateway authentication credentials"),
									remmina_plugin_service->file_get_string(remminafile, "gateway_username"),
									remmina_plugin_service->file_get_string(remminafile, "gateway_password"),
									remmina_plugin_service->file_get_string(remminafile, "gateway_domain"),
									NULL);
	}


	if (ret == GTK_RESPONSE_OK) {
		s_username = remmina_plugin_service->protocol_plugin_init_get_username(gp);
		if (s_username) freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayUsername, s_username);

		s_password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		if (s_password) freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayPassword, s_password);

		s_domain = remmina_plugin_service->protocol_plugin_init_get_domain(gp);
		if (s_domain) freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayDomain, s_domain);

		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);

		if (basecredforgw) {
			remmina_plugin_service->file_set_string(remminafile, "username", s_username);
			remmina_plugin_service->file_set_string(remminafile, "domain", s_domain);
			if (save)
				remmina_plugin_service->file_set_string(remminafile, "password", s_password);
			else
				remmina_plugin_service->file_set_string(remminafile, "password", NULL);
		} else {
			remmina_plugin_service->file_set_string(remminafile, "gateway_username", s_username);
			remmina_plugin_service->file_set_string(remminafile, "gateway_domain", s_domain);
			if (save)
				remmina_plugin_service->file_set_string(remminafile, "gateway_password", s_password);
			else
				remmina_plugin_service->file_set_string(remminafile, "gateway_password", NULL);
		}

		if (s_username) g_free(s_username);
		if (s_password) g_free(s_password);
		if (s_domain) g_free(s_domain);

		return true;
	} else {
		return false;
	}

	return true;
}
#else
static BOOL remmina_rdp_authenticate_ex(freerdp* instance, char** username, char** password,
                                char** domain, rdp_auth_reason reason)
{
	TRACE_CALL(__func__);
	gchar *s_username = NULL, *s_password = NULL, *s_domain = NULL;
	const gchar* key_user = NULL;
	const gchar* key_domain = NULL;
	const gchar* key_password = NULL;
	const gchar* key_title = NULL;
	gint ret;
	rfContext *rfi;
	RemminaProtocolWidget *gp;
	gboolean save;
	gboolean disablepasswordstoring;
	RemminaFile *remminafile;
	RemminaMessagePanelFlags flags = REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD | REMMINA_MESSAGE_PANEL_FLAG_USERNAME | REMMINA_MESSAGE_PANEL_FLAG_DOMAIN;

	rfi = (rfContext *)instance->context;
	gp = rfi->protocol_widget;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	disablepasswordstoring = remmina_plugin_service->file_get_int(remminafile, "disablepasswordstoring", FALSE);

	FreeRDP_Settings_Keys_String cfg_key_user = FreeRDP_STRING_UNUSED;
	FreeRDP_Settings_Keys_String cfg_key_domain = FreeRDP_STRING_UNUSED;
	FreeRDP_Settings_Keys_String cfg_key_password = FreeRDP_STRING_UNUSED;
	switch(reason) {
		case AUTH_NLA:
		case AUTH_TLS:
		case AUTH_RDP:
			if ((*username) && (*password)){
				return true;
			}
			key_title = _("Enter RDP authentication credentials");
			key_user = "username";
			key_domain = "domain";
			key_password = "password";
			cfg_key_user = FreeRDP_Username;
			cfg_key_domain = FreeRDP_Domain;
			cfg_key_password = FreeRDP_Password;
			break;
		case GW_AUTH_HTTP:
		case GW_AUTH_RDG:
		case GW_AUTH_RPC:
			key_title = _("Enter RDP gateway authentication credentials");
			key_user = "gateway_username";
			key_domain = "gateway_domain";
			key_password = "gateway_password";
			cfg_key_user = FreeRDP_GatewayUsername;
			cfg_key_domain = FreeRDP_GatewayDomain;
			cfg_key_password = FreeRDP_GatewayPassword;
			break;
		case AUTH_SMARTCARD_PIN:
			key_title = _("Enter RDP SmartCard PIN");
			key_password = "smartcard_pin";
			flags = 0;
			break;
		default:
			// TODO: Display an error dialog informing the user that the remote requires some mechanism FreeRDP or Remmina currently do not support
			g_fprintf(stderr, "[authentication] unsupported type %d, access denied", reason);
			return FALSE;
	}

	if (!disablepasswordstoring)
		flags |= REMMINA_MESSAGE_PANEL_FLAG_SAVEPASSWORD;

	ret = remmina_plugin_service->protocol_plugin_init_auth(gp, flags,
								key_title,
								remmina_plugin_service->file_get_string(remminafile, key_user),
								remmina_plugin_service->file_get_string(remminafile, key_password),
								remmina_plugin_service->file_get_string(remminafile, key_domain),
								NULL);
	if (ret == GTK_RESPONSE_OK) {
		if (cfg_key_user != FreeRDP_STRING_UNUSED)
		{
			s_username = remmina_plugin_service->protocol_plugin_init_get_username(gp);
			if (s_username)
				freerdp_settings_set_string(rfi->clientContext.context.settings, cfg_key_user, s_username);
			remmina_plugin_service->file_set_string(remminafile, key_user, s_username);
		}

		if (cfg_key_password != FreeRDP_STRING_UNUSED)
		{
			s_password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
			if (s_password)
				freerdp_settings_set_string(rfi->clientContext.context.settings, cfg_key_password, s_password);
		}

		if (cfg_key_domain != FreeRDP_STRING_UNUSED) {
			s_domain = remmina_plugin_service->protocol_plugin_init_get_domain(gp);
			if (s_domain)
				freerdp_settings_set_string(rfi->clientContext.context.settings, cfg_key_domain, s_domain);
			remmina_plugin_service->file_set_string(remminafile, key_domain, s_domain);
		}

		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save) {
			// User has requested to save credentials. We put the password
			// into remminafile->settings. It will be saved later, on successful connection, by
			// rcw.c
			remmina_plugin_service->file_set_string(remminafile, key_password, s_password);
		} else {
			remmina_plugin_service->file_set_string(remminafile, key_password, NULL);
		}


		if (s_username) g_free(s_username);
		if (s_password) g_free(s_password);
		if (s_domain) g_free(s_domain);

		return TRUE;
	} else {
		return FALSE;
	}

	return TRUE;
}

static BOOL remmina_rdp_choose_smartcard(freerdp* instance, SmartcardCertInfo** cert_list, DWORD count,
                                 DWORD* choice, BOOL gateway)
{
	// TODO: Display a simple list of smartcards/certs to choose from.
	// See client_cli_choose_smartcard for a sample
	return client_cli_choose_smartcard(instance, cert_list, count, choice, gateway);
}

static BOOL remmina_rdp_get_access_token(freerdp* instance, AccessTokenType tokenType, char** token,
                                    size_t count, ...)
{
	// TODO: Open a (currently hard coded) URL, authenticate in a webview/browser, return the access token.
	// See client_cli_get_access_token or sdl_webview_get_access_token for implementations
	return client_cli_get_access_token(instance, tokenType, token, count);
}

static BOOL remmina_rdp_present_gateway_message(freerdp* instance, UINT32 type, BOOL isDisplayMandatory,
                                           BOOL isConsentMandatory, size_t length,
                                           const WCHAR* message)
{
	if (!isConsentMandatory && !isDisplayMandatory){
		return TRUE;
	}

	rfContext *rfi;
	RemminaProtocolWidget *gp;
	rfi = (rfContext *)instance->context;
	gp = rfi->protocol_widget;
	int ret = remmina_plugin_service->protocol_widget_panel_accept(gp, (char*)message);
	if (ret == GTK_RESPONSE_YES){
		return TRUE;
	}
	else{
		return FALSE;
	}
}

static int remmina_rdp_logon_error_info(freerdp* instance, UINT32 data, UINT32 type)
{
	// TODO: Display the reason for connection termination
	// See client_cli_logon_error_info or sdl_logon_error_info
	return client_cli_logon_error_info(instance, data, type);
}

static SSIZE_T remmina_rdp_retry_dialog(freerdp* instance, const char* what, size_t current,
                               void* userarg)
{
	// TODO:
	// See client_common_retry_dialog or
	return client_common_retry_dialog(instance, what, current, userarg);
}

static void remmina_rdp_post_final_disconnect(freerdp* instance)
{
	// Clean up resources allocated in PreConnect
}
#endif

static DWORD remmina_rdp_verify_certificate_ex(freerdp *instance, const char *host, UINT16 port,
						   const char *common_name, const char *subject,
						   const char *issuer, const char *fingerprint, DWORD flags)
{
	TRACE_CALL(__func__);
	gint status;
	rfContext *rfi;
	RemminaProtocolWidget *gp;

	rfi = (rfContext *)instance->context;
	gp = rfi->protocol_widget;

	status = remmina_plugin_service->protocol_plugin_init_certificate(gp, subject, issuer, fingerprint);

	if (status == GTK_RESPONSE_OK)
		return 1;

	return 0;
}

static DWORD
remmina_rdp_verify_certificate(freerdp *instance, const char *common_name, const char *subject, const char *issuer, const char *fingerprint, BOOL host_mismatch) __attribute__ ((unused));
static DWORD
remmina_rdp_verify_certificate(freerdp *instance, const char *common_name, const char *subject,
				   const char *issuer, const char *fingerprint, BOOL host_mismatch)
{
	TRACE_CALL(__func__);
	gint status;
	rfContext *rfi;
	RemminaProtocolWidget *gp;

	rfi = (rfContext *)instance->context;
	gp = rfi->protocol_widget;

	status = remmina_plugin_service->protocol_plugin_init_certificate(gp, subject, issuer, fingerprint);

	if (status == GTK_RESPONSE_OK)
		return 1;

	return 0;
}

static DWORD remmina_rdp_verify_changed_certificate_ex(freerdp *instance, const char *host, UINT16 port,
							   const char *common_name, const char *subject,
							   const char *issuer, const char *fingerprint,
							   const char *old_subject, const char *old_issuer,
							   const char *old_fingerprint, DWORD flags)
{
	TRACE_CALL(__func__);
	gint status;
	rfContext *rfi;
	RemminaProtocolWidget *gp;

	rfi = (rfContext *)instance->context;
	gp = rfi->protocol_widget;

	status = remmina_plugin_service->protocol_plugin_changed_certificate(gp, subject, issuer, fingerprint, old_fingerprint);

	if (status == GTK_RESPONSE_OK)
		return 1;

	return 0;
}

static void remmina_rdp_post_disconnect(freerdp *instance)
{
	TRACE_CALL(__func__);

	if (!instance || !instance->context)
		return;

	PubSub_UnsubscribeChannelConnected(instance->context->pubSub,
					   remmina_rdp_OnChannelConnectedEventHandler);
	PubSub_UnsubscribeChannelDisconnected(instance->context->pubSub,
						  remmina_rdp_OnChannelDisconnectedEventHandler);

	/* The remaining cleanup will be continued on main thread by complete_cleanup_on_main_thread() */

	// With FreeRDP3 only resources allocated in PostConnect and later are cleaned up here.
}

static void remmina_rdp_main_loop(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	DWORD status;
	gchar buf[100];
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	time_t cur_time, time_diff_jitter, time_diff_keypress;

	int jitter_time = remmina_plugin_service->file_get_int(remminafile, "rdp_mouse_jitter", 0);
 	int keypress_time = remmina_plugin_service->file_get_int(remminafile, "rdp_idle_keypress_time", 0);
	int keypress_opts = remmina_plugin_service->file_get_int(remminafile, "rdp_idle_keypress_combo", 0);
	time(&(rfi->last_time));
	time(&(rfi->last_time_idle_keypress));
#if FREERDP_VERSION_MAJOR >= 3
	while (!freerdp_shall_disconnect_context(&rfi->clientContext.context)) {
#else
	while (!freerdp_shall_disconnect(rfi->clientContext.context.instance)) {
#endif
		// move mouse if we've been idle and option is selected
		time(&cur_time);
		time_diff_jitter = cur_time - rfi->last_time;
		if (jitter_time > 0 && time_diff_jitter > jitter_time){
			rfi->last_time = cur_time;
			remmina_rdp_mouse_jitter(gp);
		}
		// press key(s) if we've been idle and option is selected
		time(&cur_time);
		time_diff_keypress = cur_time - rfi->last_time_idle_keypress;		
		if (keypress_time > 0 && time_diff_keypress > keypress_time){
			rfi->last_time_idle_keypress = cur_time;
			remmina_rdp_idle_keypress(gp, &keypress_opts);
		}

		HANDLE handles[MAXIMUM_WAIT_OBJECTS] = {0};
		DWORD nCount = freerdp_get_event_handles(&rfi->clientContext.context, &handles[0], ARRAYSIZE(handles));
		if (rfi->event_handle)
			handles[nCount++] = rfi->event_handle;

		handles[nCount++] = freerdp_abort_event(&rfi->clientContext.context);

		if (nCount == 0) {
			fprintf(stderr, "freerdp_get_event_handles failed\n");
			break;
		}

		status = WaitForMultipleObjects(nCount, handles, FALSE, 100);

		if (status == WAIT_FAILED) {
			fprintf(stderr, "WaitForMultipleObjects failed with %lu\n", (unsigned long)status);
			break;
		}

		if (rfi->event_handle && WaitForSingleObject(rfi->event_handle, 0) == WAIT_OBJECT_0) {
			if (!rf_process_event_queue(gp)) {
				fprintf(stderr, "Could not process local keyboard/mouse event queue\n");
				break;
			}
			if (read(rfi->event_pipe[0], buf, sizeof(buf))) {
			}
		}

		/* Check if a processed event called freerdp_abort_connect() and exit if true */
		if (WaitForSingleObject(freerdp_abort_event(&rfi->clientContext.context), 0) == WAIT_OBJECT_0)
			/* Session disconnected by local user action */
			break;

		if (!freerdp_check_event_handles(&rfi->clientContext.context)) {
			if (rf_auto_reconnect(rfi)) {
				/* Reset the possible reason/error which made us doing many reconnection reattempts and continue */
				remmina_plugin_service->protocol_plugin_set_error(gp, NULL);
				continue;
			}
			if (freerdp_get_last_error(&rfi->clientContext.context) == FREERDP_ERROR_SUCCESS)
				fprintf(stderr, "Could not check FreeRDP file descriptor\n");
			break;
		}
	}
	const gchar *host = freerdp_settings_get_string (rfi->clientContext.context.settings, FreeRDP_ServerHostname);
	// TRANSLATORS: the placeholder may be either an IP/FQDN or a server hostname
	REMMINA_PLUGIN_AUDIT(_("Disconnected from %s via RDP"), host);
	freerdp_disconnect(rfi->clientContext.context.instance);
	REMMINA_PLUGIN_DEBUG("RDP client disconnected");
}

static int remmina_rdp_load_static_channel_addin(rdpChannels *channels, rdpSettings *settings, char *name, void *data)
{
	TRACE_CALL(__func__);
	PVIRTUALCHANNELENTRY entry = NULL;
	PVIRTUALCHANNELENTRYEX entryEx = NULL;

	entryEx = (PVIRTUALCHANNELENTRYEX)(void *)freerdp_load_channel_addin_entry(
		name, NULL, NULL, FREERDP_ADDIN_CHANNEL_STATIC | FREERDP_ADDIN_CHANNEL_ENTRYEX);

	if (!entryEx)
		entry = freerdp_load_channel_addin_entry(name, NULL, NULL, FREERDP_ADDIN_CHANNEL_STATIC);

	if (entryEx) {
		if (freerdp_channels_client_load_ex(channels, settings, entryEx, data) == 0) {
			fprintf(stderr, "loading channel %s\n", name);
			return TRUE;
		}
	} else if (entry) {
		if (freerdp_channels_client_load(channels, settings, entry, data) == 0) {
			fprintf(stderr, "loading channel %s\n", name);
			return TRUE;
		}
	}

	return FALSE;
}

static gchar *remmina_rdp_find_prdriver(char *smap, char *prn)
{
	char c, *p, *dr;
	int matching;
	size_t sz;

	enum { S_WAITPR,
	       S_INPRINTER,
	       S_WAITCOLON,
	       S_WAITDRIVER,
	       S_INDRIVER,
	       S_WAITSEMICOLON } state = S_WAITPR;

	matching = 0;
	while ((c = *smap++) != 0) {
		switch (state) {
		case S_WAITPR:
			if (c != '\"') return NULL;
			state = S_INPRINTER;
			p = prn;
			matching = 1;
			break;
		case S_INPRINTER:
			if (matching && c == *p && *p != 0) {
				p++;
			} else if (c == '\"') {
				if (*p != 0)
					matching = 0;
				state = S_WAITCOLON;
			} else {
				matching = 0;
			}
			break;
		case S_WAITCOLON:
			if (c != ':')
				return NULL;
			state = S_WAITDRIVER;
			break;
		case S_WAITDRIVER:
			if (c != '\"')
				return NULL;
			state = S_INDRIVER;
			dr = smap;
			break;
		case S_INDRIVER:
			if (c == '\"') {
				if (matching)
					goto found;
				else
					state = S_WAITSEMICOLON;
			}
			break;
		case S_WAITSEMICOLON:
			if (c != ';')
				return NULL;
			state = S_WAITPR;
			break;
		}
	}
	return NULL;

found:
	sz = smap - dr;
	p = (char *)malloc(sz);
	memcpy(p, dr, sz);
	p[sz - 1] = 0;
	return p;
}

#ifdef HAVE_CUPS
/**
 * Callback function used by cupsEnumDests
 *   - For each enumerated local printer tries to set the Printer Name and Driver.
 * @return 1 if there are other printers to scan or 0 when it's done.
 */
static int remmina_rdp_set_printers(void *user_data, unsigned flags, cups_dest_t *dest)
{
	rfContext *rfi = (rfContext *)user_data;
	RemminaProtocolWidget *gp = rfi->protocol_widget;

	/** @warning printer-make-and-model is not always the same as on the Windows,
	 * therefore it fails finding to right one and it fails to add
	 * the printer.
	 *
	 * We pass NULL and we do not check for errors. The following code is
	 * how it is supposed to work. @todo Ask CUPS mailing list for help.
	 *
	 * @code
	 * const char *model = cupsGetOption("printer-make-and-model",
	 * 		dest->num_options,
	 * 		dest->options);
	 * @endcode
	 */

	RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
	const gchar *s = remmina_plugin_service->file_get_string(remminafile, "printer_overrides");

	RDPDR_PRINTER *printer;
	printer = (RDPDR_PRINTER *)calloc(1, sizeof(RDPDR_PRINTER));

#if FREERDP_VERSION_MAJOR >= 3
	RDPDR_DEVICE *pdev;
	pdev = &(printer->device);
#else
	RDPDR_PRINTER *pdev;
	pdev = printer;
#endif

	pdev->Type = RDPDR_DTYP_PRINT;
	REMMINA_PLUGIN_DEBUG("Printer Type: %d", pdev->Type);

	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RedirectPrinters, TRUE);
	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_DeviceRedirection, TRUE);

	REMMINA_PLUGIN_DEBUG("Destination: %s", dest->name);
	if (!(pdev->Name = _strdup(dest->name))) {
		free(printer);
		return 1;
	}

	REMMINA_PLUGIN_DEBUG("Printer Name: %s", pdev->Name);

	if (s) {
		gchar *d = remmina_rdp_find_prdriver(strdup(s), pdev->Name);
		if (d) {
			printer->DriverName = strdup(d);
			REMMINA_PLUGIN_DEBUG("Printer DriverName set to: %s", printer->DriverName);
			g_free(d);
		} else {
			/**
			 * When remmina_rdp_find_prdriver doesn't return a DriverName
			 * it means that we don't want to share that printer
			 *
			 */
			free(pdev->Name);
			free(printer);
			return 1;
		}
	} else {
		/* We set to a default driver*/
		printer->DriverName = _strdup("MS Publisher Imagesetter");
	}

	REMMINA_PLUGIN_DEBUG("Printer Driver: %s", printer->DriverName);
	if (!freerdp_device_collection_add(rfi->clientContext.context.settings, (RDPDR_DEVICE *)printer)) {
		free(printer->DriverName);
		free(pdev->Name);
		free(printer);
		return 1;
	}

	return 1;
}
#endif /* HAVE_CUPS */

/* Send Ctrl+Alt+Del keystrokes to the plugin drawing_area widget */
static void remmina_rdp_send_ctrlaltdel(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	guint keys[] = { GDK_KEY_Control_L, GDK_KEY_Alt_L, GDK_KEY_Delete };
	rfContext *rfi = GET_PLUGIN_DATA(gp);

	remmina_plugin_service->protocol_plugin_send_keys_signals(rfi->drawing_area,
								  keys, G_N_ELEMENTS(keys), GDK_KEY_PRESS | GDK_KEY_RELEASE);
}

static gboolean remmina_rdp_set_connection_type(rdpSettings *settings, guint32 type)
{
	freerdp_settings_set_uint32(settings, FreeRDP_ConnectionType, type);

	if (type == CONNECTION_TYPE_MODEM) {
		freerdp_settings_set_bool(settings, FreeRDP_DisableWallpaper, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowFontSmoothing, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowDesktopComposition, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableFullWindowDrag, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableMenuAnims, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableThemes, TRUE);
	} else if (type == CONNECTION_TYPE_BROADBAND_LOW) {
		freerdp_settings_set_bool(settings, FreeRDP_DisableWallpaper, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowFontSmoothing, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowDesktopComposition, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableFullWindowDrag, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableMenuAnims, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableThemes, FALSE);
	} else if (type == CONNECTION_TYPE_SATELLITE) {
		freerdp_settings_set_bool(settings, FreeRDP_DisableWallpaper, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowFontSmoothing, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowDesktopComposition, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableFullWindowDrag, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableMenuAnims, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableThemes, FALSE);
	} else if (type == CONNECTION_TYPE_BROADBAND_HIGH) {
		freerdp_settings_set_bool(settings, FreeRDP_DisableWallpaper, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowFontSmoothing, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowDesktopComposition, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableFullWindowDrag, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableMenuAnims, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableThemes, FALSE);
	} else if (type == CONNECTION_TYPE_WAN) {
		freerdp_settings_set_bool(settings, FreeRDP_DisableWallpaper, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowFontSmoothing, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowDesktopComposition, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableFullWindowDrag, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableMenuAnims, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableThemes, FALSE);
	} else if (type == CONNECTION_TYPE_LAN) {
		freerdp_settings_set_bool(settings, FreeRDP_DisableWallpaper, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowFontSmoothing, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowDesktopComposition, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableFullWindowDrag, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableMenuAnims, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableThemes, FALSE);
	} else if (type == CONNECTION_TYPE_AUTODETECT) {
		freerdp_settings_set_bool(settings, FreeRDP_DisableWallpaper, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowFontSmoothing, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_AllowDesktopComposition, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableFullWindowDrag, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableMenuAnims, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_DisableThemes, FALSE);
		freerdp_settings_set_bool(settings, FreeRDP_NetworkAutoDetect, TRUE);

		/* Automatically activate GFX and RFX codec support */
#ifdef WITH_GFX_H264
		freerdp_settings_set_bool(settings, FreeRDP_GfxAVC444, gfx_h264_available);
		freerdp_settings_set_bool(settings, FreeRDP_GfxH264, gfx_h264_available);
#endif
		freerdp_settings_set_bool(settings, FreeRDP_RemoteFxCodec, TRUE);
		freerdp_settings_set_bool(settings, FreeRDP_SupportGraphicsPipeline, TRUE);
	} else if (type == REMMINA_CONNECTION_TYPE_NONE) {
		return FALSE;
	} else {
		return FALSE;
	}

	return TRUE;
}

#ifdef GDK_WINDOWING_X11
#if FREERDP_CHECK_VERSION(2, 3, 0)
static gchar *remmina_get_rdp_kbd_remap(const gchar *keymap)
{
	TRACE_CALL(__func__);
	guint *table;
	gchar keys[20];
	gchar *rdp_kbd_remap = NULL;
	gint i;
	Display *display;

	table = remmina_plugin_service->pref_keymap_get_table(keymap);
	if (!table)
		return rdp_kbd_remap;
	rdp_kbd_remap = g_malloc0(512);
	display = XOpenDisplay(0);
	for (i = 0; table[i] > 0; i += 2) {
		g_snprintf(keys, sizeof(keys), "0x%02x=0x%02x", freerdp_keyboard_get_rdp_scancode_from_x11_keycode(XKeysymToKeycode(display, table[i])),
			freerdp_keyboard_get_rdp_scancode_from_x11_keycode(XKeysymToKeycode(display, table[i + 1])));
		if (i > 0)
			g_strlcat(rdp_kbd_remap, ",", 512);
		g_strlcat(rdp_kbd_remap, keys, 512);
	}
	XCloseDisplay(display);

	return rdp_kbd_remap;
}
#endif
#endif

static gboolean remmina_rdp_main(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	const gchar *s;
	gchar *sm;
	gchar *value;
	const gchar *cs;
	RemminaFile *remminafile;
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	rdpChannels *channels;
	gchar *gateway_host;
	gint gateway_port;
	gchar *datapath = NULL;
	gboolean status = TRUE;
#ifdef GDK_WINDOWING_X11
	gchar *rdp_kbd_remap;
#endif
	gint i;

	gint desktopOrientation, desktopScaleFactor, deviceScaleFactor;

	channels = rfi->clientContext.context.channels;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	datapath = g_build_path("/",
				remmina_plugin_service->file_get_user_datadir(),
				"RDP",
				NULL);
	REMMINA_PLUGIN_DEBUG("RDP data path is %s", datapath);

	if ((datapath != NULL) && (datapath[0] != '\0'))
		if (access(datapath, W_OK) == 0)
			freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_ConfigPath, datapath);
	g_free(datapath);

	if (remmina_plugin_service->file_get_int(remminafile, "assistance_mode", 0)){
		rdpAssistanceFile* file = freerdp_assistance_file_new();
		if (!file){
			REMMINA_PLUGIN_DEBUG("Could not allocate assistance file structure");
			return FALSE;
		}
		
		if (remmina_plugin_service->file_get_string(remminafile, "assistance_file") == NULL || 
				remmina_plugin_service->file_get_string(remminafile, "assistance_pass") == NULL ){

			REMMINA_PLUGIN_DEBUG("Assistance file and password are not set while assistance mode is on");
			return FALSE;
		}

		status = freerdp_assistance_parse_file(file, 
			remmina_plugin_service->file_get_string(remminafile, "assistance_file"), 
			remmina_plugin_service->file_get_string(remminafile, "assistance_pass"));

		if (status < 0){
			REMMINA_PLUGIN_DEBUG("Could not parse assistance file");
			return FALSE;
		}
			

		if (!freerdp_assistance_populate_settings_from_assistance_file(file, rfi->clientContext.context.settings)){
			REMMINA_PLUGIN_DEBUG("Could not populate settings from assistance file");
			return FALSE;
		}		
	}


#if defined(PROXY_TYPE_IGNORE)
	if (!remmina_plugin_service->file_get_int(remminafile, "useproxyenv", FALSE) ? TRUE : FALSE) {
		REMMINA_PLUGIN_DEBUG("Not using system proxy settings");
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ProxyType, PROXY_TYPE_IGNORE);
	}
#endif

	if (!remmina_rdp_tunnel_init(gp))
		return FALSE;

	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AutoReconnectionEnabled, (remmina_plugin_service->file_get_int(remminafile, "disableautoreconnect", FALSE) ? FALSE : TRUE));
	/* Disable RDP auto reconnection when SSH tunnel is enabled */
	if (remmina_plugin_service->file_get_int(remminafile, "ssh_tunnel_enabled", FALSE))
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AutoReconnectionEnabled, FALSE);

	freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, remmina_plugin_service->file_get_int(remminafile, "colordepth", 99));

	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SoftwareGdi, TRUE);
	REMMINA_PLUGIN_DEBUG("gfx_h264_available: %d", gfx_h264_available);

	/* Avoid using H.264 modes if they are not available on libfreerdp */
	if (!gfx_h264_available &&
	    (freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth) == 65 ||
	     freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth) == 66))
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, 64); // Fallback to GFX RFX

	if (freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth) == 0) {
		/* RFX (Win7)*/
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RemoteFxCodec, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline, FALSE);
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, 32);
	} else if (freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth) == 63) {
		/* /gfx (RFX Progressive) (Win8) */
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, 32);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxH264, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxAVC444, FALSE);
	} else if (freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth) == 64) {
		/* /gfx:rfx (Win8) */
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, 32);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RemoteFxCodec, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxH264, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxAVC444, FALSE);
	} else if (freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth) == 65) {
		/* /gfx:avc420 (Win8.1) */
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, 32);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxH264, gfx_h264_available);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxAVC444, FALSE);
	} else if (freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth) == 66) {
		/* /gfx:avc444 (Win10) */
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, 32);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxH264, gfx_h264_available);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxAVC444, gfx_h264_available);
	} else if (freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth) == 99) {
		/* Automatic (Let the server choose its best format) */
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, 32);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RemoteFxCodec, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxH264, gfx_h264_available);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GfxAVC444, gfx_h264_available);
	}

	if (freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_RemoteFxCodec) ||
		freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_NSCodec) ||
		freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline)) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_FastPathOutput, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_FrameMarkerCommandEnabled, TRUE);
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ColorDepth, 32);
		rfi->bpp = 32;
	}

	gint w = remmina_plugin_service->get_profile_remote_width(gp);
	gint h = remmina_plugin_service->get_profile_remote_height(gp);
	/* multiple of 4 */
	w = (w + 3) & ~0x3;
	h = (h + 3) & ~0x3;
	freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth, w);
	freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight, h);
	REMMINA_PLUGIN_DEBUG("Resolution set by the user: %dx%d", w, h);

	/* Workaround for FreeRDP issue #5417: in GFX AVC modes we can't go under
	 * AVC_MIN_DESKTOP_WIDTH x AVC_MIN_DESKTOP_HEIGHT */
	if (freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline) &&
		freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_GfxH264)) {
		if (freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth) <
			AVC_MIN_DESKTOP_WIDTH)
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth,
							AVC_MIN_DESKTOP_WIDTH);
		if (freerdp_settings_get_uint32(rfi->clientContext.context.settings,
						FreeRDP_DesktopHeight) <
			AVC_MIN_DESKTOP_HEIGHT)
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight,
							AVC_MIN_DESKTOP_HEIGHT);
	}

	/* Workaround for FreeRDP issue #5119. This will make our horizontal resolution
	 * an even value, but it will add a vertical black 1 pixel line on the
	 * right of the desktop */
	if ((freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth) & 1) != 0) {
		UINT32 tmp = freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth);
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth, tmp - 1);
	}

	remmina_plugin_service->protocol_plugin_set_width(gp, freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth));
	remmina_plugin_service->protocol_plugin_set_height(gp, freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight));

	w = freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth);
	h = freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight);
	REMMINA_PLUGIN_DEBUG("Resolution set after workarounds: %dx%d", w, h);


	if (remmina_plugin_service->file_get_string(remminafile, "username"))
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Username, remmina_plugin_service->file_get_string(remminafile, "username"));

	if (remmina_plugin_service->file_get_string(remminafile, "domain"))
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Domain, remmina_plugin_service->file_get_string(remminafile, "domain"));

	s = remmina_plugin_service->file_get_string(remminafile, "password");
	if (s) freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Password, s);

	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AutoLogonEnabled, TRUE);

	/**
	 * Proxy support
	 * Proxy settings are hidden at the moment as an advanced feature
	 */
	gchar *proxy_type = g_strdup(remmina_plugin_service->file_get_string(remminafile, "proxy_type"));
	gchar *proxy_username = g_strdup(remmina_plugin_service->file_get_string(remminafile, "proxy_username"));
	gchar *proxy_password = g_strdup(remmina_plugin_service->file_get_string(remminafile, "proxy_password"));
	gchar *proxy_hostname = g_strdup(remmina_plugin_service->file_get_string(remminafile, "proxy_hostname"));
	gint proxy_port = remmina_plugin_service->file_get_int(remminafile, "proxy_port", 80);
	REMMINA_PLUGIN_DEBUG("proxy_type: %s", proxy_type);
	REMMINA_PLUGIN_DEBUG("proxy_username: %s", proxy_username);
	REMMINA_PLUGIN_DEBUG("proxy_password: %s", proxy_password);
	REMMINA_PLUGIN_DEBUG("proxy_hostname: %s", proxy_hostname);
	REMMINA_PLUGIN_DEBUG("proxy_port: %d", proxy_port);
	if (proxy_type && proxy_hostname) {
		if (g_strcmp0(proxy_type, "no_proxy") == 0)
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ProxyType, PROXY_TYPE_IGNORE);
		else if (g_strcmp0(proxy_type, "http") == 0)
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ProxyType, PROXY_TYPE_HTTP);
		else if (g_strcmp0(proxy_type, "socks5") == 0)
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ProxyType, PROXY_TYPE_SOCKS);
		else
			g_warning("Invalid proxy protocol, at the moment only no_proxy, HTTP and SOCKS5 are supported");
		REMMINA_PLUGIN_DEBUG("ProxyType set to: %" PRIu32, freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_ProxyType));
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_ProxyHostname, proxy_hostname);
		if (proxy_username)
			freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_ProxyUsername, proxy_username);
		if (proxy_password)
			freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_ProxyPassword, proxy_password);
		if (proxy_port)
			freerdp_settings_set_uint16(rfi->clientContext.context.settings, FreeRDP_ProxyPort, proxy_port);
	}
	g_free(proxy_hostname);
	g_free(proxy_username);
	g_free(proxy_password);

	if (remmina_plugin_service->file_get_int(remminafile, "base-cred-for-gw", FALSE)) {
		// Reset gateway credentials
		remmina_plugin_service->file_set_string(remminafile, "gateway_username", NULL);
		remmina_plugin_service->file_set_string(remminafile, "gateway_domain", NULL);
		remmina_plugin_service->file_set_string(remminafile, "gateway_password", NULL);
	}

	/* Remote Desktop Gateway server address */
	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayEnabled, FALSE);
	s = remmina_plugin_service->file_get_string(remminafile, "gateway_server");
	if (s) {
		cs = remmina_plugin_service->file_get_string(remminafile, "gwtransp");
#if FREERDP_CHECK_VERSION(2, 3, 1)
		if (remmina_plugin_service->file_get_int(remminafile, "websockets", FALSE))
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayHttpUseWebsockets, TRUE);
		else
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayHttpUseWebsockets, FALSE);
#endif
		if (g_strcmp0(cs, "http") == 0) {
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayRpcTransport, FALSE);
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayHttpTransport, TRUE);
		} else if (g_strcmp0(cs, "rpc") == 0) {
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayRpcTransport, TRUE);
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayHttpTransport, FALSE);
		} else if (g_strcmp0(cs, "auto") == 0) {
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayRpcTransport, TRUE);
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayHttpTransport, TRUE);
		}
		remmina_plugin_service->get_server_port(s, 443, &gateway_host, &gateway_port);
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayHostname, gateway_host);
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_GatewayPort, gateway_port);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayEnabled, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayUseSameCredentials, TRUE);
	}
	/* Remote Desktop Gateway domain */
	if (remmina_plugin_service->file_get_string(remminafile, "gateway_domain")) {
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayDomain, remmina_plugin_service->file_get_string(remminafile, "gateway_domain"));
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayUseSameCredentials, FALSE);
	}
	/* Remote Desktop Gateway username */
	if (remmina_plugin_service->file_get_string(remminafile, "gateway_username")) {
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayUsername, remmina_plugin_service->file_get_string(remminafile, "gateway_username"));
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayUseSameCredentials, FALSE);
	}
	/* Remote Desktop Gateway password */
	s = remmina_plugin_service->file_get_string(remminafile, "gateway_password");
	if (s) {
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayPassword, s);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayUseSameCredentials, FALSE);
	}
	/* If no different credentials were provided for the Remote Desktop Gateway
	 * use the same authentication credentials for the host */
	if (freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_GatewayEnabled) && freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_GatewayUseSameCredentials)) {
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayDomain, freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_Domain));
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayUsername, freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_Username));
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayPassword, freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_Password));
	}
	/* Remote Desktop Gateway usage */
	if (freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_GatewayEnabled))
		freerdp_set_gateway_usage_method(rfi->clientContext.context.settings,
						 remmina_plugin_service->file_get_int(remminafile, "gateway_usage", FALSE) ? TSC_PROXY_MODE_DETECT : TSC_PROXY_MODE_DIRECT);

	freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayAccessToken,
					remmina_plugin_service->file_get_string(remminafile, "gatewayaccesstoken"));

	freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_AuthenticationLevel, remmina_plugin_service->file_get_int(
						remminafile, "authentication level", freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_AuthenticationLevel)));

	/* Certificate ignore */
	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_IgnoreCertificate, remmina_plugin_service->file_get_int(remminafile, "cert_ignore", 0));
	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_OldLicenseBehaviour, remmina_plugin_service->file_get_int(remminafile, "old-license", 0));
	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AllowUnanouncedOrdersFromServer, remmina_plugin_service->file_get_int(remminafile, "relax-order-checks", 0));
	freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_GlyphSupportLevel, (remmina_plugin_service->file_get_int(remminafile, "glyph-cache", 0) ? GLYPH_SUPPORT_FULL : GLYPH_SUPPORT_NONE));

	if ((cs = remmina_plugin_service->file_get_string(remminafile, "clientname")))
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_ClientHostname, cs);
	else
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_ClientHostname, g_get_host_name());

	/* Client Build number is optional, if not specified defaults to 0, allow for comments to appear after number */
	if ((cs = remmina_plugin_service->file_get_string(remminafile, "clientbuild"))) {
		if (*cs) {
			UINT32 val = strtoul(cs, NULL, 0);
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_ClientBuild, val);
		}
	}


	if (remmina_plugin_service->file_get_string(remminafile, "loadbalanceinfo")) {
		const gchar *tmp = strdup(remmina_plugin_service->file_get_string(remminafile, "loadbalanceinfo"));
		freerdp_settings_set_pointer_len(rfi->clientContext.context.settings, FreeRDP_LoadBalanceInfo, tmp, strlen(tmp));
	}

	if (remmina_plugin_service->file_get_string(remminafile, "exec"))
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_AlternateShell, remmina_plugin_service->file_get_string(remminafile, "exec"));

	if (remmina_plugin_service->file_get_string(remminafile, "execpath"))
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_ShellWorkingDirectory, remmina_plugin_service->file_get_string(remminafile, "execpath"));

	sm = g_strdup_printf("rdp_quality_%i", remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0));
	value = remmina_plugin_service->pref_get_value(sm);
	g_free(sm);

	if (value && value[0]) {
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_PerformanceFlags, strtoul(value, NULL, 16));
	} else {
		switch (remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0)) {
		case 9:
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_PerformanceFlags, DEFAULT_QUALITY_9);
			break;

		case 2:
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_PerformanceFlags, DEFAULT_QUALITY_2);
			break;

		case 1:
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_PerformanceFlags, DEFAULT_QUALITY_1);
			break;

		case 0:
		default:
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_PerformanceFlags, DEFAULT_QUALITY_0);
			break;
		}
	}
	g_free(value);

	if ((cs = remmina_plugin_service->file_get_string(remminafile, "network"))) {
		guint32 type = 0;

		if (g_strcmp0(cs, "modem") == 0)
			type = CONNECTION_TYPE_MODEM;
		else if (g_strcmp0(cs, "broadband") == 0)
			type = CONNECTION_TYPE_BROADBAND_HIGH;
		else if (g_strcmp0(cs, "broadband-low") == 0)
			type = CONNECTION_TYPE_BROADBAND_LOW;
		else if (g_strcmp0(cs, "broadband-high") == 0)
			type = CONNECTION_TYPE_BROADBAND_HIGH;
		else if (g_strcmp0(cs, "wan") == 0)
			type = CONNECTION_TYPE_WAN;
		else if (g_strcmp0(cs, "lan") == 0)
			type = CONNECTION_TYPE_LAN;
		else if ((g_strcmp0(cs, "autodetect") == 0))
			type = CONNECTION_TYPE_AUTODETECT;
		else if ((g_strcmp0(cs, "none") == 0))
			type = REMMINA_CONNECTION_TYPE_NONE;
		else
			type = REMMINA_CONNECTION_TYPE_NONE;

		if (!remmina_rdp_set_connection_type(rfi->clientContext.context.settings, type))
			REMMINA_PLUGIN_DEBUG("Network settings not set");
	}

	/* PerformanceFlags bitmask need also to be split into BOOL variables
	 * like freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_DisableWallpaper, freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AllowFontSmoothing…
	 * or freerdp_get_param_bool() function will return the wrong value
	 */
	freerdp_performance_flags_split(rfi->clientContext.context.settings);

#ifdef GDK_WINDOWING_X11
#if FREERDP_CHECK_VERSION(2, 3, 0)
	rdp_kbd_remap = remmina_get_rdp_kbd_remap(remmina_plugin_service->file_get_string(remminafile, "keymap"));
	if (rdp_kbd_remap != NULL) {
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_KeyboardRemappingList, rdp_kbd_remap);
				REMMINA_PLUGIN_DEBUG(
					"rdp_keyboard_remapping_list: %s",
					freerdp_settings_get_string(rfi->clientContext.context.settings,
												FreeRDP_KeyboardRemappingList));
				g_free(rdp_kbd_remap);
	}
	else {
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_KeyboardRemappingList, remmina_plugin_service->pref_get_value("rdp_kbd_remap"));
				REMMINA_PLUGIN_DEBUG(
					"rdp_keyboard_remapping_list: %s",
					freerdp_settings_get_string(rfi->clientContext.context.settings,
												FreeRDP_KeyboardRemappingList));
		}
#endif
#endif

	freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_KeyboardLayout, remmina_rdp_settings_get_keyboard_layout());

	if (remmina_plugin_service->file_get_int(remminafile, "console", FALSE))
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ConsoleSession, TRUE);

	if (remmina_plugin_service->file_get_int(remminafile, "restricted-admin", FALSE)) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ConsoleSession, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RestrictedAdminModeRequired, TRUE);
	}

	if (remmina_plugin_service->file_get_string(remminafile, "pth")) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ConsoleSession, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RestrictedAdminModeRequired, TRUE);
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_PasswordHash, remmina_plugin_service->file_get_string(remminafile, "pth"));
		remmina_plugin_service->file_set_int(remminafile, "restricted-admin", TRUE);
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "security");
	if (g_strcmp0(cs, "rdp") == 0) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RdpSecurity, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_TlsSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_NlaSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ExtSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_UseRdpSecurityLayer, TRUE);
	} else if (g_strcmp0(cs, "tls") == 0) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RdpSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_TlsSecurity, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_NlaSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ExtSecurity, FALSE);
	} else if (g_strcmp0(cs, "nla") == 0) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RdpSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_TlsSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_NlaSecurity, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ExtSecurity, FALSE);
	} else if (g_strcmp0(cs, "ext") == 0) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RdpSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_TlsSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_NlaSecurity, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ExtSecurity, TRUE);
	} else {
		/* This is "-nego" switch of xfreerdp */
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_NegotiateSecurityLayer, TRUE);
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "tls-seclevel");
	if (cs && g_strcmp0(cs,"")!=0) {
		i = atoi(cs);
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_TlsSecLevel, i);
	}

	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_CompressionEnabled, TRUE);
	if (remmina_plugin_service->file_get_int(remminafile, "disable_fastpath", FALSE)) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_FastPathInput, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_FastPathOutput, FALSE);
	} else {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_FastPathInput, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_FastPathOutput, TRUE);
	}

	/* Orientation and scaling settings */
	remmina_rdp_settings_get_orientation_scale_prefs(&desktopOrientation, &desktopScaleFactor, &deviceScaleFactor);

	freerdp_settings_set_uint16(rfi->clientContext.context.settings, FreeRDP_DesktopOrientation, desktopOrientation);
	if (desktopScaleFactor != 0 && deviceScaleFactor != 0) {
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopScaleFactor, desktopScaleFactor);
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DeviceScaleFactor, deviceScaleFactor);
	}

	/* Try to enable "Display Control Virtual Channel Extension", needed to
	 * dynamically resize remote desktop. This will automatically open
	 * the "disp" dynamic channel, if available */
	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SupportDisplayControl, TRUE);

	if (freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_SupportDisplayControl)) {
		CLPARAM *d[1];
		int dcount;

		dcount = 1;
		d[0] = "disp";
		freerdp_client_add_dynamic_channel(rfi->clientContext.context.settings, dcount, d);
	}

	if (freerdp_settings_get_bool(rfi->clientContext.context.settings, FreeRDP_SupportGraphicsPipeline)) {
		CLPARAM *d[1];

		int dcount;

		dcount = 1;
		d[0] = "rdpgfx";
		freerdp_client_add_dynamic_channel(rfi->clientContext.context.settings, dcount, d);
	}

	/* Sound settings */
	cs = remmina_plugin_service->file_get_string(remminafile, "sound");
	if (g_strcmp0(cs, "remote") == 0) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RemoteConsoleAudio, TRUE);
	} else if ((cs != NULL && cs[0] != '\0') && g_str_has_prefix(cs, "local")) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AudioPlayback, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AudioCapture, TRUE);
	} else {
		/* Disable sound */
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AudioPlayback, FALSE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RemoteConsoleAudio, FALSE);
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "microphone");
	if (cs != NULL && cs[0] != '\0') {
		if (g_strcmp0(cs, "0") == 0) {
			REMMINA_PLUGIN_DEBUG("“microphone” was set to 0, setting to \"\"");
			remmina_plugin_service->file_set_string(remminafile, "microphone", "");
		} else {
			freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_AudioCapture, TRUE);
			REMMINA_PLUGIN_DEBUG("“microphone” set to “%s”", cs);
			CLPARAM **p;
			size_t count;

			p = remmina_rdp_CommandLineParseCommaSeparatedValuesEx("audin", g_strdup(cs), &count);

			freerdp_client_add_dynamic_channel(rfi->clientContext.context.settings, count, p);
			g_free(p);
		}
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "audio-output");
	if (cs != NULL && cs[0] != '\0') {
		REMMINA_PLUGIN_DEBUG("audio output set to %s", cs);
		CLPARAM **p;
		size_t count;

		p = remmina_rdp_CommandLineParseCommaSeparatedValuesEx("rdpsnd", g_strdup(cs), &count);
		status = freerdp_client_add_static_channel(rfi->clientContext.context.settings, count, p);
		if (status)
			status = freerdp_client_add_dynamic_channel(rfi->clientContext.context.settings, count, p);
		g_free(p);
	}


	cs = remmina_plugin_service->file_get_string(remminafile, "freerdp_log_level");
	if (cs != NULL && cs[0] != '\0')
		REMMINA_PLUGIN_DEBUG("Log level set to to %s", cs);
	else
		cs = g_strdup("INFO");
	wLog *root = WLog_GetRoot();
	WLog_SetStringLogLevel(root, cs);

	cs = remmina_plugin_service->file_get_string(remminafile, "freerdp_log_filters");
	if (cs != NULL && cs[0] != '\0') {
		REMMINA_PLUGIN_DEBUG("Log filters set to to %s", cs);
		WLog_AddStringLogFilters(cs);
	} else {
		WLog_AddStringLogFilters(NULL);
	}


	cs = remmina_plugin_service->file_get_string(remminafile, "usb");
	if (cs != NULL && cs[0] != '\0') {
		CLPARAM **p;
		size_t count;
		p = remmina_rdp_CommandLineParseCommaSeparatedValuesEx("urbdrc", g_strdup(cs), &count);
		freerdp_client_add_dynamic_channel(rfi->clientContext.context.settings, count, p);
		g_free(p);
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "vc");
	if (cs != NULL && cs[0] != '\0') {
		CLPARAM **p;
		size_t count;
		p = remmina_rdp_CommandLineParseCommaSeparatedValues(g_strdup(cs), &count);
		freerdp_client_add_static_channel(rfi->clientContext.context.settings, count, p);
		g_free(p);
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "dvc");
	if (cs != NULL && cs[0] != '\0') {
		CLPARAM **p;
		size_t count;
		p = remmina_rdp_CommandLineParseCommaSeparatedValues(g_strdup(cs), &count);
		freerdp_client_add_dynamic_channel(rfi->clientContext.context.settings, count, p);
		g_free(p);
	}

	cs = remmina_plugin_service->file_get_string(remminafile, RDP2TCP_DVC_CHANNEL_NAME);
	if (cs != NULL && cs[0] != '\0') {
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_RDP2TCPArgs, cs);

#if FREERDP_VERSION_MAJOR >= 3
		char* args = freerdp_settings_get_string_writable(rfi->clientContext.context.settings, FreeRDP_RDP2TCPArgs);
#else
		char* args = freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_RDP2TCPArgs);
#endif
		REMMINA_PLUGIN_DEBUG(RDP2TCP_DVC_CHANNEL_NAME " set to %s",args);
		remmina_rdp_load_static_channel_addin(channels, rfi->clientContext.context.settings, RDP2TCP_DVC_CHANNEL_NAME, args);
	}

	int vermaj, vermin, verrev;
	freerdp_get_version(&vermaj, &vermin, &verrev);

#if FREERDP_CHECK_VERSION(2, 1, 0)
	cs = remmina_plugin_service->file_get_string(remminafile, "timeout");
	if (cs != NULL && cs[0] != '\0') {
		const gchar *endptr = NULL;
		guint64 val = g_ascii_strtoull(cs, (gchar **)&endptr, 10);
		if (val > 600000 || val <= 0)
			val = 600000;
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_TcpAckTimeout, (UINT32)val);
	}
#endif

	if (remmina_plugin_service->file_get_int(remminafile, "preferipv6", FALSE) ? TRUE : FALSE)
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_PreferIPv6OverIPv4, TRUE);

	freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RedirectClipboard, remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE) ? FALSE : TRUE);

	cs = remmina_plugin_service->file_get_string(remminafile, "sharefolder");
	if (cs != NULL && cs[0] != '\0') {
		gchar *ccs = g_strdup(cs);
		REMMINA_PLUGIN_DEBUG("[Deprecated->migrating] - Old sharefolder %s to \"drive \"", ccs);
		if (!remmina_plugin_service->file_get_string(remminafile, "drive")) {
			remmina_plugin_service->file_set_string(remminafile, "drive", g_strdup(ccs));
			remmina_plugin_service->file_set_string(remminafile, "sharefolder", NULL);
			REMMINA_PLUGIN_DEBUG("[Deprecated->migrated] - drive set to %s", g_strdup(ccs));
		}
		g_free(ccs);
	}
	cs = remmina_plugin_service->file_get_string(remminafile, "drive");
	if (cs != NULL && cs[0] != '\0') {
		REMMINA_PLUGIN_DEBUG("Redirect directory set to %s", cs);
		CLPARAM **p;
		size_t count;

		gchar **folders = g_strsplit(cs, ";", -1);
		for (i = 0; folders[i] != NULL; i++) {
			REMMINA_PLUGIN_DEBUG("Parsing folder %s", folders[i]);
			p = remmina_rdp_CommandLineParseCommaSeparatedValuesEx("drive", g_strdup(folders[i]), &count);
			status = freerdp_client_add_device_channel(rfi->clientContext.context.settings, count, p);
			g_free(p);
		}
		g_strfreev(folders);
	}

	if (remmina_plugin_service->file_get_int(remminafile, "shareprinter", FALSE)) {
#ifdef HAVE_CUPS
		REMMINA_PLUGIN_DEBUG("Sharing printers");
		const gchar *po = remmina_plugin_service->file_get_string(remminafile, "printer_overrides");
		if (po && po[0] != 0) {
			/* Fallback to remmina code to override print drivers */
			if (cupsEnumDests(CUPS_DEST_FLAGS_NONE, 1000, NULL, 0, 0, remmina_rdp_set_printers, rfi))
				REMMINA_PLUGIN_DEBUG("All printers have been shared");
			else
				REMMINA_PLUGIN_DEBUG("Cannot share printers, are there any available?");
		} else {
			/* Use libfreerdp code to map all printers */
			CLPARAM *d[1];
			int dcount;
			dcount = 1;
			d[0] = "printer";
			freerdp_client_add_device_channel(rfi->clientContext.context.settings, dcount, d);
		}
#endif /* HAVE_CUPS */
	}

	if (remmina_plugin_service->file_get_int(remminafile, "span", FALSE)) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SpanMonitors, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_UseMultimon, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_Fullscreen, TRUE);
		remmina_plugin_service->file_set_int(remminafile, "multimon", 1);
	}

	if (remmina_plugin_service->file_get_int(remminafile, "multimon", FALSE)) {
		guint32 maxwidth = 0;
		guint32 maxheight = 0;
		gchar *monitorids;
		guint32 i;
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_UseMultimon, TRUE);
		/* TODO Add an option for this */
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ForceMultimon, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_Fullscreen, TRUE);

		gchar *monitorids_string = g_strdup(remmina_plugin_service->file_get_string(remminafile, "monitorids"));
		/* Otherwise we get all the attached monitors
		 * monitorids may contains desktop orientation values.
		 * But before we check if there are orientation attributes
		 */
		if (monitorids_string != NULL && monitorids_string[0] != '\0') {
			if (g_strstr_len(monitorids_string, -1, ",") != NULL) {
				if (g_strstr_len(monitorids_string, -1, ":") != NULL) {
					rdpMonitor *base = (rdpMonitor *)freerdp_settings_get_pointer(rfi->clientContext.context.settings, FreeRDP_MonitorDefArray);
					/* We have an ID and an orientation degree */
					gchar **temp_items;
					gchar **rot_items;
					temp_items = g_strsplit(monitorids_string, ",", -1);
					for (i = 0; i < g_strv_length(temp_items); i++) {
						rot_items = g_strsplit(temp_items[i], ":", -1);
						rdpMonitor *current = &base[atoi(rot_items[0])];
						if (i == 0)
							monitorids = g_strdup(rot_items[0]);
						else
							monitorids = g_strdup_printf("%s,%s", monitorids, rot_items[0]);
						current->attributes.orientation = atoi(rot_items[1]);
						REMMINA_PLUGIN_DEBUG("Monitor n %d orientation: %d", i, current->attributes.orientation);
					}
				} else {
					monitorids = g_strdup(monitorids_string);
				}
			} else {
				monitorids = g_strdup(monitorids_string);
			}
		} else {
			monitorids = g_strdup(monitorids_string);
		}
		remmina_rdp_monitor_get(rfi, &monitorids, &maxwidth, &maxheight);
		if (monitorids != NULL && monitorids[0] != '\0') {
			UINT32 *base = (UINT32 *)freerdp_settings_get_pointer(rfi->clientContext.context.settings, FreeRDP_MonitorIds);
			gchar **items;
			items = g_strsplit(monitorids, ",", -1);
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_NumMonitorIds, g_strv_length(items));
			REMMINA_PLUGIN_DEBUG("NumMonitorIds: %d", freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_NumMonitorIds));
			for (i = 0; i < g_strv_length(items); i++) {
				if (base != NULL){
					UINT32 *current = &base[i];
					*current = atoi(items[i]);
					REMMINA_PLUGIN_DEBUG("Added monitor with ID %" PRIu32, *current);
				}
			}
			g_free(monitorids);
			g_strfreev(items);
		}
		if (maxwidth && maxheight) {
			REMMINA_PLUGIN_DEBUG("Setting DesktopWidth and DesktopHeight to: %dx%d", maxwidth, maxheight);
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth, maxwidth);
			freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight, maxheight);
			REMMINA_PLUGIN_DEBUG("DesktopWidth and DesktopHeight set to: %dx%d", freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth), freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight));
		} else {
			REMMINA_PLUGIN_DEBUG("Cannot set Desktop Size, we are using the previously set values: %dx%d", freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth), freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight));
		}
		remmina_plugin_service->protocol_plugin_set_width(gp, freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopWidth));
		remmina_plugin_service->protocol_plugin_set_height(gp, freerdp_settings_get_uint32(rfi->clientContext.context.settings, FreeRDP_DesktopHeight));
	}

	const gchar *sn = remmina_plugin_service->file_get_string(remminafile, "smartcardname");
	if (remmina_plugin_service->file_get_int(remminafile, "sharesmartcard", FALSE) ||
			(sn != NULL && sn[0] != '\0')) {
		RDPDR_SMARTCARD *smartcard;
		smartcard = (RDPDR_SMARTCARD *)calloc(1, sizeof(RDPDR_SMARTCARD));

#if FREERDP_VERSION_MAJOR >= 3
		RDPDR_DEVICE *sdev;
		sdev = &(smartcard->device);
#else
		RDPDR_SMARTCARD *sdev;
		sdev = smartcard;
#endif

		sdev->Type = RDPDR_DTYP_SMARTCARD;

		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_DeviceRedirection, TRUE);

		if (sn != NULL && sn[0] != '\0')
			sdev->Name = _strdup(sn);

		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RedirectSmartCards, TRUE);

		freerdp_device_collection_add(rfi->clientContext.context.settings, (RDPDR_DEVICE *)smartcard);
	}

	if (remmina_plugin_service->file_get_int(remminafile, "passwordispin", FALSE))
		/* Option works only combined with Username and Domain, because FreeRDP
		 * doesn’t know anything about info on smart card */
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_PasswordIsSmartcardPin, TRUE);

	/* /serial[:<name>[,<path>[,<driver>[,permissive]]]] */
	if (remmina_plugin_service->file_get_int(remminafile, "shareserial", FALSE)) {
		RDPDR_SERIAL *serial;
		serial = (RDPDR_SERIAL *)calloc(1, sizeof(RDPDR_SERIAL));

#if FREERDP_VERSION_MAJOR >= 3
		RDPDR_DEVICE *sdev;
		sdev = &(serial->device);
#else
		RDPDR_SERIAL *sdev;
		sdev = serial;
#endif

		sdev->Type = RDPDR_DTYP_SERIAL;

		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_DeviceRedirection, TRUE);

		const gchar *sn = remmina_plugin_service->file_get_string(remminafile, "serialname");
		if (sn != NULL && sn[0] != '\0')
			sdev->Name = _strdup(sn);

		const gchar *sd = remmina_plugin_service->file_get_string(remminafile, "serialdriver");
		if (sd != NULL && sd[0] != '\0')
			serial->Driver = _strdup(sd);

		const gchar *sp = remmina_plugin_service->file_get_string(remminafile, "serialpath");
		if (sp != NULL && sp[0] != '\0')
			serial->Path = _strdup(sp);

		if (remmina_plugin_service->file_get_int(remminafile, "serialpermissive", FALSE))
			serial->Permissive = _strdup("permissive");

		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RedirectSerialPorts, TRUE);

		freerdp_device_collection_add(rfi->clientContext.context.settings, (RDPDR_DEVICE *)serial);
	}

	if (remmina_plugin_service->file_get_int(remminafile, "shareparallel", FALSE)) {
		RDPDR_PARALLEL *parallel;
		parallel = (RDPDR_PARALLEL *)calloc(1, sizeof(RDPDR_PARALLEL));

#if FREERDP_VERSION_MAJOR >= 3
		RDPDR_DEVICE *pdev;
		pdev = &(parallel->device);
#else
		RDPDR_PARALLEL *pdev;
		pdev = parallel;
#endif

		pdev->Type = RDPDR_DTYP_PARALLEL;

		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_DeviceRedirection, TRUE);

		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_RedirectParallelPorts, TRUE);

		const gchar *pn = remmina_plugin_service->file_get_string(remminafile, "parallelname");
		if (pn != NULL && pn[0] != '\0')
			pdev->Name = _strdup(pn);
		const gchar *dp = remmina_plugin_service->file_get_string(remminafile, "parallelpath");
		if (dp != NULL && dp[0] != '\0')
			parallel->Path = _strdup(dp);

		freerdp_device_collection_add(rfi->clientContext.context.settings, (RDPDR_DEVICE *)parallel);
	}

	/**
	 * multitransport enables RDP8 UDP support
	 */
	if (remmina_plugin_service->file_get_int(remminafile, "multitransport", FALSE)) {
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_DeviceRedirection, TRUE);
		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_SupportMultitransport, TRUE);
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_MultitransportFlags,
						(TRANSPORT_TYPE_UDP_FECR | TRANSPORT_TYPE_UDP_FECL | TRANSPORT_TYPE_UDP_PREFERRED));
	} else {
		freerdp_settings_set_uint32(rfi->clientContext.context.settings, FreeRDP_MultitransportFlags, 0);
	}

	/* If needed, force interactive authentication by deleting all authentication fields,
	 * forcing libfreerdp to call our callbacks for authentication.
	 *	  This usually happens from a second attempt of connection, never on the 1st one. */
	if (rfi->attempt_interactive_authentication) {
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Username, NULL);
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Password, NULL);
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_Domain, NULL);

		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayDomain, NULL);
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayUsername, NULL);
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_GatewayPassword, NULL);

		freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_GatewayUseSameCredentials, FALSE);
	}

	gboolean orphaned;

	if (!freerdp_connect(rfi->clientContext.context.instance)) {
		orphaned = (GET_PLUGIN_DATA(rfi->protocol_widget) == NULL);
		if (!orphaned) {
			UINT32 e;

			e = freerdp_get_last_error(&rfi->clientContext.context);

			switch (e) {
			case FREERDP_ERROR_AUTHENTICATION_FAILED:
			case STATUS_LOGON_FAILURE:						// wrong return code from FreeRDP introduced at the end of July 2016? (fixed with b86c0ba)
#ifdef FREERDP_ERROR_CONNECT_LOGON_FAILURE
			case FREERDP_ERROR_CONNECT_LOGON_FAILURE:
#endif
				/* Logon failure, will retry with interactive authentication */
				rfi->attempt_interactive_authentication = TRUE;
				break;
			case STATUS_ACCOUNT_LOCKED_OUT:
#ifdef FREERDP_ERROR_CONNECT_ACCOUNT_LOCKED_OUT
			case FREERDP_ERROR_CONNECT_ACCOUNT_LOCKED_OUT:
#endif
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not access the RDP server “%s”.\nAccount locked out."),
										  freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
			case STATUS_ACCOUNT_EXPIRED:
#ifdef FREERDP_ERROR_CONNECT_ACCOUNT_EXPIRED
			case FREERDP_ERROR_CONNECT_ACCOUNT_EXPIRED:
#endif
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not access the RDP server “%s”.\nAccount expired."),
										  freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
			case STATUS_PASSWORD_EXPIRED:
#ifdef FREERDP_ERROR_CONNECT_PASSWORD_EXPIRED
			case FREERDP_ERROR_CONNECT_PASSWORD_EXPIRED:
#endif
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not access the RDP server “%s”.\nPassword expired."),
										  freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
			case STATUS_ACCOUNT_DISABLED:
#ifdef FREERDP_ERROR_CONNECT_ACCOUNT_DISABLED
			case FREERDP_ERROR_CONNECT_ACCOUNT_DISABLED:
#endif
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not access the RDP server “%s”.\nAccount disabled."),
										  freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
#ifdef FREERDP_ERROR_SERVER_INSUFFICIENT_PRIVILEGES
			/* https://msdn.microsoft.com/en-us/library/ee392247.aspx */
			case FREERDP_ERROR_SERVER_INSUFFICIENT_PRIVILEGES:
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not access the RDP server “%s”.\nInsufficient user privileges."),
										  freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
#endif
			case STATUS_ACCOUNT_RESTRICTION:
#ifdef FREERDP_ERROR_CONNECT_ACCOUNT_RESTRICTION
			case FREERDP_ERROR_CONNECT_ACCOUNT_RESTRICTION:
#endif
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not access the RDP server “%s”.\nAccount restricted."),
										  freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;

			case STATUS_PASSWORD_MUST_CHANGE:
#ifdef FREERDP_ERROR_CONNECT_PASSWORD_MUST_CHANGE
			case FREERDP_ERROR_CONNECT_PASSWORD_MUST_CHANGE:
#endif
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not access the RDP server “%s”.\nChange user password before connecting."),
										  freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;

			case FREERDP_ERROR_CONNECT_FAILED:
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Lost connection to the RDP server “%s”."), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
			case FREERDP_ERROR_DNS_NAME_NOT_FOUND:
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not find the address for the RDP server “%s”."), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
			case FREERDP_ERROR_TLS_CONNECT_FAILED:
				remmina_plugin_service->protocol_plugin_set_error(gp,
										  _("Could not connect to the RDP server “%s” via TLS. See the DEBUG traces from a terminal for more information."), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
			case FREERDP_ERROR_SECURITY_NEGO_CONNECT_FAILED:
				// TRANSLATORS: the placeholder may be either an IP/FQDN or a server hostname
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Unable to establish a connection to the RDP server “%s”. Check “Security protocol negotiation”."), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
#ifdef FREERDP_ERROR_POST_CONNECT_FAILED
			case FREERDP_ERROR_POST_CONNECT_FAILED:
				/* remmina_rdp_post_connect() returned FALSE to libfreerdp. We saved the error on rfi->postconnect_error */
				switch (rfi->postconnect_error) {
				case REMMINA_POSTCONNECT_ERROR_OK:
					/* We should never come here */
					remmina_plugin_service->protocol_plugin_set_error(gp, _("Cannot connect to the RDP server “%s”."), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
					break;
				case REMMINA_POSTCONNECT_ERROR_GDI_INIT:
					remmina_plugin_service->protocol_plugin_set_error(gp, _("Could not start libfreerdp-gdi."));
					break;
				case REMMINA_POSTCONNECT_ERROR_NO_H264:
					remmina_plugin_service->protocol_plugin_set_error(gp, _("You requested a H.264 GFX mode for the server “%s”, but your libfreerdp does not support H.264. Please use a non-AVC colour depth setting."), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
					break;
				}
				break;
#endif
#ifdef FREERDP_ERROR_SERVER_DENIED_CONNECTION
			case FREERDP_ERROR_SERVER_DENIED_CONNECTION:
				remmina_plugin_service->protocol_plugin_set_error(gp, _("The “%s” server refused the connection."), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
#endif
			case 0x800759DB:
				// E_PROXY_NAP_ACCESSDENIED https://docs.microsoft.com/en-us/openspecs/windows_protocols/ms-tsgu/84cd92e4-592c-4219-95d8-18021ac654b0
				remmina_plugin_service->protocol_plugin_set_error(gp, _("The Remote Desktop Gateway “%s” denied the user “%s\\%s” access due to policy."),
										  freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_GatewayHostname), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_GatewayDomain), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_GatewayUsername));
				break;

			case FREERDP_ERROR_CONNECT_NO_OR_MISSING_CREDENTIALS:
				rfi->user_cancelled = TRUE;
				break;

			default:
				g_printf("libfreerdp returned code is %08X\n", e);
				remmina_plugin_service->protocol_plugin_set_error(gp, _("Cannot connect to the “%s” RDP server."), freerdp_settings_get_string(rfi->clientContext.context.settings, FreeRDP_ServerHostname));
				break;
			}
		}

		return FALSE;
	}

	if (GET_PLUGIN_DATA(rfi->protocol_widget) == NULL) orphaned = true; else orphaned = false;
	if (!orphaned && freerdp_get_last_error(&rfi->clientContext.context) == FREERDP_ERROR_SUCCESS && !rfi->user_cancelled)
		remmina_rdp_main_loop(gp);

	return TRUE;
}

static void rfi_uninit(rfContext *rfi)
{
	freerdp *instance;

	instance = rfi->clientContext.context.instance;

	if (rfi->remmina_plugin_thread) {
		rfi->thread_cancelled = TRUE;   // Avoid all rf_queue function to run
		pthread_cancel(rfi->remmina_plugin_thread);
		if (rfi->remmina_plugin_thread)
			pthread_join(rfi->remmina_plugin_thread, NULL);
	}

	if (instance) {
		if (rfi->connected) {
			freerdp_abort_connect_context(&rfi->clientContext.context);
			rfi->connected = false;
		}
	}

	if (instance) {
		RDP_CLIENT_ENTRY_POINTS *pEntryPoints = instance->pClientEntryPoints;
		if (pEntryPoints)
			IFCALL(pEntryPoints->GlobalUninit);
		free(instance->pClientEntryPoints);
		freerdp_context_free(instance); /* context is rfContext* rfi */
		freerdp_free(instance);		 /* This implicitly frees instance->context and rfi is no longer valid */
	}
}

static gboolean complete_cleanup_on_main_thread(gpointer data)
{
	TRACE_CALL(__func__);

	gboolean orphaned;
	rfContext *rfi = (rfContext *)data;
	RemminaProtocolWidget *gp;

	remmina_rdp_clipboard_free(rfi);

	gdi_free(rfi->clientContext.context.instance);

	gp = rfi->protocol_widget;
	if (GET_PLUGIN_DATA(gp) == NULL) orphaned = true; else orphaned = false;

	remmina_rdp_cliprdr_detach_owner(gp);
	if (!orphaned) remmina_rdp_event_uninit(gp);

	if (!orphaned) g_object_steal_data(G_OBJECT(gp), "plugin-data");

	rfi_uninit(rfi);

	/* Notify the RemminaProtocolWidget that we closed our connection, and the GUI interface
	 * can be removed */
	if (!orphaned)
		remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);

	return G_SOURCE_REMOVE;
}

static gpointer remmina_rdp_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp;
	rfContext *rfi;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	CANCEL_ASYNC

		gp = (RemminaProtocolWidget *)data;

	rfi = GET_PLUGIN_DATA(gp);

	rfi->attempt_interactive_authentication = FALSE;
	do
		remmina_rdp_main(gp);
	while (!remmina_plugin_service->protocol_plugin_has_error(gp) && rfi->attempt_interactive_authentication == TRUE && !rfi->user_cancelled);

	rfi->remmina_plugin_thread = 0;

	/* cleanup */
	g_idle_add(complete_cleanup_on_main_thread, (gpointer)rfi);


	return NULL;
}

static void remmina_rdp_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	freerdp *instance;
	rfContext *rfi;
	gchar* auth_list;

	instance = freerdp_new();
	instance->PreConnect = remmina_rdp_pre_connect;
	instance->PostConnect = remmina_rdp_post_connect;
	instance->PostDisconnect = remmina_rdp_post_disconnect;
	instance->VerifyCertificateEx = remmina_rdp_verify_certificate_ex;
	instance->VerifyChangedCertificateEx = remmina_rdp_verify_changed_certificate_ex;
#if FREERDP_VERSION_MAJOR >= 3
	instance->AuthenticateEx = remmina_rdp_authenticate_ex;
	instance->ChooseSmartcard = remmina_rdp_choose_smartcard;
	instance->GetAccessToken = remmina_rdp_get_access_token;
	instance->LoadChannels = freerdp_client_load_channels;
	instance->PresentGatewayMessage = remmina_rdp_present_gateway_message;
	instance->LogonErrorInfo = remmina_rdp_logon_error_info;
	instance->RetryDialog = remmina_rdp_retry_dialog;
    instance->PostFinalDisconnect = remmina_rdp_post_final_disconnect;
#else
	instance->Authenticate = remmina_rdp_authenticate;
	instance->GatewayAuthenticate = remmina_rdp_gw_authenticate;
#endif

	instance->ContextSize = sizeof(rfContext);
	freerdp_context_new(instance);
	rfi = (rfContext *)instance->context;

	g_object_set_data_full(G_OBJECT(gp), "plugin-data", rfi, free);

	rfi->protocol_widget = gp;
	rfi->clientContext.context.settings = instance->context->settings;
	rfi->connected = false;
	rfi->is_reconnecting = false;
	rfi->stop_reconnecting_requested = false;
	rfi->user_cancelled = FALSE;
	rfi->last_x = 0;
	rfi->last_y = 0;

	freerdp_register_addin_provider(freerdp_channels_load_static_addin_entry, 0);
#if FREERDP_VERSION_MAJOR >= 3
	auth_list = remmina_plugin_service->pref_get_value("rdp_auth_filter");
	if (auth_list != NULL && auth_list[0] != 0){
		freerdp_settings_set_string(rfi->clientContext.context.settings, FreeRDP_AuthenticationPackageList, auth_list);
	}
	g_free(auth_list);
	
#endif
	remmina_rdp_event_init(gp);
}

static gboolean remmina_rdp_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	const gchar *profile_name, *p;
	gchar thname[16], c;
	gint nthname = 0;

	rfi->scale = remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp);

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);


	if (pthread_create(&rfi->remmina_plugin_thread, NULL, remmina_rdp_main_thread, gp)) {
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s",
								  "Could not start pthread.");

		rfi->remmina_plugin_thread = 0;

		return FALSE;
	}

	/* Generate a thread name to be used with pthread_setname_np() for debugging */
	profile_name = remmina_plugin_service->file_get_string(remminafile, "name");
	p = profile_name;
	strcpy(thname, "RemmRDP:");
	if (p) {
		nthname = strlen(thname);
		while ((c = *p) != 0 && nthname < sizeof(thname) - 1) {
			if (isalnum(c))
				thname[nthname++] = c;
			p++;
		}
	} else {
		strcat(thname, "<NONAM>");
		nthname = strlen(thname);
	}
	thname[nthname] = 0;
#if defined(__linux__)
	pthread_setname_np(rfi->remmina_plugin_thread, thname);
#elif defined(__FreeBSD__)
	pthread_set_name_np(rfi->remmina_plugin_thread, thname);
#endif

	return TRUE;
}

static gboolean remmina_rdp_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	REMMINA_PLUGIN_DEBUG("Requesting to close the connection");
	RemminaPluginRdpEvent rdp_event = { 0 };
	rfContext *rfi = GET_PLUGIN_DATA(gp);

	if (!remmina_plugin_service->is_main_thread())
		g_warning("WARNING: %s called on a subthread, which may not work or crash Remmina.", __func__);

	if (rfi && !rfi->connected) {
		/* libfreerdp is attempting to connect, we cannot interrupt our main thread
		 * in the connect phase.
		 * So we remove "plugin-data" from gp, so our rfi remains "orphan"
		 */
		remmina_rdp_event_uninit(gp);
		g_object_steal_data(G_OBJECT(gp), "plugin-data");
		remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
		return FALSE;
	}


	if (rfi && rfi->clipboard.srv_clip_data_wait == SCDW_BUSY_WAIT) {
		REMMINA_PLUGIN_DEBUG("[RDP] requesting clipboard transfer to abort");
		/* Allow clipboard transfer from server to terminate */
		rfi->clipboard.srv_clip_data_wait = SCDW_ABORTING;
		usleep(100000);
	}

	if (rfi->is_reconnecting) {
		/* Special case: window closed when attempting to reconnect */
		rfi->stop_reconnecting_requested = TRUE;
		return FALSE;
	}

	rdp_event.type = REMMINA_RDP_EVENT_DISCONNECT;
	remmina_rdp_event_event_push(gp, &rdp_event);

	return FALSE;
}

static gboolean remmina_rdp_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	return TRUE;
}

static void remmina_rdp_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	rfContext *rfi = GET_PLUGIN_DATA(gp);

	switch (feature->id) {
	case REMMINA_RDP_FEATURE_UNFOCUS:
		remmina_rdp_event_unfocus(gp);
		break;

	case REMMINA_RDP_FEATURE_SCALE:
		if (rfi) {
			rfi->scale = remmina_plugin_service->remmina_protocol_widget_get_current_scale_mode(gp);
			remmina_rdp_event_update_scale(gp);
		} else {
			REMMINA_PLUGIN_DEBUG("Remmina RDP plugin warning: Null value for rfi by REMMINA_RDP_FEATURE_SCALE");
		}
		break;

	case REMMINA_RDP_FEATURE_MULTIMON:
		if (rfi) {
			RemminaFile *remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);
			if (remmina_plugin_service->file_get_int(remminafile, "multimon", FALSE)) {
				freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_UseMultimon, TRUE);
				/* TODO Add an option for this */
				freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_ForceMultimon, TRUE);
				freerdp_settings_set_bool(rfi->clientContext.context.settings, FreeRDP_Fullscreen, TRUE);
				remmina_rdp_event_send_delayed_monitor_layout(gp);
			}
		} else {
			REMMINA_PLUGIN_DEBUG("Remmina RDP plugin warning: Null value for rfi by REMMINA_RDP_FEATURE_MULTIMON");
		}
		break;

	case REMMINA_RDP_FEATURE_DYNRESUPDATE:
		break;

	case REMMINA_RDP_FEATURE_TOOL_REFRESH:
		if (rfi)
			gtk_widget_queue_draw_area(rfi->drawing_area, 0, 0,
						   remmina_plugin_service->protocol_plugin_get_width(gp),
						   remmina_plugin_service->protocol_plugin_get_height(gp));
		else
			REMMINA_PLUGIN_DEBUG("Remmina RDP plugin warning: Null value for rfi by REMMINA_RDP_FEATURE_TOOL_REFRESH");
		break;

	case REMMINA_RDP_FEATURE_TOOL_SENDCTRLALTDEL:
		remmina_rdp_send_ctrlaltdel(gp);
		break;

	default:
		break;
	}
}

/* Send a keystroke to the plugin window */
static void remmina_rdp_keystroke(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL(__func__);
	rfContext *rfi = GET_PLUGIN_DATA(gp);

	remmina_plugin_service->protocol_plugin_send_keys_signals(rfi->drawing_area,
								  keystrokes, keylen, GDK_KEY_PRESS | GDK_KEY_RELEASE);
	return;
}

static gboolean remmina_rdp_get_screenshot(RemminaProtocolWidget *gp, RemminaPluginScreenshotData *rpsd)
{
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	rdpGdi *gdi;
	size_t szmem;

	UINT32 bytesPerPixel;
	UINT32 bitsPerPixel;

	if (!rfi)
		return FALSE;

	gdi = ((rdpContext *)rfi)->gdi;

#if FREERDP_VERSION_MAJOR >= 3
	bytesPerPixel = FreeRDPGetBytesPerPixel(gdi->hdc->format);
	bitsPerPixel = FreeRDPGetBitsPerPixel(gdi->hdc->format);
#else
	bytesPerPixel = GetBytesPerPixel(gdi->hdc->format);
	bitsPerPixel = GetBitsPerPixel(gdi->hdc->format);
#endif

	/** @todo we should lock FreeRDP subthread to update rfi->primary_buffer, rfi->gdi and w/h,
	 * from here to memcpy, but… how ? */

	szmem = gdi->width * gdi->height * bytesPerPixel;

	REMMINA_PLUGIN_DEBUG("allocating %zu bytes for a full screenshot", szmem);
	rpsd->buffer = malloc(szmem);
	if (!rpsd->buffer) {
		REMMINA_PLUGIN_DEBUG("could not set aside %zu bytes for a full screenshot", szmem);
		return FALSE;
	}
	rpsd->width = gdi->width;
	rpsd->height = gdi->height;
	rpsd->bitsPerPixel = bitsPerPixel;
	rpsd->bytesPerPixel = bytesPerPixel;

	memcpy(rpsd->buffer, gdi->primary_buffer, szmem);

	/* Returning TRUE instruct also the caller to deallocate rpsd->buffer */
	return TRUE;
}

/* Array of key/value pairs for colour depths */
static gpointer colordepth_list[] =
{
	/* 1st one is the default in a new install */
	"99", N_("Automatic (32 bpp) (Server chooses its best format)"),
	"66", N_("GFX AVC444 (32 bpp)"),
	"65", N_("GFX AVC420 (32 bpp)"),
	"64", N_("GFX RFX (32 bpp)"),
	"63", N_("GFX RFX Progressive (32 bpp)"),
	"0",  N_("RemoteFX (32 bpp)"),
	"32", N_("True colour (32 bpp)"),
	"24", N_("True colour (24 bpp)"),
	"16", N_("High colour (16 bpp)"),
	"15", N_("High colour (15 bpp)"),
	"8",  N_("256 colours (8 bpp)"),
	NULL
};

/* Array of key/value pairs for the FreeRDP logging level */
static gpointer log_level[] =
{
	"INFO",	 "INFO",
	"FATAL", "FATAL",
	"ERROR", "ERROR",
	"WARN",	 "WARN",
	"DEBUG", "DEBUG",
	"TRACE", "TRACE",
	"OFF",	 "OFF",
	NULL
};


/* Array of key/value pairs for quality selection */
static gpointer quality_list[] =
{
	"0", N_("Poor (fastest)"),
	"1", N_("Medium"),
	"2", N_("Good"),
	"9", N_("Best (slowest)"),
	NULL
};

/* Array of key/value pairs for quality selection */
static gpointer network_list[] =
{
	"none",		  N_("None"),
	"autodetect",	  N_("Auto-detect"),
	"modem",	  N_("Modem"),
	"broadband-low",  N_("Low performance broadband"),
	"satellite",	  N_("Satellite"),
	"broadband-high", N_("High performance broadband"),
	"wan",		  N_("WAN"),
	"lan",		  N_("LAN"),
	NULL
};

/* Array of key/value pairs for sound options */
static gpointer sound_list[] =
{
	"off",	  N_("Off"),
	"local",  N_("Local"),
	"remote", N_("Remote"),
	NULL
};

/* Array of key/value pairs for security */
static gpointer security_list[] =
{
	"",    N_("Automatic negotiation"),
	"nla", N_("NLA protocol security"),
	"tls", N_("TLS protocol security"),
	"rdp", N_("RDP protocol security"),
	"ext", N_("NLA extended protocol security"),
	NULL
};

/* Array of key/value pairs for mouse movement */
static gpointer mouse_jitter_list[] =
{
	"No",	  N_("No"),
	"60",  N_("Every 1 min"),
	"180", N_("Every 3 min"),
	"300", N_("Every 5 min"),
	"600", N_("Every 10 min"),
	NULL
};

static gpointer idle_keypress_time_list[] =
{
	"No",	  N_("No"),
	"30",  N_("Every 30 sec"),
	"60",  N_("Every 1 min"),
	"180", N_("Every 3 min"),
	"300", N_("Every 5 min"),
	"600", N_("Every 10 min"),
	NULL
};

static gpointer idle_keypress_combo_list[] =
{
	"1",  N_("ALT_L + Tab"),
	"2",  N_("Win + Tab"),
	NULL
};

static gpointer gwtransp_list[] =
{
	"http", "HTTP",
	"rpc",	"RPC",
	"auto", "Auto",
	NULL
};

static gpointer tls_seclevel[] =
{
	"",  N_("Default"),
	"0", N_("0 — Windows 7 compatible"),
	"1", N_("1"),
	"2", N_("2"),
	"3", N_("3"),
	"4", N_("4"),
	"5", N_("5"),
	NULL
};

static gchar clientbuild_list[] =
	N_("2600 (Windows XP), 7601 (Windows Vista/7), 9600 (Windows 8 and newer)");

static gchar clientbuild_tooltip[] =
	N_("Used i.a. by terminal services in a smart card channel to distinguish client capabilities:\n"
	   "  • < 4034: Windows XP base smart card functions\n"
	   "  • 4034-7064: Windows Vista/7: SCardReadCache(),\n"
	   "    SCardWriteCache(), SCardGetTransmitCount()\n"
	   "  • >= 7065: Windows 8 and newer: SCardGetReaderIcon(),\n"
	   "    SCardGetDeviceTypeId()");

static gchar microphone_tooltip[] =
	N_("Options for redirection of audio input:\n"
	   "  • [sys:<sys>,][dev:<dev>,][format:<format>,][rate:<rate>,]\n"
	   "    [channel:<channel>] Audio input (microphone)\n"
	   "  • sys:pulse\n"
	   "  • format:1\n"
	   "  • sys:oss,dev:1,format:1\n"
	   "  • sys:alsa");

static gchar audio_tooltip[] =
	N_("Options for redirection of audio output:\n"
	   "  • [sys:<sys>,][dev:<dev>,][format:<format>,][rate:<rate>,]\n"
	   "    [channel:<channel>] Audio output\n"
	   "  • sys:pulse\n"
	   "  • format:1\n"
	   "  • sys:oss,dev:1,format:1\n"
	   "  • sys:alsa");


static gchar usb_tooltip[] =
	N_("Options for redirection of USB device:\n"
	   "  • [dbg,][id:<vid>:<pid>#…,][addr:<bus>:<addr>#…,][auto]\n"
	   "  • auto\n"
	   "  • id:054c:0268#4669:6e6b,addr:04:0c");

static gchar timeout_tooltip[] =
	N_("Advanced setting for high latency links:\n"
	   "Adjusts the connection timeout. Use if your connection times out.\n"
	   "The highest possible value is 600000 ms (10 minutes).\n");

static gchar network_tooltip[] =
	N_("Performance optimisations based on the network connection type:\n"
	   "Using auto-detection is advised.\n"
	   "If “Auto-detect” fails, choose the most appropriate option in the list.\n");

static gchar monitorids_tooltip[] =
	N_("Comma-separated list of monitor IDs and desktop orientations:\n"
	   "  • [<id>:<orientation-in-degrees>,]\n"
	   "  • 0,1,2,3\n"
	   "  • 0:270,1:90\n"
	   "Orientations are specified in degrees, valid values are:\n"
	   "  •   0 (landscape)\n"
	   "  •  90 (portrait)\n"
	   "  • 180 (landscape flipped)\n"
	   "  • 270 (portrait flipped)\n"
	   "\n");

static gchar drive_tooltip[] =
	N_("Redirect directory <path> as named share <name>.\n"
	   "  • <name>,<fullpath>[;<name>,<fullpath>[;…]]\n"
	   "  • MyHome,/home/remminer\n"
	   "  • /home/remminer\n"
	   "  • MyHome,/home/remminer;SomePath,/path/to/somepath\n"
	   "Hotplug support is enabled with:\n"
	   "  • hotplug,*\n"
	   "\n");

/* Array of RemminaProtocolSetting for basic settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting tooltip
 * g) Validation data pointer, will be passed to the validation callback method.
 * h) Validation callback method (Can be NULL. Every entry will be valid then.)
 *		use following prototype:
 *		gboolean mysetting_validator_method(gpointer key, gpointer value,
 *						    gpointer validator_data);
 *		gpointer key is a gchar* containing the setting's name,
 *		gpointer value contains the value which should be validated,
 *		gpointer validator_data contains your passed data.
 */
static const RemminaProtocolSetting remmina_rdp_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER,	    "server",			NULL,					  FALSE, NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	    "username",			N_("Username"),				  FALSE, NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD,   "password",			N_("Password"),				  FALSE, NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	    "domain",			N_("Domain"),				  FALSE, NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	    "drive",			N_("Share folder"),			  FALSE, NULL,		  drive_tooltip,								NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	    "restricted-admin",		N_("Restricted admin mode"),		  FALSE, NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	    "pth",			N_("Password hash"),			  FALSE, NULL,		  N_("Restricted admin mode password hash"),					NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	    "left-handed",		N_("Left-handed mouse support"),	  TRUE,	 NULL,		  N_("Swap left and right mouse buttons for left-handed mouse support"),	NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	    "disable-smooth-scrolling", N_("Disable smooth scrolling"),		  TRUE,	 NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	    "multimon",			N_("Enable multi monitor"),		  TRUE,	 NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	    "span",			N_("Span screen over multiple monitors"), TRUE,	 NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	    "monitorids",		N_("List monitor IDs"),			  FALSE, NULL,		  monitorids_tooltip,								NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_RESOLUTION, "resolution",		NULL,					  FALSE, NULL,		  NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	    "colordepth",		N_("Colour depth"),			  FALSE, colordepth_list, NULL,										NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	    "network",			N_("Network connection type"),		  FALSE, network_list,	  network_tooltip,								NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_KEYMAP,	    "keymap",	                NULL,		                          FALSE, NULL,	          NULL,	                                                                        NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	    NULL,			NULL,					  FALSE, NULL,		  NULL,										NULL, NULL }
};

/* Array of RemminaProtocolSetting for advanced settings.
 * Each item is composed by:
 * a) RemminaProtocolSettingType for setting type
 * b) Setting name
 * c) Setting description
 * d) Compact disposition
 * e) Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO
 * f) Setting Tooltip
 */
static const RemminaProtocolSetting remmina_rdp_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "quality",		    N_("Quality"),					 FALSE, quality_list,	  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "security",		    N_("Security protocol negotiation"),		 FALSE, security_list,	  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "gwtransp",		    N_("Gateway transport type"),			 FALSE, gwtransp_list,	  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "tls-seclevel",	    N_("TLS Security Level"),			 	 FALSE, tls_seclevel,	  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "freerdp_log_level",	    N_("FreeRDP log level"),				 FALSE, log_level,	  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "freerdp_log_filters",    N_("FreeRDP log filters"),				 FALSE, NULL,		  N_("tag:level[,tag:level[,…]]")										 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "sound",		    N_("Audio output mode"),				 FALSE, sound_list,	  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "audio-output",	    N_("Redirect local audio output"),			 TRUE,	NULL,		  audio_tooltip													 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "microphone",		    N_("Redirect local microphone"),			 TRUE,	NULL,		  microphone_tooltip												 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "timeout",		    N_("Connection timeout in ms"),			 TRUE,	NULL,		  timeout_tooltip												 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "gateway_server",	    N_("Remote Desktop Gateway server"),		 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "gateway_username",	    N_("Remote Desktop Gateway username"),		 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "gateway_password",	    N_("Remote Desktop Gateway password"),		 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "gateway_domain",	    N_("Remote Desktop Gateway domain"),		 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "clientname",		    N_("Client name"),					 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_COMBO,	  "clientbuild",	    N_("Client build"),					 FALSE, clientbuild_list, clientbuild_tooltip												 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "exec",		    N_("Start-up program"),				 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "execpath",		    N_("Start-up path"),				 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "loadbalanceinfo",	    N_("Load balance info"),				 FALSE, NULL,		  NULL														 },
	// TRANSLATORS: Do not use typographic quotation marks, these must stay as "double quote", also know as “Typewriter ("programmer's") quote, ambidextrous.”
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "printer_overrides",	    N_("Override printer drivers"),			 FALSE, NULL,		  N_("\"Samsung_CLX-3300_Series\":\"Samsung CLX-3300 Series PS\";\"Canon MF410\":\"Canon MF410 Series UFR II\"") },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "usb",		    N_("USB device redirection"),			 TRUE,	NULL,		  usb_tooltip													 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "serialname",		    N_("Local serial name"),				 FALSE, NULL,		  N_("COM1, COM2, etc.")											 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "serialdriver",	    N_("Local serial driver"),				 FALSE, NULL,		  N_("Serial")													 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "serialpath",		    N_("Local serial path"),				 FALSE, NULL,		  N_("/dev/ttyS0, /dev/ttyS1, etc.")										 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "parallelname",	    N_("Local parallel name"),				 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "parallelpath",	    N_("Local parallel device"),			 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "smartcardname",	    N_("Name of smart card"),				 FALSE, NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "dvc",		    N_("Dynamic virtual channel"),			 FALSE, NULL,		  N_("<channel>[,<options>]")											 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "vc",			    N_("Static virtual channel"),			 FALSE, NULL,		  N_("<channel>[,<options>]")											 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "rdp2tcp",		    N_("TCP redirection"),				 FALSE, NULL,		  N_("/PATH/TO/rdp2tcp")											 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "rdp_reconnect_attempts", N_("Reconnect attempts number"),			 FALSE, NULL,		  N_("The maximum number of reconnect attempts upon an RDP disconnect (default: 20)")				 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "rdp_mouse_jitter",    N_("Move mouse when connection is idle"),		 FALSE, mouse_jitter_list,	  NULL											},
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "rdp_idle_keypress_time",    N_("Press keys when connection is idle"),		 FALSE, idle_keypress_time_list,	  NULL							},
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "rdp_idle_keypress_combo",    N_("Keys combination"),		 FALSE, idle_keypress_combo_list,	  NULL							},

	{ REMMINA_PROTOCOL_SETTING_TYPE_ASSISTANCE,	  "assistance_mode",	    N_("Attempt to connect in assistance mode"),	TRUE,	NULL																 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "preferipv6",		    N_("Prefer IPv6 AAAA record over IPv4 A record"),	 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "shareprinter",	    N_("Share printers"),				 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "shareserial",	    N_("Share serial ports"),				 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "serialpermissive",	    N_("(SELinux) permissive mode for serial ports"),	 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "shareparallel",	    N_("Share parallel ports"),				 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "sharesmartcard",	    N_("Share a smart card"),				 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "disableclipboard",	    N_("Turn off clipboard sync"),			 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "cert_ignore",	    N_("Ignore certificate"),				 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "old-license",	    N_("Use the old license workflow"),			 TRUE,	NULL,		  N_("It disables CAL and hwId is set to 0")									 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "disablepasswordstoring", N_("Forget passwords after use"),			 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "console",		    N_("Attach to console (2003/2003 R2)"),		 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "disable_fastpath",	    N_("Turn off fast-path"),				 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "gateway_usage",	    N_("Server detection using Remote Desktop Gateway"), TRUE,	NULL,		  NULL														 },
#if defined(PROXY_TYPE_IGNORE)
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "useproxyenv",	    N_("Use system proxy settings"),			 TRUE,	NULL,		  NULL														 },
#endif
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "disableautoreconnect",   N_("Turn off automatic reconnection"),		 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "relax-order-checks",	    N_("Relax order checks"),				 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "glyph-cache",	    N_("Glyph cache"),					 TRUE,	NULL,		  NULL														 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "multitransport",	    N_("Enable multitransport protocol (UDP)"),		 TRUE,	NULL,		  N_("Using the UDP protocol may improve performance")								 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "base-cred-for-gw",	    N_("Use base credentials for gateway too"),		 TRUE,	NULL,		  NULL														 },
#if FREERDP_CHECK_VERSION(2, 3, 1)
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "websockets",		    N_("Enable Gateway websockets support"),		 TRUE,	NULL,		  NULL														 },
#endif
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	  "no-suppress",	    N_("Update framebuffer even when not visible"),	TRUE,	NULL																 },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,			    NULL,						 FALSE, NULL,		  NULL														 }
};

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static const RemminaProtocolFeature remmina_rdp_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_PREF,	 	  REMMINA_RDP_FEATURE_VIEWONLY,	     GINT_TO_POINTER(REMMINA_PROTOCOL_FEATURE_PREF_CHECK), "viewonly",
	  N_("View only") },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL,	      REMMINA_RDP_FEATURE_TOOL_REFRESH,	       N_("Refresh"),		   NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_SCALE,	      REMMINA_RDP_FEATURE_SCALE,	       NULL,			   NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_DYNRESUPDATE, REMMINA_RDP_FEATURE_DYNRESUPDATE,	       NULL,			   NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_MULTIMON,     REMMINA_RDP_FEATURE_MULTIMON,	       NULL,			   NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL,	      REMMINA_RDP_FEATURE_TOOL_SENDCTRLALTDEL, N_("Send Ctrl+Alt+Delete"), NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS,      REMMINA_RDP_FEATURE_UNFOCUS,	       NULL,			   NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END,	      0,				       NULL,			   NULL, NULL }
};

/* This will be filled with version info string */
static char remmina_plugin_rdp_version[256];

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_rdp =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   // Type
	"RDP",                                          // Name
	N_("RDP - Remote Desktop Protocol"),            // Description
	GETTEXT_PACKAGE,                                // Translation domain
	remmina_plugin_rdp_version,                     // Version number
	"org.remmina.Remmina-rdp-symbolic",             // Icon for normal connection
	"org.remmina.Remmina-rdp-ssh-symbolic",         // Icon for SSH connection
	remmina_rdp_basic_settings,                     // Array for basic settings
	remmina_rdp_advanced_settings,                  // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,            // SSH settings type
	remmina_rdp_features,                           // Array for available features
	remmina_rdp_init,                               // Plugin initialization
	remmina_rdp_open_connection,                    // Plugin open connection
	remmina_rdp_close_connection,                   // Plugin close connection
	remmina_rdp_query_feature,                      // Query for available features
	remmina_rdp_call_feature,                       // Call a feature
	remmina_rdp_keystroke,                          // Send a keystroke
	remmina_rdp_get_screenshot,                     // Screenshot
	remmina_rdp_event_on_map,                       // RCW map event
	remmina_rdp_event_on_unmap                      // RCW unmap event
};

/* File plugin definition and features */
static RemminaFilePlugin remmina_rdpf =
{
	REMMINA_PLUGIN_TYPE_FILE,                       // Type
	"RDPF",                                         // Name
	N_("RDP - RDP File Handler"),                   // Description
	GETTEXT_PACKAGE,                                // Translation domain
	remmina_plugin_rdp_version,                     // Version number
	remmina_rdp_file_import_test,                   // Test import function
	remmina_rdp_file_import,                        // Import function
	remmina_rdp_file_export_test,                   // Test export function
	remmina_rdp_file_export,                        // Export function
	NULL
};

/* Preferences plugin definition and features */
static RemminaPrefPlugin remmina_rdps =
{
	REMMINA_PLUGIN_TYPE_PREF,                       // Type
	"RDPS",                                         // Name
	N_("RDP - Preferences"),                        // Description
	GETTEXT_PACKAGE,                                // Translation domain
	remmina_plugin_rdp_version,                     // Version number
	"RDP",                                          // Label
	remmina_rdp_settings_new                        // Preferences body function
};

static char *buildconfig_strstr(const char *bc, const char *option)
{
	TRACE_CALL(__func__);

	char *p, *n;

	p = strcasestr(bc, option);
	if (p == NULL)
		return NULL;

	if (p > bc && *(p - 1) > ' ')
		return NULL;

	n = p + strlen(option);
	if (*n > ' ')
		return NULL;

	return p;
}

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService *service)
{
	int vermaj, vermin, verrev;

	TRACE_CALL(__func__);
	remmina_plugin_service = service;

	/* Check that we are linked to the correct version of libfreerdp */

	freerdp_get_version(&vermaj, &vermin, &verrev);
	if ((vermaj < FREERDP_REQUIRED_MAJOR) ||
		((vermaj == FREERDP_REQUIRED_MAJOR) && ((vermin < FREERDP_REQUIRED_MINOR) ||
						  ((vermin == FREERDP_REQUIRED_MINOR) && (verrev < FREERDP_REQUIRED_REVISION))))) {
		g_printf("Upgrade your FreeRDP library version from %d.%d.%d to at least libfreerdp %d.%d.%d "
			 "to run the RDP plugin.\n",
			 vermaj, vermin, verrev,
			 FREERDP_REQUIRED_MAJOR, FREERDP_REQUIRED_MINOR, FREERDP_REQUIRED_REVISION);
		return FALSE;
	}

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_RUNTIME_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (!service->register_plugin((RemminaPlugin *)&remmina_rdp))
		return FALSE;

	remmina_rdpf.export_hints = _("Export connection in Windows .rdp file format");

	if (!service->register_plugin((RemminaPlugin *)&remmina_rdpf))
		return FALSE;

	if (!service->register_plugin((RemminaPlugin *)&remmina_rdps))
		return FALSE;

	if (buildconfig_strstr(freerdp_get_build_config(), "WITH_GFX_H264=ON")) {
		gfx_h264_available = TRUE;
		REMMINA_PLUGIN_DEBUG("gfx_h264_available: %d", gfx_h264_available);
	} else {
		gfx_h264_available = FALSE;
		REMMINA_PLUGIN_DEBUG("gfx_h264_available: %d", gfx_h264_available);
		/* Remove values 65 and 66 from colordepth_list array by shifting it */
		gpointer *src, *dst;
		dst = src = colordepth_list;
		while (*src) {
			if (strcmp(*src, "66") != 0 && strcmp(*src, "65") != 0) {
				if (dst != src) {
					*dst = *src;
					*(dst + 1) = *(src + 1);
				}
				dst += 2;
			}
			src += 2;
		}
		*dst = NULL;
	}

	snprintf(remmina_plugin_rdp_version, sizeof(remmina_plugin_rdp_version),
		 "RDP plugin: %s (Git %s), Compiled with libfreerdp %s (%s), Running with libfreerdp %s (rev %s), H.264 %s",
		 VERSION, REMMINA_GIT_REVISION,
#if FREERDP_VERSION_MAJOR >= 3
		 FREERDP_VERSION_FULL, FREERDP_GIT_REVISION,
#else
		 FREERDP_VERSION_FULL, GIT_REVISION,
#endif
		 freerdp_get_version_string(),
		 freerdp_get_build_revision(),
		 gfx_h264_available ? "Yes" : "No"
		 );

	remmina_rdp_settings_init();

	return TRUE;
}
