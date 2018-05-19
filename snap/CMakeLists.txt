# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2017-2018 Marco Trevisan
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
if(NOT ${GIT_REVISION} MATCHES "^v?[0-9].[0-9].[0-9][0-9]?$")
    set(SNAP_VERSION ${SNAP_VERSION}+git${GIT_REVISION})
endif()

if(DEFINED ENV{SNAPCRAFT_PROJECT_NAME})
    # We're building in snapcraft so, the source dir will be
    # parts/remmina/src
    set(SNAP_DIR ${CMAKE_SOURCE_DIR}/../../../snap)
else()
    set(SNAP_DIR ${CMAKE_CURRENT_SOURCE_DIR})
endif()

set(SNAP_GUI_DIR ${SNAP_DIR}/gui)

file(MAKE_DIRECTORY ${SNAP_GUI_DIR})
file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/gui)
file(RELATIVE_PATH CMAKE_SOURCE_DIR_RELATIVE ${SNAP_DIR}
    ${CMAKE_SOURCE_DIR})

configure_file(${SNAP_DIR}/SNAP_VERSION.in
    ${SNAP_DIR}/SNAP_VERSION @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/LICENSE
    ${SNAP_GUI_DIR}/license.txt COPYONLY)

add_custom_target("snap-version-script" ALL
    DEPENDS "${SNAP_DIR}/SNAP_VERSION"
    COMMENT "Preparing snap version file ${SNAP_VERSION}"
    VERBATIM
    )

add_custom_target("snap-prime"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMAND ${SNAPCRAFT} prime
    DEPENDS "${SNAP_DIR}/SNAP_VERSION"
    COMMENT "Priming snap package ${SNAP_VERSION}"
    VERBATIM
    )

add_custom_target("snap"
    COMMAND ${SNAPCRAFT}
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    DEPENDS "${SNAP_DIR}/SNAP_VERSION"
    COMMENT "Create snap package ${SNAP_VERSION}"
    VERBATIM
    )

foreach(channel stable candidate beta edge)
    add_custom_target("snap-push-${channel}"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMAND bash -c "${SNAPCRAFT} push --release ${channel} *.snap"
        DEPENDS "${SNAP_DIR}/SNAP_VERSION"
        COMMENT "Create snap package ${SNAP_VERSION}"
        VERBATIM
        )
    add_dependencies("snap-push-${channel}" "snap")
endforeach()

add_custom_target("snap-push-transfer.sh"
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
    COMMAND bash -c "snap=$(ls *.snap -1 | head -n1); \
    curl --upload-file $snap \
    https://transfer.sh/$(basename $snap); echo"
    DEPENDS "${SNAP_DIR}/SNAP_VERSION"
    COMMENT "Uploading snap package to transfer.sh ${SNAP_VERSION}"
    VERBATIM
    )
add_dependencies("snap-push-transfer.sh" "snap")
