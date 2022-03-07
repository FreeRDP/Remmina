# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2022 Antenore Gatta, Giovanni Panozzo, Matteo F. Vescovi
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
# PCRE2_INCLUDE_DIRS
# PCRE2_LIBRARIES
# PCRE2_CFLAGS

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
    pkg_check_modules(_PCRE2 libpcre2-8)
endif(PKG_CONFIG_FOUND)

find_library(PCRE2_LIB pcre
    HINTS
    ${_PCRE2_LIBRARY_DIRS}
    ${COMMON_LIB_DIR}
)

if(PCRE2_LIB)
    set(PCRE2_LIBRARIES ${PCRE2_LIB})
    message(STATUS "PCRE2-Libs: ${PCRE2_LIBRARIES}")
endif()

find_path(PCRE2_INCLUDE_DIR pcre2.h
    PATHS
    ${_PCRE2_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)


if(PCRE2_INCLUDE_DIR)
    set(PCRE2_INCLUDE_DIRS ${PCRE2_INCLUDE_DIR})
    message(STATUS "PCRE2-Include-Dirs: ${PCRE2_INCLUDE_DIRS}")
endif()

if(PCRE2_INCLUDE_DIRS)
    set(PCRE2_FOUND TRUE)
endif()
