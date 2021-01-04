# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2021 Antenore Gatta, Giovanni Panozzo
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

pkg_check_modules(PC_LIBSOUP24 libsoup-2.4)


find_path(LIBSOUP24_INCLUDE_DIR NAMES libsoup/soup.h
	HINTS ${PC_LIBSOUP24_INCLUDEDIR} ${PC_LIBSOUP24_INCLUDE_DIRS}
)

find_library(LIBSOUP24_LIBRARY
	NAMES soup-2.4
	HINTS ${PC_LIBSOUP24_LIBDIR} ${PC_LIBSOUP24_LIBRARY_DIRS}
	)

if (LIBSOUP24_INCLUDE_DIR AND LIBSOUP24_LIBRARY)
	find_package_handle_standard_args(LIBSOUP24 DEFAULT_MSG LIBSOUP24_LIBRARY LIBSOUP24_INCLUDE_DIR)
endif()

if (LIBSOUP24_FOUND)
	set(LIBSOUP24_LIBRARIES ${LIBSOUP24_LIBRARY})
	set(LIBSOUP24_INCLUDE_DIRS ${LIBSOUP24_INCLUDE_DIR})
endif()

mark_as_advanced(LIBSOUP24_INCLUDE_DIR LIBSOUP24_LIBRARY)

