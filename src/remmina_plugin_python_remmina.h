/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2014-2021 Antenore Gatta, Giovanni Panozzo
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

/**
 * @brief The Python abstraction of the protocol widget struct.
 *
 * @details This struct is responsible to provide the same accessibility to the protocol widget for Python as for
 * native plugins.
 */
typedef struct {
    PyObject_HEAD
    RemminaProtocolWidget* gp;
} PyRemminaProtocolWidget;

/**
 * @brief Maps an instance of a Python plugin to a Remmina one.
 *
 * @details This is used to map a Python plugin instance to the Remmina plugin one. Also instance specific data as the
 * protocol widget are stored in this struct.
 */
typedef struct {
    PyObject *pythonInstance;

    RemminaProtocolPlugin *protocol_plugin;
    RemminaFilePlugin* file_plugin;
    RemminaSecretPlugin* secret_plugin;
    RemminaToolPlugin* tool_plugin;
    RemminaEntryPlugin* entry_plugin;
    RemminaPrefPlugin* pref_plugin;
    RemminaPlugin* generic_plugin;

    PyRemminaProtocolWidget *gp;
} PyPlugin;



/**
 * @brief Initializes the 'remmina' module in the Python engine.
 */
void remmina_plugin_python_module_init(void);

/**
 * @brief Returns a pointer to the Python instance, mapped to the RemminaProtocolWidget or null if not found.
 *
 * @details Remmina expects this callback function to be part of one plugin, which is the reason no instance information is explicitly passed. To bridge
 * that, this function can be used to retrieve the very plugin instance owning the given RemminaProtocolWidget.
 */
PyPlugin* remmina_plugin_python_module_get_plugin(RemminaProtocolWidget* gp);

void ToRemminaProtocolSetting(RemminaProtocolSetting* dest, PyObject* setting);
