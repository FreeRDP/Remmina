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
 */

#ifndef __REMMINAMAIN_H__
#define __REMMINAMAIN_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_MAIN               (remmina_main_get_type ())
#define REMMINA_MAIN(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_MAIN, RemminaMain))
#define REMMINA_MAIN_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_MAIN, RemminaMainClass))
#define REMMINA_IS_MAIN(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_MAIN))
#define REMMINA_IS_MAIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_MAIN))
#define REMMINA_MAIN_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_MAIN, RemminaMainClass))

typedef struct _RemminaMainPriv RemminaMainPriv;

typedef struct _RemminaMain
{
	GtkWindow window;

	RemminaMainPriv *priv;
} RemminaMain;

typedef struct _RemminaMainClass
{
	GtkWindowClass parent_class;
} RemminaMainClass;

GType remmina_main_get_type(void)
G_GNUC_CONST;

/* Create the main Remmina window */
GtkWidget* remmina_main_new(void);

G_END_DECLS

#endif  /* __REMMINAMAIN_H__  */

