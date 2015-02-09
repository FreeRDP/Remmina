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
# Foundation, Inc., 59 Temple Place, Suite 330, 
# Boston, MA 02111-1307, USA.

if(NOT _VTE_VERSION_NUM)
	set(_VTE_LIB_NAME vte)
	set(_VTE_VERSION vte)
else()
	if(${_VTE_VERSION_NUM} VERSION_LESS 2.91)
		set(_VTE_LIB_NAME vte${_VTE_VERSION_NUM})
	else()
		set(_VTE_LIB_NAME vte-${_VTE_VERSION_NUM})
	endif()
	set(_VTE_VERSION vte-${_VTE_VERSION_NUM})
endif()

if(${_VTE_VERSION_NUM} VERSION_LESS 2.91)
	string(REPLACE . _ _VTE_LIB_NAME ${_VTE_LIB_NAME})
endif()

pkg_check_modules(PC_VTE ${_VTE_VERSION})

find_path(VTE_INCLUDE_DIR NAMES vte/vte.h
	HINTS ${PC_VTE_INCLUDEDIR} ${PC_VTE_INCLUDE_DIRS}
	PATH_SUFFIXES ${_VTE_VERSION})

find_library(VTE_LIBRARY NAMES ${_VTE_LIB_NAME})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(VTE DEFAULT_MSG VTE_LIBRARY VTE_INCLUDE_DIR)

set(VTE_LIBRARIES ${VTE_LIBRARY})
set(VTE_INCLUDE_DIRS ${VTE_INCLUDE_DIR})

mark_as_advanced(VTE_INCLUDE_DIR VTE_LIBRARY)

