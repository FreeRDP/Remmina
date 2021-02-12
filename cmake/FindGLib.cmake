# FindGLib.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for GLib/GObject/GIO.
#
# License:
#
#   Copyright (c) 2016 Evan Nemerson <evan@nemerson.com>
#
#   Permission is hereby granted, free of charge, to any person
#   obtaining a copy of this software and associated documentation
#   files (the "Software"), to deal in the Software without
#   restriction, including without limitation the rights to use, copy,
#   modify, merge, publish, distribute, sublicense, and/or sell copies
#   of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.

find_package(PkgConfig)

if(PKG_CONFIG_FOUND)
  pkg_search_module(GLib_PKG glib-2.0)
  pkg_check_modules(PC_GMODULE gmodule-2.0)
endif()

find_library(GLib_LIBRARY glib-2.0 HINTS ${GLib_PKG_LIBRARY_DIRS})
set(GLib glib-2.0)

# gmodule

find_path(GMODULE_INCLUDE_DIR gmodule.h
	HINTS ${PC_GMODULE_INCLUDEDIR} ${PC_GMODULE_INCLUDE_DIRS}
	PATH_SUFFIXES gmodule-2.0)

find_library(GMODULE_LIBRARY NAMES gmodule-2.0
	HINTS ${PC_GMODULE_LIBDIR} ${PC_GMODULE_LIBRARY_DIRS})

if(GLib_LIBRARY AND NOT GLib_FOUND)
  add_library(${GLib} SHARED IMPORTED)
  set_property(TARGET ${GLib} PROPERTY IMPORTED_LOCATION "${GLib_LIBRARY}")
  set_property(TARGET ${GLib} PROPERTY INTERFACE_COMPILE_OPTIONS "${GLib_PKG_CFLAGS_OTHER}")

  find_path(GLib_INCLUDE_DIRS "glib.h"
    HINTS ${GLib_PKG_INCLUDE_DIRS}
    PATH_SUFFIXES "glib-2.0")

  get_filename_component(GLib_LIBDIR "${GLib}" DIRECTORY)
  find_path(GLib_CONFIG_INCLUDE_DIR "glibconfig.h"
    HINTS
      ${GLib_LIBDIR}
      ${GLib_PKG_INCLUDE_DIRS}
    PATHS
      "${CMAKE_LIBRARY_PATH}"
    PATH_SUFFIXES
      "glib-2.0/include"
      "glib-2.0")
  unset(GLib_LIBDIR)

  if(GLib_CONFIG_INCLUDE_DIR)
    file(STRINGS "${GLib_CONFIG_INCLUDE_DIR}/glibconfig.h" GLib_MAJOR_VERSION REGEX "^#define GLIB_MAJOR_VERSION +([0-9]+)")
    string(REGEX REPLACE "^#define GLIB_MAJOR_VERSION ([0-9]+)$" "\\1" GLib_MAJOR_VERSION "${GLib_MAJOR_VERSION}")
    file(STRINGS "${GLib_CONFIG_INCLUDE_DIR}/glibconfig.h" GLib_MINOR_VERSION REGEX "^#define GLIB_MINOR_VERSION +([0-9]+)")
    string(REGEX REPLACE "^#define GLIB_MINOR_VERSION ([0-9]+)$" "\\1" GLib_MINOR_VERSION "${GLib_MINOR_VERSION}")
    file(STRINGS "${GLib_CONFIG_INCLUDE_DIR}/glibconfig.h" GLib_MICRO_VERSION REGEX "^#define GLIB_MICRO_VERSION +([0-9]+)")
    string(REGEX REPLACE "^#define GLIB_MICRO_VERSION ([0-9]+)$" "\\1" GLib_MICRO_VERSION "${GLib_MICRO_VERSION}")
    set(GLib_VERSION "${GLib_MAJOR_VERSION}.${GLib_MINOR_VERSION}.${GLib_MICRO_VERSION}")
    unset(GLib_MAJOR_VERSION)
    unset(GLib_MINOR_VERSION)
    unset(GLib_MICRO_VERSION)

    #list(APPEND GLib_INCLUDE_DIRS ${GMODULE_INCLUDE_DIR} ${GLib_CONFIG_INCLUDE_DIR})
    list(APPEND GLib_INCLUDE_DIRS ${GLib_CONFIG_INCLUDE_DIR})
    set_property(TARGET ${GLib} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GLib_INCLUDE_DIRS}")
  endif()
endif()

##list (APPEND GLib_LIBRARY ${GMODULE_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLib
    REQUIRED_VARS
      GLib_LIBRARY
      GLib_INCLUDE_DIRS
    VERSION_VAR
      GLib_VERSION)
