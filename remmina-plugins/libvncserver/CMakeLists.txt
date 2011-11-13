# libvncclient - The GTK+ Remote Desktop Client
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/rfb/rfbconfig.h.in ${CMAKE_CURRENT_SOURCE_DIR}/rfb/rfbconfig.h)

set(REMMINA_VNC_CLIENT_SRCS
	libvncclient/cursor.c
	libvncclient/listen.c
	common/lzoconf.h
	common/minilzo.c
	common/minilzo.h
	libvncclient/rfbproto.c
	libvncclient/sockets.c
	libvncclient/tls.c
	libvncclient/tls.h
	libvncclient/tight.c
	libvncclient/vncviewer.c
	libvncclient/zlib.c
	libvncclient/zrle.c
	)

add_library(vncclient ${REMMINA_VNC_CLIENT_SRCS})

find_package(GTK3 REQUIRED)

if(GTK3_FOUND)
	add_definitions(-DWITH_GTK3)
	include_directories(${GTK3_INCLUDE_DIRS})
	target_link_libraries(vncclient ${GTK3_LIBRARY_DIRS})
endif()

