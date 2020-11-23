# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau Copyright (C) 2014-2015 Antenore Gatta,
# Fabio Castelli, Giovanni Panozzo Copyright (C) 2016-2020 Antenore Gatta,
# Giovanni Panozzo
#
# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU General Public License as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc., 51 Franklin
# Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# In addition, as a special exception, the copyright holders give permission to
# link the code of portions of this program with the OpenSSL library under
# certain conditions as described in each individual source file, and distribute
# linked combinations including the two. You must obey the GNU General Public
# License in all respects for all of the code used other than OpenSSL. If you
# modify file(s) with this exception, you may extend this exception to your
# version of the file(s), but you are not obligated to do so. If you do not wish
# to do so, delete this exception statement from your version. If you delete
# this exception statement from all source files in the program, then also
# delete it here.

cmake_minimum_required(VERSION 3.0.0)

set(CMAKE_COLOR_MAKEFILE ON)

project(Remmina C CXX)

include(CheckCCompilerFlag)
include(CheckIncludeFiles)
include(CheckLibraryExists)
include(CheckSymbolExists)
include(FindPkgConfig)
include(GNUInstallDirs)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

include(FindOptionalPackage)
include(ConfigOptions)
include(GtkUpdateIconCache)
include(CheckHasModule)
include(FindPCRE2)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()

if(NOT BUILD_SHARED_LIBS)
  set(BUILD_SHARED_LIBS ON)
endif()

if(CMAKE_COMPILER_IS_GNUCC)
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64" OR CMAKE_SYSTEM_PROCESSOR MATCHES
                                                "i686")
    check_symbol_exists(__x86_64__ "" IS_X86_64)
    if(IS_X86_64)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
    else()
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=i686")
    endif()
  else()
    if(CMAKE_POSITION_INDEPENDENT_CODE)
      if(${CMAKE_VERSION} VERSION_LESS 2.8.9)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
      endif()
    endif()
  endif()

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

  if(WITH_SSE2)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_definitions(-DDEBUG)
    check_c_compiler_flag(-Wunused-result Wunused-result)
    if(Wunused-result)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused-result")
    endif()
    check_c_compiler_flag(-Wunused-but-set-variable Wunused-but-set-variable)
    if(Wunused-but-set-variable)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused-but-set-variable")
    endif()
    check_c_compiler_flag(-Wdeprecated-declarations Wdeprecated-declarations)
    if(Wdeprecated-declarations)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeprecated-declarations")
    endif()

    check_c_compiler_flag(-Wimplicit-function-declaration
                          Wimplicit-function-declaration)
    if(Wimplicit-function-declaration)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit-function-declaration")
    endif()
    if(NOT OPENBSD)
      check_c_compiler_flag(-Wredundant-decls Wredundant-decls)
      if(Wredundant-decls)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wredundant-decls")
      endif()
    endif()
  endif()

  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG)
    check_c_compiler_flag(-Wno-unused-result Wno-unused-result)
    if(Wno-unused-result)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-result")
    endif()
    check_c_compiler_flag(-Wno-unused-but-set-variable
                          Wno-unused-but-set-variable)
    if(Wno-unused-but-set-variable)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-but-set-variable")
    endif()
    check_c_compiler_flag(-Wno-deprecated-declarations
                          Wno-deprecated-declarations)
    if(Wno-deprecated-declarations)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
    endif()

    check_c_compiler_flag(-Wimplicit-function-declaration
                          Wimplicit-function-declaration)
    if(Wimplicit-function-declaration)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wimplicit-function-declaration")
    endif()
    if(NOT OPENBSD)
      check_c_compiler_flag(-Wredundant-decls Wredundant-decls)
      if(Wredundant-decls)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wredundant-decls")
      endif()
    endif()
  else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g")
  endif()

endif()

if((TARGET_ARCH MATCHES "x86|x64") AND (NOT DEFINED WITH_SSE2))
  option(WITH_SSE2 "Enable SSE2 optimization." ON)
else()
  option(WITH_SSE2 "Enable SSE2 optimization." OFF)
endif()

if(TARGET_ARCH MATCHES "ARM")
  if(NOT DEFINED WITH_NEON)
    option(WITH_NEON "Enable NEON optimization." ON)
  else()
    option(WITH_NEON "Enable NEON optimization." OFF)
  endif()
else()
  if(NOT APPLE)
    option(WITH_IPP "Use Intel Performance Primitives." OFF)
  endif()
endif()

if(${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_definitions(-DNDEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-parameter")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-macros -Wno-padded")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-c11-extensions -Wno-gnu")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-command-line-argument")
    check_c_compiler_flag(-Wno-deprecated-declarations
                          Wno-deprecated-declarations)
    if(Wno-deprecated-declarations)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-deprecated-declarations")
    endif()
  else()
    add_definitions(-DDEBUG)
    # set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wunused-parameter") set(CMAKE_C_FLAGS
    # "${CMAKE_C_FLAGS} -Wunused-macros -Wpadded") set(CMAKE_C_FLAGS
    # "${CMAKE_C_FLAGS} -Wc11-extensions -Wgnu") set(CMAKE_C_FLAGS
    # "${CMAKE_C_FLAGS} -Wunused-command-line-argument")
    check_c_compiler_flag(-Wdeprecated-declarations Wdeprecated-declarations)
    if(Wdeprecated-declarations)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeprecated-declarations")
    endif()
  endif()
endif()

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
include_directories(src/include)

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
set(REMMINA_VERSION_MINOR "4")
set(REMMINA_VERSION_REVISION "9")
# set(REMMINA_VERSION_SUFFIX "1") set(REMMINA_VERSION
# "${REMMINA_VERSION_MAJOR}.${REMMINA_VERSION_MINOR}.${REMMINA_VERSION_REVISION}.${REMMINA_VERSION_SUFFIX}")
set(REMMINA_VERSION
    "${REMMINA_VERSION_MAJOR}.${REMMINA_VERSION_MINOR}.${REMMINA_VERSION_REVISION}"
)

message(STATUS "Remmina version: ${REMMINA_VERSION}")

include(GetGitRevisionDescription)
git_get_exact_tag(REMMINA_GIT_REVISION --tags --always)
if(${REMMINA_GIT_REVISION} STREQUAL "n/a")
  git_rev_parse(REMMINA_GIT_REVISION --short)
endif()

message(STATUS "Git Revision: ${REMMINA_GIT_REVISION}")

# REMMINA_APP_ID should follow Gnome Application ID rules
set(REMMINA_APP_ID "org.remmina.Remmina")

if(NOT REMMINA_DATADIR)
  set(REMMINA_DATADIR "${CMAKE_INSTALL_FULL_DATADIR}")
  if(NOT REMMINA_RUNTIME_DATADIR)
    set(REMMINA_RUNTIME_DATADIR "${REMMINA_DATADIR}")
  endif()
endif()
if(NOT REMMINA_TERM_CS_DIR)
  set(REMMINA_TERM_CS_DIR "${REMMINA_DATADIR}/remmina/theme")
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
if(NOT REMMINA_THEMEDIR)
  set(REMMINA_THEMEDIR "${REMMINA_DATADIR}/remmina/theme")
  if(NOT REMMINA_RUNTIME_THEMEDIR)
    set(REMMINA_RUNTIME_THEMEDIR "${REMMINA_THEMEDIR}")
  endif()
endif()
if(NOT REMMINA_RESDIR)
  set(REMMINA_RESDIR "${REMMINA_DATADIR}/remmina/res")
  if(NOT REMMINA_RUNTIME_RESDIR)
    set(REMMINA_RUNTIME_RESDIR "${REMMINA_RESDIR}")
  endif()
endif()
if(NOT REMMINA_EXTERNAL_TOOLS_DIR)
  set(REMMINA_EXTERNAL_TOOLS_DIR "${REMMINA_DATADIR}/remmina/external_tools")
  if(NOT REMMINA_RUNTIME_EXTERNAL_TOOLS_DIR)
    set(REMMINA_RUNTIME_EXTERNAL_TOOLS_DIR "${REMMINA_EXTERNAL_TOOLS_DIR}")
  endif()
endif()

option(SNAP_BUILD_ONLY "SNAP_BUILD_ONLY" OFF)
if(SNAP_BUILD_ONLY)
  if(NOT SNAPCRAFT)
    message(
      FATAL_ERROR "snapcraft not found, impossible to create only snap target")
  endif()
  return()
endif()

if(SNAP_BUILD)
  add_definitions(-DSNAP_BUILD)
endif()

# Uninstall support
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" IMMEDIATE @ONLY)
add_custom_target(uninstall "${CMAKE_COMMAND}" -P
                            "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")

find_required_package(GTK)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
if(NOT CMAKE_USE_PTHREADS_INIT)
  message(FATAL_ERROR "pthreads thread library not found")
endif()

# Add GCC sanitizer options, must be done after pthreads detection see remmina
# issue #1276
if(CMAKE_COMPILER_IS_GNUCC AND WITH_GCC_SANITIZE_ADDRESS)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
endif()

# LLVM coverage flags - https://clang.llvm.org/docs/SourceBasedCodeCoverage.html
if("${CMAKE_C_COMPILER_ID}" MATCHES "Clang" AND WITH_CLANG_COVERAGE)
  message(STATUS "Enabling coverage.")
  add_definitions(-DWITH_CLANG_COVERAGE)
  set(CMAKE_C_FLAGS
      "${CMAKE_C_FLAGS} -g -fprofile-instr-generate -fcoverage-mapping -ftest-coverage"
  )
  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} -fprofile-instr-generate")
elseif("${CMAKE_C_COMPILER_ID}" MATCHES "GNU" AND WITH_GCC_PROFILING)
  message(STATUS "Enabling profiling.")
  add_definitions(-DWITH_GCC_PROFILING)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pg")
endif()

find_suggested_package(GCRYPT)
find_suggested_package(AVAHI)
find_package(OpenSSL)
find_package(sodium REQUIRED)
if(sodium_FOUND)
  message(STATUS "sodium_LIBRARY_RELEASE: ${sodium_LIBRARY_RELEASE}")
  message(STATUS "sodium_INCLUDE_DIR: ${sodium_INCLUDE_DIR}")
  message(STATUS "sodium_LIBRARY_DEBUG: ${sodium_LIBRARY_DEBUG}")
endif()

if(WITH_TRACE_CALLS)
  message(STATUS "Enabling trace calls.")
  add_definitions(-DWITH_TRACE_CALLS)
endif()

option(WITH_NEWS "Disable online version checking" ON)
if(WITH_NEWS)
  message(STATUS "Enabling version check connecting to remmina.org")
  add_definitions(-DWITH_NEWS)
  set(RMNEWS_ENABLE_NEWS 1)
else()
  message(STATUS "Disabling version check connecting to remmina.org")
  set(RMNEWS_ENABLE_NEWS 0)
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

option(WITH_ICON_CACHE "Generate the icon cache during install target" ON)
if(WITH_ICON_CACHE)
  message(STATUS "Icon cache will be generated")
  add_definitions(-DWITH_ICON_CACHE)
  find_program(XDG_ICON_RESOURCE NAMES xdg-icon-resource)
  if(XDG_ICON_RESOURCE)
    message(STATUS "Theme icon cache will be rebuilt")
    add_custom_target(desktop-icon COMMAND ${XDG_ICON_RESOURCE} forceupdate
                                           --theme hicolor)
  else()
    message(WARNING "Cannot update icon cache: xdg-icon-resource not found")
  endif()
else()
  message(STATUS "Icons cache won't be generated automatically")
endif()

option(WITH_UPDATE_DESKTOP_DB
       "Generate desktop files MIME types cache database" ON)
if(WITH_UPDATE_DESKTOP_DB)
  message(STATUS "Desktop MIME types cache database enabled")
  add_definitions(-DWITH_UPDATE_DESKTOP_DB)
else()
  message(STATUS "Desktop MIME types cache database won't be generated")
endif()

if(GTK_FOUND)
  pkg_check_modules(gio REQUIRED gio-2.0)
  pkg_check_modules(gio-unix REQUIRED gio-unix-2.0)
  add_subdirectory(src)
  add_subdirectory(data)
  add_subdirectory(plugins)
  add_subdirectory(plugins/secret)
endif()

if(WITH_TRANSLATIONS)
  add_subdirectory(po)
endif()

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/config.h)

set(REMMINA_BUILD_CONFIG_LIST "")
get_cmake_property(res VARIABLES)
foreach(var ${res})
  if(var MATCHES "^WITH_*|^HAVE_*")
    list(APPEND REMMINA_BUILD_CONFIG_LIST "${var}=${${var}}")
  endif()
endforeach()
string(REPLACE ";" " " REMMINA_BUILD_CONFIG "${REMMINA_BUILD_CONFIG_LIST}")
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/buildflags.h.in
               ${CMAKE_CURRENT_SOURCE_DIR}/buildflags.h)
