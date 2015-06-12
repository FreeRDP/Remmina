/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINACHATWINDOW_H__
#define __REMMINACHATWINDOW_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_CHAT_WINDOW               (remmina_chat_window_get_type ())
#define REMMINA_CHAT_WINDOW(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_CHAT_WINDOW, RemminaChatWindow))
#define REMMINA_CHAT_WINDOW_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_CHAT_WINDOW, RemminaChatWindowClass))
#define REMMINA_IS_CHAT_WINDOW(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_CHAT_WINDOW))
#define REMMINA_IS_CHAT_WINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_CHAT_WINDOW))
#define REMMINA_CHAT_WINDOW_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_CHAT_WINDOW, RemminaChatWindowClass))

typedef struct _RemminaChatWindow
{
	GtkWindow window;

	GtkWidget *history_text;
	GtkWidget *send_text;
} RemminaChatWindow;

typedef struct _RemminaChatWindowClass
{
	GtkWindowClass parent_class;

	void (*send)(RemminaChatWindow *window);
} RemminaChatWindowClass;

GType remmina_chat_window_get_type(void)
G_GNUC_CONST;

GtkWidget* remmina_chat_window_new(GtkWindow *parent, const gchar *chat_with);
void remmina_chat_window_receive(RemminaChatWindow *window, const gchar *name, const gchar *text);

G_END_DECLS

#endif  /* __REMMINACHATWINDOW_H__  */

