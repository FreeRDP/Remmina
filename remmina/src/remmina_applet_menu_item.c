/*  See LICENSE and COPYING files for copyright and license details. */

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <string.h>
#include <stdarg.h>
#include "remmina_applet_menu_item.h"
#include "remmina/remmina_trace_calls.h"

G_DEFINE_TYPE( RemminaAppletMenuItem, remmina_applet_menu_item, GTK_TYPE_MENU_ITEM)

#define IS_EMPTY(s) ((!s)||(s[0]==0))

static void remmina_applet_menu_item_destroy(RemminaAppletMenuItem* item, gpointer data)
{
	TRACE_CALL("remmina_applet_menu_item_destroy");
	g_free(item->filename);
	g_free(item->name);
	g_free(item->group);
	g_free(item->protocol);
	g_free(item->server);
}

static void remmina_applet_menu_item_class_init(RemminaAppletMenuItemClass* klass)
{
	TRACE_CALL("remmina_applet_menu_item_class_init");
}

static void remmina_applet_menu_item_init(RemminaAppletMenuItem* item)
{
	TRACE_CALL("remmina_applet_menu_item_init");
	item->filename = NULL;
	item->name = NULL;
	item->group = NULL;
	item->protocol = NULL;
	item->server = NULL;
	item->ssh_enabled = FALSE;
	g_signal_connect(G_OBJECT(item), "destroy", G_CALLBACK(remmina_applet_menu_item_destroy), NULL);
}

GtkWidget* remmina_applet_menu_item_new(RemminaAppletMenuItemType item_type, ...)
{
	TRACE_CALL("remmina_applet_menu_item_new");
	va_list ap;
	RemminaAppletMenuItem* item;
	GKeyFile* gkeyfile;
	GtkWidget* widget;
	const gchar* iconname;

	va_start(ap, item_type);

	item = REMMINA_APPLET_MENU_ITEM(g_object_new(REMMINA_TYPE_APPLET_MENU_ITEM, NULL));

	item->item_type = item_type;

	switch (item_type)
	{
		case REMMINA_APPLET_MENU_ITEM_FILE:
			item->filename = g_strdup(va_arg(ap, const gchar*));

			/* Load the file */
			gkeyfile = g_key_file_new();

			if (!g_key_file_load_from_file(gkeyfile, item->filename, G_KEY_FILE_NONE, NULL))
			{
				g_key_file_free(gkeyfile);
				va_end(ap);
				return NULL;
			}

			item->name = g_key_file_get_string(gkeyfile, "remmina", "name", NULL);
			item->group = g_key_file_get_string(gkeyfile, "remmina", "group", NULL);
			item->protocol = g_key_file_get_string(gkeyfile, "remmina", "protocol", NULL);
			item->server = g_key_file_get_string(gkeyfile, "remmina", "server", NULL);
			item->ssh_enabled = g_key_file_get_boolean(gkeyfile, "remmina", "ssh_enabled", NULL);

			g_key_file_free(gkeyfile);
			break;

		case REMMINA_APPLET_MENU_ITEM_DISCOVERED:
			item->name = g_strdup(va_arg(ap, const gchar *));
			item->group = g_strdup(_("Discovered"));
			item->protocol = g_strdup("VNC");
			break;

		case REMMINA_APPLET_MENU_ITEM_NEW:
			item->name = g_strdup(_("New Connection"));
			break;
	}

	va_end(ap);

	/* Create the label */
	widget = gtk_label_new(item->name);
	gtk_widget_show(widget);
	gtk_widget_set_valign (widget, GTK_ALIGN_START);
	gtk_widget_set_halign (widget, GTK_ALIGN_START);
	gtk_container_add(GTK_CONTAINER(item), widget);

	/* Create the image */

	if (item_type == REMMINA_APPLET_MENU_ITEM_FILE || item_type == REMMINA_APPLET_MENU_ITEM_DISCOVERED)
	{
		if (g_strcmp0(item->protocol, "SFTP") == 0)
		{
			iconname = "remmina-sftp";
		}
		else if (g_strcmp0(item->protocol, "SSH") == 0)
		{
			iconname = "utilities-terminal";
		}
		else if (g_strcmp0(item->protocol, "RDP") == 0)
		{
			iconname = (item->ssh_enabled ? "remmina-rdp-ssh" : "remmina-rdp");
		}
		else if (strncmp (item->protocol, "VNC", 3) == 0)
		{
			iconname = (item->ssh_enabled ? "remmina-vnc-ssh" : "remmina-vnc");
		}
		else if (g_strcmp0(item->protocol, "XDMCP") == 0)
		{
			iconname = (item->ssh_enabled ? "remmina-xdmcp-ssh" : "remmina-xdmcp");
		}
		else if (g_strcmp0(item->protocol, "NX") == 0)
		{
			iconname = "remmina-nx";
		}
		else
		{
			iconname = "remmina";
		}
		widget = gtk_image_new_from_icon_name(iconname, GTK_ICON_SIZE_MENU);
	}
	else
	{
		widget = gtk_image_new_from_icon_name("go-jump", GTK_ICON_SIZE_MENU);
	}

	gtk_widget_show(widget);

	if (item->server)
	{
		gtk_widget_set_tooltip_text(GTK_WIDGET(item), item->server);
	}

	return GTK_WIDGET(item);
}

gint remmina_applet_menu_item_compare(gconstpointer a, gconstpointer b, gpointer user_data)
{
	TRACE_CALL("remmina_applet_menu_item_compare");
	gint cmp;
	RemminaAppletMenuItem* itema;
	RemminaAppletMenuItem* itemb;

	/* Passed in parameters are pointers to pointers */
	itema = REMMINA_APPLET_MENU_ITEM(*((void**) a));
	itemb = REMMINA_APPLET_MENU_ITEM(*((void**) b));

	/* Put ungrouped items to the last */
	if (IS_EMPTY(itema->group) && !IS_EMPTY(itemb->group))
		return 1;
	if (!IS_EMPTY(itema->group) && IS_EMPTY(itemb->group))
		return -1;

	/* Put discovered items the last group */
	if (itema->item_type == REMMINA_APPLET_MENU_ITEM_DISCOVERED && itemb->item_type != REMMINA_APPLET_MENU_ITEM_DISCOVERED)
		return 1;
	if (itema->item_type != REMMINA_APPLET_MENU_ITEM_DISCOVERED && itemb->item_type == REMMINA_APPLET_MENU_ITEM_DISCOVERED)
		return -1;

	if (itema->item_type != REMMINA_APPLET_MENU_ITEM_DISCOVERED && !IS_EMPTY(itema->group))
	{
		cmp = g_strcmp0(itema->group, itemb->group);

		if (cmp != 0)
			return cmp;
	}

	return g_strcmp0(itema->name, itemb->name);
}
