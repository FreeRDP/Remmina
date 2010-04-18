/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2010 Vic Lee 
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
#include <stdlib.h>
#include "remminapublic.h"
#include "remminastringlist.h"
#include "remminawidgetpool.h"
#include "remminakeychooser.h"
#include "remminapref.h"
#include "remminaprefdialog.h"

G_DEFINE_TYPE (RemminaPrefDialog, remmina_pref_dialog, GTK_TYPE_DIALOG)

static const gpointer default_action_list[] =
{
    "0", N_("Open Connection"),
    "1", N_("Edit Settings"),
    NULL
};

static const gpointer scale_quality_list[] =
{
    "0", N_("Nearest"),
    "1", N_("Tiles"),
    "2", N_("Bilinear"),
    "3", N_("Hyper"),
    NULL
};

static const gpointer default_mode_list[] =
{
    "0", N_("Automatic"),
    "1", N_("Scrolled Window"),
    "3", N_("Scrolled Fullscreen"),
    "4", N_("Viewport Fullscreen"),
    NULL
};

static const gpointer tab_mode_list[] =
{
    "0", N_("Tab by Groups"),
    "1", N_("Tab by Protocols"),
    "8", N_("Tab All Connections"),
    "9", N_("Do Not Use Tabs"),
    NULL
};

struct _RemminaPrefDialogPriv
{
    GtkWidget *notebook;
    GtkWidget *save_view_mode_check;
    GtkWidget *save_when_connect_check;
    GtkWidget *invisible_toolbar_check;
    GtkWidget *always_show_tab_check;
    GtkWidget *hide_connection_toolbar_check;
    GtkWidget *default_action_combo;
    GtkWidget *default_mode_combo;
    GtkWidget *tab_mode_combo;
    GtkWidget *scale_quality_combo;
    GtkWidget *sshtunnel_port_entry;
    GtkWidget *recent_maximum_entry;
    GtkWidget *resolutions_list;
    GtkWidget *applet_quick_ontop_check;
    GtkWidget *applet_hide_count_check;
    GtkWidget *hostkey_chooser;
    GtkWidget *shortcutkey_fullscreen_chooser;
    GtkWidget *shortcutkey_autofit_chooser;
    GtkWidget *shortcutkey_nexttab_chooser;
    GtkWidget *shortcutkey_prevtab_chooser;
    GtkWidget *shortcutkey_scale_chooser;
    GtkWidget *shortcutkey_grab_chooser;
    GtkWidget *shortcutkey_minimize_chooser;
    GtkWidget *shortcutkey_disconnect_chooser;
    GtkWidget *shortcutkey_toolbar_chooser;
};

static void
remmina_pref_dialog_class_init (RemminaPrefDialogClass *klass)
{
}

static gboolean
remmina_pref_resolution_validation_func (const gchar *new_str, gchar **error)
{
    gint i;
    gint width, height;
    gboolean splitted;
    gboolean result;

    width = 0;
    height = 0;
    splitted = FALSE;
    result = TRUE;
    for (i = 0; new_str[i] != '\0'; i++)
    {
        if (new_str[i] == 'x')
        {
            if (splitted)
            {
                result = FALSE;
                break;
            }
            splitted = TRUE;
            continue;
        }
        if (new_str[i] < '0' || new_str[i] > '9')
        {
                result = FALSE;
                break;
        }
        if (splitted)
        {
            height = 1;
        }
        else
        {
            width = 1;
        }
    }

    if (width == 0 || height == 0) result = FALSE;

    if (!result) *error = g_strdup (_("Please enter format 'widthxheight'."));
    return result;
}

static void
remmina_pref_dialog_clear_recent (GtkWidget *widget, gpointer data)
{
    GtkWidget *dialog;

    remmina_pref_clear_recent ();
    dialog = gtk_message_dialog_new (GTK_WINDOW (data),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        _("Recent lists cleared."));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

static void
remmina_pref_dialog_on_close_clicked (GtkWidget *widget, RemminaPrefDialog *dialog)
{
    gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
remmina_pref_dialog_destroy (GtkWidget *widget, gpointer data)
{
    gchar *s;

    RemminaPrefDialogPriv *priv = REMMINA_PREF_DIALOG (widget)->priv;

    remmina_pref.save_view_mode = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->save_view_mode_check));
    remmina_pref.save_when_connect = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->save_when_connect_check));
    remmina_pref.invisible_toolbar = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->invisible_toolbar_check));
    remmina_pref.always_show_tab = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->always_show_tab_check));
    remmina_pref.hide_connection_toolbar = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->hide_connection_toolbar_check));

    s = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->default_action_combo));
    remmina_pref.default_action = atoi (s);
    g_free (s);
    s = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->default_mode_combo));
    remmina_pref.default_mode = atoi (s);
    g_free (s);
    s = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->tab_mode_combo));
    remmina_pref.tab_mode = atoi (s);
    g_free (s);
    s = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->scale_quality_combo));
    remmina_pref.scale_quality = atoi (s);
    g_free (s);

    remmina_pref.sshtunnel_port = atoi (gtk_entry_get_text (GTK_ENTRY (priv->sshtunnel_port_entry)));
    if (remmina_pref.sshtunnel_port <= 0) remmina_pref.sshtunnel_port = DEFAULT_SSHTUNNEL_PORT;

    remmina_pref.recent_maximum = atoi (gtk_entry_get_text (GTK_ENTRY (priv->recent_maximum_entry)));
    if (remmina_pref.recent_maximum < 0) remmina_pref.recent_maximum = 0;

    g_free (remmina_pref.resolutions);
    s = remmina_string_list_get_text (REMMINA_STRING_LIST (priv->resolutions_list));
    if (s[0] == '\0') s = g_strdup (default_resolutions);
    remmina_pref.resolutions = s;

    remmina_pref.applet_quick_ontop = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->applet_quick_ontop_check));
    remmina_pref.applet_hide_count = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->applet_hide_count_check));

    remmina_pref.hostkey = REMMINA_KEY_CHOOSER (priv->hostkey_chooser)->keyval;
    remmina_pref.shortcutkey_fullscreen = REMMINA_KEY_CHOOSER (priv->shortcutkey_fullscreen_chooser)->keyval;
    remmina_pref.shortcutkey_autofit = REMMINA_KEY_CHOOSER (priv->shortcutkey_autofit_chooser)->keyval;
    remmina_pref.shortcutkey_nexttab = REMMINA_KEY_CHOOSER (priv->shortcutkey_nexttab_chooser)->keyval;
    remmina_pref.shortcutkey_prevtab = REMMINA_KEY_CHOOSER (priv->shortcutkey_prevtab_chooser)->keyval;
    remmina_pref.shortcutkey_scale = REMMINA_KEY_CHOOSER (priv->shortcutkey_scale_chooser)->keyval;
    remmina_pref.shortcutkey_grab = REMMINA_KEY_CHOOSER (priv->shortcutkey_grab_chooser)->keyval;
    remmina_pref.shortcutkey_minimize = REMMINA_KEY_CHOOSER (priv->shortcutkey_minimize_chooser)->keyval;
    remmina_pref.shortcutkey_disconnect = REMMINA_KEY_CHOOSER (priv->shortcutkey_disconnect_chooser)->keyval;
    remmina_pref.shortcutkey_toolbar = REMMINA_KEY_CHOOSER (priv->shortcutkey_toolbar_chooser)->keyval;

    remmina_pref_save ();
    g_free (priv);
}

static void
remmina_pref_dialog_init (RemminaPrefDialog *dialog)
{
    RemminaPrefDialogPriv *priv;
    GtkWidget *notebook;
    GtkWidget *tablabel;
    GtkWidget *vbox;
    GtkWidget *hbox;
    GtkWidget *table;
    GtkWidget *widget;
    gchar buf[100];

    priv = g_new (RemminaPrefDialogPriv, 1);
    dialog->priv = priv;

    /* Create the dialog */
    gtk_window_set_title (GTK_WINDOW (dialog), _("Remmina Preferences"));
    gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
    widget = gtk_dialog_add_button (GTK_DIALOG (dialog), GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE);
    g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_pref_dialog_on_close_clicked), dialog);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);
    gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);

    g_signal_connect (G_OBJECT (dialog), "destroy", G_CALLBACK (remmina_pref_dialog_destroy), NULL);

    /* Create the notebook */
    notebook = gtk_notebook_new ();
    gtk_widget_show (notebook);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), notebook, TRUE, TRUE, 4);
    gtk_container_set_border_width (GTK_CONTAINER (notebook), 4);
    priv->notebook = notebook;

    /* Options tab */
    tablabel = gtk_label_new (_("Options"));
    gtk_widget_show (tablabel);

    /* Options body */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, tablabel);

    table = gtk_table_new (11, 2, FALSE);
    gtk_widget_show (table);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_container_set_border_width (GTK_CONTAINER (table), 8);
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

    widget = gtk_check_button_new_with_label (_("Remember Last View Mode for Each Connection"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 2, 0, 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), remmina_pref.save_view_mode);
    priv->save_view_mode_check = widget;

    widget = gtk_check_button_new_with_label (_("Save Settings When Starting the Connection"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 2, 1, 2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), remmina_pref.save_when_connect);
    priv->save_when_connect_check = widget;

    widget = gtk_check_button_new_with_label (_("Invisible Floating Toolbar"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 2, 2, 3);
    if (gtk_widget_is_composited (GTK_WIDGET (dialog)))
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), remmina_pref.invisible_toolbar);
    }
    else
    {
        gtk_widget_set_sensitive (widget, FALSE);
    }
    priv->invisible_toolbar_check = widget;

    widget = gtk_check_button_new_with_label (_("Always Show Tabs"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 2, 3, 4);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), remmina_pref.always_show_tab);
    priv->always_show_tab_check = widget;

    widget = gtk_check_button_new_with_label (_("Hide Connection Toolbar"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 2, 4, 5);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), remmina_pref.hide_connection_toolbar);
    priv->hide_connection_toolbar_check = widget;

    widget = gtk_label_new (_("Double-click Action"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 5, 6);

    widget = remmina_public_create_combo_mapint (default_action_list, remmina_pref.default_action, FALSE);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 5, 6);
    priv->default_action_combo = widget;

    widget = gtk_label_new (_("Default View Mode"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 6, 7);

    widget = remmina_public_create_combo_mapint (default_mode_list, remmina_pref.default_mode, FALSE);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 6, 7);
    priv->default_mode_combo = widget;

    widget = gtk_label_new (_("Tab Interface"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 7, 8);

    widget = remmina_public_create_combo_mapint (tab_mode_list, remmina_pref.tab_mode, FALSE);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 7, 8);
    priv->tab_mode_combo = widget;

    widget = gtk_label_new (_("Scale Quality"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 8, 9);

    widget = remmina_public_create_combo_mapint (scale_quality_list, remmina_pref.scale_quality, FALSE);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 8, 9);
    priv->scale_quality_combo = widget;

    widget = gtk_label_new (_("SSH Tunnel Local Port"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 9, 10);

    widget = gtk_entry_new_with_max_length (5);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 9, 10);
    g_snprintf (buf, sizeof (buf), "%i", remmina_pref.sshtunnel_port);
    gtk_entry_set_text (GTK_ENTRY (widget), buf);
    priv->sshtunnel_port_entry = widget;

    widget = gtk_label_new (_("Maximum Recent Items"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 1, 10, 11);

    hbox = gtk_hbox_new (FALSE, 2);
    gtk_widget_show (hbox);
    gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 10, 11);

    widget = gtk_entry_new_with_max_length (2);
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
    g_snprintf (buf, sizeof (buf), "%i", remmina_pref.recent_maximum);
    gtk_entry_set_text (GTK_ENTRY (widget), buf);
    priv->recent_maximum_entry = widget;

    widget = gtk_button_new_with_label (_("Clear"));
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_pref_dialog_clear_recent), dialog);

    /* Resolutions tab */
    tablabel = gtk_label_new (_("Resolutions"));
    gtk_widget_show (tablabel);

    /* Resolutions body */
    vbox = gtk_vbox_new (FALSE, 2);
    gtk_widget_show (vbox);
    gtk_container_set_border_width (GTK_CONTAINER (vbox), 8);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, tablabel);

    widget = remmina_string_list_new ();
    gtk_widget_show (widget);
    gtk_container_set_border_width (GTK_CONTAINER (widget), 8);
    gtk_widget_set_size_request (widget, 350, -1);
    remmina_string_list_set_validation_func (REMMINA_STRING_LIST (widget),
        remmina_pref_resolution_validation_func);
    gtk_box_pack_start (GTK_BOX (vbox), widget, TRUE, TRUE, 0);
    remmina_string_list_set_text (REMMINA_STRING_LIST (widget), remmina_pref.resolutions);
    priv->resolutions_list = widget;

    /* Applet tab */
    tablabel = gtk_label_new (_("Applet"));
    gtk_widget_show (tablabel);

    /* Applet body */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, tablabel);

    table = gtk_table_new (2, 2, FALSE);
    gtk_widget_show (table);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_container_set_border_width (GTK_CONTAINER (table), 8);
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

    widget = gtk_check_button_new_with_label (_("Show Quick Connect on top of the Menu"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 2, 0, 1);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), remmina_pref.applet_quick_ontop);
    priv->applet_quick_ontop_check = widget;

    widget = gtk_check_button_new_with_label (_("Hide Total Count in Group Expander"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 0, 2, 1, 2);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), remmina_pref.applet_hide_count);
    priv->applet_hide_count_check = widget;

    /* Keyboard tab */
    tablabel = gtk_label_new (_("Keyboard"));
    gtk_widget_show (tablabel);

    /* Keyboard body */
    vbox = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (vbox);
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, tablabel);

    table = gtk_table_new (9, 2, FALSE);
    gtk_widget_show (table);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 4);
    gtk_container_set_border_width (GTK_CONTAINER (table), 8);
    gtk_box_pack_start (GTK_BOX (vbox), table, FALSE, FALSE, 0);

    widget = gtk_label_new (_("Host Key"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 0, 1, GTK_FILL, 0, 0, 0);

    widget = remmina_key_chooser_new (remmina_pref.hostkey);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 0, 1);
    priv->hostkey_chooser = widget;

    widget = gtk_label_new (_("Toggle Fullscreen Mode"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 1, 2, GTK_FILL, 0, 0, 0);

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_fullscreen);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 1, 2);
    priv->shortcutkey_fullscreen_chooser = widget;

    widget = gtk_label_new (_("Auto-Fit Window"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 2, 3, GTK_FILL, 0, 0, 0);

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_autofit);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 2, 3);
    priv->shortcutkey_autofit_chooser = widget;

    widget = gtk_label_new (_("Switch Tab Pages"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 3, 4, GTK_FILL, 0, 0, 0);

    hbox = gtk_hbox_new (TRUE, 2);
    gtk_widget_show (hbox);
    gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, 3, 4);

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_prevtab);
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
    priv->shortcutkey_prevtab_chooser = widget;

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_nexttab);
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
    priv->shortcutkey_nexttab_chooser = widget;

    widget = gtk_label_new (_("Toggle Scaled Mode"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 4, 5, GTK_FILL, 0, 0, 0);

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_scale);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 4, 5);
    priv->shortcutkey_scale_chooser = widget;

    widget = gtk_label_new (_("Grab Keyboard"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 5, 6, GTK_FILL, 0, 0, 0);

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_grab);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 5, 6);
    priv->shortcutkey_grab_chooser = widget;

    widget = gtk_label_new (_("Minimize Window"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 6, 7, GTK_FILL, 0, 0, 0);

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_minimize);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 6, 7);
    priv->shortcutkey_minimize_chooser = widget;

    widget = gtk_label_new (_("Disconnect"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 7, 8, GTK_FILL, 0, 0, 0);

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_disconnect);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 7, 8);
    priv->shortcutkey_disconnect_chooser = widget;

    widget = gtk_label_new (_("Show / Hide Toolbar"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, 8, 9, GTK_FILL, 0, 0, 0);

    widget = remmina_key_chooser_new (remmina_pref.shortcutkey_toolbar);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, 8, 9);
    priv->shortcutkey_toolbar_chooser = widget;

    remmina_widget_pool_register (GTK_WIDGET (dialog));
}

GtkWidget*
remmina_pref_dialog_new (gint default_tab)
{
    RemminaPrefDialog *dialog;

    dialog = REMMINA_PREF_DIALOG (g_object_new (REMMINA_TYPE_PREF_DIALOG, NULL));
    if (default_tab > 0) gtk_notebook_set_current_page (GTK_NOTEBOOK (dialog->priv->notebook), default_tab);

    return GTK_WIDGET (dialog);
}

