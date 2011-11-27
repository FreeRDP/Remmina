# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2011 Marc-Andre Moreau
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

# Gtk

pkg_check_modules(PC_GTK3 REQUIRED gtk+-3.0)

find_path(GTK3_INCLUDE_DIR NAMES gtk/gtk.h
	PATH_SUFFIXES gtk-3.0)

find_library(GTK3_LIBRARY NAMES gtk-3)

# Gdk

find_library(GDK3_LIBRARY NAMES gdk-3)

# Gdk-Pixbuf

pkg_check_modules(PC_GDKPIXBUF REQUIRED gdk-pixbuf-2.0)

find_path(GDKPIXBUF_INCLUDE_DIR gdk-pixbuf/gdk-pixbuf.h
	HINTS ${PC_GDKPIXBUF_INCLUDEDIR} ${PC_GDKPIXBUF_INCLUDE_DIRS}
	PATH_SUFFIXES gdk-pixbuf-2.0)

find_library(GDKPIXBUF_LIBRARY NAMES gdk-3
	HINTS ${PC_GDKPIXBUF_LIBDIR} ${PC_GDKPIXBUF_LIBRARY_DIRS})

# Glib

pkg_check_modules(PC_GLIB2 REQUIRED glib-2.0)

find_path(GLIB2_INCLUDE_DIR_PART1 NAMES glib.h
	HINTS ${PC_GLIB2_INCLUDEDIR} ${PC_GLIB2_INCLUDE_DIRS}
	PATH_SUFFIXES glib-2.0)

find_path(GLIB2_INCLUDE_DIR_PART2 NAMES glibconfig.h
	HINTS ${PC_GLIB2_INCLUDEDIR} ${PC_GLIB2_INCLUDE_DIRS}
	PATH_SUFFIXES glib-2.0/include)

set(GLIB2_INCLUDE_DIR ${GLIB2_INCLUDE_DIR_PART1} ${GLIB2_INCLUDE_DIR_PART2})

find_library(GLIB2_LIBRARY NAMES glib-2.0)

# Pango

pkg_check_modules(PC_PANGO REQUIRED pango)

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

pkg_check_modules(PC_ATK REQUIRED atk)

find_path(ATK_INCLUDE_DIR atk/atk.h
	HINTS ${PC_ATK_INCLUDEDIR} ${PC_ATK_INCLUDE_DIRS}
	PATH_SUFFIXES atk-1.0)

find_library(ATK_LIBRARY NAMES atk-1.0
	HINTS ${PC_ATK_LIBDIR} ${PC_ATK_LIBRARY_DIRS})

# Finalize

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(GTK3 DEFAULT_MSG GTK3_LIBRARY GTK3_INCLUDE_DIR)

set(GTK3_LIBRARIES ${GTK3_LIBRARY} ${GDK3_LIBRARY} ${GLIB2_LIBRARY} ${PANGO_LIBRARY} ${CAIRO_LIBRARY} ${GDKPIXBUF_LIBRARY} ${ATK_LIBRARY})
set(GTK3_INCLUDE_DIRS ${GTK3_INCLUDE_DIR} ${GLIB2_INCLUDE_DIR} ${PANGO_INCLUDE_DIR} ${CAIRO_INCLUDE_DIR} ${GDKPIXBUF_INCLUDE_DIR} ${ATK_INCLUDE_DIR})

mark_as_advanced(GTK3_INCLUDE_DIR GTK3_LIBRARY)


