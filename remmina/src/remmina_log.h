/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINALOG_H__
#define __REMMINALOG_H__

G_BEGIN_DECLS

void remmina_log_start(void);
gboolean remmina_log_running(void);
void remmina_log_print(const gchar *text);
void remmina_log_printf(const gchar *fmt, ...);

G_END_DECLS

#endif  /* __REMMINALOG_H__  */

