/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINASTRINGLIST_H__
#define __REMMINASTRINGLIST_H__

typedef gboolean (*RemminaStringListValidationFunc)(const gchar *new_str, gchar **error);

typedef struct _RemminaStringListPriv
{
	RemminaStringListValidationFunc validation_func;
	const gchar *fields_separator;
	gboolean two_columns;
} RemminaStringListPriv;

typedef struct _RemminaStringList
{
	GtkBuilder *builder;
	GtkDialog *dialog;

	GtkListStore *liststore_items;
	GtkTreeView *treeview_items;
	GtkTreeViewColumn *treeviewcolumn_item;
	GtkTreeSelection *treeview_selection;
	GtkCellRendererText *cellrenderertext_item1;
	GtkCellRendererText *cellrenderertext_item2;

	GtkButton *button_add;
	GtkButton *button_remove;
	GtkButton *button_up;
	GtkButton *button_down;

	GtkLabel *label_title;
	GtkLabel *label_status;

	RemminaStringListPriv *priv;
} RemminaStringList;

G_BEGIN_DECLS

/* RemminaStringList instance */
GtkDialog* remmina_string_list_new(gboolean two_columns, const gchar *fields_separator);
/* Load a string list by splitting a string value */
void remmina_string_list_set_text(const gchar *text, const gboolean clear_data);
/* Get a string value representing the string list */
gchar* remmina_string_list_get_text(void);
/* Set the dialog titles */
void remmina_string_list_set_titles(gchar *title1, gchar *title2);
/* Set a function that will be used to validate the new rows */
void remmina_string_list_set_validation_func(RemminaStringListValidationFunc func);

G_END_DECLS

#endif  /* __REMMINASTRINGLIST_H__  */
