#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <stdlib.h>
#include "remmina/types.h"
#include "remmina_public.h"
#include "remmina_external_tools.h"

typedef struct _RemminaExternalTools
{
	gchar remminafilename[MAX_PATH_LEN];
	gchar scriptfilename[MAX_PATH_LEN];
} RemminaExternalTools;

void view_popup_menu_onDoSomething (GtkWidget *menuitem, gpointer userdata)
{
	/* we passed the view as userdata when we connected the signal */
	RemminaExternalTools *ret = (RemminaExternalTools *)userdata;
	//gchar* filename_remmina = ret->remminafilename;
	//gchar* filename_script = ret->scriptfilename;

	remmina_external_tools_launcher(ret->remminafilename,ret->scriptfilename);
}

gboolean remmina_external_tools_from_filename(RemminaMain *remminamain,gchar* remminafilename)
{
	GtkWidget *menu, *menuitem;
	menu = gtk_menu_new();
	gchar dirname[MAX_PATH_LEN];
	gchar filename[MAX_PATH_LEN];
	GDir* dir;
	const gchar* name;
	GNode* root;
	root = g_node_new(NULL);

	g_snprintf(dirname, MAX_PATH_LEN, "%s/.remmina/external_tools", g_get_home_dir());
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

gboolean remmina_external_tools_launcher(const gchar* filename,const gchar* scriptname)
{
	RemminaFile *remminafile;
	gchar launcher[MAX_PATH_LEN];
	g_snprintf(launcher, MAX_PATH_LEN, "%s/.remmina/external_tools/launcher.sh", g_get_home_dir());

	remminafile = remmina_file_load(filename);
	GHashTableIter iter;
	const gchar *key, *value;
	g_hash_table_iter_init(&iter, remminafile->settings);
	while (g_hash_table_iter_next(&iter, (gpointer*) &key, (gpointer*) &value))
	{
		const char *env_format = "%s=%s";
		const size_t len = strlen(key) +strlen(value) + strlen(env_format);
		char *env = (char *) malloc(len);
		if (env == NULL)
		{
			return -1;
		}

		int retval = snprintf(env, len, env_format, key,value);
		if (retval < 0 || (size_t) retval >= len)
		{
			/* Handle error */
		}

		if (putenv(env) != 0)
		{
			free(env);
		}
	}
	const size_t cmdlen = strlen(launcher) +strlen(scriptname) + 2;
	gchar *cmd = (gchar *)malloc(cmdlen);
	g_snprintf(cmd, cmdlen, "%s %s", launcher, scriptname);
	system(cmd);
	free(cmd);

	return TRUE;
}
