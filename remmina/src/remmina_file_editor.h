/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAFILEEDITOR_H__
#define __REMMINAFILEEDITOR_H__

G_BEGIN_DECLS

#define REMMINA_TYPE_FILE_EDITOR               (remmina_file_editor_get_type ())
#define REMMINA_FILE_EDITOR(obj)               (G_TYPE_CHECK_INSTANCE_CAST ((obj), REMMINA_TYPE_FILE_EDITOR, RemminaFileEditor))
#define REMMINA_FILE_EDITOR_CLASS(klass)       (G_TYPE_CHECK_CLASS_CAST ((klass), REMMINA_TYPE_FILE_EDITOR, RemminaFileEditorClass))
#define REMMINA_IS_FILE_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REMMINA_TYPE_FILE_EDITOR))
#define REMMINA_IS_FILE_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_TYPE ((klass), REMMINA_TYPE_FILE_EDITOR))
#define REMMINA_FILE_EDITOR_GET_CLASS(obj)     (G_TYPE_INSTANCE_GET_CLASS ((obj), REMMINA_TYPE_FILE_EDITOR, RemminaFileEditorClass))

typedef struct _RemminaFileEditorPriv RemminaFileEditorPriv;

typedef struct _RemminaFileEditor
{
	GtkDialog dialog;

	RemminaFileEditorPriv* priv;
} RemminaFileEditor;

typedef struct _RemminaFileEditorClass
{
	GtkDialogClass parent_class;
} RemminaFileEditorClass;

GType remmina_file_editor_get_type(void)
G_GNUC_CONST;

/* Base constructor */
GtkWidget* remmina_file_editor_new_from_file(RemminaFile* remminafile);
/* Create new file */
GtkWidget* remmina_file_editor_new(void);
GtkWidget* remmina_file_editor_new_full(const gchar* server, const gchar* protocol);
GtkWidget* remmina_file_editor_new_copy(const gchar* filename);
/* Open existing file */
GtkWidget* remmina_file_editor_new_from_filename(const gchar* filename);

G_END_DECLS

#endif  /* __REMMINAFILEEDITOR_H__  */

