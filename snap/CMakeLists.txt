# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2017 Marco Trevisan
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

set(SNAP_NAME "remmina")
set(SNAP_VERSION ${REMMINA_VERSION})
if(NOT ${GIT_REVISION} STREQUAL ${REMMINA_VERSION_SUFFIX} AND
        NOT ${GIT_REVISION} MATCHES "^v?[0-9].[0-9].[0-9](-rcgit.[0-9]*)?$")
    set(SNAP_VERSION ${SNAP_VERSION}+git${GIT_REVISION})
endif()

set(SNAP_BUILD_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(SNAP_SETUP_DIR ${SNAP_BUILD_DIR}/snap)
set(SNAP_GUI_DIR ${SNAP_SETUP_DIR}/gui)

file(MAKE_DIRECTORY ${SNAP_GUI_DIR})
file(RELATIVE_PATH CMAKE_SOURCE_DIR_RELATIVE ${SNAP_BUILD_DIR} ${CMAKE_SOURCE_DIR})

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/snapcraft.yaml.in
    ${SNAP_BUILD_DIR}/snapcraft.yaml @ONLY)

set(REMMINA_BINARY_PATH "remmina")
set(REMMINA_ICON "\${SNAP}/meta/gui/icon.svg")
configure_file(${CMAKE_SOURCE_DIR}/remmina/desktop/remmina.desktop.in
    ${SNAP_GUI_DIR}/remmina.desktop @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/LICENSE
    ${SNAP_SETUP_DIR}/license.txt COPYONLY)

add_custom_target("snap-prime"
    COMMAND ${SNAPCRAFT} prime
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/snapcraft.yaml"
    COMMENT "Priming snap package ${SNAP_VERSION}"
    VERBATIM
    )

add_custom_target("snap"
    COMMAND ${SNAPCRAFT}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/snapcraft.yaml"
    COMMENT "Create snap package ${SNAP_VERSION}"
    VERBATIM
    )

foreach(channel stable candidate beta edge)
    add_custom_target("snap-push-${channel}"
        COMMAND bash -c "${SNAPCRAFT} push --release ${channel} *.snap"
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/snapcraft.yaml"
        COMMENT "Create snap package ${SNAP_VERSION}"
        VERBATIM
        )
    add_dependencies("snap-push-${channel}" "snap")
endforeach()

add_custom_target("snap-push-transfer.sh"
    COMMAND bash -c "snap=$(ls *.snap -1 | head -n1); \
    curl --upload-file $snap \
    https://transfer.sh/$(basename $snap)"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/snapcraft.yaml"
    COMMENT "Create snap package ${SNAP_VERSION}"
    VERBATIM
    )
add_dependencies("snap-push-transfer.sh" "snap")
