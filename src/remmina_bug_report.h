/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2009-2011 Vic Lee
 * Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
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

#pragma once
#include <gtk/gtk.h>
#include "json-glib/json-glib.h"

typedef struct _RemminaBugReportDialog {
    GtkBuilder *builder;
    GtkWidget  *dialog;
    GtkButton  *bug_report_submit_button;
    GtkEntry   *bug_report_name_entry;
    GtkEntry   *bug_report_email_entry;
    GtkEntry   *bug_report_title_entry;
    GtkTextView *bug_report_description_textview;
    GtkLabel   *bug_report_submit_status_label;
    GtkCheckButton *bug_report_debug_data_check_button;
    GtkCheckButton *bug_report_include_system_info_check_button;
} RemminaBugReportDialog;

G_BEGIN_DECLS

void remmina_bug_report_open(GtkWindow *parent);
void remmina_bug_report_dialog_on_action_submit(GSimpleAction *action, GVariant *param, gpointer data);
JsonNode *remmina_bug_report_get_all(void);

G_END_DECLS
