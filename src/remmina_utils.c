/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
 * Copyright (C) 2023-2024 Hiroyuki Tanaka, Sunil Bhat
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
 * General utility functions, non-GTK related.
 */

#include <stdlib.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <locale.h>

#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <openssl/evp.h>

#if OPENSSL_VERSION_MAJOR >= 3
#include <openssl/decoder.h>
#include <openssl/core_names.h>
#else
#include <openssl/pem.h>
#endif

#include <openssl/rsa.h>
#include <openssl/err.h>
#include "remmina_sodium.h"
#include "remmina/remmina_trace_calls.h"
#include "remmina_utils.h"
#include "remmina_log.h"
#include "remmina_plugin_manager.h"

/** Returns @c TRUE if @a ptr is @c NULL or @c *ptr is @c FALSE. */
#define EMPTY(ptr) \
	(!(ptr) || !*(ptr))

/* Copyright (C) 1998 VMware, Inc. All rights reserved.
 * Some of the code in this file is taken from the VMware open client.
 */
typedef struct lsb_distro_info {
	gchar * name;
	gchar * scanstring;
} LSBDistroInfo;

/*
 * static LSBDistroInfo lsbFields[] = {
 *      { "DISTRIB_ID=",	  "DISTRIB_ID=%s"		},
 *      { "DISTRIB_RELEASE=",	  "DISTRIB_RELEASE=%s"		},
 *      { "DISTRIB_CODENAME=",	  "DISTRIB_CODENAME=%s"		},
 *      { "DISTRIB_DESCRIPTION=", "DISTRIB_DESCRIPTION=%s"	},
 *      { NULL,			  NULL				},
 * };
 */

typedef struct distro_info {
	gchar * name;
	gchar * filename;
} DistroInfo;

static DistroInfo distroArray[] = {
	{ "RedHat",		"/etc/redhat-release"	     },
	{ "RedHat",		"/etc/redhat_version"	     },
	{ "Sun",		"/etc/sun-release"	     },
	{ "SuSE",		"/etc/SuSE-release"	     },
	{ "SuSE",		"/etc/novell-release"	     },
	{ "SuSE",		"/etc/sles-release"	     },
	{ "SuSE",		"/etc/os-release"	     },
	{ "Debian",		"/etc/debian_version"	     },
	{ "Debian",		"/etc/debian_release"	     },
	{ "Ubuntu",		"/etc/lsb-release"	     },
	{ "Mandrake",		"/etc/mandrake-release"	     },
	{ "Mandriva",		"/etc/mandriva-release"	     },
	{ "Mandrake",		"/etc/mandrakelinux-release" },
	{ "TurboLinux",		"/etc/turbolinux-release"    },
	{ "Fedora Core",	"/etc/fedora-release"	     },
	{ "Gentoo",		"/etc/gentoo-release"	     },
	{ "Novell",		"/etc/nld-release"	     },
	{ "Annvix",		"/etc/annvix-release"	     },
	{ "Arch",		"/etc/arch-release"	     },
	{ "Arklinux",		"/etc/arklinux-release"	     },
	{ "Aurox",		"/etc/aurox-release"	     },
	{ "BlackCat",		"/etc/blackcat-release"	     },
	{ "Cobalt",		"/etc/cobalt-release"	     },
	{ "Conectiva",		"/etc/conectiva-release"     },
	{ "Immunix",		"/etc/immunix-release"	     },
	{ "Knoppix",		"/etc/knoppix_version"	     },
	{ "Linux-From-Scratch", "/etc/lfs-release"	     },
	{ "Linux-PPC",		"/etc/linuxppc-release"	     },
	{ "MkLinux",		"/etc/mklinux-release"	     },
	{ "PLD",		"/etc/pld-release"	     },
	{ "Slackware",		"/etc/slackware-version"     },
	{ "Slackware",		"/etc/slackware-release"     },
	{ "SMEServer",		"/etc/e-smith-release"	     },
	{ "Solaris",		"/etc/release"		     },
	{ "Solus",		"/etc/solus-release"	     },
	{ "Tiny Sofa",		"/etc/tinysofa-release"	     },
	{ "UltraPenguin",	"/etc/ultrapenguin-release"  },
	{ "UnitedLinux",	"/etc/UnitedLinux-release"   },
	{ "VALinux",		"/etc/va-release"	     },
	{ "Yellow Dog",		"/etc/yellowdog-release"     },
	{ NULL,			NULL			     },
};

const char *remmina_RSA_PubKey_v1 =
		"-----BEGIN PUBLIC KEY-----\n"
		"MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAyTig5CnpPOIZQn5yVfsg\n"
		"Y+fNzVS3u0zgyZ7QPstVMgCR234KobRDy/6NOa6tHAzohn166dYZOrkOqFtAUUzQ\n"
		"wVT0P9KkA0RTIChNBCA7G+LE3BYgomoSm2ofBRsfax8RdwYzsqArBHwluv3F6zOT\n"
		"aLwUbtUbDYRwzeIOlfcBLxNlPxHKR8Jq9zREVaEHm8Vj8/e6vNZ66v5I8tg+NaUV\n"
		"omR1h0SPfU0R77x1q/LX41jHISq/cCRLoi4ft+zEJ03HhtmPeV84DfWwtQtpv9P4\n"
		"q2bC5pEH73VHs/igVC44JA9eNcJ/qGxd1uI7K3DGrqtVfa+csEzGtWD3THEo/ZXW\n"
		"bBlGIbOvh58DxKpheR4C3xzWJfeHRUPueGaanwpALydaV5uE4yFwPmQ9KvBhEQAg\n"
		"ck9d10FPKQfHXOAfXkTWToeVLoIk/oDIo9XUbEnFHELtw3gIGNHpMUmUOEk7th80\n"
		"Cldf86fyEw0SPnP+y+SR6gw1zOvs1gbBfMxHsNKIfmpps162JV0bpP0vz7eZMyme\n"
		"EuEEJ6TiyPHLaHaAluljc4UlpA+Huh/S8pZeObUhFpk3bvKVol/BHp3p388PRzZ1\n"
		"eYLNXO8muQqL2SX4k9ndFggsDnr2QYp/dkLaoclNJUiBWPDSHDCuRMX1rCm+wv1p\n"
		"wGWm2KJS9Iz7J5Bc19pcm90CAwEAAQ==\n"
		"-----END PUBLIC KEY-----\n";


const char *remmina_RSA_PubKey_v2 =
		"-----BEGIN PUBLIC KEY-----\n"
		"MIICIjANBgkqhkiG9w0BAQEFAAOCAg8AMIICCgKCAgEAxn7UtcGVxDbTvbprKNnw\n"
		"CQws91Gg7as+A6PcqJ1GkRP46OKEiwJcVigbKZH53LQXLZUY+kOTYtD21+IicrAP\n"
		"QL1er52oQf47ECEsNQ94kNirs/iqoGo+igNd+4scpFhm8WnrPgVvjbulqwGFY40d\n"
		"/zqHaKgYhLvt8OUXF+YSIAC8cQ5qkXy8ncnrpBi22Slly5MyPfXciZj4oKvtXA54\n"
		"8+TP9E+hwagH6xF7aH6fyEgTmzuI2y4hXzkWRXNg5umifpkdHfppWETA+FHIZ7Dh\n"
		"/jy2NnR3wvoDMSqxNaea/LrGJS2cCQpO5d0EzAT+umA4ou2iDi6DfUgkAU/Vh7Jt\n"
		"fedB6x2iewsMU12ef6UkRFrcf7v1tgfAjbWDUXCZTR5zNBm+nrWI6J8jsQ2h6ykP\n"
		"qFdcUjMilLisDB8XPccUoEa0xCAMS2CgPiaC/CPeZoxpBXXxUlcwHUKLpl7wKxn7\n"
		"/MxAu+ynbfL44xEJ74ka1z06Zi7pa4v16Pv0CAgoIRWI0mvZV7iANiBIvNQ5jE8I\n"
		"k273owYRDfnZEHbFMF3TBzFdG6dALpJYqPA649p6VKNwoEksubf9ygWHWmaheUA1\n"
		"Vq3SYIEx+Kymr650VdWLVXrLF+Gl+QXcGtfd5rTnVwW+erKWJU8bFDkmKLw8xADC\n"
		"LlH/YYsHZOx0hXoxQJf+VHcCAwEAAQ==\n"
		"-----END PUBLIC KEY-----\n";


static gint remmina_utils_strpos(const gchar *haystack, const gchar *needle)
{
	TRACE_CALL(__func__);
	const gchar *sub;

	if (!*needle)
		return -1;

	sub = strstr(haystack, needle);
	if (!sub)
		return -1;

	return sub - haystack;
}

// Determines if we're in a flatpak environment and prepends to the cmd if so
// The original value passed in will be freed
gchar* remmina_utils_get_flatpak_command(gchar *cmd){
	gchar* ret = NULL;
	gchar *flatpak_info = g_build_filename(g_get_user_runtime_dir(), "flatpak-info", NULL);

	if (g_file_test(flatpak_info, G_FILE_TEST_EXISTS)) {
		ret = g_strconcat("flatpak-spawn --host ", cmd, NULL);
	} 
	else{
		ret = g_strdup(cmd);
	}
	g_free(flatpak_info);
	g_free(cmd);
	return ret;
}

/* end can be -1 for haystack->len.
 * returns: position of found text or -1.
 * (C) Taken from geany */
gint remmina_utils_string_find(GString *haystack, gint start, gint end, const gchar *needle)
{
	TRACE_CALL(__func__);
	gint pos;

	g_return_val_if_fail(haystack != NULL, -1);
	if (haystack->len == 0)
		return -1;

	g_return_val_if_fail(start >= 0, -1);
	if (start >= (gint)haystack->len)
		return -1;

	g_return_val_if_fail(!EMPTY(needle), -1);

	if (end < 0)
		end = haystack->len;

	pos = remmina_utils_strpos(haystack->str + start, needle);
	if (pos == -1)
		return -1;

	pos += start;
	if (pos >= end)
		return -1;
	return pos;
}

/* Replaces @len characters from offset @a pos.
 * len can be -1 to replace the remainder of @a str.
 * returns: pos + strlen(replace).
 * (C) Taken from geany */
gint remmina_utils_string_replace(GString *str, gint pos, gint len, const gchar *replace)
{
	TRACE_CALL(__func__);
	g_string_erase(str, pos, len);
	if (replace) {
		g_string_insert(str, pos, replace);
		pos += strlen(replace);
	}
	return pos;
}

/**
 * Replaces all occurrences of @a needle in @a haystack with @a replace.
 *
 * @param haystack The input string to operate on. This string is modified in place.
 * @param needle The string which should be replaced.
 * @param replace The replacement for @a needle.
 *
 * @return Number of replacements made.
 **/
guint remmina_utils_string_replace_all(GString *haystack, const gchar *needle, const gchar *replace)
{
	TRACE_CALL(__func__);
	guint count = 0;
	gint pos = 0;
	gsize needle_length = strlen(needle);

	while (1) {
		pos = remmina_utils_string_find(haystack, pos, -1, needle);

		if (pos == -1)
			break;

		pos = remmina_utils_string_replace(haystack, pos, needle_length, replace);
		count++;
	}
	return count;
}

/**
 * Strip \n, \t and \" from a given string.
 * This function is particularly useful with g_spawn_command_line_sync that does
 * not strip control characters from the output.
 * @warning the result should be freed.
 * @param a string.
 * @return a newly allocated copy of string cleaned by \t, \n and \"
 */
gchar *remmina_utils_string_strip(const gchar *s)
{
	gchar *p = g_malloc(strlen(s) + 1);
	if (p == NULL) {
		return NULL;
	}

	if (p) {
		gchar *p2 = p;
		while (*s != '\0') {
			if (*s != '\t' && *s != '\n' && *s != '\"')
				*p2++ = *s++;
			else
				++s;
		}
		*p2 = '\0';
	}
	return p;
}

/** OS related functions */

/**
 * remmina_utils_read_distrofile.
 *
 * Look for a distro version file /etc/xxx-release.
 * Once found, read the file in and figure out which distribution.
 *
 * @param filename The file path of a Linux distribution release file.
 * @param distroSize The size of the distribution name.
 * @param distro The full distro name.
 * @return Returns a string containing distro information verbatim from /etc/xxx-release (distro). Use g_free to free the string.
 *
 */
static gchar *remmina_utils_read_distrofile(gchar *filename)
{
	TRACE_CALL(__func__);
	gsize file_sz;
	struct stat st;
	gchar *distro_desc = NULL;
	GError *err = NULL;

	if (g_stat(filename, &st) == -1) {
		return NULL;
	}

	if (st.st_size > 131072)
		return NULL;

	if (!g_file_get_contents(filename, &distro_desc, &file_sz, &err)) {
		g_error_free(err);
		return NULL;
	}

	if (file_sz == 0) {
		return NULL;
	}

	return distro_desc;
}

/**
 * Return the current language defined in the LC_ALL.
 * @return a language string or en_US.
 */
gchar *remmina_utils_get_lang(void)
{
	TRACE_CALL(__func__);
	gchar *lang = setlocale(LC_ALL, NULL);
	gchar *ptr;

	if (!lang || lang[0] == '\0') {
		lang = "en_US\0";
	} else {
		ptr = strchr(lang, '.');
		if (ptr != NULL)
			*ptr = '\0';
	}

	return lang;
}

/**
 * Return the OS name as in "uname -s".
 * @return The OS name or NULL.
 */
gchar *remmina_utils_get_kernel_name(void)
{
	TRACE_CALL(__func__);
	struct utsname u;

	if (uname(&u) == -1) {
		return NULL;
	}
	return g_strdup(u.sysname);
}

/**
 * Return the OS version as in "uname -r".
 * @return The OS release or NULL.
 */
gchar *remmina_utils_get_kernel_release(void)
{
	TRACE_CALL(__func__);
	struct utsname u;

	if (uname(&u) == -1) {
		return NULL;
	}
	return g_strdup(u.release);
}

/**
 * Return the machine hardware name as in "uname -m".
 * @return The machine hardware name or NULL.
 */
gchar *remmina_utils_get_kernel_arch(void)
{
	TRACE_CALL(__func__);
	struct utsname u;

	if (uname(&u) == -1) {
		return NULL;
	}
	return g_strdup(u.machine);
}

gchar *remmina_utils_run_command(gchar* command)
{
	gchar *std_out = NULL;
	gchar *std_err = NULL;
	gchar* cmd = remmina_utils_get_flatpak_command(g_strdup(command));
	if (g_spawn_command_line_sync(cmd, &std_out, &std_err, NULL, NULL)){
		g_free(cmd);
		g_free(std_err);
		return std_out;
	}
	g_free(cmd);
	return NULL;

}


/**
 * Print the distribution description if found.
 * Test each known distribution specific information file and print it’s content.
 * @return a string or NULL. Caller must free it with g_free().
 */
GHashTable *remmina_utils_get_etc_release(void)
{
	TRACE_CALL(__func__);
	gchar *etc_release = NULL;
	gint i;
	GHashTable *r;

	r = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

	for (i = 0; distroArray[i].filename != NULL; i++) {
		etc_release = remmina_utils_read_distrofile(distroArray[i].filename);
		if (etc_release) {
			if (etc_release[0] != '\0') {
				g_hash_table_insert(r, distroArray[i].filename, etc_release);
			} else {
				g_free(etc_release);
			}
		}
	}
	return r;
}

/**
 * Print device associated with default route.
 * @return a string or NULL. Caller must free it with g_free().
 */
gchar *remmina_utils_get_dev(void)
{
	TRACE_CALL(__func__);
	gint pos = 0;
	GString *dev;
	gchar* cmd = remmina_utils_run_command("ip route show default");
	if (cmd != NULL) {
		dev = g_string_new(cmd);
		g_free(cmd);
		pos = remmina_utils_string_find(dev, pos, -1, "dev ");
		dev = g_string_erase(dev, 0, pos + 4);
		pos = remmina_utils_string_find(dev, 0, -1, " ");
		dev = g_string_truncate(dev, pos);
		return g_string_free(dev, FALSE);
	}
	return NULL;
}

/**
 * Print address associated with default route.
 * @return a string or NULL. Caller must free it with g_free().
 */
gchar *remmina_utils_get_logical(void)
{
	TRACE_CALL(__func__);
	gchar *dev = NULL;
	gint pos = 0;

	dev = remmina_utils_get_dev();
	GString *lbuf = g_string_new("ip -4 -o addr show ");
	if ( dev != NULL ) {
		lbuf = g_string_append(lbuf, dev);
		g_free(dev);
	}

	gchar *cmd = remmina_utils_run_command(lbuf->str);
	if (cmd != NULL) {
		g_string_free(lbuf, TRUE);
		GString *log = g_string_new(cmd);
		pos = remmina_utils_string_find(log, pos, -1, "inet ");
		log = g_string_erase(log, 0, pos + 5);
		pos = remmina_utils_string_find(log, 0, -1, " ");
		log = g_string_truncate(log, pos);
		g_free(cmd);
		return g_string_free(log, FALSE);
	}
	g_string_free(lbuf, TRUE);
	return NULL;
}

/**
 * Print link associated with default route.
 * @return a string or NULL. Caller must free it with g_free().
 */
gchar *remmina_utils_get_link(void)
{
	TRACE_CALL(__func__);
	gchar *dev = NULL;
	gint pos = 0;

	dev = remmina_utils_get_dev();
	GString *pbuf = g_string_new("ip -4 -o link show ");
	if ( dev != NULL ) {
		pbuf = g_string_append(pbuf, dev);
		g_free(dev);
	}
	gchar *cmd = remmina_utils_run_command(pbuf->str);
	if (cmd != NULL)
	{
		g_string_free(pbuf, TRUE);
		GString *link = g_string_new(cmd);
		pos = remmina_utils_string_find(link, pos, -1, "link/ether ");
		link = g_string_erase(link, 0, pos + 11);
		pos = remmina_utils_string_find(link, 0, -1, " ");
		link = g_string_truncate(link, pos);
		g_free(cmd);
		return g_string_free(link, FALSE);
	}
	g_string_free(pbuf, TRUE);
	return NULL;
}

/**
 * Print python version.
 * @return a string or NULL. Caller must free it with g_free().
 */
gchar *remmina_utils_get_python(void)
{
	TRACE_CALL(__func__);
	gchar *cmd = remmina_utils_run_command("python -V");
	if (cmd == NULL || cmd[0] == 0){
		cmd = remmina_utils_run_command("python3 -V");
	}

	if (cmd != NULL){
		cmd = remmina_utils_string_strip(cmd);
	}

	return cmd;
}


/**
 * Print machine age.
 * @return a string or NULL. Caller must free it with g_free().
 */
gchar *remmina_utils_get_mage(void)
{
	TRACE_CALL(__func__);
	gchar *mage = malloc(21);
	if (mage == NULL) {
		return "";
	}
	struct stat sb;

	if (stat("/etc/machine-id", &sb) == 0) {
		sprintf(mage, "%ld", (long)(time(NULL) - sb.st_mtim.tv_sec));
	}
	else {
		strcpy(mage, "0");
	}
	
	return mage;
}

/**
 * Create a hexadecimal string version of the SHA-256 digest of the
 * buffer.
 *
 * @return a newly allocated string which the caller should free() when
 * finished.
 *
 * If any error occurs, this function returns NULL.
 */
gchar *remmina_sha256_buffer(const guchar *buffer, gssize length)
{
	GChecksum *sha256 = NULL;
	gchar *digest_string = NULL;

	sha256 = g_checksum_new(G_CHECKSUM_SHA256);
	if (sha256 == NULL) {
		goto DONE;
	}

	g_checksum_update(sha256, buffer, length);

	digest_string = g_strdup(g_checksum_get_string(sha256));

DONE:
	if (sha256) {
		g_checksum_free(sha256);
	}

	return digest_string;
}

/**
 * Create a hexadecimal string version of the SHA-1 digest of the
 * contents of the named file.
 *
 * @return a newly allocated string which the caller
 * should free() when finished.
 *
 * If any error occurs while reading the file, (permission denied,
 * file not found, etc.), this function returns NULL.
 *
 * Taken from https://github.com/ttuegel/notmuch do PR in case of substantial modifications.
 *
 */
gchar *remmina_sha1_file(const gchar *filename)
{
	FILE *file;

#define BLOCK_SIZE 4096
	unsigned char block[BLOCK_SIZE];
	size_t bytes_read;
	GChecksum *sha1;
	char *digest = NULL;

	file = fopen(filename, "r");
	if (file == NULL)
		return NULL;

	sha1 = g_checksum_new(G_CHECKSUM_SHA1);
	if (sha1 == NULL)
		goto DONE;

	while (1) {
		bytes_read = fread(block, 1, 4096, file);
		if (bytes_read == 0) {
			if (feof(file))
				break;
			else if (ferror(file))
				goto DONE;
		} else {
			g_checksum_update(sha1, block, bytes_read);
		}
	}

	digest = g_strdup(g_checksum_get_string(sha1));

DONE:
	if (sha1)
		g_checksum_free(sha1);
	if (file)
		fclose(file);

	return digest;
}

/**
 * Generate a random sting of chars to be used as part of UID for news or stats
 * @return a string or NULL. Caller must free it with g_free().
 */
gchar *remmina_gen_random_uuid(void)
{
	TRACE_CALL(__func__);
	gchar *result;
	int i;
	static char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	result = g_malloc0(15);
	if (result == NULL) {
		return "";
	}

	for (i = 0; i < 7; i++)
		result[i] = alpha[randombytes_uniform(sizeof(alpha) - 1)];

	for (i = 0; i < 7; i++)
		result[i + 7] = alpha[randombytes_uniform(sizeof(alpha) - 1)];

	return result;
}

/**
 * Uses OSSL_DECODER_CTX to convert public key string to usable OpenSSL structs
 * @return an EVP_PKEY that can be used for public encryption
 */
EVP_PKEY *remmina_get_pubkey(const char *keytype, const char *public_key_selection) {
	BIO *pkbio;
	EVP_PKEY *pubkey = NULL;

	pkbio = BIO_new_mem_buf(public_key_selection, -1);

	if (pkbio == NULL) {
		return NULL;
	}

#if OPENSSL_VERSION_MAJOR >= 3
	OSSL_DECODER_CTX *dctx;
	const char *format = "PEM";
	const char *structure = NULL;
	dctx = OSSL_DECODER_CTX_new_for_pkey(&pubkey, format, structure,
										keytype,
										EVP_PKEY_PUBLIC_KEY,
										NULL, NULL);

	if (dctx == NULL) {
		return NULL;
	}

	if (OSSL_DECODER_from_bio(dctx, pkbio) == 0) {
		BIO_free(pkbio);
		OSSL_DECODER_CTX_free(dctx);
		return NULL;
	}

	OSSL_DECODER_CTX_free(dctx);
#else
	PEM_read_bio_PUBKEY(pkbio, &pubkey, NULL, NULL);
#endif
	BIO_free(pkbio);
	return pubkey;
}

/**
 * Calls EVP_PKEY_encrypt multiple times to encrypt instr. At the end, base64
 * encode the resulting buffer.
 *
 * @param pubkey an RSA public key
 * @param instr input string to be encrypted
 *
 * @return a buffer ptr. Use g_free() to deallocate it 
 */
gchar *remmina_rsa_encrypt_string(EVP_PKEY *pubkey, const char *instr)
{
#define RSA_OAEP_PAD_SIZE 42
	gchar *enc;
	int remaining;
	size_t blksz, maxblksz;
	unsigned char *outptr;
	unsigned char *ebuf = NULL;
	gsize ebufLen;
	int pkeyLen = EVP_PKEY_size(pubkey);
	int inLen = strlen(instr);
	remaining = inLen;

	maxblksz = pkeyLen - RSA_OAEP_PAD_SIZE;
	ebufLen = (((inLen - 1) / maxblksz) + 1) * pkeyLen;
	ebuf = g_malloc(ebufLen);
	if (ebuf == NULL) {
		return NULL;
	}
	outptr = ebuf;

#if OPENSSL_VERSION_MAJOR >= 3
	EVP_PKEY_CTX *ctx = NULL;
	ctx = EVP_PKEY_CTX_new_from_pkey(NULL, pubkey, NULL);
	if (ctx == NULL) {
		g_free(ebuf);
		return NULL;
	}

	if (EVP_PKEY_encrypt_init_ex(ctx, NULL) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		g_free(ebuf);
		return NULL;
	}

	if (EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) <= 0) {
		EVP_PKEY_CTX_free(ctx);
		g_free(ebuf);
		return NULL;
	}
#else
	RSA* rsa = EVP_PKEY_get1_RSA(pubkey);
	if (rsa == NULL) {
		return NULL;
	}
#endif

	/* encrypt input string block by block */
	while (remaining > 0) {
		blksz = remaining > maxblksz ? maxblksz : remaining;
		size_t out_blksz;

#if OPENSSL_VERSION_MAJOR >= 3
		int calc_size_return_val = EVP_PKEY_encrypt(ctx, NULL, &out_blksz, (const unsigned char *)instr, blksz);
		if (calc_size_return_val == -2) {
			EVP_PKEY_CTX_free(ctx);
			g_free(ebuf);
			return NULL;
		}
		else if (calc_size_return_val <= 0) {
			EVP_PKEY_CTX_free(ctx);
			g_free(ebuf);
			return NULL;
		}

		if (EVP_PKEY_encrypt(ctx, outptr, &out_blksz, (const unsigned char *)instr, blksz) <= 0) {
			EVP_PKEY_CTX_free(ctx);
			g_free(ebuf);
			return NULL;
		}
#else
		out_blksz = RSA_public_encrypt(blksz, (const unsigned char *)instr, outptr, rsa, RSA_PKCS1_OAEP_PADDING);
		if (out_blksz == -1) {
			g_free(ebuf);
			return NULL;
		}
#endif

		instr += blksz;
		remaining -= blksz;
		outptr += out_blksz;
	}

	enc = g_base64_encode(ebuf, ebufLen);
	g_free(ebuf);
#if OPENSSL_VERSION_MAJOR >= 3
	EVP_PKEY_CTX_free(ctx);
#endif
	return enc;
}

/**
 * Attempts to verify the contents of @a plugin_file with base64 encoded @a signature. This function uses the OpenSSL EVP API
 * and expects a well-formed EC signature in DER format. The digest function used is SHA256.
 *
 * @param signature A base64 encoded signature.
 * @param plugin_file The file containing the message you wish to verify.
 * @param message_length The length of the message.
 * @param signature_length The length of the signature.
 *
 * @return TRUE if the signature can be verified or FALSE if it cannot or there is an error.
 */
gboolean remmina_verify_plugin_signature(const guchar *signature, GFile* plugin_file, size_t message_length,
										 size_t signature_length) {
	EVP_PKEY *public_key = NULL;

	/* Get public key */
	public_key = remmina_get_pubkey(RSA_KEYTYPE, remmina_RSA_PubKey_v2);
	if (public_key == NULL) {
		return FALSE;
	}

	gboolean verified = remmina_execute_plugin_signature_verification(plugin_file, message_length, signature,
																	  signature_length, public_key);

	EVP_PKEY_free(public_key);
	if (verified) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**
 * A helper function that performs the actual signature verification (that is, this function makes direct calls to the
 * OPENSSL EVP API) of @a plugin_file using signature @a sig with @a public_key.
 *
 * @param plugin_file The file containing the message you wish to verify.
 * @param msg_len The length of the message.
 * @param sig The signature to verify the message with.
 * @param sig_len The length of the signature.
 * @param public_key The public key to use in the signature verification.
 *
 * @return TRUE if the signature can be verified or FALSE if there is an error or the signature cannot be verified.
 */
gboolean remmina_execute_plugin_signature_verification(GFile* plugin_file, size_t msg_len, const guchar *sig, size_t sig_len, EVP_PKEY* public_key)
{
	/* Returned to caller */
	if(!plugin_file || !msg_len || !sig || !sig_len || !public_key) {
		return FALSE;
	}

	EVP_MD_CTX* ctx = NULL;
	gssize read = 0;
	gssize total_read = 0;
	unsigned char buffer[1025];


	/* Create context for verification */
	ctx = EVP_MD_CTX_new();
	if(ctx == NULL) {
		return FALSE;
	}

	/* Returns the message digest (hash function) specified*/
	const EVP_MD* message_digest = EVP_get_digestbyname(NAME_OF_DIGEST);
	if(message_digest == NULL) {
		EVP_MD_CTX_free(ctx);
		return FALSE;
	}

#if OPENSSL_VERSION_MAJOR >= 3
	/* Set up context with the corresponding message digest. 1 is the only code for success */
	int return_val = EVP_DigestInit_ex(ctx, message_digest, NULL);

	if(return_val != 1) {
		EVP_MD_CTX_free(ctx);
		return FALSE;
	}

	/* Add the public key and initialize the context for verification*/
	return_val = EVP_DigestVerifyInit(ctx, NULL, message_digest, NULL, public_key);
#else
	int return_val = EVP_VerifyInit(ctx, message_digest);
#endif

	if(return_val != 1) {
		EVP_MD_CTX_free(ctx);
		return FALSE;
	}

	GFileInputStream *in_stream = g_file_read(plugin_file, NULL, NULL);

	/* Add the msg to the context. The next step will be to actually verify the added message */
	read = g_input_stream_read(G_INPUT_STREAM(in_stream), buffer, 1024, NULL, NULL);
	while (read > 0){
		total_read += read;
#if OPENSSL_VERSION_MAJOR >= 3
		return_val = EVP_DigestVerifyUpdate(ctx, buffer, read);
#else
		return_val = EVP_VerifyUpdate(ctx, buffer, read);
#endif
		if(return_val != 1) {
			EVP_MD_CTX_free(ctx);
			return FALSE;
		}
		read = g_input_stream_read(G_INPUT_STREAM(in_stream), buffer, 1024, NULL, NULL);
	}
	msg_len = total_read;

	/* Clear any errors for the call below */
	ERR_clear_error();

	/* Execute the signature verification. */
#if OPENSSL_VERSION_MAJOR >= 3
	return_val = EVP_DigestVerifyFinal(ctx, sig, sig_len);
#else
	return_val = EVP_VerifyFinal(ctx, sig, sig_len, public_key);
#endif
	if(return_val != 1) {
		EVP_MD_CTX_free(ctx);
		return FALSE;
	}

	if(ctx != NULL) {
		EVP_MD_CTX_destroy(ctx);
		ctx = NULL;
	}

	return TRUE;
}

/**
 *
 * Decompresses pointer @source with length @len to file pointed to by @plugin_file. It is the caller's responsibility
 * to free @source and @plugin_file when no longer in use.
 *
 * @return The total number of compressed bytes read or -1 on error.
 */
int remmina_decompress_from_memory_to_file(guchar *source, int len, GFile* plugin_file) {
	GError *error = NULL;
	//Compress with gzip. The level is -1 for default
	GZlibDecompressor* gzlib_decompressor = g_zlib_decompressor_new(G_ZLIB_COMPRESSOR_FORMAT_GZIP);
	if (gzlib_decompressor == NULL) {
		return -1;
	}

	GInputStream *base_stream = g_memory_input_stream_new_from_data(source, len, NULL);

	if (base_stream == NULL) {
		g_object_unref(gzlib_decompressor);
		return -1;
	}
	GInputStream *converted_input_stream = g_converter_input_stream_new(base_stream, G_CONVERTER(gzlib_decompressor));

	if (converted_input_stream == NULL) {
		g_object_unref(base_stream);
		g_object_unref(gzlib_decompressor);
		return -1;
	}

	int total_read = 0;

	GFileOutputStream *f_out_stream = g_file_replace(plugin_file, NULL, FALSE, G_FILE_CREATE_NONE, NULL, &error);
	if (f_out_stream == NULL){
		g_object_unref(base_stream);
		g_object_unref(gzlib_decompressor);
		return -1;
	}

	g_output_stream_splice(G_OUTPUT_STREAM(f_out_stream), converted_input_stream, G_OUTPUT_STREAM_SPLICE_CLOSE_SOURCE | G_OUTPUT_STREAM_SPLICE_CLOSE_TARGET, NULL, NULL);
	GFileInfo* info = g_file_query_info(plugin_file, G_FILE_ATTRIBUTE_STANDARD_SIZE, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, NULL, NULL);
	total_read = g_file_info_get_size(info);


	//Freeing stuff
	g_object_unref(converted_input_stream);
	g_object_unref(f_out_stream);
	g_object_unref(gzlib_decompressor);
	g_object_unref(base_stream);
	g_object_unref(info);


	return total_read;

}

/**
 * Compresses file @source to file @dest. Creates enough space to compress MAX_COMPRESSED_FILE_SIZE. It is the caller's
 * responsibility to close @source and @dest.
 *
 * @return Total number of bytes read from source or -1 on error.
 */
int remmina_compress_from_file_to_file(GFile *source, GFile *dest)
{
	TRACE_CALL(__func__);
	GError *error = NULL;

	// Compress with gzip. The level is -1 for default
	GZlibCompressor *gzlib_compressor = g_zlib_compressor_new(G_ZLIB_COMPRESSOR_FORMAT_GZIP, -1);

	if (gzlib_compressor == NULL) {
		REMMINA_DEBUG("Compressor instantiation failed");
		return -1;
	}

	GFileInputStream *base_stream = g_file_read(source, NULL, &error);

	if (base_stream == NULL) {
		REMMINA_DEBUG("Base stream is null, error: %s", error->message);
		g_free(error);
		g_object_unref(gzlib_compressor);
		return -1;
	}

	GInputStream *converted_input_stream = g_converter_input_stream_new((GInputStream *) base_stream, G_CONVERTER(gzlib_compressor));

	if (converted_input_stream == NULL) {
		REMMINA_DEBUG("Failed to allocate converted input stream");
		g_object_unref(base_stream);
		g_object_unref(gzlib_compressor);
		return -1;
	}

	int total_read = 0;

	guint8 *converted, *current_position;
	converted = g_malloc(sizeof(guint8) * MAX_COMPRESSED_FILE_SIZE); // 100kb file max
	if (converted == NULL) {
		REMMINA_DEBUG("Failed to allocate compressed file memory");
		g_object_unref(base_stream);
		g_object_unref(gzlib_compressor);
		g_object_unref(converted_input_stream);
		return -1;
	}

	current_position = converted; // Set traveling pointer

	gsize bytes_read = 0;
	while (total_read < MAX_COMPRESSED_FILE_SIZE) {
		bytes_read = g_input_stream_read(G_INPUT_STREAM(converted_input_stream), current_position, 1, NULL, &error);
		if (error != NULL) {
			REMMINA_DEBUG("Error reading input stream: %s", error->message);
			g_free(error);
			g_object_unref(base_stream);
			g_object_unref(gzlib_compressor);
			g_object_unref(converted_input_stream);
			g_free(converted);
			return -1;
		}

		if (bytes_read == 0) {
			break; // Reached EOF, break read
		}
		current_position += bytes_read;

		total_read += bytes_read;
	}

	if (total_read >= MAX_COMPRESSED_FILE_SIZE) {
		REMMINA_DEBUG("Detected heap overflow in allocating compressed file");
		g_object_unref(base_stream);
		g_object_unref(gzlib_compressor);
		g_object_unref(converted_input_stream);
		g_free(converted);
		return -1;
	}

	gsize *bytes_written = g_malloc(sizeof(gsize) * total_read);
	if (bytes_written == NULL) {
		REMMINA_DEBUG("Failed to allocate bytes written");
		g_object_unref(base_stream);
		g_object_unref(gzlib_compressor);
		g_object_unref(converted_input_stream);
		g_free(converted);
		return -1;
	}

	GFileOutputStream *g_output_stream = g_file_append_to(dest, G_FILE_CREATE_NONE, NULL, &error);

	if (g_output_stream == NULL) {
		REMMINA_DEBUG("Error appending to file: %s", error->message);
		g_error_free(error);
		g_object_unref(base_stream);
		g_object_unref(gzlib_compressor);
		g_object_unref(converted_input_stream);
		g_free(bytes_written);
		g_free(converted);
		return -1;
	}
	g_output_stream_write_all(G_OUTPUT_STREAM(g_output_stream), converted, total_read, bytes_written, NULL, &error);
	if (error != NULL) {
		REMMINA_DEBUG("Error writing all bytes to file: %s", error->message);
		g_error_free(error);
		g_object_unref(base_stream);
		g_object_unref(gzlib_compressor);
		g_object_unref(converted_input_stream);
		g_object_unref(g_output_stream);
		g_free(bytes_written);
		g_free(converted);
		return -1;
	}

	g_object_unref(base_stream);
	g_object_unref(gzlib_compressor);
	g_object_unref(converted_input_stream);
	g_object_unref(g_output_stream);
	g_free(bytes_written);
	g_free(converted);
	return total_read;
}
