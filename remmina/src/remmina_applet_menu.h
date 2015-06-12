/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAAPPLETMENU_H__
#define __REMMINAAPPLETMENU_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_APPLET_MENU            (remmina_applet_menu_get_type ())
#define REMMINA_APPLET_MENU(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_APPLET_MENU, RemminaAppletMenu))
#define REMMINA_APPLET_MENU_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_APPLET_MENU, RemminaAppletMenuClass))
#define REMMINA_IS_APPLET_MENU(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_APPLET_MENU))
#define REMMINA_IS_APPLET_MENU_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_APPLET_MENU))
#define REMMINA_APPLET_MENU_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_APPLET_MENU, RemminaAppletMenuClass))

typedef enum
{
	REMMINA_APPLET_MENU_NEW_CONNECTION_NONE,
	REMMINA_APPLET_MENU_NEW_CONNECTION_TOP,
	REMMINA_APPLET_MENU_NEW_CONNECTION_BOTTOM
} RemminaAppletMenuNewConnectionType;

typedef struct _RemminaAppletMenuPriv RemminaAppletMenuPriv;

typedef struct _RemminaAppletMenu
{
	GtkMenu menu;

	RemminaAppletMenuPriv* priv;
} RemminaAppletMenu;

typedef struct _RemminaAppletMenuClass
{
	GtkMenuClass parent_class;

	void (*launch_item)(RemminaAppletMenu* menu);
	void (*edit_item)(RemminaAppletMenu* menu);
} RemminaAppletMenuClass;

GType remmina_applet_menu_get_type(void)
G_GNUC_CONST;

void remmina_applet_menu_register_item(RemminaAppletMenu* menu, RemminaAppletMenuItem* menuitem);
void remmina_applet_menu_add_item(RemminaAppletMenu* menu, RemminaAppletMenuItem* menuitem);
GtkWidget* remmina_applet_menu_new(void);
void remmina_applet_menu_set_hide_count(RemminaAppletMenu* menu, gboolean hide_count);
void remmina_applet_menu_populate(RemminaAppletMenu* menu);

G_END_DECLS

#endif  /* __REMMINAAPPLETMENU_H__  */

