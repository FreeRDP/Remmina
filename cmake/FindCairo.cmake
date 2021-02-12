# FindCairo.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for Cairo.
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

set(Cairo_DEPS)

if(PKG_CONFIG_FOUND)
  pkg_search_module(Cairo_PKG cairo)
endif()

find_library(Cairo_LIBRARY cairo HINTS ${Cairo_PKG_LIBRARY_DIRS})
set(Cairo cairo)

if(Cairo_LIBRARY)
  add_library(${Cairo} SHARED IMPORTED)
  set_property(TARGET ${Cairo} PROPERTY IMPORTED_LOCATION "${Cairo_LIBRARY}")
  set_property(TARGET ${Cairo} PROPERTY INTERFACE_COMPILE_OPTIONS "${Cairo_PKG_CFLAGS_OTHER}")

  set(Cairo_INCLUDE_DIRS)

  find_path(Cairo_INCLUDE_DIR "cairo.h"
    HINTS ${Cairo_PKG_INCLUDE_DIRS})

  if(Cairo_INCLUDE_DIR)
    file(STRINGS "${Cairo_INCLUDE_DIR}/cairo-version.h" Cairo_VERSION_MAJOR REGEX "^#define CAIRO_VERSION_MAJOR +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define CAIRO_VERSION_MAJOR \\(?([0-9]+)\\)?$" "\\1" Cairo_VERSION_MAJOR "${Cairo_VERSION_MAJOR}")
    file(STRINGS "${Cairo_INCLUDE_DIR}/cairo-version.h" Cairo_VERSION_MINOR REGEX "^#define CAIRO_VERSION_MINOR +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define CAIRO_VERSION_MINOR \\(?([0-9]+)\\)?$" "\\1" Cairo_VERSION_MINOR "${Cairo_VERSION_MINOR}")
    file(STRINGS "${Cairo_INCLUDE_DIR}/cairo-version.h" Cairo_VERSION_MICRO REGEX "^#define CAIRO_VERSION_MICRO +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define CAIRO_VERSION_MICRO \\(?([0-9]+)\\)?$" "\\1" Cairo_VERSION_MICRO "${Cairo_VERSION_MICRO}")
    set(Cairo_VERSION "${Cairo_VERSION_MAJOR}.${Cairo_VERSION_MINOR}.${Cairo_VERSION_MICRO}")
    unset(Cairo_VERSION_MAJOR)
    unset(Cairo_VERSION_MINOR)
    unset(Cairo_VERSION_MICRO)

    list(APPEND Cairo_INCLUDE_DIRS ${Cairo_INCLUDE_DIR})
    set_property(TARGET ${Cairo} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${Cairo_INCLUDE_DIR}")
  endif()
endif()

set(Cairo_DEPS_FOUND_VARS)
foreach(cairo_dep ${Cairo_DEPS})
  find_package(${cairo_dep})

  list(APPEND Cairo_DEPS_FOUND_VARS "${cairo_dep}_FOUND")
  list(APPEND Cairo_INCLUDE_DIRS ${${cairo_dep}_INCLUDE_DIRS})

  set_property (TARGET ${Cairo} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${cairo_dep}}")
endforeach(cairo_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cairo
    REQUIRED_VARS
      Cairo_LIBRARY
      Cairo_INCLUDE_DIRS
      ${Cairo_DEPS_FOUND_VARS}
    VERSION_VAR
      Cairo_VERSION)

unset(Cairo_DEPS_FOUND_VARS)
