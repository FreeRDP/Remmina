/*
 * Remmina - The GTK+ Remote Desktop Client
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

#include "config.h"
#include <stdlib.h>
#include <gio/gio.h>
#include <glib/gi18n.h>
#include <string.h>
#include "remmina_scheduler.h"
#include "remmina/remmina_trace_calls.h"

static gboolean remmina_scheduler_periodic_check(gpointer user_data)
{
	TRACE_CALL(__func__);
	rsSchedData *rssd = (rsSchedData *)user_data;

	rssd->count++;
	if (rssd->cb_func_ptr(rssd->cb_func_data) == G_SOURCE_REMOVE) {
		g_free(rssd);
		return G_SOURCE_REMOVE;
	}
	if (rssd->count <= 1) {
		rssd->source = g_timeout_add_full(G_PRIORITY_LOW,
						  rssd->interval,
						  remmina_scheduler_periodic_check,
						  rssd,
						  NULL);
		return G_SOURCE_REMOVE;
	}
	return G_SOURCE_CONTINUE;
}

void *remmina_scheduler_setup(GSourceFunc	cb,
			      gpointer		cb_data,
			      guint		first_interval,
			      guint		interval)
{
	TRACE_CALL(__func__);
	rsSchedData *rssd;
	rssd = g_malloc(sizeof(rsSchedData));
	rssd->cb_func_ptr = cb;
	rssd->cb_func_data = cb_data;
	rssd->interval = interval;
	rssd->count = 0;
	rssd->source = g_timeout_add_full(G_PRIORITY_LOW,
					  first_interval,
					  remmina_scheduler_periodic_check,
					  rssd,
					  NULL);
	return (void *)rssd;
}

void remmina_schedluer_remove(void *s)
{
	TRACE_CALL(__func__);
	rsSchedData *rssd = (rsSchedData *)s;
	g_source_remove(rssd->source);
	g_free(rssd);
}
