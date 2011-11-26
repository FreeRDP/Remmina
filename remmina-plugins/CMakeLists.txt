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

include_directories(${CMAKE_SOURCE_DIR}/remmina-plugins)

find_required_package(GTK3)
if(GTK3_FOUND)
	set(REMMINA_COMMON_INCLUDE_DIRS ${GTK3_INCLUDE_DIRS})
	set(REMMINA_COMMON_LIBRARIES ${GTK3_LIBRARIES})
endif()

find_required_package(SSH)
find_required_package(XKBFILE)

if(SSH_FOUND AND XKBFILE_FOUND)
	add_subdirectory(nx)
endif()

add_subdirectory(xdmcp)

find_suggested_package(FREERDP)
if(FREERDP_FOUND)
	add_subdirectory(rdp)
endif()

find_suggested_package(TELEPATHY)
if(TELEPATHY_FOUND)
	add_subdirectory(telepathy)
endif()

find_suggested_package(ZLIB)

if(ZLIB_FOUND)
	add_subdirectory(libvncserver)
	add_subdirectory(vnc)
endif()

add_subdirectory(po)

