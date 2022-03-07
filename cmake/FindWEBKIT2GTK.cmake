# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2015-2022 Antenore Gatta, Giovanni Panozzo
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
    pkg_check_modules(_WEBKIT2GTK webkit2gtk-4.0)
endif(PKG_CONFIG_FOUND)

find_library(WEBKIT2GTK_LIB webkit2gtk-4.0
    HINTS
    ${_WEBKIT2GTK_LIBRARY_DIRS}
    ${COMMON_LIB_DIR}
)

if(WEBKIT2GTK_LIB)
    set(WEBKIT2GTK_LIBRARIES ${WEBKIT2GTK_LIB})
    message(STATUS "WEBKIT2GTK-Libs: ${WEBKIT2GTK_LIBRARIES}")
endif()

find_path(WEBKIT2GTK_INCLUDE_DIR webkit2.h
    PATH_SUFFIXES webkit2
    PATHS
    ${_WEBKIT2GTK_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)


if(WEBKIT2GTK_INCLUDE_DIR)
    set(WEBKIT2GTK_INCLUDE_DIRS ${WEBKIT2GTK_INCLUDE_DIR})
    message(STATUS "WEBKIT2GTK-Include-Dirs: ${WEBKIT2GTK_INCLUDE_DIRS}")
endif()

if(WEBKIT2GTK_INCLUDE_DIRS)
    set(WEBKIT2GTK_FOUND TRUE)
endif()
