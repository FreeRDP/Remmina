/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>
#include "remmina/remmina_trace_calls.h"

/** Returns @c TRUE if @a ptr is @c NULL or @c *ptr is @c FALSE. */
#define EMPTY(ptr) \
	(!(ptr) || !*(ptr))

struct utsname u;

/* Copyright (C) 1998 VMware, Inc. All rights reserved.
 * Some of the code in this file is taken from the VMware open client.
 */
typedef struct lsb_distro_info {
	gchar *name;
	gchar *scanstring;
} LSBDistroInfo;

/*
static LSBDistroInfo lsbFields[] = {
	{ "DISTRIB_ID=",	  "DISTRIB_ID=%s"		},
	{ "DISTRIB_RELEASE=",	  "DISTRIB_RELEASE=%s"		},
	{ "DISTRIB_CODENAME=",	  "DISTRIB_CODENAME=%s"		},
	{ "DISTRIB_DESCRIPTION=", "DISTRIB_DESCRIPTION=%s"	},
	{ NULL,			  NULL				},
};
*/

typedef struct distro_info {
	gchar *name;
	gchar *filename;
} DistroInfo;

static DistroInfo distroArray[] = {
	{ "RedHat",		"/etc/redhat-release"		},
	{ "RedHat",		"/etc/redhat_version"		},
	{ "Sun",		"/etc/sun-release"		},
	{ "SuSE",		"/etc/SuSE-release"		},
	{ "SuSE",		"/etc/novell-release"		},
	{ "SuSE",		"/etc/sles-release"		},
	{ "SuSE",		"/etc/os-release"		},
	{ "Debian",		"/etc/debian_version"		},
	{ "Debian",		"/etc/debian_release"		},
	{ "Ubuntu",		"/etc/lsb-release"		},
	{ "Mandrake",		"/etc/mandrake-release"		},
	{ "Mandriva",		"/etc/mandriva-release"		},
	{ "Mandrake",		"/etc/mandrakelinux-release"	},
	{ "TurboLinux",		"/etc/turbolinux-release"	},
	{ "Fedora Core",	"/etc/fedora-release"		},
	{ "Gentoo",		"/etc/gentoo-release"		},
	{ "Novell",		"/etc/nld-release"		},
	{ "Annvix",		"/etc/annvix-release"		},
	{ "Arch",		"/etc/arch-release"		},
	{ "Arklinux",		"/etc/arklinux-release"		},
	{ "Aurox",		"/etc/aurox-release"		},
	{ "BlackCat",		"/etc/blackcat-release"		},
	{ "Cobalt",		"/etc/cobalt-release"		},
	{ "Conectiva",		"/etc/conectiva-release"	},
	{ "Immunix",		"/etc/immunix-release"		},
	{ "Knoppix",		"/etc/knoppix_version"		},
	{ "Linux-From-Scratch", "/etc/lfs-release"		},
	{ "Linux-PPC",		"/etc/linuxppc-release"		},
	{ "MkLinux",		"/etc/mklinux-release"		},
	{ "PLD",		"/etc/pld-release"		},
	{ "Slackware",		"/etc/slackware-version"	},
	{ "Slackware",		"/etc/slackware-release"	},
	{ "SMEServer",		"/etc/e-smith-release"		},
	{ "Solaris",		"/etc/release"			},
	{ "Solus",		"/etc/solus-release"		},
	{ "Tiny Sofa",		"/etc/tinysofa-release"		},
	{ "UltraPenguin",	"/etc/ultrapenguin-release"	},
	{ "UnitedLinux",	"/etc/UnitedLinux-release"	},
	{ "VALinux",		"/etc/va-release"		},
	{ "Yellow Dog",		"/etc/yellowdog-release"	},
	{ NULL,			NULL				},
};

gint remmina_utils_strpos(const gchar *haystack, const gchar *needle)
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

	if (p) {
		gchar *p2 = p;
		while (*s != '\0') {
			if (*s != '\t' && *s != '\n' && *s != '\"') {
				*p2++ = *s++;
			} else {
				++s;
			}
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
 * @return Returns a string containing distro information verbatium from /etc/xxx-release (distro). Use g_free to free the string.
 *
 */
static gchar* remmina_utils_read_distrofile(gchar *filename)
{
	TRACE_CALL(__func__);
	gsize file_sz;
	struct stat st;
	gchar *distro_desc = NULL;
	GError *err = NULL;

	if (g_stat(filename, &st) == -1) {
		g_debug("%s: could not stat the file %s\n", __func__, filename);
		return NULL;
	}

	g_debug("%s: File %s is %lu bytes long\n", __func__, filename, st.st_size);
	if (st.st_size > 131072)
		return NULL;

	if (!g_file_get_contents(filename, &distro_desc, &file_sz, &err)) {
		g_debug("%s: could not get the file content%s: %s\n", __func__, filename, err->message);
		g_error_free(err);
		return NULL;
	}

	if (file_sz == 0) {
		g_debug("%s: Cannot work with empty file.\n", __FUNCTION__);
		return NULL;
	}

	g_debug("%s: Distro description %s\n", __func__, distro_desc);
	return distro_desc;
}

/**
 * Return the current language defined in the LC_ALL.
 * @return a language string or en_US.
 */
gchar* remmina_utils_get_lang()
{
	gchar *lang = setlocale(LC_ALL, NULL);
	gchar *ptr;

	if (!lang || lang[0] == '\0') {
		lang = "en_US\0";
	} else {
		ptr = strchr(lang, '.');
		if (ptr != NULL) {
			*ptr = '\0';
		}
	}

	return lang;
}
/**
 * Return the OS name as in "uname -s".
 * @return The OS name or NULL.
 */
const gchar* remmina_utils_get_kernel_name()
{
	TRACE_CALL(__func__);
	return u.sysname;
}

const gchar* remmina_utils_get_kernel_release()
/**
 * Return the OS version as in "uname -r".
 * @return The OS release or NULL.
 */
{
	TRACE_CALL(__func__);
	return u.release;
}

/**
 * Return the machine hardware name as in "uname -m".
 * @return The machine hardware name or NULL.
 */
const gchar* remmina_utils_get_kernel_arch()
{
	TRACE_CALL(__func__);
	return u.machine;
}

/**
 * Print the Distributor as specified by the lsb_release command.
 * @return the distributor ID string or NULL. Caller must free it with g_free().
 */
gchar* remmina_utils_get_lsb_id()
{
	TRACE_CALL(__func__);
	gchar *lsb_id = NULL;
	if (g_spawn_command_line_sync("/usr/bin/lsb_release -si", &lsb_id, NULL, NULL, NULL)) {
		return lsb_id;
	}
	return NULL;
}

/**
 * Print the Distribution description as specified by the lsb_release command.
 * @return the Distribution description string or NULL. Caller must free it with g_free().
 */
gchar* remmina_utils_get_lsb_description()
{
	TRACE_CALL(__func__);
	gchar *lsb_description = NULL;
	GError *err = NULL;

	if (g_spawn_command_line_sync("/usr/bin/lsb_release -sd", &lsb_description, NULL, NULL, &err)) {
		return lsb_description;
	}else {
		g_debug("%s: could not execute lsb_release %s\n", __func__, err->message);
		g_error_free(err);
	}
	g_debug("%s: lsb_release %s\n", __func__, lsb_description);
	return NULL;
}

/**
 * Print the Distribution release name as specified by the lsb_release command.
 * @return the Distribution release name string or NULL. Caller must free it with g_free().
 */
gchar* remmina_utils_get_lsb_release()
{
	TRACE_CALL(__func__);
	gchar *lsb_release = NULL;
	if (g_spawn_command_line_sync("/usr/bin/lsb_release -sr", &lsb_release, NULL, NULL, NULL)) {
		return lsb_release;
	}
	return NULL;
}

/**
 * Print the Distribution codename as specified by the lsb_release command.
 * @return the codename string or NULL. Caller must free it with g_free().
 */
gchar* remmina_utils_get_lsb_codename()
{
	TRACE_CALL(__func__);
	gchar *lsb_codename = NULL;
	if (g_spawn_command_line_sync("/usr/bin/lsb_release -sc", &lsb_codename, NULL, NULL, NULL)) {
		return lsb_codename;
	}
	return NULL;
}

/**
 * Print the distribution description if found.
 * Test each known distribution specific information file and print it’s content.
 * @return a string or NULL. Caller must free it with g_free().
 */
GHashTable* remmina_utils_get_etc_release()
{
	TRACE_CALL(__func__);
	gchar *etc_release = NULL;
	gint i;
	GHashTable *r;

	r = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, g_free);

	for (i = 0; distroArray[i].filename != NULL; i++) {
		g_debug("%s: File %s\n", __func__, distroArray[i].filename);
		etc_release = remmina_utils_read_distrofile(distroArray[i].filename);
		if (etc_release) {
			if (etc_release[0] != '\0') {
				g_debug("%s: Distro description %s\n", __func__, etc_release);
				g_hash_table_insert(r, distroArray[i].filename, etc_release);
			} else
				g_free(etc_release);
		}
	}
	return r;
}

/**
 * A sample function to show how use the other fOS related functions.
 * @return a semicolon separated OS data like in "uname -srm".
 */
const gchar* remmina_utils_get_os_info()
{
	TRACE_CALL(__func__);
	gchar *kernel_string;

	if (uname(&u) == -1)
		g_print("uname:");

	kernel_string = g_strdup_printf("%s;%s;%s\n",
		remmina_utils_get_kernel_name(),
		remmina_utils_get_kernel_release(),
		remmina_utils_get_kernel_arch());
	if (!kernel_string || kernel_string[0] == '\0') {
		if(kernel_string)
			g_free(kernel_string);
		kernel_string = g_strdup_printf("%s;%s;%s\n",
			"UNKNOWN",
			"UNKNOWN",
			"UNKNOWN");
    }
	return kernel_string;
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
gchar* remmina_sha1_file (const gchar *filename)
{
    FILE *file;
#define BLOCK_SIZE 4096
    unsigned char block[BLOCK_SIZE];
    size_t bytes_read;
    GChecksum *sha1;
    char *digest = NULL;

    file = fopen (filename, "r");
    if (file == NULL)
	return NULL;

    sha1 = g_checksum_new (G_CHECKSUM_SHA1);
    if (sha1 == NULL)
	goto DONE;

    while (1) {
	bytes_read = fread (block, 1, 4096, file);
	if (bytes_read == 0) {
	    if (feof (file))
		break;
	    else if (ferror (file))
		goto DONE;
	} else {
	    g_checksum_update (sha1, block, bytes_read);
	}
    }

    digest = g_strdup (g_checksum_get_string (sha1));

  DONE:
    if (sha1)
	g_checksum_free (sha1);
    if (file)
	fclose (file);

    return digest;
}

/**
 * Generate a random sting of chars to be used as part of UID for news or stats
  * @return a string or NULL. Caller must free it with g_free().
 */
gchar* remmina_gen_random_uuid()
{
	TRACE_CALL(__func__);
	GRand *rand;
	GDateTime *gdt;
	gchar *result;
	int i;
	static char alpha[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

	result = g_malloc0(15);

    gdt = g_date_time_new_now_utc();
    rand = g_rand_new_with_seed((guint32)g_date_time_to_unix(gdt));
    g_date_time_unref(gdt);

	for (i = 0; i < 7; i++) {
		result[i] = alpha[g_rand_int_range(rand, 0, sizeof(alpha) - 1)];
	}

	for (i = 0; i < 7; i++) {
		result[i + 7] = alpha[g_rand_int_range(rand, 0, sizeof(alpha) - 1)];
	}
	g_rand_free(rand);

	return result;
}
