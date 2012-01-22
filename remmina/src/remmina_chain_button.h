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
 

#ifndef __REMMINACHAINBUTTON_H__
#define __REMMINACHAINBUTTON_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_CHAIN_BUTTON               (remmina_chain_button_get_type ())
#define REMMINA_CHAIN_BUTTON(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_CHAIN_BUTTON, RemminaChainButton))
#define REMMINA_CHAIN_BUTTON_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_CHAIN_BUTTON, RemminaChainButtonClass))
#define REMMINA_IS_CHAIN_BUTTON(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_CHAIN_BUTTON))
#define REMMINA_IS_CHAIN_BUTTON_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_CHAIN_BUTTON))
#define REMMINA_CHAIN_BUTTON_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_CHAIN_BUTTON, RemminaChainButtonClass))

typedef struct _RemminaChainButton
{
    GtkTable table;

    gboolean chained;
    GtkWidget *chain_image;
} RemminaChainButton;

typedef struct _RemminaChainButtonClass
{
    GtkTableClass parent_class;

    void (* chain_toggled) (RemminaChainButton *cb);
} RemminaChainButtonClass;

GType remmina_chain_button_get_type (void) G_GNUC_CONST;

GtkWidget* remmina_chain_button_new (void);

void remmina_chain_button_set (RemminaChainButton *cb, gboolean chained);

G_END_DECLS

#endif  /* __REMMINACHAINBUTTON_H__  */

