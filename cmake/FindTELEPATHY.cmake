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

find_package(PkgConfig)
pkg_check_modules(PC_TELEPATHY telepathy-glib)
set(TELEPATHY_DEFINITIONS ${PC_TELEPATHY_CFLAGS_OTHER})

find_path(TELEPATHY_INCLUDE_DIR NAMES telepathy-glib/telepathy-glib.h
	HINTS ${PC_TELEPATHY_INCLUDEDIR} ${PC_TELEPATHY_INCLUDE_DIRS}
	PATH_SUFFIXES telepathy-1.0)

find_library(TELEPATHY_LIBRARY NAMES telepathy-glib)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(TELEPATHY DEFAULT_MSG TELEPATHY_LIBRARY TELEPATHY_INCLUDE_DIR)

set(TELEPATHY_LIBRARIES ${TELEPATHY_LIBRARY})
set(TELEPATHY_INCLUDE_DIRS ${TELEPATHY_INCLUDE_DIR})

mark_as_advanced(TELEPATHY_INCLUDE_DIR TELEPATHY_LIBRARY)

