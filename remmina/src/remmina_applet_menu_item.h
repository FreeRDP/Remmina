/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAAPPLETMENUITEM_H__
#define __REMMINAAPPLETMENUITEM_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_APPLET_MENU_ITEM            (remmina_applet_menu_item_get_type())
#define REMMINA_APPLET_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), REMMINA_TYPE_APPLET_MENU_ITEM, RemminaAppletMenuItem))
#define REMMINA_APPLET_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST((klass), REMMINA_TYPE_APPLET_MENU_ITEM, RemminaAppletMenuItemClass))
#define REMMINA_IS_APPLET_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), REMMINA_TYPE_APPLET_MENU_ITEM))
#define REMMINA_IS_APPLET_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass), REMMINA_TYPE_APPLET_MENU_ITEM))
#define REMMINA_APPLET_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS((obj), REMMINA_TYPE_APPLET_MENU_ITEM, RemminaAppletMenuItemClass))

typedef enum
{
	REMMINA_APPLET_MENU_ITEM_FILE, REMMINA_APPLET_MENU_ITEM_NEW, REMMINA_APPLET_MENU_ITEM_DISCOVERED
} RemminaAppletMenuItemType;

typedef struct _RemminaAppletMenuItem
{
	GtkImageMenuItem image_menu_item;

	RemminaAppletMenuItemType item_type;
	gchar* filename;
	gchar* name;
	gchar* group;
	gchar* protocol;
	gchar* server;
	gboolean ssh_enabled;
} RemminaAppletMenuItem;

typedef struct _RemminaAppletMenuItemClass
{
	GtkImageMenuItemClass parent_class;
} RemminaAppletMenuItemClass;

GType remmina_applet_menu_item_get_type(void)
G_GNUC_CONST;

GtkWidget* remmina_applet_menu_item_new(RemminaAppletMenuItemType item_type, ...);
gint remmina_applet_menu_item_compare(gconstpointer a, gconstpointer b, gpointer user_data);

G_END_DECLS

#endif  /* __REMMINAAPPLETMENUITEM_H__  */

