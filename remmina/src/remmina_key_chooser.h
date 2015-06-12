/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAKEYCHOOSER_H__
#define __REMMINAKEYCHOOSER_H__

#define KEY_MODIFIER_SHIFT _("Shift+")
#define KEY_MODIFIER_CTRL _("Ctrl+")
#define KEY_MODIFIER_ALT _("Alt+")
#define KEY_MODIFIER_SUPER _("Super+")
#define KEY_MODIFIER_HYPER _("Hyper+")
#define KEY_MODIFIER_META _("Meta+")
#define KEY_CHOOSER_NONE _("<None>")

typedef struct _RemminaKeyChooserArguments
{
	guint keyval;
	guint state;
	gboolean use_modifiers;
	gint response;
} RemminaKeyChooserArguments;

G_BEGIN_DECLS

/* Show a key chooser dialog and return the keyval for the selected key */
RemminaKeyChooserArguments* remmina_key_chooser_new(GtkWindow *parent_window, gboolean use_modifiers);
/* Get the uppercase character value of a keyval */
gchar* remmina_key_chooser_get_value(guint keyval, guint state);
/* Get the keyval of a (lowercase) character value */
guint remmina_key_chooser_get_keyval(const gchar *value);

G_END_DECLS

#endif  /* __REMMINAKEYCHOOSER_H__  */

