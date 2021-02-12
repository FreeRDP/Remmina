# FindPango.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for ATK.
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

set(ATK_DEPS
  GLib)

if(PKG_CONFIG_FOUND)
  pkg_search_module(ATK_PKG atk)
endif()

find_library(ATK_LIBRARY atk-1.0 HINTS ${ATK_PKG_LIBRARY_DIRS})
set(ATK "atk-1.0")

if(ATK_LIBRARY AND NOT ATK_FOUND)
  add_library(${ATK} SHARED IMPORTED)
  set_property(TARGET ${ATK} PROPERTY IMPORTED_LOCATION "${ATK_LIBRARY}")
  set_property(TARGET ${ATK} PROPERTY INTERFACE_COMPILE_OPTIONS "${ATK_PKG_CFLAGS_OTHER}")

  find_path(ATK_INCLUDE_DIR "atk/atk.h"
    HINTS ${ATK_PKG_INCLUDE_DIRS})

  if(ATK_INCLUDE_DIR)
    file(STRINGS "${ATK_INCLUDE_DIR}/atk/atkversion.h" ATK_MAJOR_VERSION REGEX "^#define ATK_MAJOR_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define ATK_MAJOR_VERSION \\(([0-9]+)\\)$" "\\1" ATK_MAJOR_VERSION "${ATK_MAJOR_VERSION}")
    file(STRINGS "${ATK_INCLUDE_DIR}/atk/atkversion.h" ATK_MINOR_VERSION REGEX "^#define ATK_MINOR_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define ATK_MINOR_VERSION \\(([0-9]+)\\)$" "\\1" ATK_MINOR_VERSION "${ATK_MINOR_VERSION}")
    file(STRINGS "${ATK_INCLUDE_DIR}/atk/atkversion.h" ATK_MICRO_VERSION REGEX "^#define ATK_MICRO_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define ATK_MICRO_VERSION \\(([0-9]+)\\)$" "\\1" ATK_MICRO_VERSION "${ATK_MICRO_VERSION}")
    set(ATK_VERSION "${ATK_MAJOR_VERSION}.${ATK_MINOR_VERSION}.${ATK_MICRO_VERSION}")
    unset(ATK_MAJOR_VERSION)
    unset(ATK_MINOR_VERSION)
    unset(ATK_MICRO_VERSION)

    list(APPEND ATK_INCLUDE_DIRS ${ATK_INCLUDE_DIR})
    set_property(TARGET ${ATK} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${ATK_INCLUDE_DIR}")
  endif()
endif()

set(ATK_DEPS_FOUND_VARS)
foreach(atk_dep ${ATK_DEPS})
  find_package(${atk_dep})

  list(APPEND ATK_DEPS_FOUND_VARS "${atk_dep}_FOUND")
  list(APPEND ATK_INCLUDE_DIRS ${${atk_dep}_INCLUDE_DIRS})

  set_property (TARGET "${ATK}" APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${atk_dep}}")
endforeach(atk_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ATK
    REQUIRED_VARS
      ATK_LIBRARY
      ATK_INCLUDE_DIRS
      ${ATK_DEPS_FOUND_VARS}
    VERSION_VAR
      ATK_VERSION)

unset(ATK_DEPS_FOUND_VARS)
