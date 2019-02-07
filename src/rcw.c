/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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

#include "remmina.h"
#include "rcw.h"
#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_message_panel.h"
#include "remmina_ext_exec.h"
#include "remmina_plugin_manager.h"
#include "remmina_pref.h"
#include "remmina_protocol_widget.h"
#include "remmina_public.h"
#include "remmina_scrolled_viewport.h"
#include "remmina_utils.h"
#include "remmina_widget_pool.h"
#include "remmina_log.h"
#include "remmina/remmina_trace_calls.h"

#ifdef GDK_WINDOWING_WAYLAND
	#include <gdk/gdkwayland.h>
#endif


#define DEBUG_KB_GRABBING 0
#include "remmina_exec.h"

gchar *remmina_pref_file;
RemminaPref remmina_pref;

G_DEFINE_TYPE( RemminaConnectionWindow, rcw, GTK_TYPE_WINDOW)

#define MOTION_TIME 100

/* default timeout used to hide the floating toolbar wen switching profile */
#define TB_HIDE_TIME_TIME 1000

typedef struct _RemminaConnectionHolder RemminaConnectionHolder;

struct _RemminaConnectionWindowPriv {
	RemminaConnectionHolder* cnnhld;

	GtkWidget* notebook;

	guint switch_page_handler;

	GtkWidget* floating_toolbar_widget;
	GtkWidget* overlay;
	GtkWidget* revealer;
	GtkWidget* overlay_ftb_overlay;

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

	/* flag to disable toolbar signal handling when toolbar is
	 * reconfiguring, usually due to a tab switch */
	gboolean toolbar_is_reconfiguring;

	gint view_mode;

	gboolean kbcaptured;
	gboolean mouse_pointer_entered;

	RemminaConnectionWindowOnDeleteConfirmMode on_delete_confirm_mode;

};

typedef struct _RemminaConnectionObject {
	RemminaConnectionHolder* cnnhld;

	RemminaFile* remmina_file;

	GtkWidget* proto;
	GtkWidget* aspectframe;
	GtkWidget* viewport;

	GtkWidget* page;
	GtkWidget* scrolled_container;

	gboolean plugin_can_scale;

	gboolean connected;
	gboolean dynres_unlocked;


	gulong deferred_open_size_allocate_handler;

} RemminaConnectionObject;

struct _RemminaConnectionHolder {
	RemminaConnectionWindow* cnnwin;
	gint fullscreen_view_mode;
	gint grab_retry_eventsourceid;

	gboolean hostkey_activated;
	gboolean hostkey_used;

};

enum {
	TOOLBARPLACE_SIGNAL,
	LAST_SIGNAL
};

static guint rcw_signals[LAST_SIGNAL] =
{ 0 };

#define DECLARE_CNNOBJ \
	if (!cnnhld || !cnnhld->cnnwin || gtk_notebook_get_current_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)) < 0) return; \
	RemminaConnectionObject* cnnobj = (RemminaConnectionObject*)g_object_get_data( \
	G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook), \
			gtk_notebook_get_current_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)))), "cnnobj");

#define DECLARE_CNNOBJ_WITH_RETURN(r) \
	if (!cnnhld || !cnnhld->cnnwin || gtk_notebook_get_current_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)) < 0) return r; \
	RemminaConnectionObject* cnnobj = (RemminaConnectionObject*)g_object_get_data( \
	G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook), \
			gtk_notebook_get_current_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)))), "cnnobj");

void rch_create_scrolled(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj);
void rch_create_fullscreen(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj,
							gint view_mode);
static gboolean rcw_hostkey_func(RemminaProtocolWidget* gp, guint keyval, gboolean release);

void rch_grab_focus(GtkNotebook *notebook);
static GtkWidget* rch_create_toolbar(RemminaConnectionHolder* cnnhld, gint mode);
void rch_place_toolbar(GtkToolbar *toolbar, GtkGrid *grid, GtkWidget *sibling, int toolbar_placement);
void rch_keyboard_grab(RemminaConnectionHolder* cnnhld);

static void rcw_ftb_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data);

static const GtkTargetEntry dnd_targets_ftb[] =
{
	{
		(char*)"text/x-remmina-ftb",
		GTK_TARGET_SAME_APP | GTK_TARGET_OTHER_WIDGET,
		0
	},
};

static const GtkTargetEntry dnd_targets_tb[] =
{
	{
		(char*)"text/x-remmina-tb",
		GTK_TARGET_SAME_APP,
		0
	},
};

static void rcw_class_init(RemminaConnectionWindowClass* klass)
{
	TRACE_CALL(__func__);
	GtkCssProvider  *provider;

	provider = gtk_css_provider_new();

	/* It’s important to remove padding, border and shadow from GtkViewport or
	 * we will never know its internal area size, because GtkViweport::viewport_get_view_allocation,
	 * which returns the internal size of the GtkViewport, is private and we cannot access it */

#if GTK_CHECK_VERSION(3, 14, 0)
	gtk_css_provider_load_from_data(provider,
		"#remmina-cw-viewport, #remmina-cw-aspectframe {\n"
		"  padding:0;\n"
		"  border:0;\n"
		"  background-color: black;\n"
		"}\n"
		"GtkDrawingArea {\n"
		"}\n"
		"GtkToolbar {\n"
		"  -GtkWidget-window-dragging: 0;\n"
		"}\n"
		"#remmina-connection-window-fullscreen {\n"
		"  border-color: black;\n"
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
		"#remmina-tab-page {\n"
		"  background-color: black;\n"
		"}\n"
		"#remmina-scrolled-container {\n"
		"}\n"
		"#remmina-scrolled-container.undershoot {\n"
		"  background: none;\n"
		"}\n"
		"#remmina-tab-page {\n"
		"}\n"
		"#ftbbox-upper {\n"
		"  background-color: white;\n"
		"  color: black;\n"
		"  border-style: none solid solid solid;\n"
		"  border-width: 1px;\n"
		"  border-radius: 4px;\n"
		"  padding: 0px;\n"
		"}\n"
		"#ftbbox-lower {\n"
		"  background-color: white;\n"
		"  color: black;\n"
		"  border-style: solid solid none solid;\n"
		"  border-width: 1px;\n"
		"  border-radius: 4px;\n"
		"  padding: 0px;\n"
		"}\n"
		"#ftb-handle {\n"
		"}\n"
		".message_panel {\n"
		"  border: 0px solid;\n"
		"  padding: 20px 20px 20px 20px;\n"
		"}\n"
		".message_panel entry {\n"
		"  background-image: none;\n"
		"  border-width: 4px;\n"
		"  border-radius: 8px;\n"
		"}\n"
		".message_panel .title_label {\n"
		"  font-size: 2em; \n"
		"}\n"
		, -1, NULL);

#else
	gtk_css_provider_load_from_data(provider,
		"#remmina-cw-viewport, #remmina-cw-aspectframe {\n"
		"  padding:0;\n"
		"  border:0;\n"
		"  background-color: black;\n"
		"}\n"
		"#remmina-cw-message-panel {\n"
		"}\n"
		"GtkDrawingArea {\n"
		"}\n"
		"GtkToolbar {\n"
		"  -GtkWidget-window-dragging: 0;\n"
		"}\n"
		"#remmina-connection-window-fullscreen {\n"
		"  border-color: black;\n"
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
		"}\n"
		"#remmina-scrolled-container.undershoot {\n"
		"  background: none\n"
		"}\n"
		"#remmina-tab-page {\n"
		"}\n"
		"#ftbbox-upper {\n"
		"  border-style: none solid solid solid;\n"
		"  border-width: 1px;\n"
		"  border-radius: 4px;\n"
		"  padding: 0px;\n"
		"}\n"
		"#ftbbox-lower {\n"
		"  border-style: solid solid none solid;\n"
		"  border-width: 1px;\n"
		"  border-radius: 4px;\n"
		"  padding: 0px;\n"
		"}\n"
		"#ftb-handle {\n"
		"}\n"

		, -1, NULL);
#endif

	gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
		GTK_STYLE_PROVIDER(provider),
		GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

	g_object_unref(provider);

	/* Define a signal used to notify all rcws of toolbar move */
	rcw_signals[TOOLBARPLACE_SIGNAL] = g_signal_new("toolbar-place", G_TYPE_FROM_CLASS(klass),
		G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaConnectionWindowClass, toolbar_place), NULL, NULL,
		g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);

}

static RemminaScaleMode get_current_allowed_scale_mode(RemminaConnectionObject* cnnobj, gboolean *dynres_avail, gboolean *scale_avail)
{
	TRACE_CALL(__func__);
	RemminaScaleMode scalemode;
	gboolean plugin_has_dynres, plugin_can_scale;

	scalemode = remmina_protocol_widget_get_current_scale_mode(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));

	plugin_has_dynres = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
		REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);

	plugin_can_scale = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
		REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);

	/* forbid scalemode REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES when not possible */
	if ((!plugin_has_dynres) && scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES)
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

void rch_disconnect_current_page(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ

	/* Disconnects the connection which is currently in view in the notebook */

	remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
}

void rch_keyboard_ungrab(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	GdkDisplay *display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkDevice *keyboard = NULL;

	if (cnnhld->grab_retry_eventsourceid) {
		g_source_remove(cnnhld->grab_retry_eventsourceid);
		cnnhld->grab_retry_eventsourceid = 0;
	}

	display = gtk_widget_get_display(GTK_WIDGET(cnnhld->cnnwin));
#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(display);
	keyboard = gdk_seat_get_pointer(seat);
#else
	manager = gdk_display_get_device_manager(display);
	keyboard = gdk_device_manager_get_client_pointer(manager);
#endif

	if (!cnnhld->cnnwin->priv->kbcaptured) {
		return;
	}

	if (keyboard != NULL) {
		if ( gdk_device_get_source(keyboard) != GDK_SOURCE_KEYBOARD ) {
			keyboard = gdk_device_get_associated_device(keyboard);
		}
#if DEBUG_KB_GRABBING
		printf("DEBUG_KB_GRABBING: --- ungrabbing\n");
#endif

#if GTK_CHECK_VERSION(3, 24, 0)
		/* We can use gtk_seat_grab()/_ungrab() only after GTK 3.24 */
		gdk_seat_ungrab(seat);
#else
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
		gdk_device_ungrab(keyboard, GDK_CURRENT_TIME);
		G_GNUC_END_IGNORE_DEPRECATIONS
#endif
		cnnhld->cnnwin->priv->kbcaptured = FALSE;

	}
}

gboolean rch_keyboard_grab_retry(gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaConnectionHolder* cnnhld;
	cnnhld = (RemminaConnectionHolder *)user_data;

	rch_keyboard_grab(cnnhld);
	cnnhld->grab_retry_eventsourceid = 0;
	return G_SOURCE_REMOVE;
}

void rch_keyboard_grab(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	GdkDisplay *display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkGrabStatus ggs;
	GdkDevice *keyboard = NULL;

	if (cnnhld->cnnwin->priv->kbcaptured || !cnnhld->cnnwin->priv->mouse_pointer_entered) {
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

	if (keyboard != NULL) {

		if ( gdk_device_get_source(keyboard) != GDK_SOURCE_KEYBOARD) {
			keyboard = gdk_device_get_associated_device( keyboard );
		}

		if (remmina_file_get_int(cnnobj->remmina_file, "keyboard_grab", FALSE)) {
#if DEBUG_KB_GRABBING
			printf("DEBUG_KB_GRABBING: profile asks for grabbing, let’s try.\n");
#endif
	/* Up to GTK version 3.20 we can grab the keyboard with gdk_device_grab().
	 * in GTK 3.20 gdk_seat_grab() should be used instead of gdk_device_grab().
	 * There is a bug in GTK up to 3.22: when gdk_device_grab() fails
	 * the widget is hidden:
	 * https://gitlab.gnome.org/GNOME/gtk/commit/726ad5a5ae7c4f167e8dd454cd7c250821c400ab
	 * The bug fix will be released with GTK 3.24.
	 * Also pease note that the newer gdk_seat_grab() is still calling gdk_device_grab().
	 */
#if GTK_CHECK_VERSION(3, 24, 0)
			ggs = gdk_seat_grab(seat, gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)),
				GDK_SEAT_CAPABILITY_KEYBOARD, FALSE, NULL, NULL, NULL, NULL);
#else
			G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			ggs = gdk_device_grab(keyboard, gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)), GDK_OWNERSHIP_WINDOW,
				    TRUE, GDK_KEY_PRESS | GDK_KEY_RELEASE, NULL, GDK_CURRENT_TIME);
			G_GNUC_END_IGNORE_DEPRECATIONS
#endif
			if ( ggs != GDK_GRAB_SUCCESS )
			{
				/* Failure to GRAB keyboard */
				#if DEBUG_KB_GRABBING
					printf("GRAB FAILED. GdkGrabStatus: %d\n", (int)ggs);
				#endif
				/* Reschedule grabbing in half a second if not already done */
				if (cnnhld->grab_retry_eventsourceid == 0) {
					cnnhld->grab_retry_eventsourceid = g_timeout_add(500, (GSourceFunc)rch_keyboard_grab_retry, cnnhld);
				}
			} else {
			#if DEBUG_KB_GRABBING
				printf("GRAB SUCCESS\n");
			#endif
				if (cnnhld->grab_retry_eventsourceid != 0) {
					g_source_remove(cnnhld->grab_retry_eventsourceid);
					cnnhld->grab_retry_eventsourceid = 0;
				}
				cnnhld->cnnwin->priv->kbcaptured = TRUE;
			}
		}else {
			rch_keyboard_ungrab(cnnhld);
		}
	}
}

static void rcw_close_all_connections(RemminaConnectionWindow* cnnwin)
{
	RemminaConnectionWindowPriv* priv = cnnwin->priv;
	GtkNotebook* notebook = GTK_NOTEBOOK(priv->notebook);
	GtkWidget* w;
	RemminaConnectionObject* cnnobj;
	gint i, n;

	if (GTK_IS_WIDGET(notebook)) {
		n = gtk_notebook_get_n_pages(notebook);
		for (i = n - 1; i >= 0; i--) {
			w = gtk_notebook_get_nth_page(notebook, i);
			cnnobj = (RemminaConnectionObject*)g_object_get_data(G_OBJECT(w), "cnnobj");
			/* Do close the connection on this tab */
			remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
		}
	}

}

gboolean rcw_delete(RemminaConnectionWindow* cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnwin->priv;
	RemminaConnectionHolder *cnnhld = cnnwin->priv->cnnhld;
	GtkNotebook* notebook = GTK_NOTEBOOK(priv->notebook);
	GtkWidget* dialog;
	gint i, n;

	if (!REMMINA_IS_CONNECTION_WINDOW(cnnwin))
		return TRUE;

	if (cnnwin->priv->on_delete_confirm_mode != RCW_ONDELETE_NOCONFIRM) {
		n = gtk_notebook_get_n_pages(notebook);
		if (n > 1) {
			dialog = gtk_message_dialog_new(GTK_WINDOW(cnnwin), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				_("There are %i active connections in the current window. Are you sure to close?"), n);
			i = gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			if (i != GTK_RESPONSE_YES)
				return FALSE;
		}
	}
	rcw_close_all_connections(cnnwin);

	/* After rcw_close_all_connections() call, cnnwin
	 * has been already destroyed during a last page of notebook removal.
	 * So we must rely on cnnhld */
	if (cnnhld->cnnwin != NULL && GTK_IS_WIDGET(cnnhld->cnnwin))
		gtk_widget_destroy(GTK_WIDGET(cnnhld->cnnwin));
	cnnhld->cnnwin = NULL;

	return TRUE;
}

static gboolean rcw_delete_event(GtkWidget* widget, GdkEvent* event, gpointer data)
{
	TRACE_CALL(__func__);
	rcw_delete(RCW(widget));
	return TRUE;
}

static void rcw_destroy(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = RCW(widget)->priv;

	if (priv->kbcaptured)
		rch_keyboard_ungrab(cnnhld);

	if (priv->floating_toolbar_motion_handler) {
		g_source_remove(priv->floating_toolbar_motion_handler);
		priv->floating_toolbar_motion_handler = 0;
	}
	if (priv->ftb_hide_eventsource) {
		g_source_remove(priv->ftb_hide_eventsource);
		priv->ftb_hide_eventsource = 0;
	}
	if (priv->savestate_eventsourceid) {
		g_source_remove(priv->savestate_eventsourceid);
		priv->savestate_eventsourceid = 0;
	}

	/* There is no need to destroy priv->floating_toolbar_widget,
	 * because it’s our child and will be destroyed automatically */

	/* Timer used to hide the toolbar */
	if (priv->hidetb_timer) {
		g_source_remove(priv->hidetb_timer);
		priv->hidetb_timer = 0;
	}
	if (priv->switch_page_handler) {
		g_source_remove(priv->switch_page_handler);
		priv->switch_page_handler = 0;
	}
	g_free(priv);

	if (GTK_WIDGET(cnnhld->cnnwin) == widget) {
		cnnhld->cnnwin->priv = NULL;
		cnnhld->cnnwin = NULL;
	}

}

gboolean rcw_notify_widget_toolbar_placement(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	GType rcwtype;
	rcwtype = rcw_get_type();
	if (G_TYPE_CHECK_INSTANCE_TYPE(widget, rcwtype)) {
		g_signal_emit_by_name(G_OBJECT(widget), "toolbar-place");
		return TRUE;
	}
	return FALSE;
}

static gboolean rcw_tb_drag_failed(GtkWidget *widget, GdkDragContext *context,
							 GtkDragResult result, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaConnectionHolder* cnnhld;
	RemminaConnectionWindowPriv* priv;

	cnnhld = (RemminaConnectionHolder*)user_data;
	priv = cnnhld->cnnwin->priv;

	if (priv->toolbar)
		gtk_widget_show(GTK_WIDGET(priv->toolbar));

	return TRUE;
}

static gboolean rcw_tb_drag_drop(GtkWidget *widget, GdkDragContext *context,
						       gint x, gint y, guint time, gpointer user_data)
{
	TRACE_CALL(__func__);
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
	}else {
		if (wa.width * y > wa.height * ( wa.width - x) )
			new_toolbar_placement = TOOLBAR_PLACEMENT_RIGHT;
		else
			new_toolbar_placement = TOOLBAR_PLACEMENT_TOP;
	}

	gtk_drag_finish(context, TRUE, TRUE, time);

	if (new_toolbar_placement !=  remmina_pref.toolbar_placement) {
		/* Save new position */
		remmina_pref.toolbar_placement = new_toolbar_placement;
		remmina_pref_save();

		/* Signal all windows that the toolbar must be moved */
		remmina_widget_pool_foreach(rcw_notify_widget_toolbar_placement, NULL);

	}
	if (priv->toolbar)
		gtk_widget_show(GTK_WIDGET(priv->toolbar));

	return TRUE;

}

static void rcw_tb_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
	TRACE_CALL(__func__);

	cairo_surface_t *surface;
	cairo_t *cr;
	GtkAllocation wa;
	double dashes[] = { 10 };

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

void rch_update_toolbar_opacity(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->floating_toolbar_opacity = (1.0 - TOOLBAR_OPACITY_MIN) / ((gdouble)TOOLBAR_OPACITY_LEVEL)
					 * ((gdouble)(TOOLBAR_OPACITY_LEVEL - remmina_file_get_int(cnnobj->remmina_file, "toolbar_opacity", 0)))
					 + TOOLBAR_OPACITY_MIN;
	if (priv->floating_toolbar_widget) {
		gtk_widget_set_opacity(GTK_WIDGET(priv->overlay_ftb_overlay), priv->floating_toolbar_opacity);
	}
}

gboolean rch_floating_toolbar_make_invisible(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = (RemminaConnectionWindowPriv*)data;
	gtk_widget_set_opacity(GTK_WIDGET(priv->overlay_ftb_overlay), 0.0);
	priv->ftb_hide_eventsource = 0;
	return FALSE;
}

void rch_floating_toolbar_show(RemminaConnectionHolder* cnnhld, gboolean show)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->floating_toolbar_widget == NULL)
		return;

	if (show || priv->pin_down) {
		/* Make the FTB no longer transparent, in case we have an hidden toolbar */
		rch_update_toolbar_opacity(cnnhld);
		/* Remove outstanding hide events, if not yet active */
		if (priv->ftb_hide_eventsource) {
			g_source_remove(priv->ftb_hide_eventsource);
			priv->ftb_hide_eventsource = 0;
		}
	}else {
		/* If we are hiding and the toolbar must be made invisible, schedule
		 * a later toolbar hide */
		if (remmina_pref.fullscreen_toolbar_visibility == FLOATING_TOOLBAR_VISIBILITY_INVISIBLE) {
			if (priv->ftb_hide_eventsource == 0)
				priv->ftb_hide_eventsource = g_timeout_add(1000, rch_floating_toolbar_make_invisible, priv);
		}
	}

	gtk_revealer_set_reveal_child(GTK_REVEALER(priv->revealer), show || priv->pin_down);

}

void rch_get_desktop_size(RemminaConnectionHolder* cnnhld, gint* width, gint* height)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaProtocolWidget* gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);


	*width = remmina_protocol_widget_get_width(gp);
	*height = remmina_protocol_widget_get_height(gp);
	if (*width == 0) {
		/* Before connecting we do not have real remote width/height,
		 * so we ask profile values */
		*width = remmina_protocol_widget_get_profile_remote_width(gp);
		*height = remmina_protocol_widget_get_profile_remote_height(gp);
	}
}

void rco_set_scrolled_policy(RemminaConnectionObject* cnnobj, GtkScrolledWindow* scrolled_window)
{
	TRACE_CALL(__func__);
	RemminaScaleMode scalemode;
	scalemode = get_current_allowed_scale_mode(cnnobj, NULL, NULL);
	gtk_scrolled_window_set_policy(scrolled_window,
		scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC,
		scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC);
}

gboolean rch_toolbar_autofit_restore(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)
	RemminaConnectionWindowPriv * priv = cnnhld->cnnwin->priv;
	gint dwidth, dheight;
	GtkAllocation nba, ca, ta;

	if (priv->toolbar_is_reconfiguring)
		return FALSE;

	if (cnnobj->connected && GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container)) {
		rch_get_desktop_size(cnnhld, &dwidth, &dheight);
		gtk_widget_get_allocation(priv->notebook, &nba);
		gtk_widget_get_allocation(cnnobj->scrolled_container, &ca);
		gtk_widget_get_allocation(priv->toolbar, &ta);
		if (remmina_pref.toolbar_placement == TOOLBAR_PLACEMENT_LEFT ||
		    remmina_pref.toolbar_placement == TOOLBAR_PLACEMENT_RIGHT) {
			gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), MAX(1, dwidth + ta.width + nba.width - ca.width),
				MAX(1, dheight + nba.height - ca.height));
		}else {
			gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), MAX(1, dwidth + nba.width - ca.width),
				MAX(1, dheight + ta.height + nba.height - ca.height));
		}
		gtk_container_check_resize(GTK_CONTAINER(cnnhld->cnnwin));
	}
	if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container)) {
		rco_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(cnnobj->scrolled_container));
	}
	return FALSE;
}

void rch_toolbar_autofit(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ

	if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container)) {
		if ((gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin))) & GDK_WINDOW_STATE_MAXIMIZED) != 0) {
			gtk_window_unmaximize(GTK_WINDOW(cnnhld->cnnwin));
		}

		/* It’s tricky to make the toolbars disappear automatically, while keeping scrollable.
		   Please tell me if you know a better way to do this */
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container), GTK_POLICY_NEVER,
			GTK_POLICY_NEVER);

		/** @todo save returned source id in priv->something and then delete when main object is destroyed */
		g_timeout_add(200, (GSourceFunc)rch_toolbar_autofit_restore, cnnhld);
	}

}

void rco_get_monitor_geometry(RemminaConnectionObject* cnnobj, GdkRectangle *sz)
{
	TRACE_CALL(__func__);

	/* Fill sz with the monitor (or workarea) size and position
	 * of the monitor (or workarea) where cnnhld->cnnwin is located */

	GdkRectangle monitor_geometry;

	sz->x = sz->y = sz->width = sz->height = 0;

	if (!cnnobj->cnnhld)
		return;
	if (!cnnobj->cnnhld->cnnwin)
		return;
	if (!gtk_widget_is_visible(GTK_WIDGET(cnnobj->cnnhld->cnnwin)))
		return;

#if GTK_CHECK_VERSION(3, 22, 0)
	GdkDisplay* display;
	GdkMonitor* monitor;
	display = gtk_widget_get_display(GTK_WIDGET(cnnobj->cnnhld->cnnwin));
	monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(GTK_WIDGET(cnnobj->cnnhld->cnnwin)));
#else
	GdkScreen* screen;
	gint monitor;
	screen = gtk_window_get_screen(GTK_WINDOW(cnnobj->cnnhld->cnnwin));
	monitor = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(GTK_WIDGET(cnnobj->cnnhld->cnnwin)));
#endif

#if GTK_CHECK_VERSION(3, 22, 0)
	gdk_monitor_get_workarea(monitor, &monitor_geometry);
	/* Under Wayland, GTK 3.22, all values returned by gdk_monitor_get_geometry()
	 * and gdk_monitor_get_workarea() seem to have been divided by the
	 * gdk scale factor, so we need to adjust the returned rect
	 * undoing the division */
	#ifdef GDK_WINDOWING_WAYLAND
		if (GDK_IS_WAYLAND_DISPLAY(display)) {
			int monitor_scale_factor = gdk_monitor_get_scale_factor(monitor);
			monitor_geometry.width *= monitor_scale_factor;
			monitor_geometry.height *= monitor_scale_factor;
		}
	#endif
#elif gdk_screen_get_monitor_workarea
	gdk_screen_get_monitor_workarea(screen, monitor, &monitor_geometry);
#else
	gdk_screen_get_monitor_geometry(screen, monitor, &monitor_geometry);
#endif
	*sz = monitor_geometry;
}




void rch_check_resize(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	gboolean scroll_required = FALSE;

	GdkRectangle monitor_geometry;
	gint rd_width, rd_height;
	gint bordersz;
	gint scalemode;

	scalemode = remmina_protocol_widget_get_current_scale_mode(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));

	/* Get remote destkop size */
	rch_get_desktop_size(cnnhld, &rd_width, &rd_height);

	/* Get our monitor size */
	rco_get_monitor_geometry(cnnobj, &monitor_geometry);

	if (!remmina_protocol_widget_get_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto)) &&
	    (monitor_geometry.width < rd_width || monitor_geometry.height < rd_height) &&
	    scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE) {
		scroll_required = TRUE;
	}

	switch (cnnhld->cnnwin->priv->view_mode) {
	case SCROLLED_FULLSCREEN_MODE:
		gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), monitor_geometry.width, monitor_geometry.height);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container),
			(scroll_required ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER),
			(scroll_required ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER));
		break;

	case VIEWPORT_FULLSCREEN_MODE:
		bordersz = scroll_required ? SCROLL_BORDER_SIZE : 0;
		gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), monitor_geometry.width, monitor_geometry.height);
		if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container)) {
			/* Put a border around Notebook content (RemminaScrolledViewpord), so we can
			 * move the mouse over the border to scroll */
			gtk_container_set_border_width(GTK_CONTAINER(cnnobj->scrolled_container), bordersz);
		}

		break;

	case SCROLLED_WINDOW_MODE:
		if (remmina_file_get_int(cnnobj->remmina_file, "viewmode", AUTO_MODE) == AUTO_MODE) {
			gtk_window_set_default_size(GTK_WINDOW(cnnhld->cnnwin),
				MIN(rd_width, monitor_geometry.width), MIN(rd_height, monitor_geometry.height));
			if (rd_width >= monitor_geometry.width || rd_height >= monitor_geometry.height) {
				gtk_window_maximize(GTK_WINDOW(cnnhld->cnnwin));
				remmina_file_set_int(cnnobj->remmina_file, "window_maximize", TRUE);
			}else {
				rch_toolbar_autofit(NULL, cnnhld);
				remmina_file_set_int(cnnobj->remmina_file, "window_maximize", FALSE);
			}
		}else {
			if (remmina_file_get_int(cnnobj->remmina_file, "window_maximize", FALSE)) {
				gtk_window_maximize(GTK_WINDOW(cnnhld->cnnwin));
			}
		}
		break;

	default:
		break;
	}
}

void rch_set_tooltip(GtkWidget* item, const gchar* tip, guint key1, guint key2)
{
	TRACE_CALL(__func__);
	gchar* s1;
	gchar* s2;

	if (remmina_pref.hostkey && key1) {
		if (key2) {
			s1 = g_strdup_printf(" (%s + %s,%s)", gdk_keyval_name(remmina_pref.hostkey),
				gdk_keyval_name(gdk_keyval_to_upper(key1)), gdk_keyval_name(gdk_keyval_to_upper(key2)));
		}else if (key1 == remmina_pref.hostkey) {
			s1 = g_strdup_printf(" (%s)", gdk_keyval_name(remmina_pref.hostkey));
		}else {
			s1 = g_strdup_printf(" (%s + %s)", gdk_keyval_name(remmina_pref.hostkey),
				gdk_keyval_name(gdk_keyval_to_upper(key1)));
		}
	}else {
		s1 = NULL;
	}
	s2 = g_strdup_printf("%s%s", tip, s1 ? s1 : "");
	gtk_widget_set_tooltip_text(item, s2);
	g_free(s2);
	g_free(s1);
}

static void remmina_protocol_widget_update_alignment(RemminaConnectionObject* cnnobj)
{
	TRACE_CALL(__func__);
	RemminaScaleMode scalemode;
	gboolean scaledexpandedmode;
	int rdwidth, rdheight;
	gfloat aratio;

	if (!cnnobj->plugin_can_scale) {
		/* If we have a plugin that cannot scale,
		 * (i.e. SFTP plugin), then we expand proto */
		gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);
		gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);
	}else {
		/* Plugin can scale */

		scalemode = get_current_allowed_scale_mode(cnnobj, NULL, NULL);
		scaledexpandedmode = remmina_protocol_widget_get_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));

		/* Check if we need aspectframe and create/destroy it accordingly */
		if (scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED && !scaledexpandedmode) {
			/* We need an aspectframe as a parent of proto */
			rdwidth = remmina_protocol_widget_get_width(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
			rdheight = remmina_protocol_widget_get_height(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
			aratio = (gfloat)rdwidth / (gfloat)rdheight;
			if (!cnnobj->aspectframe) {
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
					rch_grab_focus(GTK_NOTEBOOK(cnnobj->cnnhld->cnnwin->priv->notebook));
			}else {
				gtk_aspect_frame_set(GTK_ASPECT_FRAME(cnnobj->aspectframe), 0.5, 0.5, aratio, FALSE);
			}
		}else {
			/* We do not need an aspectframe as a parent of proto */
			if (cnnobj->aspectframe) {
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
					rch_grab_focus(GTK_NOTEBOOK(cnnobj->cnnhld->cnnwin->priv->notebook));
			}
		}

		if (scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED || scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES) {
			/* We have a plugin that can be scaled, and the scale button
			 * has been pressed. Give it the correct WxH maintaining aspect
			 * ratio of remote destkop size */
			gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);
			gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);
		}else {
			/* Plugin can scale, but no scaling is active. Ensure that we have
			 * aspectframe with a ratio of 1 */
			gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_CENTER);
			gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_CENTER);
		}
	}
}


void rch_toolbar_fullscreen(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);

	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->toolbar_is_reconfiguring)
		return;

	if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget))) {
		rch_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
	}else {
		rch_create_scrolled(cnnhld, NULL);
	}
}

void rch_viewport_fullscreen_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnhld->fullscreen_view_mode = VIEWPORT_FULLSCREEN_MODE;
	rch_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
}

void rch_scrolled_fullscreen_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnhld->fullscreen_view_mode = SCROLLED_FULLSCREEN_MODE;
	rch_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
}

void rch_fullscreen_option_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->fullscreen_option_button), FALSE);
	rch_floating_toolbar_show(cnnhld, FALSE);
}

void rch_toolbar_fullscreen_option(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* menu;
	GtkWidget* menuitem;
	GSList* group;

	if (priv->toolbar_is_reconfiguring)
		return;

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	menu = gtk_menu_new();

	menuitem = gtk_radio_menu_item_new_with_label(NULL, _("Viewport fullscreen mode"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
	if (priv->view_mode == VIEWPORT_FULLSCREEN_MODE) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(rch_viewport_fullscreen_mode), cnnhld);

	menuitem = gtk_radio_menu_item_new_with_label(group, _("Scrolled fullscreen mode"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (priv->view_mode == SCROLLED_FULLSCREEN_MODE) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(rch_scrolled_fullscreen_mode), cnnhld);

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rch_fullscreen_option_popdown), cnnhld);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, priv->toolitem_fullscreen, 0,
		gtk_get_current_event_time());
#endif
}


void rch_scaler_option_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	if (priv->toolbar_is_reconfiguring)
		return;
	priv->sticky = FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->scaler_option_button), FALSE);
	rch_floating_toolbar_show(cnnhld, FALSE);
}

void rch_scaler_expand(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	remmina_protocol_widget_set_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), TRUE);
	remmina_file_set_int(cnnobj->remmina_file, "scaler_expand", TRUE);
	remmina_protocol_widget_update_alignment(cnnobj);
}
void rch_scaler_keep_aspect(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	remmina_protocol_widget_set_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), FALSE);
	remmina_file_set_int(cnnobj->remmina_file, "scaler_expand", FALSE);
	remmina_protocol_widget_update_alignment(cnnobj);
}

void rch_toolbar_scaler_option(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
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
	if (!scaler_expand) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(rch_scaler_keep_aspect), cnnhld);

	menuitem = gtk_radio_menu_item_new_with_label(group, _("Fill client window when scaled"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (scaler_expand) {
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	}
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(rch_scaler_expand), cnnhld);

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rch_scaler_option_popdown), cnnhld);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, priv->toolitem_scale, 0,
		gtk_get_current_event_time());
#endif
}

void rch_switch_page_activate(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	gint page_num;

	page_num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), "new-page-num"));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page_num);
}

void rch_toolbar_switch_page_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_switch_page), FALSE);
	rch_floating_toolbar_show(cnnhld, FALSE);
}

void rch_toolbar_switch_page(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* menu;
	GtkWidget* menuitem;
	GtkWidget* image;
	GtkWidget* page;
	gint i, n;

	if (priv->toolbar_is_reconfiguring)
		return;

	if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	menu = gtk_menu_new();

	n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook));
	for (i = 0; i < n; i++) {
		page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(priv->notebook), i);
		if (!page)
			break;
		cnnobj = (RemminaConnectionObject*)g_object_get_data(G_OBJECT(page), "cnnobj");

		menuitem = gtk_menu_item_new_with_label(remmina_file_get_string(cnnobj->remmina_file, "name"));
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		image = gtk_image_new_from_icon_name(remmina_file_get_icon_name(cnnobj->remmina_file), GTK_ICON_SIZE_MENU);
		gtk_widget_show(image);

		g_object_set_data(G_OBJECT(menuitem), "new-page-num", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(rch_switch_page_activate),
			cnnhld);
		if (i == gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook))) {
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
	}

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rch_toolbar_switch_page_popdown),
		cnnhld);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
#endif

}

void rch_update_toolbar_autofit_button(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkToolItem* toolitem;
	RemminaScaleMode sc;

	toolitem = priv->toolitem_autofit;
	if (toolitem) {
		if (priv->view_mode != SCROLLED_WINDOW_MODE) {
			gtk_widget_set_sensitive(GTK_WIDGET(toolitem), FALSE);
		}else {
			sc = get_current_allowed_scale_mode(cnnobj, NULL, NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(toolitem), sc == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE);
		}
	}
}

void rch_change_scalemode(RemminaConnectionHolder* cnnhld, gboolean bdyn, gboolean bscale)
{
	RemminaScaleMode scalemode;
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (bdyn)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES;
	else if (bscale)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED;
	else
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE;

	remmina_protocol_widget_set_current_scale_mode(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), scalemode);
	remmina_file_set_int(cnnobj->remmina_file, "scale", scalemode);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED);
	rch_update_toolbar_autofit_button(cnnhld);

	remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
		REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, 0);

	if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE) {
		rch_check_resize(cnnhld);
	}
	if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container)) {
		rco_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(cnnobj->scrolled_container));
	}

}

void rch_toolbar_dynres(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	gboolean bdyn, bscale;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->toolbar_is_reconfiguring)
		return;

	if (cnnobj->connected) {
		bdyn = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
		bscale = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale));

		if (bdyn && bscale) {
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale), FALSE);
			bscale = FALSE;
		}

		rch_change_scalemode(cnnhld, bdyn, bscale);
	}
}


void rch_toolbar_scaled_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	gboolean bdyn, bscale;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->toolbar_is_reconfiguring)
		return;

	bdyn = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres));
	bscale = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));

	if (bdyn && bscale) {
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres), FALSE);
		bdyn = FALSE;
	}

	rch_change_scalemode(cnnhld, bdyn, bscale);
}

void rch_toolbar_preferences_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->toolbar_is_reconfiguring)
		return;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_preferences), FALSE);
	rch_floating_toolbar_show(cnnhld, FALSE);
}

void rch_toolbar_tools_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->toolbar_is_reconfiguring)
		return;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_tools), FALSE);
	rch_floating_toolbar_show(cnnhld, FALSE);
}

void rch_call_protocol_feature_radio(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaProtocolFeature* feature;
	gpointer value;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) {
		feature = (RemminaProtocolFeature*)g_object_get_data(G_OBJECT(menuitem), "feature-type");
		value = g_object_get_data(G_OBJECT(menuitem), "feature-value");

		remmina_file_set_string(cnnobj->remmina_file, (const gchar*)feature->opt2, (const gchar*)value);
		remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
	}
}

void rch_call_protocol_feature_check(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaProtocolFeature* feature;
	gboolean value;

	feature = (RemminaProtocolFeature*)g_object_get_data(G_OBJECT(menuitem), "feature-type");
	value = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
	remmina_file_set_int(cnnobj->remmina_file, (const gchar*)feature->opt2, value);
	remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
}

void rch_call_protocol_feature_activate(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaProtocolFeature* feature;

	feature = (RemminaProtocolFeature*)g_object_get_data(G_OBJECT(menuitem), "feature-type");
	remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
}

void rch_toolbar_preferences_radio(RemminaConnectionHolder* cnnhld, RemminaFile* remminafile,
								GtkWidget* menu, const RemminaProtocolFeature* feature, const gchar* domain, gboolean enabled)
{
	TRACE_CALL(__func__);
	GtkWidget* menuitem;
	GSList* group;
	gint i;
	const gchar** list;
	const gchar* value;

	group = NULL;
	value = remmina_file_get_string(remminafile, (const gchar*)feature->opt2);
	list = (const gchar**)feature->opt3;
	for (i = 0; list[i]; i += 2) {
		menuitem = gtk_radio_menu_item_new_with_label(group, g_dgettext(domain, list[i + 1]));
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		if (enabled) {
			g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer)feature);
			g_object_set_data(G_OBJECT(menuitem), "feature-value", (gpointer)list[i]);

			if (value && g_strcmp0(list[i], value) == 0) {
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
			}

			g_signal_connect(G_OBJECT(menuitem), "toggled",
				G_CALLBACK(rch_call_protocol_feature_radio), cnnhld);
		}else {
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
	}
}

void rch_toolbar_preferences_check(RemminaConnectionHolder* cnnhld, RemminaFile* remminafile,
								GtkWidget* menu, const RemminaProtocolFeature* feature, const gchar* domain, gboolean enabled)
{
	TRACE_CALL(__func__);
	GtkWidget* menuitem;

	menuitem = gtk_check_menu_item_new_with_label(g_dgettext(domain, (const gchar*)feature->opt3));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	if (enabled) {
		g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer)feature);

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
			remmina_file_get_int(remminafile, (const gchar*)feature->opt2, FALSE));

		g_signal_connect(G_OBJECT(menuitem), "toggled",
			G_CALLBACK(rch_call_protocol_feature_check), cnnhld);
	}else {
		gtk_widget_set_sensitive(menuitem, FALSE);
	}
}

void rch_toolbar_preferences(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	const RemminaProtocolFeature* feature;
	GtkWidget* menu;
	GtkWidget* menuitem;
	gboolean separator;
	gchar* domain;
	gboolean enabled;

	if (priv->toolbar_is_reconfiguring)
		return;

	if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	separator = FALSE;

	domain = remmina_protocol_widget_get_domain(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	menu = gtk_menu_new();
	for (feature = remmina_protocol_widget_get_features(REMMINA_PROTOCOL_WIDGET(cnnobj->proto)); feature && feature->type;
	     feature++) {
		if (feature->type != REMMINA_PROTOCOL_FEATURE_TYPE_PREF)
			continue;

		if (separator) {
			menuitem = gtk_separator_menu_item_new();
			gtk_widget_show(menuitem);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			separator = FALSE;
		}
		enabled = remmina_protocol_widget_query_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
		switch (GPOINTER_TO_INT(feature->opt1)) {
		case REMMINA_PROTOCOL_FEATURE_PREF_RADIO:
			rch_toolbar_preferences_radio(cnnhld, cnnobj->remmina_file, menu, feature,
				domain, enabled);
			separator = TRUE;
			break;
		case REMMINA_PROTOCOL_FEATURE_PREF_CHECK:
			rch_toolbar_preferences_check(cnnhld, cnnobj->remmina_file, menu, feature,
				domain, enabled);
			break;
		}
	}

	g_free(domain);

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rch_toolbar_preferences_popdown),
		cnnhld);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
		GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
#endif

}

void rch_toolbar_tools(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
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

	if (priv->toolbar_is_reconfiguring)
		return;

	if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	domain = remmina_protocol_widget_get_domain(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	menu = gtk_menu_new();
	for (feature = remmina_protocol_widget_get_features(REMMINA_PROTOCOL_WIDGET(cnnobj->proto)); feature && feature->type;
	     feature++) {
		if (feature->type != REMMINA_PROTOCOL_FEATURE_TYPE_TOOL)
			continue;

		if (feature->opt1) {
			menuitem = gtk_menu_item_new_with_label(g_dgettext(domain, (const gchar*)feature->opt1));
		}
		if (feature->opt3) {
			rch_set_tooltip(menuitem, "", GPOINTER_TO_UINT(feature->opt3), 0);
		}
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		enabled = remmina_protocol_widget_query_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
		if (enabled) {
			g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer)feature);

			g_signal_connect(G_OBJECT(menuitem), "activate",
				G_CALLBACK(rch_call_protocol_feature_activate), cnnhld);
		}else {
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
	}

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rch_toolbar_tools_popdown), cnnhld);

	/* If the plugin accepts keystrokes include the keystrokes menu */
	if (remmina_protocol_widget_plugin_receives_keystrokes(REMMINA_PROTOCOL_WIDGET(cnnobj->proto))) {
		/* Get the registered keystrokes list */
		keystrokes = g_strsplit(remmina_pref.keystrokes, STRING_DELIMITOR, -1);
		if (g_strv_length(keystrokes)) {
			/* Add a keystrokes submenu */
			menuitem = gtk_menu_item_new_with_label(_("Keystrokes"));
			submenu_keystrokes = GTK_MENU(gtk_menu_new());
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuitem), GTK_WIDGET(submenu_keystrokes));
			gtk_widget_show(menuitem);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
			/* Add each registered keystroke */
			for (i = 0; i < g_strv_length(keystrokes); i++) {
				keystroke_values = g_strsplit(keystrokes[i], STRING_DELIMITOR2, -1);
				if (g_strv_length(keystroke_values) > 1) {
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

void rch_toolbar_screenshot(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);

	GdkPixbuf *screenshot;
	GdkWindow *active_window;
	cairo_t *cr;
	gint width, height;
	GString *pngstr;
	gchar* pngname;
	GtkWidget* dialog;
	RemminaProtocolWidget *gp;
	RemminaPluginScreenshotData rpsd;
	cairo_surface_t *srcsurface;
	cairo_format_t cairo_format;
	cairo_surface_t *surface;
	int stride;

	if (cnnhld->cnnwin->priv->toolbar_is_reconfiguring)
		return;

	GDateTime *date = g_date_time_new_now_utc();

	// We will take a screenshot of the currently displayed RemminaProtocolWidget.
	// DECLARE_CNNOBJ already did part of the job for us.
	DECLARE_CNNOBJ
		gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);

	GtkClipboard *c = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
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
		// Transfer the PixBuf in the main clipboard selection
		if (!remmina_pref.deny_screenshot_clipboard) {
			gtk_clipboard_set_image (c, gdk_pixbuf_get_from_surface (
						srcsurface, 0, 0, width, height));
		}
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
		    get_current_allowed_scale_mode(cnnobj, NULL, NULL) == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED) {
			dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
				_("Warning: screenshot is scaled or distorted. Disable scaling to have better screenshot."));
			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show(dialog);
		}

		// Get the screenshot.
		active_window = gtk_widget_get_window(GTK_WIDGET(gp));
		// width = gdk_window_get_width(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)));
		width = gdk_window_get_width(active_window);
		// height = gdk_window_get_height(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)));
		height = gdk_window_get_height(active_window);

		screenshot = gdk_pixbuf_get_from_window(active_window, 0, 0, width, height);
		if (screenshot == NULL)
			g_print("gdk_pixbuf_get_from_window failed\n");

		// Transfer the PixBuf in the main clipboard selection
		if (!remmina_pref.deny_screenshot_clipboard) {
			gtk_clipboard_set_image (c, screenshot);
		}
		// Prepare the destination cairo surface.
		surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
		cr = cairo_create(surface);

		// Copy the source pixbuf to the surface and paint it.
		gdk_cairo_set_source_pixbuf(cr, screenshot, 0, 0);
		cairo_paint(cr);

		// Deallocate screenshot pixbuf
		g_object_unref(screenshot);

	}

	//home/antenore/Pictures/remmina_%p_%h_%Y  %m %d-%H%M%S.png pngname
        //home/antenore/Pictures/remmina_st_  _2018 9 24-151958.240374.png

	pngstr = g_string_new(g_strdup_printf("%s/%s.png",
				remmina_pref.screenshot_path,
				remmina_pref.screenshot_name));
	remmina_utils_string_replace_all(pngstr, "%p",
			remmina_file_get_string(cnnobj->remmina_file, "name"));
	remmina_utils_string_replace_all(pngstr, "%h",
			remmina_file_get_string(cnnobj->remmina_file, "server"));
	remmina_utils_string_replace_all(pngstr, "%Y",
			g_strdup_printf("%d", g_date_time_get_year(date)));
	remmina_utils_string_replace_all(pngstr, "%m", g_strdup_printf("%d",
				g_date_time_get_month(date)));
	remmina_utils_string_replace_all(pngstr, "%d",
			g_strdup_printf("%d", g_date_time_get_day_of_month(date)));
	remmina_utils_string_replace_all(pngstr, "%H",
			g_strdup_printf("%d", g_date_time_get_hour(date)));
	remmina_utils_string_replace_all(pngstr, "%M",
			g_strdup_printf("%d", g_date_time_get_minute(date)));
	remmina_utils_string_replace_all(pngstr, "%S",
			g_strdup_printf("%f", g_date_time_get_seconds(date)));
	g_date_time_unref(date);
	pngname = g_string_free(pngstr, FALSE);

	cairo_surface_write_to_png(surface, pngname);

	/* send a desktop notification */
	remmina_public_send_notification("remmina-screenshot-is-ready-id", _("Screenshot taken"), pngname);

	//Clean up and return.
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

void rch_toolbar_minimize(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);

	if (cnnhld->cnnwin->priv->toolbar_is_reconfiguring)
		return;
	rch_floating_toolbar_show(cnnhld, FALSE);
	gtk_window_iconify(GTK_WINDOW(cnnhld->cnnwin));
}

void rch_toolbar_disconnect(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	if (cnnhld->cnnwin->priv->toolbar_is_reconfiguring)
		return;
	rch_disconnect_current_page(cnnhld);
}

void rch_toolbar_grab(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	gboolean capture;

	if (cnnhld->cnnwin->priv->toolbar_is_reconfiguring)
		return;

	capture = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
	remmina_file_set_int(cnnobj->remmina_file, "keyboard_grab", capture);
	if (capture) {
#if DEBUG_KB_GRABBING
		printf("DEBUG_KB_GRABBING: Grabbing for button\n");
#endif
		if (cnnobj->connected)
			rch_keyboard_grab(cnnhld);
	}else
		rch_keyboard_ungrab(cnnhld);
}

static GtkWidget*
rch_create_toolbar(RemminaConnectionHolder* cnnhld, gint mode)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* toolbar;
	GtkToolItem* toolitem;
	GtkWidget* widget;
	GtkWidget* arrow;

	priv->toolbar_is_reconfiguring = TRUE;

	toolbar = gtk_toolbar_new();
	gtk_widget_show(toolbar);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), FALSE);

	/* Auto-Fit */
	toolitem = gtk_tool_button_new(NULL, NULL);
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-fit-window-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Resize the window to fit in remote resolution"),
		remmina_pref.shortcutkey_autofit, 0);
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rch_toolbar_autofit), cnnhld);
	priv->toolitem_autofit = toolitem;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));


	/* Fullscreen toggle */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-fullscreen-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Toggle fullscreen mode"),
		remmina_pref.shortcutkey_fullscreen, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	priv->toolitem_fullscreen = toolitem;
	if (kioskmode) {
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolitem), FALSE);
	} else {
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolitem), mode != SCROLLED_WINDOW_MODE);
		g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rch_toolbar_fullscreen), cnnhld);
	}

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
	if (remmina_pref.small_toolbutton) {
		gtk_widget_set_name(widget, "remmina-small-button");
	}
#else
	gtk_button_set_focus_on_click(GTK_BUTTON(widget), FALSE);
#endif
	gtk_container_add(GTK_CONTAINER(toolitem), widget);

#if GTK_CHECK_VERSION(3, 14, 0)
	arrow = gtk_image_new_from_icon_name("remmina-pan-down-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
#endif
	gtk_widget_show(arrow);
	gtk_container_add(GTK_CONTAINER(widget), arrow);
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(rch_toolbar_fullscreen_option), cnnhld);
	priv->fullscreen_option_button = widget;
	if (mode == SCROLLED_WINDOW_MODE) {
		gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);
	}

	/* Switch tabs */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-switch-page-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Switch tab pages"), remmina_pref.shortcutkey_prevtab,
		remmina_pref.shortcutkey_nexttab);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rch_toolbar_switch_page), cnnhld);
	priv->toolitem_switch_page = toolitem;

	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));

	/* Dynamic Resolution Update */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-dynres-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Toggle dynamic resolution update"),
		remmina_pref.shortcutkey_dynres, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rch_toolbar_dynres), cnnhld);
	priv->toolitem_dynres = toolitem;

	/* Scaler button */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-scale-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Toggle scaled mode"), remmina_pref.shortcutkey_scale, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rch_toolbar_scaled_mode), cnnhld);
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
	if (remmina_pref.small_toolbutton) {
		gtk_widget_set_name(widget, "remmina-small-button");
	}
	gtk_container_add(GTK_CONTAINER(toolitem), widget);
#if GTK_CHECK_VERSION(3, 14, 0)
	arrow = gtk_image_new_from_icon_name("remmina-pan-down-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
#endif
	gtk_widget_show(arrow);
	gtk_container_add(GTK_CONTAINER(widget), arrow);
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(rch_toolbar_scaler_option), cnnhld);
	priv->scaler_option_button = widget;

	/* Grab keyboard button */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-keyboard-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Grab all keyboard events"),
		remmina_pref.shortcutkey_grab, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rch_toolbar_grab), cnnhld);
	priv->toolitem_grab = toolitem;

	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-preferences-system-symbolic");
	gtk_tool_item_set_tooltip_text(toolitem, _("Preferences"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rch_toolbar_preferences), cnnhld);
	priv->toolitem_preferences = toolitem;

	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-system-run-symbolic");
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolitem), _("Tools"));
	gtk_tool_item_set_tooltip_text(toolitem, _("Tools"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rch_toolbar_tools), cnnhld);
	priv->toolitem_tools = toolitem;

	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));

	toolitem = gtk_tool_button_new(NULL, "_Screenshot");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-camera-photo-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Screenshot"), remmina_pref.shortcutkey_screenshot, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rch_toolbar_screenshot), cnnhld);
	priv->toolitem_screenshot = toolitem;

	toolitem = gtk_tool_button_new(NULL, "_Bottom");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-go-bottom-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Minimize window"), remmina_pref.shortcutkey_minimize, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rch_toolbar_minimize), cnnhld);
	if (kioskmode) {
		gtk_widget_set_sensitive(GTK_WIDGET(toolitem), FALSE);
	}

	toolitem = gtk_tool_button_new(NULL, "_Disconnect");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-disconnect-symbolic");
	rch_set_tooltip(GTK_WIDGET(toolitem), _("Disconnect"), remmina_pref.shortcutkey_disconnect, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rch_toolbar_disconnect), cnnhld);

	priv->toolbar_is_reconfiguring = FALSE;
	return toolbar;
}

void rch_place_toolbar(GtkToolbar *toolbar, GtkGrid *grid, GtkWidget *sibling, int toolbar_placement)
{
	/* Place the toolbar inside the grid and set its orientation */

	if ( toolbar_placement == TOOLBAR_PLACEMENT_LEFT || toolbar_placement == TOOLBAR_PLACEMENT_RIGHT)
		gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), GTK_ORIENTATION_VERTICAL);
	else
		gtk_orientable_set_orientation(GTK_ORIENTABLE(toolbar), GTK_ORIENTATION_HORIZONTAL);


	switch (toolbar_placement) {
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

void rch_update_toolbar(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkToolItem* toolitem;
	gboolean bval, dynres_avail, scale_avail;
	gboolean test_floating_toolbar;
	RemminaScaleMode scalemode;

	priv->toolbar_is_reconfiguring = TRUE;

	rch_update_toolbar_autofit_button(cnnhld);


	toolitem = priv->toolitem_switch_page;
	if (kioskmode) {
		bval = FALSE;
	} else {
		bval = (gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)) > 1);
	}
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval);

	scalemode = get_current_allowed_scale_mode(cnnobj, &dynres_avail, &scale_avail);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->toolitem_dynres), dynres_avail && cnnobj->connected);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->toolitem_scale), scale_avail && cnnobj->connected);

	switch (scalemode) {
	case REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE:
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres), FALSE);
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), FALSE);
		break;
	case REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED:
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres), FALSE);
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale), TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), TRUE && cnnobj->connected);
		break;
	case REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES:
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_dynres), TRUE);
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale), FALSE);
		gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), FALSE);
		break;
	}

	toolitem = priv->toolitem_grab;
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), cnnobj->connected);
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolitem),
		remmina_file_get_int(cnnobj->remmina_file, "keyboard_grab", FALSE));

	toolitem = priv->toolitem_preferences;
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), cnnobj->connected);
	bval = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
		REMMINA_PROTOCOL_FEATURE_TYPE_PREF);
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval);

	toolitem = priv->toolitem_tools;
	bval = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
		REMMINA_PROTOCOL_FEATURE_TYPE_TOOL);
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval && cnnobj->connected);

	gtk_widget_set_sensitive(GTK_WIDGET(priv->toolitem_screenshot), cnnobj->connected);

	gtk_window_set_title(GTK_WINDOW(cnnhld->cnnwin), remmina_file_get_string(cnnobj->remmina_file, "name"));

	test_floating_toolbar = (priv->floating_toolbar_widget != NULL);
	if (test_floating_toolbar) {
		gtk_label_set_text(GTK_LABEL(priv->floating_toolbar_label),
			remmina_file_get_string(cnnobj->remmina_file, "name"));
	}

	priv->toolbar_is_reconfiguring = FALSE;

}

void rch_showhide_toolbar(RemminaConnectionHolder* cnnhld, gboolean resize)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	/* Here we should threat the resize flag, but we don’t */
	if (priv->view_mode == SCROLLED_WINDOW_MODE) {
		if (remmina_pref.hide_connection_toolbar) {
			gtk_widget_hide(priv->toolbar);
		}else {
			gtk_widget_show(priv->toolbar);
		}
	}
}

gboolean rch_floating_toolbar_on_enter(GtkWidget* widget, GdkEventCrossing* event,
								    RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	rch_floating_toolbar_show(cnnhld, TRUE);
	return TRUE;
}

gboolean rco_enter_protocol_widget(GtkWidget* widget, GdkEventCrossing* event,
								RemminaConnectionObject* cnnobj)
{
	TRACE_CALL(__func__);
	RemminaConnectionHolder* cnnhld = cnnobj->cnnhld;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	if (!priv->sticky && event->mode == GDK_CROSSING_NORMAL) {
		rch_floating_toolbar_show(cnnhld, FALSE);
		return TRUE;
	}
	return FALSE;
}

static void rcw_focus_in(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);

	DECLARE_CNNOBJ

	if (cnnobj->connected)
		rch_keyboard_grab(cnnhld);
}

static void rcw_focus_out(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ

	rch_keyboard_ungrab(cnnhld);
	cnnhld->hostkey_activated = FALSE;

	if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container)) {
		remmina_scrolled_viewport_remove_motion(REMMINA_SCROLLED_VIEWPORT(cnnobj->scrolled_container));
	}

	if (cnnobj->proto && cnnobj->scrolled_container) {
		remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, 0);
	}

}

static gboolean rcw_focus_out_event(GtkWidget* widget, GdkEvent* event, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
#if DEBUG_KB_GRABBING
	printf("DEBUG_KB_GRABBING: focus out and mouse_pointer_entered is %s\n", cnnhld->cnnwin->priv->mouse_pointer_entered ? "true" : "false");
#endif
	rcw_focus_out(widget, cnnhld);
	return FALSE;
}

static gboolean rcw_focus_in_event(GtkWidget* widget, GdkEvent* event, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
#if DEBUG_KB_GRABBING
	printf("DEBUG_KB_GRABBING: focus in and mouse_pointer_entered is %s\n", cnnhld->cnnwin->priv->mouse_pointer_entered ? "true" : "false");
#endif
	rcw_focus_in(widget, cnnhld);
	return FALSE;
}

static gboolean rcw_on_enter(GtkWidget* widget, GdkEventCrossing* event, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	cnnhld->cnnwin->priv->mouse_pointer_entered = TRUE;
	DECLARE_CNNOBJ_WITH_RETURN(FALSE);

#if DEBUG_KB_GRABBING
	printf("DEBUG_KB_GRABBING: enter detail=");
	switch (event->detail) {
	case GDK_NOTIFY_ANCESTOR: printf("GDK_NOTIFY_ANCESTOR"); break;
	case GDK_NOTIFY_VIRTUAL: printf("GDK_NOTIFY_VIRTUAL"); break;
	case GDK_NOTIFY_NONLINEAR: printf("GDK_NOTIFY_NONLINEAR"); break;
	case GDK_NOTIFY_NONLINEAR_VIRTUAL: printf("GDK_NOTIFY_NONLINEAR_VIRTUAL"); break;
	case GDK_NOTIFY_UNKNOWN: printf("GDK_NOTIFY_UNKNOWN"); break;
	case GDK_NOTIFY_INFERIOR: printf("GDK_NOTIFY_INFERIOR"); break;
	}
	printf("\n");
#endif
	if (gtk_window_is_active(GTK_WINDOW(cnnhld->cnnwin))) {
		if (cnnobj->connected)
			rch_keyboard_grab(cnnhld);
	}
	return FALSE;
}


static gboolean rcw_on_leave(GtkWidget* widget, GdkEventCrossing* event, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
#if DEBUG_KB_GRABBING
	printf("DEBUG_KB_GRABBING: leave detail=");
	switch (event->detail) {
	case GDK_NOTIFY_ANCESTOR: printf("GDK_NOTIFY_ANCESTOR"); break;
	case GDK_NOTIFY_VIRTUAL: printf("GDK_NOTIFY_VIRTUAL"); break;
	case GDK_NOTIFY_NONLINEAR: printf("GDK_NOTIFY_NONLINEAR"); break;
	case GDK_NOTIFY_NONLINEAR_VIRTUAL: printf("GDK_NOTIFY_NONLINEAR_VIRTUAL"); break;
	case GDK_NOTIFY_UNKNOWN: printf("GDK_NOTIFY_UNKNOWN"); break;
	case GDK_NOTIFY_INFERIOR: printf("GDK_NOTIFY_INFERIOR"); break;
	}
	printf("  x=%f y=%f\n", event->x, event->y);
	printf("  focus=%s\n", event->focus ? "yes" : "no");
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
		rch_keyboard_ungrab(cnnhld);
	}
	return FALSE;
}

static gboolean
rch_floating_toolbar_hide(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	priv->hidetb_timer = 0;
	rch_floating_toolbar_show(cnnhld, FALSE);
	return FALSE;
}

gboolean rch_floating_toolbar_on_scroll(GtkWidget* widget, GdkEventScroll* event,
								     RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)
	int opacity;

	opacity = remmina_file_get_int(cnnobj->remmina_file, "toolbar_opacity", 0);
	switch (event->direction) {
	case GDK_SCROLL_UP:
		if (opacity > 0) {
			remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity - 1);
			rch_update_toolbar_opacity(cnnhld);
			return TRUE;
		}
		break;
	case GDK_SCROLL_DOWN:
		if (opacity < TOOLBAR_OPACITY_LEVEL) {
			remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity + 1);
			rch_update_toolbar_opacity(cnnhld);
			return TRUE;
		}
		break;
#ifdef GDK_SCROLL_SMOOTH
	case GDK_SCROLL_SMOOTH:
		if (event->delta_y < 0 && opacity > 0) {
			remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity - 1);
			rch_update_toolbar_opacity(cnnhld);
			return TRUE;
		}
		if (event->delta_y > 0 && opacity < TOOLBAR_OPACITY_LEVEL) {
			remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity + 1);
			rch_update_toolbar_opacity(cnnhld);
			return TRUE;
		}
		break;
#endif
	default:
		break;
	}
	return FALSE;
}

static gboolean rcw_after_configure_scrolled(gpointer user_data)
{
	TRACE_CALL(__func__);
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
	for (ipg = 0; ipg < npages; ipg++) {
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

static gboolean rcw_on_configure(GtkWidget* widget, GdkEventConfigure* event,
						       RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)

	if (cnnhld->cnnwin->priv->savestate_eventsourceid) {
		g_source_remove(cnnhld->cnnwin->priv->savestate_eventsourceid);
		cnnhld->cnnwin->priv->savestate_eventsourceid = 0;
	}

	if (cnnhld->cnnwin && gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin))
	    && cnnhld->cnnwin->priv->view_mode == SCROLLED_WINDOW_MODE) {
		/* Under gnome shell we receive this configure_event BEFORE a window
		 * is really unmaximized, so we must read its new state and dimensions
		 * later, not now */
		cnnhld->cnnwin->priv->savestate_eventsourceid = g_timeout_add(500, rcw_after_configure_scrolled, cnnhld);
	}

	if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE) {
		/* Notify window of change so that scroll border can be hidden or shown if needed */
		rch_check_resize(cnnobj->cnnhld);
	}
	return FALSE;
}

void rch_update_pin(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	if (cnnhld->cnnwin->priv->pin_down) {
		gtk_button_set_image(GTK_BUTTON(cnnhld->cnnwin->priv->pin_button),
			gtk_image_new_from_icon_name("remmina-pin-down-symbolic", GTK_ICON_SIZE_MENU));
	}else {
		gtk_button_set_image(GTK_BUTTON(cnnhld->cnnwin->priv->pin_button),
			gtk_image_new_from_icon_name("remmina-pin-up-symbolic", GTK_ICON_SIZE_MENU));
	}
}

void rch_toolbar_pin(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	remmina_pref.toolbar_pin_down = cnnhld->cnnwin->priv->pin_down = !cnnhld->cnnwin->priv->pin_down;
	remmina_pref_save();
	rch_update_pin(cnnhld);
}

void rch_create_floating_toolbar(RemminaConnectionHolder* cnnhld, gint mode)
{
	TRACE_CALL(__func__);
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* ftb_widget;
	GtkWidget* vbox;
	GtkWidget* hbox;
	GtkWidget* label;
	GtkWidget* pinbutton;
	GtkWidget* tb;


	/* A widget to be used for GtkOverlay for GTK >= 3.10 */
	ftb_widget = gtk_event_box_new();

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox);

	gtk_container_add(GTK_CONTAINER(ftb_widget), vbox);

	tb = rch_create_toolbar(cnnhld, mode);
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
	g_signal_connect(G_OBJECT(pinbutton), "clicked", G_CALLBACK(rch_toolbar_pin), cnnhld);
	priv->pin_button = pinbutton;
	priv->pin_down = remmina_pref.toolbar_pin_down;
	rch_update_pin(cnnhld);


	label = gtk_label_new(remmina_file_get_string(cnnobj->remmina_file, "name"));
	gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
	gtk_widget_show(label);

	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

	priv->floating_toolbar_label = label;


	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM) {
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), tb, FALSE, FALSE, 0);
	}else {
		gtk_box_pack_start(GTK_BOX(vbox), tb, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	priv->floating_toolbar_widget = ftb_widget;
	gtk_widget_show(ftb_widget);

}

static void rcw_toolbar_place_signal(RemminaConnectionWindow* cnnwin, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv;

	priv = cnnwin->priv;
	/* Detach old toolbar widget and reattach in new position in the grid */
	if (priv->toolbar && priv->grid) {
		g_object_ref(priv->toolbar);
		gtk_container_remove(GTK_CONTAINER(priv->grid), priv->toolbar);
		rch_place_toolbar(GTK_TOOLBAR(priv->toolbar), GTK_GRID(priv->grid), priv->notebook, remmina_pref.toolbar_placement);
		g_object_unref(priv->toolbar);
	}
}


static void rcw_init(RemminaConnectionWindow* cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv;

	priv = g_new0(RemminaConnectionWindowPriv, 1);
	cnnwin->priv = priv;

	priv->view_mode = AUTO_MODE;
	if (kioskmode && kioskmode == TRUE)
		priv->view_mode = VIEWPORT_FULLSCREEN_MODE;

	priv->floating_toolbar_opacity = 1.0;
	priv->kbcaptured = FALSE;
	priv->mouse_pointer_entered = FALSE;

	gtk_container_set_border_width(GTK_CONTAINER(cnnwin), 0);

	remmina_widget_pool_register(GTK_WIDGET(cnnwin));

	g_signal_connect(G_OBJECT(cnnwin), "toolbar-place", G_CALLBACK(rcw_toolbar_place_signal), NULL);
}

static gboolean rcw_state_event(GtkWidget* widget, GdkEventWindowState* event, gpointer user_data)
{
	TRACE_CALL(__func__);

	if (event->changed_mask & GDK_WINDOW_STATE_FOCUSED) {
		if (event->new_window_state & GDK_WINDOW_STATE_FOCUSED)
			rcw_focus_in(widget, user_data);
		else
			rcw_focus_out(widget, user_data);
	}

#ifdef ENABLE_MINIMIZE_TO_TRAY
	GdkScreen* screen;

	screen = gdk_screen_get_default();
	if (remmina_pref.minimize_to_tray && (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) != 0
	    && (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) != 0
	    && remmina_public_get_current_workspace(screen)
	    == remmina_public_get_window_workspace(GTK_WINDOW(widget))
	    && gdk_screen_get_number(screen) == gdk_screen_get_number(gtk_widget_get_screen(widget))) {
		gtk_widget_hide(widget);
		return TRUE;
	}
#endif
	return FALSE; // moved here because a function should return a value. Should be correct
}

static GtkWidget*
rcw_new_from_holder(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow* cnnwin;

	cnnwin = RCW(g_object_new(REMMINA_TYPE_CONNECTION_WINDOW, NULL));
	cnnwin->priv->cnnhld = cnnhld;
	cnnwin->priv->on_delete_confirm_mode = RCW_ONDELETE_CONFIRM_IF_2_OR_MORE;

	g_signal_connect(G_OBJECT(cnnwin), "delete-event", G_CALLBACK(rcw_delete_event), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "destroy", G_CALLBACK(rcw_destroy), cnnhld);

	/* focus-in-event and focus-out-event don’t work when keyboard is grabbed
	 * via gdk_device_grab. So we listen for window-state-event to detect focus in and focus out */
	g_signal_connect(G_OBJECT(cnnwin), "window-state-event", G_CALLBACK(rcw_state_event), cnnhld);

	g_signal_connect(G_OBJECT(cnnwin), "focus-in-event", G_CALLBACK(rcw_focus_in_event), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "focus-out-event", G_CALLBACK(rcw_focus_out_event), cnnhld);

	g_signal_connect(G_OBJECT(cnnwin), "enter-notify-event", G_CALLBACK(rcw_on_enter), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "leave-notify-event", G_CALLBACK(rcw_on_leave), cnnhld);

	g_signal_connect(G_OBJECT(cnnwin), "configure_event", G_CALLBACK(rcw_on_configure), cnnhld);

	return GTK_WIDGET(cnnwin);
}

/* This function will be called for the first connection. A tag is set to the window so that
 * other connections can determine if whether a new tab should be append to the same window
 */
static void rcw_update_tag(RemminaConnectionWindow* cnnwin, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL(__func__);
	gchar* tag;

	switch (remmina_pref.tab_mode) {
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
	g_object_set_data_full(G_OBJECT(cnnwin), "tag", tag, (GDestroyNotify)g_free);
}

void rco_create_scrolled_container(RemminaConnectionObject* cnnobj, gint view_mode)
{
	TRACE_CALL(__func__);
	GtkWidget* container;

	if (view_mode == VIEWPORT_FULLSCREEN_MODE) {
		container = remmina_scrolled_viewport_new();
	}else {
		container = gtk_scrolled_window_new(NULL, NULL);
		rco_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(container));
		gtk_container_set_border_width(GTK_CONTAINER(container), 0);
		gtk_widget_set_can_focus(container, FALSE);
	}

	gtk_widget_set_name(container, "remmina-scrolled-container");

	gtk_widget_show(container);
	cnnobj->scrolled_container = container;

	g_signal_connect(G_OBJECT(cnnobj->proto), "enter-notify-event", G_CALLBACK(rco_enter_protocol_widget), cnnobj);

}

void rch_grab_focus(GtkNotebook *notebook)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj;
	GtkWidget* child;

	child = gtk_notebook_get_nth_page(notebook, gtk_notebook_get_current_page(notebook));
	cnnobj = g_object_get_data(G_OBJECT(child), "cnnobj");
	if (GTK_IS_WIDGET(cnnobj->proto)) {
		remmina_protocol_widget_grab_focus(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	}
}

void rco_closewin(RemminaProtocolWidget* gp)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj = gp->cnnobj;
	RemminaConnectionHolder* cnnhld = cnnobj->cnnhld;

	if (cnnhld && cnnhld->cnnwin) {
		gtk_notebook_remove_page(
			GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook),
			gtk_notebook_page_num(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook),
				cnnobj->page));
	}

	cnnobj->remmina_file = NULL;
	g_free(cnnobj);

	remmina_application_condexit(REMMINA_CONDEXIT_ONDISCONNECT);
}

void rco_on_close_button_clicked(GtkButton* button, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL(__func__);
	if (REMMINA_IS_PROTOCOL_WIDGET(cnnobj->proto)) {
		if (!remmina_protocol_widget_is_closed((RemminaProtocolWidget*)cnnobj->proto))
			remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
		else
			rco_closewin((RemminaProtocolWidget*)cnnobj->proto);
	}
}

static GtkWidget* rco_create_tab(RemminaConnectionObject* cnnobj)
{
	TRACE_CALL(__func__);
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

	button = gtk_button_new();      // The "x" to close the tab
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

	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(rco_on_close_button_clicked), cnnobj);

	return hbox;
}

gint rco_append_page(RemminaConnectionObject* cnnobj, GtkNotebook* notebook, GtkWidget* tab,
						  gint view_mode)
{
	TRACE_CALL(__func__);
	gint i;

	cnnobj->page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	g_object_set_data(G_OBJECT(cnnobj->page), "cnnobj", cnnobj);
	gtk_widget_set_name(cnnobj->page, "remmina-tab-page");

	rco_create_scrolled_container(cnnobj, view_mode);
	gtk_box_pack_start(GTK_BOX(cnnobj->page), cnnobj->scrolled_container, TRUE, TRUE, 0);
	i = gtk_notebook_append_page(notebook, cnnobj->page, tab);

	gtk_notebook_set_tab_reorderable(notebook, cnnobj->page, TRUE);
	gtk_notebook_set_tab_detachable(notebook, cnnobj->page, TRUE);
	/* This trick prevents the tab label from being focused */
	gtk_widget_set_can_focus(gtk_widget_get_parent(tab), FALSE);

	gtk_widget_show(cnnobj->page);

	return i;
}

static void rcw_initialize_notebook(GtkNotebook* to, GtkNotebook* from, RemminaConnectionObject* cnnobj,
							  gint view_mode)
{
	TRACE_CALL(__func__);
	gint i, n, c;
	GtkWidget* tab;
	GtkWidget* widget;
	GtkWidget* frompage;
	GList *lst, *l;
	RemminaConnectionObject* tc;

	if (cnnobj) {
		/* Search cnnobj in the "from" notebook */
		tc = NULL;
		if (from) {
			n = gtk_notebook_get_n_pages(from);
			for (i = 0; i < n; i++) {
				widget = gtk_notebook_get_nth_page(from, i);
				tc = (RemminaConnectionObject*)g_object_get_data(G_OBJECT(widget), "cnnobj");
				if (tc == cnnobj)
					break;
			}
		}
		if (tc) {
			/* if cnnobj is already in the "from" notebook, we should be in the drag and drop case.
			 * just… do not move it. GTK will do the move when the create-window signal
			 * of GtkNotebook will return */

		} else {
			/* cnnobj is not on the "from" notebook. This is a new connection for a newly created window,
			 * just add a tab put cnnobj->scrolled_container inside  cnnobj->viewport */
			tab = rco_create_tab(cnnobj);

			rco_append_page(cnnobj, to, tab, view_mode);
			/* Set the current page to the 1st tab page, otherwise the notebook
			 * will stay on page -1 for a short time and g_object_get_data(currenntab, "cnnobj") will fail
			 * together with DECLARE_CNNOBJ (issue #1809)*/
			gtk_notebook_set_current_page(to, 0);
			gtk_container_add(GTK_CONTAINER(cnnobj->scrolled_container), cnnobj->viewport);
		}
	}else {
		/* cnnobj=null: migrate all existing connections to the new notebook */
		if (from != NULL && GTK_IS_NOTEBOOK(from)) {
			c = gtk_notebook_get_current_page(from);
			n = gtk_notebook_get_n_pages(from);
			for (i = 0; i < n; i++) {
				frompage = gtk_notebook_get_nth_page(from, i);
				tc = (RemminaConnectionObject*)g_object_get_data(G_OBJECT(frompage), "cnnobj");

				tab = rco_create_tab(tc);
				rco_append_page(tc, to, tab, view_mode);
				/* Reparent message panels */
				lst = gtk_container_get_children(GTK_CONTAINER(frompage));
				for (l = lst; l != NULL; l = l->next) {
					if (REMMINA_IS_MESSAGE_PANEL(l->data)) {
						G_GNUC_BEGIN_IGNORE_DEPRECATIONS
						gtk_widget_reparent(l->data, tc->page);
						G_GNUC_END_IGNORE_DEPRECATIONS
						gtk_box_reorder_child(GTK_BOX(tc->page), GTK_WIDGET(l->data), 0);
					}
				}
				g_list_free(lst);

				/* Reparent cnnobj->viewport */
				G_GNUC_BEGIN_IGNORE_DEPRECATIONS
				gtk_widget_reparent(tc->viewport, tc->scrolled_container);
				G_GNUC_END_IGNORE_DEPRECATIONS
			}
			gtk_notebook_set_current_page(to, c);
		}
	}
}

void rch_update_notebook(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	GtkNotebook* notebook;
	gint n;

	if (!cnnhld->cnnwin)
		return;

	notebook = GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook);

	switch (cnnhld->cnnwin->priv->view_mode) {
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

gboolean rch_on_switch_page_real(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionHolder* cnnhld = (RemminaConnectionHolder*)data;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (GTK_IS_WIDGET(cnnhld->cnnwin)) {
		rch_floating_toolbar_show(cnnhld, TRUE);
		if (!priv->hidetb_timer)
			priv->hidetb_timer = g_timeout_add(TB_HIDE_TIME_TIME, (GSourceFunc)
				rch_floating_toolbar_hide, cnnhld);
		rch_update_toolbar(cnnhld);
		rch_grab_focus(GTK_NOTEBOOK(priv->notebook));
		if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE) {
			rch_check_resize(cnnhld);
		}

	}
	priv->switch_page_handler = 0;
	return FALSE;
}

void rch_on_switch_page(GtkNotebook* notebook, GtkWidget* page, guint page_num,
						     RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (!priv->switch_page_handler) {
		priv->switch_page_handler = g_idle_add(rch_on_switch_page_real, cnnhld);
	}
}

void rch_on_page_added(GtkNotebook* notebook, GtkWidget* child, guint page_num,
						    RemminaConnectionHolder* cnnhld)
{
	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)) > 0)
		rch_update_notebook(cnnhld);
}

void rch_on_page_removed(GtkNotebook* notebook, GtkWidget* child, guint page_num,
						      RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);

	if (!cnnhld->cnnwin)
		return;

	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)) <= 0) {
		gtk_widget_destroy(GTK_WIDGET(cnnhld->cnnwin));
		cnnhld->cnnwin = NULL;
	}

}

GtkNotebook*
rch_on_notebook_create_window(GtkNotebook* notebook, GtkWidget* page, gint x, gint y, gpointer data)
{
	/* This signal callback is called by GTK when a detachable tab is dropped on the root window */

	TRACE_CALL(__func__);
	RemminaConnectionWindow* srccnnwin;
	RemminaConnectionWindow* dstcnnwin;
	RemminaConnectionObject* cnnobj;
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
	srccnnwin = RCW(gtk_widget_get_toplevel(GTK_WIDGET(notebook)));
	dstcnnwin = RCW(remmina_widget_pool_find_by_window(REMMINA_TYPE_CONNECTION_WINDOW, window));

	if (srccnnwin == dstcnnwin)
		return NULL;

	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(srccnnwin->priv->notebook)) == 1 && !dstcnnwin)
		return NULL;

	cnnobj = (RemminaConnectionObject*)g_object_get_data(G_OBJECT(page), "cnnobj");

	if (dstcnnwin) {
		cnnobj->cnnhld = dstcnnwin->priv->cnnhld;
	}else {
		cnnobj->cnnhld = g_new0(RemminaConnectionHolder, 1);
		if (!cnnobj->cnnhld->cnnwin) {
			/* Create a new scrolled window to accomodate the dropped connection
			 * and move our cnnobj there */
			cnnobj->cnnhld->cnnwin = srccnnwin;
			rch_create_scrolled(cnnobj->cnnhld, cnnobj);
		}
	}

	remmina_protocol_widget_set_hostkey_func(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
		(RemminaHostkeyFunc)rcw_hostkey_func);

	return GTK_NOTEBOOK(cnnobj->cnnhld->cnnwin->priv->notebook);
}

static GtkWidget*
rch_create_notebook(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);
	GtkWidget* notebook;

	notebook = gtk_notebook_new();

	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	gtk_widget_show(notebook);

	g_signal_connect(G_OBJECT(notebook), "create-window", G_CALLBACK(rch_on_notebook_create_window),
		cnnhld);
	g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(rch_on_switch_page), cnnhld);
	g_signal_connect(G_OBJECT(notebook), "page-added", G_CALLBACK(rch_on_page_added), cnnhld);
	g_signal_connect(G_OBJECT(notebook), "page-removed", G_CALLBACK(rch_on_page_removed), cnnhld);
	gtk_widget_set_can_focus(notebook, FALSE);

	return notebook;
}

/* Create a scrolled window container */
void rch_create_scrolled(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL(__func__);
	GtkWidget* window;
	GtkWidget* oldwindow;
	GtkWidget* grid;
	GtkWidget* toolbar;
	GtkWidget* notebook;
	GList *chain;
	gchar* tag;
	int newwin_width, newwin_height;

	oldwindow = GTK_WIDGET(cnnhld->cnnwin);
	window = rcw_new_from_holder(cnnhld);
	gtk_widget_realize(window);
	cnnhld->cnnwin = RCW(window);


	newwin_width = newwin_height = 100;
	if (cnnobj) {
		/* If we have a cnnobj as a reference for this window, we can setup its default size here */
		newwin_width = remmina_file_get_int(cnnobj->remmina_file, "window_width", 640);
		newwin_height = remmina_file_get_int(cnnobj->remmina_file, "window_height", 480);
	} else {
		/* Try to get a temporary RemminaConnectionObject from the old window and get
		 * a remmina_file and width/height */
		int np;
		GtkWidget* page;
		RemminaConnectionObject* oldwindow_currentpage_cnnobj;

		np = gtk_notebook_get_current_page(GTK_NOTEBOOK(RCW(oldwindow)->priv->notebook));
		page = gtk_notebook_get_nth_page(GTK_NOTEBOOK(RCW(oldwindow)->priv->notebook), np);
		oldwindow_currentpage_cnnobj = (RemminaConnectionObject*)g_object_get_data(G_OBJECT(page), "cnnobj");
		newwin_width = remmina_file_get_int(oldwindow_currentpage_cnnobj->remmina_file, "window_width", 640);
		newwin_height = remmina_file_get_int(oldwindow_currentpage_cnnobj->remmina_file, "window_height", 480);
	}

	gtk_window_set_default_size(GTK_WINDOW(cnnhld->cnnwin), newwin_width, newwin_height);

	/* Create the toolbar */
	toolbar = rch_create_toolbar(cnnhld, SCROLLED_WINDOW_MODE);

	/* Create the notebook */
	notebook = rch_create_notebook(cnnhld);

	/* Create the grid container for toolbars+notebook and populate it */
	grid = gtk_grid_new();
	gtk_grid_attach(GTK_GRID(grid), notebook, 0, 0, 1, 1);

	gtk_widget_set_hexpand(notebook, TRUE);
	gtk_widget_set_vexpand(notebook, TRUE);

	rch_place_toolbar(GTK_TOOLBAR(toolbar), GTK_GRID(grid), notebook, remmina_pref.toolbar_placement);


	gtk_container_add(GTK_CONTAINER(window), grid);

	chain = g_list_append(NULL, notebook);
	gtk_container_set_focus_chain(GTK_CONTAINER(grid), chain);
	g_list_free(chain);

	/* Add drag capabilities to the toolbar */
	gtk_drag_source_set(GTK_WIDGET(toolbar), GDK_BUTTON1_MASK,
		dnd_targets_tb, sizeof dnd_targets_tb / sizeof *dnd_targets_tb, GDK_ACTION_MOVE);
	g_signal_connect_after(GTK_WIDGET(toolbar), "drag-begin", G_CALLBACK(rcw_tb_drag_begin), cnnhld);
	g_signal_connect(GTK_WIDGET(toolbar), "drag-failed", G_CALLBACK(rcw_tb_drag_failed), cnnhld);

	/* Add drop capabilities to the drop/dest target for the toolbar (the notebook) */
	gtk_drag_dest_set(GTK_WIDGET(notebook), GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
		dnd_targets_tb, sizeof dnd_targets_tb / sizeof *dnd_targets_tb, GDK_ACTION_MOVE);
	gtk_drag_dest_set_track_motion(GTK_WIDGET(notebook), TRUE);
	g_signal_connect(GTK_WIDGET(notebook), "drag-drop", G_CALLBACK(rcw_tb_drag_drop), cnnhld);

	cnnhld->cnnwin->priv->view_mode = SCROLLED_WINDOW_MODE;
	cnnhld->cnnwin->priv->toolbar = toolbar;
	cnnhld->cnnwin->priv->grid = grid;
	cnnhld->cnnwin->priv->notebook = notebook;

	/* The notebook and all its child must be realized now, or a reparent will
	 * call unrealize() and will destroy a GtkSocket */
	gtk_widget_show(grid);
	gtk_widget_show(GTK_WIDGET(cnnhld->cnnwin));

	rcw_initialize_notebook(GTK_NOTEBOOK(notebook),
		(oldwindow ? GTK_NOTEBOOK(RCW(oldwindow)->priv->notebook) : NULL), cnnobj,
		SCROLLED_WINDOW_MODE);

	if (cnnobj) {
		if (!oldwindow)
			rcw_update_tag(cnnhld->cnnwin, cnnobj);

		if (remmina_file_get_int(cnnobj->remmina_file, "window_maximize", FALSE)) {
			gtk_window_maximize(GTK_WINDOW(cnnhld->cnnwin));
		}
	}

	if (oldwindow) {
		tag = g_strdup((gchar*)g_object_get_data(G_OBJECT(oldwindow), "tag"));
		g_object_set_data_full(G_OBJECT(cnnhld->cnnwin), "tag", tag, (GDestroyNotify)g_free);
		if (!cnnobj)
			gtk_widget_destroy(oldwindow);
	}

	rch_update_toolbar(cnnhld);
	rch_showhide_toolbar(cnnhld, FALSE);
	rch_check_resize(cnnhld);

}

static gboolean rcw_go_fullscreen(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionHolder* cnnhld;

	cnnhld = (RemminaConnectionHolder*)data;

#if GTK_CHECK_VERSION(3, 18, 0)
	if (remmina_pref.fullscreen_on_auto) {
		gtk_window_fullscreen_on_monitor(GTK_WINDOW(cnnhld->cnnwin),
			gdk_screen_get_default(),
			gdk_screen_get_monitor_at_window
				(gdk_screen_get_default(), gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin))
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

void rch_create_overlay_ftb_overlay(RemminaConnectionHolder* cnnhld)
{
	TRACE_CALL(__func__);

	GtkWidget* revealer;
	RemminaConnectionWindowPriv* priv;
	priv = cnnhld->cnnwin->priv;

	if (priv->overlay_ftb_overlay != NULL) {
		gtk_widget_destroy(priv->overlay_ftb_overlay);
		priv->overlay_ftb_overlay = NULL;
		priv->revealer = NULL;
	}

	rch_create_floating_toolbar(cnnhld, cnnhld->fullscreen_view_mode);
	rch_update_toolbar(cnnhld);

	priv->overlay_ftb_overlay = gtk_event_box_new();

	GtkWidget* vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 0);

	GtkWidget* handle = gtk_drawing_area_new();
	gtk_widget_set_size_request(handle, 4, 4);
	gtk_widget_set_name(handle, "ftb-handle");

	revealer = gtk_revealer_new();

	gtk_widget_set_halign(GTK_WIDGET(priv->overlay_ftb_overlay), GTK_ALIGN_CENTER);

	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM) {
		gtk_box_pack_start(GTK_BOX(vbox), handle, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), revealer, FALSE, FALSE, 0);
		gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
		gtk_widget_set_valign(GTK_WIDGET(priv->overlay_ftb_overlay), GTK_ALIGN_END);
	}else {
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

	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM) {
		gtk_widget_set_name(fr, "ftbbox-lower");
	}else {
		gtk_widget_set_name(fr, "ftbbox-upper");
	}

	gtk_overlay_add_overlay(GTK_OVERLAY(priv->overlay), priv->overlay_ftb_overlay);

	rch_floating_toolbar_show(cnnhld, TRUE);

	g_signal_connect(G_OBJECT(priv->overlay_ftb_overlay), "enter-notify-event", G_CALLBACK(rch_floating_toolbar_on_enter), cnnhld);
	g_signal_connect(G_OBJECT(priv->overlay_ftb_overlay), "scroll-event", G_CALLBACK(rch_floating_toolbar_on_scroll), cnnhld);
	gtk_widget_add_events(GTK_WIDGET(priv->overlay_ftb_overlay), GDK_SCROLL_MASK);

	/* Add drag and drop capabilities to the source */
	gtk_drag_source_set(GTK_WIDGET(priv->overlay_ftb_overlay), GDK_BUTTON1_MASK,
		dnd_targets_ftb, sizeof dnd_targets_ftb / sizeof *dnd_targets_ftb, GDK_ACTION_MOVE);
	g_signal_connect_after(GTK_WIDGET(priv->overlay_ftb_overlay), "drag-begin", G_CALLBACK(rcw_ftb_drag_begin), cnnhld);
}


static gboolean rcw_ftb_drag_drop(GtkWidget *widget, GdkDragContext *context,
							gint x, gint y, guint time, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkAllocation wa;
	gint new_floating_toolbar_placement;
	RemminaConnectionHolder* cnnhld;

	cnnhld = (RemminaConnectionHolder*)user_data;

	gtk_widget_get_allocation(widget, &wa);

	if (y >= wa.height / 2) {
		new_floating_toolbar_placement = FLOATING_TOOLBAR_PLACEMENT_BOTTOM;
	}else {
		new_floating_toolbar_placement = FLOATING_TOOLBAR_PLACEMENT_TOP;
	}

	gtk_drag_finish(context, TRUE, TRUE, time);

	if (new_floating_toolbar_placement !=  remmina_pref.floating_toolbar_placement) {
		/* Destroy and recreate the FTB */
		remmina_pref.floating_toolbar_placement = new_floating_toolbar_placement;
		remmina_pref_save();
		rch_create_overlay_ftb_overlay(cnnhld);
	}

	return TRUE;

}

static void rcw_ftb_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data)
{
	TRACE_CALL(__func__);

	cairo_surface_t *surface;
	cairo_t *cr;
	GtkAllocation wa;
	double dashes[] = { 10 };

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

void rch_create_fullscreen(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj,
							gint view_mode)
{
	TRACE_CALL(__func__);
	GtkWidget* window;
	GtkWidget* oldwindow;
	GtkWidget* notebook;
	RemminaConnectionWindowPriv* priv;

	gchar* tag;

	oldwindow = GTK_WIDGET(cnnhld->cnnwin);
	window = rcw_new_from_holder(cnnhld);
	gtk_widget_set_name(GTK_WIDGET(window), "remmina-connection-window-fullscreen");
	gtk_widget_realize(window);

	cnnhld->cnnwin = RCW(window);
	priv = cnnhld->cnnwin->priv;

	if (!view_mode)
		view_mode = VIEWPORT_FULLSCREEN_MODE;

	notebook = rch_create_notebook(cnnhld);

	priv->overlay = gtk_overlay_new();
	gtk_container_add(GTK_CONTAINER(window), priv->overlay);
	gtk_container_add(GTK_CONTAINER(priv->overlay), notebook);
	gtk_widget_show(GTK_WIDGET(priv->overlay));

	priv->notebook = notebook;
	priv->view_mode = view_mode;
	cnnhld->fullscreen_view_mode = view_mode;

	rcw_initialize_notebook(GTK_NOTEBOOK(notebook),
		(oldwindow ? GTK_NOTEBOOK(RCW(oldwindow)->priv->notebook) : NULL), cnnobj,
		view_mode);

	if (cnnobj) {
		rcw_update_tag(cnnhld->cnnwin, cnnobj);
	}
	if (oldwindow) {
		tag = g_strdup((gchar*)g_object_get_data(G_OBJECT(oldwindow), "tag"));
		g_object_set_data_full(G_OBJECT(cnnhld->cnnwin), "tag", tag, (GDestroyNotify)g_free);
		gtk_widget_destroy(oldwindow);
	}

	/* Create the floating toolbar */
	if (remmina_pref.fullscreen_toolbar_visibility != FLOATING_TOOLBAR_VISIBILITY_DISABLE) {
		rch_create_overlay_ftb_overlay(cnnhld);
		/* Add drag and drop capabilities to the drop/dest target for floating toolbar */
		gtk_drag_dest_set(GTK_WIDGET(priv->overlay), GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
			dnd_targets_ftb, sizeof dnd_targets_ftb / sizeof *dnd_targets_ftb, GDK_ACTION_MOVE);
		gtk_drag_dest_set_track_motion(GTK_WIDGET(priv->notebook), TRUE);
		g_signal_connect(GTK_WIDGET(priv->overlay), "drag-drop", G_CALLBACK(rcw_ftb_drag_drop), cnnhld);
	}

	rch_check_resize(cnnhld);

	gtk_widget_show(window);

	/* Put the window in fullscreen after it is mapped to have it appear on the same monitor */
	g_signal_connect(G_OBJECT(window), "map-event", G_CALLBACK(rcw_go_fullscreen), (gpointer)cnnhld);
}

static gboolean rcw_hostkey_func(RemminaProtocolWidget* gp, guint keyval, gboolean release)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj = gp->cnnobj;
	RemminaConnectionHolder* cnnhld = cnnobj->cnnhld;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	const RemminaProtocolFeature* feature;
	gint i;

	if (release) {
		if (remmina_pref.hostkey && keyval == remmina_pref.hostkey) {
			cnnhld->hostkey_activated = FALSE;
			if (cnnhld->hostkey_used) {
				/* hostkey pressed + something else */
				return TRUE;
			}
		}
		/* If hostkey is released without pressing other keys, we should execute the
		 * shortcut key which is the same as hostkey. Be default, this is grab/ungrab
		 * keyboard */
		else if (cnnhld->hostkey_activated) {
			/* Trap all key releases when hostkey is pressed */
			/* hostkey pressed + something else */
			return TRUE;

		}else {
			/* Any key pressed, no hostkey */
			return FALSE;
		}
	}else if (remmina_pref.hostkey && keyval == remmina_pref.hostkey) {
		/** @todo Add callback for hostname transparent overlay #832 */
		cnnhld->hostkey_activated = TRUE;
		cnnhld->hostkey_used = FALSE;
		return TRUE;
	}else if (!cnnhld->hostkey_activated) {
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

			gtk_adjustment_set_value(adjust, pos);
			if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down)
				gtk_scrolled_window_set_vadjustment(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container), adjust);
			else
				gtk_scrolled_window_set_hadjustment(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container), adjust);
		}else if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container)) {
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
				sz = gdk_window_get_height(gsvwin) + 2; // Add 2px of black scroll border
				adj = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(child));
			}else {
				sz = gdk_window_get_width(gsvwin) + 2; // Add 2px of black scroll border
				adj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(child));
			}

			if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Left) {
				value = 0;
			}else {
				value = gtk_adjustment_get_upper(GTK_ADJUSTMENT(adj)) - (gdouble)sz + 2.0;
			}

			gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), value);
		}
	}

	if (keyval == remmina_pref.shortcutkey_fullscreen) {
		switch (priv->view_mode) {
		case SCROLLED_WINDOW_MODE:
			rch_create_fullscreen(
				cnnhld,
				NULL,
				cnnhld->fullscreen_view_mode ?
				cnnhld->fullscreen_view_mode : VIEWPORT_FULLSCREEN_MODE);
			break;
		case SCROLLED_FULLSCREEN_MODE:
		case VIEWPORT_FULLSCREEN_MODE:
			rch_create_scrolled(cnnhld, NULL);
			break;
		default:
			break;
		}
	}else if (keyval == remmina_pref.shortcutkey_autofit) {
		if (priv->toolitem_autofit && gtk_widget_is_sensitive(GTK_WIDGET(priv->toolitem_autofit))) {
			rch_toolbar_autofit(GTK_WIDGET(gp), cnnhld);
		}
	}else if (keyval == remmina_pref.shortcutkey_nexttab) {
		i = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)) + 1;
		if (i >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)))
			i = 0;
		gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), i);
	}else if (keyval == remmina_pref.shortcutkey_prevtab) {
		i = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)) - 1;
		if (i < 0)
			i = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)) - 1;
		gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), i);
	}else if (keyval == remmina_pref.shortcutkey_scale) {
		if (gtk_widget_is_sensitive(GTK_WIDGET(priv->toolitem_scale))) {
			gtk_toggle_tool_button_set_active(
				GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale),
				!gtk_toggle_tool_button_get_active(
					GTK_TOGGLE_TOOL_BUTTON(
						priv->toolitem_scale)));
		}
	}else if (keyval == remmina_pref.shortcutkey_grab) {
		gtk_toggle_tool_button_set_active(
			GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_grab),
			!gtk_toggle_tool_button_get_active(
				GTK_TOGGLE_TOOL_BUTTON(
					priv->toolitem_grab)));
	}else if (keyval == remmina_pref.shortcutkey_minimize) {
		rch_toolbar_minimize(GTK_WIDGET(gp),
			cnnhld);
	}else if (keyval == remmina_pref.shortcutkey_viewonly) {
		remmina_file_set_int(cnnobj->remmina_file, "viewonly",
			( remmina_file_get_int(cnnobj->remmina_file, "viewonly", 0 )
			  == 0  ) ? 1 : 0 );
	}else if (keyval == remmina_pref.shortcutkey_screenshot) {
		rch_toolbar_screenshot(GTK_WIDGET(gp),
			cnnhld);
	}else if (keyval == remmina_pref.shortcutkey_disconnect) {
		rch_disconnect_current_page(cnnhld);
	}else if (keyval == remmina_pref.shortcutkey_toolbar) {
		if (priv->view_mode == SCROLLED_WINDOW_MODE) {
			remmina_pref.hide_connection_toolbar =
				!remmina_pref.hide_connection_toolbar;
			rch_showhide_toolbar( cnnhld, TRUE);
		}
	}else {
		for (feature =
			     remmina_protocol_widget_get_features(
				     REMMINA_PROTOCOL_WIDGET(
					     cnnobj->proto));
		     feature && feature->type;
		     feature++) {
			if (feature->type
			    == REMMINA_PROTOCOL_FEATURE_TYPE_TOOL
			    && GPOINTER_TO_UINT(
				    feature->opt3)
			    == keyval) {
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

static RemminaConnectionWindow* rcw_find(RemminaFile* remminafile)
{
	TRACE_CALL(__func__);
	const gchar* tag;

	switch (remmina_pref.tab_mode) {
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
	return RCW(remmina_widget_pool_find(REMMINA_TYPE_CONNECTION_WINDOW, tag));
}

gboolean rco_delayed_window_present(gpointer user_data)
{
	RemminaConnectionObject* cnnobj = (RemminaConnectionObject*)user_data;
	if (cnnobj && cnnobj->connected && cnnobj->cnnhld && cnnobj->cnnhld->cnnwin) {
		gtk_window_present_with_time(GTK_WINDOW(cnnobj->cnnhld->cnnwin), (guint32)(g_get_monotonic_time() / 1000));
		rch_grab_focus(GTK_NOTEBOOK(cnnobj->cnnhld->cnnwin->priv->notebook));
	}
	return FALSE;
}

void rco_on_connect(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	TRACE_CALL(__func__);

	RemminaConnectionWindow* cnnwin;
	RemminaConnectionHolder* cnnhld;
	gchar *last_success;

	GDateTime *date = g_date_time_new_now_utc();

	/* This signal handler is called by a plugin when it’s correctly connected
	 * (and authenticated) */

	if (!cnnobj->cnnhld) {
		cnnwin = rcw_find(cnnobj->remmina_file);
		if (cnnwin) {
			cnnhld = cnnwin->priv->cnnhld;
		}else {
			cnnhld = g_new0(RemminaConnectionHolder, 1);
		}
		cnnobj->cnnhld = cnnhld;
	}else {
		cnnhld = cnnobj->cnnhld;
	}

	cnnobj->connected = TRUE;

	remmina_protocol_widget_set_hostkey_func(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
		(RemminaHostkeyFunc)rcw_hostkey_func);

	/** Remember recent list for quick connect, and save the current date
	 * in the last_used field.
	 */
	last_success = g_strdup_printf("%d%02d%02d",
		g_date_time_get_year(date),
		g_date_time_get_month(date),
		g_date_time_get_day_of_month(date));
	if (remmina_file_get_filename(cnnobj->remmina_file) == NULL) {
		remmina_pref_add_recent(remmina_file_get_string(cnnobj->remmina_file, "protocol"),
			remmina_file_get_string(cnnobj->remmina_file, "server"));
	}
	if (remmina_pref.periodic_usage_stats_permitted)
		remmina_file_set_string (cnnobj->remmina_file, "last_success", last_success);

	/* Save credentials */
	remmina_file_save(cnnobj->remmina_file);

	if (cnnhld->cnnwin->priv->floating_toolbar_widget) {
		gtk_widget_show(cnnhld->cnnwin->priv->floating_toolbar_widget);
	}

	rch_update_toolbar(cnnhld);

	/* Try to present window */
	g_timeout_add(200, rco_delayed_window_present, (gpointer)cnnobj);

}

static void cb_lasterror_confirmed(void *cbdata, int btn)
{
	TRACE_CALL(__func__);
	rco_closewin((RemminaProtocolWidget*)cbdata);
}

void rco_on_disconnect(RemminaProtocolWidget* gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj = gp->cnnobj;
	RemminaConnectionHolder* cnnhld = cnnobj->cnnhld;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* pparent;

	/* Detach the protocol widget from the notebook now, or we risk that a
	 * window delete will destroy cnnobj->proto before we complete disconnection.
	 */
	pparent = gtk_widget_get_parent(cnnobj->proto);
	if (pparent != NULL) {
		g_object_ref(cnnobj->proto);
		gtk_container_remove(GTK_CONTAINER(pparent), cnnobj->proto);
	}

	cnnobj->connected = FALSE;

	if (cnnhld && remmina_pref.save_view_mode) {
		if (cnnhld->cnnwin) {
			remmina_file_set_int(cnnobj->remmina_file, "viewmode", cnnhld->cnnwin->priv->view_mode);
		}
		remmina_file_save(cnnobj->remmina_file);
	}

	rch_keyboard_ungrab(cnnhld);
	gtk_toggle_tool_button_set_active(
		GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_grab),
		FALSE);

	if (remmina_protocol_widget_has_error(gp)) {
		/* We cannot close window immediately, but we must show a message panel */
		RemminaMessagePanel *mp;
		/* Destroy scrolled_contaner (and viewport) and all its children the plugin created
		 * on it, so they will not receive GUI signals */
		gtk_widget_destroy(cnnobj->scrolled_container);
		cnnobj->scrolled_container = NULL;
		cnnobj->viewport = NULL;
		mp = remmina_message_panel_new();
		remmina_message_panel_setup_message(mp, remmina_protocol_widget_get_error_message(gp), cb_lasterror_confirmed, gp);
		rco_show_message_panel(gp->cnnobj, mp);
	}else {
		rco_closewin(gp);
	}

}

void rco_on_desktop_resize(RemminaProtocolWidget* gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj = gp->cnnobj;
	if (cnnobj->cnnhld && cnnobj->cnnhld->cnnwin && cnnobj->cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE) {
		rch_check_resize(cnnobj->cnnhld);
	}
}

void rco_on_update_align(RemminaProtocolWidget* gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj = gp->cnnobj;
	remmina_protocol_widget_update_alignment(cnnobj);
}

void rco_on_unlock_dynres(RemminaProtocolWidget* gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj = gp->cnnobj;
	cnnobj->dynres_unlocked = TRUE;
	rch_update_toolbar(cnnobj->cnnhld);
}

gboolean rcw_open_from_filename(const gchar* filename)
{
	TRACE_CALL(__func__);
	RemminaFile* remminafile;
	GtkWidget* dialog;

	remminafile = remmina_file_manager_load_file(filename);
	if (remminafile) {
		rcw_open_from_file(remminafile);
		return TRUE;
	}else {
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
			_("File %s is corrupted, unreadable or not found."), filename);
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(dialog);
		remmina_widget_pool_register(dialog);
		return FALSE;
	}
}

static gboolean open_connection_last_stage(gpointer user_data)
{
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)user_data;

	/* Now we have an allocated size for our RemminaProtocolWidget. We can proceed with the connection */
	remmina_protocol_widget_update_remote_resolution(gp);
	remmina_protocol_widget_open_connection(gp);
	rch_check_resize(gp->cnnobj->cnnhld);

	return FALSE;
}

static void rpw_size_allocated_on_connection(GtkWidget *w, GdkRectangle *allocation, gpointer user_data)
{
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)w;

	/* Disconnect signal handler to avoid to be called again after a normal resize */
	g_signal_handler_disconnect(w, gp->cnnobj->deferred_open_size_allocate_handler);

	/* Allow extra 100ms for size allocation (do we really need it?) */
	g_timeout_add(100, open_connection_last_stage, gp);

	return;
}

void rcw_open_from_file(RemminaFile* remminafile)
{
	TRACE_CALL(__func__);
	rcw_open_from_file_full(remminafile, NULL, NULL, NULL);
}

GtkWidget* rcw_open_from_file_full(RemminaFile* remminafile, GCallback disconnect_cb, gpointer data, guint* handler)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject* cnnobj;

	gint ret;
	GtkWidget* dialog;
	RemminaConnectionWindow* cnnwin;
	GtkWidget* tab;
	gint i;

	/* Create the RemminaConnectionObject */
	cnnobj = g_new0(RemminaConnectionObject, 1);
	cnnobj->remmina_file = remminafile;

	/* Create the RemminaProtocolWidget */
	cnnobj->proto = remmina_protocol_widget_new();
	remmina_protocol_widget_setup((RemminaProtocolWidget*)cnnobj->proto, remminafile, cnnobj);
	/* Set a name for the widget, for CSS selector */
	gtk_widget_set_name(GTK_WIDGET(cnnobj->proto), "remmina-protocol-widget");

	gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);

	if (data) {
		g_object_set_data(G_OBJECT(cnnobj->proto), "user-data", data);
	}

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


	/* Determine whether this connection will be put on a new window
	 * or in an existing one */
	cnnwin = rcw_find(remminafile);
	if (!cnnwin) {
		/* Connection goes on a new toplevel window */
		cnnobj->cnnhld = g_new0(RemminaConnectionHolder, 1);
		i = remmina_file_get_int(cnnobj->remmina_file, "viewmode", 0);
		if (kioskmode)
			i = VIEWPORT_FULLSCREEN_MODE;
		switch (i) {
		case SCROLLED_FULLSCREEN_MODE:
		case VIEWPORT_FULLSCREEN_MODE:
			rch_create_fullscreen(cnnobj->cnnhld, cnnobj, i);
			break;
		case SCROLLED_WINDOW_MODE:
		default:
			rch_create_scrolled(cnnobj->cnnhld, cnnobj);
			break;
		}
		cnnwin = cnnobj->cnnhld->cnnwin;
	}else {
		cnnobj->cnnhld = cnnwin->priv->cnnhld;
		tab = rco_create_tab(cnnobj);
		i = rco_append_page(cnnobj, GTK_NOTEBOOK(cnnwin->priv->notebook), tab,
			cnnwin->priv->view_mode);
		gtk_container_add(GTK_CONTAINER(cnnobj->scrolled_container), cnnobj->viewport);

		gtk_window_present(GTK_WINDOW(cnnwin));
		gtk_notebook_set_current_page(GTK_NOTEBOOK(cnnwin->priv->notebook), i);
	}

	// Do not call remmina_protocol_widget_update_alignment(cnnobj); here or cnnobj->proto will not fill its parent size
	// and remmina_protocol_widget_update_remote_resolution() cannot autodetect available space

	gtk_widget_show(cnnobj->proto);
	g_signal_connect(G_OBJECT(cnnobj->proto), "connect", G_CALLBACK(rco_on_connect), cnnobj);
	if (disconnect_cb) {
		*handler = g_signal_connect(G_OBJECT(cnnobj->proto), "disconnect", disconnect_cb, data);
	}
	g_signal_connect(G_OBJECT(cnnobj->proto), "disconnect", G_CALLBACK(rco_on_disconnect), NULL);
	g_signal_connect(G_OBJECT(cnnobj->proto), "desktop-resize", G_CALLBACK(rco_on_desktop_resize), NULL);
	g_signal_connect(G_OBJECT(cnnobj->proto), "update-align", G_CALLBACK(rco_on_update_align), NULL);
	g_signal_connect(G_OBJECT(cnnobj->proto), "unlock-dynres", G_CALLBACK(rco_on_unlock_dynres), NULL);

	if (!remmina_pref.save_view_mode)
		remmina_file_set_int(cnnobj->remmina_file, "viewmode", remmina_pref.default_mode);


	/* If it is a GtkSocket plugin and X11 is not available, we inform the
	 * user and close the connection */
	ret = remmina_plugin_manager_query_feature_by_type(REMMINA_PLUGIN_TYPE_PROTOCOL,
			remmina_file_get_string(remminafile, "protocol"),
			REMMINA_PROTOCOL_FEATURE_TYPE_GTKSOCKET);
	if (ret && !remmina_gtksocket_available()) {
		dialog = gtk_message_dialog_new(
				GTK_WINDOW(cnnwin),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
				GTK_BUTTONS_OK,
				_("Warning: This plugin require GtkSocket, but it’s not available."));
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(dialog);
		return NULL;	/* Should we destroy something before returning ? */
	}

	if (cnnwin->priv->floating_toolbar_widget) {
		gtk_widget_show(cnnwin->priv->floating_toolbar_widget);
	}

	if (remmina_protocol_widget_has_error((RemminaProtocolWidget*)cnnobj->proto)) {
		printf("Ok, an error occurred in initializing the protocol plugin before connecting. The error is %s.\n"
			"ToDo: put this string as error to show", remmina_protocol_widget_get_error_message((RemminaProtocolWidget*)cnnobj->proto));
		return cnnobj->proto;
	}


	/* GTK window setup is done here, and we are almost ready to call remmina_protocol_widget_open_connection().
	 * But size has not yet been allocated by GTK
	 * to the widgets. If we are in RES_USE_INITIAL_WINDOW_SIZE resolution mode or scale is REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES,
	 * we should wait for a size allocation from GTK for cnnobj->proto
	 * before connecting */

	cnnobj->deferred_open_size_allocate_handler = g_signal_connect(G_OBJECT(cnnobj->proto), "size-allocate", G_CALLBACK(rpw_size_allocated_on_connection), NULL);

	return cnnobj->proto;

}

void rcw_set_delete_confirm_mode(RemminaConnectionWindow* cnnwin, RemminaConnectionWindowOnDeleteConfirmMode mode)
{
	TRACE_CALL(__func__);
	cnnwin->priv->on_delete_confirm_mode = mode;
}

/**
 * Deletes a RemminaMessagePanel from the current cnnobj
 * and if it was visible, make visible the last remaining one.
 */
void rco_destroy_message_panel(RemminaConnectionObject *cnnobj, RemminaMessagePanel *mp)
{
	TRACE_CALL(__func__);
	GList *childs, *cc;
	RemminaMessagePanel *lastPanel;
	gboolean was_visible;

	childs = gtk_container_get_children(GTK_CONTAINER(cnnobj->page));
	cc = g_list_first(childs);
	while(cc != NULL) {
		if ((RemminaMessagePanel*)cc->data == mp)
			break;
		cc = g_list_next(cc);
	}
	g_list_free(childs);

	if (cc == NULL) {
		printf("REMMINA: warning, request to destroy a RemminaMessagePanel which is not on the page\n");
		return;
	}
	was_visible = gtk_widget_is_visible(GTK_WIDGET(mp));
	gtk_widget_destroy(GTK_WIDGET(mp));

	/* And now, show the last remaining message panel, if needed */
	if (was_visible) {
		childs = gtk_container_get_children(GTK_CONTAINER(cnnobj->page));
		cc = g_list_first(childs);
		lastPanel = NULL;
		while(cc != NULL) {
			if (G_TYPE_CHECK_INSTANCE_TYPE(cc->data, REMMINA_TYPE_MESSAGE_PANEL))
				lastPanel = (RemminaMessagePanel*)cc->data;
			cc = g_list_next(cc);
		}
		g_list_free(childs);
		if (lastPanel)
			gtk_widget_show(GTK_WIDGET(lastPanel));
	}

}

/**
 * Each cnnobj->page can have more than one RemminaMessagePanel, but 0 or 1 are visible.
 *
 * This function adds a RemminaMessagePanel to cnnobj->page, move it to top,
 * and makes it the only visible one.
 */
void rco_show_message_panel(RemminaConnectionObject *cnnobj, RemminaMessagePanel *mp)
{
	TRACE_CALL(__func__);
	GList *childs, *cc;

	/* Hides all RemminaMessagePanels childs of cnnobj->page */
	childs = gtk_container_get_children(GTK_CONTAINER(cnnobj->page));
	cc = g_list_first(childs);
	while(cc != NULL) {
		if (G_TYPE_CHECK_INSTANCE_TYPE(cc->data, REMMINA_TYPE_MESSAGE_PANEL))
			gtk_widget_hide(GTK_WIDGET(cc->data));
		cc = g_list_next(cc);
	}
	g_list_free(childs);

	/* Add the new message panel at the top of cnnobj->page */
	gtk_box_pack_start(GTK_BOX(cnnobj->page), GTK_WIDGET(mp), FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(cnnobj->page), GTK_WIDGET(mp), 0);

	/* Show the message panel */
	gtk_widget_show_all(GTK_WIDGET(mp));

	/* Focus the correct field of the RemminaMessagePanel */
	remmina_message_panel_focus_auth_entry(mp);

}

