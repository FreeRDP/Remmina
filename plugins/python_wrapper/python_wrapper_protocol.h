/*
 * Remmina - The GTK+ Remote Desktop Client
 * Copyright (C) 2014-2022 Antenore Gatta, Giovanni Panozzo, Mathias Winterhalter (ToolsDevler)
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
 * @file 	python_wrapper_protocol.h
 *
 * @brief	Contains the specialisation of RemminaPluginFile plugins in Python.
 */

#pragma once

#include "python_wrapper_common.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// I N C L U D E S
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "remmina/plugin.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// A P I
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

G_BEGIN_DECLS

/**
 * Wrapper for a Python object that contains a pointer to an instance of RemminaProtocolFeature.
 */
typedef struct
{
	PyObject_HEAD
	RemminaProtocolFeatureType type;
	gint id;
	PyGeneric* opt1;
	PyGeneric* opt2;
	PyGeneric* opt3;
} PyRemminaProtocolFeature;

/**
 *
 */
typedef struct
{
	PyObject_HEAD
	PyByteArrayObject* buffer;
	int bitsPerPixel;
	int bytesPerPixel;
	int width;
	int height;
} PyRemminaPluginScreenshotData;

/**
 * Initializes the Python plugin specialisation for protocol plugins.
 */
void python_wrapper_protocol_init(void);

/**
 * @brief	Creates a new instance of the RemminaPluginProtocol, initializes its members and references the wrapper
 * 			functions.
 *
 * @param 	instance The instance of the Python plugin.
 *
 * @return	Returns a new instance of the RemminaPlugin (must be freed!).
 */
RemminaPlugin* python_wrapper_create_protocol_plugin(PyPlugin* plugin);

/**
 *
 * @return
 */
PyRemminaProtocolFeature* python_wrapper_protocol_feature_new(void);

/**
 *
 * @return
 */
PyRemminaPluginScreenshotData* python_wrapper_screenshot_data_new(void);

G_END_DECLS
