# remmina-plugin-telepathy - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2020 Antenore Gatta, Giovanni Panozzo
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


set(REMMINA_PLUGIN_TELEPATHY_SRCS
        telepathy_plugin.c
        telepathy_handler.c
        telepathy_channel_handler.c
        )

add_library(remmina-plugin-telepathy MODULE ${REMMINA_PLUGIN_TELEPATHY_SRCS})
set_target_properties(remmina-plugin-telepathy PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-telepathy PROPERTIES NO_SONAME 1)

find_required_package(GTK3)
pkg_check_modules(PC_DBUS_GLIB dbus-glib-1)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${TELEPATHY_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-telepathy ${REMMINA_COMMON_LIBRARIES}
    ${TELEPATHY_LIBRARIES} ${PC_DBUS_GLIB_LIBRARIES})

install(TARGETS remmina-plugin-telepathy DESTINATION ${REMMINA_PLUGINDIR})

# Telepathy client file
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/Remmina.client
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/telepathy/clients)

# DBus activation file
if(NOT REMMINA_BINARY_PATH)
    set(REMMINA_BINARY_PATH ${CMAKE_INSTALL_FULL_BINDIR}/remmina)
endif()
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/org.freedesktop.Telepathy.Client.Remmina.service.in
    ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.Remmina.service @ONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/org.freedesktop.Telepathy.Client.Remmina.service
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/dbus-1/services)
