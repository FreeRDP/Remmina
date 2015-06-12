/*  See LICENSE and COPYING files for copyright and license details. */

#include <pthread.h>
#include "remmina_protocol_widget.h"
#include "remmina_init_dialog.h"
#include "remmina_sftp_client.h"
#include "remmina_ftp_client.h"
#include "remmina_ssh_plugin.h"

typedef struct remmina_masterthread_exec_data {

	enum { FUNC_GTK_LABEL_SET_TEXT,
		FUNC_INIT_SAVE_CRED, FUNC_CHAT_RECEIVE, FUNC_FILE_GET_SECRET,
		FUNC_DIALOG_SERVERKEY_CONFIRM, FUNC_DIALOG_AUTHPWD, FUNC_DIALOG_AUTHUSERPWD,
		FUNC_DIALOG_CERT, FUNC_DIALOG_CERTCHANGED, FUNC_DIALOG_AUTHX509,
		FUNC_FTP_CLIENT_UPDATE_TASK, FUNC_FTP_CLIENT_GET_WAITING_TASK,
		FUNC_SFTP_CLIENT_CONFIRM_RESUME,
		FUNC_VTE_TERMINAL_SET_ENCODING_AND_PTY } func;

	union {
		struct {
			GtkLabel *label;
			const gchar *str;
		} gtk_label_set_text;
		struct {
			RemminaProtocolWidget* gp;
		} init_save_creds;
		struct {
			RemminaProtocolWidget* gp;
			const gchar *text;
		} chat_receive;
		struct {
			RemminaFile *remminafile;
			const gchar *setting;
			gchar* retval;
		} file_get_secret;
		struct {
			RemminaInitDialog *dialog;
			const gchar *serverkey;
			const gchar *prompt;
			gint retval;
		} dialog_serverkey_confirm;
		struct {
			RemminaInitDialog *dialog;
			gboolean allow_save;
			const gchar *label;
			gint retval;
		} dialog_authpwd;
		struct {
			RemminaInitDialog *dialog;
			gboolean want_domain;
			gboolean allow_save;
			const gchar *default_username;
			const gchar *default_domain;
			gint retval;
		} dialog_authuserpwd;
		struct {
			RemminaInitDialog *dialog;
			const gchar* subject;
			const gchar* issuer;
			const gchar* fingerprint;
			gint retval;
		} dialog_certificate;
		struct {
			RemminaInitDialog *dialog;
			const gchar* subject;
			const gchar* issuer;
			const gchar* new_fingerprint;
			const gchar* old_fingerprint;
			gint retval;
		} dialog_certchanged;
		struct {
			RemminaInitDialog *dialog;
			const gchar *cacert;
			const gchar *cacrl;
			const gchar *clientcert;
			const gchar *clientkey;
			gint retval;
		} dialog_authx509;
		struct {
			RemminaFTPClient *client;
			RemminaFTPTask* task;
		} ftp_client_update_task;
		struct {
			RemminaFTPClient *client;
			RemminaFTPTask* retval;
		} ftp_client_get_waiting_task;
#if defined (HAVE_LIBSSH) && defined (HAVE_LIBVTE)
		struct {
			RemminaSFTPClient *client;
			const gchar *path;
			gint retval;
		} sftp_client_confirm_resume;
#endif
#ifdef HAVE_LIBVTE
		struct {
			VteTerminal *terminal;
			const char *codeset;
			int slave;
		} vte_terminal_set_encoding_and_pty;
#endif
	} p;

	/* Mutex for thread synchronization */
	pthread_mutex_t mu;
	/* Flag to catch cancellations */
	gboolean cancelled;

} RemminaMTExecData;

void remmina_masterthread_exec_and_wait(RemminaMTExecData *d);

void remmina_masterthread_exec_save_main_thread_id(void);
gboolean remmina_masterthread_exec_is_main_thread(void);



