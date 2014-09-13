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
#include "remmina_chain_button.h"

G_DEFINE_TYPE (RemminaChainButton, remmina_chain_button, GTK_TYPE_GRID)

static const gchar* line_up_xpm[] =
{
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
	"         "
};

static const gchar* line_down_xpm[] =
{
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
	"+++++    "
};

static const gchar* vchain_xpm[] =
{
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
	"         "
};

static const gchar* vchain_broken_xpm[] =
{
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
	"  .....  "
};

enum
{
	CHAIN_TOGGLED_SIGNAL,
	LAST_SIGNAL
};

static guint remmina_chain_button_signals[LAST_SIGNAL] = { 0 };

static void remmina_chain_button_class_init(RemminaChainButtonClass* klass)
{
	remmina_chain_button_signals[CHAIN_TOGGLED_SIGNAL] = g_signal_new("chain-toggled", G_TYPE_FROM_CLASS(klass),
			G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION, G_STRUCT_OFFSET(RemminaChainButtonClass, chain_toggled), NULL,
			NULL, g_cclosure_marshal_VOID__VOID, G_TYPE_NONE, 0);
}

static void remmina_chain_button_update_chained(RemminaChainButton* cb)
{
	GdkPixbuf* pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data(cb->chained ? vchain_xpm : vchain_broken_xpm);
	gtk_image_set_from_pixbuf(GTK_IMAGE(cb->chain_image), pixbuf);
	g_object_unref(pixbuf);
}

static void remmina_chain_button_on_clicked(GtkWidget* widget, RemminaChainButton* cb)
{
	cb->chained = !cb->chained;
	remmina_chain_button_update_chained(cb);

	g_signal_emit(G_OBJECT(cb), remmina_chain_button_signals[CHAIN_TOGGLED_SIGNAL], 0);
}

static void remmina_chain_button_init(RemminaChainButton* cb)
{
	GtkWidget* widget;
	GtkWidget* image;
	GdkPixbuf* pixbuf;

	pixbuf = gdk_pixbuf_new_from_xpm_data(line_up_xpm);
	image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);
	gtk_widget_show(image);
	gtk_grid_attach(GTK_GRID(cb), image, 0, 0, 1, 1);

	widget = gtk_button_new();
	gtk_widget_show(widget);
	gtk_button_set_relief(GTK_BUTTON(widget), GTK_RELIEF_NONE);
	gtk_grid_attach(GTK_GRID(cb), widget, 0, 1, 1, 1);
	g_signal_connect(G_OBJECT(widget), "clicked", G_CALLBACK(remmina_chain_button_on_clicked), cb);

	image = gtk_image_new();
	gtk_widget_show(image);
	gtk_container_add(GTK_CONTAINER(widget), image);
	cb->chain_image = image;

	pixbuf = gdk_pixbuf_new_from_xpm_data(line_down_xpm);
	image = gtk_image_new_from_pixbuf(pixbuf);
	g_object_unref(pixbuf);
	gtk_widget_show(image);
	gtk_grid_attach(GTK_GRID(cb), image, 0, 2, 1, 1);

	cb->chained = FALSE;
}

GtkWidget*
remmina_chain_button_new(void)
{
	return GTK_WIDGET(g_object_new(REMMINA_TYPE_CHAIN_BUTTON, NULL));
}

void remmina_chain_button_set(RemminaChainButton* cb, gboolean chained)
{
	cb->chained = chained;
	remmina_chain_button_update_chained(cb);
	g_signal_emit(G_OBJECT(cb), remmina_chain_button_signals[CHAIN_TOGGLED_SIGNAL], 0);
}

