# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
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

# Needed FreeRDP version to build freerdp plugin


include_directories(${CMAKE_SOURCE_DIR}/plugins)

set(APPICONSCALE_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/scalable/emblems")

set(REMMINA_COMMON_INCLUDE_DIRS ${GTK_INCLUDE_DIRS})
set(REMMINA_COMMON_LIBRARIES ${GTK_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

find_suggested_package(LIBSSH)
if(LIBSSH_FOUND)
    add_definitions(-DHAVE_LIBSSH)
    include_directories(${SSH_INCLUDE_DIRS})
    target_link_libraries(remmina ${SSH_LIBRARIES})
endif()

find_required_package(XKBFILE)

if(LIBSSH_FOUND AND XKBFILE_FOUND)
    add_subdirectory(nx)
endif()

add_subdirectory(xdmcp)

set(FREERDP_REQUIRED_MAJOR 2)
set(FREERDP_REQUIRED_MINOR 0)
set(FREERDP_REQUIRED_REVISION 0)
set(FREERDP_REQUIRED_VERSIONSTRING
    ${FREERDP_REQUIRED_MAJOR}.${FREERDP_REQUIRED_MINOR}.${FREERDP_REQUIRED_REVISION})
option(WITH_FREERDP3 "Building RDP with FREERDP 3" OFF)
if(WITH_FREERDP3)
	set(FREERDP_REQUIRED_MAJOR 3)
	find_suggested_package(FREERDP3)
	if(FREERDP3_FOUND)
		set(FREERDP_FOUND FREERDP3_FOUND)
	endif()
else()
	find_suggested_package(FREERDP)
endif()
if(FREERDP_FOUND)
    add_subdirectory(rdp)
endif()

#find_suggested_package(TELEPATHY)
#if(TELEPATHY_FOUND)
#add_subdirectory(telepathy)
#endif()

find_suggested_package(LIBVNCSERVER)
if(LIBVNCSERVER_FOUND)
    add_subdirectory(vnc)
endif()

find_suggested_package(SPICE)
if(SPICE_FOUND)
    add_subdirectory(spice)
endif()

option(WITH_WWW "Build WWW plugin" ON)
if(WITH_WWW)
    message(STATUS "Enabling WWW plugin.")
    add_definitions(-DWITH_WWW)
    pkg_search_module(WEBKIT2
        webkit2gtk-4.0
        webkit2gtk-3.0
        libwebkit2gtk-4.0
        libwebkit2gtk-3.0
        )
    if(WEBKIT2_FOUND)
        message(STATUS "WWW plugin dependencies found")
        add_subdirectory(www)
    else(WEBKIT2_FOUND)
        message(WARNING "WEBKIT not found")
    endif(WEBKIT2_FOUND)
else()
    message(STATUS "Disabling WWW plugin.")
endif()

if(WITH_EXAMPLES)
    message(STATUS "Enabling examples and test plugins.")
    add_subdirectory(tool_hello_world)
endif()
add_subdirectory(exec)
add_subdirectory(st)

CheckHasModule(KF5Wallet)
if(HAS_MODULE_KF5Wallet)
    message(STATUS "Enabling KDE Wallet plugin.")
    add_definitions(-DWITH_KF5WALLET)
    add_subdirectory(kwallet)
endif()
