/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINA_RDP_SETTINGS_H__
#define __REMMINA_RDP_SETTINGS_H__

G_BEGIN_DECLS

void remmina_rdp_settings_init(void);
guint remmina_rdp_settings_get_keyboard_layout(void);
GtkWidget* remmina_rdp_settings_new(void);

G_END_DECLS

#endif

