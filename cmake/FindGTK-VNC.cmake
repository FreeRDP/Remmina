# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2015-2021 Antenore Gatta
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
    pkg_check_modules(_GVNC gvnc-1.0)
    pkg_check_modules(_GVNCPULSE gvncpulse-1.0)
    pkg_check_modules(_GTK-VNC gtk-vnc-2.0)
endif(PKG_CONFIG_FOUND)

# libgtk-vnc-2.0.so contains the GTK+ 3 bindings for Gtk VNC.
# libgvnc-1.0.so contains the GObject bindings for Gtk VNC.
# libgvncpulse-1.0.so is the PulseAudio bridge for Gtk VNC.

find_library(GVNC_LIB gvnc-1.0
    HINTS
    ${_GVNC_LIBRARY_DIRS}
    ${COMMON_LIB_DIR}
)

find_library(GVNCPULSE_LIB gvncpulse-1.0
    HINTS
    ${_GVNCPULSE_LIBRARY_DIRS}
    ${COMMON_LIB_DIR}
)

find_library(GTK-VNC_LIB gtk-vnc-2.0
    HINTS
    ${_GTK-VNC_LIBRARY_DIRS}
    ${COMMON_LIB_DIR}
)

if(GVNC_LIB AND GVNCPULSE_LIB AND GTK-VNC_LIB)
    set(GTK-VNC_LIBRARIES ${GVNC_LIB} ${GVNCPULSE_LIB} ${GTK-VNC_LIB})
    message(STATUS "GTK-VNC-Libs: ${GTK-VNC_LIBRARIES}")
endif()

find_path(GVNC_INCLUDE_DIR gvnc.h
    PATH_SUFFIXES gvnc-1.0
    PATHS
    ${_GVNC_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)

find_path(GVNCPULSE_INCLUDE_DIR gvncpulse.h
    PATH_SUFFIXES gvncpulse-1.0
    PATHS
    ${_GVNCPULSE_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)

find_path(GTK-VNC_INCLUDE_DIR gtk-vnc.h
    PATH_SUFFIXES gtk-vnc-2.0
    PATHS
    ${_GTK-VNC_INCLUDE_DIRS}
    ${COMMON_INCLUDE_DIR}
)

if(GVNC_INCLUDE_DIR AND GVNCPULSE_INCLUDE_DIR AND GTK-VNC_INCLUDE_DIR)
    set(GTK-VNC_INCLUDE_DIRS ${GVNC_INCLUDE_DIR} ${GVNCPULSE_INCLUDE_DIR} ${GTK-VNC_INCLUDE_DIR})
    message(STATUS "GTK-VNC-Include-Dirs: ${GTK-VNC_INCLUDE_DIRS}")
endif()

if(GTK-VNC_INCLUDE_DIRS)
    set(GTK-VNC_FOUND TRUE)
endif()
