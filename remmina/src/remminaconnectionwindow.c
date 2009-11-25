/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include "config.h"
#include "remminapublic.h"
#include "remminafile.h"
#include "remminainitdialog.h"
#include "remminaplug.h"
#include "remminaplug_rdp.h"
#include "remminaplug_vnc.h"
#include "remminaplug_xdmcp.h"
#include "remminapref.h"
#include "remminascrolledviewport.h"
#include "remminasftpwindow.h"
#include "remminascaler.h"
#include "remminawidgetpool.h"
#include "remminaconnectionwindow.h"

G_DEFINE_TYPE (RemminaConnectionWindow, remmina_connection_window, GTK_TYPE_WINDOW)

#define MOTION_TIME 100

/* One process can only have one scale option popup window at a time... */
static GtkWidget *scale_option_window = NULL;

typedef struct _RemminaConnectionHolder RemminaConnectionHolder;

struct _RemminaConnectionWindowPriv
{
    RemminaConnectionHolder *cnnhld;

    GtkWidget *notebook;

    GtkWidget *floating_toolbar;
    GtkWidget *floating_toolbar_label;
    /* To avoid strange event-loop */
    guint32 floating_toolbar_motion_time;

    GtkWidget *toolbar;

    /* Toolitems that need to be handled */
    GtkToolItem *toolitem_switch_page;
    GtkToolItem *toolitem_scale;
    GtkToolItem *toolitem_grab;
    GtkToolItem *toolitem_preferences;
    GtkToolItem *toolitem_tools;
    GtkWidget *scale_option_button;

    gboolean sticky;

    gint view_mode;
};

typedef struct _RemminaConnectionObject
{
    RemminaConnectionHolder *cnnhld;

    RemminaFile *remmina_file;

    /* A dummy window which will be realized as a container during initialize, before reparent to the real window */
    GtkWidget *window;

    /* Containers for RemminaPlug sub-classes: RemminaPlug->alignment->viewport...->window */
    GtkWidget *remmina_plug;
    GtkWidget *alignment;
    GtkWidget *viewport;

    /* Scrolled containers */
    GtkWidget *scrolled_container;

    gboolean connected;
} RemminaConnectionObject;

struct _RemminaConnectionHolder
{
    RemminaConnectionWindow *cnnwin;
};

#define DECLARE_CNNOBJ \
    if (!cnnhld->cnnwin || gtk_notebook_get_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)) < 0) return; \
    RemminaConnectionObject *cnnobj = (RemminaConnectionObject*) g_object_get_data ( \
        G_OBJECT (gtk_notebook_get_nth_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook), \
        gtk_notebook_get_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)))), "cnnobj");

#define DECLARE_CNNOBJ_WITH_RETURN(r) \
    if (!cnnhld->cnnwin || gtk_notebook_get_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)) < 0) return r; \
    RemminaConnectionObject *cnnobj = (RemminaConnectionObject*) g_object_get_data ( \
        G_OBJECT (gtk_notebook_get_nth_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook), \
        gtk_notebook_get_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)))), "cnnobj");

static void remmina_connection_holder_create_scrolled (RemminaConnectionHolder *cnnhld, RemminaConnectionObject *cnnobj);
static void remmina_connection_holder_create_fullscreen (RemminaConnectionHolder *cnnhld, RemminaConnectionObject *cnnobj, gint view_mode);

static void
remmina_connection_window_class_init (RemminaConnectionWindowClass *klass)
{
	gtk_rc_parse_string (
		"style \"remmina-small-button-style\"\n"
		"{\n"
		"  GtkWidget::focus-padding = 0\n"
		"  GtkWidget::focus-line-width = 0\n"
		"  xthickness = 0\n"
		"  ythickness = 0\n"
		"}\n"
		"widget \"*.remmina-small-button\" style \"remmina-small-button-style\"");
}

static void
remmina_connection_holder_disconnect (RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ

    /* Notify the RemminaPlug to disconnect, but not to close the window here.
       The window will be destroyed in RemminaPlug "disconnect" signal */
    remmina_plug_close_connection (REMMINA_PLUG (cnnobj->remmina_plug));
}

static void
remmina_connection_holder_keyboard_grab (RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ

    if (cnnobj->remmina_file->keyboard_grab)
    {
        gdk_keyboard_grab (gtk_widget_get_window (GTK_WIDGET (cnnhld->cnnwin)), TRUE, GDK_CURRENT_TIME);
    }
    else
    {
        gdk_keyboard_ungrab (GDK_CURRENT_TIME);
    }
}

static gboolean
remmina_connection_window_delete_event (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    RemminaConnectionHolder *cnnhld = (RemminaConnectionHolder*) data;
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    RemminaConnectionObject *cnnobj;
    GtkNotebook *notebook = GTK_NOTEBOOK (priv->notebook);
    GtkWidget *dialog;
    GtkWidget *w;
    gint i, n;

    n = gtk_notebook_get_n_pages (notebook);
    if (n > 1)
    {
        dialog = gtk_message_dialog_new (GTK_WINDOW (cnnhld->cnnwin),
            GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION, GTK_BUTTONS_YES_NO,
            _("There are %i active connections in the current window. Are you sure to close?"), n);
        i = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        if (i != GTK_RESPONSE_YES) return TRUE;
    }
    /* Just in case the connection already closed by the server before clicking yes */
    if (GTK_IS_WIDGET (notebook))
    {
        n = gtk_notebook_get_n_pages (notebook);
        for (i = n - 1; i >= 0; i--)
        {
            w = gtk_notebook_get_nth_page (notebook, i);
            cnnobj = (RemminaConnectionObject*) g_object_get_data (G_OBJECT (w), "cnnobj");
            remmina_plug_close_connection (REMMINA_PLUG (cnnobj->remmina_plug));
        }
    }
    return TRUE;
}

static void
remmina_connection_window_destroy (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    RemminaConnectionWindowPriv *priv = REMMINA_CONNECTION_WINDOW (widget)->priv;

    if (priv->floating_toolbar != NULL)
    {
        gtk_widget_destroy (priv->floating_toolbar);
        priv->floating_toolbar = NULL;
    }
    g_free (priv);
}

static void
remmina_connection_holder_floating_toolbar_hide (RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    GtkRequisition req;
    gint x;

    if (priv->floating_toolbar == NULL) return;

    gtk_widget_size_request (priv->floating_toolbar, &req);
    gtk_window_get_position (GTK_WINDOW (priv->floating_toolbar), &x, NULL);
    gtk_window_move (GTK_WINDOW (priv->floating_toolbar), x, 2 - req.height);
    if (remmina_pref.invisible_toolbar)
    {
        gtk_window_set_opacity (GTK_WINDOW (priv->floating_toolbar), 0.0);
    }
    priv->floating_toolbar_motion_time = gtk_get_current_event_time ();
}

static void
remmina_connection_holder_get_desktop_size (RemminaConnectionHolder* cnnhld, gint *width, gint *height, gboolean expend)
{
    DECLARE_CNNOBJ
    RemminaFile *gf = cnnobj->remmina_file;
    RemminaPlug *gp = REMMINA_PLUG (cnnobj->remmina_plug);
    gboolean scale;

    scale = gp->scale;
    *width = gp->width;
    if (scale)
    {
        if (gf->hscale > 0)
        {
            *width = (*width) * gf->hscale / 100;
        }
        else if (!expend)
        {
            *width = 1;
        }
    }
    *height = gp->height;
    if (scale)
    {
        if (gf->vscale > 0)
        {
            *height = (*height) * gf->vscale / 100;
        }
        else if (!expend)
        {
            *height = 1;
        }
    }
}

static gboolean
remmina_connection_holder_toolbar_autofit_restore (RemminaConnectionHolder* cnnhld)
{
    DECLARE_CNNOBJ_WITH_RETURN (FALSE)
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    gint width, height;

    if (cnnobj->connected && GTK_IS_SCROLLED_WINDOW (cnnobj->scrolled_container))
    {
        remmina_connection_holder_get_desktop_size (cnnhld, &width, &height, TRUE);
        gtk_window_resize (GTK_WINDOW (cnnhld->cnnwin), MAX (1, width),
            MAX (1, height + priv->toolbar->allocation.height));
        gtk_container_check_resize (GTK_CONTAINER (cnnhld->cnnwin));
    }
    if (GTK_IS_SCROLLED_WINDOW (cnnobj->scrolled_container))
    {
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (cnnobj->scrolled_container),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    }
    return FALSE;
}

static void
remmina_connection_holder_toolbar_autofit (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    DECLARE_CNNOBJ

    if (GTK_IS_SCROLLED_WINDOW (cnnobj->scrolled_container))
    {
        if ((gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (cnnhld->cnnwin))) & GDK_WINDOW_STATE_MAXIMIZED) != 0)
        {
            gtk_window_unmaximize (GTK_WINDOW (cnnhld->cnnwin));
        }

        /* It's tricky to make the toolbars disappear automatically, while keeping scrollable.
           Please tell me if you know a better way to do this */
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (cnnobj->scrolled_container),
            GTK_POLICY_NEVER, GTK_POLICY_NEVER);

        gtk_main_iteration ();

        g_timeout_add (200, (GSourceFunc) remmina_connection_holder_toolbar_autofit_restore, cnnhld);
    }

}

static void
remmina_connection_holder_check_resize (RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ
    gboolean scroll_required;
    GdkScreen *screen;
    gint screen_width, screen_height;
    gint server_width, server_height;

    remmina_connection_holder_get_desktop_size (cnnhld, &server_width, &server_height, FALSE);
    screen = gdk_screen_get_default ();
    screen_width = gdk_screen_get_width (screen);
    screen_height = gdk_screen_get_height (screen);

    if (server_width <= 0 || server_height <= 0 || screen_width < server_width || screen_height < server_height)
    {
        scroll_required = TRUE;
    }
    else
    {
        scroll_required = FALSE;
    }

    switch (cnnhld->cnnwin->priv->view_mode)
    {

    case SCROLLED_FULLSCREEN_MODE:
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (cnnobj->scrolled_container),
            (scroll_required ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER),
            (scroll_required ? GTK_POLICY_AUTOMATIC : GTK_POLICY_NEVER));
        break;

    case VIEWPORT_FULLSCREEN_MODE:
        gtk_container_set_border_width (GTK_CONTAINER (cnnhld->cnnwin), scroll_required ? 1 : 0);
        break;

    case SCROLLED_WINDOW_MODE:
        if (cnnobj->remmina_file->viewmode == AUTO_MODE)
        {
            gtk_window_set_default_size (GTK_WINDOW (cnnhld->cnnwin),
                MIN (server_width, screen_width), MIN (server_height, screen_height));
            if (server_width >= screen_width ||
                server_height >= screen_height)
            {
                gtk_window_maximize (GTK_WINDOW (cnnhld->cnnwin));
                cnnobj->remmina_file->window_maximize = TRUE;
            }
            else
            {
                remmina_connection_holder_toolbar_autofit (NULL, cnnhld);
                cnnobj->remmina_file->window_maximize = FALSE;
            }
        }
        else
        {
            gtk_window_set_default_size (GTK_WINDOW (cnnhld->cnnwin),
                cnnobj->remmina_file->window_width, cnnobj->remmina_file->window_height);
            if (cnnobj->remmina_file->window_maximize)
            {
                gtk_window_maximize (GTK_WINDOW (cnnhld->cnnwin));
            }
        }
        break;

    default:
        break;
    }
}

static void
remmina_connection_holder_toolbar_scrolled (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    remmina_connection_holder_create_scrolled (cnnhld, NULL);
}

static void
remmina_connection_holder_toolbar_scrolled_fullscreen (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    remmina_connection_holder_create_fullscreen (cnnhld, NULL, SCROLLED_FULLSCREEN_MODE);
}

static void
remmina_connection_holder_toolbar_viewport_fullscreen (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    remmina_connection_holder_create_fullscreen (cnnhld, NULL, VIEWPORT_FULLSCREEN_MODE);
}

static void
remmina_connection_holder_update_alignment (RemminaConnectionHolder* cnnhld)
{
    DECLARE_CNNOBJ
    RemminaPlug *gp = REMMINA_PLUG (cnnobj->remmina_plug);
    RemminaFile *gf = cnnobj->remmina_file;
    gboolean scale;
    gint width, height;

    scale = REMMINA_PLUG (cnnobj->remmina_plug)->scale;
    if (scale && gf->aspectscale && gf->hscale == 0)
    {
        width = cnnobj->alignment->allocation.width;
        height = cnnobj->alignment->allocation.height;
        if (width > 1 && height > 1)
        {
            if (gp->width * height >= width * gp->height)
            {
                gtk_alignment_set (GTK_ALIGNMENT (cnnobj->alignment), 0.5, 0.5,
                    1.0,
                    (gfloat) (gp->height * width) / (gfloat) gp->width / (gfloat) height);
            }
            else
            {
                gtk_alignment_set (GTK_ALIGNMENT (cnnobj->alignment), 0.5, 0.5,
                    (gfloat) (gp->width * height) / (gfloat) gp->height / (gfloat) width,
                    1.0);
            }
        }
    }
    else
    {
        gtk_alignment_set (GTK_ALIGNMENT (cnnobj->alignment), 0.5, 0.5,
            ((scale && gf->hscale == 0) ? 1.0 : 0.0),
            ((scale && gf->vscale == 0) ? 1.0 : 0.0));
    }
}

static void
remmina_connection_holder_switch_page_activate (GtkMenuItem *menuitem, RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    gint page_num;

    page_num = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "new-page-num"));
    gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), page_num);
}

static void
remmina_connection_holder_toolbar_switch_page_popdown (GtkWidget *widget, RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    priv->sticky = FALSE;

    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (priv->toolitem_switch_page), FALSE);
    remmina_connection_holder_floating_toolbar_hide (cnnhld);
}

static void
remmina_connection_holder_toolbar_switch_page (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    RemminaConnectionObject *cnnobj;
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    GtkWidget *menu;
    GtkWidget *menuitem;
    GtkWidget *image;
    GtkWidget *page;
    gint i, n;

    if (!gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget))) return;

    priv->sticky = TRUE;

    menu = gtk_menu_new ();

    n = gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook));
    for (i = 0; i < n; i++)
    {
        page = gtk_notebook_get_nth_page (GTK_NOTEBOOK (priv->notebook), i);
        if (!page) break;
        cnnobj = (RemminaConnectionObject*) g_object_get_data (G_OBJECT (page), "cnnobj");

        menuitem = gtk_image_menu_item_new_with_label (cnnobj->remmina_file->name);
        gtk_widget_show (menuitem);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

        image = gtk_image_new_from_icon_name (remmina_file_get_icon_name (cnnobj->remmina_file), GTK_ICON_SIZE_MENU);
        gtk_widget_show (image);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);

        g_object_set_data (G_OBJECT (menuitem), "new-page-num", GINT_TO_POINTER (i));
        g_signal_connect (G_OBJECT (menuitem), "activate",
            G_CALLBACK (remmina_connection_holder_switch_page_activate), cnnhld);
        if (i == gtk_notebook_get_current_page (GTK_NOTEBOOK (priv->notebook)))
        {
            gtk_widget_set_sensitive (menuitem, FALSE);
        }
    }

    g_signal_connect (G_OBJECT (menu), "deactivate",
        G_CALLBACK (remmina_connection_holder_toolbar_switch_page_popdown), cnnhld);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
        remmina_public_popup_position, widget,
        0, gtk_get_current_event_time());
}

static void
remmina_connection_holder_toolbar_scaled_mode (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    DECLARE_CNNOBJ
    gboolean scale;

    remmina_connection_holder_update_alignment (cnnhld);

    scale = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget));
    gtk_widget_set_sensitive (GTK_WIDGET (cnnhld->cnnwin->priv->scale_option_button), scale);
    if (remmina_pref.save_view_mode) cnnobj->remmina_file->scale = scale;

    remmina_plug_call_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_SCALE,
        (scale ? GINT_TO_POINTER (1) : NULL));
    if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
    {
        remmina_connection_holder_check_resize (cnnhld);
    }
}

static void
remmina_connection_holder_scale_option_on_scaled (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    DECLARE_CNNOBJ

    cnnobj->remmina_file->hscale = REMMINA_SCALER (widget)->hscale;
    cnnobj->remmina_file->vscale = REMMINA_SCALER (widget)->vscale;
    cnnobj->remmina_file->aspectscale = REMMINA_SCALER (widget)->aspectscale;
    remmina_connection_holder_update_alignment (cnnhld);
    remmina_plug_call_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_SCALE,
        GINT_TO_POINTER (1));
    if (cnnhld->cnnwin->priv->view_mode != SCROLLED_WINDOW_MODE)
    {
        remmina_connection_holder_check_resize (cnnhld);
    }
}

static void
remmina_connection_holder_scale_option_popdown (RemminaConnectionHolder* cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    priv->sticky = FALSE;

    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->scale_option_button), FALSE);

    gdk_keyboard_ungrab (GDK_CURRENT_TIME);
    gdk_pointer_ungrab (GDK_CURRENT_TIME);
    if (scale_option_window)
    {
        gtk_grab_remove (scale_option_window);
        gtk_widget_destroy (scale_option_window);
        scale_option_window = NULL;
    }
    remmina_connection_holder_floating_toolbar_hide (cnnhld);
}

static gboolean
remmina_connection_holder_trap_on_button (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    return TRUE;
}

static gboolean
remmina_connection_holder_scale_option_on_key (GtkWidget *widget, GdkEventKey *event, RemminaConnectionHolder* cnnhld)
{
    switch (event->keyval)
    {
    case GDK_Escape:
        remmina_connection_holder_scale_option_popdown (cnnhld);
        return TRUE;
    }
    return FALSE;
}

static gboolean
remmina_connection_holder_scale_option_on_button (GtkWidget *widget, GdkEventButton *event, RemminaConnectionHolder* cnnhld)
{
    remmina_connection_holder_scale_option_popdown (cnnhld);
    return TRUE;
}

static void
remmina_connection_holder_toolbar_scale_option (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    DECLARE_CNNOBJ
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    RemminaFile *gf = cnnobj->remmina_file;
    GtkWidget *window;
    GtkWidget *eventbox;
    GtkWidget *frame;
    GtkWidget *scaler;
    gint x, y;
    gboolean pushin;

    if (scale_option_window)
    {
        remmina_connection_holder_scale_option_popdown (cnnhld);
    }
    else if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)))
    {
        window = gtk_window_new (GTK_WINDOW_POPUP);
        gtk_container_set_border_width (GTK_CONTAINER (window), 0);

        /* Use an event-box to trap all button clicks events before sending up to the window */
        eventbox = gtk_event_box_new ();
        gtk_widget_show (eventbox);
        gtk_container_add (GTK_CONTAINER (window), eventbox);
        gtk_widget_add_events (eventbox, GDK_BUTTON_PRESS_MASK);
        g_signal_connect (G_OBJECT (eventbox), "button-press-event",
            G_CALLBACK (remmina_connection_holder_trap_on_button), NULL);

        frame = gtk_frame_new (NULL);
        gtk_widget_show (frame);
        gtk_container_set_border_width (GTK_CONTAINER (frame), 0);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
        gtk_container_add (GTK_CONTAINER (eventbox), frame);

        scaler = remmina_scaler_new ();
        gtk_widget_show (scaler);
        gtk_widget_set_size_request (scaler, 250, -1);
        gtk_container_set_border_width (GTK_CONTAINER (scaler), 4);
        remmina_scaler_set (REMMINA_SCALER (scaler), gf->hscale, gf->vscale, gf->aspectscale);
        remmina_scaler_set_draw_value (REMMINA_SCALER (scaler), FALSE);
        gtk_container_add (GTK_CONTAINER (frame), scaler);
        g_signal_connect (G_OBJECT (scaler), "scaled",
            G_CALLBACK (remmina_connection_holder_scale_option_on_scaled), cnnhld);

        g_signal_connect (G_OBJECT (window), "key-press-event",
            G_CALLBACK (remmina_connection_holder_scale_option_on_key), cnnhld);
        g_signal_connect (G_OBJECT (window), "button-press-event",
            G_CALLBACK (remmina_connection_holder_scale_option_on_button), cnnhld);

        gtk_widget_realize (window);
        remmina_public_popup_position (NULL, &x, &y, &pushin, priv->toolitem_scale);
        gtk_window_move (GTK_WINDOW (window), x, y);
        gtk_widget_show (window);

        gtk_grab_add (window);
        gdk_pointer_grab (window->window, TRUE,
            GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK,
            NULL, NULL, GDK_CURRENT_TIME);
        gdk_keyboard_grab (window->window, TRUE, GDK_CURRENT_TIME);

        scale_option_window = window;
        priv->sticky = TRUE;
    }
}

static void
remmina_connection_holder_toolbar_preferences_popdown (GtkWidget *widget, RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    priv->sticky = FALSE;

    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (priv->toolitem_preferences), FALSE);
    remmina_connection_holder_floating_toolbar_hide (cnnhld);
}

static void
remmina_connection_holder_toolbar_tools_popdown (GtkWidget *widget, RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    priv->sticky = FALSE;

    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (priv->toolitem_tools), FALSE);
    remmina_connection_holder_floating_toolbar_hide (cnnhld);
}

static void
remmina_connection_holder_call_plug_feature_radio (GtkMenuItem *menuitem, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ
    RemminaPlugFeature type;
    gpointer value;

    if (gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem)))
    {
        type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "feature-type"));
        value = g_object_get_data (G_OBJECT (menuitem), "feature-value");

        remmina_plug_call_feature (REMMINA_PLUG (cnnobj->remmina_plug), type, value);

        if (remmina_pref.save_view_mode)
        {
            switch (type)
            {
                case REMMINA_PLUG_FEATURE_PREF_QUALITY:
                    cnnobj->remmina_file->quality = GPOINTER_TO_INT (value);
                    break;
                default:
                    break;
            }
        }
    }
}

static void
remmina_connection_holder_call_plug_feature_check (GtkMenuItem *menuitem, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ
    RemminaPlugFeature type;
    gboolean value;

    type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "feature-type"));
    value = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (menuitem));
    remmina_plug_call_feature (REMMINA_PLUG (cnnobj->remmina_plug), type, (value ? GINT_TO_POINTER (1) : NULL));
}

static void
remmina_connection_holder_call_plug_feature_activate (GtkMenuItem *menuitem, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ
    RemminaPlugFeature type;

    type = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (menuitem), "feature-type"));
    remmina_plug_call_feature (REMMINA_PLUG (cnnobj->remmina_plug), type, NULL);
}

static void
remmina_connection_holder_toolbar_preferences (GtkWidget *widget, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    GtkWidget *menu;
    GtkWidget *menuitem;
    GSList *group;
    gint i;
    gboolean firstgroup;

    if (!gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget))) return;

    priv->sticky = TRUE;

    firstgroup = TRUE;

    menu = gtk_menu_new ();

    if (remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_PREF_QUALITY))
    {
        group = NULL;
        for (i = 0; quality_list[i]; i += 2)
        {
            menuitem = gtk_radio_menu_item_new_with_label (group, _(quality_list[i + 1]));
            group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (menuitem));
            gtk_widget_show (menuitem);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

            g_object_set_data (G_OBJECT (menuitem), "feature-type", GINT_TO_POINTER (REMMINA_PLUG_FEATURE_PREF_QUALITY));
            g_object_set_data (G_OBJECT (menuitem), "feature-value", GINT_TO_POINTER (atoi (quality_list[i])));

            if (atoi (quality_list[i]) == cnnobj->remmina_file->quality)
            {
                gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), TRUE);
            }

            g_signal_connect (G_OBJECT (menuitem), "toggled",
                G_CALLBACK (remmina_connection_holder_call_plug_feature_radio), cnnhld);
        }
        firstgroup = FALSE;
    }

    if (remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_PREF_VIEWONLY) ||
        remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_PREF_DISABLESERVERINPUT))
    {
        if (!firstgroup)
        {
            menuitem = gtk_separator_menu_item_new ();
            gtk_widget_show (menuitem);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);
        }

        if (remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_PREF_VIEWONLY))
        {
            menuitem = gtk_check_menu_item_new_with_label (_("View Only"));
            gtk_widget_show (menuitem);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

            g_object_set_data (G_OBJECT (menuitem), "feature-type", GINT_TO_POINTER (REMMINA_PLUG_FEATURE_PREF_VIEWONLY));

            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), cnnobj->remmina_file->viewonly);

            g_signal_connect (G_OBJECT (menuitem), "toggled",
                G_CALLBACK (remmina_connection_holder_call_plug_feature_check), cnnhld);
        }

        if (remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_PREF_DISABLESERVERINPUT))
        {
            menuitem = gtk_check_menu_item_new_with_label (_("Disable Server Input"));
            gtk_widget_show (menuitem);
            gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

            g_object_set_data (G_OBJECT (menuitem), "feature-type",
                GINT_TO_POINTER (REMMINA_PLUG_FEATURE_PREF_DISABLESERVERINPUT));

            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menuitem), cnnobj->remmina_file->disableserverinput);

            g_signal_connect (G_OBJECT (menuitem), "toggled",
                G_CALLBACK (remmina_connection_holder_call_plug_feature_check), cnnhld);
        }

        firstgroup = FALSE;
    }

    g_signal_connect (G_OBJECT (menu), "deactivate",
        G_CALLBACK (remmina_connection_holder_toolbar_preferences_popdown), cnnhld);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
        remmina_public_popup_position, widget,
        0, gtk_get_current_event_time());
}

static void
remmina_connection_holder_toolbar_tools (GtkWidget *widget, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    GtkWidget *menu;
    GtkWidget *menuitem;
    GtkWidget *image;

    if (!gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget))) return;

    priv->sticky = TRUE;

    menu = gtk_menu_new ();

    /* Chat */
    menuitem = gtk_image_menu_item_new_with_label (_("Open Chat..."));
    gtk_widget_show (menuitem);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

    image = gtk_image_new_from_icon_name ("face-smile", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);

    g_object_set_data (G_OBJECT (menuitem), "feature-type", GINT_TO_POINTER (REMMINA_PLUG_FEATURE_TOOL_CHAT));

    g_signal_connect (G_OBJECT (menuitem), "activate",
        G_CALLBACK (remmina_connection_holder_call_plug_feature_activate), cnnhld);
    if (!remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_TOOL_CHAT))
    {
        gtk_widget_set_sensitive (menuitem, FALSE);
    }

    /* SFTP */
    menuitem = gtk_image_menu_item_new_with_label (_("Open Secure File Transfer..."));
    gtk_widget_show (menuitem);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

    image = gtk_image_new_from_icon_name ("folder-remote", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);

    g_object_set_data (G_OBJECT (menuitem), "feature-type", GINT_TO_POINTER (REMMINA_PLUG_FEATURE_TOOL_SFTP));

    g_signal_connect (G_OBJECT (menuitem), "activate",
        G_CALLBACK (remmina_connection_holder_call_plug_feature_activate), cnnhld);
    if (!remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_TOOL_SFTP))
    {
        gtk_widget_set_sensitive (menuitem, FALSE);
    }

    /* SSH Terminal */
    menuitem = gtk_image_menu_item_new_with_label (_("Open Secure Shell in New Terminal..."));
    gtk_widget_show (menuitem);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menuitem);

    image = gtk_image_new_from_icon_name ("utilities-terminal", GTK_ICON_SIZE_MENU);
    gtk_widget_show (image);
    gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (menuitem), image);

    g_object_set_data (G_OBJECT (menuitem), "feature-type", GINT_TO_POINTER (REMMINA_PLUG_FEATURE_TOOL_SSHTERM));

    g_signal_connect (G_OBJECT (menuitem), "activate",
        G_CALLBACK (remmina_connection_holder_call_plug_feature_activate), cnnhld);
    if (!remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_TOOL_SSHTERM))
    {
        gtk_widget_set_sensitive (menuitem, FALSE);
    }

    g_signal_connect (G_OBJECT (menu), "deactivate",
        G_CALLBACK (remmina_connection_holder_toolbar_tools_popdown), cnnhld);

    gtk_menu_popup (GTK_MENU (menu), NULL, NULL,
        remmina_public_popup_position, widget,
        0, gtk_get_current_event_time());
}

static void
remmina_connection_holder_toolbar_minimize (GtkWidget *widget, RemminaConnectionHolder *cnnhld)
{
    remmina_connection_holder_floating_toolbar_hide (cnnhld);
    gtk_window_iconify (GTK_WINDOW (cnnhld->cnnwin));
}

static void
remmina_connection_holder_toolbar_disconnect (GtkWidget *widget, RemminaConnectionHolder* cnnhld)
{
    remmina_connection_holder_disconnect (cnnhld);
}

static void
remmina_connection_holder_toolbar_grab (GtkWidget *widget, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ

    cnnobj->remmina_file->keyboard_grab = gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (widget));
    remmina_connection_holder_keyboard_grab (cnnhld);
}

static GtkWidget*
remmina_connection_holder_create_toolbar (RemminaConnectionHolder *cnnhld, gint mode)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    GtkWidget *toolbar;
    GtkToolItem *toolitem;
    GtkWidget *widget;
    GtkWidget *arrow;

    toolbar = gtk_toolbar_new ();
    gtk_widget_show (toolbar);
    gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
    gtk_toolbar_set_show_arrow (GTK_TOOLBAR (toolbar), FALSE);
    if (remmina_pref.small_toolbutton)
    {
        gtk_toolbar_set_icon_size (GTK_TOOLBAR (toolbar), GTK_ICON_SIZE_MENU);
    }

    if (mode == SCROLLED_WINDOW_MODE)
    {
        toolitem = gtk_tool_button_new (NULL, _("Auto-Fit Window"));
        gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "remmina-fit-window");
        gtk_tool_item_set_tooltip_text (toolitem, _("Resize the window to fit in remote resolution"));
        g_signal_connect (G_OBJECT (toolitem), "clicked",
            G_CALLBACK(remmina_connection_holder_toolbar_autofit), cnnhld);
    }
    else
    {
        toolitem = gtk_tool_button_new (NULL, _("Scrolled Window"));
        gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "remmina-leave-fullscreen");
        gtk_tool_item_set_tooltip_text (toolitem, _("Toggle scrolled window mode"));
        g_signal_connect (G_OBJECT (toolitem), "clicked",
            G_CALLBACK(remmina_connection_holder_toolbar_scrolled), cnnhld);
    }
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));

    toolitem = gtk_tool_button_new (NULL, _("Scrolled Fullscreen"));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "remmina-scrolled-fullscreen");
    gtk_tool_item_set_tooltip_text (toolitem, _("Toggle scrolled fullscreen mode"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "clicked",
        G_CALLBACK(remmina_connection_holder_toolbar_scrolled_fullscreen), cnnhld);
    if (mode == FULLSCREEN_MODE || mode == SCROLLED_FULLSCREEN_MODE)
    {
        gtk_widget_set_sensitive (GTK_WIDGET (toolitem), FALSE);
    }

    toolitem = gtk_tool_button_new (NULL, _("Viewport Fullscreen"));
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "remmina-viewport-fullscreen");
    gtk_tool_item_set_tooltip_text (toolitem, _("Toggle viewport fullscreen mode"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "clicked",
        G_CALLBACK(remmina_connection_holder_toolbar_viewport_fullscreen), cnnhld);
    if (mode == FULLSCREEN_MODE || mode == VIEWPORT_FULLSCREEN_MODE)
    {
        gtk_widget_set_sensitive (GTK_WIDGET (toolitem), FALSE);
    }

    toolitem = gtk_toggle_tool_button_new ();
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "remmina-switch-page");
    gtk_tool_item_set_tooltip_text (toolitem, _("Switch tab pages"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "toggled",
        G_CALLBACK(remmina_connection_holder_toolbar_switch_page), cnnhld);
    priv->toolitem_switch_page = toolitem;

    toolitem = gtk_separator_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));

    toolitem = gtk_toggle_tool_button_new ();
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "remmina-scale");
    gtk_tool_item_set_tooltip_text (toolitem, _("Toggle scaled mode"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "toggled",
        G_CALLBACK(remmina_connection_holder_toolbar_scaled_mode), cnnhld);
    priv->toolitem_scale = toolitem;

    /* We need a toggle tool button with a popup arrow; and the popup is a window not a menu.
       GTK+ support neither of them. We need some tricks here... */
    toolitem = gtk_tool_item_new ();
    gtk_widget_show (GTK_WIDGET (toolitem));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);

    widget = gtk_toggle_button_new ();
    gtk_widget_show (widget);
    gtk_container_set_border_width (GTK_CONTAINER (widget), 0);
    gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);
    gtk_button_set_focus_on_click (GTK_BUTTON (widget), FALSE);
    if (remmina_pref.small_toolbutton)
    {
        gtk_widget_set_name (widget, "remmina-small-button");
    }
    gtk_container_add (GTK_CONTAINER (toolitem), widget);

    arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_NONE);
    gtk_widget_show (arrow);
    gtk_container_add (GTK_CONTAINER (widget), arrow);

    g_signal_connect (G_OBJECT (widget), "toggled",
        G_CALLBACK(remmina_connection_holder_toolbar_scale_option), cnnhld);
    priv->scale_option_button = widget;

    toolitem = gtk_toggle_tool_button_new ();
    gtk_tool_button_set_icon_name (GTK_TOOL_BUTTON (toolitem), "input-keyboard");
    gtk_tool_item_set_tooltip_text (toolitem, _("Grab all keyboard events"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "toggled",
        G_CALLBACK(remmina_connection_holder_toolbar_grab), cnnhld);
    priv->toolitem_grab = toolitem;

    toolitem = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_PREFERENCES);
    gtk_tool_item_set_tooltip_text (toolitem, _("Preferences"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "toggled",
        G_CALLBACK(remmina_connection_holder_toolbar_preferences), cnnhld);
    priv->toolitem_preferences = toolitem;

    toolitem = gtk_toggle_tool_button_new_from_stock (GTK_STOCK_EXECUTE);
    gtk_tool_button_set_label (GTK_TOOL_BUTTON (toolitem), _("Tools"));
    gtk_tool_item_set_tooltip_text (toolitem, _("Tools"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "toggled",
        G_CALLBACK(remmina_connection_holder_toolbar_tools), cnnhld);
    priv->toolitem_tools = toolitem;

    toolitem = gtk_separator_tool_item_new ();
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));

    toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_GOTO_BOTTOM);
    gtk_tool_item_set_tooltip_text (toolitem, _("Minimize Window"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "clicked",
        G_CALLBACK(remmina_connection_holder_toolbar_minimize), cnnhld);

    toolitem = gtk_tool_button_new_from_stock (GTK_STOCK_DISCONNECT);
    gtk_tool_item_set_tooltip_text (toolitem, _("Disconnect"));
    gtk_toolbar_insert (GTK_TOOLBAR (toolbar), toolitem, -1);
    gtk_widget_show (GTK_WIDGET (toolitem));
    g_signal_connect (G_OBJECT (toolitem), "clicked",
        G_CALLBACK(remmina_connection_holder_toolbar_disconnect), cnnhld);

    return toolbar;
}

static void
remmina_connection_holder_update_toolbar (RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    GtkToolItem *toolitem;
    gboolean bval;

    toolitem = priv->toolitem_switch_page;
    bval = (gtk_notebook_get_n_pages (GTK_NOTEBOOK (priv->notebook)) > 1);
    gtk_widget_set_sensitive (GTK_WIDGET (toolitem), bval);

    toolitem = priv->toolitem_scale;
    bval = REMMINA_PLUG (cnnobj->remmina_plug)->scale;
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (toolitem), bval);
    gtk_widget_set_sensitive (GTK_WIDGET (priv->scale_option_button), bval);
    bval = (remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_SCALE) != NULL);
    gtk_widget_set_sensitive (GTK_WIDGET (toolitem), bval);

    toolitem = priv->toolitem_grab;
    gtk_toggle_tool_button_set_active (GTK_TOGGLE_TOOL_BUTTON (toolitem), cnnobj->remmina_file->keyboard_grab);

    toolitem = priv->toolitem_preferences;
    bval = (remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_PREF) != NULL);
    gtk_widget_set_sensitive (GTK_WIDGET (toolitem), bval);

    toolitem = priv->toolitem_tools;
    bval = (remmina_plug_query_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_TOOL) != NULL);
    gtk_widget_set_sensitive (GTK_WIDGET (toolitem), bval);

    gtk_window_set_title (GTK_WINDOW (cnnhld->cnnwin), cnnobj->remmina_file->name);

    if (priv->floating_toolbar)
    {
        gtk_label_set_text (GTK_LABEL (priv->floating_toolbar_label), cnnobj->remmina_file->name);
    }
}

static void
remmina_connection_holder_update_toolbar_opacity (RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    gtk_window_set_opacity (GTK_WINDOW (priv->floating_toolbar),
        (1.0 - TOOLBAR_OPACITY_MIN) / ((gdouble) TOOLBAR_OPACITY_LEVEL)
        * ((gdouble) (TOOLBAR_OPACITY_LEVEL - cnnobj->remmina_file->toolbar_opacity))
        + TOOLBAR_OPACITY_MIN);
}

static gboolean
remmina_connection_holder_toolbar_enter (
    GtkWidget *widget,
    GdkEventCrossing *event,
    RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    gint x;

    /* I hardcoded a 100ms timeout here, so that the toolbar won't show up immediately after it hides */
    if (gtk_get_current_event_time () - priv->floating_toolbar_motion_time < MOTION_TIME) return TRUE;

    gtk_window_get_position (GTK_WINDOW (priv->floating_toolbar), &x, NULL);
    gtk_window_move (GTK_WINDOW (priv->floating_toolbar), x, 0);
    remmina_connection_holder_update_toolbar_opacity (cnnhld);
    return TRUE;
}

static gboolean
remmina_connection_holder_toolbar_leave (
    GtkWidget *widget,
    GdkEventCrossing *event,
    RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    if (!priv->sticky && event->mode == GDK_CROSSING_NORMAL)
    {
        remmina_connection_holder_floating_toolbar_hide (cnnhld);
        return TRUE;
    }
    return FALSE;
}

static gboolean
remmina_connection_window_focus_in (GtkWidget *widget, GdkEventFocus *event, RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    if (priv->floating_toolbar)
    {
        gtk_widget_show (priv->floating_toolbar);
    }
    return FALSE;
}

static gboolean
remmina_connection_window_focus_out (GtkWidget *widget, GdkEventFocus *event, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ_WITH_RETURN (FALSE)
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    if (!priv->sticky && priv->floating_toolbar)
    {
        gtk_widget_hide (priv->floating_toolbar);
    }
    if (REMMINA_IS_SCROLLED_VIEWPORT (cnnobj->scrolled_container))
    {
        remmina_scrolled_viewport_remove_motion (REMMINA_SCROLLED_VIEWPORT (cnnobj->scrolled_container));
    }
    remmina_plug_call_feature (REMMINA_PLUG (cnnobj->remmina_plug), REMMINA_PLUG_FEATURE_UNFOCUS, NULL);
    return FALSE;
}

static gboolean
remmina_connection_window_on_enter (
    GtkWidget *widget,
    GdkEventCrossing *event,
    RemminaConnectionHolder *cnnhld)
{
    if (event->detail == GDK_NOTIFY_VIRTUAL ||
        event->detail == GDK_NOTIFY_NONLINEAR ||
        event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL)
    {
        remmina_connection_holder_keyboard_grab (cnnhld);
    }
    return FALSE;
}

static gboolean
remmina_connection_window_on_leave (
    GtkWidget *widget,
    GdkEventCrossing *event,
    RemminaConnectionHolder *cnnhld)
{
    if (event->detail == GDK_NOTIFY_VIRTUAL ||
        event->detail == GDK_NOTIFY_NONLINEAR ||
        event->detail == GDK_NOTIFY_NONLINEAR_VIRTUAL)
    {
        gdk_keyboard_ungrab (GDK_CURRENT_TIME);
    }
    return FALSE;
}

static gboolean
remmina_connection_holder_toolbar_scroll (GtkWidget *widget, GdkEventScroll *event, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ_WITH_RETURN (FALSE)

    switch (event->direction)
    {
    case GDK_SCROLL_UP:
        if (cnnobj->remmina_file->toolbar_opacity > 0)
        {
            cnnobj->remmina_file->toolbar_opacity--;
            remmina_connection_holder_update_toolbar_opacity (cnnhld);
            return TRUE;
        }
        break;
    case GDK_SCROLL_DOWN:
        if (cnnobj->remmina_file->toolbar_opacity < TOOLBAR_OPACITY_LEVEL)
        {
            cnnobj->remmina_file->toolbar_opacity++;
            remmina_connection_holder_update_toolbar_opacity (cnnhld);
            return TRUE;
        }
        break;
    default:
        break;
    }
    return FALSE;
}

static void
remmina_connection_object_alignment_on_allocate (GtkWidget *widget, GtkAllocation *allocation, RemminaConnectionObject *cnnobj)
{
    remmina_connection_holder_update_alignment (cnnobj->cnnhld);
}

static gboolean
remmina_connection_window_on_configure (GtkWidget *widget, GdkEventConfigure *event, RemminaConnectionHolder *cnnhld)
{
    DECLARE_CNNOBJ_WITH_RETURN (FALSE)
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    GtkRequisition req;
    GdkScreen *screen;

    if (cnnhld->cnnwin && gtk_widget_get_window (GTK_WIDGET (cnnhld->cnnwin)) &&
        cnnhld->cnnwin->priv->view_mode == SCROLLED_WINDOW_MODE)
    {
        /* Here we store the window state in real-time */
        if ((gdk_window_get_state (gtk_widget_get_window (GTK_WIDGET (cnnhld->cnnwin))) & GDK_WINDOW_STATE_MAXIMIZED) == 0)
        {
            gtk_window_get_size (GTK_WINDOW (cnnhld->cnnwin),
                &cnnobj->remmina_file->window_width, &cnnobj->remmina_file->window_height);
            cnnobj->remmina_file->window_maximize = FALSE;
        }
        else
        {
            cnnobj->remmina_file->window_maximize = TRUE;
        }
    }

    if (priv->floating_toolbar)
    {
        screen = gdk_screen_get_default ();
        gtk_widget_size_request (priv->floating_toolbar, &req);
        gtk_window_move (GTK_WINDOW (priv->floating_toolbar),
            (gdk_screen_get_width (screen) - req.width) / 2, 2 - req.height);
    }
    
    if (REMMINA_IS_SCROLLED_VIEWPORT (cnnobj->scrolled_container))
    {
        /* Notify window of change so that scroll border can be hidden or shown if needed */
        remmina_plug_emit_signal (REMMINA_PLUG (cnnobj->remmina_plug), "desktop-resize");
    }
    return FALSE;
}

static void
remmina_connection_holder_create_floating_toolbar (RemminaConnectionHolder *cnnhld, gint mode)
{
    DECLARE_CNNOBJ
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;
    GtkWidget *window;
    GtkWidget *vbox;
    GtkWidget *widget;
    GtkWidget *eventbox;
    GtkRequisition req;
    GdkScreen *screen;

    screen = gdk_screen_get_default ();

    /* This has to be a popup window to become visible in fullscreen mode */
    window = gtk_window_new (GTK_WINDOW_POPUP);

    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    widget = remmina_connection_holder_create_toolbar (cnnhld, mode);
    gtk_box_pack_start (GTK_BOX (vbox), widget, FALSE, FALSE, 0);

    /* An event box is required to wrap the label to avoid infinite "leave-enter" event loop */
    eventbox = gtk_event_box_new ();
    gtk_widget_show (eventbox);
    gtk_box_pack_start (GTK_BOX (vbox), eventbox, FALSE, FALSE, 0);
    widget = gtk_label_new (cnnobj->remmina_file->name);
    gtk_label_set_max_width_chars (GTK_LABEL (widget), 50);
    gtk_widget_show (widget);
    gtk_container_add (GTK_CONTAINER (eventbox), widget);
    priv->floating_toolbar_label = widget;

    gtk_widget_size_request (window, &req);
    gtk_window_move (GTK_WINDOW (window), (gdk_screen_get_width (screen) - req.width) / 2, 2 - req.height);
    if (remmina_pref.invisible_toolbar)
    {
        gtk_window_set_opacity (GTK_WINDOW (window), 0.0);
    }

    priv->floating_toolbar = window;

    g_signal_connect (G_OBJECT (window), "enter-notify-event", 
        G_CALLBACK (remmina_connection_holder_toolbar_enter), cnnhld);
    g_signal_connect (G_OBJECT (window), "leave-notify-event", 
        G_CALLBACK (remmina_connection_holder_toolbar_leave), cnnhld);
    g_signal_connect (G_OBJECT (window), "scroll-event", 
        G_CALLBACK (remmina_connection_holder_toolbar_scroll), cnnhld);

    if (cnnobj->connected) gtk_widget_show (window);
}

static void
remmina_connection_window_init (RemminaConnectionWindow *cnnwin)
{
    RemminaConnectionWindowPriv *priv;

    priv = g_new (RemminaConnectionWindowPriv, 1);
    cnnwin->priv = priv;

    priv->floating_toolbar = NULL;
    priv->floating_toolbar_motion_time = 0;
    priv->sticky = FALSE;
    priv->toolbar = NULL;
    priv->notebook = NULL;
    priv->view_mode = AUTO_MODE;

    gtk_window_set_position (GTK_WINDOW (cnnwin), GTK_WIN_POS_CENTER);
    gtk_container_set_border_width (GTK_CONTAINER (cnnwin), 0);

    remmina_widget_pool_register (GTK_WIDGET (cnnwin));
}

static GtkWidget*
remmina_connection_window_new_from_holder (RemminaConnectionHolder *cnnhld)
{
    RemminaConnectionWindow *cnnwin;

    cnnwin = REMMINA_CONNECTION_WINDOW (g_object_new (REMMINA_TYPE_CONNECTION_WINDOW, NULL));
    cnnwin->priv->cnnhld = cnnhld;

    g_signal_connect (G_OBJECT (cnnwin), "delete-event",
        G_CALLBACK (remmina_connection_window_delete_event), cnnhld);
    g_signal_connect (G_OBJECT (cnnwin), "destroy",
        G_CALLBACK (remmina_connection_window_destroy), cnnhld);

    g_signal_connect (G_OBJECT (cnnwin), "focus-in-event",
        G_CALLBACK (remmina_connection_window_focus_in), cnnhld);
    g_signal_connect (G_OBJECT (cnnwin), "focus-out-event",
        G_CALLBACK (remmina_connection_window_focus_out), cnnhld);

    g_signal_connect (G_OBJECT (cnnwin), "enter-notify-event",
        G_CALLBACK (remmina_connection_window_on_enter), cnnhld);
    g_signal_connect (G_OBJECT (cnnwin), "leave-notify-event",
        G_CALLBACK (remmina_connection_window_on_leave), cnnhld);

    g_signal_connect (G_OBJECT (cnnwin), "configure_event",
        G_CALLBACK (remmina_connection_window_on_configure), cnnhld);

    return GTK_WIDGET (cnnwin);
}

/* This function will be called for the first connection. A tag is set to the window so that
 * other connections can determine if whether a new tab should be append to the same window
 */
static void
remmina_connection_window_update_tag (RemminaConnectionWindow *cnnwin, RemminaConnectionObject *cnnobj)
{
    gchar *tag;

    switch (remmina_pref.tab_mode)
    {
    case REMMINA_TAB_BY_GROUP:
        tag = g_strdup (cnnobj->remmina_file->group);
        break;
    case REMMINA_TAB_BY_PROTOCOL:
        tag = g_strdup (cnnobj->remmina_file->protocol);
        break;
    default:
        tag = NULL;
        break;
    }
    g_object_set_data_full (G_OBJECT (cnnwin), "tag", tag, (GDestroyNotify) g_free);
}

static void
remmina_connection_object_create_scrolled_container (RemminaConnectionObject *cnnobj, gint view_mode)
{
    GtkWidget *container;

    if (view_mode == VIEWPORT_FULLSCREEN_MODE)
    {
        container = remmina_scrolled_viewport_new ();
    }
    else
    {
        container = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (container),
            GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_set_border_width (GTK_CONTAINER (container), 0);
        GTK_WIDGET_UNSET_FLAGS (container, GTK_CAN_FOCUS);
    }
    g_object_set_data (G_OBJECT (container), "cnnobj", cnnobj);
    gtk_widget_show (container);
    cnnobj->scrolled_container = container;
}

static gboolean
remmina_connection_holder_grab_focus (gpointer data)
{
    RemminaConnectionObject *cnnobj;
    GtkNotebook *notebook;
    GtkWidget *child;

    if (GTK_IS_WIDGET (data))
    {
        notebook = GTK_NOTEBOOK (data);
        child = gtk_notebook_get_nth_page (notebook, gtk_notebook_get_current_page (notebook));
        cnnobj = g_object_get_data (G_OBJECT (child), "cnnobj");
        if (GTK_IS_WIDGET (cnnobj->remmina_plug))
        {
            remmina_plug_grab_focus (REMMINA_PLUG (cnnobj->remmina_plug));
        }
    }
    return FALSE;
}

static void
remmina_connection_object_on_close_button_clicked (GtkButton *button, RemminaConnectionObject *cnnobj)
{
    if (REMMINA_IS_PLUG (cnnobj->remmina_plug))
    {
        remmina_plug_close_connection (REMMINA_PLUG (cnnobj->remmina_plug));
    }
}

static GtkWidget*
remmina_connection_object_create_tab (RemminaConnectionObject *cnnobj)
{
    GtkWidget *hbox;
    GtkWidget *widget;
    GtkWidget *button;

    hbox = gtk_hbox_new (FALSE, 4);
    gtk_widget_show (hbox);

    widget = gtk_image_new_from_icon_name (remmina_file_get_icon_name (cnnobj->remmina_file), GTK_ICON_SIZE_MENU);
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

    widget = gtk_label_new  (cnnobj->remmina_file->name);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

    button = gtk_button_new ();
	gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
	gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
    gtk_widget_set_name (button, "remmina-small-button");
    gtk_widget_show (button);

    widget = gtk_image_new_from_stock (GTK_STOCK_CLOSE, GTK_ICON_SIZE_MENU);
    gtk_widget_show (widget);
    gtk_container_add (GTK_CONTAINER (button), widget);

    gtk_box_pack_start (GTK_BOX (hbox), button, FALSE, FALSE, 0);

    g_signal_connect (G_OBJECT (button), "clicked", G_CALLBACK (remmina_connection_object_on_close_button_clicked), cnnobj);

    return hbox;
}

static gint
remmina_connection_object_append_page (RemminaConnectionObject *cnnobj, GtkNotebook *notebook, GtkWidget *tab, gint view_mode)
{
    gint i;

    remmina_connection_object_create_scrolled_container (cnnobj, view_mode);
    i = gtk_notebook_append_page (notebook, cnnobj->scrolled_container, tab);
    gtk_notebook_set_tab_reorderable (notebook, cnnobj->scrolled_container, TRUE);
    /* This trick prevents the tab label from being focused */
    GTK_WIDGET_UNSET_FLAGS (gtk_widget_get_parent (tab), GTK_CAN_FOCUS);
    return i;
}

static void
remmina_connection_window_initialize_notebook (GtkNotebook *to, GtkNotebook *from, RemminaConnectionObject *cnnobj, gint view_mode)
{
    gint i, n, c;
    GtkWidget *tab;
    GtkWidget *widget;

    if (cnnobj)
    {
        /* Initial connection for a newly created window */
        tab = remmina_connection_object_create_tab (cnnobj);
        remmina_connection_object_append_page (cnnobj, to, tab, view_mode);

        gtk_widget_reparent (cnnobj->viewport, cnnobj->scrolled_container);

        if (cnnobj->window)
        {
            gtk_widget_destroy (cnnobj->window);
            cnnobj->window = NULL;
        }
    }
    else
    {
        /* View mode changed. Migrate all existing connections to the new notebook */
        c = gtk_notebook_get_current_page (from);
        n = gtk_notebook_get_n_pages (from);
        for (i = 0; i < n; i++)
        {
            widget = gtk_notebook_get_nth_page (from, i);
            cnnobj = (RemminaConnectionObject*) g_object_get_data (G_OBJECT (widget), "cnnobj");

            tab = remmina_connection_object_create_tab (cnnobj);
            remmina_connection_object_append_page (cnnobj, to, tab, view_mode);

            gtk_widget_reparent (cnnobj->viewport, cnnobj->scrolled_container);
        }
        gtk_notebook_set_current_page (to, c);
    }
}

static void
remmina_connection_holder_update_notebook (RemminaConnectionHolder *cnnhld)
{
    GtkNotebook *notebook;
    gint n;

    if (!cnnhld->cnnwin) return;

    notebook = GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook);

    switch (cnnhld->cnnwin->priv->view_mode)
    {
    case SCROLLED_WINDOW_MODE:
        n = gtk_notebook_get_n_pages (notebook);
        gtk_notebook_set_show_tabs (notebook, n > 1);
        gtk_notebook_set_show_border (notebook, n > 1);
        break;
    default:
        gtk_notebook_set_show_tabs (notebook, FALSE);
        gtk_notebook_set_show_border (notebook, FALSE);
        break;
    }
}

static gboolean
remmina_connection_holder_on_switch_page_real (gpointer data)
{
    RemminaConnectionHolder *cnnhld = (RemminaConnectionHolder*) data;
    RemminaConnectionWindowPriv *priv = cnnhld->cnnwin->priv;

    if (GTK_IS_WIDGET (cnnhld->cnnwin))
    {
        remmina_connection_holder_update_toolbar (cnnhld);
        remmina_connection_holder_grab_focus (priv->notebook);
    }
    return FALSE;
}

static void
remmina_connection_holder_on_switch_page (GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, RemminaConnectionHolder *cnnhld)
{
    g_idle_add (remmina_connection_holder_on_switch_page_real, cnnhld);
}

static void
remmina_connection_holder_on_page_added (GtkNotebook *notebook, GtkWidget *child, guint page_num, RemminaConnectionHolder *cnnhld)
{
    remmina_connection_holder_update_notebook (cnnhld);
}

static GtkWidget*
remmina_connection_holder_create_notebook (RemminaConnectionHolder *cnnhld)
{
    GtkWidget *notebook;

    notebook = gtk_notebook_new ();
    gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
    gtk_widget_show (notebook);
    g_signal_connect (G_OBJECT (notebook), "switch-page",
        G_CALLBACK (remmina_connection_holder_on_switch_page), cnnhld);
    g_signal_connect (G_OBJECT (notebook), "page-added",
        G_CALLBACK (remmina_connection_holder_on_page_added), cnnhld);
    g_signal_connect (G_OBJECT (notebook), "page-removed",
        G_CALLBACK (remmina_connection_holder_on_page_added), cnnhld);
    GTK_WIDGET_UNSET_FLAGS (notebook, GTK_CAN_FOCUS);

    return notebook;
}

/* Create a scrolled window container */
static void
remmina_connection_holder_create_scrolled (RemminaConnectionHolder *cnnhld, RemminaConnectionObject *cnnobj)
{
    GtkWidget *window;
    GtkWidget *oldwindow;
    GtkWidget *vbox;
    GtkWidget *toolbar;
    GtkWidget *notebook;
    gchar *tag;

    oldwindow = GTK_WIDGET (cnnhld->cnnwin);
    window = remmina_connection_window_new_from_holder (cnnhld);
    gtk_widget_realize (window);
    cnnhld->cnnwin = REMMINA_CONNECTION_WINDOW (window);

    /* Create the vbox container */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_container_add (GTK_CONTAINER (window), vbox);

    /* Create the toolbar */
    toolbar = remmina_connection_holder_create_toolbar (cnnhld, SCROLLED_WINDOW_MODE);
    gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

    /* Create the notebook */
    notebook = remmina_connection_holder_create_notebook (cnnhld);
    gtk_box_pack_start (GTK_BOX (vbox), notebook, TRUE, TRUE, 0);
    gtk_container_set_focus_chain (GTK_CONTAINER (vbox), g_list_append (NULL, notebook));

    cnnhld->cnnwin->priv->view_mode = SCROLLED_WINDOW_MODE;
    cnnhld->cnnwin->priv->toolbar = toolbar;
    cnnhld->cnnwin->priv->notebook = notebook;

    remmina_connection_window_initialize_notebook (GTK_NOTEBOOK (notebook),
        (oldwindow ? GTK_NOTEBOOK (REMMINA_CONNECTION_WINDOW (oldwindow)->priv->notebook) : NULL), cnnobj,
        SCROLLED_WINDOW_MODE);

    if (cnnobj)
    {
        remmina_connection_window_update_tag (cnnhld->cnnwin, cnnobj);
        if (cnnobj->remmina_file->window_maximize)
        {
            gtk_window_maximize (GTK_WINDOW (cnnhld->cnnwin));
        }
    }

    if (oldwindow)
    {
        tag = g_strdup ((gchar*) g_object_get_data (G_OBJECT (oldwindow), "tag"));
        g_object_set_data_full (G_OBJECT (cnnhld->cnnwin), "tag", tag, (GDestroyNotify) g_free);
        gtk_widget_destroy (oldwindow);
    }

    remmina_connection_holder_update_toolbar (cnnhld);
    remmina_connection_holder_check_resize (cnnhld);

    gtk_widget_show (GTK_WIDGET (cnnhld->cnnwin));
}

static void
remmina_connection_holder_create_fullscreen (RemminaConnectionHolder *cnnhld, RemminaConnectionObject *cnnobj, gint view_mode)
{
    GtkWidget *window;
    GtkWidget *oldwindow;
    GtkWidget *notebook;
    GdkColor color;
    gchar *tag;

    oldwindow = GTK_WIDGET (cnnhld->cnnwin);
    window = remmina_connection_window_new_from_holder (cnnhld);
    gtk_widget_realize (window);
    cnnhld->cnnwin = REMMINA_CONNECTION_WINDOW (window);

    if (view_mode == VIEWPORT_FULLSCREEN_MODE)
    {
        gdk_color_parse ("black", &color);
        gtk_widget_modify_bg (window, GTK_STATE_NORMAL, &color);
    }

    notebook = remmina_connection_holder_create_notebook (cnnhld);
    gtk_container_add (GTK_CONTAINER (window), notebook);

    cnnhld->cnnwin->priv->notebook = notebook;
    cnnhld->cnnwin->priv->view_mode = view_mode;

    remmina_connection_window_initialize_notebook (GTK_NOTEBOOK (notebook),
        (oldwindow ? GTK_NOTEBOOK (REMMINA_CONNECTION_WINDOW (oldwindow)->priv->notebook) : NULL), cnnobj,
        view_mode);

    if (cnnobj)
    {
        remmina_connection_window_update_tag (cnnhld->cnnwin, cnnobj);
    }
    if (oldwindow)
    {
        tag = g_strdup ((gchar*) g_object_get_data (G_OBJECT (oldwindow), "tag"));
        g_object_set_data_full (G_OBJECT (cnnhld->cnnwin), "tag", tag, (GDestroyNotify) g_free);
        gtk_widget_destroy (oldwindow);
    }

    /* Create the floating toolbar */
    remmina_connection_holder_create_floating_toolbar (cnnhld, view_mode);
    remmina_connection_holder_update_toolbar (cnnhld);

    gtk_window_fullscreen (GTK_WINDOW (window));
    remmina_connection_holder_check_resize (cnnhld);

    gtk_widget_show (window);
}

static void
remmina_connection_object_on_connect (RemminaPlug *gp, RemminaConnectionObject *cnnobj)
{
    RemminaConnectionHolder *cnnhld = cnnobj->cnnhld;
    GdkScreen *screen;
    GtkWidget *tab;
    gint i;

    screen = gdk_screen_get_default ();

    cnnobj->connected = TRUE;

    /* Remember recent list for quick connect */
    if (cnnobj->remmina_file->filename == NULL)
    {
        remmina_pref_add_recent (cnnobj->remmina_file->protocol, cnnobj->remmina_file->server);
    }

    if (!cnnhld->cnnwin)
    {
        switch (cnnobj->remmina_file->viewmode)
        {
            case SCROLLED_FULLSCREEN_MODE:
            case VIEWPORT_FULLSCREEN_MODE:
                remmina_connection_holder_create_fullscreen (cnnhld, cnnobj, cnnobj->remmina_file->viewmode);
                break;

            case SCROLLED_WINDOW_MODE:
            default:
                remmina_connection_holder_create_scrolled (cnnhld, cnnobj);
                break;
        }
    }
    else
    {
        tab = remmina_connection_object_create_tab (cnnobj);
        i = remmina_connection_object_append_page (cnnobj,
            GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook), tab, cnnhld->cnnwin->priv->view_mode);
        gtk_widget_reparent (cnnobj->viewport, cnnobj->scrolled_container);
        gtk_window_present (GTK_WINDOW (cnnhld->cnnwin));
        gtk_notebook_set_current_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook), i);
    }

    if (cnnhld->cnnwin->priv->floating_toolbar)
    {
        gtk_widget_show (cnnhld->cnnwin->priv->floating_toolbar);
    }
}

static void
remmina_connection_object_on_disconnect (RemminaPlug *gp, RemminaConnectionObject *cnnobj)
{
    RemminaConnectionHolder *cnnhld = cnnobj->cnnhld;
    GtkWidget *dialog;

    cnnobj->connected = FALSE;
    if (scale_option_window)
    {
        gtk_widget_destroy (scale_option_window);
        scale_option_window = NULL;
    }

    if (remmina_pref.save_view_mode)
    {
        if (cnnhld->cnnwin)
        {
            cnnobj->remmina_file->viewmode = cnnhld->cnnwin->priv->view_mode;
        }
        remmina_file_save (cnnobj->remmina_file);
    }
    g_free (cnnobj->remmina_file);

    if (REMMINA_PLUG (cnnobj->remmina_plug)->has_error)
    {
        dialog = gtk_message_dialog_new (NULL,
            GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
            REMMINA_PLUG (cnnobj->remmina_plug)->error_message, NULL);
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_widget_show (dialog);
        remmina_widget_pool_register (dialog);
    }

    if (cnnobj->window)
    {
        gtk_widget_destroy (cnnobj->window);
        cnnobj->window = NULL;
    }

    if (cnnhld->cnnwin && cnnobj->scrolled_container)
    {
        gtk_notebook_remove_page (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook),
            gtk_notebook_page_num (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook), cnnobj->scrolled_container));

        if (gtk_notebook_get_n_pages (GTK_NOTEBOOK (cnnhld->cnnwin->priv->notebook)) <= 0)
        {
            gtk_widget_destroy (GTK_WIDGET (cnnhld->cnnwin));
        }
    }
    g_free (cnnobj);
}

static void
remmina_connection_object_on_desktop_resize (RemminaPlug *gp, RemminaConnectionObject *cnnobj)
{
    remmina_connection_holder_check_resize (cnnobj->cnnhld);
}

static RemminaConnectionWindow*
remmina_connection_window_find (RemminaFile *remminafile)
{
    gchar *tag;

    switch (remmina_pref.tab_mode)
    {
    case REMMINA_TAB_BY_GROUP:
        tag = remminafile->group;
        break;
    case REMMINA_TAB_BY_PROTOCOL:
        tag = remminafile->protocol;
        break;
    case REMMINA_TAB_ALL:
        tag = NULL;
        break;
    case REMMINA_TAB_NONE:
    default:
        return NULL;
    }
    return REMMINA_CONNECTION_WINDOW (remmina_widget_pool_find (REMMINA_TYPE_CONNECTION_WINDOW, tag));
}

gboolean
remmina_connection_window_open_from_filename (const gchar *filename)
{
    RemminaFile *remminafile;
    GtkWidget *dialog;

    remminafile = remmina_file_load (filename);
    if (remminafile)
    {
        remmina_connection_window_open_from_file (remminafile);
        return TRUE;
    }
    else
    {
        dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            _("File %s not found."), filename);
        g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (gtk_widget_destroy), NULL);
        gtk_widget_show (dialog);
        remmina_widget_pool_register (dialog);
        return FALSE;
    }
}

void
remmina_connection_window_open_from_file (RemminaFile *remminafile)
{
    RemminaConnectionWindow *cnnwin;
    RemminaConnectionHolder *cnnhld;
    RemminaConnectionObject *cnnobj;
    GdkColor color;

    if (g_strcmp0 (remminafile->protocol, "SFTP") == 0)
    {
#ifdef HAVE_LIBSSH
        remmina_sftp_window_open (remminafile);
#endif
        return;
    }

    cnnwin = remmina_connection_window_find (remminafile);

    remmina_file_update_screen_resolution (remminafile);

    if (cnnwin)
    {
        cnnhld = cnnwin->priv->cnnhld;
    }
    else
    {
        cnnhld = g_new (RemminaConnectionHolder, 1);
        cnnhld->cnnwin = NULL;
    }

    cnnobj = g_new (RemminaConnectionObject, 1);
    cnnobj->cnnhld = cnnhld;
    cnnobj->remmina_file = remminafile;
    cnnobj->scrolled_container = NULL;
    cnnobj->connected = FALSE;

    /* Create the RemminaPlug */
    if (g_strcmp0 (remminafile->protocol, "RDP") == 0)
    {
        cnnobj->remmina_plug = remmina_plug_rdp_new ();
    }
    else if (strncmp (remminafile->protocol, "VNC", 3) == 0)
    {
        cnnobj->remmina_plug = remmina_plug_vnc_new (remminafile->scale);
    }
    else if (g_strcmp0 (remminafile->protocol, "XDMCP") == 0)
    {
        cnnobj->remmina_plug = remmina_plug_xdmcp_new ();
    }
    else
    {
        g_free (cnnobj);
        g_print ("Fatal: Protocol %s not supported\n", remminafile->protocol);
        return;
    }

    gtk_widget_show (cnnobj->remmina_plug);
    g_signal_connect (G_OBJECT (cnnobj->remmina_plug), "connect",
        G_CALLBACK (remmina_connection_object_on_connect), cnnobj);
    g_signal_connect (G_OBJECT (cnnobj->remmina_plug), "disconnect",
        G_CALLBACK (remmina_connection_object_on_disconnect), cnnobj);
    g_signal_connect (G_OBJECT (cnnobj->remmina_plug), "desktop-resize",
        G_CALLBACK (remmina_connection_object_on_desktop_resize), cnnobj);

    /* Create the alignment to make the RemminaPlug centered */
    cnnobj->alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
    gtk_widget_show (cnnobj->alignment);
    gtk_container_set_border_width (GTK_CONTAINER (cnnobj->alignment), 0);
    gtk_container_add (GTK_CONTAINER (cnnobj->alignment), cnnobj->remmina_plug);
    g_signal_connect (G_OBJECT (cnnobj->alignment), "size-allocate",
        G_CALLBACK (remmina_connection_object_alignment_on_allocate), cnnobj);

    /* Create the viewport to make the RemminaPlug scrollable */
    cnnobj->viewport = gtk_viewport_new (NULL, NULL);
    gtk_widget_show (cnnobj->viewport);
    gdk_color_parse ("black", &color);
    gtk_widget_modify_bg (cnnobj->viewport, GTK_STATE_NORMAL, &color);
    gtk_container_set_border_width (GTK_CONTAINER (cnnobj->viewport), 0);
    gtk_viewport_set_shadow_type (GTK_VIEWPORT (cnnobj->viewport), GTK_SHADOW_NONE);
    gtk_container_add (GTK_CONTAINER (cnnobj->viewport), cnnobj->alignment);

    cnnobj->window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_widget_realize (cnnobj->window);
    gtk_container_add (GTK_CONTAINER (cnnobj->window), cnnobj->viewport);

    if (!remmina_pref.save_view_mode) cnnobj->remmina_file->viewmode = remmina_pref.default_mode;

    remmina_plug_open_connection (REMMINA_PLUG (cnnobj->remmina_plug), remminafile);
}

