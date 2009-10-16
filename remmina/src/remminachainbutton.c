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
#include "remminachainbutton.h"

G_DEFINE_TYPE (RemminaChainButton, remmina_chain_button, GTK_TYPE_TABLE)

static gchar * line_up_xpm[] = {
"9 7 3 1",
" 	c None",
".	c #A9A5A2",
"+	c #FFFFFF",
".....    ",
"+++++.   ",
"    +.   ",
"    +.   ",
"    +.   ",
"    +.   ",
"         "};

static gchar * line_down_xpm[] = {
"9 7 3 1",
" 	c None",
".	c #FFFFFF",
"+	c #A9A5A2",
"         ",
"    .+   ",
"    .+   ",
"    .+   ",
"    .+   ",
".....+   ",
"+++++    "};

static gchar * vchain_xpm[] = {
"9 24 13 1",
" 	c None",
".	c #555753",
"+	c #8D8D8E",
"@	c #E8E8E9",
"#	c #8E8E90",
"$	c #888A85",
"%	c #8F8F91",
"&	c #2E3436",
"*	c #EEEEEC",
"=	c #989899",
"-	c #959597",
";	c #9D9D9E",
">	c #B9B9BA",
"         ",
"         ",
"  .....  ",
" .+@@@#. ",
" .@...@. ",
" .@. .@. ",
" .@. .@. ",
" .@. .@. ",
" .@.$.@. ",
" .%&*$%. ",
"  .&*$.  ",
"   &*$   ",
"   &*$   ",
"  .&*$.  ",
" .=&*$-. ",
" .@.&.@. ",
" .@. .@. ",
" .@. .@. ",
" .@. .@. ",
" .@...@. ",
" .;@@@>. ",
"  .....  ",
"         ",
"         "};

static gchar * vchain_broken_xpm[] = {
"9 24 13 1",
" 	c None",
".	c #555753",
"+	c #8D8D8E",
"@	c #E8E8E9",
"#	c #8E8E90",
"$	c #888A85",
"%	c #8F8F91",
"&	c #2E3436",
"*	c #EEEEEC",
"=	c #989899",
"-	c #959597",
";	c #9D9D9E",
">	c #B9B9BA",
"  .....  ",
" .+@@@#. ",
" .@...@. ",
" .@. .@. ",
" .@. .@. ",
" .@. .@. ",
" .@.$.@. ",
" .%&*$%. ",
"  .&*$.  ",
"   &*$   ",
"         ",
"         ",
"         ",
"         ",
"   &*$   ",
"  .&*$.  ",
" .=&*$-. ",
" .@.&.@. ",
" .@. .@. ",
" .@. .@. ",
" .@. .@. ",
" .@...@. ",
" .;@@@>. ",
"  .....  "};

enum {
    CHAIN_TOGGLED_SIGNAL,
    LAST_SIGNAL
};

static guint remmina_chain_button_signals[LAST_SIGNAL] = { 0 };

static void
remmina_chain_button_class_init (RemminaChainButtonClass *klass)
{
    remmina_chain_button_signals[CHAIN_TOGGLED_SIGNAL] =
        g_signal_new ("chain-toggled",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaChainButtonClass, chain_toggled),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
}

static void
remmina_chain_button_update_chained (RemminaChainButton *cb)
{
    GdkPixmap *pixmap;
    GdkBitmap *mask;

    pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gdk_colormap_get_system (),
        &mask, NULL, (cb->chained ? vchain_xpm : vchain_broken_xpm));
    gtk_image_set_from_pixmap (GTK_IMAGE (cb->chain_image), pixmap, mask);
    g_object_unref (pixmap);
    g_object_unref (mask);
}

static void
remmina_chain_button_on_clicked (GtkWidget *widget, RemminaChainButton *cb)
{
    cb->chained = !cb->chained;
    remmina_chain_button_update_chained (cb);

    g_signal_emit (G_OBJECT (cb), remmina_chain_button_signals[CHAIN_TOGGLED_SIGNAL], 0);
}

static void
remmina_chain_button_init (RemminaChainButton *cb)
{
    GtkWidget *widget;
    GtkWidget *image;
    GdkPixmap *pixmap;
    GdkBitmap *mask;

    gtk_table_resize (GTK_TABLE (cb), 3, 1);

    pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gdk_colormap_get_system (),
        &mask, NULL, line_up_xpm);
    image = gtk_image_new_from_pixmap (pixmap, mask);
    g_object_unref (pixmap);
    g_object_unref (mask);
    gtk_widget_show (image);
    gtk_table_attach_defaults (GTK_TABLE (cb), image, 0, 1, 0, 1);

    widget = gtk_button_new ();
    gtk_widget_show (widget);
    gtk_button_set_relief (GTK_BUTTON (widget), GTK_RELIEF_NONE);
    gtk_table_attach_defaults (GTK_TABLE (cb), widget, 0, 1, 1, 2);
    g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_chain_button_on_clicked), cb);

    image = gtk_image_new ();
    gtk_widget_show (image);
    gtk_container_add (GTK_CONTAINER (widget), image);
    cb->chain_image = image;

    pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, gdk_colormap_get_system (),
        &mask, NULL, line_down_xpm);
    image = gtk_image_new_from_pixmap (pixmap, mask);
    g_object_unref (pixmap);
    g_object_unref (mask);
    gtk_widget_show (image);
    gtk_table_attach_defaults (GTK_TABLE (cb), image, 0, 1, 2, 3);

    cb->chained = FALSE;
}

GtkWidget*
remmina_chain_button_new (void)
{
    return GTK_WIDGET (g_object_new (REMMINA_TYPE_CHAIN_BUTTON, NULL));
}

void
remmina_chain_button_set (RemminaChainButton *cb, gboolean chained)
{
    cb->chained = chained;
    remmina_chain_button_update_chained (cb);
    g_signal_emit (G_OBJECT (cb), remmina_chain_button_signals[CHAIN_TOGGLED_SIGNAL], 0);
}


