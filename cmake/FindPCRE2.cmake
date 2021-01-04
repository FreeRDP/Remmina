# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2021 Antenore Gatta, Giovanni Panozzo, Matteo F. Vescovi
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

include(FindPackageHandleStandardArgs)

pkg_check_modules(PC_LIBPCRE2 libpcre2-8-0)


find_path(PCRE2_INCLUDE_DIR
  NAMES pcre2.h
  PATHS ../../../../libs
        /usr
  PATH_SUFFIXES include)

find_library(PCRE2_LIBRARY
  NAMES pcre2-8
  PATHS ../../../../libs
        /usr
  PATH_SUFFIXES lib)

if (PCRE2_INCLUDE_DIR AND PCRE2_LIBRARY)
  message(STATUS "Found pcre2 headers at ${PCRE2_INCLUDE_DIR}")
  message(STATUS "Found pcre2 libraries at ${PCRE2_LIBRARY}")
  set(PCRE2_INCLUDE_DIRS ${PCRE2_INCLUDE_DIR})
  set(PCRE2_LIBRARIES ${PCRE2_LIBRARY})
  set(PCRE2_FOUND yes)
else()
  set(PCRE2_INCLUDE_DIRS)
  set(PCRE2_LIBRARIES)
  set(PCRE2_FOUND no)
endif()
