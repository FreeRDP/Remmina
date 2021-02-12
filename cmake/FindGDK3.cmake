# FindGDK3.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for GDK 3.
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

set(GDK3_DEPS
  Pango)

if(PKG_CONFIG_FOUND)
  pkg_search_module(GDK3_PKG gdk-3.0)
endif()

find_library(GDK3_LIBRARY gdk-3 HINTS ${GDK3_PKG_LIBRARY_DIRS})
set(GDK3 "gdk-3")

if(GDK3_LIBRARY)
  add_library(${GDK3} SHARED IMPORTED)
  set_property(TARGET ${GDK3} PROPERTY IMPORTED_LOCATION "${GDK3_LIBRARY}")
  set_property(TARGET ${GDK3} PROPERTY INTERFACE_COMPILE_OPTIONS "${GDK3_PKG_CFLAGS_OTHER}")

  set(GDK3_INCLUDE_DIRS)

  find_path(GDK3_INCLUDE_DIR "gdk/gdk.h"
    HINTS ${GDK3_PKG_INCLUDE_DIRS})

  if(GDK3_INCLUDE_DIR)
    file(STRINGS "${GDK3_INCLUDE_DIR}/gdk/gdkversionmacros.h" GDK3_MAJOR_VERSION REGEX "^#define GDK_MAJOR_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define GDK_MAJOR_VERSION \\(?([0-9]+)\\)?$" "\\1" GDK3_MAJOR_VERSION "${GDK3_MAJOR_VERSION}")
    file(STRINGS "${GDK3_INCLUDE_DIR}/gdk/gdkversionmacros.h" GDK3_MINOR_VERSION REGEX "^#define GDK_MINOR_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define GDK_MINOR_VERSION \\(?([0-9]+)\\)?$" "\\1" GDK3_MINOR_VERSION "${GDK3_MINOR_VERSION}")
    file(STRINGS "${GDK3_INCLUDE_DIR}/gdk/gdkversionmacros.h" GDK3_MICRO_VERSION REGEX "^#define GDK_MICRO_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define GDK_MICRO_VERSION \\(?([0-9]+)\\)?$" "\\1" GDK3_MICRO_VERSION "${GDK3_MICRO_VERSION}")
    set(GDK3_VERSION "${GDK3_MAJOR_VERSION}.${GDK3_MINOR_VERSION}.${GDK3_MICRO_VERSION}")
    unset(GDK3_MAJOR_VERSION)
    unset(GDK3_MINOR_VERSION)
    unset(GDK3_MICRO_VERSION)

    list(APPEND GDK3_INCLUDE_DIRS ${GDK3_INCLUDE_DIR})
    set_property(TARGET ${GDK3} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GDK3_INCLUDE_DIR}")
  endif()
endif()

set(GDK3_DEPS_FOUND_VARS)
foreach(gdk3_dep ${GDK3_DEPS})
  find_package(${gdk3_dep})

  list(APPEND GDK3_DEPS_FOUND_VARS "${gdk3_dep}_FOUND")
  list(APPEND GDK3_INCLUDE_DIRS ${${gdk3_dep}_INCLUDE_DIRS})

  set_property (TARGET ${GDK3} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${gdk3_dep}}")
endforeach(gdk3_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDK3
    REQUIRED_VARS
      GDK3_LIBRARY
      GDK3_INCLUDE_DIRS
      ${GDK3_DEPS_FOUND_VARS}
    VERSION_VAR
      GDK3_VERSION)

unset(GDK3_DEPS_FOUND_VARS)
