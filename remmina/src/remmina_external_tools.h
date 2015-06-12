/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAEXTERNALTOOLS_H__
#define __REMMINAEXTERNALTOOLS_H__

#include <gtk/gtk.h>
#include "remmina_file.h"
#include "remmina_main.h"

G_BEGIN_DECLS

/* Open a new connection window for a .remmina file */
gboolean remmina_external_tools_from_filename(RemminaMain *remminamain, gchar* filename);

G_END_DECLS

#endif  /* __REMMINAEXTERNALTOOLS_H__  */

