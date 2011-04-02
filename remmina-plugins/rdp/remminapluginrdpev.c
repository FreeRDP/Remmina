/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
 * Copyright (C) 2010-2011 Vic Lee
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

/* X11 drawings were ported from xfreerdp */

#include "remminapluginrdp.h"
#include "remminapluginrdpev.h"
#include "remminapluginrdpui.h"
#include <gdk/gdkkeysyms.h>
#include <freerdp/kbd.h>

static void
remmina_plugin_rdpev_event_push (RemminaProtocolWidget *gp, const RemminaPluginRdpEvent *e)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent *event;

    gpdata = GET_DATA (gp);
    if (gpdata->event_queue)
    {
        event = g_memdup (e, sizeof(RemminaPluginRdpEvent));
        g_async_queue_push (gpdata->event_queue, event);
        (void) write (gpdata->event_pipe[1], "\0", 1);
    }
}

static void
remmina_plugin_rdpev_release_key (RemminaProtocolWidget *gp, gint scancode)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent rdp_event = { 0 };
    gint k;
    gint i;

    gpdata = GET_DATA (gp);
    rdp_event.type = REMMINA_PLUGIN_RDP_EVENT_TYPE_SCANCODE;
    if (scancode == 0)
    {
        /* Send all release key events for previously pressed keys */
        rdp_event.key_event.up = True;
        for (i = 0; i < gpdata->pressed_keys->len; i++)
        {
            rdp_event.key_event.key_code = g_array_index (gpdata->pressed_keys, gint, i);
            remmina_plugin_rdpev_event_push (gp, &rdp_event);
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

static void
remmina_plugin_rdpev_scale_area (RemminaProtocolWidget *gp, gint *x, gint *y, gint *w, gint *h)
{
    RemminaPluginRdpData *gpdata;
    gint sx, sy, sw, sh;
    gint width, height;

    gpdata = GET_DATA (gp);
    if (!gpdata->rgb_surface) return;

    width = remmina_plugin_service->protocol_plugin_get_width (gp);
    height = remmina_plugin_service->protocol_plugin_get_height (gp);
    if (width == 0 || height == 0) return;

    if (gpdata->scale_width == width && gpdata->scale_height == height)
    {
        /* Same size, just copy the pixels */
        *x = MIN (MAX (0, *x), width - 1);
        *y = MIN (MAX (0, *y), height - 1);
        *w = MIN (width - *x, *w);
        *h = MIN (height - *y, *h);
        return;
    }

    /* We have to extend the scaled region one scaled pixel, to avoid gaps */
    sx = MIN (MAX (0, (*x) * gpdata->scale_width / width
        - gpdata->scale_width / width - 2), gpdata->scale_width - 1);
    sy = MIN (MAX (0, (*y) * gpdata->scale_height / height
        - gpdata->scale_height / height - 2), gpdata->scale_height - 1);
    sw = MIN (gpdata->scale_width - sx, (*w) * gpdata->scale_width / width
        + gpdata->scale_width / width + 4);
    sh = MIN (gpdata->scale_height - sy, (*h) * gpdata->scale_height / height
        + gpdata->scale_height / height + 4);

    *x = sx; *y = sy; *w = sw; *h = sh;
}

static void
remmina_plugin_rdpev_update_rect (RemminaProtocolWidget *gp, gint x, gint y, gint w, gint h)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    if (remmina_plugin_service->protocol_plugin_get_scale (gp))
    {
        remmina_plugin_rdpev_scale_area (gp, &x, &y, &w, &h);
    }
    gtk_widget_queue_draw_area (gpdata->drawing_area, x, y, w, h);
}

static gboolean
remmina_plugin_rdpev_update_scale_factor (RemminaProtocolWidget *gp)
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
            gpwidth = remmina_plugin_service->protocol_plugin_get_width (gp);
            gpheight = remmina_plugin_service->protocol_plugin_get_height (gp);
            hscale = remmina_plugin_service->file_get_int (remminafile, "hscale", 0);
            vscale = remmina_plugin_service->file_get_int (remminafile, "vscale", 0);
            gpdata->scale_width = (hscale > 0 ?
                MAX (1, gpwidth * hscale / 100) : width);
            gpdata->scale_height = (vscale > 0 ?
                MAX (1, gpheight * vscale / 100) : height);

            gpdata->scale_x = (gdouble)gpdata->scale_width / (gdouble)gpwidth;
            gpdata->scale_y = (gdouble)gpdata->scale_height / (gdouble)gpheight;
        }
    }
    else
    {
        gpdata->scale_width = 0;
        gpdata->scale_height = 0;
        gpdata->scale_x = 0;
        gpdata->scale_y = 0;
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
    gint x, y;
    gboolean scale;
    cairo_t *context;

    gpdata = GET_DATA (gp);

    if (!gpdata->rgb_pixmap) return FALSE;

    scale = remmina_plugin_service->protocol_plugin_get_scale (gp);
    x = event->area.x;
    y = event->area.y;

    context = gdk_cairo_create (GDK_DRAWABLE (gpdata->drawing_area->window));
    cairo_rectangle (context, x, y, event->area.width, event->area.height);
    if (scale)
    {
        cairo_scale (context, gpdata->scale_x, gpdata->scale_y);
    }
    gdk_cairo_set_source_pixmap (context, gpdata->rgb_pixmap, 0, 0);
    cairo_fill (context);
    cairo_destroy (context);
    return TRUE;
}

static gboolean
remmina_plugin_rdpev_on_configure (GtkWidget *widget, GdkEventConfigure *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    /* We do a delayed reallocating to improve performance */
    if (gpdata->scale_handler) g_source_remove (gpdata->scale_handler);
    gpdata->scale_handler = g_timeout_add (1000, (GSourceFunc) remmina_plugin_rdpev_update_scale_factor, gp);
    return FALSE;
}

static void
remmina_plugin_rdpev_translate_pos (RemminaProtocolWidget *gp, int ix, int iy, uint16 *ox, uint16 *oy)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    if (gpdata->scale && gpdata->scale_width >= 1 && gpdata->scale_height >= 1)
    {
        *ox = (uint16) (ix * remmina_plugin_service->protocol_plugin_get_width (gp) / gpdata->scale_width);
        *oy = (uint16) (iy * remmina_plugin_service->protocol_plugin_get_height (gp) / gpdata->scale_height);
    }
    else
    {
        *ox = (uint16) ix;
        *oy = (uint16) iy;
    }
}

static gboolean
remmina_plugin_rdpev_on_motion (GtkWidget *widget, GdkEventMotion *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpEvent rdp_event = { 0 };

    rdp_event.type = REMMINA_PLUGIN_RDP_EVENT_TYPE_MOUSE;
    rdp_event.mouse_event.flags = PTRFLAGS_MOVE;
    remmina_plugin_rdpev_translate_pos (gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);
    remmina_plugin_rdpev_event_push (gp, &rdp_event);
    return TRUE;
}

static gboolean
remmina_plugin_rdpev_on_button (GtkWidget *widget, GdkEventButton *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpEvent rdp_event = { 0 };
    gint flag;

    /* We only accept 3 buttons */
    if (event->button < 1 || event->button > 3) return FALSE;
    /* We bypass 2button-press and 3button-press events */
    if (event->type != GDK_BUTTON_PRESS && event->type != GDK_BUTTON_RELEASE) return TRUE;

    rdp_event.type = REMMINA_PLUGIN_RDP_EVENT_TYPE_MOUSE;
    remmina_plugin_rdpev_translate_pos (gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);

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
        rdp_event.mouse_event.flags = flag;
        remmina_plugin_rdpev_event_push (gp, &rdp_event);
    }
    return TRUE;
}

static gboolean
remmina_plugin_rdpev_on_scroll (GtkWidget *widget, GdkEventScroll *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpEvent rdp_event = { 0 };
    gint flag;

    rdp_event.type = REMMINA_PLUGIN_RDP_EVENT_TYPE_MOUSE;
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

    rdp_event.mouse_event.flags = flag;
    remmina_plugin_rdpev_translate_pos (gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);
    remmina_plugin_rdpev_event_push (gp, &rdp_event);
    return TRUE;
}

static gboolean
remmina_plugin_rdpev_on_key (GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent rdp_event;
    Display *display;
    KeyCode cooked_keycode;

    gpdata = GET_DATA (gp);
    rdp_event.type = REMMINA_PLUGIN_RDP_EVENT_TYPE_SCANCODE;
    rdp_event.key_event.up = (event->type == GDK_KEY_PRESS ? False : True);
    rdp_event.key_event.extended = False;
    switch (event->keyval)
    {
    case GDK_Pause:
        rdp_event.key_event.key_code = 0x1d;
        rdp_event.key_event.up = False;
        remmina_plugin_rdpev_event_push (gp, &rdp_event);
        rdp_event.key_event.key_code = 0x45;
        rdp_event.key_event.up = False;
        remmina_plugin_rdpev_event_push (gp, &rdp_event);
        rdp_event.key_event.key_code = 0x1d;
        rdp_event.key_event.up = True;
        remmina_plugin_rdpev_event_push (gp, &rdp_event);
        rdp_event.key_event.key_code = 0x45;
        rdp_event.key_event.up = True;
        remmina_plugin_rdpev_event_push (gp, &rdp_event);
        break;
    default:
        if (!gpdata->use_client_keymap)
        {
            rdp_event.key_event.key_code = freerdp_kbd_get_scancode_by_keycode (event->hardware_keycode, &rdp_event.key_event.extended);
            remmina_plugin_service->log_printf ("[RDP]keyval=%04X keycode=%i scancode=%i extended=%i\n",
                event->keyval, event->hardware_keycode, rdp_event.key_event.key_code, &rdp_event.key_event.extended);
        }
        else
        {
            display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
            cooked_keycode = XKeysymToKeycode(display, XKeycodeToKeysym(display, event->hardware_keycode, 0));
            rdp_event.key_event.key_code = freerdp_kbd_get_scancode_by_keycode (cooked_keycode, &rdp_event.key_event.extended);
            remmina_plugin_service->log_printf ("[RDP]keyval=%04X raw_keycode=%i cooked_keycode=%i scancode=%i extended=%i\n",
                event->keyval, event->hardware_keycode, cooked_keycode, rdp_event.key_event.key_code, &rdp_event.key_event.extended);
        }
        if (rdp_event.key_event.key_code)
        {
            remmina_plugin_rdpev_event_push (gp, &rdp_event);
        }
        break;
    }

    /* Register/unregister the pressed key */
    if (rdp_event.key_event.key_code)
    {
        if (event->type == GDK_KEY_PRESS)
        {
            g_array_append_val (gpdata->pressed_keys, rdp_event.key_event.key_code);
        }
        else
        {
            remmina_plugin_rdpev_release_key (gp, rdp_event.key_event.key_code);
        }
    }
    return TRUE;
}

void
remmina_plugin_rdpev_init (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    gint flags;
    gchar *s;
    XPixmapFormatValues *pfs;
    XPixmapFormatValues *pf;
    gint n;
    gint i;

    gpdata = GET_DATA (gp);
    gpdata->drawing_area = gtk_drawing_area_new ();
    gtk_widget_show (gpdata->drawing_area);
    gtk_container_add (GTK_CONTAINER (gp), gpdata->drawing_area);

    gtk_widget_add_events (gpdata->drawing_area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK
        | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    GTK_WIDGET_SET_FLAGS (gpdata->drawing_area, GTK_CAN_FOCUS);

    remmina_plugin_service->protocol_plugin_register_hostkey (gp, gpdata->drawing_area);

    s = remmina_plugin_service->pref_get_value ("rdp_use_client_keymap");
    gpdata->use_client_keymap = (s && s[0] == '1' ? TRUE : FALSE);
    g_free (s);

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
    gpdata->ui_queue = g_async_queue_new_full (remmina_plugin_rdpui_object_free);
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

    gpdata->object_table = g_hash_table_new_full (NULL, NULL, NULL, g_free);

    gpdata->display = GDK_DISPLAY_XDISPLAY (gdk_display_get_default ());
    gpdata->depth = DefaultDepth (gpdata->display, DefaultScreen (gpdata->display));
    gpdata->visual = GDK_VISUAL_XVISUAL (gdk_visual_get_best_with_depth (gpdata->depth));
    pfs = XListPixmapFormats (gpdata->display, &n);
    if (pfs)
    {
        for (i = 0; i < n; i++)
        {
            pf = pfs + i;
            if (pf->depth == gpdata->depth)
            {
                gpdata->bitmap_pad = pf->scanline_pad;
                gpdata->bpp = pf->bits_per_pixel;
                break;
            }
        }
        XFree(pfs);
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
    if (gpdata->ui_handler)
    {
        g_source_remove (gpdata->ui_handler);
        gpdata->ui_handler = 0;
        remmina_plugin_rdpev_queue_ui (gp);
    }

    if (gpdata->gc)
    {
        XFreeGC (gpdata->display, gpdata->gc);
        gpdata->gc = 0;
    }
    if (gpdata->gc_default)
    {
        XFreeGC (gpdata->display, gpdata->gc_default);
        gpdata->gc_default = 0;
    }
    if (gpdata->rgb_pixmap)
    {
        g_object_unref (gpdata->rgb_pixmap);
        gpdata->rgb_pixmap = NULL;
    }
    if (gpdata->gc_mono)
    {
        XFreeGC (gpdata->display, gpdata->gc_mono);
        gpdata->gc_mono = 0;
    }
    if (gpdata->bitmap_mono)
    {
        XFreePixmap (gpdata->display, gpdata->bitmap_mono);
        gpdata->bitmap_mono = 0;
    }

    g_hash_table_destroy (gpdata->object_table);

    g_array_free (gpdata->pressed_keys, TRUE);
    g_async_queue_unref (gpdata->event_queue);
    gpdata->event_queue = NULL;
    g_async_queue_unref (gpdata->ui_queue);
    gpdata->ui_queue = NULL;
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

static uint8 remmina_plugin_rdpev_rop2_map[] =
{
    GXclear,        /* 0 */
    GXnor,          /* DPon */
    GXandInverted,  /* DPna */
    GXcopyInverted, /* Pn */
    GXandReverse,   /* PDna */
    GXinvert,       /* Dn */
    GXxor,          /* DPx */
    GXnand,         /* DPan */
    GXand,          /* DPa */
    GXequiv,        /* DPxn */
    GXnoop,         /* D */
    GXorInverted,   /* DPno */
    GXcopy,         /* P */
    GXorReverse,    /* PDno */
    GXor,           /* DPo */
    GXset           /* 1 */
};

static void
remmina_plugin_rdpev_set_rop2 (RemminaPluginRdpData *gpdata, gint rop2)
{
    if ((rop2 < 0x01) || (rop2 > 0x10))
    {
        remmina_plugin_service->log_printf ("[RDP]unknown rop2 0x%x", rop2);
    }
    else
    {
        XSetFunction (gpdata->display, gpdata->gc, remmina_plugin_rdpev_rop2_map[rop2 - 1]);
    }
}

static void
remmina_plugin_rdpev_set_rop3 (RemminaPluginRdpData *gpdata, gint rop3)
{
    switch (rop3)
    {
        case 0x00: /* 0 - 0 */
            XSetFunction (gpdata->display, gpdata->gc, GXclear);
            break;
        case 0x05: /* ~(P | D) - DPon */
            XSetFunction (gpdata->display, gpdata->gc, GXnor);
            break;
        case 0x0a: /* ~P & D - DPna */
            XSetFunction (gpdata->display, gpdata->gc, GXandInverted);
            break;
        case 0x0f: /* ~P - Pn */
            XSetFunction (gpdata->display, gpdata->gc, GXcopyInverted);
            break;
        case 0x11: /* ~(S | D) - DSon */
            XSetFunction (gpdata->display, gpdata->gc, GXnor);
            break;
        case 0x22: /* ~S & D - DSna */
            XSetFunction (gpdata->display, gpdata->gc, GXandInverted);
            break;
        case 0x33: /* ~S - Sn */
            XSetFunction (gpdata->display, gpdata->gc, GXcopyInverted);
            break;
        case 0x44: /* S & ~D - SDna */
            XSetFunction (gpdata->display, gpdata->gc, GXandReverse);
            break;
        case 0x50: /* P & ~D - PDna */
            XSetFunction (gpdata->display, gpdata->gc, GXandReverse);
            break;
        case 0x55: /* ~D - Dn */
            XSetFunction (gpdata->display, gpdata->gc, GXinvert);
            break;
        case 0x5a: /* D ^ P - DPx */
            XSetFunction (gpdata->display, gpdata->gc, GXxor);
            break;
        case 0x5f: /* ~(P & D) - DPan */
            XSetFunction (gpdata->display, gpdata->gc, GXnand);
            break;
        case 0x66: /* D ^ S - DSx */
            XSetFunction (gpdata->display, gpdata->gc, GXxor);
            break;
        case 0x77: /* ~(S & D) - DSan */
            XSetFunction (gpdata->display, gpdata->gc, GXnand);
            break;
        case 0x88: /* D & S - DSa */
            XSetFunction (gpdata->display, gpdata->gc, GXand);
            break;
        case 0x99: /* ~(S ^ D) - DSxn */
            XSetFunction (gpdata->display, gpdata->gc, GXequiv);
            break;
        case 0xa0: /* P & D - DPa */
            XSetFunction (gpdata->display, gpdata->gc, GXand);
            break;
        case 0xa5: /* ~(P ^ D) - PDxn */
            XSetFunction (gpdata->display, gpdata->gc, GXequiv);
            break;
        case 0xaa: /* D - D */
            XSetFunction (gpdata->display, gpdata->gc, GXnoop);
            break;
        case 0xaf: /* ~P | D - DPno */
            XSetFunction (gpdata->display, gpdata->gc, GXorInverted);
            break;
        case 0xbb: /* ~S | D - DSno */
            XSetFunction (gpdata->display, gpdata->gc, GXorInverted);
            break;
        case 0xcc: /* S - S */
            XSetFunction (gpdata->display, gpdata->gc, GXcopy);
            break;
        case 0xdd: /* S | ~D - SDno */
            XSetFunction (gpdata->display, gpdata->gc, GXorReverse);
            break;
        case 0xee: /* D | S - DSo */
            XSetFunction (gpdata->display, gpdata->gc, GXor);
            break;
        case 0xf0: /* P - P */
            XSetFunction (gpdata->display, gpdata->gc, GXcopy);
            break;
        case 0xf5: /* P | ~D - PDno */
            XSetFunction (gpdata->display, gpdata->gc, GXorReverse);
            break;
        case 0xfa: /* P | D - DPo */
            XSetFunction (gpdata->display, gpdata->gc, GXor);
            break;
        case 0xff: /* 1 - 1 */
            XSetFunction (gpdata->display, gpdata->gc, GXset);
            break;
        default:
            remmina_plugin_service->log_printf ("[RDP]unknown rop3 0x%x", rop3);
            break;
    }
}

static void
remmina_plugin_rdpev_insert_drawable (RemminaPluginRdpData *gpdata, guint object_id, Drawable obj)
{
    Drawable *p;

    p = g_new (Drawable, 1);
    *p = obj;
    g_hash_table_insert (gpdata->object_table, (gpointer) object_id, p);
}

static Drawable
remmina_plugin_rdpev_get_drawable (RemminaPluginRdpData *gpdata, guint object_id)
{
    Drawable *p;

    p = (Drawable*) g_hash_table_lookup (gpdata->object_table, (gpointer) object_id);
    if (!p)
    {
        return 0;
    }
    return *p;
}

static void
remmina_plugin_rdpev_connected (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    XGCValues gcv = { 0 };

    gpdata = GET_DATA (gp);
    gtk_widget_realize (gpdata->drawing_area);

    gpdata->rgb_pixmap = gdk_pixmap_new (GDK_DRAWABLE (gpdata->drawing_area->window),
        gpdata->settings->width, gpdata->settings->height, gpdata->depth);
    gpdata->rgb_surface = GDK_PIXMAP_XID (gpdata->rgb_pixmap);
    gpdata->drw_surface = gpdata->rgb_surface;
    gpdata->gc = XCreateGC(gpdata->display, gpdata->rgb_surface, GCGraphicsExposures, &gcv);
    gpdata->gc_default = XCreateGC(gpdata->display, gpdata->rgb_surface, GCGraphicsExposures, &gcv);
    gpdata->bitmap_mono = XCreatePixmap (gpdata->display, GDK_WINDOW_XID (gpdata->drawing_area->window), 8, 8, 1);
    gpdata->gc_mono = XCreateGC (gpdata->display, gpdata->bitmap_mono, GCGraphicsExposures, &gcv);

    remmina_plugin_rdpev_update_scale (gp);
}

static void
remmina_plugin_rdpev_line (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    /*g_print ("line: opcode=%X srcx=%i srcy=%i x=%i y=%i fgcolor=%X\n", ui->opcode, ui->srcx, ui->srcy, ui->x, ui->y, ui->fgcolor);*/
    gpdata = GET_DATA (gp);
    remmina_plugin_rdpev_set_rop2 (gpdata, ui->opcode);
    XSetFillStyle (gpdata->display, gpdata->gc, FillSolid);
    XSetForeground (gpdata->display, gpdata->gc, ui->fgcolor);
    XDrawLine (gpdata->display, gpdata->drw_surface, gpdata->gc, ui->srcx, ui->srcy, ui->x, ui->y);
    if (gpdata->drw_surface == gpdata->rgb_surface)
    {
        remmina_plugin_rdpev_update_rect (gp,
            ui->srcx < ui->x ? ui->srcx : ui->x,
            ui->srcy < ui->y ? ui->srcy : ui->y,
            (ui->srcx < ui->x ? ui->x - ui->srcx : ui->srcx - ui->x) + 1,
            (ui->srcy < ui->y ? ui->y - ui->srcy : ui->srcy - ui->y) + 1);
    }
}

static void
remmina_plugin_rdpev_rect (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    /*g_print ("rect: opcode=%X x=%i y=%i cx=%i cy=%i fgcolor=%X\n", ui->opcode, ui->x, ui->y, ui->cx, ui->cy, ui->fgcolor);*/
    gpdata = GET_DATA (gp);
    remmina_plugin_rdpev_set_rop3 (gpdata, ui->opcode);
    XSetFillStyle (gpdata->display, gpdata->gc, FillSolid);
    XSetForeground (gpdata->display, gpdata->gc, ui->fgcolor);
    XFillRectangle (gpdata->display, gpdata->drw_surface, gpdata->gc, ui->x, ui->y, ui->cx, ui->cy);
    if (gpdata->drw_surface == gpdata->rgb_surface)
    {
        remmina_plugin_rdpev_update_rect (gp, ui->x, ui->y, ui->cx, ui->cy);
    }
}

static void
remmina_plugin_rdpev_polyline (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    XPoint *pts;
    gint x, y, x1, y1, x2, y2;
    gint i;

    /*g_print ("polyline: opcode=%X npoints=%i fgcolor=%X\n", ui->opcode, ui->width, ui->fgcolor);*/
    gpdata = GET_DATA (gp);
    remmina_plugin_rdpev_set_rop2 (gpdata, ui->opcode);
    XSetFillStyle (gpdata->display, gpdata->gc, FillSolid);
    XSetForeground (gpdata->display, gpdata->gc, ui->fgcolor);
    pts = (XPoint *) ui->data;
    XDrawLines (gpdata->display, gpdata->drw_surface, gpdata->gc, pts, ui->width, CoordModePrevious);
    if (gpdata->drw_surface == gpdata->rgb_surface)
    {
        x = x1 = x2 = pts[0].x;
        y = y1 = y2 = pts[0].y;
        for (i = 1; i < ui->width; i++)
        {
            x += pts[i].x;
            y += pts[i].y;
            x1 = (x1 < x ? x1 : x);
            x2 = (x2 > x ? x2 : x);
            y1 = (y1 < y ? y1 : y);
            y2 = (y2 > y ? y2 : y);
        }
        remmina_plugin_rdpev_update_rect (gp, x1, y1, x2 - x1 + 1, y2 - y1 + 1);
    }
}

static void
remmina_plugin_rdpev_create_surface (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap new_pix, old_pix;

    /*g_print ("create_surface: object_id=%i alt_object_id=%i\n", ui->object_id, ui->alt_object_id);*/
    gpdata = GET_DATA (gp);
    new_pix = XCreatePixmap (gpdata->display, gpdata->rgb_surface, ui->width, ui->height, gpdata->depth);
    remmina_plugin_rdpev_insert_drawable (gpdata, ui->object_id, new_pix);
    if (ui->alt_object_id)
    {
        old_pix = remmina_plugin_rdpev_get_drawable (gpdata, ui->alt_object_id);
        if (old_pix)
        {
            XCopyArea (gpdata->display, old_pix, new_pix, gpdata->gc_default, 0, 0,
                ui->width, ui->height, 0, 0);
            XFreePixmap (gpdata->display, old_pix);
            if (gpdata->drw_surface == old_pix)
            {
                gpdata->drw_surface = new_pix;
            }
        }
    }
}

static void
remmina_plugin_rdpev_set_surface (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    /*g_print ("set_surface: object_id=%i\n", ui->object_id);*/
    gpdata = GET_DATA (gp);
    if (ui->object_id)
    {
        gpdata->drw_surface = remmina_plugin_rdpev_get_drawable (gpdata, ui->object_id);
        if (!gpdata->drw_surface)
        {
            gpdata->drw_surface = gpdata->rgb_surface;
        }
    }
    else
    {
        gpdata->drw_surface = gpdata->rgb_surface;
    }
}

static void
remmina_plugin_rdpev_destroy_surface (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap pix;

    /*g_print ("destroy_surface: object_id=%i\n", ui->object_id);*/
    gpdata = GET_DATA (gp);
    pix = remmina_plugin_rdpev_get_drawable (gpdata, ui->object_id);
    if (gpdata->drw_surface == pix)
    {
        gpdata->drw_surface = gpdata->rgb_surface;
    }
    if (pix)
    {
        XFreePixmap (gpdata->display, pix);
    }
    g_hash_table_remove (gpdata->object_table, (gpointer) ui->object_id);
}

static Pixmap
remmina_plugin_rdpev_construct_bitmap (RemminaPluginRdpData *gpdata, RemminaPluginRdpUiObject *ui)
{
    XImage *image;
    Pixmap bitmap;

    bitmap = XCreatePixmap (gpdata->display, gpdata->rgb_surface, ui->width, ui->height, gpdata->depth);
    image = XCreateImage (gpdata->display, gpdata->visual, gpdata->depth, ZPixmap, 0,
        (char *) ui->data, ui->width, ui->height, gpdata->bitmap_pad, 0);
    XPutImage (gpdata->display, bitmap, gpdata->gc_default, image, 0, 0, 0, 0, ui->width, ui->height);
    XFree (image);
    return bitmap;
}

static void
remmina_plugin_rdpev_create_bitmap (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap bitmap;

    /*g_print ("create_bitmap: object_id=%i\n", ui->object_id);*/
    gpdata = GET_DATA (gp);
    bitmap = remmina_plugin_rdpev_construct_bitmap (gpdata, ui);
    remmina_plugin_rdpev_insert_drawable (gpdata, ui->object_id, bitmap);
}

static void
remmina_plugin_rdpev_paint_bitmap (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    XImage *image;

    /*g_print ("paint_bitmap: x=%i y=%i cx=%i cy=%i width=%i height=%i\n", ui->x, ui->y, ui->cx, ui->cy, ui->width, ui->height);*/
    gpdata = GET_DATA (gp);
    image = XCreateImage (gpdata->display, gpdata->visual, gpdata->depth, ZPixmap, 0,
        (char *) ui->data, ui->width, ui->height, gpdata->bitmap_pad, 0);
    XPutImage (gpdata->display, gpdata->rgb_surface, gpdata->gc_default, image, 0, 0, ui->x, ui->y, ui->cx, ui->cy);
    XFree (image);
    remmina_plugin_rdpev_update_rect (gp, ui->x, ui->y, ui->cx, ui->cy);
}

static void
remmina_plugin_rdpev_destroy_bitmap (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap bitmap;

    /*g_print ("destroy_bitmap: object_id=%i\n", ui->object_id);*/
    gpdata = GET_DATA (gp);
    bitmap = remmina_plugin_rdpev_get_drawable (gpdata, ui->object_id);
    if (bitmap)
    {
        XFreePixmap (gpdata->display, bitmap);
    }
    g_hash_table_remove (gpdata->object_table, (gpointer) ui->object_id);
}

static void
remmina_plugin_rdpev_set_clip (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    XRectangle clip_rect;

    /*g_print ("set_clip: x=%i y=%i cx=%i cy=%i\n",
        ui->x, ui->y, ui->cx, ui->cy);*/
    gpdata = GET_DATA (gp);
    clip_rect.x = ui->x;
    clip_rect.y = ui->y;
    clip_rect.width = ui->cx;
    clip_rect.height = ui->cy;
    XSetClipRectangles (gpdata->display, gpdata->gc, 0, 0, &clip_rect, 1, YXBanded);
}

static void
remmina_plugin_rdpev_reset_clip (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    /*g_print ("reset_clip:\n");*/
    gpdata = GET_DATA (gp);
    XSetClipMask (gpdata->display, gpdata->gc, None);
}

static void
remmina_plugin_rdpev_destblt (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    /*g_print ("destblt: opcode=%x x=%i y=%i cx=%i cy=%i\n",
        ui->opcode, ui->x, ui->y, ui->cx, ui->cy);*/
    gpdata = GET_DATA (gp);
    remmina_plugin_rdpev_set_rop3 (gpdata, ui->opcode);
    XSetFillStyle (gpdata->display, gpdata->gc, FillSolid);
    XFillRectangle (gpdata->display, gpdata->drw_surface, gpdata->gc, ui->x, ui->y, ui->cx, ui->cy);
    if (gpdata->drw_surface == gpdata->rgb_surface)
    {
        remmina_plugin_rdpev_update_rect (gp, ui->x, ui->y, ui->cx, ui->cy);
    }
}

static void
remmina_plugin_rdpev_screenblt (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    /*g_print ("screenblt: opcode=%x x=%i y=%i cx=%i cy=%i srcx=%i srcy=%i\n",
        ui->opcode, ui->x, ui->y, ui->cx, ui->cy, ui->srcx, ui->srcy);*/
    gpdata = GET_DATA (gp);
    remmina_plugin_rdpev_set_rop3 (gpdata, ui->opcode);
    XCopyArea (gpdata->display, gpdata->rgb_surface, gpdata->drw_surface, gpdata->gc,
        ui->srcx, ui->srcy, ui->cx, ui->cy, ui->x, ui->y);
    if (gpdata->drw_surface == gpdata->rgb_surface)
    {
        remmina_plugin_rdpev_update_rect (gp, ui->x, ui->y, ui->cx, ui->cy);
    }
}

static void
remmina_plugin_rdpev_memblt (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap pix;

    /*g_print ("memblt: object_id=%i opcode=%x x=%i y=%i cx=%i cy=%i srcx=%i srcy=%i\n",
        ui->object_id, ui->opcode, ui->x, ui->y, ui->cx, ui->cy, ui->srcx, ui->srcy);*/
    gpdata = GET_DATA (gp);
    remmina_plugin_rdpev_set_rop3 (gpdata, ui->opcode);
    pix = remmina_plugin_rdpev_get_drawable (gpdata, ui->object_id);
    if (pix)
    {
        XCopyArea (gpdata->display, pix, gpdata->drw_surface, gpdata->gc,
            ui->srcx, ui->srcy, ui->cx, ui->cy, ui->x, ui->y);
        if (gpdata->drw_surface == gpdata->rgb_surface)
        {
            remmina_plugin_rdpev_update_rect (gp, ui->x, ui->y, ui->cx, ui->cy);
        }
    }
}

static Pixmap
remmina_plugin_rdpev_construct_glyph (RemminaPluginRdpData *gpdata, RemminaPluginRdpUiObject *ui)
{
    XImage *image;
    Pixmap glyph;
    gint scanline;

    scanline = (ui->width + 7) / 8;
    glyph = XCreatePixmap (gpdata->display, gpdata->rgb_surface, ui->width, ui->height, 1);
    image = XCreateImage (gpdata->display, gpdata->visual, 1, ZPixmap, 0, (char *) ui->data,
        ui->width, ui->height, 8, scanline);
    image->byte_order = MSBFirst;
    image->bitmap_bit_order = MSBFirst;
    XInitImage (image);
    XPutImage (gpdata->display, glyph, gpdata->gc_mono, image, 0, 0, 0, 0, ui->width, ui->height);
    XFree (image);
    return glyph;
}

static void
remmina_plugin_rdpev_create_glyph (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap glyph;

    /*g_print ("create_glyph: object_id=%i width=%i height=%i\n", ui->object_id, ui->width, ui->height);*/
    gpdata = GET_DATA (gp);
    glyph = remmina_plugin_rdpev_construct_glyph (gpdata, ui);
    remmina_plugin_rdpev_insert_drawable (gpdata, ui->object_id, glyph);
}

static void
remmina_plugin_rdpev_destroy_glyph (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap glyph;

    /*g_print ("destroy_glyph: object_id=%i\n", ui->object_id);*/
    gpdata = GET_DATA (gp);
    glyph = remmina_plugin_rdpev_get_drawable (gpdata, ui->object_id);
    if (glyph)
    {
        XFreePixmap (gpdata->display, glyph);
    }
    g_hash_table_remove (gpdata->object_table, (gpointer) ui->object_id);
}

static void
remmina_plugin_rdpev_start_draw_glyphs (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    /*g_print ("start_draw_glyphs: bgcolor=%X fgcolor=%X\n", ui->bgcolor, ui->fgcolor);*/
    gpdata = GET_DATA (gp);
    XSetFunction (gpdata->display, gpdata->gc, GXcopy);
    XSetForeground (gpdata->display, gpdata->gc, ui->fgcolor);
    XSetBackground (gpdata->display, gpdata->gc, ui->bgcolor);
    XSetFillStyle (gpdata->display, gpdata->gc, FillStippled);
}

static void
remmina_plugin_rdpev_draw_glyph (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap glyph;

    /*g_print ("draw_glyph: object_id=%i x=%i y=%i cx=%i cy=%i\n", ui->object_id, ui->x, ui->y, ui->cx, ui->cy);*/
    gpdata = GET_DATA (gp);
    glyph = remmina_plugin_rdpev_get_drawable (gpdata, ui->object_id);
    if (glyph)
    {
        XSetStipple (gpdata->display, gpdata->gc, glyph);
        XSetTSOrigin (gpdata->display, gpdata->gc, ui->x, ui->y);
        XFillRectangle (gpdata->display, gpdata->drw_surface, gpdata->gc, ui->x, ui->y, ui->cx, ui->cy);
        XSetStipple (gpdata->display, gpdata->gc, gpdata->bitmap_mono);
    }
}

static void
remmina_plugin_rdpev_end_draw_glyphs (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    /*g_print ("end_draw_glyphs: x=%i y=%i cx=%i cy=%i\n", ui->x, ui->y, ui->cx, ui->cy);*/
    gpdata = GET_DATA (gp);
    if (gpdata->drw_surface == gpdata->rgb_surface)
    {
        remmina_plugin_rdpev_update_rect (gp, ui->x, ui->y, ui->cx, ui->cy);
    }
}

static void
remmina_plugin_rdpev_patblt_glyph (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap fill;

    /*g_print ("patblt_glyph: opcode=%x x=%i y=%i cx=%i cy=%i\n", ui->opcode, ui->x, ui->y, ui->cx, ui->cy);*/
    gpdata = GET_DATA (gp);
    remmina_plugin_rdpev_set_rop3 (gpdata, ui->opcode);
    fill = remmina_plugin_rdpev_construct_glyph (gpdata, ui);
    XSetForeground (gpdata->display, gpdata->gc, ui->fgcolor);
    XSetBackground (gpdata->display, gpdata->gc, ui->bgcolor);
    XSetFillStyle (gpdata->display, gpdata->gc, FillOpaqueStippled);
    XSetStipple (gpdata->display, gpdata->gc, fill);
    XSetTSOrigin (gpdata->display, gpdata->gc, ui->srcx, ui->srcy);
    XFillRectangle (gpdata->display, gpdata->drw_surface, gpdata->gc, ui->x, ui->y, ui->cx, ui->cy);
    XSetStipple (gpdata->display, gpdata->gc, gpdata->bitmap_mono);
    XFreePixmap (gpdata->display, fill);
    if (gpdata->drw_surface == gpdata->rgb_surface)
    {
        remmina_plugin_rdpev_update_rect (gp, ui->x, ui->y, ui->cx, ui->cy);
    }
}

static void
remmina_plugin_rdpev_patblt_bitmap (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    Pixmap fill;

    /*g_print ("patblt_bitmap: opcode=%x x=%i y=%i cx=%i cy=%i\n", ui->opcode, ui->x, ui->y, ui->cx, ui->cy);*/
    gpdata = GET_DATA (gp);
    remmina_plugin_rdpev_set_rop3 (gpdata, ui->opcode);
    fill = remmina_plugin_rdpev_construct_bitmap (gpdata, ui);
    XSetFillStyle (gpdata->display, gpdata->gc, FillTiled);
    XSetTile (gpdata->display, gpdata->gc, fill);
    XSetTSOrigin (gpdata->display, gpdata->gc, ui->srcx, ui->srcy);
    XFillRectangle (gpdata->display, gpdata->drw_surface, gpdata->gc, ui->x, ui->y, ui->cx, ui->cy);
    XSetTile (gpdata->display, gpdata->gc, gpdata->rgb_surface);
    XFreePixmap (gpdata->display, fill);
    if (gpdata->drw_surface == gpdata->rgb_surface)
    {
        remmina_plugin_rdpev_update_rect (gp, ui->x, ui->y, ui->cx, ui->cy);
    }
}

static void
remmina_plugin_rdpev_set_cursor (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    GdkCursor *cur;

    gpdata = GET_DATA (gp);
    if (ui->pixbuf)
    {
        cur = gdk_cursor_new_from_pixbuf (gdk_display_get_default (),
            ui->pixbuf, ui->x, ui->y);
        gdk_window_set_cursor (gtk_widget_get_window (gpdata->drawing_area), cur);
        gdk_cursor_unref (cur);
    }
    else
    {
        cur = gdk_cursor_new (GDK_BLANK_CURSOR);
        gdk_window_set_cursor (gtk_widget_get_window (gpdata->drawing_area), cur);
        gdk_cursor_unref (cur);
    }
}

static void
remmina_plugin_rdpev_set_default_cursor (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    gdk_window_set_cursor (gtk_widget_get_window (gpdata->drawing_area), NULL);
}

gboolean
remmina_plugin_rdpev_queue_ui (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gpdata = GET_DATA (gp);
    
    ui = (RemminaPluginRdpUiObject *) g_async_queue_try_pop (gpdata->ui_queue);
    if (ui)
    {
        switch (ui->type)
        {
        case REMMINA_PLUGIN_RDP_UI_CONNECTED:
            remmina_plugin_rdpev_connected (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_LINE:
            remmina_plugin_rdpev_line (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_RECT:
            remmina_plugin_rdpev_rect (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_POLYLINE:
            remmina_plugin_rdpev_polyline (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_CREATE_SURFACE:
            remmina_plugin_rdpev_create_surface (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_SET_SURFACE:
            remmina_plugin_rdpev_set_surface (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_DESTROY_SURFACE:
            remmina_plugin_rdpev_destroy_surface (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_CREATE_BITMAP:
            remmina_plugin_rdpev_create_bitmap (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_PAINT_BITMAP:
            remmina_plugin_rdpev_paint_bitmap (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_DESTROY_BITMAP:
            remmina_plugin_rdpev_destroy_bitmap (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_SET_CLIP:
            remmina_plugin_rdpev_set_clip (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_RESET_CLIP:
            remmina_plugin_rdpev_reset_clip (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_DESTBLT:
            remmina_plugin_rdpev_destblt (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_SCREENBLT:
            remmina_plugin_rdpev_screenblt (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_MEMBLT:
            remmina_plugin_rdpev_memblt (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_CREATE_GLYPH:
            remmina_plugin_rdpev_create_glyph (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_DESTROY_GLYPH:
            remmina_plugin_rdpev_destroy_glyph (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_START_DRAW_GLYPHS:
            remmina_plugin_rdpev_start_draw_glyphs (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_DRAW_GLYPH:
            remmina_plugin_rdpev_draw_glyph (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_END_DRAW_GLYPHS:
            remmina_plugin_rdpev_end_draw_glyphs (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_PATBLT_GLYPH:
            remmina_plugin_rdpev_patblt_glyph (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_PATBLT_BITMAP:
            remmina_plugin_rdpev_patblt_bitmap (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_SET_CURSOR:
            remmina_plugin_rdpev_set_cursor (gp, ui);
            break;
        case REMMINA_PLUGIN_RDP_UI_SET_DEFAULT_CURSOR:
            remmina_plugin_rdpev_set_default_cursor (gp, ui);
            break;
        default:
            break;
        }
        remmina_plugin_rdpui_object_free (ui);
        return TRUE;
    }
    else
    {
        LOCK_BUFFER (FALSE)
        gpdata->ui_handler = 0;
        UNLOCK_BUFFER (FALSE)
        return FALSE;
    }
}

void
remmina_plugin_rdpev_unfocus (RemminaProtocolWidget *gp)
{
    remmina_plugin_rdpev_release_key (gp, 0);
}

