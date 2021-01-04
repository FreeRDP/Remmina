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

/**
 * @file remmina_sodium.c
 * @brief Remmina encryption functions,
 * @author Antenore Gatta
 * @date 31 Mar 2019
 *
 * These functions are used to:
 *  - hash password using the Argon2 hashing algorithm.
 *  - Encrypt and decrypt data streams (files for examples).
 *
 * @code
 *
 *  gchar *test = remmina_sodium_pwhash("Password test");
 *  g_free(test);
 *  test = remmina_sodium_pwhash_str("Password Test");
 *  g_free(test);
 *  gint rc = remmina_sodium_pwhash_str_verify("$argon2id$v=19$m=65536,t=2,p=1$6o+kpazlHSaevezH2J9qUA$4pN75oHgyh1BLc/b+ybLYHjZbatG4ZSCSlxLI32YPY4", "Password Test");
 *
 * @endcode
 *
 */

#include <string.h>

#if defined(__linux__)
# include <fcntl.h>
# include <unistd.h>
# include <sys/ioctl.h>
# include <linux/random.h>
#endif

#include "config.h"
#include <glib.h>
#include "remmina/remmina_trace_calls.h"

#include "remmina_sodium.h"
#if SODIUM_VERSION_INT >= 90200

gchar *remmina_sodium_pwhash(const gchar *pass)
{
	TRACE_CALL(__func__);
	g_info("Generating passphrase (may take a while)...");
	/* Create a random salt for the key derivation function */
	unsigned char salt[crypto_pwhash_SALTBYTES] = { 0 };
	randombytes_buf(salt, sizeof salt);

	/* Use argon2 to convert password to a full size key */
	unsigned char key[crypto_secretbox_KEYBYTES];
	if (crypto_pwhash(key, sizeof key, pass, strlen(pass), salt,
			  crypto_pwhash_OPSLIMIT_INTERACTIVE,
			  crypto_pwhash_MEMLIMIT_INTERACTIVE,
			  crypto_pwhash_ALG_DEFAULT) != 0) {
		g_error("%s - Out of memory!", __func__);
		exit(1);
	}

	g_info("%s - Password hashed", __func__);
	return g_strdup((const char *)key);
}

gchar *remmina_sodium_pwhash_str(const gchar *pass)
{
	TRACE_CALL(__func__);
	g_info("Generating passphrase (may take a while)...");
	/* Create a random salt for the key derivation function */
	unsigned char salt[crypto_pwhash_SALTBYTES] = { 0 };
	randombytes_buf(salt, sizeof salt);

	/* Use argon2 to convert password to a full size key */
	char key[crypto_pwhash_STRBYTES];
	if (crypto_pwhash_str(key, pass, strlen(pass),
			      crypto_pwhash_OPSLIMIT_INTERACTIVE,
			      crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
		g_error("%s - Out of memory!", __func__);
		exit(1);
	}

	g_info("%s - Password hashed", __func__);
	return g_strdup((const char *)key);
}

gint remmina_sodium_pwhash_str_verify(const char *key, const char *pass)
{
	TRACE_CALL(__func__);

	gint rc;

	rc = crypto_pwhash_str_verify(key, pass, strlen(pass));

	return rc;
}

void remmina_sodium_init(void)
{
	TRACE_CALL(__func__);
#if defined(__linux__) && defined(RNDGETENTCNT)
	int fd;
	int c;

	if ((fd = open("/dev/random", O_RDONLY)) != -1) {
		if (ioctl(fd, RNDGETENTCNT, &c) == 0 && c < 160) {
			g_printerr("This system doesn't provide enough entropy to quickly generate high-quality random numbers.\n"
				   "Installing the rng-utils/rng-tools, jitterentropy or haveged packages may help.\n"
				   "On virtualized Linux environments, also consider using virtio-rng.\n"
				   "The service will not start until enough entropy has been collected.\n");
		}
		(void)close(fd);
	}
#endif

	if (sodium_init() < 0)
		g_critical("%s - Failed to initialize sodium, it is not safe to use", __func__);
}

#endif
