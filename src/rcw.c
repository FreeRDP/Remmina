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
#include "remmina_main.h"
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

G_DEFINE_TYPE(RemminaConnectionWindow, rcw, GTK_TYPE_WINDOW)

#define MOTION_TIME 100

/* default timeout used to hide the floating toolbar wen switching profile */
#define TB_HIDE_TIME_TIME 1000

#define FULL_SCREEN_TARGET_MONITOR_UNDEFINED -1

struct _RemminaConnectionWindowPriv {
	GtkNotebook *					notebook;
	guint						switch_page_finalsel_handler;
	GtkWidget *					floating_toolbar_widget;
	GtkWidget *					overlay;
	GtkWidget *					revealer;
	GtkWidget *					overlay_ftb_overlay;

	GtkWidget *					floating_toolbar_label;
	gdouble						floating_toolbar_opacity;
	/* To avoid strange event-loop */
	guint						floating_toolbar_motion_handler;
	/* Other event sources to remove when deleting the object */
	guint						ftb_hide_eventsource;
	/* Timer to hide the toolbar */
	guint						hidetb_timer;

	/* Timer to save new window state and wxh */
	guint						savestate_eventsourceid;

	GtkWidget *					toolbar;
	GtkWidget *					grid;

	/* Toolitems that need to be handled */
	GtkToolItem *					toolitem_autofit;
	GtkToolItem *					toolitem_fullscreen;
	GtkToolItem *					toolitem_switch_page;
	GtkToolItem *					toolitem_dynres;
	GtkToolItem *					toolitem_scale;
	GtkToolItem *					toolitem_grab;
	GtkToolItem *					toolitem_preferences;
	GtkToolItem *					toolitem_tools;
	GtkToolItem *					toolitem_duplicate;
	GtkToolItem *					toolitem_screenshot;
	GtkWidget *					fullscreen_option_button;
	GtkWidget *					fullscreen_scaler_button;
	GtkWidget *					scaler_option_button;

	GtkWidget *					pin_button;
	gboolean					pin_down;

	gboolean					sticky;
	gint						grab_retry_eventsourceid;

	/* Flag to turn off toolbar signal handling when toolbar is
	 * reconfiguring, usually due to a tab switch */
	gboolean					toolbar_is_reconfiguring;

	/* This is the current view mode, i.e. VIEWPORT_FULLSCREEN_MODE,
	 * as saved on the "viwemode" profile preference file */
	gint						view_mode;

	/* Status variables used when in fullscreen mode. Needed
	 * to restore a fullscreen mode after coming from scrolled */
	gint						fss_view_mode;
	/* Status variables used when in scrolled window mode. Needed
	 * to restore a scrolled window mode after coming from fullscreen */
	gint						ss_width, ss_height;
	gboolean					ss_maximized;

	gboolean					kbcaptured;
	gboolean					mouse_pointer_entered;
	gboolean					hostkey_activated;
	gboolean					hostkey_used;

	RemminaConnectionWindowOnDeleteConfirmMode	on_delete_confirm_mode;
};

typedef struct _RemminaConnectionObject {
	RemminaConnectionWindow *	cnnwin;
	RemminaFile *			remmina_file;

	GtkWidget *			proto;
	GtkWidget *			aspectframe;
	GtkWidget *			viewport;

	GtkWidget *			scrolled_container;

	gboolean			plugin_can_scale;

	gboolean			connected;
	gboolean			dynres_unlocked;

	gulong				deferred_open_size_allocate_handler;
} RemminaConnectionObject;

enum {
	TOOLBARPLACE_SIGNAL,
	LAST_SIGNAL
};

static guint rcw_signals[LAST_SIGNAL] =
{ 0 };

static RemminaConnectionWindow *rcw_create_scrolled(gint width, gint height, gboolean maximize);
static RemminaConnectionWindow *rcw_create_fullscreen(GtkWindow *old, gint view_mode);
static gboolean rcw_hostkey_func(RemminaProtocolWidget *gp, guint keyval, gboolean release);
static GtkWidget *rco_create_tab_page(RemminaConnectionObject *cnnobj);
static GtkWidget *rco_create_tab_label(RemminaConnectionObject *cnnobj);

void rcw_grab_focus(RemminaConnectionWindow *cnnwin);
static GtkWidget *rcw_create_toolbar(RemminaConnectionWindow *cnnwin, gint mode);
static void rcw_place_toolbar(GtkToolbar *toolbar, GtkGrid *grid, GtkWidget *sibling, int toolbar_placement);
static void rcw_keyboard_grab(RemminaConnectionWindow *cnnwin);
static GtkWidget *rcw_append_new_page(RemminaConnectionWindow *cnnwin, RemminaConnectionObject *cnnobj);


static void rcw_ftb_drag_begin(GtkWidget *widget, GdkDragContext *context, gpointer user_data);

static const GtkTargetEntry dnd_targets_ftb[] =
{
	{
		(char *)"text/x-remmina-ftb",
		GTK_TARGET_SAME_APP | GTK_TARGET_OTHER_WIDGET,
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

static void rcw_class_init(RemminaConnectionWindowClass *klass)
{
	TRACE_CALL(__func__);
	GtkCssProvider *provider;
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

static RemminaConnectionObject *rcw_get_cnnobj_at_page(RemminaConnectionWindow *cnnwin, gint npage)
{
	GtkWidget *po;

	if (!cnnwin->priv->notebook)
		return NULL;
	po = gtk_notebook_get_nth_page(GTK_NOTEBOOK(cnnwin->priv->notebook), npage);
	return g_object_get_data(G_OBJECT(po), "cnnobj");
}

static RemminaConnectionObject *rcw_get_visible_cnnobj(RemminaConnectionWindow *cnnwin)
{
	gint np;

	if (cnnwin != NULL && cnnwin->priv != NULL && cnnwin->priv->notebook != NULL) {
		np = gtk_notebook_get_current_page(GTK_NOTEBOOK(cnnwin->priv->notebook));
		if (np < 0)
			return NULL;
		return rcw_get_cnnobj_at_page(cnnwin, np);
	} else {
		return NULL;
	}
}

static RemminaScaleMode get_current_allowed_scale_mode(RemminaConnectionObject *cnnobj, gboolean *dynres_avail, gboolean *scale_avail)
{
	TRACE_CALL(__func__);
	RemminaScaleMode scalemode;
	gboolean plugin_has_dynres, plugin_can_scale;

	scalemode = remmina_protocol_widget_get_current_scale_mode(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));

	plugin_has_dynres = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
									  REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);

	plugin_can_scale = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
									 REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);

	/* Forbid scalemode REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES when not possible */
	if ((!plugin_has_dynres) && scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE;

	/* Forbid scalemode REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED when not possible */
	if (!plugin_can_scale && scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE;

	if (scale_avail)
		*scale_avail = plugin_can_scale;
	if (dynres_avail)
		*dynres_avail = (plugin_has_dynres && cnnobj->dynres_unlocked);

	return scalemode;
}

static void rco_disconnect_current_page(RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);

	/* Disconnects the connection which is currently in view in the notebook */
	remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
}

static void rcw_keyboard_ungrab(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	GdkDisplay *display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkDevice *keyboard = NULL;

	if (cnnwin->priv->grab_retry_eventsourceid) {
		g_source_remove(cnnwin->priv->grab_retry_eventsourceid);
		cnnwin->priv->grab_retry_eventsourceid = 0;
	}

	display = gtk_widget_get_display(GTK_WIDGET(cnnwin));
#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(display);
	keyboard = gdk_seat_get_pointer(seat);
#else
	manager = gdk_display_get_device_manager(display);
	keyboard = gdk_device_manager_get_client_pointer(manager);
#endif

	if (!cnnwin->priv->kbcaptured)
		return;

	if (keyboard != NULL) {
		if (gdk_device_get_source(keyboard) != GDK_SOURCE_KEYBOARD)
			keyboard = gdk_device_get_associated_device(keyboard);

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
		cnnwin->priv->kbcaptured = FALSE;
	}
}



static gboolean rcw_keyboard_grab_retry(gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *cnnwin = (RemminaConnectionWindow *)user_data;

	rcw_keyboard_grab(cnnwin);
	cnnwin->priv->grab_retry_eventsourceid = 0;
	return G_SOURCE_REMOVE;
}

static void rcw_keyboard_grab(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	GdkDisplay *display;
#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkGrabStatus ggs;
	GdkDevice *keyboard = NULL;

	if (cnnwin->priv->kbcaptured || !cnnwin->priv->mouse_pointer_entered)
		return;

	display = gtk_widget_get_display(GTK_WIDGET(cnnwin));
#if GTK_CHECK_VERSION(3, 20, 0)
	seat = gdk_display_get_default_seat(display);
	keyboard = gdk_seat_get_pointer(seat);
#else
	manager = gdk_display_get_device_manager(display);
	keyboard = gdk_device_manager_get_client_pointer(manager);
#endif

	if (keyboard != NULL) {
		if (gdk_device_get_source(keyboard) != GDK_SOURCE_KEYBOARD)
			keyboard = gdk_device_get_associated_device(keyboard);


#if DEBUG_KB_GRABBING
		printf("DEBUG_KB_GRABBING: profile asks for grabbing, let’s try.\n");
#endif
		/* Up to GTK version 3.20 we can grab the keyboard with gdk_device_grab().
		 * in GTK 3.20 gdk_seat_grab() should be used instead of gdk_device_grab().
		 * There is a bug in GTK up to 3.22: When gdk_device_grab() fails
		 * the widget is hidden:
		 * https://gitlab.gnome.org/GNOME/gtk/commit/726ad5a5ae7c4f167e8dd454cd7c250821c400ab
		 * The bugfix will be released with GTK 3.24.
		 * Also pease note that the newer gdk_seat_grab() is still calling gdk_device_grab().
		 *
		 * Warning: gdk_seat_grab() will call XGrabKeyboard() or XIGrabDevice()
		 * which in turn will generate a core X input event FocusOut and FocusIn
		 * but not Xinput2 events.
		 * In some cases, GTK is unable to neutralize FocusIn and FocusOut core
		 * events (ie: i3wm+Plasma with GDK_CORE_DEVICE_EVENTS=1 because detail=NotifyNonlinear
		 * instead of detail=NotifyAncestor/detail=NotifyInferior)
		 * Receiving a FocusOut event for Remmina at this time will cause an infinite loop.
		 * Therefore is important for GTK to use Xinput2 insetead of core X events
		 * by unsetting GDK_CORE_DEVICE_EVENTS
		 */
#if GTK_CHECK_VERSION(3, 24, 0)
		ggs = gdk_seat_grab(seat, gtk_widget_get_window(GTK_WIDGET(cnnwin)),
				    GDK_SEAT_CAPABILITY_ALL, TRUE, NULL, NULL, NULL, NULL);

#else
		G_GNUC_BEGIN_IGNORE_DEPRECATIONS
			ggs = gdk_device_grab(keyboard, gtk_widget_get_window(GTK_WIDGET(cnnwin)), GDK_OWNERSHIP_WINDOW,
					      TRUE, GDK_KEY_PRESS | GDK_KEY_RELEASE, NULL, GDK_CURRENT_TIME);
		G_GNUC_END_IGNORE_DEPRECATIONS
#endif
		if (ggs != GDK_GRAB_SUCCESS) {
			/* Failure to GRAB keyboard */
#if DEBUG_KB_GRABBING
			printf("GRAB FAILED. GdkGrabStatus: %d\n", ggs);
			printf("GRAB FAILED. GdkGrabStatus: %d\n", (int)ggs);
#endif
			/* Reschedule grabbing in half a second if not already done */
			if (cnnwin->priv->grab_retry_eventsourceid == 0)
				cnnwin->priv->grab_retry_eventsourceid = g_timeout_add(500, (GSourceFunc)rcw_keyboard_grab_retry, cnnwin);
		} else {
#if DEBUG_KB_GRABBING
			printf("Keyboard grabbed\n");
#endif
			if (cnnwin->priv->grab_retry_eventsourceid != 0) {
				g_source_remove(cnnwin->priv->grab_retry_eventsourceid);
				cnnwin->priv->grab_retry_eventsourceid = 0;
			}
			cnnwin->priv->kbcaptured = TRUE;
		}
	} else {
		rcw_keyboard_ungrab(cnnwin);
	}
}

static void rcw_close_all_connections(RemminaConnectionWindow *cnnwin)
{
	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	GtkNotebook *notebook = GTK_NOTEBOOK(priv->notebook);
	GtkWidget *w;
	RemminaConnectionObject *cnnobj;
	gint i, n;

	if (GTK_IS_WIDGET(notebook)) {
		n = gtk_notebook_get_n_pages(notebook);
		for (i = n - 1; i >= 0; i--) {
			w = gtk_notebook_get_nth_page(notebook, i);
			cnnobj = (RemminaConnectionObject *)g_object_get_data(G_OBJECT(w), "cnnobj");
			/* Do close the connection on this tab */
			remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
		}
	}
}

gboolean rcw_delete(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	GtkNotebook *notebook = GTK_NOTEBOOK(priv->notebook);
	GtkWidget *dialog;
	gint i, n;

	if (!REMMINA_IS_CONNECTION_WINDOW(cnnwin))
		return TRUE;

	if (cnnwin->priv->on_delete_confirm_mode != RCW_ONDELETE_NOCONFIRM) {
		n = gtk_notebook_get_n_pages(notebook);
		if (n > 1) {
			dialog = gtk_message_dialog_new(GTK_WINDOW(cnnwin), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
							GTK_BUTTONS_YES_NO,
							_("Are you sure you want to close %i active connections in the current window?"), n);
			i = gtk_dialog_run(GTK_DIALOG(dialog));
			gtk_widget_destroy(dialog);
			if (i != GTK_RESPONSE_YES)
				return FALSE;
		}
	}
	rcw_close_all_connections(cnnwin);

	return TRUE;
}

static gboolean rcw_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	TRACE_CALL(__func__);
	rcw_delete(RCW(widget));
	return TRUE;
}

static void rcw_destroy(GtkWidget *widget, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv;
	RemminaConnectionWindow *cnnwin;

	if (!REMMINA_IS_CONNECTION_WINDOW(widget))
		return;

	cnnwin = (RemminaConnectionWindow *)widget;
	priv = cnnwin->priv;

	if (priv->kbcaptured)
		rcw_keyboard_ungrab(cnnwin);

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
	if (priv->switch_page_finalsel_handler) {
		g_source_remove(priv->switch_page_finalsel_handler);
		priv->switch_page_finalsel_handler = 0;
	}

	cnnwin->priv = NULL;
	g_free(priv);
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
	RemminaConnectionWindowPriv *priv;
	RemminaConnectionWindow *cnnwin;


	cnnwin = (RemminaConnectionWindow *)user_data;
	priv = cnnwin->priv;

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
	RemminaConnectionWindowPriv *priv;
	RemminaConnectionWindow *cnnwin;

	cnnwin = (RemminaConnectionWindow *)user_data;
	priv = cnnwin->priv;

	gtk_widget_get_allocation(widget, &wa);

	if (wa.width * y >= wa.height * x) {
		if (wa.width * y > wa.height * (wa.width - x))
			new_toolbar_placement = TOOLBAR_PLACEMENT_BOTTOM;
		else
			new_toolbar_placement = TOOLBAR_PLACEMENT_LEFT;
	} else {
		if (wa.width * y > wa.height * (wa.width - x))
			new_toolbar_placement = TOOLBAR_PLACEMENT_RIGHT;
		else
			new_toolbar_placement = TOOLBAR_PLACEMENT_TOP;
	}

	gtk_drag_finish(context, TRUE, TRUE, time);

	if (new_toolbar_placement != remmina_pref.toolbar_placement) {
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
	cairo_set_dash(cr, dashes, 1, 0);
	cairo_rectangle(cr, 0, 0, 16, 16);
	cairo_stroke(cr);
	cairo_destroy(cr);

	gtk_widget_hide(widget);

	gtk_drag_set_icon_surface(context, surface);
}

void rcw_update_toolbar_opacity(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	RemminaConnectionObject *cnnobj;

	cnnobj = rcw_get_visible_cnnobj(cnnwin);
	if (!cnnobj) return;

	priv->floating_toolbar_opacity = (1.0 - TOOLBAR_OPACITY_MIN) / ((gdouble)TOOLBAR_OPACITY_LEVEL)
					 * ((gdouble)(TOOLBAR_OPACITY_LEVEL - remmina_file_get_int(cnnobj->remmina_file, "toolbar_opacity", 0)))
					 + TOOLBAR_OPACITY_MIN;
	if (priv->floating_toolbar_widget)
		gtk_widget_set_opacity(GTK_WIDGET(priv->overlay_ftb_overlay), priv->floating_toolbar_opacity);
}

static gboolean rcw_floating_toolbar_make_invisible(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = (RemminaConnectionWindowPriv *)data;
	gtk_widget_set_opacity(GTK_WIDGET(priv->overlay_ftb_overlay), 0.0);
	priv->ftb_hide_eventsource = 0;
	return FALSE;
}

static void rcw_floating_toolbar_show(RemminaConnectionWindow *cnnwin, gboolean show)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;

	if (priv->floating_toolbar_widget == NULL)
		return;

	if (show || priv->pin_down) {
		/* Make the FTB no longer transparent, in case we have an hidden toolbar */
		rcw_update_toolbar_opacity(cnnwin);
		/* Remove outstanding hide events, if not yet active */
		if (priv->ftb_hide_eventsource) {
			g_source_remove(priv->ftb_hide_eventsource);
			priv->ftb_hide_eventsource = 0;
		}
	} else {
		/* If we are hiding and the toolbar must be made invisible, schedule
		 * a later toolbar hide */
		if (remmina_pref.fullscreen_toolbar_visibility == FLOATING_TOOLBAR_VISIBILITY_INVISIBLE)
			if (priv->ftb_hide_eventsource == 0)
				priv->ftb_hide_eventsource = g_timeout_add(1000, rcw_floating_toolbar_make_invisible, priv);
	}

	gtk_revealer_set_reveal_child(GTK_REVEALER(priv->revealer), show || priv->pin_down);
}

static void rco_get_desktop_size(RemminaConnectionObject *cnnobj, gint *width, gint *height)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);


	*width = remmina_protocol_widget_get_width(gp);
	*height = remmina_protocol_widget_get_height(gp);
	if (*width == 0) {
		/* Before connecting we do not have real remote width/height,
		 * so we ask profile values */
		*width = remmina_protocol_widget_get_profile_remote_width(gp);
		*height = remmina_protocol_widget_get_profile_remote_height(gp);
	}
}

void rco_set_scrolled_policy(RemminaConnectionObject *cnnobj, GtkScrolledWindow *scrolled_window)
{
	TRACE_CALL(__func__);
	RemminaScaleMode scalemode;
	scalemode = get_current_allowed_scale_mode(cnnobj, NULL, NULL);
	gtk_scrolled_window_set_policy(scrolled_window,
				       scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC,
				       scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC);
}

static GtkWidget *rco_create_scrolled_container(RemminaConnectionObject *cnnobj, int view_mode)
{
	GtkWidget *scrolled_container;

	if (view_mode == VIEWPORT_FULLSCREEN_MODE) {
		scrolled_container = remmina_scrolled_viewport_new();
	} else {
		scrolled_container = gtk_scrolled_window_new(NULL, NULL);
		rco_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(scrolled_container));
		gtk_container_set_border_width(GTK_CONTAINER(scrolled_container), 0);
		gtk_widget_set_can_focus(scrolled_container, FALSE);
	}

	gtk_widget_set_name(scrolled_container, "remmina-scrolled-container");
	gtk_widget_show(scrolled_container);

	return scrolled_container;
}

gboolean rcw_toolbar_autofit_restore(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);

	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	RemminaConnectionObject *cnnobj;
	gint dwidth, dheight;
	GtkAllocation nba, ca, ta;

	if (priv->toolbar_is_reconfiguring)
		return FALSE;

	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return FALSE;

	if (cnnobj->connected && GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container)) {
		rco_get_desktop_size(cnnobj, &dwidth, &dheight);
		gtk_widget_get_allocation(GTK_WIDGET(priv->notebook), &nba);
		gtk_widget_get_allocation(cnnobj->scrolled_container, &ca);
		gtk_widget_get_allocation(priv->toolbar, &ta);
		if (remmina_pref.toolbar_placement == TOOLBAR_PLACEMENT_LEFT ||
		    remmina_pref.toolbar_placement == TOOLBAR_PLACEMENT_RIGHT)
			gtk_window_resize(GTK_WINDOW(cnnobj->cnnwin), MAX(1, dwidth + ta.width + nba.width - ca.width),
					  MAX(1, dheight + nba.height - ca.height));
		else
			gtk_window_resize(GTK_WINDOW(cnnobj->cnnwin), MAX(1, dwidth + nba.width - ca.width),
					  MAX(1, dheight + ta.height + nba.height - ca.height));
		gtk_container_check_resize(GTK_CONTAINER(cnnobj->cnnwin));
	}
	if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container))
		rco_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(cnnobj->scrolled_container));
	return FALSE;
}

static void rcw_toolbar_autofit(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container)) {
		if ((gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(cnnwin))) & GDK_WINDOW_STATE_MAXIMIZED) != 0)
			gtk_window_unmaximize(GTK_WINDOW(cnnwin));

		/* It’s tricky to make the toolbars disappear automatically, while keeping scrollable.
		 * Please tell me if you know a better way to do this */
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container), GTK_POLICY_NEVER,
					       GTK_POLICY_NEVER);

		/** @todo save returned source id in priv->something and then delete when main object is destroyed */
		g_timeout_add(200, (GSourceFunc)rcw_toolbar_autofit_restore, cnnwin);
	}
}

void rco_get_monitor_geometry(RemminaConnectionObject *cnnobj, GdkRectangle *sz)
{
	TRACE_CALL(__func__);

	/* Fill sz with the monitor (or workarea) size and position
	 * of the monitor (or workarea) where cnnobj->cnnwin is located */

	GdkRectangle monitor_geometry;

	sz->x = sz->y = sz->width = sz->height = 0;

	if (!cnnobj)
		return;
	if (!cnnobj->cnnwin)
		return;
	if (!gtk_widget_is_visible(GTK_WIDGET(cnnobj->cnnwin)))
		return;

#if GTK_CHECK_VERSION(3, 22, 0)
	GdkDisplay *display;
	GdkMonitor *monitor;
	display = gtk_widget_get_display(GTK_WIDGET(cnnobj->cnnwin));
	monitor = gdk_display_get_monitor_at_window(display, gtk_widget_get_window(GTK_WIDGET(cnnobj->cnnwin)));
#else
	GdkScreen *screen;
	gint monitor;
	screen = gtk_window_get_screen(GTK_WINDOW(cnnobj->cnnwin));
	monitor = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(GTK_WIDGET(cnnobj->cnnwin)));
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

static void rco_check_resize(RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	gboolean scroll_required = FALSE;

	GdkRectangle monitor_geometry;
	gint rd_width, rd_height;
	gint bordersz;
	gint scalemode;

	scalemode = remmina_protocol_widget_get_current_scale_mode(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));

	/* Get remote destkop size */
	rco_get_desktop_size(cnnobj, &rd_width, &rd_height);

	/* Get our monitor size */
	rco_get_monitor_geometry(cnnobj, &monitor_geometry);

	if (!remmina_protocol_widget_get_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto)) &&
	    (monitor_geometry.width < rd_width || monitor_geometry.height < rd_height) &&
	    scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE)
		scroll_required = TRUE;

	switch (cnnobj->cnnwin->priv->view_mode) {
	case SCROLLED_FULLSCREEN_MODE:
		gtk_window_resize(GTK_WINDOW(cnnobj->cnnwin), monitor_geometry.width, monitor_geometry.height);
		gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(cnnobj->scrolled_container),
					       (scroll_required ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER),
					       (scroll_required ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER));
		break;

	case VIEWPORT_FULLSCREEN_MODE:
		bordersz = scroll_required ? SCROLL_BORDER_SIZE : 0;
		gtk_window_resize(GTK_WINDOW(cnnobj->cnnwin), monitor_geometry.width, monitor_geometry.height);
		if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container))
			/* Put a border around Notebook content (RemminaScrolledViewpord), so we can
			 * move the mouse over the border to scroll */
			gtk_container_set_border_width(GTK_CONTAINER(cnnobj->scrolled_container), bordersz);

		break;

	case SCROLLED_WINDOW_MODE:
		if (remmina_file_get_int(cnnobj->remmina_file, "viewmode", UNDEFINED_MODE) == UNDEFINED_MODE) {
			/* ToDo: is this really needed ? When ? */
			gtk_window_set_default_size(GTK_WINDOW(cnnobj->cnnwin),
						    MIN(rd_width, monitor_geometry.width), MIN(rd_height, monitor_geometry.height));
			if (rd_width >= monitor_geometry.width || rd_height >= monitor_geometry.height) {
				gtk_window_maximize(GTK_WINDOW(cnnobj->cnnwin));
				remmina_file_set_int(cnnobj->remmina_file, "window_maximize", TRUE);
			} else {
				rcw_toolbar_autofit(NULL, cnnobj->cnnwin);
				remmina_file_set_int(cnnobj->remmina_file, "window_maximize", FALSE);
			}
		} else {
			if (remmina_file_get_int(cnnobj->remmina_file, "window_maximize", FALSE))
				gtk_window_maximize(GTK_WINDOW(cnnobj->cnnwin));
		}
		break;

	default:
		break;
	}
}

static void rcw_set_tooltip(GtkWidget *item, const gchar *tip, guint key1, guint key2)
{
	TRACE_CALL(__func__);
	gchar *s1;
	gchar *s2;

	if (remmina_pref.hostkey && key1) {
		if (key2)
			s1 = g_strdup_printf(" (%s + %s,%s)", gdk_keyval_name(remmina_pref.hostkey),
					     gdk_keyval_name(gdk_keyval_to_upper(key1)), gdk_keyval_name(gdk_keyval_to_upper(key2)));
		else if (key1 == remmina_pref.hostkey)
			s1 = g_strdup_printf(" (%s)", gdk_keyval_name(remmina_pref.hostkey));
		else
			s1 = g_strdup_printf(" (%s + %s)", gdk_keyval_name(remmina_pref.hostkey),
					     gdk_keyval_name(gdk_keyval_to_upper(key1)));
	} else {
		s1 = NULL;
	}
	s2 = g_strdup_printf("%s%s", tip, s1 ? s1 : "");
	gtk_widget_set_tooltip_text(item, s2);
	g_free(s2);
	g_free(s1);
}

static void remmina_protocol_widget_update_alignment(RemminaConnectionObject *cnnobj)
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
	} else {
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
				if (cnnobj != NULL && cnnobj->cnnwin != NULL && cnnobj->cnnwin->priv->notebook != NULL)
					rcw_grab_focus(cnnobj->cnnwin);
			} else {
				gtk_aspect_frame_set(GTK_ASPECT_FRAME(cnnobj->aspectframe), 0.5, 0.5, aratio, FALSE);
			}
		} else {
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
				if (cnnobj != NULL && cnnobj->cnnwin != NULL && cnnobj->cnnwin->priv->notebook != NULL)
					rcw_grab_focus(cnnobj->cnnwin);
			}
		}

		if (scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED || scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES) {
			/* We have a plugin that can be scaled, and the scale button
			 * has been pressed. Give it the correct WxH maintaining aspect
			 * ratio of remote destkop size */
			gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);
			gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);
		} else {
			/* Plugin can scale, but no scaling is active. Ensure that we have
			 * aspectframe with a ratio of 1 */
			gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_CENTER);
			gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_CENTER);
		}
	}
}

static void nb_set_current_page(GtkNotebook *notebook, GtkWidget *page)
{
	gint np, i;

	np = gtk_notebook_get_n_pages(notebook);
	for (i = 0; i < np; i++) {
		if (gtk_notebook_get_nth_page(notebook, i) == page) {
			gtk_notebook_set_current_page(notebook, i);
			break;
		}
	}
}

static void nb_migrate_page_content(GtkWidget *frompage, GtkWidget *topage)
{
	/* Migrate a single connection tab from a nothebook to another one */
	GList *lst, *l;
	RemminaConnectionObject *cnnobj;

	cnnobj = (RemminaConnectionObject *)g_object_get_data(G_OBJECT(frompage), "cnnobj");

	/* Reparent message panels */
	lst = gtk_container_get_children(GTK_CONTAINER(frompage));
	for (l = lst; l != NULL; l = l->next) {
		if (REMMINA_IS_MESSAGE_PANEL(l->data)) {
			g_object_ref(l->data);
			gtk_container_remove(GTK_CONTAINER(frompage), GTK_WIDGET(l->data));
			gtk_container_add(GTK_CONTAINER(topage), GTK_WIDGET(l->data));
			g_object_unref(l->data);
			gtk_box_reorder_child(GTK_BOX(topage), GTK_WIDGET(l->data), 0);
		}
	}
	g_list_free(lst);

	/* Reparent the viewport (which is inside scrolled_container inside frompage */
	G_GNUC_BEGIN_IGNORE_DEPRECATIONS
	gtk_widget_reparent(cnnobj->viewport, cnnobj->scrolled_container);
	G_GNUC_END_IGNORE_DEPRECATIONS
}

static void rcw_migrate(RemminaConnectionWindow *from, RemminaConnectionWindow *to)
{
	/* Migrate a complete notebook from a window to another */

	gchar *tag;
	gint cp, np, i;
	GtkNotebook *from_notebook;
	GtkWidget *frompage, *newpage;
	RemminaConnectionObject *cnnobj;

	/* Migrate TAG */
	tag = g_strdup((gchar *)g_object_get_data(G_OBJECT(from), "tag"));
	g_object_set_data_full(G_OBJECT(to), "tag", tag, (GDestroyNotify)g_free);

	/* Migrate notebook content */
	from_notebook = from->priv->notebook;
	if (from_notebook && GTK_IS_NOTEBOOK(from_notebook)) {
		cp = gtk_notebook_get_current_page(from_notebook);
		np = gtk_notebook_get_n_pages(from_notebook);
		/* Create pages on dest notebook and migrate
		 * page content */
		for (i = 0; i < np; i++) {
			frompage = gtk_notebook_get_nth_page(from_notebook, i);
			cnnobj = g_object_get_data(G_OBJECT(frompage), "cnnobj");
			cnnobj->scrolled_container = rco_create_scrolled_container(cnnobj, to->priv->view_mode);
			newpage = rcw_append_new_page(to, cnnobj);
			nb_migrate_page_content(frompage, newpage);
		}
		/* Remove all the pages from source notebook */
		for (i = np - 1; i >= 0; i--)
			gtk_notebook_remove_page(from_notebook, i);
		gtk_notebook_set_current_page(to->priv->notebook, cp);
	}
}

static void rcw_switch_viewmode(RemminaConnectionWindow *cnnwin, int newmode)
{
	GdkWindowState s;
	RemminaConnectionWindow *newwin;
	gint old_width, old_height;
	int old_mode;

	old_mode = cnnwin->priv->view_mode;
	if (old_mode == newmode)
		return;

	if (newmode == VIEWPORT_FULLSCREEN_MODE || newmode == SCROLLED_FULLSCREEN_MODE) {
		if (old_mode == SCROLLED_WINDOW_MODE) {
			/* We are leaving SCROLLED_WINDOW_MODE, save W,H, and maximized
			 * status before self destruction of cnnwin */
			gtk_window_get_size(GTK_WINDOW(cnnwin), &old_width, &old_height);
			s = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(cnnwin)));
		}
		newwin = rcw_create_fullscreen(GTK_WINDOW(cnnwin), cnnwin->priv->fss_view_mode);
		rcw_migrate(cnnwin, newwin);
		if (old_mode == SCROLLED_WINDOW_MODE) {
			newwin->priv->ss_maximized = (s & GDK_WINDOW_STATE_MAXIMIZED) ? TRUE : FALSE;
			newwin->priv->ss_width = old_width;
			newwin->priv->ss_height = old_height;
		}
	} else {
		newwin = rcw_create_scrolled(cnnwin->priv->ss_width, cnnwin->priv->ss_height,
					     cnnwin->priv->ss_maximized);
		rcw_migrate(cnnwin, newwin);
		if (old_mode == VIEWPORT_FULLSCREEN_MODE || old_mode == SCROLLED_FULLSCREEN_MODE)
			/* We are leaving a FULLSCREEN mode, save some parameters
			 * status before self destruction of cnnwin */
			newwin->priv->fss_view_mode = old_mode;
	}
}


static void rcw_toolbar_fullscreen(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);

	RemminaConnectionObject *cnnobj;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;

	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		rcw_switch_viewmode(cnnwin, cnnwin->priv->fss_view_mode);
	else
		rcw_switch_viewmode(cnnwin, SCROLLED_WINDOW_MODE);
}

static void rco_viewport_fullscreen_mode(GtkWidget *widget, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *newwin;
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnobj->cnnwin->priv->fss_view_mode = VIEWPORT_FULLSCREEN_MODE;
	newwin = rcw_create_fullscreen(GTK_WINDOW(cnnobj->cnnwin), VIEWPORT_FULLSCREEN_MODE);
	rcw_migrate(cnnobj->cnnwin, newwin);
}

static void rco_scrolled_fullscreen_mode(GtkWidget *widget, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *newwin;
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnobj->cnnwin->priv->fss_view_mode = SCROLLED_FULLSCREEN_MODE;
	newwin = rcw_create_fullscreen(GTK_WINDOW(cnnobj->cnnwin), SCROLLED_FULLSCREEN_MODE);
	rcw_migrate(cnnobj->cnnwin, newwin);
}

static void rcw_fullscreen_option_popdown(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;

	priv->sticky = FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->fullscreen_option_button), FALSE);
	rcw_floating_toolbar_show(cnnwin, FALSE);
}

void rcw_toolbar_fullscreen_option(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList *group;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;

	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		return;

	cnnwin->priv->sticky = TRUE;

	menu = gtk_menu_new();

	menuitem = gtk_radio_menu_item_new_with_label(NULL, _("Viewport fullscreen mode"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
	if (cnnwin->priv->view_mode == VIEWPORT_FULLSCREEN_MODE)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(rco_viewport_fullscreen_mode), cnnobj);

	menuitem = gtk_radio_menu_item_new_with_label(group, _("Scrolled fullscreen"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (cnnwin->priv->view_mode == SCROLLED_FULLSCREEN_MODE)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(rco_scrolled_fullscreen_mode), cnnobj);

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rcw_fullscreen_option_popdown), cnnwin);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
				 GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, cnnwin->priv->toolitem_fullscreen, 0,
		       gtk_get_current_event_time());
#endif
}


static void rcw_scaler_option_popdown(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	if (priv->toolbar_is_reconfiguring)
		return;
	priv->sticky = FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->scaler_option_button), FALSE);
	rcw_floating_toolbar_show(cnnwin, FALSE);
}

static void rcw_scaler_expand(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnobj = rcw_get_visible_cnnobj(cnnwin);
	if (!cnnobj)
		return;
	remmina_protocol_widget_set_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), TRUE);
	remmina_file_set_int(cnnobj->remmina_file, "scaler_expand", TRUE);
	remmina_protocol_widget_update_alignment(cnnobj);
}
static void rcw_scaler_keep_aspect(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnobj = rcw_get_visible_cnnobj(cnnwin);
	if (!cnnobj)
		return;

	remmina_protocol_widget_set_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), FALSE);
	remmina_file_set_int(cnnobj->remmina_file, "scaler_expand", FALSE);
	remmina_protocol_widget_update_alignment(cnnobj);
}

static void rcw_toolbar_scaler_option(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv;
	RemminaConnectionObject *cnnobj;
	GtkWidget *menu;
	GtkWidget *menuitem;
	GSList *group;
	gboolean scaler_expand;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;

	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;
	priv = cnnwin->priv;

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
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(rcw_scaler_keep_aspect), cnnwin);

	menuitem = gtk_radio_menu_item_new_with_label(group, _("Fill client window when scaled"));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if (scaler_expand)
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);
	g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(rcw_scaler_expand), cnnwin);

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rcw_scaler_option_popdown), cnnwin);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
				 GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, priv->toolitem_scale, 0,
		       gtk_get_current_event_time());
#endif
}

void rco_switch_page_activate(GtkMenuItem *menuitem, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnobj->cnnwin->priv;
	gint page_num;

	page_num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), "new-page-num"));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page_num);
}

void rcw_toolbar_switch_page_popdown(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_switch_page), FALSE);
	rcw_floating_toolbar_show(cnnwin, FALSE);
}

static void rcw_toolbar_switch_page(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);

	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	RemminaConnectionObject *cnnobj;

	GtkWidget *menu;
	GtkWidget *menuitem;
	GtkWidget *image;
	gint i, n;

	if (priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	menu = gtk_menu_new();

	n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook));
	for (i = 0; i < n; i++) {
		cnnobj = rcw_get_cnnobj_at_page(cnnobj->cnnwin, i);

		menuitem = gtk_menu_item_new_with_label(remmina_file_get_string(cnnobj->remmina_file, "name"));
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		image = gtk_image_new_from_icon_name(remmina_file_get_icon_name(cnnobj->remmina_file), GTK_ICON_SIZE_MENU);
		gtk_widget_show(image);

		g_object_set_data(G_OBJECT(menuitem), "new-page-num", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(rco_switch_page_activate), cnnobj);
		if (i == gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)))
			gtk_widget_set_sensitive(menuitem, FALSE);
	}

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rcw_toolbar_switch_page_popdown),
			 cnnwin);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
				 GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
#endif
}

void rco_update_toolbar_autofit_button(RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnobj->cnnwin->priv;
	GtkToolItem *toolitem;
	RemminaScaleMode sc;

	toolitem = priv->toolitem_autofit;
	if (toolitem) {
		if (priv->view_mode != SCROLLED_WINDOW_MODE) {
			gtk_widget_set_sensitive(GTK_WIDGET(toolitem), FALSE);
		} else {
			sc = get_current_allowed_scale_mode(cnnobj, NULL, NULL);
			gtk_widget_set_sensitive(GTK_WIDGET(toolitem), sc == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE);
		}
	}
}

static void rco_change_scalemode(RemminaConnectionObject *cnnobj, gboolean bdyn, gboolean bscale)
{
	RemminaScaleMode scalemode;
	RemminaConnectionWindowPriv *priv = cnnobj->cnnwin->priv;

	if (bdyn)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_DYNRES;
	else if (bscale)
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED;
	else
		scalemode = REMMINA_PROTOCOL_WIDGET_SCALE_MODE_NONE;

	remmina_protocol_widget_set_current_scale_mode(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), scalemode);
	remmina_file_set_int(cnnobj->remmina_file, "scale", scalemode);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->scaler_option_button), scalemode == REMMINA_PROTOCOL_WIDGET_SCALE_MODE_SCALED);
	rco_update_toolbar_autofit_button(cnnobj);

	remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
						     REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, 0);

	if (cnnobj->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
		rco_check_resize(cnnobj);
	if (GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container))
		rco_set_scrolled_policy(cnnobj, GTK_SCROLLED_WINDOW(cnnobj->scrolled_container));
}

static void rcw_toolbar_dynres(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	gboolean bdyn, bscale;
	RemminaConnectionObject *cnnobj;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	if (cnnobj->connected) {
		bdyn = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
		bscale = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(cnnobj->cnnwin->priv->toolitem_scale));

		if (bdyn && bscale) {
			gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(cnnobj->cnnwin->priv->toolitem_scale), FALSE);
			bscale = FALSE;
		}

		rco_change_scalemode(cnnobj, bdyn, bscale);
	}
}


static void rcw_toolbar_scaled_mode(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	gboolean bdyn, bscale;
	RemminaConnectionObject *cnnobj;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	bdyn = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(cnnobj->cnnwin->priv->toolitem_dynres));
	bscale = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));

	if (bdyn && bscale) {
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(cnnobj->cnnwin->priv->toolitem_dynres), FALSE);
		bdyn = FALSE;
	}

	rco_change_scalemode(cnnobj, bdyn, bscale);
}

static void rcw_toolbar_preferences_popdown(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	cnnobj->cnnwin->priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(cnnobj->cnnwin->priv->toolitem_preferences), FALSE);
	rcw_floating_toolbar_show(cnnwin, FALSE);
}

void rcw_toolbar_tools_popdown(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;

	if (priv->toolbar_is_reconfiguring)
		return;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_tools), FALSE);
	rcw_floating_toolbar_show(cnnwin, FALSE);
}

static void rco_call_protocol_feature_radio(GtkMenuItem *menuitem, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaProtocolFeature *feature;
	gpointer value;

	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem))) {
		feature = (RemminaProtocolFeature *)g_object_get_data(G_OBJECT(menuitem), "feature-type");
		value = g_object_get_data(G_OBJECT(menuitem), "feature-value");

		remmina_file_set_string(cnnobj->remmina_file, (const gchar *)feature->opt2, (const gchar *)value);
		remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
	}
}

static void rco_call_protocol_feature_check(GtkMenuItem *menuitem, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaProtocolFeature *feature;
	gboolean value;

	feature = (RemminaProtocolFeature *)g_object_get_data(G_OBJECT(menuitem), "feature-type");
	value = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menuitem));
	remmina_file_set_int(cnnobj->remmina_file, (const gchar *)feature->opt2, value);
	remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
}

static void rco_call_protocol_feature_activate(GtkMenuItem *menuitem, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaProtocolFeature *feature;

	feature = (RemminaProtocolFeature *)g_object_get_data(G_OBJECT(menuitem), "feature-type");
	remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
}

void rcw_toolbar_preferences_radio(RemminaConnectionObject *cnnobj, RemminaFile *remminafile,
				   GtkWidget *menu, const RemminaProtocolFeature *feature, const gchar *domain, gboolean enabled)
{
	TRACE_CALL(__func__);
	GtkWidget *menuitem;
	GSList *group;
	gint i;
	const gchar **list;
	const gchar *value;

	group = NULL;
	value = remmina_file_get_string(remminafile, (const gchar *)feature->opt2);
	list = (const gchar **)feature->opt3;
	for (i = 0; list[i]; i += 2) {
		menuitem = gtk_radio_menu_item_new_with_label(group, g_dgettext(domain, list[i + 1]));
		group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(menuitem));
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		if (enabled) {
			g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer)feature);
			g_object_set_data(G_OBJECT(menuitem), "feature-value", (gpointer)list[i]);

			if (value && g_strcmp0(list[i], value) == 0)
				gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem), TRUE);

			g_signal_connect(G_OBJECT(menuitem), "toggled",
					 G_CALLBACK(rco_call_protocol_feature_radio), cnnobj);
		} else {
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
	}
}

void rcw_toolbar_preferences_check(RemminaConnectionObject *cnnobj,
				   GtkWidget *menu, const RemminaProtocolFeature *feature,
				   const gchar *domain, gboolean enabled)
{
	TRACE_CALL(__func__);
	GtkWidget *menuitem;

	menuitem = gtk_check_menu_item_new_with_label(g_dgettext(domain, (const gchar *)feature->opt3));
	gtk_widget_show(menuitem);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	if (enabled) {
		g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer)feature);

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menuitem),
					       remmina_file_get_int(cnnobj->remmina_file, (const gchar *)feature->opt2, FALSE));

		g_signal_connect(G_OBJECT(menuitem), "toggled",
				 G_CALLBACK(rco_call_protocol_feature_check), cnnobj);
	} else {
		gtk_widget_set_sensitive(menuitem, FALSE);
	}
}

static void rcw_toolbar_preferences(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv;
	RemminaConnectionObject *cnnobj;
	const RemminaProtocolFeature *feature;
	GtkWidget *menu;
	GtkWidget *menuitem;
	gboolean separator;
	gchar *domain;
	gboolean enabled;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;
	priv = cnnobj->cnnwin->priv;

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
			rcw_toolbar_preferences_radio(cnnobj, cnnobj->remmina_file, menu, feature,
						      domain, enabled);
			separator = TRUE;
			break;
		case REMMINA_PROTOCOL_FEATURE_PREF_CHECK:
			rcw_toolbar_preferences_check(cnnobj, menu, feature,
						      domain, enabled);
			break;
		}
	}

	g_free(domain);

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rcw_toolbar_preferences_popdown), cnnwin);

#if GTK_CHECK_VERSION(3, 22, 0)
	gtk_menu_popup_at_widget(GTK_MENU(menu), widget,
				 GDK_GRAVITY_SOUTH_WEST, GDK_GRAVITY_NORTH_WEST, NULL);
#else
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
#endif
}

static void rcw_toolbar_tools(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv;
	RemminaConnectionObject *cnnobj;
	const RemminaProtocolFeature *feature;
	GtkWidget *menu;
	GtkWidget *menuitem = NULL;
	GtkMenu *submenu_keystrokes;
	const gchar *domain;
	gboolean enabled;
	gchar **keystrokes;
	gchar **keystroke_values;
	gint i;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;
	priv = cnnobj->cnnwin->priv;

	if (!gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
		return;

	priv->sticky = TRUE;

	domain = remmina_protocol_widget_get_domain(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	menu = gtk_menu_new();
	for (feature = remmina_protocol_widget_get_features(REMMINA_PROTOCOL_WIDGET(cnnobj->proto)); feature && feature->type;
	     feature++) {
		if (feature->type != REMMINA_PROTOCOL_FEATURE_TYPE_TOOL)
			continue;

		if (feature->opt1)
			menuitem = gtk_menu_item_new_with_label(g_dgettext(domain, (const gchar *)feature->opt1));
		if (feature->opt3)
			rcw_set_tooltip(menuitem, "", GPOINTER_TO_UINT(feature->opt3), 0);
		gtk_widget_show(menuitem);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

		enabled = remmina_protocol_widget_query_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
		if (enabled) {
			g_object_set_data(G_OBJECT(menuitem), "feature-type", (gpointer)feature);

			g_signal_connect(G_OBJECT(menuitem), "activate",
					 G_CALLBACK(rco_call_protocol_feature_activate), cnnobj);
		} else {
			gtk_widget_set_sensitive(menuitem, FALSE);
		}
	}

	g_signal_connect(G_OBJECT(menu), "deactivate", G_CALLBACK(rcw_toolbar_tools_popdown), cnnwin);

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

static void rcw_toolbar_duplicate(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);

	//RemminaProtocolWidget *gp;
	RemminaConnectionObject *cnnobj;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	// We will duplicate the currently displayed RemminaProtocolWidget.
	//gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);

	remmina_exec_command(REMMINA_COMMAND_CONNECT, cnnobj->remmina_file->filename);
}
static void rcw_toolbar_screenshot(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);

	GdkPixbuf *screenshot;
	GdkWindow *active_window;
	cairo_t *cr;
	gint width, height;
	GString *pngstr;
	gchar *pngname;
	GtkWidget *dialog;
	RemminaProtocolWidget *gp;
	RemminaPluginScreenshotData rpsd;
	RemminaConnectionObject *cnnobj;
	cairo_surface_t *srcsurface;
	cairo_format_t cairo_format;
	cairo_surface_t *surface;
	int stride;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	GDateTime *date = g_date_time_new_now_utc();

	// We will take a screenshot of the currently displayed RemminaProtocolWidget.
	gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);

	GtkClipboard *c = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
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
		if (!remmina_pref.deny_screenshot_clipboard)
			gtk_clipboard_set_image(c, gdk_pixbuf_get_from_surface(
							srcsurface, 0, 0, width, height));
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
							_("Turn off scaling to avoid screenshot distortion."));
			g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
			gtk_widget_show(dialog);
		}

		// Get the screenshot.
		active_window = gtk_widget_get_window(GTK_WIDGET(gp));
		// width = gdk_window_get_width(gtk_widget_get_window(GTK_WIDGET(cnnobj->cnnwin)));
		width = gdk_window_get_width(active_window);
		// height = gdk_window_get_height(gtk_widget_get_window(GTK_WIDGET(cnnobj->cnnwin)));
		height = gdk_window_get_height(active_window);

		screenshot = gdk_pixbuf_get_from_window(active_window, 0, 0, width, height);
		if (screenshot == NULL)
			g_print("gdk_pixbuf_get_from_window failed\n");

		// Transfer the PixBuf in the main clipboard selection
		if (!remmina_pref.deny_screenshot_clipboard)
			gtk_clipboard_set_image(c, screenshot);
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
	if (g_file_test(pngname, G_FILE_TEST_EXISTS))
		remmina_public_send_notification("remmina-screenshot-is-ready-id", _("Screenshot taken"), pngname);

	//Clean up and return.
	cairo_destroy(cr);
	cairo_surface_destroy(surface);
}

static void rcw_toolbar_minimize(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;

	rcw_floating_toolbar_show(cnnwin, FALSE);
	gtk_window_iconify(GTK_WINDOW(cnnwin));
}

static void rcw_toolbar_disconnect(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;
	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;
	rco_disconnect_current_page(cnnobj);
}

static void rcw_toolbar_grab(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	gboolean capture;
	RemminaConnectionObject *cnnobj;

	if (cnnwin->priv->toolbar_is_reconfiguring)
		return;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	capture = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
	remmina_file_set_int(cnnobj->remmina_file, "keyboard_grab", capture);
	if (capture && cnnobj->connected) {
#if DEBUG_KB_GRABBING
		printf("DEBUG_KB_GRABBING: Grabbing for button\n");
#endif
		rcw_keyboard_grab(cnnobj->cnnwin);
	} else {
		rcw_keyboard_ungrab(cnnobj->cnnwin);
	}
}

static GtkWidget *
rcw_create_toolbar(RemminaConnectionWindow *cnnwin, gint mode)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	GtkWidget *toolbar;
	GtkToolItem *toolitem;
	GtkWidget *widget;
	GtkWidget *arrow;

	priv->toolbar_is_reconfiguring = TRUE;

	toolbar = gtk_toolbar_new();
	gtk_widget_show(toolbar);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_toolbar_set_icon_size(GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_SMALL_TOOLBAR);
	gtk_toolbar_set_show_arrow(GTK_TOOLBAR(toolbar), FALSE);

	/* Auto-Fit */
	toolitem = gtk_tool_button_new(NULL, NULL);
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-fit-window-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Resize the window to fit in remote resolution"),
			remmina_pref.shortcutkey_autofit, 0);
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rcw_toolbar_autofit), cnnwin);
	priv->toolitem_autofit = toolitem;
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));


	/* Fullscreen toggle */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-fullscreen-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Toggle fullscreen mode"),
			remmina_pref.shortcutkey_fullscreen, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	priv->toolitem_fullscreen = toolitem;
	if (kioskmode) {
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolitem), FALSE);
	} else {
		gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolitem), mode != SCROLLED_WINDOW_MODE);
		g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rcw_toolbar_fullscreen), cnnwin);
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
	if (remmina_pref.small_toolbutton)
		gtk_widget_set_name(widget, "remmina-small-button");

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
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(rcw_toolbar_fullscreen_option), cnnwin);
	priv->fullscreen_option_button = widget;
	if (mode == SCROLLED_WINDOW_MODE)
		gtk_widget_set_sensitive(GTK_WIDGET(widget), FALSE);

	/* Switch tabs */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-switch-page-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Switch tab pages"), remmina_pref.shortcutkey_prevtab,
			remmina_pref.shortcutkey_nexttab);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rcw_toolbar_switch_page), cnnwin);
	priv->toolitem_switch_page = toolitem;

	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));

	/* Dynamic Resolution Update */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-dynres-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Toggle dynamic resolution update"),
			remmina_pref.shortcutkey_dynres, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rcw_toolbar_dynres), cnnwin);
	priv->toolitem_dynres = toolitem;

	/* Scaler button */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-scale-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Toggle scaled mode"), remmina_pref.shortcutkey_scale, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rcw_toolbar_scaled_mode), cnnwin);
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
		gtk_widget_set_name(widget, "remmina-small-button");
	gtk_container_add(GTK_CONTAINER(toolitem), widget);
#if GTK_CHECK_VERSION(3, 14, 0)
	arrow = gtk_image_new_from_icon_name("remmina-pan-down-symbolic", GTK_ICON_SIZE_SMALL_TOOLBAR);
#else
	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
#endif
	gtk_widget_show(arrow);
	gtk_container_add(GTK_CONTAINER(widget), arrow);
	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(rcw_toolbar_scaler_option), cnnwin);
	priv->scaler_option_button = widget;

	/* Grab keyboard button */
	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-keyboard-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Grab all keyboard events"),
			remmina_pref.shortcutkey_grab, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rcw_toolbar_grab), cnnwin);
	priv->toolitem_grab = toolitem;

	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-preferences-system-symbolic");
	gtk_tool_item_set_tooltip_text(toolitem, _("Preferences"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rcw_toolbar_preferences), cnnwin);
	priv->toolitem_preferences = toolitem;

	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-system-run-symbolic");
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(toolitem), _("Tools"));
	gtk_tool_item_set_tooltip_text(toolitem, _("Tools"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(rcw_toolbar_tools), cnnwin);
	priv->toolitem_tools = toolitem;

	toolitem = gtk_separator_tool_item_new();
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));

	toolitem = gtk_tool_button_new(NULL, "Duplicate connection");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-duplicate-symbolic");
	gtk_tool_item_set_tooltip_text(toolitem, _("Duplicate current connection"));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rcw_toolbar_duplicate), cnnwin);
	priv->toolitem_duplicate = toolitem;

	toolitem = gtk_tool_button_new(NULL, "_Screenshot");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-camera-photo-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Screenshot"), remmina_pref.shortcutkey_screenshot, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rcw_toolbar_screenshot), cnnwin);
	priv->toolitem_screenshot = toolitem;

	toolitem = gtk_tool_button_new(NULL, "_Bottom");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-go-bottom-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Minimize window"), remmina_pref.shortcutkey_minimize, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rcw_toolbar_minimize), cnnwin);
	if (kioskmode)
		gtk_widget_set_sensitive(GTK_WIDGET(toolitem), FALSE);

	toolitem = gtk_tool_button_new(NULL, "_Disconnect");
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-disconnect-symbolic");
	rcw_set_tooltip(GTK_WIDGET(toolitem), _("Disconnect"), remmina_pref.shortcutkey_disconnect, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "clicked", G_CALLBACK(rcw_toolbar_disconnect), cnnwin);

	priv->toolbar_is_reconfiguring = FALSE;
	return toolbar;
}

static void rcw_place_toolbar(GtkToolbar *toolbar, GtkGrid *grid, GtkWidget *sibling, int toolbar_placement)
{
	/* Place the toolbar inside the grid and set its orientation */

	if (toolbar_placement == TOOLBAR_PLACEMENT_LEFT || toolbar_placement == TOOLBAR_PLACEMENT_RIGHT)
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

static void rco_update_toolbar(RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnobj->cnnwin->priv;
	GtkToolItem *toolitem;
	gboolean bval, dynres_avail, scale_avail;
	gboolean test_floating_toolbar;
	RemminaScaleMode scalemode;

	priv->toolbar_is_reconfiguring = TRUE;

	rco_update_toolbar_autofit_button(cnnobj);


	toolitem = priv->toolitem_switch_page;
	if (kioskmode)
		bval = FALSE;
	else
		bval = (gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)) > 1);
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
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval && cnnobj->connected);

	toolitem = priv->toolitem_tools;
	bval = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
							     REMMINA_PROTOCOL_FEATURE_TYPE_TOOL);
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval && cnnobj->connected);

	gtk_widget_set_sensitive(GTK_WIDGET(priv->toolitem_screenshot), cnnobj->connected);

	gtk_window_set_title(GTK_WINDOW(cnnobj->cnnwin), remmina_file_get_string(cnnobj->remmina_file, "name"));

	test_floating_toolbar = (priv->floating_toolbar_widget != NULL);
	if (test_floating_toolbar)
		gtk_label_set_text(GTK_LABEL(priv->floating_toolbar_label),
				   remmina_file_get_string(cnnobj->remmina_file, "name"));

	priv->toolbar_is_reconfiguring = FALSE;
}

static void rcw_set_toolbar_visibility(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;

	if (priv->view_mode == SCROLLED_WINDOW_MODE) {
		if (remmina_pref.hide_connection_toolbar)
			gtk_widget_hide(priv->toolbar);
		else
			gtk_widget_show(priv->toolbar);
	}
}

static gboolean rcw_floating_toolbar_on_enter(GtkWidget *widget, GdkEventCrossing *event,
					      RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	rcw_floating_toolbar_show(cnnwin, TRUE);
	return TRUE;
}

gboolean rco_enter_protocol_widget(GtkWidget *widget, GdkEventCrossing *event,
				   RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnobj->cnnwin->priv;
	if (!priv->sticky && event->mode == GDK_CROSSING_NORMAL) {
		rcw_floating_toolbar_show(cnnobj->cnnwin, FALSE);
		return TRUE;
	}
	return FALSE;
}

static void rcw_focus_in(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	if (cnnobj && cnnobj->connected && remmina_file_get_int(cnnobj->remmina_file, "keyboard_grab", FALSE))
		rcw_keyboard_grab(cnnobj->cnnwin);
}

static void rcw_focus_out(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	rcw_keyboard_ungrab(cnnwin);

	cnnwin->priv->hostkey_activated = FALSE;
	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container))
		remmina_scrolled_viewport_remove_motion(REMMINA_SCROLLED_VIEWPORT(cnnobj->scrolled_container));

	if (cnnobj->proto && cnnobj->scrolled_container)
		remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
							     REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, 0);
}

static gboolean rcw_focus_out_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	TRACE_CALL(__func__);
#if DEBUG_KB_GRABBING
	RemminaConnectionObject *cnnobj;
	cnnobj = rcw_get_visible_cnnobj((RemminaConnectionWindow *)widget);
	printf("DEBUG_KB_GRABBING: Focus out and mouse_pointer_entered is %s\n", cnnobj->cnnwin->priv->mouse_pointer_entered ? "true" : "false");
#endif
	if (REMMINA_IS_CONNECTION_WINDOW(widget))
		rcw_focus_out((RemminaConnectionWindow *)widget);
	return FALSE;
}

static gboolean rcw_focus_in_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	TRACE_CALL(__func__);
#if DEBUG_KB_GRABBING
	RemminaConnectionObject *cnnobj;
	cnnobj = rcw_get_visible_cnnobj((RemminaConnectionWindow *)widget);
	printf("DEBUG_KB_GRABBING: Focus in and mouse_pointer_entered is %s\n", cnnobj->cnnwin->priv->mouse_pointer_entered ? "true" : "false");
#endif
	if (REMMINA_IS_CONNECTION_WINDOW(widget))
		rcw_focus_in((RemminaConnectionWindow *)widget);
	return FALSE;
}

static gboolean rcw_on_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;
	RemminaConnectionWindow *cnnwin;

	if (!REMMINA_IS_CONNECTION_WINDOW(widget))
		return FALSE;

	cnnwin = (RemminaConnectionWindow *)widget;

	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return FALSE;

	cnnwin->priv->mouse_pointer_entered = TRUE;

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
	if (gtk_window_is_active(GTK_WINDOW(cnnobj->cnnwin)))
		if (cnnobj->connected && remmina_file_get_int(cnnobj->remmina_file, "keyboard_grab", FALSE))
			rcw_keyboard_grab(cnnwin);
	return FALSE;
}

static gboolean rcw_on_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *cnnwin;

	if (!REMMINA_IS_CONNECTION_WINDOW(widget))
		return FALSE;
	cnnwin = (RemminaConnectionWindow *)widget;

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
	 * Unity: We leave windows with GDK_NOTIFY_VIRTUAL or GDK_NOTIFY_NONLINEAR_VIRTUAL
	 * Gnome shell: We leave windows with both GDK_NOTIFY_VIRTUAL or GDK_NOTIFY_ANCESTOR
	 * Xfce: We cannot drag this window when grabbed, so we need to ungrab in response to GDK_NOTIFY_NONLINEAR
	 */
	if (event->detail == GDK_NOTIFY_VIRTUAL || event->detail == GDK_NOTIFY_ANCESTOR ||
	    event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL || event->detail == GDK_NOTIFY_NONLINEAR) {
		cnnwin->priv->mouse_pointer_entered = FALSE;
		rcw_keyboard_ungrab(cnnwin);
	}
	return FALSE;
}

static gboolean
rcw_floating_toolbar_hide(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	priv->hidetb_timer = 0;
	rcw_floating_toolbar_show(cnnwin, FALSE);
	return FALSE;
}

static gboolean rcw_floating_toolbar_on_scroll(GtkWidget *widget, GdkEventScroll *event,
					       RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	int opacity;

	cnnobj = rcw_get_visible_cnnobj(cnnwin);
	if (!cnnobj)
		return TRUE;

	opacity = remmina_file_get_int(cnnobj->remmina_file, "toolbar_opacity", 0);
	switch (event->direction) {
	case GDK_SCROLL_UP:
		if (opacity > 0) {
			remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity - 1);
			rcw_update_toolbar_opacity(cnnwin);
			return TRUE;
		}
		break;
	case GDK_SCROLL_DOWN:
		if (opacity < TOOLBAR_OPACITY_LEVEL) {
			remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity + 1);
			rcw_update_toolbar_opacity(cnnwin);
			return TRUE;
		}
		break;
#ifdef GDK_SCROLL_SMOOTH
	case GDK_SCROLL_SMOOTH:
		if (event->delta_y < 0 && opacity > 0) {
			remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity - 1);
			rcw_update_toolbar_opacity(cnnwin);
			return TRUE;
		}
		if (event->delta_y > 0 && opacity < TOOLBAR_OPACITY_LEVEL) {
			remmina_file_set_int(cnnobj->remmina_file, "toolbar_opacity", opacity + 1);
			rcw_update_toolbar_opacity(cnnwin);
			return TRUE;
		}
		break;
#endif
	default:
		break;
	}
	return TRUE;
}

static gboolean rcw_after_configure_scrolled(gpointer user_data)
{
	TRACE_CALL(__func__);
	gint width, height;
	GdkWindowState s;
	gint ipg, npages;
	RemminaConnectionObject *cnnobj;

	cnnobj = (RemminaConnectionObject *)user_data;

	if (!cnnobj || !cnnobj->cnnwin)
		return FALSE;

	s = gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(cnnobj->cnnwin)));


	/* Changed window_maximize, window_width and window_height for all
	 * connections inside the notebook */
	npages = gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnobj->cnnwin->priv->notebook));
	for (ipg = 0; ipg < npages; ipg++) {
		cnnobj = g_object_get_data(
			G_OBJECT(gtk_notebook_get_nth_page(GTK_NOTEBOOK(cnnobj->cnnwin->priv->notebook), ipg)),
			"cnnobj");
		if (s & GDK_WINDOW_STATE_MAXIMIZED) {
			remmina_file_set_int(cnnobj->remmina_file, "window_maximize", TRUE);
		} else {
			gtk_window_get_size(GTK_WINDOW(cnnobj->cnnwin), &width, &height);
			remmina_file_set_int(cnnobj->remmina_file, "window_width", width);
			remmina_file_set_int(cnnobj->remmina_file, "window_height", height);
			remmina_file_set_int(cnnobj->remmina_file, "window_maximize", FALSE);
		}
	}
	cnnobj->cnnwin->priv->savestate_eventsourceid = 0;
	return FALSE;
}

static gboolean rcw_on_configure(GtkWidget *widget, GdkEventConfigure *event,
				 gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *cnnwin;
	RemminaConnectionObject *cnnobj;

	if (!REMMINA_IS_CONNECTION_WINDOW(widget))
		return FALSE;

	cnnwin = (RemminaConnectionWindow *)widget;

	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return FALSE;

	if (cnnwin->priv->savestate_eventsourceid) {
		g_source_remove(cnnwin->priv->savestate_eventsourceid);
		cnnwin->priv->savestate_eventsourceid = 0;
	}

	if (cnnwin && gtk_widget_get_window(GTK_WIDGET(cnnwin))
	    && cnnwin->priv->view_mode == SCROLLED_WINDOW_MODE)
		/* Under Gnome shell we receive this configure_event BEFORE a window
		 * is really unmaximized, so we must read its new state and dimensions
		 * later, not now */
		cnnwin->priv->savestate_eventsourceid = g_timeout_add(500, rcw_after_configure_scrolled, cnnobj);

	if (cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
		/* Notify window of change so that scroll border can be hidden or shown if needed */
		rco_check_resize(cnnobj);
	return FALSE;
}

static void rcw_update_pin(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	if (cnnwin->priv->pin_down)
		gtk_button_set_image(GTK_BUTTON(cnnwin->priv->pin_button),
				     gtk_image_new_from_icon_name("remmina-pin-down-symbolic", GTK_ICON_SIZE_MENU));
	else
		gtk_button_set_image(GTK_BUTTON(cnnwin->priv->pin_button),
				     gtk_image_new_from_icon_name("remmina-pin-up-symbolic", GTK_ICON_SIZE_MENU));
}

static void rcw_toolbar_pin(GtkWidget *widget, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	remmina_pref.toolbar_pin_down = cnnwin->priv->pin_down = !cnnwin->priv->pin_down;
	remmina_pref_save();
	rcw_update_pin(cnnwin);
}

static void rcw_create_floating_toolbar(RemminaConnectionWindow *cnnwin, gint mode)
{
	TRACE_CALL(__func__);

	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	GtkWidget *ftb_widget;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *pinbutton;
	GtkWidget *tb;


	/* A widget to be used for GtkOverlay for GTK >= 3.10 */
	ftb_widget = gtk_event_box_new();

	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_show(vbox);

	gtk_container_add(GTK_CONTAINER(ftb_widget), vbox);

	tb = rcw_create_toolbar(cnnwin, mode);
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
	g_signal_connect(G_OBJECT(pinbutton), "clicked", G_CALLBACK(rcw_toolbar_pin), cnnwin);
	priv->pin_button = pinbutton;
	priv->pin_down = remmina_pref.toolbar_pin_down;
	rcw_update_pin(cnnwin);


	label = gtk_label_new("");
	gtk_label_set_max_width_chars(GTK_LABEL(label), 50);
	gtk_widget_show(label);

	gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 0);

	priv->floating_toolbar_label = label;

	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM) {
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), tb, FALSE, FALSE, 0);
	} else {
		gtk_box_pack_start(GTK_BOX(vbox), tb, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	priv->floating_toolbar_widget = ftb_widget;
	gtk_widget_show(ftb_widget);
}

static void rcw_toolbar_place_signal(RemminaConnectionWindow *cnnwin, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv;

	priv = cnnwin->priv;
	/* Detach old toolbar widget and reattach in new position in the grid */
	if (priv->toolbar && priv->grid) {
		g_object_ref(priv->toolbar);
		gtk_container_remove(GTK_CONTAINER(priv->grid), priv->toolbar);
		rcw_place_toolbar(GTK_TOOLBAR(priv->toolbar), GTK_GRID(priv->grid), GTK_WIDGET(priv->notebook), remmina_pref.toolbar_placement);
		g_object_unref(priv->toolbar);
	}
}


static void rcw_init(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv;

	priv = g_new0(RemminaConnectionWindowPriv, 1);
	cnnwin->priv = priv;

	priv->view_mode = SCROLLED_WINDOW_MODE;
	if (kioskmode && kioskmode == TRUE)
		priv->view_mode = VIEWPORT_FULLSCREEN_MODE;

	priv->floating_toolbar_opacity = 1.0;
	priv->kbcaptured = FALSE;
	priv->mouse_pointer_entered = FALSE;
	priv->fss_view_mode = VIEWPORT_FULLSCREEN_MODE;
	priv->ss_width = 640;
	priv->ss_height = 480;
	priv->ss_maximized = FALSE;

	remmina_widget_pool_register(GTK_WIDGET(cnnwin));
}

static gboolean rcw_state_event(GtkWidget *widget, GdkEventWindowState *event, gpointer user_data)
{
	TRACE_CALL(__func__);

	if (!REMMINA_IS_CONNECTION_WINDOW(widget))
		return FALSE;

	if (event->changed_mask & GDK_WINDOW_STATE_FOCUSED) {
		if (event->new_window_state & GDK_WINDOW_STATE_FOCUSED)
			rcw_focus_in((RemminaConnectionWindow *)widget);
		else
			rcw_focus_out((RemminaConnectionWindow *)widget);
	}

	return FALSE; // moved here because a function should return a value. Should be correct
}

static gboolean rcw_map_event_fullscreen(GtkWidget *widget, GdkEvent *event, gpointer data)
{
	gint target_monitor;

	TRACE_CALL(__func__);

	if (!REMMINA_IS_CONNECTION_WINDOW(widget))
		return FALSE;

	target_monitor = GPOINTER_TO_INT(data);

#if GTK_CHECK_VERSION(3, 18, 0)
	if (remmina_pref.fullscreen_on_auto) {
		if (target_monitor == FULL_SCREEN_TARGET_MONITOR_UNDEFINED)
			gtk_window_fullscreen(GTK_WINDOW(widget));
		else
			gtk_window_fullscreen_on_monitor(GTK_WINDOW(widget), gtk_window_get_screen(GTK_WINDOW(widget)),
							 target_monitor);
	} else {
		remmina_log_print("Fullscreen managed by WM or by the user, as per settings");
		gtk_window_fullscreen(GTK_WINDOW(widget));
	}
#else
	remmina_log_print("Cannot fullscreen on a specific monitor, feature available from GTK 3.18");
	gtk_window_fullscreen(GTK_WINDOW(widget));
#endif

	return FALSE;
}

static RemminaConnectionWindow *
rcw_new(gboolean fullscreen, int full_screen_target_monitor)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *cnnwin;

	cnnwin = RCW(g_object_new(REMMINA_TYPE_CONNECTION_WINDOW, NULL));
	cnnwin->priv->on_delete_confirm_mode = RCW_ONDELETE_CONFIRM_IF_2_OR_MORE;

	if (fullscreen)
		/* Put the window in fullscreen after it is mapped to have it appear on the same monitor */
		g_signal_connect(G_OBJECT(cnnwin), "map-event", G_CALLBACK(rcw_map_event_fullscreen), GINT_TO_POINTER(full_screen_target_monitor));

	gtk_container_set_border_width(GTK_CONTAINER(cnnwin), 0);
	g_signal_connect(G_OBJECT(cnnwin), "toolbar-place", G_CALLBACK(rcw_toolbar_place_signal), NULL);

	g_signal_connect(G_OBJECT(cnnwin), "delete-event", G_CALLBACK(rcw_delete_event), NULL);
	g_signal_connect(G_OBJECT(cnnwin), "destroy", G_CALLBACK(rcw_destroy), NULL);

	/* focus-in-event and focus-out-event don’t work when keyboard is grabbed
	 * via gdk_device_grab. So we listen for window-state-event to detect focus in and focus out */
	g_signal_connect(G_OBJECT(cnnwin), "window-state-event", G_CALLBACK(rcw_state_event), NULL);

	g_signal_connect(G_OBJECT(cnnwin), "focus-in-event", G_CALLBACK(rcw_focus_in_event), NULL);
	g_signal_connect(G_OBJECT(cnnwin), "focus-out-event", G_CALLBACK(rcw_focus_out_event), NULL);

	g_signal_connect(G_OBJECT(cnnwin), "enter-notify-event", G_CALLBACK(rcw_on_enter), NULL);
	g_signal_connect(G_OBJECT(cnnwin), "leave-notify-event", G_CALLBACK(rcw_on_leave), NULL);

	g_signal_connect(G_OBJECT(cnnwin), "configure_event", G_CALLBACK(rcw_on_configure), NULL);

	return cnnwin;
}

/* This function will be called for the first connection. A tag is set to the window so that
 * other connections can determine if whether a new tab should be append to the same window
 */
static void rcw_update_tag(RemminaConnectionWindow *cnnwin, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	gchar *tag;

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

void rcw_grab_focus(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	if (!(cnnobj = rcw_get_visible_cnnobj(cnnwin))) return;

	if (GTK_IS_WIDGET(cnnobj->proto))
		remmina_protocol_widget_grab_focus(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
}

static GtkWidget *nb_find_page_by_cnnobj(GtkNotebook *notebook, RemminaConnectionObject *cnnobj)
{
	gint i, np;
	GtkWidget *found_page, *pg;

	if (cnnobj == NULL || cnnobj->cnnwin == NULL || cnnobj->cnnwin->priv == NULL)
		return NULL;
	found_page = NULL;
	np = gtk_notebook_get_n_pages(cnnobj->cnnwin->priv->notebook);
	for (i = 0; i < np; i++) {
		pg = gtk_notebook_get_nth_page(cnnobj->cnnwin->priv->notebook, i);
		if (g_object_get_data(G_OBJECT(pg), "cnnobj") == cnnobj) {
			found_page = pg;
			break;
		}
	}

	return found_page;
}


void rco_closewin(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj = gp->cnnobj;
	GtkWidget *page_to_remove;

	if (cnnobj && cnnobj->cnnwin) {
		page_to_remove = nb_find_page_by_cnnobj(cnnobj->cnnwin->priv->notebook, cnnobj);
		if (page_to_remove) {
			gtk_notebook_remove_page(
				cnnobj->cnnwin->priv->notebook,
				gtk_notebook_page_num(cnnobj->cnnwin->priv->notebook, page_to_remove));
		}
	}

	cnnobj->remmina_file = NULL;
	g_free(cnnobj);

	remmina_application_condexit(REMMINA_CONDEXIT_ONDISCONNECT);
}

void rco_on_close_button_clicked(GtkButton *button, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	if (REMMINA_IS_PROTOCOL_WIDGET(cnnobj->proto)) {
		if (!remmina_protocol_widget_is_closed((RemminaProtocolWidget *)cnnobj->proto))
			remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
		else
			rco_closewin((RemminaProtocolWidget *)cnnobj->proto);
	}
}

static GtkWidget *rco_create_tab_label(RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	GtkWidget *hbox;
	GtkWidget *widget;
	GtkWidget *button;

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

	g_signal_connect(G_OBJECT(cnnobj->proto), "enter-notify-event", G_CALLBACK(rco_enter_protocol_widget), cnnobj);

	return hbox;
}

static GtkWidget *rco_create_tab_page(RemminaConnectionObject *cnnobj)
{
	GtkWidget *page;

	page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_name(page, "remmina-tab-page");


	return page;
}

static GtkWidget *rcw_append_new_page(RemminaConnectionWindow *cnnwin, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);
	GtkWidget *page, *label;
	GtkNotebook *notebook;

	notebook = cnnwin->priv->notebook;

	page = rco_create_tab_page(cnnobj);
	g_object_set_data(G_OBJECT(page), "cnnobj", cnnobj);
	label = rco_create_tab_label(cnnobj);

	cnnobj->cnnwin = cnnwin;

	gtk_notebook_append_page(notebook, page, label);
	gtk_notebook_set_tab_reorderable(notebook, page, TRUE);
	gtk_notebook_set_tab_detachable(notebook, page, TRUE);
	/* This trick prevents the tab label from being focused */
	gtk_widget_set_can_focus(gtk_widget_get_parent(label), FALSE);

	if (gtk_widget_get_parent(cnnobj->scrolled_container) != NULL)
		printf("REMMINA WARNING in %s: scrolled_container already has a parent\n", __func__);
	gtk_box_pack_start(GTK_BOX(page), cnnobj->scrolled_container, TRUE, TRUE, 0);

	gtk_widget_show(page);

	return page;
}


static void rcw_update_notebook(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	GtkNotebook *notebook;
	gint n;

	notebook = GTK_NOTEBOOK(cnnwin->priv->notebook);

	switch (cnnwin->priv->view_mode) {
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

static gboolean rcw_on_switch_page_finalsel(gpointer user_data)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv;
	RemminaConnectionObject *cnnobj;

	if (!user_data)
		return FALSE;

	cnnobj = (RemminaConnectionObject *)user_data;
	if (!cnnobj->cnnwin)
		return FALSE;



	priv = cnnobj->cnnwin->priv;

	if (GTK_IS_WIDGET(cnnobj->cnnwin)) {
		rcw_floating_toolbar_show(cnnobj->cnnwin, TRUE);
		if (!priv->hidetb_timer)
			priv->hidetb_timer = g_timeout_add(TB_HIDE_TIME_TIME, (GSourceFunc)
							   rcw_floating_toolbar_hide, cnnobj->cnnwin);
		rco_update_toolbar(cnnobj);
		rcw_grab_focus(cnnobj->cnnwin);
		if (priv->view_mode != SCROLLED_WINDOW_MODE)
			rco_check_resize(cnnobj);
	}
	priv->switch_page_finalsel_handler = 0;
	return FALSE;
}

static void rcw_on_switch_page(GtkNotebook *notebook, GtkWidget *newpage, guint page_num,
			       RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindowPriv *priv = cnnwin->priv;
	RemminaConnectionObject *cnnobj_newpage;

	cnnobj_newpage = g_object_get_data(G_OBJECT(newpage), "cnnobj");
	if (priv->switch_page_finalsel_handler)
		g_source_remove(priv->switch_page_finalsel_handler);
	priv->switch_page_finalsel_handler = g_idle_add(rcw_on_switch_page_finalsel, cnnobj_newpage);
}

static void rcw_on_page_added(GtkNotebook *notebook, GtkWidget *child, guint page_num,
			      RemminaConnectionWindow *cnnwin)
{
	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnwin->priv->notebook)) > 0)
		rcw_update_notebook(cnnwin);
}

static void rcw_on_page_removed(GtkNotebook *notebook, GtkWidget *child, guint page_num,
				RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);

	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnwin->priv->notebook)) <= 0)
		gtk_widget_destroy(GTK_WIDGET(cnnwin));
}

static GtkNotebook *
rcw_on_notebook_create_window(GtkNotebook *notebook, GtkWidget *page, gint x, gint y, gpointer data)
{
	/* This signal callback is called by GTK when a detachable tab is dropped on the root window
	 * or in an existing window */

	TRACE_CALL(__func__);
	RemminaConnectionWindow *srccnnwin;
	RemminaConnectionWindow *dstcnnwin;
	RemminaConnectionObject *cnnobj;
	GdkWindow *window;
	gchar *srctag;
	gint width, height;

#if GTK_CHECK_VERSION(3, 20, 0)
	GdkSeat *seat;
#else
	GdkDeviceManager *manager;
#endif
	GdkDevice *device = NULL;

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

	cnnobj = (RemminaConnectionObject *)g_object_get_data(G_OBJECT(page), "cnnobj");

	if (!dstcnnwin) {
		/* Drop is directed to a new rcw: create a new scrolled window to accomodate
		 * the dropped connectionand move our cnnobj there. Width and
		 * height of the new window are cloned from the current window */
		srctag = (gchar *)g_object_get_data(G_OBJECT(srccnnwin), "tag");
		gtk_window_get_size(GTK_WINDOW(srccnnwin), &width, &height);
		dstcnnwin = rcw_create_scrolled(width, height, FALSE);  // New dropped window is never maximized
		g_object_set_data_full(G_OBJECT(dstcnnwin), "tag", g_strdup(srctag), (GDestroyNotify)g_free);
		/* when returning, GTK will move the whole tab to the new notebook.
		 * Prepare cnnobj to be hosted in the new cnnwin */
		cnnobj->cnnwin = dstcnnwin;
	} else {
		cnnobj->cnnwin = dstcnnwin;
	}

	remmina_protocol_widget_set_hostkey_func(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
						 (RemminaHostkeyFunc)rcw_hostkey_func);

	return GTK_NOTEBOOK(cnnobj->cnnwin->priv->notebook);
}

static GtkNotebook *
rcw_create_notebook(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	GtkNotebook *notebook;

	notebook = GTK_NOTEBOOK(gtk_notebook_new());

	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	gtk_widget_show(GTK_WIDGET(notebook));

	g_signal_connect(G_OBJECT(notebook), "create-window", G_CALLBACK(rcw_on_notebook_create_window), NULL);
	g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(rcw_on_switch_page), cnnwin);
	g_signal_connect(G_OBJECT(notebook), "page-added", G_CALLBACK(rcw_on_page_added), cnnwin);
	g_signal_connect(G_OBJECT(notebook), "page-removed", G_CALLBACK(rcw_on_page_removed), cnnwin);
	gtk_widget_set_can_focus(GTK_WIDGET(notebook), FALSE);

	return notebook;
}

/* Create a scrolled toplevel window */
static RemminaConnectionWindow *rcw_create_scrolled(gint width, gint height, gboolean maximize)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *cnnwin;
	GtkWidget *grid;
	GtkWidget *toolbar;
	GtkNotebook *notebook;
	GList *chain;

	cnnwin = rcw_new(FALSE, 0);
	gtk_widget_realize(GTK_WIDGET(cnnwin));

	gtk_window_set_default_size(GTK_WINDOW(cnnwin), width, height);

	/* Create the toolbar */
	toolbar = rcw_create_toolbar(cnnwin, SCROLLED_WINDOW_MODE);

	/* Create the notebook */
	notebook = rcw_create_notebook(cnnwin);

	/* Create the grid container for toolbars+notebook and populate it */
	grid = gtk_grid_new();
	gtk_grid_attach(GTK_GRID(grid), GTK_WIDGET(notebook), 0, 0, 1, 1);

	gtk_widget_set_hexpand(GTK_WIDGET(notebook), TRUE);
	gtk_widget_set_vexpand(GTK_WIDGET(notebook), TRUE);

	rcw_place_toolbar(GTK_TOOLBAR(toolbar), GTK_GRID(grid), GTK_WIDGET(notebook), remmina_pref.toolbar_placement);

	gtk_container_add(GTK_CONTAINER(cnnwin), grid);

	chain = g_list_append(NULL, notebook);
	gtk_container_set_focus_chain(GTK_CONTAINER(grid), chain);
	g_list_free(chain);

	/* Add drag capabilities to the toolbar */
	gtk_drag_source_set(GTK_WIDGET(toolbar), GDK_BUTTON1_MASK,
			    dnd_targets_tb, sizeof dnd_targets_tb / sizeof *dnd_targets_tb, GDK_ACTION_MOVE);
	g_signal_connect_after(GTK_WIDGET(toolbar), "drag-begin", G_CALLBACK(rcw_tb_drag_begin), NULL);
	g_signal_connect(GTK_WIDGET(toolbar), "drag-failed", G_CALLBACK(rcw_tb_drag_failed), cnnwin);

	/* Add drop capabilities to the drop/dest target for the toolbar (the notebook) */
	gtk_drag_dest_set(GTK_WIDGET(notebook), GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
			  dnd_targets_tb, sizeof dnd_targets_tb / sizeof *dnd_targets_tb, GDK_ACTION_MOVE);
	gtk_drag_dest_set_track_motion(GTK_WIDGET(notebook), TRUE);
	g_signal_connect(GTK_WIDGET(notebook), "drag-drop", G_CALLBACK(rcw_tb_drag_drop), cnnwin);

	cnnwin->priv->view_mode = SCROLLED_WINDOW_MODE;
	cnnwin->priv->toolbar = toolbar;
	cnnwin->priv->grid = grid;
	cnnwin->priv->notebook = notebook;
	cnnwin->priv->ss_width = width;
	cnnwin->priv->ss_height = height;
	cnnwin->priv->ss_maximized = maximize;

	/* The notebook and all its child must be realized now, or a reparent will
	 * call unrealize() and will destroy a GtkSocket */
	gtk_widget_show(grid);
	gtk_widget_show(GTK_WIDGET(cnnwin));
	GtkWindowGroup * wingrp = gtk_window_group_new ();
	gtk_window_group_add_window (wingrp, GTK_WINDOW(cnnwin));
	gtk_window_set_transient_for(GTK_WINDOW(cnnwin), NULL);

	if (maximize)
		gtk_window_maximize(GTK_WINDOW(cnnwin));


	rcw_set_toolbar_visibility(cnnwin);

	return cnnwin;
}

static void rcw_create_overlay_ftb_overlay(RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);

	GtkWidget *revealer;
	RemminaConnectionWindowPriv *priv;
	priv = cnnwin->priv;

	if (priv->overlay_ftb_overlay != NULL) {
		gtk_widget_destroy(priv->overlay_ftb_overlay);
		priv->overlay_ftb_overlay = NULL;
		priv->revealer = NULL;
	}

	rcw_create_floating_toolbar(cnnwin, priv->fss_view_mode);

	priv->overlay_ftb_overlay = gtk_event_box_new();

	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 0);

	GtkWidget *handle = gtk_drawing_area_new();
	gtk_widget_set_size_request(handle, 4, 4);
	gtk_widget_set_name(handle, "ftb-handle");

	revealer = gtk_revealer_new();

	gtk_widget_set_halign(GTK_WIDGET(priv->overlay_ftb_overlay), GTK_ALIGN_CENTER);

	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM) {
		gtk_box_pack_start(GTK_BOX(vbox), handle, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), revealer, FALSE, FALSE, 0);
		gtk_revealer_set_transition_type(GTK_REVEALER(revealer), GTK_REVEALER_TRANSITION_TYPE_SLIDE_UP);
		gtk_widget_set_valign(GTK_WIDGET(priv->overlay_ftb_overlay), GTK_ALIGN_END);
	} else {
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
	gtk_container_add(GTK_CONTAINER(priv->overlay_ftb_overlay), fr);
	gtk_container_add(GTK_CONTAINER(fr), vbox);

	gtk_widget_show(vbox);
	gtk_widget_show(revealer);
	gtk_widget_show(handle);
	gtk_widget_show(priv->overlay_ftb_overlay);
	gtk_widget_show(fr);

	if (remmina_pref.floating_toolbar_placement == FLOATING_TOOLBAR_PLACEMENT_BOTTOM)
		gtk_widget_set_name(fr, "ftbbox-lower");
	else
		gtk_widget_set_name(fr, "ftbbox-upper");

	gtk_overlay_add_overlay(GTK_OVERLAY(priv->overlay), priv->overlay_ftb_overlay);

	rcw_floating_toolbar_show(cnnwin, TRUE);

	g_signal_connect(G_OBJECT(priv->overlay_ftb_overlay), "enter-notify-event", G_CALLBACK(rcw_floating_toolbar_on_enter), cnnwin);
	g_signal_connect(G_OBJECT(priv->overlay_ftb_overlay), "scroll-event", G_CALLBACK(rcw_floating_toolbar_on_scroll), cnnwin);
	gtk_widget_add_events(GTK_WIDGET(priv->overlay_ftb_overlay), GDK_SCROLL_MASK);

	/* Add drag and drop capabilities to the source */
	gtk_drag_source_set(GTK_WIDGET(priv->overlay_ftb_overlay), GDK_BUTTON1_MASK,
			    dnd_targets_ftb, sizeof dnd_targets_ftb / sizeof *dnd_targets_ftb, GDK_ACTION_MOVE);
	g_signal_connect_after(GTK_WIDGET(priv->overlay_ftb_overlay), "drag-begin", G_CALLBACK(rcw_ftb_drag_begin), cnnwin);
}


static gboolean rcw_ftb_drag_drop(GtkWidget *widget, GdkDragContext *context,
				  gint x, gint y, guint time, RemminaConnectionWindow *cnnwin)
{
	TRACE_CALL(__func__);
	GtkAllocation wa;
	gint new_floating_toolbar_placement;
	RemminaConnectionObject *cnnobj;

	gtk_widget_get_allocation(widget, &wa);

	if (y >= wa.height / 2)
		new_floating_toolbar_placement = FLOATING_TOOLBAR_PLACEMENT_BOTTOM;
	else
		new_floating_toolbar_placement = FLOATING_TOOLBAR_PLACEMENT_TOP;

	gtk_drag_finish(context, TRUE, TRUE, time);

	if (new_floating_toolbar_placement != remmina_pref.floating_toolbar_placement) {
		/* Destroy and recreate the FTB */
		remmina_pref.floating_toolbar_placement = new_floating_toolbar_placement;
		remmina_pref_save();
		rcw_create_overlay_ftb_overlay(cnnwin);
		cnnobj = rcw_get_visible_cnnobj(cnnwin);
		if (cnnobj) rco_update_toolbar(cnnobj);
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
	cairo_set_dash(cr, dashes, 1, 0);
	cairo_rectangle(cr, 0, 0, wa.width, wa.height);
	cairo_stroke(cr);
	cairo_destroy(cr);

	gtk_drag_set_icon_surface(context, surface);
}

RemminaConnectionWindow *rcw_create_fullscreen(GtkWindow *old, gint view_mode)
{
	TRACE_CALL(__func__);
	RemminaConnectionWindow *cnnwin;
	GtkNotebook *notebook;
#if GTK_CHECK_VERSION(3, 22, 0)
	gint n_monitors;
	gint i;
	GdkMonitor *old_monitor;
	GdkDisplay *old_display;
	GdkWindow *old_window;
#endif
	gint full_screen_target_monitor;

	full_screen_target_monitor = FULL_SCREEN_TARGET_MONITOR_UNDEFINED;
	if (old) {
#if GTK_CHECK_VERSION(3, 22, 0)
		old_window = gtk_widget_get_window(GTK_WIDGET(old));
		old_display = gdk_window_get_display(old_window);
		old_monitor = gdk_display_get_monitor_at_window(old_display, old_window);
		n_monitors = gdk_display_get_n_monitors(old_display);
		for (i = 0; i < n_monitors; ++i) {
			if (gdk_display_get_monitor(old_display, i) == old_monitor) {
				full_screen_target_monitor = i;
				break;
			}
		}
#else
		full_screen_target_monitor = gdk_screen_get_monitor_at_window(
			gdk_screen_get_default(),
			gtk_widget_get_window(GTK_WIDGET(old)));
#endif
	}

	cnnwin = rcw_new(TRUE, full_screen_target_monitor);
	gtk_widget_set_name(GTK_WIDGET(cnnwin), "remmina-connection-window-fullscreen");
	gtk_widget_realize(GTK_WIDGET(cnnwin));

	if (!view_mode)
		view_mode = VIEWPORT_FULLSCREEN_MODE;

	notebook = rcw_create_notebook(cnnwin);

	cnnwin->priv->overlay = gtk_overlay_new();
	gtk_container_add(GTK_CONTAINER(cnnwin), cnnwin->priv->overlay);
	gtk_container_add(GTK_CONTAINER(cnnwin->priv->overlay), GTK_WIDGET(notebook));
	gtk_widget_show(GTK_WIDGET(cnnwin->priv->overlay));

	cnnwin->priv->notebook = notebook;
	cnnwin->priv->view_mode = view_mode;
	cnnwin->priv->fss_view_mode = view_mode;

	/* Create the floating toolbar */
	if (remmina_pref.fullscreen_toolbar_visibility != FLOATING_TOOLBAR_VISIBILITY_DISABLE) {
		rcw_create_overlay_ftb_overlay(cnnwin);
		/* Add drag and drop capabilities to the drop/dest target for floating toolbar */
		gtk_drag_dest_set(GTK_WIDGET(cnnwin->priv->overlay), GTK_DEST_DEFAULT_MOTION | GTK_DEST_DEFAULT_HIGHLIGHT,
				  dnd_targets_ftb, sizeof dnd_targets_ftb / sizeof *dnd_targets_ftb, GDK_ACTION_MOVE);
		gtk_drag_dest_set_track_motion(GTK_WIDGET(cnnwin->priv->notebook), TRUE);
		g_signal_connect(GTK_WIDGET(cnnwin->priv->overlay), "drag-drop", G_CALLBACK(rcw_ftb_drag_drop), cnnwin);
	}

	gtk_widget_show(GTK_WIDGET(cnnwin));
	GtkWindowGroup * wingrp = gtk_window_group_new ();
	gtk_window_group_add_window (wingrp, GTK_WINDOW(cnnwin));
	gtk_window_set_transient_for(GTK_WINDOW(cnnwin), NULL);

	return cnnwin;
}

static gboolean rcw_hostkey_func(RemminaProtocolWidget *gp, guint keyval, gboolean release)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj = gp->cnnobj;
	RemminaConnectionWindowPriv *priv = cnnobj->cnnwin->priv;
	const RemminaProtocolFeature *feature;
	gint i;

	if (release) {
		if (remmina_pref.hostkey && keyval == remmina_pref.hostkey) {
			priv->hostkey_activated = FALSE;
			if (priv->hostkey_used)
				/* hostkey pressed + something else */
				return TRUE;
		}
		/* If hostkey is released without pressing other keys, we should execute the
		 * shortcut key which is the same as hostkey. Be default, this is grab/ungrab
		 * keyboard */
		else if (priv->hostkey_activated) {
			/* Trap all key releases when hostkey is pressed */
			/* hostkey pressed + something else */
			return TRUE;
		} else {
			/* Any key pressed, no hostkey */
			return FALSE;
		}
	} else if (remmina_pref.hostkey && keyval == remmina_pref.hostkey) {
		/** @todo Add callback for hostname transparent overlay #832 */
		priv->hostkey_activated = TRUE;
		priv->hostkey_used = FALSE;
		return TRUE;
	} else if (!priv->hostkey_activated) {
		/* Any key pressed, no hostkey */
		return FALSE;
	}

	priv->hostkey_used = TRUE;
	keyval = gdk_keyval_to_lower(keyval);
	if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Down
	    || keyval == GDK_KEY_Left || keyval == GDK_KEY_Right) {
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
		} else if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container)) {
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
			} else {
				sz = gdk_window_get_width(gsvwin) + 2; // Add 2px of black scroll border
				adj = gtk_scrollable_get_hadjustment(GTK_SCROLLABLE(child));
			}

			if (keyval == GDK_KEY_Up || keyval == GDK_KEY_Left)
				value = 0;
			else
				value = gtk_adjustment_get_upper(GTK_ADJUSTMENT(adj)) - (gdouble)sz + 2.0;

			gtk_adjustment_set_value(GTK_ADJUSTMENT(adj), value);
		}
	}

	if (keyval == remmina_pref.shortcutkey_fullscreen) {
		switch (priv->view_mode) {
		case SCROLLED_WINDOW_MODE:
			rcw_switch_viewmode(cnnobj->cnnwin, priv->fss_view_mode);
			break;
		case SCROLLED_FULLSCREEN_MODE:
		case VIEWPORT_FULLSCREEN_MODE:
			rcw_switch_viewmode(cnnobj->cnnwin, SCROLLED_WINDOW_MODE);
			break;
		default:
			break;
		}
	} else if (keyval == remmina_pref.shortcutkey_autofit) {
		if (priv->toolitem_autofit && gtk_widget_is_sensitive(GTK_WIDGET(priv->toolitem_autofit)))
			rcw_toolbar_autofit(GTK_WIDGET(gp), cnnobj->cnnwin);
	} else if (keyval == remmina_pref.shortcutkey_nexttab) {
		i = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)) + 1;
		if (i >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)))
			i = 0;
		gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), i);
	} else if (keyval == remmina_pref.shortcutkey_prevtab) {
		i = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)) - 1;
		if (i < 0)
			i = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)) - 1;
		gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), i);
	} else if (keyval == remmina_pref.shortcutkey_scale) {
		if (gtk_widget_is_sensitive(GTK_WIDGET(priv->toolitem_scale))) {
			gtk_toggle_tool_button_set_active(
				GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_scale),
				!gtk_toggle_tool_button_get_active(
					GTK_TOGGLE_TOOL_BUTTON(
						priv->toolitem_scale)));
		}
	} else if (keyval == remmina_pref.shortcutkey_grab) {
		gtk_toggle_tool_button_set_active(
			GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_grab),
			!gtk_toggle_tool_button_get_active(
				GTK_TOGGLE_TOOL_BUTTON(
					priv->toolitem_grab)));
	} else if (keyval == remmina_pref.shortcutkey_minimize) {
		rcw_toolbar_minimize(GTK_WIDGET(gp),
				     cnnobj->cnnwin);
	} else if (keyval == remmina_pref.shortcutkey_viewonly) {
		remmina_file_set_int(cnnobj->remmina_file, "viewonly",
				     (remmina_file_get_int(cnnobj->remmina_file, "viewonly", 0)
				      == 0) ? 1 : 0);
	} else if (keyval == remmina_pref.shortcutkey_screenshot) {
		rcw_toolbar_screenshot(GTK_WIDGET(gp),
				       cnnobj->cnnwin);
	} else if (keyval == remmina_pref.shortcutkey_disconnect) {
		rco_disconnect_current_page(cnnobj);
	} else if (keyval == remmina_pref.shortcutkey_toolbar) {
		if (priv->view_mode == SCROLLED_WINDOW_MODE) {
			remmina_pref.hide_connection_toolbar =
				!remmina_pref.hide_connection_toolbar;
			rcw_set_toolbar_visibility(cnnobj->cnnwin);
		}
	} else {
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
	priv->hostkey_activated = FALSE;
	/* Trap all key presses when hostkey is pressed */
	return TRUE;
}

static RemminaConnectionWindow *rcw_find(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	const gchar *tag;

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
	RemminaConnectionObject *cnnobj = (RemminaConnectionObject *)user_data;

	if (cnnobj && cnnobj->connected && cnnobj->cnnwin) {
		gtk_window_present_with_time(GTK_WINDOW(cnnobj->cnnwin), (guint32)(g_get_monotonic_time() / 1000));
		rcw_grab_focus(cnnobj->cnnwin);
	}
	return FALSE;
}

void rco_on_connect(RemminaProtocolWidget *gp, RemminaConnectionObject *cnnobj)
{
	TRACE_CALL(__func__);

	gchar *last_success;

	g_debug("Connect signal emitted");
	GDateTime *date = g_date_time_new_now_utc();

	/* This signal handler is called by a plugin when it’s correctly connected
	 * (and authenticated) */

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
	if (remmina_file_get_filename(cnnobj->remmina_file) == NULL)
		remmina_pref_add_recent(remmina_file_get_string(cnnobj->remmina_file, "protocol"),
					remmina_file_get_string(cnnobj->remmina_file, "server"));
	if (remmina_pref.periodic_usage_stats_permitted) {
		g_debug("Stats are allowed, we save the last successful connection date");
		remmina_file_set_string(cnnobj->remmina_file, "last_success", last_success);
		g_debug("Last connection was made on %s.", last_success);
	}

	g_debug("Saving credentials");
	/* Save credentials */
	remmina_file_save(cnnobj->remmina_file);

	if (cnnobj->cnnwin->priv->floating_toolbar_widget)
		gtk_widget_show(cnnobj->cnnwin->priv->floating_toolbar_widget);

	rco_update_toolbar(cnnobj);

	g_debug("Trying to present the window");
	/* Try to present window */
	g_timeout_add(200, rco_delayed_window_present, (gpointer)cnnobj);
}

static void cb_lasterror_confirmed(void *cbdata, int btn)
{
	TRACE_CALL(__func__);
	rco_closewin((RemminaProtocolWidget *)cbdata);
}

void rco_on_disconnect(RemminaProtocolWidget *gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj = gp->cnnobj;
	RemminaConnectionWindowPriv *priv = cnnobj->cnnwin->priv;
	GtkWidget *pparent;

	g_debug("Disconnect signal received on RemminaProtocolWidget");
	/* Detach the protocol widget from the notebook now, or we risk that a
	 * window delete will destroy cnnobj->proto before we complete disconnection.
	 */
	pparent = gtk_widget_get_parent(cnnobj->proto);
	if (pparent != NULL) {
		g_object_ref(cnnobj->proto);
		gtk_container_remove(GTK_CONTAINER(pparent), cnnobj->proto);
	}

	cnnobj->connected = FALSE;

	if (remmina_pref.save_view_mode) {
		if (cnnobj->cnnwin)
			remmina_file_set_int(cnnobj->remmina_file, "viewmode", cnnobj->cnnwin->priv->view_mode);
		remmina_file_save(cnnobj->remmina_file);
	}

	rcw_keyboard_ungrab(cnnobj->cnnwin);
	gtk_toggle_tool_button_set_active(
		GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_grab),
		FALSE);

	if (remmina_protocol_widget_has_error(gp)) {
		/* We cannot close window immediately, but we must show a message panel */
		RemminaMessagePanel *mp;
		/* Destroy scrolled_contaner (and viewport) and all its children the plugin created
		 * on it, so they will not receive GUI signals */
		if (cnnobj->scrolled_container) {
			gtk_widget_destroy(cnnobj->scrolled_container);
			cnnobj->scrolled_container = NULL;
		}
		cnnobj->viewport = NULL;
		mp = remmina_message_panel_new();
		remmina_message_panel_setup_message(mp, remmina_protocol_widget_get_error_message(gp), cb_lasterror_confirmed, gp);
		rco_show_message_panel(gp->cnnobj, mp);
		g_debug("Could not disconnect");
	} else {
		rco_closewin(gp);
		g_debug("Disconnected");
	}
}

void rco_on_desktop_resize(RemminaProtocolWidget *gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj = gp->cnnobj;
	if (cnnobj && cnnobj->cnnwin && cnnobj->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
		rco_check_resize(cnnobj);
}

void rco_on_update_align(RemminaProtocolWidget *gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj = gp->cnnobj;
	remmina_protocol_widget_update_alignment(cnnobj);
}

void rco_on_unlock_dynres(RemminaProtocolWidget *gp, gpointer data)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj = gp->cnnobj;
	cnnobj->dynres_unlocked = TRUE;
	rco_update_toolbar(cnnobj);
}

gboolean rcw_open_from_filename(const gchar *filename)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile;
	GtkWidget *dialog;

	remminafile = remmina_file_manager_load_file(filename);
	if (remminafile) {
		rcw_open_from_file(remminafile);
		return TRUE;
	} else {
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						_("The file \"%s\" is corrupted, unreadable, or could not be found."), filename);
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
	rco_check_resize(gp->cnnobj);

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

void rcw_open_from_file(RemminaFile *remminafile)
{
	TRACE_CALL(__func__);
	rcw_open_from_file_full(remminafile, NULL, NULL, NULL);
}

GtkWidget *rcw_open_from_file_full(RemminaFile *remminafile, GCallback disconnect_cb, gpointer data, guint *handler)
{
	TRACE_CALL(__func__);
	RemminaConnectionObject *cnnobj;

	gint ret;
	GtkWidget *dialog;
	GtkWidget *newpage;
	gint width, height;
	gboolean maximize;
	gint view_mode;
	const gchar *msg;

	if (disconnect_cb) {
		g_print("disconnect_cb is deprecated inside rcw_open_from_file_full() and should be null");
		return NULL;
	}


	/* Create the RemminaConnectionObject */
	cnnobj = g_new0(RemminaConnectionObject, 1);
	cnnobj->remmina_file = remminafile;

	/* Create the RemminaProtocolWidget */
	cnnobj->proto = remmina_protocol_widget_new();
	remmina_protocol_widget_setup((RemminaProtocolWidget *)cnnobj->proto, remminafile, cnnobj);
	if (remmina_protocol_widget_has_error((RemminaProtocolWidget *)cnnobj->proto)) {
		GtkWindow *wparent;
		wparent = remmina_main_get_window();
		msg = remmina_protocol_widget_get_error_message((RemminaProtocolWidget *)cnnobj->proto);
		dialog = gtk_message_dialog_new(wparent, GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, "%s", msg);
		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		/* We should destroy cnnobj->proto and cnnobj now… ToDo: fix this leak */
		return NULL;
	}

	/* Set a name for the widget, for CSS selector */
	gtk_widget_set_name(GTK_WIDGET(cnnobj->proto), "remmina-protocol-widget");

	gtk_widget_set_halign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);
	gtk_widget_set_valign(GTK_WIDGET(cnnobj->proto), GTK_ALIGN_FILL);

	if (data)
		g_object_set_data(G_OBJECT(cnnobj->proto), "user-data", data);

	view_mode = remmina_file_get_int(cnnobj->remmina_file, "viewmode", 0);
	if (kioskmode)
		view_mode = VIEWPORT_FULLSCREEN_MODE;

	/* Create the viewport to make the RemminaProtocolWidget scrollable */
	cnnobj->viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_set_name(cnnobj->viewport, "remmina-cw-viewport");
	gtk_widget_show(cnnobj->viewport);
	gtk_container_set_border_width(GTK_CONTAINER(cnnobj->viewport), 0);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(cnnobj->viewport), GTK_SHADOW_NONE);

	/* Create the scrolled container */
	cnnobj->scrolled_container = rco_create_scrolled_container(cnnobj, view_mode);

	gtk_container_add(GTK_CONTAINER(cnnobj->scrolled_container), cnnobj->viewport);

	/* Determine whether the plugin can scale or not. If the plugin can scale and we do
	 * not want to expand, then we add a GtkAspectFrame to maintain aspect ratio during scaling */
	cnnobj->plugin_can_scale = remmina_plugin_manager_query_feature_by_type(REMMINA_PLUGIN_TYPE_PROTOCOL,
										remmina_file_get_string(remminafile, "protocol"),
										REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);

	cnnobj->aspectframe = NULL;
	gtk_container_add(GTK_CONTAINER(cnnobj->viewport), cnnobj->proto);

	/* Determine whether this connection will be put on a new window
	 * or in an existing one */
	cnnobj->cnnwin = rcw_find(remminafile);
	if (!cnnobj->cnnwin) {
		/* Connection goes on a new toplevel window */
		switch (view_mode) {
		case SCROLLED_FULLSCREEN_MODE:
		case VIEWPORT_FULLSCREEN_MODE:
			cnnobj->cnnwin = rcw_create_fullscreen(NULL, view_mode);
			break;
		case SCROLLED_WINDOW_MODE:
		default:
			width = remmina_file_get_int(cnnobj->remmina_file, "window_width", 640);
			height = remmina_file_get_int(cnnobj->remmina_file, "window_height", 480);
			maximize = remmina_file_get_int(cnnobj->remmina_file, "window_maximize", FALSE) ? TRUE : FALSE;
			cnnobj->cnnwin = rcw_create_scrolled(width, height, maximize);
			break;
		}
		rcw_update_tag(cnnobj->cnnwin, cnnobj);
		rcw_append_new_page(cnnobj->cnnwin, cnnobj);
	} else {
		newpage = rcw_append_new_page(cnnobj->cnnwin, cnnobj);
		gtk_window_present(GTK_WINDOW(cnnobj->cnnwin));
		nb_set_current_page(cnnobj->cnnwin->priv->notebook, newpage);
	}

	// Do not call remmina_protocol_widget_update_alignment(cnnobj); here or cnnobj->proto will not fill its parent size
	// and remmina_protocol_widget_update_remote_resolution() cannot autodetect available space

	gtk_widget_show(cnnobj->proto);
	g_signal_connect(G_OBJECT(cnnobj->proto), "connect", G_CALLBACK(rco_on_connect), cnnobj);
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
			GTK_WINDOW(cnnobj->cnnwin),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_WARNING,
			GTK_BUTTONS_OK,
			_("Warning: This plugin requires GtkSocket, but it’s not available."));
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(dialog);
		return NULL;    /* Should we destroy something before returning? */
	}

	if (cnnobj->cnnwin->priv->floating_toolbar_widget)
		gtk_widget_show(cnnobj->cnnwin->priv->floating_toolbar_widget);

	if (remmina_protocol_widget_has_error((RemminaProtocolWidget *)cnnobj->proto)) {
		printf("OK, an error occurred in initializing the protocol plugin before connecting. The error is %s.\n"
		       "ToDo: Put this string as error to show", remmina_protocol_widget_get_error_message((RemminaProtocolWidget *)cnnobj->proto));
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

void rcw_set_delete_confirm_mode(RemminaConnectionWindow *cnnwin, RemminaConnectionWindowOnDeleteConfirmMode mode)
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
	GtkWidget *page;

	page = nb_find_page_by_cnnobj(cnnobj->cnnwin->priv->notebook, cnnobj);
	childs = gtk_container_get_children(GTK_CONTAINER(page));
	cc = g_list_first(childs);
	while (cc != NULL) {
		if ((RemminaMessagePanel *)cc->data == mp)
			break;
		cc = g_list_next(cc);
	}
	g_list_free(childs);

	if (cc == NULL) {
		printf("Remmina: Warning, request to destroy a RemminaMessagePanel, which is not on the page\n");
		return;
	}
	was_visible = gtk_widget_is_visible(GTK_WIDGET(mp));
	gtk_widget_destroy(GTK_WIDGET(mp));

	/* And now, show the last remaining message panel, if needed */
	if (was_visible) {
		childs = gtk_container_get_children(GTK_CONTAINER(page));
		cc = g_list_first(childs);
		lastPanel = NULL;
		while (cc != NULL) {
			if (G_TYPE_CHECK_INSTANCE_TYPE(cc->data, REMMINA_TYPE_MESSAGE_PANEL))
				lastPanel = (RemminaMessagePanel *)cc->data;
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
	GtkWidget *page;

	/* Hides all RemminaMessagePanels childs of cnnobj->page */
	page = nb_find_page_by_cnnobj(cnnobj->cnnwin->priv->notebook, cnnobj);
	childs = gtk_container_get_children(GTK_CONTAINER(page));
	cc = g_list_first(childs);
	while (cc != NULL) {
		if (G_TYPE_CHECK_INSTANCE_TYPE(cc->data, REMMINA_TYPE_MESSAGE_PANEL))
			gtk_widget_hide(GTK_WIDGET(cc->data));
		cc = g_list_next(cc);
	}
	g_list_free(childs);

	/* Add the new message panel at the top of cnnobj->page */
	gtk_box_pack_start(GTK_BOX(page), GTK_WIDGET(mp), FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(page), GTK_WIDGET(mp), 0);

	/* Show the message panel */
	gtk_widget_show_all(GTK_WIDGET(mp));

	/* Focus the correct field of the RemminaMessagePanel */
	remmina_message_panel_focus_auth_entry(mp);
}
