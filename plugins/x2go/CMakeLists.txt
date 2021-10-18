#
#     Project: Remmina Plugin X2Go
# Description: Remmina protocol plugin to connect via X2Go using PyHoca
#              Based on Fabio Castelli Team Viewer Plugin
#              Copyright: 2013-2014 Fabio Castelli (Muflone)
#      Author: Antenore Gatta <antenore@simbiosi.org>
#   Copyright: 2015 Antenore Gatta
#     License: GPL-2+
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
#  In addition, as a special exception, the copyright holders give
#  permission to link the code of portions of this program with the
#  OpenSSL library under certain conditions as described in each
#  individual source file, and distribute linked combinations
#  including the two.
#  You must obey the GNU General Public License in all respects
#  for all of the code used other than OpenSSL. *  If you modify
#  file(s) with this exception, you may extend this exception to your
#  version of the file(s), but you are not obligated to do so. *  If you
#  do not wish to do so, delete this exception statement from your
#  version. *  If you delete this exception statement from all source
#  files in the program, then also delete it here.

set(REMMINA_PLUGIN_X2GO_SRCS
	x2go_plugin.c
	x2go_plugin.h
)

add_library(remmina-plugin-x2go ${REMMINA_PLUGIN_X2GO_SRCS})
set_target_properties(remmina-plugin-x2go PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-x2go PROPERTIES NO_SONAME 1)

find_package(X11)

include_directories(${REMMINA_COMMON_INCLUDE_DIRS}
    ${XKBFILE_INCLUDE_DIRS} ${LIBSSH_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-x2go
    ${REMMINA_COMMON_LIBRARIES}
    ${XKBFILE_LIBRARIES}
    ${LIBSSH_LIBRARIES}
    ${Intl_LIBRARIES}
    ${X11_X11_LIB})

install(TARGETS remmina-plugin-x2go DESTINATION ${REMMINA_PLUGINDIR})

set(APPICON16_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/16x16/emblems")
install(FILES 16x16/emblems/org.remmina.Remmina-x2go.png DESTINATION ${APPICON16_EMBLEMS_DIR})

set(APPICON22_EMBLEMS_DIR "${REMMINA_DATADIR}/icons/hicolor/22x22/emblems")
install(FILES 22x22/emblems/org.remmina.Remmina-x2go.png DESTINATION ${APPICON22_EMBLEMS_DIR})

gtk_update_icon_cache("${REMMINA_DATADIR}/icons/hicolor")
