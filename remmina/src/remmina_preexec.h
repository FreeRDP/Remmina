/*  See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINAPREEXEC_H__
#define __REMMINAPREEXEC_H__

G_BEGIN_DECLS

typedef struct {
	GtkDialog *dialog;
	GtkLabel *label_pleasewait;
	GtkButton *button_cancel;
	GtkWidget *spinner;
} PCon_Spinner;

GtkDialog* remmina_preexec_new(RemminaFile* remminafile);

G_END_DECLS

#endif  /* __REMMINAPREEXEC_H__  */
