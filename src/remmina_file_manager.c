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

#include "config.h"

#include <gtk/gtk.h>
#include <string.h>

#include "remmina_public.h"
#include "remmina_pref.h"
#include "remmina_string_array.h"
#include "remmina_plugin_manager.h"
#include "remmina_file_manager.h"
#include "remmina/remmina_trace_calls.h"

static gchar *remminadir;
static gchar *cachedir;

/* return first found data dir as per XDG specs.
 * The returned string must be freed by the caller with g_free */
gchar *remmina_file_get_datadir(void)
{
	TRACE_CALL(__func__);
	const gchar *dir = ".remmina";
	int i;
	/* From preferences, datadir_path */
	remminadir = remmina_pref_get_value("datadir_path");
	if (remminadir != NULL && strlen(remminadir) > 0)
		if (g_file_test(remminadir, G_FILE_TEST_IS_DIR))
			return remminadir;
	g_free(remminadir), remminadir = NULL;
	/* Legacy ~/.remmina */
	remminadir = g_build_path("/", g_get_home_dir(), dir, NULL);
	if (g_file_test(remminadir, G_FILE_TEST_IS_DIR))
		return remminadir;
	g_free(remminadir), remminadir = NULL;
	/* ~/.local/share/remmina */
	remminadir = g_build_path("/", g_get_user_data_dir(), "remmina", NULL);
	if (g_file_test(remminadir, G_FILE_TEST_IS_DIR))
		return remminadir;
	g_free(remminadir), remminadir = NULL;
	/* /usr/local/share/remmina */
	const gchar *const *dirs = g_get_system_data_dirs();
	g_free(remminadir), remminadir = NULL;
	for (i = 0; dirs[i] != NULL; ++i) {
		remminadir = g_build_path("/", dirs[i], "remmina", NULL);
		if (g_file_test(remminadir, G_FILE_TEST_IS_DIR))
			return remminadir;
		g_free(remminadir), remminadir = NULL;
	}
	/* The last case we use  the home ~/.local/share/remmina */
	remminadir = g_build_path("/", g_get_user_data_dir(), "remmina", NULL);
	return remminadir;
}

/** @todo remmina_pref_file_do_copy and remmina_file_manager_do_copy to remmina_files_copy */
static gboolean remmina_file_manager_do_copy(const char *src_path, const char *dst_path)
{
	GFile *src = g_file_new_for_path(src_path), *dst = g_file_new_for_path(dst_path);
	/* We don’t overwrite the target if it exists */
	const gboolean ok = g_file_copy(src, dst, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);

	g_object_unref(dst);
	g_object_unref(src);

	return ok;
}

void remmina_file_manager_init(void)
{
	TRACE_CALL(__func__);
	GDir *dir;
	const gchar *legacy = ".remmina";
	const gchar *filename;
	int i;

	remminadir = g_build_path("/", g_get_user_data_dir(), "remmina", NULL);
	/* Create the XDG_USER_DATA directory */
	g_mkdir_with_parents(remminadir, 0750);
	g_free(remminadir), remminadir = NULL;
	/* Create the XDG_CACHE_HOME directory */
	cachedir = g_build_path("/", g_get_user_cache_dir(), "remmina", NULL);
	g_mkdir_with_parents(cachedir, 0750);
	g_free(cachedir), cachedir = NULL;
	/* Empty legacy ~/.remmina */
	remminadir = g_build_path("/", g_get_home_dir(), legacy, NULL);
	if (g_file_test(remminadir, G_FILE_TEST_IS_DIR)) {
		dir = g_dir_open(remminadir, 0, NULL);
		while ((filename = g_dir_read_name(dir)) != NULL) {
			remmina_file_manager_do_copy(
				g_build_path("/", remminadir, filename, NULL),
				g_build_path("/", g_get_user_data_dir(),
					     "remmina", filename, NULL));
		}
	}

	/* XDG_DATA_DIRS, i.e. /usr/local/share/remmina */
	const gchar *const *dirs = g_get_system_data_dirs();
	g_free(remminadir), remminadir = NULL;
	for (i = 0; dirs[i] != NULL; ++i) {
		remminadir = g_build_path("/", dirs[i], "remmina", NULL);
		if (g_file_test(remminadir, G_FILE_TEST_IS_DIR)) {
			dir = g_dir_open(remminadir, 0, NULL);
			while ((filename = g_dir_read_name(dir)) != NULL) {
				remmina_file_manager_do_copy(
					g_build_path("/", remminadir, filename, NULL),
					g_build_path("/", g_get_user_data_dir(),
						     "remmina", filename, NULL));
			}
		}
		g_free(remminadir), remminadir = NULL;
	}
	/* At last we make sure we use XDG_USER_DATA */
	if (remminadir != NULL)
		g_free(remminadir), remminadir = NULL;
}

gint remmina_file_manager_iterate(GFunc func, gpointer user_data)
{
	TRACE_CALL(__func__);
	gchar filename[MAX_PATH_LEN];
	GDir *dir;
	const gchar *name;
	RemminaFile *remminafile;
	gint items_count = 0;
	gchar *remmina_data_dir;

	remmina_data_dir = remmina_file_get_datadir();
	dir = g_dir_open(remmina_data_dir, 0, NULL);

	if (dir) {
		while ((name = g_dir_read_name(dir)) != NULL) {
			if (!g_str_has_suffix(name, ".remmina"))
				continue;
			g_snprintf(filename, MAX_PATH_LEN, "%s/%s",
				   remmina_data_dir, name);
			remminafile = remmina_file_load(filename);
			if (remminafile) {
				(*func)(remminafile, user_data);
				remmina_file_free(remminafile);
				items_count++;
			}
		}
		g_dir_close(dir);
	}
	g_free(remmina_data_dir);
	return items_count;
}

gchar *remmina_file_manager_get_groups(void)
{
	TRACE_CALL(__func__);
	gchar filename[MAX_PATH_LEN];
	GDir *dir;
	const gchar *name;
	RemminaFile *remminafile;
	RemminaStringArray *array;
	const gchar *group;
	gchar *groups;
	gchar *remmina_data_dir;

	remmina_data_dir = remmina_file_get_datadir();
	array = remmina_string_array_new();

	dir = g_dir_open(remmina_data_dir, 0, NULL);

	if (dir == NULL)
		return 0;
	while ((name = g_dir_read_name(dir)) != NULL) {
		if (!g_str_has_suffix(name, ".remmina"))
			continue;
		g_snprintf(filename, MAX_PATH_LEN, "%s/%s", remmina_data_dir, name);
		remminafile = remmina_file_load(filename);
		if (remminafile) {
			group = remmina_file_get_string(remminafile, "group");
			if (group && remmina_string_array_find(array, group) < 0)
				remmina_string_array_add(array, group);
			remmina_file_free(remminafile);
		}
	}
	g_dir_close(dir);
	remmina_string_array_sort(array);
	groups = remmina_string_array_to_string(array);
	remmina_string_array_free(array);
	g_free(remmina_data_dir);
	return groups;
}

static void remmina_file_manager_add_group(GNode *node, const gchar *group)
{
	TRACE_CALL(__func__);
	gint cmp;
	gchar *p1;
	gchar *p2;
	GNode *child;
	gboolean found;
	RemminaGroupData *data;

	if (node == NULL)
		return;

	if (group == NULL || group[0] == '\0')
		return;

	p1 = g_strdup(group);
	p2 = strchr(p1, '/');

	if (p2)
		*p2++ = '\0';

	found = FALSE;

	for (child = g_node_first_child(node); child; child = g_node_next_sibling(child)) {
		cmp = g_strcmp0(((RemminaGroupData *)child->data)->name, p1);

		if (cmp == 0) {
			found = TRUE;
			break;
		}

		if (cmp > 0)
			break;
	}

	if (!found) {
		data = g_new0(RemminaGroupData, 1);
		data->name = p1;
		if (node->data)
			data->group = g_strdup_printf("%s/%s", ((RemminaGroupData *)node->data)->group, p1);
		else
			data->group = g_strdup(p1);
		if (child)
			child = g_node_insert_data_before(node, child, data);
		else
			child = g_node_append_data(node, data);
	}
	remmina_file_manager_add_group(child, p2);

	if (found)
		g_free(p1);
}

GNode *remmina_file_manager_get_group_tree(void)
{
	TRACE_CALL(__func__);
	gchar filename[MAX_PATH_LEN];
	GDir *dir;
	g_autofree gchar *datadir = NULL;
	const gchar *name;
	RemminaFile *remminafile;
	const gchar *group;
	GNode *root;

	root = g_node_new(NULL);

	datadir = g_strdup(remmina_file_get_datadir());
	dir = g_dir_open(datadir, 0, NULL);

	if (dir == NULL)
		return root;
	while ((name = g_dir_read_name(dir)) != NULL) {
		if (!g_str_has_suffix(name, ".remmina"))
			continue;
		g_snprintf(filename, MAX_PATH_LEN, "%s/%s", datadir, name);
		remminafile = remmina_file_load(filename);
		if (remminafile) {
			group = remmina_file_get_string(remminafile, "group");
			remmina_file_manager_add_group(root, group);
			remmina_file_free(remminafile);
		}
	}
	g_dir_close(dir);
	return root;
}

void remmina_file_manager_free_group_tree(GNode *node)
{
	TRACE_CALL(__func__);
	RemminaGroupData *data;
	GNode *child;

	if (!node)
		return;
	data = (RemminaGroupData *)node->data;
	if (data) {
		g_free(data->name);
		g_free(data->group);
		g_free(data);
		node->data = NULL;
	}
	for (child = g_node_first_child(node); child; child = g_node_next_sibling(child))
		remmina_file_manager_free_group_tree(child);
	g_node_unlink(node);
}

RemminaFile *remmina_file_manager_load_file(const gchar *filename)
{
	TRACE_CALL(__func__);
	RemminaFile *remminafile = NULL;
	RemminaFilePlugin *plugin;
	gchar *p;

	if ((p = strrchr(filename, '.')) != NULL && g_strcmp0(p + 1, "remmina") == 0) {
		remminafile = remmina_file_load(filename);
	} else {
		plugin = remmina_plugin_manager_get_import_file_handler(filename);
		if (plugin)
			remminafile = plugin->import_func(filename);
	}
	return remminafile;
}
