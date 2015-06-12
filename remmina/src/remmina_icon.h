/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAICON_H__
#define __REMMINAICON_H__

G_BEGIN_DECLS

void remmina_icon_init(void);
gboolean remmina_icon_is_autostart(void);
void remmina_icon_set_autostart(gboolean autostart);
void remmina_icon_populate_menu(void);

G_END_DECLS

#endif  /* __REMMINAICON_H__  */

