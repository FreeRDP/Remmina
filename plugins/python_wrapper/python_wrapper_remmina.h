/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2014-2022 Antenore Gatta, Giovanni Panozzo
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
 */

/**
 * @file 	python_wrapper_remmina.h
 *
 * @brief 	Contains the implementation of the Python module 'remmina', provided to interface with the application from
 * 			the Python plugin source.
 *
 * @detail 	In contrast to the wrapper functions that exist in the plugin specialisation files (e.g.
 * 			python_wrapper_protocol.c or python_wrapper_entry.c), this file contains the API for the
 * 			communication in the direction, from Python to Remmina. This means, if in the Python plugin a function
 * 			is called, that is defined in Remmina, C code, at least in this file, is executed.
 */

#pragma once

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I N C L U D E S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "python_wrapper_protocol_widget.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

G_BEGIN_DECLS

/**
 * Initializes the 'remmina' module in the Python engine.
 */
void python_wrapper_module_init(void);

/**
 * @brief 	Returns a pointer to the Python instance, mapped to the RemminaProtocolWidget or null if not found.
 *
 * @param 	gp The widget that is owned by the plugin that should be found.
 *
 * @details Remmina expects this callback function to be part of one plugin, which is the reason no instance information
 * 			is explicitly passed. To bridge that, this function can be used to retrieve the very plugin instance owning
 * 			the given RemminaProtocolWidget.
 */
PyPlugin* python_wrapper_module_get_plugin(RemminaProtocolWidget* gp);

/**
 * @brief 	Converts the PyObject to RemminaProtocolSetting.
 *
 * @param 	dest A target for the converted value.
 * @param 	setting The source value to convert.
 */
void python_wrapper_to_protocol_setting(RemminaProtocolSetting* dest, PyObject* setting);

/**
 * @brief 	Converts the PyObject to RemminaProtocolFeature.
 *
 * @param 	dest A target for the converted value.
 * @param 	setting The source value to convert.
 */
void python_wrapper_to_protocol_feature(RemminaProtocolFeature* dest, PyObject* feature);

G_END_DECLS
