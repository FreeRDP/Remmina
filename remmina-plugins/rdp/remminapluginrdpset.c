/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010 Vic Lee 
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

#include "remminapluginrdp.h"
#include "remminapluginrdpset.h"
#include <freerdp/kbd.h>

static guint rdp_keyboard_layout = 0;
static guint keyboard_layout = 0;

void
remmina_plugin_rdpset_init (void)
{
    gchar *value;

    value = remmina_plugin_service->pref_get_value ("rdp_keyboard_layout");
    if (value && value[0])
    {
        rdp_keyboard_layout = strtoul (value, NULL, 16);
    }
    g_free (value);

    keyboard_layout = freerdp_kbd_init (rdp_keyboard_layout);
}

guint
remmina_plugin_rdpset_get_keyboard_layout (void)
{
    return keyboard_layout;
}

#define REMMINA_TYPE_PLUGIN_RDPSET_DIALOG               (remmina_plugin_rdpset_dialog_get_type ())
#define REMMINA_PLUGIN_RDPSET_DIALOG(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_PLUGIN_RDPSET_DIALOG, RemminaPluginRdpsetDialog))
#define REMMINA_PLUGIN_RDPSET_DIALOG_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_PLUGIN_RDPSET_DIALOG, RemminaPluginRdpsetDialogClass))
#define REMMINA_IS_PLUGIN_RDPSET_DIALOG(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_PLUGIN_RDPSET_DIALOG))
#define REMMINA_IS_PLUGIN_RDPSET_DIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_PLUGIN_RDPSET_DIALOG))
#define REMMINA_PLUGIN_RDPSET_DIALOG_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_PLUGIN_RDPSET_DIALOG, RemminaPluginRdpsetDialogClass))

typedef struct _RemminaPluginRdpsetDialog
{
    GtkDialog dialog;

    GtkWidget *keyboard_layout_label;
    GtkWidget *keyboard_layout_combo;
    GtkListStore *keyboard_layout_store; 
} RemminaPluginRdpsetDialog;

typedef struct _RemminaPluginRdpsetDialogClass
{
    GtkDialogClass parent_class;
} RemminaPluginRdpsetDialogClass;

GType remmina_plugin_rdpset_dialog_get_type (void) G_GNUC_CONST;

G_DEFINE_TYPE (RemminaPluginRdpsetDialog, remmina_plugin_rdpset_dialog, GTK_TYPE_DIALOG)

static void
remmina_plugin_rdpset_dialog_class_init (RemminaPluginRdpsetDialogClass *klass)
{
}

static void
remmina_plugin_rdpset_dialog_on_close_clicked (GtkWidget *widget, RemminaPluginRdpsetDialog *dialog)
{
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
remmina_plugin_rdpset_dialog_destroy (GtkWidget *widget, gpointer data)
{
    RemminaPluginRdpsetDialog *dialog;
    GtkTreeIter iter;
    guint new_layout;
    gchar *s;

    dialog = REMMINA_PLUGIN_RDPSET_DIALOG (widget);
    if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (dialog->keyboard_layout_combo), &iter))
    {
        gtk_tree_model_get (GTK_TREE_MODEL (dialog->keyboard_layout_store), &iter, 0, &new_layout, -1);
        if (new_layout != rdp_keyboard_layout)
        {
            rdp_keyboard_layout = new_layout;
            s = g_strdup_printf ("%X", new_layout);
            remmina_plugin_service->pref_set_value ("rdp_keyboard_layout", s);
            g_free (s);

            keyboard_layout = freerdp_kbd_init (rdp_keyboard_layout);
        }
    }
}

static void
remmina_plugin_rdpset_dialog_load_layout (RemminaPluginRdpsetDialog *dialog)
{
    rdpKeyboardLayout *layouts;
    gint i;
    gchar *s;
    GtkTreeIter iter;

    gtk_list_store_append (dialog->keyboard_layout_store, &iter);
    gtk_list_store_set (dialog->keyboard_layout_store, &iter, 0, 0, 1, _("<Auto Detect>"), -1);
    if (rdp_keyboard_layout == 0)
    {
        gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->keyboard_layout_combo), 0);
    }
    gtk_label_set_text (GTK_LABEL (dialog->keyboard_layout_label), "-");

    layouts = freerdp_kbd_get_layouts (
        RDP_KEYBOARD_LAYOUT_TYPE_STANDARD |
        RDP_KEYBOARD_LAYOUT_TYPE_VARIANT);
    for (i = 0; layouts[i].code; i++)
    {
        s = g_strdup_printf ("%08X - %s", layouts[i].code, layouts[i].name);
        gtk_list_store_append (dialog->keyboard_layout_store, &iter);
        gtk_list_store_set (dialog->keyboard_layout_store, &iter, 0, layouts[i].code, 1, s, -1);
        if (rdp_keyboard_layout == layouts[i].code)
        {
            gtk_combo_box_set_active (GTK_COMBO_BOX (dialog->keyboard_layout_combo), i + 1);
        }
        if (keyboard_layout == layouts[i].code)
        {
            gtk_label_set_text (GTK_LABEL (dialog->keyboard_layout_label), s);
        }
        g_free (s);
    }
    free (layouts);
}

static void
remmina_plugin_rdpset_dialog_init (RemminaPluginRdpsetDialog *dialog)
{
    GtkWidget *widget;
    GtkWidget *table;
    GtkCellRenderer *renderer;

    /* Create the dialog */
    gtk_window_set_title (GTK_WINDOW (dialog), _("RDP - Global Settings"));
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);

    widget = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
    g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_plugin_rdpset_dialog_on_close_clicked), dialog);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

    g_signal_connect (G_OBJECT (dialog), "destroy", G_CALLBACK (remmina_plugin_rdpset_dialog_destroy), NULL);

    /* Create the content */
    table = gtk_table_new (2, 2, FALSE);
    gtk_widget_show (table);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 4);
    gtk_container_set_border_width (GTK_CONTAINER (table), 4);
    gtk_table_set_row_spacings (GTK_TABLE (table), 8);
    gtk_table_set_col_spacings (GTK_TABLE (table), 8);

    widget = gtk_label_new (_("Keyboard Layout"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 0, 1);

    dialog->keyboard_layout_store = gtk_list_store_new (2, G_TYPE_UINT, G_TYPE_STRING);
    widget = gtk_combo_box_new_with_model (GTK_TREE_MODEL (dialog->keyboard_layout_store));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 0, 1);

    renderer = gtk_cell_renderer_text_new ();
    gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (widget), renderer, TRUE); 
    gtk_cell_layout_add_attribute (GTK_CELL_LAYOUT (widget), renderer, "text", 1);
    dialog->keyboard_layout_combo = widget;

    widget = gtk_label_new ("-");
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 1, 2);
    dialog->keyboard_layout_label = widget;

    remmina_plugin_rdpset_dialog_load_layout (dialog);
}

void remmina_plugin_rdpset_dialog_new (void)
{
    GtkWidget *widget;

    widget = GTK_WIDGET (g_object_new (REMMINA_TYPE_PLUGIN_RDPSET_DIALOG, NULL));
    gtk_widget_show (widget);
}

