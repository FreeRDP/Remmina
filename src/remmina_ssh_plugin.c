/**
 * Remmina - The GTK+ Remote Desktop Client - SSH plugin.
 *
 * @copyright Copyright (C) 2010-2011 Vic Lee.
 * @copyright Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo.
 * @copyright Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo.
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

#include "config.h"
#include "remmina/remmina_trace_calls.h"

#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <gio/gio.h>
#include <vte/vte.h>
#include <locale.h>
#include <langinfo.h>
#include "remmina_public.h"
#include "remmina_plugin_manager.h"
#include "remmina_ssh.h"
#include "remmina_protocol_widget.h"
#include "remmina_pref.h"
#include "remmina_ssh_plugin.h"
#include "remmina_masterthread_exec.h"

#define REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY  1
#define REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE 2
#define REMMINA_PLUGIN_SSH_FEATURE_TOOL_SELECT_ALL 3

#define GET_PLUGIN_DATA(gp) (RemminaPluginSshData *)g_object_get_data(G_OBJECT(gp), "plugin-data");

/** Palette colors taken from sakura */
#define PALETTE_SIZE 16

enum color_schemes { LINUX, TANGO, GRUVBOX, SOLARIZED_DARK, SOLARIZED_LIGHT, XTERM, CUSTOM };

/** 16 color palettes in GdkRGBA format (red, green, blue, alpha).
 * Text displayed in the first 8 colors (0-7) is meek (uses thin strokes).
 * Text displayed in the second 8 colors (8-15) is bold (uses thick strokes).
 **/
const GdkRGBA linux_palette[PALETTE_SIZE] = {
	{ 0,	    0,	      0,	1 },
	{ 0.666667, 0,	      0,	1 },
	{ 0,	    0.666667, 0,	1 },
	{ 0.666667, 0.333333, 0,	1 },
	{ 0,	    0,	      0.666667, 1 },
	{ 0.666667, 0,	      0.666667, 1 },
	{ 0,	    0.666667, 0.666667, 1 },
	{ 0.666667, 0.666667, 0.666667, 1 },
	{ 0.333333, 0.333333, 0.333333, 1 },
	{ 1,	    0.333333, 0.333333, 1 },
	{ 0.333333, 1,	      0.333333, 1 },
	{ 1,	    1,	      0.333333, 1 },
	{ 0.333333, 0.333333, 1,	1 },
	{ 1,	    0.333333, 1,	1 },
	{ 0.333333, 1,	      1,	1 },
	{ 1,	    1,	      1,	1 }
};

const GdkRGBA tango_palette[PALETTE_SIZE] = {
	{ 0,	     0,	       0,	 1 },
	{ 0.8,	     0,	       0,	 1 },
	{ 0.305882,  0.603922, 0.023529, 1 },
	{ 0.768627,  0.627451, 0,	 1 },
	{ 0.203922,  0.396078, 0.643137, 1 },
	{ 0.458824,  0.313725, 0.482353, 1 },
	{ 0.0235294, 0.596078, 0.603922, 1 },
	{ 0.827451,  0.843137, 0.811765, 1 },
	{ 0.333333,  0.341176, 0.32549,	 1 },
	{ 0.937255,  0.160784, 0.160784, 1 },
	{ 0.541176,  0.886275, 0.203922, 1 },
	{ 0.988235,  0.913725, 0.309804, 1 },
	{ 0.447059,  0.623529, 0.811765, 1 },
	{ 0.678431,  0.498039, 0.658824, 1 },
	{ 0.203922,  0.886275, 0.886275, 1 },
	{ 0.933333,  0.933333, 0.92549,	 1 }
};

const GdkRGBA gruvbox_palette[PALETTE_SIZE] = {
	{ 0.156863, 0.156863, 0.156863, 1.000000 },
	{ 0.800000, 0.141176, 0.113725, 1.000000 },
	{ 0.596078, 0.592157, 0.101961, 1.000000 },
	{ 0.843137, 0.600000, 0.129412, 1.000000 },
	{ 0.270588, 0.521569, 0.533333, 1.000000 },
	{ 0.694118, 0.384314, 0.525490, 1.000000 },
	{ 0.407843, 0.615686, 0.415686, 1.000000 },
	{ 0.658824, 0.600000, 0.517647, 1.000000 },
	{ 0.572549, 0.513725, 0.454902, 1.000000 },
	{ 0.984314, 0.286275, 0.203922, 1.000000 },
	{ 0.721569, 0.733333, 0.149020, 1.000000 },
	{ 0.980392, 0.741176, 0.184314, 1.000000 },
	{ 0.513725, 0.647059, 0.596078, 1.000000 },
	{ 0.827451, 0.525490, 0.607843, 1.000000 },
	{ 0.556863, 0.752941, 0.486275, 1.000000 },
	{ 0.921569, 0.858824, 0.698039, 1.000000 },
};

const GdkRGBA solarized_dark_palette[PALETTE_SIZE] = {
	{ 0.027451, 0.211765, 0.258824, 1 },
	{ 0.862745, 0.196078, 0.184314, 1 },
	{ 0.521569, 0.600000, 0.000000, 1 },
	{ 0.709804, 0.537255, 0.000000, 1 },
	{ 0.149020, 0.545098, 0.823529, 1 },
	{ 0.827451, 0.211765, 0.509804, 1 },
	{ 0.164706, 0.631373, 0.596078, 1 },
	{ 0.933333, 0.909804, 0.835294, 1 },
	{ 0.000000, 0.168627, 0.211765, 1 },
	{ 0.796078, 0.294118, 0.086275, 1 },
	{ 0.345098, 0.431373, 0.458824, 1 },
	{ 0.396078, 0.482353, 0.513725, 1 },
	{ 0.513725, 0.580392, 0.588235, 1 },
	{ 0.423529, 0.443137, 0.768627, 1 },
	{ 0.576471, 0.631373, 0.631373, 1 },
	{ 0.992157, 0.964706, 0.890196, 1 }
};

const GdkRGBA solarized_light_palette[PALETTE_SIZE] = {
	{ 0.933333, 0.909804, 0.835294, 1 },
	{ 0.862745, 0.196078, 0.184314, 1 },
	{ 0.521569, 0.600000, 0.000000, 1 },
	{ 0.709804, 0.537255, 0.000000, 1 },
	{ 0.149020, 0.545098, 0.823529, 1 },
	{ 0.827451, 0.211765, 0.509804, 1 },
	{ 0.164706, 0.631373, 0.596078, 1 },
	{ 0.027451, 0.211765, 0.258824, 1 },
	{ 0.992157, 0.964706, 0.890196, 1 },
	{ 0.796078, 0.294118, 0.086275, 1 },
	{ 0.576471, 0.631373, 0.631373, 1 },
	{ 0.513725, 0.580392, 0.588235, 1 },
	{ 0.396078, 0.482353, 0.513725, 1 },
	{ 0.423529, 0.443137, 0.768627, 1 },
	{ 0.345098, 0.431373, 0.458824, 1 },
	{ 0.000000, 0.168627, 0.211765, 1 }
};

const GdkRGBA xterm_palette[PALETTE_SIZE] = {
	{ 0,	    0,	      0,	1 },
	{ 0.803922, 0,	      0,	1 },
	{ 0,	    0.803922, 0,	1 },
	{ 0.803922, 0.803922, 0,	1 },
	{ 0.117647, 0.564706, 1,	1 },
	{ 0.803922, 0,	      0.803922, 1 },
	{ 0,	    0.803922, 0.803922, 1 },
	{ 0.898039, 0.898039, 0.898039, 1 },
	{ 0.298039, 0.298039, 0.298039, 1 },
	{ 1,	    0,	      0,	1 },
	{ 0,	    1,	      0,	1 },
	{ 1,	    1,	      0,	1 },
	{ 0.27451,  0.509804, 0.705882, 1 },
	{ 1,	    0,	      1,	1 },
	{ 0,	    1,	      1,	1 },
	{ 1,	    1,	      1,	1 }
};

#if VTE_CHECK_VERSION(0, 38, 0)
static struct {
	const GdkRGBA *palette;
} remminavte;
#endif

#define DEFAULT_PALETTE "linux_palette"


/** The SSH plugin implementation */
typedef struct _RemminaPluginSshData {
	RemminaSSHShell *	shell;
	GFile *			vte_session_file;
	GtkWidget *		vte;

	const GdkRGBA *		palette;

	pthread_t		thread;
} RemminaPluginSshData;


static RemminaPluginService *remmina_plugin_service = NULL;

static gboolean
remmina_plugin_ssh_on_size_allocate(GtkWidget *widget, GtkAllocation *alloc, RemminaProtocolWidget *gp);


/**
 * Remmina protocol plugin main function.
 *
 * First it starts the SSH tunnel if needed and then the SSH connection.
 *
 */
static gpointer
remmina_plugin_ssh_main_thread(gpointer data)
{
	TRACE_CALL(__func__);
	RemminaProtocolWidget *gp = (RemminaProtocolWidget *)data;
	RemminaPluginSshData *gpdata;
	RemminaFile *remminafile;
	RemminaSSH *ssh;
	RemminaSSHShell *shell = NULL;
	gboolean cont = FALSE;
	gchar *hostport;
	gchar *tunnel_host;
	int tunnel_port;
	gint ret;

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
	CANCEL_ASYNC

	gpdata = GET_PLUGIN_DATA(gp);

	/**
	 * remmina_plugin_service->protocol_plugin_start_direct_tunnel start the
	 * SSH Tunnel and return the server + port string
	 *
	 **/
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	// Optionally start the SSH tunnel
	hostport = remmina_plugin_service->protocol_plugin_start_direct_tunnel(gp, 22, FALSE);
	if (hostport == NULL) {
		remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
		return NULL;
	}

	remmina_plugin_service->get_server_port(hostport, 22, &tunnel_host, &tunnel_port);
	g_free(hostport);

	ssh = g_object_get_data(G_OBJECT(gp), "user-data");
	if (ssh) {
		/* Create SSH shell connection based on existing SSH session */

		shell = remmina_ssh_shell_new_from_ssh(ssh);
		if (shell->ssh.tunnel_host)
			g_free(shell->ssh.tunnel_host);
		shell->ssh.tunnel_host = tunnel_host;
		shell->ssh.tunnel_port = tunnel_port;
		if (remmina_ssh_init_session(REMMINA_SSH(shell), FALSE) &&
		    remmina_ssh_auth(REMMINA_SSH(shell), NULL, gp, remminafile) > 0 &&
		    remmina_ssh_shell_open(shell, (RemminaSSHExitFunc)
					   remmina_plugin_service->protocol_plugin_signal_connection_closed, gp))
			cont = TRUE;
	} else {
		/* New SSH Shell connection */
		shell = remmina_ssh_shell_new_from_file(remminafile);
		shell->ssh.tunnel_host = tunnel_host;
		shell->ssh.tunnel_port = tunnel_port;
		while (1) {

			if (!remmina_ssh_init_session(REMMINA_SSH(shell), FALSE)) {
				g_debug("[SSH] init session error: %s\n",REMMINA_SSH(shell)->error);
				remmina_plugin_service->protocol_plugin_set_error(gp, "%s", REMMINA_SSH(shell)->error);
				break;
			}

			ret = remmina_ssh_auth_gui(REMMINA_SSH(shell), gp, remminafile);
			if (ret == 0)
				remmina_plugin_service->protocol_plugin_set_error(gp, "%s", REMMINA_SSH(shell)->error);
			if (ret <= 0) break;

			if (!remmina_ssh_shell_open(shell, (RemminaSSHExitFunc)
						    remmina_plugin_service->protocol_plugin_signal_connection_closed, gp)) {
				remmina_plugin_service->protocol_plugin_set_error(gp, "%s", REMMINA_SSH(shell)->error);
				break;
			}

			cont = TRUE;
			break;
		}

	}
	if (!cont) {
		if (shell) remmina_ssh_shell_free(shell);
		remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
		return NULL;
	}

	gpdata->shell = shell;

	gchar *charset = REMMINA_SSH(shell)->charset;
	remmina_plugin_ssh_vte_terminal_set_encoding_and_pty(VTE_TERMINAL(gpdata->vte), charset, shell->master, shell->slave);

	/* TODO: The following call should be moved on the main thread, or something weird could happen */
	remmina_plugin_ssh_on_size_allocate(GTK_WIDGET(gpdata->vte), NULL, gp);

	remmina_plugin_service->protocol_plugin_signal_connection_opened(gp);

	gpdata->thread = 0;
	return NULL;
}

void remmina_plugin_ssh_vte_terminal_set_encoding_and_pty(VteTerminal *terminal, const char *codeset, int master, int slave)
{
	TRACE_CALL(__func__);
	if (!remmina_masterthread_exec_is_main_thread()) {
		/* Allow the execution of this function from a non main thread */
		RemminaMTExecData *d;
		d = (RemminaMTExecData *)g_malloc(sizeof(RemminaMTExecData));
		d->func = FUNC_VTE_TERMINAL_SET_ENCODING_AND_PTY;
		d->p.vte_terminal_set_encoding_and_pty.terminal = terminal;
		d->p.vte_terminal_set_encoding_and_pty.codeset = codeset;
		d->p.vte_terminal_set_encoding_and_pty.master = master;
		remmina_masterthread_exec_and_wait(d);
		g_free(d);
		return;
	}

	setlocale(LC_ALL, "");
	if (codeset && codeset[0] != '\0') {
#if VTE_CHECK_VERSION(0, 38, 0)
		vte_terminal_set_encoding(terminal, codeset, NULL);
#else
		vte_terminal_set_emulation(terminal, "xterm");
		vte_terminal_set_encoding(terminal, codeset);
#endif
	}

	vte_terminal_set_backspace_binding(terminal, VTE_ERASE_ASCII_DELETE);
	vte_terminal_set_delete_binding(terminal, VTE_ERASE_DELETE_SEQUENCE);

#if VTE_CHECK_VERSION(0, 38, 0)
	/* vte_pty_new_foreig expect master FD, see https://bugzilla.gnome.org/show_bug.cgi?id=765382 */
	vte_terminal_set_pty(terminal, vte_pty_new_foreign_sync(master, NULL, NULL));
#else
	vte_terminal_set_pty(terminal, master);
#endif
}

static gboolean
remmina_plugin_ssh_on_focus_in(GtkWidget *widget, GdkEventFocus *event, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	gtk_widget_grab_focus(gpdata->vte);
	return TRUE;
}

static gboolean
remmina_plugin_ssh_on_size_allocate(GtkWidget *widget, GtkAllocation *alloc, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);
	gint cols, rows;

	if (!gtk_widget_get_mapped(widget)) return FALSE;

	cols = vte_terminal_get_column_count(VTE_TERMINAL(widget));
	rows = vte_terminal_get_row_count(VTE_TERMINAL(widget));

	if (gpdata->shell)
		remmina_ssh_shell_set_size(gpdata->shell, cols, rows);

	return FALSE;
}

static void
remmina_plugin_ssh_set_vte_pref(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);
	RemminaFile *remminafile;
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (remmina_plugin_service->file_get_int(remminafile, "audiblebell", FALSE)) {
		vte_terminal_set_audible_bell(VTE_TERMINAL(gpdata->vte), TRUE);
		g_info("audible_bell set to %i", vte_terminal_get_audible_bell(VTE_TERMINAL(gpdata->vte)));
	}
	if (remmina_pref.vte_font && remmina_pref.vte_font[0]) {
#if !VTE_CHECK_VERSION(0, 38, 0)
		vte_terminal_set_font_from_string(VTE_TERMINAL(gpdata->vte), remmina_pref.vte_font);
#else
		vte_terminal_set_font(VTE_TERMINAL(gpdata->vte),
				      pango_font_description_from_string(remmina_pref.vte_font));
#endif
	}
	vte_terminal_set_allow_bold(VTE_TERMINAL(gpdata->vte), remmina_pref.vte_allow_bold_text);
	if (remmina_pref.vte_lines > 0)
		vte_terminal_set_scrollback_lines(VTE_TERMINAL(gpdata->vte), remmina_pref.vte_lines);
}

void
remmina_plugin_ssh_vte_select_all(GtkMenuItem *menuitem, gpointer vte)
{
	TRACE_CALL(__func__);
	vte_terminal_select_all(VTE_TERMINAL(vte));
	/** @todo we should add the vte_terminal_unselect_all as well */
}

void
remmina_plugin_ssh_vte_copy_clipboard(GtkMenuItem *menuitem, gpointer vte)
{
	TRACE_CALL(__func__);
#if VTE_CHECK_VERSION(0, 50, 0)
	vte_terminal_copy_clipboard_format(VTE_TERMINAL(vte), VTE_FORMAT_TEXT);
#else
	vte_terminal_copy_clipboard(VTE_TERMINAL(vte));
#endif
}

void
remmina_plugin_ssh_vte_paste_clipboard(GtkMenuItem *menuitem, gpointer vte)
{
	TRACE_CALL(__func__);
	vte_terminal_paste_clipboard(VTE_TERMINAL(vte));
}

void
remmina_plugin_ssh_vte_save_session(GtkMenuItem *menuitem, RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	GtkWidget *widget;
	GError *err = NULL;

	GFileOutputStream *stream = g_file_replace(gpdata->vte_session_file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &err);

	if (err != NULL) {
		// TRANSLATORS: %s is a placeholder for an error message
		widget = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
						_("Error: %s"), err->message);
		g_signal_connect(G_OBJECT(widget), "response", G_CALLBACK(gtk_widget_destroy), NULL);
		gtk_widget_show(widget);
		return;
	}

	if (stream != NULL)
#if VTE_CHECK_VERSION(0, 38, 0)
		vte_terminal_write_contents_sync(VTE_TERMINAL(gpdata->vte), G_OUTPUT_STREAM(stream),
						 VTE_WRITE_DEFAULT, NULL, &err);
#else
		vte_terminal_write_contents(VTE_TERMINAL(gpdata->vte), G_OUTPUT_STREAM(stream),
					    VTE_TERMINAL_WRITE_DEFAULT, NULL, &err);
#endif

	if (err == NULL) {
		remmina_public_send_notification("remmina-terminal-saved",
						 _("Terminal content saved in"),
						 g_file_get_path(gpdata->vte_session_file));
	}

	g_object_unref(stream);
	g_free(err);
}

/** Send a keystroke to the plugin window */
static void remmina_ssh_keystroke(RemminaProtocolWidget *gp, const guint keystrokes[], const gint keylen)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);
	remmina_plugin_service->protocol_plugin_send_keys_signals(gpdata->vte,
								  keystrokes, keylen, GDK_KEY_PRESS | GDK_KEY_RELEASE);
	return;
}

gboolean
remmina_ssh_plugin_popup_menu(GtkWidget *widget, GdkEvent *event, GtkWidget *menu)
{
	if ((event->type == GDK_BUTTON_PRESS) && (((GdkEventButton *)event)->button == 3)) {
#if GTK_CHECK_VERSION(3, 22, 0)
		gtk_menu_popup_at_pointer(GTK_MENU(menu), NULL);
#else
		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
			       ((GdkEventButton *)event)->button, gtk_get_current_event_time());
#endif
		return TRUE;
	}

	return FALSE;
}

/**
 * Remmina SSH plugin terminal popup menu.
 *
 * This is the context menu that popup when you right click in a terminal window.
 * You can than select, copy, paste text and save the whole buffer to a file.
 * Each menu entry call back the following functions:
 * - remmina_plugin_ssh_vte_select_all()
 * - remmina_plugin_ssh_vte_copy_clipboard()
 * - remmina_plugin_ssh_vte_paste_clipboard()
 * - remmina_plugin_ssh_vte_save_session()
 * .
 *
 */
void remmina_plugin_ssh_popup_ui(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);
	/* Context menu for slection and clipboard */
	GtkWidget *menu = gtk_menu_new();

	GtkWidget *select_all = gtk_menu_item_new_with_label(_("Select All (Host + A)"));
	GtkWidget *copy = gtk_menu_item_new_with_label(_("Copy (host + C)"));
	GtkWidget *paste = gtk_menu_item_new_with_label(_("Paste (host + V)"));
	GtkWidget *save = gtk_menu_item_new_with_label(_("Save session to file"));

	gtk_menu_shell_append(GTK_MENU_SHELL(menu), select_all);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), copy);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), paste);
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), save);

	g_signal_connect(G_OBJECT(gpdata->vte), "button_press_event",
			 G_CALLBACK(remmina_ssh_plugin_popup_menu), menu);

	g_signal_connect(G_OBJECT(select_all), "activate",
			 G_CALLBACK(remmina_plugin_ssh_vte_select_all), gpdata->vte);
	g_signal_connect(G_OBJECT(copy), "activate",
			 G_CALLBACK(remmina_plugin_ssh_vte_copy_clipboard), gpdata->vte);
	g_signal_connect(G_OBJECT(paste), "activate",
			 G_CALLBACK(remmina_plugin_ssh_vte_paste_clipboard), gpdata->vte);
	g_signal_connect(G_OBJECT(save), "activate",
			 G_CALLBACK(remmina_plugin_ssh_vte_save_session), gp);

	gtk_widget_show_all(menu);
}

/**
 * Remmina SSH plugin initialization.
 *
 * This is the main function used to create the widget that will be embedded in the
 * Remmina Connection Window.
 * Initialize the terminal colours based on the user, everything is needed for the
 * terminal window, the terminal session logging and the terminal popup menu.
 *
 * @see remmina_plugin_ssh_popup_ui
 * @see RemminaProtocolWidget
 * @see https://gitlab.com/Remmina/Remmina/wikis/Remmina-SSH-Terminal-colour-schemes
 */
static void
remmina_plugin_ssh_init(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata;
	RemminaFile *remminafile;
	GtkWidget *hbox;
	GtkAdjustment *vadjustment;
	GtkWidget *vscrollbar;
	GtkWidget *vte;
	GdkRGBA foreground_color;
	GdkRGBA background_color;


#if !VTE_CHECK_VERSION(0, 38, 0)
	GdkColor foreground_gdkcolor;
	GdkColor background_gdkcolor;
#endif  /* VTE_CHECK_VERSION(0,38,0) */

	gpdata = g_new0(RemminaPluginSshData, 1);
	g_object_set_data_full(G_OBJECT(gp), "plugin-data", gpdata, g_free);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_show(hbox);
	gtk_container_add(GTK_CONTAINER(gp), hbox);
	g_signal_connect(G_OBJECT(hbox), "focus-in-event", G_CALLBACK(remmina_plugin_ssh_on_focus_in), gp);

	vte = vte_terminal_new();
	gtk_widget_show(vte);
	vte_terminal_set_size(VTE_TERMINAL(vte), 80, 25);
	vte_terminal_set_scroll_on_keystroke(VTE_TERMINAL(vte), TRUE);
#if !VTE_CHECK_VERSION(0, 38, 0)
	gdk_rgba_parse(&foreground_color, remmina_pref.color_pref.foreground);
	gdk_rgba_parse(&background_color, remmina_pref.color_pref.background);
#endif
	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

#if VTE_CHECK_VERSION(0, 38, 0)
	GdkRGBA cp[PALETTE_SIZE];
	GdkRGBA cursor_color;

	/*
	 * custom colors reside inside of the 'theme' subdir of the remmina config folder (.config/remmina/theme)
	 * with the file extension '.colors'. The name of the colorfile came from the menu (see below)
	 * sideeffect: it is possible to overwrite the standard colors with a dedicated colorfile like
	 * '0.colors' for GRUVBOX, '1.colors' for TANGO and so on
	 */
	const gchar *color_name = remmina_plugin_service->file_get_string(remminafile, "ssh_color_scheme");

	gchar *remmina_dir = g_build_path("/", g_get_user_config_dir(), "remmina", "theme", NULL);
	gchar *remmina_colors_file = g_strdup_printf("%s/%s.colors", remmina_dir, color_name);
	g_free(remmina_dir);

	/*
	 * try to load theme from one of the system data dirs (based on XDG_DATA_DIRS environment var)
	 */
	if (!g_file_test(remmina_colors_file, G_FILE_TEST_IS_REGULAR)) {
		GError *error = NULL;
		const gchar *const *dirs = g_get_system_data_dirs();

		unsigned int i = 0;
		for (i = 0; dirs[i] != NULL; ++i) {
			remmina_dir = g_build_path("/", dirs[i], "remmina", "theme", NULL);
			GDir *system_data_dir = g_dir_open(remmina_dir, 0, &error);
			// ignoring this error is OK, because the folder may not exist
			if (error) {
				g_error_free(error);
				error = NULL;
			} else {
				if (system_data_dir) {
					g_dir_close(system_data_dir);
					g_free(remmina_colors_file);
					remmina_colors_file = g_strdup_printf("%s/%s.colors", remmina_dir, color_name);
					if (g_file_test(remmina_colors_file, G_FILE_TEST_IS_REGULAR))
						break;
				}
			}
			g_free(remmina_dir);
		}
	}

	if (g_file_test(remmina_colors_file, G_FILE_TEST_IS_REGULAR)) {
		GKeyFile *gkeyfile;
		RemminaColorPref color_pref;

		gkeyfile = g_key_file_new();
		g_key_file_load_from_file(gkeyfile, remmina_colors_file, G_KEY_FILE_NONE, NULL);
		remmina_pref_file_load_colors(gkeyfile, &color_pref);

		gdk_rgba_parse(&foreground_color, color_pref.foreground);
		gdk_rgba_parse(&background_color, color_pref.background);
		gdk_rgba_parse(&cursor_color, color_pref.cursor);

		gdk_rgba_parse(&cp[0], color_pref.color0);
		gdk_rgba_parse(&cp[1], color_pref.color1);
		gdk_rgba_parse(&cp[2], color_pref.color2);
		gdk_rgba_parse(&cp[3], color_pref.color3);
		gdk_rgba_parse(&cp[4], color_pref.color4);
		gdk_rgba_parse(&cp[5], color_pref.color5);
		gdk_rgba_parse(&cp[6], color_pref.color6);
		gdk_rgba_parse(&cp[7], color_pref.color7);
		gdk_rgba_parse(&cp[8], color_pref.color8);
		gdk_rgba_parse(&cp[9], color_pref.color9);
		gdk_rgba_parse(&cp[10], color_pref.color10);
		gdk_rgba_parse(&cp[11], color_pref.color11);
		gdk_rgba_parse(&cp[12], color_pref.color12);
		gdk_rgba_parse(&cp[13], color_pref.color13);
		gdk_rgba_parse(&cp[14], color_pref.color14);
		gdk_rgba_parse(&cp[15], color_pref.color15);

		const GdkRGBA custom_palette[PALETTE_SIZE] = {
			cp[0],	cp[1],	cp[2],	cp[3],
			cp[4],	cp[5],	cp[6],	cp[7],
			cp[8],	cp[9],	cp[10], cp[11],
			cp[12], cp[13], cp[14], cp[15]
		};

		remminavte.palette = custom_palette;
	} else {
		/* Set colors to GdkRGBA */
		switch (remmina_plugin_service->file_get_int(remminafile, "ssh_color_scheme", FALSE)) {
		case LINUX:
			gdk_rgba_parse(&foreground_color, "#ffffff");
			gdk_rgba_parse(&background_color, "#000000");
			gdk_rgba_parse(&cursor_color, "#ffffff");
			remminavte.palette = linux_palette;
			break;
		case TANGO:
			gdk_rgba_parse(&foreground_color, "#eeeeec");
			gdk_rgba_parse(&background_color, "#2e3436");
			gdk_rgba_parse(&cursor_color, "#8ae234");
			remminavte.palette = tango_palette;
			break;
		case GRUVBOX:
			gdk_rgba_parse(&foreground_color, "#ebdbb2");
			gdk_rgba_parse(&background_color, "#282828");
			gdk_rgba_parse(&cursor_color, "#d3869b");
			remminavte.palette = gruvbox_palette;
			break;
		case SOLARIZED_DARK:
			gdk_rgba_parse(&foreground_color, "#839496");
			gdk_rgba_parse(&background_color, "#002b36");
			gdk_rgba_parse(&cursor_color, "#93a1a1");
			remminavte.palette = solarized_dark_palette;
			break;
		case SOLARIZED_LIGHT:
			gdk_rgba_parse(&foreground_color, "#657b83");
			gdk_rgba_parse(&background_color, "#fdf6e3");
			gdk_rgba_parse(&cursor_color, "#586e75");
			remminavte.palette = solarized_light_palette;
			break;
		case XTERM:
			gdk_rgba_parse(&foreground_color, "#000000");
			gdk_rgba_parse(&background_color, "#ffffff");
			gdk_rgba_parse(&cursor_color, "#000000");
			remminavte.palette = xterm_palette;
			break;
		case CUSTOM:
			gdk_rgba_parse(&foreground_color, remmina_pref.color_pref.foreground);
			gdk_rgba_parse(&background_color, remmina_pref.color_pref.background);
			gdk_rgba_parse(&cursor_color, remmina_pref.color_pref.cursor);

			gdk_rgba_parse(&cp[0], remmina_pref.color_pref.color0);
			gdk_rgba_parse(&cp[1], remmina_pref.color_pref.color1);
			gdk_rgba_parse(&cp[2], remmina_pref.color_pref.color2);
			gdk_rgba_parse(&cp[3], remmina_pref.color_pref.color3);
			gdk_rgba_parse(&cp[4], remmina_pref.color_pref.color4);
			gdk_rgba_parse(&cp[5], remmina_pref.color_pref.color5);
			gdk_rgba_parse(&cp[6], remmina_pref.color_pref.color6);
			gdk_rgba_parse(&cp[7], remmina_pref.color_pref.color7);
			gdk_rgba_parse(&cp[8], remmina_pref.color_pref.color8);
			gdk_rgba_parse(&cp[9], remmina_pref.color_pref.color9);
			gdk_rgba_parse(&cp[10], remmina_pref.color_pref.color10);
			gdk_rgba_parse(&cp[11], remmina_pref.color_pref.color11);
			gdk_rgba_parse(&cp[12], remmina_pref.color_pref.color12);
			gdk_rgba_parse(&cp[13], remmina_pref.color_pref.color13);
			gdk_rgba_parse(&cp[14], remmina_pref.color_pref.color14);
			gdk_rgba_parse(&cp[15], remmina_pref.color_pref.color15);

			const GdkRGBA custom_palette[PALETTE_SIZE] = {
				cp[0],	cp[1],	cp[2],	cp[3],
				cp[4],	cp[5],	cp[6],	cp[7],
				cp[8],	cp[9],	cp[10], cp[11],
				cp[12], cp[13], cp[14], cp[15]
			};

			remminavte.palette = custom_palette;
			break;
		default:
			remminavte.palette = linux_palette;
			break;
		}
	}
	g_free(remmina_colors_file);
	vte_terminal_set_colors(VTE_TERMINAL(vte), &foreground_color, &background_color, remminavte.palette, PALETTE_SIZE);
	vte_terminal_set_color_foreground(VTE_TERMINAL(vte), &foreground_color);
	vte_terminal_set_color_background(VTE_TERMINAL(vte), &background_color);
	vte_terminal_set_color_cursor(VTE_TERMINAL(vte), &cursor_color);
#else
	/* VTE <= 2.90 doesn’t support GdkRGBA so we must convert GdkRGBA to GdkColor */
	foreground_gdkcolor.red = (guint16)(foreground_color.red * 0xFFFF);
	foreground_gdkcolor.green = (guint16)(foreground_color.green * 0xFFFF);
	foreground_gdkcolor.blue = (guint16)(foreground_color.blue * 0xFFFF);
	background_gdkcolor.red = (guint16)(background_color.red * 0xFFFF);
	background_gdkcolor.green = (guint16)(background_color.green * 0xFFFF);
	background_gdkcolor.blue = (guint16)(background_color.blue * 0xFFFF);
	/* Set colors to GdkColor */
	vte_terminal_set_colors(VTE_TERMINAL(vte), &foreground_gdkcolor, &background_gdkcolor, NULL, 0);
#endif

	gtk_box_pack_start(GTK_BOX(hbox), vte, TRUE, TRUE, 0);
	gpdata->vte = vte;
	remmina_plugin_ssh_set_vte_pref(gp);
	g_signal_connect(G_OBJECT(vte), "size-allocate", G_CALLBACK(remmina_plugin_ssh_on_size_allocate), gp);

	remmina_plugin_service->protocol_plugin_register_hostkey(gp, vte);

#if VTE_CHECK_VERSION(0, 28, 0)
	vadjustment = gtk_scrollable_get_vadjustment(GTK_SCROLLABLE(vte));
#else
	vadjustment = vte_terminal_get_adjustment(VTE_TERMINAL(vc->vte.terminal));
#endif

	vscrollbar = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, vadjustment);

	gtk_widget_show(vscrollbar);
	gtk_box_pack_start(GTK_BOX(hbox), vscrollbar, FALSE, TRUE, 0);

	const gchar *dir;
	const gchar *sshlogname;
	const gchar *fp;
	GFile *rf;

	rf = g_file_new_for_path(remminafile->filename);

	if (remmina_plugin_service->file_get_string(remminafile, "sshlogfolder") == NULL)
		dir = g_build_path("/", g_get_user_cache_dir(), "remmina", NULL);
	else
		dir = remmina_plugin_service->file_get_string(remminafile, "sshlogfolder");

	if (remmina_plugin_service->file_get_string(remminafile, "sshlogname") == NULL)
		sshlogname = g_strconcat(g_file_get_basename(rf), ".", "log", NULL);
	else
		sshlogname = remmina_plugin_service->file_get_string(remminafile, "sshlogname");

	fp = g_strconcat(dir, "/", sshlogname, NULL);
	gpdata->vte_session_file = g_file_new_for_path(fp);

	remmina_plugin_ssh_popup_ui(gp);
}

/**
 * Initialize the the main window properties and the pthread.
 *
 * The call of this function is a requirement of remmina_protocol_widget_open_connection_real().
 * @return TRUE
 * @return FALSE and remmina_protocol_widget_open_connection_real() will fails.
 */
static gboolean
remmina_plugin_ssh_open_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	remmina_plugin_service->protocol_plugin_set_expand(gp, TRUE);
	remmina_plugin_service->protocol_plugin_set_width(gp, 640);
	remmina_plugin_service->protocol_plugin_set_height(gp, 480);

	if (pthread_create(&gpdata->thread, NULL, remmina_plugin_ssh_main_thread, gp)) {
		remmina_plugin_service->protocol_plugin_set_error(gp,
			"Failed to initialize pthread. Falling back to non-thread mode…");
		gpdata->thread = 0;
		return FALSE;
	} else {
		return TRUE;
	}
	return TRUE;
}

static gboolean
remmina_plugin_ssh_close_connection(RemminaProtocolWidget *gp)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	RemminaFile *remminafile;

	remminafile = remmina_plugin_service->protocol_plugin_get_file(gp);

	if (remmina_file_get_int(remminafile, "sshlogenabled", FALSE))
		remmina_plugin_ssh_vte_save_session(NULL, gp);
	if (gpdata->thread) {
		pthread_cancel(gpdata->thread);
		if (gpdata->thread) pthread_join(gpdata->thread, NULL);
	}
	if (gpdata->shell) {
		remmina_ssh_shell_free(gpdata->shell);
		gpdata->shell = NULL;
	}

	remmina_plugin_service->protocol_plugin_signal_connection_closed(gp);
	return FALSE;
}

/**
 * Not used by the the plugin.
 *
 * @return Always TRUE
 */
static gboolean
remmina_plugin_ssh_query_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	return TRUE;
}

/**
 * Functions to call when an entry in the Tool menu in the Remmina Connection Window is clicked.
 *
 * In the Remmina Connection Window toolbar, there is a tool menu, this function is used to
 * call the right function for each entry with its parameters.
 *
 * At the moment it’s possible to:
 * - Open a new SSH session.
 * - Open an SFTP session.
 * - Select, copy and paste text.
 * - Send recorded Key Strokes.
 * .
 *
 * @return the return value of the calling function.
 *
 */
static void
remmina_plugin_ssh_call_feature(RemminaProtocolWidget *gp, const RemminaProtocolFeature *feature)
{
	TRACE_CALL(__func__);
	RemminaPluginSshData *gpdata = GET_PLUGIN_DATA(gp);

	switch (feature->id) {
	case REMMINA_PROTOCOL_FEATURE_TOOL_SSH:
		remmina_plugin_service->open_connection(
			remmina_file_dup_temp_protocol(remmina_plugin_service->protocol_plugin_get_file(gp), "SSH"),
			NULL, gpdata->shell, NULL);
		return;
	case REMMINA_PROTOCOL_FEATURE_TOOL_SFTP:
		remmina_plugin_service->open_connection(
			/** @todo start the direct tunnel here */
			remmina_file_dup_temp_protocol(remmina_plugin_service->protocol_plugin_get_file(gp), "SFTP"),
			NULL, gpdata->shell, NULL);
		return;
	case REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY:
#if VTE_CHECK_VERSION(0, 50, 0)
		vte_terminal_copy_clipboard_format(VTE_TERMINAL(gpdata->vte), VTE_FORMAT_TEXT);
#else
		vte_terminal_copy_clipboard(VTE_TERMINAL(gpdata->vte));
#endif
		return;
	case REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE:
		vte_terminal_paste_clipboard(VTE_TERMINAL(gpdata->vte));
		return;
	case REMMINA_PLUGIN_SSH_FEATURE_TOOL_SELECT_ALL:
		vte_terminal_select_all(VTE_TERMINAL(gpdata->vte));
		return;
	}
}

/** Array of key/value pairs for SSH auth type*/
static gpointer ssh_auth[] =
{
	"0", N_("Password"),
	"1", N_("SSH identity file"),
	"2", N_("SSH agent"),
	"3", N_("Public key (automatic)"),
	"4", N_("Kerberos (GSSAPI)"),
	NULL
};

/** Charset list */
static gpointer ssh_charset_list[] =
{
	"", "",
	"", "ASCII",
	"", "BIG5",
	"", "CP437",
	"", "CP720",
	"", "CP737",
	"", "CP775",
	"", "CP850",
	"", "CP852",
	"", "CP855",
	"", "CP857",
	"", "CP858",
	"", "CP862",
	"", "CP866",
	"", "CP874",
	"", "CP1125",
	"", "CP1250",
	"", "CP1251",
	"", "CP1252",
	"", "CP1253",
	"", "CP1254",
	"", "CP1255",
	"", "CP1256",
	"", "CP1257",
	"", "CP1258",
	"", "EUC-JP",
	"", "EUC-KR",
	"", "GBK",
	"", "ISO-8859-1",
	"", "ISO-8859-2",
	"", "ISO-8859-3",
	"", "ISO-8859-4",
	"", "ISO-8859-5",
	"", "ISO-8859-6",
	"", "ISO-8859-7",
	"", "ISO-8859-8",
	"", "ISO-8859-9",
	"", "ISO-8859-10",
	"", "ISO-8859-11",
	"", "ISO-8859-12",
	"", "ISO-8859-13",
	"", "ISO-8859-14",
	"", "ISO-8859-15",
	"", "ISO-8859-16",
	"", "KOI8-R",
	"", "SJIS",
	"", "UTF-8",
	NULL
};

static gpointer ssh_terminal_palette[] =
{
	"0",  "Linux",
	"1",  "Tango",
	"2",  "Gruvbox",
	"3",  "Solarized Dark",
	"4",  "Solarized Light",
	"5",  "XTerm",
	"6",  "Custom (Configured in Remmina preferences)",
	NULL, NULL
};

/**
 * Array for available features.
 * The last element of the array must be REMMINA_PROTOCOL_FEATURE_TYPE_END.
 */
static RemminaProtocolFeature remmina_plugin_ssh_features[] =
{
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_COPY,	  N_("Copy"),	    N_("_Copy"),       NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_PASTE,	  N_("Paste"),	    N_("_Paste"),      NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_TOOL, REMMINA_PLUGIN_SSH_FEATURE_TOOL_SELECT_ALL, N_("Select all"), N_("_Select all"), NULL },
	{ REMMINA_PROTOCOL_FEATURE_TYPE_END,  0,					  NULL,		    NULL,	       NULL }
};

/**
 * Array of RemminaProtocolSetting for basic settings.
 * - Each item is composed by:
 *  1. RemminaProtocolSettingType for setting type.
 *  2. Setting name.
 *  3. Setting description.
 *  4. Compact disposition.
 *  5. Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO.
 *  6. Setting Tooltip.
 * .
 */
static const RemminaProtocolSetting remmina_ssh_basic_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SERVER,	  "server",	    NULL,			  FALSE, "_ssh._tcp", NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "username",   N_("Username"),		  FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "password",   N_("User password"),	  FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT,	  "ssh_auth",	    N_("Authentication type"),	  FALSE, ssh_auth,    NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_FILE,	  "ssh_privatekey", N_("Identity file"),	  FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_PASSWORD, "ssh_passphrase", N_("Password to unlock private key"), FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	  "exec",	    N_("Startup program"),	  FALSE, NULL,	      NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	  NULL,		    NULL,			  FALSE, NULL,	      NULL }
};

/**
 * Array of RemminaProtocolSetting for advanced settings.
 * - Each item is composed by:
 *  1. RemminaProtocolSettingType for setting type.
 *  2. Setting name.
 *  3. Setting description.
 *  4. Compact disposition.
 *  5. Values for REMMINA_PROTOCOL_SETTING_TYPE_SELECT or REMMINA_PROTOCOL_SETTING_TYPE_COMBO.
 *  6. Setting Tooltip.
 *
 */
static const RemminaProtocolSetting remmina_ssh_advanced_settings[] =
{
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "ssh_color_scheme",	  N_("Terminal color scheme"),		    FALSE, ssh_terminal_palette, NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_SELECT, "ssh_charset",		  N_("Character set"),			    FALSE, ssh_charset_list,	 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"ssh_proxycommand",	  N_("SSH Proxy Command"),		    FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"ssh_kex_algorithms",	  N_("KEX (Key Exchange) algorithms"),	    FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"ssh_ciphers",		  N_("Symmetric cipher client to server"),  FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"ssh_hostkeytypes",	  N_("Preferred server host key types"),    FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_FOLDER, "sshlogfolder",		  N_("Folder for SSH session log"),		    FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_TEXT,	"sshlogname",		  N_("Filename for SSH session log"),	    FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"sshlogenabled",	  N_("Log SSH session when exiting Remmina"), FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"audiblebell",		  N_("Audible terminal bell"),	    FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"ssh_compression",	  N_("SSH compression"),		    FALSE, NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"disablepasswordstoring", N_("Don't remember passwords"),	    TRUE,  NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_CHECK,	"ssh_stricthostkeycheck", N_("Strict host key checking"),	    TRUE,  NULL,		 NULL },
	{ REMMINA_PROTOCOL_SETTING_TYPE_END,	NULL,			  NULL,					    FALSE, NULL,		 NULL }
};

/**
 * SSH Protocol plugin definition and features.
 *
 * Array used to define the SSH Protocol plugin Type, name, description, version
 * Plugin icon, features, initialization and closing functions.
 */
static RemminaProtocolPlugin remmina_plugin_ssh =
{
	REMMINA_PLUGIN_TYPE_PROTOCOL,                   /**< Type */
	"SSH",                                          /**< Name */
	N_("SSH - Secure Shell"),                       /**< Description */
	GETTEXT_PACKAGE,                                /**< Translation domain */
	VERSION,                                        /**< Version number */
	"remmina-ssh-symbolic",                         /**< Icon for normal connection */
	"remmina-ssh-symbolic",                         /**< Icon for SSH connection */
	remmina_ssh_basic_settings,                     /**< Array for basic settings */
	remmina_ssh_advanced_settings,                  /**< Array for advanced settings */
	REMMINA_PROTOCOL_SSH_SETTING_TUNNEL,            /**< SSH settings type */
	remmina_plugin_ssh_features,                    /**< Array for available features */
	remmina_plugin_ssh_init,                        /**< Plugin initialization */
	remmina_plugin_ssh_open_connection,             /**< Plugin open connection */
	remmina_plugin_ssh_close_connection,            /**< Plugin close connection */
	remmina_plugin_ssh_query_feature,               /**< Query for available features */
	remmina_plugin_ssh_call_feature,                /**< Call a feature */
	remmina_ssh_keystroke,                          /**< Send a keystroke */
	NULL                                            /**< No screenshot support available */
};


/*
 * this function is used for
 * - inserting into the list to became a sorted list [g_list_insert_sorted()]
 * - checking the list to avoid doublicate entries [g_list_find_custom()]
 */
static gint
compare(gconstpointer a, gconstpointer b)
{
	return strcmp((gchar *)a, (gchar *)b);
}

void
remmina_ssh_plugin_load_terminal_palettes(gpointer *ssh_terminal_palette_new)
{
	unsigned int preset_rec_size = sizeof(ssh_terminal_palette) / sizeof(gpointer);

	GError *error = NULL;
	GList *files = NULL;
	unsigned int rec_size = 0;
	/*
	 * count number of (all) files to reserve enought memory
	 */
	/* /usr/local/share/remmina */
	const gchar *const *dirs = g_get_system_data_dirs();

	unsigned int i = 0;

	for (i = 0; dirs[i] != NULL; ++i) {
		GDir *system_data_dir = NULL;
		gchar *remmina_dir = g_build_path("/", dirs[i], "remmina", "theme", NULL);
		system_data_dir = g_dir_open(remmina_dir, 0, &error);
		g_free(remmina_dir);
		// ignoring this error is OK, because the folder may not exist
		if (error) {
			g_error_free(error);
			error = NULL;
		} else {
			if (system_data_dir) {
				const gchar *filename;
				while ((filename = g_dir_read_name(system_data_dir))) {
					if (!g_file_test(filename, G_FILE_TEST_IS_DIR)) {
						if (g_str_has_suffix(filename, ".colors")) {
							gsize len = strrchr(filename, '.') - filename;
							gchar *menu_str = g_strndup(filename, len);
							if (g_list_find_custom(files, menu_str, compare) == NULL)
								files = g_list_insert_sorted(files, menu_str, compare);
						}
					}
				}
			}
		}
	}

	/* ~/.config/remmina/colors */
	gchar *remmina_dir = g_build_path("/", g_get_user_config_dir(), "remmina", "theme", NULL);
	GDir *user_data_dir;
	user_data_dir = g_dir_open(remmina_dir, 0, &error);
	g_free(remmina_dir);
	if (error) {
		g_error_free(error);
		error = NULL;
	} else {
		if (user_data_dir) {
			const gchar *filename;
			while ((filename = g_dir_read_name(user_data_dir))) {
				if (!g_file_test(filename, G_FILE_TEST_IS_DIR)) {
					if (g_str_has_suffix(filename, ".colors")) {
						char *menu_str = g_malloc(strlen(filename) + 1);
						strcpy(menu_str, filename);
						char *t2 = strrchr(menu_str, '.');
						t2[0] = 0;
						if (g_list_find_custom(files, menu_str, compare) == NULL)
							files = g_list_insert_sorted(files, menu_str, compare);
					}
				}
			}
		}
	}

	rec_size = g_list_length(files) * 2;

	gpointer *color_palette = g_malloc((preset_rec_size + rec_size) * sizeof(gpointer));

	unsigned int field_idx = 0;
	*ssh_terminal_palette_new = color_palette;
	// preset with (old) static ssh_terminal_palette data
	for (; field_idx < preset_rec_size; field_idx++) {
		color_palette[field_idx] = ssh_terminal_palette[field_idx];
		if (!color_palette[field_idx])
			break;
	}

	GList *l_files = NULL;
	for (l_files = g_list_first(files); l_files != NULL; l_files = l_files->next) {
		gchar *menu_str = (gchar *)l_files->data;

		color_palette[field_idx++] = menu_str;
		color_palette[field_idx++] = menu_str;
	}
	g_list_free(files);

	color_palette[field_idx] = NULL;
}

void
remmina_ssh_plugin_register(void)
{
	TRACE_CALL(__func__);
	remmina_plugin_ssh_features[0].opt3 = GUINT_TO_POINTER(remmina_pref.vte_shortcutkey_copy);
	remmina_plugin_ssh_features[1].opt3 = GUINT_TO_POINTER(remmina_pref.vte_shortcutkey_paste);
	remmina_plugin_ssh_features[2].opt3 = GUINT_TO_POINTER(remmina_pref.vte_shortcutkey_select_all);
	remmina_plugin_ssh_features[3].opt3 = GUINT_TO_POINTER(remmina_pref.vte_shortcutkey_select_all);

	remmina_plugin_service = &remmina_plugin_manager_service;

	RemminaProtocolSettingOpt *settings;

	// preset new settings with (old) static remmina_ssh_advanced_settings data
	settings = g_memdup(remmina_ssh_advanced_settings, sizeof(remmina_ssh_advanced_settings));

	// create dynamic advanced settings to made replacing of ssh_terminal_palette possible
	gpointer ssh_terminal_palette_new = NULL;

	remmina_ssh_plugin_load_terminal_palettes(&ssh_terminal_palette_new);

	settings[0].opt1 = ssh_terminal_palette_new;
	remmina_plugin_ssh.advanced_settings = (RemminaProtocolSetting *)settings;

	remmina_plugin_service->register_plugin((RemminaPlugin *)&remmina_plugin_ssh);

	ssh_threads_set_callbacks(ssh_threads_get_pthread());
	ssh_init();
}

#else

void remmina_ssh_plugin_register(void)
{
	TRACE_CALL(__func__);
}

#endif
