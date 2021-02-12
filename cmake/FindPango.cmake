# FindPango.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for Pango.
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

set(Pango_DEPS
  GLib)

# Harfbuzz
pkg_check_modules(PC_HB harfbuzz)
find_path(HB_INCLUDE_DIR
    NAMES hb.h
    HINTS ${PC_HB_INCLUDE_DIRS}
    PATH_SUFFIXES harfbuzz
    )

if(PKG_CONFIG_FOUND)
  pkg_search_module(Pango_PKG pango)
endif()

find_library(Pango_LIBRARY pango-1.0 HINTS ${Pango_PKG_LIBRARY_DIRS})
set(Pango pango-1.0)

if(Pango_LIBRARY AND NOT Pango_FOUND)
  add_library(${Pango} SHARED IMPORTED)
  set_property(TARGET ${Pango} PROPERTY IMPORTED_LOCATION "${Pango_LIBRARY}")
  set_property(TARGET ${Pango} PROPERTY INTERFACE_COMPILE_OPTIONS "${Pango_PKG_CFLAGS_OTHER}")

  find_path(Pango_INCLUDE_DIR "pango/pango.h"
    HINTS ${Pango_PKG_INCLUDE_DIRS})

  if(Pango_INCLUDE_DIR)
    file(STRINGS "${Pango_INCLUDE_DIR}/pango/pango-features.h" Pango_MAJOR_VERSION REGEX "^#define PANGO_VERSION_MAJOR +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define PANGO_VERSION_MAJOR \\(?([0-9]+)\\)?" "\\1" Pango_MAJOR_VERSION "${Pango_MAJOR_VERSION}")
    file(STRINGS "${Pango_INCLUDE_DIR}/pango/pango-features.h" Pango_MINOR_VERSION REGEX "^#define PANGO_VERSION_MINOR +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define PANGO_VERSION_MINOR \\(?([0-9]+)\\)?" "\\1" Pango_MINOR_VERSION "${Pango_MINOR_VERSION}")
    file(STRINGS "${Pango_INCLUDE_DIR}/pango/pango-features.h" Pango_MICRO_VERSION REGEX "^#define PANGO_VERSION_MICRO +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define PANGO_VERSION_MICRO \\(?([0-9]+)\\)?" "\\1" Pango_MICRO_VERSION "${Pango_MICRO_VERSION}")
    set(Pango_VERSION "${Pango_MAJOR_VERSION}.${Pango_MINOR_VERSION}.${Pango_MICRO_VERSION}")
    unset(Pango_MAJOR_VERSION)
    unset(Pango_MINOR_VERSION)
    unset(Pango_MICRO_VERSION)

    list(APPEND Pango_INCLUDE_DIRS ${PC_HB_INCLUDE_DIRS} ${Pango_INCLUDE_DIR})
    set_property(TARGET ${Pango} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${Pango_INCLUDE_DIR}")
  endif()
endif()

set(Pango_DEPS_FOUND_VARS)
foreach(pango_dep ${Pango_DEPS})
  find_package(${pango_dep})

  list(APPEND Pango_DEPS_FOUND_VARS "${pango_dep}_FOUND")
  list(APPEND Pango_INCLUDE_DIRS ${${pango_dep}_INCLUDE_DIRS})

  set_property (TARGET "${Pango}" APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${pango_dep}}")
endforeach(pango_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pango
    REQUIRED_VARS
    Pango_LIBRARY
    Pango_INCLUDE_DIRS
    ${Pango_DEPS_FOUND_VARS}
    VERSION_VAR
    Pango_VERSION)

unset(Pango_DEPS_FOUND_VARS)
