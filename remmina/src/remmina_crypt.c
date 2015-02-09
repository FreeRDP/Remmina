/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

