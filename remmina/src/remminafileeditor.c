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
#include "config.h"
#ifdef HAVE_LIBAVAHI_UI
#include <avahi-ui/avahi-ui.h>
#endif
#include "remminapublic.h"
#include "remminapref.h"
#include "remminaconnectionwindow.h"
#include "remminaprefdialog.h"
#include "remminafile.h"
#include "remminafilemanager.h"
#include "remminassh.h"
#include "remminascaler.h"
#include "remminawidgetpool.h"
#include "remminapluginmanager.h"
#include "remminafileeditor.h"

G_DEFINE_TYPE (RemminaFileEditor, remmina_file_editor, GTK_TYPE_DIALOG)

/* Copied from tsclient... */
static const gchar *keymap_list = "\
ar,be,\
bg,br,cf,ch,common,\
cs,cz,da,de,de-ch,dk,dv,\
en-dv,en-gb,en-us,es,et,fi,\
fo,fr,fr-be,fr-ca,fr-ch,\
gk,gr,he,hr,hu,is,it,\
ja,ko,la,lt,lv,mk,\
nl,no,pl,po,pt,pt-br,\
ro,ru,sf,sl,sp,su,\
sv,th,tr,uk,us,usl,\
usr,yu";

static const gchar *charset_list = "\
ASCII,BIG5,\
CP437,CP720,CP737,CP775,CP850,CP852,CP855,\
CP857,CP858,CP862,CP866,CP874,CP1125,CP1250,\
CP1251,CP1252,CP1253,CP1254,CP1255,CP1256,\
CP1257,CP1258,\
EUC-JP,EUC-KR,GBK,\
ISO-8859-1,ISO-8859-2,ISO-8859-3,ISO-8859-4,\
ISO-8859-5,ISO-8859-6,ISO-8859-7,ISO-8859-8,\
ISO-8859-9,ISO-8859-10,ISO-8859-11,ISO-8859-12,\
ISO-8859-13,ISO-8859-14,ISO-8859-15,ISO-8859-16,\
KOI8-R,SJIS,UTF-8";

static const gpointer sound_list[] =
{
    "off", N_("Off"),
    "local", N_("Local"),
    "remote", N_("Remote"),
    NULL
};

static const gchar *server_tips = N_(
    "<tt><big>"
    "Supported formats\n"
    "* server\n"
    "* server:port\n"
    "* [server]:port"
    "</big></tt>");
static const gchar *server_tips2 = N_(
    "<tt><big>"
    "Supported formats\n"
    "* :port\n"
    "* server\n"
    "* server:port\n"
    "* [server]:port"
    "</big></tt>");

struct _RemminaFileEditorPriv
{
    RemminaFile *remmina_file;
    RemminaProtocolPlugin *plugin;

    GtkWidget *name_entry;
    GtkWidget *group_combo;
    GtkWidget *protocol_combo;

    GtkWidget *config_box;
    GtkWidget *config_container;

    GtkWidget *server_combo;
    GtkWidget *listenport_entry;
    GtkWidget *username_entry;
    GtkWidget *password_entry;
    GtkWidget *domain_entry;
    GtkWidget *clientname_entry;
    GtkWidget *resolution_auto_radio;
    GtkWidget *resolution_custom_radio;
    GtkWidget *resolution_custom_combo;
    GtkWidget *colordepth_combo;
    GtkWidget *quality_combo;
    GtkWidget *keymap_combo;
    GtkWidget *gkeymap_combo;
    GtkWidget *exec_widget;
    GtkWidget *execpath_entry;
    GtkWidget *sound_combo;
    GtkWidget *arguments_entry;
    GtkWidget *proxy_entry;
    GtkWidget *sharefolder_check;
    GtkWidget *sharefolder_chooser;
    GtkWidget *scaler_widget;

    GtkWidget *bitmapcaching_check;
    GtkWidget *compression_check;
    GtkWidget *showcursor_check;
    GtkWidget *viewonly_check;
    GtkWidget *console_check;
    GtkWidget *disableserverinput_check;
    GtkWidget *shareprinter_check;
    GtkWidget *once_check;
    GtkWidget *disableencryption_check;
    GtkWidget *disableclipboard_check;

    GtkWidget *ssh_enabled_check;
    GtkWidget *ssh_loopback_check;
    GtkWidget *ssh_server_default_radio;
    GtkWidget *ssh_server_custom_radio;
    GtkWidget *ssh_server_entry;
    GtkWidget *ssh_auth_password_radio;
    GtkWidget *ssh_auth_publickey_radio;
    GtkWidget *ssh_auth_auto_publickey_radio;
    GtkWidget *ssh_username_entry;
    GtkWidget *ssh_privatekey_chooser;
    GtkWidget *ssh_charset_combo;
};

static void
remmina_file_editor_class_init (RemminaFileEditorClass *klass)
{
}

#ifdef HAVE_LIBAVAHI_UI
static void
remmina_file_editor_browse_avahi (GtkWidget *button, RemminaFileEditor *gfe)
{
    GtkWidget *dialog;
    gchar *host;

    dialog = aui_service_dialog_new (_("Choose a Remote Desktop Server"),
        GTK_WINDOW (gfe),
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (gfe));
    aui_service_dialog_set_resolve_service (AUI_SERVICE_DIALOG (dialog), TRUE);
    aui_service_dialog_set_resolve_host_name (AUI_SERVICE_DIALOG (dialog), TRUE);
    aui_service_dialog_set_browse_service_types (AUI_SERVICE_DIALOG (dialog),
        gfe->priv->plugin->avahi_service_type, NULL);
    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT)
    {
        host = g_strdup_printf ("[%s]:%i",
            aui_service_dialog_get_host_name (AUI_SERVICE_DIALOG (dialog)),
            aui_service_dialog_get_port (AUI_SERVICE_DIALOG (dialog)));
    }
    else
    {
        host = NULL;
    }
    gtk_widget_destroy (dialog);

    if (host)
    {
        gtk_entry_set_text (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (gfe->priv->server_combo))), host);
        g_free (host);
    }
}
#endif

static void
remmina_file_editor_on_realize (GtkWidget *widget, gpointer user_data)
{
    RemminaFileEditor *gfe;
    GtkWidget *defaultwidget;

    gfe = REMMINA_FILE_EDITOR (widget);

    defaultwidget = (gfe->priv->remmina_file->filename == NULL ?
        (gfe->priv->server_combo ? gfe->priv->server_combo : gfe->priv->listenport_entry) : 
        gfe->priv->name_entry);

    if (defaultwidget)
    {
        if (GTK_IS_EDITABLE (defaultwidget))
        {
            gtk_editable_select_region (GTK_EDITABLE (defaultwidget), 0, -1);
        }
        gtk_widget_grab_focus (defaultwidget);
    }
}

static void
remmina_file_editor_destroy (GtkWidget *widget, gpointer data)
{
    remmina_file_free (REMMINA_FILE_EDITOR (widget)->priv->remmina_file);
    g_free (REMMINA_FILE_EDITOR (widget)->priv);
}

static void
remmina_file_editor_button_on_toggled (GtkToggleButton *togglebutton, GtkWidget *widget)
{
    gtk_widget_set_sensitive (widget, 
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)));
}

static void
remmina_file_editor_create_notebook_container (RemminaFileEditor *gfe)
{
    /* Create the notebook */
    gfe->priv->config_container = gtk_notebook_new ();
    gtk_container_add (GTK_CONTAINER (gfe->priv->config_box), gfe->priv->config_container);
    gtk_container_set_border_width (GTK_CONTAINER (gfe->priv->config_container), 4);
    gtk_widget_show (gfe->priv->config_container);
}

static GtkWidget*
remmina_file_editor_create_notebook_tab (RemminaFileEditor *gfe,
    const gchar *stock_id, const gchar *label, gint rows, gint cols)
{
    GtkWidget *tablabel, *tabbody;
    GtkWidget *table;
    GtkWidget *widget;

    tablabel = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (tablabel);

    widget = gtk_image_new_from_stock (stock_id, GTK_ICON_SIZE_MENU);
    gtk_box_pack_start (GTK_BOX (tablabel), widget, FALSE, FALSE, 0);
    gtk_widget_show (widget);

    widget = gtk_label_new (label);
    gtk_box_pack_start (GTK_BOX (tablabel), widget, FALSE, FALSE, 0);
    gtk_widget_show (widget);

    tabbody = gtk_vbox_new (FALSE, 0);
    gtk_widget_show (tabbody);
    gtk_notebook_append_page (GTK_NOTEBOOK (gfe->priv->config_container), tabbody, tablabel);

    table = gtk_table_new (rows, cols, FALSE);
    gtk_widget_show (table);
    gtk_table_set_row_spacings (GTK_TABLE (table), 8);
    gtk_table_set_col_spacings (GTK_TABLE (table), 8);
    gtk_container_set_border_width (GTK_CONTAINER (table), 15);
    gtk_box_pack_start (GTK_BOX (tabbody), table, FALSE, FALSE, 0);

    return table;
}

#ifdef HAVE_LIBSSH
static void
remmina_file_editor_ssh_server_custom_radio_on_toggled (GtkToggleButton *togglebutton, RemminaFileEditor *gfe)
{
    gtk_widget_set_sensitive (gfe->priv->ssh_server_entry,
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gfe->priv->ssh_enabled_check)) &&
            (gfe->priv->ssh_server_custom_radio == NULL ||
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gfe->priv->ssh_server_custom_radio)))
    );
}

static void
remmina_file_editor_ssh_auth_publickey_radio_on_toggled (GtkToggleButton *togglebutton, RemminaFileEditor *gfe)
{
    gboolean b;
    gchar *s;

    b = ((!gfe->priv->ssh_enabled_check ||
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gfe->priv->ssh_enabled_check))) &&
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gfe->priv->ssh_auth_publickey_radio)));
    gtk_widget_set_sensitive (gfe->priv->ssh_privatekey_chooser, b);

    if (b && (gfe->priv->remmina_file->ssh_privatekey == NULL || gfe->priv->remmina_file->ssh_privatekey[0] == '\0'))
    {
        s = remmina_ssh_find_identity ();
        if (s)
        {
            gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (gfe->priv->ssh_privatekey_chooser), s);
            g_free (s);
        }
    }
}

static void
remmina_file_editor_ssh_enabled_check_on_toggled (GtkToggleButton *togglebutton, RemminaFileEditor *gfe)
{
    gboolean enabled = TRUE;

    if (gfe->priv->ssh_enabled_check)
    {
        enabled = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (gfe->priv->ssh_enabled_check));
        if (gfe->priv->ssh_loopback_check) gtk_widget_set_sensitive (gfe->priv->ssh_loopback_check, enabled);
        if (gfe->priv->ssh_server_default_radio) gtk_widget_set_sensitive (gfe->priv->ssh_server_default_radio, enabled);
        if (gfe->priv->ssh_server_custom_radio) gtk_widget_set_sensitive (gfe->priv->ssh_server_custom_radio, enabled);
        remmina_file_editor_ssh_server_custom_radio_on_toggled (NULL, gfe);
        gtk_widget_set_sensitive (gfe->priv->ssh_charset_combo, enabled);
        gtk_widget_set_sensitive (gfe->priv->ssh_username_entry, enabled);
        gtk_widget_set_sensitive (gfe->priv->ssh_auth_password_radio, enabled);
        gtk_widget_set_sensitive (gfe->priv->ssh_auth_publickey_radio, enabled);
        gtk_widget_set_sensitive (gfe->priv->ssh_auth_auto_publickey_radio, enabled);
    }
    remmina_file_editor_ssh_auth_publickey_radio_on_toggled (NULL, gfe);

    if (enabled && gtk_entry_get_text (GTK_ENTRY (gfe->priv->ssh_username_entry)) [0] == '\0')
    {
        gtk_entry_set_text (GTK_ENTRY (gfe->priv->ssh_username_entry), g_get_user_name ());
    }
}

static gboolean
remmina_file_editor_unselect_ssh_privatekey (gpointer data)
{
    gtk_file_chooser_unselect_all (GTK_FILE_CHOOSER (data));
    return FALSE;
}

static void
remmina_file_editor_ssh_privatekey_on_response (GtkDialog *dialog, gint response_id, RemminaFileEditor *gfe)
{
    if (response_id == GTK_RESPONSE_REJECT)
    {
        g_idle_add (remmina_file_editor_unselect_ssh_privatekey, gfe->priv->ssh_privatekey_chooser);
    }
}

static void
remmina_file_editor_create_ssh_privatekey (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint column, gboolean use_label)
{
    RemminaFileEditorPriv *priv = gfe->priv;
    GtkWidget *widget;
    GtkWidget *dialog;
    gchar *s;

    if (use_label)
    {
        widget = gtk_label_new (_("Identity File"));
        gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    }
    else
    {
        widget = gtk_radio_button_new_with_label_from_widget (
            GTK_RADIO_BUTTON (priv->ssh_auth_password_radio), _("Identity File"));
        g_signal_connect (G_OBJECT (widget), "toggled",
            G_CALLBACK (remmina_file_editor_ssh_auth_publickey_radio_on_toggled), gfe);
        priv->ssh_auth_publickey_radio = widget;
    }
    gtk_widget_show (widget);
    gtk_table_attach (GTK_TABLE (table), widget, column, column + 1, row, row + 1, GTK_FILL, 0, 0, 0);

    dialog = gtk_file_chooser_dialog_new (_("Identity File"), GTK_WINDOW (gfe), GTK_FILE_CHOOSER_ACTION_OPEN,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_CLEAR, GTK_RESPONSE_REJECT,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);
    g_signal_connect (G_OBJECT (dialog), "response", G_CALLBACK (remmina_file_editor_ssh_privatekey_on_response), gfe);

    widget = gtk_file_chooser_button_new_with_dialog (dialog);
    s = g_strdup_printf ("%s/.ssh", g_get_home_dir ());
    if (g_file_test (s, G_FILE_TEST_IS_DIR))
    {
        gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (widget), s);
    }
    g_free (s);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, column + 1, column + 2, row, row + 1);
    priv->ssh_privatekey_chooser = widget;

    if (priv->remmina_file->ssh_privatekey && priv->remmina_file->ssh_privatekey[0] &&
        g_file_test (priv->remmina_file->ssh_privatekey, G_FILE_TEST_IS_REGULAR | G_FILE_TEST_EXISTS))
    {
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (priv->ssh_privatekey_chooser),
            priv->remmina_file->ssh_privatekey);
    }
    else
    {
        g_free (priv->remmina_file->ssh_privatekey);
        priv->remmina_file->ssh_privatekey = NULL;
    }
}
#endif

static void
remmina_file_editor_create_server (RemminaFileEditor *gfe, GtkWidget *table, gint row)
{
    RemminaProtocolPlugin *plugin = gfe->priv->plugin;
    GtkWidget *widget;
    GtkWidget *hbox;
    gchar *s;

    widget = gtk_label_new (_("Server"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, row, row + 1, GTK_FILL, 0, 0, 0);

    s = remmina_pref_get_recent (plugin->name);
    widget = remmina_public_create_combo_entry (s, gfe->priv->remmina_file->server, TRUE);
    gtk_widget_show (widget);
    gtk_widget_set_tooltip_markup (widget, _(server_tips));
    gtk_entry_set_activates_default (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (widget))), TRUE);
    gfe->priv->server_combo = widget;
    g_free (s);

#ifdef HAVE_LIBAVAHI_UI
    if (plugin->avahi_service_type)
    {
        hbox = gtk_hbox_new (FALSE, 0);
        gtk_widget_show (hbox);
        gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

        widget = gtk_button_new_with_label ("...");
        s = g_strdup_printf (_("Browse the network to find a %s server"), plugin->name);
        gtk_widget_set_tooltip_text (widget, s);
        g_free (s);
        gtk_widget_show (widget);
        gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
        g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_file_editor_browse_avahi), gfe);

        gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, row, row + 1);
    }
    else
#endif
    {
        gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, row, row + 1);
    }
}

static void
remmina_file_editor_create_password (RemminaFileEditor *gfe, GtkWidget *table, gint row)
{
    GtkWidget *widget;

    widget = gtk_label_new (_("Password"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, row, row + 1, GTK_FILL, 0, 0, 0);

    widget = gtk_entry_new_with_max_length (100);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, row, row + 1);
    gtk_entry_set_visibility (GTK_ENTRY (widget), FALSE);
    if (gfe->priv->remmina_file->password) gtk_entry_set_text (GTK_ENTRY (widget), gfe->priv->remmina_file->password);
    gfe->priv->password_entry = widget;
}

static void
remmina_file_editor_create_resolution (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint type)
{
    GtkWidget *widget;
    GtkWidget *hbox;

    widget = gtk_label_new (_("Resolution"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 0, 1, row, row + 1, GTK_FILL, 0, 0, 0);

    widget = gtk_radio_button_new_with_label (NULL,
        type == REMMINA_PROTOCOL_SETTING_VALUE_RESOLUTION_FLEXIBLE ? _("Use Window Size") : _("Use Client Resolution"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, row, row + 1);
    gfe->priv->resolution_auto_radio = widget;

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_table_attach_defaults (GTK_TABLE (table), hbox, 1, 2, row + 1, row + 2);

    widget = gtk_radio_button_new_with_label_from_widget (
        GTK_RADIO_BUTTON (gfe->priv->resolution_auto_radio), _("Custom"));
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
    gfe->priv->resolution_custom_radio = widget;

    widget = remmina_public_create_combo_text_d (remmina_pref.resolutions, gfe->priv->remmina_file->resolution, NULL);
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
    gfe->priv->resolution_custom_combo = widget;

    g_signal_connect (G_OBJECT (gfe->priv->resolution_custom_radio), "toggled", 
        G_CALLBACK (remmina_file_editor_button_on_toggled), gfe->priv->resolution_custom_combo);

    if (!gfe->priv->remmina_file->resolution || gfe->priv->remmina_file->resolution[0] == '\0' ||
        g_strcmp0 (gfe->priv->remmina_file->resolution, "AUTO") == 0)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gfe->priv->resolution_auto_radio), TRUE);
        gtk_widget_set_sensitive (gfe->priv->resolution_custom_combo, FALSE);
    }
    else
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (gfe->priv->resolution_custom_radio), TRUE);
    }
}

static GtkWidget*
remmina_file_editor_create_combo_map (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint col,
    const gchar *label, const gpointer *list, const gchar *value)
{
    GtkWidget *widget;

    widget = gtk_label_new (label);
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);

    widget = remmina_public_create_combo_map (list, value, FALSE);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, col + 1, col + 2, row, row + 1);

    return widget;
}

static GtkWidget*
remmina_file_editor_create_folder_chooser (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint col,
    const gchar *label, const gchar *value, GtkWidget *check)
{
    GtkWidget *widget;
    GtkWidget *hbox;

    widget = gtk_label_new (label);
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);

    hbox = gtk_hbox_new (FALSE, 0);
    gtk_widget_show (hbox);
    gtk_table_attach_defaults (GTK_TABLE (table), hbox, col + 1, col + 2, row, row + 1);

    gtk_widget_show (check);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check), (value && value[0] == '/'));
    gtk_box_pack_start (GTK_BOX (hbox), check, FALSE, FALSE, 0);

    widget = gtk_file_chooser_button_new (label, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_widget_show (widget);
    if (value)
    {
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (widget), value);
    }
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

    g_signal_connect (G_OBJECT (check), "toggled", 
        G_CALLBACK (remmina_file_editor_button_on_toggled), widget);
    remmina_file_editor_button_on_toggled (GTK_TOGGLE_BUTTON (check), widget);

    return widget;
}

static GtkWidget*
remmina_file_editor_create_combo_mapint (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint col,
    const gchar *label, const gpointer *list, gint value)
{
    GtkWidget *widget;

    widget = gtk_label_new (label);
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);

    widget = remmina_public_create_combo_mapint (list, value, FALSE);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, col + 1, col + 2, row, row + 1);

    return widget;
}

static GtkWidget*
remmina_file_editor_create_combo_entry (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint col,
    const gchar *label, const gchar *list, const gchar *value)
{
    GtkWidget *widget;

    widget = gtk_label_new (label);
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);

    widget = remmina_public_create_combo_entry (list, value, FALSE);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, col + 1, col + 2, row, row + 1);

    return widget;
}

static GtkWidget*
remmina_file_editor_create_combo_text (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint col,
    const gchar *label, const gchar *list, const gchar *value, const gchar *empty_label)
{
    GtkWidget *widget;

    widget = gtk_label_new (label);
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);

    widget = remmina_public_create_combo_text_d (list, value, empty_label);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, col + 1, col + 2, row, row + 1);

    return widget;
}

static GtkWidget*
remmina_file_editor_create_entry (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint col,
    const gchar *label, const gchar *value)
{
    GtkWidget *widget;

    widget = gtk_label_new (label);
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, col, col + 1, row, row + 1, GTK_FILL, 0, 0, 0);

    widget = gtk_entry_new_with_max_length (300);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, col + 1, col + 2, row, row + 1);
    if (value) gtk_entry_set_text (GTK_ENTRY (widget), value);

    return widget;
}

static GtkWidget*
remmina_file_editor_create_check (RemminaFileEditor *gfe, GtkWidget *table, gint row, gint col,
    const gchar *label, gboolean value)
{
    GtkWidget *widget;

    widget = gtk_check_button_new_with_label (label);
    gtk_widget_show (widget);
    if (row >= 0)
    {
        gtk_table_attach_defaults (GTK_TABLE (table), widget, col, col + 2, row, row + 1);
    }
    else
    {
        gtk_box_pack_start (GTK_BOX (table), widget, TRUE, TRUE, 0);
    }
    if (value) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
    return widget;
}

static void
remmina_file_editor_create_settings (RemminaFileEditor *gfe, GtkWidget *table, const RemminaProtocolSetting *settings)
{
    RemminaFileEditorPriv *priv = gfe->priv;
    GtkWidget *hbox = NULL;
    GtkWidget *widget;
    gint row = 0;
    gint val;
    gchar *s;

    while ((val = atoi (*settings)) != REMMINA_PROTOCOL_SETTING_VALUE_CTL_END)
    {
        switch (val)
        {
        case REMMINA_PROTOCOL_SETTING_VALUE_CTL_CONCAT:
            hbox = gtk_hbox_new (TRUE, 0);
            gtk_widget_show (hbox);
            gtk_table_attach_defaults (GTK_TABLE (table), hbox, 0, 2, row, row + 1);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_CTL_CONCAT_END:
            hbox = NULL;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_SERVER:
            remmina_file_editor_create_server (gfe, table, row);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_PASSWORD:
            remmina_file_editor_create_password (gfe, table, row);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_RESOLUTION_FIXED:
        case REMMINA_PROTOCOL_SETTING_VALUE_RESOLUTION_FLEXIBLE:
            remmina_file_editor_create_resolution (gfe, table, row, val);
            row += 2;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_USERNAME:
            priv->username_entry = remmina_file_editor_create_entry (gfe, table, row, 0,
                _("User Name"), priv->remmina_file->username);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_DOMAIN:
            priv->domain_entry = remmina_file_editor_create_entry (gfe, table, row, 0,
                _("Domain"), priv->remmina_file->domain);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_COLORDEPTH:
            priv->colordepth_combo = remmina_file_editor_create_combo_mapint (gfe, table, row, 0,
                _("Color Depth"), colordepth_list, priv->remmina_file->colordepth);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_COLORDEPTH2:
            priv->colordepth_combo = remmina_file_editor_create_combo_mapint (gfe, table, row, 0,
                _("Color Depth"), colordepth2_list, priv->remmina_file->colordepth);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_SHAREFOLDER:
            priv->sharefolder_check = gtk_check_button_new ();
            priv->sharefolder_chooser = remmina_file_editor_create_folder_chooser (gfe, table, row, 0,
                _("Share Folder"), priv->remmina_file->sharefolder, priv->sharefolder_check);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_SOUND:
            priv->sound_combo = remmina_file_editor_create_combo_map (gfe, table, row, 0,
                _("Sound"), sound_list, priv->remmina_file->sound);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_KEYMAP:
            priv->keymap_combo = remmina_file_editor_create_combo_entry (gfe, table, row, 0,
                _("Keyboard Mapping"), keymap_list, priv->remmina_file->keymap);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_CLIENTNAME:
            priv->clientname_entry = remmina_file_editor_create_entry (gfe, table, row, 0,
                _("Client Name"), priv->remmina_file->clientname);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_EXEC:
            priv->exec_widget = remmina_file_editor_create_entry (gfe, table, row, 0,
                _("Startup Program"), priv->remmina_file->exec);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_EXEC_CUSTOM:
            settings++;
            priv->exec_widget = remmina_file_editor_create_combo_entry (gfe, table, row, 0,
                _("Startup Program"), (const gchar*) *settings, priv->remmina_file->exec);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_EXECPATH:
            priv->execpath_entry = remmina_file_editor_create_entry (gfe, table, row, 0,
                _("Startup Path"), priv->remmina_file->execpath);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_SHAREPRINTER:
            priv->shareprinter_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Share Local Printers"), priv->remmina_file->shareprinter);
            if (!hbox) row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_CONSOLE:
            priv->console_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Attach to console (Windows 2003 or newer)"), priv->remmina_file->console);
            if (!hbox) row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_BITMAPCACHING:
            priv->bitmapcaching_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Enable bitmap caching"), priv->remmina_file->bitmapcaching);
            if (!hbox) row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_COMPRESSION:
            priv->compression_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Enable compression"), priv->remmina_file->compression);
            if (!hbox) row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_ARGUMENTS:
            priv->arguments_entry = remmina_file_editor_create_entry (gfe, table, row, 0,
                _("Advanced Arguments"), priv->remmina_file->arguments);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_LISTENPORT:
            s = g_strdup_printf ("%i", priv->remmina_file->listenport);
            priv->listenport_entry = remmina_file_editor_create_entry (gfe, table, row, 0,
                _("Listen on Port"), s);
            gtk_entry_set_activates_default (GTK_ENTRY (priv->listenport_entry), TRUE);
            g_free (s);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_QUALITY:
            priv->quality_combo = remmina_file_editor_create_combo_mapint (gfe, table, row, 0,
                _("Quality"), quality_list, priv->remmina_file->quality);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_SHOWCURSOR_REMOTE:
            priv->showcursor_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Show Remote Cursor"), priv->remmina_file->showcursor);
            if (!hbox) row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_SHOWCURSOR_LOCAL:
            priv->showcursor_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Use Local Cursor"), priv->remmina_file->showcursor);
            if (!hbox) row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_VIEWONLY:
            priv->viewonly_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("View Only"), priv->remmina_file->viewonly);
            if (!hbox) row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_DISABLESERVERINPUT:
            priv->disableserverinput_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Disable Server Input"), priv->remmina_file->disableserverinput);
            if (!hbox) row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_SCALE:
            widget = gtk_label_new (_("Horizontal Scale"));
            gtk_widget_show (widget);
            gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
            gtk_table_attach (GTK_TABLE (table), widget, 0, 1, row, row + 1, GTK_FILL, 0, 0, 0);

            widget = gtk_label_new (_("Vertical Scale"));
            gtk_widget_show (widget);
            gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
            gtk_table_attach (GTK_TABLE (table), widget, 0, 1, row + 1, row + 2, GTK_FILL, 0, 0, 0);

            widget = remmina_scaler_new ();
            gtk_widget_show (widget);
            gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 2, row, row + 2);
            remmina_scaler_set (REMMINA_SCALER (widget), priv->remmina_file->hscale, priv->remmina_file->vscale,
                priv->remmina_file->aspectscale);
            priv->scaler_widget = widget;

            row += 2;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_GKEYMAP:
            s = remmina_pref_keymap_groups ();
            priv->gkeymap_combo = remmina_file_editor_create_combo_text (gfe, table, row, 0,
                _("Keyboard Mapping"), s, priv->remmina_file->gkeymap, _("<None>"));
            g_free (s);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_ONCE:
            priv->once_check = remmina_file_editor_create_check (gfe, table, row, 0,
                _("Disconnect After One Session"), priv->remmina_file->once);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_DISABLEENCRYPTION:
            priv->disableencryption_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Disable Encryption"), priv->remmina_file->disableencryption);
            if (!hbox) row++;
            break;

#ifdef HAVE_LIBSSH
        case REMMINA_PROTOCOL_SETTING_VALUE_SSH_PRIVATEKEY:
            remmina_file_editor_create_ssh_privatekey (gfe, table, row, 0, TRUE);
            row++;
            break;
#endif

        case REMMINA_PROTOCOL_SETTING_VALUE_PROXY_DEST:
            priv->proxy_entry = remmina_file_editor_create_entry (gfe, table, row, 0,
                _("Proxy Destination"), priv->remmina_file->proxy);
            row++;
            break;

        case REMMINA_PROTOCOL_SETTING_VALUE_DISABLECLIPBOARD:
            priv->disableclipboard_check = remmina_file_editor_create_check (gfe, (hbox ? hbox : table), (hbox ? -1 : row), 0,
                _("Disable Clipboard Sync"), priv->remmina_file->disableclipboard);
            if (!hbox) row++;
            break;

        default:
            break;
        }
        settings++;
    }

    gtk_table_resize (GTK_TABLE (table), row, 2);
}

static void
remmina_file_editor_create_ssh_tab (RemminaFileEditor *gfe, RemminaProtocolSSHSetting ssh_setting)
{
#ifdef HAVE_LIBSSH
    RemminaFileEditorPriv *priv = gfe->priv;
    GtkWidget *table;
    GtkWidget *hbox;
    GtkWidget *widget;
    gchar *s;
    gint row = 0;

    if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_NONE) return;

    /* The SSH tab (implementation) */
    if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_SSH ||
        ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_SFTP)
    {
        s = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->protocol_combo));
        table = remmina_file_editor_create_notebook_tab (gfe, GTK_STOCK_DIALOG_AUTHENTICATION,
            (s ? s : "SSH"), 8, 3);
        g_free (s);
    }
    else
    {
        table = remmina_file_editor_create_notebook_tab (gfe, GTK_STOCK_DIALOG_AUTHENTICATION,
            "SSH", 9, 3);

        hbox = gtk_hbox_new (TRUE, 0);
        gtk_widget_show (hbox);
        gtk_table_attach_defaults (GTK_TABLE (table), hbox, 0, 3, 0, 1);
        row++;

        widget = gtk_check_button_new_with_label (_("Enable SSH Tunnel"));
        gtk_widget_show (widget);
        gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
        g_signal_connect (G_OBJECT (widget), "toggled",
            G_CALLBACK (remmina_file_editor_ssh_enabled_check_on_toggled), gfe);
        priv->ssh_enabled_check = widget;

        widget = gtk_check_button_new_with_label (_("Tunnel via Loopback Address"));
        gtk_widget_show (widget);
        gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);
        priv->ssh_loopback_check = widget;
    }

    /* SSH Server group */
    remmina_public_create_group (GTK_TABLE (table), _("SSH Server"), row, 3, 3);
    row++;

    switch (ssh_setting)
    {
    case REMMINA_PROTOCOL_SSH_SETTING_TUNNEL:
        s = g_strdup_printf (_("Same Server at Port %i"), DEFAULT_SSH_PORT);
        widget = gtk_radio_button_new_with_label (NULL, s);
        g_free (s);
        gtk_widget_show (widget);
        gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 3, row, row + 1);
        priv->ssh_server_default_radio = widget;
        row++;

        widget = gtk_radio_button_new_with_label_from_widget (
            GTK_RADIO_BUTTON (priv->ssh_server_default_radio), _("Custom"));
        gtk_widget_show (widget);
        gtk_table_attach (GTK_TABLE (table), widget, 1, 2, row, row + 1, GTK_FILL, 0, 0, 0);
        g_signal_connect (G_OBJECT (widget), "toggled",
            G_CALLBACK (remmina_file_editor_ssh_server_custom_radio_on_toggled), gfe);
        priv->ssh_server_custom_radio = widget;

        widget = gtk_entry_new_with_max_length (100);
        gtk_widget_show (widget);
        gtk_widget_set_tooltip_markup (widget, _(server_tips2));
        gtk_table_attach_defaults (GTK_TABLE (table), widget, 2, 3, row, row + 1);
        priv->ssh_server_entry = widget;
        row++;
        break;

    case REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL:
        priv->ssh_server_default_radio = NULL;
        priv->ssh_server_custom_radio = NULL;

        priv->ssh_server_entry = remmina_file_editor_create_entry (gfe, table, row, 1,
            _("Server"), NULL);
        gtk_widget_set_tooltip_markup (priv->ssh_server_entry, _(server_tips));
        row++;
        break;

    case REMMINA_PROTOCOL_SSH_SETTING_SSH:
    case REMMINA_PROTOCOL_SSH_SETTING_SFTP:
        priv->ssh_server_default_radio = NULL;
        priv->ssh_server_custom_radio = NULL;
        priv->ssh_server_entry = NULL;

        s = remmina_pref_get_recent ("SFTP");
        priv->server_combo = remmina_file_editor_create_combo_entry (gfe, table, row, 1,
            _("Server"), s, priv->remmina_file->server);
        gtk_widget_set_tooltip_markup (priv->server_combo, _(server_tips));
        gtk_entry_set_activates_default (GTK_ENTRY (gtk_bin_get_child (GTK_BIN (priv->server_combo))), TRUE);
        g_free (s);
        row++;
        break;

    default:
        break;
    }

    priv->ssh_charset_combo = remmina_file_editor_create_combo_entry (gfe, table, row, 1,
        _("Character Set"), charset_list, priv->remmina_file->ssh_charset);
    row++;

    if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_SSH)
    {
        priv->exec_widget = remmina_file_editor_create_entry (gfe, table, row, 1,
            _("Startup Program"), NULL);
        gtk_entry_set_text (GTK_ENTRY (priv->exec_widget),
            priv->remmina_file->exec ? priv->remmina_file->exec : "");
        row++;
    }

    /* SSH Authentication frame */
    remmina_public_create_group (GTK_TABLE (table), _("SSH Authentication"), row, 5, 3);
    row++;

    priv->ssh_username_entry = remmina_file_editor_create_entry (gfe, table, row, 1,
        _("User Name"), NULL);
    row++;

    widget = gtk_radio_button_new_with_label (NULL, _("Password"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 3, row, row + 1);
    priv->ssh_auth_password_radio = widget;
    row++;

    widget = gtk_radio_button_new_with_label_from_widget (
        GTK_RADIO_BUTTON (priv->ssh_auth_password_radio), _("Public Key (Automatic)"));
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 1, 3, row, row + 1);
    priv->ssh_auth_auto_publickey_radio = widget;
    row++;

    remmina_file_editor_create_ssh_privatekey (gfe, table, row, 1, FALSE);
    row++;

    /* Set the values */
    if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_TUNNEL)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->ssh_enabled_check), priv->remmina_file->ssh_enabled);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->ssh_loopback_check), priv->remmina_file->ssh_loopback);

        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (
            priv->remmina_file->ssh_server && priv->remmina_file->ssh_server[0] != '\0' ?
            priv->ssh_server_custom_radio : priv->ssh_server_default_radio), TRUE);
        gtk_entry_set_text (GTK_ENTRY (priv->ssh_server_entry),
            priv->remmina_file->ssh_server ? priv->remmina_file->ssh_server : "");
    }
    else if (ssh_setting == REMMINA_PROTOCOL_SSH_SETTING_REVERSE_TUNNEL)
    {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->ssh_enabled_check), priv->remmina_file->ssh_enabled);
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->ssh_loopback_check), priv->remmina_file->ssh_loopback);
        gtk_entry_set_text (GTK_ENTRY (priv->ssh_server_entry),
            priv->remmina_file->ssh_server ? priv->remmina_file->ssh_server : "");
    }

    gtk_entry_set_text (GTK_ENTRY (priv->ssh_username_entry),
        priv->remmina_file->ssh_username ? priv->remmina_file->ssh_username : "");
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (
        priv->remmina_file->ssh_auth == SSH_AUTH_PUBLICKEY ?
        priv->ssh_auth_publickey_radio :
        priv->remmina_file->ssh_auth == SSH_AUTH_AUTO_PUBLICKEY ?
        priv->ssh_auth_auto_publickey_radio :
        priv->ssh_auth_password_radio), TRUE);

    remmina_file_editor_ssh_enabled_check_on_toggled (NULL, gfe);
#endif
}

static void
remmina_file_editor_create_all_settings (RemminaFileEditor *gfe)
{
    RemminaFileEditorPriv *priv = gfe->priv;
    GtkWidget *table;

    remmina_file_editor_create_notebook_container (gfe);

    /* The Basic tab */
    if (priv->plugin->basic_settings)
    {
        table = remmina_file_editor_create_notebook_tab (gfe, GTK_STOCK_INFO, _("Basic"), 20, 2);
        remmina_file_editor_create_settings (gfe, table, priv->plugin->basic_settings);
    }

    /* The Advanced tab */
    if (priv->plugin->advanced_settings)
    {
        table = remmina_file_editor_create_notebook_tab (gfe, GTK_STOCK_DIALOG_WARNING, _("Advanced"), 20, 2);
        remmina_file_editor_create_settings (gfe, table, priv->plugin->advanced_settings);
    }

    /* The SSH tab */
    remmina_file_editor_create_ssh_tab (gfe, priv->plugin->ssh_setting);
}

static void
remmina_file_editor_protocol_combo_on_changed (GtkComboBox *combo, RemminaFileEditor *gfe)
{
    RemminaFileEditorPriv *priv = gfe->priv;
    gchar *protocol;

    if (priv->config_container)
    {
        gtk_container_remove (GTK_CONTAINER (priv->config_box), priv->config_container);
        priv->config_container = NULL;
    }

    priv->server_combo = NULL;
    priv->listenport_entry = NULL;
    priv->username_entry = NULL;
    priv->password_entry = NULL;
    priv->domain_entry = NULL;
    priv->clientname_entry = NULL;
    priv->resolution_auto_radio = NULL;
    priv->resolution_custom_radio = NULL;
    priv->resolution_custom_combo = NULL;
    priv->colordepth_combo = NULL;
    priv->quality_combo = NULL;
    priv->keymap_combo = NULL;
    priv->gkeymap_combo = NULL;
    priv->exec_widget = NULL;
    priv->execpath_entry = NULL;
    priv->sound_combo = NULL;
    priv->arguments_entry = NULL;
    priv->proxy_entry = NULL;
    priv->sharefolder_check = NULL;
    priv->sharefolder_chooser = NULL;
    priv->scaler_widget = NULL;

    priv->bitmapcaching_check = NULL;
    priv->compression_check = NULL;
    priv->showcursor_check = NULL;
    priv->viewonly_check = NULL;
    priv->console_check = NULL;
    priv->disableserverinput_check = NULL;
    priv->shareprinter_check = NULL;
    priv->once_check = NULL;
    priv->disableencryption_check = NULL;
    priv->disableclipboard_check = NULL;

    priv->ssh_enabled_check = NULL;
    priv->ssh_loopback_check = NULL;
    priv->ssh_server_default_radio = NULL;
    priv->ssh_server_custom_radio = NULL;
    priv->ssh_server_entry = NULL;
    priv->ssh_username_entry = NULL;
    priv->ssh_auth_password_radio = NULL;
    priv->ssh_auth_publickey_radio = NULL;
    priv->ssh_auth_auto_publickey_radio = NULL;
    priv->ssh_privatekey_chooser = NULL;
    priv->ssh_charset_combo = NULL;

    protocol = gtk_combo_box_get_active_text (combo);
    if (protocol)
    {
        priv->plugin = (RemminaProtocolPlugin *) remmina_plugin_manager_get_plugin (REMMINA_PLUGIN_TYPE_PROTOCOL, protocol);
        g_free (protocol);
        remmina_file_editor_create_all_settings (gfe);
    }
}

static void
remmina_file_editor_update (RemminaFileEditor *gfe)
{
    RemminaFileEditorPriv *priv = gfe->priv;
    gchar *s;

    g_free (priv->remmina_file->name);
    priv->remmina_file->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->name_entry)));

    g_free (priv->remmina_file->group);
    priv->remmina_file->group = (priv->group_combo ?
        gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->group_combo)) : NULL);

    g_free (priv->remmina_file->protocol);
    priv->remmina_file->protocol = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->protocol_combo));

    g_free (priv->remmina_file->server);
    priv->remmina_file->server = (priv->server_combo ?
        gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->server_combo)) : NULL);

    if (priv->username_entry)
    {
        g_free (priv->remmina_file->username);
        priv->remmina_file->username = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->username_entry)));
    }

    g_free (priv->remmina_file->password);
    priv->remmina_file->password = (priv->password_entry ?
        g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->password_entry))) : NULL);

    if (priv->domain_entry)
    {
        g_free (priv->remmina_file->domain);
        priv->remmina_file->domain = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->domain_entry)));
    }

    if (priv->clientname_entry)
    {
        g_free (priv->remmina_file->clientname);
        priv->remmina_file->clientname = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->clientname_entry)));
    }

    if (priv->resolution_auto_radio)
    {
        g_free (priv->remmina_file->resolution);
        priv->remmina_file->resolution = (
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->resolution_auto_radio)) ?
                g_strdup ("AUTO") :
                gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->resolution_custom_combo)));
    }

    if (priv->listenport_entry)
    {
        priv->remmina_file->listenport = atoi (
            gtk_entry_get_text (GTK_ENTRY (priv->listenport_entry)));
    }

    if (priv->colordepth_combo)
    {
        s = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->colordepth_combo));
        priv->remmina_file->colordepth = atoi (s);
        g_free (s);
    }

    if (priv->quality_combo)
    {
        s = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->quality_combo));
        priv->remmina_file->quality = atoi (s);
        g_free (s);
    }

    if (priv->keymap_combo)
    {
        g_free (priv->remmina_file->keymap);
        priv->remmina_file->keymap = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->keymap_combo));
    }

    if (priv->gkeymap_combo)
    {
        g_free (priv->remmina_file->gkeymap);
        priv->remmina_file->gkeymap = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->gkeymap_combo));
    }

    if (priv->sharefolder_check && priv->sharefolder_chooser)
    {
        g_free (priv->remmina_file->sharefolder);
        priv->remmina_file->sharefolder = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->sharefolder_check)) ?
            g_strdup (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->sharefolder_chooser))) : NULL;
    }

    if (priv->scaler_widget)
    {
        priv->remmina_file->hscale = REMMINA_SCALER (priv->scaler_widget)->hscale;
        priv->remmina_file->vscale = REMMINA_SCALER (priv->scaler_widget)->vscale;
        priv->remmina_file->aspectscale = REMMINA_SCALER (priv->scaler_widget)->aspectscale;
    }

    if (priv->ssh_charset_combo)
    {
        g_free (priv->remmina_file->ssh_charset);
        priv->remmina_file->ssh_charset = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->ssh_charset_combo));
    }

    if (priv->exec_widget)
    {
        g_free (priv->remmina_file->exec);
        if (GTK_IS_ENTRY (priv->exec_widget))
        {
            priv->remmina_file->exec = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->exec_widget)));
        }
        else
        {
            priv->remmina_file->exec = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->exec_widget));
        }
    }

    if (priv->execpath_entry)
    {
        g_free (priv->remmina_file->execpath);
        priv->remmina_file->execpath = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->execpath_entry)));
    }

    if (priv->sound_combo)
    {
        g_free (priv->remmina_file->sound);
        priv->remmina_file->sound = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->sound_combo));
    }

    if (priv->arguments_entry)
    {
        g_free (priv->remmina_file->arguments);
        priv->remmina_file->arguments = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->arguments_entry)));
    }

    if (priv->proxy_entry)
    {
        g_free (priv->remmina_file->proxy);
        priv->remmina_file->proxy = g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->proxy_entry)));
    }

    if (priv->bitmapcaching_check)
    {
        priv->remmina_file->bitmapcaching = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->bitmapcaching_check));
    }
    if (priv->compression_check)
    {
        priv->remmina_file->compression = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->compression_check));
    }
    if (priv->showcursor_check)
    {
        priv->remmina_file->showcursor = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->showcursor_check));
    }
    if (priv->viewonly_check)
    {
        priv->remmina_file->viewonly = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->viewonly_check));
    }
    if (priv->console_check)
    {
        priv->remmina_file->console = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->console_check));
    }
    if (priv->disableserverinput_check)
    {
        priv->remmina_file->disableserverinput = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->disableserverinput_check));
    }
    if (priv->shareprinter_check)
    {
        priv->remmina_file->shareprinter = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->shareprinter_check));
    }
    if (priv->once_check)
    {
        priv->remmina_file->once = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->once_check));
    }
    if (priv->disableencryption_check)
    {
        priv->remmina_file->disableencryption = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->disableencryption_check));
    }
    if (priv->disableclipboard_check)
    {
        priv->remmina_file->disableclipboard = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->disableclipboard_check));
    }

    if (g_strcmp0 (priv->remmina_file->protocol, "SFTP") == 0 ||
        g_strcmp0 (priv->remmina_file->protocol, "SSH") == 0)
    {
        priv->remmina_file->ssh_enabled = TRUE;
    }
    else
    {
        priv->remmina_file->ssh_enabled = (priv->ssh_enabled_check ?
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->ssh_enabled_check)) : FALSE);
        priv->remmina_file->ssh_loopback = (priv->ssh_loopback_check ?
            gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->ssh_loopback_check)) : FALSE);
    }
    g_free (priv->remmina_file->ssh_username);
    priv->remmina_file->ssh_username = (priv->remmina_file->ssh_enabled ?
        g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->ssh_username_entry))) : NULL);
    g_free (priv->remmina_file->ssh_server);
    priv->remmina_file->ssh_server = (priv->remmina_file->ssh_enabled && priv->ssh_server_entry &&
        (priv->ssh_server_custom_radio == NULL ||
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->ssh_server_custom_radio))) ?
        g_strdup (gtk_entry_get_text (GTK_ENTRY (priv->ssh_server_entry))) : NULL);
    priv->remmina_file->ssh_auth = (
        priv->ssh_auth_publickey_radio &&
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->ssh_auth_publickey_radio)) ?
        SSH_AUTH_PUBLICKEY :
        priv->ssh_auth_auto_publickey_radio &&
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->ssh_auth_auto_publickey_radio)) ?
        SSH_AUTH_AUTO_PUBLICKEY :
        SSH_AUTH_PASSWORD);
    g_free (priv->remmina_file->ssh_privatekey);
    priv->remmina_file->ssh_privatekey = (
        priv->ssh_privatekey_chooser ?
        g_strdup (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (priv->ssh_privatekey_chooser))) : NULL);
}

static void
remmina_file_editor_on_default (GtkWidget *button, RemminaFileEditor *gfe)
{
    RemminaFile *gf;
    GtkWidget *dialog;

    remmina_file_editor_update (gfe);

    gf = remmina_file_dup (gfe->priv->remmina_file);

    g_free (gf->filename);
    gf->filename = g_strdup (remmina_pref_file);

    /* Clear properties that should never be default */
    g_free (gf->name); gf->name = NULL;
    g_free (gf->server); gf->server = NULL;
    g_free (gf->password); gf->password = NULL;

    remmina_file_save_all (gf);

    remmina_file_free (gf);

    dialog = gtk_message_dialog_new (GTK_WINDOW (gfe),
        GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
        _("Default settings saved."));
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
}

static void
remmina_file_editor_on_save (GtkWidget *button, RemminaFileEditor *gfe)
{
    remmina_file_editor_update (gfe);
    remmina_file_save_all (gfe->priv->remmina_file);
    gtk_widget_destroy (GTK_WIDGET (gfe));
}

static void
remmina_file_editor_on_connect (GtkWidget *button, RemminaFileEditor *gfe)
{
    RemminaFile *gf;

    remmina_file_editor_update (gfe);
    if (remmina_pref.save_when_connect)
    {
        remmina_file_save_all (gfe->priv->remmina_file);
    }
    gf = remmina_file_dup (gfe->priv->remmina_file);
    /* Put server into name for Quick Connect */
    if (gf->filename == NULL)
    {
        g_free (gf->name);
        gf->name = g_strdup (gf->server);
    }
    gtk_widget_destroy (GTK_WIDGET (gfe));
    remmina_connection_window_open_from_file (gf);
}

static void
remmina_file_editor_on_cancel (GtkWidget *button, RemminaFileEditor *gfe)
{
    gtk_widget_destroy (GTK_WIDGET (gfe));
}

static void
remmina_file_editor_init (RemminaFileEditor *gfe)
{
    RemminaFileEditorPriv *priv;
    GtkWidget *widget;

    priv = g_new0 (RemminaFileEditorPriv, 1);
    gfe->priv = priv;

    /* Create the editor dialog */
    gtk_window_set_title (GTK_WINDOW (gfe), _("Remote Desktop Preference"));

    widget = gtk_dialog_add_button (GTK_DIALOG (gfe), GTK_STOCK_SAVE, GTK_RESPONSE_APPLY);
    g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_file_editor_on_save), gfe);

    widget = gtk_dialog_add_button (GTK_DIALOG (gfe), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_file_editor_on_cancel), gfe);

    widget = gtk_dialog_add_button (GTK_DIALOG (gfe), GTK_STOCK_CONNECT, GTK_RESPONSE_OK);
    g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_file_editor_on_connect), gfe);

    gtk_dialog_set_alternative_button_order (GTK_DIALOG (gfe),
        GTK_RESPONSE_OK, GTK_RESPONSE_APPLY, GTK_RESPONSE_CANCEL, -1);
    gtk_dialog_set_default_response (GTK_DIALOG (gfe), GTK_RESPONSE_OK);
    gtk_dialog_set_has_separator (GTK_DIALOG (gfe), FALSE);
    gtk_window_set_default_size (GTK_WINDOW (gfe), 450, 500);

    g_signal_connect (G_OBJECT (gfe),
        "destroy",G_CALLBACK (remmina_file_editor_destroy), NULL);
    g_signal_connect (G_OBJECT (gfe),
        "realize",G_CALLBACK (remmina_file_editor_on_realize), NULL);

    /* The Set As Default button */
    widget = gtk_button_new_with_label (_("Default"));
    gtk_widget_show (widget);
    gtk_button_set_image (GTK_BUTTON (widget), gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_BUTTON));
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gfe)->action_area), widget, FALSE, TRUE, 0);
    gtk_button_box_set_child_secondary (GTK_BUTTON_BOX (GTK_DIALOG (gfe)->action_area), widget, TRUE);

    g_signal_connect (G_OBJECT (widget), "clicked", G_CALLBACK (remmina_file_editor_on_default), gfe);

    remmina_widget_pool_register (GTK_WIDGET (gfe));
}

static gboolean
remmina_file_editor_iterate_protocol (gchar *protocol, RemminaPlugin *plugin, gpointer data)
{
    RemminaFileEditor *gfe = REMMINA_FILE_EDITOR (data);
    GtkListStore *store;
    GtkTreeIter iter;
    gboolean first;

    store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (gfe->priv->protocol_combo)));

    first = !gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter, 0, protocol, 1, plugin->description, 2, ((RemminaProtocolPlugin *)plugin)->icon_name, -1);

    if (first || g_strcmp0 (protocol, gfe->priv->remmina_file->protocol) == 0)
    {
        gtk_combo_box_set_active_iter (GTK_COMBO_BOX (gfe->priv->protocol_combo), &iter);
    }

    return FALSE;
}

GtkWidget*
remmina_file_editor_new_from_file (RemminaFile *remminafile)
{
    RemminaFileEditor *gfe;
    RemminaFileEditorPriv *priv;
    GtkWidget *table;
    GtkWidget *widget;
    gchar *groups;

    gfe = REMMINA_FILE_EDITOR (g_object_new (REMMINA_TYPE_FILE_EDITOR, NULL));
    priv = gfe->priv;
    priv->remmina_file = remminafile;

    if (remminafile->filename == NULL)
    {
        gtk_dialog_set_response_sensitive (GTK_DIALOG (gfe), GTK_RESPONSE_APPLY, FALSE);
    }

    /* Create the Profile group on the top (for name and protocol) */
    table = gtk_table_new (4, 3, FALSE);
    gtk_widget_show (table);
    gtk_table_set_row_spacings (GTK_TABLE (table), 4);
    gtk_table_set_col_spacings (GTK_TABLE (table), 8);
    gtk_container_set_border_width (GTK_CONTAINER (table), 8);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gfe)->vbox), table, FALSE, FALSE, 2);

    remmina_public_create_group (GTK_TABLE (table), _("Profile"), 0, 4, 3);

    /* Profile: Name */
    widget = gtk_label_new (_("Name"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 1, 2, GTK_FILL, 0, 0, 0);

    widget = gtk_entry_new_with_max_length (100);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 2, 3, 1, 2);
    priv->name_entry = widget;

    if (remminafile->filename == NULL)
    {
        gtk_widget_set_sensitive (widget, FALSE);
        gtk_entry_set_text (GTK_ENTRY (widget), _("Quick Connect"));
    }
    else
    {
        if (remminafile->name) gtk_entry_set_text (GTK_ENTRY (widget), remminafile->name);
    }

    /* Profile: Group */
    if (remminafile->filename == NULL)
    {
        priv->group_combo = NULL;
    }
    else
    {
        widget = gtk_label_new (_("Group"));
        gtk_widget_show (widget);
        gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
        gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 2, 3, GTK_FILL, 0, 0, 0);

        groups = remmina_file_manager_get_groups ();
        priv->group_combo = remmina_public_create_combo_entry (groups, remminafile->group, FALSE);
        g_free (groups);
        gtk_widget_show (priv->group_combo);
        gtk_table_attach_defaults (GTK_TABLE (table), priv->group_combo, 2, 3, 2, 3);
    }

    /* Profile: Protocol */
    widget = gtk_label_new (_("Protocol"));
    gtk_widget_show (widget);
    gtk_misc_set_alignment (GTK_MISC (widget), 0.0, 0.5);
    gtk_table_attach (GTK_TABLE (table), widget, 1, 2, 3, 4, GTK_FILL, 0, 0, 0);

    widget = remmina_public_create_combo (TRUE);
    gtk_widget_show (widget);
    gtk_table_attach_defaults (GTK_TABLE (table), widget, 2, 3, 3, 4);
    priv->protocol_combo = widget;
    remmina_plugin_manager_for_each_plugin (REMMINA_PLUGIN_TYPE_PROTOCOL, remmina_file_editor_iterate_protocol, gfe);
    g_signal_connect (G_OBJECT (widget), "changed", G_CALLBACK (remmina_file_editor_protocol_combo_on_changed), gfe);

    /* Create the Preference frame */
    widget = gtk_event_box_new ();
    gtk_widget_show (widget);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (gfe)->vbox), widget, TRUE, TRUE, 2);
    priv->config_box = widget;

    priv->config_container = NULL;

    remmina_file_editor_protocol_combo_on_changed (GTK_COMBO_BOX (priv->protocol_combo), gfe);

    return GTK_WIDGET (gfe);
}

GtkWidget*
remmina_file_editor_new (void)
{
    return remmina_file_editor_new_full (NULL, NULL);
}

GtkWidget*
remmina_file_editor_new_full (const gchar *server, const gchar *protocol)
{
    RemminaFile *remminafile;

    remminafile = remmina_file_new ();
    if (server) { g_free (remminafile->server); remminafile->server = g_strdup (server); }
    if (protocol) { g_free (remminafile->protocol); remminafile->protocol = g_strdup (protocol); }

    return remmina_file_editor_new_from_file (remminafile);
}

GtkWidget*
remmina_file_editor_new_copy (const gchar *filename)
{
    RemminaFile *remminafile;
    GtkWidget *dialog;

    remminafile = remmina_file_copy (filename);
    if (remminafile)
    {
        return remmina_file_editor_new_from_file (remminafile);
    }
    else
    {
        dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            _("File %s not found."), filename);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return NULL;
    }
}

GtkWidget*
remmina_file_editor_new_from_filename (const gchar *filename)
{
    RemminaFile *remminafile;
    GtkWidget *dialog;

    remminafile = remmina_file_manager_load_file (filename);
    if (remminafile)
    {
        return remmina_file_editor_new_from_file (remminafile);
    }
    else
    {
        dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
            _("File %s not found."), filename);
        gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);
        return NULL;
    }
}

GtkWidget*
remmina_file_editor_new_temp (void)
{
    return remmina_file_editor_new_temp_full (NULL, NULL);
}

GtkWidget*
remmina_file_editor_new_temp_full (const gchar *server, const gchar *protocol)
{
    RemminaFile *remminafile;

    remminafile = remmina_file_new_temp ();
    if (server) { g_free (remminafile->server); remminafile->server = g_strdup (server); }
    if (protocol) { g_free (remminafile->protocol); remminafile->protocol = g_strdup (protocol); }

    return remmina_file_editor_new_from_file (remminafile);
}

