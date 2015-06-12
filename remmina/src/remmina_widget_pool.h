/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAWIDGETPOOL_H__
#define __REMMINAWIDGETPOOL_H__

G_BEGIN_DECLS

typedef gboolean (*RemminaWidgetPoolForEachFunc)(GtkWidget *widget, gpointer data);

void remmina_widget_pool_init(void);
void remmina_widget_pool_register(GtkWidget *widget);
GtkWidget* remmina_widget_pool_find(GType type, const gchar *tag);
GtkWidget* remmina_widget_pool_find_by_window(GType type, GdkWindow *window);
void remmina_widget_pool_hold(gboolean hold);
gint remmina_widget_pool_foreach(RemminaWidgetPoolForEachFunc callback, gpointer data);

G_END_DECLS

#endif  /* __REMMINAWIDGETPOOL_H__  */

