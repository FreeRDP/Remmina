/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee 
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
 

#ifndef __REMMINAAVAHI_H__
#define __REMMINAAVAHI_H__

G_BEGIN_DECLS

typedef struct _RemminaAvahiPriv RemminaAvahiPriv;

typedef struct _RemminaAvahi
{
    GHashTable *discovered_services;
    gboolean started;

    RemminaAvahiPriv *priv;
} RemminaAvahi;

RemminaAvahi* remmina_avahi_new (void);
void remmina_avahi_start (RemminaAvahi* ga);
void remmina_avahi_stop (RemminaAvahi* ga);
void remmina_avahi_free (RemminaAvahi* ga);

G_END_DECLS

#endif  /* __REMMINAAVAHI_H__  */

