/**
 * FreeRDP: A Remote Desktop Protocol Implementation
 * Android Debug Interface
 *
 * Copyright 2013 Thincast Technologies GmbH, Author: Martin Fleisz
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef FREERDP_ANDROID_DEBUG_H
#define FREERDP_ANDROID_DEBUG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <freerdp/log.h>

#define ANDROID_TAG CLIENT_TAG("android")
#ifdef WITH_DEBUG_ANDROID_JNI
#define DEBUG_ANDROID(fmt, ...)	WLog_DBG(ANDROID_TAG, fmt, ## __VA_ARGS__)
#else
#define DEBUG_ANDROID(fmt, ...) do { } while (0)
#endif



#endif /* FREERDP_ANDROID_DEBUG_H */

