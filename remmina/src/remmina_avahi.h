/*  See LICENSE and COPYING files for copyright and license details. */

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

RemminaAvahi* remmina_avahi_new(void);
void remmina_avahi_start(RemminaAvahi* ga);
void remmina_avahi_stop(RemminaAvahi* ga);
void remmina_avahi_free(RemminaAvahi* ga);

G_END_DECLS

#endif  /* __REMMINAAVAHI_H__  */

