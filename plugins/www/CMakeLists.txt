# remmina-plugin-www - The GTK+ Remote Desktop Client
#
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


set(REMMINA_PLUGIN_WWW_SRCS
	www_utils.c
	www_utils.h
        www_config.h
        www_plugin.c
        www_plugin.h
)

add_library(remmina-plugin-www MODULE ${REMMINA_PLUGIN_WWW_SRCS})
set_target_properties(remmina-plugin-www PROPERTIES PREFIX "")
set_target_properties(remmina-plugin-www PROPERTIES NO_SONAME 1)

add_definitions(${WEBKIT2_CFLAGS_OTHER})
include_directories(${REMMINA_COMMON_INCLUDE_DIRS} ${GTK_INCLUDE_DIRS} ${WEBKIT2_INCLUDE_DIRS})
target_link_libraries(remmina-plugin-www ${REMMINA_COMMON_LIBRARIES} ${WEBKIT2_LIBRARIES})

install(TARGETS remmina-plugin-www DESTINATION ${REMMINA_PLUGINDIR})

install(FILES
    scalable/emblems/remmina-www-symbolic.svg
    DESTINATION ${APPICONSCALE_EMBLEMS_DIR})

if(WITH_ICON_CACHE)
    gtk_update_icon_cache("${REMMINA_DATADIR}/icons/hicolor")
endif()

# In the feature we will have more resources to add, so I use a GLOB
file(GLOB WWW_RES_FILES "${CMAKE_CURRENT_SOURCE_DIR}/resources/js/www-js.js")
foreach(ITEM ${WWW_RES_FILES})
    list( APPEND FILES_TO_DEPLOY "${ITEM}" )
endforeach()
install( FILES ${FILES_TO_DEPLOY} DESTINATION "${REMMINA_RESDIR}" )

