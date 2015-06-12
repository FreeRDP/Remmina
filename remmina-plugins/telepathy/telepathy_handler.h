/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINATPHANDLER_H__
#define __REMMINATPHANDLER_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_TP_HANDLER           (remmina_tp_handler_get_type ())
#define REMMINA_TP_HANDLER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_TP_HANDLER, RemminaTpHandler))
#define REMMINA_TP_HANDLER_CLASS(obj)     (G_TYPE_CHECK_CLASS_CAST ((obj), REMMINA_TYPE_TP_HANDLER, RemminaTpHandlerClass))
#define REMMINA_IS_TP_HANDLER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_TP_HANDLER))
#define REMMINA_IS_TP_HANDLER_CLASS(obj)  (G_TYPE_CHECK_CLASS_TYPE ((obj), REMMINA_TYPE_TP_HANDLER))
#define REMMINA_TP_HANDLER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_TP_HANDLER, RemminaTpHandlerClass))

typedef struct _RemminaTpHandler
{
	GObject parent;
} RemminaTpHandler;

typedef struct _RemminaTpHandlerClass
{
	GObjectClass parent_class;
} RemminaTpHandlerClass;

RemminaTpHandler* remmina_tp_handler_new(void);

G_END_DECLS

#endif

