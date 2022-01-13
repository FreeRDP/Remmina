/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee
 * Copyright (C) 2017-2022 Antenore Gatta, Giovanni Panozzo
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

#pragma once

#include <stdarg.h>
#include <glib.h>

G_BEGIN_DECLS

#define REMMINA_INFO(fmt, ...)     _remmina_info(fmt, ## __VA_ARGS__)
#define REMMINA_MESSAGE(fmt, ...)  _remmina_message(fmt, ## __VA_ARGS__)
#define REMMINA_DEBUG(fmt, ...)    _remmina_debug(__func__, fmt, ## __VA_ARGS__)
#define REMMINA_WARNING(fmt, ...)  _remmina_warning(__func__, fmt, ## __VA_ARGS__)
#define REMMINA_AUDIT(fmt, ...)    _remmina_audit(__func__, fmt, ## __VA_ARGS__)
#define REMMINA_ERROR(fmt, ...)    _remmina_error(__func__, fmt, ## __VA_ARGS__)
#define REMMINA_CRITICAL(fmt, ...) _remmina_critical(__func__, fmt, ## __VA_ARGS__)

void remmina_log_start(void);
gboolean remmina_log_running(void);
void remmina_log_print(const gchar *text);
void _remmina_info(const gchar *fmt, ...);
void _remmina_message(const gchar *fmt, ...);
void _remmina_debug(const gchar *fun, const gchar *fmt, ...);
void _remmina_warning(const gchar *fun, const gchar *fmt, ...);
void _remmina_audit(const gchar *fun, const gchar *fmt, ...);
void _remmina_error(const gchar *fun, const gchar *fmt, ...);
void _remmina_critical(const gchar *fun, const gchar *fmt, ...);
void remmina_log_printf(const gchar *fmt, ...);

G_END_DECLS
