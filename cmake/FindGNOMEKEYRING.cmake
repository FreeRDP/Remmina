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

find_path(GNOMEKEYRING_INCLUDE_DIR NAMES gnome-keyring.h
	PATH_SUFFIXES gnome-keyring-1)

find_library(GNOMEKEYRING_LIBRARY NAMES gnome-keyring)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(GNOMEKEYRING DEFAULT_MSG GNOMEKEYRING_LIBRARY GNOMEKEYRING_INCLUDE_DIR)

if(GNOMEKEYRING_FOUND)
	set(GNOMEKEYRING_LIBRARIES ${GNOMEKEYRING_LIBRARY})
	set(GNOMEKEYRING_INCLUDE_DIRS ${GNOMEKEYRING_INCLUDE_DIR})
endif()

mark_as_advanced(GNOMEKEYRING_INCLUDE_DIR GNOMEKEYRING_LIBRARY)

