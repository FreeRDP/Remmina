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

#include "rdp_plugin.h"
#include "rdp_gdi.h"
#include "rdp_event.h"
#include "rdp_graphics.h"
#include "rdp_file.h"
#include "rdp_settings.h"
#include "rdp_cliprdr.h"

#include <errno.h>
#include <pthread.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/freerdp.h>
#include <freerdp/constants.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/channels.h>
#include <freerdp/client/cmdline.h>
#include <freerdp/error.h>
#include <freerdp/utils/signal.h>
#include <winpr/memory.h>

#define REMMINA_RDP_FEATURE_TOOL_REFRESH		1
#define REMMINA_RDP_FEATURE_SCALE			2
#define REMMINA_RDP_FEATURE_UNFOCUS			3

RemminaPluginService* remmina_plugin_service = NULL;
static char remmina_rdp_plugin_default_drive_name[]="RemminaDisk";

void rf_get_fds(RemminaProtocolWidget* gp, void** rfds, int* rcount)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	if (rfi->event_pipe[0] != -1)
	{
		rfds[*rcount] = GINT_TO_POINTER(rfi->event_pipe[0]);
		(*rcount)++;
	}
}

BOOL rf_check_fds(RemminaProtocolWidget* gp)
{
	UINT16 flags;
	gchar buf[100];
	rdpInput* input;
	rfContext* rfi;
	RemminaPluginRdpEvent* event;

	rfi = GET_DATA(gp);

	if (rfi->event_queue == NULL)
		return True;

	input = rfi->instance->input;

	while ((event =(RemminaPluginRdpEvent*) g_async_queue_try_pop(rfi->event_queue)) != NULL)
	{
		switch (event->type)
		{
			case REMMINA_RDP_EVENT_TYPE_SCANCODE:
				flags = event->key_event.extended ? KBD_FLAGS_EXTENDED : 0;
				flags |= event->key_event.up ? KBD_FLAGS_RELEASE : KBD_FLAGS_DOWN;
				input->KeyboardEvent(input, flags, event->key_event.key_code);
				break;

			case REMMINA_RDP_EVENT_TYPE_MOUSE:
				input->MouseEvent(input, event->mouse_event.flags,
						event->mouse_event.x, event->mouse_event.y);
				break;
		}

		g_free(event);
	}

	if (read(rfi->event_pipe[0], buf, sizeof (buf)))
	{
	}

	return True;
}

void rf_queue_ui(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);
	g_async_queue_push(rfi->ui_queue, ui);

	LOCK_BUFFER(TRUE)

	if (!rfi->ui_handler)
		rfi->ui_handler = IDLE_ADD((GSourceFunc) remmina_rdp_event_queue_ui, gp);

	UNLOCK_BUFFER(TRUE)
}

void rf_object_free(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* obj)
{
	rfContext* rfi;

	rfi = GET_DATA(gp);

	switch (obj->type)
	{
		case REMMINA_RDP_UI_RFX:
			rfx_message_free(rfi->rfx_context, obj->rfx.message);
			break;

		case REMMINA_RDP_UI_NOCODEC:
			free(obj->nocodec.bitmap);
			break;

		default:
			break;
	}

	g_free(obj);
}

void rf_begin_paint(rdpContext* context)
{
	rdpGdi* gdi = context->gdi;
	gdi->primary->hdc->hwnd->invalid->null = 1;
	gdi->primary->hdc->hwnd->ninvalid = 0;
}

void rf_end_paint(rdpContext* context)
{
	INT32 x, y;
	UINT32 w, h;
	rdpGdi* gdi;
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	RemminaPluginRdpUiObject* ui;

	gdi = context->gdi;
	rfi = (rfContext*) context;
	gp = rfi->protocol_widget;

	if (gdi->primary->hdc->hwnd->invalid->null)
		return;

	x = gdi->primary->hdc->hwnd->invalid->x;
	y = gdi->primary->hdc->hwnd->invalid->y;
	w = gdi->primary->hdc->hwnd->invalid->w;
	h = gdi->primary->hdc->hwnd->invalid->h;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_UPDATE_REGION;
	ui->region.x = x;
	ui->region.y = y;
	ui->region.width = w;
	ui->region.height = h;

	rf_queue_ui(rfi->protocol_widget, ui);
}

static void rf_desktop_resize(rdpContext* context)
{
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) context;
	gp = rfi->protocol_widget;

	LOCK_BUFFER(TRUE)

	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->DesktopWidth);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->DesktopHeight);

	UNLOCK_BUFFER(TRUE)

	THREADS_ENTER
	remmina_rdp_event_update_scale(gp);
	THREADS_LEAVE

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "desktop-resize");
}

static BOOL remmina_rdp_pre_connect(freerdp* instance)
{
	rfContext* rfi;
	ALIGN64 rdpSettings* settings;
	RemminaProtocolWidget* gp;
	rdpChannels *channels;

	rfi = (rfContext*) instance->context;
	settings = instance->settings;
	gp = rfi->protocol_widget;
	channels = instance->context->channels;

	settings->BitmapCacheEnabled = True;
	settings->OffscreenSupportLevel = True;

	settings->OrderSupport[NEG_DSTBLT_INDEX] = True;
	settings->OrderSupport[NEG_PATBLT_INDEX] = True;
	settings->OrderSupport[NEG_SCRBLT_INDEX] = True;
	settings->OrderSupport[NEG_OPAQUE_RECT_INDEX] = True;
	settings->OrderSupport[NEG_DRAWNINEGRID_INDEX] = False;
	settings->OrderSupport[NEG_MULTIDSTBLT_INDEX] = False;
	settings->OrderSupport[NEG_MULTIPATBLT_INDEX] = False;
	settings->OrderSupport[NEG_MULTISCRBLT_INDEX] = False;
	settings->OrderSupport[NEG_MULTIOPAQUERECT_INDEX] = True;
	settings->OrderSupport[NEG_MULTI_DRAWNINEGRID_INDEX] = False;
	settings->OrderSupport[NEG_LINETO_INDEX] = True;
	settings->OrderSupport[NEG_POLYLINE_INDEX] = True;
	settings->OrderSupport[NEG_MEMBLT_INDEX] = settings->BitmapCacheEnabled;
	settings->OrderSupport[NEG_MEM3BLT_INDEX] = True;
	settings->OrderSupport[NEG_MEMBLT_V2_INDEX] = settings->BitmapCacheEnabled;
	settings->OrderSupport[NEG_MEM3BLT_V2_INDEX] = False;
	settings->OrderSupport[NEG_SAVEBITMAP_INDEX] = False;
	settings->OrderSupport[NEG_GLYPH_INDEX_INDEX] = True;
	settings->OrderSupport[NEG_FAST_INDEX_INDEX] = True;
	settings->OrderSupport[NEG_FAST_GLYPH_INDEX] = True;
	settings->OrderSupport[NEG_POLYGON_SC_INDEX] = False;
	settings->OrderSupport[NEG_POLYGON_CB_INDEX] = False;
	settings->OrderSupport[NEG_ELLIPSE_SC_INDEX] = False;
	settings->OrderSupport[NEG_ELLIPSE_CB_INDEX] = False;

	if (settings->RemoteFxCodec == True)
	{
		settings->FrameAcknowledge = False;
		settings->LargePointerFlag = True;
		settings->PerformanceFlags = PERF_FLAG_NONE;

		rfi->rfx_context = rfx_context_new();
	}

	freerdp_client_load_addins(instance->context->channels, instance->settings);
	freerdp_channels_pre_connect(instance->context->channels, instance);

	rfi->clrconv = freerdp_clrconv_new(CLRCONV_ALPHA);

	instance->context->cache = cache_new(instance->settings);

	return True;
}


static BOOL remmina_rdp_post_connect(freerdp* instance)
{
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	RemminaPluginRdpUiObject* ui;
	rdpGdi* gdi;
	UINT32 flags;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	rfi->width = rfi->settings->DesktopWidth;
	rfi->height = rfi->settings->DesktopHeight;
	rfi->srcBpp = rfi->settings->ColorDepth;

	if (rfi->settings->RemoteFxCodec == FALSE)
		rfi->sw_gdi = TRUE;

	rf_register_graphics(instance->context->graphics);

	flags = CLRCONV_ALPHA;

	if (rfi->bpp == 32)
	{
		flags |= CLRBUF_32BPP;
		rfi->cairo_format = CAIRO_FORMAT_ARGB32;
	}
	else
	{
		flags |= CLRBUF_16BPP;
		rfi->cairo_format = CAIRO_FORMAT_RGB16_565;
	}

	gdi_init(instance, flags, NULL);
	gdi = instance->context->gdi;
	rfi->primary_buffer = gdi->primary_buffer;

	rfi->hdc = gdi_GetDC();
	rfi->hdc->bitsPerPixel = rfi->bpp;
	rfi->hdc->bytesPerPixel = rfi->bpp / 8;

	rfi->hdc->hwnd = (HGDI_WND) malloc(sizeof(GDI_WND));
	rfi->hdc->hwnd->invalid = gdi_CreateRectRgn(0, 0, 0, 0);
	rfi->hdc->hwnd->invalid->null = 1;

	rfi->hdc->hwnd->count = 32;
	rfi->hdc->hwnd->cinvalid = (HGDI_RGN) malloc(sizeof(GDI_RGN) * rfi->hdc->hwnd->count);
	rfi->hdc->hwnd->ninvalid = 0;

	pointer_cache_register_callbacks(instance->update);

/*
	if (rfi->sw_gdi != TRUE)
	{
		glyph_cache_register_callbacks(instance->update);
		brush_cache_register_callbacks(instance->update);
		bitmap_cache_register_callbacks(instance->update);
		offscreen_cache_register_callbacks(instance->update);
		palette_cache_register_callbacks(instance->update);
	}
*/

	instance->update->BeginPaint = rf_begin_paint;
	instance->update->EndPaint = rf_end_paint;
	instance->update->DesktopResize = rf_desktop_resize;

	freerdp_channels_post_connect(instance->context->channels, instance);
	rfi->connected = True;

	remmina_plugin_service->protocol_plugin_emit_signal(gp, "connect");

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CONNECTED;
	rf_queue_ui(gp, ui);

	return True;
}

static BOOL remmina_rdp_authenticate(freerdp* instance, char** username, char** password, char** domain)
{
	gchar *s_username, *s_password, *s_domain;
	gint ret;
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	gboolean save;
	RemminaFile* remminafile;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	THREADS_ENTER
	ret = remmina_plugin_service->protocol_plugin_init_authuserpwd(gp, TRUE);
	THREADS_LEAVE

	if (ret == GTK_RESPONSE_OK)
	{
		s_username = remmina_plugin_service->protocol_plugin_init_get_username(gp);
		if (s_username) rfi->settings->Username = strdup(s_username);

		s_password = remmina_plugin_service->protocol_plugin_init_get_password(gp);
		if (s_password) rfi->settings->Password = strdup(s_password);

		s_domain = remmina_plugin_service->protocol_plugin_init_get_domain(gp);
		if (s_domain) rfi->settings->Domain = strdup(s_domain);

		save = remmina_plugin_service->protocol_plugin_init_get_savepassword(gp);
		if (save)
		{
			// User has requested to save password. We put all the new cretentials
			// into remminafile->settings. They will be saved later, when disconnecting, by
			// remmina_connection_object_on_disconnect of remmina_connection_window.c
			// (this operation should be called "save credentials" and not "save password")

			remmina_plugin_service->file_set_string( remminafile, "username", s_username );
			remmina_plugin_service->file_set_string( remminafile, "password", s_password );
			remmina_plugin_service->file_set_string( remminafile, "domain", s_domain );

		}

		if ( s_username ) g_free( s_username );
		if ( s_password ) g_free( s_password );
		if ( s_domain ) g_free( s_domain );

		return True;
	}
	else
	{
		rfi->user_cancelled = TRUE;
		return False;
	}

	return True;
}

static BOOL remmina_rdp_verify_certificate(freerdp* instance, char* subject, char* issuer, char* fingerprint)
{
	gint status;
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	THREADS_ENTER
	status = remmina_plugin_service->protocol_plugin_init_certificate(gp, subject, issuer, fingerprint);
	THREADS_LEAVE

	if (status == GTK_RESPONSE_OK)
		return True;

	return False;
}
static BOOL remmina_rdp_verify_changed_certificate(freerdp* instance, char* subject, char* issuer, char* new_fingerprint, char* old_fingerprint)
{
	gint status;
	rfContext* rfi;
	RemminaProtocolWidget* gp;

	rfi = (rfContext*) instance->context;
	gp = rfi->protocol_widget;

	THREADS_ENTER
	status = remmina_plugin_service->protocol_plugin_changed_certificate(gp, subject, issuer, new_fingerprint, old_fingerprint);
	THREADS_LEAVE

	if (status == GTK_RESPONSE_OK)
		return True;

	return False;
}

static int remmina_rdp_receive_channel_data(freerdp* instance, int channelId, UINT8* data, int size, int flags, int total_size)
{
	return freerdp_channels_data(instance, channelId, data, size, flags, total_size);
}

void remmina_rdp_channels_process_event(RemminaProtocolWidget* gp, wMessage* event)
{
	switch (GetMessageClass(event->id))
	{
		case CliprdrChannel_Class:
			remmina_rdp_channel_cliprdr_process(gp, event);
			break;
	}
}

static void remmina_rdp_main_loop(RemminaProtocolWidget* gp)
{
	int i;
	int fds;
	int rcount;
	int wcount;
	int max_fds;
	void *rfds[32];
	void *wfds[32];
	fd_set rfds_set;
	fd_set wfds_set;
	rfContext* rfi;
	wMessage* event;

	rdpChannels *channels;


	memset(rfds, 0, sizeof(rfds));
	memset(wfds, 0, sizeof(wfds));

	rfi = GET_DATA(gp);
	channels = rfi->instance->context->channels;

	while (!freerdp_shall_disconnect(rfi->instance))
	{
		rcount = 0;
		wcount = 0;

		if (!freerdp_get_fds(rfi->instance, rfds, &rcount, wfds, &wcount))
		{
			break;
		}
		if (!freerdp_channels_get_fds(channels, rfi->instance, rfds, &rcount, wfds, &wcount))
		{
			break;
		}
		rf_get_fds(gp, rfds, &rcount);

		max_fds = 0;
		FD_ZERO(&rfds_set);
		for (i = 0; i < rcount; i++)
		{
			fds = (int) (UINT64) (rfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &rfds_set);
		}

		FD_ZERO(&wfds_set);
		for (i = 0; i < wcount; i++)
		{
			fds = GPOINTER_TO_INT(wfds[i]);

			if (fds > max_fds)
				max_fds = fds;

			FD_SET(fds, &wfds_set);
		}

		/* exit if nothing to do */
		if (max_fds == 0)
		{
			break;
		}

		/* do the wait */
		if (select(max_fds + 1, &rfds_set, &wfds_set, NULL, NULL) == -1)
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
		if (!freerdp_check_fds(rfi->instance))
		{
			break;
		}
		/* check channel fds */
		if (!freerdp_channels_check_fds(channels, rfi->instance))
		{
			break;
		}
		else
		{
			event = freerdp_channels_pop_event(channels);
			if (event)
				remmina_rdp_channels_process_event(gp, event);
		}
		/* check ui */
		if (!rf_check_fds(gp))
		{
			break;
		}
	}
}

int remmina_rdp_load_static_channel_addin(rdpChannels* channels, rdpSettings* settings, char* name, void* data)
{
	void* entry;

	entry = freerdp_load_channel_addin_entry(name, NULL, NULL, FREERDP_ADDIN_CHANNEL_STATIC);




	if (entry)
	{

		if (freerdp_channels_client_load(channels, settings, entry, data) == 0)
		{
			fprintf(stderr, "loading channel %s\n", name);
			return 0;
		}
	}

	return -1;
}


int remmina_rdp_add_static_channel(rdpSettings* settings, int count, char** params)
{
	int index;
	ADDIN_ARGV* args;

	args = (ADDIN_ARGV*) malloc(sizeof(ADDIN_ARGV));

	args->argc = count;
	args->argv = (char**) malloc(sizeof(char*) * args->argc);

	for (index = 0; index < args->argc; index++)
		args->argv[index] = _strdup(params[index]);

	freerdp_static_channel_collection_add(settings, args);

	return 0;
}


static gboolean remmina_rdp_main(RemminaProtocolWidget* gp)
{
	gchar* s;
	gint port;
	gchar* host;
	gchar* value;
	gint rdpsnd_rate;
	gint rdpsnd_channel;
	char *rdpsnd_params[3];
	int rdpsnd_nparams;
	char rdpsnd_param1[16];
	char rdpsnd_param2[16];
	const gchar* cs;
	RemminaFile* remminafile;
	rfContext* rfi;

	gchar *dest_server, *dest_host;
	gint dest_port;

	rfi = GET_DATA(gp);
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	s = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, 3389, FALSE);

	if (s == NULL)
		return FALSE;

	remmina_plugin_service->get_server_port(s, 3389, &host, &port);
	rfi->settings->ServerHostname = strdup(host);
	g_free(host);
	g_free(s);
	rfi->settings->ServerPort = port;

	// Setup rfi->settings->CertificateName when tunneled with SSH
	if (remmina_plugin_service->file_get_int(remminafile, "ssh_enabled", FALSE)) {
		dest_server = remmina_plugin_service->file_get_string(remminafile, "server");
		if ( dest_server ) {
			remmina_plugin_service->get_server_port(dest_server, 0, &dest_host, &dest_port);
			rfi->settings->CertificateName = strdup( dest_host );
			g_free(dest_host);
		}
	}

	rfi->settings->ColorDepth = remmina_plugin_service->file_get_int(remminafile, "colordepth", 0);

	if (rfi->settings->ColorDepth == 0)
	{
		rfi->settings->RemoteFxCodec = True;
		rfi->settings->ColorDepth = 32;
	}

	rfi->settings->DesktopWidth = remmina_plugin_service->file_get_int(remminafile, "resolution_width", 1024);
	rfi->settings->DesktopHeight = remmina_plugin_service->file_get_int(remminafile, "resolution_height", 768);
	remmina_plugin_service->protocol_plugin_set_width(gp, rfi->settings->DesktopWidth);
	remmina_plugin_service->protocol_plugin_set_height(gp, rfi->settings->DesktopHeight);

	if (remmina_plugin_service->file_get_string(remminafile, "username"))
		rfi->settings->Username = strdup(remmina_plugin_service->file_get_string(remminafile, "username"));

	if (remmina_plugin_service->file_get_string(remminafile, "domain"))
		rfi->settings->Domain = strdup(remmina_plugin_service->file_get_string(remminafile, "domain"));

	THREADS_ENTER
	s = remmina_plugin_service->file_get_secret(remminafile, "password");
	THREADS_LEAVE

	if (s)
	{
		rfi->settings->Password = strdup(s);
		rfi->settings->AutoLogonEnabled = 1;
		g_free(s);
	}

	if (remmina_plugin_service->file_get_string(remminafile, "clientname"))
	{
		s = remmina_plugin_service->file_get_string(remminafile, "clientname");
		if ( s ) {
			free( rfi->settings->ClientHostname );
			rfi->settings->ClientHostname = strdup(s);
			g_free(s);
		}
	}
	else
	{
		free( rfi->settings->ClientHostname );
		rfi->settings->ClientHostname = strdup( g_get_host_name() );
	}

	if (remmina_plugin_service->file_get_string(remminafile, "exec"))
	{
		rfi->settings->AlternateShell = strdup(remmina_plugin_service->file_get_string(remminafile, "exec"));
	}

	if (remmina_plugin_service->file_get_string(remminafile, "execpath"))
	{
		rfi->settings->ShellWorkingDirectory = strdup(remmina_plugin_service->file_get_string(remminafile, "execpath"));
	}

	s = g_strdup_printf("rdp_quality_%i", remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0));
	value = remmina_plugin_service->pref_get_value(s);
	g_free(s);

	if (value && value[0])
	{
		rfi->settings->PerformanceFlags = strtoul(value, NULL, 16);
	}
	else
	{
		switch (remmina_plugin_service->file_get_int(remminafile, "quality", DEFAULT_QUALITY_0))
		{
			case 9:
				rfi->settings->PerformanceFlags = DEFAULT_QUALITY_9;
				break;

			case 2:
				rfi->settings->PerformanceFlags = DEFAULT_QUALITY_2;
				break;

			case 1:
				rfi->settings->PerformanceFlags = DEFAULT_QUALITY_1;
				break;

			case 0:
			default:
				rfi->settings->PerformanceFlags = DEFAULT_QUALITY_0;
				break;
		}
	}
	g_free(value);

	rfi->settings->KeyboardLayout = remmina_rdp_settings_get_keyboard_layout();

	if (remmina_plugin_service->file_get_int(remminafile, "console", FALSE))
	{
		rfi->settings->ConsoleSession = True;
	}

	cs = remmina_plugin_service->file_get_string(remminafile, "security");

	if (g_strcmp0(cs, "rdp") == 0)
	{
		rfi->settings->RdpSecurity = True;
		rfi->settings->TlsSecurity = False;
		rfi->settings->NlaSecurity = False;
	}
	else if (g_strcmp0(cs, "tls") == 0)
	{
		rfi->settings->RdpSecurity = False;
		rfi->settings->TlsSecurity = True;
		rfi->settings->NlaSecurity = False;
	}
	else if (g_strcmp0(cs, "nla") == 0)
	{
		rfi->settings->RdpSecurity = False;
		rfi->settings->TlsSecurity = False;
		rfi->settings->NlaSecurity = True;
	}

	rfi->settings->CompressionEnabled = True;
	rfi->settings->FastPathInput = True;
	rfi->settings->FastPathOutput = True;

	cs = remmina_plugin_service->file_get_string(remminafile, "sound");

	if (g_strcmp0(cs, "remote") == 0)
	{
		rfi->settings->RemoteConsoleAudio = 1;
	}
	else if (g_str_has_prefix(cs, "local"))
	{

		rdpsnd_nparams = 0;
		rdpsnd_params[rdpsnd_nparams++] = "rdpsnd";

		cs = strchr(cs, ',');
		if (cs)
		{
			rdpsnd_rate = atoi(cs + 1);
			if (rdpsnd_rate > 1000 && rdpsnd_rate < 150000)
			{
				snprintf( rdpsnd_param1, sizeof(rdpsnd_param1), "rate:%d", rdpsnd_rate );
				rdpsnd_params[rdpsnd_nparams++] = rdpsnd_param1;
				cs = strchr(cs +1, ',');
				if (cs)
				{
					rdpsnd_channel = atoi(cs + 1);
					if (rdpsnd_channel >= 1 && rdpsnd_channel <= 2)
					{
						snprintf( rdpsnd_param2, sizeof(rdpsnd_param2), "channel:%d", rdpsnd_channel );
						rdpsnd_params[rdpsnd_nparams++] = rdpsnd_param2;
					}
				}
			}
		}

		remmina_rdp_add_static_channel(rfi->settings, rdpsnd_nparams, (char**) rdpsnd_params);

	}

	rfi->settings->RedirectClipboard = ( remmina_plugin_service->file_get_int(remminafile, "disableclipboard", FALSE) ? FALSE: TRUE );

	cs = remmina_plugin_service->file_get_string(remminafile, "sharefolder");

	if (cs && cs[0] == '/')
	{
		RDPDR_DRIVE* drive;
		gsize sz;

		drive = (RDPDR_DRIVE*) malloc(sizeof(RDPDR_DRIVE));
		ZeroMemory(drive, sizeof(RDPDR_DRIVE));

		s = strrchr( cs, '/' );
		if ( s == NULL || s[1] == 0 )
			s = remmina_rdp_plugin_default_drive_name;
		else
			s++;
		s = g_convert_with_fallback(s, -1, "ascii", "utf-8", "_", NULL, &sz, NULL);

		drive->Type = RDPDR_DTYP_FILESYSTEM;
		drive->Name = _strdup(s);
		drive->Path = _strdup(cs);
		g_free(s);

		freerdp_device_collection_add(rfi->settings, (RDPDR_DEVICE*) drive);
		rfi->settings->DeviceRedirection = TRUE;
	}

	if (remmina_plugin_service->file_get_int(remminafile, "shareprinter", FALSE))
	{
		RDPDR_PRINTER* printer;
		printer = (RDPDR_PRINTER*) malloc(sizeof(RDPDR_PRINTER));
		ZeroMemory(printer, sizeof(RDPDR_PRINTER));

		printer->Type = RDPDR_DTYP_PRINT;

		freerdp_device_collection_add(rfi->settings, (RDPDR_DEVICE*) printer);
		rfi->settings->DeviceRedirection = TRUE;

	}

	if (remmina_plugin_service->file_get_int(remminafile, "sharesmartcard", FALSE))
	{
		RDPDR_SMARTCARD* smartcard;
		smartcard = (RDPDR_SMARTCARD*) malloc(sizeof(RDPDR_SMARTCARD));
		ZeroMemory(smartcard, sizeof(RDPDR_SMARTCARD));

		smartcard->Type = RDPDR_DTYP_SMARTCARD;
		smartcard->Name = _strdup("scard");

		freerdp_device_collection_add(rfi->settings, (RDPDR_DEVICE*) smartcard);
		rfi->settings->DeviceRedirection = TRUE;
	}


	if (!freerdp_connect(rfi->instance))
	{
		if (!rfi->user_cancelled)
		{
			remmina_plugin_service->protocol_plugin_set_error(gp, _("Unable to connect to RDP server %s"), rfi->settings->ServerHostname);

		}

		return FALSE;
	}



	remmina_rdp_main_loop(gp);

	return TRUE;
}

static gpointer remmina_rdp_main_thread(gpointer data)
{
	RemminaProtocolWidget* gp;
	rfContext* rfi;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	CANCEL_ASYNC
	gp = (RemminaProtocolWidget*) data;
	rfi = GET_DATA(gp);
	remmina_rdp_main(gp);
	rfi->thread = 0;
	IDLE_ADD((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);

	return NULL;
}

static void remmina_rdp_init(RemminaProtocolWidget* gp)
{
	freerdp* instance;
	rfContext* rfi;

	freerdp_register_addin_provider(freerdp_channels_load_static_addin_entry, 0);

	instance = freerdp_new();
	instance->PreConnect = remmina_rdp_pre_connect;
	instance->PostConnect = remmina_rdp_post_connect;
	instance->Authenticate = remmina_rdp_authenticate;
	instance->VerifyCertificate = remmina_rdp_verify_certificate;
	instance->VerifyChangedCertificate = remmina_rdp_verify_changed_certificate;
	instance->ReceiveChannelData = remmina_rdp_receive_channel_data;

	instance->ContextSize = sizeof(rfContext);
	freerdp_context_new(instance);
	rfi = (rfContext*) instance->context;

	g_object_set_data_full(G_OBJECT(gp), "plugin-data", rfi, free);

	rfi->protocol_widget = gp;
	rfi->instance = instance;
	rfi->settings = instance->settings;
	rfi->instance->context->channels = freerdp_channels_new();
	rfi->connected = False;

	pthread_mutex_init(&rfi->mutex, NULL);

	rfi->gmutex = g_mutex_new();
	rfi->gcond = g_cond_new();

	remmina_rdp_event_init(gp);
}

static gboolean remmina_rdp_open_connection(RemminaProtocolWidget* gp)
{

	rfContext* rfi;

	rfi = GET_DATA(gp);
	rfi->scale = remmina_plugin_service->protocol_plugin_get_scale(gp);

	if (pthread_create(&rfi->thread, NULL, remmina_rdp_main_thread, gp))
	{
		remmina_plugin_service->protocol_plugin_set_error(gp, "%s",
			"Failed to initialize pthread. Falling back to non-thread mode...");

		rfi->thread = 0;

		return FALSE;
	}

	return TRUE;
}

static gboolean remmina_rdp_close_connection(RemminaProtocolWidget* gp)
{
	rfContext* rfi;
	freerdp* instance;


	rfi = GET_DATA(gp);
	instance = rfi->instance;

	if (rfi->thread)
	{
		pthread_cancel(rfi->thread);

		if (rfi->thread)
			pthread_join(rfi->thread, NULL);
	}

	pthread_mutex_destroy(&rfi->mutex);

	g_mutex_free(rfi->gmutex);
	g_cond_free(rfi->gcond);

	remmina_rdp_event_uninit(gp);
	remmina_plugin_service->protocol_plugin_emit_signal(gp, "disconnect");

	if (instance)
	{
		if ( rfi->connected ) {
			if (instance->context->channels)
				freerdp_channels_close(instance->context->channels, instance);
			freerdp_disconnect(instance);
			rfi->connected = False;
		}
	}

	if (rfi->rfx_context)
	{
		rfx_context_free(rfi->rfx_context);
		rfi->rfx_context = NULL;
	}

	if (instance)
	{
		/* Remove instance->context from gp object data to avoid double free */
		g_object_steal_data(G_OBJECT(gp), "plugin-data");

		if (instance->context->channels) {
			freerdp_channels_free(instance->context->channels);
			instance->context->channels = NULL;
		}

		freerdp_context_free(instance); /* context is rfContext* rfi */
		freerdp_free(instance);
		rfi->instance = NULL;
	}

	return FALSE;
}

static gboolean remmina_rdp_query_feature(RemminaProtocolWidget* gp, const RemminaProtocolFeature* feature)
{
	return TRUE;
}

static void remmina_rdp_call_feature(RemminaProtocolWidget* gp, const RemminaProtocolFeature* feature)
{
	RemminaFile* remminafile;
	rfContext* rfi;

	rfi = GET_DATA(gp);
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	switch (feature->id)
	{
		case REMMINA_RDP_FEATURE_UNFOCUS:
			remmina_rdp_event_unfocus(gp);
			break;

		case REMMINA_RDP_FEATURE_SCALE:
			rfi->scale = remmina_plugin_service->file_get_int(remminafile, "scale", FALSE);
			remmina_rdp_event_update_scale(gp);
			break;

		case REMMINA_RDP_FEATURE_TOOL_REFRESH:
			gtk_widget_queue_draw_area(rfi->drawing_area, 0, 0,
				remmina_plugin_service->protocol_plugin_get_width(gp),
				remmina_plugin_service->protocol_plugin_get_height(gp));
			break;

		default:
			break;
	}
}

static gpointer colordepth_list[] =
{
	"8", N_("256 colors (8 bpp)"),
	"15", N_("High color (15 bpp)"),
	"16", N_("High color (16 bpp)"),
	"24", N_("True color (24 bpp)"),
	"32", N_("True color (32 bpp)"),
	"0", N_("RemoteFX (32 bpp)"),
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
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "sharesmartcard", N_("Share smartcard"), TRUE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "disableclipboard", N_("Disable clipboard sync"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK, "console", N_("Attach to console (Windows 2003 / 2003 R2)"), FALSE, NULL, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END, NULL, NULL, FALSE, NULL, NULL }
};

static const RemminaProtocolFeature remmina_rdp_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_RDP_FEATURE_TOOL_REFRESH, N_("Refresh"), GTK_STOCK_REFRESH, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, REMMINA_RDP_FEATURE_SCALE, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, REMMINA_RDP_FEATURE_UNFOCUS, NULL, NULL, NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

static RemminaProtocolPlugin remmina_rdp =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,
	"RDP",
	N_("RDP - Remote Desktop Protocol"),
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

G_MODULE_EXPORT gboolean remmina_plugin_entry(RemminaPluginService* service)
{
	remmina_plugin_service = service;

	bindtextdomain(GETTEXT_PACKAGE, REMMINA_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdp))
		return FALSE;

	remmina_rdpf.export_hints = _("Export connection in Windows .rdp file format");

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdpf))
		return FALSE;

	if (! service->register_plugin((RemminaPlugin*) &remmina_rdps))
		return FALSE;

	remmina_rdp_settings_init();
	freerdp_handle_signals();
	freerdp_channels_global_init();

	return TRUE;
}

