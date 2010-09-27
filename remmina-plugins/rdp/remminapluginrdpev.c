/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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

#include "remminapluginrdp.h"
#include "remminapluginrdpev.h"
#include <gdk/gdkkeysyms.h>
#include <freerdp/kbd.h>

static void
remmina_plugin_rdpev_event_push (RemminaProtocolWidget *gp,
    gint type, gint flag, gint param1, gint param2)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent *event;

    gpdata = GET_DATA (gp);
    event = g_new (RemminaPluginRdpEvent, 1);
    event->type = type;
    event->flag = flag;
    event->param1 = param1;
    event->param2 = param2;
    if (gpdata->event_queue)
    {
        g_async_queue_push (gpdata->event_queue, event);
        (void) write (gpdata->event_pipe[1], "\0", 1);
    }
}

static void
remmina_plugin_rdpev_release_key (RemminaProtocolWidget *gp, gint scancode)
{
    RemminaPluginRdpData *gpdata;
    gint k;
    gint i;

    gpdata = GET_DATA (gp);
    if (scancode == 0)
    {
        /* Send all release key events for previously pressed keys */
        for (i = 0; i < gpdata->pressed_keys->len; i++)
        {
            k = g_array_index (gpdata->pressed_keys, gint, i);
            remmina_plugin_rdpev_event_push (gp, RDP_INPUT_SCANCODE, RDP_KEYRELEASE, k, 0);
        }
        g_array_set_size (gpdata->pressed_keys, 0);
    }
    else
    {
        /* Unregister the keycode only */
        for (i = 0; i < gpdata->pressed_keys->len; i++)
        {
            k = g_array_index (gpdata->pressed_keys, gint, i);
            if (k == scancode)
            {
                g_array_remove_index_fast (gpdata->pressed_keys, i);
                break;
            }
        }
    }
}

static gboolean
remmina_plugin_rdpev_update_scale_buffer (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;
    gint width, height;
    gint gpwidth, gpheight;
    gboolean scale;
    gint hscale, vscale;

    gpdata = GET_DATA (gp);
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    width = GTK_WIDGET (gp)->allocation.width;
    height = GTK_WIDGET (gp)->allocation.height;
    scale = remmina_plugin_service->protocol_plugin_get_scale (gp);
    if (scale)
    {
        if (width > 1 && height > 1)
        {
            LOCK_BUFFER (FALSE)

            if (gpdata->scale_buffer)
            {
                g_object_unref (gpdata->scale_buffer);
            }
            gpwidth = remmina_plugin_service->protocol_plugin_get_width (gp);
            gpheight = remmina_plugin_service->protocol_plugin_get_height (gp);
            hscale = remmina_plugin_service->file_get_int (remminafile, "hscale", 0);
            vscale = remmina_plugin_service->file_get_int (remminafile, "vscale", 0);
            gpdata->scale_width = (hscale > 0 ?
                MAX (1, gpwidth * hscale / 100) : width);
            gpdata->scale_height = (vscale > 0 ?
                MAX (1, gpheight * vscale / 100) : height);

            if (gpdata->scale_width == gpwidth && gpdata->scale_height == gpheight)
            {
                /* Same size, just copy the pixels */
                gpdata->scale_buffer = gdk_pixbuf_copy (gpdata->rgb_buffer);
            }
            else
            {
                gpdata->scale_buffer = gdk_pixbuf_scale_simple (gpdata->rgb_buffer,
                    gpdata->scale_width, gpdata->scale_height,
                    remmina_plugin_service->pref_get_scale_quality ());
            }

            UNLOCK_BUFFER (FALSE)
        }
    }
    else
    {
        LOCK_BUFFER (FALSE)

        if (gpdata->scale_buffer)
        {
            g_object_unref (gpdata->scale_buffer);
            gpdata->scale_buffer = NULL;
        }
        gpdata->scale_width = 0;
        gpdata->scale_height = 0;

        UNLOCK_BUFFER (FALSE)
    }
    if (width > 1 && height > 1)
    {
        gtk_widget_queue_draw_area (GTK_WIDGET (gp), 0, 0, width, height);
    }
    gpdata->scale_handler = 0;
    return FALSE;
}

static gboolean
remmina_plugin_rdpev_on_expose (GtkWidget *widget, GdkEventExpose *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    GdkPixbuf *buffer;
    gint width, height, x, y, rowstride;
    gboolean scale;

    gpdata = GET_DATA (gp);

    LOCK_BUFFER (FALSE)

    scale = remmina_plugin_service->protocol_plugin_get_scale (gp);
    /* widget == gpdata->drawing_area */
    buffer = (scale ? gpdata->scale_buffer : gpdata->rgb_buffer);
    if (!buffer)
    {
        UNLOCK_BUFFER (FALSE)
        return FALSE;
    }

    width = (scale ? gpdata->scale_width : remmina_plugin_service->protocol_plugin_get_width (gp));
    height = (scale ? gpdata->scale_height : remmina_plugin_service->protocol_plugin_get_height (gp));
    if (event->area.x >= width || event->area.y >= height)
    {
        UNLOCK_BUFFER (FALSE)
        return FALSE;
    }
    x = event->area.x;
    y = event->area.y;
    rowstride = gdk_pixbuf_get_rowstride (buffer);

    /* this is a little tricky. It "moves" the rgb_buffer pointer to (x,y) as top-left corner,
       and keeps the same rowstride. This is an effective way to "clip" the rgb_buffer for gdk. */
    gdk_draw_rgb_image (widget->window, widget->style->white_gc,
        x, y,
        MIN (width - x, event->area.width), MIN (height - y, event->area.height),
        GDK_RGB_DITHER_MAX,
        gdk_pixbuf_get_pixels (buffer) + y * rowstride + x * 3,
        rowstride);

    UNLOCK_BUFFER (FALSE)
    return TRUE;
}

static gboolean
remmina_plugin_rdpev_on_configure (GtkWidget *widget, GdkEventConfigure *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    /* We do a delayed reallocating to improve performance */
    if (gpdata->scale_handler) g_source_remove (gpdata->scale_handler);
    gpdata->scale_handler = g_timeout_add (1000, (GSourceFunc) remmina_plugin_rdpev_update_scale_buffer, gp);
    return FALSE;
}

static void
remmina_plugin_rdpev_translate_pos (RemminaProtocolWidget *gp, int ix, int iy, int *ox, int *oy)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    if (gpdata->scale && gpdata->scale_width >= 1 && gpdata->scale_height >= 1)
    {
        *ox = ix * remmina_plugin_service->protocol_plugin_get_width (gp) / gpdata->scale_width;
        *oy = iy * remmina_plugin_service->protocol_plugin_get_height (gp) / gpdata->scale_height;
    }
    else
    {
        *ox = ix;
        *oy = iy;
    }
}

static gboolean
remmina_plugin_rdpev_on_motion (GtkWidget *widget, GdkEventMotion *event, RemminaProtocolWidget *gp)
{
    gint x, y;

    remmina_plugin_rdpev_translate_pos (gp, event->x, event->y, &x, &y);
    remmina_plugin_rdpev_event_push (gp, RDP_INPUT_MOUSE, PTRFLAGS_MOVE, x, y);
    return TRUE;
}

static gboolean
remmina_plugin_rdpev_on_button (GtkWidget *widget, GdkEventButton *event, RemminaProtocolWidget *gp)
{
    gint x, y;
    gint flag;

    /* We only accept 3 buttons */
    if (event->button < 1 || event->button > 3) return FALSE;
    /* We bypass 2button-press and 3button-press events */
    if (event->type != GDK_BUTTON_PRESS && event->type != GDK_BUTTON_RELEASE) return TRUE;

    remmina_plugin_rdpev_translate_pos (gp, event->x, event->y, &x, &y);

    flag = 0;
    if (event->type == GDK_BUTTON_PRESS)
    {
        flag = PTRFLAGS_DOWN;
    }
    switch (event->button)
    {
        case 1:
            flag |= PTRFLAGS_BUTTON1;
            break;
        case 2:
            flag |= PTRFLAGS_BUTTON3;
            break;
        case 3:
            flag |= PTRFLAGS_BUTTON2;
            break;
    }
    if (flag != 0)
    {
        remmina_plugin_rdpev_event_push (gp, RDP_INPUT_MOUSE, flag, x, y);
    }
    return TRUE;
}

static gboolean
remmina_plugin_rdpev_on_scroll (GtkWidget *widget, GdkEventScroll *event, RemminaProtocolWidget *gp)
{
    gint x, y;
    gint flag;

    flag = 0;
    switch (event->direction)
    {
    case GDK_SCROLL_UP:
        flag = PTRFLAGS_WHEEL | 0x0078;
        break;
    case GDK_SCROLL_DOWN:
        flag = PTRFLAGS_WHEEL | PTRFLAGS_WHEEL_NEGATIVE | 0x0088;
        break;
    default:
        return FALSE;
    }

    remmina_plugin_rdpev_translate_pos (gp, event->x, event->y, &x, &y);
    remmina_plugin_rdpev_event_push (gp, RDP_INPUT_MOUSE, flag, x, y);
    return TRUE;
}

static gboolean
remmina_plugin_rdpev_on_key (GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    gint flag;
    gint scancode = 0;

    gpdata = GET_DATA (gp);
    flag = (event->type == GDK_KEY_PRESS ? RDP_KEYPRESS : RDP_KEYRELEASE);
    switch (event->keyval)
    {
    case GDK_Break:
        remmina_plugin_rdpev_event_push (gp, RDP_INPUT_SCANCODE, flag, 0xc6, 0);
        break;
    case GDK_Pause:
        remmina_plugin_rdpev_event_push (gp, RDP_INPUT_SCANCODE, flag | 0x0200, 0x1d, 0);
        remmina_plugin_rdpev_event_push (gp, RDP_INPUT_SCANCODE, flag, 0x45, 0);
        break;
    default:
        scancode = freerdp_kbd_get_scancode_by_keycode (event->hardware_keycode, &flag);
        remmina_plugin_service->log_printf ("[RDP]keyval=%04X keycode=%i scancode=%i flag=%04X\n",
            event->keyval, event->hardware_keycode, scancode, flag);
        if (scancode)
        {
            remmina_plugin_rdpev_event_push (gp, RDP_INPUT_SCANCODE, flag, scancode, 0);
        }
        break;
    }

    /* Register/unregister the pressed key */
    if (scancode)
    {
        if (event->type == GDK_KEY_PRESS)
        {
            g_array_append_val (gpdata->pressed_keys, scancode);
        }
        else
        {
            remmina_plugin_rdpev_release_key (gp, scancode);
        }
    }
    return TRUE;
}

void
remmina_plugin_rdpev_init (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    gint flags;

    gpdata = GET_DATA (gp);
    gpdata->drawing_area = gtk_drawing_area_new ();
    gtk_widget_show (gpdata->drawing_area);
    gtk_container_add (GTK_CONTAINER (gp), gpdata->drawing_area);

    gtk_widget_add_events (gpdata->drawing_area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK
        | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    GTK_WIDGET_SET_FLAGS (gpdata->drawing_area, GTK_CAN_FOCUS);

    remmina_plugin_service->protocol_plugin_register_hostkey (gp, gpdata->drawing_area);

    g_signal_connect (G_OBJECT (gpdata->drawing_area), "expose_event",
        G_CALLBACK (remmina_plugin_rdpev_on_expose), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "configure_event",
        G_CALLBACK (remmina_plugin_rdpev_on_configure), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "motion-notify-event",
        G_CALLBACK (remmina_plugin_rdpev_on_motion), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "button-press-event",
        G_CALLBACK (remmina_plugin_rdpev_on_button), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "button-release-event",
        G_CALLBACK (remmina_plugin_rdpev_on_button), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "scroll-event",
        G_CALLBACK (remmina_plugin_rdpev_on_scroll), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "key-press-event",
        G_CALLBACK (remmina_plugin_rdpev_on_key), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "key-release-event",
        G_CALLBACK (remmina_plugin_rdpev_on_key), gp);

    gpdata->pressed_keys = g_array_new (FALSE, TRUE, sizeof (gint));
    gpdata->event_queue = g_async_queue_new_full (g_free);
    if (pipe (gpdata->event_pipe))
    {
        g_print ("Error creating pipes.\n");
        gpdata->event_pipe[0] = -1;
        gpdata->event_pipe[1] = -1;
    }
    else
    {
        flags = fcntl (gpdata->event_pipe[0], F_GETFL, 0);
        fcntl (gpdata->event_pipe[0], F_SETFL, flags | O_NONBLOCK);
    }
}

void
remmina_plugin_rdpev_pre_connect (RemminaProtocolWidget *gp)
{
}

void
remmina_plugin_rdpev_post_connect (RemminaProtocolWidget *gp)
{
}

void
remmina_plugin_rdpev_uninit (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    if (gpdata->scale_handler)
    {
        g_source_remove (gpdata->scale_handler);
        gpdata->scale_handler = 0;
    }
    g_array_free (gpdata->pressed_keys, TRUE);
    g_async_queue_unref (gpdata->event_queue);
    gpdata->event_queue = NULL;
    close (gpdata->event_pipe[0]);
    close (gpdata->event_pipe[1]);
}

void
remmina_plugin_rdpev_update_scale (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;
    gint width, height;
    gint hscale, vscale;

    gpdata = GET_DATA (gp);
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    width = remmina_plugin_service->protocol_plugin_get_width (gp);
    height = remmina_plugin_service->protocol_plugin_get_height (gp);
    hscale = remmina_plugin_service->file_get_int (remminafile, "hscale", 0);
    vscale = remmina_plugin_service->file_get_int (remminafile, "vscale", 0);
    if (gpdata->scale)
    {
        gtk_widget_set_size_request (GTK_WIDGET (gpdata->drawing_area),
            (hscale > 0 ? width * hscale / 100 : -1),
            (vscale > 0 ? height * vscale / 100 : -1));
    }
    else
    {
        gtk_widget_set_size_request (GTK_WIDGET (gpdata->drawing_area), width, height);
    }
}

gboolean
remmina_plugin_rdpev_queuedraw (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    gint x, y, w, h;

    gpdata = GET_DATA (gp);

    LOCK_BUFFER (FALSE)
    x = gpdata->queuedraw_x;
    y = gpdata->queuedraw_y;
    w = gpdata->queuedraw_w;
    h = gpdata->queuedraw_h;
    gpdata->queuedraw_handler = 0;
    UNLOCK_BUFFER (FALSE)

    gtk_widget_queue_draw_area (GTK_WIDGET (gp), x, y, w, h);

    return FALSE;
}

gboolean
remmina_plugin_rdpev_queuecursor (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    GdkCursor *cur;

    gpdata = GET_DATA (gp);

    LOCK_BUFFER (FALSE)
    gpdata->queuecursor_handler = 0;

    if (gpdata->queuecursor_pixbuf)
    {
        cur = gdk_cursor_new_from_pixbuf (gdk_display_get_default (),
            gpdata->queuecursor_pixbuf, gpdata->queuecursor_x, gpdata->queuecursor_y);
        gdk_window_set_cursor (gtk_widget_get_window (gpdata->drawing_area), cur);
        gdk_cursor_unref (cur);
        gpdata->queuecursor_pixbuf = NULL;
    }
    else if (gpdata->queuecursor_null)
    {
        cur = gdk_cursor_new (GDK_BLANK_CURSOR);
        gdk_window_set_cursor (gtk_widget_get_window (gpdata->drawing_area), cur);
        gdk_cursor_unref (cur);
    }
    else
    {
        gdk_window_set_cursor (gtk_widget_get_window (gpdata->drawing_area), NULL);
    }
    UNLOCK_BUFFER (FALSE)

    return FALSE;
}

void
remmina_plugin_rdpev_unfocus (RemminaProtocolWidget *gp)
{
    remmina_plugin_rdpev_release_key (gp, 0);
}

