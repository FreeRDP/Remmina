/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINASSHPLUGIN_H__
#define __REMMINASSHPLUGIN_H__

#ifdef HAVE_LIBVTE
#include <vte/vte.h>
#endif

G_BEGIN_DECLS

void remmina_ssh_plugin_register(void);

/* For callback in main thread */
#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)
void remmina_plugin_ssh_vte_terminal_set_encoding_and_pty(VteTerminal *terminal, const char *codeset, int slave);
#endif

G_END_DECLS

#endif /* __REMMINASSHPLUGIN_H__ */

