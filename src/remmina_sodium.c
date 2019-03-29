/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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

#include <string.h>

#if defined(__linux__)
# include <fcntl.h>
# include <unistd.h>
# include <sys/ioctl.h>
# include <linux/random.h>
#endif

#include "config.h"
#include <glib.h>

#include <sodium.h>

#include "remmina_sodium.h"
#include "remmina/remmina_trace_calls.h"

static void remmina_sodium_pwhash(const char *pass)
{
	g_info("Generating passphrase (may take a while)...");
	/* Create a random salt for the key derivation function */
	unsigned char salt[crypto_pwhash_SALTBYTES] = {0};
	randombytes_buf(salt, sizeof salt);

	/* Use argon2 to convert password to a full size key */
	unsigned char key[crypto_secretbox_KEYBYTES];
	if (crypto_pwhash(key, sizeof key, pass, strlen(pass), salt,
			crypto_pwhash_OPSLIMIT_INTERACTIVE,
			crypto_pwhash_MEMLIMIT_INTERACTIVE,
			crypto_pwhash_ALG_DEFAULT) != 0) {
		g_error("Out of memory!\n");
		exit(1);
	}

	g_info("Password hashed, it is: %s", key);
}
static void remmina_sodium_pwhash_str(const char *pass)
{
	g_info("Generating passphrase (may take a while)...");
	/* Create a random salt for the key derivation function */
	unsigned char salt[crypto_pwhash_SALTBYTES] = {0};
	randombytes_buf(salt, sizeof salt);

	/* Use argon2 to convert password to a full size key */
	char key[crypto_pwhash_STRBYTES];
	if (crypto_pwhash_str(key, pass, strlen(pass),
				crypto_pwhash_OPSLIMIT_INTERACTIVE,
				crypto_pwhash_MEMLIMIT_INTERACTIVE) != 0) {
		g_error("Out of memory!\n");
		exit(1);
	}

	g_info("Password hashed, it is: %s", key);
}

void remmina_sodium_init(void) {
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
		(void) close(fd);
	}
#endif
	if (sodium_init() < 0) {
		g_critical ("%s - Failed to initialize sodium, it is not safe to use", __func__);
	}

	//remmina_sodium_pwhash("Test di una password 123");
	remmina_sodium_pwhash_str("Test di una password 123");

}
