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
 */
 
#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "remminachainbutton.h"
#include "remminascaler.h"

G_DEFINE_TYPE (RemminaScaler, remmina_scaler, GTK_TYPE_TABLE)

#define MIN_SCALE_VALUE 0.05

struct _RemminaScalerPriv
{
    GtkWidget *hscale_widget;
    GtkWidget *vscale_widget;
    GtkWidget *aspectscale_button;
};

enum {
    SCALED_SIGNAL,
    LAST_SIGNAL
};

static guint remmina_scaler_signals[LAST_SIGNAL] = { 0 };

static void
remmina_scaler_class_init (RemminaScalerClass *klass)
{
    remmina_scaler_signals[SCALED_SIGNAL] =
        g_signal_new ("scaled",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION,
            G_STRUCT_OFFSET (RemminaScalerClass, scaled),
            NULL, NULL,
            g_cclosure_marshal_VOID__VOID,
            G_TYPE_NONE, 0);
}

static gchar*
remmina_scaler_format_scale_value (GtkScale *scale, gdouble value, gpointer user_data)
{
    if (value <= MIN_SCALE_VALUE)
    {
        return g_strdup (_("Fit Window Size"));
    }
    else
    {
        return g_strdup_printf ("%.2f", value);
    }
}

static void
remmina_scaler_on_chain_changed (RemminaChainButton *cb, RemminaScaler *scaler)
{
    scaler->aspectscale = cb->chained;
    if (cb->chained)
    {
        gtk_range_set_value (GTK_RANGE (scaler->priv->vscale_widget),
            gtk_range_get_value (GTK_RANGE (scaler->priv->hscale_widget)));
    }
    g_signal_emit (G_OBJECT (scaler), remmina_scaler_signals[SCALED_SIGNAL], 0);
}

static void
remmina_scaler_on_hscale_value_changed (GtkWidget *widget, RemminaScaler *scaler)
{
    gdouble value;

    value = gtk_range_get_value (GTK_RANGE (scaler->priv->hscale_widget));
    scaler->hscale = (value <= MIN_SCALE_VALUE ?  0 : (gint) (value * 100.0));
    if (REMMINA_CHAIN_BUTTON (scaler->priv->aspectscale_button)->chained &&
        gtk_range_get_value (GTK_RANGE (scaler->priv->hscale_widget)) !=
        gtk_range_get_value (GTK_RANGE (scaler->priv->vscale_widget)))
    {
        gtk_range_set_value (GTK_RANGE (scaler->priv->vscale_widget),
            gtk_range_get_value (GTK_RANGE (scaler->priv->hscale_widget)));
    }
    g_signal_emit (G_OBJECT (scaler), remmina_scaler_signals[SCALED_SIGNAL], 0);
}

static void
remmina_scaler_on_vscale_value_changed (GtkWidget *widget, RemminaScaler *scaler)
{
    gdouble value;

    value = gtk_range_get_value (GTK_RANGE (scaler->priv->vscale_widget));
    scaler->vscale = (value <= MIN_SCALE_VALUE ?  0 : (gint) (value * 100.0));
    if (REMMINA_CHAIN_BUTTON (scaler->priv->aspectscale_button)->chained &&
        gtk_range_get_value (GTK_RANGE (scaler->priv->hscale_widget)) !=
        gtk_range_get_value (GTK_RANGE (scaler->priv->vscale_widget)))
    {
        gtk_range_set_value (GTK_RANGE (scaler->priv->hscale_widget),
            gtk_range_get_value (GTK_RANGE (scaler->priv->vscale_widget)));
    }
    g_signal_emit (G_OBJECT (scaler), remmina_scaler_signals[SCALED_SIGNAL], 0);
}

static void
remmina_scaler_destroy (RemminaScaler *scaler, gpointer data)
{
    g_free (scaler->priv);
}

static void
remmina_scaler_init (RemminaScaler *scaler)
{
    RemminaScalerPriv *priv;
    GtkWidget *widget;

    priv = g_new (RemminaScalerPriv, 1);
    scaler->priv = priv;
    scaler->hscale = 0;
    scaler->vscale = 0;
    scaler->aspectscale = FALSE;

    gtk_table_resize (GTK_TABLE (scaler), 2, 2);

    widget = gtk_hscale_new_with_range (MIN_SCALE_VALUE, 1.0, 0.01);
    gtk_widget_show (widget);
    gtk_widget_set_tooltip_text (widget, _("Horizontal Scale"));
    gtk_table_attach_defaults (GTK_TABLE (scaler), widget, 1, 2, 0, 1);
    g_signal_connect (G_OBJECT (widget), "format-value", G_CALLBACK (remmina_scaler_format_scale_value), NULL);
    priv->hscale_widget = widget;

    widget = gtk_hscale_new_with_range (MIN_SCALE_VALUE, 1.0, 0.01);
    gtk_widget_show (widget);
    gtk_widget_set_tooltip_text (widget, _("Vertical Scale"));
    gtk_table_attach_defaults (GTK_TABLE (scaler), widget, 1, 2, 1, 2);
    g_signal_connect (G_OBJECT (widget), "format-value", G_CALLBACK (remmina_scaler_format_scale_value), NULL);
    priv->vscale_widget = widget;

    g_signal_connect (G_OBJECT (priv->hscale_widget), "value-changed",
        G_CALLBACK (remmina_scaler_on_hscale_value_changed), scaler);
    g_signal_connect (G_OBJECT (priv->vscale_widget), "value-changed",
        G_CALLBACK (remmina_scaler_on_vscale_value_changed), scaler);

    widget = remmina_chain_button_new ();
    gtk_widget_show (widget);
    gtk_table_attach (GTK_TABLE (scaler), widget, 2, 3, 0, 2, 0, 0, 0, 0);
    g_signal_connect (G_OBJECT (widget), "chain-toggled", G_CALLBACK (remmina_scaler_on_chain_changed), scaler);
    priv->aspectscale_button = widget;

    g_signal_connect (G_OBJECT (scaler), "destroy", G_CALLBACK (remmina_scaler_destroy), NULL);
}

GtkWidget*
remmina_scaler_new (void)
{
    return GTK_WIDGET (g_object_new (REMMINA_TYPE_SCALER, NULL));
}

void
remmina_scaler_set (RemminaScaler *scaler, gint hscale, gint vscale, gboolean chained)
{
    gtk_range_set_value (GTK_RANGE (scaler->priv->hscale_widget), ((gdouble) hscale) / 100.0);
    gtk_range_set_value (GTK_RANGE (scaler->priv->vscale_widget), ((gdouble) vscale) / 100.0);
    remmina_chain_button_set (REMMINA_CHAIN_BUTTON (scaler->priv->aspectscale_button), chained);
}

void
remmina_scaler_set_draw_value (RemminaScaler *scaler, gboolean draw_value)
{
    gtk_scale_set_draw_value (GTK_SCALE (scaler->priv->hscale_widget), draw_value);
    gtk_scale_set_draw_value (GTK_SCALE (scaler->priv->vscale_widget), draw_value);
}

