/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2017 Antenore Gatta, Giovanni Panozzo
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

#include "config.h"

#include <cairo/cairo-xlib.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <stdlib.h>

#include "remmina_connection_window.h"
#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_init_dialog.h"
#include "remmina_plugin_cmdexec.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref.h"
#include "remmina_protocol_widget.h"
#include "remmina_public.h"
#include "remmina_scrolled_viewport.h"
#include "remmina_widget_pool.h"
#include "remmina_log.h"
#include "remmina/remmina_trace_calls.h"

#define DEBUG_KB_GRABBING 0
#include "remmina_exec.h"

gchar *remmina_pref_file;
RemminaPref remmina_pref;

G_DEFINE_TYPE( RemminaConnectionWindow, remmina_connection_window, GTK_TYPE_WINDOW)

#define MOTION_TIME 100

/* default timeout used to hide the floating toolbar wen switching profile */
#define TB_HIDE_TIME_TIME 1000

#define FLOATING_TOOLBAR_WIDGET (GTK_CHECK_VERSION(3, 10, 0))

typedef struct _RemminaConnectionHolder RemminaConnectionHolder;


struct _RemminaConnectionWindowPriv
{
	RemminaConnectionHolder* cnnhld;

	GtkWidget* notebook;

	guint switch_page_handler;

#if FLOATING_TOOLBAR_WIDGET
	GtkWidget* floating_toolbar_widget;
	GtkWidget* overlay;
	GtkWidget* revealer;
	GtkWidget* overlay_ftb_overlay;
#else
	GtkWidget* floating_toolbar_window;
	gboolean floating_toolbar_motion_show;
	gboolean floating_toolbar_motion_visible;
#endif

	GtkWidget* floating_toolbar_label;
	gdouble floating_toolbar_opacity;
	/* To avoid strange event-loop */
	guint floating_toolbar_motion_handler;
	/* Other event sources to remove when deleting the object */
	guint ftb_hide_eventsource;
	/* Timer to hide the toolbar */
	guint hidetb_timer;

	/* Timer to save new window state and wxh */
	guint savestate_eventsourceid;


	GtkWidget* toolbar;
	GtkWidget* grid;

	/* Toolitems that need to be handled */
	GtkToolItem* toolitem_autofit;
	GtkToolItem* toolitem_fullscreen;
	GtkToolItem* toolitem_switch_page;
	GtkToolItem* toolitem_dynres;
	GtkToolItem* toolitem_scale;
	GtkToolItem* toolitem_grab;
	GtkToolItem* toolitem_preferences;
	GtkToolItem* toolitem_tools;
	GtkToolItem* toolitem_screenshot;
	GtkWidget* fullscreen_option_button;
	GtkWidget* fullscreen_scaler_button;
	GtkWidget* scaler_option_button;

	GtkWidget* pin_button;
	gboolean pin_down;

	gboolean sticky;

	gint view_mode;

	gboolean kbcaptured;
	gboolean mouse_pointer_entered;

	RemminaConnectionWindowOnDeleteConfirmMode on_delete_confirm_mode;

};

typedef struct _RemminaConnectionObject
{
	RemminaConnectionHolder* cnnhld;

	RemminaFile* remmina_file;

	/* A dummy window which will be realized as a container during initialize, before reparent to the real window */
	GtkWidget* window;

	/* Containers for RemminaProtocolWidget: RemminaProtocolWidget->aspectframe->viewport->scrolledcontainer->...->window */
	GtkWidget* proto;
	GtkWidget* aspectframe;
	GtkWidget* viewport;

	/* Scrolled containers */
	GtkWidget* scrolled_container;

	gboolean plugin_can_scale;

	gboolean connected;
	gboolean dynres_unlocked;

} RemminaConnectionObject;

struct _RemminaConnectionHolder
{
	RemminaConnectionWindow* cnnwin;
	gint fullscreen_view_mode;

	gboolean hostkey_activated;
	gboolean hostkey_used;

};

enum
{
	TOOLBARPLACE_SIGNAL,
	LAST_SIGNAL
};

static guint remmina_connection_window_signals[LAST_SIGNAL] =
{ 0 };

#define DECLARE_CNNOBJ \
	if (!cnnhld || !cnnhld->cnnwin || gtk_notebook_get_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)) < 0) return; \
RemminaConnectionObject* cnnobj = (RemminaConnectionObject*) g_object_get_data ( \
		G_OBJECT(gtk_notebook_get_nth_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook), \
				gtk_notebook_get_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)))), "cnnobj");

#define DECLARE_CNNOBJ_WITH_RETURN(r) \
	if (!cnnhld || !cnnhld->cnnwin || gtk_notebook_get_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)) < 0) return r; \
RemminaConnectionObject* cnnobj = (RemminaConnectionObject*) g_object_get_data ( \
		G_OBJECT(gtk_notebook_get_nth_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook), \
				gtk_notebook_get_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)))), "cnnobj");

static void remmina_connection_holder_create_scrolled(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj);
static void remmina_connection_holder_create_fullscreen(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj,
		gint view_mode);
static gboolean remmina_connection_window_hostkey_func(RemminaProtocolWidget* gp, guint keyval, gboolean release,
		RemminaConnectionHolder* cnnhld);

static void remmina_connection_holder_grab_focus(GtkNotebook *notebook);
static GtkWidget* remmina_connection_holder_create_toolbar(RemminaConnectionHolder* cnnhld, gint mode);
static void remmina_connection_holder_place_toolbar(GtkToolbar *toolbar, GtkGrid *grid, GtkWidget *sibling, int toolbar_placement);

#if FLOATING_TOOLBAR_WIDGET
static void remmina_connection_window_ftb_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data);
#endif


static const GtkTargetEntry dnd_targets_ftb[] =
{
	{
		(char *)"text/x-remmina-ftb",
		GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET,
		0
	},
};

static const GtkTargetEntry dnd_targets_tb[] =
{
	{
		(char *)"text/x-remmina-tb",
		GTK_TARGET_SAME_APP,
		0
	},
};

static void remmina_connection_window_class_init(RemminaConnectionWindowClass* klass)
{
	TRACE_CALL("remmina_connection_window_class_init");
	GtkCssProvider  *provider;

	provider = gtk_css_provider_new();

	/* It's important to remove padding, border and shadow from GtkViewport or
	 * we will never know its internal area size, because GtkViweport::viewport_get_view_allocation,
	 * which returns the internal size of the GtkViewport, is private and we cannot access it */

#if GTK_CHECK_VERSION(3, 14, 0)
	gtk_css_provider_load_from_data (provider,
			"#remmina-cw-viewport, #remmina-cw-aspectframe {\n"
			"  padding:0;\n"
			"  border:0;\n"
			"  background-color: black;\n"
			"}\n"
			"GtkDrawingArea {\n"
			"  background-color: black;\n"
			"}\n"
			"GtkToolbar {\n"
			"  -GtkWidget-window-dragging: 0;\n"
			"}\n"
			"#remmina-connection-window-fullscreen {\n"
			"  background-color: black;\n"
			"}\n"
			"#remmina-small-button {\n"
			"  outline-offset: 0;\n"
			"  outline-width: 0;\n"
			"  padding: 0;\n"
			"  border: 0;\n"
			"}\n"
			"#remmina-pin-button {\n"
			"  outline-offset: 0;\n"
			"  outline-width: 0;\n"
			"  padding: 2px;\n"
			"  border: 0;\n"
			"}\n"
			"#remmina-scrolled-container {\n"
			"  background-color: black;\n"
			"}\n"
			"#remmina-scrolled-container.undershoot {\n"
			"  background: none\n"
			"}\n"
			"#ftbbox-upper {\n"
			"  border-style: none solid solid solid;\n"
			"  border-width: 1px;\n"
			"  border-radius: 4px;\n"
			"  border-color: #808080;\n"
			"  padding: 0px;\n"
			"  background-color: #f0f0f0;\n"
			"}\n"
			"#ftbbox-lower {\n"
			"  border-style: solid solid none solid;\n"
			"  border-width: 1px;\n"
			"  border-radius: 4px;\n"
			"  border-color: #808080;\n"
			"  padding: 0px;\n"
			"  background-color: #f0f0f0;\n"
			"}\n"
			"#ftb-handle {\n"
			"  background-color: #f0f0f0;\n"
			"}\n"

			,-1, NULL);

#else
	gtk_css_provider_load_from_data (provider,
			"#remmina-cw-viewport, #remmina-cw-aspectframe {\n"
			"  padding:0;\n"
			"  border:0;\n"
			"  background-color: black;\n"
			"}\n"
			"GtkDrawingArea {\n"
			"  background-color: black;\n"
			"}\n"
			"GtkToolbar {\n"
			"  -GtkWidget-window-dragging: 0;\n"
			"}\n"
			"#remmina-connection-window-fullscreen {\n"
			"  background-color: black;\n"
			"}\n"
			"#remmina-small-button {\n"
			"  -GtkWidget-focus-padding: 0;\n"
			"  -GtkWidget-focus-line-width: 0;\n"
			"  padding: 0;\n"
			"  border: 0;\n"
			"}\n"
			"#remmina-pin-button {\n"
			"  -GtkWidget-focus-padding: 0;\n"
			"  -GtkWidget-focus-line-width: 0;\n"
			"  padding: 2px;\n"
			"  border: 0;\n"
			"}\n"
			"#remmina-scrolled-container {\n"
			"  background-color: black;\n"
			"}\n"
			"#remmina-scrolled-container.undershoot {\n"
			"  background: none\n"
			"}\n"
			"#ftbbox-upper {\n"
			"  border-style: none solid solid solid;\n"
			"  border-width: 1px;\n"
			"  border-radius: 4px;\n"
			"  border-color: #808080;\n"
			"  padding: 0px;\n"
			"  background-color: #f0f0f0;\n"
			"}\n"
			"#ftbbox-lower {\n"
			"  border-style: solid solid none solid;\n"
			"  border-width: 1px;\n"
			"  border-radius: 4px;\n"
			"  border-color: #808080;\n"
			"  padding: 0px;\n"
			"  background-color: #f0f0f0;\n"
			"}\n"
			"#ftb-handle {\n"
			"  background-color: #f0f0f0;\n"
			"}\n"

			,-1, NULL);
#endif

	gtk_style_context_add_provider_for_screen (gdk_screen_get_default(),
			GTK_STYLE_PROVIDER (provider),
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref(provider);

	/* Define a signal used to notify all remmina_connection_windows of toolbar move */
	remmina_connection_window_signals[TOOLBARPLACE_SIGNAL] = g_signal_new("toolbar-place", G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaConnectionWindowClass, toolbar_place), NULL, NULL,
			g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

}

static RemminaScaleMode get_current_allowed_scale_mode(RemminaConnectionObject* cnnobj, gboolean *dynres_avail, gboolean *scale_avail)
{
	RemminaScaleMode scalemode;
	gboolean plugin_has_dynres, plugin_can_scale;

	scalemode = remmina_protocol_widget_get_current_scale_mode(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));

	plugin_has_dynres = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);

	plugin_can_scale = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);

	/* forbid scalemode REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES when not possible */
	if ((!plugin_has_dynres || !cnnobj->dynres_unlocked) && scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE;

	/* forbid scalemode REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED when not possible */
	if (!plugin_can_scale && scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE;

	if (scale_avail)
		*scale_avail = plugin_can_scale;
	if (dynres_avail)
		*dynres_avail = (plugin_has_dynres && cnnobj->dynres_unlocked);

	return scalemode;

}

static void remmina_connection_holder_disconnect_current_page(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_disconnect_current_page");
	DECLARE_CNNOBJ

	/* Disconnects the connection which is currently in view in the notebook */

	remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
}

static void remmina_connection_holder_keyboard_ungrab(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_keyboard_ungrab");
	GdkDisplay *display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkDevice *keyboard = NULL;

	display = gtk_widget_get_display(GTK_WIDGET(cnnhld->cnnwin));
#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(display);
	keyboard = gdk_seat_get_pointer(seat);
#else
	manager = gdk_display_get_device_manager(display);
	keyboard = gdk_device_manager_get_client_pointer(manager);
#endif


	if (!cnnhld->cnnwin->priv->kbcaptured)
	{
		return;
	}

	if (keyboard != NULL)
	{
		if ( gdk_device_get_source(keyboard) != GDK_SOURCE_KEYBOARD )
		{
			keyboard = gdk_device_get_associated_device(keyboard);
		}
#if DEBUG_KB_GRABBING
		printf("DEBUG_KB_GRABBING: --- ungrabbing\n");
#endif

#if GTK_CHECK_VERSION(3, 20, 0)
		gdk_seat_ungrab(seat);
#else
		gdk_device_ungrab(keyboard, GDK_CURRENT_TIME);
#endif
		cnnhld->cnnwin->priv->kbcaptured = FALSE;

	}
}

static void remmina_connection_holder_keyboard_grab(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_keyboard_grab");
	DECLARE_CNNOBJ
	GdkDisplay *display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkDevice *keyboard = NULL;

	if (cnnhld->cnnwin->priv->kbcaptured || !cnnhld->cnnwin->priv->mouse_pointer_entered)
	{
		return;
	}

	display = gtk_widget_get_display(GTK_WIDGET(cnnhld->cnnwin));
#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(display);
	keyboard = gdk_seat_get_pointer(seat);
#else
	manager = gdk_display_get_device_manager(display);
	keyboard = gdk_device_manager_get_client_pointer(manager);
#endif

	if (keyboard != NULL)
	{

		if ( gdk_device_get_source (keyboard) != GDK_SOURCE_KEYBOARD)
		{
			keyboard = gdk_device_get_associated_device( keyboard );
		}

		if (remmina_file_get_int(cnnobj->remmina_file, "keyboard_grab", FALSE))
		{
#if DEBUG_KB_GRABBING
			printf("DEBUG_KB_GRABBING: +++ grabbing\n");
#endif
#if GTK_CHECK_VERSION(3, 20, 0)
			if (gdk_seat_grab(seat, gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)),
				GDK_SEAT_CAPABILITY_KEYBOARD, FALSE, NULL, NULL, NULL, NULL) == GDK_GRAB_SUCCESS)
#else
			if (gdk_device_grab(keyboard, gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)), GDK_OWNERSHIP_WINDOW,
				TRUE, GDK_KEY_PRESS | GDK_KEY_RELEASE, NULL, GDK_CURRENT_TIME) == GDK_GRAB_SUCCESS)
#endif
			{
				cnnhld->cnnwin->priv->kbcaptured = TRUE;
			}
		}
		else
		{
			remmina_connection_holder_keyboard_ungrab(cnnhld);
		}
	}
}

static void remmina_connection_window_close_all_connections(RemminaConnectionWindow* cnnwin)
{
	RemminaConnectionWindowPriv* priv = cnnwin->priv;
	GtkNotebook* notebook = GTK_NOTEBOOK(priv->notebook);
	GtkWidget* w;
	RemminaConnectionObject* cnnobj;
	gint i, n;

	if (GTK_IS_WIDGET(notebook))
	{
		n = gtk_notebook_get_n_pages(notebook);
		for (i = n - 1; i >= 0; i--)
		{
			w = gtk_notebook_get_nth_page(notebook, i);
			cnnobj = (RemminaConnectionObject*) g_object_get_data(G_OBJECT(w), "cnnobj");
			/* Do close the connection on this tab */
			remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
		}
	}

}

gboolean remmina_connection_window_delete(RemminaConnectionWindow* cnnwin)
{
	TRACE_CALL("remmina_connection_window_delete");
	RemminaConnectionWindowPriv* priv = cnnwin->priv;
	RemminaConnectionHolder *cnnhld = cnnwin->priv->cnnhld;
	GtkNotebook* notebook = GTK_NOTEBOOK(priv->notebook);
	GtkWidget* dialog;
	gint i, n;

	if (!REMMINA_IS_CONNECTION_WINDOW(cnnwin))
		return TRUE;

	if (cnnwin->priv->on_delete_confirm_mode != REMMINA_CONNECTION_WINDOW_ONDELETE_NOCONFIRM) {
		n = gtk_notebook_get_n_pages(notebook);
		if (n > 1)
		{
			dialog = gtk_message_dialog_new(GTK_WINDOW(cnnwin), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
					GTK_BUTTONS_YES_NO,
					_("There are %i active connections in the current window. Are you sure to close?"), n);
			i = gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			if (i != GTK_RESPONSE_YES)
				return FALSE;
		}
	}
	remmina_connection_window_close_all_connections(cnnwin);

	/* After remmina_connection_window_close_all_connections() call, cnnwin
	 * has been already destroyed during a last page of notebook removal.
	 * So we must rely on cnnhld */
	if (cnnhld->cnnwin != NULL && GTK_IS_WIDGET(cnnhld->cnnwin))
		gtk_widget_destroy(GTK_WIDGET(cnnhld->cnnwin));
	cnnhld->cnnwin = NULL;

	return TRUE;
}

static gboolean remmina_connection_window_delete_event(GtkWidget* widget, GdkEvent* event, gpointer data)
{
	TRACE_CALL("remmina_connection_window_delete_event");
	remmina_connection_window_delete(REMMINA_CONNECTION_WINDOW(widget));
	return TRUE;
}

static void remmina_connection_window_destroy(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_destroy");
	RemminaConnectionWindowPriv* priv = REMMINA_CONNECTION_WINDOW(widget)->priv;

	if (priv->kbcaptured)
		remmina_connection_holder_keyboard_ungrab(cnnhld);

	if (priv->floating_toolbar_motion_handler)
	{
		g_source_remove(priv->floating_toolbar_motion_handler);
		priv->floating_toolbar_motion_handler = 0;
	}
	if (priv->ftb_hide_eventsource)
	{
		g_source_remove(priv->ftb_hide_eventsource);
		priv->ftb_hide_eventsource = 0;
	}
	if (priv->savestate_eventsourceid)
	{
		g_source_remove(priv->savestate_eventsourceid);
		priv->savestate_eventsourceid = 0;
	}

#if FLOATING_TOOLBAR_WIDGET
	/* There is no need to destroy priv->floating_toolbar_widget,
	 * because it's our child and will be destroyed automatically */
#else
	if (priv->floating_toolbar_window != NULL)
	{
		gtk_widget_destroy(priv->floating_toolbar_window);
		priv->floating_toolbar_window = NULL;
	}
#endif
	/* Timer used to hide the toolbar */
	if (priv->hidetb_timer)
	{
		g_source_remove(priv->hidetb_timer);
		priv->hidetb_timer= 0;
	}
	if (priv->switch_page_handler)
	{
		g_source_remove(priv->switch_page_handler);
		priv->switch_page_handler = 0;
	}
	g_free(priv);

	if (GTK_WIDGET(cnnhld->cnnwin) == widget)
	{
		cnnhld->cnnwin->priv = NULL;
		cnnhld->cnnwin = NULL;
	}

}

gboolean remmina_connection_window_notify_widget_toolbar_placement(GtkWidget *widget, gpointer data)
{
	TRACE_CALL("remmina_connection_window_notify_widget_toolbar_placement");
	GType rcwtype;
	rcwtype = remmina_connection_window_get_type();
	if (G_TYPE_CHECK_INSTANCE_TYPE(widget, rcwtype)) {
		g_signal_emit_by_name(G_OBJECT(widget), "toolbar-place");
		return TRUE;
	}
	return FALSE;
}

static gboolean remmina_connection_window_tb_drag_failed(GtkWidget *widget, GdkDragContext *context,
		GtkDragResult result, gpointer user_data)
{
	TRACE_CALL("remmina_connection_window_tb_drag_failed");
	RemminaConnectionHolder* cnnhld;
	RemminaConnectionWindowPriv* priv;

	cnnhld = (RemminaConnectionHolder*)user_data;
	priv = cnnhld->cnnwin->priv;

	if (priv->toolbar)
		gtk_widget_show(GTK_WIDGET(priv->toolbar));

	return TRUE;
}

static gboolean remmina_connection_window_tb_drag_drop(GtkWidget *widget, GdkDragContext *context,
		gint x, gint y, guint time, gpointer user_data)
{
	TRACE_CALL("remmina_connection_window_tb_drag_drop");
	GtkAllocation wa;
	gint new_toolbar_placement;
	RemminaConnectionHolder* cnnhld;
	RemminaConnectionWindowPriv* priv;

	cnnhld = (RemminaConnectionHolder*)user_data;
	priv = cnnhld->cnnwin->priv;

	gtk_widget_get_allocation(widget, &wa);

	if (wa.width * y >= wa.height * x) {
		if (wa.width * y > wa.height * ( wa.width - x) )
			new_toolbar_placement = TOOLBAR_PLACEMENT_BOTTOM;
		else
			new_toolbar_placement = TOOLBAR_PLACEMENT_LEFT;
	}
	else
	{
		if (wa.width * y > wa.height * ( wa.width - x) )
			new_toolbar_placement = TOOLBAR_PLACEMENT_RIGHT;
		else
			new_toolbar_placement = TOOLBAR_PLACEMENT_TOP;
	}

	gtk_drag_finish(context, TRUE, TRUE, time);

	if (new_toolbar_placement !=  remmina_pref.toolbar_placement)
	{
		/* Save new position */
		remmina_pref.toolbar_placement = new_toolbar_placement;
		remmina_pref_save();

		/* Signal all windows that the toolbar must be moved */
		remmina_widget_pool_foreach(remmina_connection_window_notify_widget_toolbar_placement, NULL);

	}
	if (priv->toolbar)
		gtk_widget_show(GTK_WIDGET(priv->toolbar));

	return TRUE;

}

static void remmina_connection_window_tb_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
	TRACE_CALL("remmina_connection_window_tb_drag_begin");

	RemminaConnectionHolder* cnnhld;
	RemminaConnectionWindowPriv* priv;
	cairo_surface_t *surface;
	cairo_t *cr;
	GtkAllocation wa;
	double dashes[] = { 10 };

	cnnhld = (RemminaConnectionHolder*)user_data;
	priv = cnnhld->cnnwin->priv;

	gtk_widget_get_allocation(widget, &wa);

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 16, 16);
	cr = cairo_create(surface);
	cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
	cairo_set_line_width(cr, 4);
	cairo_set_dash(cr, dashes, 1, 0 );
	cairo_rectangle(cr, 0, 0, 16, 16);
	cairo_stroke(cr);
	cairo_destroy(cr);

	gtk_widget_hide(widget);

	gtk_drag_set_icon_surface(context, surface);

}

static void remmina_connection_holder_update_toolbar_opacity(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_update_toolbar_opacity");
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->floating_toolbar_opacity = (1.0 - TOOLBAR_OPACITY_MIN) / ((gdouble) TOOLBAR_OPACITY_LEVEL)
		* ((gdouble)(TOOLBAR_OPACITY_LEVEL - remmina_file_get_int(cnnobj->remmina_file, "toolbar_opacity", 0)))
		+ TOOLBAR_OPACITY_MIN;
#if FLOATING_TOOLBAR_WIDGET
	if (priv->floating_toolbar_widget)
	{
		gtk_widget_set_opacity(GTK_WIDGET(priv->overlay_ftb_overlay), priv->floating_toolbar_opacity);
	}
#else
	if (priv->floating_toolbar_window)
	{
#if GTK_CHECK_VERSION(3, 8, 0)
		gtk_widget_set_opacity(GTK_WIDGET(priv->floating_toolbar_window), priv->floating_toolbar_opacity);
#else
		gtk_window_set_opacity(GTK_WINDOW(priv->floating_toolbar_window), priv->floating_toolbar_opacity);
#endif
	}
#endif
}

#if !FLOATING_TOOLBAR_WIDGET
static gboolean remmina_connection_holder_floating_toolbar_motion(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_floating_toolbar_motion");


	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkRequisition req;
	gint x, y, t, cnnwin_x, cnnwin_y;

	if (priv->floating_toolbar_window == NULL)
	{
		priv->floating_toolbar_motion_handler = 0;
		return FALSE;
	}

	gtk_widget_get_preferred_size(priv->floating_toolbar_window, &req, NULL);

	gtk_window_get_position(GTK_WINDOW(priv->floating_toolbar_window), &x, &y);
	gtk_window_get_position(GTK_WINDOW(cnnhld->cnnwin), &cnnwin_x, &cnnwin_y );
	x -= cnnwin_x;
	y -= cnnwin_y;

	if (priv->floating_toolbar_motion_show || priv->floating_toolbar_motion_visible)
	{
		if (priv->floating_toolbar_motion_show)
			y += 2;
		else
			y -= 2;
		t = (priv->pin_down ? 18 : 2) - req.height;
		if (y > 0)
			y = 0;
		if (y < t)
			y = t;

		gtk_window_move(GTK_WINDOW(priv->floating_toolbar_window), x + cnnwin_x, y + cnnwin_y);
		if (remmina_pref.fullscreen_toolbar_visibility == FLOATING_TOOLBAR_VISIBILITY_INVISIBLE && !priv->pin_down)
		{
#if GTK_CHECK_VERSION(3, 8, 0)
			gtk_widget_set_opacity(GTK_WIDGET(priv->floating_toolbar_window),
					(gdouble)(y - t) / (gdouble)(-t) * priv->floating_toolbar_opacity);
#else
			gtk_window_set_opacity(GTK_WINDOW(priv->floating_toolbar_window),
					(gdouble)(y - t) / (gdouble)(-t) * priv->floating_toolbar_opacity);
#endif
		}
		if ((priv->floating_toolbar_motion_show && y >= 0) || (!priv->floating_toolbar_motion_show && y <= t))
		{
			priv->floating_toolbar_motion_handler = 0;
			return FALSE;
		}
	}
	else
	{
		gtk_window_move(GTK_WINDOW(priv->floating_toolbar_window), x + cnnwin_x, -20 - req.height + cnnwin_y);
		priv->floating_toolbar_motion_handler = 0;
		return FALSE;
	}
	return TRUE;
}

static void remmina_connection_holder_floating_toolbar_update(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_floating_toolbar_update");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->floating_toolbar_motion_show || priv->floating_toolbar_motion_visible)
	{
		if (priv->floating_toolbar_motion_handler)
			g_source_remove(priv->floating_toolbar_motion_handler);
		priv->floating_toolbar_motion_handler = g_idle_add(
				(GSourceFunc) remmina_connection_holder_floating_toolbar_motion, cnnhld);
	}
	else
	{
		if (priv->floating_toolbar_motion_handler == 0)
		{
			priv->floating_toolbar_motion_handler = g_timeout_add(MOTION_TIME,
					(GSourceFunc) remmina_connection_holder_floating_toolbar_motion, cnnhld);
		}
	}
}
#endif /* !FLOATING_TOOLBAR_WIDGET */

#if FLOATING_TOOLBAR_WIDGET
static gboolean remmina_connection_holder_floating_toolbar_make_invisible(gpointer data)
{
	TRACE_CALL("remmina_connection_holder_floating_toolbar_make_invisible");
	RemminaConnectionWindowPriv* priv = (RemminaConnectionWindowPriv*)data;
	gtk_widget_set_opacity(GTK_WIDGET(priv->overlay_ftb_overlay), 0.0);
	priv->ftb_hide_eventsource = 0;
	return FALSE;
}
#endif

static void remmina_connection_holder_floating_toolbar_show(RemminaConnectionHolder* cnnhld, gboolean show)
{
	TRACE_CALL("remmina_connection_holder_floating_toolbar_show");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

#if FLOATING_TOOLBAR_WIDGET
	if (priv->floating_toolbar_widget == NULL)
		return;

	if (show || priv->pin_down)
	{
		/* Make the FTB no longer transparent, in case we have an hidden toolbar */
		remmina_connection_holder_update_toolbar_opacity(cnnhld);
		/* Remove outstanding hide events, if not yet active */
		if (priv->ftb_hide_eventsource)
		{
			g_source_remove(priv->ftb_hide_eventsource);
			priv->ftb_hide_eventsource = 0;
		}
	}
	else
	{
		/* If we are hiding and the toolbar must be made invisible, schedule
		 * a later toolbar hide */
		if (remmina_pref.fullscreen_toolbar_visibility == FLOATING_TOOLBAR_VISIBILITY_INVISIBLE)
		{
			if (priv->ftb_hide_eventsource == 0)
				priv->ftb_hide_eventsource = g_timeout_add(1000, remmina_connection_holder_floating_toolbar_make_invisible, priv);
		}
	}

	gtk_revealer_set_reveal_child(GTK_REVEALER(priv->revealer), show || priv->pin_down);
#else

	if (priv->floating_toolbar_window == NULL)
		return;

	priv->floating_toolbar_motion_show = show;

	remmina_connection_holder_floating_toolbar_update(cnnhld);
#endif
}

static void remmina_connection_holder_floating_toolbar_visible(RemminaConnectionHolder* cnnhld, gboolean visible)
{
	TRACE_CALL("remmina_connection_holder_floating_toolbar_visible");
#if !FLOATING_TOOLBAR_WIDGET
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->floating_toolbar_window == NULL)
		return;

	priv->floating_toolbar_motion_visible = visible;

	remmina_connection_holder_floating_toolbar_update(cnnhld);
#endif
}

static void remmina_connection_holder_get_desktop_size(RemminaConnectionHolder* cnnhld, gint* width, gint* height)
{
	TRACE_CALL("remmina_connection_holder_get_desktop_size");
	DECLARE_CNNOBJ
	RemminaProtocolWidget* gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);


	*width = remmina_protocol_widget_get_width(gp);
	*height = remmina_protocol_widget_get_height(gp);
}

static void remmina_connection_object_set_scrolled_policy(RemminaConnectionObject* cnnobj, GtkScrolledWindow* scrolled_window)
{
	TRACE_CALL("remmina_connection_object_set_scrolled_policy");
	RemminaScaleMode scalemode;
	scalemode = get_current_allowed_scale_mode(cnnobj, NULL, NULL);
	gtk_scrolled_window_set_policy(scrolled_window,
		scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC,
		scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC);
}

static gboolean remmina_connection_holder_toolbar_autofit_restore(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_autofit_restore");
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	gint dwidth, dheight;
	GtkAllocation nba, ca, ta;

	if (cnnobj->connected && GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container))
	{
		remmina_connection_holder_get_desktop_size(cnnhld, &dwidth, &dheight);
		gtk_widget_get_allocation(priv->notebook, &nba);
		gtk_widget_get_allocation(cnnobj->scrolled_container, &ca);
		gtk_widget_get_allocation(priv->toolbar, &ta);
		if (remmina_pref.toolbar_placement == TOOLBAR_PLACEMENT_LEFT ||
				remmina_pref.toolbar_placement == TOOLBAR_PLACEMENT_RIGHT)
		{
			gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), MAX(1, dwidth + ta.width + nba.width - ca.width),
					MAX(1, dheight + nba.height - ca.height));
		}
		else
		{
			gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), MAX(1, dwidth + nba.width - ca.width),
					MAX(1, dheight + ta.height + nba.height - ca.height));
		}
		gtk_container_check_resize(GTK_CONTAINER(cnnhld->cnnwin));
	}
	if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container))
	{
		remmina_connection_object_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(cnnobj->scrolled_container));
	}
	return FALSE;
}

static void remmina_connection_holder_toolbar_autofit(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_autofit");
	DECLARE_CNNOBJ

		if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container))
		{
			if ((gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin))) & GDK_WINDOW_STATE_MAXIMIZED) != 0)
			{
				gtk_window_unmaximize(GTK_WINDOW(cnnhld->cnnwin));
			}

			/* It's tricky to make the toolbars disappear automatically, while keeping scrollable.
			   Please tell me if you know a better way to do this */
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container), GTK_POLICY_NEVER,
					GTK_POLICY_NEVER);

			/* ToDo: save returned source id in priv->something and then delete when main object is destroyed */
			g_timeout_add(200, (GSourceFunc) remmina_connection_holder_toolbar_autofit_restore, cnnhld);
		}

}


static void remmina_connection_holder_check_resize(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_check_resize");
	DECLARE_CNNOBJ
	gboolean scroll_required = FALSE;

#if GTK_CHECK_VERSION(3, 22, 0)
	GdkDisplay* display;
	GdkMonitor* monitor;
#else
	GdkScreen* screen;
	gint monitor;
#endif
	GdkRectangle screen_size;
	gint screen_width, screen_height;
	gint server_width, server_height;
	gint bordersz;

	remmina_connection_holder_get_desktop_size(cnnhld, &server_width, &server_height);
#if GTK_CHECK_VERSION(3, 22, 0)
	display = gtk_widget_get_display(GTK_WIDGET(cnnhld->cnnwin));
	monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)));
#else
	screen = gtk_window_get_screen(GTK_WINDOW(cnnhld->cnnwin));
	monitor = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)));
#endif
#if GTK_CHECK_VERSION(3, 22, 0)
	gdk_monitor_get_workarea(monitor, &screen_size);
#elif gdk_screen_get_monitor_workarea
	gdk_screen_get_monitor_workarea(screen, monitor, &screen_size);
#else
	gdk_screen_get_monitor_geometry(screen, monitor, &screen_size);
#endif
	screen_width = screen_size.width;
	screen_height = screen_size.height;

	if (!remmina_protocol_widget_get_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto))
			&& (server_width <= 0 || server_height <= 0 || screen_width < server_width
				|| screen_height < server_height))
	{
		scroll_required = TRUE;
	}

	switch (cnnhld->cnnwin->priv->view_mode)
	{
		case SCROLLED_FULLSCREEN_MODE:
			gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), screen_width, screen_height);
			gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container),
					(scroll_required ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER),
					(scroll_required ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER));
			break;

		case VIEWPORT_FULLSCREEN_MODE:
			bordersz = scroll_required ? 1 : 0;
			gtk_window_resize (GTK_WINDOW(cnnhld->cnnwin), screen_width , screen_height);
			if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container))
			{
				/* Put a border around Notebook content (RemminaScrolledViewpord), so we can
				 * move the mouse over the border to scroll */
				gtk_container_set_border_width (GTK_CONTAINER (cnnobj->scrolled_container), bordersz);
			}

			break;

		case SCROLLED_WINDOW_MODE:
			if (remmina_file_get_int (cnnobj->remmina_file, "viewmode", AUTO_MODE) == AUTO_MODE)
			{
				gtk_window_set_default_size (GTK_WINDOW(cnnhld->cnnwin),
						MIN (server_width, screen_width), MIN (server_height, screen_height));
				if (server_width >= screen_width || server_height >= screen_height)
				{
					gtk_window_maximize (GTK_WINDOW(cnnhld->cnnwin));
					remmina_file_set_int (cnnobj->remmina_file, "window_maximize", TRUE);
				}
				else
				{
					remmina_connection_holder_toolbar_autofit (NULL, cnnhld);
					remmina_file_set_int (cnnobj->remmina_file, "window_maximize", FALSE);
				}
			}
			else
			{
				if (remmina_file_get_int (cnnobj->remmina_file, "window_maximize", FALSE))
				{
					gtk_window_maximize (GTK_WINDOW(cnnhld->cnnwin));
				}
			}
			break;

		default:
			break;
	}
}

static void remmina_connection_holder_set_tooltip(GtkWidget* item, const gchar* tip, guint key1, guint key2)
{
	TRACE_CALL("remmina_connection_holder_set_tooltip");
	gchar* s1;
	gchar* s2;

	if (remmina_pref.hostkey && key1)
	{
		if (key2)
		{
			s1 = g_strdup_printf(" (%s + %s,%s)", gdk_keyval_name(remmina_pref.hostkey),
					gdk_keyval_name(gdk_keyval_to_upper(key1)), gdk_keyval_name(gdk_keyval_to_upper(key2)));
		}
		else if (key1 == remmina_pref.hostkey)
		{
			s1 = g_strdup_printf(" (%s)", gdk_keyval_name(remmina_pref.hostkey));
		}
		else
		{
			s1 = g_strdup_printf(" (%s + %s)", gdk_keyval_name(remmina_pref.hostkey),
					gdk_keyval_name(gdk_keyval_to_upper(key1)));
		}
	}
	else
	{
		s1 = NULL;
	}
	s2 = g_strdup_printf("%s%s", tip, s1 ? s1 : "");
	gtk_widget_set_tooltip_text(item, s2);
	g_free(s2);
	g_free(s1);
}

static void remmina_protocol_widget_update_alignment(RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_protocol_widget_update_alignment");
	RemminaScaleMode scalemode;
	gboolean scaledexpandedmode;
	int rdwidth, rdheight;
	gfloat aratio;

	if (!cnnobj->plugin_can_scale)
	{
		/* If we have a plugin that cannot scale,
		 * (i.e. SFTP plugin), then we expand proto */
		gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto),GTK_ALIGN_FILL);
		gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto),GTK_ALIGN_FILL);
	}
	else
	{
		/* Plugin can scale */

		scalemode = get_current_allowed_scale_mode(cnnobj, NULL, NULL);
		scaledexpandedmode = remmina_protocol_widget_get_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));

		/* Check if we need aspectframe and create/destroy it accordingly */
		if (scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED && !scaledexpandedmode)
		{
			/* We need an aspectframe as a parent of proto */
			rdwidth = remmina_protocol_widget_get_width(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
			rdheight = remmina_protocol_widget_get_height(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
			aratio = (gfloat)rdwidth / (gfloat)rdheight;
			if (!cnnobj->aspectframe)
			{
				/* We need a new aspectframe */
				cnnobj->aspectframe = gtk_aspect_frame_new(NULL, 0.5, 0.5, aratio, FALSE);
				gtk_widget_set_name(cnnobj->aspectframe, "remmina-cw-aspectframe");
				gtk_frame_set_shadow_type(GTK_FRAME(cnnobj->aspectframe), GTK_SHADOW_NONE);
				g_object_ref(cnnobj->proto);
				gtk_container_remove(GTK_CONTAINER(cnnobj->viewport), cnnobj->proto);
				gtk_container_add(GTK_CONTAINER(cnnobj->viewport), cnnobj->aspectframe);
				gtk_container_add(GTK_CONTAINER(cnnobj->aspectframe), cnnobj->proto);
				g_object_unref(cnnobj->proto);
				gtk_widget_show(cnnobj->aspectframe);
				if (cnnobj->cnnhld != NULL && cnnobj->cnnhld->cnnwin != NULL && cnnobj->cnnhld->cnnwin->priv->notebook != NULL)
					remmina_connection_holder_grab_focus(GTK_NOTEBOOK(cnnobj->cnnhld->cnnwin->priv->notebook));
			}
			else
			{
				gtk_aspect_frame_set(GTK_ASPECT_FRAME(cnnobj->aspectframe), 0.5, 0.5, aratio, FALSE);
			}
		}
		else
		{
			/* We do not need an aspectframe as a parent of proto */
			if (cnnobj->aspectframe)
			{
				/* We must remove the old aspectframe reparenting proto to viewport */
				g_object_ref(cnnobj->aspectframe);
				g_object_ref(cnnobj->proto);
				gtk_container_remove(GTK_CONTAINER(cnnobj->aspectframe), cnnobj->proto);
				gtk_container_remove(GTK_CONTAINER(cnnobj->viewport), cnnobj->aspectframe);
				g_object_unref(cnnobj->aspectframe);
				cnnobj->aspectframe = NULL;
				gtk_container_add(GTK_CONTAINER(cnnobj->viewport), cnnobj->proto);
				g_object_unref(cnnobj->proto);
				if (cnnobj->cnnhld != NULL && cnnobj->cnnhld->cnnwin != NULL && cnnobj->cnnhld->cnnwin->priv->notebook != NULL)
					remmina_connection_holder_grab_focus(GTK_NOTEBOOK(cnnobj->cnnhld->cnnwin->priv->notebook));
			}
		}

		if (scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED || scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES)
		{
			/* We have a plugin that can be scaled, and the scale button
			 * has been pressed. Give it the correct WxH maintaining aspect
			 * ratio of remote destkop size */
			gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto),GTK_ALIGN_FILL);
			gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto),GTK_ALIGN_FILL);
		}
		else
		{
			/* Plugin can scale, but no scaling is active. Ensure that we have
			 * aspectframe with a ratio of 1 */
			gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto),GTK_ALIGN_CENTER);
			gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto),GTK_ALIGN_CENTER);
		}
	}
}


static void remmina_connection_holder_toolbar_fullscreen(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_fullscreen");
	if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
	{
		remmina_connection_holder_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
	}
	else
	{
		remmina_connection_holder_create_scrolled(cnnhld, NULL);
	}
}

static void remmina_connection_holder_viewport_fullscreen_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_viewport_fullscreen_mode");
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnhld->fullscreen_view_mode = VIEWPORT_FULLSCREEN_MODE;
	remmina_connection_holder_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
}

static void remmina_connection_holder_scrolled_fullscreen_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_scrolled_fullscreen_mode");
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnhld->fullscreen_view_mode = SCROLLED_FULLSCREEN_MODE;
	remmina_connection_holder_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
}

static void remmina_connection_holder_fullscreen_option_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_fullscreen_option_popdown");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->fullscreen_option_button), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_toolbar_fullscreen_option(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_fullscreen_option");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* menu;
	GtkWidget* menuitem;
	GSList* group;

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	menu = gtk_menu_new();

	menuitem = gtk_radio_menu_item_new_with_label(NULL, _("Viewport fullscreen mode"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
	if (priv->view_mode == VIEWPORT_FULLSCREEN_MODE)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(remmina_connection_holder_viewport_fullscreen_mode), cnnhld);

	menuitem = gtk_radio_menu_item_new_with_label(group, _("Scrolled fullscreen mode"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (priv->view_mode == SCROLLED_FULLSCREEN_MODE)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(remmina_connection_holder_scrolled_fullscreen_mode), cnnhld);

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(remmina_connection_holder_fullscreen_option_popdown), cnnhld);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, priv->toolitem_fullscreen, 0,
			gtk_get_current_event_time());
#endif
}


static void remmina_connection_holder_scaler_option_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_scaler_option_popdown");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	priv->sticky = FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->scaler_option_button), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_scaler_expand(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_scaler_expand");
	DECLARE_CNNOBJ
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	remmina_protocol_widget_set_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), TRUE);
	remmina_file_set_int(cnnobj->remmina_file, "scaler_expand", TRUE);
	remmina_protocol_widget_update_alignment(cnnobj);
}
static void remmina_connection_holder_scaler_keep_aspect(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_scaler_keep_aspect");
	DECLARE_CNNOBJ
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	remmina_protocol_widget_set_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), FALSE);
	remmina_file_set_int(cnnobj->remmina_file, "scaler_expand", FALSE);
	remmina_protocol_widget_update_alignment(cnnobj);
}

static void remmina_connection_holder_toolbar_scaler_option(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_scaler_option");
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* menu;
	GtkWidget* menuitem;
	GSList* group;
	gboolean scaler_expand;

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	scaler_expand = remmina_protocol_widget_get_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));

	priv->sticky = TRUE;

	menu = gtk_menu_new();

	menuitem = gtk_radio_menu_item_new_with_label(NULL, _("Keep aspect ratio when scaled"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
	if (!scaler_expand)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(remmina_connection_holder_scaler_keep_aspect), cnnhld);

	menuitem = gtk_radio_menu_item_new_with_label(group, _("Fill client window when scaled"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (scaler_expand)
	{
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(remmina_connection_holder_scaler_expand), cnnhld);

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(remmina_connection_holder_scaler_option_popdown), cnnhld);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, priv->toolitem_scale, 0,
			gtk_get_current_event_time());
#endif
}

static void remmina_connection_holder_switch_page_activate(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_switch_page_activate");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	gint page_num;

	page_num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), "new-page-num"));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page_num);
}

static void remmina_connection_holder_toolbar_switch_page_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_switch_page_popdown");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_switch_page), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_toolbar_switch_page(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_switch_page");
	RemminaConnectionObject* cnnobj;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* menu;
	GtkWidget* menuitem;
	GtkWidget* image;
	GtkWidget* page;
	gint i, n;

	if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	menu = gtk_menu_new();

	n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook));
	for (i = 0; i < n; i++)
	{
		page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(priv->notebook), i);
		if (!page)
			break;
		cnnobj = (RemminaConnectionObject*) g_object_get_data(G_OBJECT(page), "cnnobj");

		menuitem = gtk_menu_item_new_with_label(remmina_file_get_string(cnnobj->remmina_file, "name"));
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		image = gtk_image_new_from_icon_name(remmina_file_get_icon_name(cnnobj->remmina_file), GTK_ICON_SIZE_MENU);
		gtk_widget_show(image);

		g_object_set_data(G_OBJECT(menuitem), "new-page-num", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(remmina_connection_holder_switch_page_activate),
				cnnhld);
		if (i == gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)))
		{
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
	}

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(remmina_connection_holder_toolbar_switch_page_popdown),
			cnnhld);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
#endif

}

static void remmina_connection_holder_update_toolbar_autofit_button(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_update_toolbar_autofit_button");
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkToolItem* toolitem;
	RemminaScaleMode sc;

	toolitem = priv->toolitem_autofit;
	if (toolitem)
	{
		if (priv->view_mode != SCROLLED_WINDOW_MODE)
		{
			gtk_widget_set_sensitive(GTK_WIDGET(toolitem), FALSE);
		}
		else
		{
			sc = get_current_allowed_scale_mode(cnnobj, NULL, NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(toolitem), sc == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE);
		}
	}
}

static void remmina_connection_holder_change_scalemode(RemminaConnectionHolder* cnnhld, gboolean bdyn, gboolean bscale)
{
	RemminaScaleMode scalemode;
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (bdyn && cnnobj->dynres_unlocked)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES;
	else if (bscale)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED;
	else
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE;

	if (scalemode != REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES) {
		remmina_file_set_int(cnnobj->remmina_file, "dynamic_resolution_width", 0);
		remmina_file_set_int(cnnobj->remmina_file, "dynamic_resolution_height", 0);
	}

	remmina_protocol_widget_set_current_scale_mode(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), scalemode);
	remmina_file_set_int(cnnobj->remmina_file, "scale", scalemode);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED);
	remmina_connection_holder_update_toolbar_autofit_button(cnnhld);

	remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, 0);

	if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
	{
		remmina_connection_holder_check_resize(cnnhld);
	}
	if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container))
	{
		remmina_connection_object_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(cnnobj->scrolled_container));
	}

}

static void remmina_connection_holder_toolbar_dynres(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_dynres");
	gboolean bdyn, bscale;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	bdyn = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
	bscale = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale));

	if (bdyn && bscale) {
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale), FALSE);
		bscale = FALSE;
	}

	remmina_connection_holder_change_scalemode(cnnhld, bdyn, bscale);
}


static void remmina_connection_holder_toolbar_scaled_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_scaled_mode");
	gboolean bdyn, bscale;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	bdyn = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres));
	bscale = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));

	if (bdyn && bscale) {
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres), FALSE);
		bdyn = FALSE;
	}

	remmina_connection_holder_change_scalemode(cnnhld, bdyn, bscale);
}


static gboolean remmina_connection_holder_trap_on_button(GtkWidget* widget, GdkEventButton* event, gpointer data)
{
	TRACE_CALL("remmina_connection_holder_trap_on_button");
	return TRUE;
}

static void remmina_connection_holder_toolbar_preferences_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_preferences_popdown");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_preferences), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_toolbar_tools_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_tools_popdown");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_tools), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_call_protocol_feature_radio(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_call_protocol_feature_radio");
	DECLARE_CNNOBJ
	RemminaProtocolFeature* feature;
	gpointer value;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem)))
	{
		feature = (RemminaProtocolFeature*) g_object_get_data(G_OBJECT(menuitem), "feature-type");
		value = g_object_get_data(G_OBJECT(menuitem), "feature-value");

		remmina_file_set_string(cnnobj->remmina_file, (const gchar*) feature->opt2, (const gchar*) value);
		remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
	}
}

static void remmina_connection_holder_call_protocol_feature_check(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_call_protocol_feature_check");
	DECLARE_CNNOBJ
	RemminaProtocolFeature* feature;
	gboolean value;

	feature = (RemminaProtocolFeature*) g_object_get_data(G_OBJECT(menuitem), "feature-type");
	value = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
	remmina_file_set_int(cnnobj->remmina_file, (const gchar*) feature->opt2, value);
	remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
}

static void remmina_connection_holder_call_protocol_feature_activate(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_call_protocol_feature_activate");
	DECLARE_CNNOBJ
	RemminaProtocolFeature* feature;

	feature = (RemminaProtocolFeature*) g_object_get_data(G_OBJECT(menuitem), "feature-type");
	remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
}

static void remmina_connection_holder_toolbar_preferences_radio(RemminaConnectionHolder* cnnhld, RemminaFile* remminafile,
		GtkWidget* menu, const RemminaProtocolFeature* feature, const gchar* domain, gboolean enabled)
{
	TRACE_CALL("remmina_connection_holder_toolbar_preferences_radio");
	GtkWidget* menuitem;
	GSList* group;
	gint i;
	const gchar** list;
	const gchar* value;

	group = NULL;
	value = remmina_file_get_string(remminafile, (const gchar*) feature->opt2);
	list = (const gchar**) feature->opt3;
	for (i = 0; list[i]; i += 2)
	{
		menuitem = gtk_radio_menu_item_new_with_label(group, g_dgettext(domain, list[i + 1]));
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		if (enabled)
		{
			g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer) feature);
			g_object_set_data(G_OBJECT(menuitem), "feature-value", (gpointer) list[i]);

			if (value && g_strcmp0(list[i], value) == 0)
			{
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
			}

			g_signal_connect(G_OBJECT(menuitem), "toggled",
					G_CALLBACK(remmina_connection_holder_call_protocol_feature_radio), cnnhld);
		}
		else
		{
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
	}
}

static void remmina_connection_holder_toolbar_preferences_check(RemminaConnectionHolder* cnnhld, RemminaFile* remminafile,
		GtkWidget* menu, const RemminaProtocolFeature* feature, const gchar* domain, gboolean enabled)
{
	TRACE_CALL("remmina_connection_holder_toolbar_preferences_check");
	GtkWidget* menuitem;

	menuitem = gtk_check_menu_item_new_with_label(g_dgettext(domain, (const gchar*) feature->opt3));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	if (enabled)
	{
		g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer) feature);

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
				remmina_file_get_int(remminafile, (const gchar*) feature->opt2, FALSE));

		g_signal_connect(G_OBJECT(menuitem), "toggled",
				G_CALLBACK(remmina_connection_holder_call_protocol_feature_check), cnnhld);
	}
	else
	{
		gtk_widget_set_sensitive(menuitem, FALSE);
	}
}

static void remmina_connection_holder_toolbar_preferences(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_preferences");
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	const RemminaProtocolFeature* feature;
	GtkWidget* menu;
	GtkWidget* menuitem;
	gboolean separator;
	const gchar* domain;
	gboolean enabled;

	if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	separator = FALSE;

	domain = remmina_protocol_widget_get_domain(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	menu = gtk_menu_new();
	for (feature = remmina_protocol_widget_get_features(REMMINA_PROTOCOL_WIDGET(cnnobj->proto)); feature && feature->type;
			feature++)
	{
		if (feature->type != REMMINA_PROTOCOL_FEATURE_TYPE_PREF)
			continue;

		if (separator)
		{
			menuitem = gtk_separator_menu_item_new();
			gtk_widget_show(menuitem);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			separator = FALSE;
		}
		enabled = remmina_protocol_widget_query_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
		switch (GPOINTER_TO_INT(feature->opt1))
		{
			case REMMINA_PROTOCOL_FEATURE_PREF_RADIO:
				remmina_connection_holder_toolbar_preferences_radio(cnnhld, cnnobj->remmina_file, menu, feature,
						domain, enabled);
				separator = TRUE;
				break;
			case REMMINA_PROTOCOL_FEATURE_PREF_CHECK:
				remmina_connection_holder_toolbar_preferences_check(cnnhld, cnnobj->remmina_file, menu, feature,
						domain, enabled);
				break;
		}
	}

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(remmina_connection_holder_toolbar_preferences_popdown),
			cnnhld);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
#endif

}

static void remmina_connection_holder_toolbar_tools(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_tools");
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	const RemminaProtocolFeature* feature;
	GtkWidget* menu;
	GtkWidget* menuitem = NULL;
	GtkMenu *submenu_keystrokes;
	const gchar* domain;
	gboolean enabled;
	gchar **keystrokes;
	gchar **keystroke_values;
	gint i;

	if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	domain = remmina_protocol_widget_get_domain(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	menu = gtk_menu_new();
	for (feature = remmina_protocol_widget_get_features(REMMINA_PROTOCOL_WIDGET(cnnobj->proto)); feature && feature->type;
			feature++)
	{
		if (feature->type != REMMINA_PROTOCOL_FEATURE_TYPE_TOOL)
			continue;

		if (feature->opt1)
		{
			menuitem = gtk_menu_item_new_with_label(g_dgettext(domain, (const gchar*) feature->opt1));
		}
		if (feature->opt3)
		{
			remmina_connection_holder_set_tooltip(menuitem, "", GPOINTER_TO_UINT(feature->opt3), 0);
		}
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		enabled = remmina_protocol_widget_query_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
		if (enabled)
		{
			g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer) feature);

			g_signal_connect(G_OBJECT(menuitem), "activate",
					G_CALLBACK(remmina_connection_holder_call_protocol_feature_activate), cnnhld);
		}
		else
		{
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
	}

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(remmina_connection_holder_toolbar_tools_popdown), cnnhld);

	/* If the plugin accepts keystrokes include the keystrokes menu */
	if (remmina_protocol_widget_plugin_receives_keystrokes(REMMINA_PROTOCOL_WIDGET(cnnobj->proto)))
	{
		/* Get the registered keystrokes list */
		keystrokes = g_strsplit(remmina_pref.keystrokes, STRING_DELIMITOR, -1);
		if (g_strv_length(keystrokes))
		{
			/* Add a keystrokes submenu */
			menuitem = gtk_menu_item_new_with_label(_("Keystrokes"));
			submenu_keystrokes = GTK_MENU(gtk_menu_new());
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), GTK_WIDGET(submenu_keystrokes));
			gtk_widget_show(menuitem);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			/* Add each registered keystroke */
			for (i = 0; i < g_strv_length(keystrokes); i++)
			{
				keystroke_values = g_strsplit(keystrokes[i], STRING_DELIMITOR2, -1);
				if (g_strv_length(keystroke_values) > 1)
				{
					/* Add the keystroke if no description was available */
					menuitem = gtk_menu_item_new_with_label(
							g_strdup(keystroke_values[strlen(keystroke_values[0]) ? 0 : 1]));
					g_object_set_data(G_OBJECT(menuitem), "keystrokes", g_strdup(keystroke_values[1]));
					g_signal_connect_swapped(G_OBJECT(menuitem), "activate",
							G_CALLBACK(remmina_protocol_widget_send_keystrokes),
							REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
					gtk_widget_show(menuitem);
					gtk_menu_shell_append(GTK_MENU_SHELL(submenu_keystrokes), menuitem);
				}
				g_strfreev(keystroke_values);
			}
		}
		g_strfreev(keystrokes);
	}
#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
#endif

}

static void remmina_connection_holder_toolbar_screenshot(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_screenshot");

	GdkPixbuf *screenshot;
	GdkWindow *active_window;
	cairo_t *cr;
	gint width, height;
	const gchar* remminafile;
	gchar* pngname;
	gchar* pngdate;
	GtkWidget* dialog;
	RemminaProtocolWidget *gp;
	RemminaPluginScreenshotData rpsd;
	cairo_surface_t *srcsurface;
	cairo_format_t cairo_format;
	cairo_surface_t *surface;
	int stride;

	GDateTime *date = g_date_time_new_now_utc ();

	// We will take a screenshot of the currently displayed RemminaProtocolWidget.
	// DECLARE_CNNOBJ already did part of the job for us.
	DECLARE_CNNOBJ
	gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);

	// Ask the plugin if it can give us a screenshot
	if (remmina_protocol_widget_plugin_screenshot(gp, &rpsd)) {
		// Good, we have a screenshot from the plugin !

		remmina_log_printf("Screenshot from plugin: w=%d h=%d bpp=%d bytespp=%d\n",
			rpsd.width, rpsd.height, rpsd.bitsPerPixel, rpsd.bytesPerPixel);

		width = rpsd.width;
		height = rpsd.height;

		if (rpsd.bitsPerPixel == 32)
			cairo_format = CAIRO_FORMAT_ARGB32;
		else if (rpsd.bitsPerPixel == 24)
			cairo_format = CAIRO_FORMAT_RGB24;
		else
			cairo_format = CAIRO_FORMAT_RGB16_565;

		stride = cairo_format_stride_for_width(cairo_format, width);

		srcsurface = cairo_image_surface_create_for_data(rpsd.buffer, cairo_format, width, height, stride);

		surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
		cr = cairo_create(surface);
		cairo_set_source_surface(cr, srcsurface, 0, 0);
		cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
		cairo_paint(cr);
		cairo_surface_destroy(srcsurface);

		free(rpsd.buffer);

	} else {
		// The plugin is not releasing us a screenshot, just try to catch one via GTK

		/* Warn the user if image is distorted */
		if (cnnobj->plugin_can_scale &&
				get_current_allowed_scale_mode(cnnobj,NULL,NULL) == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED) {
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
					_("Warning: screenshot is scaled or distorted. Disable scaling to have better screenshot."));
			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show(dialog);
		}

		// Get the screenshot.
		active_window = gtk_widget_get_window(GTK_WIDGET(gp));
		// width = gdk_window_get_width(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)));
		width = gdk_window_get_width (active_window);
		// height = gdk_window_get_height(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)));
		height = gdk_window_get_height (active_window);

		screenshot = gdk_pixbuf_get_from_window (active_window, 0, 0, width, height);
		if (screenshot == NULL)
			g_print("gdk_pixbuf_get_from_window failed\n");

		// Prepare the destination cairo surface.
		surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
		cr = cairo_create(surface);

		// Copy the source pixbuf to the surface and paint it.
		gdk_cairo_set_source_pixbuf(cr, screenshot, 0, 0);
		cairo_paint(cr);

		// Deallocate screenshot pixbuf
		g_object_unref(screenshot);

	}

	remminafile = remmina_file_get_filename(cnnobj->remmina_file);
	//imagedir = g_get_user_special_dir(G_USER_DIRECTORY_PICTURES);
	/* TODO: Improve file name (DONE:8743571d) + give the user the option */
	pngdate = g_strdup_printf("%d-%d-%d-%d:%d:%f",
			g_date_time_get_year (date),
			g_date_time_get_month (date),
			g_date_time_get_day_of_month (date),
			g_date_time_get_hour (date),
			g_date_time_get_minute (date),
			g_date_time_get_seconds (date));

	g_date_time_unref (date);
	if (remminafile == NULL)
		remminafile = "remmina_screenshot";
	pngname = g_strdup_printf("%s/%s-%s.png", remmina_pref.screenshot_path,
			g_path_get_basename(remminafile), pngdate);

	cairo_surface_write_to_png(surface, pngname);

	/* send a desktop notification */
	remmina_public_send_notification ("remmina-screenshot-is-ready-id", _("Screenshot taken"), pngname);

	//Clean up and return.
	cairo_destroy(cr);
	cairo_surface_destroy(surface);


}

static void remmina_connection_holder_toolbar_minimize(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_minimize");
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
	gtk_window_iconify(GTK_WINDOW(cnnhld->cnnwin));
}

static void remmina_connection_holder_toolbar_disconnect(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_disconnect");
	remmina_connection_holder_disconnect_current_page(cnnhld);
}

static void remmina_connection_holder_toolbar_grab(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_grab");
	DECLARE_CNNOBJ
	gboolean capture;

	capture = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
	remmina_file_set_int(cnnobj->remmina_file, "keyboard_grab", capture);
	if (capture)
	{
#if DEBUG_KB_GRABBING
		printf("DEBUG_KB_GRABBING: Grabbing for button\n");
#endif
		remmina_connection_holder_keyboard_grab(cnnhld);
	}
	else
		remmina_connection_holder_keyboard_ungrab(cnnhld);
}

	static GtkWidget*
remmina_connection_holder_create_toolbar(RemminaConnectionHolder* cnnhld, gint mode)
{
	TRACE_CALL("remmina_connection_holder_create_toolbar");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* toolbar;
	GtkToolItem* toolitem;
	GtkWidget* widget;
	GtkWidget* arrow;

	toolbar = gtk_toolbar_new();
	gtk_widget_show(toolbar);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), FALSE);
	if (remmina_pref.small_toolbutton)
	{
		gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	}

	/* Auto-Fit */
	toolitem = gtk_tool_button_new(NULL, NULL);
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-fit-window");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Resize the window to fit in remote resolution"),
			remmina_pref.shortcutkey_autofit, 0);
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(remmina_connection_holder_toolbar_autofit), cnnhld);
	priv->toolitem_autofit = toolitem;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));

	/* Fullscreen toggle */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-fullscreen");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Toggle fullscreen mode"),
			remmina_pref.shortcutkey_fullscreen, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	priv->toolitem_fullscreen = toolitem;
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolitem), mode != SCROLLED_WINDOW_MODE);
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(remmina_connection_holder_toolbar_fullscreen), cnnhld);

	/* Fullscreen drop-down options */
	toolitem = gtk_tool_item_new();
	gtk_widget_show(GTK_WIDGET(toolitem));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	widget = gtk_toggle_button_new();
	gtk_widget_show(widget);
	gtk_container_set_border_width(GTK_CONTAINER(widget), 0);
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
#if GTK_CHECK_VERSION(3, 20, 0)
	gtk_widget_set_focus_on_click(GTK_WIDGET(widget), FALSE);
#else
	gtk_button_set_focus_on_click(GTK_BUTTON(widget), FALSE);
#endif
	if (remmina_pref.small_toolbutton)
	{
		gtk_widget_set_name(widget, "remmina-small-button");
	}
	gtk_container_add(GTK_CONTAINER(toolitem), widget);

#if GTK_CHECK_VERSION(3, 14, 0)
	arrow = gtk_image_new_from_icon_name("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
#else
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
#endif
	gtk_widget_show(arrow);
	gtk_container_add(GTK_CONTAINER(widget), arrow);
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_fullscreen_option), cnnhld);
	priv->fullscreen_option_button = widget;
	if (mode == SCROLLED_WINDOW_MODE)
	{
		gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
	}

	/* Switch tabs */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-switch-page");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Switch tab pages"), remmina_pref.shortcutkey_prevtab,
			remmina_pref.shortcutkey_nexttab);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_switch_page), cnnhld);
	priv->toolitem_switch_page = toolitem;

	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));

	/* Dynamic Resolution Update */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-dynres");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Toggle dynamic resolution update"),
			remmina_pref.shortcutkey_dynres, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_dynres), cnnhld);
	priv->toolitem_dynres = toolitem;

	/* Scaler button */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-scale");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Toggle scaled mode"), remmina_pref.shortcutkey_scale, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_scaled_mode), cnnhld);
	priv->toolitem_scale = toolitem;

	/* Scaler aspect ratio dropdown menu */
	toolitem = gtk_tool_item_new();
	gtk_widget_show(GTK_WIDGET(toolitem));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	widget = gtk_toggle_button_new();
	gtk_widget_show(widget);
	gtk_container_set_border_width(GTK_CONTAINER(widget), 0);
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
#if GTK_CHECK_VERSION(3, 20, 0)
	gtk_widget_set_focus_on_click(GTK_WIDGET(widget), FALSE);
#else
	gtk_button_set_focus_on_click(GTK_BUTTON(widget), FALSE);
#endif
	if (remmina_pref.small_toolbutton)
	{
		gtk_widget_set_name(widget, "remmina-small-button");
	}
	gtk_container_add(GTK_CONTAINER(toolitem), widget);
#if GTK_CHECK_VERSION(3, 14, 0)
	arrow = gtk_image_new_from_icon_name("pan-down-symbolic", GTK_ICON_SIZE_BUTTON);
#else
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
#endif
	gtk_widget_show(arrow);
	gtk_container_add(GTK_CONTAINER(widget), arrow);
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_scaler_option), cnnhld);
	priv->scaler_option_button = widget;

	/* Grab keyboard button */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "input-keyboard");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Grab all keyboard events"),
			remmina_pref.shortcutkey_grab, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_grab), cnnhld);
	priv->toolitem_grab = toolitem;

	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "preferences-system");
	gtk_tool_item_set_tooltip_text(toolitem, _("Preferences"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_preferences), cnnhld);
	priv->toolitem_preferences = toolitem;

	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "system-run");
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolitem), _("Tools"));
	gtk_tool_item_set_tooltip_text(toolitem, _("Tools"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_tools), cnnhld);
	priv->toolitem_tools = toolitem;

	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));

	toolitem = gtk_tool_button_new(NULL, "_Screenshot");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON (toolitem), "camera-photo");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Screenshot"), remmina_pref.shortcutkey_screenshot, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(remmina_connection_holder_toolbar_screenshot), cnnhld);

	toolitem = gtk_tool_button_new(NULL, "_Bottom");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON (toolitem), "go-bottom");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Minimize window"), remmina_pref.shortcutkey_minimize, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(remmina_connection_holder_toolbar_minimize), cnnhld);

	toolitem = gtk_tool_button_new(NULL, "_Disconnect");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON (toolitem), "gtk-disconnect");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Disconnect"), remmina_pref.shortcutkey_disconnect, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(remmina_connection_holder_toolbar_disconnect), cnnhld);

	return toolbar;
}

static void remmina_connection_holder_place_toolbar(GtkToolbar *toolbar, GtkGrid *grid, GtkWidget *sibling, int toolbar_placement)
{
	/* Place the toolbar inside the grid and set its orientation */

	if ( toolbar_placement == TOOLBAR_PLACEMENT_LEFT || toolbar_placement == TOOLBAR_PLACEMENT_RIGHT)
		gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), GTK_ORIENTATION_VERTICAL);
	else
		gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), GTK_ORIENTATION_HORIZONTAL);


	switch(toolbar_placement)
	{
		case TOOLBAR_PLACEMENT_TOP:
			gtk_widget_set_hexpand(GTK_WIDGET(toolbar), TRUE);
			gtk_widget_set_vexpand(GTK_WIDGET(toolbar), FALSE);
			gtk_grid_attach_next_to(GTK_GRID(grid), GTK_WIDGET(toolbar), sibling, GTK_POS_TOP, 1, 1);
			break;
		case TOOLBAR_PLACEMENT_RIGHT:
			gtk_widget_set_vexpand(GTK_WIDGET(toolbar), TRUE);
			gtk_widget_set_hexpand(GTK_WIDGET(toolbar), FALSE);
			gtk_grid_attach_next_to(GTK_GRID(grid), GTK_WIDGET(toolbar), sibling, GTK_POS_RIGHT, 1, 1);
			break;
		case TOOLBAR_PLACEMENT_BOTTOM:
			gtk_widget_set_hexpand(GTK_WIDGET(toolbar), TRUE);
			gtk_widget_set_vexpand(GTK_WIDGET(toolbar), FALSE);
			gtk_grid_attach_next_to(GTK_GRID(grid), GTK_WIDGET(toolbar), sibling, GTK_POS_BOTTOM, 1, 1);
			break;
		case TOOLBAR_PLACEMENT_LEFT:
			gtk_widget_set_vexpand(GTK_WIDGET(toolbar), TRUE);
			gtk_widget_set_hexpand(GTK_WIDGET(toolbar), FALSE);
			gtk_grid_attach_next_to(GTK_GRID(grid), GTK_WIDGET(toolbar), sibling, GTK_POS_LEFT, 1, 1);
			break;
	}

}

static void remmina_connection_holder_update_toolbar(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_update_toolbar");
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkToolItem* toolitem;
	gboolean bval, dynres_avail, scale_avail;
	gboolean test_floating_toolbar;
	RemminaScaleMode scalemode;

	remmina_connection_holder_update_toolbar_autofit_button(cnnhld);

	toolitem = priv->toolitem_switch_page;
	bval = (gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)) > 1);
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval);

	scalemode = get_current_allowed_scale_mode(cnnobj, &dynres_avail, &scale_avail);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->toolitem_dynres), dynres_avail);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->toolitem_scale), scale_avail);

	switch(scalemode) {
		case REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE:
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres), FALSE);
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), FALSE);
			break;
		case REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED:
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres), FALSE);
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), TRUE);
			break;
		case REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES:
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres), TRUE);
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale), FALSE);
			gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), FALSE);
			break;
	}

	toolitem = priv->toolitem_grab;
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolitem),
			remmina_file_get_int(cnnobj->remmina_file, "keyboard_grab", FALSE));

	toolitem = priv->toolitem_preferences;
	bval = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_PREF);
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval);

	toolitem = priv->toolitem_tools;
	bval = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_TOOL);
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval);

	gtk_window_set_title(GTK_WINDOW(cnnhld->cnnwin), remmina_file_get_string(cnnobj->remmina_file, "name"));

#if FLOATING_TOOLBAR_WIDGET
	test_floating_toolbar = (priv->floating_toolbar_widget != NULL);
#else
	test_floating_toolbar = (priv->floating_toolbar_window != NULL);
#endif
	if (test_floating_toolbar)
	{
		gtk_label_set_text(GTK_LABEL(priv->floating_toolbar_label),
				remmina_file_get_string(cnnobj->remmina_file, "name"));
	}

}

static void remmina_connection_holder_showhide_toolbar(RemminaConnectionHolder* cnnhld, gboolean resize)
{
	TRACE_CALL("remmina_connection_holder_showhide_toolbar");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	/* Here we should threat the resize flag, but we don't */
	if (priv->view_mode == SCROLLED_WINDOW_MODE)
	{
		if (remmina_pref.hide_connection_toolbar)
		{
			gtk_widget_hide(priv->toolbar);
		}
		else
		{
			gtk_widget_show(priv->toolbar);
		}
	}
}

static gboolean remmina_connection_holder_floating_toolbar_on_enter(GtkWidget* widget, GdkEventCrossing* event,
		RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_floating_toolbar_on_enter");
	remmina_connection_holder_floating_toolbar_show(cnnhld, TRUE);
	return TRUE;
}

static gboolean remmina_connection_object_enter_protocol_widget(GtkWidget* widget, GdkEventCrossing* event,
		RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_object_enter_protocol_widget");
	RemminaConnectionHolder* cnnhld = cnnobj->cnnhld;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	if (!priv->sticky && event->mode == GDK_CROSSING_NORMAL)
	{
		remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
		return TRUE;
	}
	return FALSE;
}

static void remmina_connection_window_focus_in(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_focus_in");

#if !FLOATING_TOOLBAR_WIDGET
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->floating_toolbar_window)
	{
		remmina_connection_holder_floating_toolbar_visible(cnnhld, TRUE);
	}
#endif

	remmina_connection_holder_keyboard_grab(cnnhld);
}

static void remmina_connection_window_focus_out(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_focus_out");
	DECLARE_CNNOBJ

#if !FLOATING_TOOLBAR_WIDGET
		RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
#endif

	remmina_connection_holder_keyboard_ungrab(cnnhld);
	cnnhld->hostkey_activated = FALSE;

#if !FLOATING_TOOLBAR_WIDGET
	if (!priv->sticky && priv->floating_toolbar_window)
	{
		remmina_connection_holder_floating_toolbar_visible(cnnhld, FALSE);
	}
#endif
	if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container))
	{
		remmina_scrolled_viewport_remove_motion(REMMINA_SCROLLED_VIEWPORT(cnnobj->scrolled_container));
	}
	remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, 0);

}

static gboolean remmina_connection_window_focus_out_event(GtkWidget* widget, GdkEvent* event, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_focus_out_event");
#if DEBUG_KB_GRABBING
	printf("DEBUG_KB_GRABBING: focus out and mouse_pointer_entered is %s\n", cnnhld->cnnwin->priv->mouse_pointer_entered ? "true":"false");
#endif
	remmina_connection_window_focus_out(widget, cnnhld);
	return FALSE;
}

static gboolean remmina_connection_window_focus_in_event(GtkWidget* widget, GdkEvent* event, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_focus_in_event");
#if DEBUG_KB_GRABBING
	printf("DEBUG_KB_GRABBING: focus in and mouse_pointer_entered is %s\n", cnnhld->cnnwin->priv->mouse_pointer_entered ? "true":"false");
#endif
	remmina_connection_window_focus_in(widget, cnnhld);
	return FALSE;
}

static gboolean remmina_connection_window_on_enter(GtkWidget* widget, GdkEventCrossing* event, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_on_enter");
	cnnhld->cnnwin->priv->mouse_pointer_entered = TRUE;
#if DEBUG_KB_GRABBING
	printf("DEBUG_KB_GRABBING: enter detail=");
	switch(event->detail) {
		case GDK_NOTIFY_ANCESTOR: printf("GDK_NOTIFY_ANCESTOR"); break;
		case GDK_NOTIFY_VIRTUAL: printf("GDK_NOTIFY_VIRTUAL"); break;
		case GDK_NOTIFY_NONLINEAR: printf("GDK_NOTIFY_NONLINEAR"); break;
		case GDK_NOTIFY_NONLINEAR_VIRTUAL: printf("GDK_NOTIFY_NONLINEAR_VIRTUAL"); break;
		case GDK_NOTIFY_UNKNOWN: printf("GDK_NOTIFY_UNKNOWN"); break;
		case GDK_NOTIFY_INFERIOR: printf("GDK_NOTIFY_INFERIOR"); break;
	}
	printf("\n");
#endif
	if (gtk_window_is_active(GTK_WINDOW(cnnhld->cnnwin)))
	{
		remmina_connection_holder_keyboard_grab(cnnhld);
	}
	return FALSE;
}


static gboolean remmina_connection_window_on_leave(GtkWidget* widget, GdkEventCrossing* event, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_on_leave");
#if DEBUG_KB_GRABBING
	printf("DEBUG_KB_GRABBING: leave detail=");
	switch(event->detail) {
		case GDK_NOTIFY_ANCESTOR: printf("GDK_NOTIFY_ANCESTOR"); break;
		case GDK_NOTIFY_VIRTUAL: printf("GDK_NOTIFY_VIRTUAL"); break;
		case GDK_NOTIFY_NONLINEAR: printf("GDK_NOTIFY_NONLINEAR"); break;
		case GDK_NOTIFY_NONLINEAR_VIRTUAL: printf("GDK_NOTIFY_NONLINEAR_VIRTUAL"); break;
		case GDK_NOTIFY_UNKNOWN: printf("GDK_NOTIFY_UNKNOWN"); break;
		case GDK_NOTIFY_INFERIOR: printf("GDK_NOTIFY_INFERIOR"); break;
	}
	printf("  x=%f y=%f\n", event->x, event->y);
	printf("  focus=%s\n", event->focus ? "yes":"no");
	printf("\n");
#endif
	/*
	 * Unity: we leave windows with GDK_NOTIFY_VIRTUAL or GDK_NOTIFY_NONLINEAR_VIRTUAL
	 * Gnome shell: we leave windows with both GDK_NOTIFY_VIRTUAL or GDK_NOTIFY_ANCESTOR
	 * Xfce: we cannot drag this window when grabbed, so we need to ungrab in response to GDK_NOTIFY_NONLINEAR
	 */
	if (event->detail == GDK_NOTIFY_VIRTUAL || event->detail == GDK_NOTIFY_ANCESTOR ||
		event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL || event->detail == GDK_NOTIFY_NONLINEAR) {
		cnnhld->cnnwin->priv->mouse_pointer_entered = FALSE;
		remmina_connection_holder_keyboard_ungrab(cnnhld);
	}
	return FALSE;
}

static gboolean
remmina_connection_holder_floating_toolbar_hide(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_floating_toolbar_hide");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	priv->hidetb_timer = 0;
	remmina_connection_holder_floating_toolbar_show (cnnhld, FALSE);
	return FALSE;
}

static gboolean remmina_connection_holder_floating_toolbar_on_scroll(GtkWidget* widget, GdkEventScroll* event,
		RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_floating_toolbar_on_scroll");
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)
	int opacity;

	opacity = remmina_file_get_int(cnnobj->remmina_file, "toolbar_opacity", 0);
	switch (event->direction)
	{
		case GDK_SCROLL_UP:
			if (opacity > 0)
			{
				remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity - 1);
				remmina_connection_holder_update_toolbar_opacity(cnnhld);
				return TRUE;
			}
			break;
		case GDK_SCROLL_DOWN:
			if (opacity < TOOLBAR_OPACITY_LEVEL)
			{
				remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity + 1);
				remmina_connection_holder_update_toolbar_opacity(cnnhld);
				return TRUE;
			}
			break;
#ifdef GDK_SCROLL_SMOOTH
		case GDK_SCROLL_SMOOTH:
			if (event->delta_y < 0 && opacity > 0)
			{
				remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity - 1);
				remmina_connection_holder_update_toolbar_opacity(cnnhld);
				return TRUE;
			}
			if (event->delta_y > 0 && opacity < TOOLBAR_OPACITY_LEVEL)
			{
				remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity + 1);
				remmina_connection_holder_update_toolbar_opacity(cnnhld);
				return TRUE;
			}
			break;
#endif
		default:
			break;
	}
	return FALSE;
}

static gboolean remmina_connection_window_after_configure_scrolled(gpointer user_data)
{
	TRACE_CALL("remmina_connection_window_after_configure_scrolled");
	gint width, height;
	GdkWindowState s;
	gint ipg, npages;
	RemminaConnectionObject* cnnobj;
	RemminaConnectionHolder* cnnhld;

	cnnhld = (RemminaConnectionHolder*)user_data;

	s = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)));

	if (!cnnhld || !cnnhld->cnnwin)
		return FALSE;

	/* Changed window_maximize, window_width and window_height for all
	 * connections inside the notebook */
	npages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook));
	for(ipg = 0; ipg < npages; ipg ++) {
		cnnobj = g_object_get_data(
			G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook), ipg)),
			"cnnobj");
		if (s & GDK_WINDOW_STATE_MAXIMIZED) {
			remmina_file_set_int(cnnobj->remmina_file, "window_maximize", TRUE);
		} else {
			gtk_window_get_size(GTK_WINDOW(cnnhld->cnnwin), &width, &height);
			remmina_file_set_int(cnnobj->remmina_file, "window_width", width);
			remmina_file_set_int(cnnobj->remmina_file, "window_height", height);
			remmina_file_set_int(cnnobj->remmina_file, "window_maximize", FALSE);
		}
	}
	cnnhld->cnnwin->priv->savestate_eventsourceid = 0;
	return FALSE;
}

static gboolean remmina_connection_window_on_configure(GtkWidget* widget, GdkEventConfigure* event,
		RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_on_configure");
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)
#if !FLOATING_TOOLBAR_WIDGET
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkRequisition req;
	gint y;
#endif

	if (cnnhld->cnnwin->priv->savestate_eventsourceid) {
		g_source_remove(cnnhld->cnnwin->priv->savestate_eventsourceid);
		cnnhld->cnnwin->priv->savestate_eventsourceid = 0;
	}

	if (cnnhld->cnnwin && gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin))
			&& cnnhld->cnnwin->priv->view_mode == SCROLLED_WINDOW_MODE)
	{
		/* Under gnome shell we receive this configure_event BEFORE a window
		 * is really unmaximized, so we must read its new state and dimensions
		 * later, not now */
		cnnhld->cnnwin->priv->savestate_eventsourceid = g_timeout_add(500, remmina_connection_window_after_configure_scrolled, cnnhld);
	}

#if !FLOATING_TOOLBAR_WIDGET
	if (priv->floating_toolbar_window)
	{

		gtk_widget_get_preferred_size(priv->floating_toolbar_window, &req, NULL);
		gtk_window_get_position(GTK_WINDOW(priv->floating_toolbar_window), NULL, &y);
		gtk_window_move(GTK_WINDOW(priv->floating_toolbar_window), event->x + MAX(0, (event->width - req.width) / 2), y);

		remmina_connection_holder_floating_toolbar_update(cnnhld);
	}
#endif

	if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
	{
		/* Notify window of change so that scroll border can be hidden or shown if needed */
		remmina_connection_holder_check_resize(cnnobj->cnnhld);
	}
	return FALSE;
}

static void remmina_connection_holder_update_pin(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_update_pin");
	if (cnnhld->cnnwin->priv->pin_down)
	{
		gtk_button_set_image(GTK_BUTTON(cnnhld->cnnwin->priv->pin_button),
				gtk_image_new_from_icon_name("remmina-pin-down", GTK_ICON_SIZE_MENU));
	}
	else
	{
		gtk_button_set_image(GTK_BUTTON(cnnhld->cnnwin->priv->pin_button),
				gtk_image_new_from_icon_name("remmina-pin-up", GTK_ICON_SIZE_MENU));
	}
}

static void remmina_connection_holder_toolbar_pin(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_toolbar_pin");
	remmina_pref.toolbar_pin_down = cnnhld->cnnwin->priv->pin_down = !cnnhld->cnnwin->priv->pin_down;
	remmina_pref_save();
	remmina_connection_holder_update_pin(cnnhld);
}

static void remmina_connection_holder_create_floating_toolbar(RemminaConnectionHolder* cnnhld, gint mode)
{
	TRACE_CALL("remmina_connection_holder_create_floating_toolbar");
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
#if FLOATING_TOOLBAR_WIDGET
	GtkWidget* ftb_widget;
#else
	GtkWidget* ftb_popup_window;
	GtkWidget* eventbox;
#endif
	GtkWidget* vbox;
	GtkWidget* hbox;
	GtkWidget* label;
	GtkWidget* pinbutton;
	GtkWidget* tb;


#if FLOATING_TOOLBAR_WIDGET
	/* A widget to be used for GtkOverlay for GTK >= 3.10 */
	ftb_widget = gtk_event_box_new();
#else
	/* A popup window for GTK < 3.10 */
	ftb_popup_window = gtk_window_new(GTK_WINDOW_POPUP);
#endif

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox);


#if FLOATING_TOOLBAR_WIDGET
	gtk_container_add(GTK_CONTAINER(ftb_widget), vbox);
#else
	gtk_container_add(GTK_CONTAINER(ftb_popup_window), vbox);
#endif

	tb = remmina_connection_holder_create_toolbar(cnnhld, mode);
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);


	/* The pin button */
	pinbutton = gtk_button_new();
	gtk_widget_show(pinbutton);
	gtk_box_pack_start(GTK_BOX(hbox), pinbutton, FALSE, FALSE, 0);
	gtk_button_set_relief(GTK_BUTTON(pinbutton), GTK_RELIEF_NONE);
#if GTK_CHECK_VERSION(3, 20, 0)
	gtk_widget_set_focus_on_click(GTK_WIDGET(pinbutton), FALSE);
#else
	gtk_button_set_focus_on_click(GTK_BUTTON(pinbutton), FALSE);
#endif
	gtk_widget_set_name(pinbutton, "remmina-pin-button");
	g_signal_connect(G_OBJECT(pinbutton), "clicked", G_CALLBACK(remmina_connection_holder_toolbar_pin), cnnhld);
	priv->pin_button = pinbutton;
	priv->pin_down = remmina_pref.toolbar_pin_down;
	remmina_connection_holder_update_pin(cnnhld);


	label = gtk_label_new(remmina_file_get_string(cnnobj->remmina_file, "name"));
	gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
	gtk_widget_show(label);

#if FLOATING_TOOLBAR_WIDGET
	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);
#else
	/* An event box is required to wrap the label to avoid infinite "leave-enter" event loop */
	eventbox = gtk_event_box_new();
	gtk_widget_show(eventbox);
	gtk_box_pack_start(GTK_BOX(hbox), eventbox, TRUE, TRUE, 0);
	gtk_container_add(GTK_CONTAINER(eventbox), label);
#endif

	priv->floating_toolbar_label = label;


#if FLOATING_TOOLBAR_WIDGET

	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM)
	{
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), tb, FALSE, FALSE, 0);
	}
	else
	{
		gtk_box_pack_start(GTK_BOX(vbox), tb, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	priv->floating_toolbar_widget = ftb_widget;
	if (cnnobj->connected)
		gtk_widget_show(ftb_widget);

#else

	gtk_box_pack_start(GTK_BOX(vbox), tb, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* The position will be moved in configure event instead during maximizing. Just make it invisible here */
	gtk_window_move(GTK_WINDOW(ftb_popup_window), 0, 6000);
	gtk_window_set_accept_focus(GTK_WINDOW(ftb_popup_window), FALSE);

	priv->floating_toolbar_window = ftb_popup_window;

	remmina_connection_holder_update_toolbar_opacity(cnnhld);
	if (remmina_pref.fullscreen_toolbar_visibility == FLOATING_TOOLBAR_VISIBILITY_INVISIBLE && !priv->pin_down)
	{
#if GTK_CHECK_VERSION(3, 8, 0)
		gtk_widget_set_opacity(GTK_WIDGET(ftb_popup_window), 0.0);
#else
		gtk_window_set_opacity(GTK_WINDOW(ftb_popup_window), 0.0);
#endif
	}

	if (cnnobj->connected)
		gtk_widget_show(ftb_popup_window);
#endif
}

static void remmina_connection_window_toolbar_place_signal(RemminaConnectionWindow* cnnwin, gpointer data)
{
	TRACE_CALL("remmina_connection_window_toolbar_place_signal");
	RemminaConnectionWindowPriv* priv;

	priv = cnnwin->priv;
	/* Detach old toolbar widget and reattach in new position in the grid */
	if (priv->toolbar && priv->grid) {
		g_object_ref(priv->toolbar);
		gtk_container_remove(GTK_CONTAINER(priv->grid), priv->toolbar);
		remmina_connection_holder_place_toolbar(GTK_TOOLBAR(priv->toolbar), GTK_GRID(priv->grid), priv->notebook, remmina_pref.toolbar_placement);
		g_object_unref(priv->toolbar);
	}
}


static void remmina_connection_window_init(RemminaConnectionWindow* cnnwin)
{
	TRACE_CALL("remmina_connection_window_init");
	RemminaConnectionWindowPriv* priv;

	priv = g_new0(RemminaConnectionWindowPriv, 1);
	cnnwin->priv = priv;

	priv->view_mode = AUTO_MODE;
	priv->floating_toolbar_opacity = 1.0;
	priv->kbcaptured = FALSE;
	priv->mouse_pointer_entered = FALSE;

	gtk_container_set_border_width(GTK_CONTAINER(cnnwin), 0);

	remmina_widget_pool_register(GTK_WIDGET(cnnwin));

	g_signal_connect(G_OBJECT(cnnwin), "toolbar-place", G_CALLBACK(remmina_connection_window_toolbar_place_signal), NULL);
}

static gboolean remmina_connection_window_state_event(GtkWidget* widget, GdkEventWindowState* event, gpointer user_data)
{
	TRACE_CALL("remmina_connection_window_state_event");

	if (event->changed_mask & GDK_WINDOW_STATE_FOCUSED) {
		if (event->new_window_state & GDK_WINDOW_STATE_FOCUSED)
			remmina_connection_window_focus_in(widget, user_data);
		else
			remmina_connection_window_focus_out(widget, user_data);
	}

#ifdef ENABLE_MINIMIZE_TO_TRAY
	GdkScreen* screen;

	screen = gdk_screen_get_default();
	if (remmina_pref.minimize_to_tray && (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) != 0
			&& (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) != 0
			&& remmina_public_get_current_workspace(screen)
			== remmina_public_get_window_workspace(GTK_WINDOW(widget))
			&& gdk_screen_get_number(screen) == gdk_screen_get_number(gtk_widget_get_screen(widget)))
	{
		gtk_widget_hide(widget);
		return TRUE;
	}
#endif
	return FALSE; // moved here because a function should return a value. Should be correct
}

	static GtkWidget*
remmina_connection_window_new_from_holder(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_new_from_holder");
	RemminaConnectionWindow* cnnwin;

	cnnwin = REMMINA_CONNECTION_WINDOW(g_object_new(REMMINA_TYPE_CONNECTION_WINDOW, NULL));
	cnnwin->priv->cnnhld = cnnhld;
	cnnwin->priv->on_delete_confirm_mode = REMMINA_CONNECTION_WINDOW_ONDELETE_CONFIRM_IF_2_OR_MORE;

	g_signal_connect(G_OBJECT(cnnwin), "delete-event", G_CALLBACK(remmina_connection_window_delete_event), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "destroy", G_CALLBACK(remmina_connection_window_destroy), cnnhld);

	/* focus-in-event and focus-out-event don't work when keyboard is grabbed
	 * via gdk_device_grab. So we listen for window-state-event to detect focus in and focus out */
	g_signal_connect(G_OBJECT(cnnwin), "window-state-event", G_CALLBACK(remmina_connection_window_state_event), cnnhld);

	g_signal_connect(G_OBJECT(cnnwin), "focus-in-event", G_CALLBACK(remmina_connection_window_focus_in_event), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "focus-out-event", G_CALLBACK(remmina_connection_window_focus_out_event), cnnhld);

	g_signal_connect(G_OBJECT(cnnwin), "enter-notify-event", G_CALLBACK(remmina_connection_window_on_enter), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "leave-notify-event", G_CALLBACK(remmina_connection_window_on_leave), cnnhld);

	g_signal_connect(G_OBJECT(cnnwin), "configure_event", G_CALLBACK(remmina_connection_window_on_configure), cnnhld);

	return GTK_WIDGET(cnnwin);
}

/* This function will be called for the first connection. A tag is set to the window so that
 * other connections can determine if whether a new tab should be append to the same window
 */
static void remmina_connection_window_update_tag(RemminaConnectionWindow* cnnwin, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_window_update_tag");
	gchar* tag;

	switch (remmina_pref.tab_mode)
	{
		case REMMINA_TAB_BY_GROUP:
			tag = g_strdup(remmina_file_get_string(cnnobj->remmina_file, "group"));
			break;
		case REMMINA_TAB_BY_PROTOCOL:
			tag = g_strdup(remmina_file_get_string(cnnobj->remmina_file, "protocol"));
			break;
		default:
			tag = NULL;
			break;
	}
	g_object_set_data_full(G_OBJECT(cnnwin), "tag", tag, (GDestroyNotify) g_free);
}

static void remmina_connection_object_create_scrolled_container(RemminaConnectionObject* cnnobj, gint view_mode)
{
	TRACE_CALL("remmina_connection_object_create_scrolled_container");
	GtkWidget* container;

	if (view_mode == VIEWPORT_FULLSCREEN_MODE)
	{
		container = remmina_scrolled_viewport_new();
	}
	else
	{
		container = gtk_scrolled_window_new(NULL, NULL);
		remmina_connection_object_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(container));
		gtk_container_set_border_width(GTK_CONTAINER(container), 0);
		gtk_widget_set_can_focus(container, FALSE);
	}

	gtk_widget_set_name(container, "remmina-scrolled-container");

	g_object_set_data(G_OBJECT(container), "cnnobj", cnnobj);
	gtk_widget_show(container);
	cnnobj->scrolled_container = container;

	g_signal_connect(G_OBJECT(cnnobj->proto), "enter-notify-event", G_CALLBACK(remmina_connection_object_enter_protocol_widget), cnnobj);

}

static void remmina_connection_holder_grab_focus(GtkNotebook *notebook)
{
	TRACE_CALL("remmina_connection_holder_grab_focus");
	RemminaConnectionObject* cnnobj;
	GtkWidget* child;

	child = gtk_notebook_get_nth_page(notebook, gtk_notebook_get_current_page(notebook));
	cnnobj = g_object_get_data(G_OBJECT(child), "cnnobj");
	if (GTK_IS_WIDGET(cnnobj->proto))
	{
		remmina_protocol_widget_grab_focus(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	}
}

static void remmina_connection_object_on_close_button_clicked(GtkButton* button, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_object_on_close_button_clicked");
	if (REMMINA_IS_PROTOCOL_WIDGET(cnnobj->proto))
	{
		remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	}
}

static GtkWidget* remmina_connection_object_create_tab(RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_object_create_tab");
	GtkWidget* hbox;
	GtkWidget* widget;
	GtkWidget* button;


	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
	gtk_widget_show(hbox);

	widget = gtk_image_new_from_icon_name(remmina_file_get_icon_name(cnnobj->remmina_file), GTK_ICON_SIZE_MENU);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);

	widget = gtk_label_new(remmina_file_get_string(cnnobj->remmina_file, "name"));
	gtk_widget_set_valign(widget, GTK_ALIGN_CENTER);
	gtk_widget_set_halign(widget, GTK_ALIGN_CENTER);

	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

	button = gtk_button_new();	// The "x" to close the tab
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
#if GTK_CHECK_VERSION(3, 20, 0)
	gtk_widget_set_focus_on_click(GTK_WIDGET(widget), FALSE);
#else
	gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
#endif
	gtk_widget_set_name(button, "remmina-small-button");
	gtk_widget_show(button);

	widget = gtk_image_new_from_icon_name("window-close", GTK_ICON_SIZE_MENU);
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(button), widget);

	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);

	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(remmina_connection_object_on_close_button_clicked), cnnobj);

	return hbox;
}

static gint remmina_connection_object_append_page(RemminaConnectionObject* cnnobj, GtkNotebook* notebook, GtkWidget* tab,
		gint view_mode)
{
	TRACE_CALL("remmina_connection_object_append_page");
	gint i;

	remmina_connection_object_create_scrolled_container(cnnobj, view_mode);
	i = gtk_notebook_append_page(notebook, cnnobj->scrolled_container, tab);
	gtk_notebook_set_tab_reorderable(notebook, cnnobj->scrolled_container, TRUE);
	gtk_notebook_set_tab_detachable(notebook, cnnobj->scrolled_container, TRUE);
	/* This trick prevents the tab label from being focused */
	gtk_widget_set_can_focus(gtk_widget_get_parent(tab), FALSE);
	return i;
}

static void remmina_connection_window_initialize_notebook(GtkNotebook* to, GtkNotebook* from, RemminaConnectionObject* cnnobj,
		gint view_mode)
{
	TRACE_CALL("remmina_connection_window_initialize_notebook");
	gint i, n, c;
	GtkWidget* tab;
	GtkWidget* widget;
	RemminaConnectionObject* tc;

	if (cnnobj)
	{
		/* Search cnnobj in the "from" notebook */
		tc = NULL;
		if (from) {
			n = gtk_notebook_get_n_pages(from);
			for (i = 0; i < n; i++)
			{
				widget = gtk_notebook_get_nth_page(from, i);
				tc = (RemminaConnectionObject*) g_object_get_data(G_OBJECT(widget), "cnnobj");
				if (tc == cnnobj)
					break;
			}
		}
		if (tc) {
			/* if cnnobj is already in the "from" notebook, we should be in the drag and drop case.
			 * just... do not move it. GTK will do the move when the create-window signal
			 * of GtkNotebook will return */

		} else {
			/* cnnobj is not on the "from" notebook. This is a new connection for a newly created window */
			tab = remmina_connection_object_create_tab(cnnobj);
			remmina_connection_object_append_page(cnnobj, to, tab, view_mode);

			G_GNUC_BEGIN_IGNORE_DEPRECATIONS
				gtk_widget_reparent(cnnobj->viewport, cnnobj->scrolled_container);
			G_GNUC_END_IGNORE_DEPRECATIONS

				if (cnnobj->window)
				{
					gtk_widget_destroy(cnnobj->window);
					cnnobj->window = NULL;
				}
		}
	}
	else
	{
		/* cnnobj=null: migrate all existing connections to the new notebook */
		if (from != NULL && GTK_IS_NOTEBOOK(from))
		{
			c = gtk_notebook_get_current_page(from);
			n = gtk_notebook_get_n_pages(from);
			for (i = 0; i < n; i++)
			{
				widget = gtk_notebook_get_nth_page(from, i);
				tc = (RemminaConnectionObject*) g_object_get_data(G_OBJECT(widget), "cnnobj");

				tab = remmina_connection_object_create_tab(tc);
				remmina_connection_object_append_page(tc, to, tab, view_mode);

				/* Reparent cnnobj->viewport */
				G_GNUC_BEGIN_IGNORE_DEPRECATIONS
					gtk_widget_reparent(tc->viewport, tc->scrolled_container);
				G_GNUC_END_IGNORE_DEPRECATIONS
			}
			gtk_notebook_set_current_page(to, c);
		}
	}
}

static void remmina_connection_holder_update_notebook(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_update_notebook");
	GtkNotebook* notebook;
	gint n;

	if (!cnnhld->cnnwin)
		return;

	notebook = GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook);

	switch (cnnhld->cnnwin->priv->view_mode)
	{
		case SCROLLED_WINDOW_MODE:
			n = gtk_notebook_get_n_pages(notebook);
			gtk_notebook_set_show_tabs(notebook, remmina_pref.always_show_tab ? TRUE : n > 1);
			gtk_notebook_set_show_border(notebook, remmina_pref.always_show_tab ? TRUE : n > 1);
			break;
		default:
			gtk_notebook_set_show_tabs(notebook, FALSE);
			gtk_notebook_set_show_border(notebook, FALSE);
			break;
	}
}

static gboolean remmina_connection_holder_on_switch_page_real(gpointer data)
{
	TRACE_CALL("remmina_connection_holder_on_switch_page_real");
	RemminaConnectionHolder* cnnhld = (RemminaConnectionHolder*) data;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (GTK_IS_WIDGET(cnnhld->cnnwin))
	{
		remmina_connection_holder_floating_toolbar_show (cnnhld, TRUE);
		if (!priv->hidetb_timer)
			priv->hidetb_timer = g_timeout_add(TB_HIDE_TIME_TIME, (GSourceFunc)
					remmina_connection_holder_floating_toolbar_hide, cnnhld);
		remmina_connection_holder_update_toolbar(cnnhld);
		remmina_connection_holder_grab_focus(GTK_NOTEBOOK(priv->notebook));
		if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
		{
			remmina_connection_holder_check_resize(cnnhld);
		}

	}
	priv->switch_page_handler = 0;
	return FALSE;
}

static void remmina_connection_holder_on_switch_page(GtkNotebook* notebook, GtkWidget* page, guint page_num,
		RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_on_switch_page");
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (!priv->switch_page_handler)
	{
		priv->switch_page_handler = g_idle_add(remmina_connection_holder_on_switch_page_real, cnnhld);
	}
}

static void remmina_connection_holder_on_page_added(GtkNotebook* notebook, GtkWidget* child, guint page_num,
		RemminaConnectionHolder* cnnhld)
{
	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)) > 0)
		remmina_connection_holder_update_notebook(cnnhld);
}

static void remmina_connection_holder_on_page_removed(GtkNotebook* notebook, GtkWidget* child, guint page_num,
		RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_on_page_removed");

	if (!cnnhld->cnnwin)
		return;

	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)) <= 0)
	{
		gtk_widget_destroy(GTK_WIDGET(cnnhld->cnnwin));
		cnnhld->cnnwin = NULL;
	}

}

	GtkNotebook*
remmina_connection_holder_on_notebook_create_window(GtkNotebook* notebook, GtkWidget* page, gint x, gint y, gpointer data)
{
	/* This signal callback is called by GTK when a detachable tab is dropped on the root window */

	TRACE_CALL("remmina_connection_holder_on_notebook_create_window");
	RemminaConnectionWindow* srccnnwin;
	RemminaConnectionWindow* dstcnnwin;
	RemminaConnectionObject* cnnobj;
	gint srcpagenum;
	GdkWindow* window;

#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkDevice* device = NULL;

#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(gdk_display_get_default());
	device = gdk_seat_get_pointer(seat);
#else
	manager = gdk_display_get_device_manager(gdk_display_get_default());
	device = gdk_device_manager_get_client_pointer(manager);
#endif

	window = gdk_device_get_window_at_position(device, &x, &y);
	srccnnwin = REMMINA_CONNECTION_WINDOW(gtk_widget_get_toplevel(GTK_WIDGET(notebook)));
	dstcnnwin = REMMINA_CONNECTION_WINDOW(remmina_widget_pool_find_by_window(REMMINA_TYPE_CONNECTION_WINDOW, window));

	if (srccnnwin == dstcnnwin)
		return NULL;

	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(srccnnwin->priv->notebook)) == 1 && !dstcnnwin)
		return NULL;

	cnnobj = (RemminaConnectionObject*) g_object_get_data(G_OBJECT(page), "cnnobj");
	srcpagenum = gtk_notebook_page_num(GTK_NOTEBOOK(srccnnwin->priv->notebook), cnnobj->scrolled_container);

	if (dstcnnwin)
	{
		cnnobj->cnnhld = dstcnnwin->priv->cnnhld;
	}
	else
	{
		cnnobj->cnnhld = g_new0(RemminaConnectionHolder, 1);
		if (!cnnobj->cnnhld->cnnwin)
		{
			/* Create a new scrolled window to accomodate the dropped connection
			 * and move our cnnobj there */
			cnnobj->cnnhld->cnnwin = srccnnwin;
			remmina_connection_holder_create_scrolled(cnnobj->cnnhld, cnnobj);
		}
	}

	remmina_protocol_widget_set_hostkey_func(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			(RemminaHostkeyFunc) remmina_connection_window_hostkey_func, cnnobj->cnnhld);

	return GTK_NOTEBOOK(cnnobj->cnnhld->cnnwin->priv->notebook);
}

	static GtkWidget*
remmina_connection_holder_create_notebook(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_create_notebook");
	GtkWidget* notebook;

	notebook = gtk_notebook_new();

	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	gtk_widget_show(notebook);

	g_signal_connect(G_OBJECT(notebook), "create-window", G_CALLBACK(remmina_connection_holder_on_notebook_create_window),
			cnnhld);
	g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(remmina_connection_holder_on_switch_page), cnnhld);
	g_signal_connect(G_OBJECT(notebook), "page-added", G_CALLBACK(remmina_connection_holder_on_page_added), cnnhld);
	g_signal_connect(G_OBJECT(notebook), "page-removed", G_CALLBACK(remmina_connection_holder_on_page_removed), cnnhld);
	gtk_widget_set_can_focus(notebook, FALSE);

	return notebook;
}

/* Create a scrolled window container */
static void remmina_connection_holder_create_scrolled(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_holder_create_scrolled");
	GtkWidget* window;
	GtkWidget* oldwindow;
	GtkWidget* grid;
	GtkWidget* toolbar;
	GtkWidget* notebook;
	GList *chain;
	gchar* tag;
	int newwin_width, newwin_height;

	oldwindow = GTK_WIDGET(cnnhld->cnnwin);
	window = remmina_connection_window_new_from_holder(cnnhld);
	gtk_widget_realize(window);
	cnnhld->cnnwin = REMMINA_CONNECTION_WINDOW(window);


	newwin_width = newwin_height = 100;
	if (cnnobj) {
		/* If we have a cnnobj as a reference for this window, we can setup its default size here */
		newwin_width = remmina_file_get_int (cnnobj->remmina_file, "window_width", 640);
		newwin_height = remmina_file_get_int (cnnobj->remmina_file, "window_height", 480);
	} else {
		/* Try to get a temporary RemminaConnectionObject from the old window and get
		 * a remmina_file and width/height */
		int np;
		GtkWidget* page;
		RemminaConnectionObject* oldwindow_currentpage_cnnobj;

		np = gtk_notebook_get_current_page(GTK_NOTEBOOK (REMMINA_CONNECTION_WINDOW(oldwindow)->priv->notebook));
		page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (REMMINA_CONNECTION_WINDOW(oldwindow)->priv->notebook), np);
		oldwindow_currentpage_cnnobj = (RemminaConnectionObject*) g_object_get_data(G_OBJECT(page),"cnnobj");
		newwin_width = remmina_file_get_int (oldwindow_currentpage_cnnobj->remmina_file, "window_width", 640);
		newwin_height = remmina_file_get_int (oldwindow_currentpage_cnnobj->remmina_file, "window_height", 480);
	}

	gtk_window_set_default_size (GTK_WINDOW(cnnhld->cnnwin), newwin_width, newwin_height);

	/* Create the toolbar */
	toolbar = remmina_connection_holder_create_toolbar(cnnhld, SCROLLED_WINDOW_MODE);

	/* Create the notebook */
	notebook = remmina_connection_holder_create_notebook(cnnhld);

	/* Create the grid container for toolbars+notebook and populate it */
	grid = gtk_grid_new();
	gtk_grid_attach(GTK_GRID(grid), notebook, 0, 0, 1, 1);

	gtk_widget_set_hexpand(notebook, TRUE);
	gtk_widget_set_vexpand(notebook, TRUE);

	remmina_connection_holder_place_toolbar(GTK_TOOLBAR(toolbar), GTK_GRID(grid), notebook, remmina_pref.toolbar_placement);


	gtk_container_add(GTK_CONTAINER(window), grid);

	chain = g_list_append(NULL, notebook);
	gtk_container_set_focus_chain(GTK_CONTAINER(grid), chain);
	g_list_free(chain);

	/* Add drag capabilities to the toolbar */
	gtk_drag_source_set(GTK_WIDGET(toolbar), GDK_BUTTON1_MASK,
			dnd_targets_tb, sizeof dnd_targets_tb / sizeof *dnd_targets_tb, GDK_ACTION_MOVE);
	g_signal_connect_after(GTK_WIDGET(toolbar), "drag-begin", G_CALLBACK(remmina_connection_window_tb_drag_begin), cnnhld);
	g_signal_connect(GTK_WIDGET(toolbar), "drag-failed", G_CALLBACK(remmina_connection_window_tb_drag_failed), cnnhld);

	/* Add drop capabilities to the drop/dest target for the toolbar (the notebook) */
	gtk_drag_dest_set(GTK_WIDGET(notebook), GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
			dnd_targets_tb, sizeof dnd_targets_tb / sizeof *dnd_targets_tb, GDK_ACTION_MOVE);
	gtk_drag_dest_set_track_motion(GTK_WIDGET(notebook), TRUE);
	g_signal_connect(GTK_WIDGET(notebook), "drag-drop", G_CALLBACK(remmina_connection_window_tb_drag_drop), cnnhld);

	cnnhld->cnnwin->priv->view_mode = SCROLLED_WINDOW_MODE;
	cnnhld->cnnwin->priv->toolbar = toolbar;
	cnnhld->cnnwin->priv->grid = grid;
	cnnhld->cnnwin->priv->notebook = notebook;

	/* The notebook and all its child must be realized now, or a reparent will
	 * call unrealize() and will destroy a GtkSocket */
	gtk_widget_show(grid);
	gtk_widget_show(GTK_WIDGET(cnnhld->cnnwin));

	remmina_connection_window_initialize_notebook(GTK_NOTEBOOK(notebook),
			(oldwindow ? GTK_NOTEBOOK(REMMINA_CONNECTION_WINDOW(oldwindow)->priv->notebook) : NULL), cnnobj,
			SCROLLED_WINDOW_MODE);

	if (cnnobj)
	{
		if (!oldwindow)
			remmina_connection_window_update_tag(cnnhld->cnnwin, cnnobj);

		if (remmina_file_get_int(cnnobj->remmina_file, "window_maximize", FALSE))
		{
			gtk_window_maximize(GTK_WINDOW(cnnhld->cnnwin));
		}
	}

	if (oldwindow)
	{
		tag = g_strdup((gchar*) g_object_get_data(G_OBJECT(oldwindow), "tag"));
		g_object_set_data_full(G_OBJECT(cnnhld->cnnwin), "tag", tag, (GDestroyNotify) g_free);
		if(!cnnobj)
			gtk_widget_destroy(oldwindow);
	}

	remmina_connection_holder_update_toolbar(cnnhld);
	remmina_connection_holder_showhide_toolbar(cnnhld, FALSE);
	remmina_connection_holder_check_resize(cnnhld);


}

static gboolean remmina_connection_window_go_fullscreen(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	TRACE_CALL("remmina_connection_window_go_fullscreen");
	RemminaConnectionHolder* cnnhld;
	RemminaConnectionWindowPriv* priv;

	cnnhld = (RemminaConnectionHolder*)data;
	priv = cnnhld->cnnwin->priv;

#if GTK_CHECK_VERSION(3, 18, 0)
	if (remmina_pref.fullscreen_on_auto)
	{
		gtk_window_fullscreen_on_monitor(GTK_WINDOW(cnnhld->cnnwin),
				gdk_screen_get_default (),
				gdk_screen_get_monitor_at_window
				(gdk_screen_get_default (), gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin))
				));
	} else {
		remmina_log_print("Fullscreen managed by WM or by the user, as per settings");
		gtk_window_fullscreen(GTK_WINDOW(cnnhld->cnnwin));
	}
#else
	remmina_log_print("Cannot fullscreen on a specific monitor, feature available from GTK 3.18");
	gtk_window_fullscreen(GTK_WINDOW(cnnhld->cnnwin));
#endif
	return FALSE;
}

#if FLOATING_TOOLBAR_WIDGET

static void remmina_connection_holder_create_overlay_ftb_overlay(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_holder_create_overlay_ftb_overlay");

	GtkWidget* revealer;
	RemminaConnectionWindowPriv* priv;
	priv = cnnhld->cnnwin->priv;

	if (priv->overlay_ftb_overlay != NULL)
	{
		gtk_widget_destroy(priv->overlay_ftb_overlay);
		priv->overlay_ftb_overlay = NULL;
		priv->revealer = NULL;
	}

	remmina_connection_holder_create_floating_toolbar(cnnhld, cnnhld->fullscreen_view_mode);
	remmina_connection_holder_update_toolbar(cnnhld);

	priv->overlay_ftb_overlay = gtk_event_box_new();

	GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),0);

	GtkWidget* handle = gtk_drawing_area_new();
	gtk_widget_set_size_request(handle, 4, 4);
	gtk_widget_set_name(handle, "ftb-handle");

	revealer = gtk_revealer_new();

	gtk_widget_set_halign(GTK_WIDGET(priv->overlay_ftb_overlay), GTK_ALIGN_CENTER);

	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM)
	{
		gtk_box_pack_start(GTK_BOX(vbox), handle, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), revealer, FALSE, FALSE, 0);
		gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
		gtk_widget_set_valign(GTK_WIDGET(priv->overlay_ftb_overlay), GTK_ALIGN_END);
	}
	else
	{
		gtk_box_pack_start(GTK_BOX(vbox), revealer, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), handle, FALSE, FALSE, 0);
		gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_DOWN);
		gtk_widget_set_valign(GTK_WIDGET(priv->overlay_ftb_overlay), GTK_ALIGN_START);
	}


	gtk_container_add(GTK_CONTAINER(revealer), priv->floating_toolbar_widget);
	gtk_widget_set_halign(GTK_WIDGET(revealer), GTK_ALIGN_CENTER);
	gtk_widget_set_valign(GTK_WIDGET(revealer), GTK_ALIGN_START);

	priv->revealer = revealer;

	GtkWidget *fr;
	fr = gtk_frame_new(NULL);
	gtk_container_add(GTK_CONTAINER(priv->overlay_ftb_overlay), fr );
	gtk_container_add(GTK_CONTAINER(fr), vbox);

	gtk_widget_show(vbox);
	gtk_widget_show(revealer);
	gtk_widget_show(handle);
	gtk_widget_show(priv->overlay_ftb_overlay);
	gtk_widget_show(fr);

	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM)
	{
		gtk_widget_set_name(fr, "ftbbox-lower");
	}
	else
	{
		gtk_widget_set_name(fr, "ftbbox-upper");
	}

	gtk_overlay_add_overlay(GTK_OVERLAY(priv->overlay), priv->overlay_ftb_overlay);

	remmina_connection_holder_floating_toolbar_show(cnnhld, TRUE);

	g_signal_connect(G_OBJECT(priv->overlay_ftb_overlay), "enter-notify-event", G_CALLBACK(remmina_connection_holder_floating_toolbar_on_enter), cnnhld);
	g_signal_connect(G_OBJECT(priv->overlay_ftb_overlay), "scroll-event", G_CALLBACK(remmina_connection_holder_floating_toolbar_on_scroll), cnnhld);
	gtk_widget_add_events(GTK_WIDGET(priv->overlay_ftb_overlay), GDK_SCROLL_MASK);

	/* Add drag and drop capabilities to the source */
	gtk_drag_source_set(GTK_WIDGET(priv->overlay_ftb_overlay), GDK_BUTTON1_MASK,
			dnd_targets_ftb, sizeof dnd_targets_ftb / sizeof *dnd_targets_ftb, GDK_ACTION_MOVE);
	g_signal_connect_after(GTK_WIDGET(priv->overlay_ftb_overlay), "drag-begin", G_CALLBACK(remmina_connection_window_ftb_drag_begin), cnnhld);
}


static gboolean remmina_connection_window_ftb_drag_drop(GtkWidget *widget, GdkDragContext *context,
		gint x, gint y, guint time, gpointer user_data)
{
	TRACE_CALL("remmina_connection_window_ftb_drag_drop");
	GtkAllocation wa;
	gint new_floating_toolbar_placement;
	RemminaConnectionHolder* cnnhld;
	RemminaConnectionWindowPriv* priv;

	cnnhld = (RemminaConnectionHolder*)user_data;
	priv = cnnhld->cnnwin->priv;


	gtk_widget_get_allocation(widget, &wa);

	if (y >= wa.height / 2)
	{
		new_floating_toolbar_placement = FLOATING_TOOLBAR_PLACEMENT_BOTTOM;
	}
	else
	{
		new_floating_toolbar_placement = FLOATING_TOOLBAR_PLACEMENT_TOP;
	}

	gtk_drag_finish(context, TRUE, TRUE, time);

	if (new_floating_toolbar_placement !=  remmina_pref.floating_toolbar_placement)
	{
		/* Destroy and recreate the FTB */
		remmina_pref.floating_toolbar_placement = new_floating_toolbar_placement;
		remmina_pref_save();
		remmina_connection_holder_create_overlay_ftb_overlay(cnnhld);
	}

	return TRUE;

}

static void remmina_connection_window_ftb_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
	TRACE_CALL("remmina_connection_window_ftb_drag_begin");

	RemminaConnectionHolder* cnnhld;
	RemminaConnectionWindowPriv* priv;
	cairo_surface_t *surface;
	cairo_t *cr;
	GtkAllocation wa;
	double dashes[] = { 10 };

	cnnhld = (RemminaConnectionHolder*)user_data;
	priv = cnnhld->cnnwin->priv;

	gtk_widget_get_allocation(widget, &wa);

	surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, wa.width, wa.height);
	cr = cairo_create(surface);
	cairo_set_source_rgb(cr, 0.6, 0.6, 0.6);
	cairo_set_line_width(cr, 2);
	cairo_set_dash(cr, dashes, 1, 0 );
	cairo_rectangle(cr, 0, 0, wa.width, wa.height);
	cairo_stroke(cr);
	cairo_destroy(cr);

	gtk_drag_set_icon_surface(context, surface);

}


#endif

static void remmina_connection_holder_create_fullscreen(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj,
		gint view_mode)
{
	TRACE_CALL("remmina_connection_holder_create_fullscreen");
	GtkWidget* window;
	GtkWidget* oldwindow;
	GtkWidget* notebook;
	RemminaConnectionWindowPriv* priv;

	gchar* tag;

	oldwindow = GTK_WIDGET(cnnhld->cnnwin);
	window = remmina_connection_window_new_from_holder(cnnhld);
	gtk_widget_set_name(GTK_WIDGET(window), "remmina-connection-window-fullscreen");
	gtk_widget_realize(window);

	cnnhld->cnnwin = REMMINA_CONNECTION_WINDOW(window);
	priv = cnnhld->cnnwin->priv;

	if (!view_mode)
		view_mode = VIEWPORT_FULLSCREEN_MODE;

	notebook = remmina_connection_holder_create_notebook(cnnhld);

#if FLOATING_TOOLBAR_WIDGET
	priv->overlay = gtk_overlay_new();
	gtk_container_add(GTK_CONTAINER(window), priv->overlay);
	gtk_container_add(GTK_CONTAINER(priv->overlay), notebook);
	gtk_widget_show(GTK_WIDGET(priv->overlay));
#else
	gtk_container_add(GTK_CONTAINER(window), notebook);
#endif

	priv->notebook = notebook;
	priv->view_mode = view_mode;
	cnnhld->fullscreen_view_mode = view_mode;

	remmina_connection_window_initialize_notebook(GTK_NOTEBOOK(notebook),
			(oldwindow ? GTK_NOTEBOOK(REMMINA_CONNECTION_WINDOW(oldwindow)->priv->notebook) : NULL), cnnobj,
			view_mode);

	if (cnnobj)
	{
		remmina_connection_window_update_tag(cnnhld->cnnwin, cnnobj);
	}
	if (oldwindow)
	{
		tag = g_strdup((gchar*) g_object_get_data(G_OBJECT(oldwindow), "tag"));
		g_object_set_data_full(G_OBJECT(cnnhld->cnnwin), "tag", tag, (GDestroyNotify) g_free);
		gtk_widget_destroy(oldwindow);
	}

	/* Create the floating toolbar */
#if FLOATING_TOOLBAR_WIDGET
	if (remmina_pref.fullscreen_toolbar_visibility != FLOATING_TOOLBAR_VISIBILITY_DISABLE)
	{
		remmina_connection_holder_create_overlay_ftb_overlay(cnnhld);
		/* Add drag and drop capabilities to the drop/dest target for floating toolbar */
		gtk_drag_dest_set(GTK_WIDGET(priv->overlay), GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
				dnd_targets_ftb, sizeof dnd_targets_ftb / sizeof *dnd_targets_ftb, GDK_ACTION_MOVE);
		gtk_drag_dest_set_track_motion(GTK_WIDGET(priv->notebook), TRUE);
		g_signal_connect(GTK_WIDGET(priv->overlay), "drag-drop", G_CALLBACK(remmina_connection_window_ftb_drag_drop), cnnhld);
	}
#else
	if (remmina_pref.fullscreen_toolbar_visibility != FLOATING_TOOLBAR_VISIBILITY_DISABLE)
	{
		remmina_connection_holder_create_floating_toolbar(cnnhld, view_mode);
		remmina_connection_holder_update_toolbar(cnnhld);

		g_signal_connect(G_OBJECT(priv->floating_toolbar_window), "enter-notify-event", G_CALLBACK(remmina_connection_holder_floating_toolbar_on_enter), cnnhld);
		g_signal_connect(G_OBJECT(priv->floating_toolbar_window), "scroll-event", G_CALLBACK(remmina_connection_holder_floating_toolbar_on_scroll), cnnhld);
		gtk_widget_add_events(GTK_WIDGET(priv->floating_toolbar_window), GDK_SCROLL_MASK);
	}
#endif

	remmina_connection_holder_check_resize(cnnhld);

	gtk_widget_show(window);

	/* Put the window in fullscreen after it is mapped to have it appear on the same monitor */
	g_signal_connect(G_OBJECT(window), "map-event", G_CALLBACK(remmina_connection_window_go_fullscreen), (gpointer)cnnhld);
}

static gboolean remmina_connection_window_hostkey_func(RemminaProtocolWidget* gp, guint keyval, gboolean release,
		RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL("remmina_connection_window_hostkey_func");
	DECLARE_CNNOBJ_WITH_RETURN(FALSE);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	const RemminaProtocolFeature* feature;
	gint i;

	if (release)
	{
		if (remmina_pref.hostkey && keyval == remmina_pref.hostkey)
		{
			cnnhld->hostkey_activated = FALSE;
			if (cnnhld->hostkey_used)
			{
				/* hostkey pressed + something else */
				return TRUE;
			}
		}
		/* If hostkey is released without pressing other keys, we should execute the
		 * shortcut key which is the same as hostkey. Be default, this is grab/ungrab
		 * keyboard */
		else if (cnnhld->hostkey_activated)
		{
			/* Trap all key releases when hostkey is pressed */
			/* hostkey pressed + something else */
			return TRUE;

		}
		else
		{
			/* Any key pressed, no hostkey */
			return FALSE;
		}
	}
	else if (remmina_pref.hostkey && keyval == remmina_pref.hostkey)
	{
		/* TODO: Add callback for hostname transparent overlay #832 */
		cnnhld->hostkey_activated = TRUE;
		cnnhld->hostkey_used = FALSE;
		return TRUE;
	}
	else if (!cnnhld->hostkey_activated)
	{
		/* Any key pressed, no hostkey */
		return FALSE;
	}

	cnnhld->hostkey_used = TRUE;
	keyval = gdk_keyval_to_lower(keyval);
        if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down
            || keyval == GDK_KEY_Left || keyval == GDK_KEY_Right) {
            DECLARE_CNNOBJ_WITH_RETURN(FALSE);
            GtkAdjustment *adjust;
            int pos;

            if (cnnobj->connected && GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container)) {
                if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down)
                    adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container));
                else
                    adjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container));

                if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Left)
                    pos = 0;
                else
                    pos = gtk_adjustment_get_upper(adjust);

                gtk_adjustment_set_value (adjust, pos);
                if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down)
                    gtk_scrolled_window_set_vadjustment (GTK_SCROLLED_WINDOW(cnnobj->scrolled_container), adjust);
                else
                    gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW(cnnobj->scrolled_container), adjust);
            }
            else if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container)) {
                RemminaScrolledViewport *gsv;
                GtkWidget *child;
                GdkWindow *gsvwin;
                gint sz;
                GtkAdjustment *adj;
                gdouble value;

                if (!GTK_IS_BIN(cnnobj->scrolled_container))
                    return FALSE;

                gsv = REMMINA_SCROLLED_VIEWPORT(cnnobj->scrolled_container);
                child = gtk_bin_get_child(GTK_BIN(gsv));
                if (!GTK_IS_VIEWPORT(child))
                    return FALSE;

                gsvwin = gtk_widget_get_window(GTK_WIDGET(gsv));
                if (!gsv)
                    return FALSE;

                if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down) {
                    sz = gdk_window_get_height(gsvwin) + 2;	// Add 2px of black scroll border
                    adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(child));
                }
                else {
                    sz = gdk_window_get_width(gsvwin) + 2;	// Add 2px of black scroll border
                    adj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(child));
                }

                if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Left) {
                    value = 0;
                }
                else {
                    value = gtk_adjustment_get_upper(GTK_ADJUSTMENT(adj)) - (gdouble) sz + 2.0;
                }

                gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), value);
            }
        }

	if (keyval == remmina_pref.shortcutkey_fullscreen)
	{
		switch (priv->view_mode)
		{
			case SCROLLED_WINDOW_MODE:
				remmina_connection_holder_create_fullscreen(
						cnnhld,
						NULL,
						cnnhld->fullscreen_view_mode ?
						cnnhld->fullscreen_view_mode : VIEWPORT_FULLSCREEN_MODE);
				break;
			case SCROLLED_FULLSCREEN_MODE:
			case VIEWPORT_FULLSCREEN_MODE:
				remmina_connection_holder_create_scrolled(cnnhld, NULL);
				break;
			default:
				break;
		}
	}
	else if (keyval == remmina_pref.shortcutkey_autofit)
	{
		if (priv->toolitem_autofit && gtk_widget_is_sensitive(GTK_WIDGET(priv->toolitem_autofit)))
		{
			remmina_connection_holder_toolbar_autofit(GTK_WIDGET(gp), cnnhld);
		}
	}
	else if (keyval == remmina_pref.shortcutkey_nexttab)
	{
		i = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)) + 1;
		if (i >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)))
			i = 0;
		gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), i);
	}
	else if (keyval == remmina_pref.shortcutkey_prevtab)
	{
		i = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)) - 1;
		if (i < 0)
			i = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)) - 1;
		gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), i);
	}
	else if (keyval == remmina_pref.shortcutkey_scale)
	{
		if (gtk_widget_is_sensitive(GTK_WIDGET(priv->toolitem_scale)))
		{
			gtk_toggle_tool_button_set_active(
					GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale),
					!gtk_toggle_tool_button_get_active(
						GTK_TOGGLE_TOOL_BUTTON(
							priv->toolitem_scale)));
		}
	}
	else if (keyval == remmina_pref.shortcutkey_grab)
	{
		gtk_toggle_tool_button_set_active(
				GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_grab),
				!gtk_toggle_tool_button_get_active(
					GTK_TOGGLE_TOOL_BUTTON(
						priv->toolitem_grab)));
	}
	else if (keyval == remmina_pref.shortcutkey_minimize)
	{
		remmina_connection_holder_toolbar_minimize(GTK_WIDGET(gp),
				cnnhld);
	}
	else if (keyval == remmina_pref.shortcutkey_viewonly)
	{
		remmina_file_set_int(cnnobj->remmina_file, "viewonly",
				( remmina_file_get_int(cnnobj->remmina_file, "viewonly", 0 )
				 == 0  ) ? 1 : 0 );
	}
	else if (keyval == remmina_pref.shortcutkey_screenshot)
	{
		remmina_connection_holder_toolbar_screenshot(GTK_WIDGET(gp),
				cnnhld);
	}
	else if (keyval == remmina_pref.shortcutkey_disconnect)
	{
		remmina_connection_holder_disconnect_current_page(cnnhld);
	}
	else if (keyval == remmina_pref.shortcutkey_toolbar)
	{
		if (priv->view_mode == SCROLLED_WINDOW_MODE)
		{
			remmina_pref.hide_connection_toolbar =
				!remmina_pref.hide_connection_toolbar;
			remmina_connection_holder_showhide_toolbar( cnnhld, TRUE);
		}
	}
	else
	{
		for (feature =
				remmina_protocol_widget_get_features(
					REMMINA_PROTOCOL_WIDGET(
						cnnobj->proto));
				feature && feature->type;
				feature++)
		{
			if (feature->type
					== REMMINA_PROTOCOL_FEATURE_TYPE_TOOL
					&& GPOINTER_TO_UINT(
						feature->opt3)
					== keyval)
			{
				remmina_protocol_widget_call_feature_by_ref(
						REMMINA_PROTOCOL_WIDGET(
							cnnobj->proto),
						feature);
				break;
			}
		}
	}
	cnnhld->hostkey_activated = FALSE;
	/* Trap all key presses when hostkey is pressed */
	return TRUE;
}

static RemminaConnectionWindow* remmina_connection_window_find(RemminaFile* remminafile)
{
	TRACE_CALL("remmina_connection_window_find");
	const gchar* tag;

	switch (remmina_pref.tab_mode)
	{
		case REMMINA_TAB_BY_GROUP:
			tag = remmina_file_get_string(remminafile, "group");
			break;
		case REMMINA_TAB_BY_PROTOCOL:
			tag = remmina_file_get_string(remminafile, "protocol");
			break;
		case REMMINA_TAB_ALL:
			tag = NULL;
			break;
		case REMMINA_TAB_NONE:
		default:
			return NULL;
	}
	return REMMINA_CONNECTION_WINDOW(remmina_widget_pool_find(REMMINA_TYPE_CONNECTION_WINDOW, tag));
}

static void remmina_connection_object_on_connect(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_object_on_connect");
	RemminaConnectionWindow* cnnwin;
	RemminaConnectionHolder* cnnhld;
	GtkWidget* tab;
	gint i;

	/* This signal handler is called by a plugin where it's correctly connected
	 * (and authenticated) */

	if (!cnnobj->cnnhld)
	{
		cnnwin = remmina_connection_window_find(cnnobj->remmina_file);
		if (cnnwin)
		{
			cnnhld = cnnwin->priv->cnnhld;
		}
		else
		{
			cnnhld = g_new0(RemminaConnectionHolder, 1);
		}
		cnnobj->cnnhld = cnnhld;
	}
	else
	{
		cnnhld = cnnobj->cnnhld;
	}

	cnnobj->connected = TRUE;

	remmina_protocol_widget_set_hostkey_func(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			(RemminaHostkeyFunc) remmina_connection_window_hostkey_func, cnnhld);

	/* Remember recent list for quick connect */
	if (remmina_file_get_filename(cnnobj->remmina_file) == NULL)
	{
		remmina_pref_add_recent(remmina_file_get_string(cnnobj->remmina_file, "protocol"),
				remmina_file_get_string(cnnobj->remmina_file, "server"));
	}

	/* Save credentials */
	remmina_file_save(cnnobj->remmina_file);

	if (!cnnhld->cnnwin)
	{
		i = remmina_file_get_int(cnnobj->remmina_file, "viewmode", 0);
		switch (i)
		{
			case SCROLLED_FULLSCREEN_MODE:
			case VIEWPORT_FULLSCREEN_MODE:
				remmina_connection_holder_create_fullscreen(cnnhld, cnnobj, i);
				break;
			case SCROLLED_WINDOW_MODE:
			default:
				remmina_connection_holder_create_scrolled(cnnhld, cnnobj);
				break;
		}
	}
	else
	{
		tab = remmina_connection_object_create_tab(cnnobj);
		i = remmina_connection_object_append_page(cnnobj, GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook), tab,
				cnnhld->cnnwin->priv->view_mode);
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			gtk_widget_reparent(cnnobj->viewport, cnnobj->scrolled_container);
		G_GNUC_END_IGNORE_DEPRECATIONS

			gtk_window_present(GTK_WINDOW(cnnhld->cnnwin));
		gtk_notebook_set_current_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook), i);
	}

#if FLOATING_TOOLBAR_WIDGET
	if (cnnhld->cnnwin->priv->floating_toolbar_widget)
	{
		gtk_widget_show(cnnhld->cnnwin->priv->floating_toolbar_widget);
	}
#else
	if (cnnhld->cnnwin->priv->floating_toolbar_window)
	{
		gtk_widget_show(cnnhld->cnnwin->priv->floating_toolbar_window);
	}
#endif

}

static void cb_autoclose_widget(GtkWidget *widget)
{
	gtk_widget_destroy(widget);
	remmina_application_condexit(REMMINA_CONDEXIT_ONDISCONNECT);
}

static void remmina_connection_object_on_disconnect(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_object_on_disconnect");
	RemminaConnectionHolder* cnnhld = cnnobj->cnnhld;
	GtkWidget* dialog;
	GtkWidget* pparent;


	/* Detach the protocol widget from the notebook now, or we risk that a
	 * window delete will destroy cnnobj->proto before we complete disconnection.
	 */
	pparent = gtk_widget_get_parent(cnnobj->proto);
	if (pparent != NULL)
	{
		g_object_ref(cnnobj->proto);
		gtk_container_remove(GTK_CONTAINER(pparent), cnnobj->proto);
	}

	cnnobj->connected = FALSE;

	if (cnnhld && remmina_pref.save_view_mode)
	{
		if (cnnhld->cnnwin)
		{
			remmina_file_set_int(cnnobj->remmina_file, "viewmode", cnnhld->cnnwin->priv->view_mode);
		}
		remmina_file_save(cnnobj->remmina_file);
	}

	if (remmina_protocol_widget_has_error(gp))
	{
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				remmina_protocol_widget_get_error_message(gp), NULL);
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(cb_autoclose_widget), NULL);
		gtk_widget_show(dialog);
		remmina_widget_pool_register(dialog);
	}

	if (cnnhld && cnnhld->cnnwin && cnnobj->scrolled_container)
	{
		gtk_notebook_remove_page(
				GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook),
				gtk_notebook_page_num(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook),
					cnnobj->scrolled_container));
	}

	cnnobj->remmina_file = NULL;
	g_free(cnnobj);

	remmina_application_condexit(REMMINA_CONDEXIT_ONDISCONNECT);
}

static void remmina_connection_object_on_desktop_resize(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_object_on_desktop_resize");
	if (cnnobj->cnnhld && cnnobj->cnnhld->cnnwin && cnnobj->cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
	{
		remmina_connection_holder_check_resize(cnnobj->cnnhld);
	}
}

static void remmina_connection_object_on_update_align(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_object_on_update_align");
	remmina_protocol_widget_update_alignment(cnnobj);
}

static void remmina_connection_object_on_unlock_dynres(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL("remmina_connection_object_on_update_align");
	cnnobj->dynres_unlocked = TRUE;
	remmina_connection_holder_update_toolbar(cnnobj->cnnhld);
}

gboolean remmina_connection_window_open_from_filename(const gchar* filename)
{
	TRACE_CALL("remmina_connection_window_open_from_filename");
	RemminaFile* remminafile;
	GtkWidget* dialog;

	remminafile = remmina_file_manager_load_file(filename);
	if (remminafile)
	{
		remmina_connection_window_open_from_file(remminafile);
		return TRUE;
	}
	else
	{
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
				_("File %s not found."), filename);
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(dialog);
		remmina_widget_pool_register(dialog);
		return FALSE;
	}
}

void remmina_connection_window_open_from_file(RemminaFile* remminafile)
{
	TRACE_CALL("remmina_connection_window_open_from_file");
	remmina_connection_window_open_from_file_full(remminafile, NULL, NULL, NULL);
}

GtkWidget* remmina_connection_window_open_from_file_full(RemminaFile* remminafile, GCallback disconnect_cb, gpointer data, guint* handler)
{
	TRACE_CALL("remmina_connection_window_open_from_file_full");
	RemminaConnectionObject* cnnobj;
	GtkWidget* protocolwidget;

	cnnobj = g_new0(RemminaConnectionObject, 1);
	cnnobj->remmina_file = remminafile;

	/* Exec precommand before everything else */
	remmina_plugin_cmdexec_new(cnnobj->remmina_file, "precommand");

	/* Create the RemminaProtocolWidget */
	protocolwidget = cnnobj->proto = remmina_protocol_widget_new();

	/* Set default remote desktop size in the profile, so the plugins can query
	 * protocolwidget and know WxH that the user put on the profile settings */
	remmina_protocol_widget_update_remote_resolution((RemminaProtocolWidget*)protocolwidget,
		remmina_file_get_int(remminafile, "resolution_width", -1),
		remmina_file_get_int(remminafile, "resolution_height", -1)
	);

	/* Set a name for the widget, for CSS selector */
	gtk_widget_set_name(GTK_WIDGET(cnnobj->proto),"remmina-protocol-widget");

	gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto),GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto),GTK_ALIGN_FILL);

	if (data)
	{
		g_object_set_data(G_OBJECT(cnnobj->proto), "user-data", data);
	}

	gtk_widget_show(cnnobj->proto);
	g_signal_connect(G_OBJECT(cnnobj->proto), "connect", G_CALLBACK(remmina_connection_object_on_connect), cnnobj);
	if (disconnect_cb)
	{
		*handler = g_signal_connect(G_OBJECT(cnnobj->proto), "disconnect", disconnect_cb, data);
	}
	g_signal_connect(G_OBJECT(cnnobj->proto), "disconnect", G_CALLBACK(remmina_connection_object_on_disconnect), cnnobj);
	g_signal_connect(G_OBJECT(cnnobj->proto), "desktop-resize", G_CALLBACK(remmina_connection_object_on_desktop_resize),
			cnnobj);
	g_signal_connect(G_OBJECT(cnnobj->proto), "update-align", G_CALLBACK(remmina_connection_object_on_update_align),
			cnnobj);
	g_signal_connect(G_OBJECT(cnnobj->proto), "unlock-dynres", G_CALLBACK(remmina_connection_object_on_unlock_dynres),
			cnnobj);

	/* Create the viewport to make the RemminaProtocolWidget scrollable */
	cnnobj->viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_set_name(cnnobj->viewport, "remmina-cw-viewport");
	gtk_widget_show(cnnobj->viewport);
	gtk_container_set_border_width(GTK_CONTAINER(cnnobj->viewport), 0);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(cnnobj->viewport), GTK_SHADOW_NONE);

	/* Determine whether the plugin can scale or not. If the plugin can scale and we do
	 * not want to expand, then we add a GtkAspectFrame to maintain aspect ratio during scaling */
	cnnobj->plugin_can_scale = remmina_plugin_manager_query_feature_by_type(REMMINA_PLUGIN_TYPE_PROTOCOL,
			remmina_file_get_string(remminafile, "protocol"),
			REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);

	cnnobj->aspectframe = NULL;
	gtk_container_add(GTK_CONTAINER(cnnobj->viewport), cnnobj->proto);

	cnnobj->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize(cnnobj->window);
	gtk_container_add(GTK_CONTAINER(cnnobj->window), cnnobj->viewport);

	if (!remmina_pref.save_view_mode)
		remmina_file_set_int(cnnobj->remmina_file, "viewmode", remmina_pref.default_mode);

	remmina_protocol_widget_open_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), remminafile);

	return protocolwidget;
}

void remmina_connection_window_set_delete_confirm_mode(RemminaConnectionWindow* cnnwin, RemminaConnectionWindowOnDeleteConfirmMode mode)
{
	cnnwin->priv->on_delete_confirm_mode = mode;
}
