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

if(GTK3_FOUND)
	set(_AVAHI_LIB_NAME avahi-ui-gtk3)
	set(_AVAHI_PKG_NAME avahi-ui-gtk3>=0.6.30 avahi-client>=0.6.30)
else()
	set(_AVAHI_LIB_NAME avahi-ui)
	set(_AVAHI_PKG_NAME avahi-ui>=0.6.30 avahi-client>=0.6.30)
endif()

find_package(PkgConfig)
pkg_check_modules(PC_AVAHI ${_AVAHI_PKG_NAME})

find_path(AVAHI_INCLUDE_DIR avahi-ui/avahi-ui.h
	HINTS ${PC_AVAHI_INCLUDEDIR} ${PC_AVAHI_INCLUDE_DIRS})

find_library(AVAHI_LIBRARY NAMES ${_AVAHI_LIB_NAME}
	HINTS ${PC_AVAHI_LIBDIR} ${PC_AVAHI_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(AVAHI DEFAULT_MSG AVAHI_LIBRARY AVAHI_INCLUDE_DIR)

set(AVAHI_LIBRARIES ${AVAHI_LIBRARY})
set(AVAHI_INCLUDE_DIRS ${AVAHI_INCLUDE_DIR})

mark_as_advanced(AVAHI_INCLUDE_DIR AVAHI_LIBRARY)

