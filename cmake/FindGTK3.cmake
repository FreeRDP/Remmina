# FindGTK3.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for GTK+ 3.
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

set(GTK3_DEPS
  GIO
  ATK
  GDK3
  Pango
  Cairo
  GDKPixbuf)

if(PKG_CONFIG_FOUND)
  pkg_search_module(GTK3_PKG QUIET gtk+-3.0)
endif()

find_library(GTK3_LIBRARY gtk-3 HINTS ${GTK3_PKG_LIBRARY_DIRS})
set(GTK3 gtk-3)

if(GTK3_LIBRARY)
  add_library(${GTK3} SHARED IMPORTED)
  set_property(TARGET ${GTK3} PROPERTY IMPORTED_LOCATION "${GTK3_LIBRARY}")
  set_property(TARGET ${GTK3} PROPERTY INTERFACE_COMPILE_OPTIONS "${GTK3_PKG_CFLAGS_OTHER}")

  set(GTK3_INCLUDE_DIRS)

  find_path(GTK3_INCLUDE_DIR "gtk/gtk.h"
    HINTS ${GTK3_PKG_INCLUDE_DIRS})

  if(GTK3_INCLUDE_DIR)
    file(STRINGS "${GTK3_INCLUDE_DIR}/gtk/gtkversion.h" GTK3_MAJOR_VERSION REGEX "^#define GTK_MAJOR_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define GTK_MAJOR_VERSION \\(?([0-9]+)\\)?$" "\\1" GTK3_MAJOR_VERSION "${GTK3_MAJOR_VERSION}")
    file(STRINGS "${GTK3_INCLUDE_DIR}/gtk/gtkversion.h" GTK3_MINOR_VERSION REGEX "^#define GTK_MINOR_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define GTK_MINOR_VERSION \\(?([0-9]+)\\)?$" "\\1" GTK3_MINOR_VERSION "${GTK3_MINOR_VERSION}")
    file(STRINGS "${GTK3_INCLUDE_DIR}/gtk/gtkversion.h" GTK3_MICRO_VERSION REGEX "^#define GTK_MICRO_VERSION +\\(?([0-9]+)\\)?$")
    string(REGEX REPLACE "^#define GTK_MICRO_VERSION \\(?([0-9]+)\\)?$" "\\1" GTK3_MICRO_VERSION "${GTK3_MICRO_VERSION}")
    set(GTK3_VERSION "${GTK3_MAJOR_VERSION}.${GTK3_MINOR_VERSION}.${GTK3_MICRO_VERSION}")
    unset(GTK3_MAJOR_VERSION)
    unset(GTK3_MINOR_VERSION)
    unset(GTK3_MICRO_VERSION)

    list(APPEND GTK3_INCLUDE_DIRS ${GTK3_INCLUDE_DIR})
    set_property(TARGET ${GTK3} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GTK3_INCLUDE_DIR}")
  endif()
endif()

set(GTK3_DEPS_FOUND_VARS)
foreach(gtk3_dep ${GTK3_DEPS})
  find_package(${gtk3_dep})

  list(APPEND GTK3_DEPS_FOUND_VARS "${gtk3_dep}_FOUND")
  list(APPEND GTK3_INCLUDE_DIRS ${${gtk3_dep}_INCLUDE_DIRS})

  set_property (TARGET "${GTK3}" APPEND PROPERTY INTERFACE_LINK_LIBRARIES "${${gtk3_dep}}")
endforeach(gtk3_dep)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GTK3
    REQUIRED_VARS
      GTK3_LIBRARY
      GTK3_INCLUDE_DIRS
      ${GTK3_DEPS_FOUND_VARS}
    VERSION_VAR
      GTK3_VERSION)

unset(GTK3_DEPS_FOUND_VARS)
