# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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

pkg_check_modules(PC_JSONGLIB json-glib-1.0)

find_path(JSONGLIB_INCLUDE_DIR NAMES json-glib/json-glib.h
	HINTS ${PC_JSONGLIB_INCLUDEDIR} ${PC_JSONGLIB_INCLUDE_DIRS}
)

find_library(JSONGLIB_LIBRARY NAMES json-glib-1.0
	HINTS ${PC_JSONGLIB_LIBDIR} ${PC_JSONGLIB_LIBRARY_DIRS}
)

if (JSONGLIB_INCLUDE_DIR AND JSONGLIB_LIBRARY)
	find_package_handle_standard_args(JSONGLIB DEFAULT_MSG JSONGLIB_LIBRARY JSONGLIB_INCLUDE_DIR)
endif()

if (JSONGLIB_FOUND)
	set(JSONGLIB_LIBRARIES ${JSONGLIB_LIBRARY})
	set(JSONGLIB_INCLUDE_DIRS ${JSONGLIB_INCLUDE_DIR})
endif()

mark_as_advanced(JSONGLIB_INCLUDE_DIR JSONGLIB_LIBRARY)

