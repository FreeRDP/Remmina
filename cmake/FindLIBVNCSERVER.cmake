# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2012 Luca Falavigna
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
pkg_check_modules(PC_LIBVNCSERVER libvncserver)
set(LIBVNCSERVER_DEFINITIONS ${PC_LIBVNCSERVER_CFLAGS_OTHER})

find_path(LIBVNCSERVER_INCLUDE_DIR NAMES rfb/rfb.h
	HINTS ${PC_LIBVNCSERVER_INCLUDEDIR} ${PC_LIBVNCSERVER_INCLUDE_DIRS})

find_library(LIBVNCSERVER_LIBRARY NAMES vncserver
	HINTS ${PC_LIBVNCSERVER_LIBDIR} ${PC_LIBVNCSERVER_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(LIBVNCSERVER DEFAULT_MSG LIBVNCSERVER_LIBRARY LIBVNCSERVER_INCLUDE_DIR)

set(LIBVNCSERVER_LIBRARIES ${LIBVNCSERVER_LIBRARY})
set(LIBVNCSERVER_INCLUDE_DIRS ${LIBVNCSERVER_INCLUDE_DIR})

mark_as_advanced(LIBVNCSERVER_INCLUDE_DIR LIBVNCSERVER_LIBRARY)
