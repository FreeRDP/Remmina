# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, 
# Boston, MA  02110-1301, USA.

include(FindPkgConfig)

if(PKG_CONFIG_FOUND)
	pkg_check_modules(PC_AVAHI_CLIENT avahi-client)
	if(GTK3_FOUND)
		set(_AVAHI_UI_LIB_NAME avahi-ui-gtk3)
		set(_AVAHI_UI_PKG_NAME avahi-ui-gtk3>=0.6.30 avahi-client>=0.6.30)
	endif()
	pkg_check_modules(PC_AVAHI_UI ${_AVAHI_UI_PKG_NAME})
endif()


find_library(AVAHI_COMMON_LIBRARY NAMES avahi-common PATHS ${PC_AVAHI_CLIENT_LIBRARY_DIRS})
if(AVAHI_COMMON_LIBRARY)
	set(AVAHI_COMMON_FOUND TRUE)
endif()

find_library(AVAHI_CLIENT_LIBRARY NAMES avahi-client PATHS ${PC_AVAHI_CLIENT_LIBRARY_DIRS})
if(AVAHI_CLIENT_LIBRARY)
	set(AVAHI_CLIENT_FOUND TRUE)
endif()

find_path(AVAHI_UI_INCLUDE_DIR avahi-ui/avahi-ui.h PATHS ${PC_AVAHI_UI_INCLUDE_DIRS})
find_library(AVAHI_UI_LIBRARY NAMES ${_AVAHI_UI_LIB_NAME} PATHS ${PC_AVAHI_UI_LIBRARY_DIRS})
if(AVAHI_UI_INCLUDE_DIR AND AVAHI_UI_LIBRARY)
	set(AVAHI_UI_FOUND TRUE)
endif()

FIND_PACKAGE_HANDLE_STANDARD_ARGS(AVAHI DEFAULT_MSG AVAHI_COMMON_FOUND AVAHI_CLIENT_FOUND AVAHI_UI_FOUND)

if (AVAHI_FOUND)
	set(AVAHI_INCLUDE_DIRS ${AVAHI_UI_INCLUDE_DIR})
	set(AVAHI_LIBRARIES ${AVAHI_COMMON_LIBRARY} ${AVAHI_CLIENT_LIBRARY} ${AVAHI_UI_LIBRARY})
endif()

mark_as_advanced(AVAHI_INCLUDE_DIRS AVAHI_LIBRARIES)
