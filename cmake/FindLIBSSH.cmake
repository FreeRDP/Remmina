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
pkg_check_modules(PC_LIBSSH libssh>=0.4)
set(LIBSSH_DEFINITIONS ${PC_LIBSSH_CFLAGS_OTHER})

find_path(LIBSSH_INCLUDE_DIR NAMES libssh/libssh.h
	HINTS ${PC_LIBSSH_INCLUDEDIR} ${PC_LIBSSH_INCLUDE_DIRS})

find_library(LIBSSH_LIBRARY NAMES ssh
	HINTS ${PC_LIBSSH_LIBDIR} ${PC_LIBSSH_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBSSH DEFAULT_MSG LIBSSH_LIBRARY LIBSSH_INCLUDE_DIR)

set(LIBSSH_LIBRARIES ${LIBSSH_LIBRARY})
set(LIBSSH_INCLUDE_DIRS ${LIBSSH_INCLUDE_DIR})

mark_as_advanced(LIBSSH_INCLUDE_DIR LIBSSH_LIBRARY)

