/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2010-2011 Vic Lee 
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

#include "rdp_plugin.h"
#include "rdp_settings.h"
#include <freerdp/kbd/kbd.h>

static guint keyboard_layout = 0;
static guint rdp_keyboard_layout = 0;

static void remmina_rdp_settings_kbd_init(void)
{
	keyboard_layout = freerdp_kbd_init(GDK_DISPLAY_XDISPLAY(gdk_display_get_default()), rdp_keyboard_layout);
}

void remmina_rdp_settings_init(void)
{
	gchar* value;

	value = remmina_plugin_service->pref_get_value("rdp_keyboard_layout");

	if (value && value[0])
		rdp_keyboard_layout = strtoul(value, NULL, 16);

	g_free(value);

	remmina_rdp_settings_kbd_init();
}

guint remmina_rdp_settings_get_keyboard_layout(void)
{
	return keyboard_layout;
}

#define REMMINA_TYPE_PLUGIN_RDPSET_TABLE		(remmina_rdp_settings_table_get_type())
#define REMMINA_RDPSET_TABLE(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), REMMINA_TYPE_PLUGIN_RDPSET_TABLE, RemminaPluginRdpsetTable))
#define REMMINA_RDPSET_TABLE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), REMMINA_TYPE_PLUGIN_RDPSET_TABLE, RemminaPluginRdpsetTableClass))
#define REMMINA_IS_PLUGIN_RDPSET_TABLE(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), REMMINA_TYPE_PLUGIN_RDPSET_TABLE))
#define REMMINA_IS_PLUGIN_RDPSET_TABLE_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), REMMINA_TYPE_PLUGIN_RDPSET_TABLE))
#define REMMINA_RDPSET_TABLE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), REMMINA_TYPE_PLUGIN_RDPSET_TABLE, RemminaPluginRdpsetTableClass))

typedef struct _RemminaPluginRdpsetTable
{
	GtkTable table;

	GtkWidget* keyboard_layout_label;
	GtkWidget* keyboard_layout_combo;
	GtkListStore* keyboard_layout_store;

	GtkWidget* quality_combo;
	GtkListStore* quality_store;
	GtkWidget* wallpaper_check;
	GtkWidget* windowdrag_check;
	GtkWidget* menuanimation_check;
	GtkWidget* theme_check;
	GtkWidget* cursorshadow_check;
	GtkWidget* cursorblinking_check;
	GtkWidget* fontsmoothing_check;
	GtkWidget* composition_check;
	GtkWidget* use_client_keymap_check;

	guint quality_values[10];
} RemminaPluginRdpsetTable;

typedef struct _RemminaPluginRdpsetTableClass
{
	GtkTableClass parent_class;
} RemminaPluginRdpsetTableClass;

GType remmina_rdp_settings_table_get_type(void) G_GNUC_CONST;

G_DEFINE_TYPE(RemminaPluginRdpsetTable, remmina_rdp_settings_table, GTK_TYPE_TABLE)

static void remmina_rdp_settings_table_class_init(RemminaPluginRdpsetTableClass* klass)
{
}

static void remmina_rdp_settings_table_destroy(GtkWidget* widget, gpointer data)
{
	gchar* s;
	guint new_layout;
	GtkTreeIter iter;
	RemminaPluginRdpsetTable* table;

	table = REMMINA_RDPSET_TABLE(widget);

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(table->keyboard_layout_combo), &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(table->keyboard_layout_store), &iter, 0, &new_layout, -1);

		if (new_layout != rdp_keyboard_layout)
		{
			rdp_keyboard_layout = new_layout;
			s = g_strdup_printf("%X", new_layout);
			remmina_plugin_service->pref_set_value("rdp_keyboard_layout", s);
			g_free(s);

			remmina_rdp_settings_kbd_init();
		}
	}

	remmina_plugin_service->pref_set_value("rdp_use_client_keymap",
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->use_client_keymap_check)) ? "1" : "0");

	s = g_strdup_printf("%X", table->quality_values[0]);
	remmina_plugin_service->pref_set_value("rdp_quality_0", s);
	g_free(s);

	s = g_strdup_printf("%X", table->quality_values[1]);
	remmina_plugin_service->pref_set_value("rdp_quality_1", s);
	g_free(s);

	s = g_strdup_printf("%X", table->quality_values[2]);
	remmina_plugin_service->pref_set_value("rdp_quality_2", s);
	g_free(s);

	s = g_strdup_printf("%X", table->quality_values[9]);
	remmina_plugin_service->pref_set_value("rdp_quality_9", s);
	g_free(s);
}

static void remmina_rdp_settings_table_load_layout(RemminaPluginRdpsetTable* table)
{
	gint i;
	gchar* s;
	GtkTreeIter iter;
	rdpKeyboardLayout* layouts;

	gtk_list_store_append(table->keyboard_layout_store, &iter);
	gtk_list_store_set(table->keyboard_layout_store, &iter, 0, 0, 1, _("<Auto detect>"), -1);

	if (rdp_keyboard_layout == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(table->keyboard_layout_combo), 0);

	gtk_label_set_text(GTK_LABEL(table->keyboard_layout_label), "-");

	layouts = freerdp_kbd_get_layouts(RDP_KEYBOARD_LAYOUT_TYPE_STANDARD | RDP_KEYBOARD_LAYOUT_TYPE_VARIANT);

	for (i = 0; layouts[i].code; i++)
	{
		s = g_strdup_printf("%08X - %s", layouts[i].code, layouts[i].name);
		gtk_list_store_append(table->keyboard_layout_store, &iter);
		gtk_list_store_set(table->keyboard_layout_store, &iter, 0, layouts[i].code, 1, s, -1);

		if (rdp_keyboard_layout == layouts[i].code)
			gtk_combo_box_set_active(GTK_COMBO_BOX(table->keyboard_layout_combo), i + 1);

		if (keyboard_layout == layouts[i].code)
			gtk_label_set_text(GTK_LABEL(table->keyboard_layout_label), s);

		g_free(s);
	}

	free(layouts);
}

static void remmina_rdp_settings_table_load_quality(RemminaPluginRdpsetTable* table)
{
	gchar* value;
	GtkTreeIter iter;

	gtk_list_store_append(table->quality_store, &iter);
	gtk_list_store_set(table->quality_store, &iter, 0, 0, 1, _("Poor (fastest)"), -1);
	gtk_list_store_append(table->quality_store, &iter);
	gtk_list_store_set(table->quality_store, &iter, 0, 1, 1, _("Medium"), -1);
	gtk_list_store_append(table->quality_store, &iter);
	gtk_list_store_set(table->quality_store, &iter, 0, 2, 1, _("Good"), -1);
	gtk_list_store_append(table->quality_store, &iter);
	gtk_list_store_set(table->quality_store, &iter, 0, 9, 1, _("Best (slowest)"), -1);

	memset(table->quality_values, 0, sizeof (table->quality_values));

	value = remmina_plugin_service->pref_get_value("rdp_quality_0");
	table->quality_values[0] = (value && value[0] ? strtoul(value, NULL, 16) : DEFAULT_QUALITY_0);
	g_free(value);

	value = remmina_plugin_service->pref_get_value("rdp_quality_1");
	table->quality_values[1] = (value && value[0] ? strtoul(value, NULL, 16) : DEFAULT_QUALITY_1);
	g_free(value);

	value = remmina_plugin_service->pref_get_value("rdp_quality_2");
	table->quality_values[2] = (value && value[0] ? strtoul(value, NULL, 16) : DEFAULT_QUALITY_2);
	g_free(value);

	value = remmina_plugin_service->pref_get_value("rdp_quality_9");
	table->quality_values[9] = (value && value[0] ? strtoul(value, NULL, 16) : DEFAULT_QUALITY_9);
	g_free(value);
}

static void remmina_rdp_settings_quality_on_changed(GtkComboBox *widget, RemminaPluginRdpsetTable *table)
{
	guint v;
	guint i = 0;
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX (table->quality_combo), &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(table->quality_store), &iter, 0, &i, -1);
		v = table->quality_values[i];
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->wallpaper_check), (v & 1) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->windowdrag_check), (v & 2) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->menuanimation_check), (v & 4) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->theme_check), (v & 8) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->cursorshadow_check), (v & 0x20) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->cursorblinking_check), (v & 0x40) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->fontsmoothing_check), (v & 0x80) != 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(table->composition_check), (v & 0x100) != 0);
	}
}

static void remmina_rdp_settings_quality_option_on_toggled(GtkToggleButton* togglebutton, RemminaPluginRdpsetTable* table)
{
	guint v;
	guint i = 0;
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(table->quality_combo), &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(table->quality_store), &iter, 0, &i, -1);
		v = 0;
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->wallpaper_check)) ? 0 : 1);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->windowdrag_check)) ? 0 : 2);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->menuanimation_check)) ? 0 : 4);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->theme_check)) ? 0 : 8);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->cursorshadow_check)) ? 0 : 0x20);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->cursorblinking_check)) ? 0 : 0x40);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->fontsmoothing_check)) ? 0x80 : 0);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(table->composition_check)) ? 0x100 : 0);
		table->quality_values[i] = v;
	}
}

static void remmina_rdp_settings_table_init(RemminaPluginRdpsetTable *table)
{
	gchar* s;
	GtkWidget* widget;
	GtkCellRenderer* renderer;

	/* Create the table */
	g_signal_connect(G_OBJECT(table), "destroy", G_CALLBACK(remmina_rdp_settings_table_destroy), NULL);
	gtk_table_resize(GTK_TABLE(table), 8, 3);
	gtk_table_set_homogeneous(GTK_TABLE(table), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(table), 8);
	gtk_table_set_row_spacings(GTK_TABLE(table), 4);
	gtk_table_set_col_spacings(GTK_TABLE(table), 4);

	/* Create the content */
	widget = gtk_label_new(_("Keyboard layout"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 0, 1);

	table->keyboard_layout_store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
	widget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(table->keyboard_layout_store));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 4, 0, 1);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", 1);
	table->keyboard_layout_combo = widget;

	widget = gtk_label_new("-");
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 4, 1, 2);
	table->keyboard_layout_label = widget;

	remmina_rdp_settings_table_load_layout(table);

	widget = gtk_check_button_new_with_label(_("Use client keyboard mapping"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 3, 2, 3);
	table->use_client_keymap_check = widget;

	s = remmina_plugin_service->pref_get_value("rdp_use_client_keymap");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
		s && s[0] == '1' ? TRUE : FALSE);
	g_free(s);

	widget = gtk_label_new(_("Quality option"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 0, 1, 3, 4);

	table->quality_store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
	widget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(table->quality_store));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 4, 3, 4);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", 1);
	g_signal_connect(G_OBJECT(widget), "changed",
		G_CALLBACK(remmina_rdp_settings_quality_on_changed), table);
	table->quality_combo = widget;

	remmina_rdp_settings_table_load_quality(table);

	widget = gtk_check_button_new_with_label(_("Wallpaper"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 4, 5);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), table);
	table->wallpaper_check = widget;

	widget = gtk_check_button_new_with_label(_("Window drag"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 2, 3, 4, 5);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), table);
	table->windowdrag_check = widget;

	widget = gtk_check_button_new_with_label(_("Menu animation"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 5, 6);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), table);
	table->menuanimation_check = widget;

	widget = gtk_check_button_new_with_label(_("Theme"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 2, 3, 5, 6);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), table);
	table->theme_check = widget;

	widget = gtk_check_button_new_with_label(_("Cursor shadow"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 6, 7);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), table);
	table->cursorshadow_check = widget;

	widget = gtk_check_button_new_with_label(_("Cursor blinking"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 2, 3, 6, 7);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), table);
	table->cursorblinking_check = widget;

	widget = gtk_check_button_new_with_label(_("Font smoothing"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 1, 2, 7, 8);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), table);
	table->fontsmoothing_check = widget;

	widget = gtk_check_button_new_with_label(_("Composition"));
	gtk_widget_show(widget);
	gtk_table_attach_defaults(GTK_TABLE(table), widget, 2, 3, 7, 8);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), table);
	table->composition_check = widget;

	gtk_combo_box_set_active(GTK_COMBO_BOX (table->quality_combo), 0);
}

GtkWidget* remmina_rdp_settings_new(void)
{
	GtkWidget* widget;

	widget = GTK_WIDGET(g_object_new(REMMINA_TYPE_PLUGIN_RDPSET_TABLE, NULL));
	gtk_widget_show(widget);

	return widget;
}

