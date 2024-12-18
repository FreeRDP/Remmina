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
 *  @file: remmina_utils.h
 *  General utility functions, non-GTK related.
 */

#pragma once

#include <glib.h>
#include <gio/gio.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>

#define MAX_COMPRESSED_FILE_SIZE 100000 //100kb file size max
#define NAME_OF_DIGEST "SHA256"
#define RSA_KEYTYPE "RSA"

/* https://github.com/hboetes/mg/issues/7#issuecomment-475869095 */
#if defined(__APPLE__) || defined(__NetBSD__)
#define st_atim st_atimespec
#define st_ctim st_ctimespec
#define st_mtim st_mtimespec
#endif

extern const char *remmina_RSA_PubKey_v1;
extern const char *remmina_RSA_PubKey_v2;
extern const char *remmina_EC_PubKey;
G_BEGIN_DECLS
gint remmina_utils_string_find(GString *haystack, gint start, gint end, const gchar *needle);
gint remmina_utils_string_replace(GString *str, gint pos, gint len, const gchar *replace);

guint remmina_utils_string_replace_all(GString *haystack, const gchar *needle, const gchar *replace);
gchar *remmina_utils_string_strip(const gchar *s);

gchar *remmina_utils_get_lang();
gchar *remmina_utils_get_kernel_name();
gchar *remmina_utils_get_kernel_release();
gchar *remmina_utils_get_kernel_arch();
gchar *remmina_utils_get_lsb_id();
gchar *remmina_utils_get_lsb_description();
gchar *remmina_utils_get_lsb_release();
gchar *remmina_utils_get_lsb_codename();
gchar *remmina_utils_get_process_list();
GHashTable *remmina_utils_get_etc_release();
gchar *remmina_utils_get_dev();
gchar *remmina_utils_get_logical();
gchar *remmina_utils_get_link();
gchar *remmina_utils_get_python();
gchar *remmina_utils_get_mage();
gchar *remmina_sha256_buffer(const guchar *buffer, gssize length);
gchar *remmina_sha1_file(const gchar *filename);
gchar *remmina_gen_random_uuid();
EVP_PKEY *remmina_get_pubkey(const char *keytype, const char *public_key_selection);
gchar *remmina_rsa_encrypt_string(EVP_PKEY *pubkey, const char *instr);
int remmina_decompress_from_memory_to_file(guchar *source, int len, GFile* plugin_file);
int remmina_compress_from_file_to_file(GFile *source, GFile *dest);
gboolean remmina_verify_plugin_signature(const guchar *signature, GFile* plugin_file, size_t message_length, size_t signature_length);
gboolean remmina_execute_plugin_signature_verification(GFile* plugin_file, size_t msg_len, const guchar *sig, size_t sig_len, EVP_PKEY* public_key);
G_END_DECLS
