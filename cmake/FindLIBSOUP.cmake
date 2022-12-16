# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2016-2023 Antenore Gatta, Giovanni Panozzo
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

include(FindPackageHandleStandardArgs)

pkg_search_module(PC_LIBSOUP REQUIRED libsoup-3.0 libsoup-2.4)

find_path(LIBSOUP_INCLUDE_DIR NAMES libsoup/soup.h
	HINTS ${PC_LIBSOUP_INCLUDEDIR} ${PC_LIBSOUP_INCLUDE_DIRS}
)

find_library(LIBSOUP_LIBRARY
	NAMES soup soup-3.0 soup-2.4
	HINTS ${PC_LIBSOUP_LIBDIR} ${PC_LIBSOUP_LIBRARY_DIRS}
	)

if (LIBSOUP_INCLUDE_DIR AND LIBSOUP_LIBRARY)
	find_package_handle_standard_args(LIBSOUP DEFAULT_MSG LIBSOUP_LIBRARY LIBSOUP_INCLUDE_DIR)
endif()

if (LIBSOUP_FOUND)
	set(LIBSOUP_LIBRARIES ${LIBSOUP_LIBRARY})
	set(LIBSOUP_INCLUDE_DIRS ${LIBSOUP_INCLUDE_DIR})
endif()

mark_as_advanced(LIBSOUP_INCLUDE_DIR LIBSOUP_LIBRARY)

