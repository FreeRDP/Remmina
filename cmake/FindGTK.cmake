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
# Foundation, Inc., 51 Franklin Street, Fifth Floor, 
# Boston, MA  02110-1301, USA.

if(GTK_VERSION)
	find_required_package(GTK${GTK_VERSION})
	if(GTK${GTK_VERSION}_FOUND)
		set(GTK_FOUND true)
	else()
		set(GTK_FOUND false)
	endif()
else()
	# Prefer GTK+ 3 over GTK+ 2

	find_package(GTK3 QUIET)
	if(GTK3_FOUND)
		set(GTK_VERSION 3)
		set(GTK_FOUND true)
	else()
		find_package(GTK2 QUIET)
		if(GTK2_FOUND)
			set(GTK_VERSION 2)
			set(GTK_FOUND true)
		elseif()
			set(GTK_FOUND false)
		endif()
	endif()
endif()

if(GTK_VERSION)
	set(_GTK_ERR_MESSAGE "GTK ${GTK_VERSION} not found.")
else()
	set(_GTK_ERR_MESSAGE "No GTK not found.")
endif()

if (GTK_FOUND)
	set(GTK_LIBRARIES ${GTK${GTK_VERSION}_LIBRARIES})
	unset(GTK${GTK_VERSION}_LIBRARIES)

	set(GTK_INCLUDE_DIRS ${GTK${GTK_VERSION}_INCLUDE_DIRS})
	unset(GTK${GTK_VERSION}_INCLUDE_DIRS)
else()
	if(GTK_FIND_REQUIRED)
		message(FATAL_ERROR ${_GTK_ERR_MESSAGE})
	else()
		if(NOT VTK_FIND_QUIETLY)
			message(STATUS ${_GTK_ERR_MESSAGE})
		endif()
	endif()
endif()
