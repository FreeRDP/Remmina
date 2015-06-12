/*  See LICENSE and COPYING files for copyright and license details. */

#include "config.h"
#include <glib.h>
#ifdef HAVE_LIBGCRYPT
#include <gcrypt.h>
#endif
#include "remmina_pref.h"
#include "remmina_crypt.h"
#include "remmina/remmina_trace_calls.h"

#ifdef HAVE_LIBGCRYPT

static gboolean remmina_crypt_init(gcry_cipher_hd_t *phd)
{
	TRACE_CALL("remmina_crypt_init");
	guchar* secret;
	gcry_error_t err;
	gsize secret_len;

	secret = g_base64_decode(remmina_pref.secret, &secret_len);

	if (secret_len < 32)
	{
		g_print("secret corrupted\n");
		g_free(secret);
		return FALSE;
	}

	err = gcry_cipher_open(phd, GCRY_CIPHER_3DES, GCRY_CIPHER_MODE_CBC, 0);

	if (err)
	{
		g_print("gcry_cipher_open failure: %s\n", gcry_strerror(err));
		g_free(secret);
		return FALSE;
	}

	err = gcry_cipher_setkey((*phd), secret, 24);

	if (err)
	{
		g_print("gcry_cipher_setkey failure: %s\n", gcry_strerror(err));
		g_free(secret);
		gcry_cipher_close((*phd));
		return FALSE;
	}

	err = gcry_cipher_setiv((*phd), secret + 24, 8);

	if (err)
	{
		g_print("gcry_cipher_setiv failure: %s\n", gcry_strerror(err));
		g_free(secret);
		gcry_cipher_close((*phd));
		return FALSE;
	}

	g_free(secret);

	return TRUE;
}

gchar* remmina_crypt_encrypt(const gchar *str)
{
	TRACE_CALL("remmina_crypt_encrypt");
	guchar* buf;
	gint buf_len;
	gchar* result;
	gcry_error_t err;
	gcry_cipher_hd_t hd;

	if (!str || str[0] == '\0')
		return NULL;

	if (!remmina_crypt_init(&hd))
		return NULL;

	buf_len = strlen(str);
	/* Pack to 64bit block size, and make sure it's always 0-terminated */
	buf_len += 8 - buf_len % 8;
	buf = (guchar*) g_malloc(buf_len);
	memset(buf, 0, buf_len);
	memcpy(buf, str, strlen(str));

	err = gcry_cipher_encrypt(hd, buf, buf_len, NULL, 0);

	if (err)
	{
		g_print("gcry_cipher_encrypt failure: %s\n", gcry_strerror(err));
		g_free(buf);
		gcry_cipher_close(hd);
		return NULL;
	}

	result = g_base64_encode(buf, buf_len);

	g_free(buf);
	gcry_cipher_close(hd);

	return result;
}

gchar* remmina_crypt_decrypt(const gchar *str)
{
	TRACE_CALL("remmina_crypt_decrypt");
	guchar* buf;
	gsize buf_len;
	gcry_error_t err;
	gcry_cipher_hd_t hd;

	if (!str || str[0] == '\0')
		return NULL;

	if (!remmina_crypt_init(&hd))
		return NULL;

	buf = g_base64_decode(str, &buf_len);

	err = gcry_cipher_decrypt(hd, buf, buf_len, NULL, 0);

	if (err)
	{
		g_print("gcry_cipher_decrypt failure: %s\n", gcry_strerror(err));
		g_free(buf);
		gcry_cipher_close(hd);
		return NULL;
	}

	gcry_cipher_close(hd);

	/* Just in case */
	buf[buf_len - 1] = '\0';

	return (gchar*) buf;
}

#else

gchar* remmina_crypt_encrypt(const gchar *str)
{
	TRACE_CALL("remmina_crypt_encrypt");
	return NULL;
}

gchar* remmina_crypt_decrypt(const gchar *str)
{
	TRACE_CALL("remmina_crypt_decrypt");
	return NULL;
}

#endif

