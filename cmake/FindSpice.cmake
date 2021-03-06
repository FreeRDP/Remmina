# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2016-2018 Denis Ollier
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

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
    #pkg_check_modules(PC_SPICE_PROTOCOL spice-protocol)
    pkg_check_modules(_SPICE spice-client-gtk-3.0)
endif(PKG_CONFIG_FOUND)

find_library(SPICE_CLIENT_GTK3_LIB NAMES spice-client-gtk-3.0
    PATHS
    ${_SPICE_LIBRARY_DIRS}
    ${COMMON_LIB_DIR}
)

find_library(SPICE_CLIENT_GLIB_LIB NAMES spice-client-glib-2.0
    PATHS
    ${_SPICE_LIBRARY_DIRS}
    ${COMMON_LIB_DIR}
)

if(SPICE_CLIENT_GTK3_LIB AND SPICE_CLIENT_GLIB_LIB)
    set(SPICE_LIBRARIES ${SPICE_CLIENT_GTK3_LIB} ${SPICE_CLIENT_GLIB_LIB})
    message(STATUS "Spice-Libs: ${SPICE_LIBRARIES}")
endif()


find_path(SPICE_CLIENT_GTK3_INCLUDE_DIR spice-client-gtk.h
    PATH_SUFFIXES spice-client-gtk-3.0
    PATHS
    ${_SPICE_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)

find_path(SPICE_CLIENT_GLIB_INCLUDE_DIR spice-client.h
    PATH_SUFFIXES spice-client-glib-2.0
    PATHS
    ${_SPICE_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)

find_path(SPICE_PROTO_INCLUDE_DIR spice/enums.h
    PATH_SUFFIXES spice-1
    PATHS
    ${_SPICE_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)


if(SPICE_CLIENT_GTK3_INCLUDE_DIR AND SPICE_CLIENT_GLIB_INCLUDE_DIR AND SPICE_PROTO_INCLUDE_DIR)
    set(SPICE_INCLUDE_DIRS ${SPICE_CLIENT_GTK3_INCLUDE_DIR} ${SPICE_CLIENT_GLIB_INCLUDE_DIR} ${SPICE_PROTO_INCLUDE_DIR})
    message(STATUS "Spice-Include-Dirs: ${SPICE_INCLUDE_DIRS}")
endif()

if(SPICE_LIBRARIES AND SPICE_INCLUDE_DIRS)
    set(SPICE_FOUND TRUE)
endif()
