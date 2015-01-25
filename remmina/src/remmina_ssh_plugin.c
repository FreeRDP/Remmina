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
#include "remmina_masterthread_exec.h"
#include "remmina/remmina_trace_calls.h"

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
	TRACE_CALL("remmina_plugin_ssh_main_thread");
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
					REMMINA_INIT_DIALOG (remmina_protocol_widget_get_init_dialog (gp)));
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
	remmina_plugin_ssh_vte_terminal_set_encoding_and_pty(VTE_TERMINAL (gpdata->vte), charset, shell->slave);
	remmina_plugin_service->protocol_plugin_emit_signal (gp, "connect");

	gpdata->thread = 0;
	return NULL;
}

void remmina_plugin_ssh_vte_terminal_set_encoding_and_pty(VteTerminal *terminal, const char *codeset, int slave)
{
	TRACE_CALL("remmina_plugin_ssh_vte_terminal_set_encoding_and_pty");
	if ( !remmina_masterthread_exec_is_main_thread() ) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData*)g_malloc( sizeof(RemminaMTExecData) );
		d->func = FUNC_VTE_TERMINAL_SET_ENCODING_AND_PTY;
		d->p.vte_terminal_set_encoding_and_pty.terminal = terminal;
		d->p.vte_terminal_set_encoding_and_pty.codeset = codeset;
		d->p.vte_terminal_set_encoding_and_pty.slave = slave;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}

	if (codeset && codeset[0] != '\0')
	{
#if !VTE_CHECK_VERSION(0,38,0)
		vte_terminal_set_encoding (terminal, codeset);
#else
		vte_terminal_set_encoding (terminal, codeset, NULL);
#endif
	}

#if !VTE_CHECK_VERSION(0,38,0)
	vte_terminal_set_pty (terminal, slave);
#else
	vte_terminal_set_pty (terminal, vte_pty_new_foreign_sync(slave, NULL, NULL));
#endif

}


static gboolean
remmina_plugin_ssh_on_focus_in (GtkWidget *widget, GdkEventFocus *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_on_focus_in");
	RemminaPluginSshData *gpdata;

	gpdata = (RemminaPluginSshData*) g_object_get_data (G_OBJECT(gp), "plugin-data");
	gtk_widget_grab_focus (gpdata->vte);
	return TRUE;
}

static gboolean
remmina_plugin_ssh_on_size_allocate (GtkWidget *widget, GtkAllocation *alloc, RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_on_size_allocate");
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
	TRACE_CALL("remmina_plugin_ssh_set_vte_pref");
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
	TRACE_CALL("remmina_plugin_ssh_init");
	RemminaPluginSshData *gpdata;
	GtkWidget *hbox;
	GtkWidget *vscrollbar;
	GtkWidget *vte;
	GtkStyleContext *style_context;
	GdkRGBA foreground_color;
	GdkRGBA background_color;
#if !VTE_CHECK_VERSION(0,38,0)
	GdkColor foreground_gdkcolor;
	GdkColor background_gdkcolor;
#endif

	gpdata = g_new0 (RemminaPluginSshData, 1);
	g_object_set_data_full (G_OBJECT(gp), "plugin-data", gpdata, g_free);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER (gp), hbox);
	g_signal_connect(G_OBJECT(hbox), "focus-in-event", G_CALLBACK(remmina_plugin_ssh_on_focus_in), gp);

	vte = vte_terminal_new ();
	gtk_widget_show(vte);
	vte_terminal_set_size (VTE_TERMINAL (vte), 80, 25);
	vte_terminal_set_scroll_on_keystroke (VTE_TERMINAL (vte), TRUE);
	if (remmina_pref.vte_system_colors)
	{
		/* Get default system theme colors */
		style_context = gtk_widget_get_style_context(GTK_WIDGET (vte));
		gtk_style_context_get_color(style_context, GTK_STATE_FLAG_NORMAL, &foreground_color);
		gtk_style_context_get_background_color(style_context, GTK_STATE_FLAG_NORMAL, &background_color);
	}
	else
	{
		/* Get customized colors */
		gdk_rgba_parse(&foreground_color, remmina_pref.vte_foreground_color);
		gdk_rgba_parse(&background_color, remmina_pref.vte_background_color);
	}
#if !VTE_CHECK_VERSION(0,38,0)
	/* VTE <= 2.90 doesn't support GdkRGBA so we must convert GdkRGBA to GdkColor */
	foreground_gdkcolor.red = (guint16)(foreground_color.red * 0xFFFF);
	foreground_gdkcolor.green = (guint16)(foreground_color.green * 0xFFFF);
	foreground_gdkcolor.blue = (guint16)(foreground_color.blue * 0xFFFF);
	background_gdkcolor.red = (guint16)(background_color.red * 0xFFFF);
	background_gdkcolor.green = (guint16)(background_color.green * 0xFFFF);
	background_gdkcolor.blue = (guint16)(background_color.blue * 0xFFFF);
	/* Set colors to GdkColor */
	vte_terminal_set_colors (VTE_TERMINAL(vte), &foreground_gdkcolor, &background_gdkcolor, NULL, 0);
#else
	/* Set colors to GdkRGBA */
	vte_terminal_set_colors (VTE_TERMINAL(vte), &foreground_color, &background_color, NULL, 0);
#endif

	gtk_box_pack_start (GTK_BOX (hbox), vte, TRUE, TRUE, 0);
	gpdata->vte = vte;
	remmina_plugin_ssh_set_vte_pref (gp);
	g_signal_connect(G_OBJECT(vte), "size-allocate", G_CALLBACK(remmina_plugin_ssh_on_size_allocate), gp);

	remmina_plugin_service->protocol_plugin_register_hostkey (gp, vte);


	vscrollbar = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, gtk_scrollable_get_vadjustment (GTK_SCROLLABLE (vte)));

	gtk_widget_show(vscrollbar);
	gtk_box_pack_start (GTK_BOX (hbox), vscrollbar, FALSE, TRUE, 0);
}

static gboolean
remmina_plugin_ssh_open_connection (RemminaProtocolWidget *gp)
{
	TRACE_CALL("remmina_plugin_ssh_open_connection");
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
	TRACE_CALL("remmina_plugin_ssh_close_connection");
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
	TRACE_CALL("remmina_plugin_ssh_query_feature");
	return TRUE;
}

static void
remmina_plugin_ssh_call_feature (RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL("remmina_plugin_ssh_call_feature");
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

/* Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END. */
static RemminaProtocolFeature remmina_plugin_ssh_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY, N_("Copy"), N_("_Copy"), NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE, N_("Paste"), N_("_Paste"), NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END, 0, NULL, NULL, NULL }
};

/* Protocol plugin definition and features */
static RemminaProtocolPlugin remmina_plugin_ssh =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                 // Type
	"SSH",                                        // Name
	N_("SSH - Secure Shell"),                     // Description
	GETTEXT_PACKAGE,                              // Translation domain
	VERSION,                                      // Version number
	"utilities-terminal",                         // Icon for normal connection
	"utilities-terminal",                         // Icon for SSH connection
	NULL,                                         // Array for basic settings
	NULL,                                         // Array for advanced settings
	REMMINA_PROTOCOL_SSH_SETTING_SSH,             // SSH settings type
	remmina_plugin_ssh_features,                  // Array for available features
	remmina_plugin_ssh_init,                      // Plugin initialization
	remmina_plugin_ssh_open_connection,           // Plugin open connection
	remmina_plugin_ssh_close_connection,          // Plugin close connection
	remmina_plugin_ssh_query_feature,             // Query for available features
	remmina_plugin_ssh_call_feature               // Call a feature
};

void
remmina_ssh_plugin_register (void)
{
	TRACE_CALL("remmina_ssh_plugin_register");
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
	TRACE_CALL("remmina_ssh_plugin_register");
}

#endif

