# remmina-plugin-vnc - The GTK+ Remote Desktop Client
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

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../libvncserver)

set(REMMINA_PLUGIN_VNC_SRCS
	remminapluginvnc.c
	)

add_library(remmina-plugin-vnc ${REMMINA_PLUGIN_VNC_SRCS})
set_target_properties(remmina-plugin-vnc PROPERTIES PREFIX "")

find_required_package(GTK3)
if(GTK3_FOUND)
	include_directories(${GTK3_INCLUDE_DIRS})
	target_link_libraries(remmina-plugin-vnc ${GTK3_LIBRARY_DIRS})
endif()

target_link_libraries(remmina-plugin-vnc vncclient)

install(TARGETS remmina-plugin-vnc DESTINATION ${REMMINA_PLUGINDIR})

