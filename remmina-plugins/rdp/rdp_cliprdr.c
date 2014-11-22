/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2012-2012 Jean-Louis Dupond
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
#include "rdp_cliprdr.h"

#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/client/cliprdr.h>



UINT32 remmina_rdp_cliprdr_get_format_from_gdkatom(GdkAtom atom)
{
	gchar* name = gdk_atom_name(atom);
	if (g_strcmp0("UTF8_STRING", name) == 0 || g_strcmp0("text/plain;charset=utf-8", name) == 0)
	{
		return CF_UNICODETEXT;
	}
	if (g_strcmp0("TEXT", name) == 0 || g_strcmp0("text/plain", name) == 0)
	{
		return CF_TEXT;
	}
	if (g_strcmp0("text/html", name) == 0)
	{
		return CB_FORMAT_HTML;
	}
	if (g_strcmp0("image/png", name) == 0)
	{
		return CB_FORMAT_PNG;
	}
	if (g_strcmp0("image/jpeg", name) == 0)
	{
	   return CB_FORMAT_JPEG;
	}
	if (g_strcmp0("image/bmp", name) == 0)
	{
		return CF_DIB;
	}
	return 0;
}

void remmina_rdp_cliprdr_get_target_types(UINT32** formats, UINT16* size, GdkAtom* types, int count)
{
	int i;
	*size = 1;
	*formats = (UINT32*) malloc(sizeof(UINT32) * (count+1));

	*formats[0] = 0;
	for (i = 0; i < count; i++)
	{
		UINT32 format = remmina_rdp_cliprdr_get_format_from_gdkatom(types[i]);
		if (format != 0)
		{
			(*formats)[*size] = format;
			(*size)++;
		}
	}

	*formats = realloc(*formats, sizeof(UINT32) * (*size));
}

static UINT8* lf2crlf(UINT8* data, int* size)
{
        UINT8 c;
        UINT8* outbuf;
        UINT8* out;
        UINT8* in_end;
        UINT8* in;
        int out_size;

        out_size = (*size) * 2 + 1;
        outbuf = (UINT8*) malloc(out_size);
        out = outbuf;
        in = data;
        in_end = data + (*size);

        while (in < in_end)
        {
                c = *in++;
                if (c == '\n')
                {
                        *out++ = '\r';
                        *out++ = '\n';
                }
                else
                {
                        *out++ = c;
                }
        }

        *out++ = 0;
        *size = out - outbuf;

        return outbuf;
}

static void crlf2lf(UINT8* data, size_t* size)
{
        UINT8 c;
        UINT8* out;
        UINT8* in;
        UINT8* in_end;

        out = data;
        in = data;
        in_end = data + (*size);

        while (in < in_end)
        {
                c = *in++;

                if (c != '\r')
                        *out++ = c;
        }

        *size = out - data;
}

int remmina_rdp_cliprdr_server_file_contents_request(CliprdrClientContext* context, CLIPRDR_FILE_CONTENTS_REQUEST* fileContentsRequest)
{
/*	UINT32 uSize = 0;
	BYTE* pData = NULL;
	HRESULT hRet = S_OK;
	FORMATETC vFormatEtc;
	LPDATAOBJECT pDataObj = NULL;
	STGMEDIUM vStgMedium;
	LPSTREAM pStream = NULL;
	BOOL bIsStreamFile = TRUE;
	static LPSTREAM pStreamStc = NULL;
	static UINT32 uStreamIdStc = 0;
	wfClipboard* clipboard = (wfClipboard*) context->custom;
	if (fileContentsRequest->dwFlags == FILECONTENTS_SIZE)
	fileContentsRequest->cbRequested = sizeof(UINT64);
	pData = (BYTE*) calloc(1, fileContentsRequest->cbRequested);
	if (!pData)
	goto error;
	hRet = OleGetClipboard(&pDataObj);
	if (!SUCCEEDED(hRet))
	{
	WLog_ERR(TAG, "filecontents: get ole clipboard failed.");
	goto error;
	}
	ZeroMemory(&vFormatEtc, sizeof(FORMATETC));
	ZeroMemory(&vStgMedium, sizeof(STGMEDIUM));
	vFormatEtc.cfFormat = clipboard->ID_FILECONTENTS;
	vFormatEtc.tymed = TYMED_ISTREAM;
	vFormatEtc.dwAspect = 1;
	vFormatEtc.lindex = fileContentsRequest->listIndex;
	vFormatEtc.ptd = NULL;
	if ((uStreamIdStc != fileContentsRequest->streamId) || !pStreamStc)
	{
	LPENUMFORMATETC pEnumFormatEtc;
	ULONG CeltFetched;
	FORMATETC vFormatEtc2;
	if (pStreamStc)
	{
	IStream_Release(pStreamStc);
	pStreamStc = NULL;
	}
	bIsStreamFile = FALSE;
	hRet = IDataObject_EnumFormatEtc(pDataObj, DATADIR_GET, &pEnumFormatEtc);
	if (hRet == S_OK)
	{
	do
	{
	hRet = IEnumFORMATETC_Next(pEnumFormatEtc, 1, &vFormatEtc2, &CeltFetched);
	if (hRet == S_OK)
	{
	if (vFormatEtc2.cfFormat == clipboard->ID_FILECONTENTS)
	{
	hRet = IDataObject_GetData(pDataObj, &vFormatEtc, &vStgMedium);
	if (hRet == S_OK)
	{
	pStreamStc = vStgMedium.pstm;
	uStreamIdStc = fileContentsRequest->streamId;
	bIsStreamFile = TRUE;
	}
	break;
	}
	}
	}
	while (hRet == S_OK);
	}
	}
	if (bIsStreamFile == TRUE)
	{
	if (fileContentsRequest->dwFlags == FILECONTENTS_SIZE)
	{
	STATSTG vStatStg;
	ZeroMemory(&vStatStg, sizeof(STATSTG));
	hRet = IStream_Stat(pStreamStc, &vStatStg, STATFLAG_NONAME);
	if (hRet == S_OK)
	{
	*((UINT32*) &pData[0]) = vStatStg.cbSize.LowPart;
	*((UINT32*) &pData[4]) = vStatStg.cbSize.HighPart;
	uSize = fileContentsRequest->cbRequested;
	}
	}
	else if (fileContentsRequest->dwFlags == FILECONTENTS_RANGE)
	{
	LARGE_INTEGER dlibMove;
	ULARGE_INTEGER dlibNewPosition;
	dlibMove.HighPart = fileContentsRequest->nPositionHigh;
	dlibMove.LowPart = fileContentsRequest->nPositionLow;
	hRet = IStream_Seek(pStreamStc, dlibMove, STREAM_SEEK_SET, &dlibNewPosition);
	if (SUCCEEDED(hRet))
	{
	hRet = IStream_Read(pStreamStc, pData, fileContentsRequest->cbRequested, (PULONG) &uSize);
	}
	}
	}
	else
	{
	if (fileContentsRequest->dwFlags == FILECONTENTS_SIZE)
	{
	*((UINT32*) &pData[0]) = clipboard->fileDescriptor[fileContentsRequest->listIndex]->nFileSizeLow;
	*((UINT32*) &pData[4]) = clipboard->fileDescriptor[fileContentsRequest->listIndex]->nFileSizeHigh;
	uSize = fileContentsRequest->cbRequested;
	}
	else if (fileContentsRequest->dwFlags == FILECONTENTS_RANGE)
	{
	BOOL bRet;
	bRet = wf_cliprdr_get_file_contents(clipboard->file_names[fileContentsRequest->listIndex], pData,
	fileContentsRequest->nPositionLow, fileContentsRequest->nPositionHigh,
	fileContentsRequest->cbRequested, &uSize);
	if (bRet == FALSE)
	{
	WLog_ERR(TAG, "get file contents failed.");
	uSize = 0;
	goto error;
	}
	}
	}
	IDataObject_Release(pDataObj);
	if (uSize == 0)
	{
	free(pData);
	pData = NULL;
	}
	cliprdr_send_response_filecontents(clipboard, fileContentsRequest->streamId, uSize, pData);
	free(pData);
	return 1;
	error:
	if (pData)
	{
	free(pData);
	pData = NULL;
	}
	if (pDataObj)
	{
	IDataObject_Release(pDataObj);
	pDataObj = NULL;
	}
	WLog_ERR(TAG, "filecontents: send failed response.");
	cliprdr_send_response_filecontents(clipboard, fileContentsRequest->streamId, 0, NULL);
	*/
	return -1;
}
int remmina_rdp_cliprdr_server_file_contents_response(CliprdrClientContext* context, CLIPRDR_FILE_CONTENTS_RESPONSE* fileContentsResponse)
{
	/*
	wfClipboard* clipboard = (wfClipboard*) context->custom;
	clipboard->req_fsize = fileContentsResponse->cbRequested;
	clipboard->req_fdata = (char*) malloc(fileContentsResponse->cbRequested);
	CopyMemory(clipboard->req_fdata, fileContentsResponse->requestedData, fileContentsResponse->cbRequested);
	SetEvent(clipboard->req_fevent);
	*/
	return 1;
}


static int remmina_rdp_cliprdr_monitor_ready(CliprdrClientContext* context, CLIPRDR_MONITOR_READY* monitorReady)
{
	RemminaPluginRdpUiObject* ui;
	rfClipboard* clipboard = (rfClipboard*)context->custom;
	RemminaProtocolWidget* gp;

	gp = clipboard->rfi->protocol_widget;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.clipboard = clipboard;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_MONITORREADY;
	rf_queue_ui(gp, ui);

	return 1;
}

static int remmina_rdp_cliprdr_server_capabilities(CliprdrClientContext* context, CLIPRDR_CAPABILITIES* capabilities)
{
	return 1;
}


static int remmina_rdp_cliprdr_server_format_list(CliprdrClientContext* context, CLIPRDR_FORMAT_LIST* formatList)
{

	/* Called when a user do a "Copy" on the server: we collect all formats
	 * the server send us and then setup the local clipboard with the appropiate
	 * functions to request server data */

	RemminaPluginRdpUiObject* ui;
	RemminaProtocolWidget* gp;
	rfClipboard* clipboard;
	CLIPRDR_FORMAT* format;

	int i;

	clipboard = (rfClipboard*)context->custom;
	gp = clipboard->rfi->protocol_widget;
	GtkTargetList* list = gtk_target_list_new (NULL, 0);

	for (i = 0; i < formatList->numFormats; i++)
	{
		format = &formatList->formats[i];
		if (format->formatId == CF_UNICODETEXT)
		{
			GdkAtom atom = gdk_atom_intern("UTF8_STRING", TRUE);
			gtk_target_list_add(list, atom, 0, CF_UNICODETEXT);
		}
		if (format->formatId == CF_TEXT)
		{
			GdkAtom atom = gdk_atom_intern("TEXT", TRUE);
			gtk_target_list_add(list, atom, 0, CF_TEXT);
		}
		if (format->formatId == CF_DIB)
		{
			GdkAtom atom = gdk_atom_intern("image/bmp", TRUE);
			gtk_target_list_add(list, atom, 0, CF_DIB);
		}
		if (format->formatId == CF_DIBV5)
		{
			GdkAtom atom = gdk_atom_intern("image/bmp", TRUE);
			gtk_target_list_add(list, atom, 0, CF_DIBV5);
		}
		if (format->formatId == CB_FORMAT_JPEG)
		{
			GdkAtom atom = gdk_atom_intern("image/jpeg", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_JPEG);
		}
		if (format->formatId == CB_FORMAT_PNG)
		{
			GdkAtom atom = gdk_atom_intern("image/png", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_PNG);
		}
		if (format->formatId == CB_FORMAT_HTML)
		{
			GdkAtom atom = gdk_atom_intern("text/html", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_HTML);
		}
	}

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.clipboard = clipboard;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_SET_DATA;
	ui->clipboard.targetlist = list;
	rf_queue_ui(gp, ui);

	return 1;
}

static int remmina_rdp_cliprdr_server_format_list_response(CliprdrClientContext* context, CLIPRDR_FORMAT_LIST_RESPONSE* formatListResponse)
{
	return 1;
}


static int remmina_rdp_cliprdr_server_format_data_request(CliprdrClientContext* context, CLIPRDR_FORMAT_DATA_REQUEST* formatDataRequest)
{
// void remmina_rdp_cliprdr_process_data_request(RemminaProtocolWidget* gp, RDP_CB_DATA_REQUEST_EVENT* event)

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
	rf_queue_ui(gp, ui);

	return 1;
}

static int remmina_rdp_cliprdr_server_format_data_response(CliprdrClientContext* context, CLIPRDR_FORMAT_DATA_RESPONSE* formatDataResponse)
{
	// void remmina_rdp_cliprdr_process_data_response(RemminaProtocolWidget* gp, RDP_CB_DATA_RESPONSE_EVENT* event)

	UINT8* data;
	size_t size;
	rfContext* rfi;
	RemminaProtocolWidget* gp;
	rfClipboard* clipboard;
	GdkPixbufLoader *pixbuf;
	gpointer output = NULL;

	clipboard = (rfClipboard*)context->custom;
	gp = clipboard->rfi->protocol_widget;
	rfi = GET_DATA(gp);

	data = formatDataResponse->requestedFormatData;
	size = formatDataResponse->dataLen;

	if (size > 0)
	{
		switch (rfi->clipboard.format)
		{
			case CF_UNICODETEXT:
			{
				size = ConvertFromUnicode(CP_UTF8, 0, (WCHAR*)data, size / 2, (CHAR**)&data, 0, NULL, NULL);
				crlf2lf(data, &size);
				output = data;
				break;
			}

			case CF_TEXT:
			case CB_FORMAT_HTML:
			{
				crlf2lf(data, &size);
				output = data;
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
					if (pbi->biCompression == 3) // BI_BITFIELDS is 3
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
						g_print("rdp_cliprdr: gdk_pixbuf_loader_write() returned error %s\n", perr->message);
				} else {
						if ( !gdk_pixbuf_loader_close(pixbuf, &perr) ) {
								perr = NULL;
								g_print("rdp_cliprdr: gdk_pixbuf_loader_close() returned error %s\n", perr->message);
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
	g_async_queue_push(rfi->clipboard.clipboard_queue, output);

	return 1;
}

void remmina_rdp_cliprdr_request_data(GtkClipboard *gtkClipboard, GtkSelectionData *selection_data, guint info, RemminaProtocolWidget* gp )
{
	/* Called when someone press "Paste" on the client side.
	 * We ask to the server the data we need */

	GdkAtom target;
	gpointer data;
	CLIPRDR_FORMAT_DATA_REQUEST request;
	rfClipboard* clipboard;
	rfContext* rfi;

	rfi = GET_DATA(gp);
	clipboard = &(rfi->clipboard);

	target = gtk_selection_data_get_target(selection_data);
	// clipboard->format = remmina_rdp_cliprdr_get_format_from_gdkatom(target);
	clipboard->format = info;
	clipboard->clipboard_queue = g_async_queue_new();

	/* Request Clipboard data of the server */
	ZeroMemory(&request, sizeof(CLIPRDR_FORMAT_DATA_REQUEST));
	request.requestedFormatId = clipboard->format;
	request.msgFlags = CB_RESPONSE_OK;
	request.msgType = CB_FORMAT_DATA_REQUEST;

	clipboard->context->ClientFormatDataRequest(clipboard->context, &request);

	data = g_async_queue_timeout_pop(clipboard->clipboard_queue, 3000000);
	if (data != NULL)
	{
		if (info == CB_FORMAT_PNG || info == CF_DIB || info == CF_DIBV5 || info == CB_FORMAT_JPEG)
		{
			gtk_selection_data_set_pixbuf(selection_data, data);
			g_object_unref(data);
		}
		else
		{
			gtk_selection_data_set_text(selection_data, data, -1);
		}
	}

	g_async_queue_unref(clipboard->clipboard_queue);
	clipboard->clipboard_queue = NULL;
}

void remmina_rdp_cliprdr_empty_clipboard(GtkClipboard *gtkClipboard, rfClipboard *clipboard)
{
	/* No need to do anything here */
}

int remmina_rdp_cliprdr_send_client_capabilities(rfClipboard* clipboard)
{
	CLIPRDR_CAPABILITIES capabilities;
	CLIPRDR_GENERAL_CAPABILITY_SET generalCapabilitySet;

	capabilities.cCapabilitiesSets = 1;
	capabilities.capabilitySets = (CLIPRDR_CAPABILITY_SET*) &(generalCapabilitySet);

	generalCapabilitySet.capabilitySetType = CB_CAPSTYPE_GENERAL;
	generalCapabilitySet.capabilitySetLength = 12;

	generalCapabilitySet.version = CB_CAPS_VERSION_2;
	generalCapabilitySet.generalFlags = CB_USE_LONG_FORMAT_NAMES;

	clipboard->context->ClientCapabilities(clipboard->context, &capabilities);

	return 1;
}

int remmina_rdp_cliprdr_mt_send_format_list(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	GtkClipboard* gtkClipboard;
	rfClipboard* clipboard = ui->clipboard.clipboard;
	rfContext* rfi = GET_DATA(gp);
	GdkAtom* targets;
	gboolean result = 0;
	gint loccount, srvcount;
	gint formatId, i;
	CLIPRDR_FORMAT_LIST formatList;
	CLIPRDR_FORMAT* formats;

	formatList.formats = formats = NULL;
	formatList.numFormats = 0;

	if (clipboard->clipboard_wait) {
		clipboard->clipboard_wait = FALSE;
		return 0;
	}

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard) {
		result = gtk_clipboard_wait_for_targets(gtkClipboard, &targets, &loccount);
	}

	if (result) {
		formats = (CLIPRDR_FORMAT*)malloc(loccount * sizeof(CLIPRDR_FORMAT));
		srvcount = 0;
		for(i = 0 ; i < loccount ; i++)  {
			formatId = remmina_rdp_cliprdr_get_format_from_gdkatom(targets[i]);
			if ( formatId != 0 ) {
				formats[srvcount].formatId = formatId;
				formats[srvcount].formatName = NULL;
				srvcount ++;
			}
		}
		formats = (CLIPRDR_FORMAT*)realloc(formats, srvcount * sizeof(CLIPRDR_FORMAT));
		g_free(targets);

		formatList.formats = formats;
		formatList.numFormats = srvcount;
	}


	formatList.msgFlags = CB_RESPONSE_OK;
	clipboard->context->ClientFormatList(clipboard->context, &formatList);

	return 1;
}


static void remmina_rdp_cliprdr_send_data_response(rfClipboard* clipboard, BYTE* data, int size)
{
	CLIPRDR_FORMAT_DATA_RESPONSE response;

	ZeroMemory(&response, sizeof(CLIPRDR_FORMAT_DATA_RESPONSE));

	response.msgFlags = CB_RESPONSE_OK;
	response.dataLen = size;
	response.requestedFormatData = data;
	clipboard->context->ClientFormatDataResponse(clipboard->context, &response);
}


int remmina_rdp_cliprdr_mt_monitor_ready(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	rfClipboard *clipboard = ui->clipboard.clipboard;

	if ( clipboard->clipboard_wait )
	{
		clipboard->clipboard_wait = FALSE;
		return 0;
	}

	remmina_rdp_cliprdr_send_client_capabilities(clipboard);
	remmina_rdp_cliprdr_mt_send_format_list(gp,ui);

	clipboard->sync = TRUE;
	return 1;
}


void remmina_rdp_cliprdr_get_clipboard_data(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	GtkClipboard* gtkClipboard;
	rfClipboard* clipboard;

	UINT8* inbuf = NULL;
	UINT8* outbuf = NULL;
	GdkPixbuf *image = NULL;
	int size = 0;


	rfContext* rfi = GET_DATA(gp);
	clipboard = ui->clipboard.clipboard;

	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard)
	{
		switch (ui->clipboard.format)
		{
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
	if (inbuf != NULL || image != NULL)
	{
		switch (ui->clipboard.format)
		{
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
				size = (ConvertToUnicode(CP_UTF8, 0, (CHAR*)inbuf, -1, (WCHAR**)&outbuf, 0) + 1) * 2;
				g_free(inbuf);
				break;
			}
			case CB_FORMAT_PNG:
			{
				gchar* data;
				gsize buffersize;
				gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "png", NULL, NULL);
				outbuf = (UINT8*) malloc(buffersize);
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
				outbuf = (UINT8*) malloc(buffersize);
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
				outbuf = (UINT8*) malloc(size);
				memcpy(outbuf, data + 14, size);
				g_object_unref(image);
				break;
			}
		}
	}

	remmina_rdp_cliprdr_send_data_response(clipboard, outbuf, size);
}

void remmina_rdp_cliprdr_set_clipboard_data(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	GtkClipboard* gtkClipboard;
	GtkTargetEntry* targets;
	gint n_targets;
	rfContext* rfi = GET_DATA(gp);
	rfClipboard* clipboard;

	clipboard = ui->clipboard.clipboard;

	targets = gtk_target_table_new_from_list(ui->clipboard.targetlist, &n_targets);
	gtkClipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (gtkClipboard && targets)
	{
		clipboard->clipboard_wait = TRUE;
		gtk_clipboard_set_with_owner(gtkClipboard, targets, n_targets,
				(GtkClipboardGetFunc) remmina_rdp_cliprdr_request_data,
				(GtkClipboardClearFunc) remmina_rdp_cliprdr_empty_clipboard, G_OBJECT(gp));
		gtk_target_table_free(targets, n_targets);
	}
}

void remmina_rdp_event_process_clipboard(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	switch (ui->clipboard.type)
	{

		case REMMINA_RDP_UI_CLIPBOARD_FORMATLIST:
			remmina_rdp_cliprdr_mt_send_format_list(gp, ui);
			break;
		case REMMINA_RDP_UI_CLIPBOARD_MONITORREADY:
			remmina_rdp_cliprdr_mt_monitor_ready(gp, ui);
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
	// Future: initialize rfi->clipboard
}
void remmina_rdp_clipboard_free(rfContext *rfi)
{
	// Future: deinitialize rfi->clipboard
}


void remmina_rdp_cliprdr_init(rfContext* rfi, CliprdrClientContext* cliprdr)
{
	rfClipboard* clipboard;
	clipboard = &(rfi->clipboard);

	rfi->clipboard.rfi = rfi;
	cliprdr->custom = (void*)clipboard;

	clipboard->context = cliprdr;

	cliprdr->MonitorReady = remmina_rdp_cliprdr_monitor_ready;
	cliprdr->ServerCapabilities = remmina_rdp_cliprdr_server_capabilities;
	cliprdr->ServerFormatList = remmina_rdp_cliprdr_server_format_list;
	cliprdr->ServerFormatListResponse = remmina_rdp_cliprdr_server_format_list_response;
	cliprdr->ServerFormatDataRequest = remmina_rdp_cliprdr_server_format_data_request;
	cliprdr->ServerFormatDataResponse = remmina_rdp_cliprdr_server_format_data_response;

//	cliprdr->ServerFileContentsRequest = remmina_rdp_cliprdr_server_file_contents_request;
//	cliprdr->ServerFileContentsResponse = remmina_rdp_cliprdr_server_file_contents_response;

}

