# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2012 Andrey Gankov
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
# Foundation, Inc., 59 Temple Place, Suite 330, 
# Boston, MA 02111-1307, USA.

set(_GLIB_found_all true)

# Glib

pkg_check_modules(PC_GLIB2 glib-2.0>=2.28)

if(NOT PC_GLIB2_FOUND)
	set(_GLIB_found_all false)
endif()

find_path(GLIB2_INCLUDE_DIR_PART1 NAMES glib.h
	HINTS ${PC_GLIB2_INCLUDEDIR} ${PC_GLIB2_INCLUDE_DIRS}
	PATH_SUFFIXES glib-2.0)

find_path(GLIB2_INCLUDE_DIR_PART2 NAMES glibconfig.h
	HINTS ${PC_GLIB2_INCLUDEDIR} ${PC_GLIB2_INCLUDE_DIRS}
	PATH_SUFFIXES glib-2.0/include)

set(GLIB2_INCLUDE_DIR ${GLIB2_INCLUDE_DIR_PART1} ${GLIB2_INCLUDE_DIR_PART2})

find_library(GLIB2_LIBRARY NAMES glib-2.0)

# GIO

pkg_check_modules(PC_GIO gio-2.0)

if(NOT PC_GIO_FOUND)
	set(_GLIB_found_all false)
endif()

find_path(GIO_INCLUDE_DIR gio/gio.h
	HINTS ${PC_GIO_INCLUDEDIR} ${PC_GIO_INCLUDE_DIRS}
	PATH_SUFFIXES gio-2.0)

find_library(GIO_LIBRARY NAMES gio-2.0
	HINTS ${PC_GIO_LIBDIR} ${PC_GIO_LIBRARY_DIRS})

# gobject

pkg_check_modules(PC_GOBJECT gobject-2.0)

if(NOT PC_GOBJECT_FOUND)
	set(_GLIB_found_all false)
endif()

find_path(GOBJECT_INCLUDE_DIR gobject/gobject.h
	HINTS ${PC_GOBJECT_INCLUDEDIR} ${PC_GOBJECT_INCLUDE_DIRS}
	PATH_SUFFIXES gobject-2.0)

find_library(GOBJECT_LIBRARY NAMES gobject-2.0
	HINTS ${PC_GOBJECT_LIBDIR} ${PC_GOBJECT_LIBRARY_DIRS})

# gmodule

pkg_check_modules(PC_GMODULE gmodule-2.0)

if(NOT PC_GMODULE_FOUND)
	set(_GLIB_found_all false)
endif()

find_path(GMODULE_INCLUDE_DIR gmodule.h
	HINTS ${PC_GMODULE_INCLUDEDIR} ${PC_GMODULE_INCLUDE_DIRS}
	PATH_SUFFIXES gmodule-2.0)

find_library(GMODULE_LIBRARY NAMES gmodule-2.0
	HINTS ${PC_GMODULE_LIBDIR} ${PC_GMODULE_LIBRARY_DIRS})

# gthread

pkg_check_modules(PC_GTHREAD gthread-2.0)

if(NOT PC_GTHREAD_FOUND)
	set(_GLIB_found_all false)
endif()

find_path(GTHREAD_INCLUDE_DIR glib/gthread.h
	HINTS ${PC_GTHREAD_INCLUDEDIR} ${PC_GTHREAD_INCLUDE_DIRS}
	PATH_SUFFIXES gthread-2.0)

find_library(GTHREAD_LIBRARY NAMES gthread-2.0
	HINTS ${PC_GTHREAD_LIBDIR} ${PC_GTHREAD_LIBRARY_DIRS})

# Finalize

if(_GLIB_found_all)
	include(FindPackageHandleStandardArgs)

	find_package_handle_standard_args(GLIB2 DEFAULT_MSG GLIB2_LIBRARY GLIB2_INCLUDE_DIR)

	set(GLIB2_LIBRARIES ${GLIB2_LIBRARY} ${GIO_LIBRARY} ${GOBJECT_LIBRARY} ${GMODULE_LIBRARY} ${GTHREAD_LIBRARY})
	set(GLIB2_INCLUDE_DIRS ${GLIB2_INCLUDE_DIR} ${GIO_INCLUDE_DIR} ${GOBJECT_INCLUDE_DIR} ${GMODULE_INCLUDE_DIR} ${GTHREAD_INCLUDE_DIR})

	mark_as_advanced(GLIB2_INCLUDE_DIR GLIB2_LIBRARY)

	set(GLIB_FOUND true)
else()
	unset(GLIB2_LIBRARY)
	unset(GLIB2_INCLUDE_DIR)

	set(GLIB_FOUND false)
endif()
