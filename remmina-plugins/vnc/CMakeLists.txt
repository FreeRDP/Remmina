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

set(LIBVNCSERVER_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/libvncserver)
set(LIBVNCSERVER_LIBRARIES vncclient)

set(REMMINA_PLUGIN_VNC_SRCS
	vnc_plugin.c
	)

add_library(remmina-plugin-vnc ${REMMINA_PLUGIN_VNC_SRCS})
set_target_properties(remmina-plugin-vnc PROPERTIES PREFIX "")

include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${LIBVNCSERVER_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-vnc ${REMMINA_COMMON_LIBRARIES} ${LIBVNCSERVER_LIBRARIES})

install(TARGETS remmina-plugin-vnc DESTINATION ${REMMINA_PLUGINDIR})

install(FILES 16x16/emblems/remmina-vnc-ssh.png 16x16/emblems/remmina-vnc.png DESTINATION ${APPICON16_EMBLEMS_DIR})
install(FILES 22x22/emblems/remmina-vnc-ssh.png 22x22/emblems/remmina-vnc.png DESTINATION ${APPICON22_EMBLEMS_DIR})
