# FindGIO.cmake
# <https://github.com/nemequ/gnome-cmake>
#
# CMake support for GIO.
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

set(GIO_DEPS
  GObject)

if(PKG_CONFIG_FOUND)
  pkg_search_module(GIO_PKG gio-2.0)
  pkg_check_modules(GIO-UNIX_PKG REQUIRED gio-unix-2.0)
endif()

find_library(GIO_LIBRARY gio-2.0 HINTS ${GIO_PKG_LIBRARY_DIRS})
set(GIO "gio-2.0")

if(GIO_LIBRARY AND NOT GIO_FOUND)
  add_library(${GIO} SHARED IMPORTED)
  set_property(TARGET ${GIO} PROPERTY IMPORTED_LOCATION "${GIO_LIBRARY}")
  set_property(TARGET ${GIO} PROPERTY INTERFACE_COMPILE_OPTIONS "${GIO_PKG_CFLAGS_OTHER}")

  find_path(GIO_INCLUDE_DIR "gio/gio.h"
    HINTS ${GIO_PKG_INCLUDE_DIRS})

find_path(GIO-UNIX_INCLUDE_DIR "gio/gdesktopappinfo.h"
    HINTS ${GIO-UNIX_PKG_INCLUDE_DIRS})


  find_package(GLib)
  find_package(GObject)
  set(GIO_VERSION "${GLib_VERSION}")

  list(APPEND GIO_DEPS_FOUND_VARS "GObject_FOUND")
  list(APPEND GIO_INCLUDE_DIRS ${GIO-UNIX_INCLUDE_DIR} ${GObject_INCLUDE_DIRS})

  set_property (TARGET "${GIO}" APPEND PROPERTY INTERFACE_LINK_LIBRARIES "gobject-2.0")
    set_property(TARGET ${GIO} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${GIO_INCLUDE_DIR}")
endif()

find_program(GLib_COMPILE_SCHEMAS glib-compile-schemas)
if(GLib_COMPILE_SCHEMAS AND NOT GLib_FOUND)
  add_executable(glib-compile-schemas IMPORTED)
  set_property(TARGET glib-compile-schemas PROPERTY IMPORTED_LOCATION "${GLib_COMPILE_SCHEMAS}")
endif()

# glib_install_schemas(
#   [DESTINATION directory]
#   schemas…)
#
# Validate and install the listed schemas.
function(glib_install_schemas)
  set (options)
  set (oneValueArgs DESTINATION)
  set (multiValueArgs)
  cmake_parse_arguments(GSCHEMA "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  unset (options)
  unset (oneValueArgs)
  unset (multiValueArgs)

  foreach(schema ${GSCHEMA_UNPARSED_ARGUMENTS})
    get_filename_component(schema_name "${schema}" NAME)
    string(REGEX REPLACE "^(.+)\.gschema.xml$" "\\1" schema_name "${schema_name}")
    set(schema_output "${CMAKE_CURRENT_BINARY_DIR}/${schema_name}.gschema.xml.valid")

    add_custom_command(
      OUTPUT "${schema_output}"
      COMMAND glib-compile-schemas
        --strict
        --dry-run
        --schema-file="${schema}"
      COMMAND "${CMAKE_COMMAND}" ARGS -E touch "${schema_output}"
      DEPENDS "${schema}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
      COMMENT "Validating ${schema}")

    add_custom_target("gsettings-schema-${schema_name}" ALL
      DEPENDS "${schema_output}")

    if(CMAKE_INSTALL_FULL_DATADIR)
      set(SCHEMADIR "${CMAKE_INSTALL_FULL_DATADIR}/glib-2.0/schemas")
    else()
      set(SCHEMADIR "${CMAKE_INSTALL_PREFIX}/share/glib-2.0/schemas")
    endif()
    install(FILES "${schema}"
      DESTINATION "${SCHEMADIR}")
    install(CODE "execute_process(COMMAND \"${GLib_COMPILE_SCHEMAS}\" \"${SCHEMADIR}\")")
  endforeach()
endfunction()

find_program(GLib_COMPILE_RESOURCES glib-compile-resources)
if(GLib_COMPILE_RESOURCES AND NOT GLib_FOUND)
  add_executable(glib-compile-resources IMPORTED)
  set_property(TARGET glib-compile-resources PROPERTY IMPORTED_LOCATION "${GLib_COMPILE_RESOURCES}")
endif()

function(glib_compile_resources SPEC_FILE)
  set (options INTERNAL)
  set (oneValueArgs TARGET SOURCE_DIR HEADER SOURCE C_NAME)
  set (multiValueArgs)
  cmake_parse_arguments(GLib_COMPILE_RESOURCES "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  unset (options)
  unset (oneValueArgs)
  unset (multiValueArgs)

  if(NOT GLib_COMPILE_RESOURCES_SOURCE_DIR)
    set(GLib_COMPILE_RESOURCES_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  set(FLAGS)

  if(GLib_COMPILE_RESOURCES_INTERNAL)
    list(APPEND FLAGS "--internal")
  endif()

  if(GLib_COMPILE_RESOURCES_C_NAME)
    list(APPEND FLAGS "--c-name" "${GLib_COMPILE_RESOURCES_C_NAME}")
  endif()

  get_filename_component(SPEC_FILE "${SPEC_FILE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

  execute_process(
    COMMAND glib-compile-resources
      --generate-dependencies
      --sourcedir "${GLib_COMPILE_RESOURCES_SOURCE_DIR}"
      "${SPEC_FILE}"
    OUTPUT_VARIABLE deps
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  string(REPLACE "\n" ";" deps ${deps})

  if(GLib_COMPILE_RESOURCES_HEADER)
    get_filename_component(GLib_COMPILE_RESOURCES_HEADER "${GLib_COMPILE_RESOURCES_HEADER}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

    add_custom_command(
      OUTPUT "${GLib_COMPILE_RESOURCES_HEADER}"
      COMMAND glib-compile-resources
        --sourcedir "${GLib_COMPILE_RESOURCES_SOURCE_DIR}"
        --generate-header
        --target "${GLib_COMPILE_RESOURCES_HEADER}"
        ${FLAGS}
        "${SPEC_FILE}"
      DEPENDS "${SPEC_FILE}" ${deps}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()

  if(GLib_COMPILE_RESOURCES_SOURCE)
    get_filename_component(GLib_COMPILE_RESOURCES_SOURCE "${GLib_COMPILE_RESOURCES_SOURCE}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_BINARY_DIR}")

    add_custom_command(
      OUTPUT "${GLib_COMPILE_RESOURCES_SOURCE}"
      COMMAND glib-compile-resources
        --sourcedir "${GLib_COMPILE_RESOURCES_SOURCE_DIR}"
        --generate-source
        --target "${GLib_COMPILE_RESOURCES_SOURCE}"
        ${FLAGS}
        "${SPEC_FILE}"
      DEPENDS "${SPEC_FILE}" ${deps}
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  endif()
endfunction()

find_program(GDBUS_CODEGEN gdbus-codegen)
if(GDBUS_CODEGEN AND NOT GLib_FOUND)
  add_executable(gdbus-codegen IMPORTED)
  set_property(TARGET gdbus-codegen PROPERTY IMPORTED_LOCATION "${GDBUS_CODEGEN}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GIO
    REQUIRED_VARS
      GIO_LIBRARY
      GIO_INCLUDE_DIRS
      ${GIO_DEPS_FOUND_VARS}
    VERSION_VAR
      GIO_VERSION)

unset(GIO_DEPS_FOUND_VARS)
