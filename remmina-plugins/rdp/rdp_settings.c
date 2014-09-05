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

#include "rdp_plugin.h"
#include "rdp_settings.h"
#include <freerdp/locale/keyboard.h>

static guint keyboard_layout = 0;
static guint rdp_keyboard_layout = 0;

static void remmina_rdp_settings_kbd_init(void)
{
	keyboard_layout = freerdp_keyboard_init(rdp_keyboard_layout);
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

#define REMMINA_TYPE_PLUGIN_RDPSET_GRID		(remmina_rdp_settings_grid_get_type())
#define REMMINA_RDPSET_GRID(obj)			(G_TYPE_CHECK_INSTANCE_CAST((obj), REMMINA_TYPE_PLUGIN_RDPSET_GRID, RemminaPluginRdpsetGrid))
#define REMMINA_RDPSET_GRID_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST((klass), REMMINA_TYPE_PLUGIN_RDPSET_GRID, RemminaPluginRdpsetGridClass))
#define REMMINA_IS_PLUGIN_RDPSET_GRID(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), REMMINA_TYPE_PLUGIN_RDPSET_GRID))
#define REMMINA_IS_PLUGIN_RDPSET_GRID_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE((klass), REMMINA_TYPE_PLUGIN_RDPSET_GRID))
#define REMMINA_RDPSET_GRID_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS((obj), REMMINA_TYPE_PLUGIN_RDPSET_GRID, RemminaPluginRdpsetGridClass))

typedef struct _RemminaPluginRdpsetGrid
{
	GtkGrid grid;

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
} RemminaPluginRdpsetGrid;

typedef struct _RemminaPluginRdpsetGridClass
{
	GtkGridClass parent_class;
} RemminaPluginRdpsetGridClass;

GType remmina_rdp_settings_grid_get_type(void) G_GNUC_CONST;

G_DEFINE_TYPE(RemminaPluginRdpsetGrid, remmina_rdp_settings_grid, GTK_TYPE_GRID)

static void remmina_rdp_settings_grid_class_init(RemminaPluginRdpsetGridClass* klass)
{
}

static void remmina_rdp_settings_grid_destroy(GtkWidget* widget, gpointer data)
{
	gchar* s;
	guint new_layout;
	GtkTreeIter iter;
	RemminaPluginRdpsetGrid* grid;

	grid = REMMINA_RDPSET_GRID(widget);

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(grid->keyboard_layout_combo), &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(grid->keyboard_layout_store), &iter, 0, &new_layout, -1);

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
		gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->use_client_keymap_check)) ? "1" : "0");

	s = g_strdup_printf("%X", grid->quality_values[0]);
	remmina_plugin_service->pref_set_value("rdp_quality_0", s);
	g_free(s);

	s = g_strdup_printf("%X", grid->quality_values[1]);
	remmina_plugin_service->pref_set_value("rdp_quality_1", s);
	g_free(s);

	s = g_strdup_printf("%X", grid->quality_values[2]);
	remmina_plugin_service->pref_set_value("rdp_quality_2", s);
	g_free(s);

	s = g_strdup_printf("%X", grid->quality_values[9]);
	remmina_plugin_service->pref_set_value("rdp_quality_9", s);
	g_free(s);
}

static void remmina_rdp_settings_grid_load_layout(RemminaPluginRdpsetGrid* grid)
{
	gint i;
	gchar* s;
	GtkTreeIter iter;
	RDP_KEYBOARD_LAYOUT* layouts;

	gtk_list_store_append(grid->keyboard_layout_store, &iter);
	gtk_list_store_set(grid->keyboard_layout_store, &iter, 0, 0, 1, _("<Auto detect>"), -1);

	if (rdp_keyboard_layout == 0)
		gtk_combo_box_set_active(GTK_COMBO_BOX(grid->keyboard_layout_combo), 0);

	gtk_label_set_text(GTK_LABEL(grid->keyboard_layout_label), "-");

	layouts = freerdp_keyboard_get_layouts(RDP_KEYBOARD_LAYOUT_TYPE_STANDARD | RDP_KEYBOARD_LAYOUT_TYPE_VARIANT);

	for (i = 0; layouts[i].code; i++)
	{
		s = g_strdup_printf("%08X - %s", layouts[i].code, layouts[i].name);
		gtk_list_store_append(grid->keyboard_layout_store, &iter);
		gtk_list_store_set(grid->keyboard_layout_store, &iter, 0, layouts[i].code, 1, s, -1);

		if (rdp_keyboard_layout == layouts[i].code)
			gtk_combo_box_set_active(GTK_COMBO_BOX(grid->keyboard_layout_combo), i + 1);

		if (keyboard_layout == layouts[i].code)
			gtk_label_set_text(GTK_LABEL(grid->keyboard_layout_label), s);

		g_free(s);
	}

	free(layouts);
}

static void remmina_rdp_settings_grid_load_quality(RemminaPluginRdpsetGrid* grid)
{
	gchar* value;
	GtkTreeIter iter;

	gtk_list_store_append(grid->quality_store, &iter);
	gtk_list_store_set(grid->quality_store, &iter, 0, 0, 1, _("Poor (fastest)"), -1);
	gtk_list_store_append(grid->quality_store, &iter);
	gtk_list_store_set(grid->quality_store, &iter, 0, 1, 1, _("Medium"), -1);
	gtk_list_store_append(grid->quality_store, &iter);
	gtk_list_store_set(grid->quality_store, &iter, 0, 2, 1, _("Good"), -1);
	gtk_list_store_append(grid->quality_store, &iter);
	gtk_list_store_set(grid->quality_store, &iter, 0, 9, 1, _("Best (slowest)"), -1);

	memset(grid->quality_values, 0, sizeof (grid->quality_values));

	value = remmina_plugin_service->pref_get_value("rdp_quality_0");
	grid->quality_values[0] = (value && value[0] ? strtoul(value, NULL, 16) : DEFAULT_QUALITY_0);
	g_free(value);

	value = remmina_plugin_service->pref_get_value("rdp_quality_1");
	grid->quality_values[1] = (value && value[0] ? strtoul(value, NULL, 16) : DEFAULT_QUALITY_1);
	g_free(value);

	value = remmina_plugin_service->pref_get_value("rdp_quality_2");
	grid->quality_values[2] = (value && value[0] ? strtoul(value, NULL, 16) : DEFAULT_QUALITY_2);
	g_free(value);

	value = remmina_plugin_service->pref_get_value("rdp_quality_9");
	grid->quality_values[9] = (value && value[0] ? strtoul(value, NULL, 16) : DEFAULT_QUALITY_9);
	g_free(value);
}

static void remmina_rdp_settings_quality_on_changed(GtkComboBox *widget, RemminaPluginRdpsetGrid *grid)
{
	guint v;
	guint i = 0;
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX (grid->quality_combo), &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(grid->quality_store), &iter, 0, &i, -1);
		v = grid->quality_values[i];
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid->wallpaper_check), (v & 1) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid->windowdrag_check), (v & 2) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid->menuanimation_check), (v & 4) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid->theme_check), (v & 8) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid->cursorshadow_check), (v & 0x20) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid->cursorblinking_check), (v & 0x40) == 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid->fontsmoothing_check), (v & 0x80) != 0);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(grid->composition_check), (v & 0x100) != 0);
	}
}

static void remmina_rdp_settings_quality_option_on_toggled(GtkToggleButton* togglebutton, RemminaPluginRdpsetGrid* grid)
{
	guint v;
	guint i = 0;
	GtkTreeIter iter;

	if (gtk_combo_box_get_active_iter(GTK_COMBO_BOX(grid->quality_combo), &iter))
	{
		gtk_tree_model_get(GTK_TREE_MODEL(grid->quality_store), &iter, 0, &i, -1);
		v = 0;
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->wallpaper_check)) ? 0 : 1);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->windowdrag_check)) ? 0 : 2);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->menuanimation_check)) ? 0 : 4);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->theme_check)) ? 0 : 8);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->cursorshadow_check)) ? 0 : 0x20);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->cursorblinking_check)) ? 0 : 0x40);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->fontsmoothing_check)) ? 0x80 : 0);
		v |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(grid->composition_check)) ? 0x100 : 0);
		grid->quality_values[i] = v;
	}
}

static void remmina_rdp_settings_grid_init(RemminaPluginRdpsetGrid *grid)
{
	gchar* s;
	GtkWidget* widget;
	GtkCellRenderer* renderer;

	/* Create the grid */
	g_signal_connect(G_OBJECT(grid), "destroy", G_CALLBACK(remmina_rdp_settings_grid_destroy), NULL);
	gtk_grid_set_row_homogeneous(GTK_GRID(grid), FALSE);
	gtk_grid_set_column_homogeneous(GTK_GRID(grid), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(grid), 8);
	gtk_grid_set_row_spacing(GTK_GRID(grid), 4);
	gtk_grid_set_column_spacing(GTK_GRID(grid), 4);

    //gtk_grid_attach (GtkGrid *grid, GtkWidget *child, gint left, gint top, gint width, gint height);
	/* Create the content */
	widget = gtk_label_new(_("Keyboard layout"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 0, 1, 1);

	grid->keyboard_layout_store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
	widget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(grid->keyboard_layout_store));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 0, 4, 1);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", 1);
	grid->keyboard_layout_combo = widget;

	widget = gtk_label_new("-");
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 1, 4, 2);
	grid->keyboard_layout_label = widget;

	remmina_rdp_settings_grid_load_layout(grid);

	widget = gtk_check_button_new_with_label(_("Use client keyboard mapping"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 3, 3, 3);
	grid->use_client_keymap_check = widget;

	s = remmina_plugin_service->pref_get_value("rdp_use_client_keymap");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
		s && s[0] == '1' ? TRUE : FALSE);
	g_free(s);

	widget = gtk_label_new(_("Quality option"));
	gtk_widget_show(widget);
	gtk_misc_set_alignment(GTK_MISC(widget), 0.0, 0.5);
	gtk_grid_attach(GTK_GRID(grid), widget, 0, 6, 1, 4);

	grid->quality_store = gtk_list_store_new(2, G_TYPE_UINT, G_TYPE_STRING);
	widget = gtk_combo_box_new_with_model(GTK_TREE_MODEL(grid->quality_store));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 6, 4, 4);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(widget), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(widget), renderer, "text", 1);
	g_signal_connect(G_OBJECT(widget), "changed",
		G_CALLBACK(remmina_rdp_settings_quality_on_changed), grid);
	grid->quality_combo = widget;

	remmina_rdp_settings_grid_load_quality(grid);

	widget = gtk_check_button_new_with_label(_("Wallpaper"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 10, 2, 5);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), grid);
	grid->wallpaper_check = widget;

	widget = gtk_check_button_new_with_label(_("Window drag"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 3, 10, 3, 5);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), grid);
	grid->windowdrag_check = widget;

	widget = gtk_check_button_new_with_label(_("Menu animation"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 13, 2, 6);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), grid);
	grid->menuanimation_check = widget;

	widget = gtk_check_button_new_with_label(_("Theme"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 3, 13, 3, 6);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), grid);
	grid->theme_check = widget;

	widget = gtk_check_button_new_with_label(_("Cursor shadow"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 16, 2, 7);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), grid);
	grid->cursorshadow_check = widget;

	widget = gtk_check_button_new_with_label(_("Cursor blinking"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 3, 16, 3, 7);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), grid);
	grid->cursorblinking_check = widget;

	widget = gtk_check_button_new_with_label(_("Font smoothing"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 1, 19, 2, 8);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), grid);
	grid->fontsmoothing_check = widget;

	widget = gtk_check_button_new_with_label(_("Composition"));
	gtk_widget_show(widget);
	gtk_grid_attach(GTK_GRID(grid), widget, 3, 19, 3, 8);
	g_signal_connect(G_OBJECT(widget), "toggled",
		G_CALLBACK(remmina_rdp_settings_quality_option_on_toggled), grid);
	grid->composition_check = widget;

	gtk_combo_box_set_active(GTK_COMBO_BOX (grid->quality_combo), 0);
}

GtkWidget* remmina_rdp_settings_new(void)
{
	GtkWidget* widget;

	widget = GTK_WIDGET(g_object_new(REMMINA_TYPE_PLUGIN_RDPSET_GRID, NULL));
	gtk_widget_show(widget);

	return widget;
}

