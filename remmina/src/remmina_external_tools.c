/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2011 Marc-Andre Moreau
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
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include "remmina/types.h"
#include "remmina_public.h"
#include "remmina_external_tools.h"
#include "remmina/remmina_trace_calls.h"

static gboolean remmina_external_tools_launcher(const gchar* filename, const gchar* scriptname, const gchar* shortname);

static void view_popup_menu_onDoSomething (GtkWidget *menuitem, gpointer userdata)
{
	TRACE_CALL("view_popup_menu_onDoSomething");
	gchar *remminafilename = g_object_get_data(G_OBJECT(menuitem), "remminafilename");
	gchar *scriptfilename = g_object_get_data(G_OBJECT(menuitem), "scriptfilename");
	gchar *scriptshortname = g_object_get_data(G_OBJECT(menuitem), "scriptshortname");

	remmina_external_tools_launcher(remminafilename, scriptfilename, scriptshortname);
}

gboolean remmina_external_tools_from_filename(RemminaMain *remminamain, gchar* remminafilename)
{
	TRACE_CALL("remmina_external_tools_from_filename");
	GtkWidget *menu, *menuitem;
	gchar dirname[MAX_PATH_LEN];
	gchar filename[MAX_PATH_LEN];
	GDir* dir;
	const gchar* name;

	strcpy(dirname, REMMINA_EXTERNAL_TOOLS_DIR);
	dir = g_dir_open(dirname, 0, NULL);

	if (dir == NULL)
		return FALSE;

	menu = gtk_menu_new();

	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if (!g_str_has_prefix(name, "remmina_"))
			continue;
		g_snprintf(filename, MAX_PATH_LEN, "%s/%s", dirname, name);

		menuitem = gtk_menu_item_new_with_label(name + 8);
		g_object_set_data_full(G_OBJECT(menuitem), "remminafilename", g_strdup(remminafilename), g_free);
		g_object_set_data_full(G_OBJECT(menuitem), "scriptfilename", g_strdup(filename), g_free);
		g_object_set_data_full(G_OBJECT(menuitem), "scriptshortname", g_strdup(name), g_free);
		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onDoSomething, NULL);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}
	g_dir_close(dir);

	gtk_widget_show_all(menu);

	/* Note: event can be NULL here when called from view_onPopupMenu;
	*  gdk_event_get_time() accepts a NULL argument
	*/
	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,0,0);

	return TRUE;
}

static gboolean remmina_external_tools_launcher(const gchar* filename, const gchar* scriptname, const gchar* shortname)
{
	TRACE_CALL("remmina_external_tools_launcher");
	RemminaFile *remminafile;
	const char *env_format = "%s=%s";
	char **env1 = NULL, *env2 = NULL;
	size_t envstrlen;
	gchar launcher[MAX_PATH_LEN];
    gboolean rc = FALSE;
    size_t i = 0, envvars = 0, alloc = 0;
    gchar *cmd = NULL;

	g_snprintf(launcher, MAX_PATH_LEN, "%s/launcher.sh", REMMINA_EXTERNAL_TOOLS_DIR);

	remminafile = remmina_file_load(filename);
	GHashTableIter iter;
	const gchar *key, *value;
	g_hash_table_iter_init(&iter, remminafile->settings);
	while (g_hash_table_iter_next(&iter, (gpointer*) &key, (gpointer*) &value))
	{
        char **tmp = realloc(env1, (alloc + 10) * sizeof(char*));
        if (!tmp)
            goto cleanup;

        env1 = tmp;
        memset(&env1[alloc], 0, 10 * sizeof(char*));
        alloc += 10;

        i = envvars++;
		envstrlen = strlen(key) +strlen(value) + strlen(env_format) + 1;
		env1[i] = (char *) malloc(envstrlen);
		if (env1[i] == NULL)
            goto cleanup;

		int retval = snprintf(env1[i], envstrlen, env_format, key,value);
		if (retval <= 0 || (size_t) retval > envstrlen)
            goto cleanup;

        if (putenv(env1[i]) != 0)
            goto cleanup;
	}
	/* Adds the window title for the terminal window */
	const char *term_title_key = "remmina_term_title";
	const char *term_title_val_prefix = "Remmina external tool";
	envstrlen = strlen(term_title_key) + strlen(term_title_val_prefix) + strlen(shortname) + 7;
    if (envstrlen)
    {
        env2 = (char *) malloc(envstrlen);
        if (env2 == NULL)
            goto cleanup;

        if (snprintf(env2, envstrlen, "%s=%s: %s", term_title_key, term_title_val_prefix, shortname) == 0)
            goto cleanup;
            
        if (putenv(env2) != 0)
            goto cleanup;
    }

	const size_t cmdlen = strlen(launcher) +strlen(scriptname) + 2;
	cmd = (gchar *)malloc(cmdlen);
    if (!cmd)
        goto cleanup;
	g_snprintf(cmd, cmdlen, "%s %s", launcher, scriptname);
	system(cmd);

    rc = TRUE;

cleanup:
	remmina_file_free(remminafile);
    for (i=0; i<envvars; i++)
        free(env1[i]);
    free(env1);
    free(env2);
	free(cmd);

	return rc;
}
