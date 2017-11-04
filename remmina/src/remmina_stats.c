/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2018 Antenore Gatta, Giovanni Panozzo
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
#include <gtk/gtk.h>
#ifdef GDK_WINDOWING_WAYLAND
	#include <gdk/gdkwayland.h>
#endif
#ifdef GDK_WINDOWING_X11
	#include <gdk/gdkx.h>
#endif
#include "remmina_stats.h"

JsonNode *remmina_stats_get_gtk_version()
{
	JsonBuilder *b;
	JsonNode *r;

	b = json_builder_new();
	json_builder_begin_object(b);
	json_builder_set_member_name(b, "major");
	json_builder_add_int_value(b, gtk_get_major_version());
	json_builder_set_member_name(b, "minor");
	json_builder_add_int_value(b, gtk_get_minor_version());
	json_builder_set_member_name(b, "micro");
	json_builder_add_int_value(b, gtk_get_micro_version());
	json_builder_end_object (b);
	r = json_builder_get_root(b);
	g_object_unref(b);
	return r;

}

JsonNode *remmina_stats_get_gtk_backend()
{
	JsonNode *r;
	GdkDisplay *disp;
	gchar *bkend;

	disp = gdk_display_get_default();

#ifdef GDK_WINDOWING_WAYLAND
	if (GDK_IS_WAYLAND_DISPLAY(disp))
	{
		bkend = "Wayland";
	}
	else
#endif
#ifdef GDK_WINDOWING_X11
	if (GDK_IS_X11_DISPLAY (disp))
	{
		bkend = "X11";
    }
	else
#endif
	bkend = "Unknown";

	r = json_node_alloc();
	json_node_init_string(r, bkend);

	return r;

}

JsonNode *remmina_stats_get_all()
{
	/* Get all statistics in json format to send periodically to the PHP server.
	 * Return a pointer to the JSON string.
	 * The caller should free the returned buffer with g_free() */

	JsonBuilder *b;
	JsonNode *n;
	b = json_builder_new();
	json_builder_begin_object(b);

	n = remmina_stats_get_gtk_version();
	json_builder_set_member_name(b,"GTKVERSION");
	json_builder_add_value(b, n);

	n = remmina_stats_get_gtk_backend();
	json_builder_set_member_name(b,"GTKBACKEND");
	json_builder_add_value(b, n);

	json_builder_end_object(b);
	n = json_builder_get_root(b);
	g_object_unref(b);

	return n;

}
