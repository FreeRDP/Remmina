/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

#ifndef __REMMINA_RDP_H__
#define __REMMINA_RDP_H__

#include "common/remmina_plugin.h"
#include <freerdp/freerdp.h>
#include <freerdp/channels/channels.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/gdi/dc.h>
#include <freerdp/gdi/region.h>
#include <freerdp/client/cliprdr.h>
#include <gdk/gdkx.h>

#include <winpr/clipboard.h>

typedef struct rf_context rfContext;

#define GET_PLUGIN_DATA(gp) (rfContext*) g_object_get_data(G_OBJECT(gp), "plugin-data")

#define DEFAULT_QUALITY_0	0x6f
#define DEFAULT_QUALITY_1	0x07
#define DEFAULT_QUALITY_2	0x01
#define DEFAULT_QUALITY_9	0x80

extern RemminaPluginService* remmina_plugin_service;

struct rf_clipboard
{
	rfContext* rfi;
	CliprdrClientContext* context;
	BOOL sync;
	wClipboard* system;
	int requestedFormatId;

	gboolean clipboard_wait;
	UINT32 format;
	gulong clipboard_handler;


	pthread_mutex_t transfer_clip_mutex;
	pthread_cond_t transfer_clip_cond;
	enum  { SCDW_NONE, SCDW_BUSY_WAIT, SCDW_ASYNCWAIT } srv_clip_data_wait ;
	gpointer srv_data;

};
typedef struct rf_clipboard rfClipboard;


struct rf_pointer
{
	rdpPointer pointer;
	GdkCursor* cursor;
};
typedef struct rf_pointer rfPointer;

struct rf_bitmap
{
	rdpBitmap bitmap;
	Pixmap pixmap;
	cairo_surface_t* surface;
};
typedef struct rf_bitmap rfBitmap;

struct rf_glyph
{
	rdpGlyph glyph;
	Pixmap pixmap;
};
typedef struct rf_glyph rfGlyph;


typedef enum
{
	REMMINA_RDP_EVENT_TYPE_SCANCODE,
	REMMINA_RDP_EVENT_TYPE_MOUSE
} RemminaPluginRdpEventType;

struct remmina_plugin_rdp_event
{
	RemminaPluginRdpEventType type;
	union
	{
		struct
		{
			BOOL up;
			BOOL extended;
			UINT8 key_code;
		} key_event;
		struct
		{
			UINT16 flags;
			UINT16 x;
			UINT16 y;
			BOOL extended;
		} mouse_event;
	};
};
typedef struct remmina_plugin_rdp_event RemminaPluginRdpEvent;

typedef enum
{
	REMMINA_RDP_UI_UPDATE_REGION = 0,
	REMMINA_RDP_UI_CONNECTED,
	REMMINA_RDP_UI_RECONNECT_PROGRESS,
	REMMINA_RDP_UI_CURSOR,
	REMMINA_RDP_UI_RFX,
	REMMINA_RDP_UI_NOCODEC,
	REMMINA_RDP_UI_CLIPBOARD,
	REMMINA_RDP_UI_EVENT
} RemminaPluginRdpUiType;

typedef enum
{
	REMMINA_RDP_UI_CLIPBOARD_MONITORREADY,
	REMMINA_RDP_UI_CLIPBOARD_FORMATLIST,
	REMMINA_RDP_UI_CLIPBOARD_GET_DATA,
	REMMINA_RDP_UI_CLIPBOARD_SET_DATA,
	REMMINA_RDP_UI_CLIPBOARD_SET_CONTENT,
	REMMINA_RDP_UI_CLIPBOARD_DETACH_OWNER
} RemminaPluginRdpUiClipboardType;

typedef enum
{
	REMMINA_RDP_POINTER_NEW,
	REMMINA_RDP_POINTER_FREE,
	REMMINA_RDP_POINTER_SET,
	REMMINA_RDP_POINTER_NULL,
	REMMINA_RDP_POINTER_DEFAULT,
	REMMINA_RDP_POINTER_SETPOS
} RemminaPluginRdpUiPointerType;

typedef enum
{
	REMMINA_RDP_UI_EVENT_UPDATE_SCALE
} RemminaPluginRdpUiEeventType;

struct remmina_plugin_rdp_ui_object
{
	RemminaPluginRdpUiType type;
	gboolean sync;
	gboolean complete;
	pthread_mutex_t sync_wait_mutex;
	pthread_cond_t sync_wait_cond;
	union
	{
		struct
		{
			gint x;
			gint y;
			gint width;
			gint height;
		} region;
		struct
		{
			rdpContext* context;
			rfPointer* pointer;
			RemminaPluginRdpUiPointerType type;
		} cursor;
		struct
		{
			gint left;
			gint top;
			RFX_MESSAGE* message;
		} rfx;
		struct
		{
			gint left;
			gint top;
			gint width;
			gint height;
			UINT8* bitmap;
		} nocodec;
		struct
		{
			RemminaPluginRdpUiClipboardType type;
			GtkTargetList* targetlist;
			UINT32 format;
			rfClipboard* clipboard;
			gpointer data;
		} clipboard;
		struct {
			RemminaPluginRdpUiEeventType type;
		} event;
		struct {
			gint x;
			gint y;
		} pos;
	};
	/* We can also return values here, only integer, and -1 is reserved for rf_queue_ui own error */
	int retval;
};

struct rf_context
{
	rdpContext _p;

	RemminaProtocolWidget* protocol_widget;

	/* main */
	rdpSettings* settings;
	freerdp* instance;

	pthread_t thread;
	gboolean scale;
	gboolean user_cancelled;
	gboolean thread_cancelled;

	CliprdrClientContext* cliprdr;

	RDP_PLUGIN_DATA rdpdr_data[5];
	RDP_PLUGIN_DATA drdynvc_data[5];
	gchar rdpsnd_options[20];

	RFX_CONTEXT* rfx_context;

	gboolean connected;
	gboolean is_reconnecting;
	int reconnect_maxattempts;
	int reconnect_nattempt;

	gboolean sw_gdi;
	GtkWidget* drawing_area;
	gint scale_width;
	gint scale_height;
	gdouble scale_x;
	gdouble scale_y;
	guint scale_handler;
	gboolean use_client_keymap;

	HGDI_DC hdc;
	gint srcBpp;
	GdkDisplay* display;
	GdkVisual* visual;
	cairo_surface_t* surface;
	cairo_format_t cairo_format;
	gint bpp;
	gint width;
	gint height;
	gint scanline_pad;
	gint* colormap;
	UINT8* primary_buffer;

	guint object_id_seq;
	GHashTable* object_table;

	GAsyncQueue* ui_queue;
	pthread_mutex_t ui_queue_mutex;
	guint ui_handler;

	GArray* pressed_keys;
	GAsyncQueue* event_queue;
	gint event_pipe[2];
	HANDLE event_handle;

	rfClipboard clipboard;
};

typedef struct remmina_plugin_rdp_ui_object RemminaPluginRdpUiObject;

void rf_init(RemminaProtocolWidget* gp);
void rf_uninit(RemminaProtocolWidget* gp);
void rf_get_fds(RemminaProtocolWidget* gp, void** rfds, int* rcount);
BOOL rf_check_fds(RemminaProtocolWidget* gp);
void rf_object_free(RemminaProtocolWidget* gp, RemminaPluginRdpUiObject* obj);

#endif

