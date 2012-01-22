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

#include <gtk/gtk.h>
#include <string.h>
#include "remmina_public.h"
#include "remmina_string_array.h"
#include "remmina_plugin_manager.h"
#include "remmina_file_manager.h"

void remmina_file_manager_init(void)
{
	gchar dirname[MAX_PATH_LEN];

	g_snprintf(dirname, MAX_PATH_LEN, "%s/.remmina", g_get_home_dir());
	g_mkdir_with_parents(dirname, 0700);
}

gint remmina_file_manager_iterate(GFunc func, gpointer user_data)
{
	gchar dirname[MAX_PATH_LEN];
	gchar filename[MAX_PATH_LEN];
	GDir* dir;
	const gchar* name;
	RemminaFile* remminafile;
	gint n;

	g_snprintf(dirname, MAX_PATH_LEN, "%s/.remmina", g_get_home_dir());
	dir = g_dir_open(dirname, 0, NULL);

	if (dir == NULL)
		return 0;

	n = 0;
	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if (!g_str_has_suffix(name, ".remmina"))
			continue;
		g_snprintf(filename, MAX_PATH_LEN, "%s/%s", dirname, name);
		remminafile = remmina_file_load(filename);
		if (remminafile)
		{
			(*func)(remminafile, user_data);
			remmina_file_free(remminafile);
			n++;
		}
	}
	g_dir_close(dir);
	return n;
}

gchar* remmina_file_manager_get_groups(void)
{
	gchar dirname[MAX_PATH_LEN];
	gchar filename[MAX_PATH_LEN];
	GDir* dir;
	const gchar* name;
	RemminaFile* remminafile;
	RemminaStringArray* array;
	const gchar* group;
	gchar* groups;

	array = remmina_string_array_new();

	g_snprintf(dirname, MAX_PATH_LEN, "%s/.remmina", g_get_home_dir());
	dir = g_dir_open(dirname, 0, NULL);
	if (dir == NULL)
		return 0;
	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if (!g_str_has_suffix(name, ".remmina"))
			continue;
		g_snprintf(filename, MAX_PATH_LEN, "%s/%s", dirname, name);
		remminafile = remmina_file_load(filename);
		group = remmina_file_get_string(remminafile, "group");
		if (group && remmina_string_array_find(array, group) < 0)
		{
			remmina_string_array_add(array, group);
		}
		remmina_file_free(remminafile);
	}
	g_dir_close(dir);
	remmina_string_array_sort(array);
	groups = remmina_string_array_to_string(array);
	remmina_string_array_free(array);
	return groups;
}

static void remmina_file_manager_add_group(GNode* node, const gchar* group)
{
	gint cmp;
	gchar* p1;
	gchar* p2;
	GNode* child;
	gboolean found;
	RemminaGroupData* data;

	if (group == NULL || group[0] == '\0')
		return;

	p1 = g_strdup(group);
	p2 = strchr(p1, '/');

	if (p2)
		*p2++ = '\0';

	found = FALSE;

	for (child = g_node_first_child(node); child; child = g_node_next_sibling(child))
	{
		cmp = g_strcmp0(((RemminaGroupData*) child->data)->name, p1);

		if (cmp == 0)
		{
			found = TRUE;
			break;
		}

		if (cmp > 0)
			break;
	}

	if (!found)
	{
		data = g_new0(RemminaGroupData, 1);
		data->name = p1;
		if (node->data)
		{
			data->group = g_strdup_printf("%s/%s", ((RemminaGroupData*) node->data)->group, p1);
		}
		else
		{
			data->group = g_strdup(p1);
		}
		if (child)
		{
			child = g_node_insert_data_before(node, child, data);
		}
		else
		{
			child = g_node_append_data(node, data);
		}
	}
	remmina_file_manager_add_group(child, p2);

	if (found)
	{
		g_free(p1);
	}
}

GNode* remmina_file_manager_get_group_tree(void)
{
	gchar dirname[MAX_PATH_LEN];
	gchar filename[MAX_PATH_LEN];
	GDir* dir;
	const gchar* name;
	RemminaFile* remminafile;
	const gchar* group;
	GNode* root;

	root = g_node_new(NULL);

	g_snprintf(dirname, MAX_PATH_LEN, "%s/.remmina", g_get_home_dir());
	dir = g_dir_open(dirname, 0, NULL);
	if (dir == NULL)
		return root;
	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if (!g_str_has_suffix(name, ".remmina"))
			continue;
		g_snprintf(filename, MAX_PATH_LEN, "%s/%s", dirname, name);
		remminafile = remmina_file_load(filename);
		group = remmina_file_get_string(remminafile, "group");
		remmina_file_manager_add_group(root, group);
		remmina_file_free(remminafile);
	}
	g_dir_close(dir);
	return root;
}

void remmina_file_manager_free_group_tree(GNode* node)
{
	RemminaGroupData* data;
	GNode* child;

	if (!node)
		return;
	data = (RemminaGroupData*) node->data;
	if (data)
	{
		g_free(data->name);
		g_free(data->group);
		g_free(data);
		node->data = NULL;
	}
	for (child = g_node_first_child(node); child; child = g_node_next_sibling(child))
	{
		remmina_file_manager_free_group_tree(child);
	}
	g_node_unlink(node);
}

RemminaFile* remmina_file_manager_load_file(const gchar* filename)
{
	RemminaFile* remminafile = NULL;
	RemminaFilePlugin* plugin;
	gchar* p;

	if ((p = strrchr(filename, '.')) != NULL && g_strcmp0(p + 1, "remmina") == 0)
	{
		remminafile = remmina_file_load(filename);
	}
	else
	{
		plugin = remmina_plugin_manager_get_import_file_handler(filename);
		if (plugin)
		{
			remminafile = plugin->import_func(filename);
		}
	}
	return remminafile;
}

