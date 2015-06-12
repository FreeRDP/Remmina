/*  See LICENSE and COPYING files for copyright and license details. */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include "remmina/types.h"
#include "remmina_public.h"
#include "remmina_external_tools.h"
#include "remmina/remmina_trace_calls.h"

typedef struct _RemminaExternalTools
{
	gchar remminafilename[MAX_PATH_LEN];
	gchar scriptfilename[MAX_PATH_LEN];
	gchar scriptshortname[MAX_PATH_LEN];
} RemminaExternalTools;

static gboolean remmina_external_tools_launcher(const gchar* filename, const gchar* scriptname, const gchar* shortname);

void view_popup_menu_onDoSomething (GtkWidget *menuitem, gpointer userdata)
{
	TRACE_CALL("view_popup_menu_onDoSomething");
	/* we passed the view as userdata when we connected the signal */
	RemminaExternalTools *ret = (RemminaExternalTools *)userdata;
	//gchar* filename_remmina = ret->remminafilename;
	//gchar* filename_script = ret->scriptfilename;

	remmina_external_tools_launcher(ret->remminafilename, ret->scriptfilename, ret->scriptshortname);
}

gboolean remmina_external_tools_from_filename(RemminaMain *remminamain, gchar* remminafilename)
{
	TRACE_CALL("remmina_external_tools_from_filename");
	GtkWidget *menu, *menuitem;
	menu = gtk_menu_new();
	gchar dirname[MAX_PATH_LEN];
	gchar filename[MAX_PATH_LEN];
	GDir* dir;
	const gchar* name;

	strcpy(dirname, REMMINA_EXTERNAL_TOOLS_DIR);
	dir = g_dir_open(dirname, 0, NULL);

	if (dir == NULL)
		return FALSE;

	while ((name = g_dir_read_name(dir)) != NULL)
	{
		if (!g_str_has_prefix(name, "remmina_"))
			continue;
		g_snprintf(filename, MAX_PATH_LEN, "%s/%s", dirname, name);
		RemminaExternalTools *ret;
		ret = (RemminaExternalTools *)malloc(sizeof(RemminaExternalTools));
		strcpy(ret->remminafilename,remminafilename);
		strcpy(ret->scriptfilename,filename);
		strcpy(ret->scriptshortname,name);
		menuitem = gtk_menu_item_new_with_label(strndup(name + 8, strlen(name) -8));
		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_onDoSomething, ret);

		//g_signal_connect(menuitem, "activate",
		//                  (GCallback) view_popup_menu_onDoSomething, treeview);

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
	char *env;
	size_t envstrlen;
	gchar launcher[MAX_PATH_LEN];

	g_snprintf(launcher, MAX_PATH_LEN, "%s/launcher.sh", REMMINA_EXTERNAL_TOOLS_DIR);

	remminafile = remmina_file_load(filename);
	GHashTableIter iter;
	const gchar *key, *value;
	g_hash_table_iter_init(&iter, remminafile->settings);
	while (g_hash_table_iter_next(&iter, (gpointer*) &key, (gpointer*) &value))
	{
		envstrlen = strlen(key) +strlen(value) + strlen(env_format) + 1;
		env = (char *) malloc(envstrlen);
		if (env == NULL)
		{
			return -1;
		}

		int retval = snprintf(env, envstrlen, env_format, key,value);
		if (retval > 0 && (size_t) retval <= envstrlen)
		{
			if (putenv(env) !=0)
			{
				/* If putenv fails, we must free the unused space */
				free(env);
			}
		}
	}
	/* Adds the window title for the terminal window */
	const char *term_title_key = "remmina_term_title";
	const char *term_title_val_prefix = "Remmina external tool";
	envstrlen = strlen(term_title_key) + strlen(term_title_val_prefix) + strlen(shortname) + 7;
	env = (char *) malloc(envstrlen);
	if (env != NULL)
	{
		if (snprintf(env, envstrlen, "%s=%s: %s", term_title_key, term_title_val_prefix, shortname) )
		{
			if (putenv(env) != 0)
			{
				/* If putenv fails, we must free the unused space */
				free(env);
			}
		}
	}

	const size_t cmdlen = strlen(launcher) +strlen(scriptname) + 2;
	gchar *cmd = (gchar *)malloc(cmdlen);
	g_snprintf(cmd, cmdlen, "%s %s", launcher, scriptname);
	system(cmd);
	free(cmd);

	remmina_file_free(remminafile);

	return TRUE;
}
