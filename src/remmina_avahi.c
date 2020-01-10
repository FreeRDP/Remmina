/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
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
#include "config.h"
#include "remmina_avahi.h"
#include "remmina/remmina_trace_calls.h"

#ifdef HAVE_LIBAVAHI_CLIENT

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/malloc.h>
#include <avahi-common/error.h>

struct _RemminaAvahiPriv {
	AvahiSimplePoll* simple_poll;
	AvahiClient* client;
	AvahiServiceBrowser* sb;
	guint iterate_handler;
	gboolean has_event;
};

static void
remmina_avahi_resolve_callback(
	AvahiServiceResolver* r,
	AVAHI_GCC_UNUSED AvahiIfIndex interface,
	AVAHI_GCC_UNUSED AvahiProtocol protocol,
	AvahiResolverEvent event,
	const char* name,
	const char* type,
	const char* domain,
	const char* host_name,
	const AvahiAddress* address,
	uint16_t port,
	AvahiStringList* txt,
	AvahiLookupResultFlags flags,
	AVAHI_GCC_UNUSED void* userdata)
{
	TRACE_CALL(__func__);
	gchar* key;
	gchar* value;
	RemminaAvahi* ga = (RemminaAvahi*)userdata;

	assert(r);

	ga->priv->has_event = TRUE;

	switch (event) {
	case AVAHI_RESOLVER_FAILURE:
		g_print("(remmina-applet avahi-resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n",
			name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
		break;

	case AVAHI_RESOLVER_FOUND:
		key = g_strdup_printf("%s,%s,%s", name, type, domain);
		if (g_hash_table_lookup(ga->discovered_services, key)) {
			g_free(key);
			break;
		}
		value = g_strdup_printf("[%s]:%i", host_name, port);
		g_hash_table_insert(ga->discovered_services, key, value);
		/* key and value will be freed with g_free when the has table is freed */

		g_print("(remmina-applet avahi-resolver) Added service '%s'\n", value);

		break;
	}

	avahi_service_resolver_free(r);
}

static void
remmina_avahi_browse_callback(
	AvahiServiceBrowser* b,
	AvahiIfIndex interface,
	AvahiProtocol protocol,
	AvahiBrowserEvent event,
	const char* name,
	const char* type,
	const char* domain,
	AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
	void* userdata)
{
	TRACE_CALL(__func__);
	gchar* key;
	RemminaAvahi* ga = (RemminaAvahi*)userdata;

	assert(b);

	ga->priv->has_event = TRUE;

	switch (event) {
	case AVAHI_BROWSER_FAILURE:
		g_print("(remmina-applet avahi-browser) %s\n",
			avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
		return;

	case AVAHI_BROWSER_NEW:
		key = g_strdup_printf("%s,%s,%s", name, type, domain);
		if (g_hash_table_lookup(ga->discovered_services, key)) {
			g_free(key);
			break;
		}
		g_free(key);

		g_print("(remmina-applet avahi-browser) Found service '%s' of type '%s' in domain '%s'\n", name, type, domain);

		if (!(avahi_service_resolver_new(ga->priv->client, interface, protocol, name, type, domain,
			      AVAHI_PROTO_UNSPEC, 0, remmina_avahi_resolve_callback, ga))) {
			g_print("(remmina-applet avahi-browser) Failed to resolve service '%s': %s\n",
				name, avahi_strerror(avahi_client_errno(ga->priv->client)));
		}
		break;

	case AVAHI_BROWSER_REMOVE:
		g_print("(remmina-applet avahi-browser) Removed service '%s' of type '%s' in domain '%s'\n", name, type, domain);
		key = g_strdup_printf("%s,%s,%s", name, type, domain);
		g_hash_table_remove(ga->discovered_services, key);
		g_free(key);
		break;

	case AVAHI_BROWSER_ALL_FOR_NOW:
	case AVAHI_BROWSER_CACHE_EXHAUSTED:
		break;
	}
}

static void remmina_avahi_client_callback(AvahiClient* c, AvahiClientState state, AVAHI_GCC_UNUSED void * userdata)
{
	TRACE_CALL(__func__);
	RemminaAvahi* ga = (RemminaAvahi*)userdata;

	ga->priv->has_event = TRUE;

	if (state == AVAHI_CLIENT_FAILURE) {
		g_print("(remmina-applet avahi) Server connection failure: %s\n", avahi_strerror(avahi_client_errno(c)));
	}
}

static gboolean remmina_avahi_iterate(RemminaAvahi* ga)
{
	TRACE_CALL(__func__);
	while (TRUE) {
		/* Call the iteration until no further events */
		ga->priv->has_event = FALSE;
		avahi_simple_poll_iterate(ga->priv->simple_poll, 0);
		if (!ga->priv->has_event)
			break;
	}

	return TRUE;
}

RemminaAvahi* remmina_avahi_new(void)
{
	TRACE_CALL(__func__);
	RemminaAvahi* ga;

	ga = g_new(RemminaAvahi, 1);
	ga->discovered_services = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, g_free);
	ga->started = FALSE;
	ga->priv = g_new(RemminaAvahiPriv, 1);
	ga->priv->simple_poll = NULL;
	ga->priv->client = NULL;
	ga->priv->sb = NULL;
	ga->priv->iterate_handler = 0;
	ga->priv->has_event = FALSE;

	return ga;
}

void remmina_avahi_start(RemminaAvahi* ga)
{
	TRACE_CALL(__func__);
	int error;

	if (ga->started)
		return;

	ga->started = TRUE;

	ga->priv->simple_poll = avahi_simple_poll_new();
	if (!ga->priv->simple_poll) {
		g_print("Failed to create simple poll object.\n");
		return;
	}

	ga->priv->client = avahi_client_new(avahi_simple_poll_get(ga->priv->simple_poll), 0, remmina_avahi_client_callback, ga,
		&error);
	if (!ga->priv->client) {
		g_print("Failed to create client: %s\n", avahi_strerror(error));
		return;
	}

	/** @todo Customize the default domain here */
	ga->priv->sb = avahi_service_browser_new(ga->priv->client, AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, "_rfb._tcp", NULL, 0,
		remmina_avahi_browse_callback, ga);
	if (!ga->priv->sb) {
		g_print("Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(ga->priv->client)));
		return;
	}

	ga->priv->iterate_handler = g_timeout_add(5000, (GSourceFunc)remmina_avahi_iterate, ga);
}

void remmina_avahi_stop(RemminaAvahi* ga)
{
	TRACE_CALL(__func__);
	g_hash_table_remove_all(ga->discovered_services);
	if (ga->priv->iterate_handler) {
		g_source_remove(ga->priv->iterate_handler);
		ga->priv->iterate_handler = 0;
	}
	if (ga->priv->sb) {
		avahi_service_browser_free(ga->priv->sb);
		ga->priv->sb = NULL;
	}
	if (ga->priv->client) {
		avahi_client_free(ga->priv->client);
		ga->priv->client = NULL;
	}
	if (ga->priv->simple_poll) {
		avahi_simple_poll_free(ga->priv->simple_poll);
		ga->priv->simple_poll = NULL;
	}
	ga->started = FALSE;
}

void remmina_avahi_free(RemminaAvahi* ga)
{
	TRACE_CALL(__func__);
	if (ga == NULL)
		return;

	remmina_avahi_stop(ga);

	g_free(ga->priv);
	g_hash_table_destroy(ga->discovered_services);
	g_free(ga);
}

#else

RemminaAvahi* remmina_avahi_new(void)
{
	TRACE_CALL(__func__);
	return NULL;
}

void remmina_avahi_start(RemminaAvahi* ga)
{
	TRACE_CALL(__func__);
}

void remmina_avahi_stop(RemminaAvahi* ga)
{
	TRACE_CALL(__func__);
}

void remmina_avahi_free(RemminaAvahi* ga)
{
	TRACE_CALL(__func__);
}

#endif

