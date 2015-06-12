/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINATPCHANNELHANDLER_H__
#define __REMMINATPCHANNELHANDLER_H__

G_BEGIN_DECLS

void
remmina_tp_channel_handler_new(const gchar *account_path, const gchar *connection_path, const gchar *channel_path,
		GHashTable *channel_properties, DBusGMethodInvocation *context);

G_END_DECLS

#endif
