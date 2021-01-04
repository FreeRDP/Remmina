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

# Try with ayatana-libappindicator
pkg_check_modules(PC_AYATANA_APPINDICATOR ayatana-appindicator3-0.1)

find_path(AYATANA_APPINDICATOR_INCLUDE_DIR NAMES libayatana-appindicator/app-indicator.h
	HINTS ${PC_AYATANA_APPINDICATOR_INCLUDEDIR} ${PC_AYATANA_APPINDICATOR_INCLUDE_DIRS}
	PATH_SUFFIXES libayatana-appindicator3-0.1)

find_library(AYATANA_APPINDICATOR_LIBRARY NAMES ayatana-appindicator3)

if (AYATANA_APPINDICATOR_INCLUDE_DIR AND AYATANA_APPINDICATOR_LIBRARY)
	find_package_handle_standard_args(APPINDICATOR DEFAULT_MSG AYATANA_APPINDICATOR_LIBRARY AYATANA_APPINDICATOR_INCLUDE_DIR)
endif()

if (APPINDICATOR_FOUND)
	add_definitions(-DHAVE_AYATANA_LIBAPPINDICATOR)
	set(APPINDICATOR_LIBRARIES ${AYATANA_APPINDICATOR_LIBRARY})
	set(APPINDICATOR_INCLUDE_DIRS ${AYATANA_APPINDICATOR_INCLUDE_DIR})
else()
	# Try with normal libappindicator
	pkg_check_modules(PC_APPINDICATOR appindicator3-0.1)

	find_path(APPINDICATOR_INCLUDE_DIR NAMES libappindicator/app-indicator.h
		HINTS ${PC_APPINDICATOR_INCLUDEDIR} ${PC_APPINDICATOR_INCLUDE_DIRS}
		PATH_SUFFIXES libappindicator3-0.1)

	find_library(APPINDICATOR_LIBRARY NAMES appindicator3)

	find_package_handle_standard_args(APPINDICATOR DEFAULT_MSG APPINDICATOR_LIBRARY APPINDICATOR_INCLUDE_DIR)

	if(APPINDICATOR_FOUND)
		set(APPINDICATOR_LIBRARIES ${APPINDICATOR_LIBRARY})
		set(APPINDICATOR_INCLUDE_DIRS ${APPINDICATOR_INCLUDE_DIR})
	endif()
endif()

mark_as_advanced(APPINDICATOR_INCLUDE_DIR APPINDICATOR_LIBRARY)

