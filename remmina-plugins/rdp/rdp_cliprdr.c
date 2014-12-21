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
#include <freerdp/utils/event.h>
#include <freerdp/channels/channels.h>
#include <freerdp/client/cliprdr.h>

#pragma pack(push,1)
typedef unsigned int wDWORD;
typedef unsigned short wWORD;
typedef int wLONG;
typedef unsigned char wBYTE;
typedef wLONG FXPT2DOT30;
typedef struct tagCIEXYZ
{
        FXPT2DOT30 ciexyzX;
        FXPT2DOT30 ciexyzY;
        FXPT2DOT30 ciexyzZ;
} CIEXYZ;

typedef struct tagICEXYZTRIPLE
{
        CIEXYZ ciexyzRed;
        CIEXYZ ciexyzGreen;
        CIEXYZ ciexyzBlue;
} CIEXYZTRIPLE;
typedef struct
{
        wDWORD bV5Size;
        wLONG bV5Width;
        wLONG bV5Height;
        wWORD bV5Planes;
        wWORD bV5BitCount;
        wDWORD bV5Compression;
        wDWORD bV5SizeImage;
        wLONG bV5XPelsPerMeter;
        wLONG bV5YPelsPerMeter;
        wDWORD bV5ClrUsed;
        wDWORD bV5ClrImportant;
        wDWORD bV5RedMask;
        wDWORD bV5GreenMask;
        wDWORD bV5BlueMask;
        wDWORD bV5AlphaMask;
        wDWORD bV5CSType;
        CIEXYZTRIPLE bV5Endpoints;
        wDWORD bV5GammaRed;
        wDWORD bV5GammaGreen;
        wDWORD bV5GammaBlue;
        wDWORD bV5Intent;
        wDWORD bV5ProfileData;
        wDWORD bV5ProfileSize;
        wDWORD bV5Reserved;
} BITMAPV5HEADER;
typedef struct tagRGBQUAD
{
        wBYTE rgbBlue;
        wBYTE rgbGreen;
        wBYTE rgbRed;
        wBYTE rgbReserved;
} RGBQUAD;
typedef struct tagBITMAPINFOHEADER
{
        wDWORD biSize;
        wLONG biWidth;
        wLONG biHeight;
        wWORD biPlanes;
        wWORD biBitCount;
        wDWORD biCompression;
        wDWORD biSizeImage;
        wLONG biXPelsPerMeter;
        wLONG biYPelsPerMeter;
        wDWORD biClrUsed;
        wDWORD biClrImportant;
} BITMAPINFOHEADER;
#pragma pack(pop)


UINT32 remmina_rdp_cliprdr_get_format_from_gdkatom(GdkAtom atom)
{
	gchar* name = gdk_atom_name(atom);
	if (g_strcmp0("UTF8_STRING", name) == 0 || g_strcmp0("text/plain;charset=utf-8", name) == 0)
	{
		return CB_FORMAT_UNICODETEXT;
	}
	if (g_strcmp0("TEXT", name) == 0 || g_strcmp0("text/plain", name) == 0)
	{
		return CB_FORMAT_TEXT;
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
		return CB_FORMAT_DIB;
	}
	return CB_FORMAT_RAW;
}

void remmina_rdp_cliprdr_get_target_types(UINT32** formats, UINT16* size, GdkAtom* types, int count)
{
	int i;
	*size = 1;
	*formats = (UINT32*) malloc(sizeof(UINT32) * (count+1));

	*formats[0] = CB_FORMAT_RAW;
	for (i = 0; i < count; i++)
	{
		UINT32 format = remmina_rdp_cliprdr_get_format_from_gdkatom(types[i]);
		if (format != CB_FORMAT_RAW)
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

void remmina_rdp_cliprdr_process_monitor_ready(RemminaProtocolWidget* gp, RDP_CB_MONITOR_READY_EVENT* event)
{
	RemminaPluginRdpUiObject* ui;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_FORMATLIST;
	rf_queue_ui(gp, ui);
}

void remmina_rdp_cliprdr_process_format_list(RemminaProtocolWidget* gp, RDP_CB_FORMAT_LIST_EVENT* event)
{
	RemminaPluginRdpUiObject* ui;
	int i;
	GtkTargetList* list = gtk_target_list_new (NULL, 0);

	for (i = 0; i < event->num_formats; i++)
	{
		if (event->formats[i] == CB_FORMAT_UNICODETEXT)
		{
			GdkAtom atom = gdk_atom_intern("UTF8_STRING", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_UNICODETEXT);
		}
		if (event->formats[i] == CB_FORMAT_TEXT)
		{
			GdkAtom atom = gdk_atom_intern("TEXT", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_TEXT);
		}
		if (event->formats[i] == CB_FORMAT_DIB)
		{
			GdkAtom atom = gdk_atom_intern("image/bmp", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_DIB);
		}
		if (event->formats[i] == CB_FORMAT_JPEG)
		{
			GdkAtom atom = gdk_atom_intern("image/jpeg", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_JPEG);
		}
		/* it seems that in freerdp 1.1 PNG images are not correctly
		 * received from server. So just disable it.
		if (event->formats[i] == CB_FORMAT_PNG)
		{
			GdkAtom atom = gdk_atom_intern("image/png", TRUE);
			gtk_target_list_add(list, atom, 0, CB_FORMAT_PNG);
		}
		*/
	}

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_SET_DATA;
	ui->clipboard.targetlist = list;
	rf_queue_ui(gp, ui);
}

void remmina_rdp_cliprdr_process_data_request(RemminaProtocolWidget* gp, RDP_CB_DATA_REQUEST_EVENT* event)
{
	RemminaPluginRdpUiObject* ui;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CLIPBOARD;
	ui->clipboard.type = REMMINA_RDP_UI_CLIPBOARD_GET_DATA;
	ui->clipboard.format = event->format;
	rf_queue_ui(gp, ui);
}

void remmina_rdp_cliprdr_process_data_response(RemminaProtocolWidget* gp, RDP_CB_DATA_RESPONSE_EVENT* event)
{
	UINT8* data;
	size_t size;
	rfContext* rfi = GET_DATA(gp);
	GdkPixbufLoader *pixbuf;
	gpointer output = NULL;

	data = event->data;
	size = event->size;

	if (size > 0)
	{
		switch (rfi->format)
		{
			case CB_FORMAT_UNICODETEXT:
			{
				size = ConvertFromUnicode(CP_UTF8, 0, (WCHAR*)data, size / 2, (CHAR**)&output, 0, NULL, NULL);
				crlf2lf(output, &size);
				break;
			}

			case CB_FORMAT_TEXT:
			case CB_FORMAT_HTML:
			{
				output = (gpointer)calloc(1, size + 1);
				if (output) {
					memcpy(output, data, size);
					crlf2lf(output, &size);
				}
				break;
			}

			case CB_FORMAT_DIB:
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
					remmina_plugin_service->log_printf("[RDP] rdp_cliprdr: gdk_pixbuf_loader_write() returned error %s\n", perr->message);
				}
				else
				{
					if ( !gdk_pixbuf_loader_close(pixbuf, &perr) ) {
						perr = NULL;
						remmina_plugin_service->log_printf("[RDP] rdp_cliprdr: gdk_pixbuf_loader_close() returned error %s\n", perr->message);
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

	if (rfi->clipboard_queue) {
		g_async_queue_push(rfi->clipboard_queue, output);
	}

}

void remmina_rdp_channel_cliprdr_process(RemminaProtocolWidget* gp, wMessage* event)
{
	switch (GetMessageType(event->id))
	{
		case CliprdrChannel_MonitorReady:
			remmina_rdp_cliprdr_process_monitor_ready(gp, (RDP_CB_MONITOR_READY_EVENT*) event);
			break;

		case CliprdrChannel_FormatList:
			remmina_rdp_cliprdr_process_format_list(gp, (RDP_CB_FORMAT_LIST_EVENT*) event);
			break;

		case CliprdrChannel_DataRequest:
			remmina_rdp_cliprdr_process_data_request(gp, (RDP_CB_DATA_REQUEST_EVENT*) event);
			break;

		case CliprdrChannel_DataResponse:
			remmina_rdp_cliprdr_process_data_response(gp, (RDP_CB_DATA_RESPONSE_EVENT*) event);
			break;
	}
}

void remmina_rdp_cliprdr_request_data(GtkClipboard *clipboard, GtkSelectionData *selection_data, guint info, RemminaProtocolWidget* gp)
{
	GdkAtom target;
	gpointer data;
	RDP_CB_DATA_REQUEST_EVENT* event;
	rfContext* rfi = GET_DATA(gp);
	GAsyncQueue* q;

	if (rfi->clipboard_queue != NULL)
		return;

	target = gtk_selection_data_get_target(selection_data);
	rfi->format = remmina_rdp_cliprdr_get_format_from_gdkatom(target);
	rfi->clipboard_queue = g_async_queue_new();

	/* Request Clipboard data of the server */
	event = (RDP_CB_DATA_REQUEST_EVENT*)
		freerdp_event_new(CliprdrChannel_Class, CliprdrChannel_DataRequest, NULL, NULL);
	event->format = rfi->format;
	freerdp_channels_send_event(rfi->instance->context->channels, (wMessage*) event);

	data = g_async_queue_timeout_pop(rfi->clipboard_queue, 2000000);
	if (data != NULL)
	{
		if (info == CB_FORMAT_PNG || info == CB_FORMAT_DIB || info == CB_FORMAT_JPEG)
		{
			gtk_selection_data_set_pixbuf(selection_data, data);
			g_object_unref(data);
		}
		else
		{
			gtk_selection_data_set_text(selection_data, data, -1);
			free(data);
		}
	}

	q = rfi->clipboard_queue;
	rfi->clipboard_queue = NULL;
	g_async_queue_unref(q);

}

void remmina_rdp_cliprdr_empty_clipboard(GtkClipboard *clipboard, RemminaProtocolWidget* gp)
{
	/* No need to do anything here */
}

int remmina_rdp_cliprdr_send_format_list(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	GtkClipboard* clipboard;
	GdkAtom* targets;
	gboolean result = 0;
	gint count;
	RDP_CB_FORMAT_LIST_EVENT* event;

	rfContext* rfi = GET_DATA(gp);

	if (rfi->clipboard_wait)
	{
		rfi->clipboard_wait = FALSE;
		return 0;
	}

	/* Lets see if we have something in our clipboard */
	clipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (clipboard)
	{
		result = gtk_clipboard_wait_for_targets(clipboard, &targets, &count);
	}


	event = (RDP_CB_FORMAT_LIST_EVENT*)
		freerdp_event_new(CliprdrChannel_Class, CliprdrChannel_FormatList, NULL, NULL);

	if (result)
	{
		remmina_rdp_cliprdr_get_target_types(&event->formats, &event->num_formats, targets, count);
		g_free(targets);
	}
	else
		event->num_formats = 0;


	return freerdp_channels_send_event(rfi->instance->context->channels, (wMessage*) event);
}

void remmina_rdp_cliprdr_get_clipboard_data(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	RDP_CB_DATA_RESPONSE_EVENT* event;
	GtkClipboard* clipboard;
	UINT8* inbuf = NULL;
	UINT8* outbuf = NULL;
	GdkPixbuf *image = NULL;
	int size = 0;

	rfContext* rfi = GET_DATA(gp);

	clipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (clipboard)
	{
		switch (ui->clipboard.format)
		{
			case CB_FORMAT_TEXT:
			case CB_FORMAT_UNICODETEXT:
			case CB_FORMAT_HTML:
			{
				inbuf = (UINT8*)gtk_clipboard_wait_for_text(clipboard);
				break;
			}

			case CB_FORMAT_PNG:
			case CB_FORMAT_JPEG:
			case CB_FORMAT_DIB:
			{
				image = gtk_clipboard_wait_for_image(clipboard);
				break;
			}
		}
	}

	/* No data received, send nothing */
	if (inbuf != NULL || image != NULL)
	{
		switch (ui->clipboard.format)
		{
			case CB_FORMAT_TEXT:
			case CB_FORMAT_HTML:
			{
				size = strlen((char*)inbuf);
				outbuf = lf2crlf(inbuf, &size);
				break;
			}
			case CB_FORMAT_UNICODETEXT:
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
			case CB_FORMAT_DIB:
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
	event = (RDP_CB_DATA_RESPONSE_EVENT*)
		        freerdp_event_new(CliprdrChannel_Class, CliprdrChannel_DataResponse, NULL, NULL);
	event->data = outbuf;
	event->size = size;
	freerdp_channels_send_event(rfi->instance->context->channels, (wMessage*) event);
}

void remmina_rdp_cliprdr_set_clipboard_data(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* ui)
{
	GtkClipboard* clipboard;
	GtkTargetEntry* targets;
	gint n_targets;
	rfContext* rfi = GET_DATA(gp);

	targets = gtk_target_table_new_from_list(ui->clipboard.targetlist, &n_targets);
	clipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (clipboard && targets)
	{
		rfi->clipboard_wait = TRUE;
		gtk_clipboard_set_with_owner(clipboard, targets, n_targets,
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
			remmina_rdp_cliprdr_send_format_list(gp, ui);
			break;

		case REMMINA_RDP_UI_CLIPBOARD_GET_DATA:
			remmina_rdp_cliprdr_get_clipboard_data(gp, ui);
			break;

		case REMMINA_RDP_UI_CLIPBOARD_SET_DATA:
			remmina_rdp_cliprdr_set_clipboard_data(gp, ui);
			break;
	}
}
