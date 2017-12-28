# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
# Copyright (C) 2014-2015 Antenore Gatta, Fabio Castelli, Giovanni Panozzo
# Copyright (C) 2016-2017 Antenore Gatta, Giovanni Panozzo
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


cmake_minimum_required(VERSION 2.8.2)

set(CMAKE_COLOR_MAKEFILE ON)

project(Remmina C)

include(CheckIncludeFiles)
include(CheckLibraryExists)
include(FindPkgConfig)
include(CheckCCompilerFlag)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

include(FindOptionalPackage)
include(ConfigOptions)

if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ON)
endif()

if(CMAKE_COMPILER_IS_GNUCC)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
        if(WITH_SSE2)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
        endif()
        CHECK_C_COMPILER_FLAG(-Wno-unused-result Wno-unused-result)
        if(Wno-unused-result)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
        endif()
        CHECK_C_COMPILER_FLAG(-Wno-unused-but-set-variable Wno-unused-but-set-variable)
        if(Wno-unused-but-set-variable)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
        endif()
        CHECK_C_COMPILER_FLAG(-Wno-unused-function Wno-unused-function)
        if(Wno-unused-function)
                set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")
        endif()
endif()

check_include_files(sys/param.h HAVE_SYS_PARAM_H)
check_include_files(sys/socket.h HAVE_SYS_SOCKET_H)
check_include_files(arpa/inet.h HAVE_ARPA_INET_H)
check_include_files(netinet/in.h HAVE_NETINET_IN_H)
check_include_files(netinet/tcp.h HAVE_NETINET_TCP_H)
check_include_files(termios.h HAVE_TERMIOS_H)
check_include_files(netdb.h HAVE_NETDB_H)
check_include_files(fcntl.h HAVE_FCNTL_H)
check_include_files(unistd.h HAVE_UNISTD_H)
check_include_files(sys/un.h HAVE_SYS_UN_H)
check_include_files(errno.h HAVE_ERRNO_H)

include_directories(.)
include_directories(remmina/include)

# FreeBSD

if(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
        set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "-rdynamic")
        include_directories(/usr/local/include /usr/include)
        link_directories(/usr/local/lib /usr/lib)
        set(CMAKE_REQUIRED_INCLUDES /usr/local/include)
        set(CMAKE_SHARED_LIBRARY_PREFIX ${CMAKE_SHARED_LIBRARY_PREFIX} /usr/local/lib)
endif()

# Versioning
set(WITH_LIBRARY_VERSIONING "ON")
set(REMMINA_VERSION_MAJOR "1")
set(REMMINA_VERSION_MINOR "2")
set(REMMINA_VERSION_REVISION "0")
set(REMMINA_VERSION_SUFFIX "rcgit-26")
set(REMMINA_VERSION "${REMMINA_VERSION_MAJOR}.${REMMINA_VERSION_MINOR}.${REMMINA_VERSION_REVISION}")
if(NOT ${REMMINA_VERSION_SUFFIX} STREQUAL "")
        set(REMMINA_VERSION "${REMMINA_VERSION}-${REMMINA_VERSION_SUFFIX}")
endif()

include(GetGitRevisionDescription)
git_get_exact_tag(REMMINA_GIT_REVISION --tags --always)
if(${REMMINA_GIT_REVISION} STREQUAL "n/a")
    git_rev_parse(REMMINA_GIT_REVISION --short)
endif()

if(NOT REMMINA_GIT_REVISION OR ${REMMINA_GIT_REVISION} STREQUAL "n/a")
    set(REMMINA_GIT_REVISION ${REMMINA_VERSION_SUFFIX})
endif()
message(STATUS "Git Revision ${REMMINA_GIT_REVISION}")

if(NOT UNIQUE_APPNAME)
    # UNIQUE_APPNAME should follow Gnome Application ID rules
    set(UNIQUE_APPNAME "org.remmina.Remmina")
endif()

if(NOT REMMINA_DATADIR)
    set(REMMINA_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}")
    if(NOT REMMINA_RUNTIME_DATADIR)
        set(REMMINA_RUNTIME_DATADIR "${REMMINA_DATADIR}")
    endif()
endif()
if(NOT REMMINA_LOCALEDIR)
    set(REMMINA_LOCALEDIR "${CMAKE_INSTALL_FULL_LOCALEDIR}")
    if(NOT REMMINA_RUNTIME_LOCALEDIR)
        set(REMMINA_RUNTIME_LOCALEDIR "${REMMINA_LOCALEDIR}")
    endif()
endif()
if(NOT REMMINA_PLUGINDIR)
    set(REMMINA_PLUGINDIR "${CMAKE_INSTALL_FULL_LIBDIR}/remmina/plugins")
    if(NOT REMMINA_RUNTIME_PLUGINDIR)
        set(REMMINA_RUNTIME_PLUGINDIR "${REMMINA_PLUGINDIR}")
    endif()
endif()
if(NOT REMMINA_UIDIR)
    set(REMMINA_UIDIR "${REMMINA_DATADIR}/remmina/ui")
    if(NOT REMMINA_RUNTIME_UIDIR)
        set(REMMINA_RUNTIME_UIDIR "${REMMINA_UIDIR}")
    endif()
endif()
if(NOT REMMINA_EXTERNAL_TOOLS_DIR)
    set(REMMINA_EXTERNAL_TOOLS_DIR "${REMMINA_DATADIR}/remmina/external_tools")
    if(NOT REMMINA_RUNTIME_EXTERNAL_TOOLS_DIR)
        set(REMMINA_RUNTIME_EXTERNAL_TOOLS_DIR "${REMMINA_EXTERNAL_TOOLS_DIR}")
    endif()
endif()

# Snap package generation support
if(NOT DEFINED ENV{SNAPCRAFT_PROJECT_NAME})
    find_program(SNAPCRAFT "snapcraft")
    if(SNAPCRAFT)
        add_subdirectory(snap)
    endif()
endif()

option(SNAP_BUILD_ONLY "SNAP_BUILD_ONLY" OFF)
if(SNAP_BUILD_ONLY)
    if(NOT SNAPCRAFT)
        message(FATAL_ERROR "snapcraft not found, impossible to create only snap target")
    endif()
    return()
endif()

if(SNAP_BUILD)
    add_definitions(-DSNAP_BUILD)
endif()

# Uninstall support
configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
        IMMEDIATE @ONLY)
add_custom_target(uninstall
        "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

find_required_package(GTK)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
if(NOT CMAKE_USE_PTHREADS_INIT)
        message(FATAL_ERROR "pthreads thread library not found")
endif()

# Add GCC sanitizer options, must be done after pthreads detection
# see remmina issue #1276
if(CMAKE_COMPILER_IS_GNUCC AND WITH_GCC_SANITIZE_ADDRESS)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

find_suggested_package(GCRYPT)
find_suggested_package(AVAHI)

if(WITH_TRACE_CALLS)
        message(STATUS "Enabling trace calls.")
        add_definitions(-DWITH_TRACE_CALLS)
endif()

option(WITH_MANPAGES "Build with MANPAGES" ON)
if(WITH_MANPAGES)
    message(STATUS "Enabling man pages.")
    add_definitions(-DWITH_MANPAGES)
else()
    message(STATUS "Man pages disabled")
endif()

if(GCRYPT_FOUND)
        add_definitions(-DHAVE_LIBGCRYPT)
endif()

if(AVAHI_FOUND)
        add_definitions(-DHAVE_LIBAVAHI_UI)
        add_definitions(-DHAVE_LIBAVAHI_CLIENT)
endif()

if(GTK_FOUND)
        add_subdirectory(remmina)
        add_subdirectory(remmina-plugins)
        add_subdirectory(remmina-plugin-secret)
endif()

if(WITH_EXAMPLES)
        message(STATUS "Enabling examples and test plugins.")
        add_definitions(-DWITH_EXAMPLES)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/config.h)
