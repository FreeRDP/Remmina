# remmina-plugin-gtk - The GTK+ Remote Desktop Client
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

set(CMAKE_THREAD_PREFER_PTHREAD)
find_package(Threads)

set(REMMINA_PLUGIN_RDP_SRCS
	remminapluginrdp.c
	remminapluginrdp.h
	remminapluginrdpui.c
	remminapluginrdpui.h
	remminapluginrdpev.c
	remminapluginrdpev.h
	remminapluginrdpfile.c
	remminapluginrdpfile.h
	remminapluginrdpset.c
	remminapluginrdpset.h
	)

add_library(remmina-plugin-rdp ${REMMINA_PLUGIN_RDP_SRCS})

find_package(GTK3 REQUIRED)

if(GTK3_FOUND)
	add_definitions(-DWITH_GTK3)
	include_directories(${GTK3_INCLUDE_DIRS})
	target_link_libraries(remmina-plugin-rdp ${GTK3_LIBRARY_DIRS})
endif()

