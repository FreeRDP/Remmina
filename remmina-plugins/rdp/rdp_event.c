/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Jay Sorg
 * Copyright (C) 2010-2011 Vic Lee
 * Copyright (C) 2011 Marc-Andre Moreau <marcandre.moreau@gmail.com>
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

#include "rdp_plugin.h"
#include "rdp_event.h"
#include "rdp_gdi.h"
#include <gdk/gdkkeysyms.h>
#include <cairo/cairo-xlib.h>
#include <freerdp/kbd/kbd.h>

static void
remmina_rdp_event_event_push (RemminaProtocolWidget *gp, const RemminaPluginRdpEvent *e)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent *event;

    gpdata = GET_DATA (gp);
    if (gpdata->event_queue)
    {
        event = g_memdup (e, sizeof(RemminaPluginRdpEvent));
        g_async_queue_push (gpdata->event_queue, event);
        if (write (gpdata->event_pipe[1], "\0", 1))
        {
        }
    }
}

static void
remmina_rdp_event_release_key (RemminaProtocolWidget *gp, gint scancode)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent rdp_event = { 0 };
    gint k;
    gint i;

    gpdata = GET_DATA (gp);
    rdp_event.type = REMMINA_RDP_EVENT_TYPE_SCANCODE;
    if (scancode == 0)
    {
        /* Send all release key events for previously pressed keys */
        rdp_event.key_event.up = True;
        for (i = 0; i < gpdata->pressed_keys->len; i++)
        {
            rdp_event.key_event.key_code = g_array_index (gpdata->pressed_keys, gint, i);
            remmina_rdp_event_event_push (gp, &rdp_event);
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
remmina_rdp_event_scale_area (RemminaProtocolWidget *gp, gint *x, gint *y, gint *w, gint *h)
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
remmina_rdp_event_update_rect (RemminaProtocolWidget *gp, gint x, gint y, gint w, gint h)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    if (remmina_plugin_service->protocol_plugin_get_scale (gp))
    {
        remmina_rdp_event_scale_area (gp, &x, &y, &w, &h);
    }
    gtk_widget_queue_draw_area (gpdata->drawing_area, x, y, w, h);
}

static gboolean
remmina_rdp_event_update_scale_factor (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaFile *remminafile;
    GtkAllocation a;
    gint width, height;
    gint gpwidth, gpheight;
    gboolean scale;
    gint hscale, vscale;

    gpdata = GET_DATA (gp);
    remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);

    gtk_widget_get_allocation (GTK_WIDGET (gp), &a);
    width = a.width;
    height = a.height;
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
remmina_rdp_event_on_draw (GtkWidget *widget, cairo_t *context, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    gboolean scale;

    gpdata = GET_DATA (gp);

    if (!gpdata->rgb_cairo_surface) return FALSE;

    scale = remmina_plugin_service->protocol_plugin_get_scale (gp);

    cairo_rectangle (context, 0, 0, gtk_widget_get_allocated_width (widget),
        gtk_widget_get_allocated_height (widget));
    if (scale)
    {
        cairo_scale (context, gpdata->scale_x, gpdata->scale_y);
    }
    cairo_set_source_surface (context, gpdata->rgb_cairo_surface, 0, 0);
    cairo_fill (context);

    return TRUE;
}

static gboolean
remmina_rdp_event_on_configure (GtkWidget *widget, GdkEventConfigure *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    /* We do a delayed reallocating to improve performance */
    if (gpdata->scale_handler) g_source_remove (gpdata->scale_handler);
    gpdata->scale_handler = g_timeout_add (1000, (GSourceFunc) remmina_rdp_event_update_scale_factor, gp);
    return FALSE;
}

static void
remmina_rdp_event_translate_pos (RemminaProtocolWidget *gp, int ix, int iy, uint16 *ox, uint16 *oy)
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
remmina_rdp_event_on_motion (GtkWidget *widget, GdkEventMotion *event, RemminaProtocolWidget *gp)
{
    RemminaPluginRdpEvent rdp_event = { 0 };

    rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;
    rdp_event.mouse_event.flags = PTR_FLAGS_MOVE;
    remmina_rdp_event_translate_pos (gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);
    remmina_rdp_event_event_push (gp, &rdp_event);
    return TRUE;
}

static gboolean
remmina_rdp_event_on_button (GtkWidget *widget, GdkEventButton *event, RemminaProtocolWidget *gp)
{
    gint flag;
    RemminaPluginRdpEvent rdp_event = { 0 };

    /* We only accept 3 buttons */
    if (event->button < 1 || event->button > 3) return FALSE;
    /* We bypass 2button-press and 3button-press events */
    if (event->type != GDK_BUTTON_PRESS && event->type != GDK_BUTTON_RELEASE) return TRUE;

    rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;
    remmina_rdp_event_translate_pos (gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);

    flag = 0;
    if (event->type == GDK_BUTTON_PRESS)
    {
        flag = PTR_FLAGS_DOWN;
    }
    switch (event->button)
    {
        case 1:
            flag |= PTR_FLAGS_BUTTON1;
            break;
        case 2:
            flag |= PTR_FLAGS_BUTTON3;
            break;
        case 3:
            flag |= PTR_FLAGS_BUTTON2;
            break;
    }
    if (flag != 0)
    {
        rdp_event.mouse_event.flags = flag;
        remmina_rdp_event_event_push (gp, &rdp_event);
    }
    return TRUE;
}

static gboolean
remmina_rdp_event_on_scroll (GtkWidget *widget, GdkEventScroll *event, RemminaProtocolWidget *gp)
{
    gint flag;
    RemminaPluginRdpEvent rdp_event = { 0 };

    rdp_event.type = REMMINA_RDP_EVENT_TYPE_MOUSE;
    flag = 0;
    switch (event->direction)
    {
    case GDK_SCROLL_UP:
        flag = PTR_FLAGS_WHEEL | 0x0078;
        break;
    case GDK_SCROLL_DOWN:
        flag = PTR_FLAGS_WHEEL | PTR_FLAGS_WHEEL_NEGATIVE | 0x0088;
        break;
    default:
        return FALSE;
    }

    rdp_event.mouse_event.flags = flag;
    remmina_rdp_event_translate_pos (gp, event->x, event->y, &rdp_event.mouse_event.x, &rdp_event.mouse_event.y);
    remmina_rdp_event_event_push (gp, &rdp_event);
    return TRUE;
}

static gboolean
remmina_rdp_event_on_key (GtkWidget *widget, GdkEventKey *event, RemminaProtocolWidget *gp)
{
    Display *display;
    KeyCode cooked_keycode;
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpEvent rdp_event;

    gpdata = GET_DATA (gp);
    rdp_event.type = REMMINA_RDP_EVENT_TYPE_SCANCODE;
    rdp_event.key_event.up = (event->type == GDK_KEY_PRESS ? False : True);
    rdp_event.key_event.extended = False;

    switch (event->keyval)
    {
    case GDK_KEY_Pause:
        rdp_event.key_event.key_code = 0x1d;
        rdp_event.key_event.up = False;
        remmina_rdp_event_event_push (gp, &rdp_event);
        rdp_event.key_event.key_code = 0x45;
        rdp_event.key_event.up = False;
        remmina_rdp_event_event_push (gp, &rdp_event);
        rdp_event.key_event.key_code = 0x1d;
        rdp_event.key_event.up = True;
        remmina_rdp_event_event_push (gp, &rdp_event);
        rdp_event.key_event.key_code = 0x45;
        rdp_event.key_event.up = True;
        remmina_rdp_event_event_push (gp, &rdp_event);
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
            remmina_rdp_event_event_push (gp, &rdp_event);
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
            remmina_rdp_event_release_key (gp, rdp_event.key_event.key_code);
        }
    }

    return TRUE;
}

void
remmina_rdp_event_init (RemminaProtocolWidget *gp)
{
    gint n;
    gint i;
    gchar *s;
    gint flags;
    XPixmapFormatValues *pf;
    XPixmapFormatValues *pfs;
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    gpdata->drawing_area = gtk_drawing_area_new ();
    gtk_widget_show (gpdata->drawing_area);
    gtk_container_add (GTK_CONTAINER (gp), gpdata->drawing_area);

    gtk_widget_add_events (gpdata->drawing_area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK
        | GDK_BUTTON_RELEASE_MASK | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    gtk_widget_set_can_focus (gpdata->drawing_area, TRUE);

    remmina_plugin_service->protocol_plugin_register_hostkey (gp, gpdata->drawing_area);

    s = remmina_plugin_service->pref_get_value ("rdp_use_client_keymap");
    gpdata->use_client_keymap = (s && s[0] == '1' ? TRUE : FALSE);
    g_free (s);

    g_signal_connect (G_OBJECT (gpdata->drawing_area), "draw",
        G_CALLBACK (remmina_rdp_event_on_draw), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "configure-event",
        G_CALLBACK (remmina_rdp_event_on_configure), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "motion-notify-event",
        G_CALLBACK (remmina_rdp_event_on_motion), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "button-press-event",
        G_CALLBACK (remmina_rdp_event_on_button), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "button-release-event",
        G_CALLBACK (remmina_rdp_event_on_button), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "scroll-event",
        G_CALLBACK (remmina_rdp_event_on_scroll), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "key-press-event",
        G_CALLBACK (remmina_rdp_event_on_key), gp);
    g_signal_connect (G_OBJECT (gpdata->drawing_area), "key-release-event",
        G_CALLBACK (remmina_rdp_event_on_key), gp);

    gpdata->pressed_keys = g_array_new (FALSE, TRUE, sizeof (gint));
    gpdata->event_queue = g_async_queue_new_full (g_free);
    gpdata->ui_queue = g_async_queue_new ();

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
remmina_rdp_event_pre_connect (RemminaProtocolWidget *gp)
{
}

void
remmina_rdp_event_post_connect (RemminaProtocolWidget *gp)
{
}

void
remmina_rdp_event_uninit (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

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
    }
    while ((ui = (RemminaPluginRdpUiObject *) g_async_queue_try_pop (gpdata->ui_queue)) != NULL)
    {
        rf_object_free (gp, ui);
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
    if (gpdata->rgb_cairo_surface)
    {
        cairo_surface_destroy (gpdata->rgb_cairo_surface);
        gpdata->rgb_cairo_surface = NULL;
    }
    if (gpdata->rgb_surface)
    {
        XFreePixmap (gpdata->display, gpdata->rgb_surface);
        gpdata->rgb_surface = 0;
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
remmina_rdp_event_update_scale (RemminaProtocolWidget *gp)
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
        gtk_widget_set_size_request (gpdata->drawing_area,
            (hscale > 0 ? width * hscale / 100 : -1),
            (vscale > 0 ? height * vscale / 100 : -1));
    }
    else
    {
        gtk_widget_set_size_request (gpdata->drawing_area, width, height);
    }
    remmina_plugin_service->protocol_plugin_emit_signal (gp, "update-align");
}

static uint8 remmina_rdp_event_rop2_map[] =
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
remmina_rdp_event_set_rop2 (RemminaPluginRdpData *gpdata, gint rop2)
{
    if ((rop2 < 0x01) || (rop2 > 0x10))
    {
        remmina_plugin_service->log_printf ("[RDP]unknown rop2 0x%x", rop2);
    }
    else
    {
        XSetFunction (gpdata->display, gpdata->gc, remmina_rdp_event_rop2_map[rop2 - 1]);
    }
}

static void
remmina_rdp_event_set_rop3 (RemminaPluginRdpData *gpdata, gint rop3)
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
remmina_rdp_event_insert_drawable (RemminaPluginRdpData *gpdata, guint object_id, Drawable obj)
{
    Drawable *p;

    p = g_new (Drawable, 1);
    *p = obj;
    g_hash_table_insert (gpdata->object_table, GINT_TO_POINTER (object_id), p);
}

static Drawable
remmina_rdp_event_get_drawable (RemminaPluginRdpData *gpdata, guint object_id)
{
    Drawable *p;

    p = (Drawable*) g_hash_table_lookup (gpdata->object_table, GINT_TO_POINTER (object_id));
    if (!p)
    {
        return 0;
    }
    return *p;
}

static void
remmina_rdp_event_connected (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    XGCValues gcv = { 0 };

    gpdata = GET_DATA (gp);
    gtk_widget_realize (gpdata->drawing_area);

    gpdata->rgb_surface = XCreatePixmap (gpdata->display, GDK_WINDOW_XID (gtk_widget_get_window (gpdata->drawing_area)),
        gpdata->settings->width, gpdata->settings->height, gpdata->depth);
    gpdata->rgb_cairo_surface = cairo_xlib_surface_create (gpdata->display, gpdata->rgb_surface, gpdata->visual,
        gpdata->settings->width, gpdata->settings->height);
    gpdata->drw_surface = gpdata->rgb_surface;
    gpdata->gc = XCreateGC(gpdata->display, gpdata->rgb_surface, GCGraphicsExposures, &gcv);
    gpdata->gc_default = XCreateGC(gpdata->display, gpdata->rgb_surface, GCGraphicsExposures, &gcv);
    gpdata->bitmap_mono = XCreatePixmap (gpdata->display, GDK_WINDOW_XID (gtk_widget_get_window (gpdata->drawing_area)),
        8, 8, 1);
    gpdata->gc_mono = XCreateGC (gpdata->display, gpdata->bitmap_mono, GCGraphicsExposures, &gcv);

    remmina_rdp_event_update_scale (gp);
}

static void
remmina_rdp_event_rfx (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    XImage *image;
    gint i, tx, ty;
    RFX_MESSAGE *message;
    RemminaPluginRdpData *gpdata;

    gpdata = GET_DATA (gp);
    message = ui->rfx.message;

    XSetFunction(gpdata->display, gpdata->gc, GXcopy);
    XSetFillStyle(gpdata->display, gpdata->gc, FillSolid);

    XSetClipRectangles(gpdata->display, gpdata->gc, ui->rfx.left, ui->rfx.top,
        (XRectangle*) message->rects, message->num_rects, YXBanded);

    /* Draw the tiles to primary surface, each is 64x64. */
    for (i = 0; i < message->num_tiles; i++)
    {
        image = XCreateImage(gpdata->display, gpdata->visual, 24, ZPixmap, 0,
            (char*) message->tiles[i]->data, 64, 64, 32, 0);

        tx = message->tiles[i]->x + ui->rfx.left;
        ty = message->tiles[i]->y + ui->rfx.top;

        XPutImage(gpdata->display, gpdata->rgb_surface, gpdata->gc, image, 0, 0, tx, ty, 64, 64);
        XFree(image);

        remmina_rdp_event_update_rect (gp, tx, ty, message->rects[i].width, message->rects[i].height);
    }

    XSetClipMask(gpdata->display, gpdata->gc, None);
}

static void
remmina_rdp_event_nocodec (RemminaProtocolWidget *gp, RemminaPluginRdpUiObject *ui)
{
    RemminaPluginRdpData *gpdata;
    XImage *image;

    gpdata = GET_DATA (gp);

    XSetFunction(gpdata->display, gpdata->gc, GXcopy);
    XSetFillStyle(gpdata->display, gpdata->gc, FillSolid);

    image = XCreateImage(gpdata->display, gpdata->visual, 24, ZPixmap, 0,
        (char*) ui->nocodec.bitmap, ui->nocodec.width, ui->nocodec.height, 32, 0);

    XPutImage(gpdata->display, gpdata->rgb_surface, gpdata->gc, image, 0, 0,
        ui->nocodec.left, ui->nocodec.top,
        ui->nocodec.width, ui->nocodec.height);

    remmina_rdp_event_update_rect (gp,
        ui->nocodec.left, ui->nocodec.top,
        ui->nocodec.width, ui->nocodec.height);

    XSetClipMask(gpdata->display, gpdata->gc, None);
}

gboolean
remmina_rdp_event_queue_ui (RemminaProtocolWidget *gp)
{
    RemminaPluginRdpData *gpdata;
    RemminaPluginRdpUiObject *ui;

    gpdata = GET_DATA (gp);
    
    ui = (RemminaPluginRdpUiObject *) g_async_queue_try_pop (gpdata->ui_queue);
    if (ui)
    {
        switch (ui->type)
        {
        case REMMINA_RDP_UI_CONNECTED:
            remmina_rdp_event_connected (gp, ui);
            break;
        case REMMINA_RDP_UI_RFX:
            remmina_rdp_event_rfx (gp, ui);
            break;
        case REMMINA_RDP_UI_NOCODEC:
            remmina_rdp_event_nocodec (gp, ui);
            break;
        default:
            break;
        }
        rf_object_free (gp, ui);
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
remmina_rdp_event_unfocus (RemminaProtocolWidget *gp)
{
    remmina_rdp_event_release_key (gp, 0);
}

