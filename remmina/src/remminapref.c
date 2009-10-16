/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009 - Vic Lee 
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
 */

#include <gtk/gtk.h>
#include <stdlib.h>
#include <string.h>
#include "remminastringarray.h"
#include "remminapref.h"

const gchar *default_resolutions = "640x480,800x600,1024x768,1152x864,1280x960,1400x1050";

gchar *remmina_pref_file;
RemminaPref remmina_pref;

gchar *remmina_keymap_file;
static GHashTable *remmina_keymap_table = NULL;

/* We could customize this further if there are more requirements */
static const gchar *default_keymap_data =
"# Please check gdk/gdkkeysyms.h for a full list of all key names or hex key values\n"
"\n"
"[Map Meta Keys]\n"
"Super_L = Meta_L\n"
"Super_R = Meta_R\n"
"Meta_L = Super_L\n"
"Meta_R = Super_R\n"
;

static void
remmina_pref_gen_secret (void)
{
    guchar s[32];
    gint i;
    GTimeVal gtime;
    GKeyFile *gkeyfile;
    gchar *content;
    gsize length;

    g_get_current_time (&gtime);
    srand (gtime.tv_sec);

    for (i = 0; i < 32; i++)
    {
        s[i] = (guchar) (rand () % 256);
    }
    remmina_pref.secret = g_base64_encode (s, 32);

    gkeyfile = g_key_file_new ();
    g_key_file_load_from_file (gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
    g_key_file_set_string (gkeyfile, "remmina_pref", "secret", remmina_pref.secret);
    content = g_key_file_to_data (gkeyfile, &length, NULL);
    g_file_set_contents (remmina_pref_file, content, length, NULL);

    g_key_file_free (gkeyfile);
    g_free (content);
}

static guint
remmina_pref_get_keyval_from_str (const gchar *str)
{
    guint k;

    if (!str) return 0;

    k = gdk_keyval_from_name (str);
    if (!k)
    {
        if (sscanf (str, "%x", &k) < 1) k = 0;
    }
    return k;
}

static void
remmina_pref_init_keymap (void)
{
    GKeyFile *gkeyfile;
    gchar **groups;
    gchar **gptr;
    gchar **keys;
    gchar **kptr;
    gsize nkeys;
    gchar *value;
    guint *table;
    guint *tableptr;
    guint k1, k2;

    if (remmina_keymap_table) g_hash_table_destroy (remmina_keymap_table);
    remmina_keymap_table = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

    gkeyfile = g_key_file_new ();
    if (!g_key_file_load_from_file (gkeyfile, remmina_keymap_file, G_KEY_FILE_NONE, NULL))
    {
        if (!g_key_file_load_from_data (gkeyfile,
            default_keymap_data, strlen (default_keymap_data),
            G_KEY_FILE_NONE, NULL))
        {
            g_print ("Failed to initialize keymap table\n");
            g_key_file_free (gkeyfile);
            return;
        }
    }

    groups = g_key_file_get_groups (gkeyfile, NULL);
    gptr = groups;
    while (*gptr)
    {
        keys = g_key_file_get_keys (gkeyfile, *gptr, &nkeys, NULL);
        table = g_new0 (guint, nkeys * 2 + 1);
        g_hash_table_insert (remmina_keymap_table, g_strdup (*gptr), table);

        kptr = keys;
        tableptr = table;
        while (*kptr)
        {
            k1 = remmina_pref_get_keyval_from_str (*kptr);
            if (k1)
            {
                value = g_key_file_get_string (gkeyfile, *gptr, *kptr, NULL);
                k2 = remmina_pref_get_keyval_from_str (value);
                g_free (value);
                *tableptr++ = k1;
                *tableptr++ = k2;
            }
            kptr++;
        }
        g_strfreev (keys);
        gptr++;
    }
    g_strfreev (groups);
}

void
remmina_pref_init (void)
{
    GKeyFile *gkeyfile;

    remmina_pref_file = g_strdup_printf ("%s/.remmina/remmina.pref", g_get_home_dir ());
    remmina_keymap_file = g_strdup_printf ("%s/.remmina/remmina.keymap", g_get_home_dir ());

    gkeyfile = g_key_file_new ();
    g_key_file_load_from_file (gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "save_view_mode", NULL))
        remmina_pref.save_view_mode = g_key_file_get_boolean (gkeyfile, "remmina_pref", "save_view_mode", NULL);
    else
        remmina_pref.save_view_mode = TRUE;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "invisible_toolbar", NULL))
        remmina_pref.invisible_toolbar = g_key_file_get_boolean (gkeyfile, "remmina_pref", "invisible_toolbar", NULL);
    else
        remmina_pref.invisible_toolbar = FALSE;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "default_action", NULL))
        remmina_pref.default_action = g_key_file_get_integer (gkeyfile, "remmina_pref", "default_action", NULL);
    else
        remmina_pref.default_action = REMMINA_ACTION_CONNECT;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "scale_quality", NULL))
        remmina_pref.scale_quality = g_key_file_get_integer (gkeyfile, "remmina_pref", "scale_quality", NULL);
    else
        remmina_pref.scale_quality = GDK_INTERP_HYPER;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "hide_toolbar", NULL))
        remmina_pref.hide_toolbar = g_key_file_get_boolean (gkeyfile, "remmina_pref", "hide_toolbar", NULL);
    else
        remmina_pref.hide_toolbar = FALSE;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "hide_statusbar", NULL))
        remmina_pref.hide_statusbar = g_key_file_get_boolean (gkeyfile, "remmina_pref", "hide_statusbar", NULL);
    else
        remmina_pref.hide_statusbar = FALSE;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "small_toolbutton", NULL))
        remmina_pref.small_toolbutton = g_key_file_get_boolean (gkeyfile, "remmina_pref", "small_toolbutton", NULL);
    else
        remmina_pref.small_toolbutton = FALSE;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "view_file_mode", NULL))
        remmina_pref.view_file_mode = g_key_file_get_integer (gkeyfile, "remmina_pref", "view_file_mode", NULL);
    else
        remmina_pref.view_file_mode = REMMINA_VIEW_FILE_LIST;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "resolutions", NULL))
        remmina_pref.resolutions = g_key_file_get_string (gkeyfile, "remmina_pref", "resolutions", NULL);
    else
        remmina_pref.resolutions = g_strdup (default_resolutions);

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "main_width", NULL))
        remmina_pref.main_width = MAX (600, g_key_file_get_integer (gkeyfile, "remmina_pref", "main_width", NULL));
    else
        remmina_pref.main_width = 600;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "main_height", NULL))
        remmina_pref.main_height = MAX (400, g_key_file_get_integer (gkeyfile, "remmina_pref", "main_height", NULL));
    else
        remmina_pref.main_height = 400;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "main_maximize", NULL))
        remmina_pref.main_maximize = g_key_file_get_boolean (gkeyfile, "remmina_pref", "main_maximize", NULL);
    else
        remmina_pref.main_maximize = FALSE;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "main_sort_column_id", NULL))
        remmina_pref.main_sort_column_id = g_key_file_get_integer (gkeyfile, "remmina_pref", "main_sort_column_id", NULL);
    else
        remmina_pref.main_sort_column_id = 1;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "main_sort_order", NULL))
        remmina_pref.main_sort_order = g_key_file_get_integer (gkeyfile, "remmina_pref", "main_sort_order", NULL);
    else
        remmina_pref.main_sort_order = 0;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "sshtunnel_port", NULL))
        remmina_pref.sshtunnel_port = g_key_file_get_integer (gkeyfile, "remmina_pref", "sshtunnel_port", NULL);
    else
        remmina_pref.sshtunnel_port = DEFAULT_SSHTUNNEL_PORT;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "applet_quick_ontop", NULL))
        remmina_pref.applet_quick_ontop = g_key_file_get_boolean (gkeyfile, "remmina_pref", "applet_quick_ontop", NULL);
    else
        remmina_pref.applet_quick_ontop = FALSE;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "applet_hide_count", NULL))
        remmina_pref.applet_hide_count = g_key_file_get_boolean (gkeyfile, "remmina_pref", "applet_hide_count", NULL);
    else
        remmina_pref.applet_hide_count = FALSE;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "recent_maximum", NULL))
        remmina_pref.recent_maximum = g_key_file_get_integer (gkeyfile, "remmina_pref", "recent_maximum", NULL);
    else
        remmina_pref.recent_maximum = 10;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "default_mode", NULL))
        remmina_pref.default_mode = g_key_file_get_integer (gkeyfile, "remmina_pref", "default_mode", NULL);
    else
        remmina_pref.default_mode = 0;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "tab_mode", NULL))
        remmina_pref.tab_mode = g_key_file_get_integer (gkeyfile, "remmina_pref", "tab_mode", NULL);
    else
        remmina_pref.tab_mode = 0;

    if (g_key_file_has_key (gkeyfile, "remmina_pref", "secret", NULL))
        remmina_pref.secret = g_key_file_get_string (gkeyfile, "remmina_pref", "secret", NULL);
    else
        remmina_pref.secret = NULL;

    g_key_file_free (gkeyfile);

    if (remmina_pref.secret == NULL) remmina_pref_gen_secret ();

    remmina_pref_init_keymap ();
}

void
remmina_pref_save (void)
{
    GKeyFile *gkeyfile;
    gchar *content;
    gsize length;

    gkeyfile = g_key_file_new ();

    g_key_file_load_from_file (gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

    g_key_file_set_boolean (gkeyfile, "remmina_pref", "save_view_mode", remmina_pref.save_view_mode);
    g_key_file_set_boolean (gkeyfile, "remmina_pref", "invisible_toolbar", remmina_pref.invisible_toolbar);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "default_action", remmina_pref.default_action);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "scale_quality", remmina_pref.scale_quality);
    g_key_file_set_boolean (gkeyfile, "remmina_pref", "hide_toolbar", remmina_pref.hide_toolbar);
    g_key_file_set_boolean (gkeyfile, "remmina_pref", "hide_statusbar", remmina_pref.hide_statusbar);
    g_key_file_set_boolean (gkeyfile, "remmina_pref", "small_toolbutton", remmina_pref.small_toolbutton);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "view_file_mode", remmina_pref.view_file_mode);
    g_key_file_set_string (gkeyfile, "remmina_pref", "resolutions", remmina_pref.resolutions);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "main_width", remmina_pref.main_width);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "main_height", remmina_pref.main_height);
    g_key_file_set_boolean (gkeyfile, "remmina_pref", "main_maximize", remmina_pref.main_maximize);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "main_sort_column_id", remmina_pref.main_sort_column_id);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "main_sort_order", remmina_pref.main_sort_order);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "sshtunnel_port", remmina_pref.sshtunnel_port);
    g_key_file_set_boolean (gkeyfile, "remmina_pref", "applet_quick_ontop", remmina_pref.applet_quick_ontop);
    g_key_file_set_boolean (gkeyfile, "remmina_pref", "applet_hide_count", remmina_pref.applet_hide_count);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "recent_maximum", remmina_pref.recent_maximum);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "default_mode", remmina_pref.default_mode);
    g_key_file_set_integer (gkeyfile, "remmina_pref", "tab_mode", remmina_pref.tab_mode);

    content = g_key_file_to_data (gkeyfile, &length, NULL);
    g_file_set_contents (remmina_pref_file, content, length, NULL);

    g_key_file_free (gkeyfile);
    g_free (content);
}

void
remmina_pref_add_recent (const gchar *protocol, const gchar *server)
{
    RemminaStringArray *array;
    GKeyFile *gkeyfile;
    gchar key[20];
    gchar *val;
    gchar *content;
    gsize length;

    if (remmina_pref.recent_maximum <= 0 || server == NULL || server[0] == 0) return;

    /* Load original value into memory */
    gkeyfile = g_key_file_new ();

    g_key_file_load_from_file (gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

    g_snprintf (key, sizeof (key), "recent_%s", protocol);
    array = remmina_string_array_new_from_allocated_string (
        g_key_file_get_string (gkeyfile, "remmina_pref", key, NULL));

    /* Add the new value */
    remmina_string_array_remove (array, server);
    while (array->len >= remmina_pref.recent_maximum)
    {
        remmina_string_array_remove_index (array, 0);
    }
    remmina_string_array_add (array, server);

    /* Save */
    val = remmina_string_array_to_string (array);
    g_key_file_set_string (gkeyfile, "remmina_pref", key, val);
    g_free (val);

    content = g_key_file_to_data (gkeyfile, &length, NULL);
    g_file_set_contents (remmina_pref_file, content, length, NULL);

    g_key_file_free (gkeyfile);
    g_free (content);
}

gchar*
remmina_pref_get_recent (const gchar *protocol)
{
    GKeyFile *gkeyfile;
    gchar key[20];
    gchar *val;

    gkeyfile = g_key_file_new ();

    g_key_file_load_from_file (gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);

    g_snprintf (key, sizeof (key), "recent_%s", protocol);
    val = g_key_file_get_string (gkeyfile, "remmina_pref", key, NULL);

    g_key_file_free (gkeyfile);

    return val;
}

void
remmina_pref_clear_recent (void)
{
    GKeyFile *gkeyfile;
    gchar *content;
    gsize length;

    gkeyfile = g_key_file_new ();

    g_key_file_load_from_file (gkeyfile, remmina_pref_file, G_KEY_FILE_NONE, NULL);
    g_key_file_set_string (gkeyfile, "remmina_pref", "recent_RDP", "");
    g_key_file_set_string (gkeyfile, "remmina_pref", "recent_VNC", "");

    content = g_key_file_to_data (gkeyfile, &length, NULL);
    g_file_set_contents (remmina_pref_file, content, length, NULL);

    g_key_file_free (gkeyfile);
    g_free (content);
}

guint
remmina_pref_keymap_keyval (const gchar *keymap, guint keyval)
{
    guint *table;
    gint i;

    if (!keymap || keymap[0] == '\0') return keyval;

    table = (guint*) g_hash_table_lookup (remmina_keymap_table, keymap);
    if (!table) return keyval;
    for (i = 0; table[i] > 0; i += 2)
    {
        if (table[i] == keyval) return table[i + 1];
    }
    return keyval;
}

gchar*
remmina_pref_keymap_groups (void)
{
    GList *list;
    guint len;
    GString *keys;
    guint i;
    gboolean first;

    list = g_hash_table_get_keys (remmina_keymap_table);
    len = g_list_length (list);

    keys = g_string_new (NULL);
    first = TRUE;
    for (i = 0; i < len; i++)
    {
        if (first)
        {
            first = FALSE;
        }
        else
        {
            g_string_append (keys, ",");
        }
        g_string_append (keys, (gchar*) g_list_nth_data (list, i));
    }

    g_list_free (list);

    return g_string_free (keys, FALSE);
}

