# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2012 Daniel M. Weeks
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

set(_GTK2_found_all true)

# Gtk

pkg_check_modules(PC_GTK2 gtk+-2.0)

if(NOT PC_GTK2_FOUND)
	set(_GTK2_found_all false)
endif()

find_path(GTK2_INCLUDE_DIR NAMES gtk/gtk.h
	PATH_SUFFIXES gtk-2.0)

find_library(GTK2_LIBRARY NAMES gtk-x11-2.0)

# Gdk

pkg_check_modules(PC_GDK2 gdk-x11-2.0)

if(NOT PC_GDK2_FOUND)
	set(_GTK2_found_all false)
endif()

find_library(GDK2_LIBRARY NAMES gdk-x11-2.0)

find_path(GDK2_INCLUDE_DIR gdkconfig.h
	HINTS ${PC_GDK2_INCLUDEDIR} ${PC_GDK2_INCLUDE_DIRS}
	PATH_SUFFIXES gtk-2.0/include)

# Gdk-Pixbuf

pkg_check_modules(PC_GDKPIXBUF gdk-pixbuf-2.0)

if(NOT PC_GDKPIXBUF_FOUND)
	set(_GTK2_found_all false)
endif()

find_path(GDKPIXBUF_INCLUDE_DIR gdk-pixbuf/gdk-pixbuf.h
	HINTS ${PC_GDKPIXBUF_INCLUDEDIR} ${PC_GDKPIXBUF_INCLUDE_DIRS}
	PATH_SUFFIXES gdk-pixbuf-2.0)

find_library(GDKPIXBUF_LIBRARY NAMES gdk_pixbuf-2.0
	HINTS ${PC_GDKPIXBUF_LIBDIR} ${PC_GDKPIXBUF_LIBRARY_DIRS})

# Glib

find_required_package(GLIB2)
if(NOT GLIB2_FOUND)
	set(_GTK2_found_all false)
endif()

# Pango

pkg_check_modules(PC_PANGO pango)

if(NOT PC_PANGO_FOUND)
	set(_GTK2_found_all false)
endif()

find_path(PANGO_INCLUDE_DIR pango/pango.h
	HINTS ${PC_PANGO_INCLUDEDIR} ${PC_PANGO_INCLUDE_DIRS}
	PATH_SUFFIXES pango-1.0)

find_library(PANGO_LIBRARY NAMES pango-1.0
	HINTS ${PC_PANGO_LIBDIR} ${PC_PANGO_LIBRARY_DIRS})

# Cairo

set(CAIRO_DEFINITIONS ${PC_CAIRO_CXXFLAGS_OTHER})

find_path(CAIRO_INCLUDE_DIR cairo.h
	HINTS ${PC_CAIRO_INCLUDEDIR} ${PC_CAIRO_INCLUDE_DIRS}
	PATH_SUFFIXES cairo)

find_library(CAIRO_LIBRARY NAMES cairo
	HINTS ${PC_CAIRO_LIBDIR} ${PC_CAIRO_LIBRARY_DIRS})

# Atk

pkg_check_modules(PC_ATK atk)

if(NOT PC_ATK_FOUND)
	set(_GTK2_found_all false)
endif()

find_path(ATK_INCLUDE_DIR atk/atk.h
	HINTS ${PC_ATK_INCLUDEDIR} ${PC_ATK_INCLUDE_DIRS}
	PATH_SUFFIXES atk-1.0)

find_library(ATK_LIBRARY NAMES atk-1.0
	HINTS ${PC_ATK_LIBDIR} ${PC_ATK_LIBRARY_DIRS})

# Finalize

if(_GTK2_found_all)
	include(FindPackageHandleStandardArgs)

	find_package_handle_standard_args(GTK2 DEFAULT_MSG GTK2_LIBRARY GTK2_INCLUDE_DIR)

	set(GTK2_LIBRARIES ${GTK2_LIBRARY} ${GDK2_LIBRARY} ${GLIB2_LIBRARIES} ${PANGO_LIBRARY} ${CAIRO_LIBRARY} ${GDKPIXBUF_LIBRARY} ${ATK_LIBRARY})
	set(GTK2_INCLUDE_DIRS ${GTK2_INCLUDE_DIR} ${GDK2_INCLUDE_DIR} ${GLIB2_INCLUDE_DIRS} ${PANGO_INCLUDE_DIR} ${CAIRO_INCLUDE_DIR} ${GDKPIXBUF_INCLUDE_DIR} ${ATK_INCLUDE_DIR})

	mark_as_advanced(GTK2_INCLUDE_DIR GTK2_LIBRARY)

	set(GTK2_FOUND true)
else()
	unset(GTK2_LIBRARY)
	unset(GTK2_INCLUDE_DIR)

	unset(GDK2_LIBRARY)
	unset(GDK2_INCLUDE_DIR)

	set(GTK2_FOUND false)
endif()
