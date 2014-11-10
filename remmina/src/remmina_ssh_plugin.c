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

#include "config.h"

#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <vte/vte.h>
#include "remmina_public.h"
#include "remmina_plugin_manager.h"
#include "remmina_ssh.h"
#include "remmina_protocol_widget.h"
#include "remmina_pref.h"
#include "remmina_ssh_plugin.h"

#define REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY  1
#define REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE 2

/***** The SSH plugin implementation *****/
typedef struct _RemminaPluginSshData
{
	RemminaSSHShell *shell;
	GtkWidget *vte;
	pthread_t thread;
}RemminaPluginSshData;

static RemminaPluginService *remmina_plugin_service = NULL;

static gpointer
remmina_plugin_ssh_main_thread (gpointer data)
{
	RemminaProtocolWidget *gp = (RemminaProtocolWidget*) data;
	RemminaPluginSshData *gpdata;
	RemminaFile *remminafile;
	RemminaSSH *ssh;
	RemminaSSHShell *shell = NULL;
	gboolean cont = FALSE;
	gint ret;
	gchar *charset;

	pthread_setcancelstate (PTHREAD_CANCEL_ENABLE, NULL);
	CANCEL_ASYNC

	gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT(gp), "plugin-data");

	ssh = g_object_get_data (G_OBJECT(gp), "user-data");
	if (ssh)
	{
		/* Create SSH Shell connection based on existing SSH session */
		shell = remmina_ssh_shell_new_from_ssh (ssh);
		if (remmina_ssh_init_session (REMMINA_SSH (shell)) &&
				remmina_ssh_auth (REMMINA_SSH (shell), NULL) > 0 &&
				remmina_ssh_shell_open (shell, (RemminaSSHExitFunc) remmina_plugin_service->protocol_plugin_close_connection, gp))
		{
			cont = TRUE;
		}
	}
	else
	{
		/* New SSH Shell connection */
		remminafile = remmina_plugin_service->protocol_plugin_get_file (gp);
		remmina_plugin_service->file_set_string (remminafile, "ssh_server",
				remmina_plugin_service->file_get_string (remminafile, "server"));

		shell = remmina_ssh_shell_new_from_file (remminafile);
		while (1)
		{
			if (!remmina_ssh_init_session (REMMINA_SSH (shell)))
			{
				remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
				break;
			}

			ret = remmina_ssh_auth_gui (REMMINA_SSH (shell),
					REMMINA_INIT_DIALOG (remmina_protocol_widget_get_init_dialog (gp)), TRUE);
			if (ret == 0)
			{
				remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
			}
			if (ret <= 0) break;

			if (!remmina_ssh_shell_open (shell, (RemminaSSHExitFunc) remmina_plugin_service->protocol_plugin_close_connection, gp))
			{
				remmina_plugin_service->protocol_plugin_set_error (gp, "%s", REMMINA_SSH (shell)->error);
				break;
			}

			cont = TRUE;
			break;
		}
	}
	if (!cont)
	{
		if (shell) remmina_ssh_shell_free (shell);
		IDLE_ADD ((GSourceFunc) remmina_plugin_service->protocol_plugin_close_connection, gp);
		return NULL;
	}

	gpdata->shell = shell;

	charset = REMMINA_SSH (shell)->charset;

	THREADS_ENTER
	if (charset && charset[0] != '\0')
	{
#if !VTE_CHECK_VERSION(0,38,0)
		vte_terminal_set_encoding (VTE_TERMINAL (gpdata->vte), charset);
#else
		vte_terminal_set_encoding (VTE_TERMINAL (gpdata->vte), charset, NULL);
#endif
	}
#if !VTE_CHECK_VERSION(0,38,0)
	vte_terminal_set_pty (VTE_TERMINAL (gpdata->vte), shell->slave);
#else
	vte_terminal_set_pty (VTE_TERMINAL (gpdata->vte),
                          vte_pty_new_foreign_sync (shell->slave, NULL, NULL));
#endif
	THREADS_LEAVE

	remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");

	gpdata->thread = 0;
	return NULL;
}

static gboolean
remmina_plugin_ssh_on_focus_in (GtkWidget *widget, GdkEventFocus *event, RemminaProtocolWidget *gp)
{
	RemminaPluginSshData *gpdata;

	gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT(gp), "plugin-data");
	gtk_widget_grab_focus (gpdata->vte);
	return TRUE;
}

static gboolean
remmina_plugin_ssh_on_size_allocate (GtkWidget *widget, GtkAllocation *alloc, RemminaProtocolWidget *gp)
{
	RemminaPluginSshData *gpdata;
	gint cols, rows;

	if (!gtk_widget_get_mapped (widget)) return FALSE;

	gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT(gp), "plugin-data");

	cols = vte_terminal_get_column_count (VTE_TERMINAL (widget));
	rows = vte_terminal_get_row_count (VTE_TERMINAL (widget));

	remmina_ssh_shell_set_size (gpdata->shell, cols, rows);

	return FALSE;
}

static void
remmina_plugin_ssh_set_vte_pref (RemminaProtocolWidget *gp)
{
	RemminaPluginSshData *gpdata;

	gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT(gp), "plugin-data");
	if (remmina_pref.vte_font && remmina_pref.vte_font[0])
	{
#if !VTE_CHECK_VERSION(0,38,0)
		vte_terminal_set_font_from_string (VTE_TERMINAL (gpdata->vte), remmina_pref.vte_font);
#else
		vte_terminal_set_font (VTE_TERMINAL (gpdata->vte),
							   pango_font_description_from_string (remmina_pref.vte_font));
#endif
	}
	vte_terminal_set_allow_bold (VTE_TERMINAL (gpdata->vte), remmina_pref.vte_allow_bold_text);
	if (remmina_pref.vte_lines > 0)
	{
		vte_terminal_set_scrollback_lines (VTE_TERMINAL (gpdata->vte), remmina_pref.vte_lines);
	}
}

static void
remmina_plugin_ssh_init (RemminaProtocolWidget *gp)
{
	RemminaPluginSshData *gpdata;
	GtkWidget *hbox;
	GtkWidget *vscrollbar;
	GtkWidget *vte;

	gpdata = g_new0 (RemminaPluginSshData, 1);
	g_object_set_data_full (G_OBJECT(gp), "plugin-data", gpdata, g_free);

#if GTK_VERSION == 3
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
#elif GTK_VERSION == 2
	hbox = gtk_hbox_new (FALSE, 0);
#endif
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER (gp), hbox);
	g_signal_connect(G_OBJECT(hbox), "focus-in-event", G_CALLBACK(remmina_plugin_ssh_on_focus_in), gp);

	vte = vte_terminal_new ();
	gtk_widget_show(vte);
	vte_terminal_set_size (VTE_TERMINAL (vte), 80, 25);
	vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (vte), TRUE);
	gtk_box_pack_start (GTK_BOX (hbox), vte, TRUE, TRUE, 0);
	gpdata->vte = vte;
	remmina_plugin_ssh_set_vte_pref (gp);
	g_signal_connect(G_OBJECT(vte), "size-allocate", G_CALLBACK(remmina_plugin_ssh_on_size_allocate), gp);

	remmina_plugin_service->protocol_plugin_register_hostkey (gp, vte);

#if GTK_VERSION == 3
#if VTE_CHECK_VERSION(0, 38, 0)
	vscrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (vte)));
#else
	vscrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, vte_terminal_get_adjustment (VTE_TERMINAL (vte)));
#endif
#elif GTK_VERSION == 2
#if VTE_CHECK_VERSION(0, 38, 0)
	vscrollbar = gtk_vscrollbar_new (gtk_scrollable_get_vadjustment (GTK_SCROLLABLE_TERMINAL (vte)));
#else
	vscrollbar = gtk_vscrollbar_new (vte_terminal_get_adjustment (VTE_TERMINAL (vte)));
#endif
#endif
	gtk_widget_show(vscrollbar);
	gtk_box_pack_start (GTK_BOX (hbox), vscrollbar, FALSE, TRUE, 0);
}

static gboolean
remmina_plugin_ssh_open_connection (RemminaProtocolWidget *gp)
{
	RemminaPluginSshData *gpdata;

	gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT(gp), "plugin-data");

	remmina_plugin_service->protocol_plugin_set_expand (gp, TRUE);
	remmina_plugin_service->protocol_plugin_set_width (gp, 640);
	remmina_plugin_service->protocol_plugin_set_height (gp, 480);

	if (pthread_create (&gpdata->thread, NULL, remmina_plugin_ssh_main_thread, gp))
	{
		remmina_plugin_service->protocol_plugin_set_error (gp,
				"Failed to initialize pthread. Falling back to non-thread mode...");
		gpdata->thread = 0;
		return FALSE;
	}
	else
	{
		return TRUE;
	}
	return TRUE;
}

static gboolean
remmina_plugin_ssh_close_connection (RemminaProtocolWidget *gp)
{
	RemminaPluginSshData *gpdata;

	gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT(gp), "plugin-data");
	if (gpdata->thread)
	{
		pthread_cancel (gpdata->thread);
		if (gpdata->thread) pthread_join (gpdata->thread, NULL);
	}
	if (gpdata->shell)
	{
		remmina_ssh_shell_free (gpdata->shell);
		gpdata->shell = NULL;
	}

	remmina_plugin_service->protocol_plugin_emit_signal (gp, "disconnect");
	return FALSE;
}

static gboolean
remmina_plugin_ssh_query_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	return TRUE;
}

static void
remmina_plugin_ssh_call_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	RemminaPluginSshData *gpdata;

	gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT(gp), "plugin-data");
	switch (feature->id)
	{
		case REMMINA_PROTOCOL_FEATURE_TOOL_SSH:
		remmina_plugin_service->open_connection (
				remmina_file_dup_temp_protocol (remmina_plugin_service->protocol_plugin_get_file (gp), "SSH"),
				NULL, gpdata->shell, NULL);
		return;
		case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
		remmina_plugin_service->open_connection (
				remmina_file_dup_temp_protocol (remmina_plugin_service->protocol_plugin_get_file (gp), "SFTP"),
				NULL, gpdata->shell, NULL);
		return;
		case REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY:
		vte_terminal_copy_clipboard (VTE_TERMINAL (gpdata->vte));
		return;
		case REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE:
		vte_terminal_paste_clipboard (VTE_TERMINAL (gpdata->vte));
		return;
	}
}

static RemminaProtocolFeature remmina_plugin_ssh_features[] =
{
	{	REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY, N_("Copy"), GTK_STOCK_COPY, NULL},
	{	REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE, N_("Paste"), GTK_STOCK_PASTE, NULL},
	{	REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL}
};

static RemminaProtocolPlugin remmina_plugin_ssh =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,
	"SSH",
	N_("SSH - Secure Shell"),
	GETTEXT_PACKAGE,
	VERSION,

	"utilities-terminal",
	"utilities-terminal",
	NULL,
	NULL,
	REMMINA_PROTOCOL_SSH_SETTING_SSH,
	remmina_plugin_ssh_features,

	remmina_plugin_ssh_init,
	remmina_plugin_ssh_open_connection,
	remmina_plugin_ssh_close_connection,
	remmina_plugin_ssh_query_feature,
	remmina_plugin_ssh_call_feature
};

void
remmina_ssh_plugin_register (void)
{
	remmina_plugin_ssh_features[0].opt3 = GUINT_TO_POINTER (remmina_pref.vte_shortcutkey_copy);
	remmina_plugin_ssh_features[1].opt3 = GUINT_TO_POINTER (remmina_pref.vte_shortcutkey_paste);

	remmina_plugin_service = &remmina_plugin_manager_service;
	remmina_plugin_service->register_plugin ((RemminaPlugin *) &remmina_plugin_ssh);

	ssh_threads_set_callbacks(ssh_threads_get_pthread());
	ssh_init();

}

#else

void remmina_ssh_plugin_register(void)
{
}

#endif

