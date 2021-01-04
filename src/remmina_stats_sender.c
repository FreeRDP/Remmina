/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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
#include <gtk/gtk.h>
#include <string.h>
#include <libsoup/soup.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include "remmina_scheduler.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_log.h"
#include "remmina_stats.h"
#include "remmina_pref.h"

#if !JSON_CHECK_VERSION(1, 2, 0)
	#define json_node_unref(x) json_node_free(x)
#endif

/* Timers */
#define PERIODIC_CHECK_1ST_MS 60000
#define PERIODIC_CHECK_INTERVAL_MS 1200000

#define PERIODIC_UPLOAD_INTERVAL_SEC    2678400

#define PERIODIC_UPLOAD_URL "https://www.remmina.org/stats/upload_stats.php"


//static gint periodic_check_source;
//static gint periodic_check_counter;

static char *remmina_RSA_PubKey_v1 =
	"-----BEGIN PUBLIC KEY-----\n"
	"MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAwuI8eOnDV2y9uPdhN+6Q\n"
	"Cju8+YapN0wKlvwfy1ccQBS+4YnM7/+vzelOzLXJwWBDr/He7G5XEIzOcc9LZsRw\n"
	"XYAoeB3+kP4OrNIVmKfxL7uijoh+79t3WpR8OOOTFDLmtk23tvdJVj+KfRpm0REK\n"
	"BmdPHP8NpBzQElEDgXP9weHwQhPLB6MqpaJmfR4AqSumAcsukjbSaCWhqjO2rEiA\n"
	"eXqJ0JE+PIe4WO1IBvKyYBYP3S77FEMJojkVWGVsjOUGe2VqpX02GaRajRkbqzNK\n"
	"dGmLQt//kcCuPkiqm/qQQTZc0JJYUrmOjFJW9jODQKXHdZrSz8Xz5+v6VJ49v2TM\n"
	"PwIDAQAB\n"
	"-----END PUBLIC KEY-----\n";

typedef struct {
	gboolean show_only;
	JsonNode *statsroot;
} sc_tdata;

static void soup_callback(SoupSession *session, SoupMessage *msg, gpointer user_data)
{
	TRACE_CALL(__func__);
	gchar *s = (gchar*)user_data;
	SoupBuffer *sb;
	gboolean passed;
	GDateTime *gdt;
	gint64 unixts;

	g_free(s);

	if (msg->status_code != 200) {
		REMMINA_DEBUG("HTTP status error sending stats: %d\n", msg->status_code);
		return;
	}

	gdt = g_date_time_new_now_utc();
	unixts = g_date_time_to_unix(gdt);
	g_date_time_to_unix(gdt);

	passed = FALSE;
	sb = soup_message_body_flatten(msg->response_body);
	REMMINA_DEBUG("STATS script response: %.40s\n", sb->data);
	if (strncmp(sb->data, "200 ", 4) != 0) {
		REMMINA_DEBUG("STATS http upload error from server side script: %s\n", sb->data);
	} else {
		passed = TRUE;
	}
	soup_buffer_free(sb);

	if (passed) {
		remmina_pref.periodic_usage_stats_last_sent = unixts;
		remmina_pref_save();
	}

}

static gchar *rsa_encrypt_string(RSA *pubKey, const char *instr)
{
	TRACE_CALL(__func__);
	/* Calls RSA_public_encrypt multiple times to encrypt instr.
	 * At the end, base64 encode the resulting buffer
	 * Return a buffer ptr. Use g_free() to deallocate it */

	int rsaLen = RSA_size(pubKey);
	int inLen = strlen(instr);
	int remaining, r;
	int blksz, maxblksz;
	int ebufSize;
	unsigned char *ebuf, *outptr;
	gchar *enc;

	maxblksz = rsaLen - 12;
	ebufSize = (((inLen - 1) / maxblksz) + 1) * rsaLen;
	ebuf = g_malloc(ebufSize);
	outptr = ebuf;
	remaining = strlen(instr);

	while(remaining > 0) {
		blksz = remaining > maxblksz ? maxblksz : remaining;
		r = RSA_public_encrypt(blksz,
			(const unsigned char *)instr,
			outptr,
			pubKey, RSA_PKCS1_PADDING);	/* Our poor JS libraries only supports RSA_PKCS1_PADDING */
		if (r == -1 ) {
			unsigned long e;
			ERR_load_crypto_strings();
			e = ERR_get_error();
			g_print("Error RSA_public_encrypt(): %s - func: %s -  reason: %s\n", ERR_lib_error_string(e), ERR_func_error_string(e), ERR_reason_error_string(e));
			g_free(ebuf);
			ERR_free_strings();
			return NULL;
		}
		instr += blksz;
		remaining -= blksz;
		outptr += r;
	}

	enc = g_base64_encode(ebuf, ebufSize);
	g_free(ebuf);

	return enc;


}

static gboolean remmina_stats_collector_done(gpointer data)
{
	TRACE_CALL(__func__);
	JsonNode *n;
	JsonGenerator *g;
	gchar *unenc_s, *enc_s;
	SoupSession *ss;
	SoupMessage *msg;
	JsonBuilder *b;
	JsonObject *o;
	BIO *pkbio;
	RSA *pubkey;
	gchar *uid;
	sc_tdata *sctdata;

	sctdata = (sc_tdata *)data;
	if (sctdata == NULL)
		return G_SOURCE_REMOVE;

	n = sctdata->statsroot;
	if (n == NULL) {
		g_free(data);
		return G_SOURCE_REMOVE;
	}

	if ((o = json_node_get_object(n)) == NULL) {
		g_free(data);
		return G_SOURCE_REMOVE;
	}

	uid = g_strdup(json_object_get_string_member(o, "UID"));

	g = json_generator_new();
	json_generator_set_root(g, n);
	json_node_unref(n);
	unenc_s = json_generator_to_data(g, NULL);	// unenc_s=serialized stats
	REMMINA_DEBUG("STATS upload: JSON data%s\n", unenc_s);
	g_object_unref(g);

	/* Now encrypt "s" with remminastats public key */

	pkbio = BIO_new_mem_buf(remmina_RSA_PubKey_v1, -1);
	pubkey = PEM_read_bio_RSA_PUBKEY(pkbio, NULL, NULL, NULL);
	if (pubkey == NULL) {
		ERR_load_crypto_strings();
		unsigned long e;
		e = ERR_get_error();
		g_print("Failure in PEM_read_bio_RSAPublicKey: %s - func: %s -  reason: %s\n", ERR_lib_error_string(e), ERR_func_error_string(e), ERR_reason_error_string(e));
		g_print("%s\n", ERR_error_string( e, NULL ));
		BIO_free(pkbio);
		g_free(unenc_s);
		ERR_free_strings();
		g_free(data);
		g_free(uid);
		return G_SOURCE_REMOVE;
	}

	enc_s = rsa_encrypt_string(pubkey, unenc_s);

	g_free(unenc_s);
	BIO_free(pkbio);


	/* Create new json encrypted object */

	b = json_builder_new();
	json_builder_begin_object(b);
	json_builder_set_member_name(b, "keyversion");
	json_builder_add_int_value(b, 1);
	json_builder_set_member_name(b, "encdata");
	json_builder_add_string_value(b, enc_s);
	json_builder_set_member_name(b, "UID");
	json_builder_add_string_value(b, uid);
	json_builder_end_object(b);
	n = json_builder_get_root(b);
	g_object_unref(b);

	g_free(uid);
	g_free(enc_s);

	if (!sctdata->show_only) {

		g = json_generator_new();
		json_generator_set_root(g, n);
		enc_s = json_generator_to_data(g, NULL);	// unenc_s=serialized stats
		g_object_unref(g);

		ss = soup_session_new();
		msg = soup_message_new("POST", PERIODIC_UPLOAD_URL);
		soup_message_set_request(msg, "application/json",
			SOUP_MEMORY_COPY, enc_s, strlen(enc_s));
		soup_session_queue_message(ss, msg, soup_callback, enc_s);

		REMMINA_DEBUG("STATS upload: Starting upload to url %s\n", PERIODIC_UPLOAD_URL);
	}

	json_node_unref(n);
	g_free(data);

	return G_SOURCE_REMOVE;
}


static gpointer remmina_stats_collector(gpointer data)
{
	TRACE_CALL(__func__);
	JsonNode *n;
	sc_tdata *sctdata;

	sctdata = (sc_tdata *)data;
	n = remmina_stats_get_all();

	/* stats collecting is done. Notify main thread calling
	 * remmina_stats_collector_done() */
	sctdata->statsroot = n;
	g_idle_add(remmina_stats_collector_done, sctdata);

	return NULL;
}

void remmina_stats_sender_send(gboolean show_only)
{
	TRACE_CALL(__func__);

	sc_tdata *sctdata;

	sctdata = g_malloc(sizeof(sc_tdata));
	sctdata->show_only = show_only;

	g_thread_new("stats_collector", remmina_stats_collector, (gpointer)sctdata);

}

gboolean remmina_stat_sender_can_send()
{
	if (remmina_pref.periodic_usage_stats_permitted && remmina_pref_save())
		return TRUE;
	else
		return FALSE;
}

static gboolean remmina_stats_sender_periodic_check(gpointer user_data)
{
	TRACE_CALL(__func__);
	GDateTime *gdt;
	gint64 unixts;
	glong next;

	gdt = g_date_time_new_now_utc();
	unixts = g_date_time_to_unix(gdt);
	g_date_time_to_unix(gdt);

	if (!remmina_stat_sender_can_send())
		return G_SOURCE_REMOVE;

	/* Calculate "next" upload time based on last sent time */
	next = remmina_pref.periodic_usage_stats_last_sent + PERIODIC_UPLOAD_INTERVAL_SEC;
	/* If current time is after "next" or clock is going back (but > 1/1/2018), then do send stats */
	if (unixts > next || (unixts < remmina_pref.periodic_usage_stats_last_sent && unixts > 1514764800)) {
		remmina_stats_sender_send(FALSE);
	}

	return G_SOURCE_CONTINUE;
}

void remmina_stats_sender_schedule()
{
	TRACE_CALL(__func__);
	remmina_scheduler_setup(remmina_stats_sender_periodic_check,
			NULL,
			PERIODIC_CHECK_1ST_MS,
			PERIODIC_CHECK_INTERVAL_MS);
}
