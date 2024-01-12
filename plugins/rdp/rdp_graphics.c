/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_graphics.h"

#include <freerdp/codec/color.h>
#include <freerdp/codec/bitmap.h>
#include <winpr/memory.h>

//#define RF_BITMAP
//#define RF_GLYPH

/* Pointer Class */
#if FREERDP_VERSION_MAJOR < 3
#define CONST_ARG const
#else
#define CONST_ARG
#endif

static BOOL rf_Pointer_New(rdpContext* context, rdpPointer* pointer)
{
	TRACE_CALL(__func__);
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*)context;

	if (pointer->xorMaskData != 0) {
		ui = g_new0(RemminaPluginRdpUiObject, 1);
		ui->type = REMMINA_RDP_UI_CURSOR;
		ui->cursor.context = context;
		ui->cursor.pointer = (rfPointer*)pointer;
		ui->cursor.type = REMMINA_RDP_POINTER_NEW;
		return remmina_rdp_event_queue_ui_sync_retint(rfi->protocol_widget, ui) ? TRUE : FALSE;
	}
	return FALSE;
}

static void rf_Pointer_Free(rdpContext* context, rdpPointer* pointer)
{
	TRACE_CALL(__func__);
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*)context;

	if (G_IS_OBJECT(((rfPointer*)pointer)->cursor))
	{
		ui = g_new0(RemminaPluginRdpUiObject, 1);
		ui->type = REMMINA_RDP_UI_CURSOR;
		ui->cursor.context = context;
		ui->cursor.pointer = (rfPointer*)pointer;
		ui->cursor.type = REMMINA_RDP_POINTER_FREE;
		remmina_rdp_event_queue_ui_sync_retint(rfi->protocol_widget, ui);
	}
}

static BOOL rf_Pointer_Set(rdpContext* context, CONST_ARG rdpPointer* pointer)
{
	TRACE_CALL(__func__);
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*)context;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->cursor.pointer = (rfPointer*)pointer;
	ui->cursor.type = REMMINA_RDP_POINTER_SET;

	return remmina_rdp_event_queue_ui_sync_retint(rfi->protocol_widget, ui) ? TRUE : FALSE;

}

static BOOL rf_Pointer_SetNull(rdpContext* context)
{
	TRACE_CALL(__func__);
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*)context;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->cursor.type = REMMINA_RDP_POINTER_NULL;

	return remmina_rdp_event_queue_ui_sync_retint(rfi->protocol_widget, ui) ? TRUE : FALSE;
}

static BOOL rf_Pointer_SetDefault(rdpContext* context)
{
	TRACE_CALL(__func__);
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*)context;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->cursor.type = REMMINA_RDP_POINTER_DEFAULT;

	return remmina_rdp_event_queue_ui_sync_retint(rfi->protocol_widget, ui) ? TRUE : FALSE;
}

static BOOL rf_Pointer_SetPosition(rdpContext* context, UINT32 x, UINT32 y)
{
	TRACE_CALL(__func__);
	RemminaPluginRdpUiObject* ui;
	rfContext* rfi = (rfContext*)context;
	ui = g_new0(RemminaPluginRdpUiObject, 1);
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->cursor.type = REMMINA_RDP_POINTER_SETPOS;
	ui->pos.x = x;
	ui->pos.y = y;

	return remmina_rdp_event_queue_ui_sync_retint(rfi->protocol_widget, ui) ? TRUE : FALSE;
}

/* Graphics Module */

void rf_register_graphics(rdpGraphics* graphics)
{
	TRACE_CALL(__func__);
	rdpPointer pointer={0};

	pointer.size = sizeof(rfPointer);

	pointer.New = rf_Pointer_New;
	pointer.Free = rf_Pointer_Free;
	pointer.Set = rf_Pointer_Set;
	pointer.SetNull = rf_Pointer_SetNull;
	pointer.SetDefault = rf_Pointer_SetDefault;
	pointer.SetPosition = rf_Pointer_SetPosition;

	graphics_register_pointer(graphics, &pointer);
}
