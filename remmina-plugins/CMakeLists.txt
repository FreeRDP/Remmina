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
#
# In addition, as a special exception, the copyright holders give
# permission to link the code of portions of this program with the
# OpenSSL library under certain conditions as described in each
# individual source file, and distribute linked combinations
# including the two.
# You must obey the GNU General Public License in all respects
# for all of the code used other than OpenSSL. If you modify
# file(s) with this exception, you may extend this exception to your
# version of the file(s), but you are not obligated to do so. If you
# do not wish to do so, delete this exception statement from your
# version. If you delete this exception statement from all source
# files in the program, then also delete it here.


include_directories(${CMAKE_SOURCE_DIR}/remmina-plugins)

set(APPICON16_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/16x16/emblems")
set(APPICON22_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/22x22/emblems")

set(REMMINA_COMMON_INCLUDE_DIRS ${GTK_INCLUDE_DIRS})
set(REMMINA_COMMON_LIBRARIES ${GTK_LIBRARIES})

find_suggested_package(LIBSSH)
if(LIBSSH_FOUND)
	add_definitions(-DHAVE_LIBSSH)
	include_directories(${SSH_INCLUDE_DIRS})
	target_link_libraries(remmina ${SSH_LIBRARIES})
endif()

find_required_package(LIBVNCSERVER)

find_required_package(XKBFILE)

if(LIBSSH_FOUND AND XKBFILE_FOUND)
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
	add_subdirectory(vnc)
endif()

if(WITH_TRANSLATIONS)
	add_subdirectory(po)
endif()

