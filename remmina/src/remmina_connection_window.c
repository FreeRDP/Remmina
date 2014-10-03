/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "config.h"
#include "remmina_public.h"
#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_init_dialog.h"
#include "remmina_protocol_widget.h"
#include "remmina_pref.h"
#include "remmina_scrolled_viewport.h"
#include "remmina_scaler.h"
#include "remmina_widget_pool.h"
#include "remmina_connection_window.h"

G_DEFINE_TYPE( RemminaConnectionWindow, remmina_connection_window, GTK_TYPE_WINDOW)

#define MOTION_TIME 100

/* One process can only have one option popup at a time */
static GtkWidget* scale_option_window = NULL;

typedef struct _RemminaConnectionHolder RemminaConnectionHolder;

struct _RemminaConnectionWindowPriv
{
	RemminaConnectionHolder* cnnhld;

	GtkWidget* notebook;

	guint switch_page_handler;

	GtkWidget* floating_toolbar;
	GtkWidget* floating_toolbar_label;
	gdouble floating_toolbar_opacity;
	/* To avoid strange event-loop */
	guint floating_toolbar_motion_handler;

	gboolean floating_toolbar_motion_show;
	gboolean floating_toolbar_motion_visible;

	GtkWidget* toolbar;

	/* Toolitems that need to be handled */
	GtkToolItem* toolitem_autofit;
	GtkToolItem* toolitem_fullscreen;
	GtkToolItem* toolitem_switch_page;
	GtkToolItem* toolitem_scale;
	GtkToolItem* toolitem_grab;
	GtkToolItem* toolitem_preferences;
	GtkToolItem* toolitem_tools;
	GtkWidget* scale_option_button;
	GtkWidget* fullscreen_option_button;

	GtkWidget* pin_button;
	gboolean pin_down;

	gboolean sticky;

	gint view_mode;
};

typedef struct _RemminaConnectionObject
{
	RemminaConnectionHolder* cnnhld;

	RemminaFile* remmina_file;

	/* A dummy window which will be realized as a container during initialize, before reparent to the real window */
	GtkWidget* window;

	/* Containers for RemminaProtocolWidget: RemminaProtocolWidget->alignment->viewport...->window */
	GtkWidget* proto;
	GtkWidget* alignment;
	GtkWidget* viewport;

	/* Scrolled containers */
	GtkWidget* scrolled_container;

	gboolean connected;
} RemminaConnectionObject;

struct _RemminaConnectionHolder
{
	RemminaConnectionWindow* cnnwin;
	gint fullscreen_view_mode;

	gboolean hostkey_activated;
	gboolean hostkey_used;
};

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

#if GTK_VERSION == 3
static void remmina_connection_window_class_init(RemminaConnectionWindowClass* klass)
{
	GtkCssProvider  *provider;

	provider = gtk_css_provider_new();
	gtk_css_provider_load_from_data (provider,
			"*.remmina-small-button\n"
			"{\n"
			"  -GtkWidget::focus-padding = 0\n"
			"  -GtkWidget::focus-line-width = 0\n"
			"  xthickness = 0\n"
			"  ythickness = 0\n"
			"}\n",
			-1, NULL);
	gtk_style_context_add_provider_for_screen (gdk_screen_get_default(),
							GTK_STYLE_PROVIDER (provider),
							GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	g_object_unref(provider);
}
#elif GTK_VERSION == 2
static void remmina_connection_window_class_init(RemminaConnectionWindowClass* klass)
{
	gtk_rc_parse_string("style \"remmina-small-button-style\"\n"
				"{\n"
				" GtkWidget::focus-padding = 0\n"
				" GtkWidget::focus-line-width = 0\n"
				" xthickness = 0\n"
				" ythickness = 0\n"
				"}\n"
				"widget \"*.remmina-small-button\" style \"remmina-small-button-style\"");
}
#endif

static void remmina_connection_holder_disconnect(RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ

	/* Notify the RemminaProtocolWidget to disconnect, but not to close the window here.
	 The window will be destroyed in RemminaProtocolWidget "disconnect" signal */
	remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
}

#if GTK_VERSION == 3
static void remmina_connection_holder_keyboard_grab(RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ
	GdkDisplay *display;
	GdkDeviceManager *manager;
	GdkDevice *keyboard = NULL;

	display = gtk_widget_get_display(GTK_WIDGET(cnnhld->cnnwin));
	manager = gdk_display_get_device_manager(display);
	keyboard = gdk_device_manager_get_client_pointer(manager);

	if (keyboard != NULL)
	{
		
		if ( gdk_device_get_source (keyboard) != GDK_SOURCE_KEYBOARD)
		{
			keyboard = gdk_device_get_associated_device( keyboard );
		}
		
		if (remmina_file_get_int(cnnobj->remmina_file, "keyboard_grab", FALSE))
		{
			gdk_device_grab(keyboard, gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)), GDK_OWNERSHIP_WINDOW, TRUE, GDK_KEY_PRESS | GDK_KEY_RELEASE, NULL, GDK_CURRENT_TIME);
		}
		else
		{
			gdk_device_ungrab(keyboard, GDK_CURRENT_TIME);
		}
	}
}
#elif GTK_VERSION == 2
static void remmina_connection_holder_keyboard_grab(RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ

	if (remmina_file_get_int(cnnobj->remmina_file, "keyboard_grab", FALSE))
	{
		gdk_keyboard_grab(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)), TRUE, GDK_CURRENT_TIME);
	}
	else
	{
		gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	}
}
#endif

static gboolean remmina_connection_window_delete_event(GtkWidget* widget, GdkEvent* event, gpointer data)
{
	RemminaConnectionHolder* cnnhld = (RemminaConnectionHolder*) data;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	RemminaConnectionObject* cnnobj;
	GtkNotebook* notebook = GTK_NOTEBOOK(priv->notebook);
	GtkWidget* dialog;
	GtkWidget* w;
	gint i, n;

	n = gtk_notebook_get_n_pages(notebook);
	if (n > 1)
	{
		dialog = gtk_message_dialog_new(GTK_WINDOW(cnnhld->cnnwin), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO,
				_("There are %i active connections in the current window. Are you sure to close?"), n);
		i = gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
		if (i != GTK_RESPONSE_YES)
			return TRUE;
	}
	/* Just in case the connection already closed by the server before clicking yes */
	if (GTK_IS_WIDGET(notebook))
	{
		n = gtk_notebook_get_n_pages(notebook);
		for (i = n - 1; i >= 0; i--)
		{
			w = gtk_notebook_get_nth_page(notebook, i);
			cnnobj = (RemminaConnectionObject*) g_object_get_data(G_OBJECT(w), "cnnobj");
			remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
		}
	}
	return TRUE;
}

static void remmina_connection_window_destroy(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = REMMINA_CONNECTION_WINDOW(widget)->priv;

	if (priv->floating_toolbar_motion_handler)
	{
		g_source_remove(priv->floating_toolbar_motion_handler);
		priv->floating_toolbar_motion_handler = 0;
	}
	if (priv->floating_toolbar != NULL)
	{
		gtk_widget_destroy(priv->floating_toolbar);
		priv->floating_toolbar = NULL;
	}
	if (priv->switch_page_handler)
	{
		g_source_remove(priv->switch_page_handler);
		priv->switch_page_handler = 0;
	}
	g_free(priv);
}

static void remmina_connection_holder_update_toolbar_opacity(RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->floating_toolbar_opacity = (1.0 - TOOLBAR_OPACITY_MIN) / ((gdouble) TOOLBAR_OPACITY_LEVEL)
			* ((gdouble)(TOOLBAR_OPACITY_LEVEL - remmina_file_get_int(cnnobj->remmina_file, "toolbar_opacity", 0)))
			+ TOOLBAR_OPACITY_MIN;

	if (priv->floating_toolbar)
	{
#if GTK_CHECK_VERSION(3, 8, 0)
		gtk_widget_set_opacity(GTK_WIDGET(priv->floating_toolbar), priv->floating_toolbar_opacity);
#else
		gtk_window_set_opacity(GTK_WINDOW(priv->floating_toolbar), priv->floating_toolbar_opacity);
#endif
	}
}

static gboolean remmina_connection_holder_floating_toolbar_motion(RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkRequisition req;
	gint x, y, t, cnnwin_x, cnnwin_y;

	if (priv->floating_toolbar == NULL)
	{
		priv->floating_toolbar_motion_handler = 0;
		return FALSE;
	}

#if GTK_VERSION == 3
	gtk_widget_get_preferred_size(priv->floating_toolbar, &req, NULL);
#elif GTK_VERSION == 2
	gtk_widget_size_request(priv->floating_toolbar, &req);
#endif

	gtk_window_get_position(GTK_WINDOW(priv->floating_toolbar), &x, &y);
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

		gtk_window_move(GTK_WINDOW(priv->floating_toolbar), x + cnnwin_x, y + cnnwin_y);
		if (remmina_pref.invisible_toolbar && !priv->pin_down)
		{
#if GTK_CHECK_VERSION(3, 8, 0)
			gtk_widget_set_opacity(GTK_WIDGET(priv->floating_toolbar),
				(gdouble)(y - t) / (gdouble)(-t) * priv->floating_toolbar_opacity);
#else
			gtk_window_set_opacity(GTK_WINDOW(priv->floating_toolbar),
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
		gtk_window_move(GTK_WINDOW(priv->floating_toolbar), x + cnnwin_x, -20 - req.height + cnnwin_y);
		priv->floating_toolbar_motion_handler = 0;
		return FALSE;
	}
	return TRUE;
}

static void remmina_connection_holder_floating_toolbar_update(RemminaConnectionHolder* cnnhld)
{
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

static void remmina_connection_holder_floating_toolbar_show(RemminaConnectionHolder* cnnhld, gboolean show)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->floating_toolbar == NULL)
		return;

	priv->floating_toolbar_motion_show = show;

	remmina_connection_holder_floating_toolbar_update(cnnhld);
}

static void remmina_connection_holder_floating_toolbar_visible(RemminaConnectionHolder* cnnhld, gboolean visible)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->floating_toolbar == NULL)
		return;

	priv->floating_toolbar_motion_visible = visible;

	remmina_connection_holder_floating_toolbar_update(cnnhld);
}

static void remmina_connection_holder_get_desktop_size(RemminaConnectionHolder* cnnhld, gint* width, gint* height,
		gboolean expand)
{
	DECLARE_CNNOBJ
	RemminaFile* remminafile = cnnobj->remmina_file;
	RemminaProtocolWidget* gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);
	gboolean scale;

	scale = remmina_protocol_widget_get_scale(gp);
	*width = remmina_protocol_widget_get_width(gp);
	if (scale)
	{
		if (remmina_file_get_int(remminafile, "hscale", 0) > 0)
		{
			*width = (*width) * remmina_file_get_int(remminafile, "hscale", 0) / 100;
		}
		else
			if (!expand)
			{
				*width = 1;
			}
	}
	*height = remmina_protocol_widget_get_height(gp);
	if (scale)
	{
		if (remmina_file_get_int(remminafile, "vscale", 0) > 0)
		{
			*height = (*height) * remmina_file_get_int(remminafile, "vscale", 0) / 100;
		}
		else
			if (!expand)
			{
				*height = 1;
			}
	}
}

static void remmina_connection_object_set_scrolled_policy(RemminaConnectionObject* cnnobj, GtkScrolledWindow* scrolled_window)
{
	gboolean expand;

	expand = remmina_protocol_widget_get_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	gtk_scrolled_window_set_policy(scrolled_window, expand ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC,
			expand ? GTK_POLICY_NEVER : GTK_POLICY_AUTOMATIC);
}

static gboolean remmina_connection_holder_toolbar_autofit_restore(RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	gint width, height;
	GtkAllocation na, ca, ta;

	if (cnnobj->connected && GTK_IS_SCROLLED_WINDOW(cnnobj->scrolled_container))
	{
		remmina_connection_holder_get_desktop_size(cnnhld, &width, &height, TRUE);
		gtk_widget_get_allocation(priv->notebook, &na);
		gtk_widget_get_allocation(cnnobj->scrolled_container, &ca);
		gtk_widget_get_allocation(priv->toolbar, &ta);
		gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), MAX(1, width + na.width - ca.width),
				MAX(1, height + ta.height + na.height - ca.height));
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

		g_timeout_add(200, (GSourceFunc) remmina_connection_holder_toolbar_autofit_restore, cnnhld);
	}

}


static void remmina_connection_holder_check_resize(RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ
	gboolean scroll_required = FALSE;
	GdkScreen* screen;
	gint monitor;
	GdkRectangle screen_size;
	gint screen_width, screen_height;
	gint server_width, server_height;

	remmina_connection_holder_get_desktop_size(cnnhld, &server_width, &server_height, FALSE);
	screen = gtk_window_get_screen(GTK_WINDOW(cnnhld->cnnwin));
	monitor = gdk_screen_get_monitor_at_window(screen, gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)));
#ifdef gdk_screen_get_monitor_workarea
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
				gtk_window_resize (GTK_WINDOW(cnnhld->cnnwin), screen_width, screen_height);
				gtk_container_set_border_width (GTK_CONTAINER (cnnhld->cnnwin), scroll_required ? 1 : 0);
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
				gtk_window_set_default_size (GTK_WINDOW(cnnhld->cnnwin),
					remmina_file_get_int (cnnobj->remmina_file, "window_width", 640),
				remmina_file_get_int (cnnobj->remmina_file, "window_height", 480));
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
	gchar* s1;
	gchar* s2;

	if (remmina_pref.hostkey && key1)
	{
		if (key2)
		{
			s1 = g_strdup_printf(" (%s + %s,%s)", gdk_keyval_name(remmina_pref.hostkey),
					gdk_keyval_name(gdk_keyval_to_upper(key1)), gdk_keyval_name(gdk_keyval_to_upper(key2)));
		}
		else
			if (key1 == remmina_pref.hostkey)
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

static void remmina_connection_holder_toolbar_fullscreen(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)))
	{
		remmina_connection_holder_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
	}
	else
	{
		remmina_pref.toolbar_pin_down = cnnhld->cnnwin->priv->pin_down;
		remmina_connection_holder_create_scrolled(cnnhld, NULL);
	}
}

static void remmina_connection_holder_viewport_fullscreen_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnhld->fullscreen_view_mode = VIEWPORT_FULLSCREEN_MODE;
	remmina_connection_holder_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
}

static void remmina_connection_holder_scrolled_fullscreen_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget)))
		return;
	cnnhld->fullscreen_view_mode = SCROLLED_FULLSCREEN_MODE;
	remmina_connection_holder_create_fullscreen(cnnhld, NULL, cnnhld->fullscreen_view_mode);
}

static void remmina_connection_holder_fullscreen_option_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->fullscreen_option_button), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_toolbar_fullscreen_option(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
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

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, priv->toolitem_fullscreen, 0,
			gtk_get_current_event_time());
}

static void remmina_connection_holder_update_alignment(RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ
	RemminaProtocolWidget* gp = REMMINA_PROTOCOL_WIDGET(cnnobj->proto);
	RemminaFile* remminafile = cnnobj->remmina_file;
	gboolean scale, expand;
	gint gp_width, gp_height;
	gint width, height;
	GtkAllocation a;

	scale = remmina_protocol_widget_get_scale(gp);
	expand = remmina_protocol_widget_get_expand(gp);
	gp_width = remmina_protocol_widget_get_width(gp);
	gp_height = remmina_protocol_widget_get_height(gp);

	if (scale && remmina_file_get_int(remminafile, "aspectscale", FALSE)
			&& remmina_file_get_int(remminafile, "hscale", 0) == 0)
	{
		gtk_widget_get_allocation(cnnobj->alignment, &a);
		width = a.width;
		height = a.height;
		if (width > 1 && height > 1)
		{
			if (gp_width * height >= width * gp_height)
			{
				gtk_alignment_set(GTK_ALIGNMENT(cnnobj->alignment), 0.5, 0.5, 1.0,
						(gfloat)(gp_height * width) / (gfloat) gp_width / (gfloat) height);
			}
			else
			{
				gtk_alignment_set(GTK_ALIGNMENT(cnnobj->alignment), 0.5, 0.5,
						(gfloat)(gp_width * height) / (gfloat) gp_height / (gfloat) width, 1.0);
			}
		}
	}
	else
	{
		gtk_alignment_set(
				GTK_ALIGNMENT(cnnobj->alignment),
				0.5,
				0.5,
				((scale && remmina_file_get_int(remminafile, "hscale", 0) == 0) || expand ? 1.0 : 0.0),
				((scale && remmina_file_get_int (remminafile, "vscale", 0) == 0) || expand ? 1.0 : 0.0));
			}
		}

static void remmina_connection_holder_switch_page_activate(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	gint page_num;

	page_num = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(menuitem), "new-page-num"));
	gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), page_num);
}

static void remmina_connection_holder_toolbar_switch_page_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_switch_page), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_toolbar_switch_page(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
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

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
}

static void remmina_connection_holder_toolbar_scaled_mode(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ
	gboolean scale;

	remmina_connection_holder_update_alignment(cnnhld);

	scale = gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget));
	gtk_widget_set_sensitive(GTK_WIDGET(cnnhld->cnnwin->priv->scale_option_button), scale);
	remmina_protocol_widget_set_scale(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), scale);
	remmina_file_set_int(cnnobj->remmina_file, "scale", scale);

	remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, 0);
	if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
	{
		remmina_connection_holder_check_resize(cnnhld);
	}
}

static void remmina_connection_holder_scale_option_on_scaled(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ

	remmina_file_set_int(cnnobj->remmina_file, "hscale", REMMINA_SCALER(widget)->hscale);
	remmina_file_set_int(cnnobj->remmina_file, "vscale", REMMINA_SCALER(widget)->vscale);
	remmina_file_set_int(cnnobj->remmina_file, "aspectscale", REMMINA_SCALER(widget)->aspectscale);
	remmina_connection_holder_update_alignment(cnnhld);
	remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_SCALE, 0);
	if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
	{
		remmina_connection_holder_check_resize(cnnhld);
	}
}

static void remmina_connection_holder_scale_option_popdown(RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
#if GTK_VERSION == 3
	GdkDisplay *display;
	GdkDeviceManager *manager;
	GdkDevice *device = NULL;

	priv->sticky = FALSE;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->scale_option_button), FALSE);

	display = gtk_widget_get_display(GTK_WIDGET(cnnhld));
	manager = gdk_display_get_device_manager(display);
	device = gdk_device_manager_get_client_pointer(manager);
	if (device != NULL)
	{
		gdk_device_ungrab(device, GDK_CURRENT_TIME);
	}
#elif GTK_VERSION == 2
	priv->sticky = FALSE;

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(priv->scale_option_button), FALSE);

	gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	gdk_pointer_ungrab(GDK_CURRENT_TIME);
#endif
	if (scale_option_window)
	{
		gtk_grab_remove(scale_option_window);
		gtk_widget_destroy(scale_option_window);
		scale_option_window = NULL;
	}
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static gboolean remmina_connection_holder_trap_on_button(GtkWidget* widget, GdkEventButton* event, gpointer data)
{
	return TRUE;
}

static gboolean remmina_connection_holder_scale_option_on_key(GtkWidget* widget, GdkEventKey* event,
		RemminaConnectionHolder* cnnhld)
{
	switch (event->keyval)
	{
		case GDK_KEY_Escape:
			remmina_connection_holder_scale_option_popdown(cnnhld);
			return TRUE;
	}
	return FALSE;
}

static gboolean remmina_connection_holder_scale_option_on_button(GtkWidget* widget, GdkEventButton* event,
		RemminaConnectionHolder* cnnhld)
{
	remmina_connection_holder_scale_option_popdown(cnnhld);
	return TRUE;
}

static void remmina_connection_holder_toolbar_scale_option(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	RemminaFile* remminafile = cnnobj->remmina_file;
	GtkWidget* window;
	GtkWidget* eventbox;
	GtkWidget* frame;
	GtkWidget* scaler;
#if GTK_VERSION == 3
	GdkDisplay* display;
	GdkDevice* pointer;
	GdkDeviceManager* device_manager;
#endif
	gint x, y;
	gboolean pushin;

	if (scale_option_window)
	{
		remmina_connection_holder_scale_option_popdown(cnnhld);
	}
	else
		if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)))
		{
			window = gtk_window_new(GTK_WINDOW_POPUP);
			gtk_container_set_border_width(GTK_CONTAINER(window), 0);

			/* Use an event-box to trap all button clicks events before sending up to the window */
			eventbox = gtk_event_box_new();
			gtk_widget_show(eventbox);
			gtk_container_add(GTK_CONTAINER(window), eventbox);
			gtk_widget_add_events(eventbox, GDK_BUTTON_PRESS_MASK);
			g_signal_connect(G_OBJECT(eventbox), "button-press-event",
					G_CALLBACK(remmina_connection_holder_trap_on_button), NULL);

			frame = gtk_frame_new(NULL);
			gtk_widget_show(frame);
			gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
			gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
			gtk_container_add(GTK_CONTAINER(eventbox), frame);

			scaler = remmina_scaler_new();
			gtk_widget_show(scaler);
			gtk_widget_set_size_request(scaler, 250, -1);
			gtk_container_set_border_width(GTK_CONTAINER(scaler), 4);
			remmina_scaler_set(REMMINA_SCALER(scaler), remmina_file_get_int(remminafile, "hscale", 0),
					remmina_file_get_int(remminafile, "vscale", 0),
					remmina_file_get_int(remminafile, "aspectscale", FALSE));
			remmina_scaler_set_draw_value(REMMINA_SCALER(scaler), FALSE);
			gtk_container_add(GTK_CONTAINER(frame), scaler);
			g_signal_connect(G_OBJECT(scaler), "scaled",
					G_CALLBACK(remmina_connection_holder_scale_option_on_scaled), cnnhld);
			g_signal_connect(G_OBJECT(window), "key-press-event",
					G_CALLBACK(remmina_connection_holder_scale_option_on_key), cnnhld);
			g_signal_connect(G_OBJECT(window), "button-press-event",
					G_CALLBACK(remmina_connection_holder_scale_option_on_button), cnnhld);

			gtk_widget_realize(window);
			remmina_public_popup_position(NULL, &x, &y, &pushin, priv->toolitem_scale);
			gtk_window_move(GTK_WINDOW(window), x, y);
			gtk_widget_show(window);

			gtk_grab_add(window);
#if GTK_VERSION == 3
			display = gdk_window_get_display(gtk_widget_get_window(window));
			device_manager = gdk_display_get_device_manager(display);
			pointer = gdk_device_manager_get_client_pointer(device_manager);
			gdk_device_grab(pointer, gtk_widget_get_window(window), GDK_OWNERSHIP_NONE, TRUE,
					GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK, NULL,
					GDK_CURRENT_TIME);
#elif GTK_VERSION == 2
			gdk_pointer_grab(gtk_widget_get_window(window), TRUE,
					GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK, NULL, NULL,
					GDK_CURRENT_TIME);
			gdk_keyboard_grab(gtk_widget_get_window(window), TRUE, GDK_CURRENT_TIME);
#endif
			scale_option_window = window;
			priv->sticky = TRUE;
		}
}

static void remmina_connection_holder_toolbar_preferences_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_preferences), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_toolbar_tools_popdown(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	priv->sticky = FALSE;

	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_tools), FALSE);
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
}

static void remmina_connection_holder_call_protocol_feature_radio(GtkMenuItem* menuitem, RemminaConnectionHolder* cnnhld)
{
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
	DECLARE_CNNOBJ
	RemminaProtocolFeature* feature;

	feature = (RemminaProtocolFeature*) g_object_get_data(G_OBJECT(menuitem), "feature-type");
	remmina_protocol_widget_call_feature_by_ref(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), feature);
}

static void remmina_connection_holder_toolbar_preferences_radio(RemminaConnectionHolder* cnnhld, RemminaFile* remminafile,
		GtkWidget* menu, const RemminaProtocolFeature* feature, const gchar* domain, gboolean enabled)
{
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

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
}

static void remmina_connection_holder_toolbar_tools(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	const RemminaProtocolFeature* feature;
	GtkWidget* menu;
	GtkWidget* menuitem;
	GtkWidget* image;
	const gchar* domain;
	gboolean enabled;

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
			if (feature->opt2)
			{
				image = gtk_image_new_from_icon_name((const gchar*) feature->opt2, GTK_ICON_SIZE_MENU);
				gtk_widget_show(image);
			}
		}
		else
		{
			menuitem = gtk_image_menu_item_new_from_stock((const gchar*) feature->opt2, NULL);
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

	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, remmina_public_popup_position, widget, 0, gtk_get_current_event_time());
}

static void remmina_connection_holder_toolbar_minimize(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
	gtk_window_iconify(GTK_WINDOW(cnnhld->cnnwin));
}

static void remmina_connection_holder_toolbar_disconnect(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	remmina_connection_holder_disconnect(cnnhld);
}

static void remmina_connection_holder_toolbar_grab(GtkWidget* widget, RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ

	remmina_file_set_int(cnnobj->remmina_file, "keyboard_grab",
			gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(widget)));
	remmina_connection_holder_keyboard_grab(cnnhld);
}

static GtkWidget*
remmina_connection_holder_create_toolbar(RemminaConnectionHolder* cnnhld, gint mode)
{
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
	gtk_button_set_focus_on_click(GTK_BUTTON(widget), FALSE);
	if (remmina_pref.small_toolbutton)
	{
		gtk_widget_set_name(widget, "remmina-small-button");
	}
	gtk_container_add(GTK_CONTAINER(toolitem), widget);

	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
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

	toolitem = gtk_toggle_tool_button_new();
	gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(toolitem), "remmina-scale");
	remmina_connection_holder_set_tooltip(GTK_WIDGET(toolitem), _("Toggle scaled mode"), remmina_pref.shortcutkey_scale, 0);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);
	gtk_widget_show(GTK_WIDGET(toolitem));
	g_signal_connect(G_OBJECT(toolitem), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_scaled_mode), cnnhld);
	priv->toolitem_scale = toolitem;

	/* We need a toggle tool button with a popup arrow; and the popup is a window not a menu.
	 GTK+ support neither of them. We need some tricks here... */
	toolitem = gtk_tool_item_new();
	gtk_widget_show(GTK_WIDGET(toolitem));
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), toolitem, -1);

	widget = gtk_toggle_button_new();
	gtk_widget_show(widget);
	gtk_container_set_border_width(GTK_CONTAINER(widget), 0);
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(widget), FALSE);
	if (remmina_pref.small_toolbutton)
	{
		gtk_widget_set_name(widget, "remmina-small-button");
	}
	gtk_container_add(GTK_CONTAINER(toolitem), widget);

	arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_NONE);
	gtk_widget_show(arrow);
	gtk_container_add(GTK_CONTAINER(widget), arrow);

	g_signal_connect(G_OBJECT(widget), "toggled", G_CALLBACK(remmina_connection_holder_toolbar_scale_option), cnnhld);
	priv->scale_option_button = widget;

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

static void remmina_connection_holder_update_toolbar(RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkToolItem* toolitem;
	gboolean bval;

	toolitem = priv->toolitem_autofit;
	if (toolitem)
	{
		if (priv->view_mode != SCROLLED_WINDOW_MODE)
		{
			gtk_widget_set_sensitive(GTK_WIDGET(toolitem), FALSE);
		}
		else
		{
			bval = remmina_protocol_widget_get_expand(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
			gtk_widget_set_sensitive(GTK_WIDGET(toolitem), !bval);
		}
	}

	toolitem = priv->toolitem_switch_page;
	bval = (gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)) > 1);
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval);

	toolitem = priv->toolitem_scale;
	bval = remmina_protocol_widget_get_scale(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(toolitem), bval);
	gtk_widget_set_sensitive(GTK_WIDGET(priv->scale_option_button), bval);
	bval = remmina_protocol_widget_query_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_SCALE);
	gtk_widget_set_sensitive(GTK_WIDGET(toolitem), bval);

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

	if (priv->floating_toolbar)
	{
		gtk_label_set_text(GTK_LABEL(priv->floating_toolbar_label),
				remmina_file_get_string(cnnobj->remmina_file, "name"));
	}
}

static void remmina_connection_holder_showhide_toolbar(RemminaConnectionHolder* cnnhld, gboolean resize)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkRequisition req;
	gint width, height;

	if (priv->view_mode == SCROLLED_WINDOW_MODE)
	{
		if (resize
				&& (gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin)))
						& GDK_WINDOW_STATE_MAXIMIZED) == 0)
		{
			gtk_window_get_size(GTK_WINDOW(cnnhld->cnnwin), &width, &height);
#if GTK_VERSION == 3
			gtk_widget_get_preferred_size(priv->toolbar, &req, NULL);
#elif GTK_VERSION == 2
			gtk_widget_size_request(priv->toolbar, &req);
#endif
			if (remmina_pref.hide_connection_toolbar)
			{
				gtk_widget_hide(priv->toolbar);
				gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), width, height - req.height);
			}
			else
			{
				gtk_window_resize(GTK_WINDOW(cnnhld->cnnwin), width, height + req.height);
				gtk_widget_show(priv->toolbar);
			}
		}
		else
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
}

static gboolean remmina_connection_holder_floating_toolbar_on_enter(GtkWidget* widget, GdkEventCrossing* event,
		RemminaConnectionHolder* cnnhld)
{
	remmina_connection_holder_floating_toolbar_show(cnnhld, TRUE);
	return TRUE;
}

static gboolean remmina_connection_holder_floating_toolbar_on_leave(GtkWidget* widget, GdkEventCrossing* event,
		RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (!priv->sticky && event->mode == GDK_CROSSING_NORMAL)
	{
		remmina_connection_holder_floating_toolbar_show(cnnhld, FALSE);
		return TRUE;
	}
	return FALSE;
}

static gboolean remmina_connection_window_focus_in(GtkWidget* widget, GdkEventFocus* event, RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (priv->floating_toolbar)
	{
		remmina_connection_holder_floating_toolbar_visible(cnnhld, TRUE);
	}
	return FALSE;
}

static gboolean remmina_connection_window_focus_out(GtkWidget* widget, GdkEventFocus* event, RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	cnnhld->hostkey_activated = FALSE;
	if (!priv->sticky && priv->floating_toolbar)
	{
		remmina_connection_holder_floating_toolbar_visible(cnnhld, FALSE);
	}
	if (REMMINA_IS_SCROLLED_VIEWPORT(cnnobj->scrolled_container))
	{
		remmina_scrolled_viewport_remove_motion(REMMINA_SCROLLED_VIEWPORT(cnnobj->scrolled_container));
	}
	remmina_protocol_widget_call_feature_by_type(REMMINA_PROTOCOL_WIDGET(cnnobj->proto),
			REMMINA_PROTOCOL_FEATURE_TYPE_UNFOCUS, 0);
	return FALSE;
}

static gboolean remmina_connection_window_on_enter(GtkWidget* widget, GdkEventCrossing* event, RemminaConnectionHolder* cnnhld)
{
	if (event->detail == GDK_NOTIFY_VIRTUAL || event->detail == GDK_NOTIFY_NONLINEAR
			|| event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL)
	{
		remmina_connection_holder_keyboard_grab(cnnhld);
	}
	return FALSE;
}

#if GTK_VERSION == 3
static gboolean remmina_connection_window_on_leave(GtkWidget* widget, GdkEventCrossing* event, RemminaConnectionHolder* cnnhld)
{
	GdkDisplay *display;
	GdkDeviceManager *manager;
	GdkDevice *device = NULL;
	
	if (event->detail == GDK_NOTIFY_VIRTUAL || event->detail == GDK_NOTIFY_NONLINEAR
			|| event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL)
	{
		display = gtk_widget_get_display(widget);
		manager = gdk_display_get_device_manager(display);
		device = gdk_device_manager_get_client_pointer(manager);
		if (device != NULL)
		{
			if ( gdk_device_get_source (device) != GDK_SOURCE_KEYBOARD ) {
				device = gdk_device_get_associated_device (device);
			}
			gdk_device_ungrab(device, GDK_CURRENT_TIME);
		}
	}
	return FALSE;
}
#elif GTK_VERSION == 2
static gboolean remmina_connection_window_on_leave(GtkWidget* widget, GdkEventCrossing* event, RemminaConnectionHolder* cnnhld)
{
	if (event->detail == GDK_NOTIFY_VIRTUAL || event->detail == GDK_NOTIFY_NONLINEAR
		|| event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL)
	{
		gdk_keyboard_ungrab(GDK_CURRENT_TIME);
	}
	return FALSE;
}
#endif

static gboolean remmina_connection_holder_floating_toolbar_on_scroll(GtkWidget* widget, GdkEventScroll* event,
		RemminaConnectionHolder* cnnhld)
{
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

static void remmina_connection_object_alignment_on_allocate(GtkWidget* widget, GtkAllocation* allocation,
		RemminaConnectionObject* cnnobj)
{
	remmina_connection_holder_update_alignment(cnnobj->cnnhld);
}

static gboolean remmina_connection_window_on_configure(GtkWidget* widget, GdkEventConfigure* event,
		RemminaConnectionHolder* cnnhld)
{
	DECLARE_CNNOBJ_WITH_RETURN(FALSE)
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkRequisition req;
	gint width, height;
	gint y;

	if (cnnhld->cnnwin && gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin))
			&& cnnhld->cnnwin->priv->view_mode == SCROLLED_WINDOW_MODE)
	{
		/* Here we store the window state in real-time */
		if ((gdk_window_get_state(gtk_widget_get_window(GTK_WIDGET(cnnhld->cnnwin))) & GDK_WINDOW_STATE_MAXIMIZED) == 0)
		{
			gtk_window_get_size(GTK_WINDOW(cnnhld->cnnwin), &width, &height);
			remmina_file_set_int(cnnobj->remmina_file, "window_width", width);
			remmina_file_set_int(cnnobj->remmina_file, "window_height", height);
			remmina_file_set_int(cnnobj->remmina_file, "window_maximize", FALSE);
		}
		else
		{
			remmina_file_set_int(cnnobj->remmina_file, "window_maximize", TRUE);
		}
	}

	if (priv->floating_toolbar)
	{
#if GTK_VERSION == 3
		gtk_widget_get_preferred_size(priv->floating_toolbar, &req, NULL);
#elif GTK_VERSION == 2
		gtk_widget_size_request(priv->floating_toolbar, &req);
#endif
		gtk_window_get_position(GTK_WINDOW(priv->floating_toolbar), NULL, &y);
		gtk_window_move(GTK_WINDOW(priv->floating_toolbar), event->x + MAX(0, (event->width - req.width) / 2), y);

		remmina_connection_holder_floating_toolbar_update(cnnhld);
	}

	if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
	{
		/* Notify window of change so that scroll border can be hidden or shown if needed */
		remmina_connection_holder_check_resize(cnnobj->cnnhld);
	}
	return FALSE;
}

static void remmina_connection_holder_update_pin(RemminaConnectionHolder* cnnhld)
{
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
	cnnhld->cnnwin->priv->pin_down = !cnnhld->cnnwin->priv->pin_down;
	remmina_connection_holder_update_pin(cnnhld);
}

static void remmina_connection_holder_create_floating_toolbar(RemminaConnectionHolder* cnnhld, gint mode)
{
	DECLARE_CNNOBJ
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;
	GtkWidget* window;
	GtkWidget* vbox;
	GtkWidget* widget;
	GtkWidget* eventbox;
	GtkWidget* hbox;

	/* This has to be a popup window to become visible in fullscreen mode */
	window = gtk_window_new(GTK_WINDOW_POPUP);

#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	widget = remmina_connection_holder_create_toolbar(cnnhld, mode);
	gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, FALSE, 0);

#if GTK_VERSION == 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#elif GTK_VERSION == 2
	hbox = gtk_hbox_new(FALSE, 0);
#endif
	gtk_widget_show(hbox);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	/* The pin button */
	widget = gtk_button_new();
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(widget), FALSE);
	gtk_widget_set_name(widget, "remmina-small-button");
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_connection_holder_toolbar_pin), cnnhld);
	priv->pin_button = widget;
	priv->pin_down = remmina_pref.toolbar_pin_down;
	remmina_connection_holder_update_pin(cnnhld);

	/* An event box is required to wrap the label to avoid infinite "leave-enter" event loop */
	eventbox = gtk_event_box_new();
	gtk_widget_show(eventbox);
	gtk_box_pack_start(GTK_BOX(hbox), eventbox, TRUE, TRUE, 0);
	widget = gtk_label_new(remmina_file_get_string(cnnobj->remmina_file, "name"));
	gtk_label_set_max_width_chars(GTK_LABEL(widget), 50);
	gtk_widget_show(widget);
	gtk_container_add(GTK_CONTAINER(eventbox), widget);
	priv->floating_toolbar_label = widget;

	/* The position will be moved in configure event instead during maximizing. Just make it invisible here */
	gtk_window_move(GTK_WINDOW(window), 0, 6000);
	gtk_window_set_accept_focus(GTK_WINDOW(window), FALSE);

	priv->floating_toolbar = window;

	remmina_connection_holder_update_toolbar_opacity(cnnhld);
	if (remmina_pref.invisible_toolbar && !priv->pin_down)
	{
#if GTK_CHECK_VERSION(3, 8, 0)
		gtk_widget_set_opacity(GTK_WIDGET(window), 0.0);
#else
		gtk_window_set_opacity(GTK_WINDOW(window), 0.0);
#endif
	}

	g_signal_connect(G_OBJECT(window), "enter-notify-event", G_CALLBACK(remmina_connection_holder_floating_toolbar_on_enter), cnnhld);
	g_signal_connect(G_OBJECT(window), "leave-notify-event", G_CALLBACK(remmina_connection_holder_floating_toolbar_on_leave), cnnhld);
	g_signal_connect(G_OBJECT(window), "scroll-event", G_CALLBACK(remmina_connection_holder_floating_toolbar_on_scroll), cnnhld);
	gtk_widget_add_events(GTK_WIDGET(window), GDK_SCROLL_MASK);

	if (cnnobj->connected)
		gtk_widget_show(window);
}

static void remmina_connection_window_init(RemminaConnectionWindow* cnnwin)
{
	RemminaConnectionWindowPriv* priv;

	priv = g_new0(RemminaConnectionWindowPriv, 1);
	cnnwin->priv = priv;

	priv->view_mode = AUTO_MODE;
	priv->floating_toolbar_opacity = 1.0;

	gtk_window_set_position(GTK_WINDOW(cnnwin), GTK_WIN_POS_CENTER);
	gtk_container_set_border_width(GTK_CONTAINER(cnnwin), 0);

	remmina_widget_pool_register(GTK_WIDGET(cnnwin));
}

static gboolean remmina_connection_window_state_event(GtkWidget* widget, GdkEventWindowState* event, gpointer user_data)
{
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
    //return FALSE; uncomment if next one gives trouble
#endif
    return FALSE; // moved here because a function should return a value. Should be correct
}

static GtkWidget*
remmina_connection_window_new_from_holder(RemminaConnectionHolder* cnnhld)
{
	RemminaConnectionWindow* cnnwin;

	cnnwin = REMMINA_CONNECTION_WINDOW(g_object_new(REMMINA_TYPE_CONNECTION_WINDOW, NULL));
	cnnwin->priv->cnnhld = cnnhld;

	g_signal_connect(G_OBJECT(cnnwin), "delete-event", G_CALLBACK(remmina_connection_window_delete_event), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "destroy", G_CALLBACK(remmina_connection_window_destroy), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "window-state-event", G_CALLBACK(remmina_connection_window_state_event), cnnhld);

	g_signal_connect(G_OBJECT(cnnwin), "focus-in-event", G_CALLBACK(remmina_connection_window_focus_in), cnnhld);
	g_signal_connect(G_OBJECT(cnnwin), "focus-out-event", G_CALLBACK(remmina_connection_window_focus_out), cnnhld);

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
	g_object_set_data(G_OBJECT(container), "cnnobj", cnnobj);
	gtk_widget_show(container);
	cnnobj->scrolled_container = container;
}

static gboolean remmina_connection_holder_grab_focus(gpointer data)
{
	RemminaConnectionObject* cnnobj;
	GtkNotebook* notebook;
	GtkWidget* child;

	if (GTK_IS_WIDGET(data))
	{
		notebook = GTK_NOTEBOOK(data);
		child = gtk_notebook_get_nth_page(notebook, gtk_notebook_get_current_page(notebook));
		cnnobj = g_object_get_data(G_OBJECT(child), "cnnobj");
		if (GTK_IS_WIDGET(cnnobj->proto))
		{
			remmina_protocol_widget_grab_focus(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
		}
	}
	return FALSE;
}

static void remmina_connection_object_on_close_button_clicked(GtkButton* button, RemminaConnectionObject* cnnobj)
{
	if (REMMINA_IS_PROTOCOL_WIDGET(cnnobj->proto))
	{
		remmina_protocol_widget_close_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto));
	}
}

static GtkWidget* remmina_connection_object_create_tab(RemminaConnectionObject* cnnobj)
{
	GtkWidget* hbox;
	GtkWidget* widget;
	GtkWidget* button;

#if GTK_VERSION == 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#elif GTK_VERSION == 2
	hbox = gtk_hbox_new(FALSE, 4);
#endif
	gtk_widget_show(hbox);

	widget = gtk_image_new_from_icon_name(remmina_file_get_icon_name(cnnobj->remmina_file), GTK_ICON_SIZE_MENU);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, FALSE, FALSE, 0);

	widget = gtk_label_new(remmina_file_get_string(cnnobj->remmina_file, "name"));
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_widget_show(widget);
	gtk_box_pack_start(GTK_BOX(hbox), widget, TRUE, TRUE, 0);

	button = gtk_button_new();
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click(GTK_BUTTON(button), FALSE);
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
	gint i, n, c;
	GtkWidget* tab;
	GtkWidget* widget;

	if (cnnobj)
	{
		/* Initial connection for a newly created window */
		tab = remmina_connection_object_create_tab(cnnobj);
		remmina_connection_object_append_page(cnnobj, to, tab, view_mode);

#if GTK_VERSION == 3		
		/* Reparent cnnobj->viewport */
		g_object_ref(cnnobj->viewport);
		gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(cnnobj->viewport)), cnnobj->viewport);
		gtk_container_add(GTK_CONTAINER(cnnobj->scrolled_container), cnnobj->viewport );
		g_object_unref(cnnobj->viewport);
#elif GTK_VERSION == 2
		gtk_widget_reparent(cnnobj->viewport, cnnobj->scrolled_container);
#endif

		if (cnnobj->window)
		{
			gtk_widget_destroy(cnnobj->window);
			cnnobj->window = NULL;
		}
	}
	else
	{
		/* View mode changed. Migrate all existing connections to the new notebook */
		c = gtk_notebook_get_current_page(from);
		n = gtk_notebook_get_n_pages(from);
		for (i = 0; i < n; i++)
		{
			widget = gtk_notebook_get_nth_page(from, i);
			cnnobj = (RemminaConnectionObject*) g_object_get_data(G_OBJECT(widget), "cnnobj");

			tab = remmina_connection_object_create_tab(cnnobj);
			remmina_connection_object_append_page(cnnobj, to, tab, view_mode);

#if GTK_VERSION == 3		
			/* Reparent cnnobj->viewport */
			g_object_ref(cnnobj->viewport);
			gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(cnnobj->viewport)), cnnobj->viewport);
			gtk_container_add(GTK_CONTAINER(cnnobj->scrolled_container), cnnobj->viewport );
			g_object_unref(cnnobj->viewport);
#elif GTK_VERSION == 2
			gtk_widget_reparent(cnnobj->viewport, cnnobj->scrolled_container);
#endif

		}
		gtk_notebook_set_current_page(to, c);
	}
}

static void remmina_connection_holder_update_notebook(RemminaConnectionHolder* cnnhld)
{
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
	RemminaConnectionHolder* cnnhld = (RemminaConnectionHolder*) data;
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (GTK_IS_WIDGET(cnnhld->cnnwin))
	{
		remmina_connection_holder_update_toolbar(cnnhld);
		remmina_connection_holder_grab_focus(priv->notebook);
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
	RemminaConnectionWindowPriv* priv = cnnhld->cnnwin->priv;

	if (!priv->switch_page_handler)
	{
		priv->switch_page_handler = g_idle_add(remmina_connection_holder_on_switch_page_real, cnnhld);
	}
}

static void remmina_connection_holder_on_page_added(GtkNotebook* notebook, GtkWidget* child, guint page_num,
		RemminaConnectionHolder* cnnhld)
{
	if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook)) <= 0)
	{
		gtk_widget_destroy(GTK_WIDGET(cnnhld->cnnwin));
		g_free(cnnhld);
	}
	else
	{
		remmina_connection_holder_update_notebook(cnnhld);
	}
}

GtkNotebook*
remmina_connection_holder_on_notebook_create_window(GtkNotebook* notebook, GtkWidget* page, gint x, gint y, gpointer data)
{
	RemminaConnectionWindow* srccnnwin;
	RemminaConnectionWindow* dstcnnwin;
	RemminaConnectionObject* cnnobj;
	gint srcpagenum;
	GdkWindow* window;

#if GTK_VERSION == 3
	GdkDeviceManager* manager;
	GdkDevice* device = NULL;

	manager = gdk_display_get_device_manager(gdk_display_get_default());
	device = gdk_device_manager_get_client_pointer(manager);
	window = gdk_device_get_window_at_position(device, &x, &y);
#elif GTK_VERSION == 2
	window = gdk_display_get_window_at_pointer(gdk_display_get_default(), &x, &y);
#endif
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
	}

	g_signal_emit_by_name(cnnobj->proto, "connect", cnnobj);
	gtk_notebook_remove_page(GTK_NOTEBOOK(srccnnwin->priv->notebook), srcpagenum);

	return NULL;
}

static GtkWidget*
remmina_connection_holder_create_notebook(RemminaConnectionHolder* cnnhld)
{
	GtkWidget* notebook;

	notebook = gtk_notebook_new();
	gtk_notebook_set_scrollable(GTK_NOTEBOOK(notebook), TRUE);
	gtk_widget_show(notebook);

	g_signal_connect(G_OBJECT(notebook), "create-window", G_CALLBACK(remmina_connection_holder_on_notebook_create_window),
			cnnhld);
	g_signal_connect(G_OBJECT(notebook), "switch-page", G_CALLBACK(remmina_connection_holder_on_switch_page), cnnhld);
	g_signal_connect(G_OBJECT(notebook), "page-added", G_CALLBACK(remmina_connection_holder_on_page_added), cnnhld);
	g_signal_connect(G_OBJECT(notebook), "page-removed", G_CALLBACK(remmina_connection_holder_on_page_added), cnnhld);
	gtk_widget_set_can_focus(notebook, FALSE);

	return notebook;
}

/* Create a scrolled window container */
static void remmina_connection_holder_create_scrolled(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj)
{
	GtkWidget* window;
	GtkWidget* oldwindow;
	GtkWidget* vbox;
	GtkWidget* toolbar;
	GtkWidget* notebook;
	gchar* tag;

	oldwindow = GTK_WIDGET(cnnhld->cnnwin);
	window = remmina_connection_window_new_from_holder(cnnhld);
	gtk_widget_realize(window);
	cnnhld->cnnwin = REMMINA_CONNECTION_WINDOW(window);

	/* Create the vbox container */
#if GTK_VERSION == 3
	vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#elif GTK_VERSION == 2
	vbox = gtk_vbox_new(FALSE, 0);
#endif
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(window), vbox);

	/* Create the toolbar */
	toolbar = remmina_connection_holder_create_toolbar(cnnhld, SCROLLED_WINDOW_MODE);
	gtk_box_pack_start(GTK_BOX(vbox), toolbar, FALSE, FALSE, 0);

	/* Create the notebook */
	notebook = remmina_connection_holder_create_notebook(cnnhld);
	gtk_box_pack_start(GTK_BOX(vbox), notebook, TRUE, TRUE, 0);
	gtk_container_set_focus_chain(GTK_CONTAINER(vbox), g_list_append(NULL, notebook));

	cnnhld->cnnwin->priv->view_mode = SCROLLED_WINDOW_MODE;
	cnnhld->cnnwin->priv->toolbar = toolbar;
	cnnhld->cnnwin->priv->notebook = notebook;

	remmina_connection_window_initialize_notebook(GTK_NOTEBOOK(notebook),
			(oldwindow ? GTK_NOTEBOOK(REMMINA_CONNECTION_WINDOW(oldwindow)->priv->notebook) : NULL), cnnobj,
			SCROLLED_WINDOW_MODE);

	if (cnnobj)
	{
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
		gtk_widget_destroy(oldwindow);
	}

	remmina_connection_holder_update_toolbar(cnnhld);
	remmina_connection_holder_showhide_toolbar(cnnhld, FALSE);
	remmina_connection_holder_check_resize(cnnhld);

	gtk_widget_show(GTK_WIDGET(cnnhld->cnnwin));
}

static void remmina_connection_holder_create_fullscreen(RemminaConnectionHolder* cnnhld, RemminaConnectionObject* cnnobj,
		gint view_mode)
{
	GtkWidget* window;
	GtkWidget* oldwindow;
	GtkWidget* notebook;
	gchar* tag;

	oldwindow = GTK_WIDGET(cnnhld->cnnwin);
	window = remmina_connection_window_new_from_holder(cnnhld);
	gtk_widget_realize(window);
	cnnhld->cnnwin = REMMINA_CONNECTION_WINDOW(window);

	if (!view_mode)
		view_mode = VIEWPORT_FULLSCREEN_MODE;

	if (view_mode == VIEWPORT_FULLSCREEN_MODE)
	{
		//gdk_color_parse("black", &color);
		GdkRGBA color = {.0, .0, .0, 1.0};
		gtk_widget_override_background_color(window, GTK_STATE_NORMAL, &color);
	}

	notebook = remmina_connection_holder_create_notebook(cnnhld);
	gtk_container_add(GTK_CONTAINER(window), notebook);

	cnnhld->cnnwin->priv->notebook = notebook;
	cnnhld->cnnwin->priv->view_mode = view_mode;
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
	remmina_connection_holder_create_floating_toolbar(cnnhld, view_mode);
	remmina_connection_holder_update_toolbar(cnnhld);

	gtk_window_fullscreen(GTK_WINDOW(window));
	remmina_connection_holder_check_resize(cnnhld);

	gtk_widget_show(window);
}

static gboolean remmina_connection_window_hostkey_func(RemminaProtocolWidget* gp, guint keyval, gboolean release,
		RemminaConnectionHolder* cnnhld)
{
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
				return TRUE;
			}
			/* If hostkey is released without pressing other keys, we should execute the
			 * shortcut key which is the same as hostkey. Be default, this is grab/ungrab
			 * keyboard */
		}
		else
			if (cnnhld->hostkey_activated)
			{
				/* Trap all key releases when hostkey is pressed */
				return TRUE;
			}
			else
			{
				return FALSE;
			}
	}
	else
		if (remmina_pref.hostkey && keyval == remmina_pref.hostkey)
		{
			cnnhld->hostkey_activated = TRUE;
			cnnhld->hostkey_used = FALSE;
			return TRUE;
		}
		else
			if (!cnnhld->hostkey_activated)
			{
				return FALSE;
			}

	cnnhld->hostkey_used = TRUE;
	keyval = gdk_keyval_to_lower(keyval);
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
				remmina_pref.toolbar_pin_down = cnnhld->cnnwin->priv->pin_down;
				remmina_connection_holder_create_scrolled(cnnhld, NULL);
				break;
			default:
				break;
		}
	}
	else
		if (keyval == remmina_pref.shortcutkey_autofit)
		{
			if (priv->toolitem_autofit && gtk_widget_is_sensitive(GTK_WIDGET(priv->toolitem_autofit)))
			{
				remmina_connection_holder_toolbar_autofit(GTK_WIDGET(gp), cnnhld);
			}
		}
		else
			if (keyval == remmina_pref.shortcutkey_nexttab)
			{
				i = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)) + 1;
				if (i >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)))
					i = 0;
				gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), i);
			}
			else
				if (keyval == remmina_pref.shortcutkey_prevtab)
				{
					i = gtk_notebook_get_current_page(GTK_NOTEBOOK(priv->notebook)) - 1;
					if (i < 0)
						i = gtk_notebook_get_n_pages(GTK_NOTEBOOK(priv->notebook)) - 1;
					gtk_notebook_set_current_page(GTK_NOTEBOOK(priv->notebook), i);
				}
				else
					if (keyval == remmina_pref.shortcutkey_scale)
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
					else
						if (keyval == remmina_pref.shortcutkey_grab)
						{
							gtk_toggle_tool_button_set_active(
									GTK_TOGGLE_TOOL_BUTTON(priv->toolitem_grab),
									!gtk_toggle_tool_button_get_active(
											GTK_TOGGLE_TOOL_BUTTON(
													priv->toolitem_grab)));
						}
						else
							if (keyval == remmina_pref.shortcutkey_minimize)
							{
								remmina_connection_holder_toolbar_minimize(GTK_WIDGET(gp),
										cnnhld);
							}
							else
								if (keyval == remmina_pref.shortcutkey_disconnect)
								{
									remmina_connection_holder_disconnect(cnnhld);
								}
								else
									if (keyval == remmina_pref.shortcutkey_toolbar)
									{
										if (priv->view_mode == SCROLLED_WINDOW_MODE)
										{
											remmina_pref.hide_connection_toolbar =
													!remmina_pref.hide_connection_toolbar;
											remmina_connection_holder_showhide_toolbar(
													cnnhld, TRUE);
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
	/* Trap all key presses when hostkey is pressed */
	return TRUE;
}

static RemminaConnectionWindow* remmina_connection_window_find(RemminaFile* remminafile)
{
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
	RemminaConnectionWindow* cnnwin;
	RemminaConnectionHolder* cnnhld;
	GtkWidget* tab;
	gint i;

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
				
#if GTK_VERSION == 3		
		/* Reparent cnnobj->viewport */
		g_object_ref(cnnobj->viewport);
		gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(cnnobj->viewport)), cnnobj->viewport);
		gtk_container_add(GTK_CONTAINER(cnnobj->scrolled_container), cnnobj->viewport );
		g_object_unref(cnnobj->viewport);
#elif GTK_VERSION == 2
		gtk_widget_reparent(cnnobj->viewport, cnnobj->scrolled_container);
#endif

		gtk_window_present(GTK_WINDOW(cnnhld->cnnwin));
		gtk_notebook_set_current_page(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook), i);
	}

	if (cnnhld->cnnwin->priv->floating_toolbar)
	{
		gtk_widget_show(cnnhld->cnnwin->priv->floating_toolbar);
	}
}

static void remmina_connection_object_on_disconnect(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	RemminaConnectionHolder* cnnhld = cnnobj->cnnhld;
	GtkWidget* dialog;

	cnnobj->connected = FALSE;
	if (scale_option_window)
	{
		gtk_widget_destroy(scale_option_window);
		scale_option_window = NULL;
	}

	if (cnnhld && remmina_pref.save_view_mode)
	{
		if (cnnhld->cnnwin)
		{
			remmina_file_set_int(cnnobj->remmina_file, "viewmode", cnnhld->cnnwin->priv->view_mode);
		}
		if (remmina_pref.save_when_connect)
		{
			remmina_file_save_all(cnnobj->remmina_file);
		}
		else
		{
			remmina_file_save_group(cnnobj->remmina_file, REMMINA_SETTING_GROUP_RUNTIME);
		}
		if (cnnhld->cnnwin && remmina_pref.toolbar_pin_down != cnnhld->cnnwin->priv->pin_down)
		{
			remmina_pref.toolbar_pin_down = cnnhld->cnnwin->priv->pin_down;
			remmina_pref_save();
		}
	}
	remmina_file_free(cnnobj->remmina_file);

	if (remmina_protocol_widget_has_error(gp))
	{
		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
				remmina_protocol_widget_get_error_message(gp), NULL);
		g_signal_connect(G_OBJECT(dialog), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(dialog);
		remmina_widget_pool_register(dialog);
	}

	if (cnnobj->window)
	{
		gtk_widget_destroy(cnnobj->window);
		cnnobj->window = NULL;
	}

	if (cnnhld && cnnhld->cnnwin && cnnobj->scrolled_container)
	{
		gtk_notebook_remove_page(
				GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook),
				gtk_notebook_page_num(GTK_NOTEBOOK(cnnhld->cnnwin->priv->notebook),
						cnnobj->scrolled_container));
	}
	g_free(cnnobj);
}

static void remmina_connection_object_on_desktop_resize(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	if (cnnobj->cnnhld && cnnobj->cnnhld->cnnwin && cnnobj->cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
	{
		remmina_connection_holder_check_resize(cnnobj->cnnhld);
	}
}

static void remmina_connection_object_on_update_align(RemminaProtocolWidget* gp, RemminaConnectionObject* cnnobj)
{
	remmina_connection_holder_update_alignment(cnnobj->cnnhld);
}

gboolean remmina_connection_window_open_from_filename(const gchar* filename)
{
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
	remmina_connection_window_open_from_file_full(remminafile, NULL, NULL, NULL);
}

GtkWidget*
remmina_connection_window_open_from_file_full(RemminaFile* remminafile, GCallback disconnect_cb, gpointer data, guint* handler)
{
	RemminaConnectionObject* cnnobj;
	//GdkRGBA color;

	remmina_file_update_screen_resolution(remminafile);

	cnnobj = g_new0(RemminaConnectionObject, 1);
	cnnobj->remmina_file = remminafile;

	/* Create the RemminaProtocolWidget */
	cnnobj->proto = remmina_protocol_widget_new();

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

	/* Create the alignment to make the RemminaProtocolWidget centered */
	cnnobj->alignment = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_widget_show(cnnobj->alignment);
	gtk_container_set_border_width(GTK_CONTAINER(cnnobj->alignment), 0);
	gtk_container_add(GTK_CONTAINER(cnnobj->alignment), cnnobj->proto);
	g_signal_connect(G_OBJECT(cnnobj->alignment), "size-allocate",
			G_CALLBACK(remmina_connection_object_alignment_on_allocate), cnnobj);

	/* Create the viewport to make the RemminaProtocolWidget scrollable */
	cnnobj->viewport = gtk_viewport_new(NULL, NULL);
	gtk_widget_show(cnnobj->viewport);
	//gdk_color_parse("black", &color);
    GdkRGBA color = {.0, .0, .0, 1.0};
	gtk_widget_override_background_color(cnnobj->viewport, GTK_STATE_NORMAL, &color);
	gtk_container_set_border_width(GTK_CONTAINER(cnnobj->viewport), 0);
	gtk_viewport_set_shadow_type(GTK_VIEWPORT(cnnobj->viewport), GTK_SHADOW_NONE);
	gtk_container_add(GTK_CONTAINER(cnnobj->viewport), cnnobj->alignment);

	cnnobj->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_realize(cnnobj->window);
	gtk_container_add(GTK_CONTAINER(cnnobj->window), cnnobj->viewport);

	if (!remmina_pref.save_view_mode)
		remmina_file_set_int(cnnobj->remmina_file, "viewmode", remmina_pref.default_mode);

	remmina_protocol_widget_open_connection(REMMINA_PROTOCOL_WIDGET(cnnobj->proto), remminafile);

	return cnnobj->proto;
}

