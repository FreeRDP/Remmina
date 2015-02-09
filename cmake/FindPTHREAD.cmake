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
pkg_check_modules(PC_PTHREAD pthread)
set(PTHREAD_DEFINITIONS ${PC_PTHREAD_CFLAGS_OTHER})

find_path(PTHREAD_INCLUDE_DIR NAMES pthread.h
	HINTS ${PC_PTHREAD_INCLUDEDIR} ${PC_PTHREAD_INCLUDE_DIRS})

find_library(PTHREAD_LIBRARY NAMES pthread
	HINTS ${PC_PTHREAD_LIBDIR} ${PC_PTHREAD_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PTHREAD DEFAULT_MSG PTHREAD_LIBRARY PTHREAD_INCLUDE_DIR)

set(PTHREAD_LIBRARIES ${PTHREAD_LIBRARY})
set(PTHREAD_INCLUDE_DIRS ${PTHREAD_INCLUDE_DIR})

mark_as_advanced(PTHREAD_INCLUDE_DIR PTHREAD_LIBRARY)

