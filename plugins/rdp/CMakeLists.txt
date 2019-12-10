# remmina-plugin-rdp - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2019 Antenore Gatta, Giovanni Panozzo
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


set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
find_package(X11)

find_suggested_package(Cups)

set(REMMINA_PLUGIN_RDP_SRCS
        rdp_plugin.c
        rdp_plugin.h
        rdp_event.c
        rdp_event.h
        rdp_file.c
        rdp_file.h
        rdp_settings.c
        rdp_settings.h
        rdp_graphics.c
        rdp_graphics.h
        rdp_cliprdr.c
        rdp_cliprdr.h
        rdp_channels.c
        rdp_channels.h
        )

add_definitions(-DFREERDP_REQUIRED_MAJOR=${FREERDP_REQUIRED_MAJOR})
add_definitions(-DFREERDP_REQUIRED_MINOR=${FREERDP_REQUIRED_MINOR})
add_definitions(-DFREERDP_REQUIRED_REVISION=${FREERDP_REQUIRED_REVISION})

option(WITH_FREERDP_MASTER "Compile the RDP plugin using symbols from FreeRDP master branch." OFF)
if(WITH_FREERDP_MASTER)
	message(STATUS "Compiling the RDP plugin using symbols from FreeRDP master branch.")
	add_definitions(-DWITH_FREERDP_MASTER=ON)
endif()

add_library(remmina-plugin-rdp MODULE ${REMMINA_PLUGIN_RDP_SRCS})
set_target_properties(remmina-plugin-rdp PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-rdp PROPERTIES NO_SONAME 1)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${FREERDP_INCLUDE_DIRS} ${X11_INCLUDE_DIR})
target_link_libraries(remmina-plugin-rdp
    ${REMMINA_COMMON_LIBRARIES} ${FREERDP_LIBRARIES} ${X11_LIBRARIES})

if(CUPS_FOUND)
    add_definitions(-DHAVE_CUPS)
    include_directories(${CUPS_INCLUDE_DIR})
    target_link_libraries(remmina-plugin-rdp
        ${REMMINA_COMMON_LIBRARIES} ${FREERDP_LIBRARIES} ${X11_LIBRARIES} ${CUPS_LIBRARIES})
endif()

install(TARGETS remmina-plugin-rdp DESTINATION ${REMMINA_PLUGINDIR})

install(FILES
    scalable/emblems/remmina-rdp-ssh-symbolic.svg
    scalable/emblems/remmina-rdp-symbolic.svg
    DESTINATION ${APPICONSCALE_EMBLEMS_DIR})

if(WITH_ICON_CACHE)
    gtk_update_icon_cache("${REMMINA_DATADIR}/icons/hicolor")
endif()
