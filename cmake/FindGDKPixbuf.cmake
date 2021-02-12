# FindGDKPixbuf.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for GDK Pixbuf.
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

set(GDKPixbuf_DEPS
  GLib)

if(PKG_CONFIG_FOUND)
  pkg_search_module(GDKPixbuf_PKG gdk-pixbuf-2.0)
endif()

find_library(GDKPixbuf_LIBRARY gdk_pixbuf-2.0 HINTS ${GDKPixbuf_PKG_LIBRARY_DIRS})
set(GDKPixbuf "gdk_pixbuf-2.0")

if(GDKPixbuf_LIBRARY)
  add_library(${GDKPixbuf} SHARED IMPORTED)
  set_property(TARGET ${GDKPixbuf} PROPERTY IMPORTED_LOCATION "${GDKPixbuf_LIBRARY}")
  set_property(TARGET ${GDKPixbuf} PROPERTY INTERFACE_COMPILE_OPTIONS "${GDKPixbuf_PKG_CFLAGS_OTHER}")

  set(GDKPixbuf_INCLUDE_DIRS)

  find_path(GDKPixbuf_INCLUDE_DIR "gdk-pixbuf/gdk-pixbuf.h"
    HINTS ${GDKPixbuf_PKG_INCLUDE_DIRS})

  if(GDKPixbuf_INCLUDE_DIR)
    file(STRINGS "${GDKPixbuf_INCLUDE_DIR}/gdk-pixbuf/gdk-pixbuf-features.h" GDKPixbuf_VERSION REGEX "^#define GDKPIXBUF_VERSION \\\"[^\\\"]+\\\"")
    string(REGEX REPLACE "^#define GDKPIXBUF_VERSION \\\"([0-9]+)\\.([0-9]+)\\.([0-9]+)\\\"$" "\\1.\\2.\\3" GDKPixbuf_VERSION "${GDKPixbuf_VERSION}")

    list(APPEND GDKPixbuf_INCLUDE_DIRS ${GDKPixbuf_INCLUDE_DIR})
    set_property(TARGET ${GDKPixbuf} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GDKPixbuf_INCLUDE_DIR}")
  endif()
endif()

set(GDKPixbuf_DEPS_FOUND_VARS)
foreach(gdkpixbuf_dep ${GDKPixbuf_DEPS})
  find_package(${gdkpixbuf_dep})

  list(APPEND GDKPixbuf_DEPS_FOUND_VARS "${gdkpixbuf_dep}_FOUND")
  list(APPEND GDKPixbuf_INCLUDE_DIRS ${${gdkpixbuf_dep}_INCLUDE_DIRS})

  set_property (TARGET ${GDKPixbuf} APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${gdkpixbuf_dep}}")
endforeach(gdkpixbuf_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GDKPixbuf
    REQUIRED_VARS
      GDKPixbuf_LIBRARY
      GDKPixbuf_INCLUDE_DIRS
      ${GDKPixbuf_DEPS_FOUND_VARS}
    VERSION_VAR
      GDKPixbuf_VERSION)

unset(GDKPixbuf_DEPS_FOUND_VARS)
