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
pkg_check_modules(PC_XKBFILE xkbfile)

find_path(XKBFILE_INCLUDE_DIR NAMES XKBfile.h
	HINTS ${PC_XKBFILE_INCLUDEDIR} ${PC_XKBFILE_INCLUDE_DIRS}
	PATH_SUFFIXES X11/extensions)

find_library(XKBFILE_LIBRARY NAMES xkbfile
	HINTS ${PC_XKBFILE_LIBDIR} ${PC_XKBFILE_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(XKBFILE DEFAULT_MSG XKBFILE_LIBRARY XKBFILE_INCLUDE_DIR)

set(XKBFILE_LIBRARIES ${XKBFILE_LIBRARY})
set(XKBFILE_INCLUDE_DIRS ${XKBFILE_INCLUDE_DIR})

mark_as_advanced(XKBFILE_INCLUDE_DIR XKBFILE_LIBRARY)

