/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
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

#pragma once

#include "common/remmina_plugin.h"
#include <freerdp/freerdp.h>
#include <freerdp/version.h>
#include <freerdp/channels/channels.h>
#include <freerdp/codec/color.h>
#include <freerdp/codec/rfx.h>
#include <freerdp/gdi/gdi.h>
#include <freerdp/gdi/dc.h>
#include <freerdp/gdi/region.h>
#include <freerdp/client/cliprdr.h>
#include <freerdp/client/disp.h>
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#elif defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdkwayland.h>
#endif

#include <winpr/clipboard.h>

/**
 * FREERDP_CHECK_VERSION:
 * @major: major version (e.g. 2 for version 2.1.0)
 * @minor: minor version (e.g. 0 for version 2.1.0)
 * @release: release version (e.g. 0 for version 2.1.0)
 *
 * Returns %TRUE if the version of the FRERDDP header files
 * is the same as or newer than the passed-in version.
 *
 * Returns: %TRUE if FREERDP headers are new enough
 */
#define FREERDP_CHECK_VERSION(major,minor,revision)                           \
	(FREERDP_VERSION_MAJOR > (major) ||                                       \
	 (FREERDP_VERSION_MAJOR == (major) && FREERDP_VERSION_MINOR > (minor)) || \
	 (FREERDP_VERSION_MAJOR == (major) && FREERDP_VERSION_MINOR == (minor) && \
	  FREERDP_VERSION_REVISION >= (revision)))

/**
 * Constants to workaround FreeRDP issue #5417 (min resolution in AVC mode)
 * Must be 4 aligned (multiple of 4).
 * We should instead check destRect and be sure the condition is met
 * See https://gitlab.com/Remmina/Remmina/-/issues/2507 for a deep discussion
 */
#define AVC_MIN_DESKTOP_WIDTH 644
#define AVC_MIN_DESKTOP_HEIGHT 480

typedef struct rf_context rfContext;

#define GET_PLUGIN_DATA(gp) (rfContext *)g_object_get_data(G_OBJECT(gp), "plugin-data")

/**
 * Performance Flags, from FreeRDP source
 *
 * PERF_FLAG_NONE 0x00000000
 * PERF_DISABLE_WALLPAPER 0x00000001
 * PERF_DISABLE_FULLWINDOWDRAG 0x00000002
 * PERF_DISABLE_MENUANIMATIONS 0x00000004
 * PERF_DISABLE_THEMING 0x00000008
 * PERF_DISABLE_CURSOR_SHADOW 0x00000020
 * PERF_DISABLE_CURSORSETTINGS 0x00000040
 * PERF_ENABLE_FONT_SMOOTHING 0x00000080
 * PERF_ENABLE_DESKTOP_COMPOSITION 0x00000100
 */
/* Poor (default) - all disabled */
#define DEFAULT_QUALITY_0       0x6f
/* Medium - only THEMING, CURSOR_SHADOW and CURSORSETTINGS enabled, all other disabled */
#define DEFAULT_QUALITY_1       0x07
/* Good - WALLPAPER, FONT_SMOOTHING, DESKTOP_COMPOSITION disabled, all other enabled */
#define DEFAULT_QUALITY_2       0x01
/* Best - DESKTOP_COMPOSITION disabled, all other enabled */
#define DEFAULT_QUALITY_9       0x80

extern RemminaPluginService *remmina_plugin_service;

#define REMMINA_PLUGIN_INFO(fmt, ...) \
		remmina_plugin_service->_remmina_info(__func__, fmt, ##__VA_ARGS__)

#define REMMINA_PLUGIN_MESSAGE(fmt, ...) \
		remmina_plugin_service->_remmina_message(__func, fmt, ##__VA_ARGS__)

#define REMMINA_PLUGIN_DEBUG(fmt, ...) \
		remmina_plugin_service->_remmina_debug(__func__, fmt, ##__VA_ARGS__)

#define REMMINA_PLUGIN_WARNING(fmt, ...) \
		remmina_plugin_service->_remmina_warning(__func__, fmt, ##__VA_ARGS__)

/* This will intentionally crash Remmina */
#define REMMINA_PLUGIN_ERROR(fmt, ...) \
		remmina_plugin_service->_remmina_error(__func__, fmt, ##__VA_ARGS__)

#define REMMINA_PLUGIN_CRITICAL(fmt, ...) \
		remmina_plugin_service->_remmina_critical(__func__, fmt, ##__VA_ARGS__)
#define REMMINA_PLUGIN_AUDIT(fmt, ...) \
		remmina_plugin_service->_remmina_audit(__func__, fmt, ##__VA_ARGS__)

struct rf_clipboard {
	rfContext *		rfi;
	CliprdrClientContext *	context;
	wClipboard *		system;
	int			requestedFormatId;

	UINT32			format;
	gulong			clipboard_handler;

	pthread_mutex_t		transfer_clip_mutex;
	pthread_cond_t		transfer_clip_cond;
	enum  { SCDW_NONE, SCDW_BUSY_WAIT, SCDW_ABORTING } srv_clip_data_wait;
	gpointer		srv_data;
	pthread_mutex_t	srv_data_mutex;

	UINT32			server_html_format_id;

	/* Stats for clipboard download */
	struct timeval clientformatdatarequest_tv;
};
typedef struct rf_clipboard rfClipboard;


struct rf_pointer {
	rdpPointer	pointer;
	GdkCursor *	cursor;
};
typedef struct rf_pointer rfPointer;

#ifdef RF_BITMAP
struct rf_bitmap {
	rdpBitmap		bitmap;
	Pixmap			pixmap;
	cairo_surface_t *	surface;
};
typedef struct rf_bitmap rfBitmap;
#endif

#ifdef RF_GLYPH
struct rf_glyph {
	rdpGlyph	glyph;
	Pixmap		pixmap;
};
typedef struct rf_glyph rfGlyph;
#endif

typedef enum {
	REMMINA_RDP_EVENT_TYPE_SCANCODE,
	REMMINA_RDP_EVENT_TYPE_SCANCODE_UNICODE,
	REMMINA_RDP_EVENT_TYPE_MOUSE,
	REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_LIST,
	REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_DATA_RESPONSE,
	REMMINA_RDP_EVENT_TYPE_CLIPBOARD_SEND_CLIENT_FORMAT_DATA_REQUEST,
	REMMINA_RDP_EVENT_TYPE_SEND_MONITOR_LAYOUT,
	REMMINA_RDP_EVENT_DISCONNECT
} RemminaPluginRdpEventType;

struct remmina_plugin_rdp_event {
	RemminaPluginRdpEventType type;
	union {
		struct {
			BOOL	up;
			BOOL	extended;
			UINT8	key_code;
			UINT32	unicode_code;
			BOOL	extended1;
		} key_event;
		struct {
			UINT16	flags;
			UINT16	x;
			UINT16	y;
			BOOL	extended;
		} mouse_event;
		struct {
			CLIPRDR_FORMAT_LIST *pFormatList;
		} clipboard_formatlist;
		struct {
			BYTE *	data;
			int	size;
		} clipboard_formatdataresponse;
		struct {
			CLIPRDR_FORMAT_DATA_REQUEST *pFormatDataRequest;
		} clipboard_formatdatarequest;
		struct {
			gint    Flags;
			gint    Left;
			gint    Top;
			gint	width;
			gint	height;
			gint	desktopOrientation;
			gint	desktopScaleFactor;
			gint	deviceScaleFactor;
			gint    physicalWidth;
			gint    physicalHeight;
		} monitor_layout;
	};
};
typedef struct remmina_plugin_rdp_event RemminaPluginRdpEvent;

typedef enum {
	REMMINA_RDP_UI_UPDATE_REGIONS = 0,
	REMMINA_RDP_UI_CONNECTED,
	REMMINA_RDP_UI_RECONNECT_PROGRESS,
	REMMINA_RDP_UI_CURSOR,
	REMMINA_RDP_UI_NOCODEC,
	REMMINA_RDP_UI_CLIPBOARD,
	REMMINA_RDP_UI_EVENT
} RemminaPluginRdpUiType;

typedef enum {
	REMMINA_RDP_UI_CLIPBOARD_FORMATLIST,
	REMMINA_RDP_UI_CLIPBOARD_GET_DATA,
	REMMINA_RDP_UI_CLIPBOARD_SET_DATA
} RemminaPluginRdpUiClipboardType;

typedef enum {
	REMMINA_RDP_POINTER_NEW,
	REMMINA_RDP_POINTER_FREE,
	REMMINA_RDP_POINTER_SET,
	REMMINA_RDP_POINTER_NULL,
	REMMINA_RDP_POINTER_DEFAULT,
	REMMINA_RDP_POINTER_SETPOS
} RemminaPluginRdpUiPointerType;

typedef enum {
	REMMINA_RDP_UI_EVENT_UPDATE_SCALE,
	REMMINA_RDP_UI_EVENT_DESTROY_CAIRO_SURFACE
} RemminaPluginRdpUiEeventType;

typedef struct {
	gint x, y, w, h;
} region;

struct remmina_plugin_rdp_ui_object {
	RemminaPluginRdpUiType	type;
	gboolean		sync;
	gboolean		complete;
	pthread_mutex_t		sync_wait_mutex;
	pthread_cond_t		sync_wait_cond;
	union {
		struct {
			region *ureg;
			gint	ninvalid;
		} reg;
		struct {
			rdpContext *			context;
			rfPointer *			pointer;
			RemminaPluginRdpUiPointerType	type;
		} cursor;
		struct {
			gint		left;
			gint		top;
			RFX_MESSAGE *	message;
		} rfx;
		struct {
			gint	left;
			gint	top;
			gint	width;
			gint	height;
			UINT8 * bitmap;
		} nocodec;
		struct {
			RemminaPluginRdpUiClipboardType type;
			GtkTargetList *			targetlist;
			UINT32				format;
			rfClipboard *			clipboard;
			gpointer			data;
		} clipboard;
		struct {
			RemminaPluginRdpUiEeventType type;
		} event;
		struct {
			gint	x;
			gint	y;
		} pos;
	};
	/* We can also return values here, usually integers*/
	int	retval;
	/* Some functions also may return a pointer. */
	void *	retptr;
};

typedef struct remmina_plugin_rdp_keymap_entry {
	unsigned	orig_keycode;
	unsigned	translated_keycode;
} RemminaPluginRdpKeymapEntry;

typedef struct rdp_remap_table FREERDP_REMAP_TABLE;
struct rf_context {
	rdpClientContext clientContext;

	RemminaProtocolWidget *protocol_widget;

	/* main */
	pthread_t remmina_plugin_thread;
	RemminaScaleMode scale;
	gboolean user_cancelled;
	gboolean thread_cancelled;

	CliprdrClientContext *	cliprdr;
	DispClientContext *	dispcontext;

	RDP_PLUGIN_DATA		rdpdr_data[5];
	RDP_PLUGIN_DATA		drdynvc_data[5];
	gchar			rdpsnd_options[20];

	gboolean		rdpgfxchan;

	gboolean		connected;
	gboolean		is_reconnecting;
	gboolean		stop_reconnecting_requested;
	/* orphaned: rf_context has still one or more libfreerdp thread active,
	 * but no longer maintained by an open RemminaProtocolWidget/tab.
	 * When the orphaned thread terminates, we must cleanup rf_context.
	 */
	gboolean		orphaned;
	int			reconnect_maxattempts;
	int			reconnect_nattempt;

	gboolean		sw_gdi;
	GtkWidget *		drawing_area;
	gint			scale_width;
	gint			scale_height;
	gdouble			scale_x;
	gdouble			scale_y;
	guint			delayed_monitor_layout_handler;
	gboolean		use_client_keymap;

	gint			srcBpp;
	GdkDisplay *		display;
	GdkVisual *		visual;
	cairo_surface_t *	surface;
	cairo_format_t		cairo_format;
	gint			bpp;
	gint			scanline_pad;
	gint *			colormap;

	guint			object_id_seq;
	GHashTable *		object_table;

	GAsyncQueue *		ui_queue;
	pthread_mutex_t		ui_queue_mutex;
	guint			ui_handler;

	GArray *		pressed_keys;
	GAsyncQueue *		event_queue;
	gint			event_pipe[2];
	HANDLE			event_handle;
	UINT16         	last_x;
	UINT16         	last_y;

	rfClipboard		clipboard;

	GArray *		keymap; /* Array of RemminaPluginRdpKeymapEntry */

	gboolean		attempt_interactive_authentication;
	
	// keep track of last interaction time for keep alive
	time_t			last_time;
	time_t			last_time_idle_keypress;
	FREERDP_REMAP_TABLE* remap_table;

	enum { REMMINA_POSTCONNECT_ERROR_OK = 0, REMMINA_POSTCONNECT_ERROR_GDI_INIT = 1, REMMINA_POSTCONNECT_ERROR_NO_H264 } postconnect_error;
};

typedef struct remmina_plugin_rdp_ui_object RemminaPluginRdpUiObject;

void rf_init(RemminaProtocolWidget *gp);
void rf_uninit(RemminaProtocolWidget *gp);
void rf_get_fds(RemminaProtocolWidget *gp, void **rfds, int *rcount);
BOOL rf_check_fds(RemminaProtocolWidget *gp);
void rf_object_free(RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *obj);

void remmina_rdp_event_event_push(RemminaProtocolWidget *gp, const RemminaPluginRdpEvent *e);
