/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_graphics.h"

#include <freerdp/codec/color.h>
#include <freerdp/codec/bitmap.h>
#include <winpr/memory.h>

/* Helper function */
static BOOL enqueue(RemminaProtocolWidget *widget,
					RemminaPluginRdpUiPointerType ptype, ...)
{
	va_list ap;
	RemminaPluginRdpUiObject* ui;

    if (!widget)
        return FALSE;

	ui = g_new0(RemminaPluginRdpUiObject, 1);
	if (!ui)
		return FALSE;

	memset(ui, 0, sizeof(RemminaPluginRdpUiObject));
	ui->type = REMMINA_RDP_UI_CURSOR;
	ui->sync = TRUE;	// Also wait for completion
	ui->cursor.type = ptype;

	va_start(ap, ptype);
	switch(ptype) {
		case REMMINA_RDP_POINTER_FREE:
		case REMMINA_RDP_POINTER_NEW:
		case REMMINA_RDP_POINTER_SET:
			ui->cursor.pointer = va_arg(ap, rfPointer*);
			break;
		case REMMINA_RDP_POINTER_SETPOS:
			ui->pos.x = va_arg(ap, UINT32);
			ui->pos.y = va_arg(ap, UINT32);
			break;
		default:
			ui->cursor.pointer = NULL;
			break;
	}
	va_end(ap);

	return rf_queue_ui(widget, ui) ? TRUE : FALSE;
}

/* Pointer Class */

static BOOL rf_Pointer_New(rdpContext* context, rdpPointer* pointer)
{
	TRACE_CALL("rf_Pointer_New");

	rfContext* rfi = (rfContext*) context;

	if (!rfi || !pointer) {
		return FALSE;
	}
	if ((pointer->andMaskData != 0) && (pointer->xorMaskData != 0))
	{
		return enqueue(rfi->protocol_widget,
					   REMMINA_RDP_POINTER_NEW, pointer);
	}

	return FALSE;
}

static void rf_Pointer_Free(rdpContext* context, rdpPointer* pointer)
{
	TRACE_CALL("rf_Pointer_Free");

	rfContext* rfi = (rfContext*) context;
	if (!rfi || !pointer) {
		return;
	}
#if GTK_VERSION == 2
	if (((rfPointer*) pointer)->cursor != NULL)
#else
	if (G_IS_OBJECT(((rfPointer*) pointer)->cursor))
#endif
	{
		enqueue(rfi->protocol_widget,
				REMMINA_RDP_POINTER_FREE, pointer);
	}
}

static BOOL rf_Pointer_Set(rdpContext* context, rdpPointer* pointer)
{
	TRACE_CALL("rf_Pointer_Set");

	rfContext* rfi = (rfContext*) context;
	if (!rfi || !pointer) {
		return FALSE;
	}

	return enqueue(rfi->protocol_widget,
				   REMMINA_RDP_POINTER_SET, pointer);
}

static BOOL rf_Pointer_SetNull(rdpContext* context)
{
	TRACE_CALL("rf_Pointer_SetNull");

	rfContext* rfi = (rfContext*) context;
	if (!rfi) {
		return FALSE;
	}

	return enqueue(rfi->protocol_widget,
				   REMMINA_RDP_POINTER_NULL);
}

static BOOL rf_Pointer_SetDefault(rdpContext* context)
{
	TRACE_CALL("rf_Pointer_SetDefault");

	rfContext* rfi = (rfContext*) context;
	if (!rfi) {
		return FALSE;
	}

	return enqueue(rfi->protocol_widget,
				   REMMINA_RDP_POINTER_DEFAULT);
}

static BOOL rf_Pointer_SetPosition(rdpContext* context, UINT32 x, UINT32 y)
{
	TRACE_CALL("rf_Pointer_SePosition");
	rfContext* rfi = (rfContext*) context;

	if (!rfi) {
		return FALSE;
	}

	return enqueue(rfi->protocol_widget,
				   REMMINA_RDP_POINTER_SETPOS, x, y);
}

/* Graphics Module */

void rf_register_graphics(rdpGraphics* graphics)
{
	TRACE_CALL("rf_register_graphics");
	rdpPointer pointer;

	memset(&pointer, 0, sizeof(pointer));
	pointer.size = sizeof(rfPointer);

	pointer.New = rf_Pointer_New;
	pointer.Free = rf_Pointer_Free;
	pointer.Set = rf_Pointer_Set;
	pointer.SetNull = rf_Pointer_SetNull;
	pointer.SetDefault = rf_Pointer_SetDefault;
	pointer.SetPosition = rf_Pointer_SetPosition;

	graphics_register_pointer(graphics, &pointer);
}
