/*   See LICENSE and COPYING files for copyright and license details. */

#ifndef __REMMINANXSESSION_H__
#define __REMMINANXSESSION_H__

G_BEGIN_DECLS

enum
{
	REMMINA_NX_SESSION_COLUMN_DISPLAY,
	REMMINA_NX_SESSION_COLUMN_TYPE,
	REMMINA_NX_SESSION_COLUMN_ID,
	REMMINA_NX_SESSION_COLUMN_STATUS,
	REMMINA_NX_SESSION_COLUMN_NAME,
	REMMINA_NX_SESSION_N_COLUMNS
};

typedef struct _RemminaNXSession RemminaNXSession;

typedef gboolean (*RemminaNXPassphraseCallback)(gchar **passphrase, gpointer userdata);
typedef void (*RemminaNXLogCallback)(const gchar *fmt, ...);

RemminaNXSession* remmina_nx_session_new(void);

void remmina_nx_session_free(RemminaNXSession *nx);

gboolean remmina_nx_session_has_error(RemminaNXSession *nx);

const gchar* remmina_nx_session_get_error(RemminaNXSession *nx);

void remmina_nx_session_clear_error(RemminaNXSession *nx);

void remmina_nx_session_set_encryption(RemminaNXSession *nx, gint encryption);

void remmina_nx_session_set_localport(RemminaNXSession *nx, gint localport);

void remmina_nx_session_set_log_callback(RemminaNXSession *nx, RemminaNXLogCallback log_callback);

gboolean remmina_nx_session_open(RemminaNXSession *nx, const gchar *server, guint port, const gchar *private_key_file,
		RemminaNXPassphraseCallback passphrase_func, gpointer userdata);

gboolean remmina_nx_session_login(RemminaNXSession *nx, const gchar *username, const gchar *password);

void remmina_nx_session_add_parameter(RemminaNXSession *nx, const gchar *name, const gchar *valuefmt, ...);

gboolean remmina_nx_session_list(RemminaNXSession *nx);

void remmina_nx_session_set_tree_view(RemminaNXSession *nx, GtkTreeView *tree);

gboolean remmina_nx_session_iter_first(RemminaNXSession *nx, GtkTreeIter *iter);

gboolean remmina_nx_session_iter_next(RemminaNXSession *nx, GtkTreeIter *iter);

gchar* remmina_nx_session_iter_get(RemminaNXSession *nx, GtkTreeIter *iter, gint column);

void remmina_nx_session_iter_set(RemminaNXSession *nx, GtkTreeIter *iter, gint column, const gchar *data);

gboolean remmina_nx_session_allow_start(RemminaNXSession *nx);

gboolean remmina_nx_session_start(RemminaNXSession *nx);

gboolean remmina_nx_session_attach(RemminaNXSession *nx);

gboolean remmina_nx_session_restore(RemminaNXSession *nx);

gboolean remmina_nx_session_terminate(RemminaNXSession *nx);

gboolean remmina_nx_session_tunnel_open(RemminaNXSession *nx);

gboolean remmina_nx_session_invoke_proxy(RemminaNXSession *nx, gint display, GChildWatchFunc exit_func, gpointer user_data);

void remmina_nx_session_bye(RemminaNXSession *nx);

G_END_DECLS

#endif  /* __REMMINANXSESSION_H__  */

