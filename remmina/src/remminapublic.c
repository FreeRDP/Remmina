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

#include "config.h"
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif
#ifdef GDK_WINDOWING_X11
#include <gdk/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#endif
#ifdef HAVE_GTK_PRINTER
#include <gtk/gtkprinter.h>
#endif
#include "remminapublic.h"

GtkWidget*
remmina_public_create_combo_entry (const gchar *text, const gchar *def, gboolean descending)
{
    GtkWidget *combo;
    gboolean found;
    gchar *buf, *ptr1, *ptr2;
    gint i;

    combo = gtk_combo_box_entry_new_text ();
    found = FALSE;

    if (text && text[0] != '\0')
    {
        buf = g_strdup (text);
        ptr1 = buf;
        i = 0;
        while (ptr1 && *ptr1 != '\0')
        {
            ptr2 = strchr (ptr1, STRING_DELIMITOR);
            if (ptr2) *ptr2++ = '\0';

            if (descending)
            {
                gtk_combo_box_prepend_text (GTK_COMBO_BOX (combo), ptr1);
                if (!found && g_strcmp0 (ptr1, def) == 0)
                {
                    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
                    found = TRUE;
                }
            }
            else
            {
                gtk_combo_box_append_text (GTK_COMBO_BOX (combo), ptr1);
                if (!found && g_strcmp0 (ptr1, def) == 0)
                {
                    gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
                    found = TRUE;
                }
            }

            ptr1 = ptr2;
            i++;
        }

        g_free (buf);
    }

    if (!found && def && def[0] != '\0')
    {
        gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (combo))), def);
    }

    return combo;
}

GtkWidget*
remmina_public_create_combo_text_d (const gchar *text, const gchar *def, const gchar *empty_choice)
{
    GtkWidget *combo;
    GtkListStore *store;
    GtkCellRenderer *text_renderer;

    store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
    combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL(store)); 

    text_renderer = gtk_cell_renderer_text_new (); 
    gtk_cell_layout_pack_end (GTK_CELL_LAYOUT(combo), text_renderer, TRUE); 
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(combo), text_renderer, "text", 1);

    remmina_public_load_combo_text_d (combo, text, def, empty_choice);

    return combo;
}

void
remmina_public_load_combo_text_d (GtkWidget *combo, const gchar *text, const gchar *def, const gchar *empty_choice)
{
    GtkListStore *store;
    GtkTreeIter iter;
    gint i;
    gchar *buf, *ptr1, *ptr2;

    store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)));
    gtk_list_store_clear (store);

    i = 0;

    if (empty_choice)
    {
        gtk_list_store_append (store, &iter); 
        gtk_list_store_set (store, &iter, 0, "", 1, empty_choice, -1);
        gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
        i++;
    }

    if (text == NULL || text[0] == '\0') return;

    buf = g_strdup (text);
    ptr1 = buf;
    while (ptr1 && *ptr1 != '\0')
    {
        ptr2 = strchr (ptr1, STRING_DELIMITOR);
        if (ptr2) *ptr2++ = '\0';

        gtk_list_store_append (store, &iter); 
        gtk_list_store_set (store, &iter, 0, ptr1, 1, ptr1, -1);

        if (i == 0 || g_strcmp0 (ptr1, def) == 0)
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i);
        }

        i++;
        ptr1 = ptr2;
    }

    g_free (buf);
}

GtkWidget*
remmina_public_create_combo_map (const gpointer *key_value_list, const gchar *def, gboolean use_icon)
{
    gint i;
    GtkWidget *combo;
    GtkListStore *store; 
    GtkTreeIter iter; 
    GtkCellRenderer *renderer;

    if (use_icon)
    {
        store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
    }
    else
    {
        store = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_STRING);
    }
    combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL(store)); 

    if (use_icon)
    {
        renderer = gtk_cell_renderer_pixbuf_new (); 
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), renderer, FALSE); 
        gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(combo), renderer, "icon-name", 2);
    }
    renderer = gtk_cell_renderer_text_new (); 
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combo), renderer, TRUE); 
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT(combo), renderer, "text", 1);
    if (use_icon) g_object_set (G_OBJECT (renderer), "xpad", 5, NULL);

    for (i = 0; key_value_list[i]; i += (use_icon ? 3 : 2))
    {
        gtk_list_store_append (store, &iter); 
        gtk_list_store_set (store, &iter, 0, key_value_list[i], 1, _(key_value_list[i + 1]), -1);
        if (use_icon)
        {
            gtk_list_store_set (store, &iter, 2, key_value_list[i + 2], -1);
        }
        if (i == 0 || g_strcmp0 (key_value_list[i], def) == 0)
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (combo), i / (use_icon ? 3 : 2));
        }
    }
    return combo;
}

GtkWidget*
remmina_public_create_combo_mapint (const gpointer *key_value_list, gint def, gboolean use_icon)
{
    gchar buf[20];
    g_snprintf (buf, sizeof (buf), "%i", def);
    return remmina_public_create_combo_map (key_value_list, buf, use_icon);
}

void
remmina_public_create_group (GtkTable* table, const gchar *group, gint row, gint rows, gint cols)
{
    GtkWidget *widget;
    gchar *str;
  
    widget = gtk_label_new (NULL);
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    str = g_markup_printf_escaped ("<b>%s</b>", group);
    gtk_label_set_markup (GTK_LABEL (widget), str);
    g_free (str);
    gtk_table_attach_defaults (table, widget, 0, cols, row, row + 1);

    widget = gtk_label_new (NULL);
    gtk_widget_show (widget);
    gtk_widget_set_size_request (widget, 15, -1);
    gtk_table_attach (table, widget, 0, 1, row + 1, row + rows, 0, 0, 0, 0);
}

void
remmina_public_popup_position (GtkMenu *menu, gint *x, gint *y, gboolean *push_in, gpointer user_data)
{
    GtkWidget *widget;
    gint tx, ty;

    widget = GTK_WIDGET (user_data);
    if (widget->window == NULL)
    {
        *x = 0; *y = 0; *push_in = TRUE; return;
    }
    gdk_window_get_origin (widget->window, &tx, &ty);
    if (GTK_WIDGET_NO_WINDOW (widget))
    {
        tx += widget->allocation.x;
        ty += widget->allocation.y;
    }

    *x = tx;
    *y = ty + widget->allocation.height - 1;
    *push_in = TRUE;
}

gchar*
remmina_public_combine_path (const gchar *path1, const gchar *path2)
{
    if (!path1 || path1[0] == '\0') return g_strdup (path2);
    if (path1[strlen (path1) - 1] == '/') return g_strdup_printf ("%s%s", path1, path2);
    return g_strdup_printf ("%s/%s", path1, path2);
}

void
remmina_public_threads_leave (void* data)
{
    gdk_threads_leave();
}

void
remmina_public_get_server_port (const gchar *server, gint defaultport, gchar **host, gint *port)
{
    gchar *str, *ptr;

    str = g_strdup (server);
    ptr = strchr (str, ':');
    if (ptr)
    {
        *ptr++ = '\0';
        if (port) *port = atoi (ptr);
    }
    else
    {
        if (port) *port = defaultport;
    }
    if (host) *host = str;
}

gboolean
remmina_public_get_xauth_cookie (const gchar *display, gchar **msg)
{
    gchar buf[200];
    gchar *out = NULL;
    gchar *ptr;
    GError *error = NULL;
    gboolean ret;

    g_snprintf (buf, sizeof (buf), "xauth list %s", display);
    ret = g_spawn_command_line_sync (buf, &out, NULL, NULL, &error);
    if (ret)
    {
        if ((ptr = g_strrstr (out, "MIT-MAGIC-COOKIE-1")) == NULL)
        {
            *msg = g_strdup_printf ("xauth returns %s", out);
            ret = FALSE;
        }
        else
        {
            ptr += 19; while (*ptr == ' ') ptr++;
            *msg = g_strndup (ptr, 32);
        }
        g_free (out);
    }
    else
    {
        *msg = g_strdup (error->message);
    }
    return ret;
}

gint
remmina_public_open_xdisplay (const gchar *disp)
{
    gchar *display;
    gchar *ptr;
    gint port;
    struct sockaddr_un addr;
    gint sock = -1;

    display = g_strdup (disp);
    ptr = g_strrstr (display, ":");
    if (ptr)
    {
        *ptr++ = '\0';
        /* Assume you are using a local display... might need to implement remote display in the future */
        if (display[0] == '\0' || strcmp (display, "unix") == 0)
        {
            port = atoi (ptr);
            sock = socket (AF_UNIX, SOCK_STREAM, 0);
            if (sock >= 0)
            {
                memset (&addr, 0, sizeof (addr));
                addr.sun_family = AF_UNIX;
                snprintf (addr.sun_path, sizeof(addr.sun_path), X_UNIX_SOCKET, port);
                if (connect (sock, (struct sockaddr *) &addr, sizeof (addr)) == -1)
                {
                    close (sock);
                    sock = -1;
                }
            }
        }
    }

    g_free (display);
    return sock;
}

gint
remmina_public_get_available_xdisplay (void)
{
    gint i;
    gint display = 0;
    gchar fn[MAX_PATH_LEN];

    for (i = 1; i < MAX_X_DISPLAY_NUMBER; i++)
    {
        g_snprintf (fn, sizeof (fn), X_UNIX_SOCKET, i);
        if (!g_file_test (fn, G_FILE_TEST_EXISTS))
        {
            display = i;
            break;
        }
    }
    return display;
}

/* This function was copied from GEdit (gedit-utils.c). */
guint
remmina_public_get_current_workspace (GdkScreen *screen)
{
#ifdef GDK_WINDOWING_X11
    GdkWindow *root_win;
    GdkDisplay *display;
    Atom type;
    gint format;
    gulong nitems;
    gulong bytes_after;
    guint *current_desktop;
    gint err, result;
    guint ret = 0;

    g_return_val_if_fail (GDK_IS_SCREEN (screen), 0);

    root_win = gdk_screen_get_root_window (screen);
    display = gdk_screen_get_display (screen);

    gdk_error_trap_push ();
    result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (root_win),
                     gdk_x11_get_xatom_by_name_for_display (display, "_NET_CURRENT_DESKTOP"),
                     0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
                     &bytes_after, (gpointer) &current_desktop);
    err = gdk_error_trap_pop ();

    if (err != Success || result != Success)
        return ret;

    if (type == XA_CARDINAL && format == 32 && nitems > 0)
        ret = current_desktop[0];

    XFree (current_desktop);
    return ret;
#else
    /* FIXME: on mac etc proably there are native APIs
     * to get the current workspace etc */
    return 0;
#endif
}

/* This function was copied from GEdit (gedit-utils.c). */
guint
remmina_public_get_window_workspace (GtkWindow *gtkwindow)
{
#ifdef GDK_WINDOWING_X11
    GdkWindow *window;
    GdkDisplay *display;
    Atom type;
    gint format;
    gulong nitems;
    gulong bytes_after;
    guint *workspace;
    gint err, result;
    guint ret = 0;

    g_return_val_if_fail (GTK_IS_WINDOW (gtkwindow), 0);
    g_return_val_if_fail (GTK_WIDGET_REALIZED (GTK_WIDGET (gtkwindow)), 0);

    window = GTK_WIDGET (gtkwindow)->window;
    display = gdk_drawable_get_display (window);

    gdk_error_trap_push ();
    result = XGetWindowProperty (GDK_DISPLAY_XDISPLAY (display), GDK_WINDOW_XID (window),
                     gdk_x11_get_xatom_by_name_for_display (display, "_NET_WM_DESKTOP"),
                     0, G_MAXLONG, False, XA_CARDINAL, &type, &format, &nitems,
                     &bytes_after, (gpointer) &workspace);
    err = gdk_error_trap_pop ();

    if (err != Success || result != Success)
        return ret;

    if (type == XA_CARDINAL && format == 32 && nitems > 0)
        ret = workspace[0];

    XFree (workspace);
    return ret;
#else
    /* FIXME: on mac etc proably there are native APIs
     * to get the current workspace etc */
    return 0;
#endif
}

#ifdef HAVE_GTK_PRINTER
typedef struct _RemminaPrinterList
{
    GPtrArray *printers;
    RemminaGetPrintersCallback callback;
    gpointer user_data;
} RemminaPrinterList;

static void
remmina_public_printer_finalize (gpointer data)
{
    RemminaPrinterList *lst = (RemminaPrinterList*) data;

    /* callback owns the pointer array */
    lst->callback (lst->printers, lst->user_data);
    g_free (lst);
}

static gboolean
remmina_public_printer_func (GtkPrinter *printer, gpointer data)
{
    RemminaPrinterList *lst = (RemminaPrinterList*) data;
    const gchar *printername;

    printername = gtk_printer_get_name (printer);
    g_ptr_array_add (lst->printers, g_strdup (printername));

    return FALSE;
}

/* By using this function, we can safely perform enumerating printers asynchronously (which is needed by GTK),
 * while we don't have to enter the glib main loop (wait=TRUE, which can cause freezing). We use callback
 * function to inform the caller that enumeration is done, and pass a pointer array to the callback.
 */
void
remmina_public_get_printers (RemminaGetPrintersCallback callback, gpointer user_data)
{
    RemminaPrinterList *lst;

    lst = g_new (RemminaPrinterList, 1);
    lst->printers = g_ptr_array_new ();
    lst->callback = callback;
    lst->user_data = user_data;

    gtk_enumerate_printers (remmina_public_printer_func, lst, remmina_public_printer_finalize, FALSE);
}

#else

void
remmina_public_get_printers (RemminaGetPrintersCallback callback, gpointer user_data)
{
    callback (NULL, user_data);
}

#endif

