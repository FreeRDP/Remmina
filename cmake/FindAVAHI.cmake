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

find_library(AVAHI_LIBRARY_UI NAMES avahi-ui-gtk3)

find_path(AVAHI_INCLUDE_DIR avahi-ui/avahi-ui.h)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(AVAHI DEFAULT_MSG AVAHI_LIBRARY_UI AVAHI_INCLUDE_DIR)

if(AVAHI_FOUND)
	set(AVAHI_LIBRARIES ${AVAHI_LIBRARY_UI})
	set(AVAHI_INCLUDE_DIRS ${AVAHI_INCLUDE_DIR})
endif()

mark_as_advanced(AVAHI_INCLUDE_DIR AVAHI_LIBRARY)

