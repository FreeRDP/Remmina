# Remmina - The GTK+ Remote Desktop Client
#
# Copyright (C) 2012 Jean-Louis Dupond
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

if((CMAKE_SYSTEM_PROCESSOR MATCHES "i386|i686|x86|AMD64") AND (CMAKE_SIZEOF_VOID_P EQUAL 4))
	set(TARGET_ARCH "x86")
elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64") AND (CMAKE_SIZEOF_VOID_P EQUAL 8))
	set(TARGET_ARCH "x64")
elseif((CMAKE_SYSTEM_PROCESSOR MATCHES "i386") AND (CMAKE_SIZEOF_VOID_P EQUAL 8) AND (APPLE))
	# Mac is weird like that.
	set(TARGET_ARCH "x64")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm*")
	set(TARGET_ARCH "ARM")
elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "sparc")
	set(TARGET_ARCH "sparc")
endif()

option(WITH_TRANSLATIONS "Generate translations." ON)
