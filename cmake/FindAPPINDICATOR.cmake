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

pkg_check_modules(PC_APPINDICATOR appindicator3-0.1)

find_path(APPINDICATOR_INCLUDE_DIR NAMES libappindicator/app-indicator.h
	HINTS ${PC_APPINDICATOR_INCLUDEDIR} ${PC_APPINDICATOR_INCLUDE_DIRS}
	PATH_SUFFIXES libappindicator-0.1)

find_library(APPINDICATOR_LIBRARY NAMES appindicator3)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(APPINDICATOR DEFAULT_MSG APPINDICATOR_LIBRARY APPINDICATOR_INCLUDE_DIR)

if(APPINDICATOR_FOUND)
	set(APPINDICATOR_LIBRARIES ${APPINDICATOR_LIBRARY})
	set(APPINDICATOR_INCLUDE_DIRS ${APPINDICATOR_INCLUDE_DIR})
endif()

mark_as_advanced(APPINDICATOR_INCLUDE_DIR APPINDICATOR_LIBRARY)

