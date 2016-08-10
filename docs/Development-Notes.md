Use this page to add useful links and notes to help out with the development

# GTK3

## GTK3 Deprecation mini guide

* [GTK3 Reference Manual](https://developer.gnome.org/gtk3/unstable/index.html)
* [Icon names standard](http://standards.freedesktop.org/icon-naming-spec/icon-naming-spec-latest.html#names)
* [GTK+ Stock Item Deprecation](https://docs.google.com/document/d/1KCVPoYQBqMbDP11tHPpjW6uaEHrvLUmcDPqKAppCY8o/pub)
* [Stock Items Migration Guide](https://docs.google.com/spreadsheet/pub?key=0AsPAM3pPwxagdGF4THNMMUpjUW5xMXZfdUNzMXhEa2c&output=html)
* Menu and GtkActionGroup deprecation: [what to use instead](http://stackoverflow.com/questions/24788045/gtk-action-group-new-and-gtkstock-what-to-use-instead)
* _"GtkAlignment has been deprecated in 3.14 and should not be used in newly-written code. The desired effect can be achieved by using the “halign”, “valign” and “margin” properties on the child widget."_ In remmina GtkAlignment has been replaced by a GtkAspectFrame, used by the "scaler" code to scale the remote desktop.
* Icons in menu are being removed. See [here](https://igurublog.wordpress.com/2014/03/22/gtk-3-10-drops-menu-icons-and-mnemonics/). Also mnemonics should be removed.

## Deprecated to be migrated in Remmina next

 * g_cond_free
 * g_cond_new
 * gdk_color_parse
 * gdk_get_display
 * gdk_threads_enter
 * gdk_threads_init
 * gdk_threads_leave
 * g_io_channel_close
 * g_io_channel_seek
 * g_mutex_free
 * g_mutex_new
 * gnome_keyring_delete_password_sync
 * gnome_keyring_find_password_sync
 * gnome_keyring_free_password
 * gnome_keyring_result_to_message
 * gnome_keyring_store_password_sync
 * gtk_action_group_add_action
 * gtk_action_group_add_actions
 * gtk_action_group_add_radio_actions
 * gtk_action_group_add_toggle_actions
 * gtk_action_group_get_action
 * gtk_action_group_new
 * gtk_action_group_set_translation_domain
 * gtk_action_new
 * gtk_alignment_get_type
 * gtk_alignment_new
 * gtk_alignment_set
 * gtk_arrow_new
 * gtk_dialog_get_action_area
 * gtk_dialog_set_alternative_button_order
 * gtk_entry_set_icon_from_stock
 * gtk_image_menu_item_get_type
 * gtk_image_menu_item_new_from_stock
 * gtk_misc_get_type
 * gtk_misc_set_alignment
 * gtk_radio_action_get_current_value
 * GtkStock
 * gtk_table_attach
 * gtk_table_attach_defaults
 * gtk_table_get_type
 * gtk_table_resize
 * gtk_toggle_action_get_active
 * gtk_toggle_action_get_type
 * gtk_toggle_action_set_active
 * gtk_ui_manager_add_ui
 * gtk_ui_manager_add_ui_from_string
 * gtk_ui_manager_get_accel_group
 * gtk_ui_manager_get_widget
 * gtk_ui_manager_insert_action_group
 * gtk_ui_manager_new
 * gtk_ui_manager_new_merge_id
 * gtk_vbox_get_type
 * gtk_widget_modify_fg
 * gtk_widget_set_margin_right
 * g_type_init
 * vte_terminal_get_adjustment



