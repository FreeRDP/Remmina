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

/**
 *  @file: remmina_utils.h
 *  General utility functions, non-GTK related.
 */

#pragma once

G_BEGIN_DECLS
gint remmina_utils_string_find(GString *haystack, gint start, gint end, const gchar *needle);
gint remmina_utils_string_replace(GString *str, gint pos, gint len, const gchar *replace);
guint remmina_utils_string_replace_all(GString *haystack, const gchar *needle, const gchar *replace);
gchar *remmina_utils_string_strip(const gchar *s);

gchar *remmina_utils_get_lang();
const gchar *remmina_utils_get_kernel_name();
const gchar *remmina_utils_get_kernel_release();
const gchar *remmina_utils_get_kernel_arch();
gchar *remmina_utils_get_lsb_id();
gchar *remmina_utils_get_lsb_description();
gchar *remmina_utils_get_lsb_release();
gchar *remmina_utils_get_lsb_codename();
GHashTable *remmina_utils_get_etc_release();
const gchar *remmina_utils_get_os_info();
gchar *remmina_sha1_file(const gchar *filename);
gchar *remmina_gen_random_uuid();
G_END_DECLS
