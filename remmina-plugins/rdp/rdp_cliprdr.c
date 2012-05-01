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
 */

#include "rdp_plugin.h"
#include "rdp_cliprdr.h"

#include <freerdp/freerdp.h>
#include <freerdp/utils/memory.h>
#include <freerdp/channels/channels.h>
#include <freerdp/plugins/cliprdr.h>

/*
 * Get the formats we can export based on the current clipboard data.
 */
void remmina_rdp_cliprdr_get_target_types(uint32** dst_formats, uint16* size, GdkAtom* types, int count)
{
	int i;
	gboolean image = FALSE;
	gboolean text = FALSE;
	gboolean textutf8 = FALSE;
	int matches = 1;
	uint32* formats = (uint32*) xmalloc(sizeof(uint32) * 10);

	formats[0] = CB_FORMAT_RAW;	
	for (i = 0; i < count; i++)
	{	
		GdkAtom atom = GDK_POINTER_TO_ATOM(types[i]);
		gchar* name = gdk_atom_name(atom);
		if (g_strcmp0("UTF8_STRING", name) == 0 || g_strcmp0("text/plain;charset=utf-8", name) == 0)
		{
			textutf8 = TRUE;
		}
		if (g_strcmp0("TEXT", name) == 0 || g_strcmp0("text/plain", name) == 0)
		{
			text = TRUE;
		}
		if (g_strcmp0("text/html", name) == 0)
		{
			formats[matches] = CB_FORMAT_HTML;
			matches++;
		}
		if (g_strcmp0("image/png", name) == 0)
		{
			formats[matches] = CB_FORMAT_PNG;
			image = TRUE;
			matches++;
		}
		if (g_strcmp0("image/jpeg", name) == 0)
		{
			formats[matches] = CB_FORMAT_JPEG;
			image = TRUE;
			matches++;
		}
		if (g_strcmp0("image/bmp", name) == 0)
		{
			formats[matches] = CB_FORMAT_DIB;
			image = TRUE;
			matches++;
		}
	}
	//Only add text formats if we don't have image formats
	if (!image)
	{
		if (textutf8)
		{
			formats[matches] = CB_FORMAT_UNICODETEXT;
			matches++;
		}
		if (text)
		{
			formats[matches] = CB_FORMAT_TEXT;
			matches++;
		}
	}

	*size = (uint16)matches;
	*dst_formats = (uint32*) xmalloc(sizeof(uint32) * matches);
	memcpy(*dst_formats, formats, sizeof(uint32) * matches);
	g_free(formats);
}

int remmina_rdp_cliprdr_send_format_list_event(RemminaProtocolWidget* gp)
{
	GtkClipboard* clipboard;
	GdkAtom* targets;
	gboolean result = 0;
	gint count;
	RDP_EVENT* rdp_event;
	RDP_CB_FORMAT_LIST_EVENT* format_list_event;
	rfContext* rfi = GET_DATA(gp);

	/* Lets see if we have something in our clipboard */
	THREADS_ENTER
	clipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (clipboard)
	{
		result = gtk_clipboard_wait_for_targets(clipboard, &targets, &count);
	}
	THREADS_LEAVE
	
	if (!result)
		return 1;

	int i;
	for (i = 0; i < count; i++)
	{
		g_printf("Target %d: %s\n", i, gdk_atom_name(targets[i]));
	}

	rdp_event = (RDP_EVENT*) xnew(RDP_CB_FORMAT_LIST_EVENT);
	rdp_event->event_class = RDP_EVENT_CLASS_CLIPRDR;
	rdp_event->event_type = RDP_EVENT_TYPE_CB_FORMAT_LIST;
	format_list_event = (RDP_CB_FORMAT_LIST_EVENT*) rdp_event;
	
	remmina_rdp_cliprdr_get_target_types(&format_list_event->formats, &format_list_event->num_formats, targets, count);
	g_free(targets);

	int num_formats = format_list_event->num_formats;
	g_printf("Sending %d formats\n", num_formats);
	for (i = 0; i < num_formats; i++)
	{
		g_printf("Sending format %#X\n", format_list_event->formats[i]);
	}
	
	return freerdp_channels_send_event(rfi->channels, (RDP_EVENT*) format_list_event);
}

static uint8* lf2crlf(uint8* data, int* size)
{
        uint8 c;
        uint8* outbuf;
        uint8* out;
        uint8* in_end;
        uint8* in;
        int out_size;

        out_size = (*size) * 2 + 1;
        outbuf = (uint8*) xmalloc(out_size);
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

static void crlf2lf(uint8* data, int* size)
{
        uint8 c;
        uint8* out;
        uint8* in;
        uint8* in_end;

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

uint8* remmina_rdp_cliprdr_get_data(RemminaProtocolWidget* gp, uint32 format, int* size)
{
	g_printf("GetData: Requested Format: %#X\n", format);
	rfContext* rfi = GET_DATA(gp);
	GtkClipboard* clipboard;
	uint8* inbuf = NULL;
	uint8* outbuf = NULL;
	GdkPixbuf *image = NULL;
	
	THREADS_ENTER
	clipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (clipboard)
	{
		if (format == CB_FORMAT_TEXT || format == CB_FORMAT_UNICODETEXT || format == CB_FORMAT_HTML)
		{
			inbuf = (uint8*)gtk_clipboard_wait_for_text(clipboard);
		}
		if (format == CB_FORMAT_PNG || format == CB_FORMAT_JPEG || format == CB_FORMAT_DIB)
		{
			image = gtk_clipboard_wait_for_image(clipboard);
		}
	}
	THREADS_LEAVE

	if (format == CB_FORMAT_TEXT || format == CB_FORMAT_HTML || format == CB_FORMAT_UNICODETEXT)
	{
		inbuf = lf2crlf(inbuf, size);
		if (format == CB_FORMAT_TEXT)
		{
			outbuf = inbuf;
		}
		if (format == CB_FORMAT_HTML)
		{
			//TODO: check if we need special handling for HTML
			outbuf = inbuf;
		}
		if (format == CB_FORMAT_UNICODETEXT)
		{
			size_t out_size;
			UNICONV* uniconv;
			
			uniconv = freerdp_uniconv_new();
			outbuf = (uint8*) freerdp_uniconv_out(uniconv, (char*) inbuf, &out_size);
			freerdp_uniconv_free(uniconv);
			*size = out_size + 2;
		}
	}
	if (format == CB_FORMAT_PNG || format == CB_FORMAT_JPEG || format == CB_FORMAT_DIB)
	{
		gchar* data;
		gsize buffersize;
		if (format == CB_FORMAT_PNG)
		{
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "png", NULL, NULL);
			memcpy(outbuf, data, buffersize);
		}
		if (format == CB_FORMAT_JPEG)
		{
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "jpeg", NULL, NULL);
			memcpy(outbuf, data, buffersize);
		}
		if (format == CB_FORMAT_DIB)
		{
			gdk_pixbuf_save_to_buffer(image, &data, &buffersize, "bmp", NULL, NULL);
			*size = buffersize - 14;
			g_printf("Size of pixels: %d\n", *size);
			outbuf = (uint8*) xmalloc(*size);
			memcpy(outbuf, data + 14, *size);
		}
	}

	if (inbuf)
		g_free(inbuf);
	if (G_IS_OBJECT(image))
		g_object_unref(image);
	
	if (!outbuf)
		outbuf = (uint8*)"";

	return outbuf;
}

void remmina_rdp_cliprdr_parse_response_event(RemminaProtocolWidget* gp, RDP_EVENT* event)
{
	g_printf("Received RDP_EVENT_TYPE_CB_DATA_RESPONSE\n");

	GtkClipboard* clipboard;
	GdkPixbuf *image = NULL;
	uint8* data;
	int size;
	gboolean text = FALSE;
	gboolean img = FALSE;
	rfContext* rfi = GET_DATA(gp);
	RDP_CB_DATA_RESPONSE_EVENT* data_response_event;
	GdkPixbufLoader *pixbuf;
	data_response_event = (RDP_CB_DATA_RESPONSE_EVENT*) event;
	data = data_response_event->data;
	size = data_response_event->size;

	g_printf("Requested format was: 0x%x\n", rfi->requested_format);
	
	if (rfi->requested_format == CB_FORMAT_TEXT || rfi->requested_format == CB_FORMAT_UNICODETEXT || rfi->requested_format == CB_FORMAT_HTML)
	{
		if (rfi->requested_format == CB_FORMAT_UNICODETEXT)
		{
			UNICONV* uniconv;

			uniconv = freerdp_uniconv_new();
			data = (uint8*) freerdp_uniconv_in(uniconv, data, size);
			size = strlen((char*) data);
			freerdp_uniconv_free(uniconv);
		}
		crlf2lf(data, &size);
		text = TRUE;
	}
	if (rfi->requested_format == CB_FORMAT_DIB || rfi->requested_format == CB_FORMAT_PNG || rfi->requested_format == CB_FORMAT_JPEG)
	{
		/* Reconstruct header */
		if (rfi->requested_format == CB_FORMAT_DIB)
		{
			STREAM* s;
			uint16 bpp;
			uint32 offset;
			uint32 ncolors;

			s = stream_new(0);
			stream_attach(s, data, size);
			stream_seek(s, 14);
			stream_read_uint16(s, bpp);
			stream_read_uint32(s, ncolors);
			offset = 14 + 40 + (bpp <= 8 ? (ncolors == 0 ? (1 << bpp) : ncolors) * 4 : 0);
			stream_detach(s);
			stream_free(s);

			s = stream_new(14 + size);
			stream_write_uint8(s, 'B');
			stream_write_uint8(s, 'M');
			stream_write_uint32(s, 14 + size);
			stream_write_uint32(s, 0);
			stream_write_uint32(s, offset);
			stream_write(s, data, size);

			data = stream_get_head(s);
			size = stream_get_length(s);
			stream_detach(s);
			stream_free(s);
		}
		pixbuf = gdk_pixbuf_loader_new();
		gdk_pixbuf_loader_write(pixbuf, data, size, NULL);
		image = gdk_pixbuf_loader_get_pixbuf(pixbuf);
		img = TRUE;
	}

	THREADS_ENTER
	clipboard = gtk_widget_get_clipboard(rfi->drawing_area, GDK_SELECTION_CLIPBOARD);
	if (clipboard)
	{
		if (text || img)
			rfi->clipboard_wait = TRUE;
		if (text)
		{
			gtk_clipboard_set_text(clipboard, (gchar*)data, size);
			gtk_clipboard_store(clipboard);
		}
		if (img)
		{
			gtk_clipboard_set_image(clipboard, image);
			gtk_clipboard_store(clipboard);
			gdk_pixbuf_loader_close(pixbuf, NULL);
			g_object_unref(pixbuf);
		}
	}
	THREADS_LEAVE
	
}

void remmina_handle_channel_event(RemminaProtocolWidget* gp, RDP_EVENT* event)
{
	RDP_EVENT* rdp_event = NULL;
	rfContext* rfi = GET_DATA(gp);

	switch (event->event_class)
	{
		case RDP_EVENT_CLASS_CLIPRDR:
			g_printf("Event ID: %d\n", event->event_type);
			if (event->event_type == RDP_EVENT_TYPE_CB_MONITOR_READY)
			{
				g_printf("Received CB_MONITOR_READY - Sending RDP_EVENT_TYPE_CB_FORMAT_LIST\n");
				/* Sending our format list */
				remmina_rdp_cliprdr_send_format_list_event(gp);
			}
			if (event->event_type == RDP_EVENT_TYPE_CB_FORMAT_LIST)
			{
				/* We received a FORMAT_LIST from the server, update our clipboard */
				g_printf("Received RDP_EVENT_TYPE_CB_FORMAT_LIST\n");
				int i;	
				uint32 format = CB_FORMAT_RAW;	
				RDP_CB_FORMAT_LIST_EVENT* format_list_event;

				format_list_event = (RDP_CB_FORMAT_LIST_EVENT*) event;

				g_printf("Format List Size: %d\n", format_list_event->num_formats);
				for (i = 0; i < format_list_event->num_formats; i++)
				{
					g_printf("Format: 0x%X\n", format_list_event->formats[i]);
					if (format_list_event->formats[i] == CB_FORMAT_UNICODETEXT)
					{
						format = CB_FORMAT_UNICODETEXT;
						break;
					}
					if (format_list_event->formats[i] == CB_FORMAT_DIB)
					{
						format = CB_FORMAT_DIB;
						break;
					}
					if (format_list_event->formats[i] == CB_FORMAT_JPEG)
					{
						format = CB_FORMAT_JPEG;
						break;
					}
					if (format_list_event->formats[i] == CB_FORMAT_PNG)
					{
						format = CB_FORMAT_PNG;
						break;
					}
					if (format_list_event->formats[i] == CB_FORMAT_TEXT)
					{
						format = CB_FORMAT_TEXT;
						break;
					}				
				}
				rfi->requested_format = format;
				
				g_printf("Format Requested: 0x%X\n", format);
				/* Request Clipboard data of the server */
				RDP_CB_DATA_REQUEST_EVENT* data_request_event;
				rdp_event = (RDP_EVENT*) xnew(RDP_CB_DATA_REQUEST_EVENT);
				rdp_event->event_class = RDP_EVENT_CLASS_CLIPRDR;
				rdp_event->event_type = RDP_EVENT_TYPE_CB_DATA_REQUEST;
				data_request_event = (RDP_CB_DATA_REQUEST_EVENT*) rdp_event;
				data_request_event->format = format;
				freerdp_channels_send_event(rfi->channels, (RDP_EVENT*) data_request_event);
			}
			if (event->event_type == RDP_EVENT_TYPE_CB_DATA_REQUEST)
			{	
				g_printf("Received RDP_EVENT_TYPE_CB_DATA_REQUEST\n");
				
				uint8* data;
				int size;
				RDP_CB_DATA_REQUEST_EVENT* data_request_event = (RDP_CB_DATA_REQUEST_EVENT*) event;
				RDP_CB_DATA_RESPONSE_EVENT* data_response_event;

				g_printf("Event Format: %d\n", data_request_event->format);

				/* Send Data */
				rdp_event = (RDP_EVENT*) xnew(RDP_CB_DATA_RESPONSE_EVENT);
				rdp_event->event_class = RDP_EVENT_CLASS_CLIPRDR;
				rdp_event->event_type = RDP_EVENT_TYPE_CB_DATA_RESPONSE;
				data_response_event = (RDP_CB_DATA_RESPONSE_EVENT*) rdp_event;
				data = remmina_rdp_cliprdr_get_data(gp, data_request_event->format, &size);
				data_response_event->data = data;
				data_response_event->size = size;
				freerdp_channels_send_event(rfi->channels, rdp_event);
			}
			if (event->event_type == RDP_EVENT_TYPE_CB_DATA_RESPONSE)
			{
				remmina_rdp_cliprdr_parse_response_event(gp, event);
			}
	}
}
