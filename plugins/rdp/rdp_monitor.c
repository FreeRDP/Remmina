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

#include "rdp_plugin.h"
#include "rdp_monitor.h"

/** @ToDo Utility functions should be moved somewhere else */
gint remmina_rdp_utils_strpos(const gchar *haystack, const gchar *needle)
{
	TRACE_CALL(__func__);
	const gchar *sub;

	if (!*needle)
		return -1;

	sub = strstr(haystack, needle);
	if (!sub)
		return -1;

	return sub - haystack;
}

/* https://github.com/adlocode/xfwm4/blob/1d21be9ffc0fa1cea91905a07d1446c5227745f4/common/xfwm-common.c */


/**
 * Set the MonitorIDs, the maxwidth and maxheight
 *
 * - number of monitors
 * - Geometry of each
 * - Choosen by the user
 * - Primary monitor
 *   - Otherwise use current monitor as the origin
 *
 *   The origin must be 0,0
 */
void remmina_rdp_monitor_get (rfContext *rfi, gchar **monitorids, guint32 *maxwidth, guint32 *maxheight)
{
	TRACE_CALL(__func__);

	GdkDisplay *display;
	GdkMonitor *monitor;
	gboolean has_custom_monitors = FALSE;

	gboolean primary_found = FALSE;

	gint n_monitors;
	gint scale;
	gint index = 0;
	gint count = 0;

	static gchar buffer[256];

	GdkRectangle geometry = { 0, 0, 0, 0 };
	GdkRectangle tempgeom = { 0, 0, 0, 0 };
	GdkRectangle destgeom = { 0, 0, 0, 0 };
	rdpSettings* settings;
	if (!rfi || !rfi->settings)
		return;

	settings = rfi->settings;

	*maxwidth = freerdp_settings_get_uint32(settings, FreeRDP_DesktopWidth);
	*maxheight = freerdp_settings_get_uint32(settings, FreeRDP_DesktopHeight);

	display = gdk_display_get_default ();
	n_monitors = gdk_display_get_n_monitors(display);

	/* Get monitor at windows curently in use */
	//w = gtk_widget_get_window(rfi->drawing_area);

	//current_monitor = gdk_display_get_monitor_at_window (display, w);

	/* we got monitorids as options */
	if (*monitorids)
		has_custom_monitors = TRUE;

	buffer[0] = '\0';

	rdpMonitor* base = (rdpMonitor *)freerdp_settings_get_pointer(settings, FreeRDP_MonitorDefArray);
	for (gint i = 0; i < n_monitors; ++i) {
		rdpMonitor* current;
		if (has_custom_monitors) {
			REMMINA_PLUGIN_DEBUG("We have custom monitors");
			gchar itoc[10];
			sprintf(itoc, "%d", i);;
			if (remmina_rdp_utils_strpos(*monitorids, itoc) < 0 ) {
				REMMINA_PLUGIN_DEBUG("Monitor n %d it's out of the provided list", i);
				index += 1;
				continue;
			}
		}

		monitor = gdk_display_get_monitor(display, i);
		if (monitor == NULL) {
			REMMINA_PLUGIN_DEBUG("Monitor n %d does not exist or is not active", i);
			index +=1;
			continue;
		}

		monitor = gdk_display_get_monitor(display, index);
		current = &base[index];
		REMMINA_PLUGIN_DEBUG("Monitor n %d", index);
		/* If the desktop env in use doesn't have the working area concept
		 * gdk_monitor_get_workarea will return the monitor geometry*/
		//gdk_monitor_get_workarea (monitor, &geometry);
		gdk_monitor_get_geometry (monitor, &geometry);
		current->x = geometry.x;
		REMMINA_PLUGIN_DEBUG("Monitor n %d x: %d", index, geometry.x);
		current->y = geometry.y;
		REMMINA_PLUGIN_DEBUG("Monitor n %d y: %d", index, geometry.y);
		/* geometry contain the application geometry, to obtain the real one
		 * we must multiply by the scale factor */
		scale = gdk_monitor_get_scale_factor (monitor);
		REMMINA_PLUGIN_DEBUG("Monitor n %d scale: %d", index, scale);
		geometry.width *= scale;
		geometry.height *= scale;
		REMMINA_PLUGIN_DEBUG("Monitor n %d width: %d", index, geometry.width);
		REMMINA_PLUGIN_DEBUG("Monitor n %d height: %d", index, geometry.height);
		current->width = geometry.width;
		current->height = geometry.height;
		current->attributes.physicalHeight = gdk_monitor_get_height_mm (monitor);
		REMMINA_PLUGIN_DEBUG("Monitor n %d physical  height: %d", i, current->attributes.physicalHeight);
		current->attributes.physicalWidth = gdk_monitor_get_width_mm (monitor);
		REMMINA_PLUGIN_DEBUG("Monitor n %d physical  width: %d", i, current->attributes.physicalWidth);
		current->orig_screen = index;
		if (!primary_found) {
			freerdp_settings_set_uint32(settings, FreeRDP_MonitorLocalShiftX, current->x);
			freerdp_settings_set_uint32(settings, FreeRDP_MonitorLocalShiftY, current->y);
		}
		if (gdk_monitor_is_primary(monitor)) {
			REMMINA_PLUGIN_DEBUG ("Primary monitor found with id: %d", index);
			current->is_primary = TRUE;
			primary_found = TRUE;
			if (current->x != 0 || current->y != 0)
			{
				REMMINA_PLUGIN_DEBUG ("Primary monitor not at 0,0 coordinates: %d", index);
				freerdp_settings_set_uint32(settings, FreeRDP_MonitorLocalShiftX, current->x);
				freerdp_settings_set_uint32(settings, FreeRDP_MonitorLocalShiftY, current->y);
			}
		} else {
			if (!primary_found && current->x == 0 &&
					current->y == 0)
			{
				REMMINA_PLUGIN_DEBUG ("Monitor %d has 0,0 coordinates", index);
				current->is_primary = TRUE;
				freerdp_settings_set_uint32(settings, FreeRDP_MonitorLocalShiftX, current->x);
				freerdp_settings_set_uint32(settings, FreeRDP_MonitorLocalShiftY, current->y);
				primary_found = TRUE;
				REMMINA_PLUGIN_DEBUG ("Primary monitor set to id: %d", index);
			}
		}
		REMMINA_PLUGIN_DEBUG ("Local X Shift: %d", freerdp_settings_get_uint32(settings, FreeRDP_MonitorLocalShiftX));
		REMMINA_PLUGIN_DEBUG ("Local Y Shift: %d", freerdp_settings_get_uint32(settings, FreeRDP_MonitorLocalShiftY));
		//current->x =
			//current->x - freerdp_settings_get_uint32(settings, FreeRDP_MonitorLocalShiftX);
		//REMMINA_PLUGIN_DEBUG("Monitor n %d calculated x: %d", index, current->x);
		//current->y =
			//current->y - freerdp_settings_get_uint32(settings, FreeRDP_MonitorLocalShiftY);
		//REMMINA_PLUGIN_DEBUG("Monitor n %d calculated y: %d", index, current->y);

		if (buffer[0] == '\0')
			g_sprintf (buffer, "%d", i);
		else
			g_sprintf(buffer, "%s,%d", buffer, i);
		REMMINA_PLUGIN_DEBUG("Monitor IDs buffer: %s", buffer);
		gdk_rectangle_union(&tempgeom, &geometry, &destgeom);
		memcpy(&tempgeom, &destgeom, sizeof tempgeom);
		count++;
		index++;

	}
	freerdp_settings_set_uint32(settings, FreeRDP_MonitorCount, index);
	/* Subtract monitor shift from monitor variables for server-side use.
	 * We maintain monitor shift value as Window requires the primary monitor to have a
	 * coordinate of 0,0 In some X configurations, no monitor may have a coordinate of 0,0. This
	 * can also be happen if the user requests specific monitors from the command-line as well.
	 * So, we make sure to translate our primary monitor's upper-left corner to 0,0 on the
	 * server.
	 */
	for (gint i = 0; i < freerdp_settings_get_uint32(settings, FreeRDP_MonitorCount); i++)
	{
		rdpMonitor* current = &base[i];
		current->x =
			current->x - freerdp_settings_get_uint32(settings, FreeRDP_MonitorLocalShiftX);
		REMMINA_PLUGIN_DEBUG("Monitor n %d calculated x: %d", i, current->x);
		current->y =
			current->y - freerdp_settings_get_uint32(settings, FreeRDP_MonitorLocalShiftY);
		REMMINA_PLUGIN_DEBUG("Monitor n %d calculated y: %d", i, current->y);
	}

	REMMINA_PLUGIN_DEBUG("%d monitors on %d have been configured", freerdp_settings_get_uint32(settings, FreeRDP_MonitorCount), count);
	*maxwidth = destgeom.width;
	*maxheight = destgeom.height;
	REMMINA_PLUGIN_DEBUG("maxw and maxh: %ux%u", *maxwidth, *maxheight);
	if (n_monitors > 1)
		freerdp_settings_set_bool(rfi->settings, FreeRDP_SupportMonitorLayoutPdu, TRUE);
	*monitorids = g_strdup(buffer);
}
