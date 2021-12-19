/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2012-2012 Jean-Louis Dupond
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2022 Antenore Gatta, Giovanni Panozzo
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

#include "rdp_plugin.h"
#include "rdp_cliprdr.h"
#include "rdp_event.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/client/cliprdr.h>
#include <sys/time.h>

#define CLIPBOARD_TRANSFER_WAIT_TIME 6

UINT32 remmina_rdp_cliprdr_get_format_from_gdkatom(GdkAtom atom)
{
	TRACE_CALL(__func__);
	UINT32 rc;
	gchar *name = gdk_atom_name(atom);
	rc = 0;
	if (g_strcmp0("UTF8_STRING", name) == 0 || g_strcmp0("text/plain;charset=utf-8", name) == 0)
		rc = CF_UNICODETEXT;
	if (g_strcmp0("TEXT", name) == 0 || g_strcmp0("text/plain", name) == 0)
		rc = CF_TEXT;
	if (g_strcmp0("text/html", name) == 0)
		rc = CB_FORMAT_HTML;
	if (g_strcmp0("image/png", name) == 0)
		rc = CB_FORMAT_PNG;
	if (g_strcmp0("image/jpeg", name) == 0)
		rc = CB_FORMAT_JPEG;
	if (g_strcmp0("image/bmp", name) == 0)
		rc = CF_DIB;
	if (g_strcmp0("text/uri-list", name) == 0)
		rc = CB_FORMAT_TEXTURILIST;
	g_free(name);
	return rc;
}

/* Never used? */
void remmina_rdp_cliprdr_get_target_types(UINT32 **formats, UINT16 *size, GdkAtom *types, int count)
{
	TRACE_CALL(__func__);
	int i;
	*size = 1;
	*formats = (UINT32 *)malloc(sizeof(UINT32) * (count + 1));

	*formats[0] = 0;
	for (i = 0; i < count; i++) {
		UINT32 format = remmina_rdp_cliprdr_get_format_from_gdkatom(types[i]);
		if (format != 0) {
			(*formats)[*size] = format;
			(*size)++;
		}
	}

	*formats = realloc(*formats, sizeof(UINT32) * (*size));
}

static UINT8 *lf2crlf(UINT8 *data, int *size)
{
	TRACE_CALL(__func__);
	UINT8 c;
	UINT8 *outbuf;
	UINT8 *out;
	UINT8 *in_end;
	UINT8 *in;
	int out_size;

	out_size = (*size) * 2 + 1;
	outbuf = (UINT8 *)malloc(out_size);
	out = outbuf;
	in = data;
	in_end = data + (*size);

	while (in < in_end) {
		c = *in++;
		if (c == '\n') {
			*out++ = '\r';
			*out++ = '\n';
		} else {
			*out++ = c;
		}
	}

	*out++ = 0;
	*size = out - outbuf;

	return outbuf;
}

static void crlf2lf(UINT8 *data, size_t *size)
{
	TRACE_CALL(__func__);
	UINT8 c;
	UINT8 *out;
	UINT8 *in;
	UINT8 *in_end;

	out = data;
	in = data;
	in_end = data + (*size);

	while (in < in_end) {
		c = *in++;
		if (c != '\r')
			*out++ = c;
	}

	*size = out - data;
}

/* Never used? */
int remmina_rdp_cliprdr_server_file_contents_request(CliprdrClientContext *context, CLIPRDR_FILE_CONTENTS_REQUEST *fileContentsRequest)
{
	TRACE_CALL(__func__);
	return -1;
}

/* Never used? */
int remmina_rdp_cliprdr_server_file_contents_response(CliprdrClientContext *context, CLIPRDR_FILE_CONTENTS_RESPONSE *fileContentsResponse)
{
	TRACE_CALL(__func__);
	return 1;
}

void remmina_rdp_cliprdr_send_client_format_list(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginRdpUiObject *ui;
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	rfClipboard *clipboard;
	CLIPRDR_FORMAT_LIST *pFormatList;
	RemminaPluginRdpEvent rdp_event = { 0 };

	if (!rfi || !rfi->connected || rfi->is_reconnecting)
		return;

	clipboard = &(rfi->clipboard);

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.clipboard = clipboard;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_FORMATLIST;
	pFormatList = remmina_rdp_event_queue_ui_sync_retptr(gp, ui);

	rdp_event.type = REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_LIST;
	rdp_event.clipboard_formatlist.pFormatList = pFormatList;
	remmina_rdp_event_event_push(gp, &rdp_event);
}

static void remmina_rdp_cliprdr_send_client_capabilities(rfClipboard *clipboard)
{
	TRACE_CALL(__func__);
	CLIPRDR_CAPABILITIES capabilities;
	CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet;

	capabilities.cCapabilitiesSets = 1;
	capabilities.capabilitySets = (CLIPRDR_CAPABILITY_SET *)&(generalCapabilitySet);

	generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
	generalCapabilitySet.capabilitySetLength = 12;

	generalCapabilitySet.version = CB_CAPS_VERSION_2;
	generalCapabilitySet.generalFlags = CB_USE_LONG_FORMAT_NAMES;

	clipboard->context->ClientCapabilities(clipboard->context, &capabilities);
}

static UINT remmina_rdp_cliprdr_monitor_ready(CliprdrClientContext *context, const CLIPRDR_MONITOR_READY *monitorReady)
{
	TRACE_CALL(__func__);
	rfClipboard *clipboard = (rfClipboard *)context->custom;
	RemminaProtocolWidget *gp;

	remmina_rdp_cliprdr_send_client_capabilities(clipboard);
	gp = clipboard->rfi->protocol_widget;
	remmina_rdp_cliprdr_send_client_format_list(gp);

	return CHANNEL_RC_OK;
}

static UINT remmina_rdp_cliprdr_server_capabilities(CliprdrClientContext *context, const CLIPRDR_CAPABILITIES *capabilities)
{
	TRACE_CALL(__func__);
	return CHANNEL_RC_OK;
}


void remmina_rdp_cliprdr_cached_clipboard_free(rfClipboard *clipboard)
{
	TRACE_CALL(__func__);
	guint fmt;


	if (clipboard->srv_data == NULL)
		return;

	fmt = clipboard->format;
	if (fmt == CB_FORMAT_PNG || fmt == CF_DIB || fmt == CF_DIBV5 || fmt == CB_FORMAT_JPEG) {
		g_object_unref(clipboard->srv_data);
	} else {
		free(clipboard->srv_data);
	}
	clipboard->srv_data = NULL;
}


static UINT remmina_rdp_cliprdr_server_format_list(CliprdrClientContext *context, const CLIPRDR_FORMAT_LIST *formatList)
{
	TRACE_CALL(__func__);

	/* Called when a user do a "Copy" on the server: we collect all formats
	 * the server send us and then setup the local clipboard with the appropriate
	 * targets to request server data */

	RemminaPluginRdpUiObject *ui;
	RemminaProtocolWidget *gp;
	rfClipboard *clipboard;
	CLIPRDR_FORMAT *format;
	CLIPRDR_FORMAT_LIST_RESPONSE formatListResponse;
	UINT rc;

	int has_dib_level = 0;

	int i;

	clipboard = (rfClipboard *)context->custom;
	gp = clipboard->rfi->protocol_widget;

	REMMINA_PLUGIN_DEBUG("gp=%p: Received a new ServerFormatList from server clipboard. Remmina version = %s",
						 gp, VERSION);

	GtkTargetList *list = gtk_target_list_new(NULL, 0);

	if (clipboard->srv_clip_data_wait == SCDW_BUSY_WAIT) {
		REMMINA_PLUGIN_DEBUG("gp=%p: we already have a FormatDataRequest in progress to the server, aborting", gp);
		remmina_rdp_clipboard_abort_client_format_data_request(clipboard->rfi);
	}

	remmina_rdp_cliprdr_cached_clipboard_free(clipboard);
	clipboard->server_html_format_id = 0;

	REMMINA_PLUGIN_DEBUG("gp=%p: format list from the server:", gp);
	for (i = 0; i < formatList->numFormats; i++) {
		format = &formatList->formats[i];
		const char *serverFormatName = format->formatName;
		gchar *gtkFormatName = NULL;
		if (format->formatId == CF_UNICODETEXT) {
			serverFormatName = "CF_UNICODETEXT";
			gtkFormatName = "UTF8_STRING";
			GdkAtom atom = gdk_atom_intern(gtkFormatName, TRUE);
			gtk_target_list_add(list, atom, 0, CF_UNICODETEXT);
		} else if (format->formatId == CF_TEXT) {
			serverFormatName = "CF_TEXT";
			gtkFormatName = "TEXT";
			GdkAtom atom = gdk_atom_intern(gtkFormatName, TRUE);
			gtk_target_list_add(list, atom, 0, CF_TEXT);
		} else if (format->formatId == CF_DIB) {
			serverFormatName = "CF_DIB";
			if (has_dib_level < 1)
				has_dib_level = 1;
		} else if (format->formatId == CF_DIBV5) {
			serverFormatName = "CF_DIBV5";
			if (has_dib_level < 5)
				has_dib_level = 5;
		} else if (format->formatId == CB_FORMAT_JPEG) {
			serverFormatName = "CB_FORMAT_JPEG";
			gtkFormatName = "image/jpeg";
			GdkAtom atom = gdk_atom_intern(gtkFormatName, TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_JPEG);
		} else if (format->formatId == CB_FORMAT_PNG) {
			serverFormatName = "CB_FORMAT_PNG";
			gtkFormatName = "image/png";
			GdkAtom atom = gdk_atom_intern(gtkFormatName, TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_PNG);
		} else if (format->formatId == CB_FORMAT_HTML) {
			serverFormatName = "CB_FORMAT_HTML";
			gtkFormatName = "text/html";
			GdkAtom atom = gdk_atom_intern(gtkFormatName, TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_HTML);
		} else if (format->formatId == CB_FORMAT_TEXTURILIST) {
			serverFormatName = "CB_FORMAT_TEXTURILIST";
			gtkFormatName = "text/uri-list";
			GdkAtom atom = gdk_atom_intern(gtkFormatName, TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_TEXTURILIST);
		} else if (format->formatId == CF_LOCALE) {
			serverFormatName = "CF_LOCALE";
		} else if (format->formatId == CF_METAFILEPICT) {
			serverFormatName = "CF_METAFILEPICT";
		} else if (serverFormatName != NULL && strcmp(serverFormatName, "HTML Format") == 0) {
			gtkFormatName = "text/html";
			GdkAtom atom = gdk_atom_intern(gtkFormatName, TRUE);
			gtk_target_list_add(list, atom, 0, format->formatId);
			clipboard->server_html_format_id = format->formatId;
		}
		REMMINA_PLUGIN_DEBUG("the server has clipboard format %d: %s -> GTK %s", format->formatId, serverFormatName, gtkFormatName);
	}

	/* Keep only one DIB format, if present */
	if (has_dib_level) {
		GdkAtom atom = gdk_atom_intern("image/bmp", TRUE);
		if (has_dib_level == 5)
			gtk_target_list_add(list, atom, 0, CF_DIBV5);
		else
			gtk_target_list_add(list, atom, 0, CF_DIB);
	}


	REMMINA_PLUGIN_DEBUG("gp=%p: sending ClientFormatListResponse to server", gp);
	formatListResponse.msgType = CB_FORMAT_LIST_RESPONSE;
	formatListResponse.msgFlags = CB_RESPONSE_OK; // Can be CB_RESPONSE_FAIL in case of error
	formatListResponse.dataLen = 0;
	rc = clipboard->context->ClientFormatListResponse(clipboard->context, &formatListResponse);
	/* Schedule GTK event to tell GTK to change the local clipboard calling gtk_clipboard_set_with_owner
	 * via REMMINA_RDP_UI_CLIPBOARD_SET_DATA
	 * GTK will immediately fire an "owner-change" event, that we should ignore.
	 * And if we are putting text on the clipboard, mutter or other clipboard
	 * managers may try immediately request a clipboard transfer. */

	/* Ensure we have at least one target into list, or Gtk will not change owner
	  when setting clipboard owner later.
	  We put it directly in the clipboard cache (clipboard->srv_data),
	  so remmina will never ask it to the server via ClientFormatDataRequest */
	gint n_targets;
	GtkTargetEntry *target_table = gtk_target_table_new_from_list(list, &n_targets);
	if (target_table)
		gtk_target_table_free(target_table, n_targets);
	if (n_targets == 0) {
		REMMINA_PLUGIN_DEBUG("gp=%p adding a dummy text target (empty text) for local clipboard, because we have no interesting targets from the server. Putting it in the local clipboard cache.");
		GdkAtom atom = gdk_atom_intern("UTF8_STRING", TRUE);
		gtk_target_list_add(list, atom, 0, CF_UNICODETEXT);
		clipboard->srv_data = malloc(1);
		((char *)(clipboard->srv_data))[0] = 0;
		clipboard->format = CF_UNICODETEXT;
	}


	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.clipboard = clipboard;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_SET_DATA;
	ui->clipboard.targetlist = list;
	remmina_rdp_event_queue_ui_async(gp, ui);

	REMMINA_PLUGIN_DEBUG("gp=%p: processing of ServerFormatList ended, returning rc=%u to libfreerdp", gp, rc);
	return rc;
}

static UINT remmina_rdp_cliprdr_server_format_list_response(CliprdrClientContext *context, const CLIPRDR_FORMAT_LIST_RESPONSE *formatListResponse)
{
	TRACE_CALL(__func__);
	return CHANNEL_RC_OK;
}

static UINT remmina_rdp_cliprdr_server_format_data_request(CliprdrClientContext *context, const CLIPRDR_FORMAT_DATA_REQUEST *formatDataRequest)
{
	TRACE_CALL(__func__);

	RemminaPluginRdpUiObject *ui;
	RemminaProtocolWidget *gp;
	rfClipboard *clipboard;

	clipboard = (rfClipboard *)context->custom;
	gp = clipboard->rfi->protocol_widget;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.clipboard = clipboard;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_GET_DATA;
	ui->clipboard.format = formatDataRequest->requestedFormatId;
	remmina_rdp_event_queue_ui_sync_retint(gp, ui);

	return CHANNEL_RC_OK;
}

int timeval_diff(struct timeval *start, struct timeval *end)
{
	/* Returns the time elapsed from start to end, in ms */
	int64_t ms_end = (end->tv_sec * 1000) + (end->tv_usec / 1000);
	int64_t ms_start = (start->tv_sec * 1000) + (start->tv_usec / 1000);

	return (int)(ms_end - ms_start);
}


static UINT remmina_rdp_cliprdr_server_format_data_response(CliprdrClientContext *context, const CLIPRDR_FORMAT_DATA_RESPONSE *formatDataResponse)
{
	TRACE_CALL(__func__);

	const UINT8 *data;
	size_t size;
	rfContext *rfi;
	RemminaProtocolWidget *gp;
	rfClipboard *clipboard;
	gpointer output = NULL;
	struct timeval now;
	int mstrans;

	clipboard = (rfClipboard *)context->custom;
	gp = clipboard->rfi->protocol_widget;
	rfi = GET_PLUGIN_DATA(gp);

	data = formatDataResponse->requestedFormatData;
	size = formatDataResponse->dataLen;

	REMMINA_PLUGIN_DEBUG("gp=%p server FormatDataResponse received: clipboard data arrived form server.", gp);
	gettimeofday(&now, NULL);
	remmina_rdp_cliprdr_cached_clipboard_free(clipboard);

	/* Calculate stats */
	mstrans = timeval_diff(&(clipboard->clientformatdatarequest_tv), &now);
	REMMINA_PLUGIN_DEBUG("gp=%p %zu bytes transferred from server in %d ms. Speed is %d bytes/sec",
		gp, (size_t)size, mstrans, mstrans != 0 ? (int)((int64_t)size * 1000 / mstrans) : 0);

	if (size > 0) {
		switch (rfi->clipboard.format) {
		case CF_UNICODETEXT:
		{
			size = ConvertFromUnicode(CP_UTF8, 0, (WCHAR *)data, size / 2, (CHAR **)&output, 0, NULL, NULL);
			crlf2lf(output, &size);
			break;
		}

		case CF_TEXT:
		case CB_FORMAT_HTML:
		{
			output = (gpointer)calloc(1, size + 1);
			if (output) {
				memcpy(output, data, size);
				crlf2lf(output, &size);
			}
			break;
		}

		case CF_DIBV5:
		case CF_DIB:
		{
			wStream *s;
			UINT32 offset;
			GError *perr;
			BITMAPINFOHEADER *pbi;
			BITMAPV5HEADER *pbi5;

			pbi = (BITMAPINFOHEADER *)data;

			// offset calculation inspired by http://downloads.poolelan.com/MSDN/MSDNLibrary6/Disk1/Samples/VC/OS/WindowsXP/GetImage/BitmapUtil.cpp
			offset = 14 + pbi->biSize;
			if (pbi->biClrUsed != 0)
				offset += sizeof(RGBQUAD) * pbi->biClrUsed;
			else if (pbi->biBitCount <= 8)
				offset += sizeof(RGBQUAD) * (1 << pbi->biBitCount);
			if (pbi->biSize == sizeof(BITMAPINFOHEADER)) {
				if (pbi->biCompression == 3)         // BI_BITFIELDS is 3
					offset += 12;
			} else if (pbi->biSize >= sizeof(BITMAPV5HEADER)) {
				pbi5 = (BITMAPV5HEADER *)pbi;
				if (pbi5->bV5ProfileData <= offset)
					offset += pbi5->bV5ProfileSize;
			}
			s = Stream_New(NULL, 14 + size);
			Stream_Write_UINT8(s, 'B');
			Stream_Write_UINT8(s, 'M');
			Stream_Write_UINT32(s, 14 + size);
			Stream_Write_UINT32(s, 0);
			Stream_Write_UINT32(s, offset);
			Stream_Write(s, data, size);

			data = Stream_Buffer(s);
			size = Stream_Length(s);

			GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
			perr = NULL;
			if (!gdk_pixbuf_loader_write(loader, data, size, &perr)) {
				Stream_Free(s, TRUE);
				g_warning("[RDP] rdp_cliprdr: gdk_pixbuf_loader_write() returned error %s\n", perr->message);
			} else {
				if (!gdk_pixbuf_loader_close(loader, &perr)) {
					g_warning("[RDP] rdp_cliprdr: gdk_pixbuf_loader_close() returned error %s\n", perr->message);
					perr = NULL;
				}
				Stream_Free(s, TRUE);
				output = g_object_ref(gdk_pixbuf_loader_get_pixbuf(loader));
			}
			g_object_unref(loader);
			break;
		}

		case CB_FORMAT_PNG:
		case CB_FORMAT_JPEG:
		{
			GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
			gdk_pixbuf_loader_write(loader, data, size, NULL);
			output = g_object_ref(gdk_pixbuf_loader_get_pixbuf(loader));
			gdk_pixbuf_loader_close(loader, NULL);
			g_object_unref(loader);
			break;
		}
		default:
		{
			if (rfi->clipboard.format == clipboard->server_html_format_id) {
				/* Converting from Microsoft HTML Clipboard Format to pure text/html
				 * https://docs.microsoft.com/en-us/windows/win32/dataxchg/html-clipboard-format */
				int p = 0, lstart = 0;
				size_t osize;
				char c;
				/* Find the 1st starting with '<' */
				while(p < size && (c = data[p]) != 0) {
					if (c == '<' && p == lstart) break;
					if (c == '\n') lstart = p + 1;
					p++;
				}
				if (p < size) {
					osize = size - lstart;
					output = (gpointer)calloc(1, osize + 1);
					if (output) {
						memcpy(output, data + lstart, osize);
						((char *)output)[osize] = 0;
					}
				}
			}
		}
		}
	}

	clipboard->srv_data = output;
	if (output != NULL)
		REMMINA_PLUGIN_DEBUG("gp=%p: clipboard local cache data has been loaded", gp);
	else
		REMMINA_PLUGIN_DEBUG("gp=%p: data from server is not valid (size=%zu format=%d), cannot load into cache", gp, size, rfi->clipboard.format);


	REMMINA_PLUGIN_DEBUG("gp=%p: signalling main GTK thread that we have some clipboard data.", gp);

	pthread_mutex_lock(&clipboard->transfer_clip_mutex);
	pthread_cond_signal(&clipboard->transfer_clip_cond);
	if (clipboard->srv_clip_data_wait == SCDW_BUSY_WAIT) {
		REMMINA_PLUGIN_DEBUG("gp=%p: clipboard transfer from server completed.", gp);
	} else {
		// Clipboard data arrived from server when we are not busy waiting on main loop
		// Unfortunately, we must discard it
		REMMINA_PLUGIN_DEBUG("gp=%p: clipboard transfer from server completed, but no local application is requesting it. Data is on local cache now, try to paste later.", gp);
	}
	clipboard->srv_clip_data_wait = SCDW_NONE;
	pthread_mutex_unlock(&clipboard->transfer_clip_mutex);

	return CHANNEL_RC_OK;
}


void remmina_rdp_cliprdr_request_data(GtkClipboard *gtkClipboard, GtkSelectionData *selection_data, guint info, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	/* Called by GTK when someone presses "Paste" on the client side.
	 * We ask to the server the data we need */

	CLIPRDR_FORMAT_DATA_REQUEST *pFormatDataRequest;
	rfClipboard *clipboard;
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event = { 0 };
	struct timespec to;
	struct timeval tv;
	int rc;
	time_t tlimit, tlimit1s, tnow, tstart;

	REMMINA_PLUGIN_DEBUG("gp=%p: A local application has requested remote clipboard data for remote format id %d", gp, info);

	clipboard = &(rfi->clipboard);
	if (clipboard->srv_clip_data_wait != SCDW_NONE) {
		g_message("[RDP] Cannot paste now, I’m already transferring clipboard data from server. Try again later\n");
		return;
	}

	if (clipboard->format != info || clipboard->srv_data == NULL) {
		/* We do not have a local cached clipoard, so we have to start a remote request */
		remmina_rdp_cliprdr_cached_clipboard_free(clipboard);

		clipboard->format = info;

		pthread_mutex_lock(&clipboard->transfer_clip_mutex);

		pFormatDataRequest = (CLIPRDR_FORMAT_DATA_REQUEST *)malloc(sizeof(CLIPRDR_FORMAT_DATA_REQUEST));
		ZeroMemory(pFormatDataRequest, sizeof(CLIPRDR_FORMAT_DATA_REQUEST));
		pFormatDataRequest->requestedFormatId = clipboard->format;

		clipboard->srv_clip_data_wait = SCDW_BUSY_WAIT;	// Annotate that we are waiting for ServerFormatDataResponse

		REMMINA_PLUGIN_DEBUG("gp=%p Requesting clipboard data with format %d from the server via ServerFormatDataRequest", gp, clipboard->format);
		rdp_event.type = REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_DATA_REQUEST;
		rdp_event.clipboard_formatdatarequest.pFormatDataRequest = pFormatDataRequest;
		remmina_rdp_event_event_push(gp, &rdp_event);

		/* Busy wait clipboard data for CLIPBOARD_TRANSFER_WAIT_TIME seconds.
		*      In the meanwhile allow GTK event loop to proceed */

		tstart = time(NULL);
		tlimit = tstart + CLIPBOARD_TRANSFER_WAIT_TIME;
		rc = 100000;
		tlimit1s = tstart + 1;
		while ((tnow = time(NULL)) < tlimit && rc != 0 && clipboard->srv_clip_data_wait == SCDW_BUSY_WAIT) {

			if (tnow >= tlimit1s) {
				REMMINA_PLUGIN_DEBUG("gp=%p, clipboard data is still not here after %u seconds", gp, (unsigned)(tnow - tstart));
				tlimit1s = time(NULL) + 1;
			}

			gettimeofday(&tv, NULL);
			to.tv_sec = tv.tv_sec;
			to.tv_nsec = tv.tv_usec * 1000 + 5000000;      // wait for data for 5ms
			if (to.tv_nsec >= 1000000000) {
				to.tv_nsec -= 1000000000;
				to.tv_sec++;
			}
			rc = pthread_cond_timedwait(&clipboard->transfer_clip_cond, &clipboard->transfer_clip_mutex, &to);
			if (rc == 0)
				break;

			gtk_main_iteration_do(FALSE);
		}

		if (rc != 0) {
			/* Timeout, just log it and hope that data will arrive later */
			if (clipboard->srv_clip_data_wait == SCDW_ABORTING) {
				g_warning("[RDP] gp=%p Clipboard data wait aborted.",gp);
			} else {
				if (rc == ETIMEDOUT)
					g_warning("[RDP] gp=%p Clipboard data from the server is not available in %d seconds. No data will be available to user.",
						gp, CLIPBOARD_TRANSFER_WAIT_TIME);
				else
					g_warning("[RDP] gp=%p internal error: pthread_cond_timedwait() returned %d\n", gp, rc);
			}
		}

		pthread_mutex_unlock(&clipboard->transfer_clip_mutex);

	}

	if (clipboard->srv_data != NULL) {
		REMMINA_PLUGIN_DEBUG("gp=%p pasting data to local application", gp);
		/* We have data in cache, just paste it */
		if (info == CB_FORMAT_PNG || info == CF_DIB || info == CF_DIBV5 || info == CB_FORMAT_JPEG) {
			gtk_selection_data_set_pixbuf(selection_data, clipboard->srv_data);
		} else if (info == CB_FORMAT_HTML || info == clipboard->server_html_format_id) {
			REMMINA_PLUGIN_DEBUG("gp=%p returning %zu bytes of HTML in clipboard to requesting application", gp, strlen(clipboard->srv_data));
			GdkAtom atom = gdk_atom_intern("text/html", TRUE);
			gtk_selection_data_set(selection_data, atom, 8, clipboard->srv_data, strlen(clipboard->srv_data));
		} else {
			REMMINA_PLUGIN_DEBUG("gp=%p returning %zu bytes of text in clipboard to requesting application", gp, strlen(clipboard->srv_data));
			gtk_selection_data_set_text(selection_data, clipboard->srv_data, -1);
		}
		clipboard->srv_clip_data_wait = SCDW_NONE;
	} else {
		REMMINA_PLUGIN_DEBUG("gp=%p cannot paste data to local application because ->srv_data is NULL", gp);
	}

}

void remmina_rdp_cliprdr_empty_clipboard(GtkClipboard *gtkClipboard, rfClipboard *clipboard)
{
	TRACE_CALL(__func__);
	/* No need to do anything here */
}

CLIPRDR_FORMAT_LIST *remmina_rdp_cliprdr_get_client_format_list(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);

	GtkClipboard *gtkClipboard;
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	GdkAtom *targets;
	gboolean result = 0;
	gint loccount, srvcount;
	gint formatId, i;
	CLIPRDR_FORMAT *formats;
	struct retp_t {
		CLIPRDR_FORMAT_LIST	pFormatList;
		CLIPRDR_FORMAT		formats[];
	} *retp;

	formats = NULL;
	retp = NULL;
	loccount = 0;

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard)
		result = gtk_clipboard_wait_for_targets(gtkClipboard, &targets, &loccount);
	REMMINA_PLUGIN_DEBUG("gp=%p sending to server the following local clipboard content formats", gp);
	if (result && loccount > 0) {
		formats = (CLIPRDR_FORMAT *)malloc(loccount * sizeof(CLIPRDR_FORMAT));
		srvcount = 0;
		for (i = 0; i < loccount; i++) {
			formatId = remmina_rdp_cliprdr_get_format_from_gdkatom(targets[i]);
			if (formatId != 0) {
				gchar *name = gdk_atom_name(targets[i]);
				REMMINA_PLUGIN_DEBUG("     local clipboard format %s will be sent to remote as %d", name, formatId);
				g_free(name);
				formats[srvcount].formatId = formatId;
				formats[srvcount].formatName = NULL;
				srvcount++;
			}
		}
		if (srvcount > 0) {
			retp = (struct retp_t *)malloc(sizeof(struct retp_t) + sizeof(CLIPRDR_FORMAT) * srvcount);
			retp->pFormatList.formats = retp->formats;
			retp->pFormatList.numFormats = srvcount;
			memcpy(retp->formats, formats, sizeof(CLIPRDR_FORMAT) * srvcount);
		} else {
			retp = (struct retp_t *)malloc(sizeof(struct retp_t));
			retp->pFormatList.formats = NULL;
			retp->pFormatList.numFormats = 0;
		}
		free(formats);
	} else {
		retp = (struct retp_t *)malloc(sizeof(struct retp_t) + sizeof(CLIPRDR_FORMAT));
		retp->pFormatList.formats = NULL;
		retp->pFormatList.numFormats = 0;
	}

	if (result)
		g_free(targets);

	retp->pFormatList.msgType = CB_FORMAT_LIST;
	retp->pFormatList.msgFlags = 0;

	return (CLIPRDR_FORMAT_LIST *)retp;
}

static void remmina_rdp_cliprdr_mt_get_format_list(RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
	TRACE_CALL(__func__);
	ui->retptr = (void *)remmina_rdp_cliprdr_get_client_format_list(gp);
}

void remmina_rdp_cliprdr_get_clipboard_data(RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
	TRACE_CALL(__func__);
	GtkClipboard *gtkClipboard;
	UINT8 *inbuf = NULL;
	UINT8 *outbuf = NULL;
	GdkPixbuf *image = NULL;
	int size = 0;
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event = { 0 };

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard) {
		switch (ui->clipboard.format) {
		case CF_TEXT:
		case CF_UNICODETEXT:
		case CB_FORMAT_HTML:
		{
			inbuf = (UINT8 *)gtk_clipboard_wait_for_text(gtkClipboard);
			break;
		}

		case CB_FORMAT_PNG:
		case CB_FORMAT_JPEG:
		case CF_DIB:
		case CF_DIBV5:
		{
			image = gtk_clipboard_wait_for_image(gtkClipboard);
			break;
		}
		}
	}

	/* No data received, send nothing */
	if (inbuf != NULL || image != NULL) {
		switch (ui->clipboard.format) {
		case CF_TEXT:
		case CB_FORMAT_HTML:
		{
			size = strlen((char *)inbuf);
			outbuf = lf2crlf(inbuf, &size);
			break;
		}
		case CF_UNICODETEXT:
		{
			size = strlen((char *)inbuf);
			inbuf = lf2crlf(inbuf, &size);
			size = (ConvertToUnicode(CP_UTF8, 0, (CHAR *)inbuf, -1, (WCHAR **)&outbuf, 0)) * sizeof(WCHAR);
			g_free(inbuf);
			break;
		}
		case CB_FORMAT_PNG:
		{
			gchar *data;
			gsize buffersize;
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "png", NULL, NULL);
			outbuf = (UINT8 *)malloc(buffersize);
			memcpy(outbuf, data, buffersize);
			size = buffersize;
			g_object_unref(image);
			break;
		}
		case CB_FORMAT_JPEG:
		{
			gchar *data;
			gsize buffersize;
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "jpeg", NULL, NULL);
			outbuf = (UINT8 *)malloc(buffersize);
			memcpy(outbuf, data, buffersize);
			size = buffersize;
			g_object_unref(image);
			break;
		}
		case CF_DIB:
		case CF_DIBV5:
		{
			gchar *data;
			gsize buffersize;
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "bmp", NULL, NULL);
			size = buffersize - 14;
			outbuf = (UINT8 *)malloc(size);
			memcpy(outbuf, data + 14, size);
			g_object_unref(image);
			break;
		}
		}
	}

	rdp_event.type = REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_DATA_RESPONSE;
	rdp_event.clipboard_formatdataresponse.data = outbuf;
	rdp_event.clipboard_formatdataresponse.size = size;
	remmina_rdp_event_event_push(gp, &rdp_event);
}

void remmina_rdp_cliprdr_set_clipboard_data(RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
	TRACE_CALL(__func__);
	GtkClipboard *gtkClipboard;
	gint n_targets;
	rfContext *rfi = GET_PLUGIN_DATA(gp);

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard) {
		GtkTargetEntry *targets = gtk_target_table_new_from_list(ui->clipboard.targetlist, &n_targets);
		if (!targets) {
			/* If no targets exists, this is an internal error, because
			 * remmina_rdp_cliprdr_server_format_list() must have produced
			 * at least one target before calling REMMINA_RDP_UI_CLIPBOARD_SET_DATA */
			g_warning("[RDP] internal error: no targets to insert into the local clipboard");
		}

		REMMINA_PLUGIN_DEBUG("setting clipboard with owner to me: %p", gp);
		gtk_clipboard_set_with_owner(gtkClipboard, targets, n_targets,
							(GtkClipboardGetFunc)remmina_rdp_cliprdr_request_data,
							(GtkClipboardClearFunc)remmina_rdp_cliprdr_empty_clipboard, G_OBJECT(gp));
		gtk_target_table_free(targets, n_targets);
	}
}

void remmina_rdp_cliprdr_detach_owner(RemminaProtocolWidget *gp)
{
	/* When closing a rdp connection, we should check if gp is a clipboard owner.
	 * If it’s an owner, detach it from the clipboard */
	TRACE_CALL(__func__);
	rfContext *rfi = GET_PLUGIN_DATA(gp);
	GtkClipboard *gtkClipboard;

	if (!rfi || !rfi->drawing_area) return;

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard && gtk_clipboard_get_owner(gtkClipboard) == (GObject *)gp)
		gtk_clipboard_clear(gtkClipboard);

}

void remmina_rdp_event_process_clipboard(RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
	TRACE_CALL(__func__);

	switch (ui->clipboard.type) {
	case REMMINA_RDP_UI_CLIPBOARD_FORMATLIST:
		remmina_rdp_cliprdr_mt_get_format_list(gp, ui);
		break;

	case REMMINA_RDP_UI_CLIPBOARD_GET_DATA:
		remmina_rdp_cliprdr_get_clipboard_data(gp, ui);
		break;

	case REMMINA_RDP_UI_CLIPBOARD_SET_DATA:
		remmina_rdp_cliprdr_set_clipboard_data(gp, ui);
		break;

	}
}

void remmina_rdp_clipboard_init(rfContext *rfi)
{
	TRACE_CALL(__func__);
	// Future: initialize rfi->clipboard
}
void remmina_rdp_clipboard_free(rfContext *rfi)
{
	TRACE_CALL(__func__);

	remmina_rdp_cliprdr_cached_clipboard_free(&(rfi->clipboard));

}

void remmina_rdp_clipboard_abort_client_format_data_request(rfContext *rfi)
{
	TRACE_CALL(__func__);
	if (rfi && rfi->clipboard.srv_clip_data_wait == SCDW_BUSY_WAIT) {
		REMMINA_PLUGIN_DEBUG("requesting clipboard data transfer from server to be ignored and busywait loop to exit");
		/* Allow clipboard transfer from server to terminate */
		rfi->clipboard.srv_clip_data_wait = SCDW_ABORTING;
		usleep(100000);
	}
}

void remmina_rdp_cliprdr_init(rfContext *rfi, CliprdrClientContext *cliprdr)
{
	TRACE_CALL(__func__);

	rfClipboard *clipboard;
	clipboard = &(rfi->clipboard);

	rfi->clipboard.rfi = rfi;
	cliprdr->custom = (void *)clipboard;

	clipboard->context = cliprdr;
	pthread_mutex_init(&clipboard->transfer_clip_mutex, NULL);
	pthread_cond_init(&clipboard->transfer_clip_cond, NULL);
	clipboard->srv_clip_data_wait = SCDW_NONE;

	cliprdr->MonitorReady = remmina_rdp_cliprdr_monitor_ready;
	cliprdr->ServerCapabilities = remmina_rdp_cliprdr_server_capabilities;
	cliprdr->ServerFormatList = remmina_rdp_cliprdr_server_format_list;
	cliprdr->ServerFormatListResponse = remmina_rdp_cliprdr_server_format_list_response;
	cliprdr->ServerFormatDataRequest = remmina_rdp_cliprdr_server_format_data_request;
	cliprdr->ServerFormatDataResponse = remmina_rdp_cliprdr_server_format_data_response;

//	cliprdr->ServerFileContentsRequest = remmina_rdp_cliprdr_server_file_contents_request;
//	cliprdr->ServerFileContentsResponse = remmina_rdp_cliprdr_server_file_contents_response;
}
