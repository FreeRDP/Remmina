/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2012-2012 Jean-Louis Dupond
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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
	gchar* name = gdk_atom_name(atom);
	rc = 0;
	if (g_strcmp0("UTF8_STRING", name) == 0 || g_strcmp0("text/plain;charset=utf-8", name) == 0) {
		rc = CF_UNICODETEXT;
	}
	if (g_strcmp0("TEXT", name) == 0 || g_strcmp0("text/plain", name) == 0) {
		rc =  CF_TEXT;
	}
	if (g_strcmp0("text/html", name) == 0) {
		rc =  CB_FORMAT_HTML;
	}
	if (g_strcmp0("image/png", name) == 0) {
		rc =  CB_FORMAT_PNG;
	}
	if (g_strcmp0("image/jpeg", name) == 0) {
		rc =  CB_FORMAT_JPEG;
	}
	if (g_strcmp0("image/bmp", name) == 0) {
		rc =  CF_DIB;
	}
	if (g_strcmp0("text/uri-list", name) == 0) {
		rc =  CB_FORMAT_TEXTURILIST;
	}
	g_free(name);
	return rc;
}

void remmina_rdp_cliprdr_get_target_types(UINT32** formats, UINT16* size, GdkAtom* types, int count)
{
	TRACE_CALL(__func__);
	int i;
	*size = 1;
	*formats = (UINT32*)malloc(sizeof(UINT32) * (count + 1));

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

static UINT8* lf2crlf(UINT8* data, int* size)
{
	TRACE_CALL(__func__);
	UINT8 c;
	UINT8* outbuf;
	UINT8* out;
	UINT8* in_end;
	UINT8* in;
	int out_size;

	out_size = (*size) * 2 + 1;
	outbuf = (UINT8*)malloc(out_size);
	out = outbuf;
	in = data;
	in_end = data + (*size);

	while (in < in_end) {
		c = *in++;
		if (c == '\n') {
			*out++ = '\r';
			*out++ = '\n';
		}else  {
			*out++ = c;
		}
	}

	*out++ = 0;
	*size = out - outbuf;

	return outbuf;
}

static void crlf2lf(UINT8* data, size_t* size)
{
	TRACE_CALL(__func__);
	UINT8 c;
	UINT8* out;
	UINT8* in;
	UINT8* in_end;

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

int remmina_rdp_cliprdr_server_file_contents_request(CliprdrClientContext* context, CLIPRDR_FILE_CONTENTS_REQUEST* fileContentsRequest)
{
	TRACE_CALL(__func__);
	return -1;
}
int remmina_rdp_cliprdr_server_file_contents_response(CliprdrClientContext* context, CLIPRDR_FILE_CONTENTS_RESPONSE* fileContentsResponse)
{
	TRACE_CALL(__func__);
	return 1;
}

void remmina_rdp_cliprdr_send_client_format_list(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	rfClipboard* clipboard;
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

static void remmina_rdp_cliprdr_send_client_capabilities(rfClipboard* clipboard)
{
	TRACE_CALL(__func__);
	CLIPRDR_CAPABILITIES capabilities;
	CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet;

	capabilities.cCapabilitiesSets = 1;
	capabilities.capabilitySets = (CLIPRDR_CAPABILITY_SET*)&(generalCapabilitySet);

	generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
	generalCapabilitySet.capabilitySetLength = 12;

	generalCapabilitySet.version = CB_CAPS_VERSION_2;
	generalCapabilitySet.generalFlags = CB_USE_LONG_FORMAT_NAMES;

	clipboard->context->ClientCapabilities(clipboard->context, &capabilities);
}


static UINT remmina_rdp_cliprdr_monitor_ready(CliprdrClientContext* context, const CLIPRDR_MONITOR_READY* monitorReady)
{
	TRACE_CALL(__func__);
	rfClipboard* clipboard = (rfClipboard*)context->custom;
	RemminaProtocolWidget* gp;

	remmina_rdp_cliprdr_send_client_capabilities(clipboard);
	gp = clipboard->rfi->protocol_widget;
	remmina_rdp_cliprdr_send_client_format_list(gp);

	return CHANNEL_RC_OK;
}

static UINT remmina_rdp_cliprdr_server_capabilities(CliprdrClientContext* context, const CLIPRDR_CAPABILITIES* capabilities)
{
	TRACE_CALL(__func__);
	return CHANNEL_RC_OK;
}


static UINT remmina_rdp_cliprdr_server_format_list(CliprdrClientContext* context, const CLIPRDR_FORMAT_LIST* formatList)
{
	TRACE_CALL(__func__);

	/* Called when a user do a "Copy" on the server: we collect all formats
	 * the server send us and then setup the local clipboard with the appropiate
	 * targets to request server data */

	RemminaPluginRdpUiObject* ui;
	RemminaProtocolWidget* gp;
	rfClipboard* clipboard;
	CLIPRDR_FORMAT* format;
	CLIPRDR_FORMAT_LIST_RESPONSE formatListResponse;
	const char *serverFormatName;

	int has_dib_level = 0;

	int i;

	clipboard = (rfClipboard*)context->custom;
	gp = clipboard->rfi->protocol_widget;
	GtkTargetList* list = gtk_target_list_new(NULL, 0);

	g_debug("[RDP] format list from the server:");
	for (i = 0; i < formatList->numFormats; i++) {
		format = &formatList->formats[i];
		serverFormatName = format->formatName;
		if (format->formatId == CF_UNICODETEXT) {
			serverFormatName = "CF_UNICODETEXT";
			GdkAtom atom = gdk_atom_intern("UTF8_STRING", TRUE);
			gtk_target_list_add(list, atom, 0, CF_UNICODETEXT);
		}else if (format->formatId == CF_TEXT) {
			serverFormatName = "CF_TEXT";
			GdkAtom atom = gdk_atom_intern("TEXT", TRUE);
			gtk_target_list_add(list, atom, 0, CF_TEXT);
		}else if (format->formatId == CF_DIB) {
			serverFormatName = "CF_DIB";
			if (has_dib_level < 1)
				has_dib_level = 1;
		}else if (format->formatId == CF_DIBV5) {
			serverFormatName = "CF_DIBV5";
			if (has_dib_level < 5)
				has_dib_level = 5;
		}else if (format->formatId == CB_FORMAT_JPEG) {
			serverFormatName = "CB_FORMAT_JPEG";
			GdkAtom atom = gdk_atom_intern("image/jpeg", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_JPEG);
		}else if (format->formatId == CB_FORMAT_PNG) {
			serverFormatName = "CB_FORMAT_PNG";
			GdkAtom atom = gdk_atom_intern("image/png", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_PNG);
		}else if (format->formatId == CB_FORMAT_HTML) {
			serverFormatName = "CB_FORMAT_HTML";
			GdkAtom atom = gdk_atom_intern("text/html", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_HTML);
		}else if (format->formatId == CB_FORMAT_TEXTURILIST) {
			serverFormatName = "CB_FORMAT_TEXTURILIST";
			GdkAtom atom = gdk_atom_intern("text/uri-list", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_TEXTURILIST);
		}else if (format->formatId == CF_LOCALE) {
			serverFormatName = "CF_LOCALE";
		}else if (format->formatId == CF_METAFILEPICT) {
			serverFormatName = "CF_METAFILEPICT";
		}
		g_debug("[RDP] the server has clipboard format %d: %s", format->formatId, serverFormatName);
	}

	/* Keep only one DIB format, if present */
	if (has_dib_level) {
		GdkAtom atom = gdk_atom_intern("image/bmp", TRUE);
		if (has_dib_level == 5)
			gtk_target_list_add(list, atom, 0, CF_DIBV5);
		else
			gtk_target_list_add(list, atom, 0, CF_DIB);
	}

	/* Now we tell GTK to change the local keyboard calling gtk_clipboard_set_with_owner
	 * via REMMINA_RDP_UI_CLIPBOARD_SET_DATA
	 * GTK will immediately fire an "owner-change" event, that we should ignore */

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.clipboard = clipboard;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_SET_DATA;
	ui->clipboard.targetlist = list;
	remmina_rdp_event_queue_ui_sync_retint(gp, ui);

	/* Send FormatListResponse to server */

	formatListResponse.msgType = CB_FORMAT_LIST_RESPONSE;
	formatListResponse.msgFlags = CB_RESPONSE_OK; // Can be CB_RESPONSE_FAIL in case of error
	formatListResponse.dataLen = 0;

	return clipboard->context->ClientFormatListResponse(clipboard->context, &formatListResponse);

}

static UINT remmina_rdp_cliprdr_server_format_list_response(CliprdrClientContext* context, const CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
	TRACE_CALL(__func__);
	return CHANNEL_RC_OK;
}


static UINT remmina_rdp_cliprdr_server_format_data_request(CliprdrClientContext* context, const CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
	TRACE_CALL(__func__);

	RemminaPluginRdpUiObject* ui;
	RemminaProtocolWidget* gp;
	rfClipboard* clipboard;

	clipboard = (rfClipboard*)context->custom;
	gp = clipboard->rfi->protocol_widget;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.clipboard = clipboard;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_GET_DATA;
	ui->clipboard.format = formatDataRequest->requestedFormatId;
	remmina_rdp_event_queue_ui_sync_retint(gp, ui);

	return CHANNEL_RC_OK;
}

static UINT remmina_rdp_cliprdr_server_format_data_response(CliprdrClientContext* context, const CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
	TRACE_CALL(__func__);

	const UINT8* data;
	size_t size;
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	rfClipboard* clipboard;
	GdkPixbufLoader *pixbuf;
	gpointer output = NULL;

	clipboard = (rfClipboard*)context->custom;
	gp = clipboard->rfi->protocol_widget;
	rfi = GET_PLUGIN_DATA(gp);

	data = formatDataResponse->requestedFormatData;
	size = formatDataResponse->dataLen;

	// formatDataResponse->requestedFormatData is allocated
	//  by freerdp and freed after returning from this callback function.
	//  So we must make a copy if we need to preserve it

	if (size > 0) {
		switch (rfi->clipboard.format) {
		case CF_UNICODETEXT:
		{
			size = ConvertFromUnicode(CP_UTF8, 0, (WCHAR*)data, size / 2, (CHAR**)&output, 0, NULL, NULL);
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
			wStream* s;
			UINT32 offset;
			GError *perr;
			BITMAPINFOHEADER* pbi;
			BITMAPV5HEADER* pbi5;

			pbi = (BITMAPINFOHEADER*)data;

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
				pbi5 = (BITMAPV5HEADER*)pbi;
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

			pixbuf = gdk_pixbuf_loader_new();
			perr = NULL;
			if ( !gdk_pixbuf_loader_write(pixbuf, data, size, &perr) ) {
				Stream_Free(s, TRUE);
				g_warning("[RDP] rdp_cliprdr: gdk_pixbuf_loader_write() returned error %s\n", perr->message);
			}else  {
				if ( !gdk_pixbuf_loader_close(pixbuf, &perr) ) {
					g_warning("[RDP] rdp_cliprdr: gdk_pixbuf_loader_close() returned error %s\n", perr->message);
					perr = NULL;
				}
				Stream_Free(s, TRUE);
				output = g_object_ref(gdk_pixbuf_loader_get_pixbuf(pixbuf));
			}
			g_object_unref(pixbuf);
			break;
		}

		case CB_FORMAT_PNG:
		case CB_FORMAT_JPEG:
		{
			pixbuf = gdk_pixbuf_loader_new();
			gdk_pixbuf_loader_write(pixbuf, data, size, NULL);
			output = g_object_ref(gdk_pixbuf_loader_get_pixbuf(pixbuf));
			gdk_pixbuf_loader_close(pixbuf, NULL);
			g_object_unref(pixbuf);
			break;
		}
		}
	}

	pthread_mutex_lock(&clipboard->transfer_clip_mutex);
	pthread_cond_signal(&clipboard->transfer_clip_cond);
	if ( clipboard->srv_clip_data_wait == SCDW_BUSY_WAIT ) {
		g_debug("[RDP] clibpoard transfer from server completed.");
		clipboard->srv_data = output;
	}else  {
		// Clipboard data arrived from server when we are not busywaiting on main loop
		// Unfortunately, we must discard it
		g_debug("[RDP] clibpoard transfer from server completed. Data discarded due to abort or timeout.");
		clipboard->srv_clip_data_wait = SCDW_NONE;

	}
	pthread_mutex_unlock(&clipboard->transfer_clip_mutex);

	return CHANNEL_RC_OK;
}

void remmina_rdp_cliprdr_request_data(GtkClipboard *gtkClipboard, GtkSelectionData *selection_data, guint info, RemminaProtocolWidget* gp )
{
	TRACE_CALL(__func__);

	/* Called by GTK when someone press "Paste" on the client side.
	 * We ask to the server the data we need */

	CLIPRDR_FORMAT_DATA_REQUEST* pFormatDataRequest;
	rfClipboard* clipboard;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event = { 0 };
	struct timespec to;
	struct timeval tv;
	int rc;
	time_t tlimit;

	g_debug("[RDP] A local application has requested remote clipboard data for local format id %d", info);

	clipboard = &(rfi->clipboard);
	if ( clipboard->srv_clip_data_wait != SCDW_NONE ) {
		g_message("[RDP] Cannot paste now, I’m already transferring clipboard data from server. Try again later\n");
		return;
	}

	clipboard->format = info;

	/* Request Clipboard content from the server, the request is async */

	pthread_mutex_lock(&clipboard->transfer_clip_mutex);

	pFormatDataRequest = (CLIPRDR_FORMAT_DATA_REQUEST*)malloc(sizeof(CLIPRDR_FORMAT_DATA_REQUEST));
	ZeroMemory(pFormatDataRequest, sizeof(CLIPRDR_FORMAT_DATA_REQUEST));
	pFormatDataRequest->requestedFormatId = clipboard->format;
	clipboard->srv_clip_data_wait = SCDW_BUSY_WAIT;

	g_debug("[RDP] Requesting clipboard data with fotmat %d from the server", clipboard->format);

	rdp_event.type = REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_DATA_REQUEST;
	rdp_event.clipboard_formatdatarequest.pFormatDataRequest = pFormatDataRequest;
	remmina_rdp_event_event_push(gp, &rdp_event);

	/* Busy wait clibpoard data for CLIPBOARD_TRANSFER_WAIT_TIME seconds.
		In the meanwhile allow GTK event loop to proceed */

	tlimit = time(NULL) + CLIPBOARD_TRANSFER_WAIT_TIME;
	rc = 100000;
	while(time(NULL) < tlimit && rc != 0 && clipboard->srv_clip_data_wait == SCDW_BUSY_WAIT) {
		gettimeofday(&tv, NULL);
		to.tv_sec = tv.tv_sec;
		to.tv_nsec = tv.tv_usec * 1000 + 40000000;	// wait for 40ms
		if (to.tv_nsec >= 1000000000) {
			to.tv_nsec -= 1000000000;
			to.tv_sec ++;
		}
		rc = pthread_cond_timedwait(&clipboard->transfer_clip_cond, &clipboard->transfer_clip_mutex, &to);
		if (rc == 0)
			break;
		gtk_main_iteration_do(FALSE);
	}


	if ( rc == 0 ) {
		/* Data has arrived without timeout */
		if (clipboard->srv_data != NULL) {
			if (info == CB_FORMAT_PNG || info == CF_DIB || info == CF_DIBV5 || info == CB_FORMAT_JPEG) {
				gtk_selection_data_set_pixbuf(selection_data, clipboard->srv_data);
				g_object_unref(clipboard->srv_data);
			}else  {
				gtk_selection_data_set_text(selection_data, clipboard->srv_data, -1);
				free(clipboard->srv_data);
			}
		}
		clipboard->srv_clip_data_wait = SCDW_NONE;
	} else {
		if (clipboard->srv_clip_data_wait == SCDW_ABORTING) {
			g_warning("[RDP] Clipboard data wait aborted.");
		} else {
			if ( rc == ETIMEDOUT ) {
				g_warning("[RDP] Clipboard data from the server is not available in %d seconds. No data will be available to user.",
						CLIPBOARD_TRANSFER_WAIT_TIME);
			}else  {
				g_warning("[RDP] internal error: pthread_cond_timedwait() returned %d\n", rc);
				clipboard->srv_clip_data_wait = SCDW_NONE;
			}
		}
		clipboard->srv_clip_data_wait = SCDW_NONE;
	}
	pthread_mutex_unlock(&clipboard->transfer_clip_mutex);

}

void remmina_rdp_cliprdr_empty_clipboard(GtkClipboard *gtkClipboard, rfClipboard *clipboard)
{
	TRACE_CALL(__func__);
	/* No need to do anything here */
}

CLIPRDR_FORMAT_LIST *remmina_rdp_cliprdr_get_client_format_list(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);

	GtkClipboard* gtkClipboard;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	GdkAtom* targets;
	gboolean result = 0;
	gint loccount, srvcount;
	gint formatId, i;
	CLIPRDR_FORMAT *formats;
	gchar *name;
	struct retp_t {
		CLIPRDR_FORMAT_LIST pFormatList;
		CLIPRDR_FORMAT formats[];
	} *retp;

	formats = NULL;

	retp = NULL;

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard) {
		result = gtk_clipboard_wait_for_targets(gtkClipboard, &targets, &loccount);
	}
	g_debug("[RDP] Sending to server the following local clipboard content formats");
	if (result && loccount > 0) {
		formats = (CLIPRDR_FORMAT*)malloc(loccount * sizeof(CLIPRDR_FORMAT));
		srvcount = 0;
		for (i = 0; i < loccount; i++) {
			formatId = remmina_rdp_cliprdr_get_format_from_gdkatom(targets[i]);
			if ( formatId != 0 ) {
				name = gdk_atom_name(targets[i]);
				g_debug("[RDP]     local clipboard format %s will be sent to remote as %d", name, formatId);
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

	retp->pFormatList.msgFlags = CB_RESPONSE_OK;

	return (CLIPRDR_FORMAT_LIST*)retp;
}

static void remmina_rdp_cliprdr_mt_get_format_list(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	ui->retptr = (void*)remmina_rdp_cliprdr_get_client_format_list(gp);
}


void remmina_rdp_cliprdr_get_clipboard_data(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	GtkClipboard* gtkClipboard;
	UINT8* inbuf = NULL;
	UINT8* outbuf = NULL;
	GdkPixbuf *image = NULL;
	int size = 0;
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	RemminaPluginRdpEvent rdp_event = { 0 };

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard) {
		switch (ui->clipboard.format) {
		case CF_TEXT:
		case CF_UNICODETEXT:
		case CB_FORMAT_HTML:
		{
			inbuf = (UINT8*)gtk_clipboard_wait_for_text(gtkClipboard);
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
			size = strlen((char*)inbuf);
			outbuf = lf2crlf(inbuf, &size);
			break;
		}
		case CF_UNICODETEXT:
		{
			size = strlen((char*)inbuf);
			inbuf = lf2crlf(inbuf, &size);
			size = (ConvertToUnicode(CP_UTF8, 0, (CHAR*)inbuf, -1, (WCHAR**)&outbuf, 0) ) * sizeof(WCHAR);
			g_free(inbuf);
			break;
		}
		case CB_FORMAT_PNG:
		{
			gchar* data;
			gsize buffersize;
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "png", NULL, NULL);
			outbuf = (UINT8*)malloc(buffersize);
			memcpy(outbuf, data, buffersize);
			size = buffersize;
			g_object_unref(image);
			break;
		}
		case CB_FORMAT_JPEG:
		{
			gchar* data;
			gsize buffersize;
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "jpeg", NULL, NULL);
			outbuf = (UINT8*)malloc(buffersize);
			memcpy(outbuf, data, buffersize);
			size = buffersize;
			g_object_unref(image);
			break;
		}
		case CF_DIB:
		case CF_DIBV5:
		{
			gchar* data;
			gsize buffersize;
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "bmp", NULL, NULL);
			size = buffersize - 14;
			outbuf = (UINT8*)malloc(size);
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

void remmina_rdp_cliprdr_set_clipboard_content(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	GtkClipboard* gtkClipboard;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (ui->clipboard.format == CB_FORMAT_PNG || ui->clipboard.format == CF_DIB || ui->clipboard.format == CF_DIBV5 || ui->clipboard.format == CB_FORMAT_JPEG) {
		gtk_clipboard_set_image( gtkClipboard, ui->clipboard.data );
		g_object_unref(ui->clipboard.data);
	}else  {
		gtk_clipboard_set_text( gtkClipboard, ui->clipboard.data, -1 );
		free(ui->clipboard.data);
	}

}

void remmina_rdp_cliprdr_set_clipboard_data(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	TRACE_CALL(__func__);
	GtkClipboard* gtkClipboard;
	GtkTargetEntry* targets;
	gint n_targets;
	rfContext* rfi = GET_PLUGIN_DATA(gp);

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard) {
		targets = gtk_target_table_new_from_list(ui->clipboard.targetlist, &n_targets);
		if (targets) {
			g_debug("[RDP] setting clipboard with owner to owner %p", gp);
			gtk_clipboard_set_with_owner(gtkClipboard, targets, n_targets,
				(GtkClipboardGetFunc)remmina_rdp_cliprdr_request_data,
				(GtkClipboardClearFunc)remmina_rdp_cliprdr_empty_clipboard, G_OBJECT(gp));
			gtk_target_table_free(targets, n_targets);
		}
	}
}

void remmina_rdp_cliprdr_detach_owner(RemminaProtocolWidget* gp)
{
	/* When closing a rdp connection, we should check if gp is a clipboard owner.
	 * If it’s an owner, detach it from the clipboard */
	TRACE_CALL(__func__);
	rfContext* rfi = GET_PLUGIN_DATA(gp);
	GtkClipboard* gtkClipboard;

	if (!rfi || !rfi->drawing_area) return;

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard && gtk_clipboard_get_owner(gtkClipboard) == (GObject*)gp) {
		gtk_clipboard_clear(gtkClipboard);
	}

}

void remmina_rdp_event_process_clipboard(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
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

	case REMMINA_RDP_UI_CLIPBOARD_SET_CONTENT:
		remmina_rdp_cliprdr_set_clipboard_content(gp, ui);
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

	// Future: deinitialize rfi->clipboard
}

void remmina_rdp_clipboard_abort_transfer(rfContext *rfi)
{
	if (rfi && rfi->clipboard.srv_clip_data_wait == SCDW_BUSY_WAIT) {
		g_debug("[RDP] requesting clipboard transfer to abort");
		/* Allow clipboard transfer from server to terminate */
		rfi->clipboard.srv_clip_data_wait = SCDW_ABORTING;
		usleep(100000);
	}
}



void remmina_rdp_cliprdr_init(rfContext* rfi, CliprdrClientContext* cliprdr)
{
	TRACE_CALL(__func__);

	rfClipboard* clipboard;
	clipboard = &(rfi->clipboard);

	rfi->clipboard.rfi = rfi;
	cliprdr->custom = (void*)clipboard;

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

