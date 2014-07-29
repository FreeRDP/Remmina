/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, 
 * Boston, MA 02111-1307, USA.
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU General Public License in all respects
 *  for all of the code used other than OpenSSL. *  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so. *  If you
 *  do not wish to do so, delete this exception statement from your
 *  version. *  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#ifndef __REMMINASCALER_H__
#define __REMMINASCALER_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_SCALER               (remmina_scaler_get_type ())
#define REMMINA_SCALER(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_SCALER, RemminaScaler))
#define REMMINA_SCALER_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_SCALER, RemminaScalerClass))
#define REMMINA_IS_SCALER(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_SCALER))
#define REMMINA_IS_SCALER_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_SCALER))
#define REMMINA_SCALER_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_SCALER, RemminaScalerClass))

typedef struct _RemminaScalerPriv RemminaScalerPriv;

typedef struct _RemminaScaler
{
	GtkTable table;

	gint hscale;
	gint vscale;
	gboolean aspectscale;

	RemminaScalerPriv *priv;
} RemminaScaler;

typedef struct _RemminaScalerClass
{
	GtkTableClass parent_class;

	void (*scaled)(RemminaScaler *scaler);
} RemminaScalerClass;

GType remmina_scaler_get_type(void)
G_GNUC_CONST;

GtkWidget* remmina_scaler_new(void);

void remmina_scaler_set(RemminaScaler *scaler, gint hscale, gint vscale, gboolean chained);

void remmina_scaler_set_draw_value(RemminaScaler *scaler, gboolean draw_value);

G_END_DECLS

#endif  /* __REMMINASCALER_H__  */

