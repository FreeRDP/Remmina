/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
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

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include "config.h"
#include "remmina_mpchange.h"
#include "remmina_file.h"
#include "remmina_file_manager.h"
#include "remmina_pref.h"
#include "remmina_public.h"
#include "remmina_main.h"
#include "remmina_plugin_manager.h"
#include "remmina_log.h"
#include "remmina/remmina_trace_calls.h"

#define GET_DIALOG_OBJECT(object_name) gtk_builder_get_object(bu, object_name)

struct mpchanger_params {
	gchar *username;        // New username
	gchar *domain;          // New domain
	gchar *password;        // New password
	gchar *group;

	GtkEntry *eGroup, *eUsername, *eDomain;
	GtkEntry *ePassword1, *ePassword2;
	GtkListStore* store;
	GtkDialog* dialog;
	GtkTreeView* table;
	GtkButton* btnDoChange;
	GtkLabel* statusLabel;

	GtkTreeIter iter;
	int changed_passwords_count;
	guint sid;
	guint searchentrychange_timeout_source_id;
};

enum {
	COL_F = 0,
	COL_NAME,
	COL_GROUP,
	COL_USERNAME,
	COL_FILENAME,
	NUM_COLS
};

static gboolean remmina_mpchange_fieldcompare(const gchar *needle, const gchar *haystack, int *matchcount)
{
	TRACE_CALL(__func__);

	if (needle[0] == 0) {
		(*matchcount)++;
		return TRUE;
	}

	if (strcasecmp(needle, haystack) != 0)
		return FALSE;

	(*matchcount)++;
	return TRUE;

}

static void remmina_mpchange_file_list_callback(RemminaFile *remminafile, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkListStore* store;
	GtkTreeIter iter;
	int matchcount;
	const gchar *username, *domain, *group;

	gchar* s;
	struct mpchanger_params* mpcp;

	mpcp = (struct mpchanger_params*)user_data;
	store = GTK_LIST_STORE(mpcp->store);


	username = remmina_file_get_string(remminafile, "username");
	domain = remmina_file_get_string(remminafile, "domain");
	group = remmina_file_get_string(remminafile, "group");

	if (username == NULL)
		username = "";

	if (domain == NULL)
		domain = "";

	if (group == NULL)
		group = "";

	matchcount = 0;
	if (!remmina_mpchange_fieldcompare(mpcp->username, username, &matchcount))
		return;
	if (!remmina_mpchange_fieldcompare(mpcp->domain, domain, &matchcount))
		return;
	if (!remmina_mpchange_fieldcompare(mpcp->group, group, &matchcount))
		return;

	gtk_list_store_append(store, &iter);
	s = g_strdup_printf("%s\\%s", domain, username);

	gtk_list_store_set(store, &iter,
		COL_F, matchcount >= 3 ? TRUE : FALSE,
		COL_NAME, remmina_file_get_string(remminafile, "name"),
		COL_GROUP, group,
		COL_USERNAME,   s,
		COL_FILENAME, remminafile->filename,
		-1);
	g_free(s);

}

static void remmina_mpchange_checkbox_toggle(GtkCellRendererToggle *cell, gchar *path_string, gpointer user_data)
{
	TRACE_CALL(__func__);
	GtkTreeIter iter;
	struct mpchanger_params* mpcp = (struct mpchanger_params*)user_data;
	GtkTreePath *path;

	gboolean a = gtk_cell_renderer_toggle_get_active(cell);
	path = gtk_tree_path_new_from_string(path_string);
	gtk_tree_model_get_iter(GTK_TREE_MODEL(mpcp->store), &iter, path);
	gtk_tree_path_free(path);
	gtk_list_store_set(mpcp->store, &iter, COL_F, !a, -1);
}

static void remmina_mpchange_dochange(gchar* fname, struct mpchanger_params* mpcp)
{
	TRACE_CALL(__func__);

	RemminaFile* remminafile;

	remminafile = remmina_file_load(fname);
	if (remminafile) {
		remmina_file_store_secret_plugin_password(remminafile, "password", mpcp->password);
		remmina_file_free(remminafile);
		mpcp->changed_passwords_count++;
	}

}

static void enable_inputs(struct mpchanger_params* mpcp, gboolean ena)
{
	gtk_widget_set_sensitive(GTK_WIDGET(mpcp->eGroup), ena);
	gtk_widget_set_sensitive(GTK_WIDGET(mpcp->eUsername), ena);
	gtk_widget_set_sensitive(GTK_WIDGET(mpcp->eDomain), ena);
	gtk_widget_set_sensitive(GTK_WIDGET(mpcp->ePassword1), ena);
	gtk_widget_set_sensitive(GTK_WIDGET(mpcp->ePassword2), ena);
	gtk_widget_set_sensitive(GTK_WIDGET(mpcp->btnDoChange), ena);
	gtk_widget_set_sensitive(GTK_WIDGET(mpcp->table), ena);
}

static gboolean changenext(gpointer user_data)
{
	TRACE_CALL(__func__);
	struct mpchanger_params* mpcp = (struct mpchanger_params*)user_data;
	gchar* fname;
	gboolean sel;

	gtk_tree_model_get(GTK_TREE_MODEL(mpcp->store), &mpcp->iter, COL_F, &sel, -1);
	gtk_tree_model_get(GTK_TREE_MODEL(mpcp->store), &mpcp->iter, COL_FILENAME, &fname, -1);
	if (sel) {
		remmina_mpchange_dochange(fname, mpcp);
	}
	g_free(fname);

	if (gtk_tree_model_iter_next(GTK_TREE_MODEL(mpcp->store), &mpcp->iter)) {
		return G_SOURCE_CONTINUE;
	}else  {
		gtk_dialog_response(mpcp->dialog, 1);
		mpcp->sid = 0;
		return G_SOURCE_REMOVE;
	}
}

static void remmina_mpchange_dochange_clicked(GtkButton *btn, gpointer user_data)
{
	TRACE_CALL(__func__);
	struct mpchanger_params* mpcp = (struct mpchanger_params*)user_data;
	const gchar *passwd1, *passwd2;

	if (mpcp->searchentrychange_timeout_source_id) {
		g_source_remove(mpcp->searchentrychange_timeout_source_id);
		mpcp->searchentrychange_timeout_source_id = 0;
	}

	if (!gtk_tree_model_get_iter_first(GTK_TREE_MODEL(mpcp->store), &mpcp->iter))
		return;

	passwd1 = gtk_entry_get_text(mpcp->ePassword1);
	passwd2 = gtk_entry_get_text(mpcp->ePassword2);

	if (g_strcmp0(passwd1, passwd2) != 0) {
		GtkWidget *msgDialog;
		msgDialog = gtk_message_dialog_new(GTK_WINDOW(mpcp->dialog),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_ERROR,
			GTK_BUTTONS_CLOSE,
			_("The passwords do not match"));
		gtk_dialog_run(GTK_DIALOG(msgDialog));
		gtk_widget_destroy(msgDialog);
		return;
	}

	g_free(mpcp->password);
	mpcp->password = g_strdup(passwd1);
	mpcp->changed_passwords_count = 0;

	gtk_label_set_text(mpcp->statusLabel, _("Resetting passwords, please wait…"));

	enable_inputs(mpcp, FALSE);
	mpcp->sid = g_idle_add(changenext, (gpointer)mpcp);

}

static gboolean remmina_mpchange_searchfield_changed_to(gpointer user_data)
{
	TRACE_CALL(__func__);
	struct mpchanger_params *mpcp = (struct mpchanger_params *)user_data;
	const gchar *s;

	if (mpcp->searchentrychange_timeout_source_id) {
		g_source_remove(mpcp->searchentrychange_timeout_source_id);
		mpcp->searchentrychange_timeout_source_id = 0;
	}

	s = gtk_entry_get_text(mpcp->eGroup);
	g_free(mpcp->group);
	mpcp->group = g_strdup(s);

	s = gtk_entry_get_text(mpcp->eDomain);
	g_free(mpcp->domain);
	mpcp->domain = g_strdup(s);

	s = gtk_entry_get_text(mpcp->eUsername);
	g_free(mpcp->username);
	mpcp->username = g_strdup(s);

	if (mpcp->store != NULL) {
		gtk_tree_view_set_model(mpcp->table, NULL);
	}
	mpcp->store = gtk_list_store_new(NUM_COLS, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	if (mpcp->group[0] != 0 || mpcp->domain[0] != 0 || mpcp->username[0] != 0)
		remmina_file_manager_iterate((GFunc)remmina_mpchange_file_list_callback, (gpointer)mpcp);

	gtk_tree_view_set_model(mpcp->table, GTK_TREE_MODEL(mpcp->store));

	return G_SOURCE_CONTINUE;       // Source already remove at the beginning

}

static void remmina_mpchange_searchfield_changed(GtkSearchEntry *se, gpointer user_data)
{
	TRACE_CALL(__func__);
	struct mpchanger_params *mpcp = (struct mpchanger_params *)user_data;

	if (mpcp->searchentrychange_timeout_source_id) {
		g_source_remove(mpcp->searchentrychange_timeout_source_id);
		mpcp->searchentrychange_timeout_source_id = 0;
	}

	mpcp->searchentrychange_timeout_source_id = g_timeout_add(500, remmina_mpchange_searchfield_changed_to, user_data);
}


static void remmina_mpchange_stopsearch(GtkSearchEntry *entry, gpointer user_data)
{
	TRACE_CALL(__func__);
	/* The stop-search signal is emitted when pressing Esc on a GtkSearchEntry. We end the dialog. */
	struct mpchanger_params *mpcp = (struct mpchanger_params *)user_data;
	gtk_dialog_response(mpcp->dialog, 1);
}

static gboolean remmina_file_multipasswd_changer_mt(gpointer d)
{
	TRACE_CALL(__func__);
	struct mpchanger_params *mpcp = d;
	GtkBuilder* bu;
	GtkDialog* dialog;
	GtkWindow* mainwindow;
	GtkCellRendererToggle *toggle;
	RemminaSecretPlugin *secret_plugin;
	char *initerror;

	mainwindow = remmina_main_get_window();

	/* The multiple password changer works only when a secrecy plugin is available */
	initerror = NULL;
	secret_plugin = remmina_plugin_manager_get_secret_plugin();
	if (secret_plugin == NULL) {
		initerror = _("The multi password changer requires a secrecy plugin.\n");
	}else {
		if (!secret_plugin->is_service_available()) {
			initerror = _("The multi password changer requires a secrecy service.\n");
		}
	}
	if (initerror) {
		GtkWidget *msgDialog;
		msgDialog = gtk_message_dialog_new(mainwindow, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING, GTK_BUTTONS_OK,
			"%s", initerror);
		gtk_dialog_run(GTK_DIALOG(msgDialog));
		gtk_widget_destroy(msgDialog);
		return FALSE;
	}


	bu = remmina_public_gtk_builder_new_from_file("remmina_mpc.glade");
	if (!bu) {
		REMMINA_DEBUG("Unable to load the multiple password changer glade file interface\n");
		return FALSE;
	}

	dialog = GTK_DIALOG(gtk_builder_get_object(bu, "MPCDialog"));
	mpcp->dialog = dialog;
	if (mainwindow)
		gtk_window_set_transient_for(GTK_WINDOW(dialog), mainwindow);


	mpcp->eGroup = GTK_ENTRY(GET_DIALOG_OBJECT("groupEntry"));
	gtk_entry_set_text(mpcp->eGroup, mpcp->group);
	g_signal_connect(G_OBJECT(mpcp->eGroup), "changed", G_CALLBACK(remmina_mpchange_searchfield_changed), (gpointer)mpcp);
	g_signal_connect(G_OBJECT(mpcp->eGroup), "stop-search", G_CALLBACK(remmina_mpchange_stopsearch), (gpointer)mpcp);

	mpcp->eUsername = GTK_ENTRY(GET_DIALOG_OBJECT("usernameEntry"));
	gtk_entry_set_text(mpcp->eUsername, mpcp->username);
	g_signal_connect(G_OBJECT(mpcp->eUsername), "changed", G_CALLBACK(remmina_mpchange_searchfield_changed), (gpointer)mpcp);

	mpcp->eDomain = GTK_ENTRY(GET_DIALOG_OBJECT("domainEntry"));
	gtk_entry_set_text(mpcp->eDomain, mpcp->domain);
	g_signal_connect(G_OBJECT(mpcp->eDomain), "changed", G_CALLBACK(remmina_mpchange_searchfield_changed), (gpointer)mpcp);

	mpcp->ePassword1 = GTK_ENTRY(GET_DIALOG_OBJECT("password1Entry"));
	gtk_entry_set_text(mpcp->ePassword1, mpcp->password);

	mpcp->ePassword2 = GTK_ENTRY(GET_DIALOG_OBJECT("password2Entry"));
	gtk_entry_set_text(mpcp->ePassword2, mpcp->password);

	mpcp->statusLabel = GTK_LABEL(GET_DIALOG_OBJECT("statusLabel"));


	mpcp->store = NULL;

	mpcp->table = GTK_TREE_VIEW(GET_DIALOG_OBJECT("profchangelist"));

	/* Fire a fake searchfield changed, so a new list store is created */
	remmina_mpchange_searchfield_changed(NULL, (gpointer)mpcp);

	toggle = GTK_CELL_RENDERER_TOGGLE(GET_DIALOG_OBJECT("cellrenderertoggle1"));
	g_signal_connect(G_OBJECT(toggle), "toggled", G_CALLBACK(remmina_mpchange_checkbox_toggle), (gpointer)mpcp);

	mpcp->btnDoChange = GTK_BUTTON(GET_DIALOG_OBJECT("btnDoChange"));
	g_signal_connect(mpcp->btnDoChange, "clicked", G_CALLBACK(remmina_mpchange_dochange_clicked), (gpointer)mpcp);

	gtk_dialog_run(dialog);
	gtk_widget_destroy(GTK_WIDGET(dialog));

	if (mpcp->sid) {
		g_source_remove(mpcp->sid);
		mpcp->sid = 0;
	}

	if (mpcp->searchentrychange_timeout_source_id) {
		g_source_remove(mpcp->searchentrychange_timeout_source_id);
		mpcp->searchentrychange_timeout_source_id = 0;
	}

	if (mpcp->changed_passwords_count) {
		GtkWidget *msgDialog;
		msgDialog = gtk_message_dialog_new(GTK_WINDOW(mpcp->dialog),
			GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_OK,
			ngettext("%d password changed.", "%d passwords changed.", mpcp->changed_passwords_count), mpcp->changed_passwords_count);
		gtk_dialog_run(GTK_DIALOG(msgDialog));
		gtk_widget_destroy(msgDialog);
	}

	// Free data
	g_free(mpcp->username);
	g_free(mpcp->password);
	g_free(mpcp->domain);
	g_free(mpcp->group);
	g_free(mpcp);
	return FALSE;
}


void
remmina_mpchange_schedule(gboolean has_domain, const gchar *group, const gchar *domain, const gchar *username, const gchar *password)
{
	// We could also be called in a subthread after a successful connection
	// (not currently implemented)
	// So we just schedule the multipassword changer to be executed on
	// the main thread

	TRACE_CALL(__func__);
	struct mpchanger_params *mpcp;

	mpcp = g_malloc0(sizeof(struct mpchanger_params));
	mpcp->username = g_strdup(username);
	mpcp->password = g_strdup(password);
	mpcp->domain = g_strdup(domain);
	mpcp->group = g_strdup(group);
	gdk_threads_add_idle(remmina_file_multipasswd_changer_mt, (gpointer)mpcp);

}

