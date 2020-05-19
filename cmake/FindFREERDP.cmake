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

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(PC_FREERDP freerdp2>=2.0.0)
endif()

set(FREERDP_DEFINITIONS ${PC_FREERDP_CFLAGS_OTHER})

find_path(FREERDP_INCLUDE_DIR NAMES freerdp/freerdp.h
	HINTS ${PC_FREERDP_INCLUDEDIR} ${PC_FREERDP_INCLUDE_DIRS} ${CMAKE_PREFIX_PATH}/include/freerdp2/)

find_path(WINPR_INCLUDE_DIR NAMES winpr/winpr.h
	HINTS ${PC_FREERDP_INCLUDEDIR} ${PC_FREERDP_INCLUDE_DIRS} ${CMAKE_PREFIX_PATH}/include/winpr2/)

find_library(FREERDP_LIBRARY NAMES freerdp2
	HINTS ${PC_FREERDP_LIBDIR} ${PC_FREERDP_LIBRARY_DIRS})

if(NOT FREERDP_LIBRARY)
  find_library(FREERDP_LIBRARY NAMES freerdp
    HINTS ${PC_FREERDP_LIBDIR} ${PC_FREERDP_LIBRARY_DIRS})
endif()

find_library(FREERDP_CLIENT_LIBRARY NAMES freerdp-client2
	HINTS ${PC_FREERDP_LIBDIR} ${PC_FREERDP_LIBRARY_DIRS})

if(NOT FREERDP_CLIENT_LIBRARY)
  find_library(FREERDP_CLIENT_LIBRARY NAMES freerdp-client
    HINTS ${PC_FREERDP_LIBDIR} ${PC_FREERDP_LIBRARY_DIRS})
endif()

find_library(FREERDP_WINPR_LIBRARY NAMES winpr2
	HINTS ${PC_FREERDP_LIBDIR} ${PC_FREERDP_LIBRARY_DIRS})

if(NOT FREERDP_WINPR_LIBRARY)
  find_library(FREERDP_WINPR_LIBRARY NAMES winpr
    HINTS ${PC_FREERDP_LIBDIR} ${PC_FREERDP_LIBRARY_DIRS})
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(FREERDP DEFAULT_MSG FREERDP_LIBRARY FREERDP_INCLUDE_DIR)

set(FREERDP_LIBRARIES ${FREERDP_LIBRARY} ${FREERDP_CLIENT_LIBRARY} ${FREERDP_WINPR_LIBRARY} )
set(FREERDP_INCLUDE_DIRS ${FREERDP_INCLUDE_DIR} ${WINPR_INCLUDE_DIR})

mark_as_advanced(FREERDP_INCLUDE_DIR FREERDP_LIBRARY)
